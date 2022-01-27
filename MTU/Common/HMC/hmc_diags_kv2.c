// <editor-fold defaultstate="collapsed" desc="File Header">
/***********************************************************************************************************************
 *
 * Filename: hmc_diags_kv2.c
 *
 * Global Designator: HMC_DIAGS_KV2_
 *
 * Contents: Read Diagnostics from the KV2, MT71
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license. ******************************************************************************
 *
 * $Log$
 *
 *  Created 08/16/13     KAD  Created
 *
 ***********************************************************************************************************************
 ***********************************************************************************************************************
 * Revision History:
 * v1.0 - Initial Release
 **********************************************************************************************************************/// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#define HMC_DIAGS_KV2_GLOBAL
#include "hmc_diags_kv2.h"
#undef HMC_DIAGS_KV2_GLOBAL

#include "pack.h"
#include "ansi_tables.h"
#include "time_sys.h"

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define DIAG_KV2_RETRIES            ((uint8_t)2)
#define DIAG_KV2_ALARM_PEROID_MIN   ((uint8_t)5)           /* Called every 5 minutes */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

#if 0
xxx
typedef struct
{
   unsigned diag1: 1;
   unsigned diag2: 1;
   unsigned diag3: 1;
   unsigned diag4: 1;
   unsigned diag5: 1;
   unsigned diag6: 1;
   unsigned diag7: 1;
   unsigned diag8: 1;
   uint8_t  diagDisplay;
   uint8_t  diagFreeze;
   uint8_t  diagTrigOutput;
   uint8_t  phAVoltTol;
   uint16_t lowCurrThres;
   uint16_t curAngTol;
   uint8_t  distortionTol;
   uint16_t phAVoltRef;
   uint8_t  phAVoltSagTol;
   uint8_t  phAVoltSwellTol;
   uint16_t highNeutralCurrThres;
   unsigned diag5Config: 3;
   unsigned filler:      5;
   uint16_t diags_6_7_duration;
   uint16_t otherDiagsDuration;
}mfgTbl71_t;  /* Line-Side Diagnostics/Power Quality Configuration Table  (Changed in kV2) */
#endif

