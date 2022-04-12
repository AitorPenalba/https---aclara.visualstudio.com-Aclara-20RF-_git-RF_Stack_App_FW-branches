/******************************************************************************
 *
 * Filename: HEEP_util.h
 *
 * Contents: General HEEP message retrieval/storage functions
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ cmusial Created May 27, 2015
 *
 **********************************************************************************************************************/

#ifndef HEEP_UTIL_H
#define HEEP_UTIL_H

/* INCLUDE FILES */
#include "buffer.h"
#include "MAC_Protocol.h"
#include "STACK.h"
#include "project.h"
#include "pack.h"

#ifdef HEEP_UTIL_GLOBALS
#define HEEP_UTIL_GLOBALS
#else
#define HEEP_UTIL_GLOBALS extern
#endif

/* CONSTANTS */

#define HEEP_APP_HEADER_SIZE                   (4)    //Size of application header

#define EXCHANGE_WITH_ID_FLAGS_IS_COINCIDENT 0x80
#define EXCHANGE_WITH_ID_FLAGS_TIMESTAMP_PRESENT 0x40
#define EXCHANGE_WITH_ID_FLAGS_READINGQUAL_PRESENT 0x20
#define EXCHANGE_WITH_ID_FLAGS_USE 0x10

#define MAX_OR_PM_PAYLOAD_SIZE  ( 512 )

#define HEEP_APPHDR_INTERFACE_REVISION_SIZE_BITS  (8)    //Size of the interface revision field
#define HEEP_APPHDR_TRANSACTION_TYPE_SIZE_BITS    (2)    //Size of the transaction type field
#define HEEP_APPHDR_RESOURCE_SIZE_BITS            (6)    //Size of the resource field
#define HEEP_APPHDR_METHOD_STATUS_SIZE_BITS       (8)    //Size of the method(verb)/status field
#define HEEP_APPHDR_REQUEST_RESPONSE_ID_SIZE_BITS (8)    //Size of the request/response ID field

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

typedef struct
{
   uint16_t rValLen;
   struct
   {
     uint8_t rValTypecast : 4;
     uint8_t powerOfTen   : 3;
     uint8_t reserved     : 1;
   };
} OR_PM_Attr_t;

/* Application layer header fields */

typedef struct
{
   uint8_t TransactionType;
   uint8_t TransactionContext;
   uint8_t Resource;
   union
   {
      uint8_t Method_Status; // Original Field Name
      uint8_t Method;        // For a Request  this is Method
      uint8_t Status;        // For a Response this is Status
   };
   union
   {
      uint8_t Req_Resp_ID;   // Original Field Name
      uint8_t ReqID;         // For a Request  this is Request Id
      uint8_t RspID;         // For a Response this is Response Id
   };
   uint8_t qos;
   uint8_t appSecurityAuthMode;  /* security mode   */
   MAC_dataConfCallback callback; //Callback function - used for transmit

} HEEP_APPHDR_t, *pHEEP_APPHDR_t;

typedef struct
{
   uint8_t TransactionType;
   uint8_t Resource;
   uint8_t Method_Status;
   uint8_t Req_Resp_ID;
} HEEP_HDR_t;

/* FILE VARIABLE DEFINITIONS */

/* GLOBAL FUNCTION PROTOTYPES */

#ifdef ERROR_CODES_H_
#ifdef OS_aclara_H
HEEP_UTIL_GLOBALS returnStatus_t HEEP_MSG_Tx (HEEP_APPHDR_t const *, buffer_t const *);
#endif
HEEP_UTIL_GLOBALS returnStatus_t HEEP_MSG_TxHeepHdrOnly (HEEP_APPHDR_t const *heepHdr);
#endif
HEEP_UTIL_GLOBALS bool HEEP_UTIL_unpackHeader (NWK_DataInd_t *indication, uint16_t *bitNo, HEEP_APPHDR_t *appHdr);
HEEP_UTIL_GLOBALS uint16_t HEEP_GetNextReadingInfo(void *payloadBuf, uint16_t *bitNo, uint16_t *readingValNumBytes,
                                                        uint8_t *readingValTypecast, int8_t *readingValPow10,
                                                        uint8_t *readingFlags);
HEEP_UTIL_GLOBALS uint8_t HEEP_getPowerOf10Code(uint8_t ch, int64_t *val);
HEEP_UTIL_GLOBALS uint8_t HEEP_getMinByteNeeded( int64_t val, ReadingsValueTypecast typecast, uint16_t valueSizeInBytes );
HEEP_UTIL_GLOBALS void    HEEP_initHeader(HEEP_APPHDR_t *hdr);
HEEP_UTIL_GLOBALS void OR_PM_MsgHandler(HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length);
HEEP_UTIL_GLOBALS returnStatus_t HEEP_util_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t NWK_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t DTLS_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr);
returnStatus_t MAC_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t PHY_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t HMC_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
HEEP_UTIL_GLOBALS void HEEP_setEnableOTATest( bool enabledStatus);
HEEP_UTIL_GLOBALS bool HEEP_getEnableOTATest( void );

#if ( ANSI_SECURITY == 1 )
returnStatus_t HEEP_util_passordPort0Master_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t HEEP_util_passordPort0_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
#endif

struct readings_s
{
   uint16_t size;    // Current size of the data
   uint8_t  rtype;   // 0 = with time stamp
                     // 1 = w/o  time stamp
   uint8_t  *pData;  // Pointer to the beginning of the buffer
};


#define MAX_UI_LIST_ELEMENTS 32
struct ui_list_s
{
   uint8_t  num_elements;
   uint16_t data[MAX_UI_LIST_ELEMENTS];
};

#if 0 // TODO: RA6E1 - Heep support
void HEEP_Put_Boolean( struct readings_s *p, meterReadingType ReadingType, uint8_t value);
void HEEP_Put_HexBinary ( struct readings_s *p, meterReadingType ReadingType,  uint8_t const Data[], uint16_t  Size);
void HEEP_Put_DateTimeValue( struct readings_s *p, meterReadingType   ReadingType,TIMESTAMP_t const *DateTime);
void HEEP_Put_U8(  struct readings_s *p, meterReadingType ReadingType, uint8_t  u8_value);
void HEEP_Put_U16( struct readings_s *p, meterReadingType ReadingType, uint16_t value, uint8_t powerOfTen);
void HEEP_Put_I16( struct readings_s *p, meterReadingType ReadingType, int16_t value, uint8_t powerOfTen);
void HEEP_Put_U32( struct readings_s *p, meterReadingType ReadingType, uint32_t u32_value);
void HEEP_Put_U64( struct readings_s *p, meterReadingType ReadingType, uint64_t u64_value);
void HEEP_Add_ReadingType(struct readings_s *p,  meterReadingType ReadingType);
void HEEP_Put_ui_list(struct readings_s *p, uint16_t ReadingType, struct ui_list_s const *ui_list);
#endif
void HEEP_ReadingsInit(
   struct readings_s *readings,
   uint8_t  rtype,
   uint8_t  *buffer);

// todo: 1/18/2016 4:18:10 PM [BAH] - These should be somewhere else!
uint64_t htonll(uint64_t value);
uint64_t ntohll(uint64_t value);
#if ( MAC_LINK_PARAMETERS == 1 )
uint8_t  HEEP_scaleNumber( double Input, int16_t InputLow, int16_t InputHigh );
float    HEEP_UnscaleNumber( uint8_t ScaledNum, int16_t InputLow, int16_t InputHigh );
#endif
#undef HEEP_UTIL_GLOBALS

#endif

