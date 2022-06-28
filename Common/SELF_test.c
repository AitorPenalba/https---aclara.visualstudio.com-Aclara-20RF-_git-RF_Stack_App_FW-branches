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
#include "CompileSwitch.h"
#include "hal_data.h"
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
#define SELF_TEST_EVENT_MASK             0x0000000F   /* TODO: RA6E1: Review */
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

/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

static uint16_t RunSelfTest( void );

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
   ( void )snprintf( resBuff, sizeof( resBuff ), "\r\n%s %04X\r\n", "SelfTest", selfTestResults );
   ( void )UART_write( mfgPortUart, (uint8_t*)resBuff, sizeof( resBuff ) );
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
#if ( MCU_SELECTED == NXP_K24 )
   // TODO: RA6 Melvin: LED Module not available
   LED_setRedLedStatus(SELFTEST_RUNNING);
#endif
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
#if 0
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
#else
   DBG_logPrintf( 'I', "Self Test completed" );
   return 0;
#endif
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
   } while ( ( retVal == eFAILURE ) && ( OS_TICK_Get_ElapsedMilliseconds() < 1200 ) );
#elif ( MCU_SELECTED == RA6E1 )
   sysTime_dateFormat_t setTime =
   {
   .sec  = 55,
   .min  = 59,
   .hour = 23,       /* 24-HOUR mode */
   .day = 31,
   .month  = 12,
   .year = 2021
   };
   sysTime_dateFormat_t getTime1;
   sysTime_dateFormat_t getTime2;
   bool isTimeSetSuccess;
   isTimeSetSuccess = RTC_SetDateTime (&setTime);// TODO: RA6 [name_Balaji]: Remove Set Time as this is not a valid method
   if(isTimeSetSuccess == 0)
   {
     retVal = eFAILURE;
   }
   do
   {
      retVal = eSUCCESS;                        /* Assume success */
      tries++;

      RTC_GetDateTime (&getTime1);              /* Record RTC milliseconds, at entry.    */
      OS_TASK_Sleep( 10 );                      /* Wait for 10ms.                   */
      RTC_GetDateTime (&getTime2);              /* Record RTC milliseconds after wait.   */

      /* If readings don't change, then not running.  */
      if ( getTime1.msec == getTime2.msec )
      {
         retVal = eFAILURE;
      }
   } while ( retVal == eFAILURE );// TODO: RA6 [name_Balaji]: Need to test after OS_TICK_Get_ElapsedMilliseconds integration
   R_RTC_Close (&g_rtc0_ctrl);// TODO: RA6 [name_Balaji]: Closing RTC, for now as there is no other known method to check the RTC valid or not
#endif
#if ( TM_RTC_UNIT_TEST == 1 )
// TODO: RA6 [name_Balaji]: Remove unit test if not required
   bool isRTCUnitTestFailed;
   isRTCUnitTestFailed = RTC_UnitTest();
   if (isRTCUnitTestFailed == 1)
   {
     DBG_printf( "ERROR - RTC failed to Set Error Adjustment\n" );
   }
#endif // TM_RTC_UNIT_TEST
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

#if ( MCU_SELECTED == RA6E1 )
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