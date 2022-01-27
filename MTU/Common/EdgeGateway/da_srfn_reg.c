/** ********************************************************************************************************************
@file DA_SRFN_REG.c

Contains the routines to support the SRFN Registration.

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2017 Aclara.  All Rights Reserved.
***********************************************************************************************************************/

/***********************************************************************************************************************
#INCLUDES
***********************************************************************************************************************/

#include <stdbool.h>

#include "project.h" // required for a lot of random headers so be safe and include 1st
#include "b2b.h"
#include "b2bmessage.h"
#include "dbg_serialdebug.h"
#include "heep.h"

#include "da_srfn_reg.h"

/***********************************************************************************************************************
Local #defines (Object-like macros)
***********************************************************************************************************************/

/***********************************************************************************************************************
Local #defines (Function-like macros)
***********************************************************************************************************************/

#define MAX_REG_SZ 200
#define MAX_TIME_BETWEEN_REFRESH 5000
#define DA_REG_STR_LEN 10

/***********************************************************************************************************************
Local Struct, Typedef, and Enum Definitions (Private to this file)
***********************************************************************************************************************/

typedef struct
{
   bool validData;
   uint32_t lastReadTime;
   uint16_t numRegBytes;
   uint8_t regData[MAX_REG_SZ]; // Note: data is stored as specified in B2B Specification: 1 byte - len, 2 byte - type, len bytes - value
} DA_DRIVER_Info_t;

/***********************************************************************************************************************
Local Const Definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Static Function Prototypes
***********************************************************************************************************************/

/***********************************************************************************************************************
Global Variable Definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Static Variable Definitions
***********************************************************************************************************************/

static DA_DRIVER_Info_t daSrfnRegistrationInfo;

/***********************************************************************************************************************
Function Definitions
***********************************************************************************************************************/

/**
Initialize the registration information structure.

@return eSUCCESS/eFAILURE indication
*/
returnStatus_t DA_SRFN_REG_Init(void)
{
   daSrfnRegistrationInfo.numRegBytes = 0;
   daSrfnRegistrationInfo.validData = false;

   return eSUCCESS;
}

/**
Gets a registration value read from the external device.

@param reqType Specifies the value to get.
@param buffer location to store the data.
@param buffLen Max number of bytes available in storage location.

@return Number of bytes stored in the buffer.  Will be 0 if not available or cannot fit.
*/
uint8_t RegistraionGetValue(meterReadingType reqType, void *buffer, uint8_t buffLen)
{
   uint32_t currTime = OS_TICK_Get_ElapsedMilliseconds();

   // if registration not updated recently get the latest values
   if (currTime - daSrfnRegistrationInfo.lastReadTime > MAX_TIME_BETWEEN_REFRESH)
   {
      daSrfnRegistrationInfo.validData = false;
   }

   if (!daSrfnRegistrationInfo.validData)
   {
      if (B2BSendIdentityReq(daSrfnRegistrationInfo.regData, MAX_REG_SZ, &daSrfnRegistrationInfo.numRegBytes))
      {
         daSrfnRegistrationInfo.validData = true;
         daSrfnRegistrationInfo.lastReadTime = currTime;
         INFO_printf("Registration information successfully read from DA");
      }
   }

   if (!daSrfnRegistrationInfo.validData)
   {
      return 0;
   }

   uint16_t offset = 0;
   uint8_t *readLoc = daSrfnRegistrationInfo.regData;

   while (offset < daSrfnRegistrationInfo.numRegBytes)
   {
      uint8_t itemLen = *readLoc++;
      uint16_t itemType = B2BGetUint16(readLoc);
      readLoc += sizeof(itemType);

      if ((itemType == (uint16_t)reqType) && (buffLen >= itemLen))
      {
         (void)memcpy(buffer, readLoc, itemLen);
         return itemLen;
      }

      offset += sizeof(itemLen) + sizeof(itemType) + itemLen;
      readLoc += itemLen;
   }

   return 0;
}

/**
Handles over the air reading of registration information

@param action specifes the request action.
@param id the reading type requested.
@param value pointer to the value requested
@param attr pointer to the typecast and length of value

@return status of the request
*/
returnStatus_t DA_SRFN_REG_OR_PM_Handler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr)
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure
   char strDaSrfnReg [DA_REG_STR_LEN];
   uint8_t regStrLen = 0;
   (void)memset(strDaSrfnReg, 0, sizeof(strDaSrfnReg));

   // all registation values are read only
   if (action == method_get)
   {
      regStrLen = RegistraionGetValue(id, strDaSrfnReg, sizeof(strDaSrfnReg));
      if (regStrLen != 0)
      {
         memcpy((char*) value, strDaSrfnReg, regStrLen);
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)regStrLen;
            attr->rValTypecast = (uint8_t)ASCIIStringValue;
         }
      }
      else
      {
         retVal = eFAILURE;
      }
   }
   else
   {
      retVal = eAPP_READ_ONLY;
   }

   return retVal;
}


/**
Handles reading requests for the DA

@param RdgType specifies the reading type id.
@param reading pointer to store info about the reading.

@return status of the request
*/
enum_CIM_QualityCode DA_SRFN_REG_getReading( meterReadingType RdgType, pValueInfo_t reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_CODE_INDETERMINATE;
   uint8_t numberOfBytes = 0;

   numberOfBytes = RegistraionGetValue(RdgType, reading->Value.buffer, (uint8_t)sizeof(reading->Value.buffer));
   reading->typecast = ASCIIStringValue;
   if( numberOfBytes > 0 )
   {
      reading->valueSizeInBytes = numberOfBytes;
      retVal = CIM_QUALCODE_SUCCESS;
   }

   return retVal;
}
