/* ****************************************************************************************************************** */
/***********************************************************************************************************************

   Filename: meter_ansi_sig.h

   Contents: Project Specific Includes for the GE i210+c

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2013-2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   @author KAD

   $Log$ Created March 29, 2013

 ***********************************************************************************************************************
   Revision History:
   v0.1 - KAD 03/29/2013 - Initial Release

   @version    0.1
   #since      2013-03-29
 **********************************************************************************************************************/
#if HMC_I210_PLUS_C
#ifndef meter_ansi_sig_H_
#define meter_ansi_sig_H_

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define STD_TBL_PROC_INIT_MAX_PARAMS          ((uint16_t)8)           /* Maximum parameter bytes allowed - I210+c */
#define STD_TBL_PROC_RESP_MAX_BYTES           ((uint16_t)13)          /* Maximum response bytes allowed - I210+c */

/* Ansi Procedure Names */
#define MFG_PROC_CLR_IND_STD_STATUS_FLAGS     ((uint16_t)0  | 0x0800) /* Clear Individual Standard Status Flags */
#define MFG_PROC_CLR_IND_MFG_STATUS_FLAGS     ((uint16_t)1  | 0x0800) /* Clear Individual Mfg. Status Flags */
#define MFG_PROC_DIRECT_EXECUTE               ((uint16_t)2  | 0x0800)
#define MFG_PROC_BATTERY_TEST                 ((uint16_t)3  | 0x0800)
#define MFG_PROC_CFG_TST_PULSES               ((uint16_t)64 | 0x0800)
#define MFG_PROC_RST_CUM_PWR_OUTAGE_DUR       ((uint16_t)65 | 0x0800)
#define MFG_PROC_CHANGE_CFG_STATUS            ((uint16_t)66 | 0x0800)
#define MFG_PROC_CONVERT_MTR_MODE             ((uint16_t)67 | 0x0800)
#define MFG_PROC_UPGRADE_MTR                  ((uint16_t)68 | 0x0800)
#define MFG_PROC_RST_LSD_DIAGS                ((uint16_t)69 | 0x0800)
#define MFG_PROC_PGM_CTRL                     ((uint16_t)70 | 0x0800)
#define MFG_PROC_RST_ERROR_HIST               ((uint16_t)71 | 0x0800)
#define MFG_PROC_RST_START_WAVE_FORM_CTRL     ((uint16_t)72 | 0x0800)
#define MFG_PROC_FLASH_CAL_MODE_CTRL          ((uint16_t)73 | 0x0800)
#define MFG_PROC_SET_CAL_DATE_TIME            ((uint16_t)75 | 0x0800)
#define MFG_PROC_REMOVE_SOFT_SW               ((uint16_t)76 | 0x0800)
#define MFG_PROC_RTP_CTRL                     ((uint16_t)78 | 0x0800)
#define MFG_PROC_RST_VOLT_EVENT_MON_PTR       ((uint16_t)79 | 0x0800)
#define MFG_PROC_UPDATE_LAST_READ_ENTRY       ((uint16_t)80 | 0x0800)
#define MFG_PROC_DNLD_MFG_TST_CODE            ((uint16_t)81 | 0x0800)
#define MFG_PROC_FLASH_ERASE                  ((uint16_t)82 | 0x0800)
#define MFG_PROC_DNLD_PRODUCTION_CODE         ((uint16_t)83 | 0x0800)
#define MFG_PROC_SNAPSHOT_REVENUE_DATA        ((uint16_t)84 | 0x0800)
#define MFG_PROC_TIME_ACCELERATION_CTRL       ((uint16_t)85 | 0x0800)

/* Ansi Manufacturer Tables */
#define MFG_TBL_GE_DEVICE_TABLE               ((uint16_t)  0 | 0x0800)
#define MFG_TBL_MEMORY_TABLE                  ((uint16_t) 12 | 0x0800)
#define MFG_TBL_PROGRAM_CONSTANTS             ((uint16_t) 67 | 0x0800)
#define MFG_TBL_ERROR_CAUTION                 ((uint16_t) 68 | 0x0800)
#define MFG_TBL_ERROR_HISTORY                 ((uint16_t) 69 | 0x0800)
#define MFG_TBL_DISPLAY_CONFIG                ((uint16_t) 70 | 0x0800)
#define MFG_TBL_POWER_QUALITY_CONFIG          ((uint16_t) 71 | 0x0800)
#define MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA  ((uint16_t) 72 | 0x0800)
#define MFG_TBL_POWER_FACTOR_CONFIG           ((uint16_t) 73 | 0x0800)
#define MFG_TBL_TEST_MODE_CONFIG              ((uint16_t) 74 | 0x0800)
#define MFG_TBL_SCALE_FACTORS                 ((uint16_t) 75 | 0x0800)
#define MFG_TBL_IO_OPTION_BOARD_CONTROL       ((uint16_t) 76 | 0x0800)
#define MFG_TBL_LOAD_CONTROL_SWITCH           ((uint16_t) 77 | 0x0800)
#define MFG_TBL_SECURITY_LOG                  ((uint16_t) 78 | 0x0800)
#define MFG_TBL_ALTERNATE_CALIBRATION         ((uint16_t) 79 | 0x0800)
#define MFG_TBL_SECURITY                      ((uint16_t) 84 | 0x0800)
#define MFG_TBL_METER_STATE                   ((uint16_t) 85 | 0x0800)
#define MFG_TBL_CYCLE_INSENSITIVE_DMD_CONFIG  ((uint16_t) 97 | 0x0800)
#define MFG_TBL_CYCLE_INSENSITIVE_DMD_DATA    ((uint16_t) 98 | 0x0800)
#define MFG_TBL_AMI_SECURITY                 ((uint16_t) 107 | 0x0800)
#define MFG_TBL_VOLTAGE_EVENT_MONITOR_CONFIG ((uint16_t) 111 | 0x0800)
#define MFG_TBL_VOLTAGE_EVENT_MONITOR_LOG    ((uint16_t) 112 | 0x0800)
#define MFG_TBL_POWER_QUALITY_DATA           ((uint16_t) 113 | 0x0800)
#define MFG_TBL_LOAD_CONTROL_CONFIG          ((uint16_t) 114 | 0x0800)
#define MFG_TBL_LOAD_CONTROL_STATUS          ((uint16_t) 115 | 0x0800)
#define MFG_TBL_RCDC_SWITCH_STATUS           ((uint16_t) 117 | 0x0800)
#define MFG_TBL_NETWORK_STATUS_DISPLAY_DATA  ((uint16_t) 119 | 0x0800)

#define MODE_DEMAND_ONLY      0
#define MODE_DEMAND_LP        1
#define MODE_DEMAND_TOU       2

#define MT98_NBR_DEMANDS      2
#define MT98_NBR_TIERS        2
#define MT98_NBR_OF_ENTRIES   35


/* Meter log event argument   */
#define  voltagePhaseA        (1<<0)   /*lint !e778 shift qty of 0 is OK   */
#define  voltagePhaseB        (1<<1)
#define  voltagePhaseC        (1<<2)
#define  currentPhaseA        (1<<3)
#define  currentPhaseB        (1<<4)
#define  currentPhaseC        (1<<5)
#define  totalEvent           (1<<6)

#define SWELL_EVENT           (( uint8_t ) 1 )
#define SAG_EVENT             (( uint8_t ) 0 )

