/******************************************************************************

   Filename: Linked_list.c

   Global Designator: OS_LINKEDLIST_

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
#if ( TM_LINKED_LIST == 1 )
#include "DBG_SerialDebug.h"  // Debug macros only neede in test mode
#include "DBG_CommandLine.h" // Calls to DBG_printf are only needed in test mode
#endif

/* CONSTANTS */

/* MACRO DEFINITIONS */
#if ( TM_LINKED_LIST == 1 )
#define OS_LINKEDLIST_TEST_PRINTF(a) ERR_printf(a)
#else
#define OS_LINKEDLIST_TEST_PRINTF(a)
#endif
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

   if( ( list == NULL ) || ( listElement == NULL ) )
   {
      return retVal;
   }
   if ( ( NULL == list->Head ) || ( NULL == list->Tail ) )
   {
      OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_verifyListElement called before OS_LINKEDLIST_Create");
      return retVal;
   }
   pElement = list->Head;
   if(  0 == list->size  )
   {
      return ((bool)false);
   }

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
      OS_LINKEDLIST_TEST_PRINTF( "NULL handle passed to OS_LINKEDLIST_Create" );
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
   if ( (NULL == list) ||  (NULL == listElement) )
   {
      OS_LINKEDLIST_TEST_PRINTF( "NULL handle or element passed to OS_LINKEDLIST_Enqueue");
      return;
   }
   if ( list->Head == NULL || list->Tail == NULL )
   {
      OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Enqueue called before OS_LINKEDLIST_Create");
      return;
   }
   if ( verifyListElement( list, listElement ) )
   {
      OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Enqueue called with element already in the list" );
      return;
   }
   OS_Linked_List_Element_Ptr pTail                = list->Tail;
   /* Update the Prev and Next of the new Element */
   ((OS_Linked_List_Element_Ptr)listElement)->NEXT = (OS_Linked_List_Element_Ptr)list;
   ((OS_Linked_List_Element_Ptr)listElement)->PREV = pTail;
   /* Update the Next of the new Tail, Prev is already pointing to the List, hence no need to update it. */
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
//   bool found = verifyListElement(list, ( OS_Linked_List_Element_Ptr ) listPosition);
   if( verifyListElement(list, ( OS_Linked_List_Element_Ptr ) listPosition) )
   {
      if ( ( list->Head != NULL ) && ( list->Tail != NULL ) ) // TODO: RA6E1 Bob: redundant check.  Remove later.
      {
         if ( verifyListElement( list, listElement ) )
         {
            OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Enqueue called with element already in the list" );
         }
         else
         {
            OS_Linked_List_Element_Ptr nxt = ((OS_Linked_List_Element_Ptr) listPosition)->NEXT;
            ((OS_Linked_List_Element_Ptr) listElement)->NEXT  = nxt;
            ((OS_Linked_List_Element_Ptr) listPosition)->NEXT = (OS_Linked_List_Element_Ptr) listElement;
            ((OS_Linked_List_Element_Ptr) listElement)->PREV  = (OS_Linked_List_Element_Ptr) listPosition;
            nxt->PREV                                         = (OS_Linked_List_Element_Ptr) listElement;

            list->size++;
            retVal = (bool)true;
         }
      }
      else
      {
         OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Insert called before OS_LINKEDLIST_Create");
      }
   }
   else if ( listPosition == NULL )
   {
      if ( ( list == NULL ) || ( listElement == NULL ) )
      {
         OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Insert called with NULL list or listelement");
      }
      else if ( ( list->Head == NULL ) || ( list->Tail == NULL ) )
      {
         OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Insert called before OS_LINKEDLIST_Create was called");
      }
      else if ( verifyListElement( list, listElement ) )
      {
         OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Enqueue called with element already in the list" );
      }
      else if ( list->size == 0)
      {
         OS_LINKEDLIST_Enqueue( list, listElement );
         retVal = (bool)true;
      }
      else
      {
         OS_Linked_List_Element_Ptr first = (OS_Linked_List_Element_Ptr)list->Head;
         list->Head = (OS_Linked_List_Element_Ptr)listElement;     /* set Head pointer in handle to point to new first element */
         first->PREV = (OS_Linked_List_Element_Ptr)listElement;    /* set PREV pointer in old first element to new first element */
         ((OS_Linked_List_Element_Ptr) listElement)->NEXT = first; /* set NEXT pointer in new first element to old first element */
         ((OS_Linked_List_Element_Ptr) listElement)->PREV = (OS_Linked_List_Element_Ptr)list;
         list->size++;
         retVal = (bool)true;
      }
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
void OS_LINKEDLIST_Remove ( OS_List_Handle list, void *listElement )
{
#if( RTOS_SELECTION == MQX_RTOS )
   OS_QUEUE_Remove(list, Elem);
#elif( RTOS_SELECTION == FREE_RTOS )
   if ( ( NULL == listElement ) || ( NULL == list ) ) // TODO: RA6E1 Future: create common parameter check function
   {
      OS_LINKEDLIST_TEST_PRINTF( "NULL handle or element passed to OS_LINKEDLIST_Remove");
      return;
   }
   if ( list->Head == NULL || list->Tail == NULL )
   {
      OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Remove called before OS_LINKEDLIST_Create");
      return;
   }

   if( verifyListElement(list, (OS_Linked_List_Element_Ptr) listElement) )
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
   if ( (NULL == list) || (NULL == listElement) )
   {
      OS_LINKEDLIST_TEST_PRINTF( "NULL handle or element passed to OS_LINKEDLIST_Next");
      return (NULL);
   }
   if ( ( NULL == list->Head ) || ( NULL == list->Tail ) )
   {
      OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Next called before OS_LINKEDLIST_Create");
      return (NULL);
   }
   else
   {
      OS_Linked_List_Element *next_ptr = ((OS_Linked_List_Element *) listElement)->NEXT;

      if( next_ptr == (OS_Linked_List_Element *) (( void *) list) )
      {
         next_ptr = NULL;
      }

      return (next_ptr);
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
   if ( NULL == list )
   {
      OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Dequeue called with a NULL handle");
      return NULL;
   }
   if ( ( list->Head == NULL ) || ( list->Tail == NULL ) )
   {
      OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Dequeue called before OS_LINKEDLIST_Create");
      return NULL;
   }
   if ( list->size == 0 )
   {
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
   if ( NULL == list )
   {
      OS_LINKEDLIST_TEST_PRINTF( "NULL handle passed to OS_LINKEDLIST_NumElements");
      return 0;
   }
   if ( ( NULL == list->Head ) || ( NULL == list->Tail ) )
   {
      OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_NumElements called before OS_LINKEDLIST_Create");
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
   if ( NULL == list )
   {
      OS_LINKEDLIST_TEST_PRINTF( "NULL handle passed to OS_LINKEDLIST_Head");
      return NULL;
   }
   if ( ( list->Head == NULL ) || ( list->Tail == NULL ) )
   {
      OS_LINKEDLIST_TEST_PRINTF( "OS_LINKEDLIST_Head called before OS_LINKEDLIST_Create");
      return NULL;
   }
   if ( list->size == 0)
   {
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

/******************************************************************************
 *
 * Function name: *OS_LINKEDLIST_ValidateList
 *
 * Purpose: Validate the linked list with the passed handle for errors/inconsistencies
 *
 * Arguments: Linked List handle
 *
 * Returns: String starting with either PASS: or FAIL: with explanation
 *
 *****************************************************************************/
char *OS_LINKEDLIST_ValidateList ( OS_List_Handle list, char *code )
{
#if ( TM_LINKED_LIST == 1 ) /* Make this a null function unless we are running in test mode 1 */
   static char str[100] = {0};
   const char *OKtext = "PASS: List traversed successfully with %u entries";
   *code = 'E';
   if ( NULL == list )
   {
      return ( "FAIL: List pointer was NULL" ); /* This should never occur but handle just in case */
   }
   if ( ( (void *)(list->Head) == NULL ) || ( (void *)(list->Tail) == NULL ) )
   {
     return ( "FAIL: List handle was never created, one or both pointers are NULL" );
   }
   if ( (void *)(list->Head) == (void *)list )
   {
      if ( (void *)(list->Tail) == (void *)list )
      {
         if ( list->size == 0 ) /* Pointers indicate an empty list so size should be zero */
         {
            *code = 'I';
            (void)snprintf( str, sizeof(str)-2, OKtext, list->size );
            return ( str );
         }
         else
         {
            return ( "FAIL: List pointers show empty list but list size is non-zero" );
         }
      }
      else
      {
         return ( "FAIL: List Head pointer points back to list but Tail pointer does not" );
      }
   }
   /* Head pointer does not point to the handle so list should not be empty */
   if ( list->size == 0 )
   {
      return ( "FAIL: List Head pointer shows non-empty list but list size is zero" );
   }
   else if ( (void *)(list->Tail) == (void *)list )
   {
      return ( "FAIL: List Tail pointer points back to list but Head pointer does not" );
   }
   /* The linked list is definitely not empty, does it have just one entry? */
   if ( list->size == 1 )
   {
      /* The check for a one-element linked list is straightforward, just check 4 pointers */
      if ( ( (void *)(list) == (void *)(list->Head->NEXT) ) &&
           ( (void *)(list) == (void *)(list->Head->PREV) ) &&
           ( (void *)(list) == (void *)(list->Tail->NEXT) ) &&
           ( (void *)(list) == (void *)(list->Tail->PREV) )    )
      {
         *code = 'I';
         (void)snprintf( str, sizeof(str)-2, OKtext, list->size );
      }
      else
      {
         (void)snprintf( str, sizeof(str)-2, "FAIL: One or more pointers are incorrect on list with %u entry", list->size );
      }
      return ( str );
   }
   else
   {
      /* The linked list contains more than one element.  Need to loop through and check consistency */
      OS_Linked_List_Element *prev = (OS_Linked_List_Element *)(list); /* Predecessor to first element, which is the handle */
      OS_Linked_List_Element *next = (OS_Linked_List_Element *)(list->Head); /* First element in the list */
      if ( ( (void *)(list) == (void *)(list->Head->PREV) ) && ( (void *)(list) == (void *)(list->Tail->NEXT) ) )
      {
         /* Loop through all but the last element in the linked list, last element gets special handling */
         for ( uint32_t i = 0; i < list->size-1; i++ )
         {
            char *errText = NULL;
            if ( (void *)next == NULL)
            {
               errText = "FAIL: Reached NULL pointer on element %u before the entire list was traversed";
            }
            else if ( (void *)next == (void *)list )
            {
               errText = "FAIL: Link list element %u pointed back to List before number of entries was exhausted";
            }
            else if ( (void *)(next->NEXT ) == NULL )
            {
               errText = "FAIL: NEXT pointer in element %u is NULL";
            }
            else if ( (void *)(next->NEXT->PREV) != (void *)next )
            {
               errText = "FAIL: PREV pointer from next element does not point back to element %u";
            }
            else if ( (void *)(next->PREV) == NULL )
            {
               errText = "FAIL: PREV pointer in element %u is NULL";
            }
            else if ( (void *)(next->PREV->NEXT) != (void *)next )
            {
               errText = "FAIL: NEXT pointer from previous element does not point to next element %u";
            }
            else if ( (void *)prev == NULL )
            {
               errText = "FAIL: Logic error in OS_LINKEDLIST_ValidateList; element %u, prev == NULL";
            }
            else if ( (void *)(prev->NEXT) != (void *)next )
            {
               errText = "FAIL: Logic error in OS_LINKEDLIST_ValidateList; element %u, prev->NEXT != next";
            }
            if ( errText != NULL )
            {
               (void)snprintf( str, sizeof(str)-2, errText, i );
               return ( str );
            }
            prev = next;       /* Save pointer to element we just checked */
            next = next->NEXT; /* Move to the next element in the linked list */
         }
         if ( (void *)next == NULL ) /* We should never encounter a NULL pointer but check just in case */
         {
            return ( "FAIL: NEXT pointer in the last element is NULL" );
         }
         else if ( (void *)(next->NEXT) != (void *)list )
         {
            return ( "FAIL: NEXT pointer in last element does not point to the List" );
         }
         *code = 'I';
         (void)snprintf( str, sizeof(str)-2, OKtext, list->size );
         return ( str );
      }
      else
      {
         if ( (void *)(list) != (void *)(list->Head->PREV) && ( (void *)(list) == (void *)(list->Tail->NEXT) ) )
         {
            (void)snprintf( str, sizeof(str)-2, "FAIL: list->Head->PREV does not point to the handle on first element" );
         }
         else if ( (void *)(list) == (void *)(list->Head->PREV) && ( (void *)(list) != (void *)(list->Tail->NEXT) ) )
         {
            (void)snprintf( str, sizeof(str)-2, "FAIL: list->Tail->NEXT does not point to the handle on first element" );
         }
         else if ( (void *)(list) != (void *)(list->Head->PREV) && ( (void *)(list) != (void *)(list->Tail->NEXT) ) )
         {
            (void)snprintf( str, sizeof(str)-2, "FAIL: list->Head->PREV and list->Tail->NEXT do not point to the handle on first element" );
         }
         return ( str );
      }
   }
#else // ( TM_LINKED_LIST == 1)
   return ( "Unhandled case");
#endif
}

/******************************************************************************
 *
 * Function name: OS_LINKEDLIST_DumpList
 *
 * Purpose: Pretty-print the list handle and elements in linked list order
 *
 * Arguments: Linked List handle, address of first element to calculate the entry number
 *
 * Returns: None
 *
 *****************************************************************************/
void OS_LINKEDLIST_DumpList ( OS_List_Handle list, OS_Linked_List_Element *firstElement )
{
#if ( TM_LINKED_LIST == 1 ) /* Make this a null function unless we are running in test mode 1 */
   if ( (void *)list != NULL )
   {
      DBG_log( 0, ADD_LF, " Address            Head      Tail      Size\r\n%08x        %08x  %08x  %8u",
                          (void *)list, (void *)list->Head, (void *)list->Tail, list->size );
      if ( list->size > 0 )
      {
         DBG_log( 0, ADD_LF, " Address  Entry      NEXT      PREV" );
         OS_Linked_List_Element *next = (OS_Linked_List_Element *)(list->Head);
         for (uint32_t i=0; ( ( i < list->size ) && ( NULL != next ) ); i++ )
         {
            uint32_t entry = 1 + ( (uint32_t)(void *)next - (uint32_t)firstElement ) / sizeof(OS_Linked_List_Element);
            DBG_log( 0, ADD_LF, "%08x   %2u    %08x  %08x", (void *)next, entry,
                    (void *)next->NEXT, (void *)next->PREV );
            next = next->NEXT;
         }
      }
   }
#endif // ( TM_LINKED_LIST == 1)
}
