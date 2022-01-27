/***********************************************************************************************************************
 *
 * Filename: ILC_DRU_DRIVER.h
 *
 * Contents:  #defs, typedefs, and function prototypes for the routines to
 *    support DRU communication needs
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2017 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ kad Created 080207
 *
 **********************************************************************************************************************/

#ifndef ILC_DRU_DRIVER_H
#define ILC_DRU_DRIVER_H

/* ***************************************************************************************************************** */
/* INCLUDE FILES */
#include "OS_aclara.h"
#include "error_codes.h"
#include "time_sys.h"
#include "time_util.h"
#include "MP_SPP.h"
#include "DBG_SerialDebug.h"


/* ***************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ***************************************************************************************************************** */
/* TYPE DEFINITIONS */
typedef enum
{
   eDRU_READ_REGISTER = 0,                      /* Read a DRU Register  */
   eDRU_WRITE_REGISTER,                         /* Write a DRU Register  */
   eDRU_SET_DATE_TIME,                          /* Update DRU Date and Time   */
   eDRU_OUTBOUND_TUNNEL,                        /* Tunnel Message to the DRU */
} druOperation_t;

typedef struct
{
  uint8_t     HwGeneration_u8;
  uint8_t     HwRevision_u8;
}HwVersion_t;

typedef struct
{
   uint8_t     FwMajor_u8;
   uint8_t     FwMinor_u8;
   uint16_t    FwEngBuild_u16;
}FwVersion_t;

typedef struct
{
   uint8_t     RceType_u8;
   uint8_t     RceModel_u8;
}Model_t;

typedef struct
{
   uint32_t    SerialNumber_u32;
   HwVersion_t HwVer;
   FwVersion_t FwVer;
   Model_t     ModNum;
   bool        ValidData_b;
} ILC_DRU_DRIVER_druInfo_t;

typedef union
{
   uint32_t    SerialNumber_u32;
   HwVersion_t HwVer;
   FwVersion_t FwVer;
   Model_t     ModNum;
} ILC_DRU_DRIVER_druRegister_u;

typedef struct
{
   OS_QUEUE_Element             QueueElement;           /* MUST BE 1ST ELEMENT; QueueElement as defined by MQX */
   druOperation_t               druOperation;           /* See druOperation_t definition */
   uint16_t                     druRegister_u16;        /* DRU Register to be read */
   struct
   {
     uint16_t Id;
     uint8_t  Length;
     uint8_t  Data[15];
   }writeReg;
   sysTime_t                    druSysTime;             /* Current DRU's System Time */
   uint8_t                      *pOutMsg_u8;            /* Location of Outbound Message */
   uint8_t                      OutMsgSize_u8;          /* Length of Outbound Message */
   bool                         WaitForInMsg_b;         /* Flag to indicate if DRU Driver shall wait for a DRU Response or not */
   bool                         *pInMsgFlag_b;          /* Flag to indicate if an Inbound Message is available to be transmitted  */
   uint8_t                      *pInMsg_u8;             /* Location of Inbound Message */
   uint8_t                      *pInMsgSize_u8;         /* Location of the Inbound Message Length */
   returnStatus_t               druStatus;              /* Successful or not? */
   ILC_DRU_DRIVER_druRegister_u druData;                /* DRU Register Value read from DRU */
   OS_SEM_Handle                pSem;                   /* Semaphore to post when complete processing the DRU Request */
} ILC_DRU_DRIVER_queue_t;


/* ***************************************************************************************************************** */
/* GLOBAL VARIABLES */
extern OS_QUEUE_Obj ILC_DRU_DRIVER_QueueHandle;
extern uint8_t ILC_DRU_DRIVER_QueueCreated;

extern OS_MUTEX_Obj  ILC_DRU_DRIVER_Mutex_;
extern bool          ILC_DRU_DRIVER_MutexCreated_;


/* ***************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
enum_CIM_QualityCode ILC_DRU_DRIVER_readDruRegister  ( ILC_DRU_DRIVER_druRegister_u *pDest, uint16_t RegisterId_u16, uint32_t TimeOutMs_u32 );
enum_CIM_QualityCode ILC_DRU_DRIVER_writeDruRegister ( uint16_t RegisterId, uint8_t RegisterLen, uint8_t const *RegisterData, uint32_t TimeOutMs_u32 );
enum_CIM_QualityCode ILC_DRU_DRIVER_setDruDateTime ( sysTime_t newTime, uint32_t TimeOutMs_u32 );
enum_CIM_QualityCode ILC_DRU_DRIVER_tunnelOutboundMessage ( uint8_t *pOutboundMsg_u8, uint8_t OutboundMsgSize_u8, uint32_t TimeOutMs_u32, bool WaitForInboundMessage_b,
                                                            bool *pInboundMsgFlag_b, uint8_t *pInboundMsg_u8, uint8_t *pInboundMsgSize_u8 );
returnStatus_t ILC_DRU_DRIVER_Init( void );
void ILC_DRU_DRIVER_Task( uint32_t Arg0 );

#endif

/* End of file */