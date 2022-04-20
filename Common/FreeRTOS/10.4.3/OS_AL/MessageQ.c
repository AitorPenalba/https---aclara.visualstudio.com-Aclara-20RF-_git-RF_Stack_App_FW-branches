/******************************************************************************
 *
 * Filename: MessageQ.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2022 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include "OS_aclara.h"
#include "DBG_SerialDebug.h"
#include "buffer.h"

/* #DEFINE DEFINITIONS */
#define DEFAULT_NUM_ITEMS_MSGQ 10
/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */


/*******************************************************************************

  Function name: OS_MSGQ_Create

  Purpose: This function is used to create a new Message Q Object

  Arguments: MsgqHandle - pointer to the MessageQ object

  Returns: RetStatus - True if MessageQ created successfully, False if error

  Notes: Calling function must pass in an allocated OS_MSGQ_Obj

*******************************************************************************/

bool OS_MSGQ_CREATE ( OS_MSGQ_Handle MsgqHandle,  uint32_t NumMessages )
{
   bool RetStatus = true;
   if ( false == OS_QUEUE_Create(&(MsgqHandle->MSGQ_QueueObj), NumMessages) )
   {
      RetStatus = false;
   } /* end if() */
   if ( false == OS_SEM_Create(&(MsgqHandle->MSGQ_SemObj), NumMessages ) )
   {
      RetStatus = false;
   } /* end if() */

   return ( RetStatus );
} /* end OS_MSGQ_Create () */
/*******************************************************************************

  Function name: OS_MSGQ_POST

  Purpose: This function is used to Post a message into a MessageQ structure

  Arguments: MsgqHandle - pointer to the MessageQ object
             MessageData - pointer to the pointer of the Message data to post (see Notes below)
             ErrorCheck - flag to check some errors or not. This is need when BM post a free buffer.

  Returns: None

  Notes: The memory for MessageData must be allocated by the calling function and
         and is retained in the original allocated memory location.
         Allocation and De-allocation must be handled by the application calling
         these OS_MSGQ_xxx functions This OS_MSGQ module does not allocate,
         nor free any memory, and just deals
         with a pointer to the memory location

         Function will not return if it fails

*******************************************************************************/
void OS_MSGQ_POST ( OS_MSGQ_Handle MsgqHandle, void *MessageData, bool ErrorCheck, char *file, int line )
{
  OS_QUEUE_Element *ptr = MessageData;

  // Sanity check
  if (ErrorCheck && ptr->flag.isFree) {
    // The buffer was freed
    DBG_LW_printf("\nERROR: OS_MSGQ_POST got a buffer marked as free. Size: %u, pool = %u, addr=0x%p\n",
                  ptr->dataLen, ptr->bufPool, MessageData);
    DBG_LW_printf("ERROR: OS_MSGQ_POST called from %s:%d\n", file, line);
  }
  if (ptr->flag.inQueue) {
    // The buffer is already in use.
    DBG_LW_printf("\nERROR: OS_MSGQ_POST got a buffer marked as in used by another queue. Size: %u, pool = %u, addr=0x%p\n",
                  ptr->dataLen, ptr->bufPool, MessageData);
    DBG_LW_printf("ERROR: OS_MSGQ_POST called from %s:%d\n", file, line);
  }

  // Mark as on queue
  ptr->flag.inQueue++;

  OS_QUEUE_ENQUEUE(&(MsgqHandle->MSGQ_QueueObj), MessageData, file, line); // Function will not return if it fails

  OS_SEM_POST(&(MsgqHandle->MSGQ_SemObj), file, line); // Function will not return if it fails


} /* end OS_MSGQ_Post () */

