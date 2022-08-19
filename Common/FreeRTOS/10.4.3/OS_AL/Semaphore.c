/**********************************************************************************************************************

   Filename: Semaphore.c

   Global Designator: OS_SEM_

   Contents:

 **********************************************************************************************************************
  A product of
  Aclara Technologies LLC
  Confidential and Proprietary
  Copyright 2014-2022 Aclara. All Rights Reserved.

  PROPRIETARY NOTICE
  The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
  (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
  authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
  be used or copied only in accordance with the terms of such license.
 ********************************************************************************************************************* */

/* INCLUDE FILES */
#include "project.h"
#include "EVL_event_log.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/*******************************************************************************

  Function name: OS_SEM_Create

  Purpose: This function will create a new Semaphore

  Arguments: SemHandle - pointer to the Handle structure of the Semaphore
              maxCount - maximum count of semaphore

  Returns: FuncStatus - True if Semaphore created successfully, False if error

  Notes: maxCount is only used in FreeRTOS.
         If the maxcount is 0 then a binary semaphore is created,

*******************************************************************************/
bool OS_SEM_Create ( OS_SEM_Handle SemHandle, uint32_t maxCount )
{
   bool FuncStatus = true;

   if( 0 == maxCount )
   {
      *SemHandle = xSemaphoreCreateBinary();
   }
   else
   {
      *SemHandle = xSemaphoreCreateCounting(maxCount, 0); /* Always create with initial count of 0 */
   }
   if( NULL == SemHandle )
   {
      /* TODO: DG: Add Print */
      /* The semaphore could not be created because there was insufficient heap memory available for FreeRTOS to allocate the semaphore data structures.*/
      FuncStatus = false;
   }
   return ( FuncStatus );
} /* end OS_SEM_Create () */

/*******************************************************************************

  Function name: OS_SEM_Post

  Purpose: This function will post to the passed in Semaphore

  Arguments: SemHandle - pointer to the Handle structure of the Semaphore

  Returns: None

  Notes: Although MQX can return false for the _lwsem_post function, it should
         never happen since the semaphore should always be valid.
         If this happens, we consider this a catastrophic failure.

         Function will not return if it fails

*******************************************************************************/
void OS_SEM_POST ( OS_SEM_Handle SemHandle, char *file, int line )
{
   if( pdFAIL == xSemaphoreGive(*SemHandle) )
   {
      EVL_FirmwareError( "OS_SEM_Post" , file, line );
   }
} /* end OS_SEM_Post () */

/*******************************************************************************

  Function name: OS_SEM_POST_RetStatus

  Purpose: This function will post to the passed in Semaphore

  Arguments: SemHandle - pointer to the Handle structure of the Semaphore

  Returns: If the Semaphore has been posted, then eSUCCESS otherwise eFAILURE

  Notes: FreeRTOS has a limit to the the number of posts to a Semaphore whereas
         MQX just used heap memory until it is exhaused.  Therefore, this version
         of the function is needed for FreeRTOS but not for MQX.

*******************************************************************************/
returnStatus_t OS_SEM_POST_RetStatus ( OS_SEM_Handle SemHandle, char *file, int line )
{
   returnStatus_t eRetVal = eSUCCESS;
   if( pdFAIL == xSemaphoreGive(*SemHandle) )
   {
      eRetVal = eFAILURE;
   }
   return ( eRetVal );
} /* end OS_SEM_POST_RetStatus () */

