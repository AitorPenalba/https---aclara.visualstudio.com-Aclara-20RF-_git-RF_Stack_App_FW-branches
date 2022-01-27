/** ****************************************************************************
@file indication.c

Code for registering indication handlers and passing indications.

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.
*******************************************************************************/

/*******************************************************************************
#INCLUDES
*******************************************************************************/

#include "indication.h"
#include "DBG_SerialDebug.h"

/*******************************************************************************
Local #defines (Object-like macros)
*******************************************************************************/

/*******************************************************************************
Local #defines (Function-like macros)
*******************************************************************************/

/*******************************************************************************
Local Struct, Typedef, and Enum Definitions (Private to this file)
*******************************************************************************/

/*******************************************************************************
Local Const Definitions
*******************************************************************************/

/*******************************************************************************
Static Function Prototypes
*******************************************************************************/

/*******************************************************************************
Global Variable Definitions
*******************************************************************************/

/*******************************************************************************
Static Variable Definitions
*******************************************************************************/

/*******************************************************************************
Function Definitions
*******************************************************************************/

/**
Initializes a new instance of the indication handler object.

@param instance Pointer to the data structure storing information about the indication handlers.

@return true on success else false
*/
bool IndicationCreate(IndicationHandle_t instance)
{
   if (!OS_MUTEX_Create(&instance->mutex))
   {
      return false;
   }

   instance->count = 0;

   return true;
}

/**
Registers a new handler for indications.

@param instance Pointer to the data structure storing information about the indication handlers.
@param handler Function pointer to the new handler being added.

@return true on success else false
*/
bool IndicationRegisterHandler(IndicationHandle_t instance, IndicationFunction_t handler)
{
   bool result = true;

   OS_MUTEX_Lock(&instance->mutex);

   if (instance->count < MAX_INDICATION_HANDLERS)
   {
      instance->handlers[instance->count] = handler;
      instance->count++;
   }
   else
   {
      result = false;
   }

   OS_MUTEX_Unlock(&instance->mutex);

   return result;
}

/**
Checks if any handlers are currently registered.

@param instance Pointer to the data structure storing information about the indication handlers.

@return true if a handler is registered, else false
*/
bool IndicationHandlerRegistered(IndicationHandle_t instance)
{
   return instance->count > 0;
} /*lint !e818 */

/**
Sends an indication to all registered handlers.

@param instance Pointer to the data structure storing information about the
   indication handlers.
@param indication Pointer to the indication to send.  Handlers are responsible
   for cleaning up the indication memory.  Byte copies of the data are
   created if there are multiple handlers registered.
*/
void IndicationSend(IndicationHandle_t instance, buffer_t *indication)
{
   int numHandlers = instance->count;

   if (numHandlers == 0)
   {
      // as handlers free the memory but nothing is registered clean up the memory
      BM_free(indication);
      return;
   }

   // if there is more than 1 registered handler create a byte copy to send to the other handlers
   // Note: might want to explore reference counting for buffers if there ends up being a lot of copying
   while (numHandlers > 1)
   {
      numHandlers--;

      buffer_t *copy = BM_allocStack(indication->x.dataLen);
      if (copy != NULL)
      {
         copy->eSysFmt = indication->eSysFmt;
         (void)memcpy(copy->data, indication->data, indication->x.dataLen);
         instance->handlers[numHandlers](copy);
      }
      else
      {
         ERR_printf("Error creating copy of indication type %u length %u", indication->eSysFmt, indication->x.dataLen);
      }
   }

   // finally send the original indication to the last handler
   instance->handlers[0](indication);
}  /*lint !e818 */
