/* ************************************************************************* */
/******************************************************************************
 *
 * Filename:   mem_chk.c
 *
 * Global Designator: MEMC_
 *
 * Notes:  This module tests the NV memory by writing sequentially to the NV
 *         memory and reading the contents back.  When using the an nv memory
 *         that can be power cycled, the memory is power cycled.  The generic
 *         timer module is used to time the power down to ensure the chip is
 *         completely powered off.  The nvmem module is used to access the
 *         memory so that this module can access any NV memory device.
 *
 ******************************************************************************
 * Copyright (c) 2001-2010 Aclara Power-Line Systems Inc.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ******************************************************************************
 *
 * $Log$ KAD Created 030310
 *
 *****************************************************************************/
/* ***************************************************************************
 * Revision History:
 * v1.0 - 1.3 - original code for Ramtron's FRAM device
 * v2.0 - Rewrite for new 128K NV Memory.  This test is now hardware independant
 *        and accesses the NV memory through the nvmem interface.
 * v2.1 - Data buffer written to NV memory was not being filled with data properly.
 * v2.2 - 020612 kad - Added cast(s) to remove warning(s).
 *************************************************************************** */

/* ************************************************************************* */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#define MEMC_GLOBAL
#include "mem_chk.h"
#undef MEMC_GLOBAL
#include "DBG_SerialDebug.h"
#include "dvr_extflash.h"
#include "sys_clock.h"

/* ************************************************************************* */
/* TYPE DEFINITIONS */

/* ************************************************************************* */
/* CONSTANTS */

#define NV_CHECK_START_CHAR   ((uint8_t)1)     /* Character to start with when testing memory */

/* The following two parameters are limited by the amount of RAM the controller has available */
/* Chose a NV_CHECK_BLOCK_SIZE size that is least likely to repeat the same character value on a 'page' boundary */
#define NV_CHECK_BLOCK_SIZE   ((uint8_t)127)    /* How many sequential characters to write before repeating */

#define NV_TEST_RANGE_START   ((uint32_t)0)
#define NV_TEST_RANGE_LEN     ((uint32_t)EXT_FLASH_SIZE)
//#define NV_TEST_RANGE_LEN     ((uint32_t)20000)
#define NV_CLEAR_START        ((uint32_t)0)
#define NV_CLEAR_LEN          ((uint32_t)EXT_FLASH_SIZE)

#define NV_CHECK_INVERT       (bool)true
#define NV_CHECK_NONINVERT    (bool)false

/* ************************************************************************* */
/* MACRO DEFINITIONS */

/* ************************************************************************* */
/* FILE VARIABLE DEFINITIONS */

/* ************************************************************************* */
/* FUNCTION PROTOTYPES */

static returnStatus_t CheckRange(uint8_t, uint16_t, bool, uint32_t, uint32_t);

/* ************************************************************************* */
/* FUNCTION DEFINITIONS */

/* ************************************************************************* */

/******************************************************************************
 *
 * Function name: MEMC_testPowerDown
 *
 * Purpose: Tests low power mode timing and current draw
 *
 * Arguments: bool verbose
 *
 * Returns: returnStatus_t - eSUCCESS or eFAILURE
 *
 *****************************************************************************/
