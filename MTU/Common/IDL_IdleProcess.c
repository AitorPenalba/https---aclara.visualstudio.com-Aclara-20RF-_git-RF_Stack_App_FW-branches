/******************************************************************************
 *
 * Filename: IDL_IdleProcess.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2020 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include <stdbool.h>
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#endif
#include "IDL_IdleProcess.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */
static volatile uint32_t IdleCounter = 0;

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/*lint -esym(715,Arg0) */
/*******************************************************************************

  Function name: IDL_IdleTask

  Purpose: This is the Idle process for the main application and runs when no
           other tasks are ready to execute

  Arguments: Arg0 - Not used, but required here because this is a task

  Returns:

  Notes: This task must always be the next to lowest priority task in the main application
         and should not share a task priority with any other tasks.

*******************************************************************************/
//TODO: adhere to new task parameter macro
#if ( RTOS_SELECTION == MQX_RTOS )
void IDL_IdleTask ( uint32_t Arg0 )
#elif ( RTOS_SELECTION == FREE_RTOS )
void IDL_IdleTask ( void* pvParameters )
#endif
{
   for ( ;; )
   {
      IdleCounter++;
      __asm volatile ("wfi"); // Put processor to sleep
   }
}
/*lint +esym(715,Arg0) */

/*******************************************************************************

  Function name: IDL_Get_IdleCounter

  Purpose: This function will return the current IdleCounter so CPU usage
           calculations can be performed

  Arguments:

  Returns: IdleCounter - number of cycles of the IDL_IdleTask over the last period

  Notes:

*******************************************************************************/
uint32_t IDL_Get_IdleCounter ( void )
{
  return ( IdleCounter );
}

