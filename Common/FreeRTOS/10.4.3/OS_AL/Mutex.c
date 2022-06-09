/******************************************************************************
 *
 * Filename: Mutex.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2022 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
//#include <mqx.h>      /* TODO: RA6: DG: We might not need this as its already included in project.h */
//#include <mutex.h>    /* TODO: RA6: DG: We might not need this as its already included in project.h */
#include "OS_aclara.h"  /* TODO: RA6: DG: We might not need this as its already included in project.h */
//#include "DBG_SerialDebug.h"
//#include "EVL_event_log.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/*******************************************************************************

  Function name: OS_MUTEX_Create

  Purpose: This function will create a new Mutex

  Arguments: MutexHandle - pointer to the Mutex Object

  Returns: FuncStatus - True if Mutex created successfully, False if error

  Notes: Initial state of created Mutexes will be unlocked

*******************************************************************************/
bool OS_MUTEX_Create ( OS_MUTEX_Handle MutexHandle )
{
   bool FuncStatus = true;

   *MutexHandle = xSemaphoreCreateMutex();

   if ( MutexHandle == NULL )
   {
      FuncStatus = false;
//      APP_ERR_PRINT("OS_MUTEX_Create!");
      /* TODO: DG: Add Print */
      /* There was insufficient heap memory available for the mutex to be
         created. */
   }
   return ( FuncStatus );
} /* end OS_MUTEX_Create () */

/*******************************************************************************

  Function name: OS_MUTEX_Lock

  Purpose: This function will lock the passed in Mutex

  Arguments: MutexHandle - pointer to the Mutex Handle

  Returns: None

  Notes: xSemaphoreTake() must only be called from an executing task and therefore must not be called while the scheduler is in the Initialization state (prior to the scheduler being started).
         xSemaphoreTake() must not be called from within a critical section or while the scheduler is suspended.

  Notes: For each function call to Lock a Mutex there should be a corresponding
         unlock function call

         Function will not return if it fails

*******************************************************************************/
//static char const * const fmtSpec = "task: %s, file: %s, line: %d %s 0x%p\n";
void OS_MUTEX_LOCK ( OS_MUTEX_Handle MutexHandle, char *file, int line )
{

   if ( xSemaphoreTake ( *MutexHandle, portMAX_DELAY) != pdPASS ) //TODO: DG Decide on the delay; portMAX_DELAY
   {
//       APP_ERR_PRINT("OS_MUTEX_Lock!");
      /* TODO: DG: Print Mutex Could not be obtained */

//      switch (RetStatus) {
//         case MQX_CANNOT_CALL_FUNCTION_FROM_ISR:
//              // Dont use ERR_printf here. This function can be called before the logger is active and might never get to run
//              DBG_LW_printf("Function cannot be called from an ISR.\n");
//              break;
//
//         case MQX_EBUSY:
//              // Dont use ERR_printf here. This function can be called before the logger is active and might never get to run
//              DBG_LW_printf( fmtSpec, _task_get_template_ptr(_task_get_id())->TASK_NAME, file, line, "Mutex is already locked.", MutexHandle);
//              break;
//
//         case MQX_EDEADLK:
//              // Dont use ERR_printf here. This function can be called before the logger is active and might never get to run
//              DBG_LW_printf( fmtSpec, _task_get_template_ptr(_task_get_id())->TASK_NAME, file, line, "Task already has the mutex locked.",
//                             MutexHandle);
//              break;
//
//         case MQX_EINVAL:
//              // Dont use ERR_printf here. This function can be called before the logger is active and might never get to run
//              DBG_LW_printf( fmtSpec, _task_get_template_ptr(_task_get_id())->TASK_NAME, file, line,
//                             "One of the following: mutex_ptr is NULL or mutex was destroyed.", MutexHandle);
//              break;
//
//         default:
//              break;
//      }
//      EVL_FirmwareError( "OS_MUTEX_Lock" , file, line );
   } /* end if() */
} /* end OS_MUTEX_Lock () */

/*******************************************************************************

  Function name: OS_MUTEX_Unlock

  Purpose: This function will unlock the passed in Mutex

  Arguments: MutexHandle - pointer to the Mutex Handle

  Returns: None

  Notes: Although MQX can return false for the _mutex_unlock function, it should
         never happen since the mutex should always be valid.
         If this happens, we consider this a catastrophic failure.

         Function will not return if it fails

*******************************************************************************/
void OS_MUTEX_UNLOCK ( OS_MUTEX_Handle MutexHandle, char *file, int line )
{
   if ( pdFAIL == xSemaphoreGive ( *MutexHandle ) ) {
//      APP_ERR_PRINT("OS_MUTEX_Unlock!");
      /* TODO: Add the below code */
//      EVL_FirmwareError( "OS_MUTEX_Unlock" , file, line );
   }
} /* end OS_MUTEX_Unlock () */


#if (TM_MUTEX == 1)
void OS_MUTEX_Test( void )
{
   volatile uint8_t     counter = 0;
   static OS_MUTEX_Obj  testMutex_ = NULL;

   if( OS_MUTEX_Create(&testMutex_) )
   {
      counter++;
      OS_MUTEX_Lock(&testMutex_);
      /* Critical Section */
      counter--;
      OS_MUTEX_Unlock(&testMutex_);
   }
   else
   {
      counter++;
//      APP_ERR_PRINT("Unable to Create the Mutex!");

   }
   if( 0 == counter )
   {
//      APP_PRINT("Success");
   }
   else
   {
//      APP_ERR_PRINT("Fail");
   }

}
#endif