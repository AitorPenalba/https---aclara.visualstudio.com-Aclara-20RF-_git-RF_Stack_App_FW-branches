/***********************************************************************************************************************
 *
 * Filename:   ILC_DRU_DRIVER.c
 *
 * Global Designator: ILC_DRU_DRIVER_
 *
 * Contents: Contains the DRU communication packet control to the uart interface
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2017 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ KAD Created 080207
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 *
 **********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "ilc_dru_driver.h"
#include "ascii.h"
#include <assert.h>
#include "DBG_SerialDebug.h"
#include "pack.h"
#if( RTOS_SELECTION == FREE_RTOS )
#define ILC_DRIVER_QUEUE_SIZE 10 //NRJ: TODO Figure out sizing
#else
#define ILC_DRIVER_QUEUE_SIZE 0
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */
static const uint8_t ReadDruSerialNumberRequestPacket_au8 [] =
{
   '\1','3','E','0','1','0','0','0','0','0','0','0','0','0','0','0','1','0','1','8','4','C','5','\4'
/*
   0x01U, 0x33U, 0x45U, 0x30U, 0x31U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U,
   0x31U, 0x30U, 0x31U, 0x38U, 0x34U, 0x43U, 0x35U, 0x04U
*/
};


static const uint8_t ReadDruFwVersionRequestPacket_au8 [] =
{
   '\1','3','E','0','1','0','0','0','0','0','0','0','0','0','0','0','1','0','3','0','4','4','7','\4'
/*
   0x01U, 0x33U, 0x45U, 0x30U, 0x31U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U,
   0x31U, 0x30U, 0x33U, 0x30U, 0x34U, 0x34U, 0x37U, 0x04U
*/
};

static const uint8_t ReadDruHwVersionRequestPacket_au8 [] =
{
   '\1','3','E','0','1','0','0','0','0','0','0','0','0','0','0','0','1','0','3','2','2','6','5','\4'
/*
   0x01U, 0x33U, 0x45U, 0x30U, 0x31U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U,
   0x31U, 0x30U, 0x33U, 0x32U, 0x32U, 0x36U, 0x35U, 0x04U
*/
};

static const uint8_t ReadDruRceProductRequestPacket_au8 [] =
{
   '\1','3','E','0','1','0','0','0','0','0','0','0','0','0','0','0','1','0','3','3','2','7','5','\4'
/*
   0x01U, 0x33U, 0x45U, 0x30U, 0x31U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U,
   0x31U, 0x30U, 0x33U, 0x33U, 0x32U, 0x37U, 0x35U, 0x04U
*/
};


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define ROUNDUP_BYTE_TX_TIME_AT_2400_BPS_MS     ( (uint8_t) 4 )
#define TIME_OUT_PRINT_ENABLED                  true
#define TIME_OUT_PRINT_DISABLED                 false

#define REQUEST_TIME_OUT_FACTOR                 ( (uint8_t) 2 )
#define RESPONSE_TIME_OUT_FACTOR                ( (uint8_t) 4 )


/* The following time outs give more than "REQUEST_TIME_OUT_FACTOR" times the maximum expected request time because the
   UART_HOST_COMM_PORT UART might be busy transmitting something else */

#define REQUEST_TIME_OUT_mS(len)                ( (uint32_t) ( ROUNDUP_BYTE_TX_TIME_AT_2400_BPS_MS * \
                                                   ((len) * REQUEST_TIME_OUT_FACTOR ) ) )

/* The following time outs give more than "RESPONSE_TIME_OUT_FACTOR" times the maximum expected response time because
   the DRU doesn't start transmitting its response immediately */

#define RESPONSE_TIME_OUT_mS(len)               ( (uint32_t) ( ROUNDUP_BYTE_TX_TIME_AT_2400_BPS_MS * \
                                                   ( ACK_SIZE + len ) *\
                                                   RESPONSE_TIME_OUT_FACTOR ) )

#define DRU_INVALID_DATE                             0
#define DRU_INVALID_TIME                        0x8700

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ****************************************************************************************************************** */
/* GLOBAL VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

OS_QUEUE_Obj ILC_DRU_DRIVER_QueueHandle;                        /* DRU Driver Queue Handle */
OS_MUTEX_Obj ILC_DRU_DRIVER_Mutex_;                             /* DRU Driver Mutex Handle */

static OS_SEM_Obj    ILC_SRFN_REG_Sem_;                         /* SRFN Registration Module Semaphore Handle */
static OS_SEM_Obj    ILC_TIME_SYNC_Sem_;                        /* Time Sync Semaphore Handle */
static OS_SEM_Obj    ILC_TNL_Sem_;                              /* Tunneling Handler Semaphore Handle */

static ILC_DRU_DRIVER_queue_t *pQueue_;


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static uint16_t       Get_Diff_InMilliseconds ( OS_TICK_Struct *pPrevTickValue, OS_TICK_Struct *pCurrTickValue );
static returnStatus_t TransmitDruRequest ( uint8_t *pDataBuffer_u8, uint32_t DataLength_u32,
                                          uint16_t DruRequestTimeOutMs_u16, bool PrintIfTimeOutExpires_b );
static returnStatus_t WaitDruResponse ( uint8_t *pDest_u8, uint8_t *pRxLength_u8, uint16_t DruResponseTimeOutMs_u16,
                                          bool PrintIfTimeOutExpires_b );
static returnStatus_t ProcessDruReadRegister ( uint16_t DruRegisterToRead_u16 );
static returnStatus_t ProcessDruWriteRegister ( uint16_t RegisterId, uint8_t RegisterLen, uint8_t const * data );
static returnStatus_t ProcessDruTimeSync ( sysTime_t sTime );
static returnStatus_t ProcessDruMessageTunneling ( uint8_t *pOutboundMsg_u8, uint8_t OutboundMsgSize_u8,
                                                   bool WaitForInboundMessage_b, bool *pInboundMsgAvailable_b,
                                                   uint8_t *pInboundMsg_u8, uint8_t *pInboundMsgSize_u8 );

static enum_CIM_QualityCode ProcessDruReq(ILC_DRU_DRIVER_queue_t *req, uint32_t TimeOutMs_u32);


/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
/*******************************************************************************
  Function name: Get_Diff_InMilliseconds

  Purpose: This function will return the number of milliseconds difference between
           the two passed in Tick Structure values

  Arguments: OS_TICK_Struct *pPrevTickValue - Location of smaller tick value for time difference calculation
             OS_TICK_Struct *pCurrTickValue - Location of greater tick value for time difference calculation

  Returns: TimeDiff_u16 TimeDiff - difference in milliseconds

  Notes: This function returns a 16-bit value to have a consistent data type with the configurable time outs
*******************************************************************************/
static uint16_t Get_Diff_InMilliseconds ( OS_TICK_Struct *pPrevTickValue, OS_TICK_Struct *pCurrTickValue )
{
   uint32_t TimeDiff_u32;
   bool     Overflow_b;

   TimeDiff_u32 = (uint32_t)_time_diff_milliseconds ( pCurrTickValue, pPrevTickValue, &Overflow_b );

   if ( Overflow_b == true )
   {
      TimeDiff_u32 = 0;
      INFO_printf( "Overflow in Get_Diff_InMilliseconds" );
   }

   return ( (uint16_t) TimeDiff_u32 );
}


