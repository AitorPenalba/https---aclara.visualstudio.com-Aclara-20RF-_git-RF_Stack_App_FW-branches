/***********************************************************************************************************************
 *
 * Filename: pwr_last_gasp.h
 *
 * Global Designator: PWRLG_
 *
 * Contents: Last gasp message build and transmission.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2018-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author David McGraw
 *
 * $Log$
 *
 ***********************************************************************************************************************
 * Revision History:
 *
 * @version
 * #since
 **********************************************************************************************************************/

#ifndef pwr_last_gasp_H
#define pwr_last_gasp_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "vbat_reg.h"


/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */

#ifdef PWRLG_GLOBALS
   #define PWRLG_EXTERN
#else
   #define PWRLG_EXTERN extern
#endif

// Create a forward reference so mac.h is not required
       struct mac_config_s;
extern struct mac_config_s  *PWRLG_MacConfigHandle(void);

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/** SMC - Peripheral register structure */
/* NOTE: This struct CANNOT exceed the size of RFSYS in the processor */
typedef struct PWRLG_SysRegisterFile
{
   uint32_t timeOutage;                /* System time at initial power failure   */
   uint32_t uMilliseconds;             /* Total outage time based on capacity.   */
   uint16_t uSleepSeconds;             /* Seconds portion of time to sleep until next message.  */
   uint16_t uSleepMilliseconds;        /* Milliseconds portion of time to sleep until next message.  */
   union
   {
      struct
      {
         uint8_t  llwu           : 1;  /* 1-> awakened by the low power timer.   */
         uint8_t  pwrOutage      : 1;  /* 1->outage declared.  */
         uint8_t  sent           : 1;  /* 1->last gasp message attempted.  */
         uint8_t  softwareReset  : 1;  /* Set when PF occurs.  */
         uint8_t  state          : 4;  /* See PWRLG_STATE_xxx below. */
      } bits;
      uint8_t byte;
   } flags;
   struct
   {
      uint8_t sent   :  4; /* Number of LG messages sent */
      uint8_t total  :  4; /* Total Number of LG messages to send. Based on Super Cap at initial power failure. */
   } uMessageCounts;
#if 0
   union
   {
      float    fCapBefore;    /* Super cap voltage at start of power failure. */
      uint32_t pwrUpTime;     /* Time recorded in PWR_TSK_init */
   };
#endif
   uint32_t restorationTime;  /* time when power restoration met  */
   uint16_t totalBackOffTime; /* total random backoff time per message ( Txfailures )  */
   uint16_t curBackOffTime;   /* random backoff time for current message ( Txfailures )   */
   uint16_t txFailures;       /* Number of Tx failures */
   uint16_t prevMsgTime;      /* time taken to send previous message   */
   uint8_t  llwu_f3;
   uint8_t  pad;
   uint8_t  llwu_filt1;
} *PWRLG_SysRegisterFilePtr, PWRLG_SysRegisterFile;


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#if ( MCU_SELECTED  == NXP_K24 )
#define PWRLG_RFSYS_BASE_PTR        ((PWRLG_SysRegisterFilePtr) (void *)RFSYS_BASE_PTR)
#elif ( MCU_SELECTED  == RA6E1 )
/* Note: In RA6E1 we use the Battery Backup Register to store Last Gasp related information */
#define PWRLG_RFSYS_BASE_PTR        ((PWRLG_SysRegisterFilePtr) (void *)(R_SYSTEM->VBTBKR + VBATREG_FILE_SIZE) )
#endif

#define PWRLG_FLAGS()               (PWRLG_RFSYS_BASE_PTR->flags)

#define PWRLG_LLWU()                (PWRLG_FLAGS().bits.llwu)
#define PWRLG_LLWU_SET(x)           (PWRLG_FLAGS().bits.llwu = (x))

#define PWRLG_OUTAGE()              (PWRLG_FLAGS().bits.pwrOutage)
#define PWRLG_OUTAGE_SET(x)         (PWRLG_FLAGS().bits.pwrOutage = (x))

#define PWRLG_SENT()                ((PWRLG_FLAGS().bits.sent)
#define PWRLG_SENT_SET(x)           ( PWRLG_FLAGS().bits.sent = (x) )

/* This flag is set at the start of a power outage by last gasp. It is used to determine if last gasp ended with a
   software reset or a hardware reset. If power is restored before all last gasp messages have been sent, a software
   reset is performed and this bit will remain set. If all last gasp messages have been sent, a hardware reset occurs
   when power is restored and this bit will be cleared. */
#define PWRLG_SOFTWARE_RESET()      (PWRLG_FLAGS().bits.softwareReset)
#define PWRLG_SOFTWARE_RESET_SET(x) (PWRLG_FLAGS().bits.softwareReset = (x))

#define PWRLG_STATE()               (PWRLG_FLAGS().bits.state)
#define PWRLG_STATE_SET(x)          (PWRLG_FLAGS().bits.state = (x))

#define PWRLG_MESSAGE_COUNTS()      (PWRLG_RFSYS_BASE_PTR->uMessageCounts)

#define PWRLG_MESSAGE_COUNT()       (PWRLG_MESSAGE_COUNTS().total)
#define PWRLG_MESSAGE_COUNT_SET(x)  (PWRLG_MESSAGE_COUNTS().total = (x))

