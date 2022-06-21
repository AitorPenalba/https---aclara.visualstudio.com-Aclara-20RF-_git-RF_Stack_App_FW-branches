/******************************************************************************
 *
 * Filename: Queue.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2012-2022 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
//#include "OS_aclara.h"  /* TODO: RA6: DG: We might not need this as its already included in project.h */
#include "buffer.h"
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

  Function name: OS_QUEUE_Create

  Purpose: This function is used to create a new Queue structure

  Arguments: QueueHandle - pointer to the handle structure of the queue
             QueueLength    - number of items for the queue to hold

  Returns: FuncStatus - True if Queue created successfully, False if error

  Notes:
   xQueueCreate parameters,
        uxQueueLength
           The maximum number of items that the queue being created can hold at any one time.
        uxItemSize
           The size, in bytes, of each data item that can be stored in the queue.

*******************************************************************************/
#if ( ( BM_USE_KERNEL_AWARE_DEBUGGING == 1 ) && ( RTOS_SELECTION == FREE_RTOS ) )
bool OS_QUEUE_Create ( OS_QUEUE_Handle QueueHandle, uint32_t QueueLength, char *name)
#else
bool OS_QUEUE_Create ( OS_QUEUE_Handle QueueHandle, uint32_t QueueLength )
#endif
{
   bool FuncStatus = true;

#if( RTOS_SELECTION == FREE_RTOS )
   uint32_t numItems = (QueueLength == 0) ? DEFAULT_NUM_QUEUE_ITEMS : QueueLength;
   *QueueHandle = xQueueCreate(numItems,  QUEUE_ITEM_SIZE);
   if( NULL == QueueHandle )
   {
      FuncStatus = false;
   }
#if ( ( BM_USE_KERNEL_AWARE_DEBUGGING == 1 ) && ( RTOS_SELECTION == FREE_RTOS ) )
   else
   {
      vQueueAddToRegistry(*QueueHandle, name);
   }
#endif // BOB_DEBUG_BUFMGR
#elif( RTOS_SELECTION == MQX_RTOS )
    //queuelength is not neceary in MQX
    _queue_init ( QueueHandle, 0 );
#endif

   return ( FuncStatus );
} /* end OS_QUEUE_Create () */
/*******************************************************************************

  Function name: OS_QUEUE_Enqueue

  Purpose: This function will add a new Queue element to the end of the existing
           Queue Handle structure

  Arguments: QueueHandle - pointer to the handle structure of the queue
             QueueElement - pointer to the element that is being added to the queue

  Returns: None

  Notes: for xQueueSend return values
      pdPASS
         Returned if data was successfully sent to the queue.
         If a block time was specified (xTicksToWait was not zero),
         then it is possible that the calling task was placed into
         the Blocked state, to wait for space to become available
         in the queue before the function returned, but data was
         successfully written to the queue before the block time expired.
      errQUEUE_FULL
         Returned if data could not be written to the queue because the queue
         was already full. If a block time was specified (xTicksToWait was not zero)
         then the calling task will have been placed into the Blocked state
         to wait for another task or interrupt to make room in the queue,
         but the specified block time expired before that happened.

*******************************************************************************/
void OS_QUEUE_ENQUEUE ( OS_QUEUE_Handle QueueHandle, void *QueueElement, char *file, int line )
{

#if( RTOS_SELECTION == FREE_RTOS )
   OS_QUEUE_Element_Handle ptr = ( OS_QUEUE_Element_Handle )QueueElement;
   if (pdPASS != xQueueSend ( *QueueHandle, (void *)&ptr, 0 ) )
   {
      DBG_printf("Could not add item to queue at %s line %d", file, line);
      EVL_FirmwareError( "OS_QUEUE_Enqueue" , file, line );
   }

#elif( RTOS_SELECTION == MQX_RTOS )
   if (!_queue_enqueue ( QueueHandle, (QUEUE_ELEMENT_STRUCT_PTR)QueueElement ) ) {
      EVL_FirmwareError( "OS_QUEUE_Enqueue" , file, line );
   }
#endif

} /* end OS_QUEUE_Enqueue () */

