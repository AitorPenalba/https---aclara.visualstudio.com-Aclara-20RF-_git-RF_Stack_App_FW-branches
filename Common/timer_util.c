// <editor-fold defaultstate="collapsed" desc="File Header">
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename:    timer_util.c
 *
 * Global Designator: TMR_
 *
 * Contents: Functions related to subscribable millisecond Timer
 *
 * Timer Usage:
 * 1.  Define the MAX_TIMERS used in the project
 * 2.  Initialize timer data-structure before creating tasks
 * 3.  Call TMR_AddTimer function with (timer_t *)pData to add timer.
 * 4.  The usiTimerId is set by the timer.c module and is used to reference the timer for timer management (such as
 *     delete, stop, start, reset, read timer). The calling function should store this ID for such purpose.
 * 5.  Set the ulDuration_mS (this value is in mS, not a multiple of mS). Do not set ulDuration_ticks and
 *     ulTimer_ticks, it is used for internal purpose
 * 6.  If the callback function is not NULL, this task will call the callback function once the timer expires.
 *     Note - This is a high priority task and call back functions should take minimal amount of CPU.
 *     Do not use CALLBACK functionality in new code. It will  be descoped. Use semaphores or queues.
 * 7.  If the semaphore member is not NULL, this task will give the semaphore once the timer expires
 * 8.  If the queue member is not NULL, this task will send a queue message once the timer expires
 * 9.  timer_t *pData is passed to the call back.  Set to what ever you wish
 * 10. timer_t ucCmd is passed to the call back.  Set to what ever you wish
 * 10. bOneShot:  true = Call once, false = Call over and over again (The expired flag will be set for one shot timers,
 *                once they are done)
 * 11. bOneShotDelete: Only relevant for one-shot timers. true = Delete the timer after it fires.
 *                     false = Do not delete the timer
 *
 * Note:  The following #defines needs to be defined in the configuration file (cfg_app.h)
 *        MAX_TIMERS //Maximum number of timers supported
 *
 ***********************************************************************************************************************
 * Copyright (c) 2013 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
 * or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 * Revision History:
 * 100710  MS    - Initial Release
 *
 **********************************************************************************************************************/
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include <stdbool.h>
#include <string.h>
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#endif
#include "DBG_SerialDebug.h"
#define TMR_GLOBAL
#include "timer_util.h"
#undef  TMR_GLOBAL

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Constant Definitions">
/* ****************************************************************************************************************** */
/* CONSTANTS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
#if ( RTOS_SELECTION == MQX_RTOS ) // TODO: RA6E1 - ISR control for FreeRTOS
typedef struct my_isr_struct
{
   void *         OLD_ISR_DATA;
   void           (_CODE_PTR_ OLD_ISR)(void *);
} MY_ISR_STRUCT, *MY_ISR_STRUCT_PTR;
#endif
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="File Static Variables">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

STATIC uint64_t      _TMR_Tick_Cntr;               /* RTOS Tick counter */
STATIC timer_t       _sTimer[MAX_TIMERS];          /* Manage timer requests */
STATIC timer_t       *_TMR_headNode;               /* Link-list head */
static OS_MUTEX_Obj  _tmrUtilMutex;                /* Serialize access to timer data-structure */
static OS_SEM_Obj    _tmrUtilSem;                  /* Semaphore used by timer module to count the number of ticks */
static bool        _tmrUtilSemCreated = false;

#ifdef TM_TIMER_DEBUG
nInt MaxTimersUsed = 0;
nInt TimersUsed = 0;
#endif

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Static Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

STATIC void insertTimerNode( timer_t *pTimer );   /* Inserts the timer in the active list */
STATIC returnStatus_t deleteTimerNode( uint16_t usiTimerId );  /* Deletes the timer from the active timer list */
STATIC returnStatus_t isTimerActive( uint16_t usiTimerId );    /* Checks if the timer is active */
STATIC void TMR_vApplicationTickHook( void *user_isr_ptr );

// </editor-fold>
/* ****************************************************************************************************************** */
/* Local Function Definitions */