/*******************************************************************************

  Function name: OS_SEM_Pend

  Purpose: This function will wait for the passed in Semaphore until it has been
           posted by another process (Semaphore count must be greater than 0)

  Arguments: SemHandle - pointer to the Handle structure of the Semaphore
             TimeoutMs - time in Milliseconds that this function should wait for
                         the semaphore before timing out and returning to the calling process.

  Returns: FuncStatus - True if Semaphore pended successfully, False if error or if timed out.

  Notes: When TimeoutMs is =0, this function will return immediately.
         When TimeoutMs is =OS_WAIT_FOREVER, this function will wait (forever) until
         the semaphore is posted by another process
         The exact number of Milliseconds passed in may not be the exact number
         of Milliseconds that this task will wait for - it is because there may
         not be a direct correlation between milliseconds and how long a tick is
         This will be rounded up to the next largest tick time.

*******************************************************************************/
bool OS_SEM_PEND ( OS_SEM_Handle SemHandle, uint32_t Timeout_msec, char *file, int line )
{
   BaseType_t RetStatus;
   bool FuncStatus = true;

   TickType_t timeout_ticks;  // Timeout in ticks

   if ( Timeout_msec > 0 )
   {
      if ( Timeout_msec == OS_WAIT_FOREVER )
      {
         timeout_ticks = portMAX_DELAY;
      }
      else
      {
         /* Convert the passed in Timeout from Milliseconds into Ticks */
         /* This is limited to a maximum of 96 hours, to prevent an overflow!
         * This is OK for 200 ticks per second.
         */
         if(Timeout_msec > (ONE_MIN * 60 * 24 * 4))
         {
            Timeout_msec = (ONE_MIN * 60 * 24 * 4);
         }

         timeout_ticks = pdMS_TO_TICKS(Timeout_msec);
         if( (uint32_t) ( (uint64_t) ((uint64_t) (Timeout_msec) * (uint64_t) configTICK_RATE_HZ ) % 1000 ) )
         {   /* Round the value up to ensure the time is >= the time requested */
            timeout_ticks = timeout_ticks + 1;
         }
      }
      RetStatus = xSemaphoreTake( *SemHandle, timeout_ticks );
      if ( pdFAIL == RetStatus )
      {
         // NOTE: In FreeRTOS, function returns pdFAIL if timeout_ticks expired without the semaphore becoming available.
         FuncStatus = false;
      }
   }
   else
   {
      RetStatus = xSemaphoreTake ( *SemHandle, ( TickType_t )0 );
      if ( RetStatus == pdFAIL )
      {
         FuncStatus = false;
      }
   }

   return ( FuncStatus );
} /* end OS_SEM_Pend () */

#if (RTOS_SELECTION == FREE_RTOS)
/*******************************************************************************

  Function name: OS_SEM_POST_fromISR

  Purpose: This function will post to the passed in Semaphore

  Arguments: SemHandle - pointer to the Handle structure of the Semaphore

  Returns: None

  Notes:

*******************************************************************************/
static uint32_t OS_SEM_PostFromIsrFailures = 0;
void OS_SEM_POST_fromISR ( OS_SEM_Handle SemHandle, char *file, int line )
{
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   if( pdFAIL == xSemaphoreGiveFromISR( *SemHandle, &xHigherPriorityTaskWoken ) )
   {
      OS_SEM_PostFromIsrFailures++;
      /* TODO: */
      //      APP_ERR_PRINT("OS_SEM_POST!");
      //      EVL_FirmwareError( "OS_SEM_Post" , file, line );
   }

   /* If xHigherPriorityTaskWoken was set to true you we should yield.  The actual macro used here is port specific. */
   portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
} /* end OS_SEM_POST_fromISR () */

/*******************************************************************************

  Function name: OS_SEM_PEND_fromISR

  Purpose: This function will wait for the passed in Semaphore in ISR context

  Arguments: SemHandle - pointer to the Handle structure of the Semaphore

  Returns: FuncStatus - True if Semaphore pended successfully, False if error or if timed out.

  Notes:

*******************************************************************************/
void OS_SEM_PEND_fromISR ( OS_SEM_Handle SemHandle, char *file, int line )
{
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   if( pdFAIL == xSemaphoreTakeFromISR( *SemHandle, &xHigherPriorityTaskWoken ) )
   {
      /* TODO: */
      //      APP_ERR_PRINT("OS_SEM_PEND!");
      //      EVL_FirmwareError( "OS_SEM_Pend" , file, line );
   }
} /* end OS_SEM_PEND_fromISR () */

#endif

/*******************************************************************************

  Function name: OS_SEM_Reset

  Purpose: Reset the semaphore count back to 0

  Arguments: SemHandle - pointer to the Handle structure of the Semaphore

  Returns:

  Notes: This basically performs a Pend operation until all of them have been
         consumed

*******************************************************************************/
void OS_SEM_Reset ( OS_SEM_Handle SemHandle )
{
   BaseType_t RetStatus;

   do
   {
      RetStatus = xSemaphoreTake ( *SemHandle, ( TickType_t )0 );
   } while ( RetStatus != pdFAIL );
} /* end OS_SEM_Reset () */


#if (TM_SEMAPHORE == 1)
static OS_SEM_Obj       testSem_ = NULL;
static volatile uint8_t counter = 0;
void OS_SEM_TestCreate ( void )
{
   if( OS_SEM_Create(&testSem_, 0) )
   {
      counter++;
   }
   else
   {
      counter++;
      APP_ERR_PRINT("Unable to Create the Mutex!");
   }

}
bool OS_SEM_TestPend( void )
{
   bool retVal = false;
   if( OS_SEM_Pend(&testSem_, OS_WAIT_FOREVER) )
   {
      counter--;
      retVal = true;
   }

   if( 0 == counter )
   {
      APP_PRINT("Success");
   }
   else
   {
      APP_ERR_PRINT("Fail");
   }
   return(retVal);
}

void OS_SEM_TestPost( void )
{
   OS_SEM_Post(&testSem_);
}

#endif