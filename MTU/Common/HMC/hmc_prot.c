/* ************************************************************************* */
/******************************************************************************
 *
 * Filename:   hmc_prot.c
 *
 * Global Designator: HMC_PROTO_
 *
 * Contents: Contains the meter communication protocol interface
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
 * $Log$ KAD Created 07-20-2007
 *
 ******************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 * v0.2 - Fixed the write table data.  The data count and checksum needed to be
 *        sent to the host meter.
 * v0.3 - 101909 SMG Removed some compiler warnings.
 *                   Fixed the checksum for data sent in a Write command.
 *                   Fixed the write register data to be the size of the source
 *                     data so it shifts correctly.
 * v0.4 - 2011-07-11 DTM Fixed bug that caused the target register etc. to always be
 * written to zeros on error even if that option flag was not set.
 * Fixed bug that caused the target register to always be written even if an error occurred.
 * v0.5 - 2011-12-21 KAD - Made some functions/variables static.
 * v0.6 - 2012-04-03 KAD - Changed 'HMC_ENG_Execute' function.  The 'pData' pointer is passed as the 2nd parameter.
 *************************************************************************** */
/* ************************************************************************* */
/* INCLUDE FILES */
#include "project.h"
#include <stdbool.h>
#define HMC_PROTO_GLOBAL
#include "hmc_prot.h"
#undef HMC_PROTO_GLOBAL

#include "hmc_msg.h"
#include "hmc_eng.h"
#include "byteswap.h"
#include "DBG_SerialDebug.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/*lint -esym(750,HMC_PROT_PRNT_INFO,HMC_PROT_PRNT_HEX_INFO,HMC_PROT_PRNT_WARN,HMC_PROT_PRNT_ERROR)   not used */
#if ENABLE_PRNT_HMC_PROT_INFO
#define HMC_PROT_PRNT_INFO( a, fmt,... )  DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define HMC_PROT_PRNT_HEX_INFO( a, fmt,... )   DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_PROT_PRNT_INFO( a, fmt,... )
#define HMC_PROT_PRNT_HEX_INFO( a, fmt,... )
#endif
#if ENABLE_PRNT_HMC_PROT_WARN
#define HMC_PROT_PRNT_WARN( a, fmt,... )  DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_PROT_PRNT_WARN( a, fmt,... )
#endif
#if ENABLE_PRNT_HMC_PROT_ERROR
#define HMC_PROT_PRNT_ERROR( a, fmt,... ) DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_PROT_PRNT_ERROR( a, fmt,... )
#endif

/* ************************************************************************* */
/* CONSTANTS */

/* Masks for Decode data */
#define HMC_PROTO_MEM_MASK    ((uint8_t)7)

//lint -esym(753,HMC_PROTO_STATE)
//lint -esym(749,HMC_PROTO_STATE::*)
enum  HMC_PROTO_STATE
{
   HMC_PROTO_SESSION_OPEN,
   HMC_PROTO_CMD_START,
   HMC_PROTO_PROCESSING,
   HMC_PROTO_IDLE
};

/* ************************************************************************* */
/* MACRO DEFINITIONS */

/* ************************************************************************* */
/* TYPE DEFINITIONS */

/* ************************************************************************* */
/* FILE VARIABLE DEFINITIONS */

/* ************************************************************************* */
/* FUNCTION PROTOTYPES */

static uint8_t AssembleCommand(uint8_t *, HMC_PROTO_Table far const *);
static uint16_t DecodeResponse(HMC_COM_INFO *, HMC_PROTO_Table far const *);

/* ************************************************************************* */
/* FUNCTION DEFINITIONS */

/* ************************************************************************* */
/******************************************************************************
 *
 * Function name: HMC_PROTO_Protocol()
 *
 * Purpose: Handles the session to the meter.  Commands are sent to this
 *          function to talk to the meter.  This function does the following:
 *
 *  1.  Handles the control byte
 *  2.  Increments the Sequence numbers (goes through the commands)
 *
 * Arguments: uint8_t - Command, HMC_COM_INFO * pPointer to the communications structures
 *
 * Returns: uint16_t - STATUS in 'HMC_PROTO_STATUS' structure format
 *
 *****************************************************************************/
