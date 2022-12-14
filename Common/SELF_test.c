/**********************************************************************************************************************
 *
 * Filename:   SELF_test.c
 *
 * Global Designator: SELF_
 *
 * Contents: routines to support the manufacturing port
 *
 **********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 *********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include <stdlib.h>
#if ( MCU_SELECTED == NXP_K24 )
#include <mqx.h>
#include <rtc.h>
#include <fio.h>
#include <bsp.h>
#endif
/*lint -esym(750,SELF_GLOBALS) not referenced   */
#define SELF_GLOBALS
#include "SELF_test.h"    // Include self
#undef  SELF_GLOBALS

#include "DBG_SerialDebug.h"
//#include "ascii.h"
//#include "buffer.h"
#include "partition_cfg.h"
//#include "file_io.h"
#include "ecc108_lib_return_codes.h"
//#include "ecc108_mqx.h"
#include "ecc108_apps.h"
#include "EVL_event_log.h"
//#include "time_sys.h"
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
#include "hmc_display.h"
#endif
#if ( USE_USB_MFG != 0 )
#include "virtual_com.h"
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#if ( RTOS_SELECTION == MQX_RTOS )
#define SELF_TEST_EVENT_MASK             0xFFFFFFFF
#elif ( RTOS_SELECTION == FREE_RTOS )
#define SELF_TEST_EVENT_MASK             0x0000000F
#endif
#if ( TM_EVENTS == 1 )
#define OS_TEST_BIT_0 ( 1 << 0 )
#define OS_TEST_BIT_4 ( 1 << 4 )
#endif
#if( TM_QUEUE == 1)
#define OS_TEST_NUM_ITEMS 10
#endif
#if (TM_MSGQ ==1 )
#define OS_TEST_MSGQ_DATA 94;
#endif
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
#if ( DCU == 1 )  /* DCU will always support external RAM */
static uint32_t      sdRAM[32] @ "EXTERNAL_RAM" ; /*lint !e430*/
#endif

static OS_EVNT_Obj      SELF_events;      /* Self test Events   */
static OS_EVNT_Obj *    SELF_notify;      /* Event handler to "notify" when test results are completed.     */
static uint32_t         event_flags;      /* Event flags returned by self test routine.                     */
static SELF_TestData_t  SELF_TestData;
static SELF_file_t      SELF_testFile =
{
   .ePartition      = ePART_SEARCH_BY_TIMING,
   .FileName        = "SELF_Test",
   .FileId          = eFN_SELF_TEST,                  /* Self test   */
   .Attr            = FILE_IS_NOT_CHECKSUMED,
   .Data            = &SELF_TestData,
   .Size            = sizeof(SELF_TestData),
   .UpdateFreq      = DVR_BANKED_MAX_UPDATE_RATE_SEC  /* Updated often enough to force into banked area. */
}; /*lint !e785 too few initializers.  */

// For internal flash checking
uint8_t flashBuffer[512];
uint8_t checkFlashBuffer[512];

#if ( MCU_SELECTED == RA6E1 )
static const enum_UART_ID mfgPortUart = UART_MANUF_TEST;     /* UART used for MFG port operations   */
#endif
#if ( ( TEST_MODE_ENABLE == 1) && ( ( TM_SEMAPHORE == 1 ) || ( TM_MSGQ == 1 ) || ( TM_EVENTS == 1 ) || ( TM_QUEUE == 1 ) || ( TM_MUTEX == 1 ) ) )
const char pTskName_osTestModule[]      = "OS_TEST_MODULE";
#endif
#if ( TM_SEMAPHORE == 1 )
static OS_SEM_Obj       OS_TestSem = NULL;
static volatile uint8_t OS_TestSemCounter = 0;
#endif
#if (TM_MSGQ ==1 )
static OS_MSGQ_Obj             OS_TestMsgQueue;
bool retVal = false;
static buffer_t OS_TestMsgqPayload;
static buffer_t *OS_TestMsgqPtr = &OS_TestMsgqPayload;
static buffer_t *OS_TestRxMsg;
static volatile uint8_t OS_TestMsgqCounter;
static uint8_t OS_TestMsgqData = OS_TEST_MSGQ_DATA;
#endif
#if ( TM_EVENTS == 1 )
static OS_EVNT_Obj OS_TestEventObj;
static OS_EVNT_Handle OS_TestEventHandle = &OS_TestEventObj;
#endif
#if( TM_QUEUE == 1)
bool retValStatus = false;
static buffer_t OS_TestQueuePayload;
static buffer_t *OS_TestQueuePtr = &OS_TestQueuePayload;
static OS_QUEUE_Obj OS_TestMsgQueueObj;
static OS_QUEUE_Handle OS_TestMsgQueueHandle = &OS_TestMsgQueueObj;
static buffer_t *OS_TestRxMsg;
#endif
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

static uint16_t RunSelfTest( void );
#if ( ( TEST_MODE_ENABLE == 1) && ( ( TM_SEMAPHORE == 1 ) || ( TM_MSGQ == 1 ) || ( TM_EVENTS == 1) || ( TM_QUEUE == 1 ) || ( TM_MUTEX == 1) ) )
static returnStatus_t OS_testModules( void );
static void SELF_testModulesTask ( taskParameter );
#endif

#if ( TM_SEMAPHORE == 1 )
static void SELF_testSemPost( void );
static void SELF_testSemCreate( void );
static bool SELF_testSemPend( void );
#endif
#if (TM_MSGQ == 1)
static void SELF_testMsgqPost( void );
static void SELF_testMsgqCreate( void );
static bool SELF_testMsgqPend( void );
#endif

#if( TM_EVENTS == 1)
static void SELF_testEventCreate( void );
static bool SELF_testEventWait( void );;
static void SELF_testEventSet( void );
#endif

#if (TM_MUTEX == 1)
static bool SELF_testMutex( void );
#endif

#if( TM_QUEUE == 1)
static bool SELF_testQueue( void );
#endif