/***********************************************************************************************************************
  Function name: TransmitDruRequest

  Purpose: Transmit a DRU Request with a configurable time out.

  Arguments: uint8_t  *pDataBuffer_u8         - Pointer to the data that is going to be sent to the DRU
             uint32_t DataLength_u32          - Number of bytes that are going to be sent (size of data in
                                                pDataBuffer_u8)
             uint16_t DruRequestTimeOutMs_u16 - Time out in milliseconds for transmitting a DRU Request
             bool     PrintIfTimeOutExpires_b - Enable/Disable to print a debug message if TransmitDruRequest is exiting
                                                because the time out for transmitting a DRU Request has expired

  Returns: eSUCCESS/eFAILURE indication
 **********************************************************************************************************************/
static returnStatus_t TransmitDruRequest ( uint8_t *pDataBuffer_u8, uint32_t DataLength_u32,
                                          uint16_t DruRequestTimeOutMs_u16, bool PrintIfTimeOutExpires_b )
{
   returnStatus_t retVal = eFAILURE;
   OS_TICK_Struct CurrentTicks;
   OS_TICK_Struct StartTimestampTicks;

   /* Take timestamp of the time out start */
   OS_TICK_Get_CurrentElapsedTicks ( &StartTimestampTicks );

   /* Get initial timestamp for comparison */
   OS_TICK_Get_CurrentElapsedTicks ( &CurrentTicks );

   /* Flush the uart TX buffers */
   ( void )UART_flush ( UART_HOST_COMM_PORT );

   /* Flush the uart RX buffers */
   UART_RX_flush ( UART_HOST_COMM_PORT );

   INFO_printHex( "DRU command : ", pDataBuffer_u8, ( uint16_t )DataLength_u32 );
   /* Non-blocking transmition */
   ( void )UART_write( UART_HOST_COMM_PORT, pDataBuffer_u8, DataLength_u32 );

   /* Wait for the request to be transmitted. The time out is to cover a UART hardware error */
   while  ( ( 0 ==  UART_isTxEmpty ( UART_HOST_COMM_PORT ) )  &&
            ( DruRequestTimeOutMs_u16 > Get_Diff_InMilliseconds ( &StartTimestampTicks, &CurrentTicks ) ) )
   {
      /* At 2400 bps, transmitting 24 bytes takes 80 ms. Sleep while UART transmission is completed */
      OS_TASK_Sleep ( TEN_MSEC );
   }

   if ( DruRequestTimeOutMs_u16 <= Get_Diff_InMilliseconds ( &StartTimestampTicks, &CurrentTicks ) )
   {
      if ( TIME_OUT_PRINT_ENABLED == PrintIfTimeOutExpires_b )
      {
         INFO_printf( "DRU Request timed out" );
      }
   }
   else
   {
      retVal = eSUCCESS;
   }

   return retVal;
}


/***********************************************************************************************************************
  Function name: WaitDruResponse

  Purpose: Receive a DRU response until any of the following conditions is true: time out expires, a NACK is received,
           a predefined maximum amount of bytes is received or an EOT character is received; the latter condition
           indicates a complete DRU response has been received.

  Arguments: uint8_t  *pDest_u8                - Location where the received data will be stored
             uint8_t  *pRxLength_u8            - Number of bytes received from DRU. NOTE: This parameter only gets
                                                 updated if an EOT character was received.
             uint16_t DruResponseTimeOutMs_u16 - Time out in milliseconds for waiting a complete DRU response
             bool     PrintIfTimeOutExpires_b  - Enable/Disable to print a debug message if WaitDruResponse is exiting
                                                 because the time out waiting a DRU Response expired

  Returns: eSUCCESS/eFAILURE indication if an ACK was received
 **********************************************************************************************************************/
static returnStatus_t WaitDruResponse ( uint8_t *pDest_u8, uint8_t *pRxLength_u8, uint16_t DruResponseTimeOutMs_u16,
                                       bool PrintIfTimeOutExpires_b )
{
   returnStatus_t retVal = eFAILURE;
   OS_TICK_Struct CurrentTicks;
   OS_TICK_Struct StartTimestampTicks;
   uint8_t rxByte_u8 = 0;
   uint8_t rxByteCounter_u8 = 0;

   *pRxLength_u8 = 0;

   /* Take timestamp of the time out start */
   OS_TICK_Get_CurrentElapsedTicks ( &StartTimestampTicks );

   /* Get initial timestamp for comparison */
   OS_TICK_Get_CurrentElapsedTicks ( &CurrentTicks );

   /* wait for first response byte, it should be ACK or NACK */
   while ( ( ACK_VALUE != rxByte_u8 ) &&
           ( NACK_VALUE != rxByte_u8 ) &&
           ( sizeof( uint8_t ) != rxByteCounter_u8 ) &&
           ( DruResponseTimeOutMs_u16 > Get_Diff_InMilliseconds ( &StartTimestampTicks, &CurrentTicks ) ) )
   {
      /* non-blocking reception, read one byte at a time */
      if ( 0 != UART_read( UART_HOST_COMM_PORT, &rxByte_u8, sizeof( rxByte_u8 ) ) )
      {
         rxByteCounter_u8++;
      }
      /* Update current ticks */
      OS_TICK_Get_CurrentElapsedTicks ( &CurrentTicks );

      OS_TASK_Sleep ( 2 );
   }

   /* If the first byte received is an ACK the DRU accepted the Request Packet and it may or may not send an Inbound
      Message */
   if ( ACK_VALUE == rxByte_u8 )
   {
      retVal = eSUCCESS;

      /* Set rxByteCounter_u8 to 0, since the ACK doesn't count as part of the response */
      rxByteCounter_u8 = 0;

      OS_TICK_Get_CurrentElapsedTicks ( &CurrentTicks );

      /* wait for EOT to be received, for the maximum amount of bytes to be received or for the time out to expire */
      while ( ( EOT_VALUE != rxByte_u8 ) &&
              ( DRU_RESPONSE_MAX_SIZE_IN_BYTES > rxByteCounter_u8 ) &&
              ( DruResponseTimeOutMs_u16 > Get_Diff_InMilliseconds ( &StartTimestampTicks, &CurrentTicks ) ) )
      {
         /* non-blocking reception, one byte at a time */
         if ( 0 != UART_read( UART_HOST_COMM_PORT, &rxByte_u8, sizeof( rxByte_u8 ) ) )
         {
            *pDest_u8 = rxByte_u8;
             pDest_u8++;
             rxByteCounter_u8++;
         }

         /* Update current ticks */
         OS_TICK_Get_CurrentElapsedTicks ( &CurrentTicks );
      }

      if ( EOT_VALUE == rxByte_u8 )
      {
         /* Update the number of bytes received only if a complete DRU Response (i.e. an EOT) was received*/
         *pRxLength_u8 = rxByteCounter_u8;
         INFO_printHex( "DRU response: ", pDest_u8 - rxByteCounter_u8, ( uint16_t )rxByteCounter_u8 );
      }
      /* If the function is exiting because the maximum amount of expected bytes was received, print related message and
         return eFAILURE */
      else if ( DRU_RESPONSE_MAX_SIZE_IN_BYTES <= rxByteCounter_u8 )
      {
         INFO_printf( "EOT was not received and maximum amount of expected bytes was received " );
      }
      /* If the function is exiting because the communication timed out, print related message and return
         eDRU_COMMUNICATION_TIMED_OUT*/
      else if ( DruResponseTimeOutMs_u16 <= Get_Diff_InMilliseconds ( &StartTimestampTicks, &CurrentTicks ) )
      {
         if ( ACK_VALUE != rxByte_u8 )
         {
            /* Print related time out debug message if the calling function requested it */
            if ( TIME_OUT_PRINT_ENABLED == PrintIfTimeOutExpires_b )
            {
               INFO_printf( "DRU Request timed out waiting for EOT" );
            }

            retVal = eDRU_COMMUNICATION_TIMED_OUT;
         }
      }
   }
   /* If the first byte received is a NACK the DRU rejected the Request Packet, return error */
   else if (NACK_VALUE == rxByte_u8)
   {
      INFO_printf( "DRU responded with NACK" );
      retVal = eDRU_NACK_RESPONSE;
   }
   /* If the function is exiting because the communication timed out, print related message and return
      eDRU_COMMUNICATION_TIMED_OUT*/
   else if ( DruResponseTimeOutMs_u16 <= Get_Diff_InMilliseconds ( &StartTimestampTicks, &CurrentTicks ) )
   {
      /* Print related time out debug message if the calling function requested it */
      if ( TIME_OUT_PRINT_ENABLED == PrintIfTimeOutExpires_b )
      {
         INFO_printf("DRU Response timed out");
      }

      retVal = eDRU_COMMUNICATION_TIMED_OUT;
   }
   /* If the first byte received is not ACK or NACK, print related message and return eFAILURE */
   else
   {
      INFO_printf("DRU did not responded with ACK nor NACK");
   }

  return retVal;
}


