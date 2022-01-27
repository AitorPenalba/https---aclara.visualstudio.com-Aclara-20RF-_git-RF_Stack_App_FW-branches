/***********************************************************************************************************************
 *
 * Filename: hmc_mtr_lp_tsk.c
 *
 * Global Designator: HMC_LPTSK_
 *
 * Contents: Reads LP data from the host meter.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$
 *
 *  Created 06/24/13     KAD  Created
 *
 ***********************************************************************************************************************
 ***********************************************************************************************************************
 * Revision History:
 * v1.0 - Initial Release
 **********************************************************************************************************************/

/* INCLUDE FILES */

#include "project.h"
#if ENABLE_HMC_LPTSK
#include <stdint.h>
#include <stdbool.h>
#define HMC_LPTSK_GLOBAL
#include "hmc_mtr_lp_tsk.h"
#undef HMC_LPTSK_GLOBAL

#include "filenames.h"
#include "hmc_request.h"
#include "hmc_mtr_lp.h"
#include "byteswap.h"

/* MACRO DEFINITIONS */
#define MAX_LP_SETS           ((uint8_t)4)     /* Maximum number of SETS that are possible in the host meter */
#define MAX_HMC_WAIT_TIME     ((uint16_t)30000)/* Wait for 30s */

/* FILE VARIABLE DEFINITIONS */
static OS_SEM_Obj HMC_LP_Sem;

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FUNCTION PROTOTYPES */


/* FUNCTION DEFINITIONS */

/* Read Applet API */

/* Local Supporting Functions */

/***********************************************************************************************************************
 *
 * Function name: HMC_LPTSK_task
 *
 * Purpose: Task for reading LP data
 *
 * Arguments: uint32_t Arg0
 *
 * Returns: None
 *
 **********************************************************************************************************************/
