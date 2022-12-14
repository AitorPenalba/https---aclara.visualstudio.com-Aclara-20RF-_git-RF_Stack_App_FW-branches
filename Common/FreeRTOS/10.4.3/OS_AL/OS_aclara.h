/******************************************************************************
 *
 * Filename: OS_aclara.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
  A product of
  Aclara Technologies LLC
  Confidential and Proprietary
  Copyright 2012-2022 Aclara. All Rights Reserved.

  PROPRIETARY NOTICE
  The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
  (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
  authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
  be used or copied only in accordance with the terms of such license.
 *****************************************************************************/
#ifndef OS_aclara_H
#define OS_aclara_H

#include "error_codes.h"

#ifndef CFG_HAL_H
#warning "Requires cfg_hal.h configuration!!!"
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
#include "event_groups.h"
#endif  // RTOS_SELECTION
/* #DEFINE DEFINITIONS */
#define OS_WAIT_FOREVER 0xFFFFFFFF


#define TRUE        ( bool ) 1
#define FALSE       ( bool ) 0

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
#define DEFAULT_NUM_ITEMS_MSGQ 10

#define OS_MAX_TASK_PRIORITY    (9) // highest priority value allowed by MQX, value cannot be less than 9 per AN3905.pdf
#define OS_MIN_TASK_PRIORITY   (39) // Aclara Idle Task will be at this level

#if OS_MAX_TASK_PRIORITY <= 8
#error  Highest allowable task priority is nine
#endif
#if ( RTOS_SELECTION == MQX_RTOS )
#define AUTO_START_TASK    MQX_AUTO_START_TASK
#elif ( RTOS_SELECTION == FREE_RTOS )
#define AUTO_START_TASK    0x01  /* Defining for the FreeRTOS */

#if OS_MIN_TASK_PRIORITY >= configMAX_PRIORITIES
#error  Task minimum must be less than configMAX_PRIORITIES in FreeRTOSConfig.h
#endif
#endif


#ifndef __BOOTLOADER
#define OS_EVENT_ID 200

#if ( RTOS_SELECTION == MQX_RTOS ) /* MQX */
/* MACRO DEFINITIONS */
#define OS_Get_OsVersion() (_mqx_version)
#define OS_Get_OsGenRevision() (_mqx_generic_revision)
#define OS_Get_OsLibDate() (_mqx_date)
#endif

/* the following macro returns the task priority immediately above the IDL task. MQX
   uses an Aclara IDLE task above MQX IDL so will be adjusted */
#if ( RTOS_SELECTION == MQX_RTOS )
#define LOWEST_TASK_PRIORITY_ABOVE_IDLE() ( OS_MIN_TASK_PRIORITY - 1 )
#elif ( RTOS_SELECTION == FREE_RTOS )
#define LOWEST_TASK_PRIORITY_ABOVE_IDLE() ( OS_MIN_TASK_PRIORITY )
#endif

/* the following macro converts the FREE_RTOS task priority to descending order value, used by MQX */
#if ( RTOS_SELECTION == FREE_RTOS )
#define FREE_RTOS_TASK_PRIORITY_CONVERT(x)  (configMAX_PRIORITIES - x)
#endif

#if ( RTOS_SELECTION == MQX_RTOS )
/* The MQX version of interrupt disable/enable will disable the RTOS tick and ALL peripheral interrupts.
   Any Fault interrupt can still occur.
   The SVCall interrupt is active, but the scheduler will not allow another task to run (PendSV priority lower than max allowed).
      SVCall only occurs if an MQX call is made
   If the active task blocks while interrupts are disabled, the state of the interrupts (disabled or enabled) depends on the
      interrupt-disabled state of the next task MQX makes ready.
         Bottom line: Do not allow task to block while interrupts are disabled.
   The RTOS keeps track of nested disable calls and only enables after all have been completed.
   NOTE:
      Use of PRIMASK is discouraged - this disables ALL interrupts and is frequently called in MQX without regard to it being
      already disabled (will re-enable interrupts without warning).
      Bottom line: When using PRIMASK do not make any MQX calls.
*/
#define OS_INT_disable()                                     _int_disable()
#define OS_INT_enable()                                       _int_enable()
#define OS_INT_ISR_disable()                                 _int_disable()
#define OS_INT_ISR_enable(x)                                  _int_enable()
#define OS_TASK_Yield()                                      _sched_yield()
#elif ( RTOS_SELECTION == FREE_RTOS )
#define OS_INT_disable()                               taskENTER_CRITICAL()
#define OS_INT_enable()                                 taskEXIT_CRITICAL()
#define OS_INT_ISR_disable()                  taskENTER_CRITICAL_FROM_ISR() // this returns a value to be used for the enable
#define OS_INT_ISR_enable(x)                   taskEXIT_CRITICAL_FROM_ISR(x)
#define OS_TASK_Yield()                                         taskYIELD()
#endif