/***********************************************************************************************************************
  Function name: encodeDruPacket

  Purpose: Encode the packet to be transmited to the DRU.
           This inserts SOH, converts the data to ASCII, adds the checksum ends with an EOT

  Arguments: uint8_t  *src               - Pointer to the source data
                       srcBytes          - Number of source bytes
                      *dst               - Pointer to the destination buffer

  Returns:   number of bytes in the destination buffer
 **********************************************************************************************************************/
static uint8_t encodeDruPacket( uint8_t const *pSrc, uint8_t srcBytes, uint8_t *pDst)
{
   uint8_t length = 0;
   uint8_t Checksum = 0;

   *pDst++ = SOH_VALUE;
   length++;

   // encode the data as ascii
   uint8_t index_u8;
   for ( index_u8 = 0U; index_u8 < srcBytes; index_u8++ )
   {
      ASCII_htoa( pDst, *pSrc);
      Checksum += *pSrc;
      pDst += 2;
      length += 2;
      pSrc++;
   }

   // Add the checksum
   ASCII_htoa( pDst, Checksum);
   pDst += 2;
   length += 2;

   *pDst++ = EOT_VALUE;
   length++;

   return length;
}



/***********************************************************************************************************************
  Function name: ProcessDruReadRegister

  Purpose: Process the request to read any of the DRU Registers for SRFN Registration

  Arguments: uint16_t RegisterId   - Register to write
             uint16_t RegisterLen  - number of bytes to write
             uint16_t RegisterData - Array of bytes (15 max)

  Returns: eSUCCESS/eFAILURE indication
 **********************************************************************************************************************/
static returnStatus_t ProcessDruWriteRegister ( uint16_t RegisterId, uint8_t RegisterLen, uint8_t const * RegisterData )
{
   buffer_t       *rxBuf;
   uint8_t        *rx_string;
   returnStatus_t retVal = eFAILURE;
   uint8_t        bytesReceived_u8 = 0;

   rxBuf = BM_alloc( DRU_RESPONSE_MAX_SIZE_IN_BYTES );
   if ( rxBuf != NULL )
   {
      rx_string = rxBuf->data;
      /* initialize response to zeros */
      ( void )memset( rx_string, 0, DRU_RESPONSE_MAX_SIZE_IN_BYTES );

      // Build the write register command
      // Create a buffer for the request message
      uint8_t reqBuffer[DRU_REQUEST_MAX_SIZE_IN_BYTES/2];

      uint16_t bitNo = 0;
      uint8_t* pDst = reqBuffer;

      /*lint --e{838} previously assigned values not used OK.  */
      bitNo = PACK_uint8_2bits(  &(uint8_t) {63},   8,  pDst, bitNo);    // Opcode        (  8 bits )
      bitNo = PACK_uint8_2bits(  &(uint8_t) { 1},   8,  pDst, bitNo);    // Address Mode  (  8 bits )
      bitNo = PACK_uint32_2bits( &(uint32_t){ 0},  32,  pDst, bitNo);    // Serial Number ( 32 bits )
      bitNo = PACK_uint16_2bits( &(uint16_t){ 0},  14,  pDst, bitNo);    // inbound Spec  ( 14 bits )

      // pack the register id
      bitNo = PACK_uint8_2bits ( &(uint8_t) { 1},   2,  pDst, bitNo);    // Register Mode (  2 bits )
      bitNo = PACK_uint16_2bits( &RegisterId,      12,  pDst, bitNo);    // Register Id  ( 12 bits )
      bitNo = PACK_uint8_2bits ( &RegisterLen,      4,  pDst, bitNo);    // Register Len (  4 bits )

      for(int i=0;i<RegisterLen; i++)
      {
         bitNo = PACK_uint8_2bits ( &RegisterData[i],         8,  pDst, bitNo);
      }

      uint8_t reqBytes = (uint8_t) bitNo/8;

      uint8_t txBuffer[DRU_REQUEST_MAX_SIZE_IN_BYTES];
      uint8_t txLength;

      txLength = encodeDruPacket( reqBuffer, reqBytes, txBuffer);

      /* Transmit the Request Packet */
      if ( eSUCCESS == TransmitDruRequest ( ( uint8_t * ) txBuffer, txLength,
                                              RESPONSE_TIME_OUT_mS(txLength), ( bool )TIME_OUT_PRINT_ENABLED ) )
      {
         /* Wait with a time out for a complete DRU response */
         if ( eSUCCESS == WaitDruResponse ( rx_string, &bytesReceived_u8, RESPONSE_TIME_OUT_mS(16),
                                             ( bool)TIME_OUT_PRINT_ENABLED ) )
         {
            /* If the expected amount of response bytes was received, convert the DRU Register value to the
               corresponding data type */

            // For now this will be 16 bytes
            if ( bytesReceived_u8  != 16 )
            {
               INFO_printf( "Unexpected amount of bytes received for write register command" );
            }
            else{
               retVal = eSUCCESS;
            }
         }
      }
      BM_free( rxBuf );
   }
   return retVal;
}

