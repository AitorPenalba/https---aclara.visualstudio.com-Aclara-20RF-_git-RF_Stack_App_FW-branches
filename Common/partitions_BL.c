/***********************************************************************************************************************

   Filename:   partition.c

   Global Designator: PAR_

   Contents: Contains partition information for performing memory I/O.  Provides an access table to all of the memory commands.

 ***********************************************************************************************************************
   Copyright (c) 2013 - 2020 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
   or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA POWER-LINE SYSTEMS INC.
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
   Revision History:

   20110415, KAD - Initial Release
   20130205, MS - Changed flush routine. If the parameter passed is NULL, flush all partitions
   20130625, MS - Excluded meta-data from logical read/write/erase/restore to a partition.
   20150402, mkv - Fixed comparison of partition under test being whole device. Format change to narrow print out.
   20170330, mkv - No longer allow NULL pointer passed to the erase function.

 **********************************************************************************************************************/
/* INCLUDE FILES */

#include "project_BL.h"
#include <stdbool.h>
#include <string.h>
#ifndef __BOOTLOADER
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#endif
#endif /* __BOOTLOADER  */
#define partitions_GLOBAL
#include "partitions_BL.h"
#include "partition_cfg_BL.h"
#ifndef __BOOTLOADER
#ifdef TM_PARTITION_TBL
#include "DBG_SerialDebug.h"
#endif
#ifdef TM_DVR_PARTITION_UNIT_TEST
#include "unit_test_partition.h"
#endif
#endif /* __BOOTLOADER  */

#undef  partitions_GLOBAL

#include "file_io.h"

#ifndef __BOOTLOADER
#include "time_sys.h"
#include "timer_util.h"
#ifdef PARTITION_TABLE_LOG_ENABLE
#include "logger.h"
#include "os_aclara.h"
#endif
#endif /* __BOOTLOADER  */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* The partition open command may search for timeing that best fits the timing requirements until the ID exceeds this
 * value.  */
#define MAX_AUTO_SEARCH_TIMING   ((uint8_t)0x80)

#ifndef __BOOTLOADER
#define PAR_SLEEP_TIME_mS        ((uint32_t)100)
#endif   /* __BOOTLOADER   */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static bool flushPartitions_ = (bool)false;

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

static returnStatus_t init( void );
static returnStatus_t open( PartitionData_t const **pPartitionTbl, const ePartitionName ePartitionId, uint32_t u32UpdateRate );
static returnStatus_t close( PartitionData_t const *pPartitionTbl );
static returnStatus_t pwrMode( const ePowerMode powerMode );
static returnStatus_t read( uint8_t *pDest, const dSize dSrc, lCnt Cnt, PartitionData_t const *pPartitionTbl );
static returnStatus_t write( const dSize dDest, uint8_t const *pSrc, lCnt Cnt, PartitionData_t const *pPartitionTbl );
static returnStatus_t erase( lAddr lDest, lCnt Cnt, PartitionData_t const *pPartitionTbl );
static returnStatus_t flush( PartitionData_t const * );
static returnStatus_t restore( lAddr lDest, lCnt Cnt, PartitionData_t const *pPartitionTbl );
static returnStatus_t ioctl( const void *pCmd, void *pData, PartitionData_t const *pPartitionTbl );
static returnStatus_t size(const ePartitionName ePName, lCnt *pPartitionSize);
#ifndef __BOOTLOADER
static void           flushPartitions_CB(uint8_t cmd, void *pData);
#endif   /* __BOOTLOADER  */

#if defined TEST_MODE_ENABLE && !defined TM_DVR_PARTITION_UNIT_TEST
// STATIC returnStatus_t ValidatePartitionTable( PartitionData_t const *pPartitionTbl, uint8_t numOfPartitions );
#endif
/* ****************************************************************************************************************** */
/* CONSTANTS */

/* The following are the access functions for the partition module. */
#ifndef TM_FILE_IO_UNIT_TEST
const PartitionTbl_t PAR_partitionFptr =
{
   init,       /* Initialize each partition in the partition list */
   open,       /* Opens each partition in the partition list*/
   close,      /* Closes the desired partition */
   pwrMode,    /* Sets the power mode in the drivers.  Used for system power down and normal run modes. */
   read,       /* Reads data from the desired partition */
   write,      /* writes data from the desired partition */
   erase,      /* erases an entire partition */
   flush,      /* flushes the desired partition */
   restore,    /* Restores the desired partition */
   ioctl,      /* IOCTL commands to send to a partition */
   size,       /* returns the size of the partition requested. */
   PAR_timeSlice /* Allows maintenance to be completed on all drivers under the partition manager. */
};
#endif