// Low order 8 bits of the RTC Status Register saved at reset before MQX modifies it.

#define PWRLG_MESSAGE_NUM()         (PWRLG_MESSAGE_COUNTS().sent)
#define PWRLG_MESSAGE_NUM_SET(x)    (PWRLG_MESSAGE_COUNTS().sent = (x))

#define PWRLG_SLEEP_SECONDS()       (PWRLG_RFSYS_BASE_PTR->uSleepSeconds)
#define PWRLG_SLEEP_MILLISECONDS()  (PWRLG_RFSYS_BASE_PTR->uSleepMilliseconds)
#define PWRLG_MILLISECONDS()        (PWRLG_RFSYS_BASE_PTR->uMilliseconds)
#define PWRLG_TIME_OUTAGE()         (PWRLG_RFSYS_BASE_PTR->timeOutage)
#define PWRLG_TIME_OUTAGE_SET(x)    (PWRLG_RFSYS_BASE_PTR->timeOutage = (x))

//#define PWRLG_CAP_BEFORE()          (PWRLG_RFSYS_BASE_PTR->fCapBefore)

#define PWRLG_DEBUG(x)              (PWRLG_RFSYS_BASE_PTR->uDebug[(x)])
#define PWRLG_DEBUG_SET(x,y)        (PWRLG_DEBUG(x) |= (1u << y))

#define RESTORATION_TIME()          (PWRLG_RFSYS_BASE_PTR->restorationTime)
#define TOTAL_BACK_OFF_TIME()       (PWRLG_RFSYS_BASE_PTR->totalBackOffTime)
#define CUR_BACK_OFF_TIME()         (PWRLG_RFSYS_BASE_PTR->curBackOffTime)
#define TX_FAILURES()               (PWRLG_RFSYS_BASE_PTR->txFailures)
#define PREV_MSG_TIME()             (PWRLG_RFSYS_BASE_PTR->prevMsgTime)

#define RESTORATION_TIME_SET(x)     (PWRLG_RFSYS_BASE_PTR->restorationTime = (x))
#define TOTAL_BACK_OFF_TIME_SET(x)  (PWRLG_RFSYS_BASE_PTR->totalBackOffTime = (x))
#define CUR_BACK_OFF_TIME_SET(x)    (PWRLG_RFSYS_BASE_PTR->curBackOffTime = (x))
#define TX_FAILURES_SET(x)          (PWRLG_RFSYS_BASE_PTR->txFailures = (x))
#define PREV_MSG_TIME_SET(x)        (PWRLG_RFSYS_BASE_PTR->prevMsgTime = (x))

#define PWRLG_LLWU_F3_SET(x)       (PWRLG_RFSYS_BASE_PTR->llwu_f3 = (x))
#define PWRLG_LLWU_F3()            (PWRLG_RFSYS_BASE_PTR->llwu_f3)
#define PWRLG_LLWU_FILT1_SET(x)    (PWRLG_RFSYS_BASE_PTR->llwu_filt1 = (x))
#define PWRLG_LLWU_FILT1()         (PWRLG_RFSYS_BASE_PTR->llwu_filt1 )

#define PWRLG_STATE_SLEEP              0
#define PWRLG_STATE_TRANSMIT           1
#define PWRLG_STATE_WAIT_FOR_RADIO     2
#define PWRLG_STATE_WAIT_RESTORATION   3
#define PWRLG_STATE_WAIT_LP_EXIT       4
#define PWRLG_STATE_COMPLETE           5

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
/*lint -esym(714,PWRLG_Startup) PWRLG_Startup called from outside PCLint scope  */
/*lint -esym(765,PWRLG_Startup) PWRLG_Startup called from outside PCLint scope  */
PWRLG_EXTERN void     PWRLG_Startup(void);
PWRLG_EXTERN void     PWRLG_Begin( uint16_t anomalyCount );
PWRLG_EXTERN void     PWRLG_Task( taskParameter );
PWRLG_EXTERN void     PWRLG_Idle_Task( taskParameter );
PWRLG_EXTERN uint8_t  PWRLG_LastGasp( void );
PWRLG_EXTERN void     PWRLG_BSP_Setup( void );
#if 0 // Not used
PWRLG_EXTERN uint32_t PWRLG_RandomRange(uint32_t uLow, uint32_t uHigh);
#endif
PWRLG_EXTERN void     PWRLG_Restoration( uint16_t powerAnomalyCount );
PWRLG_EXTERN void     PWRLG_RestLastGaspFlags(void);
void                  PWRLG_LLWU_ISR( void );

PWRLG_EXTERN returnStatus_t PWRLG_TxMessage( uint32_t timeOutage );
PWRLG_EXTERN void           PWRLG_TxFailure( void );
PWRLG_EXTERN void           PWRLG_SetCsmaParameters( void );
PWRLG_EXTERN uint16_t       PWRLG_CalculateMessages( void );
PWRLG_EXTERN void           PWRLG_CalculateSleep( uint8_t step );
PWRLG_EXTERN void           PWRLG_SetupLastGasp( void );
#if ( MCU_SELECTED == RA6E1 )
#if 1  // TEST // TODO: RA6E1: Remove later
void PWRLG_print_LG_Flags ( void );
#endif
#endif
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#endif // #ifndef pwr_last_gasp_H