// <editor-fold defaultstate="collapsed" desc="void insertTimerNode(timer_t *pTimer)">
/***********************************************************************************************************************
 *
 * Function name: insertTimerNode
 *
 * Purpose: This function insert timer node in the active list. The timer tick count (ulTimer_ticks) must be set
 *          before calling this function.
 *
 * Arguments: timer_t *pData: Pointer to the Timer node
 *
 * Returns: None
 *
 * Side effects: Updates the active timer link list
 *
 * Reentrant: NO. This function should be called after taking the _TMR_Mutex.
 *
 **********************************************************************************************************************/
STATIC void insertTimerNode( timer_t *pTimer )
{
   pTimer->next = NULL;  /* Initialize to null */

   if ( NULL == _TMR_headNode )
   {  /* List is empty, set as head node */
      _TMR_headNode = pTimer;
   }
   else
   {  /* Insert the node, start at the head node */
      timer_t *currentNode = _TMR_headNode; /* Points to the current node being processed in the active timer list */
      timer_t *previousNode = NULL;         /* Points to the previous node processed in the active timer list */

      while ( currentNode != NULL )
      {
         if ( pTimer->ulTimer_ticks < currentNode->ulTimer_ticks )
         {  /* Insert the node before the current node */
            if ( NULL == previousNode )
            {  /* New node will be the head node */
               _TMR_headNode = pTimer;
            }
            else
            {  /* Insert between the previous node and the current node */
               previousNode->next = pTimer;
            }
            pTimer->next = currentNode;
            /* adjust the timer value of the current node */
            currentNode->ulTimer_ticks -= pTimer->ulTimer_ticks;
            break;
         }
         else
         {  /* Move to the next node */
            pTimer->ulTimer_ticks -= currentNode->ulTimer_ticks; /* Reduce the new node's tick count */
            previousNode = currentNode;
            currentNode = currentNode->next;
            if ( NULL == currentNode )
            {  /* insert at the end of the link list */
               previousNode->next = pTimer;
               break;
            }
         }
      } /* end while */
   } /* end else */
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t deleteTimerNode(uint16_t usiTimerId)">
/***********************************************************************************************************************
 *
 * Function name: deleteTimerNode
 *
 * Purpose: Removes a timer node from the timer link list
 *
 * Arguments: uint16_t usiTimerId: Timer ID of the node to be removed from the list
 *
 * Returns: returnStatus_t eRetVal - eSUCCESS - node deleted eSUCCESSfully
 *                                  eFAILURE - Node not found i.e. timer not vaild or not active
 *
 * Side effects: Removes a timer node from the timer link list
 *
 * Reentrant: NO. This function should be called after taking the _TMR_Mutex.
 *
 **********************************************************************************************************************/
STATIC returnStatus_t deleteTimerNode( uint16_t usiTimerId )
{
   uint32_t ulRemTickCnt = 0;             /* compute the remaining ticks to expiration */
   timer_t *previousNode = NULL;          /* Points to the previous node processed in the active timer list */
   timer_t *currentNode = _TMR_headNode;  /* Points to the current node being processed in the active timer list */
   returnStatus_t eRetVal = eFAILURE;     /* Return value, default timer not valid or not active */

   /* Check the link list and remove the timer from the link list, if active */
   while ( currentNode != NULL )
   {
      ulRemTickCnt += currentNode->ulTimer_ticks;

      if ( currentNode->usiTimerId == usiTimerId )
      {  /* Remove the timer from the active timers */
         if ( NULL == previousNode )
         {  /* Remove the head node */
            _TMR_headNode = _TMR_headNode->next;
            if ( _TMR_headNode != NULL )
            {  /* more active timers, adjust the timer value of the next timer */
               _TMR_headNode->ulTimer_ticks += currentNode->ulTimer_ticks;
            }
         }
         else if ( NULL == currentNode->next )
         {  /* Remove the tail node */
            previousNode->next = NULL;
         }
         else
         {
            timer_t *nextNode = currentNode->next;  /* Remember the next node to the current node */
            /* remove the node from the middle */
            previousNode->next = nextNode;
            /* update the timer value */
            nextNode->ulTimer_ticks += currentNode->ulTimer_ticks;
         }
         /* Restore the number of ticks to expiration, used if this is called from stop timer function */
         currentNode->ulTimer_ticks = ulRemTickCnt;
         eRetVal = eSUCCESS;
         break;
      }
      previousNode = currentNode;
      currentNode = currentNode->next;
   }
   return (eRetVal);
}
// </editor-fold>
/* ****************************************************************************************************************** */
/* Global Function Definitions */

/*lint -e454 -e456 The mutex is handled properly. */

// <editor-fold defaultstate="collapsed" desc="returnStatus_t TMR_HandlerInit( void )">
/*****************************************************************************************************************
 *
 * Function name: TMR_HandlerInit
 *
 * Purpose: This function is called before starting the scheduler. Initialize data-structures and create resources
 *          needed by this module
 *
 * Arguments: None
 *
 * Returns: eSUCCESS/eFAILURE indication
 *
 * Side effects: Clear the timer structure and tick counter
 *
 * Reentrant: This function is not reentrant and should be called once before starting the scheduler
 *
 ******************************************************************************************************************/
returnStatus_t TMR_HandlerInit( void )
{
   returnStatus_t retVal = eFAILURE;

   if ( _tmrUtilSemCreated == false )
   {
      //TODO RA6: NRJ: determine if semaphores need to be counting
      if ( OS_SEM_Create(&_tmrUtilSem, 0) && OS_MUTEX_Create(&_tmrUtilMutex) )
      {
         _tmrUtilSemCreated = true;
         (void)memset(_sTimer, 0, sizeof(_sTimer)); //Initialize Timer data-structure
         _TMR_headNode = NULL; //Set the head node of the active timer list to NULL
         _TMR_Tick_Cntr = 0;
         retVal = eSUCCESS;
      }
   }
   return (retVal);
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void TMR_HandlerTask( void *pvParameters )">
/*****************************************************************************************************************
 *
 * Function name: TMR_HandlerTask
 *
 * Purpose: This function pends on _TMR_cntSem semaphore. Calls the callback function and/or
 *          gives the requested semaphore and/or sends queue message, when the requested timer has elapsed.
 *
 * Arguments: uint32_t Arg0 - Value passed as part of task creation. Not used for this task.
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: This function is reentrant and implements Timer task. This function should not be called by any
 *            module except during startup (task creation).
 *
 ******************************************************************************************************************/
void TMR_HandlerTask( uint32_t Arg0 )
{
   timer_t           *pTimer;          /* Pointer to timer data structure */
#if ( RTOS_SELECTION == MQX_RTOS ) // TODO: RA6E1 - ISR control for FreeRTOS
   MY_ISR_STRUCT_PTR isr_ptr;          /* */

   /* The following code was taken from MQX example code for adding our timer code to the RTOS tick interrupt.  So, the
    * following code will replace the RTOS tick interrupt with our own and then our ISR will call the RTOS tick. */
   isr_ptr = (MY_ISR_STRUCT_PTR)_mem_alloc_zero(sizeof(MY_ISR_STRUCT));
   isr_ptr->OLD_ISR_DATA = _int_get_isr_data(INT_SysTick);              /*lint !e641  This code is from an example */
   isr_ptr->OLD_ISR      = _int_get_isr(INT_SysTick);                   /*lint !e641  This code is from an example */
   _int_install_isr(INT_SysTick, TMR_vApplicationTickHook, isr_ptr);    /*lint !e641 !e64 !e534 code is from an example */
#elif ( RTOS_SELECTION == FREE_RTOS )
   /* FreeRTOS uses the Tick Hook method to call so no initialization needed for that case */
#endif
   /* End the example code! */

   for ( ; ; ) /* RTOS Task, keep running forever */
   {
      /* Wait for the semaphore, at each tick, RTOS ISR will give a semaphore */
      if ( true == OS_SEM_Pend(&_tmrUtilSem, OS_WAIT_FOREVER ) )
      {  /* RTOS Tick */

         OS_MUTEX_Lock(&_tmrUtilMutex); // Function will not return if it fails

         _TMR_Tick_Cntr++;  // Increment the global tick counter (ticks since powerup)

         /* Check if the active list is empty */
         if ( _TMR_headNode != NULL )
         {
            _TMR_headNode->ulTimer_ticks--;

            /* Check if timer expired */
            while ( (_TMR_headNode != NULL) && (0 == _TMR_headNode->ulTimer_ticks) )
            {  /* Time has elapsed, execute the timers */
               pTimer =  _TMR_headNode;  // remember the current timer node to be processed
               /* Set head node to point to the next node in the list */
               _TMR_headNode = _TMR_headNode->next;

               if ( pTimer->pFunctCallBack != NULL )   // Is this a call back?
               {
                  pTimer->pFunctCallBack((uint8_t)pTimer->ucCmd, pTimer->pData);
               }
               if ( pTimer->pSemHandle != NULL )   // give semaphore, if not NULL
               {
                  OS_SEM_Post(pTimer->pSemHandle);
               }
               /* Set the timer countdown */
               pTimer->ulTimer_ticks = pTimer->ulDuration_ticks;

               /* insert the node back on the active list, if it not a one shot timer */
               if ( pTimer->bOneShot )
               {  /* One shot timer, stop the timer, set the expired flag */
                  pTimer->bStopped = true;
                  pTimer->bExpired = true;
                  if ( pTimer->bOneShotDelete )
                  {  /* Delete one shot timer */
                     /* Clearing ulDuration_mS indicates timer is free */
                     //DBG_logPrintf ('I', "Self Delete Timer ID=%u", pTimer->usiTimerId);
                     pTimer->ulDuration_mS = 0;
#ifdef TM_TIMER_DEBUG
                     TimersUsed--;
#endif
                  }
                  // else, Do not delete this one-shot timer. This timer does not need to be rearmed
               }
               else
               {
                  /* Rearm, Add the timer to the active list */
                  insertTimerNode(pTimer);
               }
            } //while
         }
         OS_MUTEX_Unlock(&_tmrUtilMutex);  /* End critical section */ // Function will not return if it fails
      }
   }
} /*lint !e715   Arg0 is not used */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t isTimerActive(uint16_t timerId)">
/***********************************************************************************************************************
 *
 * Function name: isTimerActive
 *
 * Purpose: Checks if a timer is active
 *
 * Arguments: uint16_t timerId: Timer ID
 *
 * Returns: returnStatus_t eRetVal - eSUCCESS - Timer active
 *                                  eFAILURE - Timer not active
 *
 * Side effects: None
 *
 * Reentrant: NO. This function should be called after taking the _TMR_Mutex.
 *
 **********************************************************************************************************************/
STATIC returnStatus_t isTimerActive( uint16_t usiTimerId )
{
   timer_t *currentNode;       /* Points to the current node being processed in the active timer list */
   returnStatus_t eRetVal = eFAILURE;   /* Return value, default timer not active */

   /* Check the link list first and remove the timer from the link list, if active */
   for ( currentNode = _TMR_headNode; currentNode != NULL; currentNode = currentNode->next )
   {
      if ( currentNode->usiTimerId == usiTimerId )
      {  /* Timer in active timers list */
         eRetVal = eSUCCESS;
         break;
      }
   }
   return(eRetVal);
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t TMR_AddTimer(timer_t *pData)">
/***********************************************************************************************************************
 *
 * Function name: TMR_AddTimer
 *
 * Purpose: Add Timer to the timer data-structure and active timer list
 *
 * Arguments: timer_t *pData: Pointer to the parameters required for configuring timer
 *
 * Returns: returnStatus_t eRetVal - eSUCCESS - Timer added eSUCCESSfully, eFAILURE otherwise.
 *          The Timer Id is returned in timerId member of the timer_t structure
 *
 * Side effects: Updates timer data-structure and active timer list
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
returnStatus_t TMR_AddTimer( timer_t *pData )
{
   uint16_t i;
   static uint16_t lastAllocatedIndex_ = ARRAY_IDX_CNT(_sTimer) - 1; /* Index, start at last timer as last allocated */
   timer_t *pTimer;                          /* Pointer to timer data structure */
   returnStatus_t eRetVal = eFAILURE;        /* Return value, default timer not added (out of timers) */

   /* Validate the timer parameters */
   if ( 0 != pData->ulDuration_mS )
   {  /* Non zero timer duration, find empty slot and add timer */

      OS_MUTEX_Lock(&_tmrUtilMutex); // Function will not return if it fails

      for ( i = 1; i <= ARRAY_IDX_CNT(_sTimer); i++ )
      {  /* Find first empty slot */
         uint16_t nextSlotId;  //Next potential unused slot

         nextSlotId = (lastAllocatedIndex_ + i) % ARRAY_IDX_CNT(_sTimer);
         pTimer = &_sTimer[nextSlotId];
         if ( 0 == pTimer->ulDuration_mS ) /* If timer duration is 0, timer slot is available */
         {
            pData->usiTimerId = (nextSlotId + 1);    /* Use index + 1 as Timer ID, so skip using 0 as valid ID */
            //DBG_logPrintf ('I', "ADD Timer ID=%u", pData->usiTimerId);
            (void)memcpy(pTimer, pData, sizeof(timer_t)); /* Set the timer parameters */
            pTimer->bExpired = false; /* can not be expired, while adding the timer */
            /* Compute the number of ticks, round up to the next tick */
            pTimer->ulDuration_ticks = pTimer->ulDuration_mS / portTICK_RATE_MS;
            if ( (pTimer->ulDuration_mS % portTICK_RATE_MS) != 0 )
            {  /* round up to the next tick */
               pTimer->ulDuration_ticks++;
            }
            /* Set the timer countdown */
            pTimer->ulTimer_ticks = pTimer->ulDuration_ticks;
            if ( !pTimer->bStopped )
            {  /* Add the timer to the active list only if the timer is not stopped */
               insertTimerNode(pTimer);
            }
            eRetVal = eSUCCESS;
            lastAllocatedIndex_ = nextSlotId; //Save the last used slot ID
#ifdef TM_TIMER_DEBUG
            TimersUsed++;
            if ( TimersUsed > MaxTimersUsed )
            {
               MaxTimersUsed = TimersUsed;
            }
#endif
            break; /* Break the for loop */
         }
      }
      OS_MUTEX_Unlock(&_tmrUtilMutex);  /* End critical section */ // Function will not return if it fails
   }
   return(eRetVal);
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t TMR_DeleteTimer(uint16_t usiTimerId)">
/***********************************************************************************************************************
 *
 * Function name: TMR_DeleteTimer
 *
 * Purpose: This function clears the data from the timer data-structure and removes the timer from the
 *          active timer list
 *
 * Arguments: uint16_t usiTimerId - Id of the timer to delete
 *
 * Returns: returnStatus_t eRetVal - eSUCCESS - Timer is valid and deleted eSUCCESSfully, eFAILURE otherwise.
 *
 * Side effects: Updates timer data-structure and active timer list
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
returnStatus_t TMR_DeleteTimer( uint16_t usiTimerId )
{
   returnStatus_t eRetVal = eFAILURE;   /* Return value, default timer not found */
   timer_t *pTimer;                     /* Pointer to timer data structure */
   uint16_t slotId = usiTimerId - 1;    /* Get the slot Id for this timer */

   if ( slotId < ARRAY_IDX_CNT(_sTimer) )
   { /* ID within range */

      OS_MUTEX_Lock(&_tmrUtilMutex); // Function will not return if it fails

      pTimer = &_sTimer[slotId];
      /* Check if the timer is valid */
      if ( pTimer->ulDuration_mS != 0 )
      {  /* Timer is valid */
         //DBG_logPrintf ('I', "Delete Timer ID=%u", usiTimerId);
         /* Delete the timer from the active list, if present */
         (void)deleteTimerNode(usiTimerId);
         /* Clearing ulDuration_mS indicates timer is free */
         pTimer->ulDuration_mS = 0;
         eRetVal = eSUCCESS;
#ifdef TM_TIMER_DEBUG
         TimersUsed--;
#endif
      }
      OS_MUTEX_Unlock(&_tmrUtilMutex);  /* End critical section */ // Function will not return if it fails
   }
   return(eRetVal);
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t TMR_StartTimer(uint16_t usiTimerId)">
/***********************************************************************************************************************
 *
 * Function name: TMR_StartTimer
 *
 * Purpose: This function starts timer by clearing the bStopped bit and adding the timer to the active list.
 *          If the timer was already running, this will have no effect on such timers.
 *
 * Arguments: uint16_t usiTimerId - Id of the timer to start
 *
 * Returns: returnStatus_t eRetVal - eSUCCESS - Timer is valid, was stopped and now started eSUCCESSfully,
 *                                  eFAILURE otherwise.
 *
 * Side effects: Clear bStopped bit in the timer data-structure associated with given timer id
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
returnStatus_t TMR_StartTimer( uint16_t usiTimerId )
{
   returnStatus_t eRetVal = eFAILURE; /* Return value, default timer not found or was already running */
   timer_t *pTimer;                   /* Pointer to timer data structure */
   uint16_t slotId = usiTimerId - 1;  /* Get the slot Id for this timer */

   if ( slotId < ARRAY_IDX_CNT(_sTimer) )
   { /* ID within range */

      OS_MUTEX_Lock(&_tmrUtilMutex); // Function will not return if it fails

      pTimer = &_sTimer[slotId];
      /* Check if the timer is valid */
      if ( pTimer->ulDuration_mS != 0 )
      {
         if ( eFAILURE == isTimerActive(usiTimerId) )
         {  /* Timer is not in the active list, activate the timer by putting it in the active list */
            if ( !(pTimer->bExpired && pTimer->bOneShot)  )
            {  /* Timer not expired */
               insertTimerNode(pTimer);
               pTimer->bStopped = false;
               eRetVal = eSUCCESS;
            }
         }
      }
      OS_MUTEX_Unlock( &_tmrUtilMutex );  /* End critical section */ // Function will not return if it fails
   }
   return(eRetVal);
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t TMR_StopTimer(uint16_t usiTimerId)">
/***********************************************************************************************************************
 *
 * Function name: TMR_StopTimer
 *
 * Purpose: This function stops timer by setting the bStopped bit and removing it from the active list.
 *
 * Arguments: uint16_t usiTimerId - Id of the timer to stop
 *
 * Returns: returnStatus_t eRetVal - eSUCCESS - Timer is valid, was running and now stopped eSUCCESSfully,
 *                                  eFAILURE otherwise.
 *
 * Side effects: Set bStopped bit in the timer data-structure associated with given timer id
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
returnStatus_t TMR_StopTimer( uint16_t usiTimerId )
{
   returnStatus_t eRetVal = eFAILURE; /* Return value, default timer not found or was already stopped */
   timer_t *pTimer;                   /* Pointer to timer data structure */
   uint16_t slotId = usiTimerId - 1;  /* Get the slot Id for this timer */

   if ( slotId < ARRAY_IDX_CNT(_sTimer) )
   { /* ID within range */

      OS_MUTEX_Lock(&_tmrUtilMutex); // Function will not return if it fails
      pTimer = &_sTimer[slotId];
      /* Check if the timer is allocated */
      if ( pTimer->ulDuration_mS != 0 )
      {
         /* Delete the timer from the active list, if present */
         if ( eSUCCESS == deleteTimerNode(usiTimerId) )
         {  /* Timer was is in the active list, timer removed from the active list */
            pTimer->bStopped = true;
            eRetVal = eSUCCESS;
         }
      }
      OS_MUTEX_Unlock(&_tmrUtilMutex);  /* End critical section */ // Function will not return if it fails
   }
   return(eRetVal);
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t TMR_ResetTimer( uint16_t usiTimerId, uint32_t ulTimerValue)">
/***********************************************************************************************************************
 *
 * Function name: TMR_ResetTimer
 *
 * Purpose: Reset the Timer. If timeout value passed > 0, set the initial value to the passed value.
 *          Reset the timer countdown to the initial value. Add the timer to the active list.
 *
 * Arguments: uint16_t usiTimerId - Id of the timer to reset
 *            uint32_t ulTimerValue: If ulTimerValue > 0, set timer's timeout = ulTimerValue, otherwise do not change the
 *                                 the timer's timeout value.
 *
 * Returns: returnStatus_t eRetVal - eSUCCESS - Timer is valid and reset was eSUCCESSful,
 *                                  eFAILURE otherwise.
 *
 * Side effects: Clear expired bit, stopped bit and reset the timer count-down value in the timer data-structure
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
returnStatus_t TMR_ResetTimer( uint16_t usiTimerId, uint32_t ulTimerValue )
{
   returnStatus_t eRetVal = eFAILURE; /* Return value */
   timer_t *pTimer;                   /* Pointer to timer data structure */
   uint16_t slotId = usiTimerId - 1;  /* Get the slot Id for this timer */

   if ( slotId < ARRAY_IDX_CNT(_sTimer) )
   {  /* ID within range */
      OS_MUTEX_Lock(&_tmrUtilMutex); // Function will not return if it fails

      pTimer = &_sTimer[slotId];  /* Point to correct timer */
      /* Check if the timer is allocated */
      if ( pTimer->ulDuration_mS != 0 )
      {
         /* Delete the timer from the active list, if present */
         (void)deleteTimerNode(usiTimerId);

         if ( ulTimerValue > 0 )
         { /* New initial value */
            pTimer->ulDuration_mS = ulTimerValue;
            pTimer->ulDuration_ticks = pTimer->ulDuration_mS / portTICK_RATE_MS;
            if ( (pTimer->ulDuration_mS % portTICK_RATE_MS) != 0 )
            {  /* round up to the next tick */
               pTimer->ulDuration_ticks++;
            }
         }
         /* Set the timer countdown */
         pTimer->ulTimer_ticks = pTimer->ulDuration_ticks;
         /* Add the timer to the active list */
         insertTimerNode(pTimer);
         /* Timer will start after the reset command */
         pTimer->bStopped = false;
         pTimer->bExpired = false;

         eRetVal = eSUCCESS;
      }
      OS_MUTEX_Unlock(&_tmrUtilMutex);  /* End critical section */ // Function will not return if it fails
   }
   return(eRetVal);
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t TMR_ReadTimer(timer_t *pData)">
/***********************************************************************************************************************
 *
 * Function name: TMR_ReadTimer
 *
 * Purpose: Read the timer and return the read values in timer_t structure
 *
 * Arguments: timer_t *pData: Return timer values in this structure
 *
 * Returns: returnStatus_t eRetVal - eSUCCESS - Timer read eSUCCESSfully, eFAILURE otherwise.
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
returnStatus_t TMR_ReadTimer( timer_t *pData )
{
   returnStatus_t eRetVal = eFAILURE; /* Return value */
   uint16_t slotId;                   /* Get the slot Id for this timer */

   slotId = pData->usiTimerId - 1; /* Get the slot of this timer */
   if ( slotId < ARRAY_IDX_CNT(_sTimer) )
   { /* ID within range */
      OS_MUTEX_Lock(&_tmrUtilMutex); // Function will not return if it fails
      (void)memcpy(pData, &_sTimer[slotId], sizeof(timer_t));
      OS_MUTEX_Unlock(&_tmrUtilMutex);  /* End critical section */ // Function will not return if it fails

      eRetVal = eSUCCESS;
   }
   return(eRetVal);
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t TMR_GetMillisecondCntr(uint64_t *ulMSCntr)">
/***********************************************************************************************************************
 *
 * Function name: TMR_GetMillisecondCntr
 *
 * Purpose: Return the milliseconds elapsed since power-up
 *
 * Arguments: uint64_t *ulMSCntr - Return milliseconds counter value
 *
 * Returns: void
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
void TMR_GetMillisecondCntr( uint64_t *ulMSCntr )
{
   OS_MUTEX_Lock(&_tmrUtilMutex); // Function will not return if it fails
   *ulMSCntr = (_TMR_Tick_Cntr * portTICK_RATE_MS);
   OS_MUTEX_Unlock(&_tmrUtilMutex);  /* End critical section */ // Function will not return if it fails
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void TMR_vApplicationTickHook(void)">
/*****************************************************************************************************************
 *
 * Function name: TMR_vApplicationTickHook
 *
 * Purpose: This function extends RTOS tick ISR. This function runs at ISR level and should only use ISR safe
 *          RTOS API's. This function give semaphore to the timer task on each RTOS tick.
 *
 * Arguments: None
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: This function is reentrant. This function should only be called from an RTOS Tick ISR
 *
 ******************************************************************************************************************/
STATIC void TMR_vApplicationTickHook( void *user_isr_ptr )
{
#if ( RTOS_SELECTION == MQX_RTOS ) // TODO: RA6E1 - ISR control for FreeRTOS
   MY_ISR_STRUCT_PTR  isr_ptr;   /* */

   /* This code is taken from the MQX example isr.c code to use the system tick to tick our own module. */
   isr_ptr = (MY_ISR_STRUCT_PTR)user_isr_ptr;
#endif
   /* RTOS tick, signal the timer task */
   if ( _tmrUtilSemCreated == true )
   {
      OS_SEM_Post(&_tmrUtilSem);
   }

#if ( RTOS_SELECTION == MQX_RTOS ) // TODO: RA6E1 - ISR control for FreeRTOS
   (*isr_ptr->OLD_ISR)(isr_ptr->OLD_ISR_DATA);     /* Chain to the previous notifier - This will call the RTOS tick. */
#endif
}
// </editor-fold>
/*lint +e454 +e456 The mutex is handled properly. */
