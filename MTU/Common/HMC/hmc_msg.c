/* ************************************************************************* */
/******************************************************************************
 *
 * Filename:   hmc_msg.c
 *
 * Global Designator: HMC_MSG_
 *
 * Contents: Contains the meter communication packet control to the uart interface
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2020 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ******************************************************************************
 *
 * $Log$ KAD Created 071907
 *
 ******************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 * v0.2 - Updated the toggle bit to verify only the RX toggle bit and not our
 *        transmitted bit.
 * v0.3 - Updated for Meter Comm Error count Engineering registers
 * v0.4 - Fixed bug that caused an infinite loop while receiving a packet that was longer than allowed.
 *        Missing ACK will now cause an error even if a response packet is received.
 * v0.5 - Fixed a casting issue when calling CRC16_InitMtr().
 * v0.6 - 04/03/2012 - KAD - Changed 'HMC_ENG_Execute' function.  The 'pData' pointer is passed as the 2nd parameter.
 *************************************************************************** */
/* ************************************************************************* */
/* INCLUDE FILES */
#include "project.h"
#include <stdbool.h>
#define HMC_MSG_GLOBAL
#include "hmc_msg.h"
#undef HMC_MSG_GLOBAL

#include "crc16_m.h"
#include "hmc_eng.h"
#include "hmc_start.h"
#include "byteswap.h"
#include "timer_util.h"

/*lint -esym(750,HMC_MSG_PRNT_INFO,HMC_MSG_PRNT_WARN,HMC_MSG_PRNT_ERROR) */
#if ( ENABLE_PRNT_HMC_MSG_INFO || ENABLE_PRNT_HMC_MSG_WARN || ENABLE_PRNT_HMC_MSG_ERROR )
#include "DBG_SerialDebug.h"   /* for printf, debug only  */
#endif


/* ************************************************************************* */
/* CONSTANTS */

#define TOGGLE_BIT_INITIAL_SESSION ((int8_t)-1)

//lint -esym(749,HMC_MSG_MODE::HMC_MSG_MODE_TIME_OUT) Included for completeness
enum HMC_MSG_MODE
{
   HMC_MSG_MODE_IDLE = 0,  /* Idle mode, nothing is happening in this layer */
   HMC_MSG_MODE_RX,        /* In RX mode, waiting for bytes to make complete message */
   HMC_MSG_MODE_TX,        /* In TX mode, sending bytes to the host meter */
   HMC_MSG_MODE_TIME_OUT,  /* Timed out - Not used in this version */
   HMC_MSG_MODE_INVALID    /* Invalid */
};

/* ************************************************************************* */
/* MACRO DEFINITIONS */

#if ENABLE_PRNT_HMC_MSG_INFO
#define HMC_MSG_PRNT_INFO( a, fmt,... )  DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define HMC_MSG_PRNT_HEX_INFO( a, fmt,... )   DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_MSG_PRNT_INFO( a, fmt,... )
#define HMC_MSG_PRNT_HEX_INFO( a, fmt,... )
#endif
#if ENABLE_PRNT_HMC_MSG_WARN
#define HMC_MSG_PRNT_WARN( a, fmt,... )  DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_MSG_PRNT_WARN( a, fmt,... )
#endif
#if ENABLE_PRNT_HMC_MSG_ERROR
#define HMC_MSG_PRNT_ERROR( a, fmt,... ) DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_MSG_PRNT_ERROR( a, fmt,... )
#endif

/* ************************************************************************* */
/* TYPE DEFINITIONS */

/* ************************************************************************* */
/* FUNCTION PROTOTYPES */

/* ************************************************************************* */
/* FILE VARIABLE DEFINITIONS */

static uint16_t resExpiredTimerId_ = 0;
static uint16_t taDelayExpiredTimerId_ = 0;
static uint16_t comTimerExpiredTimerId_ = 0;

/* ************************************************************************* */
/* FUNCTION DEFINITIONS */

/* ************************************************************************* */
/******************************************************************************
 *
 * Function name: HMC_MSG_Processor()
 *
 * Purpose: Manages the packets to and from the host meter including retries
 *          if the meter doesn't respond.
 *
 * Arguments: uint8_t ucCommand, HMC_COM_INFO * pPointer to the communications structures
 *
 * Returns: uint16_t - STATUS
 *
 * NOTE:  Only can support 255 bytes of read/write data!
 *
 *****************************************************************************/
