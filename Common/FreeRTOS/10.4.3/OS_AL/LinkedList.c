/******************************************************************************

   Filename: Linked_list.c

   Global Designator:

   Contents:

 ******************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"


/* CONSTANTS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/******************************************************************************
 *
 * Function name: verifyListElement
 *
 * Purpose: verify if an element exists in a List Object
 *
 * Arguments: List - list Object handle to search
              listElement - pointer to list element you would like to check
 *
 * Returns: true: element exists in list; false: element does not exist in list
 *
 * Notes: internal use only for linked list object
 *
 *****************************************************************************/
static bool verifyListElement(OS_List_Handle list, OS_Linked_List_Element_Ptr listElement)
{
   bool retVal = (bool)false;
   OS_Linked_List_Element_Ptr pElement;

   if( list == NULL || listElement == NULL )
   {
      return retVal;
   }

   pElement = list->Head;
   if ( 0 == list->size )
      return ((bool)true);

   for(uint32_t i = 0; (i < list->size) ; i++ )
   {
      if( pElement == listElement )
      {
         retVal = (bool)true;
         break;
      }
      pElement = pElement->NEXT;
   }

   return retVal;
}

/******************************************************************************
 *
 * Function name: OS_QUEUE_Create
 *
 * Purpose: Initializes a linkedList Object
 *
 * Arguments: listHandle, preallocated list handle
 *
 * Returns: true: list object correctly initialized; false: invalid list object
 *
 *****************************************************************************/
bool OS_LINKEDLIST_Create (OS_List_Handle listHandle )
{
#if( RTOS_SELECTION == MQX_RTOS )
   return OS_QUEUE_Create ( listHandle, 0 );
#elif( RTOS_SELECTION == FREE_RTOS )

   if (listHandle == NULL)
   {
      return ((bool)false);
   }
   listHandle->Head = (OS_Linked_List_Element_Ptr)(void *)&(listHandle->Head);
   listHandle->Tail = (OS_Linked_List_Element_Ptr)(void *)&(listHandle->Head);
   listHandle->size = 0;

   return ((bool)true);
#endif
}


/******************************************************************************
 *
 * Function name: OS_LINKEDLIST_Enqueue
 *
 * Purpose: Put the given element at the end of the given list
 *
 * Arguments: Pointer to a list object, pointer to the list element
 *
 * Returns: None
 *
 * Notes: Not thread-safe!
 *
 *****************************************************************************/
void OS_LINKEDLIST_Enqueue( OS_List_Handle list, void *listElement)
{
#if( RTOS_SELECTION == MQX_RTOS )
   OS_QUEUE_Enqueue(list, listElement);
#elif( RTOS_SELECTION == FREE_RTOS )
   if ( (NULL == list) ||  (NULL == listElement))
   {
      return;
   }

   OS_Linked_List_Element_Ptr pTail                = list->Tail;
   ((OS_Linked_List_Element_Ptr)listElement)->NEXT = (OS_Linked_List_Element_Ptr)list;
   ((OS_Linked_List_Element_Ptr)listElement)->PREV = pTail;
   pTail->NEXT    = ((OS_Linked_List_Element_Ptr)listElement);
   list->Tail     = ((OS_Linked_List_Element_Ptr)listElement);
   list->size++;
#endif
}


/******************************************************************************
 *
 * Function name: OS_LINKEDLIST_Insert
 *
 * Purpose: Add an element in the middle of the list, in back of an
 *          existing element.
 *
 * Arguments: list, Pointer to an element already in the list, pointer to new element.
 *
 * Returns: bool
 *
 *****************************************************************************/
bool OS_LINKEDLIST_Insert (OS_List_Handle list,  void *listPosition, void *listElement )
{
#if( RTOS_SELECTION == MQX_RTOS )
   return OS_QUEUE_Insert(list, listPosition, listElement);
#elif( RTOS_SELECTION == FREE_RTOS )
   bool retVal = (bool)false;
   bool found  = (bool)false;

   found = verifyListElement(list, ( OS_Linked_List_Element_Ptr ) listPosition);
   if( found )
   {
      OS_Linked_List_Element_Ptr nxt = ((OS_Linked_List_Element_Ptr) listPosition)->NEXT;
      ((OS_Linked_List_Element_Ptr) listElement)->NEXT  = nxt;
      ((OS_Linked_List_Element_Ptr) listPosition)->NEXT = (OS_Linked_List_Element_Ptr) listElement;
      ((OS_Linked_List_Element_Ptr) listElement)->PREV  = (OS_Linked_List_Element_Ptr) listPosition;
      nxt->PREV                                         = (OS_Linked_List_Element_Ptr) listElement;

      list->size++;
      retVal = (bool)true;
   }
   return retVal;
#endif
}

/******************************************************************************
 *
 * Function name: OS_QUEUE_Remove
 *
 * Purpose: Removes the given "elem" from the list if it exists in that list
 *
 * Arguments: list object pointer, Pointer to element
 *
 * Returns: None
 *
 *****************************************************************************/