/***********************************************************************************************************************
  Function name: ProcessDruReadRegister

  Purpose: Process the request to read any of the DRU Registers for SRFN Registration

  Arguments: uint16_t DruRegisterToRead_u16 - DRU Register to be read

  Returns: eSUCCESS/eFAILURE indication
 **********************************************************************************************************************/
static returnStatus_t ProcessDruReadRegister ( uint16_t DruRegisterToRead_u16 )
{
   buffer_t       *rxBuf;
   uint8_t        *rx_string;
   returnStatus_t retVal = eFAILURE;
   uint8_t        bytesReceived_u8 = 0;

   rxBuf = BM_alloc( DRU_RESPONSE_MAX_SIZE_IN_BYTES );
   if ( rxBuf != NULL )
   {
      rx_string = rxBuf->data;
      /* initialize response to zeros */
      ( void )memset( rx_string, 0, DRU_RESPONSE_MAX_SIZE_IN_BYTES );

      switch ( DruRegisterToRead_u16 )
      {
         case ILC_DRU_DRIVER_REG_SERIAL_NUMBER:
         {
            uint32_t SerialNumberTemp_u32 = 0;

            /* Transmit the Request Packet */
            if ( eSUCCESS == TransmitDruRequest ( ( uint8_t * ) ReadDruSerialNumberRequestPacket_au8,
                                                   DRU_SERIAL_NUMBER_REQUEST_SIZE,
                                                   REQUEST_TIME_OUT_mS(DRU_SERIAL_NUMBER_REQUEST_SIZE),
                                                   ( bool )TIME_OUT_PRINT_ENABLED ) )
            {
               /* Wait with a time out for a complete DRU response */
               if ( eSUCCESS == WaitDruResponse ( rx_string, &bytesReceived_u8,
                                                  RESPONSE_TIME_OUT_mS(DRU_SERIAL_NUMBER_RESPONSE_SIZE ) ,
                                                  (bool)TIME_OUT_PRINT_ENABLED ) )
               {
                  /* If the expected amount of response bytes was received, convert the DRU Register value to the
                     corresponding data type */
                  if ( DRU_SERIAL_NUMBER_RESPONSE_SIZE == bytesReceived_u8 )
                  {
                     /* If all converted bytes are valid ASCIIs, update registers' values and return eSUCCESS */
                     if ( eSUCCESS == ASCII_atohU32 ( &rx_string[ MPSPP_RESPONSE_DATA_OFFSET ], &SerialNumberTemp_u32 ) )
                     {
                        pQueue_->druData.SerialNumber_u32 = SerialNumberTemp_u32;
                        retVal = eSUCCESS;
                     }
                     else
                     {
                        INFO_printf( "Invalid ASCII character received while reading DRU Serial Number" );
                     }
                  }
                  else
                  {
                     INFO_printf( "Unexpected amount of bytes received while reading DRU Serial Number" );
                  }
               }
            }
            break;
         }

         case ILC_DRU_DRIVER_REG_FW_VERSION:
         {
            uint8_t  FwMajorTemp_u8  = 0;
            uint8_t  FwMinorTemp_u8  = 0;
            uint16_t FwBuildTemp_u16 = 0;

            /* Transmit the Request Packet */
            if ( eSUCCESS == TransmitDruRequest ( ( uint8_t * ) ReadDruFwVersionRequestPacket_au8,
                                                   DRU_FW_VERSION_REQUEST_SIZE,
                                                   REQUEST_TIME_OUT_mS(DRU_FW_VERSION_REQUEST_SIZE),
                                                   ( bool )TIME_OUT_PRINT_ENABLED ) )
            {
               /* Wait with a time out for a complete DRU response */
               if ( eSUCCESS == WaitDruResponse ( rx_string, &bytesReceived_u8,
                                                 RESPONSE_TIME_OUT_mS(DRU_FW_VERSION_RESPONSE_SIZE ),
                                                ( bool )TIME_OUT_PRINT_ENABLED ) )
               {
                  /* If the expected amount of response bytes was received, convert the DRU Register value to the
                     corresponding data type */
                  if ( ( DRU_FW_VERSION_RESPONSE_SIZE ) == bytesReceived_u8 )
                  {
                    /* If all converted bytes are valid ASCIIs, update registers values and return eSUCCESS */
                    if ( ( eSUCCESS == ASCII_atohByte( rx_string[ MPSPP_RESPONSE_DATA_OFFSET ],
                                                        rx_string[ MPSPP_RESPONSE_DATA_OFFSET + 1 ], &FwMajorTemp_u8 ) ) &&
                         ( eSUCCESS == ASCII_atohByte( rx_string[ MPSPP_RESPONSE_DATA_OFFSET + 2 ],
                                                       rx_string[ MPSPP_RESPONSE_DATA_OFFSET + 3 ], &FwMinorTemp_u8 ) ) &&
                         ( eSUCCESS == ASCII_atohU16 ( &rx_string[ MPSPP_RESPONSE_DATA_OFFSET + 4 ], &FwBuildTemp_u16 ) ) )
                    {
                       pQueue_->druData.FwVer.FwMajor_u8 = FwMajorTemp_u8;
                       pQueue_->druData.FwVer.FwMinor_u8 = FwMinorTemp_u8;
                       pQueue_->druData.FwVer.FwEngBuild_u16 = FwBuildTemp_u16;
                       retVal = eSUCCESS;
                    }
                    else
                    {
                       INFO_printf( "Invalid ASCII character received while reading DRU FW Version" );
                    }
                  }
                  else
                  {
                     INFO_printf( "Unexpected amount of bytes received while reading DRU FW Version" );
                  }
               }
            }
            break;
         }

         case ILC_DRU_DRIVER_REG_HW_VERSION:
         {
            uint8_t HwGenTemp_u8  = 0;
            uint8_t HwRevTemp_u8  = 0;

            /* Transmit the Request Packet */
            if ( eSUCCESS == TransmitDruRequest ( ( uint8_t * ) ReadDruHwVersionRequestPacket_au8,
                                                   DRU_HW_VERSION_REQUEST_SIZE,
                                                   REQUEST_TIME_OUT_mS(DRU_HW_VERSION_REQUEST_SIZE),
                                                   ( bool )TIME_OUT_PRINT_ENABLED ) )
            {
               /* Wait with a time out for a complete DRU response */
               if ( eSUCCESS == WaitDruResponse ( rx_string, &bytesReceived_u8,
                                                 RESPONSE_TIME_OUT_mS(DRU_HW_VERSION_RESPONSE_SIZE ),
                                                ( bool )TIME_OUT_PRINT_ENABLED ) )
               {
                  /* If the expected amount of response bytes was received, convert the DRU Register value to the
                     corresponding data type */
                  if ( DRU_HW_VERSION_RESPONSE_SIZE == bytesReceived_u8 )
                  {
                     /* If all converted bytes are valid ASCIIs, update registers values and return eSUCCESS */
                     if ( ( eSUCCESS == ASCII_atohByte( rx_string[ MPSPP_RESPONSE_DATA_OFFSET ],
                                                         rx_string[ MPSPP_RESPONSE_DATA_OFFSET + 1 ], &HwGenTemp_u8 ) ) &&
                          ( eSUCCESS == ASCII_atohByte( rx_string[ MPSPP_RESPONSE_DATA_OFFSET + 2 ],
                                                        rx_string[ MPSPP_RESPONSE_DATA_OFFSET + 3 ], &HwRevTemp_u8 ) ) )
                     {
                        pQueue_->druData.HwVer.HwGeneration_u8 = HwGenTemp_u8;
                        pQueue_->druData.HwVer.HwRevision_u8 = HwRevTemp_u8;
                        retVal = eSUCCESS;
                     }
                     else
                     {
                        INFO_printf( "Invalid ASCII character received while reading DRU HW Version" );
                     }
                  }
                  else
                  {
                     INFO_printf( "Unexpected amount of bytes received while reading DRU HW Version" );
                  }
               }
            }
            break;
         }

         case ILC_DRU_DRIVER_REG_MODEL:
         {
            uint8_t RceTypTemp_u8  = 0;
            uint8_t RceModTemp_u8  = 0;

            /* Transmit the Request Packet */
            if ( eSUCCESS == TransmitDruRequest ( ( uint8_t * ) ReadDruRceProductRequestPacket_au8, DRU_MODEL_REQUEST_SIZE,
                                                  REQUEST_TIME_OUT_mS(DRU_MODEL_REQUEST_SIZE),
                                                  ( bool )TIME_OUT_PRINT_ENABLED ) )
            {
               /* Wait with a time out for a complete DRU response */
               if ( eSUCCESS == WaitDruResponse ( rx_string, &bytesReceived_u8,
                                                 RESPONSE_TIME_OUT_mS(DRU_MODEL_RESPONSE_SIZE ),
                                                ( bool )TIME_OUT_PRINT_ENABLED ) )
               {
                  /* If the expected amount of response bytes was received, convert the DRU Register value to the
                     corresponding data type */
                  if ( DRU_MODEL_RESPONSE_SIZE == bytesReceived_u8 )
                  {
                     /* If all converted bytes are valid ASCIIs, update registers values and return eSUCCESS */
                     if ( ( eSUCCESS == ASCII_atohByte( rx_string[ MPSPP_RESPONSE_DATA_OFFSET ],
                                                         rx_string[ MPSPP_RESPONSE_DATA_OFFSET + 1 ], &RceTypTemp_u8 ) ) &&
                          ( eSUCCESS == ASCII_atohByte( rx_string[ MPSPP_RESPONSE_DATA_OFFSET + 2 ],
                                                        rx_string[ MPSPP_RESPONSE_DATA_OFFSET + 3 ], &RceModTemp_u8 ) ) )
                     {
                        pQueue_->druData.ModNum.RceType_u8  = RceTypTemp_u8;
                        pQueue_->druData.ModNum.RceModel_u8 = RceModTemp_u8;
                        retVal = eSUCCESS;
                     }
                     else
                     {
                        INFO_printf( "Invalid ASCII character received while reading DRU Model" );
                     }
                  }
                  else
                  {
                     INFO_printf( "Unexpected amount of bytes received while reading DRU model" );
                  }
               }
            }
            break;
         }

         default:
         {
            INFO_printf( "Unsupported DRU Register requested to be read" );
            retVal = eDRU_UNSUPPORTED_REGISTER_ERROR;
            break;
         }
      }
      BM_free( rxBuf );
   }

   return retVal;
}