/*******************************************************************************

  Function name: OS_QUEUE_Dequeue

  Purpose: This function will remove the first element in the queue and return
           a pointer to this first element

  Arguments: QueueHandle - pointer to the handle structure of the queue

  Returns: QueueElement - pointer to the element that is being removed from the queue

  Notes: xQueueReceive return value
         pdPASS
            Returned if data was successfully read from the queue.
         errQUEUE_EMPTY
            Returned if data cannot be read from the queue
            because the queue is already empty.

*******************************************************************************/
void *OS_QUEUE_Dequeue ( OS_QUEUE_Handle QueueHandle )
{
  OS_QUEUE_Element_Handle QueueElement;
#if( RTOS_SELECTION == FREE_RTOS )
  if( pdPASS != xQueueReceive ( *QueueHandle, (void *) &QueueElement, 0))
  {
    QueueElement = NULL;
  }/*lint !e816 area too small   */
#elif( RTOS_SELECTION == MQX_RTOS )
  QueueElement = (OS_QUEUE_Element*)_queue_dequeue ( QueueHandle ); /*lint !e816 area too small   */
#endif
  return ( (void *)QueueElement );
} /* end OS_QUEUE_Dequeue () */

/*******************************************************************************

  Function name: OS_QUEUE_Insert

  Purpose: This function will add a new Queue element into an existing
           Queue Handle structure just after the element QueuePosition

  Arguments: QueueHandle - pointer to the handle structure of the queue
             QueuePosition - pointer to the element in the queue (so the new element will be added just after this one)
             QueueElement - pointer to the element that is being added to the queue

  Returns: FuncStatus - True if Queue element added successfully, False if error

  Notes: NJ: TODO, in Linked Queue
         xQueueReceive return value
         pdPASS
            Returned if data was successfully read from the queue.
         errQUEUE_EMPTY
            Returned if data cannot be read from the queue
            because the queue is already empty.

*******************************************************************************/
bool OS_QUEUE_Insert ( OS_QUEUE_Handle QueueHandle, void *QueuePosition, void *QueueElement )
{
   bool funcStatus = true;
#if( RTOS_SELECTION == FREE_RTOS )
   //NJ: TODO, LINKED Queue for MAC. Currently only places item at end of queue
   if (pdPASS != xQueueSend ( *QueueHandle, (void *)QueueElement, 0 ) )
   {
     funcStatus = false;

     APP_PRINT("Could not add insert to queue");
   }
#elif( RTOS_SELECTION == MQX_RTOS )
   funcStatus = (bool)_queue_insert ( QueueHandle, (QUEUE_ELEMENT_STRUCT_PTR)QueuePosition, (QUEUE_ELEMENT_STRUCT_PTR)QueueElement );
#endif
   return funcStatus;
} /* end OS_QUEUE_Insert () */

/*******************************************************************************

  Function name: OS_QUEUE_Remove

  Purpose: This function will remove a Queue element from a Queue structure

  Arguments: QueueHandle - pointer to the handle structure of the queue
             QueueElement - pointer to the element that is being removed from the queue

  Returns:

  Notes: NJ: TODO, in Linked Queue

*******************************************************************************/
void OS_QUEUE_Remove ( OS_QUEUE_Handle QueueHandle, void *QueueElement )
{
#if( RTOS_SELECTION == FREE_RTOS )
  //NRJ: TODO may not be possible with FreeRTOS
  return;
#elif( RTOS_SELECTION == MQX_RTOS )
  _queue_unlink ( QueueHandle, (QUEUE_ELEMENT_STRUCT_PTR)QueueElement );
#endif
} /* end OS_QUEUE_Remove () */
/*******************************************************************************

  Function name: OS_QUEUE_NumElements

  Purpose: This function will return a status of whether the Queue is empty or not

  Arguments: QueueHandle - pointer to the handle structure of the queue

  Returns: NumElements - 0 if the queue is empty, otherwise it is the number of elements in the queue

  Notes:

*******************************************************************************/
uint16_t OS_QUEUE_NumElements ( OS_QUEUE_Handle QueueHandle )
{
  uint16_t NumElements;
#if( RTOS_SELECTION == FREE_RTOS )

  NumElements = (uint16_t) uxQueueMessagesWaiting( *QueueHandle );
#elif( RTOS_SELECTION == MQX_RTOS )
   bool QueueEmpty;
   QueueEmpty = (bool)_queue_is_empty ( QueueHandle );

   if ( QueueEmpty == false )
   {
      NumElements = (uint16_t)_queue_get_size ( QueueHandle );
   } /* end if() */
   else
   {
      NumElements = 0;
   } /* end else() */
#endif
   return ( NumElements );
} /* end OS_QUEUE_NumElements () */
/*******************************************************************************

  Function name: OS_QUEUE_Head

  Purpose: This function will return the queue element that is at the head of the queue

  Arguments: QueueHandle - pointer to the handle structure of the queue

  Returns: QueueElement - pointer to the element that is at the beginning of the queue

  Notes:

*******************************************************************************/
void *OS_QUEUE_Head ( OS_QUEUE_Handle QueueHandle )
{
   OS_QUEUE_Element_Handle *QueueElement;
#if( RTOS_SELECTION == FREE_RTOS )
   UBaseType_t result;
   result = xQueuePeekFromISR(*QueueHandle, (void*)&QueueElement); //zQueuePeek or zQueuePeekISR?
   if( errQUEUE_EMPTY == result )
   {
     QueueElement = NULL;
   }
#elif( RTOS_SELECTION == MQX_RTOS )
   QueueElement = (OS_QUEUE_Element*)_queue_head ( QueueHandle ); /*lint !e816 area too small   */
#endif
   return ( (void *)QueueElement );
} /* end OS_QUEUE_Head () */