void MEMC_testPowerDown(bool verbose)
{
   PartitionData_t const *pParTbl;
   uint8_t data;

   (void)PAR_partitionFptr.parOpen(&pParTbl, ePART_WHOLE_NV, 0);

   (void)PAR_partitionFptr.parMode(ePWR_NORMAL);
   if (verbose)
   {
      DBG_logPrintf('U', "  Chip Erase - Normal Power Mode");
   }
   (void)PAR_partitionFptr.parErase(NV_CLEAR_START, NV_CLEAR_LEN, pParTbl);

   if (verbose)
   {
      DBG_logPrintf('U', "  Sector Erase - Normal Power Mode");
   }
   (void)PAR_partitionFptr.parErase(NV_CLEAR_START, EXT_FLASH_SECTOR_SIZE, pParTbl);

   if (verbose)
   {
      DBG_logPrintf('U', "  Byte Write/Read - Normal Power Mode");
   }
   data = 0x55;
   (void)PAR_partitionFptr.parWrite(0, &data, (lCnt)sizeof(data), pParTbl);
   (void)PAR_partitionFptr.parRead(&data, 0, (lCnt)sizeof(data), pParTbl);

   uint16_t delay = 0xFFFF;
   while(--delay)
   {}

   (void)PAR_partitionFptr.parMode(ePWR_LOW);
   (void)SCLK_intClk();
   (void)PAR_partitionFptr.parInit();

   if (verbose)
   {
//      DBG_logPrintf('U', "  Chip Erase - Low Power Mode");
   }
   (void)PAR_partitionFptr.parErase(NV_CLEAR_START, NV_CLEAR_LEN, pParTbl);
   (void)PAR_partitionFptr.parRead(&data, 0, (lCnt)sizeof(data), pParTbl);

   if (verbose)
   {
//      DBG_logPrintf('U', "  Sector Erase - Low Power Mode");
   }
   (void)PAR_partitionFptr.parErase(NV_CLEAR_START, EXT_FLASH_SECTOR_SIZE, pParTbl);
   (void)PAR_partitionFptr.parRead(&data, 0, (lCnt)sizeof(data), pParTbl);

   if (verbose)
   {
//      DBG_logPrintf('U', "  Byte Write/Read - Low Power Mode");
   }
   (void)PAR_partitionFptr.parWrite(0, &data, (lCnt)sizeof(data), pParTbl);
   (void)PAR_partitionFptr.parRead(&data, 0, (lCnt)sizeof(data), pParTbl);
   (void)PAR_partitionFptr.parMode(ePWR_NORMAL);

   RESET();
}
/* ************************************************************************* */

/******************************************************************************
 *
 * Function name: MEMC_Check
 *
 * Purpose: Checks External NV Memory (Internal simply takes too long and we
 *          haven't seen any failures).
 *
 * Arguments: bool verbose, ePowerMode pwrMode
 *
 * Returns: returnStatus_t - eSUCCESS or eFAILURE
 *
 *****************************************************************************/