#ifdef TM_PARTITION_TBL
#ifndef __BOOTLOADER
static const uint8_t _no[] = "No";
static const uint8_t _yes[] = "Yes";
//static const uint8_t _error[] = "Error";
static const uint8_t _ok[] = "ok";
static const uint8_t _errorAddr[] = "Addr";
static const uint8_t _errorDataSize[] = "Data Size";
static const uint8_t _warnDesc[] = "Dev Desc";
static const uint8_t _errorBanked[] = "Bank Attr";
static const uint8_t _warnCacheUpdate[] = "Cache Update Rate";
static const uint8_t _warnEraseSize[] = "Strt Addr not on Sect";
static const uint8_t _errorCacheData[] = "Cache Data ptr";
#endif   /* BOOTLOADER  */
#endif
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#if RTOS
/***********************************************************************************************************************

   Function name: PAR_appTask()

   Purpose: Manages the partitions

   Arguments: void *pvParameters

   Returns: None

 **********************************************************************************************************************/
void PAR_appTask( taskParameter )
{
   timer_t tmrSettings;

   (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
   tmrSettings.ulDuration_mS  = 6 * TIME_TICKS_PER_HR;  //flush partitions
   tmrSettings.pFunctCallBack = flushPartitions_CB;
   (void)TMR_AddTimer(&tmrSettings);

   for ( ; ; )
   {
      OS_TASK_Sleep ( PAR_SLEEP_TIME_mS );
      //Todo:  Should check to make sure HMC is idle.
      (void)PAR_timeSlice();
   }
}  /*lint !e715 !e818  pvParameters is not used */
#endif

#if RTOS || defined( __BOOTLOADER )
/***********************************************************************************************************************

   Function name: PAR_init()

   Purpose: API for initializing the partition manager

   Arguments: None

   Returns: returnStatus_t

 **********************************************************************************************************************/
returnStatus_t PAR_init( void )
{
   return(init());
}  /*lint !e715 !e818  pvParameters is not used */
#endif

/***********************************************************************************************************************

   Function Name: PAR_initRtos

   Purpose: Enables the RTOS functions

   Arguments: None

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects:

   Reentrant Code: No - This function is intended to be called at power up.

 **********************************************************************************************************************/
returnStatus_t PAR_initRtos( void )
{
   PartitionData_t const *pPTbl;
   uint8_t dvrCmd = (uint8_t)eRtosCmds;
   uint8_t dvrData = (uint8_t)eRtosCmdsEn;
   returnStatus_t retVal = eFAILURE;

   /* Need to enable the NV driver(s) to take advantage of the RTOS.  For example, when the SPI port does a read, the
      SPI port will pend on a semaphore while waiting for the SPI's DMA to complete. */
   if ( eSUCCESS == PAR_partitionFptr.parOpen(&pPTbl, ePART_NV_APP_RAW, 0) )  /* Gets partition information for IOCTL cmd below. */
   {
      retVal = PAR_partitionFptr.parIoctl((void *)&dvrCmd, (void *)&dvrData, pPTbl);
   }
   return (retVal);
}

/***********************************************************************************************************************

   Function Name: init

   Purpose: Initializes all drivers that listed in the partition table.  After each partition initializes properly, the
            drivers are opened.  This creates the mutexes and semaphores and opens the memory driver(s).

   Arguments: None

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: Initializes all lower layer drivers.

   Reentrant Code: No - This function is intended to be called at power up.

 **********************************************************************************************************************/
static returnStatus_t init( void )
{
   returnStatus_t eRetVal = eSUCCESS; /* Return status */
   uint8_t i; /* Loop variable */
   PartitionData_t const *pPartitionData; /* Pointer to the partition table, sPartitionData */

   for ( i = 0, pPartitionData = &sPartitionData[0];
         (eSUCCESS == eRetVal) && (i < ARRAY_IDX_CNT(sPartitionData));
         i++, pPartitionData++ )
   {
      DeviceDriverMem_t const *const *pNextDvr = sPartitionData[i].pDriverTbl + 1; /* Next Driver for the lower layers */

      /* Initialize the memory driver (create the mutexes and semaphores) */
      eRetVal = (*pPartitionData->pDriverTbl)->devInit(pPartitionData, pNextDvr);

      if ( eSUCCESS == eRetVal )
      {
         /* Open the memory driver (prepares all of the memory drivers to read/write data.  This includes setting
            up the cache memory, finding the current bank for a banked driver, etc...) */
         eRetVal = (*pPartitionData->pDriverTbl)->devOpen(pPartitionData, pNextDvr);
      }
   }
   return (eRetVal);
}

/***********************************************************************************************************************

   Function Name: open

   Purpose: Provides the handle of the partition that is best suited for the update rate given and matches the
   ePartitionID.  An error is returned if no suitable match is found.

   Arguments:
      PartitionData_t **pPartitionTbl: Open populates this pointer to an entry in the partition table.
      ePartitionName ePartitionId:     Partition to get the location of the starting address.
      uint32_t u32UpdateRate:          Minimum time (in seconds) between updates (write/erase) requested by the app.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t open( PartitionData_t const **pPartitionTbl,
                            const ePartitionName ePartitionId,
                            uint32_t u32UpdateRate )
{

   //   MAX_AUTO_SEARCH_TIMING

   returnStatus_t eRetVal = eFAILURE;
   uint8_t u8PartitionIndex; /* Used as the index for searching the partition table. */
   PartitionData_t const *pPartitionData; /* Pointer to the partition table, sPartitionData */

   *pPartitionTbl = (PartitionData_t *)NULL; /* Assume a failure */

   /* Index through the partition table until the correct partition is found or until the end of the list. */
   for ( u8PartitionIndex = 0, pPartitionData = &sPartitionData[0];
         u8PartitionIndex < ARRAY_IDX_CNT(sPartitionData);
         u8PartitionIndex++, pPartitionData++ )
   {
      if ( (ePartitionId == pPartitionData->ePartition)                                ||
            ( (   (ePART_SEARCH_BY_TIMING == ePartitionId)                             &&
                  (pPartitionData->sAttributes.MaxUpdateRateSec <= u32UpdateRate) )    &&
                  (MAX_AUTO_SEARCH_TIMING > (uint8_t)pPartitionData->ePartition) ) )
      {
         eRetVal = eSUCCESS;
         *pPartitionTbl = pPartitionData;
         break; /* Stop looping, the correct partition was found. */
      }
   }
   return (eRetVal);
}

