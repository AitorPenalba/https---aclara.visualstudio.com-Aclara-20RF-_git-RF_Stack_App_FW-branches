/***********************************************************************************************************************

   Filename: HMC_REQ_.c

   Global Designator: HMC_REQ_

   Contents: Receives a message from a mailbox, performs HMC and then responds via a mailbox send or posts a semaphore.

 ***********************************************************************************************************************
   Copyright (c) 2013 - 2021 Aclara.  All rights reserved. This program may not be reproduced, in
   whole or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
   Revision History:
   v1.0 - Initial Release
 ******************************************************************************************************************** */

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include <stdbool.h>
#include <string.h>
#include "psem.h"
#if ( ANSI_STANDARD_TABLES == 1 )
#include "ansi_tables.h"
#endif
#include "hmc_eng.h"
#include "hmc_request.h"
#include "hmc_mtr_info.h"
#include "hmc_snapshot.h"
#include "byteswap.h"
#include "buffer.h"
#include "time_util.h"
#include "DBG_SerialDebug.h"

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
#include "hmc_prg_mtr.h"
#endif


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef union
{
   tReadPartial  rdPart;       /* Table parameters to read */
   tReadFull     rdFull;       /* Table parameters to read */
   tWritePartial wrPart;       /* Table parameters to write */ /*lint -esym(754,wrPart)  May not be referenced */
   tWriteFull    wrFull;       /* Table parameters to write */
} tblData_t;

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLE DEFINITIONS */

OS_QUEUE_Obj   HMC_REQ_queueHandle;
uint8_t        HMC_REQ_queueCreated = false;

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static buffer_t                     *pTableData_ = NULL;
static HMC_PROTO_TableCmd           cmdProto_[3];  /* Table Command for protocol layer. */
#if ( ANSI_STANDARD_TABLES == 1 )
static MtrTimeFormatHighPrecision_t stopDate;      /* Meter's date time structure   */
#endif

static HMC_REQ_queue_t     *pQueue_;

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define HMC_REQ_RETRIES          ((uint8_t)0)         /* Number of times to retry on an error */
/*lint -esym(750, HMC_REQ_PRNT_WARN, HMC_REQ_PRNT_HEX_WARN, HMC_REQ_PRNT_HEX_ERROR) */

#if ENABLE_PRNT_HMC_REQ_INFO
#define HMC_REQ_PRNT_INFO( a, fmt,... )      DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define HMC_REQ_PRNT_HEX_INFO( a, fmt,... )  DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_REQ_PRNT_INFO( a, fmt,... )
#define HMC_REQ_PRNT_HEX_INFO( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_REQ_WARN
#define HMC_REQ_PRNT_WARN( a, fmt,... )      DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define HMC_REQ_PRNT_HEX_WARN( a, fmt,... )  DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_REQ_PRNT_WARN( a, fmt,... )
#define HMC_REQ_PRNT_HEX_WARN( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_REQ_ERROR
#define HMC_REQ_PRNT_ERROR( a, fmt,... )     DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define HMC_REQ_PRNT_HEX_ERROR( a, fmt,... ) DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_REQ_PRNT_ERROR( a, fmt,... )
#define HMC_REQ_PRNT_HEX_ERROR( a, fmt,... )
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */

static const HMC_PROTO_TableCmd  cmdProtoDefault_[] =      /* Table Command for protocol layer. */
{
   { HMC_PROTO_MEM_RAM, 0, NULL },     /* Just a template for initializing cmdProto. Specifics added later  */
   { HMC_PROTO_MEM_NULL },
   { HMC_PROTO_MEM_NULL }
};

