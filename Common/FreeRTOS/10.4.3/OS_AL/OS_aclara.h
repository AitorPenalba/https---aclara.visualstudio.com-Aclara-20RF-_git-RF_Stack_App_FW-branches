/******************************************************************************
 *
 * Filename: OS_aclara.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2012-2022 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
#ifndef OS_aclara_H
#define OS_aclara_H

#ifndef features_H
#warning "Requires features.h"
#endif

#if ( RTOS_SELECTION == MQX_RTOS )
/* INCLUDE FILES */
/* General rules state that include files should not include other include files
   this rule is being broken to simplify the interface layer for the MQX specific
   OS structures */
/*lint -efile(537,mqx.h,mutex.h,lwmsgq.h)*/ /* Remove the Repeated include file errors */
#ifndef __BOOTLOADER
#ifndef __mqx_h__
#include <mqx.h>
#endif
#ifndef __lwevent_h__
#include <lwevent.h>
#endif
#ifndef __mutex_h__
#include <mutex.h>
#endif
#ifndef __lwmsq_h__
#include <lwmsgq.h>
#endif

#endif // #ifndef __BOOTLOADER

#elif( RTOS_SELECTION == FREE_RTOS )
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif  // RTOS_SELECTION
/* #DEFINE DEFINITIONS */
#define OS_WAIT_FOREVER 0xFFFFFFFF

#define FIVE_MSEC   (5)
#define TEN_MSEC    (10)
#define FIFTY_MSEC  (50)
#define TENTH_SEC   (100)
#define QTR_SEC     (250)
#define HALF_SEC    (500)
#define ONE_SEC     (1000)
#define ONE_MIN     (ONE_SEC * 60)
#define FIVE_MIN    (ONE_MIN * 5)
#define ONE_HOUR    (ONE_MIN * 60)
#define ONE_DAY     (ONE_HOUR * 24)
#define ONE_WEEK    (ONE_DAY * 7)

#ifndef __BOOTLOADER
#define OS_EVENT_ID 200

#if 0 //( RTOS_SELECTION == MQX_RTOS ) /* MQX */
/* MACRO DEFINITIONS */
#define OS_Get_OsVersion() (_mqx_version)
#define OS_Get_OsGenRevision() (_mqx_generic_revision)
#define OS_Get_OsLibDate() (_mqx_date)
#endif

#define OS_INT_disable()                                 _int_disable()
#define OS_INT_enable()                                  _int_enable()

#define OS_EVNT_Set(EventHandle, EventMask)              OS_EVNT_SET(EventHandle, EventMask, __FILE__, __LINE__)
#define OS_EVNT_Wait(EventHandle, EventMask, WaitForAll, Timeout) OS_EVNT_WAIT(EventHandle, EventMask, WaitForAll, Timeout, __FILE__, __LINE__)

#define OS_MSGQ_Post(MsgqHandle, MessageData)            OS_MSGQ_POST(MsgqHandle, MessageData, (bool)true, __FILE__, __LINE__)
#define OS_MSGQ_Pend(MsgqHandle, MessageData, TimeoutMs) OS_MSGQ_PEND(MsgqHandle, MessageData, TimeoutMs, (bool)true, __FILE__, __LINE__)

#define OS_MUTEX_Lock(MutexHandle)                       OS_MUTEX_LOCK(MutexHandle, __FILE__, __LINE__)
#define OS_MUTEX_Unlock(MutexHandle)                     OS_MUTEX_UNLOCK(MutexHandle, __FILE__, __LINE__)

#define OS_QUEUE_Enqueue(QueueHandle, QueueElement)      OS_QUEUE_ENQUEUE(QueueHandle, QueueElement, __FILE__, __LINE__)

#define OS_SEM_Post(SemHandle)                           OS_SEM_POST(SemHandle, __FILE__, __LINE__)
#define OS_SEM_Pend(SemHandle, TimeoutMs)                OS_SEM_PEND(SemHandle, TimeoutMs, __FILE__, __LINE__)