/***********************************************************************************************************************

   Function Name: close

   Purpose: Calls the partition with a close command.

   Arguments: PartitionData_t const *pPartitionTbl - Partition entry to operate on.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t close( PartitionData_t const *pPartitionTbl )
{
   return ((*pPartitionTbl->pDriverTbl)->devClose(pPartitionTbl, pPartitionTbl->pDriverTbl + 1));
}

/***********************************************************************************************************************

   Function Name: pwrMode

   Purpose: Sets the power mode in every partition.  This is used when powering down the system.  The drivers will
            either put the processor to sleep or wait for a semaphore when writing/erasing the NVmemory.

   Arguments: ePowerMode powerMode:  Current power mode (low power mode or normal power mode)

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: In low power mode, the processor will sleep.

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t pwrMode( const ePowerMode powerMode )
{
   returnStatus_t eRetVal;
   uint8_t i; /* Loop variable */
   PartitionData_t const *pPartitionData; /* Pointer to the partition table, sPartitionData */

   for ( i = 0, pPartitionData = &sPartitionData[0]; i < ARRAY_IDX_CNT(sPartitionData); i++, pPartitionData++ )
   {
      /* Initialize the memory driver (create the mutexes and semaphores) */
      eRetVal = (*pPartitionData->pDriverTbl)->devSetPwrMode(powerMode, pPartitionData, pPartitionData->pDriverTbl + 1);
      /* Don't care what the return value is, all partitions need to be called with the correct power mode! */
   }
   return (eRetVal);
}

/***********************************************************************************************************************

   Function Name: read

   Purpose: Reads data from a partition, pPartitionTbl.

   Arguments:
      uint8_t *pDest:         Location to write data
      dSize dSrc:             offset in the partition to read the source data
      lCnt Cnt:               Number of bytes to read.
      PartitionData_t *pPTbl: Points to entry in the partition table for driver/partition information.  If NULL is
                              passed in, use the entire partition table as if it were one contiguous piece of memory.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t read( uint8_t *pDest, dSize dSrc, lCnt cnt, PartitionData_t const *pPTbl )
{
   returnStatus_t eRetVal = eFAILURE;

   if ((PartitionData_t const *)NULL == pPTbl)
   {
      PartitionData_t const *pPtData;  /* pointer to the table that will be operated on. */
      lCnt numToRead;                  /* Number of bytes to read. */
      dSize partitionOffset;           /* Keeps track of the offset into the partition table.  */
      uint8_t i;                         /* Loop Counter, used to make sure the partition table isn't exceeded. */
      for ( i = 0, pPtData = &sPartitionData[0], partitionOffset = 0; cnt && (i < ARRAY_IDX_CNT(sPartitionData));
            partitionOffset += pPtData->lDataSize, i++, cnt -= numToRead, pDest += numToRead, dSrc += numToRead, pPtData++)
      {
         /* Find the correct partition where the dSrc (starting address) takes place. */
         if (dSrc < (partitionOffset + pPtData->lDataSize))  /* Data falls in the current partition? */
         { /* The starting point is in the current partition. */
            numToRead = cnt;
            if ((dSrc + cnt) > (partitionOffset + pPtData->lDataSize)) /* Does the read span to the next partition? */
            {
               numToRead -= (dSrc + cnt) - (partitionOffset + pPtData->lDataSize);
            }
            eRetVal = (*pPtData->pDriverTbl)->devRead(pDest,
                                                      dSrc - partitionOffset,   /* Compensate for spanning */
                                                      numToRead,
                                                      pPtData, pPtData->pDriverTbl + 1);
            if (eSUCCESS != eRetVal)   /* Failure reading? */
            {
               break;                  /* Read failed, return with failure. */
            }
         }
         else
         {
            numToRead = 0;
         }
      }
      /* If the there are bytes left & no access error(s), then more bytes were requested than exist in the partition table. */
      if ((0 != cnt) && (eSUCCESS == eRetVal))
      {
         eRetVal = eFAILURE;
      }
   }
   else  /* Get data from a defined partition. */
   {
      if ( pPTbl->lDataSize >= (dSrc + cnt) ) /* Validate # of bytes to read are within the partition's limits. */
      {
         eRetVal = (*pPTbl->pDriverTbl)->devRead(pDest, dSrc, cnt, pPTbl, pPTbl->pDriverTbl + 1);
      }
   }
   return (eRetVal);
}

