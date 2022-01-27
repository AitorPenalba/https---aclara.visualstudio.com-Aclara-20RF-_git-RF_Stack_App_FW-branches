/***********************************************************************************************************************
 *
 * Filename:   object_list.c
 *
 * Global Designator: OL_
 *
 * Contents: OL_Manager - manages list(s) of enumerated object types (e.g., Registers, Opcodes, etc.) - objTypes
 *   Groups of each enumerated type are kept in linked lists - as modules start up, each can add lists of registers,
 *   opcodes, etc.  Each group is kept in a linked list (groupLL)
 *
 * NOTE:  All objects should be added before the task scheduler is running.
 *
 * Here is an overview of the link list structure:
 *
 * In this example, the type list consists of 2 types, Opcodes and Registers.  The Registers had a module that added
 * Object 1 which consists of 3 elements or registers.  Another modules added Object 2 that consists of 3 elements or
 * registers. Another type "Opcodes" was added with Objects A and B.  As shown below, the register/opcode numbers do
 * not need to be in any particular order.  The number of elements may vary and is specified in OL_tTableDef.
 *
 *
 * Type List:  Registers..........Opcodes
 * (TypesLL)       |                 |
 *                 |                 |
 *                 |                 --->ObjectLL: Object A.....................Object B
 *                 |                                   |                             |
 *                 |                                   |                             --->OL_tTableDef:  Element Cnt: 3
 *                 |                                   |                                                Opcode 22
 *                 |                                   |                                                Opcode 23
 *                 |                                   |                                                Opcode 120
 *                 |                                   |
 *                 |                                   --->OL_tTableDef:  Element Cnt: 2
 *                 |                                                      Opcode 1
 *                 |                                                      Opcode 50
 *                 |
 *                 |
 *                 --->ObjectLL: Object 1............Object 2
 *                                    |                   |
 *                                    |                   --->OL_tTableDef:  Element Cnt: 3
 *                                    |                                      Register 1
 *                                    |                                      Register 12
 *                                    |                                      Register 400
 *                                    |
 *                                    --->OL_tTableDef:  Element Cnt: 2
 *                                                       Register 2
 *                                                       Register 300
 *
 ***********************************************************************************************************************
 * Copyright (c) 2011 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
 * or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * $Log$ kdavlin Created Mar 31, 2011
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - 03/31/2011 - Initial Release
 *
 **********************************************************************************************************************/
/* INCLUDE FILES */
#include "project.h"
#include <stdbool.h>
#include <stdlib.h>
#include "string.h"

#define object_list_GLOBAL
#include "object_list.h"
#undef  object_list_GLOBAL

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

struct ObjectLinkedList   /* Forward reference mechanism for structure below */
{
   OL_tTableDef            *pTblDef;     /* Points to first element in this object */
   struct ObjectLinkedList *pNextObject; /* Pointer to next group in this type list   */
};

typedef struct ObjectLinkedList  ObjectLL;     /* Linked list of groups of objects, all of the same type   */

struct typesLinkedList   /* Top level linked list - list of object types (e.g., registers, opcodes, etc.) */
{
   OL_eObjectType         eType;        /* type of list (e.g., registers, opcodes, etc.)   */
   ELEMENT_SIZE           ElementSize;  /* Size of each element in this list   */
   ObjectLL               *pObjectHead; /* Pointer to head of object group     */
   struct typesLinkedList *pNextType;   /* Pointer to next type                */
};

typedef struct typesLinkedList TypesLL;   /* Forward reference mechanism for structure below */
/*
   The only attribute required of an "object" is an ID.
   To instantiate an object that is compatible with the list manager, include an "obj" element as the first element.
*/

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* Head of master object list - initialized to be EMPTY  */
STATIC struct
{
   TypesLL *pHead;  /* Initialize to NULL, The head will be set when the 1st type is added. */
   uint16_t  u16Cnt;  /* Number of types that have been added. */
}_sTypeInfo =
{
   NULL,    // No forward pointer
   0        // Zero entries in the list
};

#ifdef TM_OL_HEAP_SIZE
uint16_t   heapUsed = 0;  /* Used with the debugger to see how much heap is used.  Just set a breakpoint in the main loop
                         * and look at this variable in a watch window. */
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