/***********************************************************************************************************************
  Function name: ProcessDruTimeSync

  Purpose: Process the request to set a new DRU's date/time

  Arguments: sysTime_t sTime - Date/time (in SysTime format) the DRU will be updated with

  Returns: eSUCCESS/eFAILURE indication
 **********************************************************************************************************************/
static returnStatus_t ProcessDruTimeSync ( sysTime_t sTime )
{
   buffer_t       *rxBuf;
   uint8_t        *rx_string;
   returnStatus_t retVal = eFAILURE;
   uint8_t        bytesReceived_u8 = 0;

   rxBuf = BM_alloc( DRU_RESPONSE_MAX_SIZE_IN_BYTES );
   if ( rxBuf != NULL )
   {
      rx_string = rxBuf->data;
      ( void )memset( rx_string, 0, DRU_RESPONSE_MAX_SIZE_IN_BYTES );
      // Create a buffer for the request message
      uint8_t reqBuffer[DRU_REQUEST_MAX_SIZE_IN_BYTES/2];

      uint16_t bitNo = 0;
      uint8_t* pDst = reqBuffer;

      /*lint --e{838} previously assigned values not used OK.  */
      bitNo = PACK_uint8_2bits(  &(uint8_t) {69},   8,  pDst, bitNo);    // Opcode        (  8 bits )
      bitNo = PACK_uint8_2bits(  &(uint8_t) { 1},   8,  pDst, bitNo);    // Address Mode  (  8 bits )
      bitNo = PACK_uint32_2bits( &(uint32_t){ 0},  32,  pDst, bitNo);    // Serial Number ( 32 bits )
      bitNo = PACK_uint16_2bits( &(uint16_t){ 0},  14,  pDst, bitNo);    // inbound Spec  ( 14 bits )

      {
         /* Initialize to invalid values */
         sysTime_druFormat_t sDruTime;
         sDruTime.days_u16 = DRU_INVALID_DATE;
         sDruTime.counts_u16 = DRU_INVALID_TIME;

         ( void )TIME_UTIL_ConvertSysFormatToDruFormat( &sTime, &sDruTime );
         bitNo = PACK_uint16_2bits( &sDruTime.days_u16,     16,  pDst, bitNo);
         bitNo = PACK_uint16_2bits( &sDruTime.counts_u16,   16,  pDst, bitNo);

         // Pad message to ensure that number of bits is evenly dividible by 8
         uint8_t numPadBits = 8 - (bitNo % 8);
         if(numPadBits > 0)
         {
            INFO_printf("numPadBits = %d", numPadBits);
            bitNo = PACK_uint8_2bits(  &(uint8_t) { 0},  numPadBits,  pDst, bitNo);    // pad bits
         }
      }
      uint8_t reqBytes = (uint8_t) bitNo/8;

      uint8_t txBuffer[DRU_REQUEST_MAX_SIZE_IN_BYTES];
      uint8_t txLength;

      // Call the encoder function to convert to serial ascii protocol
      txLength = encodeDruPacket( reqBuffer, reqBytes, txBuffer);

      /* Transmit the Request Packet */
      if ( eSUCCESS == TransmitDruRequest ( txBuffer, txLength,
                                            REQUEST_TIME_OUT_mS(txLength),  ( bool )TIME_OUT_PRINT_ENABLED ) )
      {
         /* Wait with a time out for a complete DRU response */
         if ( eSUCCESS == WaitDruResponse ( rx_string, &bytesReceived_u8,
                                            RESPONSE_TIME_OUT_mS(DRU_SET_DATE_TIME_RESPONSE_SIZE),
                                            ( bool )TIME_OUT_PRINT_ENABLED ) )
         {
            /* If an EOT was received and the expected amount of response bytes was received, return eSUCCESS */
            if ( DRU_SET_DATE_TIME_RESPONSE_SIZE == bytesReceived_u8 )
            {
               retVal = eSUCCESS;
            }
         }
      }
      BM_free( rxBuf );
   }

   return retVal;
}