/***********************************************************************************************************************

   Function Name: write

   Purpose: Writes data to the partition specified by pPartitionTbl pointer.

   Arguments:
      dSize lDest:            offset into the partition to write data
      uint8_t *pSrc:          Location of the source data
      lCnt Cnt:               Number of bytes to write to the device
      PartitionData_t *pPTbl: Points to entry in the partition table for driver/partition information.  If NULL is
                              passed in, use the entire partition table as if it were one contiguous piece of memory.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t write( dSize lDest, uint8_t const *pSrc, lCnt cnt, PartitionData_t const *pPTbl )
{
   returnStatus_t eRetVal = eFAILURE;

   if ((PartitionData_t const *)NULL == pPTbl)
   {
      PartitionData_t const *pPtData;  /* pointer to the table that will be operated on. */
      lCnt numToWrite;                 /* Number of bytes to write. */
      dSize partitionOffset;           /* Keeps track of the offset into the partition table.  */
      uint8_t i;                       /* Loop Counter, used to make sure the partition table isn't exceeded. */
      for ( i = 0, pPtData = &sPartitionData[0], partitionOffset = 0;
            cnt && (i < ARRAY_IDX_CNT(sPartitionData));
            partitionOffset += pPtData->lDataSize, i++, cnt -= numToWrite, lDest += numToWrite, pSrc += numToWrite, pPtData++)
      {
         /* Find the correct partition where the dSrc (starting address) takes place. */
         if (lDest < (partitionOffset + pPtData->lDataSize))  /* Data falls in the current partition? */
         { /* The starting point is in the current partition. */
            numToWrite = cnt;
            if ((lDest + cnt) > (partitionOffset + pPtData->lDataSize)) /* Does the write span to the next partition? */
            {
               numToWrite -= (lDest + cnt) - (partitionOffset + pPtData->lDataSize);
            }
            eRetVal = (*pPtData->pDriverTbl)->devWrite(lDest - partitionOffset,     /* Compensate for spanning */
                                                       pSrc,
                                                       numToWrite,
                                                       pPtData, pPtData->pDriverTbl + 1);
            if (eSUCCESS != eRetVal)   /* Failure writing? */
            {
               break;                  /* Write failed, return with failure. */
            }
         }
         else
         {
            numToWrite = 0;
         }
      }
      /* If the there are bytes left & no access error(s), then more bytes were requested than exist in the partition table. */
      if ((0 != cnt) && (eSUCCESS == eRetVal))
      {
         eRetVal = eFAILURE;
      }
   }
   else  /* Get data from a defined partition. */
   {
      if ( pPTbl->lDataSize >= (lDest + cnt) ) /* Validate the number of bytes to write defined in the partition limits. */
      {
         eRetVal = (*pPTbl->pDriverTbl)->devWrite(lDest, pSrc, cnt, pPTbl, pPTbl->pDriverTbl + 1);
      }
   }
   return (eRetVal);
}

