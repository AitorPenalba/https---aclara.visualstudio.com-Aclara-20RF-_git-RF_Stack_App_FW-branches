/** ****************************************************************************
@file b2bCommand.h
 
API for sending Board to Board Messages.
 
A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.
*******************************************************************************/

#ifndef B2BMESSAGE_H
#define B2BMESSAGE_H

/*******************************************************************************
#INCLUDES
*******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "project.h" // Needed for MAC_Protocol.h
#include "heep.h" // for meterReadingType
#include "MAC_Protocol.h" // for time_set_t

/*******************************************************************************
Public #defines (Object-like macros)
*******************************************************************************/

/** Specifies supported message version */
#define B2B_VER 1

/*******************************************************************************
Public #defines (Function-like macros)
*******************************************************************************/

/*******************************************************************************
Public Struct, Typedef, and Enum Definitions
*******************************************************************************/

/** Defines the responses to a Get request */
typedef enum
{
   B2B_GET_SUCCESS = 0,
   B2B_GET_NOT_SUPP = 254,
   B2B_GET_GEN_ERR = 255
} B2BGetResp_t;

/** Defines the responses to a Set command */
typedef enum
{
   B2B_SET_SUCCESS = 0,
   B2B_SET_READ_ONLY = 1,
   B2B_SET_INVALID = 2,
   B2B_SET_NOT_SUPP = 254,
   B2B_SET_GEN_ERR = 255
} B2BSetResp_t;

/*******************************************************************************
Global Variable Extern Statements
*******************************************************************************/

/*******************************************************************************
Public Function Prototypes
*******************************************************************************/

B2BGetResp_t B2BGetParameter(meterReadingType type, void *buffer, uint16_t bufferLen);
B2BSetResp_t B2BSetParameter(meterReadingType type, void *buffer, uint16_t bufferLen);

bool B2BSendIdentityReq(uint8_t *buffer, uint16_t bufferLen, uint16_t *numBytesRec);
bool B2BSendTimeReq(time_set_t *time);
bool B2BSendEchoReq(void);
bool B2BSendReset(void);
bool B2BSendDecommission(void);
bool B2BSendRefreshNw(void);
bool B2BSendGatewayTraffic(const uint8_t *buffer, uint16_t bufferLen);
bool B2BSendTimeSync(void);

bool B2BSendTimeResp(uint16_t reqHandle);
bool B2BSendGetResp(B2BGetResp_t status, uint16_t reqHandle, meterReadingType type, const void *buffer, uint16_t bufferLen);
bool B2BSendSetResp(B2BSetResp_t status, uint16_t reqHandle, meterReadingType type, const void *buffer, uint16_t bufferLen);
bool B2BSendEchoResp(uint16_t reqHandle);

#endif
