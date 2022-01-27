/******************************************************************************
 *
 * Filename: Mutex.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2012 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include <stdint.h>
#include <stdbool.h>
#include <mqx.h>
#include <mutex.h>
#include "OS_aclara.h"
#include "DBG_SerialDebug.h"
#include "EVL_event_log.h"

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
   uint32_t RetStatus;
   bool FuncStatus = true;
   MUTEX_ATTR_STRUCT MutexAttr;

   RetStatus = _mutatr_init ( &MutexAttr );
   if ( RetStatus == MQX_EOK )
   {
      RetStatus = _mutatr_set_sched_protocol ( &MutexAttr, MUTEX_PRIO_INHERIT );
      if ( RetStatus == MQX_EOK )
      {
         RetStatus = _mutatr_set_wait_protocol ( &MutexAttr, MUTEX_PRIORITY_QUEUEING );
         if ( RetStatus == MQX_EOK )
         {
            RetStatus = _mutex_init ( MutexHandle, &MutexAttr );
            if ( RetStatus != MQX_EOK )
            {
               FuncStatus = false;
            } /* end if() */
         } /* end if() */
         else
         {
            FuncStatus = false;
         } /* end else() */
      } /* end if() */
      else
      {
         FuncStatus = false;
      } /* end else() */
   } /* end if() */
   else
   {
      FuncStatus = false;
   } /* end else() */
   return ( FuncStatus );
} /* end OS_MUTEX_Create () */

/*******************************************************************************

  Function name: OS_MUTEX_Lock

  Purpose: This function will lock the passed in Mutex

  Arguments: MutexHandle - pointer to the Mutex Object

  Returns: None

  Notes: Although MQX can return false for the _mutex_lock function, it should
         never happen.
         If this happens, we consider this a catastrophic failure.

  Notes: For each function call to Lock a Mutex there should be a corresponding
         unlock function call

         Function will not return if it fails

*******************************************************************************/
static char const * const fmtSpec = "task: %s, file: %s, line: %d %s 0x%p\n";
void OS_MUTEX_LOCK ( OS_MUTEX_Handle MutexHandle, char *file, int line )
{
   uint32_t RetStatus;

   RetStatus = _mutex_lock ( MutexHandle );
   if ( RetStatus != MQX_EOK )
   {
      switch (RetStatus) {
         case MQX_CANNOT_CALL_FUNCTION_FROM_ISR:
              // Dont use ERR_printf here. This function can be called before the logger is active and might never get to run
              DBG_LW_printf("Function cannot be called from an ISR.\n");
              break;

         case MQX_EBUSY:
              // Dont use ERR_printf here. This function can be called before the logger is active and might never get to run
              DBG_LW_printf( fmtSpec, _task_get_template_ptr(_task_get_id())->TASK_NAME, file, line, "Mutex is already locked.", MutexHandle);
              break;

         case MQX_EDEADLK:
              // Dont use ERR_printf here. This function can be called before the logger is active and might never get to run
              DBG_LW_printf( fmtSpec, _task_get_template_ptr(_task_get_id())->TASK_NAME, file, line, "Task already has the mutex locked.",
                             MutexHandle);
              break;

         case MQX_EINVAL:
              // Dont use ERR_printf here. This function can be called before the logger is active and might never get to run
              DBG_LW_printf( fmtSpec, _task_get_template_ptr(_task_get_id())->TASK_NAME, file, line,
                             "One of the following: mutex_ptr is NULL or mutex was destroyed.", MutexHandle);
              break;

         default:
              break;
      }
      EVL_FirmwareError( "OS_MUTEX_Lock" , file, line );
   } /* end if() */
} /* end OS_MUTEX_Lock () */

/*******************************************************************************

  Function name: OS_MUTEX_Unlock

  Purpose: This function will unlock the passed in Mutex

  Arguments: MutexHandle - pointer to the Mutex Object

  Returns: None

  Notes: Although MQX can return false for the _mutex_unlock function, it should
         never happen since the mutex should always be valid.
         If this happens, we consider this a catastrophic failure.

         Function will not return if it fails

*******************************************************************************/
void OS_MUTEX_UNLOCK ( OS_MUTEX_Handle MutexHandle, char *file, int line )
{
   if ( _mutex_unlock ( MutexHandle ) ) {
      EVL_FirmwareError( "OS_MUTEX_Unlock" , file, line );
   }
} /* end OS_MUTEX_Unlock () */