/***********************************************************************************************************************

   Function Name: erase

   Purpose: Erases all or a portion of the memory in a partition.

   Arguments:
      lAddr lDest: Start offset of the bytes to erase in the partition.
      lCnt Cnt:    Number of bytes to erase
      *pPTbl:      Points to entry in the partition table for driver/partition information.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t erase( lAddr lDest, lCnt cnt, PartitionData_t const *pPTbl )
{
   returnStatus_t eRetVal = eFAILURE;

#if 0
   if ((PartitionData_t const *)NULL == pPTbl)
   {
      PartitionData_t const *pPtData;  /* pointer to the table that will be operated on. */
      lCnt numToErase;                 /* Number of bytes to erase. */
      dSize partitionOffset;           /* Keeps track of the offset into the partition table.  */
      uint8_t i;                         /* Loop Counter, used to make sure the partition table isn't exceeded. */
      for ( i = 0, pPtData = &sPartitionData[0], partitionOffset = 0;
            cnt && (i < ARRAY_IDX_CNT(sPartitionData));
            partitionOffset += pPtData->lDataSize, i++, cnt -= numToErase, lDest += numToErase, pPtData++)
      {
         /* Find the correct partition where the dSrc (starting address) takes place. */
         if (lDest < (partitionOffset + pPtData->lDataSize))  /* Data falls in the current partition? */
         { /* The starting point is in the current partition. */
            numToErase = cnt;
            if ((lDest + cnt) >= (partitionOffset + pPtData->lDataSize)) /* Does the erase span to the next partition? */
            {
               numToErase -= (lDest + cnt) - (partitionOffset + pPtData->lDataSize);
            }
            eRetVal = (*pPtData->pDriverTbl)->devErase(lDest - partitionOffset,     /* Compensate for spanning */
                                                         numToErase,
                                                         pPtData, pPtData->pDriverTbl + 1);
            if (eSUCCESS != eRetVal)   /* Failure writing? */
            {
               break;                  /* Erase failed, return with failure. */
            }
         }
         else
         {
            numToErase = 0;
         }
      }
      /* If the there are bytes left & no access error(s), then more bytes were requested than exist in the partition table. */
      if ((0 != cnt) && (eSUCCESS == eRetVal))
      {
         eRetVal = eFAILURE;
      }
   }
   else  /* Get data from a defined partition. */
   {
      if ( pPTbl->lDataSize >= (lDest + cnt) ) /* Validate the number of bytes to erase are in the partition limits. */
      {
         eRetVal = (*pPTbl->pDriverTbl)->devErase(lDest, cnt, pPTbl, pPTbl->pDriverTbl + 1);
      }
   }
#endif
   if ( ( PartitionData_t const * )NULL != pPTbl )
   {
#if 0
      if ( pPTbl->lDataSize >= ( lDest + cnt ) ) /* Validate number of bytes to erase within partition limits. */
#else
      uint32_t maxEraseLen;
      maxEraseLen = pPTbl->lDataSize;
      if ( pPTbl->PartitionType.banked )
      {
         maxEraseLen = pPTbl->lSize * ( pPTbl->sAttributes.NumOfBanks + 1 );
      }
      if ( maxEraseLen >= ( lDest + cnt ) )     /* Validate total bytes to erase within partition limits. */
#endif
      {
         eRetVal = ( *pPTbl->pDriverTbl )->devErase( lDest, cnt, pPTbl, pPTbl->pDriverTbl + 1 );
      }
      if ( pPTbl->ePartition == ePART_WHOLE_NV ) /* If whole device, erase all the cache partitions, as well!   */
      {
         PartitionData_t const   *pCache = &sPartitionData[0];
         uint32_t                i;

         for ( i = 0; i < ARRAY_IDX_CNT( sPartitionData ); i++ ) /* Find all cached partitions in master table.  */
         {
            if ( pCache->PartitionType.cached )
            {
               CLRWDT();
               /*lint --e{655,641} ORing enum is OK here. Just looking for non-zero value. */
               eRetVal |= ( *pCache->pDriverTbl )->devErase( 0, pCache->lDataSize, pCache, pCache->pDriverTbl + 1 );
            }
            pCache++;
         }
      }
   }
   return (eRetVal);
}

/***********************************************************************************************************************

   Function Name: flush

   Purpose: Calls selected partition with the flush command.  Drivers that have data in a buffer will flush them to NV.
            If called with NULL, flushes all partitions

   Arguments:
      *pPartitionTbl: Points to entry in the partition table for driver/partition information.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t flush( PartitionData_t const *pPartitionTbl )
{
   returnStatus_t eRetVal;
   uint8_t i; /* Loop variable */

   if ((PartitionData_t const *)NULL != pPartitionTbl)
   {
      eRetVal = ((*pPartitionTbl->pDriverTbl)->devFlush(pPartitionTbl, pPartitionTbl->pDriverTbl + 1));
   }
   else
   {  /* Flush all the drivers, if the partition passed is NULL */
      eRetVal = eSUCCESS;
      for ( i = 0, pPartitionTbl = &sPartitionData[0];
            (i < ARRAY_IDX_CNT(sPartitionData));
            i++, pPartitionTbl++ )
      {
         if (eSUCCESS != ((*pPartitionTbl->pDriverTbl)->devFlush(pPartitionTbl, pPartitionTbl->pDriverTbl + 1)) )
         {
            eRetVal = eFAILURE;
         }
      }
   }
   return (eRetVal);
}

/***********************************************************************************************************************

   Function Name: restore

   Purpose: Calls the partition with the restore command.  Drivers will restore data from NV to their if applicable.

   Arguments:
      lAddr *pDest:                   Offset into the partition of the data that needs to be restored
      lCnt Cnt:                       Number of bytes to restore.
      PartitionData_t *pPartitionTbl: Points to entry in the partition table for driver/partition information.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t restore( lAddr lDest, lCnt Cnt, PartitionData_t const *pPartitionTbl )
{
   returnStatus_t eRetVal = eFAILURE;

   if ( pPartitionTbl->lDataSize >= (lDest + Cnt) ) /* Validate the # of bytes to restore are in the partition limits. */
   {
      eRetVal = (*pPartitionTbl->pDriverTbl)->devRestore(lDest, Cnt, pPartitionTbl, pPartitionTbl->pDriverTbl + 1);
   }
   return (eRetVal);
}

