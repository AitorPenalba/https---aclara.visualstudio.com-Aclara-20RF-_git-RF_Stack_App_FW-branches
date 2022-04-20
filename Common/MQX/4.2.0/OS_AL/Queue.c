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
#include <stdint.h>
#include <stdbool.h>
#include <mqx.h>
#include "OS_aclara.h"
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
             QueueLength - not used in MQX

  Returns: FuncStatus - True if Queue created successfully, False if error

  Notes: exta parameter

*******************************************************************************/
bool OS_QUEUE_Create ( OS_QUEUE_Handle QueueHandle,  uint32_t QueueLength )
{
   bool FuncStatus = true;

   _queue_init ( QueueHandle, 0 );

   return ( FuncStatus );
} /* end OS_QUEUE_Create () */

/*******************************************************************************

  Function name: OS_QUEUE_Enqueue

  Purpose: This function will add a new Queue element to the end of the existing
           Queue Handle structure

  Arguments: QueueHandle - pointer to the handle structure of the queue
             QueueElement - pointer to the element that is being added to the queue

  Returns: None

  Notes: Although MQX can return false for the _queue_enqueue function, that will
         never happen since the queue can't never be full.
         If this happens, we consider this a catastrophic failure.

         Function will not return if it fails

*******************************************************************************/
void OS_QUEUE_ENQUEUE ( OS_QUEUE_Handle QueueHandle, void *QueueElement, char *file, int line )
{
   if (!_queue_enqueue ( QueueHandle, (QUEUE_ELEMENT_STRUCT_PTR)QueueElement ) ) {
      EVL_FirmwareError( "OS_QUEUE_Enqueue" , file, line );
   }
} /* end OS_QUEUE_Enqueue () */

/*******************************************************************************

  Function name: OS_QUEUE_Dequeue

  Purpose: This function will remove the first element in the queue and return
           a pointer to this first element

  Arguments: QueueHandle - pointer to the handle structure of the queue

  Returns: QueueElement - pointer to the element that is being removed from the queue

  Notes:

*******************************************************************************/
void *OS_QUEUE_Dequeue ( OS_QUEUE_Handle QueueHandle )
{
   OS_QUEUE_Element *QueueElement;

   QueueElement = (OS_QUEUE_Element*)_queue_dequeue ( QueueHandle ); /*lint !e816 area too small   */

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

  Notes:

*******************************************************************************/
bool OS_QUEUE_Insert ( OS_QUEUE_Handle QueueHandle, void *QueuePosition, void *QueueElement )
{
   bool FuncStatus;

   FuncStatus = (bool)_queue_insert ( QueueHandle, (QUEUE_ELEMENT_STRUCT_PTR)QueuePosition, (QUEUE_ELEMENT_STRUCT_PTR)QueueElement );

   return ( FuncStatus );
} /* end OS_QUEUE_Insert () */

/*******************************************************************************

  Function name: OS_QUEUE_Remove

  Purpose: This function will remove a Queue element from a Queue structure

  Arguments: QueueHandle - pointer to the handle structure of the queue
             QueueElement - pointer to the element that is being removed from the queue

  Returns:

  Notes:

*******************************************************************************/
void OS_QUEUE_Remove ( OS_QUEUE_Handle QueueHandle, void *QueueElement )
{
   _queue_unlink ( QueueHandle, (QUEUE_ELEMENT_STRUCT_PTR)QueueElement );
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
   bool QueueEmpty;
   uint16_t NumElements;

   QueueEmpty = (bool)_queue_is_empty ( QueueHandle );

   if ( QueueEmpty == false )
   {
      NumElements = (uint16_t)_queue_get_size ( QueueHandle );
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
   OS_QUEUE_Element *QueueElement;

   QueueElement = (OS_QUEUE_Element*)_queue_head ( QueueHandle ); /*lint !e816 area too small   */

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
   OS_QUEUE_Element *NextElement;

   NextElement = (OS_QUEUE_Element*)_queue_next ( QueueHandle, (QUEUE_ELEMENT_STRUCT_PTR)QueueElement ); /*lint !e816 area too small   */

   return ( (void *)NextElement );
} /* end OS_QUEUE_Next () */

