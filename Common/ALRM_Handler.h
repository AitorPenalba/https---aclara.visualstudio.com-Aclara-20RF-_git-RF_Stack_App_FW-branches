/******************************************************************************
 *
 * Filename: ALRM_Handler.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2018 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
#ifndef ALRM_Handler_H
#define ALRM_Handler_H

/* INCLUDE FILES */
#include "EVL_event_log.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */
/* TODO: 2019-03-28 SMG. This is not correct.  Currently the dtls success or failure events include two NVP's (16B) each.
         This should be: 86 = (Event "header"(7B) + ( Key (2B) + ValueSizeMax(16B) ) * NVP count of 2 )* 2 events */
#define MAX_ALARM_MEMORY   62    /* Maximum # bytes allowed to be included in ID bubble up message. This is currently
                                    the size of two dlts session success/failure messages */

#if ( EP == 1 )
#define NUMBER_METER_DIAGS   40                             /* Standard table 3 (5 bytes) */
#if 0   /* Note: meterEvents not currently used. Keep for future enhancements.  */
#define NUMBER_METER_EVENTS  (1024 - NUMBER_METER_DIAGS )   /* Must be multiple of 8  */
#endif
#endif

/* TYPE DEFINITIONS */
/* Create a structure to hold event history; one bit per event. If set, event is active, otherwise event cleared. Group
   into transponder based events and meter based events. This structure is kept in a file.
*/
PACK_BEGIN
typedef struct PACK_MID
{
   struct
   {
      uint16_t highTempThresholdEvent :1;  /* Keeps track of the High Temperature Threshold Event */
      uint16_t lowTempThresholdEvent  :1;  /* Keeps track of the low Temperature Threshold Event */
      uint16_t reserved : 14;
   }comEvents_t;
#if ( EP == 1 )
   uint8_t meterDiags[ NUMBER_METER_DIAGS / 8 ];
#if 0  /* Note: meterEvents not currently used. Keep for future enhancements.  */
   uint8_t meterEvents[ NUMBER_METER_EVENTS / 8 ]; /* Up to NUMBER_METER_EVENTS meter        event IDs   */
#endif
#endif
} eventHistory_t;
PACK_END

#if 0
/* KTL - NOTE:  When the size of this enum changes, Non-Volatile memory will break
                we will need to force an erase of NovRam to get it working again.  See Karl D. for better explanation */
typedef enum
{
   ALRM_PHY_LOOPBACK_FAIL,       /* 26.1.122.85 */
   ALRM_NVM_COMM_ERROR,          /* 26.1.72.79 */
   ALRM_HMC_LOOPBACK_FAIL,       /* 26.1.76.85 */
   ALRM_NVM_SELFTEST_ERROR,      /* 26.18.100.79 */
   ALRM_NVM_MEM_CKSUM_ERROR,     /* 26.18.72.43 */
   ALRM_PROG_MEM_ERROR,          /* 26.18.92.79 */
   ALRM_SIGNIF_MTR_CLOCK_CHANGE, /* 26.21.117.139 */
   ALRM_POWER_RESTORE,           /* 26.26.0.216 */
   ALRM_POWER_OUT,               /* 26.26.0.85 */
   ALRM_HIGH_TEMP,               /* 26.35.261.93 */
   ALRM_RTC_FAILURE,             /* 26.36.0.85 */
   ALRM_RTC_LOST_DATE_TIME,      /* 26.36.109.79 */
   ALRM_DISCONTINUOUS_TIME,      /* 26.36.114.24 */
   ALRM_UNTRUSTED_DATE_TIME,     /* 26.36.114.63 */
   ALRM_DISCONTINUOUS_DATE,      /* 26.36.34.24 */
   ALRM_MTR_SYS_ERROR,           /* 3.0.0.79 */
   ALRM_MTR_SELF_CHECK_ERROR,    /* 3.0.100.79 */
   ALRM_MTR_DSP_ERROR,           /* 3.0.82.79 */
   ALRM_HM_COMM_ERROR,           /* 3.1.0.85 */
   ALRM_MTR_FW_CHANGED,          /* 3.11.17.52 */
   ALRM_HM_AUTH_FAIL,            /* 3.12.24.35 */
   ALRM_COVER_REMOVED,           /* 3.12.29.212 */
   ALRM_MTR_MEM_DATA_ERROR,      /* 3.18.31.79 */
   ALRM_MTR_NVM_MEM_FAIL,        /* 3.18.72.85 */
   ALRM_MTR_RAM_FAIL,            /* 3.18.85.85 */
   ALRM_MTR_ROM_FAIL,            /* 3.18.92.85 */
   ALRM_MTR_MEM_CODE_ERROR,      /* 3.18.83.79 */
   ALRM_MTR_LOW_BATTERY,         /* 3.2.22.150 */
   ALRM_MTR_MEASURE_ERROR,       /* 3.21.67.79 */
   ALRM_MTR_POWER_FAILURE,       /* 3.26.0.85 */
   ALRM_MTR_PHASE_ANGLE_ALERT,   /* 3.26.130.40 */
   ALRM_MTR_UNDER_VOLT_PH_A,     /* 3.26.131.223 */
   ALRM_MTR_OVER_VOLT_PH_A,      /* 3.26.131.248 */
   ALRM_MTR_HIGH_NEUT_CURRENT,   /* 3.26.137.93 */
   ALRM_MTR_INACTIVE_PH_CURRENT, /* 3.26.25.100 */
   ALRM_MTR_CROSS_PHASE,         /* 3.26.25.45 */
   ALRM_MTR_HIGH_DISTORTION,     /* 3.26.28.69 */
   ALRM_MTR_LEADING_KVARH,       /* 3.26.294.219 */
   ALRM_MTR_LOW_LOSS_POTENTIAL,  /* 3.26.38.150 */
   ALRM_MTR_VOLT_IMBALANCE,      /* 3.26.38.98 */
   ALRM_MTR_RECV_KWH,            /* 3.26.93.219 */
   ALRM_MTR_CLOCK_ERROR,         /* 3.36.0.79 */
   ALRM_MTR_DATE_CHANGE,         /* 3.36.34.24 */
   ALRM_ALTERED_HM_CONFIG,       /* 3.7.0.24 */
   ALRM_MTR_CONFIG_ERROR,        /* 3.7.0.79 */
   ALRM_ALTERED_HM_PROG,         /* 3.7.83.24 */
   ALRM_MTR_LOSS_OF_PROGRAM,     /* 3.7.83.47 */
   ALRM_MTR_UNPROGRAMMED,        /* 3.7.83.61 */
   ALRM_MTR_DEMAND_OVERLOAD,     /* 3.8.0.177 */
   ALRM_MAX_NUM_TYPES /* KTL - NOTE:  when the numerical value of this changes, we have to force an erase of NovRam */
} enum_ALRM_Types;
#endif


/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
extern returnStatus_t   ALRM_init ( void );
returnStatus_t          ALRM_clearAllAlarms ( void );
extern void             ALRM_RealTimeTask ( taskParameter );
extern uint8_t          ALRM_AddOpportunisticAlarms(  uint16_t msgLen, uint16_t *alarmLen, uint8_t *alarmData, uint8_t *alarmID, EventMarkSent_s *markedSent );

#endif /* this must be the last line of the file */