STATIC returnStatus_t SearchForType(OL_eObjectType eType, TypesLL **pTypeAddr);

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************
 *
 * Function Name: OL_Add
 *
 * Purpose: Adds an object to the list.  If the eType passed is has not been defined, it is created and added to the
 *          list of types.
 *
 *          NOTE:  This should only be called before the task scheduler is running.
 *
 * Arguments:
 *    OL_eObjectType eType: Enumerated name of the type being added
 *    uint16_t u16SizeOfElement: Number of bytes per element.
 *    OL_tTableDef *pAddr:  Address of the object being added
 *
 * Returns: returnStatus_t - eSUCCESS or eFAILURE
 *
 * Side Effects: Each item added uses RAM (heap) to create the link.
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
returnStatus_t OL_Add(const OL_eObjectType eType, const uint16_t u16SizeOfElement, OL_tTableDef const *pAddr)
{
   returnStatus_t eRetVal = eSUCCESS;   /* Return value */
   TypesLL *pType = NULL;  /* Contains the pointer to the type requested, it is filled in by the search function. */

   /* Search the eTypes to see if eType is found */
   if (eSUCCESS != SearchForType(eType, &pType))  /* pType is set to the last valid link OR the one found. */
   {
      /* The eType requested doesn't exist.  The type will be created and added to the list of types. */

      TypesLL *pNewType;  /* Contains a pointer to the new type that will be created. */

      /* Not found, so Create the next type link */
      pNewType = (TypesLL *)malloc(sizeof(TypesLL)); /* Allocate the new block of memory (next link) */
#ifdef TM_OL_HEAP_SIZE
               heapUsed += sizeof(TypesLL);  /* Add number of bytes malloc'd, for test mode. */
#endif
      if (NULL != pNewType) /* Error check, did the malloc work? */
      {
         pNewType->eType = eType;   /* Set the type to the value passed in */
         pNewType->pNextType = NULL; /* Set the next link to NULL */
         pNewType->ElementSize = u16SizeOfElement; /* Set the Element size to the value passed in */
         if (NULL == pType)   /* If pType is NULL, that means there are no eTypes defined yet, so initialize it */
         {
            pType = pNewType;    /* pType now points to the 'found' eType (the only one) in the list */
            _sTypeInfo.pHead = pType;  /* The Head now points to the only item in the list */
         }
         else
         {
            /* At least one type has been created, since pType points to the last item, set its next link to point to the
             * new type created. */
            pType->pNextType = pNewType; /* Make the link to the new eType added to the list */
            pType = (TypesLL *)pType->pNextType; /* Make pType point to the new link just added */
            /* pType now points to the object just created (pNewType). */
         }

         /* The New Type Link has been created, now create the head of the new Object List. Add the Object passed in. */
         pType->pObjectHead = (ObjectLL *)malloc(sizeof(ObjectLL)); /* Allocate new block of memory */
#ifdef TM_OL_HEAP_SIZE
         heapUsed += sizeof(ObjectLL);  /* Add number of bytes malloc'd, for test mode. */
#endif
         if (NULL != pType->pObjectHead) /* Error check, did the malloc work? */
         {
            pType->pObjectHead->pNextObject = NULL; /* Clear the pointer to the next link since this is the last item. */
            pType->pObjectHead->pTblDef = (OL_tTableDef *)pAddr;  /* Set the Object Pointer to the item passed in, pAddr. */
            _sTypeInfo.u16Cnt++; /* Increment the number of types defined. */
         }
         else
         {
            eRetVal = eFAILURE;
         }
      }
      else
      {
         eRetVal = eFAILURE;
      }
   }
   else
   {
      /* The eType requested exists.  The new object needs to be added to the end of the list for eType. */

      /* Error check, the number of elements passed in should match the existing number of elements that was previously
       * defined for the eType.  If it doesn't match, there is an error!  */

      /* For example, if module A defines a register definition as 68 bytes and module B defines it as 70, the search
       * process below will not know how to properly index through the list to find the requested definition.  */

      if (pType->ElementSize == u16SizeOfElement)
      {
         /* Number of elements passed, now create the new entry */
         ObjectLL *pLastObject = pType->pObjectHead;

         /* Find the last entry to add the new link. */
         while (NULL != pLastObject->pNextObject)
         {
            pLastObject = (ObjectLL *)pLastObject->pNextObject;
         }

         /* Now pLastObject is pointing the last object, add the new link and configure it. */
         /* The New link has been created, now create the start of the new Object Link List with the previous object */
         pLastObject->pNextObject = (ObjectLL *)malloc(sizeof(ObjectLL)); /* Allocate the new object */
         if (NULL != pLastObject->pNextObject) /* Error check, did the malloc work? */
         {
            pLastObject = pLastObject->pNextObject; /* Make the pLastObject the last object again. */
            pLastObject->pNextObject = NULL;  /* Set the next link as NULL since this is the last object. */
            pLastObject->pTblDef = (OL_tTableDef *)pAddr; /* Set the object pointer to point to the desired object. */
         }
         else
         {
            eRetVal = eFAILURE;
         }
      }
      else
      {
         eRetVal = eFAILURE;  /* The element size cannot change! If the size changed, the search wouldn't work. */
      }
   }
   return(eRetVal);
}
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Function Name: OL_Search
 *
 * Purpose: Searches for an element in an object (example: register ID or Opcode Number).  This implementation requires
 *          that the ID is defined as two bytes.
 *
 * Arguments:
 *    OL_eObjectType eType: Enumerated name of the type being searched
 *    uint16_t u16Id: The objects ID (register number or Opcode number for example).
 *    void **pAddr:  Address of the element.
 *
 * Returns: returnStatus_t - eSUCCESS or eFAILURE
 *
 * Side Effects: None
 *
 * Reentrant Code: Yes
 *
 **********************************************************************************************************************/
