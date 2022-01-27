/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: object_list.h
 *
 * Contents: Creates a list of objects, searches the lists of objects for a unique ID.  (Used for register & Opcode
 *           lists.
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
 **********************************************************************************************************************/
#ifndef OBJECT_LIST_H_
#define OBJECT_LIST_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef object_list_GLOBAL
   #define GLOBAL
#else
   #define GLOBAL extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef uint16_t ELEMENT_ID;    /* used for object ID's; allows easy changing of size for other platforms, etc.  */
typedef uint16_t ELEMENT_SIZE;  /* used for size of an object; allows easy changing of size for other platforms, etc.  */
typedef uint16_t NUM_ELEMENTS;  /* used for number of objects in a group; allows easy changing of size for other platforms, etc.  */
typedef uint16_t NUM_TYPES;     /* used for number of types in the list; allows easy changing of size for other platforms, etc.  */
typedef uint16_t NUM_OBJECTS;   /* used for number of objects in the list; allows easy changing of size for other platforms, etc.  */

typedef enum
{
   OL_eUNDEFINED = (uint8_t)0,  /* Undefined Object */
   OL_eREGISTER,                /* Used for registers */
   OL_eOPCODE,                  /* Used for Opcodes */
   OL_eHMC_QUANT_SIG,           /* Used for HMC Quantity Signature List */
   OL_eHMC_DIRECT               /* Use for HMC direct read lookup */
}OL_eObjectType;                /* List of Objects we plan to use this module for */

typedef struct                /* Lowest level item handled by OL_manager   */
{
   ELEMENT_ID   ElementId; /* Unique identifier for this object (e.g., register ID, opcode number, etc.) */
}tElement;                 /* Lowest level object managed by the OL_manager   */

/* Each module will use this structure to define the table added to the object list.  This structure is what the Object
 * list module will use to access the module's table. */
typedef struct
{
   tElement  *pElement;          /* a pointer to the 1st element in the table. */
   uint16_t    u16NumOfElements;   /* The number of elements the object contains */
}OL_tTableDef;                   /* The Object points to this definition. */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

GLOBAL returnStatus_t OL_Add(const OL_eObjectType eType, const uint16_t u16SizeOfElement, OL_tTableDef const *pAddr);
GLOBAL returnStatus_t OL_Search(const OL_eObjectType eType, const ELEMENT_ID ElementId, void **pAddr);
GLOBAL NUM_TYPES     OL_GetNumOfTypes(void);
GLOBAL NUM_OBJECTS   OL_GetNumOfObjectsOfType(const OL_eObjectType eType);
GLOBAL NUM_ELEMENTS  OL_GetNumOfElements(const OL_eObjectType eType);

#undef GLOBAL

#endif