#define METER_PASSWORD_SIZE   ((uint8_t)20)

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "object_list.h"
#include "hmc_ds.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef ANSI_SIG_GLOBALS
#define ANSI_SIG_EXTERN
#else
#define ANSI_SIG_EXTERN extern
#endif
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* Begin - Manufacturer Table 0  */
PACK_BEGIN
typedef struct
{
   uint32_t    tou                  : 1;
   uint32_t    secondMeasure        : 1;
   uint32_t    twoChanRecording     : 1;
   uint32_t    eventLogging         : 1;
   uint32_t    alternateComm        : 1;
   uint32_t    notUsed1             : 1;
   uint32_t    notUsed2             : 1;
   uint32_t    demandCalculation    : 1;

   uint32_t    notUsed3             : 1;
   uint32_t    notUsed4             : 1;
   uint32_t    notUsed5             : 1;
   uint32_t    notUsed6             : 1;
   uint32_t    voltageEventMonitor  : 1;
   uint32_t    notUsed7             : 1;
   uint32_t    notUsed8             : 1;
   uint32_t    notUsed9             : 1;

   uint32_t    voltageMeasurement   : 1;
   uint32_t    notused10            : 1;
   uint32_t    notused11            : 1;
   uint32_t    notused12            : 1;
   uint32_t    notused13            : 1;
   uint32_t    notused14            : 1;
   uint32_t    notused15            : 1;
   uint32_t    ecp                  : 1;

   uint32_t    dlp                  : 1;
   uint32_t    ppm                  : 1;
   uint32_t    rsvd                 : 6;

} UpgradesBfld_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           GE_PartNumber[5];
   uint8_t           FwVersion;
   uint8_t           FwRevision;
   uint8_t           FwBuild;
   uint8_t           MfgTestVector[4];
}romConstDataRCD_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           partNumber[5];       /* BCD   */
   uint8_t           fwver;
   uint8_t           fwRev;
   uint8_t           fwBuild;
}tdkConstRCD_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           partNum[5];          /* BCD   */
   uint8_t           romVer;
   uint8_t           romRev;
   uint8_t           romBuild;
   uint8_t           romEepromVer;
   uint8_t           updateVer;        /* a.k.a patch version */
   uint8_t           updateRev;        /* a.k.a patch revision */
   uint8_t           updateBuild;
   uint8_t           updateEepromVer;
   uint8_t           updateFlags;
   uint16_t          updateChecksum;
}updateConstants_t;                    /* a.k.a patch constants in MeterMate */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           MfgVersionNbr;
   uint8_t           MfgRevisionNbr;
   romConstDataRCD_t romConstDataRcd;
   uint8_t           MeterType;
   uint8_t           MeterMode;
   uint8_t           RegisterFunction;
   uint8_t           InstalledOption1;
   uint8_t           InstalledOption2;
   uint8_t           InstalledOption3;
   uint8_t           InstalledOption4;
   uint8_t           InstalledOption5;
   uint8_t           InstalledOption6;
   UpgradesBfld_t    upgrades;
   uint8_t           reserved[4];
   tdkConstRCD_t     TDKConstRCD;
   uint8_t           updateEnabledFlag;
   uint8_t           updateActiveFlag;
   updateConstants_t updateConstants;
   uint32_t          baseCRC;
   uint8_t           eepromVer;
   uint16_t          updateCRCcalculated;
   uint16_t          updateCRCdownloaded;
   uint8_t           lpMemSize[3];
   uint32_t          patchCodeStartAddress;
} mfgTbl0_t;   /* Manufacturer Table #0 - GE Device Table   */
PACK_END
/* End - Manufacturer Table 0  */

/* Begin - Manufacturer Table 69  */
PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint8_t  MeterChipError:   1; /* Indicates if error in meter chip occurred. */
      uint8_t  optionBrd:        1; /* Not used   */
      uint8_t  watchdogTimeout:  1; /* Indicates if a watchdog timeout has occurred   */
      uint8_t  ram:              1; /* Indicates if a RAM error has occurred.   */
      uint8_t  rom:              1; /* Indicates if a ROM error has occurred.   */
      uint8_t  eeprom:           1; /* Indicates if an EEPROM error has occurred.  */
      uint8_t  battPotFail:      1; /* Indicates if battery failure/power outage has occurred  */
      uint8_t  measurementError: 1; /* Indicates if a bad voltage reference was detected.   0 */
   } bits;
   uint8_t  byte;
} errorRcd_t;
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint8_t  meterChipError1:        1;    /* Indicates if error in meter chip occurred. */
      uint8_t  meterChipCommError:     1;    /* Indicates if the meter chip comm failed. */
      uint8_t  meterChipError2:        1;    /* Indicates if error in meter chip occurred. */
      uint8_t  meterChipMomIntTimeout: 1;    /* Indicates no end of momentary interval signal rec'd from meter chip */
      uint8_t  meterChipError3:        1;    /* Indicates if error in meter chip occurred. */
      uint8_t  meterChipOverrun:       1;    /* Indicates if overrun occurred. (0 = False, 1 = True) */
      uint8_t  meterChipOverflow:      1;    /* Not used in I-210+c. */
      uint8_t  meterChipZeroCross:     1;    /* Not used in I-210+c. */
   } bits;
   uint8_t  byte;
} meteringError_t;
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint8_t  romCodeError:        1; /* Error occurred in code sotred in ROM.   */
      uint8_t  flashCodeError:      1; /* Error occurred in code stored in Flash.  */
      uint8_t  userDefCalcError:    1; /* Error occurred in user-calculations code in Flash */
      uint8_t  reserved:            5; /* 5 bits 0   */
   } bits;
   uint8_t  byte;
} romFlashErrors_t;
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint8_t     powerFailSegmentError:  1; /* Error occurred in Power Fail area of EEPROM */
      uint8_t     configSegmentError:     1; /* Error occurred in configuration area of EEPROM.   */
      uint8_t     programSegment0Error:   1; /* Error occurred in program segment 0.  */
      uint8_t     programSegment1Error:   1; /* Error occurred in program segment 1.  */
      uint8_t     currRevSegment0Error:   1; /* Error occurred in current revenue segment 0.   */
      uint8_t     currRevSegment1Error:   1; /* Error occurred in current revenue segment 1.   */
      uint8_t     staticRevSegmentError:  1; /* Error occurred in the static revenue segment.  */
      uint8_t     currRevSegment2Error:   1; /* Error occurred in current revenue segment 2.   0 0  */
   } bits;
   uint8_t  byte;
} eepromErrors_t;
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint8_t     lpDataError:            1; /* Error occurred in LP data memory. (0 = False, 1 = True) */
      uint8_t     reserved:               1; /* */
   } bits;
   uint8_t  byte;
} eepromErrors2_t;
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint16_t   lowBattery:        1; /* A low battery caution is present   */
      uint16_t   lowPotential:      1; /* A low potential caution is present */
      uint16_t   dmdThreshOverload: 1; /* A demand threshold overload caution is present */
      uint16_t   receivedKwh:       1; /* A received kWh caution is present  */
      uint16_t   leadingKvarh:      1; /* A leading kvarh caution is present */
      uint16_t   unprogrammed:      1; /* An unprogrammed caution is present */
      uint16_t   lossOfProgram:     1; /* A loss-of-program caution is present  */
      uint16_t   timeChanged:       1; /* A time change caution is present.  */
      uint16_t   badPassword:       1; /* A bad password caution is present  */
      uint16_t   reserved:          1; /* Not used in the I210+c   */
      uint16_t   highTemperature:   1; /* Determines if meter has detected a high temperature condition (Available only
                                          for firmware versions 4.11 update 10.12 and later and FW 4.12 and later). */
      uint16_t   filler:            5; /* Reserved 5 bit   */
   } bits;
   uint8_t  bytes[2];
} cautionRcd_t;
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      /* Diag BFLD   */
      uint8_t     diag1:   1; /* A diagnostic 1 caution present. */
      uint8_t     diag2:   1; /* A diagnostic 2 caution present. */
      uint8_t     diag3:   1; /* A diagnostic 3 caution present. */
      uint8_t     diag4:   1; /* A diagnostic 4 caution present. */
      uint8_t     diag5:   1; /* A diagnostic 5 caution present  */
      uint8_t     diag6:   1; /* A diagnostic 6 caution present  */
      uint8_t     diag7:   1; /* A diagnostic 7 caution present  */
      uint8_t     diag8:   1; /* A diagnostic 8 caution present  */
   } bits;
   uint8_t  byte;
} diagBfld_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   errorRcd_t     errorRcd;      /* Error RCD   */
   cautionRcd_t   cautionRcd;    /* Caution RCD */
   diagBfld_t     diagBfld;      /* Diag BFLD   */
} statusRcd_t;
PACK_END

