/** ****************************************************************************
@file indication.h

Defines the API for indication handling in the SRFN product.

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.
*******************************************************************************/

#ifndef INDICATION_H
#define INDICATION_H

/*******************************************************************************
#INCLUDES
*******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "buffer.h"
#include "OS_Aclara.h"

/*******************************************************************************
Public #defines (Object-like macros)
*******************************************************************************/

#define MAX_INDICATION_HANDLERS 5

/*******************************************************************************
Public #defines (Function-like macros)
*******************************************************************************/

/*******************************************************************************
Public Struct, Typedef, and Enum Definitions
*******************************************************************************/

/** Defines the handler function interface */
typedef void (*IndicationFunction_t)(buffer_t *indication);

/** Stores all the information about registered handlers. */
typedef struct
{
   /** Used to control access to state when adding new handlers */
   OS_MUTEX_Obj mutex;

   /** Current number of handlers stored */
   uint8_t count;

   /** Pointers to the handlers */
   IndicationFunction_t handlers[MAX_INDICATION_HANDLERS];
} IndicationObject_t, *IndicationHandle_t;

/*******************************************************************************
Global Variable Extern Statements
*******************************************************************************/

/*******************************************************************************
Public Function Prototypes
*******************************************************************************/

bool IndicationCreate(IndicationHandle_t instance);
bool IndicationRegisterHandler(IndicationHandle_t instance, IndicationFunction_t handler);
bool IndicationHandlerRegistered(IndicationHandle_t instance);
void IndicationSend(IndicationHandle_t instance, buffer_t *indication);

#endif