/***********************************************************************************************************************
   Function Name: SELF_init

   Purpose: Initializes the MFG/SELF test related items.

   Arguments: none

   Returns: eSUCCESS if all went well.  eFAILURE if not
***********************************************************************************************************************/
returnStatus_t SELF_init( void )
{
   returnStatus_t retVal = eFAILURE;
   FileStatus_t   fileStatus;
   SELF_file_t    *pFile = &SELF_testFile;

   if ( eSUCCESS == FIO_fopen(&pFile->handle,     /* File Handle so that MFG_Port can access the file. */
                              pFile->ePartition,  /* Search for the best partition according to the timing. */
                              (uint16_t) pFile->FileId, /* File ID (filename) */
                              pFile->Size,        /* Size of the data in the file. */
                              pFile->Attr,        /* File attributes to use. */
                              pFile->UpdateFreq,  /* The update rate of the data in the file. */
                              &fileStatus)        /* Contains the file status */
        )
   {
      if ( fileStatus.bFileCreated )
      {  // The file was just created for the first time.
         (void)memset( &SELF_TestData, 0, sizeof(SELF_TestData) );
         retVal = FIO_fwrite( &pFile->handle, 0, (uint8_t *)pFile->Data, pFile->Size);
      }
      else
      {  //Read the SELF_test File Data
         retVal = FIO_fread( &pFile->handle, (uint8_t *)pFile->Data, 0, pFile->Size);
      }
      if ( eSUCCESS == retVal )
      {  /* Create an event pool */
         if ( OS_EVNT_Create ( &SELF_events ) )
         {
            retVal = eSUCCESS;
         }
      }
   }

   return(retVal);
}

/***********************************************************************************************************************
   Function Name: SELF_eventHandle

   Purpose: Returns the event handle used to invoke the self tests.

   Arguments: none

   Returns: pointer to event handle
***********************************************************************************************************************/
OS_EVNT_Obj * SELF_getEventHandle( void )
{
   return &SELF_events;
}
/***********************************************************************************************************************
   Function Name: SELF_eventHandle

   Purpose: Returns the event handle used to invoke the self tests.

   Arguments: none

   Returns: pointer to event handle
***********************************************************************************************************************/
void SELF_setEventNotify( OS_EVNT_Obj *handle )
{
   SELF_notify = handle;
}

/***********************************************************************************************************************
   Function Name: SELF_testTask

   Purpose: Runs power up self test and schedules it to run every 24 hours after power up.

   Arguments: Arg0 - an unused MQX required argument for tasks

   Returns: none
***********************************************************************************************************************/
void SELF_testTask( taskParameter )
{
#if ( RTOS_SELECTION == MQX_RTOS )
   (void)Arg0;    /* Not used - avoids lint warning   */
#endif
   uint16_t       selfTestResults;

   DBG_logPrintf( 'I', "SELF_testTask: Up time = %ld ms", OS_TICK_Get_ElapsedMilliseconds() );

#if ( RTOS_SELECTION == MQX_RTOS )
#if ( USE_USB_MFG == 0 )
   MQX_FILE_PTR   stdout_ptr;       /* mqx file pointer for UART  */
   if (NULL != (stdout_ptr = fopen(MFG_PORT_IO_CHANNEL, NULL)))
   {
#if 0
      (void)_io_set_handle(IO_STDOUT, stdout_ptr);
#endif // if 0
   }
#endif
#endif // if MQX_RTOS
   selfTestResults = RunSelfTest();       /* Run once during the init phase   */
#if ( RTOS_SELECTION == MQX_RTOS )
#if ( USE_USB_MFG == 0 )
   (void)fprintf( stdout_ptr, "%s %04X\n", "SelfTest", selfTestResults );
   OS_TASK_Sleep( 20 );
   (void)fflush( stdout_ptr );
   (void)fclose( stdout_ptr );
#else
   char resBuff[16];
   (void)snprintf( resBuff, sizeof( resBuff ), "%s %04X\n", "SelfTest", selfTestResults );
   for ( uint8_t i = 0; i < 10; i++ ) /* Try up to 10 times, since usb_puts will suspend until sent. Could prevent daily self test from running.  */
   {
      if( USB_isReady() )
      {
         usb_puts( resBuff );
         if ( SELF_notify != NULL )
         {
            OS_EVNT_Set ( SELF_notify, 1U );
         }
         break;
      }
      else
      {
         OS_TASK_Sleep( 1000U ); /* Wait 1 second before trying again.   */
      }
   }
#endif //USE_USB_MFG
#elif ( RTOS_SELECTION == FREE_RTOS )
   char resBuff[18];
   uint32_t len = snprintf( resBuff, sizeof( resBuff ), "%s %04X\r\n", "SelfTest", selfTestResults );
   ( void )UART_write( mfgPortUart, (uint8_t*)resBuff, len );
   if ( SELF_notify != NULL )
   {
      OS_EVNT_Set ( SELF_notify, 1U );
   }
#endif //RTOS_SELECTION
   /* Now, lower the task priority to just higher than the IDLE task.  */
   (void)OS_TASK_Set_Priority ( pTskName_Test, LOWEST_TASK_PRIORITY_ABOVE_IDLE() );

   for (;;)                                                    /* Task Loop */
   {
      /* Wait for an event or 24 hours to elapse   */
      event_flags = OS_EVNT_Wait ( &SELF_events, SELF_TEST_EVENT_MASK, (bool)false , ONE_MIN * 60 * 24 );
      if ( event_flags != 0 )
      {
         //NOTE: Selftests initiated by MFg port do not generate Fail/Succeed Events
         if ( ( event_flags & (uint32_t)( 1 << (uint16_t)e_nvFail ) ) != 0 )
         {
            (void)SELF_testNV();
            if ( SELF_notify != NULL )
            {
               OS_EVNT_Set ( SELF_notify, (uint32_t)( 1 << (uint16_t)e_nvFail ) );
            }
         }
         if ( ( event_flags & (uint32_t)( 1 << (uint16_t)e_securityFail ) ) != 0  )
         {
            (void)SELF_testSecurity();
            if ( SELF_notify != NULL )
            {
               OS_EVNT_Set ( SELF_notify, (uint32_t)( 1 << (uint16_t)e_securityFail ) );
            }
         }
#if ( DCU == 1 )  /* DCU will always support external RAM */
         if ( ( event_flags & (uint32_t)( 1 << (uint16_t)e_sdRAMFail ) ) != 0 )
         {
            (void)SELF_testSDRAM( 1L );
            if ( SELF_notify != NULL )
            {
               OS_EVNT_Set ( SELF_notify, (uint32_t)( 1 << (uint16_t)e_sdRAMFail ) );
            }
         }
#endif
         /* Run this test last to maximize power up time; allows RTC oscillator to start/stabilize.   */
         if ( ( event_flags & (uint32_t)( 1 << (uint16_t)e_RTCFail ) ) != 0  )
         {
            (void)SELF_testRTC();
            if ( SELF_notify != NULL )
            {
               OS_EVNT_Set ( SELF_notify, (uint32_t)( 1 << (uint16_t)e_RTCFail ) );
            }
         }
      }
      else  /* Timed out - perform daily self tests   */
      {
         (void)RunSelfTest();
      }
   }
}