/***********************************************************************************************************************
  Function name: ProcessDruMessageTunneling

  Purpose: Process the request to tunnel an outbound message and, if exists, its response (inbound message)

  Arguments: uint8_t *pOutboundMsg_u8         - Location of the Outbound Message
             uint8_t  OutboundMsgSize_u8      - Size of the Outbound Message
             bool     WaitForInboundMessage_b - Indicates to DRU Driver if it shall wait for a DRU Response (Two-way
                                                tunneling) or not (One-way tunneling)
             bool    *pInboundMsgAvailable_b  - Pointer to flag that indicates to the TunnelingHandler that there is an
                                                Inbound Message available
             uint8_t *pInboundMsg_u8          - Location of the Inbound Message
             uint8_t *pInboundMsgSize_u8      - Location of the Inbound Message Size

  Returns: eSUCCESS/eFAILURE indication
 **********************************************************************************************************************/
static returnStatus_t ProcessDruMessageTunneling ( uint8_t *pOutboundMsg_u8, uint8_t OutboundMsgSize_u8,
                                                   bool WaitForInboundMessage_b, bool *pInboundMsgAvailable_b,
                                                   uint8_t *pInboundMsg_u8, uint8_t *pInboundMsgSize_u8 )
{
   returnStatus_t retVal = eFAILURE;

   if ( ( NULL != pOutboundMsg_u8 ) && ( 0 < OutboundMsgSize_u8 ) )
   {
      /* Transmit the Request Packet */
      if ( eSUCCESS == TransmitDruRequest ( pOutboundMsg_u8, (uint32_t ) OutboundMsgSize_u8,
           REQUEST_TIME_OUT_mS(DRU_REQUEST_MAX_SIZE_IN_BYTES),
           ( bool )TIME_OUT_PRINT_ENABLED ) )
      {
         if ( true == WaitForInboundMessage_b )
         {
            /* Verify Outbound Message is not pointing to null due to the Tunneling Feature's criticality */
            assert( NULL != pOutboundMsg_u8 );

            /* initialize to No Inbound Message to be transmitted*/
            *pInboundMsgAvailable_b = false;
            *pInboundMsgSize_u8 = 0;

            /* Wait, with a time out, for a complete DRU response */
            retVal = WaitDruResponse ( pInboundMsg_u8, pInboundMsgSize_u8,
                                      RESPONSE_TIME_OUT_mS( DRU_RESPONSE_MAX_SIZE_IN_BYTES ) ,
                                     ( bool )TIME_OUT_PRINT_DISABLED );
            if ( eSUCCESS == retVal )
            {
               /* if the value of pInboundMsgSize_u8's content is greater than zero, i.e. an EOT was received, it means
                  a complete Inbound Message is available */
               if ( 0 < *pInboundMsgSize_u8 )
               {
                  *pInboundMsgAvailable_b = true;
               }
            }
         }
         else
         {
            /* Don't wait for a DRU Response */
            retVal = eSUCCESS;
         }
      }
   }
   else
   {
      INFO_printf( "Invalid ProcessDruMessageTunneling input parameters" );
   }
  return retVal;
}


/***********************************************************************************************************************
   Function name: ILC_DRU_DRIVER_readDruRegister

   Purpose: Enqueues a request to read a DRU Register

   Arguments:  ILC_DRU_DRIVER_druRegister_u *pDest - Location to store the DRU Register to be read
               uint16_t RegisterId_u16             - DRU Register to be read
               uint32_t TimeOutMs_u32              - Time out in milliseconds this function will wait for its
                                                     corresponding semaphore to be posted on

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Note: This is a blocking function
 **********************************************************************************************************************/
enum_CIM_QualityCode ILC_DRU_DRIVER_readDruRegister ( ILC_DRU_DRIVER_druRegister_u *pDest, uint16_t RegisterId_u16,
                                                      uint32_t TimeOutMs_u32 )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   OS_MUTEX_Lock( &ILC_DRU_DRIVER_Mutex_ );
   buffer_t *pBuf = BM_alloc( sizeof( ILC_DRU_DRIVER_queue_t ) );

   if ( NULL != pBuf )
   {
      /* Contains the request to DRU */
      ILC_DRU_DRIVER_queue_t *req = ( ILC_DRU_DRIVER_queue_t * )( void * )pBuf->data;

      /* Read from the DRU */
      req->druOperation = eDRU_READ_REGISTER;

      /* DRU Register to be read */
      req->druRegister_u16 = RegisterId_u16;

      /* Semaphore handle */
      req->pSem = &ILC_SRFN_REG_Sem_;

      /* Start the transmission to the DRU */
      OS_QUEUE_Enqueue( &ILC_DRU_DRIVER_QueueHandle, req );

      /* Wait for DRU module to process */
      if ( OS_SEM_Pend( &ILC_SRFN_REG_Sem_, TimeOutMs_u32 ) )
      {
         /* Was the DRU Register's value retrieved successfully? */
         if ( eSUCCESS == req->druStatus )
         {
            /* DRU was successfully read! */
            (void)memcpy( (void*) pDest, &req->druData, sizeof( ILC_DRU_DRIVER_druRegister_u ) );

            retVal = CIM_QUALCODE_SUCCESS;
         }
         else
         {
            INFO_printf( "DRU Register's value was not retrieved successfully" );

            if ( eDRU_COMMUNICATION_TIMED_OUT == req->druStatus )
            {
               retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
            }
         }
      }
      /* Remove the request and return a "failed" read.  */
      else
      {
         ( void )OS_QUEUE_Dequeue ( &ILC_DRU_DRIVER_QueueHandle );
         INFO_printf( "ILC_DRU_DRIVER_readDruRegister request timed out" );
      }

      BM_free( pBuf );
   }
   else
   {
      INFO_printf( "Ran out of heap in ILC_DRU_DRIVER_readDruRegister" );
   }

   OS_MUTEX_Unlock( &ILC_DRU_DRIVER_Mutex_ );

   return retVal;
}


/***********************************************************************************************************************
Function name: ProcessDruReq

Purpose: Enqueues a DRU request and waits for request to be processed.

Arguments:  ILC_DRU_DRIVER_queue_t *req     -
            uint32_t TimeOutMs_u32          - Time out in milliseconds

Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

Note: This is a blocking function
 **********************************************************************************************************************/
