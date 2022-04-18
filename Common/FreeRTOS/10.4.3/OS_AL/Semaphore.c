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
//#include <mqx.h>
//#include "EVL_event_log.h"

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
         If the maxcount is 1 then a binary sempahore is created

*******************************************************************************/
bool OS_SEM_Create ( OS_SEM_Handle SemHandle, uint32_t maxCount )
{
   bool FuncStatus = true;
#if 0
   uint32_t RetStatus;

   RetStatus = _lwsem_create ( SemHandle, 0 ); /* Always create with initial count of 0 */
   if ( RetStatus != MQX_OK )
   {
      FuncStatus = false;
   } /* end if() */

#endif
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
         never happen since the semepaphore should always be valid.
         If this happens, we consider this a catastrophic failure.

         Function will not return if it fails

*******************************************************************************/
void OS_SEM_POST ( OS_SEM_Handle SemHandle, char *file, int line )
{
#if 0
  if ( _lwsem_post ( SemHandle ) )
#else
   if( pdFAIL == xSemaphoreGive(*SemHandle) )
#endif
   {
      /* TODO: */
//      APP_ERR_PRINT("OS_SEM_POST!");
//      EVL_FirmwareError( "OS_SEM_Post" , file, line );
   }
} /* end OS_SEM_Post () */

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
   //uint32_t RetStatus;
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
#if 0
         /* Convert the Timeout from Milliseconds into Ticks */
         timeout_ticks = (uint32_t)  ((uint64_t) ((uint64_t) Timeout_msec * (uint64_t) _time_get_ticks_per_sec()) / 1000);
         if( (uint32_t) ((uint64_t) ((uint64_t) Timeout_msec * (uint64_t) _time_get_ticks_per_sec()) % 1000) > 0)
         {   /* Round the value up to ensure the time is >= the time requested */
            timeout_ticks = timeout_ticks + 1;
         } /* end if() */
#else
         timeout_ticks = pdMS_TO_TICKS(Timeout_msec);
         if( ( ( TickType_t ) ( ( ( TickType_t ) ( Timeout_msec ) * ( TickType_t ) configTICK_RATE_HZ ) % ( TickType_t ) 1000U ) ) )
         {   /* Round the value up to ensure the time is >= the time requested */
            timeout_ticks = timeout_ticks + 1;
         }
#endif
      }

      RetStatus = xSemaphoreTake( *SemHandle, timeout_ticks );

      if ( pdFAIL == RetStatus )
      {
         FuncStatus = false;
         /* TODO: Add Print */
//         APP_ERR_PRINT("OS_SEM_PEND!");
//         if ( RetStatus == MQX_INVALID_LWSEM ) {
//         EVL_FirmwareError( "OS_SEM_Pend" , file, line );
//      }
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
void OS_SEM_POST_fromISR ( OS_SEM_Handle SemHandle, char *file, int line )
{
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   if( pdFAIL == xSemaphoreGiveFromISR( *SemHandle, &xHigherPriorityTaskWoken ) )
   {
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

  Notes: This basically performs a Pend opperation until all of them have been
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
   if( OS_SEM_Create(&testSem_) )
   {
      counter++;
   }
   else
   {
      counter++;
//      APP_ERR_PRINT("Unable to Create the Mutex!");
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
//      APP_PRINT("Success");
   }
   else
   {
//      APP_ERR_PRINT("Fail");
   }
   return(retVal);
}

void OS_SEM_TestPost( void )
{
   OS_SEM_Post(&testSem_);
}

#endif