uint16_t HMC_PROTO_Protocol(uint8_t ucCmd, HMC_COM_INFO *pData)
{
   static HMC_PROTO_Table far const *pTableIndex_; /* Pointer to the Protocol Table */
   static bool                      bProcessData_; /* Process the response from the message layer data flag */
   static HMC_PROTO_STATUS          status_ = {0}; /* Status of the Protocol Function */ /*lint !e708 */
   static uint8_t                   psem_IdByte_;  /* Contains the PSEM ID byte */

   HMC_MSG_STATUS                   sMMPStatus;    /* Result of the message processor */
   uint8_t                          ucNumOfBytes;  /* Number of bytes to send to the host meter */
   uint8_t                          ucByteCount;   /* Used for counting bytes shifted for write operations */
   uint8_t                          ucResponseType;/* Type of response packet expected from the host meter */
   /* the following are for the write command to the host meter */
   uint8_t                          *pWrData;      /* Location of the data */
   uint8_t                          *pWrCount;     /* Location of the count - For sending full table writes. */
   uint8_t                          *pWrChecksum;  /* Location of the checksum - In the data field. */
   uint16_BA                        uiCount;       /* Count to write to the count location */

   switch(ucCmd)
   {
      case (uint8_t)HMC_PROTO_CMD_INIT: /* If the Initialize command was sent, initialize all variables */
      {
         status_.uiStatus = 0;   /* Clear the status bytes */
         status_.Bits.bInitialized = true;   /* Set the Initialized Bit */
         pData->TxPacket.ucControl = 0x20;   /* Start with 0x20 (1st time toggle will send 0x00) */
         bProcessData_ = false;   /* Clear the Process data, this will be set after a message is sent. */
         pData->TxPacket.ucSequenceNum = 0;  /* Initialize the Sequence Number */
         (void)HMC_MSG_Processor((uint8_t)HMC_MSG_CMD_INIT, pData);  /* Initialize the Message Layer */
         psem_IdByte_ = PSEM_UNIVERSAL_ID;  /* Set to universal ID */
         break;
      }
      case (uint8_t)HMC_PROTO_CMD_NEW_SESSION:
      {
         (void)HMC_MSG_Processor((uint8_t)HMC_MSG_CMD_NEW_SESSION, pData);
         status_.uiStatus = 0;   /* New Session, clear the status. */
         break;
      }
      case (uint8_t)HMC_PROTO_CMD_SET_ID: /* Sets the ID Byte, for 'standard' transponders this may never be used */
      {
         psem_IdByte_ = *((uint8_t *)pData);
         break;
      }
      case (uint8_t)HMC_PROTO_CMD_READ_COM_PARAM:  /* Retrieves the location of the communication parameters */
      {
         (void)HMC_MSG_Processor((uint8_t)HMC_MSG_CMD_RD_COM, pData);
         break;
      }
      case (uint8_t)HMC_PROTO_CMD_WRITE_COM_PARAM:  /* Retrieves the location of the communication parameters */
      {
         (void)HMC_MSG_Processor((uint8_t)HMC_MSG_CMD_WR_COM, pData);
         break;
      }
      case (uint8_t)HMC_PROTO_CMD_SET_DEFAULT_COM:
      {
         (void)HMC_MSG_Processor((uint8_t)HMC_MSG_CMD_SET_DEFAULTS, pData);
         break;
      }
      case (uint8_t)HMC_PROTO_CMD_READ_COMPORT:
      {
         (void)HMC_MSG_Processor((uint8_t)HMC_MSG_CMD_RD_COMPORT, pData);
         break;
      }
      case (uint8_t)HMC_PROTO_CMD_WRITE_COMPORT:
      {
         (void)HMC_MSG_Processor((uint8_t)HMC_MSG_CMD_WR_COMPORT, pData);
         break;
      }
      case (uint8_t)HMC_PROTO_CMD_NEW_CMD:
      {
         /* New command, so prepare the lower layer for new com */
         (void)HMC_MSG_Processor((uint8_t)HMC_MSG_CMD_CLR_FLAGS, pData);
         status_.uiStatus = 0;   /* Clear all status flags */
         status_.Bits.bBusy = true; /* Remains set until the table is processed */
         bProcessData_ = false;   /* Will be set AFTER a packet is sent to the lower layer */
         pTableIndex_ = (HMC_PROTO_Table far *)(pData->pCommandTable);   /* Set table index pointer */
         break;   /* Next time the protocol layer gets called, the data will be sent to the message layer */
      }
      case (uint8_t)HMC_PROTO_CMD_PROCESS:
      {
         /* Update the message command process */
         sMMPStatus.uiStatus = HMC_MSG_Processor((uint8_t)HMC_MSG_CMD_PROCESS, pData);
//         DBG_logPrintf('R', "Prot layer" );
         status_.Bits.bCPUTime = sMMPStatus.Bits.bCPUTime;
         /* Process Results from the message handler, if any */
         if (!sMMPStatus.Bits.bBusy)   /* Not Busy? */
         {
            if (bProcessData_)
            {
               /* Was there an error from the message layer? */
               if (sMMPStatus.Bits.bComError || sMMPStatus.Bits.bDataOverFlow)
               {
                  status_.Bits.bComFailure = true;   /* Need to report an ERROR */
                  status_.Bits.bBusy = false;        /* Abort the protocol layer by Clearing the Busy flag */
                  pData->RxPacket.ucResponseCode = RESP_COM_ERROR;
               }
               /* A NAK from the message layer means we have to restart the communications */
               else if (sMMPStatus.Bits.bNAK || sMMPStatus.Bits.bToggleBitError || sMMPStatus.Bits.bTimeOut)
               {
                  status_.Bits.bRestart = true;   /* Need to report an ERROR, and restart session */
                  status_.Bits.bBusy = false;   /* Abort the protocol layer by Clearing the Busy flag */
                  pData->RxPacket.ucResponseCode = RESP_COM_ERROR;
               }
               /* Increment Engineering Counter/Registers */
               HMC_ENG_Execute((uint8_t)HMC_ENG_PCKT_RESP, pData);

               status_.Bits.bCPUTime = true; /* Data is going to be processed, CPU time will be taken. */
               status_.uiStatus |= DecodeResponse(pData, pTableIndex_); /* Save the data */
               if (RESP_TYPE_TABLE_DATA == pData->RxPacket.ucRespType)
               {
                  /* Was there a problem reading the table information? */
                  if (RESP_OK != pData->RxPacket.ucResponseCode)
                  {
                     status_.Bits.bTblError = true;   /* Need to report an ERROR */
                     status_.Bits.bBusy = false;
                  }
               }
               if (sMMPStatus.Bits.bComplete)  /* bComplete means no packet errors. */
               {
                  /* If the HMC_MSG process is done AND we are suppose to process data */
                  bProcessData_ = false;
                  status_.Bits.bMsgComplete = true;
                  if (status_.Bits.bBusy) /* If the protocol layer has NOT aborted, then increment the Table Index */
                  {
                     pTableIndex_++;  /* Increment pointer to the next item to process */
                  }
               }
               if (!status_.Bits.bBusy)
               {
                  bProcessData_ = false;
               }
            }

            /* If not busy, the application layer will be notified and respond to the applet.  If still
               busy, there is possibly another command to send to the host meter. */

            /* Since the status could have changed, re-read the Message Layer status. */
            sMMPStatus.uiStatus = HMC_MSG_Processor((uint8_t)HMC_MSG_CMD_CLR_FLAGS, pData);

            /* *** Start Page 2 of the Design Document *** */

            /* Process the current state if the protocol layer is still Busy and the Message Layer is NOT in
               Turn-Around delay mode.  */

            if (status_.Bits.bBusy && !sMMPStatus.Bits.bTxWait)
            {
               /* Clear the TX buffer.  This is only here to make debugging easier.  It could be removed or
                  conditionally compiled in.  Leaving this code will not hurt anything!  */
#if 0
               memset(&((HMC_COM_INFO *)pData)->TxPacket.uTxData, 0, sizeof(((HMC_COM_INFO *)pData)->TxPacket.uTxData));
#endif
               /* Assemble the command */
               ucNumOfBytes = AssembleCommand((uint8_t *)&pData->TxPacket.uTxData, pTableIndex_);
               status_.Bits.bCPUTime = true; /* Since we are accessing lower layer of code with a command, Set True */
               if (0 != ucNumOfBytes)  /* Anything to send? */
               {
                  ucResponseType = RESP_TYPE_NILL; /* Assume the NILL */
                  if (CMD_NEG_SERVICE_REQ == pData->TxPacket.uTxData.sReadFull.ucServiceCode)
                  {
                     ucResponseType = RESP_TYPE_NEGOTIATE0;
                  }
                  else if ( (pData->TxPacket.uTxData.sReadFull.ucServiceCode >= CMD_TBL_RD_FULL &&
                             pData->TxPacket.uTxData.sReadFull.ucServiceCode <= CMD_TBL_RD_PARTIAL ) )
                  {
                     ucResponseType = RESP_TYPE_TABLE_DATA;
                  }
                  else if (   ( pData->TxPacket.uTxData.sReadFull.ucServiceCode >= CMD_TBL_WR_FULL )     &&
                              ( pData->TxPacket.uTxData.sReadFull.ucServiceCode <= CMD_TBL_WR_PARTIAL ) )
                  {
                     ucResponseType = RESP_TYPE_TABLE_DATA;
                     /* If write command, then the data needs to be shifted 2-bytes to the right for the length field */
                     /* and a checksum needs to be appended to data. The ucNumOfBytes needs to be incremented by 3 */
                     if (CMD_TBL_WR_PARTIAL == pData->TxPacket.uTxData.sReadFull.ucServiceCode )
                     {
                        /* Set for partial table writes */
                        pWrData = &pData->TxPacket.uTxData.sWritePartialWithBuffer.sTxBuffer[0];
                        ucByteCount = (ucNumOfBytes -
                              sizeof(pData->TxPacket.uTxData.sWritePartialWithBuffer.sWritePartial));
                        pWrChecksum = pWrData + ucByteCount; /* Set the checksum to the last location of data */
                        for (*pWrChecksum = 0; 0 != ucByteCount; ucByteCount--, pWrData++)   /* Calc Checksum */
                        {
                              *pWrChecksum += *pWrData;
                        }
                        ucNumOfBytes += sizeof(*pWrChecksum);   /* Compensate for the checksum */
                     }
                     else
                     {
                        /* Set up for full table writes */
                        pWrData = &pData->TxPacket.uTxData.sWriteFullWithBuffer.sTxBuffer[0];
                        pWrCount = (uint8_t *)&pData->TxPacket.uTxData.sWriteFullWithBuffer.uiCount;
                        ucByteCount = (ucNumOfBytes -
                                    sizeof(pData->TxPacket.uTxData.sWriteFullWithBuffer.sWriteFull));
                        uiCount.n16 = (uint16_t)ucByteCount;  /* save byte count to write after data has been moved */
                        pWrChecksum = pWrData + ucByteCount; /* Set the checksum to the last location of data */
                        pWrData = pWrChecksum - 1;  /* Set the data pointer directly before the checksum byte */
                        for (*pWrChecksum = 0; ucByteCount; ucByteCount--, pWrData--)
                        {
                           /* Update the checksum */
                           *pWrChecksum += *(pWrData -
                                       sizeof(pData->TxPacket.uTxData.sWriteFullWithBuffer.uiCount));
                           /* Copy the data over to the "sWriteFullWithBuffer" after the count field. */
                           *pWrData = *(pWrData -
                                       sizeof(pData->TxPacket.uTxData.sWriteFullWithBuffer.uiCount));
                        }
                        /* Write the data count, this is sent in big endian formation */
                        *pWrCount = uiCount.LE.Msb;   /* MSB is sent first */
                        pWrCount++;
                        *pWrCount = uiCount.LE.Lsb;   /* Then LSB is sent */
                        /* Adjust total number of bytes in the packet, account for the count field and checksum field */
                        ucNumOfBytes += sizeof(pData->TxPacket.uTxData.sWriteFullWithBuffer.uiCount) +
                                        sizeof(*pWrChecksum);
                     }
                     /* Copy the data starting at the end.  Using memcpy could corrupt the data, must use a for loop */
                     /* Need 2's complement checksum */
                     *pWrChecksum = (uint8_t)(-(int8_t)*pWrChecksum);
                  }
                  bProcessData_ = true;                        /* When the message layer is done, process the results */
                  pData->TxPacket.ucSTP = PSEM_STP;            /* Always set the start byte to EE */
                  pData->TxPacket.ucId = psem_IdByte_;
                  pData->TxPacket.uiPacketLength.BE.Msb = 0;
                  pData->TxPacket.uiPacketLength.BE.Lsb = ucNumOfBytes;/* # of bytes being sent (payload) */
                  pData->TxPacket.ucControl &= PSEM_CTRL_MASK;    /* Ensure the control is correct */
                  pData->TxPacket.ucControl ^= PSEM_CTRL_TOGGLE;  /* Toggle the PSEM Control bit 5 */
                  pData->RxPacket.ucRespType = ucResponseType;
                  (void)HMC_MSG_Processor((uint8_t)HMC_MSG_CMD_SEND_DATA, pData);  /* Send the data */
               }
               else
               {
                  status_.Bits.bBusy = false;   /* Nothing to send, set status to NOT busy */
               }
            }
         }
         break;
      }
      default:
      {
         status_.Bits.bInvalidCmd = true;                                 /* Nothing to send, set status to NOT busy */
         break;
      }  /* Not a valid command */
   }

   return(status_.uiStatus);  /* Return the status as a uint16_t */
}
/* ************************************************************************* */
/******************************************************************************
 *
 * Function name: AssembleCommand()
 *
 * Purpose: Assembles a command (loads data from a location in memory).  If the
 *          end of the table is reached, then 0 bytes are returned to indicate
 *          there is nothing to send.
 *
 * Arguments: uint8_t * Buffer to write to, HMC_PROTO_Table far *TableToReadFrom
 *
 * Returns: uint8_t - Number Of Bytes (0 - Send nothing)
 *
 *****************************************************************************/