#if ( RTOS_SELECTION == MQX_RTOS ) /* MQX */
/* TYPE DEFINITIONS */
typedef LWEVENT_STRUCT        OS_EVNT_Obj, *OS_EVNT_Handle;
typedef LWMSGQ_STRUCT         OS_MBOX_Handle;
typedef MUTEX_STRUCT          OS_MUTEX_Obj, *OS_MUTEX_Handle;
typedef QUEUE_STRUCT          OS_QUEUE_Obj, *OS_QUEUE_Handle;
typedef LWSEM_STRUCT          OS_SEM_Obj, *OS_SEM_Handle;
typedef MQX_TICK_STRUCT       OS_TICK_Struct;
typedef _task_id              OS_TASK_id;

typedef struct
{
   QUEUE_ELEMENT_STRUCT queue;
   /* The following variables are required either to provide better error dectection or */
   /* belong to buffer_t but are put here for better memory packing */
   uint16_t  dataLen;      /**< User filled - # of bytes in data[] (initialized with requested buf size) */
   struct {
      uint8_t isFree:   1; /**< buffer was freed through BM_free */
      uint8_t isStatic: 1; /**< buffer was statically allocated */
      uint8_t inQueue:  2; /**< used to detect msg is queued. Used as a counter */
//      uint8_t checksum: 4; /**< used to help with validation */
   } flag;
   uint8_t bufPool       ; /**< identifies source buffer pool */
} OS_QUEUE_Element;

typedef struct
{
   OS_QUEUE_Obj MSGQ_QueueObj;
   OS_SEM_Obj   MSGQ_SemObj;
} OS_MSGQ_Obj, *OS_MSGQ_Handle;

typedef struct
{
   OS_MSGQ_Obj *pSrcHandle;  /* Source handle of the message, a response should be sent to this handle */
   uint16_t    cmdCat;
   int         data[];       /* payload to the destination. */
}msgQueueSrc_t;
#warning "Nooo"
#endif  // ( RTOS_SELECTION == MQX_RTOS ) /* MQX */


#define OS_MSGQ_Create(arg)
#define OS_TICK_Struct
#define OS_TICK_Sleep
#define OS_TASK_Summary

#if ( RTOS_SELECTION == MQX_RTOS )
#define taskParameter         uint32_t Arg0
#define OS_TASK_Template_t    TASK_TEMPLATE_STRUCT

#elif ( RTOS_SELECTION == FREE_RTOS )
#define taskParameter   void* pvParameters
typedef SemaphoreHandle_t     OS_SEM_Obj, OS_MUTEX_Obj, *OS_SEM_Handle, *OS_MUTEX_Handle;
typedef void (* TASK_FPTR)(void *);

typedef struct
{
   uint32_t         TASK_TEMPLATE_INDEX;  /* TODO: RA6: This should be an enum*/

   TASK_FPTR        pvTaskCode;

   size_t           usStackDepth; /* The number of words (not Bytes!)*/

   UBaseType_t      uxPriority;

   char             *pcName;

   uint32_t         TASK_ATTRIBUTES;

   uint32_t         *pvParameters;

   TaskHandle_t     *pxCreatedTask;
} OS_TASK_Template_t, * pOS_TASK_Template_t;
#endif