#define OS_EVNT_Set(EventHandle, EventMask)              OS_EVNT_SET(EventHandle, EventMask, __FILE__, __LINE__)
#define OS_EVNT_Set_from_ISR(EventHandle, EventMask)     OS_EVNT_SET_from_ISR(EventHandle, EventMask, __FILE__, __LINE__)
#define OS_EVNT_Wait(EventHandle, EventMask, WaitForAll, Timeout) OS_EVNT_WAIT(EventHandle, EventMask, WaitForAll, Timeout, __FILE__, __LINE__)
#define OS_EVNT_Clear(EventHandle, EventMask)            OS_EVNT_CLEAR(EventHandle, EventMask, __FILE__, __LINE__)
#if ( ( BM_USE_KERNEL_AWARE_DEBUGGING == 1 ) && ( RTOS_SELECTION == FREE_RTOS ) )
#define OS_MSGQ_Create( MsgqHandle, numItems, name )     OS_MSGQ_CREATE( MsgqHandle, numItems, name )
#else
#define OS_MSGQ_Create( MsgqHandle, numItems, name )     OS_MSGQ_CREATE( MsgqHandle, numItems)
#endif
#if ( ( BM_USE_KERNEL_AWARE_DEBUGGING == 1 ) && ( RTOS_SELECTION == FREE_RTOS ) )
#define OS_QUEUE_Create(QueueHandle, QueueLength, name)  OS_QUEUE_CREATE( QueueHandle, QueueLength, name )
#else
#define OS_QUEUE_Create(QueueHandle, QueueLength, name)  OS_QUEUE_CREATE( QueueHandle, QueueLength )
#endif
#define OS_MSGQ_Post(MsgqHandle, MessageData)            OS_MSGQ_POST(MsgqHandle, MessageData, (bool)true, __FILE__, __LINE__)
#define OS_MSGQ_Post_RetStatus(MsgqHandle, MessageData)  OS_MSGQ_POST_RetStatus(MsgqHandle, MessageData, (bool)true, __FILE__, __LINE__)
#define OS_MSGQ_Pend(MsgqHandle, MessageData, TimeoutMs) OS_MSGQ_PEND(MsgqHandle, MessageData, TimeoutMs, (bool)true, __FILE__, __LINE__)

#define OS_MUTEX_Lock(MutexHandle)                       OS_MUTEX_LOCK(MutexHandle, __FILE__, __LINE__)
#define OS_MUTEX_Unlock(MutexHandle)                     OS_MUTEX_UNLOCK(MutexHandle, __FILE__, __LINE__)

#define OS_QUEUE_Enqueue(QueueHandle, QueueElement)      OS_QUEUE_ENQUEUE(QueueHandle, QueueElement, __FILE__, __LINE__)

#define OS_SEM_Post(SemHandle)                           OS_SEM_POST(SemHandle, __FILE__, __LINE__)
#define OS_SEM_Pend(SemHandle, TimeoutMs)                OS_SEM_PEND(SemHandle, TimeoutMs, __FILE__, __LINE__)

#define OS_SEM_Post_fromISR(SemHandle)                   OS_SEM_POST_fromISR(SemHandle, __FILE__, __LINE__)
#define OS_SEM_Pend_fromISR(SemHandle)                   OS_SEM_PEND_fromISR(SemHandle, __FILE__, __LINE__)
#define OS_SEM_Post_fromISR_retStatus(SemHandle)         OS_SEM_POST_fromISR_retStatus(SemHandle, __FILE__, __LINE__)

// Mapping HTONS and HTONL with Byte_Swap internal APIs
#if ( RTOS_SELECTION == FREE_RTOS ) /* FREE_RTOS */
#define htonl(value)    htonx_FreeRTOS(value, 4)
#define htons(value)    htonx_FreeRTOS(value, 2)
#endif

#if ( RTOS_SELECTION == FREE_RTOS ) /* FREE_RTOS */
#define malloc(value)   pvPortMalloc(value)
#define free(value)     vPortFree(value)
#endif