static HMC_PROTO_Table hmcTbl_[] =
{
   &cmdProto_[0], /* Used for reads */
   NULL,          /* Used for writes with Stop programming procedure.   */
   NULL
};
#if ( ANSI_STANDARD_TABLES == 1 )
static const tWriteFull stopProcedure[] =
{
   {  /* Meter Display Information */
      CMD_TBL_WR_FULL,
      BIG_ENDIAN_16BIT( STD_TBL_PROCEDURE_INITIATE ), /* ST.Tbl 7 will be written  */
   }
};
static const uint8_t stopData[] =
{
   ( uint8_t )( MFG_PROC_PGM_CTRL & 0xff ),  /* Manufacturer Procedure */
   ( uint8_t )( MFG_PROC_PGM_CTRL >> 8 ),    /* Manufacturer Procedure */
   0xcc,                                     /* Selector ID for the procedure. */
   3                                         /* Stop programming, no clear, no log off */
};
static const HMC_PROTO_TableCmd stopProgramming[] =
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof ( stopProcedure ),   ( uint8_t far * )&stopProcedure[0]  },
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof ( stopData ),        ( uint8_t far * )&stopData[0]       },
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof ( stopDate ) - 1,    ( uint8_t far * )&stopDate          },
};
#endif
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/* ****************************************************************************************************************** */

/* *********************************************************************************************************************

   Function name: HMC_REQ_applet

   Purpose:  Requests HMC table access.  This module can read/write full or partial tables.

   Arguments: uint8_t ucCmd, void *pData

   Returns: uint8_t  - See cfg_app.h enum 'HMC_APP_API_RPLY' for response codes.

 ******************************************************************************************************************** */