/* Manufacturer Table 68 */
PACK_BEGIN
typedef struct PACK_MID
{
   /* ERROR_HISTORY  */
   errorRcd_t        errorFreeze;
   cautionRcd_t      cautionEnable;             /* Caution Enable   */
   cautionRcd_t      cautionDisplay;
   cautionRcd_t      cautionFreeze;
   uint32_t          dmdOverloadThresh;
   uint8_t           rsvd1;
   uint8_t           minNbrBadSecurityCodes;
   uint8_t           phAvoltSagTolerance;
   uint8_t           serviceErrorDetectEnable;
   uint16_t          serviceMinVoltage;
   uint16_t          serviceMaxVoltage;
   uint8_t           highTempThreshold;
   uint8_t           securityCodeLockoutDuration;
   uint8_t           relayStateChangeEnabled;   /* Not supported  */
   uint8_t           cautionEnable2;            /* Not supported  */
   uint32_t          receivedWhThresh;
   uint32_t          leadingVarThresh;
   uint32_t          demandWarningThresh;
   uint8_t           eventAlertControl[8];      /* bit fields  */
   uint8_t           eventAlertControlMfg[12];  /* bit fields  */
   uint8_t           currentImbalanceI;         /* Current imbalance in amps  */
   uint8_t           currentImbalancePercent;   /* Current imbalance in Percent  */
   uint8_t           currentImbalanceDuration;  /* Current imbalance duration in minutes  */
   uint8_t           magTamperDuration;         /* Magnetic tamper duration in minutes  */
} mfgTbl68_t;  /* Manufacturer Table 68 Error/Caution Table */
PACK_END

/* Begin - Manufacturer Table 69  */
PACK_BEGIN
typedef struct PACK_MID
{
   /* ERROR_HISTORY  */
   errorRcd_t        errorHistory;
   meteringError_t   meteringError;
   meteringError_t   meteringErrorHistory;

   cautionRcd_t      cautionHistory;            /* Caution History   */
   eepromErrors2_t   eepromError2;
   eepromErrors2_t   eepromError2History;
   romFlashErrors_t  romError;
   romFlashErrors_t  romErrorHistory;
   eepromErrors_t    eepromError1;              /* Bits defined as above */
   eepromErrors_t    eepromError1History;       /* Bits defined as above */
   uint32_t          flashSectorErrors;         /* One error bit for each sector, 0 - 31. */
   uint32_t          flashSectorErrorHistory;   /* One error bit for each sector, 0 - 31. */

   statusRcd_t       rollingStatus;             /* Errors and cautions that occurred in the 35-day billing period */
   statusRcd_t       dailyStatus;               /* Errors, cautions and diagnostics that occurred today */
   statusRcd_t       realTimeStatus;            /* Real-time indication of errors, cautions and diagnostics (see
                                                   STATUS_RCD above) */
} mfgTbl69_t;  /* History of errors from Standard Table #3 - End Device Mode and Status */
PACK_END
/* End - Manufacturer Table 69  */

/* Begin - Manufacturer Table 72  */
PACK_BEGIN
typedef struct PACK_MID
{
   /* ERROR_HISTORY  */
   uint16_t          iAnglePhA;        /* Phase A current angle in 10th degrees */
   uint16_t          vAnglePhA;        /* Phase A voltage angle in 10th degrees   */
   uint16_t          iAnglePhB;        /* Phase A current angle in 10th degrees */
   uint16_t          vAnglePhB;        /* Phase A voltage angle in 10th degrees */
   uint16_t          iAnglePhC;        /* Phase A current angle in 10th degrees */
   uint16_t          vAnglePhC;        /* Phase A voltage angle in 10th degrees */

   uint16_t          iMagnitudePhA;    /* Phase A current magnitude */
   uint16_t          vMagnitudePhA;    /* Phase A voltage magnitude */
   uint16_t          iMagnitudePhB;    /* Phase A current magnitude */
   uint16_t          vMagnitudePhB;    /* Phase A voltage magnitude */
   uint16_t          iMagnitudePhC;    /* Phase A current magnitude */
   uint16_t          vMagnitudePhC;    /* Phase A voltage magnitude */

   uint8_t           duPF;             /* Distortion power factor   */

   /* Diagnostic counters   */
   uint8_t           diag1;            /* Count of number of occurrences of diagnostic 1           */
   uint8_t           diag2;            /* Count of number of occurrences of diagnostic 2           */
   uint8_t           diag3;            /* Count of number of occurrences of diagnostic 3           */
   uint8_t           diag4;            /* Count of number of occurrences of diagnostic 4           */
   uint8_t           diag5phA;         /* Count of number of occurrences of diagnostic 5 phase A   */
   uint8_t           diag5phB;         /* Count of number of occurrences of diagnostic 5 phase B   */
   uint8_t           diag5phC;         /* Count of number of occurrences of diagnostic 5 phase C   */
   uint8_t           diag5Total;       /* Count of number of occurrences of diagnostic 5 all phases*/
   uint8_t           diag6;            /* Count of number of occurrences of diagnostic 6           */
   uint8_t           diag7;            /* Count of number of occurrences of diagnostic 7           */
   uint8_t           diag8;            /* Count of number of occurrences of diagnostic 8           */

   /* Diagnostic cautions   */
   struct
   {
      uint8_t        diag1Bit: 1;      /* Indicates occurrence of diagnostic 1.  */
      uint8_t        diag2Bit: 1;      /* Indicates occurrence of diagnostic 2.  */
      uint8_t        diag3Bit: 1;      /* Indicates occurrence of diagnostic 3.  */
      uint8_t        diag4Bit: 1;      /* Indicates occurrence of diagnostic 4.  */
      uint8_t        diag5Bit: 1;      /* Indicates occurrence of diagnostic 5.  */
      uint8_t        diag6Bit: 1;      /* Indicates occurrence of diagnostic 6.  */
      uint8_t        diag7Bit: 1;      /* Indicates occurrence of diagnostic 7.  */
      uint8_t        diag8Bit: 1;      /* Indicates occurrence of diagnostic 8.  */
   } diagCautions;

} mfgTbl72_t;  /* Standard Table #3 - End Device Mode and Status */
PACK_END
/* End - Manufacturer Table 72  */


/*****************************************************/
/* Begin - Manufacturer Table 78                     */
/*****************************************************/
typedef struct /* Not PACKed so the usage in hdFileIndex_t remains aligned the same as previous releases (no DFW impact that way) */
{
   uint8_t               dtLastCalibrated[5];
   uint8_t                calibratorName[10];
   uint8_t               dtLastProgrammed[5];
   uint8_t                programmerName[10];
   uint16_t          numberOfTimesProgrammed;
} meterProgInfo_t;   // Created struct for convenience

