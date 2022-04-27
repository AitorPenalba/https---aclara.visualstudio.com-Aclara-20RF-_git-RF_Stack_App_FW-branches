/***********************************************************************************************************************

   Filename: pwr_task.h

   Contents: Power Task - Monitors power down condition.  The module creates a semaphore and mutex when initialized.
             The task then waits for a semaphore given by the power down ISR.  Then waits for the mutex to start calling
             a list of functions to get the system ready to power down.  The last item in the power down list should be
             flushing the cache memory.  Once the cache memory is flushed, the unit will wait for power to die.  If
             power should come back and the unit 'debounces' the power signal, the processor will reset.

 ***********************************************************************************************************************
   Copyright (c) 2013 - 2020 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced,
   in whole or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA POWER-LINE SYSTEMS INC.
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************

   $Log$ kdavlin Created May 6, 2013

 **********************************************************************************************************************/
#ifndef pwr_task_H_
#define pwr_task_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "time_util.h"
#include "HEEP_util.h"
#if ( SIMULATE_POWER_DOWN == 1 )
#include "DBG_CommandLine.h"
#endif
/* ****************************************************************************************************************** */
/* Global Definition */
#if ( SIMULATE_POWER_DOWN == 1 )
extern OS_SEM_Obj    PWR_SimulatePowerDn_Sem;           /* Used for the simulation of power down interrupt */
#endif
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

#if ( RTOS_SELECTION == MQX_RTOS )
typedef void ( *vFPtr_strct )( OS_TASK_id );
#endif

PACK_BEGIN
typedef struct
{
   uint16_t             uPowerDownCount;
   /* Starting with uSpuriousResets value, through the JTAGResets, the entries must be
      in the order of bits in the RESET_SOURCE_XXXX bits in BSP_aclara.h   */
   uint16_t             uSpuriousResets;
   uint16_t             WatchDogResets;
   uint16_t             ClockLostResets;
   uint16_t             LowVoltageResets;
   uint16_t             LowLeakageWakeup;
   uint16_t             StopModeAckResets;
   uint16_t             EzPortResets;
   uint16_t             MDM_APResets;
   uint16_t             SoftwareResets;
   uint16_t             ArmCoreLockupResets;
   uint16_t             JTAGResets;
   uint16_t             uPowerAnomalyCount;
   uint64_t             uPowerDownSignature;
   sysTimeCombined_t    outageTime;                   /* System time at outage (final) */
} pwrFileData_t;
PACK_END

typedef enum
{
   PWR_MUTEX_ONLY = 0,         // use when multiple NV writes must complete (e.g. multiple files)
   EMBEDDED_PWR_MUTEX,         // use when multiple actions must complete where the action already uses PWR_MUTEX_ONLY (e.g. when logging multiple events)
   PWR_AND_EMBEDDED_PWR_MUTEX, // use at power down or reset to ensure PWR_MUTEX_ONLY and EMBEDDED_PWR_MUTEX are not locked
   LAST_VALID_PWR_MUTEX_STATE = PWR_AND_EMBEDDED_PWR_MUTEX
}enum_PwrMutexState_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/*
   PWR_task - Power down task - This should be the highest priority task in the system.
   @see Power Task.vsd

   @param  uint32_t Arg0
   @return None
*/

void PWR_task( taskParameter );

/*
   PWR_TSK_init - Create resources and open file

   @param  None
   @return returnStatus_t - eSUCCESS or eFAILURE
*/
returnStatus_t PWR_TSK_init( void );

/*
   PWR_lockMutex - Locks the mutex
   @see Power Task.vsd

   @param  enum_PwrMutexState_t
   @return None
*/
void PWR_lockMutex( enum_PwrMutexState_t reqMutexState );

/*
   PWR_unlockMutex - Unlocks the mutex
   @see Power Task.vsd

   @param  enum_PwrMutexState_t
   @return None
*/
void PWR_unlockMutex( enum_PwrMutexState_t reqMutexState );

/*
   PWR_waitForStablePower - De-bounces the power down signal and captures the reason for the reset.
   @see Power Task.vsd

   @param  None
   @return returnStatus_t
*/
returnStatus_t PWR_waitForStablePower( void );

/**
 * PWR_powerUp - De-bounces the power down signal and captures the reason for the reset.
 * @see Power Task.vsd
 *
 * @param  None
 * @return returnStatus_t
 */
returnStatus_t PWR_powerUp(void);

/*
   PWR_getResetCause - returns the reset cause

   @param  None
   @return uint16_t
*/
uint16_t PWR_getResetCause( void );

/*
   PWR_printResetCause - Prints the reset cause.  Called after the partitions have been initialized.
   @see Power Task.vsd

   @param  None
   @return returnStatus_t
*/
returnStatus_t PWR_printResetCause( void );

/*
   PWR_getPowerFailCount - Get Aclara Power Signal Fail Count.

   @param  None
   @return uint16_t
*/
uint16_t PWR_getPowerFailCount( void );

/*
   PWR_getPowerAnomalyCount - Get Aclara Power Anomaly Count.

   @param  uint32_t *pValue
   @return returnStatus_t
*/
returnStatus_t PWR_getPowerAnomalyCount( uint32_t *pValue );

/*
   isr_brownOut - Brown Out Interrupt Service Routing.

   @param  None
   @return None
*/
//void isr_brownOut( void );

/*
   PWR_setPowerFailCount - Sets power fail count.

   @param  uint16_t pwrFailCnt
   @return none
*/
void PWR_setPowerFailCount( uint32_t pwrFailCnt );
/*
   PWR_setPowerAnomalyCount - Sets blink count.

   @param  uint16_t powerAnomalyCount
   @return uint16_t
*/
uint16_t PWR_setPowerAnomalyCount( uint16_t powerAnomalyCount );

/*
   PWR_getSpuriousResetCnt - Returns spurious reset count.

   @param  None
   @return uint32_t
*/
uint32_t PWR_getSpuriousResetCnt( void );

/*

   PWR_getSpuriousResetCnt - Sets the spurious reset count

   @param  uint32_t
   @return None
*/
void PWR_setSpuriousResetCnt( uint32_t count );

/*

   PWR_SafeReset - Gracefully call a software reset.

   @param  None
   @return None
*/
void PWR_SafeReset( void );


/*
   PWR_getWatchDogResetCnt - Returns the WatchDog Reset Counter

   @param  uint32_t
   @return None
*/
uint32_t PWR_getWatchDogResetCnt( void );

/*
   PWR_setWatchDogResetCnt - Used to set the WatchDog Reset Counter

   @param  uint32_t
   @return None
*/
void PWR_setWatchDogResetCnt( uint16_t count );


/*

   PWR_getpwrFileData - Returns a pointer to the pwrFileData structure

   @param  uint32_t
   @return None
*/
void                    PWR_setOutageTime( sysTimeCombined_t outageTime );
sysTimeCombined_t       PWR_getOutageTime( void );
pwrFileData_t const *   PWR_getpwrFileData( void );
extern returnStatus_t   PWR_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
void                    PWR_resetCounters( void );
#endif