void MEMC_Check(bool verbose, ePowerMode pwrMode)
{
   returnStatus_t retVal;           /* Initialize the return value */
   PartitionData_t const *pParTbl;
   (void)PAR_partitionFptr.parOpen(&pParTbl, ePART_WHOLE_NV, 0);

   (void)OS_TASK_Set_Priority(pTskName_Manuf, 10);  /* Increase pri so no other task corrupts testing */
   (void)OS_TASK_Set_Priority(pTskName_Idle, 11);

#if 0
   uint8_t data[2];
   PAR_partitionFptr.pwrMode(ePWR_NORMAL);
   retVal = PAR_partitionFptr.erase(NV_CLEAR_START, NV_CLEAR_LEN, pParTbl);
   retVal = PAR_partitionFptr.read(&data[0], 0, (lCnt)sizeof(data), pParTbl);
   data[0] = 0x55;
   data[1] = 0x55;
   retVal = PAR_partitionFptr.write(0, &data[0], (lCnt)sizeof(data), pParTbl);
   retVal = PAR_partitionFptr.read(&data[0], 0, (lCnt)sizeof(data), pParTbl);

   PAR_partitionFptr.pwrMode(ePWR_LOW);
   data[0] = 0x55;
   data[1] = 0x55;
   retVal = PAR_partitionFptr.erase(NV_CLEAR_START, NV_CLEAR_LEN, pParTbl);
   retVal = PAR_partitionFptr.read(&data[0], 0, (lCnt)sizeof(data), pParTbl);
   data[0] = 0x55;
   data[1] = 0x55;
   retVal = PAR_partitionFptr.write(0, &data[0], (lCnt)sizeof(data), pParTbl);
   retVal = PAR_partitionFptr.read(&data[0], 0, (lCnt)sizeof(data), pParTbl);
   PAR_partitionFptr.pwrMode(ePWR_NORMAL);

#endif

   if (verbose)
   {
      DBG_logPrintf('U', "  Erasing");
   }
   (void)PAR_partitionFptr.parMode(pwrMode);

   uint8_t dvrCmd = (uint8_t)eRtosCmds;
   uint8_t dvrData = (uint8_t)eRtosCmdsEn;
   (void)PAR_partitionFptr.parIoctl((void *)&dvrCmd, (void *)&dvrData, pParTbl);

   retVal = PAR_partitionFptr.parErase(NV_CLEAR_START, NV_CLEAR_LEN, pParTbl);

   if (verbose)
   {
      DBG_logPrintf('U', "  Performing 1st Run");
   }

   if (eSUCCESS == retVal)
   {
      /* 1st Pass */
      (void)PAR_partitionFptr.parErase(NV_CLEAR_START, NV_CLEAR_LEN, pParTbl);
      retVal = CheckRange(NV_CHECK_START_CHAR,   /* Set the Start Character */
                                  NV_CHECK_BLOCK_SIZE,   /* Amount of memory to write to at a time */
                                  NV_CHECK_NONINVERT,    /* Do not invert the characters being written */
                                  NV_TEST_RANGE_START,   /* Set the 1st address of memory to test */
                                  NV_TEST_RANGE_LEN);    /* Set the amount of memory to test */
      if ( eSUCCESS == retVal )
      {
         /* 2nd Pass */
         if (verbose)
         {
            DBG_logPrintf('U', "    1st Run - Passed");
            DBG_logPrintf('U', "  Erasing");
         }
         (void)PAR_partitionFptr.parErase(NV_CLEAR_START, NV_CLEAR_LEN, pParTbl);
         if (verbose)
         {
            DBG_logPrintf('U', "  Performing 2nd Run (Inverting Bits)");
         }
         retVal = CheckRange(NV_CHECK_START_CHAR,   /* Set the Start Character */
                             NV_CHECK_BLOCK_SIZE,   /* Amount of memory to write to at a time */
                             NV_CHECK_INVERT,       /* Do not invert the characters being written */
                             NV_TEST_RANGE_START,   /* Set the 1st address of memory to test */
                             NV_TEST_RANGE_LEN);    /* Set the amount of memory to test */
         if (eSUCCESS == retVal)
         {
            if (verbose)
            {
               DBG_logPrintf('U', "  2nd Run - Passed");
            }
         }
      }
   }
   if (verbose)
   {
      DBG_logPrintf('U', "  Erasing");
      (void)PAR_partitionFptr.parErase(NV_CLEAR_START, NV_CLEAR_LEN, pParTbl);
      if (eSUCCESS == retVal)
      {
         DBG_logPrintf('U', "  Testing Complete - Passed!");
      }
      else
      {
         DBG_logPrintf('U', "  Testing Complete - Failure!");
      }
   }
   (void)PAR_partitionFptr.parMode(ePWR_NORMAL);
   DBG_logPrintf('U', "");
   OS_TASK_Sleep(ONE_SEC);
   RESET();
}
/* ************************************************************************* */
/******************************************************************************
 *
 * Function name: CheckRange
 *
 * Purpose: Writes sequential data to the NV memory and verifies the results.
 *
 * Arguments: uint8_t  startVal,
 *            uint16_t  cnt (max len is NV_CHECK_BLOCK_SIZE),
 *            uint8_t  bInvert
 *            uint32_t nvStrtAddr
 *            uint32_t nvLen
 *
 * Returns: returnStatus_t - eSUCCESS or eFAILURE
 *
 *****************************************************************************/