#if ( RTOS_SELECTION == MQX_RTOS ) /* MQX */
/* TYPE DEFINITIONS */
typedef LWEVENT_STRUCT        OS_EVNT_Obj, *OS_EVNT_Handle;
typedef LWMSGQ_STRUCT         OS_MBOX_Handle;
typedef MUTEX_STRUCT          OS_MUTEX_Obj, *OS_MUTEX_Handle;
typedef QUEUE_STRUCT          OS_QUEUE_Obj, *OS_QUEUE_Handle;
typedef LWSEM_STRUCT          OS_SEM_Obj, *OS_SEM_Handle;
typedef QUEUE_STRUCT          OS_List_Obj, *OS_List_Handle;
typedef MQX_TICK_STRUCT       OS_TICK_Struct;
typedef _task_id              OS_TASK_id;
typedef QUEUE_ELEMENT_STRUCT  OS_LINKED_LIST_STRUCT;

typedef struct
{
   OS_LINKED_LIST_STRUCT queue;
   /* The following variables are required either to provide better error detection or */
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
#endif  // ( RTOS_SELECTION == MQX_RTOS ) /* MQX */

typedef struct
{
   uint32_t   HW_TICKS;    // CYCCNT value

   uint32_t   tickCount;   // Tick count from xTaskGetTickCount();

   uint32_t   xNumOfOverflows; // Tick count overflow
} OS_TICK_Struct, * OS_TICK_Struct_Ptr;


#if ( RTOS_SELECTION == MQX_RTOS )
#define taskParameter         uint32_t Arg0
#define OS_TASK_Template_t    TASK_TEMPLATE_STRUCT
typedef _task_id              OS_TASK_id;
#elif ( RTOS_SELECTION == FREE_RTOS )
typedef UBaseType_t           OS_TASK_id;
#define taskParameter   void* pvParameters
typedef SemaphoreHandle_t     OS_SEM_Obj, OS_MUTEX_Obj, *OS_SEM_Handle, *OS_MUTEX_Handle;
typedef QueueHandle_t         OS_QUEUE_Obj, *OS_QUEUE_Handle;
typedef EventGroupHandle_t    OS_EVNT_Obj,  *OS_EVNT_Handle;
typedef struct OS_Linked_List_Element
{
   struct OS_Linked_List_Element *NEXT;
   struct OS_Linked_List_Element *PREV;
} OS_Linked_List_Element, *OS_Linked_List_Element_Ptr;

typedef OS_Linked_List_Element OS_LINKED_LIST_STRUCT;

typedef struct
{
  OS_LINKED_LIST_STRUCT queue;
   uint16_t  dataLen;      /**< User filled - # of bytes in data[] (initialized with requested buf size) */
   struct {
      uint8_t isFree:   1; /**< buffer was freed through BM_free */
      uint8_t isStatic: 1; /**< buffer was statically allocated */
      uint8_t inQueue:  2; /**< used to detect msg is queued. Used as a counter */
//      uint8_t checksum: 4; /**< used to help with validation */
   } flag;
   uint8_t bufPool       ; /**< identifies source buffer pool */
} OS_QUEUE_Element, *OS_QUEUE_Element_Handle;

typedef struct
{
  OS_Linked_List_Element  *Head;
  OS_Linked_List_Element  *Tail;
  uint32_t                size;
} OS_List_Obj, *OS_List_Handle;

#define DEFAULT_NUM_QUEUE_ITEMS          10
#define QUEUE_ITEM_SIZE sizeof( OS_QUEUE_Element_Handle )
typedef struct
{
   OS_QUEUE_Obj MSGQ_QueueObj;
   OS_SEM_Obj MSGQ_SemObj;
} OS_MSGQ_Obj, *OS_MSGQ_Handle;

typedef void (* TASK_FPTR)(void *);
typedef struct
{
   uint32_t         TASK_TEMPLATE_INDEX;

   TASK_FPTR        pvTaskCode;

   size_t           usStackDepth; /* The number of words (not Bytes!)*/

   UBaseType_t      uxPriority;

   char             *pcName;

   uint32_t         TASK_ATTRIBUTES;

   uint32_t         *pvParameters;

   uint32_t         defaultTimeSlice; /* added to accommodate MQX and a universal template list between RTOS */

   TaskHandle_t     *pxCreatedTask;
} OS_TASK_Template_t, * pOS_TASK_Template_t;

#endif


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

// unify the return value each RTOS returns as each task is created
#if ( RTOS_SELECTION == MQX_RTOS )
typedef _task_id taskCreateReturnValue_t;
#elif ( RTOS_SELECTION == FREE_RTOS )
typedef UBaseType_t taskCreateReturnValue_t;
#endif

/* CONSTANTS */
/* This list of const needs to match the list in Task.c   These values are used for Get/Set Task Priority */
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

extern const char pTskName_Pwr[];
extern const char pTskName_PhyTx[];
extern const char pTskName_PhyRx[];
#if ( SIMULATE_POWER_DOWN == 1 )
extern const char pTskName_Mfg[];
#endif
#if ( ACLARA_DA == 1 ) // TODO: RA6E1 Bob: This reference is required by DBG_CommandLine in the DBG_CommandLine_SimulatePowerDown function, not just for ACLARA_DA
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


/* FUNCTION PROTOTYPES */
bool     OS_EVNT_Create ( OS_EVNT_Handle EventHandle );
void     OS_EVNT_SET ( OS_EVNT_Handle EventHandle, uint32_t EventMask, char *file, int line );
void OS_EVNT_SET_from_ISR ( OS_EVNT_Handle EventHandle, uint32_t EventMask, char *file, int line );
void OS_EVNT_DELETE ( OS_EVNT_Obj EventObject );

uint32_t OS_EVNT_WAIT ( OS_EVNT_Handle EventHandle, uint32_t EventMask, bool WaitForAll, uint32_t Timeout, char *file, int line );
void     OS_EVNT_CLEAR ( OS_EVNT_Handle EventHandle, uint32_t EventMask, char *file, int line );

bool OS_QUEUE_Insert ( OS_QUEUE_Handle QueueHandle, void *QueuePosition, void *QueueElement );
void OS_QUEUE_Remove ( OS_QUEUE_Handle QueueHandle, void *QueueElement );
void *OS_QUEUE_Next ( OS_QUEUE_Handle QueueHandle, void *QueueElement );
#if ( BM_USE_KERNEL_AWARE_DEBUGGING == 1 )
bool OS_MSGQ_CREATE ( OS_MSGQ_Handle MsgqHandle, uint32_t NumMessages, const char *name);
#else
bool OS_MSGQ_CREATE ( OS_MSGQ_Handle MsgqHandle, uint32_t NumMessages);
#endif
void OS_MSGQ_POST ( OS_MSGQ_Handle MsgqHandle, void *MessageData, bool ErrorCheck, char *file, int line );
returnStatus_t OS_MSGQ_POST_RetStatus ( OS_MSGQ_Handle MsgqHandle, void *MessageData, bool ErrorCheck, char *file, int line );
bool OS_MSGQ_PEND ( OS_MSGQ_Handle MsgqHandle, void **MessageData, uint32_t TimeoutMs, bool ErrorCheck, char *file, int line );

#if ( BM_USE_KERNEL_AWARE_DEBUGGING == 1 )
bool OS_QUEUE_CREATE ( OS_QUEUE_Handle QueueHandle, uint32_t QueueLength, const char *name );
#else
bool OS_QUEUE_CREATE ( OS_QUEUE_Handle QueueHandle, uint32_t QueueLength );
#endif
void OS_QUEUE_ENQUEUE ( OS_QUEUE_Handle QueueHandle, void *QueueElement, char *file, int line );
returnStatus_t OS_QUEUE_ENQUEUE_RetStatus ( OS_QUEUE_Handle QueueHandle, void *QueueElement, char *file, int line );
void *OS_QUEUE_Dequeue ( OS_QUEUE_Handle QueueHandle );
uint16_t OS_QUEUE_NumElements ( OS_QUEUE_Handle QueueHandle );
void *OS_QUEUE_Head ( OS_QUEUE_Handle QueueHandle );

bool OS_LINKEDLIST_Create ( OS_List_Handle listHandle );
void OS_LINKEDLIST_Enqueue( OS_List_Handle list, void *listElement);
bool OS_LINKEDLIST_Insert (OS_List_Handle list,  void *listPosition, void *listElement );
void OS_LINKEDLIST_Remove ( OS_List_Handle list,void *listElement );
void *OS_LINKEDLIST_Next ( OS_List_Handle list, void *listElement );
void *OS_LINKEDLIST_Dequeue  ( OS_List_Handle list );
uint16_t OS_LINKEDLIST_NumElements ( OS_List_Handle list );
void *OS_LINKEDLIST_Head ( OS_List_Handle list );
/* If TM_LINKED_LIST == 0 (release version), the following two functions will be created but empty.  Cannot use conditional compile here */
char *OS_LINKEDLIST_ValidateList ( OS_List_Handle list, char *code );
void OS_LINKEDLIST_DumpList      ( OS_List_Handle list, OS_Linked_List_Element *firstElement ); /* same */

bool OS_MUTEX_Create ( OS_MUTEX_Handle MutexHandle );
void OS_MUTEX_LOCK ( OS_MUTEX_Handle MutexHandle, char *file, int line );
void OS_MUTEX_UNLOCK ( OS_MUTEX_Handle MutexHandle, char *file, int line );

bool OS_SEM_Create ( OS_SEM_Handle SemHandle, uint32_t maxCount );
void OS_SEM_POST ( OS_SEM_Handle SemHandle, char *file, int line );
returnStatus_t OS_SEM_POST_RetStatus ( OS_SEM_Handle SemHandle, char *file, int line );
bool OS_SEM_PEND ( OS_SEM_Handle SemHandle, uint32_t TimeoutMs, char *file, int line );
void OS_SEM_Reset ( OS_SEM_Handle SemHandle );

#if ( RTOS_SELECTION == FREE_RTOS ) /* FREE_RTOS */
void OS_SEM_POST_fromISR ( OS_SEM_Handle SemHandle, char *file, int line );
void OS_SEM_PEND_fromISR ( OS_SEM_Handle SemHandle, char *file, int line );
returnStatus_t OS_SEM_POST_fromISR_retStatus( OS_SEM_Handle SemHandle, char *file, int line );
#endif

#if ( RTOS_SELECTION == FREE_RTOS ) /* FREE_RTOS */
uint32_t htonx_FreeRTOS( uint32_t value, uint8_t numOfBytes );
#endif

#if ( RTOS_SELECTION == MQX_RTOS ) /* MQX */
void OS_TASK_Create_Idle ( void );
#endif
taskCreateReturnValue_t OS_TASK_Create ( OS_TASK_Template_t const *pTaskList );
void                    OS_TASK_Create_All ( bool initSuccess );
void                    OS_TASK_Create_STRT( void );
void                    OS_TASK_Create_PWRLG( void );

uint32_t                OS_TASK_Get_Priority ( char const *pTaskName );
uint32_t                OS_TASK_Set_Priority ( char const *pTaskName, uint32_t NewPriority );
void                    OS_TASK_Sleep ( uint32_t MSec );
void                    OS_TASK_Exit ( void );
void                    OS_TASK_ExitId ( char const *pTaskName );
OS_TASK_id              OS_TASK_GetId (void);
OS_TASK_id              OS_TASK_GetID_fromName ( const char *taskName );
bool                    OS_TASK_IsCurrentTask ( char const *pTaskName );
char *                  OS_TASK_GetTaskName ( void );
uint32_t                OS_TASK_UpdateCpuLoad ( void );
void                    OS_TASK_GetCpuLoad ( OS_TASK_id taskIdx, uint32_t * CPULoad );
void                    OS_TASK_Summary ( bool safePrint );


void     OS_TICK_Get_CurrentElapsedTicks ( OS_TICK_Struct *TickValue );
uint32_t OS_TICK_Get_ElapsedMilliseconds ( void );
uint32_t OS_TICK_Get_Diff_InMicroseconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue );
uint32_t OS_TICK_Get_Diff_InNanoseconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue );
uint32_t OS_TICK_Get_Diff_InMilliseconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue );
uint32_t OS_TICK_Get_Diff_InSeconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue );
uint32_t OS_TICK_Get_Diff_InMinutes ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue );
uint32_t OS_TICK_Get_Diff_InHours ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue );
bool OS_TICK_Is_FutureTime_Greater ( OS_TICK_Struct *CurrTickValue, OS_TICK_Struct *FutureTickValue );
OS_TICK_Struct_Ptr OS_TICK_Add_msec_to_ticks ( OS_TICK_Struct *TickValue, uint32_t TimeDelay );
void OS_TICK_Sleep ( OS_TICK_Struct *TickValue, uint32_t TimeDelay );
/* FUNCTION DEFINITIONS */
#endif   /* __BOOTLOADER */
#if( TM_LINKED_LIST == 2)
void OS_LINKEDLIST_Test(void);
#endif
#if ( ( BM_USE_KERNEL_AWARE_DEBUGGING == 1 ) && ( RTOS_SELECTION == FREE_RTOS ) && ( configQUEUE_REGISTRY_SIZE > 0 ) )
void OS_QUEUE_DumpQueues( bool safePrint );
#endif
#endif /* this must be the last line of the file */