/*******************************************************************************

  Function name: OS_QUEUE_Next

  Purpose: This function will return the queue element that is next in the queue

  Arguments: QueueHandle - pointer to the handle structure of the queue
             QueueElement - pointer to the current element

  Returns: NextElement - pointer to the next element in the queue

  Notes:

*******************************************************************************/
void *OS_QUEUE_Next ( OS_QUEUE_Handle QueueHandle, void *QueueElement )
{
   OS_QUEUE_Element_Handle NextElement;
#if( RTOS_SELECTION == FREE_RTOS )
   //NRJ: TODO, This cannot be done easily in FreeRTOS
   NextElement = NULL;
#elif( RTOS_SELECTION == MQX_RTOS )
   NextElement = (OS_QUEUE_Element*)_queue_next ( QueueHandle, (QUEUE_ELEMENT_STRUCT_PTR)QueueElement ); /*lint !e816 area too small   */
#endif
   return ( (void *)NextElement );

} /* end OS_QUEUE_Next () */

#if( TM_QUEUE == 1)
#define NUM_ITEMS 10
bool retVal = false;
static buffer_t payload1;
static buffer_t payload2;
static buffer_t *ptr1 = &payload1;
static buffer_t *ptr2 = &payload2;
static OS_QUEUE_Obj msgQueueObj;
static OS_QUEUE_Handle msgQueueHandle = &msgQueueObj;
static buffer_t *rxMsg;

void OS_QUEUE_Test( void )
{

   uint32_t length;
   payload1.bufMaxSize = 250;
   payload2.bufMaxSize = 100;

   retVal = OS_QUEUE_Create( (void *) msgQueueHandle, NUM_ITEMS);
   if(true == retVal)
   {
       OS_QUEUE_Enqueue( msgQueueHandle, (void *)ptr1);
       rxMsg = (buffer_t * )OS_QUEUE_Dequeue(msgQueueHandle);
       if( rxMsg->bufMaxSize == 250 )
       {
         APP_PRINT(" Success");
       }
       else
       {
         APP_PRINT(" Fail");
       }


     //queue the pointer to the element
//     for(int i = 0; i< NUM_ITEMS; i++)
//     {
//       //enqueue elements one greater than the total length
//       OS_QUEUE_Enqueue( msgQueueHandle, (void *)txMsg);
//     }
//     OS_QUEUE_Enqueue( msgQueueHandle, (void *)txMsg); //this should fail
//     OS_QUEUE_Dequeue( msgQueueHandle );
//     txMsg = &payload2;
//     OS_QUEUE_Enqueue( msgQueueHandle, (void *) txMsg); // this should succeed
//     length = OS_QUEUE_NumElements(msgQueueHandle);
//     //dequeue all elements and print there msg length as ID
//     for(int i = 0; i<length; i++)
//     {
//       rxMsg = (OS_QUEUE_Element*) OS_QUEUE_Dequeue( msgQueueHandle );
//       if(rxMsg->dataLen == 200)
//       {
//         APP_PRINT("Message 1");
//       }
//       else if (rxMsg->dataLen == 100)
//       {
//         APP_PRINT("Message 2");
//       }
//       else
//       {
//         APP_PRINT("Unknown Message");
//       }
//     }


   }



    return;

}
#endif