/***********************************************************************************************************************
   Function Name: RunSelfTest

   Purpose: Runs power up self test and updates NV files, as necessary.

   Arguments: none

   Returns: none
***********************************************************************************************************************/
static uint16_t RunSelfTest()
{
   EventData_s         eventData;
   EventKeyValuePair_s keyVal;                            /* Logging only one NVP */

#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
   uint8_t        buf[HMC_DISP_MSG_LENGTH + 1];
#endif

#if ( EP == 1 )
   LED_setRedLedStatus(SELFTEST_RUNNING);
#endif

   (void)memset( ( uint8_t * )&eventData, 0, sizeof( eventData ) );
   (void)memset( ( uint8_t * )&keyVal, 0, sizeof( keyVal ) );

   /* Test external NV memory */
   if( eSUCCESS == SELF_testNV() )
   {
      if ( SELF_TestData.lastResults.uAllResults.Bits.nvFail )  //If this had failed...
      {

         SELF_TestData.lastResults.uAllResults.Bits.nvFail = 0;
         eventData.markSent                    = (bool)false;
         eventData.eventId                     = (uint16_t)comDeviceMemoryNVramSucceeded;
         (void)EVL_LogEvent( 90, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
      }
   }
   else
   {
      if ( !SELF_TestData.lastResults.uAllResults.Bits.nvFail )  //If this had passed...
      {
         SELF_TestData.lastResults.uAllResults.Bits.nvFail = 1;
         eventData.markSent                    = (bool)false;
         eventData.eventId                     = (uint16_t)comDeviceMemoryNVramFailed;
         eventData.eventKeyValuePairsCount     = 1;
         eventData.eventKeyValueSize           = sizeof ( SELF_TestData.nvFail );
         *( uint16_t * )keyVal.Key             = stNvmRWFailCount;
         *( uint16_t * )keyVal.Value           = SELF_TestData.nvFail;
         (void)EVL_LogEvent( 180, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
      }
   }

   /* Test the security device   */
   if( eSUCCESS == SELF_testSecurity() )
   {
      if ( SELF_TestData.lastResults.uAllResults.Bits.securityFail )  //If this had failed...
      {
         SELF_TestData.lastResults.uAllResults.Bits.securityFail = 0;
#if 0 //2021 Mar 01, SG: A succeeded event for the Security Self Test has not been define
         eventData.markSent                          = (bool)false;
         eventData.eventId                           = (uint16_t)???;
         (void)EVL_LogEvent( 999, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
#endif
      }

   }
   else
   {
      if ( !SELF_TestData.lastResults.uAllResults.Bits.securityFail )  //If this had passed...
      {
         SELF_TestData.lastResults.uAllResults.Bits.securityFail = 1;
         eventData.markSent                          = (bool)false;
         eventData.eventId                           = (uint16_t)comDeviceSecurityTestFailed;
         eventData.eventKeyValuePairsCount           = 1;
         eventData.eventKeyValueSize                 = sizeof ( SELF_TestData.securityFail );
         *( uint16_t * )keyVal.Key                   = stSecurityFailCount;
         *( uint16_t * )keyVal.Value                 = SELF_TestData.securityFail;
         (void)EVL_LogEvent( 196, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
      }
   }

#if ( DCU == 1 )   /* DCU will always support external RAM */
   if(  eSUCCESS == SELF_testSDRAM( 1L ) )   /* Test the external SDRAM */
   {
      if ( SELF_TestData.lastResults.uAllResults.Bits.sdRAMFail )  //If this had failed...
      {
         SELF_TestData.lastResults.uAllResults.Bits.sdRAMFail = 0;
         eventData.markSent                       = (bool)false;
         eventData.eventId                        = (uint16_t)comDeviceMemoryRAMSucceeded;
         (void)EVL_LogEvent( 90, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
      }
   }
   else
   {
      if ( !SELF_TestData.lastResults.uAllResults.Bits.sdRAMFail )  //If this had passed...
      {
         SELF_TestData.lastResults.uAllResults.Bits.sdRAMFail = 1;
         eventData.markSent                       = (bool)false;
         eventData.eventId                        = (uint16_t)comDeviceMemoryRAMFailed;
         eventData.eventKeyValuePairsCount        = 1;
         eventData.eventKeyValueSize              = sizeof ( SELF_TestData.SDRAMFail );
         *( uint16_t * )keyVal.Key                = stRamRWFailCount;
         *( uint16_t * )keyVal.Value              = SELF_TestData.SDRAMFail;
         (void)EVL_LogEvent( 180, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
      }
   }
#endif
#if 0 // TODO: RA6: DG: To review and Add
   // Can be removed if not required - Currently in place for testing
   if( eSUCCESS != SELF_testInternalFlash() )
   {
      // TODO: Error handling
      printf("ERROR: SELF_Test IF");
   }

   if( eSUCCESS != SELF_testTimeCompound() )
   {
      // TODO: Error handling
   }
#endif
#if ( ( TEST_MODE_ENABLE == 1) && ( ( TM_SEMAPHORE == 1 ) || ( TM_MSGQ == 1 ) || ( TM_EVENTS == 1 ) || ( TM_QUEUE == 1 ) || ( TM_MUTEX == 1) ) )
   SELF_TestData.lastResults.uAllResults.Bits.testModulesFail = 0;  //Clear to avoid garbage
   if( eSUCCESS != OS_testModules() )
   {
      SELF_TestData.lastResults.uAllResults.Bits.testModulesFail = 1;
   }
#endif

   /* Execute the RTC Test last (to allow sufficient time for VBAT to come up and the OSC to start?)
      otherwise if the RTC SuperCap is completely discharged on the T-board the RTC test may fail on
      first time from power up.  The EP's are OK when in a meter or if powered up standalone and
      deassert power fail some time afterwards.
   */
   if( eSUCCESS == SELF_testRTC() )       /* Test Real Time Clock */
   {
      if ( SELF_TestData.lastResults.uAllResults.Bits.RTCFail )  //If this had failed...
      {
         SELF_TestData.lastResults.uAllResults.Bits.RTCFail = 0;
         eventData.markSent                     = (bool)false;
         eventData.eventId                      = (uint16_t)comDeviceClockIOSucceeded;
         (void)EVL_LogEvent( 45, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
      }
   }
   else
   {
      if ( !SELF_TestData.lastResults.uAllResults.Bits.RTCFail )  //If this had passed...
      {
         SELF_TestData.lastResults.uAllResults.Bits.RTCFail = 1;
         eventData.markSent                     = (bool)false;
         eventData.eventId                      = (uint16_t)comDeviceClockIOFailed;
         eventData.eventKeyValuePairsCount      = 1;
         eventData.eventKeyValueSize            = sizeof ( SELF_TestData.RTCFail );
         *( uint16_t * )keyVal.Key              = stRTCFailCount;
         *( uint16_t * )keyVal.Value            = SELF_TestData.RTCFail;
         (void)EVL_LogEvent( 86, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
      }
   }

   /* Update the number of times self test has run */
   SELF_TestData.selfTestCount++;
   /* Update all the counters in the file */
   (void)FIO_fwrite( &SELF_testFile.handle, 0, (uint8_t *)SELF_testFile.Data, SELF_testFile.Size);

#if ( EP == 1 )
   /* Update the LED status based upon results of selftest */
   if ( SELF_TestData.lastResults.uAllResults.Bytes != 0  )
      LED_setRedLedStatus(SELFTEST_FAIL);
   else
      LED_setRedLedStatus(SELFTEST_PASS);
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
   (void)snprintf ( (char *)buf, sizeof( buf ), "ST%04X", SELF_TestData.lastResults.uAllResults.Bytes );
   HMC_DISP_UpdateDisplayBuffer(buf, HMC_DISP_POS_SELF_TEST_STATUS);
#endif
#endif
   return SELF_TestData.lastResults.uAllResults.Bytes;
}

/***********************************************************************************************************************
   Function Name: SELF_GetTestFileHandle

   Purpose: Return the SELF_test file handle

   Arguments: none

   Returns: none
***********************************************************************************************************************/
SELF_file_t *SELF_GetTestFileHandle( void )
{
   return &SELF_testFile;
}
/***********************************************************************************************************************
   Function Name: SELF_UpdateTestResults

   Purpose: Write the self test results to the file

   Arguments: none

   Returns: none
***********************************************************************************************************************/
returnStatus_t SELF_UpdateTestResults( void )
{
   returnStatus_t retVal;
   retVal = FIO_fwrite( &SELF_testFile.handle, 0, (uint8_t *)SELF_testFile.Data, SELF_testFile.Size);
   if( retVal == eSUCCESS )
   {
      retVal = FIO_fflush( &SELF_testFile.handle ); // Write back in flash
   }
   return retVal;
}

/***********************************************************************************************************************
   Function Name: SELF_testInternalFlash

   Purpose: Test the Internal flash

   Arguments: none

   Note:

   Returns: Internal flash test success/failure
***********************************************************************************************************************/
returnStatus_t SELF_testInternalFlash( void )
{
   returnStatus_t retVal;
   static PartitionData_t const *pDFWBLInfoPar_;
//   static PartitionData_t const *pDFWAppCode_;

   /* Test Internal flash mechanism */
   for (uint32_t index = 0; index < 512; index++)
   {
       flashBuffer[index] = 100;
   }

   /* BL Info partition test - BL info is stored in data flash */
   (void)PAR_partitionFptr.parInit();  // FileIO init has been done here
   retVal = PAR_partitionFptr.parOpen(&pDFWBLInfoPar_, ePART_DFW_BL_INFO, 0L);
   retVal |= PAR_partitionFptr.parErase(0, 4096, pDFWBLInfoPar_);
   retVal |= PAR_partitionFptr.parWrite( 0, ( uint8_t * )&flashBuffer[0], ( lCnt )sizeof( flashBuffer ), pDFWBLInfoPar_ );
   retVal |= PAR_partitionFptr.parRead(checkFlashBuffer, 0, ( lCnt ) sizeof( checkFlashBuffer ), pDFWBLInfoPar_ );
   if ( 0 != memcmp( flashBuffer, checkFlashBuffer, sizeof( flashBuffer ) ) )
   {
      retVal |= eFAILURE;
   }

   memset(checkFlashBuffer, 0, sizeof(checkFlashBuffer));

   /* App partition test - App code is stored in code flash */
//   retVal |= PAR_partitionFptr.parOpen( &pDFWAppCode_, ePART_APP_CODE, 0L );
//   /* TODO: ******** Remove ***********
//    *       This write will write the data at the location mentioned.
//    *       Hence it may lead to a crash of a system when the
//    *       code exists that location. Hence it should be removed later. Only added now for testing */
//   retVal |= PAR_partitionFptr.parWrite( 65536, ( uint8_t * ) &flashBuffer[0], ( lCnt )sizeof( flashBuffer ), pDFWAppCode_ );
//   retVal |= PAR_partitionFptr.parRead( checkFlashBuffer, 65536, ( lCnt )sizeof( checkFlashBuffer ), pDFWAppCode_ );
//   if ( 0 != memcmp( flashBuffer, checkFlashBuffer, sizeof( flashBuffer ) ) )
//   {
//      retVal |= eFAILURE;
//   }
//
//   memset( flashBuffer, 0, sizeof( flashBuffer ) );
//   memset( checkFlashBuffer, 0, sizeof( checkFlashBuffer ) );

   // Enable this to test large write of internal flash
   // retVal |= PAR_partitionFptr.parWrite( 0x80000, ( uint8_t * ) 0, ( lCnt )0x77FE0, pDFWAppCode_ );
   return retVal;
}

/***********************************************************************************************************************
   Function Name: SELF_testTimeCompound

   Purpose: Test the time compound API

   Arguments: none

   Note:

   Returns: success/failure
***********************************************************************************************************************/

returnStatus_t SELF_testTimeCompound( void )
{
   returnStatus_t retVal;
   uint8_t loopCnt = 20;
   OS_TICK_Struct startTick, currentTick;
   OS_TICK_Get_CurrentElapsedTicks( &startTick );
   while ( loopCnt-- > 0 )
   {
      OS_TICK_Get_CurrentElapsedTicks( &currentTick );
      if ( OS_TICK_Get_Diff_InMilliseconds( &startTick, &currentTick ) > 100 )
      {
         break;
      }

      OS_TASK_Sleep( 10 );
   }

   if ( ( loopCnt >= 8 ) && ( loopCnt < 11 ) )
   {
      retVal = eSUCCESS;
   }
   else
   {
      retVal = eFAILURE;
   }

   loopCnt = 20;
   OS_TICK_Get_CurrentElapsedTicks( &startTick );
   while ( loopCnt-- > 0 )
   {
      OS_TICK_Get_CurrentElapsedTicks( &currentTick );
      if ( OS_TICK_Get_Diff_InMicroseconds( &startTick, &currentTick ) > 100000 )
      {
         break;
      }

      OS_TASK_Sleep( 10 );
   }

   if ( ( loopCnt >= 8 ) && ( loopCnt < 11 ) )
   {
      retVal = eSUCCESS;
   }
   else
   {
      retVal = eFAILURE;
   }
   return retVal;
}

/***********************************************************************************************************************
   Function Name: SELF_testRTC

   Purpose: Test the Real Time Clock

   Arguments: none

   Note: Runs for up to 1.2 seconds after power up. RTC oscillator specs state typical 1000ms start up. We allow 20%
         over that.  If RTC prescalar doesn't change within that time, the test fails.
         In case of power up (especially a unit with caps discharged), this typically completes in 1 second or less.
         In case of running every 24 hours, the prescalar should certainly be running upon entry to this test, and the
         check is only attempted once.
   Returns: RTC test success/failure
***********************************************************************************************************************/
returnStatus_t SELF_testRTC( void )
{
   uint16_t       tries = 0;  /* Total number of attempts before exiting the loop.  */
   returnStatus_t retVal;

   DBG_logPrintf( 'I', "SELF_testRTC: Start - Up time = %ld ms", OS_TICK_Get_ElapsedMilliseconds() );
#if ( MCU_SELECTED == NXP_K24 )
   uint16_t       RTC_time_1; /* Used for difference in RTC prescalar   */
   uint16_t       RTC_time_2; /* Used for difference in RTC prescalar   */
   do
   {
      retVal = eSUCCESS;                        /* Assume success */
      tries++;

      (void)_rtc_get_prescale( &RTC_time_1 );   /* Record RTC seconds, at entry.    */
      OS_TASK_Sleep( TEN_MSEC );                /* Wait for 10ms.                   */
      (void)_rtc_get_prescale( &RTC_time_2 );   /* Record RTC seconds after wait.   */

      /* The prescaler cycles from 0 to 32767 then rollsover. If readings don't change, then not running.  */
      if ( RTC_time_1 == RTC_time_2 )
      {
         retVal = eFAILURE;
      }
   } while ( ( retVal == eFAILURE ) && ( OS_TICK_Get_ElapsedMilliseconds() < 1200 ) ); //TODO: K24: SG: Verify the Elapsed actually works
#elif ( MCU_SELECTED == RA6E1 )
   uint32_t       RTC_time_1;    /* Used for difference in RTC */
   uint32_t       RTCTime1_msec; /* Used for difference in RTC prescalar   */
   uint32_t       RTC_time_2;    /* Used for difference in RTC */
   uint32_t       RTCTime2_msec; /* Used for difference in RTC prescalar   */
   OS_TICK_Struct startTick, currentTick;
   OS_TICK_Get_CurrentElapsedTicks( &startTick );
   do
   {
      retVal = eSUCCESS;                        /* Assume success */
      tries++;

      RTC_GetTimeInSecMicroSec( &RTC_time_1, &RTCTime1_msec );   /* Record RTC seconds, at entry.    */
      OS_TASK_Sleep( TEN_MSEC );                /* Wait for 10ms.                   */
      RTC_GetTimeInSecMicroSec( &RTC_time_2, &RTCTime2_msec );   /* Record RTC seconds after wait.   */

      /* The "prescaler" cycles from 0 to 127 then rollsover. If readings don't change, then not running.  */
      if ( ( RTC_time_1 == RTC_time_2 ) && ( RTCTime1_msec == RTCTime2_msec ) )
      {
         retVal = eFAILURE;
      }
      OS_TICK_Get_CurrentElapsedTicks( &currentTick );
   } while ( ( retVal == eFAILURE ) && ( OS_TICK_Get_Diff_InMilliseconds( &startTick, &currentTick ) < 1200 ) );
#endif
   if ( retVal == eFAILURE )
   {
      if( SELF_TestData.RTCFail < ( ( 1 << ( 8 * sizeof( SELF_TestData.RTCFail ) ) ) - 1 ) ) /* Do not let counter rollover.  */
      {
         SELF_TestData.RTCFail++;
      }
   }

   DBG_logPrintf( 'I', "SELF_testRTC: Done - Up time = %ld ms, attempts: %hd", OS_TICK_Get_ElapsedMilliseconds(), tries );
   return retVal;
}

/***********************************************************************************************************************
   Function Name: SELF_testSecurity

   Purpose: Test the Atmel security device

   Arguments: none

   Returns: Security test success/failure
***********************************************************************************************************************/
returnStatus_t SELF_testSecurity( void )
{
   returnStatus_t retVal = eFAILURE;

   DBG_logPrintf( 'I', "SELF_testSecurity: Up time = %ld ms", OS_TICK_Get_ElapsedMilliseconds() );
   if( ECC108_SUCCESS == ecc108e_bfc() ) /* Run BFC - is device present and communicating?  */
   {
      if ( ECC108_SUCCESS == ecc108e_SelfTest() )
      {
         retVal = eSUCCESS;
      }
   }
#if ( MCU_SELECTED == RA6E1 ) //TODO: K24: SG: Add this to the next release of the K24
   DBG_logPrintf( 'I', "SELF_testSecurity: Done - Up time = %ld ms", OS_TICK_Get_ElapsedMilliseconds() );
#endif
   if ( eFAILURE == retVal )
   {
      /* If any failure, update the counter w/o overflow  */
      if( SELF_TestData.securityFail  < ( ( 1 << ( 8 * sizeof( SELF_TestData.securityFail ) ) ) - 1 ) )
      {
         SELF_TestData.securityFail++;
      }
   }
   return retVal;
}

/***********************************************************************************************************************
   Function Name: SELF_testNV

   Purpose: Test the external NV device

   Arguments: none

   Returns: NV memory test success/failure
***********************************************************************************************************************/
returnStatus_t SELF_testNV( void )
{
   uint32_t errCount;         /* Number of failures reported by DVR_EFL_unitTest */

   DBG_logPrintf( 'I', "SELF_testNV: Up time = %ld ms", OS_TICK_Get_ElapsedMilliseconds() );

   errCount = DVR_EFL_UnitTest(1L); //External-FLash UnitTest
   if( errCount != 0 )     /* Need to update the failure counter? */
   {
      /* Update counter, avoiding overflow   */
      if ( ( errCount + SELF_TestData.nvFail ) < errCount )
      {
         SELF_TestData.nvFail  = ( 1 << ( 8 * sizeof( SELF_TestData.nvFail ) ) ) - 1;
      }
      else
      {
         SELF_TestData.nvFail += (uint16_t)errCount;
      }
   }

#if ( MCU_SELECTED == RA6E1 ) //TODO: K24: SG: Add this to the next release of the K24
   DBG_logPrintf( 'I', "SELF_testNV: Done - Up time = %ld ms", OS_TICK_Get_ElapsedMilliseconds() );
#endif
   return ( ( 0 == errCount ) ? eSUCCESS : eFAILURE );
}

#if ( DCU == 1 )  /* DCU will always support externam RAM */
/***********************************************************************************************************************
   Function Name: SELF_testSDRAM

   Purpose: Test the external SDRAM device

   Arguments: none

   Returns: NV memory test success/failure
***********************************************************************************************************************/
returnStatus_t SELF_testSDRAM( uint32_t LoopCount )
{
   returnStatus_t status;
   uint32_t       errorCount = 0;
   uint32_t       *sdptr;              /* Pointer to SDRAM  */
   uint32_t       i;

   DBG_logPrintf( 'I', "SELF_testSDRAM: Up time = %ld ms", OS_TICK_Get_ElapsedMilliseconds() );
   /*lint -e{443} status in first clause of for, but not in 3rd OK   */
   for ( status = eSUCCESS; LoopCount && (status == eSUCCESS); LoopCount-- )
   {
      /* Fill sector with pattern (data = address low byte inverted) */
      sdptr = sdRAM;
      for( i = 0; i < sizeof(sdRAM)/sizeof(*sdptr); i++ , sdptr++ )
      {
         *sdptr = ~(uint32_t)sdptr;
      }
      /* Check sector for pattern (data = address low byte) */
      sdptr = sdRAM;
      for( i = 0; i < sizeof(sdRAM)/sizeof(*sdptr); i++ , sdptr++ )
      {
         if ( *sdptr != ~(uint32_t)sdptr )
         {
            errorCount++;
            status = eFAILURE;
         }
      }
   }
   /* Update counter, avoiding overflow   */
   if ( ( errorCount + SELF_TestData.SDRAMFail ) < errorCount )
   {
      SELF_TestData.SDRAMFail  = ( 1 << ( 8 * sizeof( SELF_TestData.SDRAMFail ) ) ) - 1;
   }
   else
   {
      SELF_TestData.SDRAMFail += (uint16_t)errorCount;
   }
   return status;
}
#endif // ( DCU == 1 )

/***********************************************************************************************************************

   Function Name: SELF_OR_PM_Handler

   Purpose: Get/Set parameters related to SELF_test

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t SELF_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal; // Success/failure
   SELF_TestData_t   *SelfTestData;
   SelfTestData = SELF_GetTestFileHandle()->Data;

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case stRTCFailCount :
         {
            if ( sizeof(SelfTestData->RTCFail) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint16_t *)value = SelfTestData->RTCFail;
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(SelfTestData->RTCFail);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case stSecurityFailCount :
         {
            if ( sizeof(SelfTestData->securityFail) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint16_t *)value = SelfTestData->securityFail;
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(SelfTestData->securityFail);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case stNvmRWFailCount:
         {
            if ( sizeof(SelfTestData->nvFail) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint16_t *)value = SelfTestData->nvFail;
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(SelfTestData->nvFail);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case stRTCFailCount :
         {
            SelfTestData->RTCFail = *(uint16_t *)value;
            retVal = SELF_UpdateTestResults();   /* Update the file values  */
            break;
         }
         case stSecurityFailCount :
         {
            SelfTestData->securityFail = *(uint16_t *)value;;
            retVal = SELF_UpdateTestResults();   /* Update the file values  */
            break;
         }
         case stNvmRWFailCount:
         {
            SelfTestData->nvFail = *(uint16_t *)value;;
            retVal = SELF_UpdateTestResults();   /* Update the file values  */
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   return ( retVal );
}


/***********************************************************************************************************************
   Function Name: SELF_testIWDT

   Purpose: Test the IWDT

   Arguments: none

   Returns: none
***********************************************************************************************************************/

void SELF_testIWDT( void )
{
   static uint32_t iwdt_counter;

   while ( TRUE )
   {
      /* Read the current IWDT counter value for debugging purpose. Can be watched in the live watch. */
      R_IWDT_CounterGet( &g_wdt0_ctrl, &iwdt_counter );
   }
}
/***********************************************************************************************************************
   Function Name: OS_testModules

   Purpose: Test modules

   Arguments: none

   Returns: none
***********************************************************************************************************************/
#if ( ( TEST_MODE_ENABLE == 1) && ( ( TM_SEMAPHORE == 1 ) || ( TM_MSGQ == 1 ) || ( TM_EVENTS == 1 ) || ( TM_QUEUE == 1 ) || ( TM_MUTEX == 1) ) )
static returnStatus_t OS_testModules( void )
{
static uint8_t testModeCount = 0;
//Step 1 : Create the modules for testing
   returnStatus_t  retVal;
#if ( TM_SEMAPHORE == 1 )
   SELF_testSemCreate();
#endif
#if ( TM_MSGQ == 1 )
   SELF_testMsgqCreate();
#endif
#if ( TM_EVENTS == 1 )
   SELF_testEventCreate();
#endif

//Step 2: Create task for test the modules
   const OS_TASK_Template_t  semTaskTemplate[1] =
   {
      /* Task Index, Function,   Stack, Pri, Name,           tributes, Param, Time Slice */
      { 1,      SELF_testModulesTask, 500, 38, (char *)pTskName_osTestModule, 0, 0, 0 },
   };
   OS_TASK_Template_t const *semTestTask;
   semTestTask = &semTaskTemplate[0];
   if ( pdPASS != OS_TASK_Create(semTestTask) )
   {
      DBG_logPrintf( 'R', "ERROR - Test mode Task Creation Failed" );
   }
//Step 3 : Test the modules
#if (TM_SEMAPHORE == 1)
   if( SELF_testSemPend() )
   {
      testModeCount++;
   }
#endif
#if (TM_MSGQ == 1)
   if( SELF_testMsgqPend() )
   {
      testModeCount++;
   }
#endif
#if (TM_EVENTS == 1)
   if( SELF_testEventWait() )
   {
      testModeCount++;
   }
#endif
#if (TM_MUTEX == 1)
   if ( SELF_testMutex() )
   {
      testModeCount++;
   }
#endif
#if (TM_QUEUE == 1)
   if ( SELF_testQueue() )
   {
      testModeCount++;
   }
#endif
   if(testModeCount == ( TM_SEMAPHORE + TM_MSGQ + TM_EVENTS + TM_MUTEX + TM_QUEUE ) )
   {
      retVal = eSUCCESS;
   }
   else
   {
      retVal = eFAILURE;
   }
   return retVal;
}

/***********************************************************************************************************************
   Function Name: SELF_testModulesTask

   Purpose:

   Arguments: taskParameter

   Returns: none
***********************************************************************************************************************/
static void SELF_testModulesTask ( taskParameter )
{
#if (TM_SEMAPHORE == 1)
   SELF_testSemPost();
#endif
#if (TM_MSGQ == 1)
   SELF_testMsgqPost();
#endif
#if (TM_EVENTS == 1)
   SELF_testEventSet();
#endif
   vTaskDelete(NULL);
}
#endif

#if (TM_SEMAPHORE == 1)

/***********************************************************************************************************************
   Function Name: SELF_testSemCreate

   Purpose: Create semaphore

   Arguments: none

   Returns: none
***********************************************************************************************************************/
static void SELF_testSemCreate ( void )
{
   if( OS_SEM_Create(&OS_TestSem, 0) )
   {
      OS_TestSemCounter++;
   }
   else
   {
      OS_TestSemCounter++;
      APP_ERR_PRINT("Unable to Create the Mutex!");
   }

}

/***********************************************************************************************************************
   Function Name: SELF_testSemPend

   Purpose: Pend semaphore

   Arguments: none

   Returns: none
***********************************************************************************************************************/
static bool SELF_testSemPend( void )
{
   bool retVal = false;
   if( OS_SEM_Pend(&OS_TestSem, HALF_SEC) )
   {
      OS_TestSemCounter--;
      retVal = true;
   }

   if( 0 == OS_TestSemCounter )
   {
      DBG_logPrintf( 'R', "Sem Test Success" );
   }
   else
   {
      DBG_logPrintf( 'R', "Sem Test Fail" );
   }
   return(retVal);
}

/***********************************************************************************************************************
   Function Name: SELF_testSemPost

   Purpose: Post semaphore

   Arguments: none

   Returns: none
***********************************************************************************************************************/
static void SELF_testSemPost( void )
{
   OS_SEM_Post(&OS_TestSem);
}

#endif

#if (TM_MSGQ ==1 )

/***********************************************************************************************************************
   Function Name: SELF_testMsgqCreate

   Purpose: Create msg queue

   Arguments: none

   Returns: none
***********************************************************************************************************************/
static void SELF_testMsgqCreate ( void )
{
   if( OS_MSGQ_Create(&OS_TestMsgQueue, 10, "testqueue") )
   {
      OS_TestMsgqCounter++;
   }
   else
   {
      OS_TestMsgqCounter++;
      DBG_logPrintf( 'R', "Unable to Create the Message Queue!" );
   }
   /*initialize static message*/
   OS_TestMsgqPayload.data = &OS_TestMsgqData;

}
/***********************************************************************************************************************
   Function Name: SELF_testMsgqPend

   Purpose: Pend msg queue

   Arguments: none

   Returns: none
***********************************************************************************************************************/
static bool SELF_testMsgqPend( void )
{
   bool retVal = false;

   if( OS_MSGQ_Pend(&OS_TestMsgQueue, (void *)&OS_TestRxMsg, HALF_SEC ) )
   {
      OS_TestMsgqCounter--;
      retVal = true;
   }

   if( OS_TestMsgqData ==  *(OS_TestRxMsg->data))
   {
      DBG_logPrintf( 'R', "Success MSGQ Test" );
   }
   else if( 0 != OS_TestMsgqCounter )
   {
      DBG_logPrintf( 'R', "Fail MSGQ Test" );
   }
   DBG_logPrintf( 'R', "Complete MSGQ Test" );
   return(retVal);
}
/***********************************************************************************************************************
   Function Name: SELF_testMsgqPost

   Purpose: Post msg queue

   Arguments: none

   Returns: none
***********************************************************************************************************************/
static void SELF_testMsgqPost( void )
{
   //post address of the txMsg to the Queue
   OS_MSGQ_Post(&OS_TestMsgQueue, (void *)OS_TestMsgqPtr);
}
#endif

#if( TM_EVENTS == 1 )
/***********************************************************************************************************************
   Function Name: SELF_testEventCreate

   Purpose: Create Event

   Arguments: none

   Returns: none
***********************************************************************************************************************/
static void SELF_testEventCreate(void)
{
  bool status;
  status = OS_EVNT_Create(OS_TestEventHandle);
  if( status )
  {
    DBG_logPrintf( 'R', "Created Event Object" );
  }
  else
  {
    DBG_logPrintf( 'R', "Failed to create Event Object" );
  }
  return;
}
/***********************************************************************************************************************
   Function Name: SELF_testEventWait

   Purpose: Event Wait

   Arguments: none

   Returns: none
***********************************************************************************************************************/
static bool SELF_testEventWait(void)
{
  EventBits_t recv;
  recv = OS_EVNT_Wait(OS_TestEventHandle, OS_TEST_BIT_4 | OS_TEST_BIT_0 , false, HALF_SEC);
  if( recv & OS_TEST_BIT_4 )
  {
    DBG_logPrintf( 'R', "Received Event 4" );
  }
  else if( recv & OS_TEST_BIT_0 )
  {
    DBG_logPrintf( 'R', "Received Event 0" );
  }
  else if( ( recv & ( OS_TEST_BIT_0 | OS_TEST_BIT_4 ) ) == ( OS_TEST_BIT_0 | OS_TEST_BIT_4 ) )
  {
     DBG_logPrintf( 'R', "Received both Event 0 and Event 4" );
  }
  else
  {
    DBG_logPrintf( 'R', "Timeout" );
    return false;
  }
  return true;
}
/***********************************************************************************************************************
   Function Name: SELF_testEventSet

   Purpose: Set Event

   Arguments: none

   Returns: none
***********************************************************************************************************************/
static void SELF_testEventSet(void)
{
  OS_EVNT_Set(OS_TestEventHandle, OS_TEST_BIT_4 );
  return;
}
#endif

#if (TM_MUTEX == 1)
/***********************************************************************************************************************
   Function Name: SELF_testMutex

   Purpose: Test the Mutex

   Arguments: none

   Returns: none
***********************************************************************************************************************/
bool SELF_testMutex( void )
{
   volatile uint8_t     mutexCounter = 0;
   static OS_MUTEX_Obj  testMutex_ = NULL;

   if( OS_MUTEX_Create(&testMutex_) )
   {
      mutexCounter++;
      OS_MUTEX_Lock(&testMutex_);
      /* Critical Section */
      mutexCounter--;
      OS_MUTEX_Unlock(&testMutex_);
   }
   else
   {
      mutexCounter++;
      DBG_logPrintf( 'R', "Unable to Create the Mutex!" );
      return FALSE;

   }
   if( 0 == mutexCounter )
   {
      DBG_logPrintf( 'R', "Mutex test Success" );
      return TRUE;
   }
   else
   {
      DBG_logPrintf( 'R', "Mutex test Fail" );
      return FALSE;
   }

}
#endif

#if( TM_QUEUE == 1)
/***********************************************************************************************************************
   Function Name: SELF_testQueue

   Purpose: Test the Queue

   Arguments: none

   Returns: none
***********************************************************************************************************************/
bool SELF_testQueue( void )
{
   OS_TestQueuePayload.bufMaxSize = 250;
   retValStatus = OS_QUEUE_Create( (void *) OS_TestMsgQueueHandle, OS_TEST_NUM_ITEMS, "OS_QUEUE_TEST");
   if(true == retValStatus)
   {
      OS_QUEUE_Enqueue( OS_TestMsgQueueHandle, (void *)OS_TestQueuePtr);
      OS_TestRxMsg = (buffer_t * )OS_QUEUE_Dequeue(OS_TestMsgQueueHandle);
      if( OS_TestRxMsg->bufMaxSize == 250 )
      {
         DBG_logPrintf( 'R', "OS_QUEUE Test Success" );
         return TRUE;
      }
      else
      {
         DBG_logPrintf( 'R', "OS_QUEUE Test Fail" );
         return FALSE;
      }
   }
   else
   {
      DBG_logPrintf( 'R', "OS_QUEUE Create Fail" );
      return FALSE;
   }
}
#endif