PACK_BEGIN
typedef struct PACK_MID
{
   meterProgInfo_t               mtrProgInfo;   //Changed from R&P table definition for convenience
   uint8_t              numberOfDemandResets;
   uint8_t              dtLastDemandReset[5];
   uint16_t       nbrOfcommunicationSessions;
   uint8_t     dtLastCommunicationSession[5];
   uint16_t                nbrOfBadPasswords;
   uint16_t                    nbrRtpEntries;
   uint8_t                 dtLastRtpEntry[5];
   uint32_t            cumPowerOutageSeconds;
   uint16_t                nbrOfPowerOutages;
   uint8_t              dtLastPowerOutage[5];
   uint8_t              nbrOfEepromWrites[3];
   uint8_t                dtLastTlcUpdate[5];
   uint8_t                       tlcName[10];
   uint8_t       dtLastTransfIncaccAdjsut[5];
   uint8_t         transfInaccAdjustName[10];
   uint8_t               dtLastTimeChange[5];
   uint16_t              nbrSustainedMinutes;
   uint16_t           nbrSustainedInterrupts;
   uint16_t           nbrMomentaryInterrupts;
   uint16_t               nbrMomentaryEvents;
   uint8_t         dtLastAmrCommunication[5];
} mfgTbl78_t;  /* Manufacturing Table 78 - Security Log */
PACK_END
/*****************************************************/
/* End - Manufacturer Table 78 Description           */
/*****************************************************/


/* Begin - Manufacturer Table 97 */
PACK_BEGIN
typedef struct
{
   uint8_t  statusEnable:     1;    /* Enable Cycle Insensitive Rolling Status */
   uint8_t  demandEnable:     1;    /* Enable Cycle Insensitive Demand */
   uint8_t  businessEnable:   1;    /* Enable business day search */
   uint8_t  filler:           5;    /* not used*/

} MT97configFlags_t;
PACK_END

PACK_BEGIN
typedef struct
{
   MT97configFlags_t       configFlags;
   uint8_t                 statusDays;       /* Number of calendar day's worth of status that is used to compute Rolling Status */
   uint8_t                 peak1Days;        /* Number of days to include in search for Peak 1 */
   uint8_t                 peak2Days;        /* Number of days to include in search for Peak 2 */

} mfgTbl97_t;  /* Manufacturer Table 97: Cycle Insensitive/ Rolling Billing Demand Config*/
PACK_END
/* End - Manufacturer Table 97  */

/* Begin - Manufacturer Table 98*/
PACK_BEGIN
typedef struct
{
   uint8_t year:                 7;    /* 7 least significant bits */
   uint8_t nonBusinessDayFlag:   1;    /* Indicates whether entry is considered a business day or not.
                                          (1 = Non-business day, 0 = business day) Most Significant Bit */
   uint8_t month;
   uint8_t day;

} MT98Date_t;
PACK_END
typedef struct
{
   uint8_t hours;
   uint8_t minute;
} MT98Time_t;
#if ( HMC_I210_PLUS_C == 1 )
PACK_BEGIN
// Note: For KV2c the structure is different
typedef struct
{
   uint32_t    demand;     /* Daily Max Demand */
   MT98Time_t  time;       /* Time of the Demand */
} MT98Demand_t;
PACK_END
#endif

PACK_BEGIN
typedef struct
{
   MT98Demand_t   demands[MT98_NBR_DEMANDS];  /* Array of 2 quantities */

} MT98totDataBlock_t; /* TOT_DATA_BLOCK in Field in MT 98 */
PACK_END

PACK_BEGIN
typedef struct
{
   MT98Date_t           date;                   /* Date entry YY MM DD */
   statusRcd_t          status;                 /* MT 69 STATUS_RCD */
   MT98totDataBlock_t   totDataBlock;
   MT98totDataBlock_t   tiers[MT98_NBR_TIERS];  /* Array of 2 tiers(A & B only) */

} MT98Entries_t;  // ENTRIES Field in MT 98
PACK_END

PACK_BEGIN
typedef struct
{
   uint8_t     order:                1;         /* Indicates whether log is ascending or descending order(0 = ascending) */
   uint8_t     overflowFlag:         1;         /* See R&P guide */
   uint8_t     listType:             1;         /* Indicates if log is stored as FIFO or circular list ( Always, 1 = Circular List) */
   uint8_t     inhibitOverflowFlag:  1;         /* Indicates if the device is inhibiting recording of events which would cause overflow (0 = False, 1 = True) */
   uint8_t     filler:               4;         /* Not used */

} MT98DayFlags_t;  // Day_Flags Field in MT98
PACK_END
// This structure is created to ease the process of caluculating the size and offset
PACK_BEGIN
typedef struct
{
   uint8_t  nbrOfValidEntries;     /* Number of valid entries in the table. 0 means the table is empty. */
   uint8_t  lastEntryElement;      /* Array element number of the newest table entry. Value: 0 to 34. */
}MT98ElementEntry_t;
PACK_END

PACK_BEGIN
typedef struct
{
   MT98DayFlags_t      dayFlags;
   MT98ElementEntry_t  ElementEntry;                    /* nbrOfValidEntries and lastEntryElement fields in MT #98 */
   MT98Entries_t       entries[MT98_NBR_OF_ENTRIES];    /* Array of maximum of 35 entries. */
   MT98Entries_t       todaysEntry;                     /* Cycle Insensitive Values since midnight */

} mfgTbl98_t; /* Manufacturer Table 98: Cycle Insensitive/ Rolling Billing Demand Data*/
PACK_END
// Note: This creates a big Structure of 1551 Bytes
/* End - Manufacturer Table 98  */

/* Begin - Manufacturer Table 112  */
PACK_BEGIN
typedef struct
{
   uint8_t order           : 1;  /* 0->ascending, 1->descending   */
   uint8_t overflow        : 1;  /* 0->log hasn't overflowed; 1->log overflowed */
   uint8_t listType        : 1;  /* 0->FIFO, 1->circular list  */
   uint8_t inhibitOverflow : 1;  /* 0->overflow not inhibited, 1->overflow inhibited   */
   uint8_t fill            : 4;  /* not used */
} mfgTbl112EventFlags_t;
PACK_END

PACK_BEGIN
typedef struct
{
   uint8_t  eventTime[ 6 ];
   uint16_t eventSeqNbr;
   uint8_t  eventType;
   uint16_t eventDuration;
   uint16_t voltagePhA;
   uint16_t voltagePhB;
   uint16_t voltagePhC;
   uint16_t currentPhA;
   uint16_t currentPhB;
   uint16_t currentPhC;
} sagSwellEntry_t, *pSagSwellEntry_t;
PACK_END

PACK_BEGIN
typedef struct
{
   mfgTbl112EventFlags_t   eventFlags;
   uint16_t                nbrValidEntries;
   uint16_t                lastEntry;
   uint32_t                lastEntrySeqNbr;
   uint16_t                nbrUnread;
   uint16_t                sagCounter;
   uint16_t                swellCounter;
   sagSwellEntry_t         eventArray[];
} mfgTbl112_t, *pMfgTbl112_t;
PACK_END
/* End - Manufacturer Table 112  */


typedef struct
{
   uint16_t totalData;  // reading type value for initial non tiered data block
   uint16_t     tierA;  // reading type value for tier A data block
   uint16_t     tierB;  // reading type value for tier B data block
   uint16_t     tierC;  // reading type value for tier C data block
   uint16_t     tierD;  // reading type value for tier D data block
}DataBlock_t;

typedef struct
{
   uint32_t                   uom;  // The 4 byte ST12 value that represents or differentiates between each meter measurement
   uint16_t           dataControl;  // The 2 byte ST14 value that futher defines a unit of measurement
   DataBlock_t        presentData;  // Block of Tiers to represent the present data
   DataBlock_t       previousData;  // Block of Tiers to represent the previous data
}uomLookupEntry_t;

typedef struct
{
   uint16_t     oneMin;
   uint16_t    fiveMin;
   uint16_t fifteenMin;
   uint16_t  thirtyMin;
   uint16_t   sixtyMin;
}LpDataBlock_t;


typedef struct
{
   uint32_t                    uom;
   uint16_t            dataControl;
   LpDataBlock_t    lpReadingTypes;
}uomLpLookupEntry_t;


