/** ********************************************************************************************************************
@file UnitTest.h

Header file for the UnitTest module (see the C file for a description of the module).

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2017 Aclara.  All Rights Reserved.
***********************************************************************************************************************/

#ifndef UNITTEST_H_
#define UNITTEST_H_

/***********************************************************************************************************************
#INCLUDES
***********************************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>


/***********************************************************************************************************************
Public #defines (Object-like macros)
***********************************************************************************************************************/
// Warning: Please stick to this formatting...the build server will be enabling these one at a time...
// BUILD_SERVER_SECTION_START
// BUILD_SERVER_SECTION_END
#define PMEM_TESTS 0
#define FMEM_TESTS 0
#define EVENT_SYSTEM_TESTS 1
#define STREAM_TESTS 0
#define PERIODIC_TEMP_AND_VOLTAGE_SAMPLING_TESTS 0
#define RTC_CALIBRATION_TESTS 0
#define DATALOG_TESTS 0


/***********************************************************************************************************************
Public #defines (Function-like macros)
***********************************************************************************************************************/
#define UT_ASSERT(boolean)  UT_assert(boolean, __LINE__)



/***********************************************************************************************************************
Public Struct, Typedef, and Enum Definitions
***********************************************************************************************************************/


/***********************************************************************************************************************
Global Variable Extern Statements
***********************************************************************************************************************/


/***********************************************************************************************************************
Public Function Prototypes
***********************************************************************************************************************/
void UT_Init(void);
void UT_RunAllEnabledUnitTests(void);
void UT_assert(bool assertion, uint16_t line);


#endif //UNITTEST_H_






















































































