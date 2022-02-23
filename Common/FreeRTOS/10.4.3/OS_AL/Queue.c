/******************************************************************************
 *
 * Filename: Queue.c
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
#include "OS_aclara.h"  /* TODO: RA6: DG: We might not need this as its already included in project.h */
//#include "EVL_event_log.h"

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
bool OS_QUEUE_Create ( OS_QUEUE_Handle QueueHandle, uint32_t QueueLength )
{
   bool FuncStatus = true;
   uint32_t numItems = (QueueLength == 0) ? DEFAULT_NUM_QUEUE_ITEMS : QueueLength;
   //add new parameter number items
   *QueueHandle = xQueueCreate(numItems,  QUEUE_ITEM_SIZE);
   if( NULL == QueueHandle )
   {
      FuncStatus = false;
   }

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

   if (pdPASS != xQueueSend ( *QueueHandle, (void *)QueueElement, 0 ) )
   {
      //EVL_FirmwareError( "OS_QUEUE_Enqueue" , file, line );
     APP_PRINT("Could not add item to queue");
   }
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

   if( pdPASS != xQueueReceive ( *QueueHandle, (void *) &QueueElement, 0))
    {
      QueueElement = NULL;
    }/*lint !e816 area too small   */

   return (void *)QueueElement;
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
   //NJ: TODO, LINKED Queue for MAC. Currently only places item at end of queue
   if (pdPASS != xQueueSend ( *QueueHandle, (void *)QueueElement, 0 ) )
   {
     funcStatus = false;

     APP_PRINT("Could not add insert to queue");
   }
   return funcStatus;
} /* end OS_QUEUE_Insert () */
#if 0
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
   //NJ: TODO, LINKED Queue for MAC
   _queue_unlink ( QueueHandle, (QUEUE_ELEMENT_STRUCT_PTR)QueueElement );
} /* end OS_QUEUE_Remove () */
#endif
/*******************************************************************************

  Function name: OS_QUEUE_NumElements

  Purpose: This function will return a status of whether the Queue is empty or not

  Arguments: QueueHandle - pointer to the handle structure of the queue

  Returns: NumElements - 0 if the queue is empty, otherwise it is the number of elements in the queue

  Notes:

*******************************************************************************/
uint16_t OS_QUEUE_NumElements ( OS_QUEUE_Handle QueueHandle )
{
   UBaseType_t QueueEmpty;
   uint16_t NumElements;

   QueueEmpty = xQueueIsQueueEmptyFromISR( *QueueHandle );

   if ( QueueEmpty != pdFALSE )
   {
      NumElements = (uint16_t) uxQueueMessagesWaitingFromISR( *QueueHandle );
   } /* end if() */
   else
   {
      NumElements = 0;
   } /* end else() */

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
   UBaseType_t result;
   result = xQueuePeekFromISR(*QueueHandle, (void*)&QueueElement); //zQueuePeek or zQueuePeekISR?
   if( errQUEUE_EMPTY == result )
   {
     QueueElement = NULL;
   }
   return ( (void *)QueueElement );
} /* end OS_QUEUE_Head () */
#if 0
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
   //NJ: TODO, LINKED Queue for MAC
   OS_QUEUE_Element_Handle QueueElement;


   return ( (void *)QueueElement );
} /* end OS_QUEUE_Next () */
#endif
#if (TM_QUEUE == 1)
void OS_QUEUE_Test( void )
{
   bool retVal = false;
   static OS_QUEUE_Element txMsg1;
   static OS_QUEUE_Element txMsg2;
   static OS_QUEUE_Obj msgQueueObj;
   OS_QUEUE_Handle msgQueueHandle = &msgQueueObj;

   txMsg1.flag.isFree = false;
   txMsg1.dataLen = 11;
   txMsg2.flag.isFree = false;
   txMsg2.dataLen = 22;

   OS_QUEUE_Element_Handle elementTx = (void *)&txMsg1; //message is at static address
   OS_QUEUE_Element_Handle rxMsg;
   retVal = OS_QUEUE_Create( (void *) msgQueueHandle, 0);
   if(true == retVal)
   {
     //queue the pointer to the element
     for(int i = 0; i<DEFAULT_NUM_QUEUE_ITEMS; i++)
     {
       //enqueue elements one greater than the total length
       OS_QUEUE_Enqueue( msgQueueHandle, &elementTx);
     }
     OS_QUEUE_Enqueue( msgQueueHandle, &elementTx); // this should fail
     rxMsg = OS_QUEUE_Dequeue( msgQueueHandle );
     elementTx = (void *)&txMsg2;
     OS_QUEUE_Enqueue( msgQueueHandle, &elementTx); // this should succeed

     //dequeue all elements and print there msg length as ID
     for(int i = 0; i<DEFAULT_NUM_QUEUE_ITEMS; i++)
     {
       rxMsg = OS_QUEUE_Dequeue( msgQueueHandle );
       if(rxMsg->dataLen == 11)
       {
         APP_PRINT("Message 1");
       }
       else if (rxMsg->dataLen == 22)
       {
         APP_PRINT("Message 2");
       }
       else
       {
         APP_PRINT("Unknown Message");
       }
     }


   }



    return;

}
#endif