void OS_LINKEDLIST_Remove ( OS_List_Handle list,void *listElement )
{
#if( RTOS_SELECTION == MQX_RTOS )
   OS_QUEUE_Remove(list, Elem);
#elif( RTOS_SELECTION == FREE_RTOS )
   if( NULL == listElement)
   {
      return;
   }
   bool found = (bool)false;

   found = verifyListElement(list, (OS_Linked_List_Element_Ptr) listElement);
   if( found )
   {
      ((OS_Linked_List_Element *) listElement)->PREV->NEXT = ((OS_Linked_List_Element *) listElement)->NEXT;
      ((OS_Linked_List_Element *) listElement)->NEXT->PREV = ((OS_Linked_List_Element *) listElement)->PREV;
      list->size--;
      /*clear out the pointers for the removed element*/
      ((OS_Linked_List_Element *) listElement)->PREV = NULL;
      ((OS_Linked_List_Element *) listElement)->NEXT = NULL;
   }
#endif
}


/******************************************************************************
 *
 * Function name: OS_LINKEDLIST_Next
 *
 * Purpose: Get the next element in the list
 *
 * Arguments: Pointer to the queue Preceding element
 *
 * Returns: the next element in the list
 *
 *****************************************************************************/
void *OS_LINKEDLIST_Next (OS_List_Handle list, void *listElement )
{
#if( RTOS_SELECTION == MQX_RTOS )
   OS_QUEUE_Next ( OS_QUEUE_Handle) list, listElement );
#elif( RTOS_SELECTION == FREE_RTOS )
   if ( (NULL == list) ||  (NULL ==listElement) )
   {
      return (NULL);
   }
   else
   {
#if 0  // TODO: RA6E1: Remove once verified
      return ((OS_Linked_List_Element *) listElement)->NEXT;
#else
      OS_Linked_List_Element *next_ptr;
      next_ptr = ((OS_Linked_List_Element *) listElement)->NEXT;
      if( next_ptr == (OS_Linked_List_Element *) (( void *) list) )
      {
         next_ptr = NULL;
      }

      return (next_ptr);
#endif
   }
#endif
}

/******************************************************************************
 *
 * Function name: OS_LINKEDLIST_Dequeue
 *
 * Purpose: Returns the element at the front of the list
 *
 * Arguments: Given list
 *
 * Returns: Pointer to the first element in the list
 *
 *****************************************************************************/
void *OS_LINKEDLIST_Dequeue (OS_List_Handle list )
{
#if( RTOS_SELECTION == MQX_RTOS )
   return  OS_QUEUE_Dequeue( (OS_QUEUE_Handle) list);
#elif( RTOS_SELECTION == FREE_RTOS )
   if ( NULL == list || list->size == 0 ) {
      return NULL;
   }

   OS_Linked_List_Element *element  = list->Head;
   OS_Linked_List_Element *prev_ptr = element->PREV;
   OS_Linked_List_Element *nxt_ptr  = element->NEXT;
   prev_ptr->NEXT  = nxt_ptr;
   nxt_ptr->PREV   = prev_ptr;
   list->size--;
   /*clear out the pointers for the removed element*/
   element->NEXT = NULL;
   element->PREV = NULL;

   return ((void *) (element));
#endif
}


/******************************************************************************
 *
 * Function name: OS_LINKEDLIST_NumElements
 *
 * Purpose: Returns the number of elements in a list
 *
 * Arguments: Given element
 *
 * Returns: Pointer to the next element.
 *
 *****************************************************************************/
uint16_t OS_LINKEDLIST_NumElements ( OS_List_Handle list )
{
#if( RTOS_SELECTION == MQX_RTOS )
   return OS_QUEUE_NumElements( (OS_QUEUE_Handle) list);
#elif( RTOS_SELECTION == FREE_RTOS )
   if ( NULL == list) {
      return 0;
   }

   return (uint16_t)(list->size);
#endif
}

/******************************************************************************
 *
 * Function name: OS_QUEUE_Head
 *
 * Purpose: Returns the first element of the given list
 *
 * Arguments: List pointer
 *
 * Returns: Pointer to the first element in the list
 *
 *****************************************************************************/
void *OS_LINKEDLIST_Head ( OS_List_Handle list )
{
#if( RTOS_SELECTION == MQX_RTOS )
   return OS_QUEUE_Head( (OS_QUEUE_Handle) list);
#elif( RTOS_SELECTION == FREE_RTOS )
   if ( NULL == list  || list->size == 0 ){
      return NULL;
   }
   return ((void *) list->Head);
#endif
}


 /*****************************************************************************
                              TEST MODE CODE
*****************************************************************************/
#if( TM_LINKED_LIST == 2)
static OS_List_Obj testList;
static OS_List_Handle handle = &testList;
static OS_Linked_List_Element data1;
static OS_Linked_List_Element data2;
static OS_Linked_List_Element data3;
static OS_Linked_List_Element data4;
static OS_Linked_List_Element data5;
static uint16_t num;
static OS_Linked_List_Element_Ptr tmp;