static
enum_CIM_QualityCode ProcessDruReq(ILC_DRU_DRIVER_queue_t *req, uint32_t TimeOutMs_u32)
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   /* Start the transmission to the DRU */
   OS_QUEUE_Enqueue( &ILC_DRU_DRIVER_QueueHandle, req );

   /* Wait for DRU module to process */
   if ( OS_SEM_Pend( req->pSem, TimeOutMs_u32))
   {
      /* Was the command executed successfully? */
      if ( eSUCCESS == req->druStatus )
      {
         /* Wuccessfully updated! */
         retVal = CIM_QUALCODE_SUCCESS;
      }
      else
      {
         INFO_printf( "DRU write failed" );

         if ( eDRU_COMMUNICATION_TIMED_OUT == req->druStatus )
         {
            retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
         }
      }
   }
   else
   {  /* Remove the request and return a "failed" read */
      ( void )OS_QUEUE_Dequeue ( &ILC_DRU_DRIVER_QueueHandle );
      INFO_printf( "ILC_DRU_DRIVER: request timed out" );
   }
   return retVal;
}


/***********************************************************************************************************************
Function name: ILC_DRU_DRIVER_writeDruRegister

Purpose: Enqueues a request to write a DRU Register

Arguments:  uint16_t RegisterId             - Register to be read
            uint8_t  RegisterLen            - Register length
            uint8_t  Data                   - Pointer to the data to write
            uint32_t TimeOutMs_u32          - Time out in milliseconds this function will wait for its
                                              corresponding semaphore to be posted on

Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

Note: This is a blocking function
 **********************************************************************************************************************/


enum_CIM_QualityCode ILC_DRU_DRIVER_writeDruRegister ( uint16_t registerId, uint8_t registerLen, uint8_t const *data, uint32_t TimeOutMs_u32 )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   OS_MUTEX_Lock( &ILC_DRU_DRIVER_Mutex_ );
   buffer_t *pBuf = BM_alloc( sizeof( ILC_DRU_DRIVER_queue_t ) );

   if ( NULL != pBuf )
   {
      /* Contains the request to DRU */
      ILC_DRU_DRIVER_queue_t   *req = ( ILC_DRU_DRIVER_queue_t * )( void * )pBuf->data;

      /* Set DRU's/time */
      req->druOperation = eDRU_WRITE_REGISTER;

      req->writeReg.Id      =  registerId;
      req->writeReg.Length  =  registerLen;
      (void) memcpy( req->writeReg.Data, data, registerLen);

      /* Semaphore handle */
      req->pSem = &ILC_SRFN_REG_Sem_;

      retVal = ProcessDruReq(req, TimeOutMs_u32);

      BM_free( pBuf );
   }
   else
   {
      INFO_printf( "Ran out of heap in ILC_DRU_DRIVER_writeDruRegister" );
   }

   OS_MUTEX_Unlock( &ILC_DRU_DRIVER_Mutex_ );

   return retVal;
}


/***********************************************************************************************************************
   Function name: ILC_DRU_DRIVER_setDruDateTime

   Purpose: Enqueues a request to set the DRU's date and time

   Arguments:  sysTime_t newTime      - The date/time (in SysTime format) the DRU will be updated with
               uint32_t TimeOutMs_u32 - Time out in milliseconds this function will wait for its corresponding semaphore
                                        to be posted on

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Note: This is a blocking function
 **********************************************************************************************************************/
enum_CIM_QualityCode ILC_DRU_DRIVER_setDruDateTime ( sysTime_t newTime, uint32_t TimeOutMs_u32 )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   OS_MUTEX_Lock( &ILC_DRU_DRIVER_Mutex_ );
   buffer_t *pBuf = BM_alloc( sizeof( ILC_DRU_DRIVER_queue_t ) );

   if ( NULL != pBuf )
   {
      /* Contains the request to DRU */
      ILC_DRU_DRIVER_queue_t   *req = ( ILC_DRU_DRIVER_queue_t * )( void * )pBuf->data;

      /* Set DRU's/time */
      req->druOperation = eDRU_SET_DATE_TIME;

      /* New date/time for DRU in Sys Time format, to be converted to DRU date/time format */
      req->druSysTime = newTime;

      /* Semaphore handle */
      req->pSem = &ILC_TIME_SYNC_Sem_;

      /* Start the transmission to the DRU */
      OS_QUEUE_Enqueue( &ILC_DRU_DRIVER_QueueHandle, req );

      /* Wait for DRU module to process */
      if ( OS_SEM_Pend( &ILC_TIME_SYNC_Sem_, TimeOutMs_u32 ) )
      {
         /* Were the date and time updated successfully? */
         if ( eSUCCESS == req->druStatus )
         {
            /* DRU date and time were successfully updated! */
            retVal = CIM_QUALCODE_SUCCESS;
         }
         else
         {
            INFO_printf( "DRU date/time was not updated successfully" );

            if ( eDRU_COMMUNICATION_TIMED_OUT == req->druStatus )
            {
               retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
            }
         }
      }
      /* Remove the request and return a "failed" read */
      else
      {
         ( void )OS_QUEUE_Dequeue ( &ILC_DRU_DRIVER_QueueHandle );
         INFO_printf( "ILC_DRU_DRIVER_setDruDateTime request timed out" );
      }

      BM_free( pBuf );
   }
   else
   {
      INFO_printf( "Ran out of heap in ILC_DRU_DRIVER_setDruDateTime" );
   }

   OS_MUTEX_Unlock( &ILC_DRU_DRIVER_Mutex_ );

   return retVal;
}


/***********************************************************************************************************************
   Function name: ILC_DRU_DRIVER_tunnelOutboundMessage

   Purpose: Enqueues a request to tunnel messages to/from the DRU

   Arguments: uint8_t *pOutboundMsg_u8          - Location of the Outbound Message
              uint8_t  OutboundMsgSize_u8       - Size of the Outbound Message
              uint32_t TimeOutMs_u32            - Time out in milliseconds this function will wait for its corresponding
                                                  semaphore to be posted on
              bool     WaitForInboundMessage_b  - Indicates to DRU Driver if it shall wait for a DRU Response (Two-way
                                                  tunneling) or not (One-way tunneling)
              bool    *pInboundMsgFlag_b        - Pointer to indicate that there is an Inbound Message available
              uint8_t *pInboundMsg_u8           - Location of the Inbound Message
              uint8_t *pInboundMsgSize_u8       - Location of the Inbound Message Size

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Note: This is a blocking function
 **********************************************************************************************************************/