// this structure will be used to represent each item in hmc direct lookup table
typedef struct{
   meterReadingType              quantity;
   ReadingsValueTypecast         typecast;
   tableLoc_t                         tbl;
}directLookup_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */
/* Tables requiring a password on read
      ST 12, 23, 61, 63       //Read of ST12 only occurs on power up or LP config error.  61 & 63 only occur if LP enabled
*/
static const uint16_t rqdPrivTables[]
=
{
   STD_TBL_UNITS_OF_MEASURE_ENTRY,
   STD_TBL_CURRENT_REGISTER_DATA,
   STD_TBL_ACTUAL_LOAD_PROFILE_LIMITING,
   STD_TBL_LOAD_PROFILE_STATUS
};

ANSI_SIG_EXTERN
const stdTbl76Lookup_t logEvents[57]
#ifdef ANSI_SIG_GLOBALS
=
{
/*   event  Std/  selector    EDEventType,                                                   mask, priority
       # ,  Mfg   (NotUsed),
           flag,                                                                                                */
   { {  1,    0,     0    },  electricMeterPowerFailed,                                         0,  88 },
   { {  2,    0,     0    },  electricMeterPowerRestored,                                       0,  88 },
   { {  3,    0,     0    },  electricMeterclocktimechanged,                                    0,  88 },
   { {  7,    0,     0    },  electricMetersecurityaccessuploaded,                              0, 100 },
   { {  8,    0,     0    },  electricMetersecurityaccesswritten,                               0, 120 },
   { { 11,    0,     0    },  electricMeterConfigurationProgramChanged,                         0, 190 },
   { { 20,    0,     0    },  electricMeterDemandResetDatalogOccurred,                          0, 160 },
   { { 21,    0,     0    },  electricMetermetrologyselfReadsucceeded,                          0,  85 },
   { { 32,    0,     0    },  electricMetermetrologyTestModestarted,                            0, 110 },
   { { 33,    0,     0    },  electricMetermetrologyTestModestopped,                            0, 110 },
   { { 10,    1,     0    },  electricMeterPowerPhaseAVoltageSagStarted,                        1,  87 },
   { { 11,    1,     0    },  electricMeterPowerPhaseAVoltageSagStopped,                        0,  87 }, // I210+c does not return a mask
   { { 11,    1,     0    },  electricMeterPowerPhaseAVoltageSagStopped,                        1,  87 },
   { { 10,    1,     0    },  electricMeterPowerPhaseCVoltageSagStarted,                        4,  87 },
   { { 11,    1,     0    },  electricMeterPowerPhaseCVoltageSagStopped,                        4,  87 },
   { { 12,    1,     0    },  electricMeterPowerPhaseAVoltageSwellStarted,                      1,  87 },
   { { 13,    1,     0    },  electricMeterPowerPhaseAVoltageSwellStopped,                      0,  87 }, // I210+c does not return a mask
   { { 13,    1,     0    },  electricMeterPowerPhaseAVoltageSwellStopped,                      1,  87 },
   { { 12,    1,     0    },  electricMeterPowerPhaseCVoltageSwellStarted,                      4,  87 },
   { { 13,    1,     0    },  electricMeterPowerPhaseCVoltageSwellStopped,                      4,  87 },
   { { 16,    1,     0    },  electricMeterPowerVoltageMinLimitReached,                         1, 109 },
   { { 17,    1,     0    },  electricMeterPowerVoltageMinLimitCleared,                         0, 109 },
   { { 18,    1,     0    },  electricMeterDemandOverflow,                                      0, 176 },
   { { 19,    1,     0    },  electricMeterDemandNormal,                                        0,  17 },
   { { 20,    1,     0    },  electricMeterSecurityRotationReversed,                            0,  90 },  //Caution 400000-Received kWh
   { { 21,    1,     0    },  electricMeterSecurityRotationNormal,                              0, 100 },
   { { 22,    1,     0    },  electricMeterPowerReactivePowerReversed,                          0, 102 },
   { { 23,    1,     0    },  electricMeterPowerReactivePowerNormal,                            0, 102 },
   { { 24,    1,     0    },  electricMeterBillingRTPActivated,                                 0, 100 },
   { { 25,    1,     0    },  electricMeterBillingRTPDeactivated,                               0, 100 },
   { { 28,    1,     0    },  electricMeterConfigurationCalibrationstarted,                     0, 120 },
   { { 33,    1,     0    },  electricMeterpowervoltagefrozen,                                  0, 110 },
   { { 34,    1,     0    },  electricMeterpowervoltagereleased,                                0, 109 },
   { { 35,    1,     0    },  electricMeterloadControlEmergencySupplyCapacityLimitEventStarted, 0, 130 },
   { { 36,    1,     0    },  electricMeterloadControlEmergencySupplyCapacityLimitEventStopped, 0, 110 },
   { { 37,    1,     0    },  electricMeterloadControlSupplyCapacityLimitEventStarted,          0, 105 },
   { { 38,    1,     0    },  electricMeterloadControlSupplyCapacityLimitEventStopped,          0, 100 },
   { { 39,    1,     0    },  electricMeterRcdSwitchDataLogDisconnected,                        1, 250 },
   { { 39,    1,     0    },  electricMeterRCDSwitchEmergencySupplyCapacityLimitDisconnected,   2, 108 },
   { { 39,    1,     0    },  electricMeterRCDSwitchSupplyCapacityLimitDisconnected,            4, 108 },
   { { 39,    1,     0    },  electricMeterRCDSwitchPrepaymentCreditExpired,                    8, 120 },
   { { 40,    1,     0    },  electricMeterRcdSwitchDataLogConnected,                           1, 250 },
   { { 41,    1,     0    },  electricMeterSecurityTilted,                                      0, 105 },  //Tamper Detect (Meter Inversion)
   { { 56,    1,     0    },  electricMeterTemperatureThresholdMaxLimitReached,                 0, 254 },
   { { 57,    1,     0    },  electricMeterTemperatureThresholdMaxLimitCleared,                 0, 180 },
   { { 58,    1,     0    },  electricMeterSecurityPasswordInvalid,                             0,  92 },  //Bad Security Code Lockout Set
   { { 59,    1,     0    },  electricMeterSecurityPasswordUnlocked,                            0,  92 },  //Bad Security Code Lockout Cleared
   { { 60,    1,     0    },  electricMeterIODisabled,                                          0,  91 },  //Optical Port Communication Disabled
   { { 61,    1,     0    },  electricMeterIOEnabled,                                           0,  91 },  //Optical Port Communication Enabled
   { { 66,    1,     0    },  electricMeterPowerCurrentMaxLimitReached,                         0, 100 },
   { { 67,    1,     0    },  electricMeterPowerCurrentMaxLimitCleared,                         0, 100 },
   { { 68,    1,     0    },  electricMeterPowerCurrentImbalanced,                              0, 102 },
   { { 69,    1,     0    },  electricMeterPowerCurrentImbalanceCleared,                        0, 102 },
   { { 70,    1,     0    },  electricMeterSecurityMagneticSwitchTamperDetected,                0, 105 },
   { { 71,    1,     0    },  electricMeterSecurityMagneticSwitchTamperCleared,                 0, 105 },
   { { 72,    1,     0    },  electricMeterPowerError,                                          0,  95 },
   { { 73,    1,     0    },  electricMeterPowerErrorCleared,                                   0,  95 }
}
#endif
;

/*lint -e{64,65,133}  Lint doesn't like this initialization! */
#ifdef ANSI_SIG_GLOBALS