typedef struct
{
   uint16_t currentAnglePhA; /* CURRENT_ANGLE_PHA	Phase A current angle in 10th degrees.  See note below. */
	uint16_t voltageAnglePhA; /*	VOLTAGE_ANGLE_PHA	Phase A voltage angle in 10th degrees.  See note below. */
	uint16_t currentAnglePhB; /*	CURRENT_ANGLE_PHB	Phase B current angle in 10th degrees.  See note below. */
	uint16_t voltageAnglePhB; /*	VOLTAGE_ANGLE_PHB	Phase B voltage angle in 10th degrees.  See note below. */
   uint16_t currentAnglePhC; /* CURRENT_ANGLE_PHC	Phase C current angle in 10th degrees.  See note below. */
	uint16_t voltageAnglePhC; /*	VOLTAGE_ANGLE_PHC	Phase C voltage angle in 10th degrees.  See note below. */
	uint16_t currentMagPhA;   /*	CURRENT_MAG_PHA	Phase A current magnitude.  See note below. */
	uint16_t voltageMagPhA;   /*	VOLTAGE_MAG_PHA	Phase A voltage magnitude.  See note below. */
	uint16_t currentMagPhB;   /*	CURRENT_MAG_PHB	Phase B current magnitude.  See note below. */
	uint16_t voltageMagPhB;   /*	VOLTAGE_MAG_PHB	Phase B voltage magnitude.  See note below. */
	uint16_t currentMagPhC;   /*	CURRENT_MAG_PHC	Phase C current magnitude.  See note below. */
	uint16_t voltageMagPhC;   /*	VOLTAGE_MAG_PHC	Phase C voltage magnitude.  See note below. */
   uint8_t  duPf;            /* DU_PF	Distortion power factor. */
	uint8_t  diagCnt1;        /* DIAG_1	Count of number of occurrences of diagnostic 1. */
	uint8_t  diagCnt2;        /* DIAG_2	Count of number of occurrences of diagnostic 2. */
	uint8_t  diagCnt3;        /* DIAG_3	Count of number of occurrences of diagnostic 3. */
	uint8_t  diagCnt4;        /* DIAG_4	Count of number of occurrences of diagnostic 4. */
	uint8_t  diagCnt5PhA;     /* DIAG_5_PHA			Count of number of occurrences of diagnostic 5 for phase A. */
	uint8_t  diagCnt5PhB;     /* DIAG_5_PHB	Count of number of occurrences of diagnostic 5 for phase B. */
	uint8_t  diagCnt5PhC;     /* DIAG_5_PHC	Count of number of occurrences of diagnostic 5 for phase C. */
	uint8_t  diagCnt5Total;   /* DIAG_5_TOTAL	Count of number of occurrences of diagnostic 5 for all phases. */
	uint8_t  diagCnt6;        /* DIAG_6	Count of number of occurrences of diagnostic 6. */
	uint8_t  diagCnt7;        /* DIAG_7	Count of number of occurrences of diagnostic 7. */
	uint8_t  diagCnt8;        /* DIAG_8	Count of number of occurrences of diagnostic 8. */
	unsigned diag1:   1;    /* DIAG_1	Indicates occurrence of diagnostic 1.  (0 = False, 1 = True) */
	unsigned diag2:   1;    /* DIAG_2	Indicates occurrence of diagnostic 2.  (0 = False, 1 = True) */
	unsigned diag3:   1;    /* DIAG_3	Indicates occurrence of diagnostic 3.  (0 = False, 1 = True) */
	unsigned diag4:   1;    /* DIAG_4	Indicates occurrence of diagnostic 4.  (0 = False, 1 = True) */
	unsigned diag5:   1;    /* DIAG_5	Indicates occurrence of diagnostic 5.  (0 = False, 1 = True) */
	unsigned diag6:   1;    /* DIAG_6	Indicates occurrence of diagnostic 6.  (0 = False, 1 = True) */
	unsigned diag7:   1;    /* DIAG_7	Indicates occurrence of diagnostic 7.  (0 = False, 1 = True) */
	unsigned diag8:   1;    /* DIAG_8	Indicates occurrence of diagnostic 8.  (0 = False, 1 = True) */
}mfgTbl72_t;  /* Line-Side Diagnostics/Power Quality Data Table  (Changed in kV2) */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local File Variables">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static mfgTbl72_t kv2Diags_;   /* Manufacturing Table 72 - KV2 Diags */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Constant Definitions">
/* ****************************************************************************************************************** */
/* CONSTANTS */

static const tReadPartial tblKv2Diags_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT((72 | (1<<11))),        /* MT72 holds the diag info */  /*lint !e572 !e778 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)0 },      /* Offset for diag in ST3   */  /*lint !e651 */
   BIG_ENDIAN_16BIT(sizeof(kv2Diags_))    /* Length for diag in ST3   */  /*lint !e572 !e778 */
};

static const HMC_PROTO_TableCmd cmdKv2Diags_[] =
{
   { HMC_PROTO_MEM_PGM, (uint8_t)sizeof(tblKv2Diags_), (uint8_t far *)&tblKv2Diags_[0] },
   { (enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_RAM | (uint16_t)HMC_PROTO_MEM_WRITE),
      sizeof(kv2Diags_), (uint8_t far *)&kv2Diags_ },
   { HMC_PROTO_MEM_NULL }
};

static const HMC_PROTO_Table tblsKv2Diags_[] =
{
   { &cmdKv2Diags_[0] },
   { NULL }
};

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

// </editor-fold>

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Function name: HMC_DIAGS_KV2_app
 *
 * Purpose: Checks tasks
 *
 * Arguments: uint8_t cmd, void *pData
 *
 * Returns: uint8_t Results of request
 *
 **********************************************************************************************************************/