enum_CIM_QualityCode ILC_DRU_DRIVER_tunnelOutboundMessage ( uint8_t *pOutboundMsg_u8, uint8_t OutboundMsgSize_u8,
                                                            uint32_t TimeOutMs_u32, bool WaitForInboundMessage_b,
                                                            bool *pInboundMsgFlag_b, uint8_t *pInboundMsg_u8,
                                                            uint8_t *pInboundMsgSize_u8 )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   if ( ( NULL != pOutboundMsg_u8 ) && ( 0 < OutboundMsgSize_u8 ) )
   {
      OS_MUTEX_Lock( &ILC_DRU_DRIVER_Mutex_ );
      buffer_t *pBuf = BM_alloc( sizeof( ILC_DRU_DRIVER_queue_t ) );

      if ( NULL != pBuf )
      {
         /* Contains the request to DRU */
         ILC_DRU_DRIVER_queue_t   *req = ( ILC_DRU_DRIVER_queue_t * )( void * )pBuf->data;

         /* Tunnel outbound message */
         req->druOperation = eDRU_OUTBOUND_TUNNEL;
         req->pOutMsg_u8 = pOutboundMsg_u8;
         req->OutMsgSize_u8 = OutboundMsgSize_u8;
         req->WaitForInMsg_b = WaitForInboundMessage_b;
         req->pInMsgFlag_b = pInboundMsgFlag_b;
         req->pInMsg_u8 = pInboundMsg_u8;
         req->pInMsgSize_u8 = pInboundMsgSize_u8;

         /* Semaphore handle */
         req->pSem = &ILC_TNL_Sem_;

         /* Start the transmission to the DRU */
         OS_QUEUE_Enqueue( &ILC_DRU_DRIVER_QueueHandle, req );

         /* Wait for DRU module to process */
         if ( OS_SEM_Pend( &ILC_TNL_Sem_, TimeOutMs_u32 ) )
         {
            if ( eSUCCESS == req->druStatus )
            {
               /* Outbound Message was successfully tunneled! */
               retVal = CIM_QUALCODE_SUCCESS;
            }
            else
            {
               INFO_printf( "Outbound Message was not successfully tunneled" );

               if ( eDRU_COMMUNICATION_TIMED_OUT == req->druStatus )
               {
                  retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
               }
            }
         }
         /* Remove the request and return a "failed" read.  */
         else
         {
            ( void )OS_QUEUE_Dequeue ( &ILC_DRU_DRIVER_QueueHandle );
            INFO_printf( "ILC_DRU_DRIVER_tunnelOutboundMessage request timed out" );
         }

         BM_free( pBuf );
      }
      else
      {
         INFO_printf( "Ran out of heap in ILC_DRU_DRIVER_tunnelOutboundMessage" );
      }

      OS_MUTEX_Unlock( &ILC_DRU_DRIVER_Mutex_ );
   }
   else
   {
      INFO_printf( "Invalid ILC_DRU_DRIVER_tunnelOutboundMessage input parameters" );
   }

   return retVal;
}


/* *********************************************************************************************************************
  Function name: ILC_DRU_DRIVER_Init

  Purpose: Creates all the OS Objects that DRU Driver needs

  Arguments: None

  Returns: eSUCCESS/eFAILURE indication
 **********************************************************************************************************************/
returnStatus_t ILC_DRU_DRIVER_Init ( void )
{
   returnStatus_t retVal = eFAILURE;

   if ( true == OS_QUEUE_Create( &ILC_DRU_DRIVER_QueueHandle, ILC_DRIVER_QUEUE_SIZE ) )
   {
      if ( true == OS_MUTEX_Create( &ILC_DRU_DRIVER_Mutex_ ) )
      {
         //TODO NRJ: determine if semaphores need to be counting
         if ( true == OS_SEM_Create( &ILC_SRFN_REG_Sem_, 0 ) )
         {
            if ( true == OS_SEM_Create( &ILC_TIME_SYNC_Sem_,0 ) )
            {
               if ( true == OS_SEM_Create( &ILC_TNL_Sem_, 0 ) )
               {
                  retVal = eSUCCESS;
               }
               else
               {
                  INFO_printf("ILC_TNL_Sem_ semaphore was not created.");
               }
            }
            else
            {
               INFO_printf("ILC_TIME_SYNC_Sem_ semaphore was not created.");
            }
         }
         else
         {
            INFO_printf("ILC_SRFN_REG_Sem_ semaphore was not created.");
         }
      }
      else
      {
         INFO_printf("ILC_DRU_DRIVER_Mutex_ mutex was not created.");
      }
   }
   else
   {
      INFO_printf("ILC_DRU_DRIVER_QueueHandle queue was not created.");
   }

   return retVal;
}


/***********************************************************************************************************************
  Function name: ILC_DRU_DRIVER_Task

  Purpose: Task for handling the DRU Driver

  Arguments: uint32_t Arg0 - Not used, but required here by MQX because this is a task

  Returns: None
 **********************************************************************************************************************/
/*lint -esym(715,Arg0) required by API */
void ILC_DRU_DRIVER_Task( uint32_t Arg0 )
{
   for( ;; )
   {
      if ( 0U != OS_QUEUE_NumElements( &ILC_DRU_DRIVER_QueueHandle ) )
      {
         pQueue_ = OS_QUEUE_Dequeue ( &ILC_DRU_DRIVER_QueueHandle );

         /* if pQueue_ doesn't point to NULL process the request */
         if ( NULL != pQueue_ )
         {
            pQueue_->druStatus = eDRU_NOT_PROCCESSED;

            switch ( pQueue_->druOperation )
            {
               case eDRU_OUTBOUND_TUNNEL:
               {
                  pQueue_->druStatus = ProcessDruMessageTunneling( pQueue_->pOutMsg_u8, pQueue_->OutMsgSize_u8,
                                                                     pQueue_->WaitForInMsg_b, pQueue_->pInMsgFlag_b,
                                                                     pQueue_->pInMsg_u8, pQueue_->pInMsgSize_u8 );
                  break;
               }
               case eDRU_SET_DATE_TIME:
               {
                  pQueue_->druStatus = ProcessDruTimeSync( pQueue_->druSysTime );
                  break;
               }

               case eDRU_WRITE_REGISTER:
               {
                  pQueue_->druStatus = ProcessDruWriteRegister( pQueue_->writeReg.Id, pQueue_->writeReg.Length, (uint8_t*) pQueue_->writeReg.Data );
                  break;
               }

               case eDRU_READ_REGISTER:
               {
                  pQueue_->druStatus = ProcessDruReadRegister( pQueue_->druRegister_u16 );
                  break;
               }
               default :
               {
                  pQueue_->druStatus = eDRU_UNSUPPORTED_OPERATION_ERROR;
                  break;
               }
            }

            /* If the queue semaphore is not pointing to NULL, post on it */
            if ( NULL != pQueue_->pSem )
            {
               OS_SEM_Post(pQueue_->pSem);
            }
            else
            {
               INFO_printf( "Queue semaphore is pointing to NULL" );
            }
         }
         /* if pQueue_ points to NULL print error message*/
         else
         {
            INFO_printf( "pQueue_ is pointing to NULL, ignoring request" );
         }
      }
      /* else, i.e. if there are no elements in ILC_DRU_DRIVER_QueueHandle, sleep to allow other tasks to enqueue */
      else
      {
         OS_TASK_Sleep( 50U );
         //OS_SEM_Pend( &DruDriverSem_, OS_WAIT_FOREVER );
      }
   }
}


/* End of file */