void HMC_LPTSK_task( uint32_t Arg0 )
{
   static   stdTbl63_t           stdTbl63;               /* Table 63 data */
   static   HMC_REQ_queue_t      hmcReq;                 /* Request to send to the HMC request module */
   static   timeDateRcd_t        timeDateRcd;            /* Contains date/time from the meter's Block of LP data */
   static   uint8_t              lpData[sizeof(uint64_t)];

            HMC_LPTSK_queue_t    *pQueue;             /* Pointer to the queue - Used to get request and post results */
            stdTbl61_t           *pStdTbl61;          /* Points to a local copy of ST61 - Actual Load Profile Table */
            uint8_t              lpSet = 0;           /* Current SET being operated on */
            uint8_t              ch;                  /* Channel in the set being processed */
            quantDesc_t          quantProp;           /* Contains properties of the LP ch being read. */
            bool              lpSetFound = false;

   (void)OS_QUEUE_Create(&HMC_LPTSK_queueHandle);     /* Create the queue */
   (void)OS_SEM_Create ( &HMC_LP_Sem );                /* Create the semaphore handle */

   for(;;)  /* Task Loop */
   {
      pQueue = OS_QUEUE_Dequeue ( &HMC_LPTSK_queueHandle );   /* Get request */

      (void)memset(pQueue->pSiData, 0, pQueue->dataSize);   /* Clear pData */
      if (eSUCCESS == HMC_LD_getLPTbl(&pStdTbl61))  /* Get ST61 - Actual Load Profile Table */
      {
         tSysTime sysTime; /* Contains the current system time */
         pQueue->retVal = eHMC_LP_INVALID_REQ_TIME;   /* Assume invalid time */
         (void)TIME_SYS_GetSysDateTime(&sysTime);     /* Get the system time */
         /* Validate the requested data is NOT in the future. */
         if (TIME_UTIL_TimeCompare(&sysTime, &pQueue->reqTime) >= 0)
         {  /* Request time is okay */
            pQueue->retVal = eHMC_LP_INVALID_TIME_BASE;   /* Assume invalid interval time */
            /* Search for the SET that corresponds to the timeBase */
            for (lpSet = 0; (lpSet < MAX_LP_SETS) && !lpSetFound; lpSet++)
            {
               if (pStdTbl61->lpDataSet[lpSet].nbrMaxIntTimeSet == pQueue->intTime) /* Set contains req interval? */
               {  /* Found the correct time-base.  Now see if the quantity exists in the lpSet */
                  pQueue->retVal = eHMC_LP_INVALID_REQ_QUANT;
                  if (eSUCCESS == HMC_LP_getQuantityProp(lpSet, pQueue->eReadingType, &ch, &quantProp))
                  {
                     lpSetFound = true;
                     break;   /* Found the lpSet value!  Stop looking. */
                  }
               }
            }
         }
      }
      if (lpSetFound)   /* Was the LP Set and Channel found? */
      {  /* Need to read the LP status from the meter. */
         hmcReq.bOperation = eHMC_READ;
         hmcReq.tblInfo.id = STD_TBL_LOAD_PROFILE_STATUS;
         hmcReq.tblInfo.offset = sizeof(stdTbl63) * lpSet;
         hmcReq.tblInfo.cnt = sizeof(stdTbl63);
         hmcReq.maxDataLen = sizeof(stdTbl63);
         hmcReq.pSem = &HMC_LP_Sem;
         hmcReq.pData = &stdTbl63;
         (void)OS_QUEUE_Enqueue ( &HMC_REQ_queueHandle, &hmcReq );
         pQueue->retVal = eHMC_TBL_READ_FAILURE;
         if (OS_SEM_Pend ( &HMC_LP_Sem, MAX_HMC_WAIT_TIME )) /* Wait for the meter read */
         {
            uint8_t          sizeOfFmt1;          /* Contains the size (in bytes) of FMT1 */
            niFormatEnum_t fmt1;                /* Contains the LP FMT1 configuration */
            uint8_t          sizeOfInt;           /* Contains the size of a delta element in LP data */
            uint16_t         blockOffset;         /* Contains the offset of a block into ST64. */
            uint16_t         lastBlockOffset;     /* Contains the offset of the last block in ST64 */
            uint16_t         numBlockIntervals;   /* Contains the number of blocks in ST64 */

            (void)HMC_MTRINFO_niFormat1(&fmt1, &sizeOfFmt1);
            (void)HMC_LP_getFmtSize(&sizeOfInt);
            /* Calculate the size of a block */
            uint16_t blkSize = sizeof(timeDateRcd_t) + (sizeOfFmt1 * pStdTbl61->lpDataSet[lpSet].nbrChnsSet) +
                             (uint16_t)(((((8 + ((uint16_t)4 * pStdTbl61->lpDataSet[lpSet].nbrChnsSet)) / 8) + sizeOfInt)) *
                             pStdTbl61->lpDataSet[lpSet].nbrBlksIntsSet);
            /* Get the starting offset of the last block. */
            blockOffset = (uint16_t)(stdTbl63.lastBlockElement * blkSize);
            lastBlockOffset = blockOffset;

            for (;;)
            {
               /* Read the block's Date and Time. */
               hmcReq.bOperation = eHMC_READ;
               hmcReq.tblInfo.id = STD_TBL_LOAD_PROFILE_DATASET_1 + lpSet;
               hmcReq.tblInfo.offset = blockOffset;
               hmcReq.tblInfo.cnt = sizeof(timeDateRcd);
               hmcReq.maxDataLen = sizeof(timeDateRcd);
               hmcReq.pSem = &HMC_LP_Sem;
               hmcReq.pData = &timeDateRcd;
               (void)OS_QUEUE_Enqueue ( &HMC_REQ_queueHandle, &hmcReq );
               pQueue->retVal = eHMC_TBL_READ_FAILURE;
               if (OS_SEM_Pend ( &HMC_LP_Sem, MAX_HMC_WAIT_TIME ))
               {
                  tSysTime mtrBlkTime; /* Contains the meter's block time */
                  (void)TIME_UTIL_ConvertMtrToSys(&timeDateRcd, &mtrBlkTime); /* Convert meter's time to sys */
                  /* Is the block time <= the requested time? */
                  if (TIME_UTIL_TimeCompare(&mtrBlkTime, &pQueue->reqTime) >= 0)
                  {  /* Calculate the number of intervals in history we need to navigate */
                     uint64_t deltaTime = TIME_UTIL_ConvertSysToSysComb(&mtrBlkTime) -
                                        TIME_UTIL_ConvertSysToSysComb(&pQueue->reqTime);
                     uint64_t maxIntTimeSet = pStdTbl61->lpDataSet[lpSet].nbrMaxIntTimeSet * (uint64_t)TIME_TICKS_PER_MIN;
                     /* Make sure the delta time is a multiple of the time base.  Convert the MAX_INT_TIME_SET to system
                      * time units. */
                     pQueue->retVal = eHMC_LP_INVALID_REQ_TIME;
                     if (0 != (deltaTime % maxIntTimeSet))
                     {
                        uint32_t numIntsBack = (uint32_t)(deltaTime / maxIntTimeSet); /* How far to go back in the block */

                        if (lastBlockOffset == blockOffset)
                        {
                           numBlockIntervals = stdTbl63.nbrValidInt / pStdTbl61->lpDataSet[lpSet].nbrChnsSet;
                        }
                        else
                        {
                           numBlockIntervals = blkSize;
                        }
                        /* Does the block contain this many intervals? */
                        if (numBlockIntervals <= numIntsBack)
                        {
                           /* Calculate the Offset into the Block which is the Offset into ST6X.  Read data! */
                           hmcReq.bOperation = eHMC_READ;
                           hmcReq.tblInfo.id = STD_TBL_LOAD_PROFILE_DATASET_1 + lpSet;
                           hmcReq.tblInfo.offset =  blockOffset + sizeof(timeDateRcd_t) +
                                                    (sizeOfFmt1 * pStdTbl61->lpDataSet[lpSet].nbrChnsSet) +
                                                    ((((4+(4*pStdTbl61->lpDataSet[lpSet].nbrChnsSet))/8) + sizeOfInt) *
                                                     numIntsBack);
                           hmcReq.tblInfo.cnt = sizeOfInt;
                           hmcReq.maxDataLen = sizeOfInt;
                           hmcReq.pSem = &HMC_LP_Sem;
                           hmcReq.pData = &lpData[0];
                           (void)OS_QUEUE_Enqueue ( &HMC_REQ_queueHandle, &hmcReq );
                           pQueue->retVal = eHMC_TBL_READ_FAILURE;
                           if (OS_SEM_Pend ( &HMC_LP_Sem, MAX_HMC_WAIT_TIME ))
                           {
                              quantProp.tblInfo.cnt =  hmcReq.tblInfo.cnt; /* Count is required for Si conversion */
                              Byte_Swap((uint8_t *)&quantProp.tblInfo.cnt, sizeof(quantProp.tblInfo.cnt)); /* big endian */
                              /* The data has been read.  Now convert the data to SI units. */
                              calcSiData_t siRes;
                              if (eSUCCESS == HMC_MTRINFO_convertDataToSI(&siRes, &lpData[0], &quantProp))
                              {
                                 loadProfileCtrl_t lpCtrl;
                                 /* Get Divisor and Multiplier. */
                                 if (eSUCCESS == HMC_LD_readSetxChx(lpSet, ch, &lpCtrl))  //lint !e644
                                 {
                                    siRes.result *= lpCtrl.divisorsSet;
                                    siRes.result /= lpCtrl.scalarsSet;
                                    (void)memcpy(pQueue->pSiData, &siRes, sizeof(siRes));
                                    pQueue->retVal = eSUCCESS;
                                    /* SUCCESS!!!!  We have data! */
                                 }
                              }
                           }
                        }
                        else
                        {
                           /* Check if blockOffset is 0 or > than the last block */
                           if (0 == lastBlockOffset)
                           {
                              pQueue->retVal = eHMC_LP_INVALID_REQ_TIME_RANGE;
                              break;
                           }
                           else
                           {
                              blockOffset -= blkSize;
                           }
                        }
                     }
                     else
                     {  /* LP is missing the data requested */
                        pQueue->retVal = eHMC_LP_MISSING_VALUE;
                        break;
                     }
                  }
               }
            }
         }
      }
      if (NULL != pQueue->pSem)
      {
         (void)OS_SEM_Post( pQueue->pSem );
      }
   }
} /*lint !e715  arg 0 not used */
#endif /* ENABLE_HMC_LPTSK */

