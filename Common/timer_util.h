/******************************************************************************
 *
 * Filename: timer_util.h
 *
 * Contents: Functions related to subscribable milliseconds Timer
 *
 ******************************************************************************
 * Copyright (c) 2010 Aclara Power-Line Systems Inc.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ******************************************************************************
 * Revision History:
 * 100710  MS    - Initial Release
 *
 *****************************************************************************/

#ifndef timer_util_H
#define timer_util_H

/* INCLUDE FILES */

#include "project.h"
#ifdef TMR_GLOBAL
   #define TMR_EXTERN
#else
   #define TMR_EXTERN extern
#endif

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

typedef void  (*vFPtr_u8_pv)(uint8_t, void *);

struct sTimerList
{
   vFPtr_u8_pv pFunctCallBack;         /* Keep call-back functions small, low CPU time */
   OS_SEM_Handle pSemHandle;          /* Semaphore handle, if not NULL, give semaphore */
   uint32_t ulDuration_mS;               /* Duration of timer in milli-seconds */
   uint32_t ulDuration_ticks;            /* Duration of timer in RTOS ticks */
   uint32_t ulTimer_ticks;               /* Current timer value in ticks */
   void *pData;                        /* Pointer to Data to Pass */
   struct sTimerList *next;            /* Points to the next member in the link list */
   unsigned usiTimerId: 16;            /* Timer ID */
   unsigned ucCmd:      8;             /* Command Parameter to Send */
   unsigned bOneShot:   1;             /* true = Run timer once, false = Reload Timer */
   unsigned bStopped:   1;             /* false = timer is active, true = Timer inactive (once one shot timer
                                          expires, this bit will be set to true */
   unsigned bExpired:   1;             /* true = timer expired, false otherwise. Meaningful for one shot timer */
   unsigned bOneShotDelete:   1;       /* Valid for one shot timers only. Ignored for periodic timers.
                                          true = Delete the timer after it fires. false = Do not delete the timer */
   unsigned bRsvd:      4;             /* Reserved Field */
};

typedef struct sTimerList timer_t;


/* TMR_EXTERN VARIABLES */

/* FUNCTION PROTOTYPES */
#ifdef __mqx_h__
void TMR_HandlerTask( uint32_t Arg0 );
#endif /* __mqx_h__ */
returnStatus_t TMR_HandlerInit(void);
returnStatus_t TMR_AddTimer(timer_t *pData);
returnStatus_t TMR_DeleteTimer(uint16_t usiTimerId);
returnStatus_t TMR_StartTimer(uint16_t usiTimerId);
returnStatus_t TMR_StopTimer(uint16_t usiTimerId);
returnStatus_t TMR_ResetTimer( uint16_t usiTimerId, uint32_t ulTimerValue);
returnStatus_t TMR_ReadTimer(timer_t *pData);
void           TMR_GetMillisecondCntr(uint64_t *ulMSCntr);

#undef TMR_EXTERN

#endif