static returnStatus_t CheckRange(uint8_t startVal, uint16_t cnt, bool bInvert, uint32_t nvStrtAddr, uint32_t nvLen)
{
   returnStatus_t  retVal = eSUCCESS;        /* Assume we pass the test! */
   uint8_t  ucData[NV_CHECK_BLOCK_SIZE];     /* Buffer of the data to write */
   uint8_t  ucReadData[NV_CHECK_BLOCK_SIZE]; /* Buffer of the data read to compare with the data written */
   uint16_t  rwcount;                        /* Passes the number of bytes to read/write to NV handler */
   uint16_t  i;                              /* Used as a loop counter */
   uint32_t ulNvAddr;                        /* NV Address passed to NV handler */
   uint32_t ulMaxAddr;                       /* Last address written */
   uint8_t  *pDataWritten;                   /* Pointer to location of data to be written */
   uint8_t  *pDataRead;                      /* Pointer to location of data to read */

   PartitionData_t const *pParTbl;
   (void)PAR_partitionFptr.parOpen(&pParTbl, ePART_WHOLE_NV, 0);

   if (cnt <= NV_CHECK_BLOCK_SIZE)
   {
      /* Set the memory up to be written */
      pDataWritten = &ucData[0];
      if (bInvert)
      {
         for (i = 0; i < cnt; i++)
         {
            *pDataWritten++ = (uint8_t)(~(startVal + (uint8_t)i));
         }
      }
      else
      {
         for (i = 0; i < cnt; i++)
         {
            *pDataWritten++ = (uint8_t)(startVal + i);
         }
      }

      /* Write all locations of the NV memory */
      DBG_printfNoCr("    Writing to all locations");
      rwcount = cnt;
      ulMaxAddr = (nvStrtAddr + nvLen) - 1;
      uint16_t printUpdate;
      for (ulNvAddr = nvStrtAddr, printUpdate = 0; ulNvAddr <= ulMaxAddr; ulNvAddr+=cnt, printUpdate++)
      {
         if ( (ulNvAddr + (uint32_t)rwcount) > ulMaxAddr )
         {
            rwcount = (uint16_t)((ulMaxAddr - ulNvAddr) + 1);
         }
         retVal = PAR_partitionFptr.parWrite(ulNvAddr, &ucData[0], (lCnt)rwcount, pParTbl);
         if (0 == (printUpdate % 500))
         {
            DBG_printfNoCr(".");
         }
         CLRWDT();
      }

      /* Read and verify all locations of the NV memory */
      DBG_logPrintf('U', " ");
      DBG_logPrintf('U', "    Reading/Verifying all locations");
      rwcount = cnt;
      for (ulNvAddr = nvStrtAddr; (ulNvAddr <= ulMaxAddr) && (eSUCCESS == retVal); ulNvAddr+=cnt)
      {
         if ( (ulNvAddr + (uint32_t)rwcount) > ulMaxAddr )
         {
            rwcount = (uint16_t)((ulMaxAddr - ulNvAddr) + 1);
         }
         ucReadData[0] = 0;   /* Init the 1st byte to zero just in case the read fails the check below will catch i. */
         (void)memset(&ucReadData[0], 0, sizeof(ucReadData));
         retVal = PAR_partitionFptr.parRead(&ucReadData[0], (dSize)ulNvAddr, (lCnt)rwcount, pParTbl);
         /* Using pointers because it executes fastest */
         pDataWritten = &ucData[0];
         pDataRead = &ucReadData[0];
         i = rwcount;
         do /* This is a 'safe' do/while loop because 'i' can never be 0 due to the tests above */
         {
            if (*pDataWritten++ != *pDataRead++)
            {
               retVal = eFAILURE;
               break;
            }
         }while(0 < --i);   /* do-while loop executes the fastest! */
         CLRWDT();
      }
   }
   return(retVal);
}
/* ************************************************************************* */