/***********************************************************************************************************************

   Function Name: ioctl

   Purpose: send an ioctl command to a driver

   Arguments:
      void *:                         Command parameter
      void *:                         Data for the device
      PartitionData_t *pPartitionTbl: Points to entry in the partition table for driver/partition information.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t ioctl( const void *pCmd, void *pData, PartitionData_t const *pPartitionTbl )
{
   return ((*pPartitionTbl->pDriverTbl)->devIoctl(pCmd, pData, pPartitionTbl, pPartitionTbl->pDriverTbl + 1));
}

/***********************************************************************************************************************

   Function Name: PAR_timeSlice

   Purpose: Common API function, allows for the driver to perform any housekeeping that may be required.  For this
            module, nothing is to be completed.

   Arguments: None

   Returns: BOOLEAN - Indicate if this function took CPU time. FALSE - No time taken

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
bool PAR_timeSlice(void)
{
   bool retVal = false;
   uint8_t i;             /* Loop variable */
   PartitionData_t  const *pPartitionData;   /* Pointer to the partition table, sPartitionData */

   for ( i = 0, pPartitionData = &sPartitionData[0]; !retVal && (i < ARRAY_IDX_CNT(sPartitionData)); i++, pPartitionData++)
   {
      /* Initialize the memory driver (create the mutexes and semaphores) */
      retVal = (*pPartitionData->pDriverTbl)->devTimeSlice(pPartitionData, pPartitionData->pDriverTbl + 1);
   }

   if ( flushPartitions_ )
   {  // flush all partitions
      (void)PAR_partitionFptr.parFlush ((PartitionData_t const *)NULL);
      flushPartitions_ = false;
      retVal = true;
   }

   return(retVal);
}

/***********************************************************************************************************************

   Function Name: size

   Purpose: Common API function, returns the size of the partition requested via the pointer pPartitionSize.  See
            the typedef, 'ePartitionName', in cfg_app.h.

   Arguments:
      ePartitionName ePName    - Partition Name
      lCnt *pPartitionSize     - Points to the location to store the size of the partition

   Returns: returnStatus_t

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t size(const ePartitionName ePName, lCnt *pPartitionSize)
{
   returnStatus_t retVal = ePAR_NOT_FOUND;         /* Return value. */
   uint8_t i;                                      /* Loop variable */
   PartitionData_t  const *pPtData;                /* Pointer to the partition table, sPartitionData */

   *pPartitionSize = 0;          /* Assume the partition is either not found or starts the accumulative value to 0. */
   if ( (ePART_ALL_APP == ePName) || (ePART_ALL_PARTITIONS == ePName))  /* Scan multiple partitions? */
   {
      ePartitionName scanLimit = ePART_START_NAMED_PARTITIONS; /* Assume we'll search data partitions only. */

      retVal = eSUCCESS;                           /* The ePName exists and will be operated on. */

      if (ePART_ALL_PARTITIONS == ePName)          /* Scan all partitions? Yes -  set the limit to the last parition. */
      {
         scanLimit = ePART_LAST_PARTITION;  /* Set the scan limit to the end. */
      }

      for (i = 0, pPtData = &sPartitionData[0]; i < ARRAY_IDX_CNT(sPartitionData); i++, pPtData++)
      {
         if (pPtData->ePartition < scanLimit)      /* Is this the partition requested? */
         {
            *pPartitionSize += pPtData->lDataSize;     /* got the size, add it to the total size */
         }
      }
   }
   else  /* Requesting a single partition. */
   {  /* Look for the partition requested.  Need to look at every partition to find ePName */
      for (i = 0, pPtData = &sPartitionData[0]; i < ARRAY_IDX_CNT(sPartitionData); i++, pPtData++)
      {
         if (pPtData->ePartition == ePName)        /* Is this the partition requested? */
         {
            *pPartitionSize = pPtData->lDataSize;      /* got the size, save it. */
            retVal = eSUCCESS;                     /* Return success since the partition was found. */
            break;                                 /* get out of the loop, the partition was found. */
         }
      }
   }
   return(retVal);
}

/***********************************************************************************************************************

   Function Name: PAR_flushPartitions

   Purpose: Set the variable to flush the partitions later.

   Arguments: None

   Returns: None

   Side Effects: Set the file variable to flush the partitions later

   Reentrant Code: Yes

 **********************************************************************************************************************/
void PAR_flushPartitions(void)
{
   flushPartitions_ = (bool)true;
}