//#endif //#if ( RTOS_SELECTION == MQX_RTOS )
typedef enum
{
  eSYSFMT_NULL = ((uint8_t)0),
  eSYSFMT_TIME,
  eSYSFMT_DFW,
  eSYSFMT_CDL,
  eSYSFMT_MFGP_CFG,
  eSYSFMT_MFGP_DATA_LAYER,
  eSYSFMT_MFGP_DATA_CMDLINE,
  eSYSFMT_MFGP_DATA_TERMINAL,
  eSYSFMT_MFGP_DFW_INIT,
  eSYSFMT_MFGP_DFW_CANCEL,
  eSYSFMT_MFGP_DFW_DL,
  eSYSFMT_MFGP_DFW_APPLY,
  eSYSFMT_MFGP_DFW_VERIFY,
  eSYSFMT_MFGP_DFW_INIT_RESP,
  eSYSFMT_MFGP_DFW_VERIFY_RESP,
  eSYSFMT_MFGP_DFW_APPLY_RESP,
  eSYSFMT_MFGP_DFW_CANCEL_RESP,
  eSYSFMT_MFGP_DEBUG_DATA,
  eSYSFMT_MFGP_DEBUG_MAC,
  eSYSFMT_MFGP_DEBUG_MEMREAD,
  eSYSFMT_MFGP_DEBUG_TIME,
  eSYSFMT_MFGP_DEBUG_VER,

  eSYSFMT_STACK_DATA_CONF,

  eSYSFMT_NWK_REQUEST,
  eSYSFMT_NWK_CONFIRM,
  eSYSFMT_NWK_INDICATION,

  eSYSFMT_MAC_REQUEST,
  eSYSFMT_MAC_CONFIRM,
  eSYSFMT_MAC_INDICATION,

  eSYSFMT_PHY_REQUEST,
  eSYSFMT_PHY_CONFIRM,
  eSYSFMT_PHY_INDICATION,

  eSYSFMT_SM_REQUEST,
  eSYSFMT_SM_CONFIRM,
  eSYSFMT_SM_INDICATION,

  eSYSFMT_VISUAL_INDICATION,
  eSYSFMT_TIME_DIVERSITY,

  eSYSFMT_B2B_FRAME_ARRIVAL,
  eSYSFMT_B2B_TX_REQ,

  eSYSFMT_DMD_TIMEOUT,

  eSYSFMT_EVL_ALARM_MSG,
  eSYSFMT_EVL_ALARM_LOG_RESET_EVENT
}eSysFormat_t;

/* CONSTANTS */
/* This list of const needs to match the list in Task.c   These values are used for Get/Set Task Priority */
#if 0
extern const char pTskName_Strt[];
extern const char pTskName_Sm[];
extern const char pTskName_Phy[];
extern const char pTskName_Tmr[];
extern const char pTskName_Time[];
extern const char pTskName_BuAm[];
extern const char pTskName_BubLp[];
extern const char pTskName_Mac[];
extern const char pTskName_Dbg[];
extern const char pTskName_Par[];
extern const char pTskName_Print[];
#endif

extern const char pTskName_Pwr[];
extern const char pTskName_PhyTx[];
extern const char pTskName_PhyRx[];
#if ( SIMULATE_POWER_DOWN == 1 )
extern const char pTskName_Mfg[];
#endif
#if ( ACLARA_DA == 1 )
extern const char pTskName_MfgUartRecv[];
#endif
extern const char pTskName_MfgUartCmd[];
extern const char pTskName_MfgUartOpto[];
extern const char pTskName_Alrm[];
extern const char pTskName_Manuf[];
extern const char pTskName_Hmc[];
extern const char pTskName_Id[];
extern const char pTskName_Dfw[];
extern const char pTskName_Test[];
extern const char pTskName_PhyMfg[];
extern const char pTskName_Nwk[];
extern const char pTskName_Idle[];
extern const char pTskName_Sleep[];

/* FILE VARIABLE DEFINITIONS */
#if 0
/* FUNCTION PROTOTYPES */
bool OS_EVNT_Create ( OS_EVNT_Handle EventHandle );
void OS_EVNT_SET ( OS_EVNT_Handle EventHandle, uint32_t EventMask, char *file, int line );
uint32_t OS_EVNT_WAIT ( OS_EVNT_Handle EventHandle, uint32_t EventMask, bool WaitForAll, uint32_t Timeout, char *file, int line );