returnStatus_t OL_Search(const OL_eObjectType eType, const ELEMENT_ID ElementId, void **pAddr)
{
   returnStatus_t eRetVal = eFAILURE;   /* Return Value */
   TypesLL *pType;  /* After the search for the Type, pType will point to the List of Objects to search in. */

   /* Search the eTypes to see if eType is found */
   if (eSUCCESS == SearchForType(eType, &pType))
   {
      ObjectLL  *pObject;  /* Pointer to object that may contain the u16ID requested. */

      /* Search each Object until either the ID is found or the end of the object list is reached. */
      for (pObject = (ObjectLL *)pType->pObjectHead;
           (NULL != pObject) && (eSUCCESS != eRetVal);
           pObject = pObject->pNextObject)
      {
         NUM_ELEMENTS i; /* Used as a counter. */
         tElement *pElement = pObject->pTblDef->pElement; /* Search each element in the object. */
         for (i = 0; (i < pObject->pTblDef->u16NumOfElements) && (eSUCCESS != eRetVal); i++)
         {
            if (ElementId == pElement->ElementId)  /* Does this element match the ID requested? */
            {
               *pAddr = pElement;   /* Yes, found it!  Now set the address pointer to point to the element. */
               eRetVal = eSUCCESS;  /* Return SUCCESS! Setting return value will get us out of the loops. */
            }
            else
            {
               void     *Next = pElement;
               Next = (uint8_t *)Next + pType->ElementSize;
               pElement = (tElement *)Next;
            }
         }
      }
   }
   return(eRetVal);
}
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Function Name: OL_GetNumOfTypes
 *
 * Purpose: Sets *pCnt to the number of Types created.
 *
 * Arguments:
 *    uint16_t *pCnt:  Pointer to Number of objects
 *
 * Returns: returnStatus_t - eSUCCESS, there is nothing to fail here!
 *
 * Side Effects: None
 *
 * Reentrant Code: Yes
 *
 **********************************************************************************************************************/
#if 0
NUM_TYPES OL_GetNumOfTypes(void)
{
   return(_sTypeInfo.u16Cnt);          /* There is no way to fail here! */
}
#endif
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Function Name: OL_GetNumOfObjectsOfType
 *
 * Purpose: Sets *pCnt to the number of Objects of a certain type.  For example, returns the number of register objects.
 *
 * Arguments:
 *    OL_eObjectType eType: Enumerated name of the type being searched
 *    uint16_t *pCnt:  Number of Elements inside an object.
 *
 * Returns: returnStatus_t - eSUCCESS, there is nothing to fail here!
 *
 * Side Effects: None
 *
 * Reentrant Code: Yes
 *
 **********************************************************************************************************************/