uint16_t HMC_MSG_Processor(uint8_t ucCommand, HMC_COM_INFO *pData)
{
   static enum    HMC_MSG_MODE eMode_ = HMC_MSG_MODE_IDLE;  /* Idle, RX Mode, TX Mode, Time Out */
   static HMC_MSG_Configuration comSettings_;/* Comm protocol params used when communication with host meter. */
   static HMC_MSG_STATUS sStatus_;           /* Bit Structure for status */
   static bool expectingACK_ = false;     /* If true, we are expecting an ack (or nak) to the packet we just sent. */
   static bool bDoData_;                  /* Indicator if the data field is being received or transmitted */
   static uint8_t   retries_;                  /* Tx Packet Retry Count */
   static uint8_t   sendAckNak_ = 0;           /* Sends the Ack or NAK to the host meter when a non-zero value */
   static uint8_t   *pTxRx_;                   /* Pointer to the data */
   static int8_t    rxToggleBitExpected_;      /* Expected return tgl bit, -1 not defined, 0 or 1 are expected values */
   static int8_t    rxToggleBitActual_;        /* Contains the received toggle bit */
          uint8_t   ucResult = 0x00;           /* Used to store the received byte or result of the UART */
          timer_t timerCfg;                  /* Used to configure the timers and read the status of a timer. */

   sStatus_.Bits.bCPUTime = false;           /* No CPU Time taken yet! */

   if (HMC_MSG_MODE_INVALID <= eMode_)
   {
      sStatus_.uiStatus = 0;
      sStatus_.Bits.bInitialized = true;
      eMode_ = HMC_MSG_MODE_IDLE;
   }

   switch(ucCommand)
   {
      // <editor-fold defaultstate="collapsed" desc="case HMC_MSG_CMD_INIT">
      case (uint8_t)HMC_MSG_CMD_INIT:
      {
         /* This initialize command will initialize all static variables, and put the module into an idle state. */
         sStatus_.uiStatus = 0;
         sStatus_.Bits.bInitialized = true;
         eMode_ = HMC_MSG_MODE_IDLE;

         (void)memset(&timerCfg, 0, sizeof(timerCfg));  /* Clear the timer values */
         timerCfg.bOneShot = true;
         timerCfg.bExpired = true;
         timerCfg.bStopped = true;
         timerCfg.ulDuration_mS = 1;

         timerCfg.usiTimerId = resExpiredTimerId_;
         (void)TMR_AddTimer(&timerCfg);
         resExpiredTimerId_ = timerCfg.usiTimerId;

         timerCfg.usiTimerId = taDelayExpiredTimerId_;
         (void)TMR_AddTimer(&timerCfg);
         taDelayExpiredTimerId_ = timerCfg.usiTimerId;

         timerCfg.usiTimerId = comTimerExpiredTimerId_;
         (void)TMR_AddTimer(&timerCfg);
         comTimerExpiredTimerId_ = timerCfg.usiTimerId;
         /* Fall through to set the default values */
      }
      //lint -fallthrough
      // </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="case HMC_MSG_CMD_SET_DEFAULTS">
      case (uint8_t)HMC_MSG_CMD_SET_DEFAULTS:  /* Sets all of the communication defaults */
      {
#ifndef TEST_COM_UPDATE_APPLET
         // Actual defaults
         comSettings_.ucRetries             = HMC_MSG_RETRIES;
         comSettings_.uiResponseTimeOut_mS  = HMC_MSG_RESPONSE_TIME_OUT_mS;
         comSettings_.ucErrorTimeOut_mS     = HMC_MSG_ERROR_TIME_OUT_mS;
         comSettings_.ucTurnAroundDelay     = HMC_MSG_TURN_AROUND_TIME_mS;
         comSettings_.uiMeterBusyDelay      = HMC_MSG_BUSY_DELAY_mS;
         comSettings_.uiInterCharDelay      = HMC_MSG_ICHAR_TIME_OUT_mS;
         comSettings_.uiTrafficTimeOut_mS   = HMC_MSG_TRAFFIC_TIME_OUT_mS;
#else
         // test defaults to allow seeing that the values change when we read them from the meter.
         comSettings_.ucRetries             = HMC_MSG_RETRIES;
         comSettings_.uiResponseTimeOut_mS  = HMC_MSG_RESPONSE_TIME_OUT_mS + 11;
         comSettings_.ucErrorTimeOut_mS     = HMC_MSG_ERROR_TIME_OUT_mS + 22;
         comSettings_.ucTurnAroundDelay     = HMC_MSG_TURN_AROUND_TIME_mS + 3;
         comSettings_.uiMeterBusyDelay      = HMC_MSG_BUSY_DELAY_mS + 31;
         comSettings_.uiInterCharDelay      = HMC_MSG_ICHAR_TIME_OUT_mS + 41;
         comSettings_.uiTrafficTimeOut_mS   = HMC_MSG_TRAFFIC_TIME_OUT_mS + 57;
#endif

         // Call the "meter start" applet with a pointer to the comm params, so it can update its internal
         // settings.
         (void)HMC_STRT_LogOn( (uint8_t)HMC_APP_API_CMD_CUSTOM_UPDATE_COM_PARAMS, &comSettings_ );
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="case HMC_MSG_CMD_NEW_SESSION">
      case (uint8_t)HMC_MSG_CMD_NEW_SESSION: /* Sets the toggle bit up for the 1st communication */
      {
         rxToggleBitExpected_ = TOGGLE_BIT_INITIAL_SESSION;
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="case HMC_MSG_CMD_WR_COMPORT">
      case (uint8_t)HMC_MSG_CMD_WR_COMPORT: /* Open the comport with the new settings */
      {
         //xxx         (void)DVR_UART_App(UART_HOST_COMM_PORT, PORT_OPEN, (void *)pData);
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="case HMC_MSG_CMD_RD_COMPORT">
      case (uint8_t)HMC_MSG_CMD_RD_COMPORT: /* Reads the comport settings */
      {
         //xxx         (void)DVR_UART_App(UART_HOST_COMM_PORT, PORT_RD_CFG, pData);
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="case HMC_MSG_CMD_CLR_FLAGS">
      case (uint8_t)HMC_MSG_CMD_CLR_FLAGS: /* Clear the status flags (except IDLE) and set mode to IDLE */
      {
         sStatus_.uiStatus = 0;
         eMode_ = HMC_MSG_MODE_IDLE;

         (void)TMR_StopTimer(resExpiredTimerId_);
         (void)TMR_StopTimer(comTimerExpiredTimerId_);

         timerCfg.usiTimerId = comTimerExpiredTimerId_;
         (void)TMR_ReadTimer(&timerCfg);
         if ( !timerCfg.bExpired &&!timerCfg.bStopped ) /* Did a Time-out occur and timer is running? */
         {
            sStatus_.Bits.bTxWait = true;       /* Set the TxWait Flag, because of the TA Delay */
         }
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="case HMC_MSG_CMD_RD_COM">
      case (uint8_t)HMC_MSG_CMD_RD_COM:  /* Returns a pointer to the configuration */
      {
         (void)memcpy(( uint8_t * )pData, ( uint8_t * )&comSettings_, sizeof(comSettings_));
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="case HMC_MSG_CMD_WR_COM">
      case (uint8_t)HMC_MSG_CMD_WR_COM:  /* Returns a pointer to the configuration */
      {
         (void)memcpy(( uint8_t * )&comSettings_, ( uint8_t * )pData, sizeof(comSettings_));

         // Call the "meter start" applet with a pointer to the comm params, so it can update its internal
         // settings.
         (void)HMC_STRT_LogOn( (uint8_t)HMC_APP_API_CMD_CUSTOM_UPDATE_COM_PARAMS, &comSettings_ );
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="case HMC_MSG_CMD_SEND_DATA">
      case (uint8_t)HMC_MSG_CMD_SEND_DATA:  /* Send the next packet */
      {
         HMC_ENG_Execute((uint8_t)HMC_ENG_TOTAL_PACKETS, pData);
         retries_ = comSettings_.ucRetries;                          /* Reset Retries */
         sStatus_.Bits.bBusy = true;                                 /* Set status to Busy */
         eMode_ = HMC_MSG_MODE_TX;                                   /* Now in TX mode */
         bDoData_ = false;
         pTxRx_ = &pData->TxPacket.ucSTP;                            /* Set up the pointer */
#if 0 // TODO: RA6E1 Implement UART flush command
         (void)UART_flush(UART_HOST_COMM_PORT);
#endif
         HMC_MSG_PRNT_HEX_INFO( 'H', "Send:", pTxRx_, offsetof(sMtrTxPacket,uTxData) +
                           BIG_ENDIAN_16BIT( pData->TxPacket.uiPacketLength.n16 ) );
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="case HMC_MSG_CMD_PROCESS">
      case (uint8_t)HMC_MSG_CMD_PROCESS:
      {
         timerCfg.usiTimerId = taDelayExpiredTimerId_;
         (void)TMR_ReadTimer(&timerCfg);

         if ( timerCfg.bExpired || timerCfg.bStopped )                 /* Check if "Turn Around" delay is in progress */
         {
            if ( sendAckNak_ )                                         /* Need to send an Ack or Nak? */
            {
              (void)TMR_ResetTimer(taDelayExpiredTimerId_, comSettings_.ucTurnAroundDelay); /* Start the TA Delay */
               if ( 1 == UART_write(UART_HOST_COMM_PORT, &sendAckNak_, sizeof(sendAckNak_)) ) /* Send ACK or NAK */
               {
                  /* No UART error, so continue */
                  sendAckNak_ = 0;
               }
            }
            /* ****************************************************************************************************** */
            // <editor-fold defaultstate="collapsed" desc="RX Mode">
            /* RX MODE */
            else  if ( HMC_MSG_MODE_RX == eMode_ )                      /* in Receive Mode? */
            {
               uint16_t numRxBytes;

               timerCfg.usiTimerId = comTimerExpiredTimerId_;  /* Read the communication (session) time out timer */
               (void)TMR_ReadTimer(&timerCfg);
               if ( timerCfg.bExpired )   /* Communication session Time out? */
               {
                  HMC_ENG_Execute((uint8_t)HMC_ENG_COM_TIME_OUT, HMC_ENG_VARDATA);
                  sStatus_.Bits.bTimeOut = true; /* Yes, set return value to Timeout */
                  eMode_ = HMC_MSG_MODE_IDLE;
               }
               else
               {
                  sStatus_.Bits.bTxWait = true;
                  numRxBytes = UART_read(UART_HOST_COMM_PORT, &ucResult, sizeof(ucResult));
                  timerCfg.usiTimerId = resExpiredTimerId_;
                  (void)TMR_ReadTimer(&timerCfg);
                  if ( (timerCfg.bExpired) && (0 == numRxBytes) ) /* Did a Time-out occur &no data received? */
                  {
                     sStatus_.Bits.bTxWait = false; /* Clear the TX Wait Flag */
                     if ( retries_ != 0 ) /* Any retries left? */
                     {
                        /* Yes, there are retries left */
                        retries_--;
                        HMC_ENG_Execute((uint8_t)HMC_ENG_RETRY, pData);   /* Increment Engineering Registers */
                        if ( pTxRx_ != &pData->RxPacket.ucSTP )                    /* Did we receive any bytes at all? */
                        {
                           /* Bytes received, so NAK the host meter because the message is not complete */
                           sendAckNak_ = PSEM_NAK;                             /* Send NAK to Host Meter */
                           bDoData_ = false;  /* Doing Protocol when we get to RX mode */
                           HMC_ENG_Execute((uint8_t)HMC_ENG_INTER_CHAR_T_OUT, pData);
                        }
                        else
                        {
                           /* No response from the host meter, try sending the same data again, per ANSI C12 */
                           eMode_ = HMC_MSG_MODE_TX;  /* Get ready to TX data again */
                        }
                        pTxRx_ = &pData->TxPacket.ucSTP;  /* Reset the pointer */
                        (void)TMR_ResetTimer( resExpiredTimerId_, comSettings_.uiResponseTimeOut_mS );
                     }
                     else
                     {
                        /* No retries left, need to return with error */
                        eMode_ = HMC_MSG_MODE_IDLE;
                        sStatus_.Bits.bTimeOut = true; /* Yes, set return value to Timeout */
                        HMC_ENG_Execute((uint8_t)HMC_ENG_TIME_OUT, pData);
                     }
                  }
                  else /* See if we have data to process */
                  {
                     while ( (0 != numRxBytes) && (HMC_MSG_MODE_RX == eMode_) ) /* Buffer Not Empty? */
                     {
                        if ( expectingACK_ )
                        {
                           // This should be an ACK (or a NAK of course).
                           if ( PSEM_ACK == ucResult ) /* if ACK in buffer, Still doing protocol */
                           {
                              (void)TMR_StopTimer(resExpiredTimerId_); /* we received the ACK, stop the response timeout timer */
                              (void)TMR_ResetTimer(comTimerExpiredTimerId_, comSettings_.uiTrafficTimeOut_mS); /* reset the session timer */
                              expectingACK_ = false;
                              bDoData_ = false;
                           }
                           else if ( PSEM_NAK == ucResult )
                           {
                              expectingACK_ = false;

                              // Either an NAK or something else.  It's not an ACK which is all we really need to know.
                              /* Must wait before starting com again */
                              (void)TMR_ResetTimer(taDelayExpiredTimerId_, comSettings_.ucTurnAroundDelay);

                              /* We received a NAK.  We need resend the request to the meter. If there are retries left,
                                 the endpoint will switch to tx mode and the response timeout will be reset once the new
                                 request is sent to the meter. Since a NAK is a valid response and not gargabe data,
                                 reset the communication session timout. */
                              (void)TMR_StopTimer(resExpiredTimerId_);
                              (void)TMR_ResetTimer(comTimerExpiredTimerId_, comSettings_.uiTrafficTimeOut_mS);

                              HMC_ENG_Execute((uint8_t)HMC_ENG_NAK, pData);
                              if ( retries_ ) /* Any retries left? */
                              {
                                 retries_--;   /* Yes, there are retries, decrement it */
                                 eMode_ = HMC_MSG_MODE_TX;  /* Get ready to TX data again */
                                 pTxRx_ = &pData->TxPacket.ucSTP;  /* Reset the pointer */
                                 HMC_ENG_Execute((uint8_t)HMC_ENG_RETRY, pData);
                              }
                              else
                              {
                                 sStatus_.Bits.bNAK = true;     /* No retries left, need to return with error */
                                 eMode_ = HMC_MSG_MODE_IDLE;   /* Set mode to IDLE, nothing left to do */
                              }
                           }
                           else  /* It was neither a ACK or NAK, so it may have been garbage in the buffer. */
                           {
                              break;
                           }
                        }
                        else
                        {
                           /* Reset Inter Character Delay */
                           (void)TMR_ResetTimer( resExpiredTimerId_, comSettings_.uiInterCharDelay );
                           (void)TMR_ResetTimer(comTimerExpiredTimerId_, comSettings_.uiTrafficTimeOut_mS); /* reset the session timer */
                           // We are receiving a response packet, having already gotten our ACK.

                           /* If buffer is full, collect byte */
                           if ( pTxRx_ <= (uint8_t *) &pData->RxPacket.uiPacketCRC16.BE.Lsb )
                           {
                              /* Process Rx Data */
                              /* The first byte of data needs to be analyzed differently than the rest of the data
                                 received.  The buffer needs to be checked for an ACK or NAK. */
                              /* Is this the 1st byte? */
                              if ( (pTxRx_ == &pData->RxPacket.ucSTP) && (ucResult != PSEM_STP) )
                              {
                                 // First byte of packet, and not an STP, that's not right.
                                 sStatus_.Bits.bComError = true;  /* Return with error */
                                 eMode_ = HMC_MSG_MODE_IDLE;   /* Set mode to IDLE, nothing left to do */
                              }
                              else
                                 /* Should be an STP or not the first byte received. ACK/NAK taken care of above. */
                              {
                                 *pTxRx_ = ucResult; /* Put the data in the RxBuffer or the Data section */
                                 sStatus_.Bits.bCPUTime = true; /* CRC16 calc performed, CPU Time Flag is now set */
                                 if ( (pTxRx_ == &pData->RxPacket.ucId) && (PSEM_STP == pData->RxPacket.ucSTP) ) /* Is pointer at location ID? */
                                 {
                                    /* This CRC initializes with first two bytes of protocol collected, STP and ID */
                                    CRC16_InitMtr((uint8_t *)&pData->RxPacket); /* Init CRC16 calc */
                                 }// Range to perform CRC
//                                 else if ( (pTxRx_ > &pData->RxPacket.ucId) && (pTxRx_ < (uint8_t *)&pData->RxPacket.uiPacketCRC16.n16) )
                                 else // if ( (pTxRx_ > &pData->RxPacket.ucId) && (pTxRx_ < (uint8_t *)&pData->RxPacket.ucResponseCode) )
                                 {
                                    CRC16_CalcMtr(ucResult);   /* Update CRC16 */
                                 }
                                 pTxRx_++;  /* Increment the data pointer */
                                 if ( !bDoData_ )   /* Is the length response length known yet? */
                                 {
                                    /* Pointer at data yet? */
                                    if ( pTxRx_ == &pData->RxPacket.ucResponseCode )
                                    {
                                       bDoData_ = true;                     /* Yes, now doing data */
                                       /* Byte swap Packet Length to Little Endian */
#if ( PROCESSOR_LITTLE_ENDIAN )
                                       Byte_Swap((uint8_t *) &pData->RxPacket.uiPacketLength,
                                                 (int8_t)sizeof(pData->RxPacket.uiPacketLength));
#endif
                                       HMC_MSG_PRNT_INFO( 'H', "Rx len: %hu", pData->RxPacket.uiPacketLength );
                                    }
                                       /* All done with the message? */
                                       /* pointer at CRC location? */
                                    else if ( pTxRx_ > &pData->RxPacket.uiPacketCRC16.BE.Lsb )
                                    {
                                       /* Message complete, start turn-around delay */
//                                       (void)TMR_ResetTimer(taDelayExpiredTimerId_, comSettings_.ucTurnAroundDelay);
                                       (void)TMR_ResetTimer(taDelayExpiredTimerId_, comSettings_.ucTurnAroundDelay);

                                       if ( 0 == CRC16_ResultMtr() )   /* CRC16 passed? */
                                       {
                                          HMC_MSG_PRNT_HEX_INFO( 'H', "Rec :", (uint8_t *)&pData->RxPacket,
                                                            offsetof(sMtrRxPacket,uRxData) + pData->RxPacket.uiPacketLength.n16 );
#if 0
                                          /* mkv debug reading table 12 */
                                          if ( pData->TxPacket.uTxData.sReadPartial.uiTbleID == 0x0c00 )
                                          {
                                             HMC_MSG_PRNT_INFO( 'H', "Tbl 12 offset %02x: %02x %02x %02x %02x",
                                                pData->TxPacket.uTxData.sReadPartial.ucOffset[2],
                                                pData->RxPacket.uRxData.sTblData.sRxBuffer[0],
                                                pData->RxPacket.uRxData.sTblData.sRxBuffer[1],
                                                pData->RxPacket.uRxData.sTblData.sRxBuffer[2],
                                                pData->RxPacket.uRxData.sTblData.sRxBuffer[3] );
                                          }
#endif
                                          rxToggleBitActual_ = (pData->RxPacket.ucControl &
                                                                PSEM_CTRL_TOGGLE) ? true : false;
                                          sendAckNak_ = PSEM_ACK;   /* Send ACK to Host Meter */
                                          (void)TMR_StopTimer( resExpiredTimerId_);
#if HMC_MSG_IGNORE_TOGGLE_BIT
                                          sStatus_.Bits.bComplete = true;
                                          eMode_ = HMC_MSG_MODE_IDLE;   /* Back to Idle mode */

                                          /* Don't abort the session, just log it */
                                          if ( (TOGGLE_BIT_INITIAL_SESSION != rxToggleBitExpected_ ) &&
                                               (rxToggleBitExpected_ != rxToggleBitActual_) )
                                          {
                                             HMC_ENG_Execute((uint8_t)HMC_ENG_TOGGLE_ERROR, pData);
                                          }
#else
                                          if ( (TOGGLE_BIT_INITIAL_SESSION == rxToggleBitExpected_ ) ||
                                               (rxToggleBitExpected_ == rxToggleBitActual_) )
                                          {
                                             sStatus_.Bits.bComplete = true;
                                             eMode_ = HMC_MSG_MODE_IDLE;   /* Back to Idle mode */
                                          }
                                          else
                                          {
                                             /* ACK is sent to host meter to shut it up, but we'll start the
                                                protocol over again */
                                             sStatus_.Bits.bToggleBitError = true;
                                             eMode_ = HMC_MSG_MODE_IDLE;   /* Back to Idle mode */
                                             HMC_ENG_Execute((uint8_t)HMC_ENG_TOGGLE_ERROR, pData);
                                          }
#endif
                                          /* Set the Expected value up for the next communication session */
                                          rxToggleBitExpected_ = (rxToggleBitActual_) ? false : true;
                                       }
                                       else
                                       {
                                          sendAckNak_ = PSEM_NAK;  /* CRC16 failed, send NAK */
#if HMC_MSG_CLEAR_RX_BUFFER
                                          (void)memset(&pData->RxPacket.uRxData, 0,
                                                       sizeof(pData->RxPacket.uRxData));
#endif
                                          /* Prepare to receive message again from host */
                                          pTxRx_ = &pData->RxPacket.ucSTP;
                                          (void)TMR_ResetTimer(resExpiredTimerId_, comSettings_.uiResponseTimeOut_mS);
                                          HMC_ENG_Execute((uint8_t)HMC_ENG_CRC16, pData);
                                       }
                                    }
                                 }
                                 else
                                 {
                                    /* Is a table being read? */
                                    if ( ( RESP_TYPE_TABLE_DATA == pData->RxPacket.ucRespType ) &&
                                         (pData->RxPacket.uiPacketLength.n16 > 1) )
                                    {
                                       /* A table is being read. Format the data so that the Applet can use this data
                                          easily. If the Rx pointer is located on the RxBuffer, then the byte found
                                          has been gathered for the data.  Byte swap the byte count to make it little
                                          endian
                                        */
                                       if ( pTxRx_ == (uint8_t *)pData->RxPacket.uRxData.sTblData.sRxBuffer )
                                       {
                                          /* Swap Table Length to Little Endian */
#if ( PROCESSOR_LITTLE_ENDIAN )
                                          Byte_Swap((uint8_t *) &pData->RxPacket.uRxData.sTblData.uiCount,
                                                    (int8_t)sizeof(pData->RxPacket.uRxData.sTblData.uiCount));
#endif
                                          if ( 0 == pData->RxPacket.uRxData.sTblData.uiCount ) /* No bytes to recieve */
                                          {  /* Get the checksum. */
                                             pTxRx_ = &pData->RxPacket.uRxData.sTblData.ucChecksum;
                                             bDoData_ = false;  /* Done with RAW data, get the checksum and CRC */
                                          }
                                       }
                                       else if ( (pTxRx_ ==
                                                  ((uint8_t *)&pData->RxPacket.ucResponseCode) +
                                                  (pData->RxPacket.uiPacketLength.n16 - 1) ) )
                                       {
                                          /* Since we are done with the data portion, the next byte will be the data
                                             checksum.  Put this data in the checksum location.
                                           */
                                          /* set pointer at checksum */
                                          pTxRx_ = &pData->RxPacket.uRxData.sTblData.ucChecksum;
                                          bDoData_ = false;  /* Done with RAW data, get the checksum and CRC */
                                       }
                                    }
                                       /* Done with the data portion of the code? */
                                    else if ( (pTxRx_ ==
                                               ((uint8_t *)&pData->RxPacket.ucResponseCode) +
                                               pData->RxPacket.uiPacketLength.n16) )
                                    {
                                       /* Response isn't data, need to keep collecting to get the CRC */
                                       /* set pointer at checksum */
                                       pTxRx_ = (uint8_t *)&pData->RxPacket.uiPacketCRC16;
                                       bDoData_ = false;  /* Not data yet */
                                    }
                                 }
                              }
                           } /*  if (RX Buffer Full) */
                           else
                           {
                              /* Buffer is full; wait for host to stop sending data or a communication time out. */
                              if ( !sStatus_.Bits.bDataOverFlow ) /* Only update Eng registers once */
                              {
                                 HMC_ENG_Execute((uint8_t)HMC_ENG_OVERRUN, pData);
                                 sStatus_.Bits.bDataOverFlow = true;  /* Set the over flow flag */
                              }
                              break;  /* get out of the while so that the main can process data */
                           }
                        } // receiving a response packet
                        numRxBytes = UART_read(UART_HOST_COMM_PORT, &ucResult, sizeof(ucResult));
                        /* If no bytes were received, and we're still in RX mode, pause the task for 1mS.  Note: If the
                         * RTOS tick time is greater than 1mS, the delay here may take longer which may slow down comm.
                         * a little! */
                        if ((0 == numRxBytes) && (HMC_MSG_MODE_RX == eMode_))
                        {
                           OS_TASK_Sleep ( 1 );
                        }
                     } /* while (!DVR_UART_App(UART_HOST_COMM_PORT, PORT_RX_EMPTY, NULL) &UART_STATUS_RC_NOT_EMPTY) */
                     if (0 == numRxBytes)
                     {
                        OS_TASK_Sleep(1);
                     }
                  }
               }
            } /* end if (eMode_ == HMC_MSG_MODE_RX) */
            // </editor-fold>
            /* ****************************************************************************************************** */
            // <editor-fold defaultstate="collapsed" desc="TX Mode">
            /* TX MODE */
            else if ( HMC_MSG_MODE_TX == eMode_ ) /* In TX mode? */
            {
               sStatus_.Bits.bBusy = true; /* Transmitting, so the status is busy */
               do
               {
                  /* Transmit next byte, store the result in ucResult */
                  ucResult = (uint8_t)UART_write(UART_HOST_COMM_PORT, pTxRx_, 1);  /* Send 1 byte */
                  if ( 1 == ucResult )  /* Was the transmit successful?  1 byte transmitted (ucResult == 1) */
                  {  /* Successful */
                     sStatus_.Bits.bCPUTime = true;
                     if ( pTxRx_ == &pData->TxPacket.ucId )    /* Is the pointer at ID? */
                     {
                        CRC16_InitMtr((uint8_t *) &pData->TxPacket); /* Init CRC16 Tx'ing 2nd byte */
                     }
                     else
                     {
                        CRC16_CalcMtr(*pTxRx_);                /* Update the CRC16 with the byte sent */
                     }
                     pTxRx_++;                                 /* Increment the pointer */
                     if ( !bDoData_ )                          /* If DATA isn't being set, protocol is being sent */
                     {
                        if ( pTxRx_ == (uint8_t *) &pData->TxPacket.uTxData ) /* Pointing to data yet? */
                        {
                           bDoData_ = true;                   /* Yes, pointing to data */
                        }                                     /* Check if data string is completely sent */
                        /* Done Transmitting? */
                        else if ( pTxRx_ > (uint8_t *)&(pData->TxPacket.uiPacketCRC16.BE.Lsb) )
                        {
                           eMode_ = HMC_MSG_MODE_RX;          /* Switch mode to receive, host meter should respond */
#if HMC_MSG_CLEAR_RX_BUFFER
                           (void)memset(&pData->RxPacket.uRxData, 0,
                                        sizeof (pData->RxPacket.uRxData));
#endif
                           pTxRx_ = &pData->RxPacket.ucSTP; /* Set the receive pointer up */
                           bDoData_ = false;                /* Doing Protocol when we get to RX mode */
                           expectingACK_ = true;            // Next byte we receive should be the ack/nak to our packet.
                        }
                     }
                     else if ( (pTxRx_ - (uint8_t *) &pData->TxPacket.uTxData) ==
                               pData->TxPacket.uiPacketLength.BE.Lsb )
                     {
                        pTxRx_ = &pData->TxPacket.uiPacketCRC16.BE.Msb;  /* Set pointer to the CRC */
                        bDoData_ = false;  /* No longer doing data */
                        CRC16_CalcMtr(0); /* Per ANCI C12 */
                        CRC16_CalcMtr(0); /* Per ANCI C12 */
                        //lint -e{826} There is enough room to store a 16 bit value at this location
                        *(uint16_t *)pTxRx_ = CRC16_ResultMtr();  /* Put the CRC result into the TX buffer */
                     }
                  }
                  else
                  {
                     OS_TASK_Sleep( 1 );      /* The TX buffer is full, wait for buffer to free up */
                  }
               }while( eMode_ == HMC_MSG_MODE_TX );   /* While TX buffer not full and in TX Mode */
#if 0 // TODO: RA6E1 Implement UART Tx empty command
               while(!UART_isTxEmpty(UART_HOST_COMM_PORT))  /* Wait until TX is empty. */
#else
               while( 0 )
#endif
               {
                  /* The receive is called only to clear the glitch on the KV2 meter when starting a session! */
                  uint8_t rxGarbage;
                  while(0 != UART_read(UART_HOST_COMM_PORT, &rxGarbage, sizeof(rxGarbage)))
                  {}
                  OS_TASK_Sleep( 1 );   /* Wait a while for the TX buffer to empty before starting times. */
               }
               (void)TMR_ResetTimer(resExpiredTimerId_, comSettings_.uiResponseTimeOut_mS);
               (void)TMR_ResetTimer(comTimerExpiredTimerId_, comSettings_.uiTrafficTimeOut_mS);
            } /* end if (_eMode == HMC_MSG_MODE_TX) */// </editor-fold>
         }
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="default">
      default:
      {
         sStatus_.Bits.bInvalidCmd = true;
         break;
      }// </editor-fold>
   }
   /* If any status flags are set, then the msg processor is no longer busy. */
   if (sStatus_.sBytes.ucTransactionComplete && !sendAckNak_)
   {
      sStatus_.Bits.bBusy = false;
   }
   return(sStatus_.uiStatus);
}
/* ************************************************************************* */