uint8_t HMC_REQ_applet( uint8_t ucCmd, void *pData )
{
          uint8_t  retVal = ( uint8_t ) HMC_APP_API_RPLY_IDLE; /* Assume module is idle. */
   static uint16_t totalBytesToRead_ = 0;                      /* Keeps track of the total # of bytes to read (partial access) */
   static uint32_t offset_ = 0;                                /* Keeps track of the offset for partial table access. */
   static uint16_t cnt_ = 0;                                   /* Number of bytes read/written for partial table access. */
   static uint8_t  *pDest;
   static bool     writeOperation_ = (bool)false;              /* Write/procedure operation in progress. No returned data. */
   static uint8_t  retryCnt_ = HMC_REQ_RETRIES;                /* Number of application level retries */

/*
   The sequence of calls to this applet, when no read/write/procedure errors occur, is HMC_APP_API_CMD_STATUS, then
   HMC_APP_API_CMD_PROCESS, and finally HMC_APP_API_CMD_MSG_COMPLETE.
   Retries are needed if a Table error occurred, which causes the app to send HMC_APP_API_CMD_TBL_ERROR to this applet.
   If a NAK or timeout occurred, which causes the app to send HMC_APP_API_CMD_ABORT to this applet, the message level has
   already performed retries, so retrying has been removed from HMC_APP_API_CMD_ABORT.
   If a toggle bit error occurred, the message level records the error without retrying. The app sends HMC_APP_API_CMD_ABORT
   to this applet, and no retries are performed.
   A failed File write results in the app sending HMC_APP_API_CMD_ERROR.
*/

#if 0
   HMC_REQ_PRNT_INFO( 'I', "HMC_REQ_applet, cmd: %d, pData: 0x%08x", ucCmd, (uint32_t)pData );
#endif

   switch( ucCmd )                                 /* Command passed in */
   {
      case ( uint8_t )HMC_APP_API_CMD_INIT:          /* Initialize the applet? */
      {
         if ( !HMC_REQ_queueCreated ) // RCZ added for second instance of this applet
         {
            HMC_REQ_queueCreated = OS_QUEUE_Create( &HMC_REQ_queueHandle );
         }

         totalBytesToRead_ = 0;
         offset_ = 0;
         break;
      }
      case ( uint8_t )HMC_APP_API_CMD_STATUS:        /* Command to check to see if the applet needs communication */
      {
         if ( ( OS_QUEUE_NumElements( &HMC_REQ_queueHandle ) != 0 ) || ( 0 != totalBytesToRead_ ) )
         {
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
            if ( !HMC_PRG_MTR_IsSynced( ) )
#endif
            {
               ( void )HMC_SS_applet( ( uint8_t )HMC_APP_API_CMD_ACTIVATE, NULL ); /* Do a snapshot before reading data */
               if ( eHMC_SS_SNAPPED == HMC_SS_isSnapped() )
               {
                  retVal = ( uint8_t ) HMC_APP_API_RPLY_READY_COM;
               }
            }
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
            else
            {
               retVal = ( uint8_t ) HMC_APP_API_RPLY_RDY_COM_PERSIST;
            }
         }
         else // RCZ added
         {
            if ( HMC_PRG_MTR_IsSynced( ) ) // RCZ added
            {
               retVal = ( uint8_t ) HMC_APP_API_RPLY_RDY_COM_PERSIST; // RCZ added
            }
#endif
         }
         break;
      }
      case ( uint8_t )HMC_APP_API_CMD_PROCESS:       /*  */
      {
         if ( eHMC_SS_SNAPPED == HMC_SS_isSnapped() )
         {
            if ( 0 == totalBytesToRead_ )
            {
               pQueue_ = OS_QUEUE_Dequeue ( &HMC_REQ_queueHandle );
#if 0
               HMC_REQ_PRNT_INFO( 'I', "dequeue. op:%d, id: %d, off: %d, len: %d, rsp queue: 0x%08x",
                                    pQueue_->bOperation, pQueue_->tblInfo.id, pQueue_->tblInfo.offset,
                                    pQueue_->tblInfo.cnt, pQueue_->pSem );
#endif
               if ( NULL != pQueue_ )
               {
                  ( void )memcpy( &cmdProto_[0], &cmdProtoDefault_[0], sizeof( cmdProto_ ) );
                  pDest = ( uint8_t * )pQueue_->pData;
                  pQueue_->rdLen = 0;
                  pQueue_->hmcStatus = eHMC_NOT_PROCESSED;
                  pQueue_->tblResp = 0xFF; /* Set to the extreme limit */
                  offset_ = pQueue_->tblInfo.offset;
                  totalBytesToRead_ = pQueue_->tblInfo.cnt;
                  cnt_ = min( totalBytesToRead_, HMC_REQ_MAX_BYTES );
                  switch ( pQueue_->bOperation )
                  {
                     case eHMC_READ:   /* Partial table read and Partial table write structures are identical  */
                     case eHMC_WRITE:
                     {
                        pTableData_ = BM_alloc( sizeof( tblData_t ) );
#if 0
                        HMC_REQ_PRNT_HEX_INFO( 'I', "dequeue data: ", pQueue_->pData, pQueue_->maxDataLen );
                        HMC_REQ_PRNT_INFO( 'I', "Alloc %d, %d, 0x%08x", ucCmd, pQueue_->bOperation,
                                             (uint32_t)pTableData_ );
#endif
                        if( pTableData_ != NULL )
                        {
                           tblData_t *ptblData = ( tblData_t * )( void * )pTableData_->data;
                           cmdProto_[0].ucLen = sizeof( tblData_t );
                           cmdProto_[0].pPtr = ( void * )ptblData;
                           /* Copy table information */
                           ( void )memcpy( &ptblData->wrPart.uiTbleID, &pQueue_->tblInfo.id,
                                             sizeof( ptblData->wrPart.uiTbleID ) );
                           ( void )memcpy( &ptblData->wrPart.ucOffset[0], &offset_,
                                             sizeof( ptblData->wrPart.ucOffset ) );
                           ( void )memcpy( &ptblData->wrPart.uiCount, &cnt_, sizeof( ptblData->wrPart.uiCount ) );
                           /* Correct the endianess. */
                           Byte_Swap( ( uint8_t * ) &ptblData->rdPart.uiTbleID, sizeof( ptblData->rdPart.uiTbleID ) );
                           Byte_Swap( ( uint8_t * ) &ptblData->rdPart.ucOffset[0],
                                       sizeof( ptblData->rdPart.ucOffset ) );
                           Byte_Swap( ( uint8_t * ) &ptblData->rdPart.uiCount, sizeof( ptblData->rdPart.uiCount ) );
                           if ( pQueue_->bOperation == eHMC_READ )
                           {
                              hmcTbl_[1].pData = NULL;
                              writeOperation_ = (bool)false;
                              ptblData->rdPart.ucServiceCode = CMD_TBL_RD_PARTIAL;
                           }
                           else
                           {
#if ( ANSI_STANDARD_TABLES == 1 )
                              sysTime_t sysTime;
#endif

                              writeOperation_ = (bool)true;
#if ( ANSI_STANDARD_TABLES == 1 )
                              ptblData->wrPart.ucServiceCode = CMD_TBL_WR_PARTIAL;
                              cmdProto_[1].eMemType = HMC_PROTO_MEM_RAM;
                              cmdProto_[1].ucLen = ( uint8_t )cnt_;
                              cmdProto_[1].pPtr = pDest;
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
                              if ( HMC_PRG_MTR_IsSynced( ) ) // RCZ addition
                              {
                                 hmcTbl_[1].pData = NULL;
                              }
                              else
#endif
                              {
                                 hmcTbl_[1].pData = stopProgramming;
                                ( void )TIME_SYS_GetSysDateTime( &sysTime );                /* Gets current system date/time */
                                ( void )TIME_UTIL_ConvertSysFormatToMtrHighFormat( &sysTime, &stopDate );
                              }
#endif
                           }
                           ( ( HMC_COM_INFO * )pData )->pCommandTable = ( uint8_t far * ) & hmcTbl_[0];
                        }
                        break;
                     }
                     case eHMC_PROCEDURE: /* Initiate procedure   */
                     {
                        pTableData_ = BM_alloc( sizeof( tblData_t ) );  /* Not used; just indicate need to do work   */
                        if( pTableData_ != NULL )
                        {

                           /* pQueue_->pData MUST be a ready-to-go HMC_PROTO_Table  */
                           ( ( HMC_COM_INFO * )pData )->pCommandTable = pQueue_->pData;

                           /* First part MUST be a write; additional parts MUST be reads (e.g., read table 8).  */
                           writeOperation_ = (bool)true;
                        }

#if ENABLE_PRNT_HMC_REQ_INFO
                           HMC_PROTO_TableCmd *pTblCmd =
                              ( HMC_PROTO_TableCmd * )(( HMC_PROTO_Table * )pQueue_->pData)->pData;

                           HMC_REQ_PRNT_HEX_INFO( 'I', "HMC_PROC procedure: ", pTblCmd[1].pPtr, 2 );
                           HMC_REQ_PRNT_HEX_INFO( 'I', "HMC_PROC data     : ", pTblCmd[2].pPtr, pTblCmd[2].ucLen );
#endif

                        break;
                     }
                     case eHMC_WRITE_FULL:   /* Full table write, not supported.  */
                     case eHMC_READ_FULL:    /* Full table read,  not supported.  */
                     /* FALLTHROUGH */
                     default:                /* Error */
                     {
                        pTableData_ = NULL;
                        break;
                     }
                  }
#if 0
                  HMC_REQ_PRNT_INFO( 'I', "Req ANSI Rd: Tbl=%d, Offset=%d, Len=%d",
                                     pQueue_->tblInfo.id, pQueue_->tblInfo.offset, pQueue_->tblInfo.cnt );
#endif
               }
            }
            else  /* There are bytes left to be read from a previous request; work that first.  */
            {
               /* NOTE: 4/1/19 - Based on the test, it seemed that
                  below code('else case') was executed only when there was no buffer(from task context) to allocate.
                  The real problem was, pTableData_ was pointing to NULL. Remove the '#if' to fix the problem */
               /* TODO: DG: Rethink of a better solution to fix this problem  */
#if 0
               pTableData_ = BM_alloc( sizeof( tblData_t ) );
               if ( NULL != pTableData_)
#endif
               {
                  tblData_t *ptblData = ( tblData_t * )( void * )pTableData_->data;
                  /* Copy table information */
                  ( void )memcpy( &ptblData->rdPart.ucOffset[0], &offset_, sizeof( ptblData->rdPart.ucOffset ) );
                  ( void )memcpy( &ptblData->rdPart.uiCount, &cnt_, sizeof( ptblData->rdPart.uiCount ) );
                  /* Correct the endianess. */
                  Byte_Swap( ( uint8_t * ) &ptblData->rdPart.ucOffset[0], sizeof( ptblData->rdPart.ucOffset ) );
                  Byte_Swap( ( uint8_t * ) &ptblData->rdPart.uiCount, sizeof( ptblData->rdPart.uiCount ) );
               }
            }

            if ( ( pTableData_ != NULL ) && ( pQueue_ != NULL ) ) /* Check for need to do anything */
            {
               if ( totalBytesToRead_ <= pQueue_->maxDataLen )    /* Room for response?   */
               {
                  retVal = ( uint8_t ) HMC_APP_API_RPLY_READY_COM;   /* Let HMC know there's work to do. */
               }
               else
               {
                  pQueue_->hmcStatus = eHMC_SIZE_ERROR;  /* Insufficient room or bad request */
                  totalBytesToRead_ = 0;
                  if ( NULL != pQueue_->pSem )
                  {
                     OS_SEM_Post( pQueue_->pSem );
                  }
               }
            }
         }

         break;
      }
      case HMC_APP_API_CMD_TBL_ERROR:
      {
         HMC_COM_INFO *pkt = ( HMC_COM_INFO * )pData;
         HMC_REQ_PRNT_WARN( 'W', "Req Tbl Error: %d, retries left: %d",
                              pkt->RxPacket.ucResponseCode, retryCnt_ );
         HMC_REQ_PRNT_WARN( 'W', "tblInfo: 0x%x 0x%02hhx%02hhx%02hhx 0x%x",
                              BIG_ENDIAN_16BIT( pkt->TxPacket.uTxData.sReadPartial.uiTbleID ),
                              pkt->TxPacket.uTxData.sReadPartial.ucOffset[0],
                              pkt->TxPacket.uTxData.sReadPartial.ucOffset[1],
                              pkt->TxPacket.uTxData.sReadPartial.ucOffset[2],
                              BIG_ENDIAN_16BIT( pkt->TxPacket.uTxData.sReadPartial.uiCount ) );
         /* No point retrying if the error is one of the following OR there are no retries left. */
#if END_DEVICE_PROGRAMMING_CONFIG == 1
         if ( ( RESP_SNS == ( ( HMC_COM_INFO * )pData )->RxPacket.ucResponseCode ) ||
               ( RESP_ISC == ( ( HMC_COM_INFO * )pData )->RxPacket.ucResponseCode ) ||
               ( RESP_ONP == ( ( HMC_COM_INFO * )pData )->RxPacket.ucResponseCode ) ||
               ( RESP_IAR == ( ( HMC_COM_INFO * )pData )->RxPacket.ucResponseCode ) )
         {
            HMC_PRG_MTR_SetNotTestable ( );
         }
         if ( HMC_PRG_MTR_IsNotTestable( ) || ( 0 == retryCnt_ ) )
#else
         if (  ( RESP_SNS == pkt->RxPacket.ucResponseCode ) ||
               ( RESP_ISC == pkt->RxPacket.ucResponseCode ) ||
               ( RESP_ONP == pkt->RxPacket.ucResponseCode ) ||
               ( RESP_IAR == pkt->RxPacket.ucResponseCode ) ||
               ( 0 == retryCnt_ ) )
#endif
         {
            totalBytesToRead_ = 0;
            pQueue_->hmcStatus = eHMC_TBL_ERROR;
            pQueue_->tblResp = ( ( HMC_COM_INFO * )pData )->RxPacket.ucResponseCode;
#if 0
            HMC_REQ_PRNT_INFO( 'E', "Free %d, tblid: %d, buf: 0x%08x, isFree: %d", ucCmd, pQueue_->tblInfo.id,
                                 (uint32_t)pTableData_, ( uint8_t )pTableData_->x.flag.isFree );
#endif

            if ( NULL != pQueue_->pSem )
            {
               OS_SEM_Post( pQueue_->pSem );
            }
            if ( OS_QUEUE_NumElements( &HMC_REQ_queueHandle ) != 0 )
            {
               retVal = ( uint8_t ) HMC_APP_API_RPLY_READY_COM;
            }
            BM_free( pTableData_ );
         }
         else
         {
            retryCnt_--;
            retVal = ( uint8_t )HMC_APP_API_RPLY_ABORT_SESSION; /* Restart session to try to get the data. */
         }
         break;
      }
      case HMC_APP_API_CMD_ABORT:
      {
         if ( 0 != retryCnt_ )
         {
            retryCnt_--;
            retVal = ( uint8_t )HMC_APP_API_RPLY_ABORT_SESSION; /* Restart session to try to get the data. */
            HMC_REQ_PRNT_INFO( 'W', "Req Retry w/Abort" );
         }
         else
         {
            HMC_REQ_PRNT_ERROR( 'E', "Req Aborted" );
            totalBytesToRead_ = 0;
            pQueue_->hmcStatus = eHMC_ABORT;
            if ( NULL != pQueue_->pSem )
            {
               OS_SEM_Post( pQueue_->pSem );
            }
            if ( OS_QUEUE_NumElements( &HMC_REQ_queueHandle ) != 0 )
            {
               retVal = ( uint8_t ) HMC_APP_API_RPLY_READY_COM;
            }
#if 0
            HMC_REQ_PRNT_INFO( 'E', "Free %d, tblid: %d, buf: 0x%08x, isfree: %d", ucCmd, pQueue_->tblInfo.id,
                                 (uint32_t)pTableData_, ( uint8_t )pTableData_->x.flag.isFree );
#endif
            BM_free( pTableData_ );
         }
         break;
      }
      case HMC_APP_API_CMD_ERROR:
      {
         HMC_REQ_PRNT_ERROR( 'E', "Req Cmd Error" );
         totalBytesToRead_ = 0;
         pQueue_->hmcStatus = eHMC_ERROR;
         if ( NULL != pQueue_->pSem )
         {
            OS_SEM_Post( pQueue_->pSem );
         }
         if ( OS_QUEUE_NumElements( &HMC_REQ_queueHandle ) != 0 )
         {
            retVal = ( uint8_t ) HMC_APP_API_RPLY_READY_COM;
         }
#if 0
         HMC_REQ_PRNT_INFO( 'E', "Free %d, tblid: %d, buf: 0x%08x, isfree: %d", ucCmd, pQueue_->tblInfo.id,
                              (uint32_t)pTableData_, ( uint8_t )pTableData_->x.flag.isFree );
#endif
         BM_free( pTableData_ );
         break;
      }
      case HMC_APP_API_CMD_MSG_COMPLETE:
      {
         totalBytesToRead_ -= cnt_;
         if ( 0 != totalBytesToRead_ ) /* More to do for this request?  */
         {
            pQueue_->rdLen += ( uint8_t )cnt_;
            if ( NULL != pDest )
            {
               ( void )memcpy( pDest,
                               &( ( HMC_COM_INFO * )pData )->RxPacket.uRxData.sTblData.sRxBuffer[0],
                               ( ( HMC_COM_INFO * )pData )->RxPacket.uRxData.sTblData.uiCount );
               pDest += cnt_;
            }
            offset_ += cnt_;
            cnt_ = min( totalBytesToRead_, HMC_REQ_MAX_BYTES );
            retVal = ( uint8_t ) HMC_APP_API_RPLY_READY_COM;
         }
         else
         {
            pQueue_->hmcStatus = eHMC_SUCCESS;
            pQueue_->tblResp = ( ( HMC_COM_INFO * )pData )->RxPacket.ucResponseCode;
            if ( !writeOperation_ )
            {
               if ( ( ( HMC_COM_INFO * )pData )->RxPacket.uRxData.sTblData.uiCount <= pQueue_->maxDataLen )
               {
                  if ( NULL != pDest )
                  {
                     ( void )memcpy( pDest,
                                     &( ( HMC_COM_INFO * )pData )->RxPacket.uRxData.sTblData.sRxBuffer[0],
                                     ( ( HMC_COM_INFO * )pData )->RxPacket.uRxData.sTblData.uiCount );
                  }
               }
               else
               {
                  pQueue_->hmcStatus = eHMC_SIZE_ERROR;
               }
            }
            else
            {
#if ENABLE_PRNT_HMC_REQ_INFO
               HMC_COM_INFO *pResult = ( HMC_COM_INFO * )pData;
               sMtrTxPacket *pTx = &pResult->TxPacket;
               sMtrRxPacket *pRx = &pResult->RxPacket;

               HMC_REQ_PRNT_INFO( 'R', "pResult: 0x%08x, result code: 0x%02x", pResult, pResult->RxPacket.ucResponseCode );
               HMC_REQ_PRNT_INFO( 'R', "pTx: 0x%08x, seq: 0x%02x", pTx, pTx->ucSequenceNum );
               HMC_REQ_PRNT_INFO( 'R', "pRx: 0x%08x, seq: 0x%02x", pRx, pRx->ucSequenceNum );
#endif
               NOP();
            }
            writeOperation_ = (bool)false; /* No "chained" write operations allowed. */

            if ( OS_QUEUE_NumElements( &HMC_REQ_queueHandle ) != 0 )
            {
               retVal = ( uint8_t ) HMC_APP_API_RPLY_READY_COM;
            }

            pQueue_->rdLen += ( uint8_t )( ( HMC_COM_INFO * )pData )->RxPacket.uRxData.sTblData.uiCount;
            if ( NULL != pQueue_->pSem )
            {
               OS_SEM_Post( pQueue_->pSem );
            }
#if 0
            HMC_REQ_PRNT_INFO( 'I', "Free %d, tblid: %d, buf: 0x%08x, isfree: %d", ucCmd, pQueue_->tblInfo.id,
                                 (uint32_t)pTableData_, ( uint8_t )pTableData_->x.flag.isFree );
#endif
            BM_free( pTableData_ );
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
            if ( HMC_PRG_MTR_IsSynced( ) ) // RCZ added
            {
               retVal = ( uint8_t ) HMC_APP_API_RPLY_RDY_COM_PERSIST; // RCZ added
            }
#endif
         }
         break;
      }
      default: /* No other commands are supported, if we get here, the command was not recognized by this applet. */
      {
         retVal = ( uint8_t )HMC_APP_API_RPLY_UNSUPPORTED_CMD; /* Respond with unsupported command. */
         break;
      }
   }
#if 0
   HMC_REQ_PRNT_INFO( 'I', "HMC_REQ_applet, cmd: %d, retVal: %d", ucCmd, retVal );
#endif

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
   if ( ( retVal == ( uint8_t ) HMC_APP_API_RPLY_RDY_COM_PERSIST ) && ( !HMC_PRG_MTR_IsStarted( ) ) )
   {
      retVal = ( uint8_t )HMC_APP_API_RPLY_IDLE;
   }
#endif

   return( retVal );
}
/* ****************************************************************************************************************** */