#if 0
NUM_OBJECTS OL_GetNumOfObjectsOfType(const OL_eObjectType eType)
{
   TypesLL     *pType;     /* After the search for the Type, pType will point to the List of Objects to search in. */
   NUM_OBJECTS NumObjects = 0;   /* If the eType is not found, no objects exist.  Assume no types are found. */

   if (SearchForType(eType, &pType) == eSUCCESS)   /* Search for the Type passed in */
   {
      ObjectLL *pObject; /* Points to the head object and then to each object in the list. */

      for (pObject = (ObjectLL *)pType->pObjectHead; /* There is always 1 object if the type exists */
           NULL != pObject;
           pObject = (ObjectLL *)pObject->pNextObject)
      {
         NumObjects++;   /* Another Object exists, increment the counter. */
      }
   }
   return(NumObjects); /* Always successful because all return values are valid. */
}
#endif
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Function Name: OL_GetNumOfElements
 *
 * Purpose: Sets *pCnt to the number of elements in an object.
 *
 * Arguments:
 *    OL_eObjectType eType: Enumerated name of the type being searched
 *    uint16_t *pCnt:  Number of Elements inside an object.
 *
 * Returns: returnStatus_t - eSUCCESS, there is nothing to fail here!
 *
 * Side Effects: None
 *
 * Reentrant Code: Yes
 *
 **********************************************************************************************************************/
#if 0
NUM_ELEMENTS OL_GetNumOfElements(const OL_eObjectType eType)
{
   TypesLL *pType;  /* After the search for the Type, pType will point to the List of Objects to search in. */
   NUM_ELEMENTS NumElements = 0;        /* Assume no elements are found. */

   if (SearchForType(eType, &pType) == eSUCCESS)
   {
      ObjectLL *pObject; /* Points to the head object and then to each object in the list. */

      for (pObject = (ObjectLL *)pType->pObjectHead;  /* Start at the head of the object list. */
           NULL != pObject;   /* While pObject is not NULL. */
           pObject = (ObjectLL *)pObject->pNextObject) /* Set the pObject pointer to the next link. */
      {
         NumElements += pObject->pTblDef->u16NumOfElements; /* Inc. the counter by the number of elements defined. */
      }
   }
   return(NumElements); /* Always successful because all return values are valid. */
}
#endif
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Function Name: SearchForType
 *
 * Purpose: To find a type in the master list
 *
 * Arguments:
 *    OL_eObjectType eType:  Type to find
 *    TypesLL **pTypeAddr: Pointer to Location of the type if found, if not found, this points to the last object
 *
 * Returns: returnStatus_t - eSUCCESS or eFAILURE
 *
 * Side Effects: None
 *
 * Reentrant Code: Yes
 *
 **********************************************************************************************************************/
STATIC returnStatus_t SearchForType(OL_eObjectType eType, TypesLL **pTypeAddr)
{
   returnStatus_t eRetVal   = eFAILURE;         /* Presume type won't be found */
   TypesLL  *pLocTypes     = _sTypeInfo.pHead; /* Use to traverse the types linked list */

   /* Traverse the linked list until either:
      1) The structure pointer is NULL - failure
      2) The request type is found - SUCCESS
   */

   // Is the check for OL_eUNDEFINED needed?
   // If so, must check AFTER locTypes found not NULL - move inside loop or REMOVE??

   while (NULL != pLocTypes) // && (OL_eUNDEFINED != pLocTypes->eType))
   {
      *pTypeAddr = pLocTypes;
      if (eType == pLocTypes->eType)   /* Does the type match? */
      {
         eRetVal = eSUCCESS;  /* Found the request type; SUCCESS! */
         break;
      }
      else  /* The type didn't match, move to the next type. */
      {
         pLocTypes = pLocTypes->pNextType;   /* If NULL, while loop will exit, with return value set to failure. */
      }
   }
   return eRetVal;
}
/* ****************************************************************************************************************** */