uint8_t HMC_DIAGS_KV2_app(uint8_t cmd, void *pData)
{
   static OS_QUEUE_Obj  hmcDiagQueueHandle_;                   /* Used for alarm as a trigger to update. */
   static tTimeSysMsg   alarmMsg_;                             /* Contains the message data from the time module. */
   static bool       bModuleInitialized_ = false;           /* true = Initialized, false = uninitialized. */
   static uint8_t       retries_ = DIAG_KV2_RETRIES;           /* Used for application level retries */
   static bool       bReadFromMeter_ = false;               /* true = Read from host meter, false = Do nothing. */
   static uint8_t       alarmId_ = 0;                          /* Alarm ID */
          uint8_t       retVal = (uint8_t)HMC_APP_API_RPLY_IDLE; /* Return value, assume always idle */

   switch(cmd)
   {
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_INIT">
      case HMC_APP_API_CMD_INIT:
      {
         if ( !bModuleInitialized_ )
         {
            tTimeSysPerAlarm alarmSettings;                    /* Configure the periodic alarm for time */

            (void)OS_QUEUE_Create(&hmcDiagQueueHandle_);
            bModuleInitialized_ = true;
            alarmSettings.pMboxHandle = NULL;                  /* Don't use the mailbox */
            alarmSettings.bOnInvalidTime = false;              /* Only alarmed on valid time, not invalid */
            alarmSettings.bOnValidTime = true;                 /* Alarmed on valid time */
            alarmSettings.bSkipTimeChange = true;              /* do NOT notify on time change */
            alarmSettings.bUseBSOffset = false;                /* do NOT use the BS offset */
            alarmSettings.bUseUTCOffset = false;               /* do NOT use UTC offset */
            alarmSettings.pQueueHandle = &hmcDiagQueueHandle_;
            alarmSettings.pAlarmMsg = &alarmMsg_;              /* Location to store alarm data */
            alarmSettings.ucAlarmId = 0;                       /* Alarm Id */
            alarmSettings.ulOffset = 0;
            alarmSettings.ulPeriod = TIME_TICKS_PER_MIN * DIAG_KV2_ALARM_PEROID_MIN;
            (void)TIME_SYS_AddPerAlarm(&alarmSettings);
            alarmId_ = alarmSettings.ucAlarmId;                /* Store alarm ID that was given by Time Sys. module */
         }
         (void)memset(&kv2Diags_, 0, sizeof(kv2Diags_));
         /* Fall through to Activate */
      }
      // FALLTHROUGH
      // </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_ACTIVATE">
      case HMC_APP_API_CMD_ACTIVATE: // Fall through to Init
      {
         retries_ = DIAG_KV2_RETRIES;
         bReadFromMeter_ = true;
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_STATUS">
      case HMC_APP_API_CMD_STATUS:
      {
         tTimeSysMsg *pAlarmQueue;
         pAlarmQueue = OS_QUEUE_Dequeue(&hmcDiagQueueHandle_);   /* See if there is a message in the queue */
         if ( (NULL != pAlarmQueue) && (alarmMsg_.alarmId == alarmId_) )
         {  /* Got message, this message can be for applet serviced by this task */
            if ( bReadFromMeter_ )
            {
               bReadFromMeter_ = true;
               retries_ = DIAG_KV2_RETRIES;
               retVal = (uint8_t)HMC_APP_API_RPLY_READY_COM;
            }
         }
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_PROCESS">
      case HMC_APP_API_CMD_PROCESS:
      {
         retVal = (uint8_t)HMC_APP_API_RPLY_READY_COM;
         ((HMC_COM_INFO *)pData)->pCommandTable = (uint8_t far *)tblsKv2Diags_;
         bReadFromMeter_ = false;
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_MSG_COMPLETE">
      case HMC_APP_API_CMD_MSG_COMPLETE:
      {
         bReadFromMeter_ = false;
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_ABORT, HMC_APP_API_CMD_TBL_ERROR, HMC_APP_API_CMD_ERROR">
      case HMC_APP_API_CMD_ABORT:
      case HMC_APP_API_CMD_TBL_ERROR:
      case HMC_APP_API_CMD_ERROR:
      {
         if ( 0 != retries_ )
         {
            retries_--;
         }
         if ( 0 == retries_ )
         {
            bReadFromMeter_ = false;
         }
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="default">
      default: // No other commands are supported in this applet
      {
         retVal = (uint8_t)HMC_APP_API_RPLY_UNSUPPORTED_CMD;
         break;
      }// </editor-fold>
   }
   return(retVal);
}
/* ****************************************************************************************************************** */