const directLookup_t directLookupTable[] =
{
   {
      .quantity                        = disconnectCapable,             /* Meter has disconnect capability  */
      .typecast                        = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LOAD_CONTROL_CONFIG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 0,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 7,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = switchPositionStatus,          /* Switch open/close status   */
      .typecast                        = uintValue,                     // TODO: K24 Bob: changed from Boolean to uintValue on RA6
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LOAD_CONTROL_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 4,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = nominalSwitchPositionStatus,       /* The desired switch status   */
      .typecast                        = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LOAD_CONTROL_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 4,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 1,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = netkW,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_POWER_QUALITY_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 6,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 4 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 3,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = vRMSA,                         /* Phase A RMS voltage  */
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_POWER_QUALITY_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 0,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = vRMSC,                         /* Phase C RMS voltage  */
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_POWER_QUALITY_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 2,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = iA,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_POWER_QUALITY_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 10,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = iC,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_POWER_QUALITY_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 12,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = pF,                /* Unit of Measure */
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_POWER_QUALITY_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 4,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 2,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   { // TODO: RA6E1 Bob: Added from kV2c version of this table; confirm that this is the right thing to do.
      .quantity                        = edModel,                /* Unit of Measure */
      .typecast                        = ASCIIStringValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_GENERAL_MANUFACTURE_ID ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 0x04,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 8 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = highThresholdswell,   /* Unit of Measure */
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_VOLTAGE_EVENT_MONITOR_CONFIG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 2,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = lowThresholdsag,   /* Unit of Measure */
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_VOLTAGE_EVENT_MONITOR_CONFIG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 1,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = lowThresholdDuration,   /* Unit of Measure */
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_VOLTAGE_EVENT_MONITOR_CONFIG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 3,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = energization,                  /* Number power outages detected by meter */
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_SECURITY_LOG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl78_t, nbrOfPowerOutages ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   { // TODO: RA6E1 Bob: Added from the kV2c version of this file; confirm that this is the right thing to do.
      .quantity                        = edMfgSerialNumber,
      .typecast                        = ASCIIStringValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_GENERAL_MANUFACTURE_ID ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 16,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 16 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   { // TODO: RA6E1 Bob: Added from the kV2c version of this file; confirm that this is the right thing to do.
      .quantity                        = ansiTableOID,
      .typecast                        = hexBinary,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_GENERAL_CONFIGURATION ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 3,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 4 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = edProgramID,                   /* Meter Program ID */
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PROGRAM_CONSTANTS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 154,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = edProgrammerName,            /* Meter's date and time   */
      .typecast                        = ASCIIStringValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_SECURITY_LOG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl78_t, mtrProgInfo.programmerName ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( ED_PROGRAMMER_NAME_LENTGH ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterProgramLostState,         /* Meter detects program loss */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edMfgStatus1 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 5,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = reverseReactivePowerflowStatus,         /* Interpreted as Leading kVArs   */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edMfgStatus1  ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 4,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterSecurityRotationReversed,    /* Meter detects inversion tamper  */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS  ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edStdStatus1Bfld.bytes[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 4,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterCurrentMaxLimitReached, /* Meter's max current limit reached   */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edStdStatus1Bfld.bytes[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 7,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterDemandOverflow,           /* Demand overflow occurred flag   */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edStdStatus1Bfld.bytes[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 2,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterVoltageLossDetected, /* Meter detects voltage loss */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edStdStatus1Bfld.bytes[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 1,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterDiagRomMemoryErrorStatus, /* Meter detect ROM error  */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edStdStatus1Bfld  ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 4,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterBatteryStatus,            /* Meter detects low battery voltage */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edStdStatus1Bfld.bytes[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,  /* Battery low flag  */
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterTempThreshMaxLimitReached,   /* Meter detects high temp alert */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edMfgStatus1 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 6,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterDiagProcessorErrorStatus, /* Any Processor diagnostic error   */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_ERROR_HISTORY ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl69_t, realTimeStatus.errorRcd.byte ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 8,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterDiagFlashMemoryErrorStatus,
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_ERROR_HISTORY ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl69_t, eepromError1 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 8,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterError,
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edMfgStatus1 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 2,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterDiagRamMemoryErrorStatus, /* Meter detects RAM error */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edStdStatus1Bfld ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 3,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = highThreshIndDeviceTemp,       /* Meter's high temperature threshold  */
      .typecast                         = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_ERROR_CAUTION ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl68_t, highTempThreshold ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = sag,                           /* Count of sag events  */
      .typecast                         = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_VOLTAGE_EVENT_MONITOR_LOG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 11,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = swell,                         /* Count of swell events  */
      .typecast                         = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_VOLTAGE_EVENT_MONITOR_LOG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 13,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = edTemperature,                   /* Meter's current temperature */
      .typecast                         = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_POWER_QUALITY_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 5,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterSagPhAStarted,  /* Meter detects a sag on phase A */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCautions ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 5,  /* diagnostic 6   */
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterSwellPhAStarted,    /* Meter detects a swell on phase A */
      .typecast                         = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCautions ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 6,  /* diagnostic 7   */
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = meterProgramCount,             /* Number of times meter has been programmed */
      .typecast                         = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_SECURITY_LOG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl78_t, mtrProgInfo.numberOfTimesProgrammed ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT64,
      .tbl.pFunct                      = NULL
   }
};

/*lint -e{64,65,133}  Lint doesn't like this initialization! */
const quantitySig_t quantitySigLP[] =
{
   {
      .quantity                        =  vRMSA,
      .eAccess                         =  eANSI_MTR_LOOKUP,
      .sig.location                    =  eCURRENT_DATA,
      .sig.wild.q1                     =  MATCH,
      .sig.wild.q2                     =  MATCH,
      .sig.wild.q3                     =  MATCH,
      .sig.wild.q4                     =  MATCH,
      .sig.wild.netFlowAcnt            =  MATCH,
      .sig.wild.segmentation           =  MATCH,
      .sig.wild.harmonic               =  WILD,
      .sig.wild.rsvd                   =  0,
      .sig.multCfg.applyScalars        =  0,
      .sig.multCfg.multiplierOveride   =  1,
      .sig.tbl12.idCode                =  eRmsVolts,
      .sig.tbl12.timeBase              =  ePeriodBased,
      .sig.tbl12.multiplier            =  0,
      .sig.tbl12.q1Accountability      =  0,
      .sig.tbl12.q2Accountability      =  0,
      .sig.tbl12.q3Accountability      =  0,
      .sig.tbl12.q4Accountability      =  0,
      .sig.tbl12.netFlowAccountability =  0,
      .sig.tbl12.segmentation          =  ePHASE_AN,
      .sig.tbl12.harmonic              =  0,
      .sig.tbl12.nfs                   =  1,
      .sig.tbl14Byte0                  =  ACLARA_STDTBL14_SNAP,
      .sig.tbl14Byte1                  =  0
   },
   {
      .quantity                        =  iA,
      .eAccess                         =  eANSI_MTR_LOOKUP,
      .sig.location                    =  eCURRENT_DATA,
      .sig.wild.q1                     =  MATCH,
      .sig.wild.q2                     =  MATCH,
      .sig.wild.q3                     =  MATCH,
      .sig.wild.q4                     =  MATCH,
      .sig.wild.netFlowAcnt            =  MATCH,
      .sig.wild.segmentation           =  MATCH,
      .sig.wild.harmonic               =  WILD,
      .sig.wild.rsvd                   =  0,
      .sig.multCfg.applyScalars        =  0,
      .sig.multCfg.multiplierOveride   =  1,
      .sig.tbl12.idCode                =  eRmsAmps,
      .sig.tbl12.timeBase              =  ePeriodBased,
      .sig.tbl12.multiplier            =  0,
      .sig.tbl12.q1Accountability      =  0,
      .sig.tbl12.q2Accountability      =  0,
      .sig.tbl12.q3Accountability      =  0,
      .sig.tbl12.q4Accountability      =  0,
      .sig.tbl12.netFlowAccountability =  0,
      .sig.tbl12.segmentation          =  ePHASE_AN,
      .sig.tbl12.harmonic              =  0,
      .sig.tbl12.nfs                   =  1,
      .sig.tbl14Byte0                  =  ACLARA_STDTBL14_SNAP,
      .sig.tbl14Byte1                  =  0
   },
   {
      .quantity                        =  iC,
      .eAccess                         =  eANSI_MTR_LOOKUP,
      .sig.location                    =  eCURRENT_DATA,
      .sig.wild.q1                     =  MATCH,
      .sig.wild.q2                     =  MATCH,
      .sig.wild.q3                     =  MATCH,
      .sig.wild.q4                     =  MATCH,
      .sig.wild.netFlowAcnt            =  MATCH,
      .sig.wild.segmentation           =  MATCH,
      .sig.wild.harmonic               =  WILD,
      .sig.wild.rsvd                   =  0,
      .sig.multCfg.applyScalars        =  0,
      .sig.multCfg.multiplierOveride   =  1,
      .sig.tbl12.idCode                =  eRmsAmps,
      .sig.tbl12.timeBase              =  ePeriodBased,
      .sig.tbl12.multiplier            =  0,
      .sig.tbl12.q1Accountability      =  0,
      .sig.tbl12.q2Accountability      =  0,
      .sig.tbl12.q3Accountability      =  0,
      .sig.tbl12.q4Accountability      =  0,
      .sig.tbl12.netFlowAccountability =  0,
      .sig.tbl12.segmentation          =  ePHASE_CN,
      .sig.tbl12.harmonic              =  0,
      .sig.tbl12.nfs                   =  1,
      .sig.tbl14Byte0                  =  ACLARA_STDTBL14_SNAP,
      .sig.tbl14Byte1                  =  0
   },
   {
      .quantity                        =  avgIndTempC,
      .eAccess                         =  eANSI_MTR_LOOKUP,
      .sig.location                    =  eCURRENT_DATA,
      .sig.wild.q1                     =  MATCH,
      .sig.wild.q2                     =  MATCH,
      .sig.wild.q3                     =  MATCH,
      .sig.wild.q4                     =  MATCH,
      .sig.wild.netFlowAcnt            =  MATCH,
      .sig.wild.segmentation           =  MATCH,
      .sig.wild.harmonic               =  WILD,
      .sig.wild.rsvd                   =  0,
      .sig.multCfg.applyScalars        =  0,
      .sig.multCfg.multiplierOveride   =  0,
      .sig.tbl12.idCode                =  eTempDegreesC,
      .sig.tbl12.timeBase              =  ePeriodBased,
      .sig.tbl12.multiplier            =  0,
      .sig.tbl12.q1Accountability      =  0,
      .sig.tbl12.q2Accountability      =  0,
      .sig.tbl12.q3Accountability      =  0,
      .sig.tbl12.q4Accountability      =  0,
      .sig.tbl12.netFlowAccountability =  0,
      .sig.tbl12.segmentation          =  eALL_PHASES,
      .sig.tbl12.harmonic              =  0,
      .sig.tbl12.nfs                   =  1,
      .sig.tbl14Byte0                  =  ACLARA_STDTBL14_AVG,
      .sig.tbl14Byte1                  =  0
   }
};

static const OL_tTableDef quantitySigTblLP_                                     =
{
   ( tElement  * )&quantitySigLP[0].quantity,           /* Points to the 1st element in the register table. */
   ARRAY_IDX_CNT( quantitySigLP )                       /* Number of registers defined for the time module. */
};

static const OL_tTableDef dircectLookupTbl_                                    =
{
   ( tElement  * )&directLookupTable[0].quantity,
   ARRAY_IDX_CNT( directLookupTable )
};

#endif

 // lookup table for summation measurements
static const uomLookupEntry_t masterSummationLookup [] =
{  //   ST12      ST14      pres   presA   presB   presC   presD    prev   prevA   prevB   prevC   prevD
   { 0x00024000, 0x0000,     12,     57,     58,     59,    139,    578,    587,    596,    605,    614 },  // Wh Q1+Q4 fund + harmonics
   { 0x00018000, 0x0000,     15,    143,    145,    147,    149,    579,    588,    597,    606,    615 },  // Wh Q2+Q3 fund + harmonics
   { 0x8003C002, 0x0000,     17,    429,    430,    431,    432,    586,    595,    604,    613,    622 },  // total Energy VAh (Q1+Q4)+(Q2+Q3)
   { 0x0003C000, 0x0000,     19,     60,     61,     62,    242,    580,    589,    598,    607,    616 },  // Wh (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x0007C000, 0x0000,     20,    105,    106,    107,    108,    581,    590,    599,    608,    617 },  // Wh (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x0000C001, 0x0000,     13,    136,    137,    138,    140,    582,    591,    600,    609,    618 },  // varh Q1+Q2 fund + harmonics
   { 0x00030001, 0x0000,     16,    144,    146,    148,    150,    583,    592,    601,    610,    619 },  // varh Q3+Q4 fund + harmonics
   { 0x0003C001, 0x0000,    247,    243,    244,    245,    246,    584,    593,    602,    611,    620 },  // varh (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x0007C001, 0x0000,     21,    103,    102,    101,    104,    585,    594,    603,    612,    621 },  // varh (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x0003C003, 0x0000,   3338,   3771,   4201,   4631,   5061,   3552,   3986,   4416,   4846,   5276 }   // Phasor Apparent VAh, fund + harmonics (read)
};


 // lookup table for maximum demand measurements
static const uomLookupEntry_t masterMaxDemandLookup [] =
{  //   ST12      ST14      pres   presA   presB   presC   presD    prev   prevA   prevB   prevC   prevD
   { 0x00024400, 0x0000,     45,    334,    335,    336,    337,     88,     48,     50,     93,    169 },  // W Q1+Q4 fund + harmonics
   { 0x00018400, 0x0000,    183,    338,    339,    340,    341,    188,    173,    175,    177,    179 },  // W Q2+Q3 fund + harmonics
   { 0x0003C400, 0x0000,    253,    346,    347,    344,    349,     55,     97,     98,     99,     96 },  // W (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x0007C400, 0x0000,    250,    342,    343,    348,    345,     56,     89,     90,     91,     92 },  // W (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x0000C401, 0x0000,     47,    201,    449,    454,    459,    187,     49,     51,    168,    170 },  // var Q1+Q2 fund + harmonics
   { 0x00030401, 0x0000,    228,    445,    450,    455,    460,    229,    174,    176,    178,    180 },  // var Q3+Q4 fund + harmonics
   { 0x0003C401, 0x0000,    254,    446,    451,    456,    461,    257,    238,    239,    240,    241 },  // var (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x0007C401, 0x0000,    252,    447,    452,    457,    462,    256,    234,    235,    236,    237 },  // var (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x0003C403, 0x0000,   7544,   5491,   6005,   6519,   7033,   7796,   5748,   6262,   6776,   7290 },  // Phasor Apparent VA, fund + harmonics
   { 0x8003C402, 0x0000,    230,    448,    453,    458,    463,    231,    464,    465,    466,    467 }   // Apparent VA (Q1+Q4)+(Q2+Q3)
};


 // lookup table for cumulative demand measurements
static const uomLookupEntry_t masterCumulativeDemandLookup [] =
{  //   ST12      ST14      pres   presA   presB   presC   presD    prev   prevA   prevB   prevC   prevD
   { 0x00024400, 0x0000,    468,    477,    486,    495,    504,    513,    522,    531,    540,    549 },  // W Q1+Q4 fund + harmonics
   { 0x00018400, 0x0000,    469,    478,    487,    496,    505,    514,    523,    532,    541,    550 },  // W Q2+Q3 fund + harmonics
   { 0x0003C400, 0x0000,    470,    479,    488,    497,    506,    515,    524,    533,    542,    551 },  // W (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x0007C400, 0x0000,    471,    480,    489,    498,    507,    516,    525,    534,    543,    552 },  // W (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x0000C401, 0x0000,    472,    481,    490,    499,    508,    517,    526,    535,    544,    553 },  // var Q1+Q2 fund + harmonics
   { 0x00030401, 0x0000,    473,    482,    491,    500,    509,    518,    527,    536,    545,    554 },  // var Q3+Q4 fund + harmonics
   { 0x0003C401, 0x0000,    474,    483,    492,    501,    510,    519,    528,    537,    546,    555 },  // var (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x0007C401, 0x0000,    475,    484,    493,    502,    511,    520,    529,    538,    547,    556 },  // var (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x8003C402, 0x0000,    476,    485,    494,    503,    512,    521,    530,    539,    548,    557 },  // Cumulative Total VA (Q1+Q4)+(Q2+Q3)
   { 0x0003C403, 0x0000,   8053,   8453,   8853,   9253,   9653,   8253,   8653,   9053,   9453,   9853 }   // Phasor Apparent VA, fund + harmonics
};


static const uomLpLookupEntry_t uomLoadProfileLookupTable [] =
{ //   ST12      ST14    1Min    5Min   15Min    30Min  60Min
   { 0x00024500, 0x0000,  357,     73,     63,    119,    110 },  //Wh Q1+Q4 fund + harmonics (LP)
   { 0x00018500, 0x0000,  358,     75,     66,    122,    113 },  //Wh Q2+Q3 fund + harmonics (LP)
   { 0x0007C500, 0x0000,  359,     79,     70,    126,    117 },  //Wh (Q1+Q4)-(Q2+Q3) fund + harmonics (LP)
   { 0x0003C500, 0x0000,  360,     78,     69,    125,    116 },  //Wh (Q1+Q4)+(Q2+Q3) fund + harmonics (LP)
   { 0x0000C501, 0x0000,  361,     72,     64,    120,    111 },  //varh Q1+Q2 fund + harmonics (LP)
   { 0x00030501, 0x0000,  362,     76,     67,    123,    114 },  //varh Q3+Q4 fund + harmonics (LP)
   { 0x0007C501, 0x0000,  363,     80,     71,    127,    118 },  //varh (Q1+Q2)-(Q3+Q4) fund + harmonics (LP)
   { 0x0003C501, 0x0000,  364,    353,    352,    351,    350 },  //varh (Q1+Q2)+(Q3+Q4) fund + harmonics (LP)
   { 0x8003C502, 0x0000,  373,    132,    133,    134,    135 },  //Apparent VAh (Q1+Q4)+(Q2+Q3) (LP)
   { 0x80280208, 0x0001,  383,    382,    381,    380,    379 },  // VA fund + harmonics MAX
   { 0x80280208, 0x0002,  388,    387,    386,    385,    384 },  // VA fund + harmonics MIN
   { 0x80280208, 0x0003,    0,     34,     34,     34,     34 },  // VA fund + harmonics SNAPSHOT
   { 0x80300208, 0x0003,    0,     36,     36,     36,     36 },  // VB fund + harmonics SNAPSHOT
   { 0x80380208, 0x0001,  444,    443,    442,    441,     440},  // VC fund + harmonics MAX
   { 0x80380208, 0x0002,  439,    438,    437,    436,     435},  // VC fund + harmonics MIN
   { 0x80380208, 0x0003,    0,     35,     35,     35,     35 },  // VC fund + harmonics SNAPSHOT
   { 0x80380209, 0x0004,  376,    165,    162,    433,    434 },  //Average VC fund + harmonics
   { 0x80300209, 0x0004,  377,    166,    163,   3059,   3058 },  //Average VB fund + harmonics
   { 0x80280209, 0x0004,  378,    164,    161,    375,    374 },  //Average VA fund + harmonics
   { 0x80000246, 0x0004,  370,    369,    368,    367,    366 },  //Temperature Avg
   { 0x80000246, 0x0001,  393,    392,    391,    390,    389 },  //Temperature Max
   { 0x80000246, 0x0002,  398,    397,    396,    395,    394 },  //Temperature Min
   { 0x80000246, 0x0003,    0,     32,     32,     32,     32 },  //Temperature
   { 0x8028020C, 0x0003,    0,     25,     25,     25,     25 },  //IA fund + harmonics SNAPSHOT
   { 0x8028020C, 0x0001,    0,    402,    401,    400,    399 },  //IA fund + harmonics MAX
   { 0x8028020C, 0x0002,    0,    407,    406,    405,    404 },  //IA fund + harmonics MIN
   { 0x8028020D, 0x0004,  413,    412,    411,    410,    409 },  //Average IA fund + harmonics
   { 0x8038020C, 0x0001,    0,    417,    416,    415,    414 },  // IC fund + harmonics MAX
   { 0x8038020C, 0x0002,    0,    422,    421,    420,    419 },  // IC fund + harmonics MIN
   { 0x8038020C, 0x0003,    0,     27,     27,     27,     27 },  // IC fund + harmonics SNAPSHOT
   { 0x8038020D, 0x0004,  428,    427,    426,    425,    424 },  //Average IC fund + harmonics
   { 0x00424500, 0x0000, 2313,     74,     65,    121,    112 },  //Wh Q1+Q4 fund only (LP)
   { 0x00418500, 0x0000, 2318,   2317,   2316,   2315,   2314 },  //Wh Q2+Q3 fund only (LP)
   { 0x0043C500, 0x0000, 2323,   2322,   2321,   2320,   2319 },  //Wh (Q1+Q4)+(Q2+Q3) fund only (LP)
   { 0x0047C500, 0x0000, 2328,   2327,   2326,   2325,   2324 },  //Wh (Q1+Q4)-(Q2+Q3) fund only (LP)
   { 0x00004501, 0x0000, 2569,    269,    273,    277,    281 },  //varh Q1 fund + harmonics (LP)
   { 0x00008501, 0x0000, 2570,    270,    274,    278,    282 },  //varh Q2 fund + harmonics (LP)
   { 0x00010501, 0x0000, 2571,    271,    275,    279,    283 },  //varh Q3 fund + harmonics (LP)
   { 0x00020501, 0x0000, 2572,    272,    276,    280,    284 },  //varh Q4 fund + harmonics (LP)
   { 0x00404501, 0x0000, 2577,   2576,   2575,   2574,   2573 },  //varh Q1 fund only (LP)
   { 0x00408501, 0x0000, 2582,   2581,   2580,   2579,   2578 },  //varh Q2 fund only (LP)
   { 0x00410501, 0x0000, 2587,   2586,   2585,   2584,   2583 },  //varh Q3 fund only (LP)
   { 0x00420501, 0x0000, 2592,   2591,   2590,   2589,   2588 },  //varh Q4 fund only (LP)
   { 0x0040C501, 0x0000, 2597,   2596,   2595,   2594,   2593 },  //varh Q1+Q2 fund only (LP)
   { 0x00430501, 0x0000, 2602,   2601,   2600,   2599,   2598 },  //varh Q3+Q4 fund only (LP)
   { 0x0043C501, 0x0000, 2607,   2606,   2605,   2604,   2603 },  //varh (Q1+Q2)+(Q3+Q4) fund only (LP)
   { 0x0047C501, 0x0000, 2612,   2611,   2610,   2609,   2608 },  //varh (Q1+Q2)-(Q3+Q4) fund only (LP)
   { 0x0003C503, 0x0000, 2932,   2931,   2930,   2929,   2928 }   //Phasor Apparent VAh, fund + harmonics (LP)
};

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
#undef PROJ_EXTERN

#endif
#else
#error "Incompatable HMC meter project path selected"
#endif   // #if HMC_I210_PLUS_C
