#ifdef UNIT_TESTS
/** ********************************************************************************************************************
@file .c

This module is used with the UnitTest target. When running that target, before entering the main loop a series of unit
tests are run to test various code functions. This file holds the code necessary to run the various tests.

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2017 Aclara.  All Rights Reserved.
***********************************************************************************************************************/

/***********************************************************************************************************************
#INCLUDES
***********************************************************************************************************************/

#include <assert.h>

#include "UnitTest.h"

#include "afc.h"
#include "Atime.h"
#include "datalogging.h"
#include "fmem_pdd.h"
#include "fmem.h"
#include "MTU.h"
#include "Si446x_driver.h"
#include "memory.h"
#include "stream.h"


#ifdef NDEBUG
#error "This file shouldn't be compiled on release targets!"
#endif

/***********************************************************************************************************************
Local #defines (Object-like macros)
***********************************************************************************************************************/

/***********************************************************************************************************************
Local #defines (Function-like macros)
***********************************************************************************************************************/

/***********************************************************************************************************************
Local Struct, Typedef, and Enum Definitions (Private to this file)
***********************************************************************************************************************/

/***********************************************************************************************************************
Static Function Prototypes
***********************************************************************************************************************/

/***********************************************************************************************************************
Global Variable Definitions
***********************************************************************************************************************/

uint16_t ucnt;
uint16_t urc;

/***********************************************************************************************************************
Static Variable Definitions
***********************************************************************************************************************/


/***********************************************************************************************************************
Function Definitions
***********************************************************************************************************************/


void UT_Init(void)
{
   CP.UT_lineOfAssertionFailure = 0;
}


void UT_assert(bool assertion, uint16_t line)
{
   if (!assertion)
   {
      if (CP.UT_lineOfAssertionFailure == 0)
      {
         CP.UT_lineOfAssertionFailure = line;
      }
   }
}



void UT_RunAllEnabledUnitTests(void)
{
#if (PMEM_TESTS == 1)
   assert(!Pmem_UnitTests());
#endif

#if (FMEM_TESTS == 1)
   assert(!fmem_UnitTests());
#endif

#if (EVENT_SYSTEM_TESTS == 1)
   assert(!ES_UnitTests());
#endif

#if (STREAM_TESTS == 1)
   assert(!Stream_UnitTests());
#endif

#if (PERIODIC_TEMP_AND_VOLTAGE_SAMPLING_TESTS == 1)
   assert(!PeriodicTempVoltageSampling_UnitTests());
#endif

#if (RTC_CALIBRATION_TESTS == 1)
   assert(!RtcCalibration_UnitTests());
#endif

#if (DATALOG_TESTS == 1)
   assert(!Datalog_UnitTests());
#endif
}

#endif
