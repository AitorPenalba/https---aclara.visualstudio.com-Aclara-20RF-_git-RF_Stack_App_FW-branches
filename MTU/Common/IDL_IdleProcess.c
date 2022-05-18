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

/* These values are exact copies from freeRTOS and were copied here to avoid modifying the actual freeRTOS code.
   These values are used during the process of putting the processor to sleep during the idleTaskHook and defined
   the within port.c module.  They are used to unlock/lock LPM periphial register. */
#if ( RTOS_SELECTION == FREE_RTOS )
#define RM_FREERTOS_PORT_UNLOCK_LPM_REGISTER_ACCESS    (0xA502U)
#define RM_FREERTOS_PORT_LOCK_LPM_REGISTER_ACCESS      (0xA500U)
#endif

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */
static volatile uint32_t IdleCounter = 0;

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */
#if ( RTOS_SELECTION == FREE_RTOS )
void freertos_sleep(uint32_t xExpectedIdleTime);
#endif

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
#if ( RTOS_SELECTION == MQX_RTOS )
void IDL_IdleTask ( uint32_t Arg0 )
{
   for ( ;; )
   {
      IdleCounter++;
      __asm volatile ("wfi"); // Put processor to sleep
   }
}
/*lint +esym(715,Arg0) */
#endif

/*******************************************************************************

  Function name: freertos_sleep

  Purpose: Saves the LPM state, then enters sleep mode. After waking, reenables
           interrupts briefly to allow any pending interrupts to run.  This is an
           exact copy of rm_freertos_port_sleep_preserving_lpm() from port.c of freeRTOS.

  Arguments: xExpectedIdleTime Expected idle time in ticks

  Returns: None

  Notes: This function was copied and utilized here to avoid possible tweeks which would
         modify the existing freeRTOS code. If changes are made to this function,
         comment the code as an Aclara change.

*******************************************************************************/
#if ( RTOS_SELECTION == FREE_RTOS )
void freertos_sleep (uint32_t xExpectedIdleTime)
{
    uint32_t saved_sbycr = 0U;

    /* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
     * set its parameter to 0 to indicate that its implementation contains
     * its own wait for interrupt or wait for event instruction, and so wfi
     * should not be executed again. The original expected idle
     * time variable must remain unmodified, so this is done in a subroutine. */
    configPRE_SLEEP_PROCESSING(xExpectedIdleTime);
    if (xExpectedIdleTime > 0)
    {
        /* Save LPM Mode */
        saved_sbycr = R_SYSTEM->SBYCR;

        /** Check if the LPM peripheral is set to go to Software Standby mode with WFI instruction.
         *  If so, change the LPM peripheral to go to Sleep mode. */
        if (R_SYSTEM_SBYCR_SSBY_Msk & saved_sbycr)
        {
            /* Save register protect value */
            uint32_t saved_prcr = R_SYSTEM->PRCR;

            /* Unlock LPM peripheral registers */
            R_SYSTEM->PRCR = RM_FREERTOS_PORT_UNLOCK_LPM_REGISTER_ACCESS;

            /* Clear to set to sleep low power mode (not standby or deep standby) */
            R_SYSTEM->SBYCR = 0U;

            /* Restore register lock */
            R_SYSTEM->PRCR = (uint16_t) (RM_FREERTOS_PORT_LOCK_LPM_REGISTER_ACCESS | saved_prcr);
        }

        /**
         * DSB should be last instruction executed before WFI
         * infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
         */
        __DSB();

        /* If there is a pending interrupt (wake up condition for WFI is true), the MCU does not enter low power mode:
         * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/BABHHGEB.html
         * Note that interrupt will bring the CPU out of the low power mode.  After exiting from low power mode,
         * interrupt will be re-enabled. */
        __WFI();

        /* Instruction Synchronization Barrier. */
        __ISB();

        /* Re-enable interrupts to allow the interrupt that brought the MCU
         * out of sleep mode to execute immediately. This will not cause a
         * context switch because all tasks are currently suspended. */
        __enable_irq();
        __ISB();

        /* Disable interrupts again to restore the LPM state. */
        __disable_irq();
    }

    configPOST_SLEEP_PROCESSING(xExpectedIdleTime);

    /** Check if the LPM peripheral was supposed to go to Software Standby mode with WFI instruction.
     *  If yes, restore the LPM peripheral setting. */
    if (R_SYSTEM_SBYCR_SSBY_Msk & saved_sbycr)
    {
        /* Save register protect value */
        uint32_t saved_prcr = R_SYSTEM->PRCR;

        /* Unlock LPM peripheral registers */
        R_SYSTEM->PRCR = RM_FREERTOS_PORT_UNLOCK_LPM_REGISTER_ACCESS;

        /* Restore LPM Mode */
        R_SYSTEM->SBYCR = (uint16_t) saved_sbycr;

        /* Restore register lock */
        R_SYSTEM->PRCR = (uint16_t) (RM_FREERTOS_PORT_LOCK_LPM_REGISTER_ACCESS | saved_prcr);
    }
}
#endif

/*******************************************************************************

  Function name: vApplicationIdleHook

  Purpose: Used to put the processor to sleep and increment the NIC idle counter
           when the USE_IDLE_HOOK feature is enabled within freeRTOS.

  Arguments: None

  Returns: None

  Notes: This function supersedes the weak definition made by default freeRTOS.
         This function is an exact copy of the weak definition made by freeRTOS
         contained within the port.c module. It is utilized here to avoid possible tweeks
         which would modify the existing freeRTOS code. If changes are made to this,
         function, comment the code as an Aclara change

*******************************************************************************/
#if ( RTOS_SELECTION == FREE_RTOS )
void vApplicationIdleHook (void)
{
    /* Enter a critical section but don't use the taskENTER_CRITICAL() method as that will mask interrupts that should
     * exit sleep mode. This must be done before suspending tasks because a pending interrupt will prevent sleep from
     * WFI, but a task ready to run will not. If a task becomes ready to run before disabling interrupts, a context
     * switch will happen. */
    __disable_irq();

    IdleCounter++; // Aclara Added - increment the idle counter used for statistical data

    /* Don't allow a context switch during sleep processing to ensure the LPM state is restored
     * before switching from idle to the next ready task. This is done in the idle task
     * before vPortSuppressTicksAndSleep when configUSE_TICKLESS_IDLE is used. */
    vTaskSuspendAll();

    /* Save current LPM state, then sleep. */
    freertos_sleep(1);

    /* Exit with interrupts enabled. */
    __enable_irq();

    /* Allow context switches again. No need to yield here since the idle task yields when it loops around. */
    (void) xTaskResumeAll();
}
#endif


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