/*******************************************************************************

  Function name: OS_MSGQ_PEND

  Purpose: This function is used to Pend on a message from a MessageQ structure

  Arguments: MsgqHandle - pointer to the MessageQ object
             MessageData - pointer to the location of a pointer where the data resides
             TimeoutMs - Timeout in Milliseconds to wait for a message
             ErrorCheck - flag to check some errors or not. This is needed when BM pends for a buffer.

  Returns: RetStatus - True if MessageQ Pended Message successfully, False if error, or Timeout

  Notes: The memory for MessageData has been allocated by an application module outside
         of this OS_MSGQ_xxx functions.  The application is also responsible for
         de-allocating this memory after it has been consumed.
         This OS_MSGQ module does not allocate, nor free any memory, and just deals
         with a pointer to the memory location

*******************************************************************************/
bool OS_MSGQ_PEND ( OS_MSGQ_Handle MsgqHandle, void **MessageData, uint32_t TimeoutMs, bool ErrorCheck, char *file, int line )
{
   bool RetStatus = true;
   OS_QUEUE_Element_Handle ptr;

   /* Converting TimeoutMs = 0 to wait forever */
#if ( RTOS_SELECTION == FREE_RTOS )
   if ( TimeoutMs == 0 )
   {
     TimeoutMs = portMAX_DELAY;
   }
#endif

   if ( true == OS_SEM_Pend(&(MsgqHandle->MSGQ_SemObj), TimeoutMs) )
   {
      ptr = OS_QUEUE_Dequeue(&(MsgqHandle->MSGQ_QueueObj));
      *MessageData = ptr;

      // Sanity check
      if (ErrorCheck && ptr->flag.isFree) {
         // The buffer was freed
         DBG_LW_printf("\nERROR: OS_MSGQ_PEND got a buffer marked as free. Size: %u, pool = %u, addr=0x%p\n",
                       ptr->dataLen, ptr->bufPool, *MessageData);
         DBG_LW_printf("ERROR: OS_MSGQ_PEND called from %s:%d\n", file, line);
      }
      if (ptr->flag.inQueue == 0 ) {
         // The buffer marked as not in queue.
         DBG_LW_printf("\nERROR: OS_MSGQ_PEND got a buffer marked as not on queue. Size: %u, pool = %u, addr=0x%p\n",
                       ptr->dataLen, ptr->bufPool, *MessageData);
         DBG_LW_printf("ERROR: OS_MSGQ_PEND called from %s:%d\n", file, line);
      } else {
         if (ptr->flag.inQueue > 1 ) {
            // The buffer is on more than one queue.
            DBG_LW_printf("\nERROR: OS_MSGQ_PEND got a buffer marked as in used by another queue. Size: %u, pool = %u, addr=0x%p\n",
                          ptr->dataLen, ptr->bufPool, *MessageData);
            DBG_LW_printf("ERROR: OS_MSGQ_PEND called from %s:%d\n", file, line);
         }
         // Mark as dequeued
         ptr->flag.inQueue--;
      }
   } /* end if() */
   else
   {
      RetStatus = false;
   } /* end else() */

   return ( RetStatus );
} /* end OS_MSGQ_Pend () */

#if (TM_MSGQ ==1 )
static OS_MSGQ_Obj             testMsgQueue_;
bool retVal = false;
static uint8_t value = 94;
static buffer_t payload1;
static buffer_t payload2;
static buffer_t *ptr1 = &payload1;
static buffer_t *ptr2 = &payload2;
static buffer_t *rx_msg;

static volatile uint8_t counter = 0;

void OS_MSGQ_TestCreate ( void )
{
   if( OS_MSGQ_Create(&testMsgQueue_, 10) )
   {
      counter++;
   }
   else
   {
      counter++;
      APP_ERR_PRINT("Unable to Create the Message Queue!");
   }
   /*initialize static message*/
   payload1.data = &value;

}
bool OS_MSGQ_TestPend( void )
{
   bool retVal = false;

   if( OS_MSGQ_Pend(&testMsgQueue_, (void *)&rx_msg, OS_WAIT_FOREVER) )
   {
      counter--;
      retVal = true;
   }

   if( 94 ==  *(rx_msg->data))
   {
      APP_PRINT("Success MSGQ Test ");
   }
   else if( 0 != counter )
   {
      APP_PRINT("Fail MSGQ Test");
   }
   APP_PRINT("Complete MSGQ Test");
   return(retVal);
}

void OS_MSGQ_TestPost( void )
{
   //post address of the txMsg to the Queue
   OS_MSGQ_Post(&testMsgQueue_, (void *)ptr1);
}
#endif