void OS_LINKEDLIST_Test( void )
{
   OS_LINKEDLIST_Create(handle);
   OS_LINKEDLIST_Enqueue(handle, &data1);
   tmp = OS_LINKEDLIST_Head(handle);
   OS_LINKEDLIST_Enqueue(handle, &data3);
   OS_LINKEDLIST_Insert(handle, &data1, &data2);
   num = OS_LINKEDLIST_NumElements(handle);
   for( int i = 0; i< num ; i++)
   {
      tmp = OS_LINKEDLIST_Next(handle, tmp);
   }
   //tmp should point to the tail at this point
   tmp = OS_LINKEDLIST_Dequeue(handle);
   OS_LINKEDLIST_Remove(handle, &data2);

}
#endif

//
////******************************************************************************
// *
// * Function name: OS_QUEUE_Put
// *
// * Purpose: Atomically put an element at the end of the queue.
// *
// * Arguments: Pointers to the queue and the element
// *
// * Returns: -
// *
// * Notes: This function temporarily increases the priority of the calling thread
// *        to ensure that priority inversion doesn't occur.
// *
// *****************************************************************************/
//void OS_QUEUE_Put (OS_LIST_Obj *Queue, void *Elem)
//{
//
//
//   struct sched_param old, new;
//   int policy;
//   OS_LIST_Obj *prev;
//
//   if (ALERT (NULL == Elem) || ALERT (Queue == NULL) ||
//      ALERT (Queue->Prev == NULL) || ALERT (Queue->Lock == NULL)) {
//      return ;
//   }
//
//   prev = Queue->Prev;
//
//   if (pthread_getschedparam (pthread_self(), &policy, &old))
//   {
//    if (ALERT (EPERM == errno)) {
//         return; /* XXX Return an error? */
//      }
//   }
//
//   /* XXX BUMP priority to prevent priority inversion, this can be removed once glibc implements priority inheritance in locks */
//   new.__sched_priority = TSK_MAXPRI+1;
//
//   if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &new))
//   {
//    if (ALERT (EPERM == errno)) {
//         return; /* XXX Return an error? */
//      }
//   }
//
//   if (ALERT (bool_false == OS_LOCK_Lock (Queue->Lock, OS_WAIT_FOREVER))) {
//      return; /* XXX Return an error? */
//   }
//
//   ((OS_LIST_Obj *) Elem)->Lock = Queue->Lock;
//   ((OS_LIST_Obj *) Elem)->Next = Queue;
//   ((OS_LIST_Obj *) Elem)->Prev = prev;
//   prev->Next = (OS_LIST_Obj *) Elem;
//   Queue->Prev = (OS_LIST_Obj *) Elem ;
////   OS_LOCK_Unlock (Queue->Lock);
//
//   if (pthread_setschedparam (pthread_self(), policy, &old))
//   {
//    if (ALERT (EPERM == errno)) {
//         return; /* XXX Return an error? */
//      }
//   }
//
//}
//
//
////******************************************************************************
// *
// * Function name: OS_QUEUE_Get
// *
// * Purpose: Get an element from the head of the queue
// *
// * Arguments: Pointer to the queue
// *
// * Returns: Pointer to the element, or NULL if an error occurred
// *
// * Notes: This function temporarily increases the priority of the calling thread
// *        to ensure that priority inversion doesn't occur.
// *
// *****************************************************************************/
//void *OS_QUEUE_Get (OS_LIST_Obj *Queue)
//{
//   struct sched_param old, new;
//   int policy;
//   OS_LIST_Obj *elem;
//   OS_LIST_Obj *next;
//
//
//   if (ALERT (Queue == NULL) || ALERT (Queue->Next == NULL) ||
//      ALERT (Queue->Next->Next == NULL) || ALERT (Queue->Lock == NULL)) {
//      return NULL;
//  }
//
//   elem = Queue->Next;
//   next = elem->Next;
//   Queue->Next = next;
//   next->Prev = Queue;
//
//   if (pthread_getschedparam (pthread_self(), &policy, &old))
//   {
//      if (EPERM != errno)
//   perror("pthread_getschedparam"); /* XXX add error reporting */
//      return NULL;
//   }
//
//   /* XXX BUMP priority to prevent priority inversion, this can be removed once glibc implements priority inheritance in locks */
//   new.__sched_priority = TSK_MAXPRI+1;
//
//   if (pthread_setschedparam (pthread_self (), SCHED_FIFO, &new))
//   {
//      if (EPERM != errno)
//   perror ("pthread_setschedparam");
//      return NULL;
//   }
//
//   if (OS_LOCK_Lock (Queue->Lock, OS_WAIT_FOREVER))
//   {
//      Queue->Next = next;
//      next->Prev = Queue;
//      OS_LOCK_Unlock (Queue->Lock);
//   }
//   else
//   {
//      elem = NULL;
//   }
//
//   if (pthread_setschedparam(pthread_self(), policy, &old)) {
//      if (EPERM != errno)
//   perror ("pthread_setschedparam");  /* XXX add error reporting */
//      return NULL;
//   }
//
//   return (elem);
//}