static uint8_t AssembleCommand(uint8_t *pBuffer, HMC_PROTO_Table far const *pStructData)
{
   const HMC_PROTO_TableCmd far  *pCmdPtr;      /* Command Pointer */
   uint8_t                  far  *pAdr;         /* Pointer to Address */
   bool                          bComplete;
   uint8_t                       ucSize;
   uint8_t                       ucTotalBytes = 0;
   uint8_t                       ucMemType;
#if ENABLE_PRNT_HMC_PROT_INFO
   uint8_t                       *locpBuffer = pBuffer;  /* For printing pBuffer */
#endif

   pCmdPtr = pStructData->pData;    /* Set Command Pointer */

   bComplete = false;

   if (pCmdPtr != (void *)NULL)
   {
      HMC_PROT_PRNT_INFO( 'H', "Assemble: tbl len: %hu", pCmdPtr->ucLen );
      (void)memset(pBuffer, 0, sizeof(tWritePartial));
      do  /* Repeat until all data is loaded */
      {
         ucMemType = (uint8_t)pCmdPtr->eMemType;   /* Load Memory Type */
         pAdr = pCmdPtr->pPtr;   /* Load Address */
         ucSize = pCmdPtr->ucLen;   /* Load Size */
         if (!(ucMemType & HMC_PROTO_MEM_WRITE))
         {
            switch(ucMemType & HMC_PROTO_MEM_MASK) /* Memory Type is Valid? */
            {
               case HMC_PROTO_MEM_NULL:  /* End of commands?  */
               {
                  bComplete = true;
                  break;
               }
               case HMC_PROTO_MEM_FILL:  /* Fill memory? */  // RCZ not used
               {
                  (void)memset(pBuffer, (int32_t)pAdr, ucSize);
                  break;
               }
#if 0
               case HMC_PROTO_MEM_REG: /* Read a register? */
               {
                  /* Get Register Data */
                  /* Find the register and get the register info (if register not being mapped, user error!) */
                  uRegResponse regResp;
                  regResp.uiRetVal = REG_Read((uint16_t)((uint32_t)pAdr), ucSize, pBuffer);
                  if (!regResp.sFlags.bRegFound)   /* Does the register exist? */
                  {
                     /* Register Not Found, so do nothing here */
                     ucTotalBytes = 0; /* Do nothing, the register doesn't exist */
                     bComplete = true;
                     HMC_ENG_Execute((uint8_t)HMC_ENG_INV_REG, (void *)NULL);
                  }
                  break;
               }
#endif
               case HMC_PROTO_MEM_RAM:
               {
                  (void)memcpy(pBuffer, (uint8_t *)pAdr, ucSize);
                  break;
               }
#if 0
               case HMC_PROTO_MEM_NV:
               {
                  if (NV_read(pBuffer, (nvAddr)pAdr, (nvSize)ucSize))
                  {
                     ucTotalBytes = 0; /* Do nothing, Read Error! */
                  }
                  break;
               }
#endif
               case HMC_PROTO_MEM_FILE:
               {
                  HMC_PROTO_file_t *pFileInfo = (HMC_PROTO_file_t *)pAdr;  /*lint !e826 */
                  if (eSUCCESS != FIO_fread(pFileInfo->pFileHandle, pBuffer, pFileInfo->offset, (nvSize)ucSize))
                  {
                     ucTotalBytes = 0; /* Do nothing, Read Error! */
                  }
                  break;
               }
               case HMC_PROTO_MEM_PGM:
               {
                  (void)memcpy(pBuffer, pAdr, ucSize);
                  break;
               }
               default:
               {
                  bComplete = true;
                  break; /* Invalid command code, just return with what we have */
               }
            }
         }
         else
         {
            bComplete = true;
         }
         if (ucMemType & HMC_PROTO_MEM_ENDIAN)
         {
            Byte_Swap(pBuffer, ucSize);  /* Switch Endian! */
         }

         if (!bComplete)
         {
            /* Increment pointers to fill the next part of data */
            ucTotalBytes += ucSize;
            pBuffer += ucSize;
            pCmdPtr++;  /* Next command */
         }
      }while(!bComplete);
      /* Extra spaces below make the string align with Send string in hmc_msg printout */
      HMC_PROT_PRNT_HEX_INFO( 'H', "Assemble:        ", locpBuffer, ucTotalBytes );
   }
   return (ucTotalBytes);
}
/* ************************************************************************* */
/******************************************************************************
 *
 * Function name: DecodeResponse()
 *
 * Purpose: Decodes and writes to memory.  Can decode many bytes into multiple
 *          registers or memory locations.
 *
 * Arguments: HMC_COM_INFO *, HMC_PROTO_Table far *TableToReadFrom
 *
 * Returns: uint16_t
 *
 *****************************************************************************/