#ifndef __BOOTLOADER
/***********************************************************************************************************************

   Function Name: flushPartitions_CB(uint8_t cmd, void *pData)

   Purpose: Called by the timer to set flag to flush the partitions later

   Arguments:  uint8_t cmd - from the ucCmd member when the timer was created.
               void *pData - from the pData member when the timer was created

   Returns: None

   Side Effects: Set the file variable to flush the partitions later

   Reentrant Code: NO

 **********************************************************************************************************************/
/*lint -esym(715, cmd, pData ) not used; required by API  */
static void flushPartitions_CB(uint8_t cmd, void *pData)
{
   flushPartitions_ = (bool)true;
} /*lint !e818 pData could be pointer to const */
#endif   /* __BOOTLOADER   */

/***********************************************************************************************************************

   Function Name: PAR_GetFirstPartition

   Purpose: This function works together with PAR_GetNextPartition to get all the partitions.
            The partitions are returned, one at a time in the order they were defined.
            This function returns a pointer to first partition.
            Call function PAR_GetNextPartition to get rest of the partitions by passing the
            PAR_getNextMember_t *pMember parameter.

   Arguments:
      PAR_getNextMember_t *pMember: Saves/remembers the current position in the partition table. Subsequent calls
                                    to PAR_GetNextPartition function shall use this structure

   Returns: PartitionData_t *pPartitionData: Pointer to the partition table, NULL if no more entries

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
PartitionData_t *PAR_GetFirstPartition(PAR_getNextMember_t *pMember)
{
   PartitionData_t const *pPartitionData;

   if( pMember != NULL ) // Validate the pointers
   {
      pMember->nextPartition = 1; //next partition to return
      pPartitionData = &sPartitionData[0];
   }
   else
   {
      pPartitionData = NULL;
   }
   return (PartitionData_t *)pPartitionData;
}

/***********************************************************************************************************************

   Function Name: PAR_GetNextPartition

   Purpose: This function works together with PAR_GetFirstPartition to get all the partitions.
            The partitions are returned, one at a time in the order they were defined.
            Call PAR_GetFirstPartition function first to get the first partition.
            Then call this function with PAR_getNextMember_t parameter that was updated by PAR_GetFirstPartition
            function to get rest of the partitions.

   Arguments:
      PAR_getNextMember_t *pMember: Saves/remembers the current position in the partition table.

   Returns: PartitionData_t *pPartitionData: Pointer to the partition table, NULL if no more entries

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
PartitionData_t *PAR_GetNextPartition(PAR_getNextMember_t *pMember)
{
   PartitionData_t const *pPartitionData;

   if( pMember != NULL ) // Validate the pointers
   {
      if ( pMember->nextPartition < ARRAY_IDX_CNT(sPartitionData) )
      {  // more partition entries
         pPartitionData = &sPartitionData[pMember->nextPartition];
         pMember->nextPartition++;
      }
      else
      {
         pPartitionData = NULL;
      }
   }
   else
   {
      pPartitionData = NULL;
   }
   return (PartitionData_t *)pPartitionData;
}

/***********************************************************************************************************************

   Function Name: PAR_ValidatePartitionTable

   Purpose: Validates the partition table

   Arguments: None

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
#ifdef TM_PARTITION_TBL
#ifndef __BOOTLOADER
returnStatus_t PAR_ValidatePartitionTable( void )
{
   returnStatus_t eRetVal = eSUCCESS;      /* Assume success */
   PartitionData_t const *pEntryToCompare; /* Entry in the table that we comparing the entry under test against. */
   PartitionData_t const *pEntryUnderTest; /* The Entry currently being validated. */
   uint8_t i;                  /* Loop counter */
   uint8_t parCnt;             /* Loop counter */
   uint8_t const *pCached;
   uint8_t const *pDfwManip;
   uint8_t const *pBanked;
   uint8_t const *pError;
   uint8_t const *pWarn;
   uint8_t metaDataSize;        /* Number of bytes used for meta data */

   DBG_printf( "PId,   Partition Desc, Cached, Banked, dfwManip,    Ext Bus,   NV Type, Phy,   Adr St,  Adr End,   Size  , Erase Size, Error, Warn");

   /* Check for memory overlaps... */
   for ( parCnt = 0, pEntryUnderTest = sPartitionData; parCnt < ARRAY_IDX_CNT(sPartitionData); parCnt++, pEntryUnderTest++ )
   {
      pCached = _no;
      pBanked = _no;
      pDfwManip = _no;
      pError = _ok;
      pWarn = _ok;
      if ( pEntryUnderTest->PartitionType.banked )
      {
         pBanked = _yes;
      }
      if ( pEntryUnderTest->PartitionType.cached )
      {
         pCached = _yes;
      }
      if ( pEntryUnderTest->PartitionType.dfwManip )
      {
         pDfwManip = _yes;
      }

      /* Check for address overlaps */
      for ( i = 0, pEntryToCompare = sPartitionData; i < ARRAY_IDX_CNT(sPartitionData); i++, pEntryToCompare++ )
      {
         if (  i != parCnt &&                                   /* Don't compare a partition to itself                */
               ePART_WHOLE_NV != pEntryToCompare->ePartition && /* Don't look for overlap with whole device in table  */
               ePART_WHOLE_NV != pEntryUnderTest->ePartition)   /* Don't look for overlap with whole device under test */
         {  /* Not the same partition being tested and not the whole partition */
            if ( pEntryUnderTest->PartitionType.pDevice == pEntryToCompare->PartitionType.pDevice ) /* Is the driver chain the same? */
            {
               /* Validate Address Range */
               if (  ((pEntryToCompare->PhyStartingAddress <= pEntryUnderTest->PhyStartingAddress)                                  &&
                     ((pEntryToCompare->PhyStartingAddress + pEntryToCompare->lSize - 1) >= pEntryUnderTest->PhyStartingAddress))   ||
                     ((pEntryToCompare->PhyStartingAddress >= pEntryUnderTest->PhyStartingAddress)                                  &&
                     ( pEntryToCompare->PhyStartingAddress <= (pEntryUnderTest->PhyStartingAddress + pEntryUnderTest->lSize - 1))) )
               {
                  pError = _errorAddr;
                  eRetVal = eFAILURE;
               }
               /* Validate partition type description */
               if ( pEntryUnderTest->PartitionType.pBus != pEntryToCompare->PartitionType.pBus )
               {
                  pWarn = _warnDesc;
               }
            }
         }
      }
      /* Validate that the partition starts at a multiple of the erase sector size */
      if ( pEntryUnderTest->PhyStartingAddress % pEntryUnderTest->sAttributes.EraseBlockSize )
      {
         pWarn = _warnEraseSize;
      }

      /* Validate lsize and lDataSize */
      metaDataSize = 0; //Number of bytes of meta data
      if(pEntryUnderTest->PartitionType.cached)
      {  //Cached partition
         metaDataSize+= sizeof(cacheMetaData_t);
      }
      if(pEntryUnderTest->PartitionType.banked)
      {   //Banked partition
         metaDataSize+= sizeof(bankMetaData_t);
      }
      if (pEntryUnderTest->lSize != (pEntryUnderTest->lDataSize + metaDataSize) )
      {
         pError = _errorDataSize;
         eRetVal = eFAILURE;
      }

      /* Validate banked information */
      if (  ( pEntryUnderTest->PartitionType.banked && !pEntryUnderTest->sAttributes.NumOfBanks)         ||
            ( pEntryUnderTest->PartitionType.banked && (NULL == pEntryUnderTest->sAttributes.pMetaData)) ||
            (!pEntryUnderTest->PartitionType.banked && pEntryUnderTest->sAttributes.NumOfBanks)          ||
            (!pEntryUnderTest->PartitionType.banked && (NULL != pEntryUnderTest->sAttributes.pMetaData)) ||
            (!pEntryUnderTest->PartitionType.banked && pEntryUnderTest->PartitionType.AutoEraseBank) )
      {
         pError = _errorBanked;
         eRetVal = eFAILURE;
      }
      /* Validate cached information */
      if ( pEntryUnderTest->PartitionType.cached )
      {
         if ( NULL == pEntryUnderTest->sAttributes.pCachedData )
         {
            pError = _errorCacheData;
            eRetVal = eFAILURE;
         }
         if ( 0 < pEntryUnderTest->sAttributes.MaxUpdateRateSec )
         {
            pWarn = _warnCacheUpdate;
         }
      }
      else
      {
         if ( NULL != pEntryUnderTest->sAttributes.pCachedData )
         {
            pError = _errorAddr;
            eRetVal = eFAILURE;
         }
      }
      CLRWDT();
      DBG_printf( "%3u,%17s,%7s,%7s,%9s,%11s, %14s, 0x%06lX, 0x%06lX, 0x%06lX, %10lu,%6s, %-15s",
                  (uint32_t)pEntryUnderTest->ePartition,
                  pEntryUnderTest->PartitionType.pPartType,
                  pCached,
                  pBanked,
                  pDfwManip,
                  pEntryUnderTest->PartitionType.pBus,
                  pEntryUnderTest->PartitionType.pDevice,
                  pEntryUnderTest->PhyStartingAddress,
                  pEntryUnderTest->PhyStartingAddress + (pEntryUnderTest->lSize * (pEntryUnderTest->sAttributes.NumOfBanks + 1)) - 1,
                  pEntryUnderTest->lSize,
                  pEntryUnderTest->sAttributes.EraseBlockSize,
                  pError,
                  pWarn);
      OS_TASK_Sleep ( 10 );   /* Allow line to print. */
   }
   return (eRetVal);
}
#endif
#endif