bool OS_MSGQ_Create ( OS_MSGQ_Handle MsgqHandle );
void OS_MSGQ_POST ( OS_MSGQ_Handle MsgqHandle, void *MessageData, bool ErrorCheck, char *file, int line );
bool OS_MSGQ_PEND ( OS_MSGQ_Handle MsgqHandle, void **MessageData, uint32_t TimeoutMs, bool ErrorCheck, char *file, int line );

bool OS_QUEUE_Create ( OS_QUEUE_Handle QueueHandle );
void OS_QUEUE_ENQUEUE ( OS_QUEUE_Handle QueueHandle, void *QueueElement, char *file, int line );
void *OS_QUEUE_Dequeue ( OS_QUEUE_Handle QueueHandle );
bool OS_QUEUE_Insert ( OS_QUEUE_Handle QueueHandle, void *QueuePosition, void *QueueElement );
void OS_QUEUE_Remove ( OS_QUEUE_Handle QueueHandle, void *QueueElement );
uint16_t OS_QUEUE_NumElements ( OS_QUEUE_Handle QueueHandle );
void *OS_QUEUE_Head ( OS_QUEUE_Handle QueueHandle );
void *OS_QUEUE_Next ( OS_QUEUE_Handle QueueHandle, void *QueueElement );
#endif

bool OS_MUTEX_Create ( OS_MUTEX_Handle MutexHandle );
void OS_MUTEX_LOCK ( OS_MUTEX_Handle MutexHandle, char *file, int line );
void OS_MUTEX_UNLOCK ( OS_MUTEX_Handle MutexHandle, char *file, int line );

bool OS_SEM_Create ( OS_SEM_Handle SemHandle );
void OS_SEM_POST ( OS_SEM_Handle SemHandle, char *file, int line );
bool OS_SEM_PEND ( OS_SEM_Handle SemHandle, uint32_t TimeoutMs, char *file, int line );
void OS_SEM_Reset ( OS_SEM_Handle SemHandle );

#if ( RTOS_SELECTION == MQX_RTOS ) /* MQX */
void OS_TASK_Create_Idle ( void );
#endif
void OS_TASK_Create_All ( bool initSuccess );
void OS_TASK_Create_STRT( void );
#if 0
uint32_t OS_TASK_Get_Priority ( char const *pTaskName );
uint32_t OS_TASK_Set_Priority ( char const *pTaskName, uint32_t NewPriority );
void OS_TASK_Sleep ( uint32_t MSec );
void OS_TASK_Exit ( void );
void OS_TASK_ExitId ( char const *pTaskName );
OS_TASK_id OS_TASK_GetId (void);
bool OS_TASK_IsCurrentTask ( char const *pTaskName );
uint32_t OS_TASK_UpdateCpuLoad ( void );
void OS_TASK_GetCpuLoad ( uint32_t taskIdx, uint32_t * CPULoad );
void OS_TASK_Summary ( bool safePrint );

void OS_TICK_Get_CurrentElapsedTicks ( OS_TICK_Struct *TickValue );
uint32_t OS_TICK_Get_ElapsedMilliseconds ( void );
uint32_t OS_TICK_Get_Diff_InMicroseconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue );
uint32_t OS_TICK_Get_Diff_InNanoseconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue );
bool OS_TICK_Is_FutureTime_Greater ( OS_TICK_Struct *CurrTickValue, OS_TICK_Struct *FutureTickValue );
void OS_TICK_Sleep ( OS_TICK_Struct *TickValue, uint32_t TimeDelay );
#endif
/* FUNCTION DEFINITIONS */
#endif   /* __BOOTLOADER */

#endif /* this must be the last line of the file */
#if (TM_MUTEX == 1)
void OS_MUTEX_Test( void );
#endif
#if (TM_SEMAPHORE == 1)
void OS_SEM_TestPost( void );
void OS_SEM_TestCreate ( void );
bool OS_SEM_TestPend( void );
#endif