static uint16_t DecodeResponse(HMC_COM_INFO *ucCmd, HMC_PROTO_Table far const *pStructData)
{
   uint8_t                       *pBufferData;
   const HMC_PROTO_TableCmd far  *pCmdPtr;
   uint8_t far                   *pAdr;
   uint16_t                      size;
   enum HMC_PROTO_MEM_TYPE       eType;
   uint8_t                       ucMemType;
   HMC_PROTO_STATUS              sRetVal;

   sRetVal.uiStatus = 0;

	// DG: Print the Response code as sRxData will be null for RESP_TYPE_NILL.
   HMC_PROT_PRNT_HEX_INFO( 'H', "Response code:   ", &ucCmd->RxPacket.ucResponseCode, sizeof( ucCmd->RxPacket.ucResponseCode ) );
   pBufferData = ucCmd->RxPacket.uRxData.sRxData;
   size        = ucCmd->RxPacket.uiPacketLength.n16;   /* Get length of meter response   */
   if (ucCmd->RxPacket.ucRespType == RESP_TYPE_TABLE_DATA)
   {
      pBufferData = ucCmd->RxPacket.uRxData.sTblData.sRxBuffer;
      size = ucCmd->RxPacket.uRxData.sTblData.uiCount;   /* Get length of table data read   */
      HMC_PROT_PRNT_INFO( 'H', "pkt len: %hu, pkt count: %hu   ", ucCmd->RxPacket.uiPacketLength.n16, size );
      //  There is more to the message than just the response code
      HMC_PROT_PRNT_HEX_INFO( 'H', "Decode:          ", pBufferData, size );
      HMC_PROT_PRNT_HEX_INFO( 'H', "Decode:          ", pBufferData, ucCmd->RxPacket.uiPacketLength.n16 );
   }

   pCmdPtr = pStructData->pData;

   if ((void *)NULL != pCmdPtr)
   {
      for( ; ; )
      {
         pAdr = pCmdPtr->pPtr;
         eType = pCmdPtr->eMemType;

         if (eType && size)
         {
            if ((uint8_t)eType & HMC_PROTO_MEM_WRITE) /* Write Data? */
            {
               /* Com Error and Zero Fill data on Error? */
               if ( (ucCmd->RxPacket.ucResponseCode != RESP_OK) && ((uint8_t)eType & HMC_PROTO_MEM_ZF) )
               {
                  // Set the "result" to all zeros, which will be written into the destination below.
                  (void)memset(pBufferData, 0, size);
               }

               /* Com ok OR bad and we want to write zeros? */
               if(  (ucCmd->RxPacket.ucResponseCode == RESP_OK ) || ((uint8_t)eType & HMC_PROTO_MEM_ZF ) )
               {
                  // Either response was ok, OR it was an error response and we want to write zeros to the destination.
                  if ((uint8_t)eType & HMC_PROTO_MEM_ENDIAN)
                  {
                     Byte_Swap(pBufferData, (uint8_t)size);  /* Switch Endian! */
                  }
                  ucMemType = (uint8_t)((uint8_t)eType & HMC_PROTO_MEM_MASK);  /* Only look at the memory type */
#if 0
                  if (ucMemType == (uint8_t)HMC_PROTO_MEM_REG)
                  {
                     uRegResponse regResp;
                     regResp.uiRetVal = REG_Write((uint16_t)((uint32_t)pAdr), size, pBufferData);
                     if (!regResp.sFlags.bRegFound)
                     {
                        sRetVal.Bits.bInvRegister = true;
                        break;
                     }
                     if (regResp.sFlags.bReqLenLong)
                     {
                        sRetVal.Bits.bGenError = true;
                        break;
                     }
                  }
#endif
                  if (ucMemType == (uint8_t)HMC_PROTO_MEM_RAM)
                  {
                     (void)memcpy(pAdr, pBufferData, size);
                  }
#if 0
                  else if (ucMemType == (uint8_t)HMC_PROTO_MEM_NV)
                  {
                     if (NV_write((nvAddr)pAdr, pBufferData, (nvSize)size))
                     {
                        sRetVal.Bits.bGenError = true;
                        break;
                     }
                  }
#endif
                  else if (ucMemType == (uint8_t)HMC_PROTO_MEM_FILE)
                  {
                     HMC_PROTO_file_t *pFileInfo = (HMC_PROTO_file_t *)pAdr;  /*lint !e826 */
                     if (eSUCCESS != FIO_fwrite(pFileInfo->pFileHandle, pFileInfo->offset, pBufferData, (lCnt)size))
                     {
                        sRetVal.Bits.bGenError = true;
                        break;
                     }
                     break;
                  }
               }
               break;
            }
         }
         else
         {
            break;
         }
         pCmdPtr++;
      }
   }
   return(sRetVal.uiStatus);
}
/* ************************************************************************* */
