/** ****************************************************************************
@file b2b.h
 
API for the Board to Board interface code.
 
A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.
*******************************************************************************/

#ifndef B2B_H
#define B2B_H

/*******************************************************************************
#INCLUDES
*******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "buffer.h"
#include "error_codes.h" // for returnStatus_t
#include "hdlc_frame.h"
#include "heep.h" // for meterReadingType

/*******************************************************************************
Public #defines (Object-like macros)
*******************************************************************************/

#define B2B_HDLC_ADDR 1
#define NW_HDLC_ADDR 2

#define B2B_PKT_HDR_SZ 7

/*******************************************************************************
Public #defines (Function-like macros)
*******************************************************************************/

/*******************************************************************************
Public Struct, Typedef, and Enum Definitions
*******************************************************************************/

/** Defines the different types of messages that are allowed. */
typedef enum
{
   B2B_MT_GET_REQ = 0,
   B2B_MT_GET_RESP = 1,
   B2B_MT_SET_REQ = 2,
   B2B_MT_SET_RESP = 3,
   B2B_MT_BASIC_GATEWAY = 4,
   B2B_MT_TIME_REQ = 6,
   B2B_MT_TIME_RESP = 7,
   B2B_MT_TIME_SYNC = 8,
   B2B_MT_RESET = 9,
   B2B_MT_ECHO_REQ = 10,
   B2B_MT_ECHO_RESP = 11,
   B2B_MT_DECOM = 12,
   B2B_MT_IDENT_REQ = 13,
   B2B_MT_IDENT_RESP = 14,
   B2B_MT_REFRESH_NW_REQ = 15
} B2BMsg_t;

/** Stores the B2B packet header information. */
typedef struct
{
   uint8_t version;
   uint16_t length;
   B2BMsg_t messageType;
   uint16_t handle;
} B2BPacketHeader_t;

/*******************************************************************************
Global Variable Extern Statements
*******************************************************************************/

/*******************************************************************************
Public Function Prototypes
*******************************************************************************/

returnStatus_t B2BInit(void);
void B2BRxTask(uint32_t arg0);

uint16_t GetNextHandle(void);

buffer_t *SendRequest(const HdlcFrameData_t *data, uint8_t retries, uint16_t timeout, B2BMsg_t expResp, uint16_t expRespHandle);

void B2BStoreUint16(uint8_t *buffer, uint16_t value);
void B2BStoreUint32(uint8_t *buffer, uint32_t value);
uint16_t B2BGetUint16(const uint8_t *buffer);
uint32_t B2BGetUint32(const uint8_t *buffer);

#endif
