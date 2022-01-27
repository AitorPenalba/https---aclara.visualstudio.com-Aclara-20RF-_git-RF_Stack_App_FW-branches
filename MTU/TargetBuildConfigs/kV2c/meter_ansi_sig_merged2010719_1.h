/* ****************************************************************************************************************** */
/***********************************************************************************************************************

   Filename: meter_ansi_sig.h

   Contents: Project Specific Includes for the GE KV2c

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2013-2021 Aclara.  All Rights Reserved.

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
#if HMC_KV
#ifndef meter_ansi_sig_H_
#define meter_ansi_sig_H_

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define STD_TBL_PROC_INIT_MAX_PARAMS          ((uint16_t)10)          /* Maximum parameter bytes allowed - kV2c */
#define STD_TBL_PROC_RESP_MAX_BYTES           ((uint16_t)13)          /* Maximum response bytes allowed - kV2c */

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
#define MFG_TBL_GE_DEVICE_TABLE               ((uint16_t)  0  | 0x0800)
#define MFG_TBL_FW_DL_STATUS_EVENT_LOG        ((uint16_t) 42  | 0x0800)
#define MFG_TBL_PROGRAM_CONSTANTS             ((uint16_t) 67  | 0x0800)
#define MFG_TBL_ERROR_CAUTION                 ((uint16_t) 68  | 0x0800)
#define MFG_TBL_ERROR_HISTORY                 ((uint16_t) 69  | 0x0800)
#define MFG_TBL_DISPLAY_CONFIG                ((uint16_t) 70  | 0x0800)
#define MFG_TBL_POWER_QUALITY_CONFIG          ((uint16_t) 71  | 0x0800)
#define MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA  ((uint16_t) 72  | 0x0800)
#define MFG_TBL_POWER_FACTOR_CONFIG           ((uint16_t) 73  | 0x0800)
#define MFG_TBL_TEST_MODE_CONFIG              ((uint16_t) 74  | 0x0800)
#define MFG_TBL_SCALE_FACTORS                 ((uint16_t) 75  | 0x0800)
#define MFG_TBL_IO_OPTION_BOARD_CONTROL       ((uint16_t) 76  | 0x0800)
#define MFG_TBL_LOAD_CONTROL_SWITCH           ((uint16_t) 77  | 0x0800)
#define MFG_TBL_SECURITY_LOG                  ((uint16_t) 78  | 0x0800)
#define MFG_TBL_ALTERNATE_CALIBRATION         ((uint16_t) 79  | 0x0800)
#define MFG_TBL_AVERAGE_PF_DATA               ((uint16_t) 81  | 0x0800)
#define MFG_TBL_SECURITY                      ((uint16_t) 84  | 0x0800)
#define MFG_TBL_METER_STATE                   ((uint16_t) 85  | 0x0800)
#define MFG_TBL_ELECTRICAL_SERVICE_CONFIG     ((uint16_t) 86  | 0x0800)
#define MFG_TBL_ELECTRICAL_SERVICE_STATUS     ((uint16_t) 87  | 0x0800)
#define MFG_TBL_CYCLE_INSENSITIVE_DMD_CONFIG  ((uint16_t) 97  | 0x0800)
#define MFG_TBL_CYCLE_INSENSITIVE_DMD_DATA    ((uint16_t) 98  | 0x0800)
#define MFG_TBL_AMI_SECURITY                  ((uint16_t) 107 | 0x0800)
#define MFG_TBL_PRESENT_REGISTER_DATA         ((uint16_t) 110 | 0x0800)
#define MFG_TBL_VOLTAGE_EVENT_MONITOR_CONFIG  ((uint16_t) 111 | 0x0800)
#define MFG_TBL_VOLTAGE_EVENT_MONITOR_LOG     ((uint16_t) 112 | 0x0800)
#define MFG_TBL_LOAD_CONTROL_CONFIG           ((uint16_t) 114 | 0x0800)
#define MFG_TBL_LOAD_CONTROL_STATUS           ((uint16_t) 115 | 0x0800)
#define MFG_TBL_RCDC_SWITCH_CONFIG            ((uint16_t) 116 | 0x0800)
#define MFG_TBL_RCDC_SWITCH_STATUS            ((uint16_t) 118 | 0x0800)
#define MFG_TBL_NETWORK_STATUS_DISPLAY_DATA   ((uint16_t) 119 | 0x0800)

#define MODE_DEMAND_ONLY                        0
#define MODE_DEMAND_LP                          1
#define MODE_DEMAND_TOU                         2

#define MT98_DEMANDS_SIZE                       4
#define MT98_NBR_DEMANDS                        2
#define MT98_NBR_TIERS                          2
#define MT98_NBR_OF_ENTRIES                     35

/* distoration types that can be configured in manufacturing table 71 */
#define MT71_DIAG5_DISTORTION_PF                      0
#define MT71_DIAG5_TOTAL_DEMAND_DISTORTION            1
#define MT71_DIAG5_TOTAL_HARMONIC_CURRENT_DISTORTION  2
#define MT71_DIAG5_TOTAL_HARMONIC_VOLTAGE_DISTORTION  3
#define MT71_DIAG5_DC_DETECTION                       4

/* Meter log event argument   */
#define  voltagePhaseA                          (1<<0)   /*lint !e778 shift qty of 0 is OK   */
#define  voltagePhaseB                          (1<<1)
#define  voltagePhaseC                          (1<<2)
#define  currentPhaseA                          (1<<3)
#define  currentPhaseB                          (1<<4)
#define  currentPhaseC                          (1<<5)
#define  totalEvent                             (1<<6)

#define SWELL_EVENT                             (( uint8_t ) 1 )
#define SAG_EVENT                               (( uint8_t ) 0 )

#define METER_PASSWORD_SIZE   ((uint8_t)20)
#define ANSI_FORM_SIZE        ((uint8_t)5)

#define NWK_STAT_INFO_SIZE                      ((uint8_t)6)   /* ASCII characters for one line of meter display */

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

/*****************************************************/
/* Manufacturer Table 0                              */
/*****************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           GE_PartNumber[5];
   uint8_t           FwVersion;
   uint8_t           FwRevision;
   uint8_t           FwBuild;
   uint8_t           MfgTestVector[4];
}romConstDataRCD_t;  // 12 bytes
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{  // need to add comments
   uint8_t           GE_PartNumber[5];
   uint8_t           FwVersion;
   uint8_t           FwRevision;
   uint8_t           FwBuild;
   uint8_t           flashVersion;
   uint8_t           flashRevision;
   uint8_t           flashBuild;
   uint16_t          checksum;
   uint16_t          firmwareUpdateFlags;
}flashConstants_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{  // need to add comments
   uint8_t           partNumber[5];
   uint8_t           romVersion;
   uint8_t           romRevision;
   uint8_t           romBuild;
   uint8_t           userVersion;
   uint8_t           userRevision;
   uint8_t           userBuild;
   uint8_t           checkSum;
   uint16_t          firmwareUpdateFlags;
}userCalcConstants_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{  // need to add comments
   uint8_t           romEepromVersion;
   uint8_t           fwUpdateEepromVersion;
   uint8_t           baseCrc[4];
   uint8_t           eepromVer;
   uint16_t          fwUpdatedCrcCalculated;
   uint16_t          fwUpdatedCrcDownloaded;
   uint32_t          fwUpdateStartAddress;
   uint32_t          fwUpdateEndAddress;
}fwUpdateConstants_t;
PACK_END

PACK_BEGIN
typedef struct
{
   uint32_t    tou                  : 1;
   uint32_t    secondMeasure        : 1;
   uint32_t    fourChanRecording    : 1;
   uint32_t    eventLogging         : 1;
   uint32_t    alternateComm        : 1;
   uint32_t    DSPsampleOut         : 1;
   uint32_t    pulseInitiatorOut    : 1;
   uint32_t    demandCalculation    : 1;

   uint32_t    twentyChannel        : 1;  //Upgrade8
   uint32_t    xfmrLossCompensation : 1;  //Upgrade9
   uint32_t    xfmrAccuracy         : 1;  //Upgrade10
   uint32_t    revenueGuard         : 1;  //Upgrade11
   uint32_t    voltageEventMonitor  : 1;  //Upgrade12
   uint32_t    bidirectional        : 1;  //Upgrade13
   uint32_t    waveformCapture      : 1;  //Upgrade14
   uint32_t    expandedMeasure      : 1;  //Upgrade15

   uint32_t    powerQualityMonitor  : 1;  //Upgrade16
   uint32_t    totalization         : 1;  //Upgrade17
   uint32_t    hugeLPmemory         : 1;  //Upgrade18
   uint32_t    rsvd19               : 1;  //Upgrade19
   uint32_t    rsvd20               : 1;  //Upgrade20
   uint32_t    rsvd21               : 1;  //Upgrade21
   uint32_t    rsvd22               : 1;  //Upgrade22
   uint32_t    ecp                  : 1;  //Upgrade23 - Note Only available in v4.14

   uint32_t    dlp                  : 1;  //Upgrade24 - Note Only available in v4.14
   uint32_t    ppm                  : 1;  //Upgrade25 - Note Only available in v4.14
   uint32_t    upgradeRsvd0_5       : 6;
} UpgradesBfld_t; // 4 bytes
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  Version;
   uint8_t  Revision;
   uint8_t  Build;
   uint8_t  BootCodeHash[32];
} BootloaderFwConstants_t; // 35 bytes
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  CustomerId;
   uint8_t  ModuleType;
   uint8_t  Version;
   uint8_t  Revision;
   uint8_t  Build;
} MetrologyFwConstants_t;  // 5 bytes
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  Signature[256];
   uint8_t  BuildId[8];
} MeterFwConstants_t;   // 264 bytes
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t BuildNumber;
   uint8_t  MinorRevision;
   uint8_t  MajorRevision;
} BuildMinorMajor_t; //4 bytes
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   BuildMinorMajor_t bootloaderRevision;  // 4 bytes
   BuildMinorMajor_t applicationRevision; // 4 bytes
} RelayFwConstants_t;   // 8 bytes
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t             MfgVersionNbr;
   uint8_t             MfgRevisionNbr;
   romConstDataRCD_t   romConstDataRcd;
   uint8_t             MeterType;
   uint8_t             MeterMode;
   uint8_t             RegisterFunction;
   uint8_t             InstalledOption1;
   uint8_t             InstalledOption2;
   uint8_t             InstalledOption3;
   uint8_t             InstalledOption4;
   uint8_t             InstalledOption5;
   uint8_t             InstalledOption6;
   UpgradesBfld_t      upgrades;
   uint8_t             flashEnabledFlags;
   uint8_t             flashPartId;
   uint8_t             reserved[2];
   flashConstants_t    flashConstants;
   userCalcConstants_t userCalcConstants;
   fwUpdateConstants_t fwUpdateConstants;
} mfgTbl0_t;   /* Manufacturer Table #0 - GE Device Table   */
PACK_END

/*****************************************************/
/* Manufacturer Table 69                             */
/*****************************************************/
PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint8_t  dspError:         1; /* A DSP error is present    */
      uint8_t  optionBrd:        1; /* An Option Board error is present (Not used in kV2c). */
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
      uint8_t  dspConfigFail:    1; /* Indicates if DSP Config Fail error occurred.  Not used in kV2c */
      uint8_t  dspTimeout:       1; /* Indicates if DSP timeout error occurred.    */
      uint8_t  dspRom:           1; /* Indicates if DSP ROM error occurred.  Not used in kV2c.   */
      uint8_t  dspMomIntTimeout: 1; /* Indicates if DSP momentary interval timeout error occurred. Not used in kV2c. */
      uint8_t  momIntCksum:      1; /* Indicates if momentary interval checksum error occurred.  Not used in kV2c.   */
      uint8_t  dspOverrun:       1; /* Indicates if overrun occurred.  */
      uint8_t  dspOverflow:      1; /* Indicates if overflow occurred. Not used in kV2c. */
      uint8_t  dspZeroCross:     1; /* Indicates if DSP zero cross occurred.  Not used in kV2c.  */
   } bits;
   uint8_t  byte;
} DSPerror_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  nonLpDataError:   1; /* Error occurred in non-LP data portion of flash.   */
   uint8_t  lpDataError:      1; /* Error occurred in LP data memory.  */
   uint8_t  reserved:         6; /* unused  */
} dataFlashErrors_t;
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
      uint16_t   lowBattery:        1; /* A low battery caution is present   */
      uint16_t   lowPotential:      1; /* A low potential caution is present */
      uint16_t   dmdThreshOverload: 1; /* A demand threshold overload caution is present */
      uint16_t   receivedKwh:       1; /* A received kWh caution is present  */
      uint16_t   leadingKvarh:      1; /* A leading kvarh caution is present */
      uint16_t   unprogrammed:      1; /* An unprogrammed caution is present */
      uint16_t   lossOfProgram:     1; /* A loss-of-program caution is present  */
      uint16_t   timeChanged:       1; /* A time change caution is present.  */
      uint16_t   badPassword:       1; /* A bad password caution is present  */
      uint16_t   reserved:          1; /* Not used in the kV2c   */
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

PACK_BEGIN
typedef struct PACK_MID
{
   errorRcd_t        errorHistory;              /* ERROR_HISTORY  */
   DSPerror_t        dspErrorInfo;
   DSPerror_t        dspErrorHistory;

   cautionRcd_t      cautionHistory;            /* Caution History   */
   dataFlashErrors_t flashErrors;
   dataFlashErrors_t flashErrorsHistory;
   romFlashErrors_t  romflasherrors;            /* Bits defined as above. */
   romFlashErrors_t  romFlashErrorHistory;      /* Bits defined as above. */
   eepromErrors_t    eepromError;
   eepromErrors_t    eepromErrorHistory;        /* Bits defined as above. */
   uint32_t          flashSectorErrors;         /* One error bit for each sector, 0 - 31. */
   uint32_t          flashSectorErrorHistory;   /* One error bit for each sector, 0 - 31. */

   statusRcd_t       rollingStatus;             /* Errors and cautions that occurred in the 35-day billing period */
   statusRcd_t       dailyStatus;               /* Errors, cautions and diagnostics that occurred today */
   statusRcd_t       realTimeStatus;            /* Real-time indication of errors, cautions and diagnostics (see
                                                   STATUS_RCD above) */
} mfgTbl69_t;  /* Standard Table #3 - End Device Mode and Status */
PACK_END

/*****************************************************/
/* Manufacturer Table 71                             */
/*****************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           diag1 : 1;   /* Determines if diagnostic 1 is enabled to report in MT72 */
   uint8_t           diag2 : 1;   /* Determines if diagnostic 2 is enabled to report in MT72 */
   uint8_t           diag3 : 1;   /* Determines if diagnostic 3 is enabled to report in MT72 */
   uint8_t           diag4 : 1;   /* Determines if diagnostic 4 is enabled to report in MT72 */
   uint8_t           diag5 : 1;   /* Determines if diagnostic 5 is enabled to report in MT72 */
   uint8_t           diag6 : 1;   /* Determines if diagnostic 6 is enabled to report in MT72 */
   uint8_t           diag7 : 1;   /* Determines if diagnostic 7 is enabled to report in MT72 */
   uint8_t           diag8 : 1;   /* Determines if diagnostic 8 is enabled to report in MT72 */
} DiagEnabledBitField_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           diag1 : 1;   /* Determines if diagnostic 1 is displayed */
   uint8_t           diag2 : 1;   /* Determines if diagnostic 2 is displayed */
   uint8_t           diag3 : 1;   /* Determines if diagnostic 3 is displayed */
   uint8_t           diag4 : 1;   /* Determines if diagnostic 4 is displayed */
   uint8_t           diag5 : 1;   /* Determines if diagnostic 5 is displayed */
   uint8_t           diag6 : 1;   /* Determines if diagnostic 6 is displayed */
   uint8_t           diag7 : 1;   /* Determines if diagnostic 7 is displayed */
   uint8_t           diag8 : 1;   /* Determines if diagnostic 8 is displayed */
} DiagDisplayBitField_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           diag1 : 1;   /* Determines if diagnostic 1 will freeze the display scroll */
   uint8_t           diag2 : 1;   /* Determines if diagnostic 2 will freeze the display scroll */
   uint8_t           diag3 : 1;   /* Determines if diagnostic 3 will freeze the display scroll */
   uint8_t           diag4 : 1;   /* Determines if diagnostic 4 will freeze the display scroll */
   uint8_t           diag5 : 1;   /* Determines if diagnostic 5 will freeze the display scroll */
   uint8_t           diag6 : 1;   /* Determines if diagnostic 6 will freeze the display scroll */
   uint8_t           diag7 : 1;   /* Determines if diagnostic 7 will freeze the display scroll */
   uint8_t           diag8 : 1;   /* Determines if diagnostic 8 will freeze the display scroll */
} DiagFreezeBitField_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           diag1 : 1;   /* Determines if diagnostic 1 will trigger an output */
   uint8_t           diag2 : 1;   /* Determines if diagnostic 2 will trigger an output */
   uint8_t           diag3 : 1;   /* Determines if diagnostic 3 will trigger an output */
   uint8_t           diag4 : 1;   /* Determines if diagnostic 4 will trigger an output */
   uint8_t           diag5 : 1;   /* Determines if diagnostic 5 will trigger an output */
   uint8_t           diag6 : 1;   /* Determines if diagnostic 6 will trigger an output */
   uint8_t           diag7 : 1;   /* Determines if diagnostic 7 will trigger an output */
   uint8_t           diag8 : 1;   /* Determines if diagnostic 8 will trigger an output */
} DiagTriggerBitField_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   DiagEnabledBitField_t               diagEnabledBitfield;  /* specifies which diagnostics are enabled */
   DiagDisplayBitField_t               diagDisplayBitfield;  /* specifies which diagnostics will be presented at the display */
   DiagFreezeBitField_t                 diagFreezeBitfield;  /* specifies which diagnostics causes display scroll to freeze */
   DiagTriggerBitField_t               diagTriggerBitfield;  /* specifies which diagnostic will trigger an output */
   uint8_t                          phaseAVoltageTolerance;  /* specifies voltage imbalance tolerance */
   uint16_t                             lowCurrentTreshold;  /* threshold for diagnostic 3 */
   uint16_t                          currentAngleTolerance;  /* specifies current imabalance for diagnostic 4 in 10th degrees */
   uint8_t                            distorationTolerance;  /* specifies distoration tolerance for diagnostic 5 as a percent */
   uint16_t                         phaseAVoltageReference;  /* specifies the reference voltage for Phase A in 10th volts */
   uint8_t                       phaseAVoltageSagTolerance;  /* specifies the phase A voltage sag tolerance as a percent */
   uint8_t                     phaseAVoltageSwellTolerance;  /* specifies the phase A voltage swell tolerance as a percent */
   uint16_t                    highNeutralCurrentThreshold;  /* specifies threshold for diagnostic 8 in 10th amps */
   uint8_t                               diagnostic5Config;  /* specifies distortion type to be monitored by diagnostic 5 */
   uint16_t                       diagnostics6and7Duration;  /* specifies time in seconds for diagnostic 6 and 7 to be reported */
   uint16_t                       otherDiagnosticsDuration;  /* specifies time in seconds for diagnostics other than 6 and 7 to be reported */
   uint16_t                            diagnostic2Duration;  /* specifies time in seconds for diagnostic 2 to be reported */
} mfgTbl71_t;  /* Manufacturing Table 71 - Line-Side Diagnostics/Power Quality Configuration Table */
PACK_END

/*****************************************************/
/* Manufacturer Table 72                             */
/*****************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t          iAnglePhA;        /* Phase A current angle in 10th degrees */
   uint16_t          vAnglePhA;        /* Phase A voltage angle in 10th degrees   */
   uint16_t          iAnglePhB;        /* Phase B current angle in 10th degrees */
   uint16_t          vAnglePhB;        /* Phase B voltage angle in 10th degrees */
   uint16_t          iAnglePhC;        /* Phase C current angle in 10th degrees */
   uint16_t          vAnglePhC;        /* Phase C voltage angle in 10th degrees */
} VIAngles_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t          iMagnitudePhA;    /* Phase A current magnitude */
   uint16_t          vMagnitudePhA;    /* Phase A voltage magnitude */
   uint16_t          iMagnitudePhB;    /* Phase B current magnitude */
   uint16_t          vMagnitudePhB;    /* Phase B voltage magnitude */
   uint16_t          iMagnitudePhC;    /* Phase C current magnitude */
   uint16_t          vMagnitudePhC;    /* Phase C voltage magnitude */
} VIMagnitudes_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   VIAngles_t     viAngles;
   VIMagnitudes_t viMagnitudes;
} LineSideDiagRcd_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           diag1;            /* Number of occurrences of diagnostic 1  (Polarity, Cross Phase, Reverse Energy Flow) */
   uint8_t           diag2;            /* Number of occurrences of diagnostic 2  (Voltage Imbalance)                          */
   uint8_t           diag3;            /* Number of occurrences of diagnostic 3  (Inactive Phase Current)                     */
   uint8_t           diag4;            /* Number of occurrences of diagnostic 4  (Phase Angle Alert)                          */
   uint8_t           diag5phA;         /* Number of occurrences of diagnostic 5  (High Distortion PhA)                        */
   uint8_t           diag5phB;         /* Number of occurrences of diagnostic 5  (High Distortion PhB)                        */
   uint8_t           diag5phC;         /* Number of occurrences of diagnostic 5  (High Distortion PhC)                        */
   uint8_t           diag5Total;       /* Number of occurrences of diagnostic 5  (High Distortion Total)                      */
   uint8_t           diag6A;           /* Number of occurrences of diagnostic 6A (Under Voltage, PhA)                         */
   uint8_t           diag7A;           /* Number of occurrences of diagnostic 7A (Over Voltage, PhA)                          */
   uint8_t           diag8;            /* Number of occurrences of diagnostic 8  (High Neutral Current)                       */
} DiagCounters_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t        diag1Bit:  1;        /* Indicates occurrence of diagnostic 1  (Polarity, Cross Phase, Reverse Energy Flow)  */
   uint8_t        diag2Bit:  1;        /* Indicates occurrence of diagnostic 2  (Voltage Imbalance)                           */
   uint8_t        diag3Bit:  1;        /* Indicates occurrence of diagnostic 3  (Inactive Phase Current)                      */
   uint8_t        diag4Bit:  1;        /* Indicates occurrence of diagnostic 4  (Phase Angle Alert)                           */
   uint8_t        diag5Bit:  1;        /* Indicates occurrence of diagnostic 5  (High Distortion)                             */
   uint8_t        diag6ABit: 1;        /* Indicates occurrence of diagnostic 6A (Under Voltage, PhA)                          */
   uint8_t        diag7ABit: 1;        /* Indicates occurrence of diagnostic 7A (Over Voltage, PhA)                           */
   uint8_t        diag8Bit:  1;        /* Indicates occurrence of diagnostic 8  (High Neutral Current)                        */
} DiagCautions_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           diag6B;           /* Number of occurrences of diagnostic 6B (Under Voltage, PhB) */
   uint8_t           diag7B;           /* Number of occurrences of diagnostic 7B (Over Voltage, PhB)  */
   uint8_t           diag6C;           /* Number of occurrences of diagnostic 6C (Under Voltage, PhC) */
   uint8_t           diag7C;           /* Number of occurrences of diagnostic 7C (Over Voltage, PhC)  */
} DiagCounters2_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t        diag6BBit: 1;        /* Indicates occurrence of diagnostic 6B (Under Voltage, PhB)  */
   uint8_t        diag7BBit: 1;        /* Indicates occurrence of diagnostic 7B (Over Voltage, PhB)   */
   uint8_t        diag6CBit: 1;        /* Indicates occurrence of diagnostic 6C (Under Voltage, PhC)  */
   uint8_t        diag7CBit: 1;        /* Indicates occurrence of diagnostic 7C (Over Voltage, PhC)   */
   uint8_t        filler:    4;
} DiagCautions2_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   LineSideDiagRcd_t lsdRcd;           /* Line Side Diagnostics record  */
   uint8_t           duPF;             /* Distortion power factor       */
   DiagCounters_t    diagCounters;     /* Diagnostic counters           */
   DiagCautions_t    diagCautions;     /* Diagnostic cautions           */
   DiagCounters2_t   diagCounters2;    /* Diagnostic counters 2         */
   DiagCautions2_t   diagCautions2;    /* Diagnostic cautions 2         */
} mfgTbl72_t;  /* Manufacturing Table 72 - Line-Side Diagnostics/Power Quality Data Table */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   LineSideDiagRcd_t lsdRcd;           /* Line Side Diagnostics record  */
   uint8_t           duPF;             /* Distortion power factor       */
   DiagCounters_t    diagCounters;     /* Diagnostic counters           */
   DiagCautions_t    diagCautions;     /* Diagnostic cautions           */
   DiagCounters2_t   diagCounters2;    /* Diagnostic counters 2         */
   DiagCautions2_t   diagCautions2;    /* Diagnostic cautions 2         */
   uint8_t           tamperCounter;    /* Number of possible meter inversion tamper occurrences detected */
} mfgTbl72_V4_14_t;  /* Manufacturing Table 72 - Line-Side Diagnostics/Power Quality Data Table */
PACK_END


/*****************************************************/
/* Begin - Manufacturer Table 78                     */
/*****************************************************/
typedef struct /* Not PACKed so the usage in hdFileIndex_t remains aligned the same as previous releases (no DFW impact that way) */
{
   uint8_t               dtLastCalibrated[5];  // date and time of last calibration
   uint8_t                calibratorName[10];  // name of person who last calibrated the meter
   uint8_t               dtLastProgrammed[5];  // date and time the meter was last programmed
   uint8_t                programmerName[10];  // name of person who last programmed the meter
   uint16_t          numberOfTimesProgrammed;  // number of programming sessions
} meterProgInfo_t;   // Created struct for convenience

PACK_BEGIN
typedef struct PACK_MID
{
   meterProgInfo_t               mtrProgInfo;  //Changed from table definition for convenience
   uint8_t              numberOfDemandResets;  // number of demand resets
   uint8_t              dtLastDemandReset[5];  // date and time of the last demand reset
   uint16_t       nbrOfcommunicationSessions;  // number of communication sessions with the meter, AMI, and optical
   uint8_t     dtLastCommunicationSession[5];  // date and time of the latest communication session with the meter
   uint16_t                nbrOfBadPasswords;  // number of invalid passwords received by the meter
   uint16_t                    nbrRtpEntries;  // number of times real time pricing entered
   uint8_t                 dtLastRtpEntry[5];  // data and time the real time pricing was last entered
   uint32_t            cumPowerOutageSeconds;  // number of seconds the meter has been without power
   uint16_t                nbrOfPowerOutages;  // number of power outages observed by the meter
   uint8_t              dtLastPowerOutage[5];  // date and time of the last power outage
   uint8_t              nbrOfEepromWrites[3];  // number of times data has been written to EEPROM
   uint8_t                dtLastTlcUpdate[5];  // date and time the transformer loss compensation adjustments were made
   uint8_t                       tlcName[10];  // name of person who performed transformer loss compensation
   uint8_t       dtLastTransfIncaccAdjsut[5];  // date and time adjustments for transformer accuracy were last made
   uint8_t         transfInaccAdjustName[10];  // name of person who last performed transformer incaccuracy adjustments
   uint8_t               dtLastTimeChange[5];  // date and time that time was last reset in the meter
   uint16_t              nbrSustainedMinutes;  // each sustained interruption is rounded to the nearest whole minute
   uint16_t           nbrSustainedInterrupts;  // count of the number of IEEE sustained interruptions
   uint16_t           nbrMomentaryInterrupts;  // count of the number of IEEE momentary interruptions
   uint16_t               nbrMomentaryEvents;  // count of the number of IEEE momentary events
   uint8_t         dtLastAmrCommunication[5];  // date and time of the last AMR to meter communication session
   uint16_t             nbrAmrcommunications;  // count of AMR to meter communication sessions
} mfgTbl78_t;  // Manufacturing Table 78 - Security Log
PACK_END
/*****************************************************/
/* End - Manufacturer Table 78 Description           */
/*****************************************************/

/*****************************************************/
/* Begin - Manufacturer Table 81                     */
/*****************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t    numerator[6]; // numerator accumulation
   uint8_t  denominator[6]; // denominator accumulation
} avgPfData_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   avgPfData_t prevSeasonAvgPfs; // previous season average power factor accumulators
   avgPfData_t  prevResetAvgPfs; // previous reset average power factor accumulators
   avgPfData_t    currentAvgPfs; // current average power factor accumulators
} mfgTbl81_t;
PACK_END
/*****************************************************/
/* End - Manufacturer Table 81 Description           */
/*****************************************************/

/*****************************************************/
/* Begin - Manufacturer Table 86                    */
/*****************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t      autoDetectOverrideFlag;
   uint8_t             derviceOverride;
   uint8_t              defaultDspCase;
   uint8_t    ansiForm[ANSI_FORM_SIZE];
   uint8_t    dailyServiceDetectEnable;
   uint8_t   manualServiceDetectEnable;
   uint8_t          revGuardPlusEnable;
   uint8_t        revGuardVoltCheckInt;
   uint16_t     maxInstallationCurrent;
} mfgTbl86_t;
PACK_END
/*****************************************************/
/* End - Manufacturer Table 86 Description           */
/*****************************************************/

/*****************************************************/
/* Begin - Manufacturer Table 87                    */
/*****************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t                    meterBase; // indicates meter base type
   uint16_t                maxClassAmps; // max amps
   uint8_t           elementVoltageCode; // element voltage id code
   uint8_t    autoDetectServiceProgress; // indicates if automatic service detection is currenlty in progress
   uint8_t                 serviceInUse; // indicates current electrical service
   uint8_t                 dspCaseInUse; // indicates current DSP case
   uint16_t         elementVoltageInUse; // current element volts, in tenths
} mfgTbl87_t;
PACK_END
/*****************************************************/
/* End - Manufacturer Table 87 Description           */
/*****************************************************/

/*****************************************************/
/* Begin - Manufacturer Table 97                     */
/*****************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  statusEnable:     1;    /* Enable Cycle Insensitive Rolling Status */
   uint8_t  demandEnable:     1;    /* Enable Cycle Insensitive Demand */
   uint8_t  businessEnable:   1;    /* Enable business day search */
   uint8_t  filler:           5;    /* not used*/
} MT97configFlags_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   MT97configFlags_t    configFlags;
   uint8_t              statusDays;       /* Number of calendar day's worth of status that is used to compute Rolling Status*/
   uint8_t              peak1Days;        /* Number of days to include in search for Peak 1 */
   uint8_t              peak2Days;        /* Number of days to include in search for Peak 2 */
} mfgTbl97_t;  /* Manufacturer Table 97: Cycle Insensitive/ Rolling Billing Demand Config*/
PACK_END

/*****************************************************/
/* Manufacturer Table 98                             */
/*****************************************************/
PACK_BEGIN
typedef struct PACK_MID
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
typedef struct PACK_MID
{
   uint8_t year:                 7;    // 7 least significant bits
   uint8_t nonBusinessDayFlag:   1;    // Indicates whether entry is considered a business day or not. (1 = Non-business day, 0 = business day) Most Significant Bit
   uint8_t month;
   uint8_t day;
} MT98Date_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t hours;
   uint8_t minute;
} MT98Time_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   MT98Time_t  time;       /* Time of the Demand */
   uint32_t    demand;     /* Daily Max Demand */
} MT98Demand_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   MT98Demand_t demands[MT98_NBR_DEMANDS];  /* Array of 2 quantities */
} MT98DataBlock_t; /* TOT_DATA_BLOCK in Field in MT 98 */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   MT98Date_t        date;                   /* Date entry YY MM DD */
   statusRcd_t       status;                 /* MT 69 STATUS_RCD */
   MT98DataBlock_t   totDataBlock;
   MT98DataBlock_t   tiers[MT98_NBR_TIERS];  /* Array of 2 tiers(A & B only) */
} MT98Entries_t;  // ENTRIES Field in MT 98
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   diagBfld_t  diagBfld;               /* See MT69 */
} MT98Entries2_t;  // ENTRIES_2 Field in MT 98
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   MT98DayFlags_t       dayFlags;
   MT98ElementEntry_t   ElementEntry;                    /* nbrOfValidEntries and lastEntryElement fields in MT #98 */
   MT98Entries_t        entries[MT98_NBR_OF_ENTRIES];    /* Array of maximum of 35 entries. */
   MT98Entries_t        todaysEntry;                     /* Cycle Insensitive Values since midnight */
   MT98Entries2_t       entries2[MT98_NBR_OF_ENTRIES];   /* Array of maximum of 35 entries. */
   MT98Entries2_t       todaysEntry2;                    /* Cycle Insensitive Values since midnight */
} mfgTbl98_t; /* Manufacturer Table 98: Cycle Insensitive/ Rolling Billing Demand Data*/
PACK_END
// Note: This creates a big Structure of 1587 Bytes



/*****************************************************/
/* Manufacturer Table 110                            */
/*****************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint32_t       previousIntDemands[5]; // demand for the last completed interval, one entry for each demand
   uint32_t                  demands[5]; // momentary interval demand for each demand
   uint32_t            kwDmdFundPlus[3]; // momentary interval kW demand fund plus harmonics
   uint32_t            kwDmdFundOnly[3]; // momentary interval kW demand fund only
   uint32_t          kvarDmdFundPlus[3]; // momentary interval kVar demand fund plus harmonics
   uint32_t          kvarDmdFundOnly[3]; // momentary interval kVar demand fund only
   uint32_t         distortionKvaDmd[3]; // momentary interval distortion kVA demand
   uint32_t           apparentKvaDmd[3]; // momentary interval apparent kVA demand
   uint16_t    lineToNeutralFundPlus[3]; // momentary interval line to neutral voltage fundamental plus harmonics
   uint16_t    lineToNeutralFundOnly[3]; // momentary interval line to neutral voltage fundamenal only
   uint16_t       lineToLineFundPlus[3]; // momentary interval line to line voltage fundamental plus harmonics
   uint16_t       lineToLineFundOnly[3]; // momentary interval line to line voltage fundamental only
   uint16_t          currentFundPlus[3]; // momentary interval current fundamental plus harmonics
   uint16_t          currentFundOnly[3]; // momentary interval current fundamental only
   uint16_t       imputedNeutralCurrent; // momentary interval imputed neutral current
   uint8_t                  powerFactor; // momentary interval power factor
   uint16_t                   frequency; // momentary interval line frequency
   uint8_t                       tdd[3]; // total demand distoration
   uint8_t                      ithd[3]; // current total harmonic distortion
   uint8_t                      vthd[3]; // voltage total harmonic distortion
   uint8_t              distortionPf[4];
   uint8_t        timeRemainingInSubint; // time remaining in demand subinterval
   uint8_t              realTimeSeconds; // number of seconds in the current minute
   uint8_t                  temperature; // temperature reading at metrology chip
   uint8_t               batteryVoltage; // voltage of battery in tenth of volts
} mfgTbl110_t; /* Manufacturer Table 110: Present Register Table*/
PACK_END


/*****************************************************/
/* Manufacturer Table 112                            */
/*****************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t order           : 1;  /* 0->ascending, 1->descending   */
   uint8_t overflow        : 1;  /* 0->log hasn't overflowed; 1->log overflowed */
   uint8_t listType        : 1;  /* 0->FIFO, 1->circular list  */
   uint8_t inhibitOverflow : 1;  /* 0->overflow not inhibited, 1->overflow inhibited   */
   uint8_t fill            : 4;  /* not used */
} MT112EventFlags_t;    // 1 byte
PACK_END

PACK_BEGIN
typedef struct PACK_MID
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
} sagSwellEntry_t, *pSagSwellEntry_t;  // 23 bytes
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   MT112EventFlags_t    eventFlags;       // 1 byte
   uint16_t             nbrValidEntries;  // 2 bytes
   uint16_t             lastEntry;        // 2 bytes
   uint32_t             lastEntrySeqNbr;  // 4 bytes
   uint16_t             nbrUnread;        // 2 bytes
   uint16_t             sagCounter;       // 2 bytes
   uint16_t             swellCounter;     // 2 bytes
   sagSwellEntry_t      eventArray[];     // 4 bytes (actual meter Table contains 200 elements of 23 bytes each)
} mfgTbl112_t, *pMfgTbl112_t;    // 20 bytes (actual meter Table is 4615 bytes)
PACK_END


/*****************************************************/
/* Manufacturer Table 115                            */
/*****************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t        commError              : 1;  // indicates communication error with the RD board
   uint8_t        switchControllerError  : 1;  // indicates that the switch state and controller status are not valid combination
   uint8_t        switch_failed_to_close    : 1;  // indicates that the switch controller detected no load side voltage and current flow when switch closed
   uint8_t        alternate_source        : 1;  // indicates that the switch controller detected load side voltage out of phase with line
   uint8_t        reserved1              : 1;  // reserved for future use
   uint8_t        bypassed               : 1;  // indicates load side voltage and current flow when switch is open
   uint8_t        switch_failed_to_open     : 1;  // indicates load side voltage and current flow after switch was commanded to open
   uint8_t        ppmAlert               : 1;  // indicates there is present PPM credit available
   uint8_t        reserved2              : 1;  // reserved for future use
   uint8_t        lcMemoryError          : 1;  // indicates that the meter detected an error in the load control status
   uint8_t        lcSwitchStatesMismatch : 1;  // indicates load side voltage on one phase, but not on another phase
   uint8_t        lcInvalidLsVoltage     : 1;  // indicates voltage reported by releay control board is invalid
   uint8_t        filler                 : 4;  // unused, reserved for future use
} LcStatusFlags_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t actual_switch_state    : 1;  // represents the current state of the disconnect switch
   uint8_t desired_switch_state   : 1;  // represent the desired switch state
   uint8_t open_hold_for_command  : 1;  // indicates the meter received an open command and will keep switch open until told to release
   uint8_t reserved1              : 1;  // reserved for future use
   uint8_t reserved2              : 1;  // reserved for future use
   uint8_t outage_open_in_effect  : 1;  // an outage open is current in affect
   uint8_t lockout_in_effect      : 1;  // switch is frozen in the current state
   uint8_t reserved3              : 1;  // reseved for future use
} RcdcState_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  reserved          : 1;  // reserved for future use
   uint8_t ecpState           : 1;  // indicates if emergency conservation period is active
   uint8_t dlpState           : 1;  // indicates if demand limiting period is active
   uint8_t ppmState           : 1;  // indicates if prepayment mode is active
   uint8_t directLcDisconnect : 1;  // indicates if a service disconnect is in progress due to direct load control command
   uint8_t ecpDisconnect      : 1;  // indicates if a service disconnect is in progress due to emergency conservation
   uint8_t dlpDisconnect      : 1;  // indicates if a service disconnect is in progress due to demand limiting period
   uint8_t ppmDisconnect      : 1;  // indicates if a service disconnect is in progress due to prepayment mode
} LcState_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t phaseA;  // phase A load side voltage
   uint16_t phaseB;  // phase B load side voltage
   uint16_t phaseC;  // pahse C load side voltage
} ElementLoadSideVoltage_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   LcStatusFlags_t                         status;  // flags indicating current state of load control functions
   LcStatusFlags_t       loadControlStatusHistory;  // history of status flags, cleared via SP3
   RcdcState_t                          rcdcState;  // switch state information
   LcState_t                     loadControlState;  // current load control state
   uint16_t               lcReconnectAttemptCount;  // number of times remaining service will be reconnected during current ECP or DLP
   uint8_t                 ecpOrPpmaccumulator[6];  // shared accumulator or ECP or PPM
   uint32_t                              lcDemand;  // demand during the previous LC dmeand interval, raw energy units
   uint16_t                       rcdcSwitchCount;  // counts the RD switch operations
   ElementLoadSideVoltage_t   loadSideVoltageMeas;  // load side voltage measurements values
} mfgTbl115_t, *pMfgTbl115_t;
PACK_END


PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  NwkStatusEntry[8][NWK_STAT_INFO_SIZE];   // (NetworkStatusInfo) Free-form field to post status info on the meter display
   uint8_t  AMISignalQuality;       // AMI signal quality (0 = no signal; 1 = poor signal; 2 = good signal; 3 = excellent signal)
   int16_t  AmiRssiValue;           // AMI RSSI value, measured in dBm as a signed integer.
   int16_t  AmiRssiMax;             // Maximum value for AMI device RSSI value, measured in dBm as a signed integer
} mfgTbl119_t, *pMfgTbl119_t;       // 53 bytes
PACK_END


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
const stdTbl76Lookup_t logEvents[88]
#ifdef ANSI_SIG_GLOBALS
=
{
/*   event flag selector          EDEventType                                                      mask priority */
          /*flag 0 = STD, 1 = MFG*/
          /*selector not used*/
   { {  1,  0,    0 },            electricMeterPowerFailed,                                         0,  88 },
   { {  2,  0,    0 },            electricMeterPowerRestored,                                       0,  88 },
   { {  3,  0,    0 },            electricMeterclocktimechanged,                                    0,  88 },
   { {  7,  0,    0 },            electricMetersecurityaccessuploaded,                              0, 100 },
   { {  8,  0,    0 },            electricMetersecurityaccesswritten,                               0, 120 },
   { { 11,  0,    0 },            electricMeterConfigurationProgramChanged,                         0, 190 },
   { { 20,  0,    0 },            electricMeterDemandResetDatalogOccurred,                          0, 160 },
   { { 21,  0,    0 },            electricMetermetrologyselfReadsucceeded,                          0,  85 },
   { { 32,  0,    0 },            electricMetermetrologyTestModestarted,                            0, 110 },
   { { 33,  0,    0 },            electricMetermetrologyTestModestopped,                            0, 110 },
   { {  0,  1,    0 },            electricMeterPowerPhaseBVoltageCrossPhaseDetected,                2, 102 },
   { {  0,  1,    0 },            electricMeterPowerPhaseCVoltageCrossPhaseDetected,                4, 102 },
   { {  0,  1,    0 },            electricMeterPowerPhaseACurrentCrossPhaseDetected,                8, 102 },
   { {  0,  1,    0 },            electricMeterPowerPhaseBCurrentCrossPhaseDetected,               16, 102 },
   { {  0,  1,    0 },            electricMeterPowerPhaseCCurrentCrossPhaseDetected,               32, 102 },
   { {  1,  1,    0 },            electricMeterPowerPhaseCrossPhaseCleared,                         0, 102 },
   { {  2,  1,    0 },            electricMeterPowerPhaseBVoltageImbalanced,                        2, 102 },
   { {  2,  1,    0 },            electricMeterPowerPhaseCVoltageImbalanced,                        4, 102 },
   { {  3,  1,    0 },            electricMeterPowerVoltageImbalanceCleared,                        0, 102 },
   { {  4,  1,    0 },            electricMeterPowerPhaseACurrentInactive,                          8,  90 },
   { {  4,  1,    0 },            electricMeterPowerPhaseBCurrentInactive,                         16,  90 },
   { {  4,  1,    0 },            electricMeterPowerPhaseCCurrentInactive,                         32,  90 },
   { {  5,  1,    0 },            electricMeterPowerPhaseInactiveCleared,                           0, 100 },
   { {  6,  1,    0 },            electricMeterPowerPhaseAngleAOutofRange,                          9,  90 }, //current or voltage
   { {  6,  1,    0 },            electricMeterPowerPhaseAngleBOutofRange,                         18,  90 }, //current or voltage
   { {  6,  1,    0 },            electricMeterPowerPhaseAngleCOutofRange,                         36,  90 }, //current or voltage
   { {  7,  1,    0 },            electricMeterPowerPhaseAngleOutOfRangeCleared,                    0,  80 },
   { {  8,  1,    0 },            electricMeterPowerPhaseAHighDistortion,                           1,  90 },
   { {  8,  1,    0 },            electricMeterPowerPhaseBHighDistortion,                           2,  90 },
   { {  8,  1,    0 },            electricMeterPowerPhaseCHighDistortion,                           4,  90 },
   { {  8,  1,    0 },            electricMeterPowerPowerQualityHighDistortion,                    64, 103 },
   { {  9,  1,    0 },            electricmeterPowerPowerQualityHighDistortionCleared,              0, 103 },
   { { 10,  1,    0 },            electricMeterPowerPhaseAVoltageSagStarted,                        1,  87 },
   { { 11,  1,    0 },            electricMeterPowerPhaseAVoltageSagStopped,                        1,  87 },
   { { 10,  1,    0 },            electricMeterPowerPhaseBVoltageSagStarted,                        2,  87 },
   { { 11,  1,    0 },            electricMeterPowerPhaseBVoltageSagStopped,                        2,  87 },
   { { 10,  1,    0 },            electricMeterPowerPhaseCVoltageSagStarted,                        4,  87 },
   { { 11,  1,    0 },            electricMeterPowerPhaseCVoltageSagStopped,                        4,  87 },
   { { 12,  1,    0 },            electricMeterPowerPhaseAVoltageSwellStarted,                      1,  87 },
   { { 13,  1,    0 },            electricMeterPowerPhaseAVoltageSwellStopped,                      1,  87 },
   { { 12,  1,    0 },            electricMeterPowerPhaseBVoltageSwellStarted,                      2,  87 },
   { { 13,  1,    0 },            electricMeterPowerPhaseBVoltageSwellStopped,                      2,  87 },
   { { 12,  1,    0 },            electricMeterPowerPhaseCVoltageSwellStarted,                      4,  87 },
   { { 13,  1,    0 },            electricMeterPowerPhaseCVoltageSwellStopped,                      4,  87 },
   { { 14,  1,    0 },            electricMeterPowerNeutralCurrentMaxLimitReached,                  0,  71 },
   { { 15,  1,    0 },            electricMeterPowerNeutralCurrentMaxLimitCleared,                  0,  71 },
   { { 16,  1,    0 },            electricMeterPowerPhaseAVoltageMinLimitReached,                   1, 100 },
   { { 16,  1,    0 },            electricMeterPowerPhaseBVoltageMinLimitReached,                   2, 100 },
   { { 16,  1,    0 },            electricMeterPowerPhaseCVoltageMinLimitReached,                   4, 100 },
   { { 17,  1,    0 },            electricMeterPowerVoltageMinLimitCleared,                         0, 109 },
   { { 18,  1,    0 },            electricMeterDemandOverflow,                                      0, 176 },
   { { 19,  1,    0 },            electricMeterDemandNormal,                                        0,  17 },
   { { 20,  1,    0 },            electricMeterSecurityRotationReversed,                            0,  90 },
   { { 21,  1,    0 },            electricMeterSecurityRotationNormal,                              0, 100 },
   { { 22,  1,    0 },            electricMeterPowerReactivePowerReversed,                          0, 102 },
   { { 23,  1,    0 },            electricMeterPowerReactivePowerNormal,                            0, 102 },
   { { 24,  1,    0 },            electricMeterbillingRTPstarted,                                   0, 100 },
   { { 25,  1,    0 },            electricMeterbillingRTPstopped,                                   0, 100 },
   { { 28,  1,    0 },            electricMeterConfigurationCalibrationstarted,                     0, 120 },
   { { 30,  1,    0 },            electricMeterSecurityPhaseAVoltageLossDetected,                   1, 180 },
   { { 30,  1,    0 },            electricMeterSecurityPhaseBVoltageLossDetected,                   2, 180 },
   { { 30,  1,    0 },            electricMeterSecurityPhaseCVoltageLossDetected,                   3, 180 },
   { { 30,  1,    0 },            electricMeterSecurityVoltageRestored,                             0, 180 },
   { { 35,  1,    0 },            electricMeterloadControlEmergencySupplyCapacityLimitEventStarted, 0, 130 },
   { { 36,  1,    0 },            electricMeterloadControlEmergencySupplyCapacityLimitEventStopped, 0, 110 },
   { { 37,  1,    0 },            electricMeterloadControlSupplyCapacityLimitEventStarted,          0, 105 },
   { { 38,  1,    0 },            electricMeterloadControlSupplyCapacityLimitEventStopped,          0, 100 },
   { { 39,  1,    0 },            electricMeterRcdSwitchDataLogDisconnected,                        7, 250 },  // Not yet supported by meter (FW V4.15)
   { { 39,  1,    0 },            electricMeterRCDSwitchSupplyCapacityLimitDisconnected,           16, 108 },  // Not yet supported by meter (FW V4.15)
   { { 39,  1,    0 },            loadControlDeviceRCDSwitchEventLogDisconnected,                  64, 120 },  // Not yet supported by meter (FW V4.15)
   { { 40,  1,    0 },            electricMeterRcdSwitchDataLogConnected,                           7, 250 },  // Not yet supported by meter (FW V4.15)
   { { 40,  1,    0 },            electricMeterRCDSwitchSupplyCapacityLimitConnected,              16, 120 },  // Not yet supported by meter (FW V4.15)
   { { 40,  1,    0 },            loadControlDeviceRCDSwitchEventLogConnected,                     64, 120 },  // Not yet supported by meter (FW V4.15)
   { { 56,  1,    0 },            electricMeterTemperatureThresholdMaxLimitReached,                 0, 254 },
   { { 57,  1,    0 },            electricMeterTemperatureThresholdMaxLimitCleared,                 0, 180 },
   { { 60,  1,    0 },            electricMeterIODisabled,                                          0,  91 },  // Optical Port Lockout Activation.   // Not yet supported by meter (FW V4.15)
   { { 61,  1,    0 },            electricMeterIOEnabled,                                           0,  91 },  // Optical Port Lockout Deactivation.   // Not yet supported by meter (FW V4.15)
   { { 66,  1,    0 },            electricMeterPowerPhaseACurrentLimitReached,                      8, 100 },  // High Line Current Detected.   // Not yet supported by meter (FW V4.15)
   { { 66,  1,    0 },            electricMeterPowerPhaseBCurrentLimitReached,                     16, 100 },  // High Line Current Detected.   // Not yet supported by meter (FW V4.15)
   { { 66,  1,    0 },            electricMeterPowerPhaseCCurrentLimitReached,                     32, 100 },  // High Line Current Detected.   // Not yet supported by meter (FW V4.15)
   { { 67,  1,    0 },            electricMeterPowerCurrentMaxLimitCleared,                         0, 100 },  // Generic High Line Current Cleared.   // Not yet supported by meter (FW V4.15)
   { { 68,  1,    0 },            electricMeterPowerCurrentImbalanced,                              0, 102 },  // Not yet supported by meter (FW V4.15)
   { { 69,  1,    0 },            electricMeterPowerCurrentImbalanceCleared,                        0, 102 },  // Not yet supported by meter (FW V4.15)
   { { 70,  1,    0 },            electricMeterSecurityMagneticSwitchTamperDetected,                0, 105 },  // Not yet supported by meter (FW V4.15, expected in FW V4.17)
   { { 71,  1,    0 },            electricMeterSecurityMagneticSwitchTamperCleared,                 0, 105 },  // Not yet supported by meter (FW V4.15, expected in FW V4.17)
   { { 72,  1,    0 },            electricMeterPowerError,                                          0,  95 },  // Service Error Detected.   // Not yet supported by meter (FW V4.15)
   { { 73,  1,    0 },            electricMeterPowerErrorCleared,                                   0,  95 },  // Service Error Cleared.   // Not yet supported by meter (FW V4.15)
  }
#endif
;

#ifdef ANSI_SIG_GLOBALS

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

const directLookup_t directLookupTable[] =
{
   {
      .quantity                        = Hz,                /* Unit of Measure */
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, frequency ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 2,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = pF,                /* Unit of Measure */
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 0x96,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 2,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
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
      .quantity                        = vRMSA,                         /* Phase A RMS voltage  */
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 14,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = vRMSB,                         /* Phase B RMS voltage  */
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 18,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = vRMSC,                         /* Phase C RMS voltage  */
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 22,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = iA,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
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
      .quantity                        = iB,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 16,
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
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 20,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL
   },
   {
      .quantity                        = iPhaseN,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, imputedNeutralCurrent ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = highThresholdswell,   /* Unit of Measure */
      .typecast                        = intValue,
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
      .typecast                        = intValue,
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
      .tbl.pFunct                      = NULL,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterProgramLostState,         /* Meter detects program loss */
      .typecast                        = Boolean,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = reverseReactivePowerflowStatus,         /* Interpreted as Leading kVArs   */
      .typecast                        = Boolean,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterSecurityRotationReversed,    /* Meter detects inversion tamper  */
      .typecast                        = Boolean,
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
      .typecast                        = Boolean,
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
      .typecast                        = Boolean,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterVoltageLossDetected, /* Meter detects voltage loss */
      .typecast                        = Boolean,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterDiagRomMemoryErrorStatus, /* Meter detect ROM error  */
      .typecast                        = Boolean,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterBatteryStatus,            /* Meter detects low battery voltage */
      .typecast                        = Boolean,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterTempThreshMaxLimitReached,   /* Meter detects high temp alert */
      .typecast                        = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_ERROR_HISTORY ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl69_t, realTimeStatus.cautionRcd.bytes[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 2,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterDiagProcessorErrorStatus, /* Any Processor diagnostic error   */
      .typecast                        = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( stdTbl3_t, edMfgStatus1 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterDiagFlashMemoryErrorStatus,
      .typecast                        = Boolean,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterError,
      .typecast                        = Boolean,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterDiagRamMemoryErrorStatus, /* Meter detects RAM error */
      .typecast                        = Boolean,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = sag,                           /* Count of sag events  */
      .typecast                        = uintValue,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = swell,                         /* Count of swell events  */
      .typecast                        = uintValue,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = edManufacturer,
      .typecast                        = ASCIIStringValue,
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
      .quantity                        = edTemperature,                   /* Meter's current temperature */
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, temperature ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indCurrentPhaseAngleA,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 0,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltagePhaseAngleA,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 2,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indCurrentPhaseAngleB,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 4,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltagePhaseAngleB,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 6,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indCurrentPhaseAngleC,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 8,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltagePhaseAngleC,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = 10,
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indDistortionPf,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, duPF ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN1,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters ) + offsetof( DiagCounters_t, diag1 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN2,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters ) + offsetof( DiagCounters_t, diag2 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN3,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters ) + offsetof( DiagCounters_t, diag3 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN4,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters ) + offsetof( DiagCounters_t, diag4 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN5PhaseA,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters ) + offsetof( DiagCounters_t, diag5phA ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN5PhaseB,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters ) + offsetof( DiagCounters_t, diag5phB ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN5PhaseC,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters ) + offsetof( DiagCounters_t, diag5phC ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN5PhaseTotal,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters ) + offsetof( DiagCounters_t, diag5Total ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN6PhaseA,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters ) + offsetof( DiagCounters_t, diag6A ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN7PhaseA,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters ) + offsetof( DiagCounters_t, diag7A ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN8,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters ) + offsetof( DiagCounters_t, diag8 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN6PhaseB,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters2 ) + offsetof( DiagCounters2_t, diag6B ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN7PhaseB,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters2 ) + offsetof( DiagCounters2_t, diag7B ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN6PhaseC,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters2 ) + offsetof( DiagCounters2_t, diag6C ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indMeterDiagN7PhaseC,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters2 ) + offsetof( DiagCounters2_t, diag7C ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
#if 0
   {
      .quantity                        = indMeterTamperCount,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCounters2 ) + offsetof( DiagCounters2_t, diag7C ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
#endif

   {
      .quantity                        = meterSagPhAStarted,  /* Meter detects a sag on phase A */
      .typecast                        = Boolean,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterSagPhBStarted,  /* Meter detects a sag on phase B */
      .typecast                        = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCautions2 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,  /* diagnostic 6   */
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterSagPhCStarted,  /* Meter detects a sag on phase B */
      .typecast                        = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCautions2 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 2,  /* diagnostic 6   */
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterSwellPhAStarted,    /* Meter detects a swell on phase A */
      .typecast                        = Boolean,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterSwellPhBStarted,    /* Meter detects a swell on phase B */
      .typecast                        = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCautions2 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 1,  /* diagnostic 7   */
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterSwellPhCStarted,    /* Meter detects a swell on phase C */
      .typecast                        = Boolean,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl72_t, diagCautions2 ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 3,  /* diagnostic 7   */
      .tbl.tblInfo.width               = 1,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = meterProgramCount,             /* Number of times meter has been programmed */
      .typecast                        = uintValue,
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
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = energizationLoadSidePhaseA,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LOAD_CONTROL_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl115_t, loadSideVoltageMeas ) + offsetof( ElementLoadSideVoltage_t, phaseA),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = energizationLoadSidePhaseB,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LOAD_CONTROL_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl115_t, loadSideVoltageMeas ) + offsetof( ElementLoadSideVoltage_t, phaseB),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = energizationLoadSidePhaseC,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_LOAD_CONTROL_STATUS ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl115_t, loadSideVoltageMeas ) + offsetof( ElementLoadSideVoltage_t, phaseC),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = amiCommunSignalStrength,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_NETWORK_STATUS_DISPLAY_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl119_t, AmiRssiValue ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = maxAmiCommunSignalStrength,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_NETWORK_STATUS_DISPLAY_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl119_t, AmiRssiMax ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = thdphaseAV,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, vthd[0] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = thdphaseBV,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, vthd[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = thdphaseCV,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, vthd[2] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = thdphaseAA,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, ithd[0] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = thdphaseBA,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, ithd[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = thdphaseCA,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, ithd[2] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltRmsPhaseAtoNFundPlus,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToNeutralFundPlus[0] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltRmsPhaseBtoNFundPlus,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToNeutralFundPlus[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltRmsPhaseCtoNFundPlus,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToNeutralFundPlus[2] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltRmsPhaseAtoNFundOnly,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToNeutralFundOnly[0] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltRmsPhaseBtoNFundOnly,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToNeutralFundOnly[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltRmsPhaseCtoNFundOnly,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToNeutralFundOnly[2] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltRmsPhaseAtoCFundPlus,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToLineFundPlus[0] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltRmsPhaseBtoAFundPlus,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToLineFundPlus[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
    {
      .quantity                        = indVoltRmsPhaseCtoBFundPlus,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToLineFundPlus[2] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltRmsPhaseAtoCFundOnly,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToLineFundOnly[0] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltRmsPhaseBtoAFundOnly,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToLineFundOnly[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indVoltRmsPhaseCtoBFundOnly,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, lineToLineFundOnly[2] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indCurrentPhaseAFundOnly,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, currentFundOnly[0] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indCurrentPhaseBFundOnly,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, currentFundOnly[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indCurrentPhaseCFundOnly,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, currentFundOnly[2] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indDeviceBatteryVoltage,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, batteryVoltage ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 1,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indTotDistorationDemandPhaseA,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, tdd[0] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indTotDistorationDemandPhaseB,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, tdd[1] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = indTotDistorationDemandPhaseC,
      .typecast                        = intValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_PRESENT_REGISTER_DATA ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl110_t, tdd[2] ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 1 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = edPwrRelSustainedIntrpMin,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_SECURITY_LOG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl78_t, nbrSustainedMinutes ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = edPwrRelSustainedIntrpCnt,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_SECURITY_LOG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl78_t, nbrSustainedInterrupts ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = edPwrRelMomentaryIntrpCnt,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_SECURITY_LOG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl78_t, nbrMomentaryInterrupts ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   },
   {
      .quantity                        = edPwrRelMomentaryIntrpEventCnt,
      .typecast                        = uintValue,
      .tbl.tblInfo.tableID             = BIG_ENDIAN_16BIT( MFG_TBL_SECURITY_LOG ),
      .tbl.tblInfo.offset[0]           = 0,
      .tbl.tblInfo.offset[1]           = 0,
      .tbl.tblInfo.offset[2]           = offsetof( mfgTbl78_t, nbrMomentaryEvents ),
      .tbl.tblInfo.cnt                 = BIG_ENDIAN_16BIT( 2 ),
      .tbl.tblInfo.lsb                 = 0,
      .tbl.tblInfo.width               = 0,
      .tbl.multiplier15                = 1,
      .tbl.multiplier12                = 0,
      .tbl.dataFmt                     = eNIFMT_INT32,
      .tbl.pFunct                      = NULL,
   }
};

static const OL_tTableDef quantitySigTblLP_                                    =
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
{ //   ST12      ST14      pres   presA   presB   presC   presD    prev   prevA   prevB   prevC   prevD
   { 0x00284000, 0x0000,   3182,   3604,   4034,   4464,   4894,   3385,   3819,   4249,   4679,   5109 },// Wh Element A Q1 fund + harmonics
   { 0x00288000, 0x0000,   3183,   3605,   4035,   4465,   4895,   3386,   3820,   4250,   4680,   5110 },// Wh Element A Q2 fund + harmonics
   { 0x00290000, 0x0000,   3184,   3606,   4036,   4466,   4896,   3387,   3821,   4251,   4681,   5111 },// Wh Element A Q3 fund + harmonics
   { 0x002A0000, 0x0000,   3185,   3607,   4037,   4467,   4897,   3388,   3822,   4252,   4682,   5112 },// Wh Element A Q4 fund + harmonics
   { 0x002A4000, 0x0000,   3186,   3608,   4038,   4468,   4898,   3389,   3823,   4253,   4683,   5113 },// Wh Element A Q1+Q4 fund + harmonics
   { 0x00298000, 0x0000,   3187,   3609,   4039,   4469,   4899,   3390,   3824,   4254,   4684,   5114 },// Wh Element A Q2+Q3 fund + harmonics
   { 0x002BC000, 0x0000,   3188,   3610,   4040,   4470,   4900,   3391,   3825,   4255,   4685,   5115 },// Wh Element A (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x002FC000, 0x0000,   3189,   3611,   4041,   4471,   4901,   3392,   3826,   4256,   4686,   5116 },// Wh Element A (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00304000, 0x0000,   3190,   3612,   4042,   4472,   4902,   3393,   3827,   4257,   4687,   5117 },// Wh Element B Q1 fund + harmonics
   { 0x00308000, 0x0000,   3191,   3613,   4043,   4473,   4903,   3394,   3828,   4258,   4688,   5118 },// Wh Element B Q2 fund + harmonics
   { 0x00310000, 0x0000,   3192,   3614,   4044,   4474,   4904,   3395,   3829,   4259,   4689,   5119 },// Wh Element B Q3 fund + harmonics
   { 0x00320000, 0x0000,   3193,   3615,   4045,   4475,   4905,   3396,   3830,   4260,   4690,   5120 },// Wh Element B Q4 fund + harmonics
   { 0x00324000, 0x0000,   3194,   3616,   4046,   4476,   4906,   3397,   3831,   4261,   4691,   5121 },// Wh Element B Q1+Q4 fund + harmonics
   { 0x00318000, 0x0000,   3195,   3617,   4047,   4477,   4907,   3398,   3832,   4262,   4692,   5122 },// Wh Element B Q2+Q3 fund + harmonics
   { 0x0033C000, 0x0000,   3196,   3618,   4048,   4478,   4908,   3399,   3833,   4263,   4693,   5123 },// Wh Element B (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x0037C000, 0x0000,   3197,   3619,   4049,   4479,   4909,   3400,   3834,   4264,   4694,   5124 },// Wh Element B (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00384000, 0x0000,   3198,   3620,   4050,   4480,   4910,   3401,   3835,   4265,   4695,   5125 },// Wh Element C Q1 fund + harmonics
   { 0x00388000, 0x0000,   3199,   3621,   4051,   4481,   4911,   3402,   3836,   4266,   4696,   5126 },// Wh Element C Q2 fund + harmonics
   { 0x00390000, 0x0000,   3200,   3622,   4052,   4482,   4912,   3403,   3837,   4267,   4697,   5127 },// Wh Element C Q3 fund + harmonics
   { 0x003A0000, 0x0000,   3201,   3623,   4053,   4483,   4913,   3404,   3838,   4268,   4698,   5128 },// Wh Element C Q4 fund + harmonics
   { 0x003A4000, 0x0000,   3202,   3624,   4054,   4484,   4914,   3405,   3839,   4269,   4699,   5129 },// Wh Element C Q1+Q4 fund + harmonics
   { 0x00398000, 0x0000,   3203,   3625,   4055,   4485,   4915,   3406,   3840,   4270,   4700,   5130 },// Wh Element C Q2+Q3 fund + harmonics
   { 0x003BC000, 0x0000,   3204,   3626,   4056,   4486,   4916,   3407,   3841,   4271,   4701,   5131 },// Wh Element C (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x003FC000, 0x0000,   3205,   3627,   4057,   4487,   4917,   3408,   3842,   4272,   4702,   5132 },// Wh Element C (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00684000, 0x0000,   3206,   3628,   4058,   4488,   4918,   3409,   3843,   4273,   4703,   5133 },// Wh Element A Q1 fund only
   { 0x00688000, 0x0000,   3207,   3629,   4059,   4489,   4919,   3410,   3844,   4274,   4704,   5134 },// Wh Element A Q2 fund only
   { 0x00690000, 0x0000,   3208,   3630,   4060,   4490,   4920,   3411,   3845,   4275,   4705,   5135 },// Wh Element A Q3 fund only
   { 0x006A0000, 0x0000,   3209,   3631,   4061,   4491,   4921,   3412,   3846,   4276,   4706,   5136 },// Wh Element A Q4 fund only
   { 0x006A4000, 0x0000,   3210,   3632,   4062,   4492,   4922,   3413,   3847,   4277,   4707,   5137 },// Wh Element A Q1+Q4 fund only
   { 0x00698000, 0x0000,   3211,   3633,   4063,   4493,   4923,   3414,   3848,   4278,   4708,   5138 },// Wh Element A Q2+Q3 fund only
   { 0x006BC000, 0x0000,   3212,   3634,   4064,   4494,   4924,   3415,   3849,   4279,   4709,   5139 },// Wh Element A (Q1+Q4)+(Q2+Q3) fund only
   { 0x006FC000, 0x0000,   3213,   3635,   4065,   4495,   4925,   3416,   3850,   4280,   4710,   5140 },// Wh Element A (Q1+Q4)-(Q2+Q3) fund only
   { 0x00704000, 0x0000,   3214,   3636,   4066,   4496,   4926,   3417,   3851,   4281,   4711,   5141 },// Wh Element B Q1 fund only
   { 0x00708000, 0x0000,   3215,   3637,   4067,   4497,   4927,   3418,   3852,   4282,   4712,   5142 },// Wh Element B Q2 fund only
   { 0x00710000, 0x0000,   3216,   3638,   4068,   4498,   4928,   3419,   3853,   4283,   4713,   5143 },// Wh Element B Q3 fund only
   { 0x00720000, 0x0000,   3217,   3639,   4069,   4499,   4929,   3420,   3854,   4284,   4714,   5144 },// Wh Element B Q4 fund only
   { 0x00724000, 0x0000,   3218,   3640,   4070,   4500,   4930,   3421,   3855,   4285,   4715,   5145 },// Wh Element B Q1+Q4 fund only
   { 0x00718000, 0x0000,   3219,   3641,   4071,   4501,   4931,   3422,   3856,   4286,   4716,   5146 },// Wh Element B Q2+Q3 fund only
   { 0x0073C000, 0x0000,   3220,   3642,   4072,   4502,   4932,   3423,   3857,   4287,   4717,   5147 },// Wh Element B (Q1+Q4)+(Q2+Q3) fund only
   { 0x0077C000, 0x0000,   3221,   3643,   4073,   4503,   4933,   3424,   3858,   4288,   4718,   5148 },// Wh Element B (Q1+Q4)-(Q2+Q3) fund only
   { 0x00784000, 0x0000,   3222,   3644,   4074,   4504,   4934,   3425,   3859,   4289,   4719,   5149 },// Wh Element C Q1 fund only
   { 0x00788000, 0x0000,   3223,   3645,   4075,   4505,   4935,   3426,   3860,   4290,   4720,   5150 },// Wh Element C Q2 fund only
   { 0x00790000, 0x0000,   3224,   3646,   4076,   4506,   4936,   3427,   3861,   4291,   4721,   5151 },// Wh Element C Q3 fund only
   { 0x007A0000, 0x0000,   3225,   3647,   4077,   4507,   4937,   3428,   3862,   4292,   4722,   5152 },// Wh Element C Q4 fund only
   { 0x007A4000, 0x0000,   3226,   3648,   4078,   4508,   4938,   3429,   3863,   4293,   4723,   5153 },// Wh Element C Q1+Q4 fund only
   { 0x00798000, 0x0000,   3227,   3649,   4079,   4509,   4939,   3430,   3864,   4294,   4724,   5154 },// Wh Element C Q2+Q3 fund only
   { 0x007BC000, 0x0000,   3228,   3650,   4080,   4510,   4940,   3431,   3865,   4295,   4725,   5155 },// Wh Element C (Q1+Q4)+(Q2+Q3) fund only
   { 0x007FC000, 0x0000,   3229,   3651,   4081,   4511,   4941,   3432,   3866,   4296,   4726,   5156 },// Wh Element C (Q1+Q4)-(Q2+Q3) fund only
   { 0x00004000, 0x0000,    307,   3652,   4082,   4512,   4942,   3433,   3867,   4297,   4727,   5157 },// Wh Q1 fund + harmonics
   { 0x00008000, 0x0000,    308,   3653,   4083,   4513,   4943,   3434,   3868,   4298,   4728,   5158 },// Wh Q2 fund + harmonics
   { 0x00010000, 0x0000,    309,   3654,   4084,   4514,   4944,   3435,   3869,   4299,   4729,   5159 },// Wh Q3 fund + harmonics
   { 0x00020000, 0x0000,    310,   3655,   4085,   4515,   4945,   3436,   3870,   4300,   4730,   5160 },// Wh Q4 fund + harmonics
   { 0x00024000, 0x0000,     12,     57,     58,     59,    139,    578,    587,    596,    605,    614 },// Wh Q1+Q4 fund + harmonics
   { 0x00018000, 0x0000,     15,    143,    145,    147,    149,    579,    588,    597,    606,    615 },// Wh Q2+Q3 fund + harmonics
   { 0x0003C000, 0x0000,     19,     60,     61,     62,    242,    580,    589,    598,    607,    616 },// Wh (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x0007C000, 0x0000,     20,    105,    106,    107,    108,    581,    590,    599,    608,    617 },// Wh (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00404000, 0x0000,   3230,   3656,   4086,   4516,   4946,   3437,   3871,   4301,   4731,   5161 },// Wh Q1 fund only
   { 0x00408000, 0x0000,   3231,   3657,   4087,   4517,   4947,   3438,   3872,   4302,   4732,   5162 },// Wh Q2 fund only
   { 0x00410000, 0x0000,   3232,   3658,   4088,   4518,   4948,   3439,   3873,   4303,   4733,   5163 },// Wh Q3 fund only
   { 0x00420000, 0x0000,   3233,   3659,   4089,   4519,   4949,   3440,   3874,   4304,   4734,   5164 },// Wh Q4 fund only
   { 0x00424000, 0x0000,   3234,   3660,   4090,   4520,   4950,   3441,   3875,   4305,   4735,   5165 },// Wh Q1+Q4 fund only
   { 0x00418000, 0x0000,   3235,   3661,   4091,   4521,   4951,   3442,   3876,   4306,   4736,   5166 },// Wh Q2+Q3 fund only
   { 0x0043C000, 0x0000,   3236,   3662,   4092,   4522,   4952,   3443,   3877,   4307,   4737,   5167 },// Wh (Q1+Q4)+(Q2+Q3) fund only
   { 0x0047C000, 0x0000,   3237,   3663,   4093,   4523,   4953,   3444,   3878,   4308,   4738,   5168 },// Wh (Q1+Q4)-(Q2+Q3) fund only
   { 0x00284001, 0x0000,   3238,   3664,   4094,   4524,   4954,   3445,   3879,   4309,   4739,   5169 },// varh Element A Q1 fund + harmonics
   { 0x00288001, 0x0000,   3239,   3665,   4095,   4525,   4955,   3446,   3880,   4310,   4740,   5170 },// varh Element A Q2 fund + harmonics
   { 0x00290001, 0x0000,   3240,   3666,   4096,   4526,   4956,   3447,   3881,   4311,   4741,   5171 },// varh Element A Q3 fund + harmonics
   { 0x002A0001, 0x0000,   3241,   3667,   4097,   4527,   4957,   3448,   3882,   4312,   4742,   5172 },// varh Element A Q4 fund + harmonics
   { 0x0028C001, 0x0000,   3242,   3668,   4098,   4528,   4958,   3449,   3883,   4313,   4743,   5173 },// varh Element A Q1+Q2 fund + harmonics
   { 0x002B0001, 0x0000,   3243,   3669,   4099,   4529,   4959,   3450,   3884,   4314,   4744,   5174 },// varh Element A Q3+Q4 fund + harmonics
   { 0x002BC001, 0x0000,   3244,   3670,   4100,   4530,   4960,   3451,   3885,   4315,   4745,   5175 },// varh Element A (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x002FC001, 0x0000,   3245,   3671,   4101,   4531,   4961,   3452,   3886,   4316,   4746,   5176 },// varh Element A (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00304001, 0x0000,   3246,   3672,   4102,   4532,   4962,   3453,   3887,   4317,   4747,   5177 },// varh Element B Q1 fund + harmonics
   { 0x00308001, 0x0000,   3247,   3673,   4103,   4533,   4963,   3454,   3888,   4318,   4748,   5178 },// varh Element B Q2 fund + harmonics
   { 0x00310001, 0x0000,   3248,   3674,   4104,   4534,   4964,   3455,   3889,   4319,   4749,   5179 },// varh Element B Q3 fund + harmonics
   { 0x00320001, 0x0000,   3249,   3675,   4105,   4535,   4965,   3456,   3890,   4320,   4750,   5180 },// varh Element B Q4 fund + harmonics
   { 0x0030C001, 0x0000,   3250,   3676,   4106,   4536,   4966,   3457,   3891,   4321,   4751,   5181 },// varh Element B Q1+Q2 fund + harmonics
   { 0x00330001, 0x0000,   3251,   3677,   4107,   4537,   4967,   3458,   3892,   4322,   4752,   5182 },// varh Element B Q3+Q4 fund + harmonics
   { 0x0033C001, 0x0000,   3252,   3678,   4108,   4538,   4968,   3459,   3893,   4323,   4753,   5183 },// varh Element B (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x0037C001, 0x0000,   3253,   3679,   4109,   4539,   4969,   3460,   3894,   4324,   4754,   5184 },// varh Element B (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00384001, 0x0000,   3254,   3680,   4110,   4540,   4970,   3461,   3895,   4325,   4755,   5185 },// varh Element C Q1 fund + harmonics
   { 0x00388001, 0x0000,   3255,   3681,   4111,   4541,   4971,   3462,   3896,   4326,   4756,   5186 },// varh Element C Q2 fund + harmonics
   { 0x00390001, 0x0000,   3256,   3682,   4112,   4542,   4972,   3463,   3897,   4327,   4757,   5187 },// varh Element C Q3 fund + harmonics
   { 0x003A0001, 0x0000,   3257,   3683,   4113,   4543,   4973,   3464,   3898,   4328,   4758,   5188 },// varh Element C Q4 fund + harmonics
   { 0x0038C001, 0x0000,   3258,   3684,   4114,   4544,   4974,   3465,   3899,   4329,   4759,   5189 },// varh Element C Q1+Q2 fund + harmonics
   { 0x003B0001, 0x0000,   3259,   3685,   4115,   4545,   4975,   3466,   3900,   4330,   4760,   5190 },// varh Element C Q3+Q4 fund + harmonics
   { 0x003BC001, 0x0000,   3260,   3686,   4116,   4546,   4976,   3467,   3901,   4331,   4761,   5191 },// varh Element C (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x003FC001, 0x0000,   3261,   3687,   4117,   4547,   4977,   3468,   3902,   4332,   4762,   5192 },// varh Element C (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00684001, 0x0000,   3262,   3688,   4118,   4548,   4978,   3469,   3903,   4333,   4763,   5193 },// varh Element A Q1 fund only
   { 0x00688001, 0x0000,   3263,   3689,   4119,   4549,   4979,   3470,   3904,   4334,   4764,   5194 },// varh Element A Q2 fund only
   { 0x00690001, 0x0000,   3264,   3690,   4120,   4550,   4980,   3471,   3905,   4335,   4765,   5195 },// varh Element A Q3 fund only
   { 0x006A0001, 0x0000,   3265,   3691,   4121,   4551,   4981,   3472,   3906,   4336,   4766,   5196 },// varh Element A Q4 fund only
   { 0x0068C001, 0x0000,   3266,   3692,   4122,   4552,   4982,   3473,   3907,   4337,   4767,   5197 },// varh Element A Q1+Q2 fund only
   { 0x006B0001, 0x0000,   3267,   3693,   4123,   4553,   4983,   3474,   3908,   4338,   4768,   5198 },// varh Element A Q3+Q4 fund only
   { 0x006BC001, 0x0000,   3268,   3694,   4124,   4554,   4984,   3475,   3909,   4339,   4769,   5199 },// varh Element A (Q1+Q2)+(Q3+Q4) fund only
   { 0x006FC001, 0x0000,   3269,   3695,   4125,   4555,   4985,   3476,   3910,   4340,   4770,   5200 },// varh Element A (Q1+Q2)-(Q3+Q4) fund only
   { 0x00704001, 0x0000,   3270,   3696,   4126,   4556,   4986,   3477,   3911,   4341,   4771,   5201 },// varh Element B Q1 fund only
   { 0x00708001, 0x0000,   3271,   3697,   4127,   4557,   4987,   3478,   3912,   4342,   4772,   5202 },// varh Element B Q2 fund only
   { 0x00710001, 0x0000,   3272,   3698,   4128,   4558,   4988,   3479,   3913,   4343,   4773,   5203 },// varh Element B Q3 fund only
   { 0x00720001, 0x0000,   3273,   3699,   4129,   4559,   4989,   3480,   3914,   4344,   4774,   5204 },// varh Element B Q4 fund only
   { 0x0070C001, 0x0000,   3274,   3700,   4130,   4560,   4990,   3481,   3915,   4345,   4775,   5205 },// varh Element B Q1+Q2 fund only
   { 0x00730001, 0x0000,   3275,   3701,   4131,   4561,   4991,   3482,   3916,   4346,   4776,   5206 },// varh Element B Q3+Q4 fund only
   { 0x0073C001, 0x0000,   3276,   3702,   4132,   4562,   4992,   3483,   3917,   4347,   4777,   5207 },// varh Element B (Q1+Q2)+(Q3+Q4) fund only
   { 0x0077C001, 0x0000,   3277,   3703,   4133,   4563,   4993,   3484,   3918,   4348,   4778,   5208 },// varh Element B (Q1+Q2)-(Q3+Q4) fund only
   { 0x00784001, 0x0000,   3278,   3704,   4134,   4564,   4994,   3485,   3919,   4349,   4779,   5209 },// varh Element C Q1 fund only
   { 0x00788001, 0x0000,   3279,   3705,   4135,   4565,   4995,   3486,   3920,   4350,   4780,   5210 },// varh Element C Q2 fund only
   { 0x00790001, 0x0000,   3280,   3706,   4136,   4566,   4996,   3487,   3921,   4351,   4781,   5211 },// varh Element C Q3 fund only
   { 0x007A0001, 0x0000,   3281,   3707,   4137,   4567,   4997,   3488,   3922,   4352,   4782,   5212 },// varh Element C Q4 fund only
   { 0x0078C001, 0x0000,   3282,   3708,   4138,   4568,   4998,   3489,   3923,   4353,   4783,   5213 },// varh Element C Q1+Q2 fund only
   { 0x007B0001, 0x0000,   3283,   3709,   4139,   4569,   4999,   3490,   3924,   4354,   4784,   5214 },// varh Element C Q3+Q4 fund only
   { 0x007BC001, 0x0000,   3284,   3710,   4140,   4570,   5000,   3491,   3925,   4355,   4785,   5215 },// varh Element C (Q1+Q2)+(Q3+Q4) fund only
   { 0x007FC001, 0x0000,   3285,   3711,   4141,   4571,   5001,   3492,   3926,   4356,   4786,   5216 },// varh Element C (Q1+Q2)-(Q3+Q4) fund only
   { 0x00004001, 0x0000,    265,   3712,   4142,   4572,   5002,   3493,   3927,   4357,   4787,   5217 },// varh Q1 fund + harmonics
   { 0x00008001, 0x0000,    266,   3713,   4143,   4573,   5003,   3494,   3928,   4358,   4788,   5218 },// varh Q2 fund + harmonics
   { 0x00010001, 0x0000,    267,   3714,   4144,   4574,   5004,   3495,   3929,   4359,   4789,   5219 },// varh Q3 fund + harmonics
   { 0x00020001, 0x0000,    268,   3715,   4145,   4575,   5005,   3496,   3930,   4360,   4790,   5220 },// varh Q4 fund + harmonics
   { 0x0000C001, 0x0000,     13,    136,    137,    138,    140,    582,    591,    600,    609,    618 },// varh Q1+Q2 fund + harmonics
   { 0x00030001, 0x0000,     16,    144,    146,    148,    150,    583,    592,    601,    610,    619 },// varh Q3+Q4 fund + harmonics
   { 0x0003C001, 0x0000,    247,    243,    244,    245,    246,    584,    593,    602,    611,    620 },// varh (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x0007C001, 0x0000,     21,    103,    102,    101,    104,    585,    594,    603,    612,    621 },// varh (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00404001, 0x0000,   3286,   3716,   4146,   4576,   5006,   3497,   3931,   4361,   4791,   5221 },// varh Q1 fund only
   { 0x00408001, 0x0000,   3287,   3717,   4147,   4577,   5007,   3498,   3932,   4362,   4792,   5222 },// varh Q2 fund only
   { 0x00410001, 0x0000,   3288,   3718,   4148,   4578,   5008,   3499,   3933,   4363,   4793,   5223 },// varh Q3 fund only
   { 0x00420001, 0x0000,   3289,   3719,   4149,   4579,   5009,   3500,   3934,   4364,   4794,   5224 },// varh Q4 fund only
   { 0x0040C001, 0x0000,   3290,   3720,   4150,   4580,   5010,   3501,   3935,   4365,   4795,   5225 },// varh Q1+Q2 fund only
   { 0x00430001, 0x0000,   3291,   3721,   4151,   4581,   5011,   3502,   3936,   4366,   4796,   5226 },// varh Q3+Q4 fund only
   { 0x0043C001, 0x0000,   3292,   3722,   4152,   4582,   5012,   3503,   3937,   4367,   4797,   5227 },// varh (Q1+Q2)+(Q3+Q4) fund only
   { 0x0047C001, 0x0000,   3293,   3723,   4153,   4583,   5013,   3504,   3938,   4368,   4798,   5228 },// varh (Q1+Q2)-(Q3+Q4) fund only
   { 0x80284001, 0x0000,  10159,  10817,  11005,  11193,  11381,  10253,  10911,  11099,  11287,  11475 },// varh Element A Q1 fund + harmonics
   { 0x80288001, 0x0000,  10160,  10818,  11006,  11194,  11382,  10254,  10912,  11100,  11288,  11476 },// varh Element A Q2 fund + harmonics
   { 0x80290001, 0x0000,  10161,  10819,  11007,  11195,  11383,  10255,  10913,  11101,  11289,  11477 },// varh Element A Q3 fund + harmonics
   { 0x802A0001, 0x0000,  10162,  10820,  11008,  11196,  11384,  10256,  10914,  11102,  11290,  11478 },// varh Element A Q4 fund + harmonics
   { 0x8028C001, 0x0000,  10163,  10821,  11009,  11197,  11385,  10257,  10915,  11103,  11291,  11479 },// varh Element A Q1+Q2 fund + harmonics
   { 0x802B0001, 0x0000,  10164,  10822,  11010,  11198,  11386,  10258,  10916,  11104,  11292,  11480 },// varh Element A Q3+Q4 fund + harmonics
   { 0x802BC001, 0x0000,  10165,  10823,  11011,  11199,  11387,  10259,  10917,  11105,  11293,  11481 },// varh Element A (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x802FC001, 0x0000,  10166,  10824,  11012,  11200,  11388,  10260,  10918,  11106,  11294,  11482 },// varh Element A (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x80304001, 0x0000,  10167,  10825,  11013,  11201,  11389,  10261,  10919,  11107,  11295,  11483 },// varh Element B Q1 fund + harmonics
   { 0x80308001, 0x0000,  10168,  10826,  11014,  11202,  11390,  10262,  10920,  11108,  11296,  11484 },// varh Element B Q2 fund + harmonics
   { 0x80310001, 0x0000,  10169,  10827,  11015,  11203,  11391,  10263,  10921,  11109,  11297,  11485 },// varh Element B Q3 fund + harmonics
   { 0x80320001, 0x0000,  10170,  10828,  11016,  11204,  11392,  10264,  10922,  11110,  11298,  11486 },// varh Element B Q4 fund + harmonics
   { 0x8030C001, 0x0000,  10171,  10829,  11017,  11205,  11393,  10265,  10923,  11111,  11299,  11487 },// varh Element B Q1+Q2 fund + harmonics
   { 0x80330001, 0x0000,  10172,  10830,  11018,  11206,  11394,  10266,  10924,  11112,  11300,  11488 },// varh Element B Q3+Q4 fund + harmonics
   { 0x8033C001, 0x0000,  10173,  10831,  11019,  11207,  11395,  10267,  10925,  11113,  11301,  11489 },// varh Element B (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x8037C001, 0x0000,  10174,  10832,  11020,  11208,  11396,  10268,  10926,  11114,  11302,  11490 },// varh Element B (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x80384001, 0x0000,  10175,  10833,  11021,  11209,  11397,  10269,  10927,  11115,  11303,  11491 },// varh Element C Q1 fund + harmonics
   { 0x80388001, 0x0000,  10176,  10834,  11022,  11210,  11398,  10270,  10928,  11116,  11304,  11492 },// varh Element C Q2 fund + harmonics
   { 0x80390001, 0x0000,  10177,  10835,  11023,  11211,  11399,  10271,  10929,  11117,  11305,  11493 },// varh Element C Q3 fund + harmonics
   { 0x803A0001, 0x0000,  10178,  10836,  11024,  11212,  11400,  10272,  10930,  11118,  11306,  11494 },// varh Element C Q4 fund + harmonics
   { 0x8038C001, 0x0000,  10179,  10837,  11025,  11213,  11401,  10273,  10931,  11119,  11307,  11495 },// varh Element C Q1+Q2 fund + harmonics
   { 0x803B0001, 0x0000,  10180,  10838,  11026,  11214,  11402,  10274,  10932,  11120,  11308,  11496 },// varh Element C Q3+Q4 fund + harmonics
   { 0x803BC001, 0x0000,  10181,  10839,  11027,  11215,  11403,  10275,  10933,  11121,  11309,  11497 },// varh Element C (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x803FC001, 0x0000,  10182,  10840,  11028,  11216,  11404,  10276,  10934,  11122,  11310,  11498 },// varh Element C (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x80684001, 0x0000,  10183,  10841,  11029,  11217,  11405,  10277,  10935,  11123,  11311,  11499 },// varh Element A Q1 fund only
   { 0x80688001, 0x0000,  10184,  10842,  11030,  11218,  11406,  10278,  10936,  11124,  11312,  11500 },// varh Element A Q2 fund only
   { 0x80690001, 0x0000,  10185,  10843,  11031,  11219,  11407,  10279,  10937,  11125,  11313,  11501 },// varh Element A Q3 fund only
   { 0x806A0001, 0x0000,  10186,  10844,  11032,  11220,  11408,  10280,  10938,  11126,  11314,  11502 },// varh Element A Q4 fund only
   { 0x8068C001, 0x0000,  10187,  10845,  11033,  11221,  11409,  10281,  10939,  11127,  11315,  11503 },// varh Element A Q1+Q2 fund only
   { 0x806B0001, 0x0000,  10188,  10846,  11034,  11222,  11410,  10282,  10940,  11128,  11316,  11504 },// varh Element A Q3+Q4 fund only
   { 0x806BC001, 0x0000,  10189,  10847,  11035,  11223,  11411,  10283,  10941,  11129,  11317,  11505 },// varh Element A (Q1+Q2)+(Q3+Q4) fund only
   { 0x806FC001, 0x0000,  10190,  10848,  11036,  11224,  11412,  10284,  10942,  11130,  11318,  11506 },// varh Element A (Q1+Q2)-(Q3+Q4) fund only
   { 0x80704001, 0x0000,  10191,  10849,  11037,  11225,  11413,  10285,  10943,  11131,  11319,  11507 },// varh Element B Q1 fund only
   { 0x80708001, 0x0000,  10192,  10850,  11038,  11226,  11414,  10286,  10944,  11132,  11320,  11508 },// varh Element B Q2 fund only
   { 0x80710001, 0x0000,  10193,  10851,  11039,  11227,  11415,  10287,  10945,  11133,  11321,  11509 },// varh Element B Q3 fund only
   { 0x80720001, 0x0000,  10194,  10852,  11040,  11228,  11416,  10288,  10946,  11134,  11322,  11510 },// varh Element B Q4 fund only
   { 0x8070C001, 0x0000,  10195,  10853,  11041,  11229,  11417,  10289,  10947,  11135,  11323,  11511 },// varh Element B Q1+Q2 fund only
   { 0x80730001, 0x0000,  10196,  10854,  11042,  11230,  11418,  10290,  10948,  11136,  11324,  11512 },// varh Element B Q3+Q4 fund only
   { 0x8073C001, 0x0000,  10197,  10855,  11043,  11231,  11419,  10291,  10949,  11137,  11325,  11513 },// varh Element B (Q1+Q2)+(Q3+Q4) fund only
   { 0x8077C001, 0x0000,  10198,  10856,  11044,  11232,  11420,  10292,  10950,  11138,  11326,  11514 },// varh Element B (Q1+Q2)-(Q3+Q4) fund only
   { 0x80784001, 0x0000,  10199,  10857,  11045,  11233,  11421,  10293,  10951,  11139,  11327,  11515 },// varh Element C Q1 fund only
   { 0x80788001, 0x0000,  10200,  10858,  11046,  11234,  11422,  10294,  10952,  11140,  11328,  11516 },// varh Element C Q2 fund only
   { 0x80790001, 0x0000,  10201,  10859,  11047,  11235,  11423,  10295,  10953,  11141,  11329,  11517 },// varh Element C Q3 fund only
   { 0x807A0001, 0x0000,  10202,  10860,  11048,  11236,  11424,  10296,  10954,  11142,  11330,  11518 },// varh Element C Q4 fund only
   { 0x8078C001, 0x0000,  10203,  10861,  11049,  11237,  11425,  10297,  10955,  11143,  11331,  11519 },// varh Element C Q1+Q2 fund only
   { 0x807B0001, 0x0000,  10204,  10862,  11050,  11238,  11426,  10298,  10956,  11144,  11332,  11520 },// varh Element C Q3+Q4 fund only
   { 0x807BC001, 0x0000,  10205,  10863,  11051,  11239,  11427,  10299,  10957,  11145,  11333,  11521 },// varh Element C (Q1+Q2)+(Q3+Q4) fund only
   { 0x807FC001, 0x0000,  10206,  10864,  11052,  11240,  11428,  10300,  10958,  11146,  11334,  11522 },// varh Element C (Q1+Q2)-(Q3+Q4) fund only
   { 0x80004001, 0x0000,  10207,  10865,  11053,  11241,  11429,  10301,  10959,  11147,  11335,  11523 },// varh Q1 fund + harmonics
   { 0x80008001, 0x0000,  10208,  10866,  11054,  11242,  11430,  10302,  10960,  11148,  11336,  11524 },// varh Q2 fund + harmonics
   { 0x80010001, 0x0000,  10209,  10867,  11055,  11243,  11431,  10303,  10961,  11149,  11337,  11525 },// varh Q3 fund + harmonics
   { 0x80020001, 0x0000,  10210,  10868,  11056,  11244,  11432,  10304,  10962,  11150,  11338,  11526 },// varh Q4 fund + harmonics
   { 0x8000C001, 0x0000,  10211,  10869,  11057,  11245,  11433,  10305,  10963,  11151,  11339,  11527 },// varh Q1+Q2 fund + harmonics
   { 0x80030001, 0x0000,  10212,  10870,  11058,  11246,  11434,  10306,  10964,  11152,  11340,  11528 },// varh Q3+Q4 fund + harmonics
   { 0x8003C001, 0x0000,  10213,  10871,  11059,  11247,  11435,  10307,  10965,  11153,  11341,  11529 },// varh (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x8007C001, 0x0000,  10214,  10872,  11060,  11248,  11436,  10308,  10966,  11154,  11342,  11530 },// varh (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x80404001, 0x0000,  10215,  10873,  11061,  11249,  11437,  10309,  10967,  11155,  11343,  11531 },// varh Q1 fund only
   { 0x80408001, 0x0000,  10216,  10874,  11062,  11250,  11438,  10310,  10968,  11156,  11344,  11532 },// varh Q2 fund only
   { 0x80410001, 0x0000,  10217,  10875,  11063,  11251,  11439,  10311,  10969,  11157,  11345,  11533 },// varh Q3 fund only
   { 0x80420001, 0x0000,  10218,  10876,  11064,  11252,  11440,  10312,  10970,  11158,  11346,  11534 },// varh Q4 fund only
   { 0x8040C001, 0x0000,  10219,  10877,  11065,  11253,  11441,  10313,  10971,  11159,  11347,  11535 },// varh Q1+Q2 fund only
   { 0x80430001, 0x0000,  10220,  10878,  11066,  11254,  11442,  10314,  10972,  11160,  11348,  11536 },// varh Q3+Q4 fund only
   { 0x8043C001, 0x0000,  10221,  10879,  11067,  11255,  11443,  10315,  10973,  11161,  11349,  11537 },// varh (Q1+Q2)+(Q3+Q4) fund only
   { 0x8047C001, 0x0000,  10222,  10880,  11068,  11256,  11444,  10316,  10974,  11162,  11350,  11538 },// varh (Q1+Q2)-(Q3+Q4) fund only

   { 0x00284002, 0x0000,   3297,   3727,   4157,   4587,   5017,   3508,   3942,   4372,   4802,   5232 },// Apparent Vah Element A Q1
   { 0x00288002, 0x0000,   3298,   3728,   4158,   4588,   5018,   3509,   3943,   4373,   4803,   5233 },// Apparent Vah Element A Q2
   { 0x00290002, 0x0000,   3299,   3729,   4159,   4589,   5019,   3510,   3944,   4374,   4804,   5234 },// Apparent Vah Element A Q3
   { 0x002A0002, 0x0000,   3300,   3730,   4160,   4590,   5020,   3511,   3945,   4375,   4805,   5235 },// Apparent Vah Element A Q4

   { 0x00304002, 0x0000,   3301,   3731,   4161,   4591,   5021,   3512,   3946,   4376,   4806,   5236 },// Apparent Vah Element B Q1
   { 0x00308002, 0x0000,   3302,   3732,   4162,   4592,   5022,   3513,   3947,   4377,   4807,   5237 },// Apparent Vah Element B Q2
   { 0x00310002, 0x0000,   3303,   3733,   4163,   4593,   5023,   3514,   3948,   4378,   4808,   5238 },// Apparent Vah Element B Q3
   { 0x00320002, 0x0000,   3304,   3734,   4164,   4594,   5024,   3515,   3949,   4379,   4809,   5239 },// Apparent Vah Element B Q4

   { 0x00384002, 0x0000,   3305,   3735,   4165,   4595,   5025,   3516,   3950,   4380,   4810,   5240 },// Apparent Vah Element C Q1
   { 0x00388002, 0x0000,   3306,   3736,   4166,   4596,   5026,   3517,   3951,   4381,   4811,   5241 },// Apparent Vah Element C Q2
   { 0x00390002, 0x0000,   3307,   3737,   4167,   4597,   5027,   3518,   3952,   4382,   4812,   5242 },// Apparent Vah Element C Q3
   { 0x003A0002, 0x0000,   3308,   3738,   4168,   4598,   5028,   3519,   3953,   4383,   4813,   5243 },// Apparent Vah Element C Q4

   { 0x00004002, 0x0000,   3309,   3739,   4169,   4599,   5029,   3520,   3954,   4384,   4814,   5244 },// Apparent Vah Q1
   { 0x80004002, 0x0000,   3309,   3739,   4169,   4599,   5029,   3520,   3954,   4384,   4814,   5244 },// Apparent Vah Q1
   { 0x80004002, 0x0001,   3350,   3783,   4213,   4643,   5073,   3564,   3998,   4428,   4858,   5288 },// Arithmetic Apparent VAh Q1 (read)
   { 0x80004002, 0x0002,  13563,  13789,  13897,  14005,  14113,  13615,  13843,  13951,  14059,  14297 },// Distortion Vah Q1
   { 0x80004002, 0x0100,  13564,  13790,  13898,  14006,  14114,  13616,  13844,  13952,  14060,  14298 },// Apparent () Q1 (read)
   { 0x80004002, 0x0101,   3351,   3784,   4214,   4644,   5074,   3565,   3999,   4429,   4859,   5289 },// Arithmetic Apparent () Q1 (read)

   { 0x00008002, 0x0000,   3310,   3740,   4170,   4600,   5030,   3521,   3955,   4385,   4815,   5245 },// Apparent Vah Q2
   { 0x80008002, 0x0000,   3310,   3740,   4170,   4600,   5030,   3521,   3955,   4385,   4815,   5245 },// Apparent Vah Q2
   { 0x80008002, 0x0001,   3352,   3785,   4215,   4645,   5075,   3566,   4000,   4430,   4860,   5290 },// Arithmetic Apparent VAh Q2 (read)
   { 0x80008002, 0x0002,  13565,  13791,  13899,  14007,  14115,  13617,  13845,  13953,  14061,  14299 },// Distortion Vah Q2
   { 0x80008002, 0x0100,  13566,  13792,  13900,  14008,  14116,  13618,  13846,  13954,  14062,  14300 },// Apparent () Q2 (read)
   { 0x80008002, 0x0101,   3353,   3786,   4216,   4646,   5076,   3567,   4001,   4431,   4861,   5291 },// Arithmetic Apparent () Q2 (read)

   { 0x00010002, 0x0000,   3311,   3741,   4171,   4601,   5031,   3522,   3956,   4386,   4816,   5246 },// Apparent Vah Q3
   { 0x80010002, 0x0000,   3311,   3741,   4171,   4601,   5031,   3522,   3956,   4386,   4816,   5246 },// Apparent Vah Q3
   { 0x80010002, 0x0001,   3354,   3787,   4217,   4647,   5077,   3568,   4002,   4432,   4862,   5292 },// Arithmetic Apparent VAh Q3 (read)
   { 0x80010002, 0x0002,  13567,  13793,  13901,  14009,  14117,  13619,  13847,  13955,  14063,  14301 },// Distortion Vah Q3
   { 0x80010002, 0x0100,  13568,  13794,  13902,  14010,  14118,  13620,  13848,  13956,  14064,  14302 },// Apparent () Q3 (read)
   { 0x80010002, 0x0101,   3355,   3788,   4218,   4648,   5078,   3569,   4003,   4433,   4863,   5293 },// Arithmetic Apparent () Q3 (read)

   { 0x00020002, 0x0000,   3312,   3742,   4172,   4602,   5032,   3523,   3957,   4387,   4817,   5247 },// Apparent Vah Q4
   { 0x80020002, 0x0000,   3312,   3742,   4172,   4602,   5032,   3523,   3957,   4387,   4817,   5247 },// Apparent Vah Q4
   { 0x80020002, 0x0001,   3356,   3789,   4219,   4649,   5079,   3570,   4004,   4434,   4864,   5294 },// Arithmetic Apparent VAh Q4 (read)
   { 0x80020002, 0x0002,  13569,  13795,  13903,  14011,  14119,  13621,  13849,  13957,  14065,  14303 },// Distortion Vah Q4
   { 0x80020002, 0x0100,  13570,  13796,  13904,  14012,  14120,  13622,  13850,  13958,  14066,  14304 },// Apparent () Q4 (read)
   { 0x80020002, 0x0101,   3357,   3790,   4220,   4650,   5080,   3571,   4005,   4435,   4865,   5295 },// Arithmetic Apparent () Q4 (read)

   { 0x80024002, 0x0000,     11,   3743,   4173,   4603,   5033,   3524,   3958,   4388,   4818,   5248 },// Apparent VAh Q1+Q4
   { 0x80024002, 0x0001,  13517,  13743,  13851,  13959,  14067,  13571,  13797,  13905,  14013,  14251 },// Arithmetic Apparent VAh Q1+Q4
   { 0x80024002, 0x0002,  13518,  13744,  13852,  13960,  14068,  13572,  13798,  13906,  14014,  14252 },// Distortion VAh Q1+Q4
   { 0x80024002, 0x0100,   3313,   3744,   4174,   4604,   5034,   3525,   3959,   4389,   4819,   5249 },// Apparent () Q1+Q4
   { 0x80024002, 0x0101,  13519,  13745,  13853,  13961,  14069,  13573,  13799,  13907,  14015,  14253 },// Arithmetic Apparent () Q1+Q4

   { 0x80018002, 0x0000,    248,   3745,   4175,   4605,   5035,   3526,   3960,   4390,   4820,   5250 },// Apparent VAh Q2+Q3
   { 0x80018002, 0x0001,  13520,  13746,  13854,  13962,  14070,  13574,  13800,  13908,  14016,  14254 },// Arithmetic Apparent VAh Q2+Q3
   { 0x80018002, 0x0002,  13521,  13747,  13855,  13963,  14071,  13575,  13801,  13909,  14017,  14255 },// Distortion VAh Q2+Q3
   { 0x80018002, 0x0100,   3314,   3746,   4176,   4606,   5036,   3527,   3961,   4391,   4821,   5251 },// Apparent () Q2+Q3
   { 0x80018002, 0x0101,  13522,  13748,  13856,  13964,  14072,  13576,  13802,  13910,  14018,  14256 },// Arithmetic Apparent () Q2+Q3

   { 0x0003C002, 0x0000,     17,    429,    430,    431,    432,    586,    595,    604,    613,    622 },// Apparent VAh
   { 0x8003C002, 0x0000,     17,    429,    430,    431,    432,    586,    595,    604,    613,    622 },// Apparent VAh (Q1+Q4)+(Q2+Q3)
   { 0x8003C002, 0x0001,   3348,   3781,   4211,   4641,   5071,   3562,   3996,   4426,   4856,   5286 },// Arithmetic Apparent VAh (Q1+Q4)+(Q2+Q3)
   { 0x8003C002, 0x0002,  13523,  13749,  13857,  13965,  14073,  10317,  13803,  13911,  14019,  14257 },// Distortion VAh (Q1+Q4)+(Q2+Q3)
   { 0x8003C002, 0x0002,   3380,   3813,   4243,   4673,   5103,   3594,   4028,   4458,   4888,   5318 },// Distortion VAh (read)
   { 0x8003C002, 0x0100,   3315,   3747,   4177,   4607,   5037,   3528,   3962,   4392,   4822,   5252 },// Apparent () (Q1+Q4)+(Q2+Q3)
   { 0x8003C002, 0x0101,   3349,   3782,   4212,   4642,   5072,   3563,   3997,   4427,   4857,   5287 },// Arithmetic Apparent () (Q1+Q4)+(Q2+Q3)

   { 0x8007C002, 0x0000,    249,   3748,   4178,   4608,   5038,   3529,   3963,   4393,   4823,   5253 },// Apparent VAh (Q1+Q4)-(Q2+Q3)
   { 0x8007C002, 0x0001,  13524,  13750,  13858,  13966,  14074,  13577,  13804,  13912,  14020,  14258 },// Arithmetic Apparent VAh (Q1+Q4)-(Q2+Q3)
   { 0x8007C002, 0x0002,  13525,  13751,  13859,  13967,  14075,  13578,  13805,  13913,  14021,  14259 },// Distortion VAh (Q1+Q4)-(Q2+Q3)
   { 0x8007C002, 0x0100,   3316,   3749,   4179,   4609,   5039,   3530,   3964,   4394,   4824,   5254 },// Apparent () (Q1+Q4)-(Q2+Q3)
   { 0x8007C002, 0x0101,  13526,  13752,  13860,  13968,  14076,  13579,  13806,  13914,  14022,  14260 },// Arithmetic Apparent () (Q1+Q4)-(Q2+Q3)

   { 0x802A4002, 0x0000,   3317,   3750,   4180,   4610,   5040,   3531,   3965,   4395,   4825,   5255 },// Apparent VAh Element A Q1+Q4
   { 0x802A4002, 0x0001,  13527,  13753,  13861,  13969,  14077,  13580,  13807,  13915,  14023,  14261 },// Arithmetic Apparent VAh Element A Q1+Q4
   { 0x802A4002, 0x0002,  13528,  13754,  13862,  13970,  14078,  13581,  13808,  13916,  14024,  14262 },// Distortion VAh Element A Q1+Q4
   { 0x802A4002, 0x0100,   3318,   3751,   4181,   4611,   5041,   3532,   3966,   4396,   4826,   5256 },// Apparent () Element A Q1+Q4
   { 0x802A4002, 0x0101,  13529,  13755,  13863,  13971,  14079,  13582,  13809,  13917,  14025,  14263 },// Arithmetic Apparent () Element A Q1+Q4

   { 0x80298002, 0x0000,   3319,   3752,   4182,   4612,   5042,   3533,   3967,   4397,   4827,   5257 },// Apparent VAh Element A Q2+Q3
   { 0x80298002, 0x0001,  13530,  13756,  13864,  13972,  14080,  13583,  13810,  13918,  14026,  14264 },// Arithmetic Apparent VAh Element A Q2+Q3
   { 0x80298002, 0x0002,  13531,  13757,  13865,  13973,  14081,  13584,  13811,  13919,  14027,  14265 },// Distortion VAh Element A Q2+Q3
   { 0x80298002, 0x0100,   3320,   3753,   4183,   4613,   5043,   3534,   3968,   4398,   4828,   5258 },// Apparent () Element A Q2+Q3
   { 0x80298002, 0x0101,  13532,  13758,  13866,  13974,  14082,  13585,  13812,  13920,  14028,  14266 },// Arithmetic Apparent () Element A Q2+Q3

   { 0x002BC002, 0x0000,   3294,   3724,   4154,   4584,   5014,   3505,   3939,   4369,   4799,   5229 },// Apparent VAh Element A
   { 0x802BC002, 0x0000,   3294,   3724,   4154,   4584,   5014,   3505,   3939,   4369,   4799,   5229 },// Apparent VAh Element A (Q1+Q4)+(Q2+Q3)
   { 0x802BC002, 0x0001,  13533,  13759,  13867,  13975,  14083,  13586,  13813,  13921,  14029,  14267 },// Arithmetic Apparent VAh Element A (Q1+Q4)+(Q2+Q3)
   { 0x802BC002, 0x0002,  13534,  13760,  13868,  13976,  14084,  13587,  13814,  13922,  14030,  14268 },// Distortion VAh, Element A (read)
   { 0x802BC002, 0x0002,   3377,   3810,   4240,   4670,   5100,   3591,   4025,   4455,   4885,   5315 },// Distortion VAh, Element A (read)
   { 0x802BC002, 0x0100,   3321,   3754,   4184,   4614,   5044,   3535,   3969,   4399,   4829,   5259 },// Apparent () Element A (Q1+Q4)+(Q2+Q3)
   { 0x802BC002, 0x0101,  13535,  13761,  13869,  13977,  14085,  13588,  13815,  13923,  14031,  14269 },// Arithmetic Apparent () Element A (Q1+Q4)+(Q2+Q3)

   { 0x802FC002, 0x0000,   3322,   3755,   4185,   4615,   5045,   3536,   3970,   4400,   4830,   5260 },// Apparent VAh Element A (Q1+Q4)-(Q2+Q3) (read)
   { 0x802FC002, 0x0001,  13536,  13762,  13870,  13978,  14086,  13589,  13816,  13924,  14032,  14270 },// Arithmetic Apparent VAh Element A (Q1+Q4)-(Q2+Q3) (read)
   { 0x802FC002, 0x0002,  13537,  13763,  13871,  13979,  14087,  13590,  13817,  13925,  14033,  14271 },// Distortion VAh Element A (Q1+Q4)-(Q2+Q3) (read)
   { 0x802FC002, 0x0100,   3323,   3756,   4186,   4616,   5046,   3537,   3971,   4401,   4831,   5261 },// Apparent () Element A (Q1+Q4)-(Q2+Q3) (read)
   { 0x802FC002, 0x0101,  13538,  13764,  13872,  13980,  14088,  13591,  13818,  13926,  14034,  14272 },// Arithmetic Apparent () Element A (Q1+Q4)-(Q2+Q3) (read)

   { 0x80324002, 0x0000,   3324,   3757,   4187,   4617,   5047,   3538,   3972,   4402,   4832,   5262 },// Apparent VAh Element B Q1+Q4 (read)
   { 0x80324002, 0x0001,  13539,  13765,  13873,  13981,  14089,  13592,  13819,  13927,  14035,  14273 },// Arithmetic Apparent VAh Element B Q1+Q4 (read)
   { 0x80324002, 0x0002,  13540,  13766,  13874,  13982,  14090,  13593,  13820,  13928,  14036,  14274 },// Distortion VAh Element B Q1+Q4 (read)
   { 0x80324002, 0x0100,   3325,   3758,   4188,   4618,   5048,   3539,   3973,   4403,   4833,   5263 },// Apparent () Element B Q1+Q4 (read)
   { 0x80324002, 0x0101,  13541,  13767,  13875,  13983,  14091,  13594,  13821,  13929,  14037,  14275 },// Arithmetic Apparent () Element B Q1+Q4 (read)

   { 0x80318002, 0x0000,   3326,   3759,   4189,   4619,   5049,   3540,   3974,   4404,   4834,   5264 },// Apparent VAh Element B Q2+Q3 (read)
   { 0x80318002, 0x0001,  13542,  13768,  13876,  13984,  14092,  13595,  13822,  13930,  14038,  14276 },// Arithemtic Apparent VAh Element B Q2+Q3 (read)
   { 0x80318002, 0x0002,  13543,  13769,  13877,  13985,  14093,  13596,  13823,  13931,  14039,  14277 },// Distortion VAh Element B Q2+Q3 (read)
   { 0x80318002, 0x0100,   3327,   3760,   4190,   4620,   5050,   3541,   3975,   4405,   4835,   5265 },// Apparent () Element B Q2+Q3 (read)
   { 0x80318002, 0x0101,  13544,  13770,  13878,  13986,  14094,  13597,  13824,  13932,  14040,  14278 },// Arithemtic Apparent () Element B Q2+Q3 (read)

   { 0x0033C002, 0x0000,   3295,   3725,   4155,   4585,   5015,   3506,   3940,   4370,   4800,   5230 },// Apparent VAh Element B
   { 0x8033C002, 0x0000,   3295,   3725,   4155,   4585,   5015,   3506,   3940,   4370,   4800,   5230 },// Apparent VAh Element B (Q1+Q4)+(Q2+Q3) (read)
   { 0x8033C002, 0x0001,  13545,  13771,  13879,  13987,  14095,  13598,  13825,  13933,  14041,  14279 },// Arithmetic Apparent VAh Element B (Q1+Q4)+(Q2+Q3) (read)
   { 0x8033C002, 0x0002,  13546,  13772,  13880,  13988,  14096,  13599,  13826,  13934,  14042,  14280 },// Distortion VAh Element B (Q1+Q4)+(Q2+Q3) (read)
   { 0x8033C002, 0x0002,   3378,   3811,   4241,   4671,   5101,   3592,   4026,   4456,   4886,   5316 },// Distortion VAh, Element B (read)
   { 0x8033C002, 0x0100,   3328,   3761,   4191,   4621,   5051,   3542,   3976,   4406,   4836,   5266 },// Apparent () Element B (Q1+Q4)+(Q2+Q3) (read)
   { 0x8033C002, 0x0101,  13547,  13773,  13881,  13989,  14097,  13600,  13827,  13935,  14043,  14281 },// Arithmetic Apparent () Element B (Q1+Q4)+(Q2+Q3) (read)

   { 0x8037C002, 0x0000,   3329,   3762,   4192,   4622,   5052,   3543,   3977,   4407,   4837,   5267 },// Apparent VAh Element B (Q1+Q4)-(Q2+Q3) (read)
   { 0x8037C002, 0x0001,  13548,  13774,  13882,  13990,  14098,  13601,  13828,  13936,  14044,  14282 },// Arithmetic Apparent VAh Element B (Q1+Q4)-(Q2+Q3) (read)
   { 0x8037C002, 0x0002,  13549,  13775,  13883,  13991,  14099,  13602,  13829,  13937,  14045,  14283 },// Distortion VAh Element B (Q1+Q4)-(Q2+Q3) (read)
   { 0x8037C002, 0x0100,   3330,   3763,   4193,   4623,   5053,   3544,   3978,   4408,   4838,   5268 },// Apparent () Element B (Q1+Q4)-(Q2+Q3) (read)
   { 0x8037C002, 0x0101,  13550,  13776,  13884,  13992,  14100,  13603,  13830,  13938,  14046,  14284},// Arithmetic Apparent () Element B (Q1+Q4)-(Q2+Q3) (read)

   { 0x803A4002, 0x0000,   3331,   3764,   4194,   4624,   5054,   3545,   3979,   4409,   4839,   5269 },// Apparent VAh Element C Q1+Q4 (read)
   { 0x803A4002, 0x0001,  13551,  13777,  13885,  13993,  14101,  13604,  13831,  13939,  14047,  14285 },// Arithmetic Apparent VAh Element C Q1+Q4 (read)
   { 0x803A4002, 0x0002,  13552,  13778,  13886,  13994,  14102,  13605,  13832,  13940,  14048,  14286 },// Distortion VAh Element C Q1+Q4 (read)
   { 0x803A4002, 0x0100,   3332,   3765,   4195,   4625,   5055,   3546,   3980,   4410,   4840,   5270 },// Apparent () Element C Q1+Q4 (read)
   { 0x803A4002, 0x0101,  13553,  13779,  13887,  13995,  14103,  13606,  13833,  13941,  14049,  14287 },// Arithmetic Apparent () Element C Q1+Q4 (read)

   { 0x80398002, 0x0000,   3333,   3766,   4196,   4626,   5056,   3547,   3981,   4411,   4841,   5271 },// Apparent VAh Element C Q2+Q3 (read)
   { 0x80398002, 0x0001,  13554,  13780,  13888,  13996,  14104,  13607,  13834,  13942,  14050,  14288 },// Arithmetic Apparent VAh Element C Q2+Q3 (read)
   { 0x80398002, 0x0002,  13555,  13781,  13889,  13997,  14105,  13608,  13835,  13943,  14051,  14289 },// Distortion VAh Element C Q2+Q3 (read)
   { 0x80398002, 0x0100,   3334,   3767,   4197,   4627,   5057,   3548,   3982,   4412,   4842,   5272 },// Apparent () Element C Q2+Q3 (read)
   { 0x80398002, 0x0101,  13556,  13782,  13890,  13998,  14106,      0,  13836,  13944,  14052,  14290 },// Arithmetic Apparent () Element C Q2+Q3 (read)

   { 0x003BC002, 0x0000,   3296,   3726,   4156,   4586,   5016,   3507,   3941,   4371,   4801,   5231 },// Apparent VAh Element C
   { 0x803BC002, 0x0000,   3296,   3726,   4156,   4586,   5016,   3507,   3941,   4371,   4801,   5231 },// Apparent VAh Element C (Q1+Q4)+(Q2+Q3) (read)
   { 0x803BC002, 0x0001,  13557,  13783,  13891,  13999,  14107,  13609,  13837,  13945,  14053,  14291 },// Arithmetic Apparent VAh Element C (Q1+Q4)+(Q2+Q3) (read)
   { 0x803BC002, 0x0002,  13558,  13784,  13892,  14000,  14108,  13610,  13838,  13946,  14054,  14292 },// Distortion VAh Element C (Q1+Q4)+(Q2+Q3) (read)
   { 0x803BC002, 0x0002,   3379,   3812,   4242,   4672,   5102,   3593,   4027,   4457,   4887,   5317 },// Distortion VAh, Element C (read)
   { 0x803BC002, 0x0100,   3335,   3768,   4198,   4628,   5058,   3549,   3983,   4413,   4843,   5273 },// Apparent () Element C (Q1+Q4)+(Q2+Q3) (read)
   { 0x803BC002, 0x0101,  13559,  13785,  13893,  14001,  14109,  13611,  13839,  13947,  14055,  14293 },// Arithmetic Apparent () Element C (Q1+Q4)+(Q2+Q3) (read)

   { 0x803FC002, 0x0000,   3336,   3769,   4199,   4629,   5059,   3550,   3984,   4414,   4844,   5274 },// Apparent VAh Element C (Q1+Q4)-(Q2+Q3) (read)
   { 0x803FC002, 0x0001,  13560,  13786,  13894,  14002,  14110,  13612,  13840,  13948,  14056,  14294 },// Arithmetic Apparent VAh Element C (Q1+Q4)-(Q2+Q3) (read)
   { 0x803FC002, 0x0002,  13561,  13787,  13895,  14003,  14111,  13613,  13841,  13949,  14057,  14295 },// Distortion VAh Element C (Q1+Q4)-(Q2+Q3) (read)
   { 0x803FC002, 0x0100,   3337,   3770,   4200,   4630,   5060,   3551,   3985,   4415,   4845,   5275 },// Apparent () Element C (Q1+Q4)-(Q2+Q3) (read)
   { 0x803FC002, 0x0101,  13562,  13788,  13896,  14004,  14112,  13614,  13842,  13950,  14058,  14296 },// Arithmetic Apparent () Element C (Q1+Q4)-(Q2+Q3) (read)

   { 0x0003C003, 0x0000,   3338,   3771,   4201,   4631,   5061,   3552,   3986,   4416,   4846,   5276 },// Phasor Apparent VAh, fund + harmonics (read)
   { 0x0043C003, 0x0000,   3339,   3772,   4202,   4632,   5062,   3553,   3987,   4417,   4847,   5277 },// Phasor Apparent VAh, fund only (read)
   { 0x00004003, 0x0000,   3340,   3773,   4203,   4633,   5063,   3554,   3988,   4418,   4848,   5278 },// Phasor Apparent VAh Q1 fund+harmonics (read)
   { 0x00008003, 0x0000,   3341,   3774,   4204,   4634,   5064,   3555,   3989,   4419,   4849,   5279 },// Phasor Apparent VAh Q2 fund+harmonics (read)
   { 0x00010003, 0x0000,   3342,   3775,   4205,   4635,   5065,   3556,   3990,   4420,   4850,   5280 },// Phasor Apparent VAh Q3 fund+harmonics (read)
   { 0x00020003, 0x0000,   3343,   3776,   4206,   4636,   5066,   3557,   3991,   4421,   4851,   5281 },// Phasor Apparent VAh Q4 fund+harmonics (read)
   { 0x00404003, 0x0000,   3344,   3777,   4207,   4637,   5067,   3558,   3992,   4422,   4852,   5282 },// Phasor Apparent VAh Q1 fund only (read)
   { 0x00408003, 0x0000,   3345,   3778,   4208,   4638,   5068,   3559,   3993,   4423,   4853,   5283 },// Phasor Apparent VAh Q2 fund only (read)
   { 0x00410003, 0x0000,   3346,   3779,   4209,   4639,   5069,   3560,   3994,   4424,   4854,   5284 },// Phasor Apparent VAh Q3 fund only (read)
   { 0x00420003, 0x0000,   3347,   3780,   4210,   4640,   5070,   3561,   3995,   4425,   4855,   5285 },// Phasor Apparent VAh Q4 fund only (read)

   { 0x8003C003, 0x0000,   3338,   3771,   4201,   4631,   5061,   3552,   3986,   4416,   4846,   5276 },// Phasor Apparent VAh, fund + harmonics (read)
   { 0x8043C003, 0x0000,   3339,   3772,   4202,   4632,   5062,   3553,   3987,   4417,   4847,   5277 },// Phasor Apparent VAh, fund only (read)
   { 0x80004003, 0x0000,   3340,   3773,   4203,   4633,   5063,   3554,   3988,   4418,   4848,   5278 },// Phasor Apparent VAh Q1 fund+harmonics (read)
   { 0x80008003, 0x0000,   3341,   3774,   4204,   4634,   5064,   3555,   3989,   4419,   4849,   5279 },// Phasor Apparent VAh Q2 fund+harmonics (read)
   { 0x80010003, 0x0000,   3342,   3775,   4205,   4635,   5065,   3556,   3990,   4420,   4850,   5280 },// Phasor Apparent VAh Q3 fund+harmonics (read)
   { 0x80020003, 0x0000,   3343,   3776,   4206,   4636,   5066,   3557,   3991,   4421,   4851,   5281 },// Phasor Apparent VAh Q4 fund+harmonics (read)
   { 0x80404003, 0x0000,   3344,   3777,   4207,   4637,   5067,   3558,   3992,   4422,   4852,   5282 },// Phasor Apparent VAh Q1 fund only (read)
   { 0x80408003, 0x0000,   3345,   3778,   4208,   4638,   5068,   3559,   3993,   4423,   4853,   5283 },// Phasor Apparent VAh Q2 fund only (read)
   { 0x80410003, 0x0000,   3346,   3779,   4209,   4639,   5069,   3560,   3994,   4424,   4854,   5284 },// Phasor Apparent VAh Q3 fund only (read)
   { 0x80420003, 0x0000,   3347,   3780,   4210,   4640,   5070,   3561,   3995,   4425,   4855,   5285 },// Phasor Apparent VAh Q4 fund only (read)

   { 0x8003C003, 0x0001,  10223,  10881,  11069,  11257,  11445,  10317,  10975,  11163,  11351,  11539 },// Phasor Apparent VAh, fund + harmonics (read)
   { 0x8043C003, 0x0001,  10224,  10882,  11070,  11258,  11446,  10318,  10976,  11164,  11352,  11540 },// Phasor Apparent VAh, fund only (read)
   { 0x80004003, 0x0001,  10225,  10883,  11071,  11259,  11447,  10319,  10977,  11165,  11353,  11541 },// Phasor Apparent VAh Q1 fund+harmonics (read)
   { 0x80008003, 0x0001,  10226,  10884,  11072,  11260,  11448,  10320,  10978,  11166,  11354,  11542 },// Phasor Apparent VAh Q2 fund+harmonics (read)
   { 0x80010003, 0x0001,  10227,  10885,  11073,  11261,  11449,  10321,  10979,  11167,  11355,  11543 },// Phasor Apparent VAh Q3 fund+harmonics (read)
   { 0x80020003, 0x0001,  10228,  10886,  11074,  11262,  11450,  10322,  10980,  11168,  11356,  11544 },// Phasor Apparent VAh Q4 fund+harmonics (read)
   { 0x80404003, 0x0001,  10229,  10887,  11075,  11263,  11451,  10323,  10981,  11169,  11357,  11545 },// Phasor Apparent VAh Q1 fund only (read)
   { 0x80408003, 0x0001,  10230,  10888,  11076,  11264,  11452,  10324,  10982,  11170,  11358,  11546 },// Phasor Apparent VAh Q2 fund only (read)
   { 0x80410003, 0x0001,  10231,  10889,  11077,  11265,  11453,  10325,  10983,  11171,  11359,  11547 },// Phasor Apparent VAh Q3 fund only (read)
   { 0x80420003, 0x0001,  10232,  10890,  11078,  11266,  11454,  10326,  10984,  11172,  11360,  11548 },// Phasor Apparent VAh Q4 fund only (read)

   { 0x8003C003, 0x0100,  10233,  10891,  11079,  11267,  11455,  10327,  10985,  11173,  11361,  11549 },// Phasor Apparent VAh, fund + harmonics (read)
   { 0x8043C003, 0x0100,  10234,  10892,  11080,  11268,  11456,  10328,  10986,  11174,  11362,  11550 },// Phasor Apparent VAh, fund only (read)
   { 0x80004003, 0x0100,  10235,  10893,  11081,  11269,  11457,  10329,  10987,  11175,  11363,  11551 },// Phasor Apparent VAh Q1 fund+harmonics (read)
   { 0x80008003, 0x0100,  10236,  10894,  11082,  11270,  11458,  10330,  10988,  11176,  11364,  11552 },// Phasor Apparent VAh Q2 fund+harmonics (read)
   { 0x80010003, 0x0100,  10237,  10895,  11083,  11271,  11459,  10331,  10989,  11177,  11365,  11553 },// Phasor Apparent VAh Q3 fund+harmonics (read)
   { 0x80020003, 0x0100,  10238,  10896,  11084,  11272,  11460,  10332,  10990,  11178,  11366,  11554 },// Phasor Apparent VAh Q4 fund+harmonics (read)
   { 0x80404003, 0x0100,  10239,  10897,  11085,  11273,  11461,  10333,  10991,  11179,  11367,  11555 },// Phasor Apparent VAh Q1 fund only (read)
   { 0x80408003, 0x0100,  10240,  10898,  11086,  11274,  11462,  10334,  10992,  11180,  11368,  11556 },// Phasor Apparent VAh Q2 fund only (read)
   { 0x80410003, 0x0100,  10241,  10899,  11087,  11275,  11463,  10335,  10993,  11181,  11369,  11557 },// Phasor Apparent VAh Q3 fund only (read)
   { 0x80420003, 0x0100,  10242,  10900,  11088,  11276,  11464,  10336,  10994,  11182,  11370,  11558 },// Phasor Apparent VAh Q4 fund only (read)

   { 0x8003C003, 0x0101,  10243,  10901,  11089,  11277,  11465,  10337,  10995,  11183,  11371,  11559 },// Phasor Apparent VAh, fund + harmonics (read)
   { 0x8043C003, 0x0101,  10244,  10902,  11090,  11278,  11466,  10338,  10996,  11184,  11372,  11560 },// Phasor Apparent VAh, fund only (read)
   { 0x80004003, 0x0101,  10245,  10903,  11091,  11279,  11467,  10339,  10997,  11185,  11373,  11561 },// Phasor Apparent VAh Q1 fund+harmonics (read)
   { 0x80008003, 0x0101,  10246,  10904,  11092,  11280,  11468,  10340,  10998,  11186,  11374,  11562 },// Phasor Apparent VAh Q2 fund+harmonics (read)
   { 0x80010003, 0x0101,  10247,  10905,  11093,  11281,  11469,  10341,  10999,  11187,  11375,  11563 },// Phasor Apparent VAh Q3 fund+harmonics (read)
   { 0x80020003, 0x0101,  10248,  10906,  11094,  11282,  11470,  10342,  11000,  11188,  11376,  11564 },// Phasor Apparent VAh Q4 fund+harmonics (read)
   { 0x80404003, 0x0101,  10249,  10907,  11095,  11283,  11471,  10343,  11001,  11189,  11377,  11565 },// Phasor Apparent VAh Q1 fund only (read)
   { 0x80408003, 0x0101,  10250,  10908,  11096,  11284,  11472,  10344,  11002,  11190,  11378,  11566 },// Phasor Apparent VAh Q2 fund only (read)
   { 0x80410003, 0x0101,  10251,  10909,  11097,  11285,  11473,  10345,  11003,  11191,  11379,  11567 },// Phasor Apparent VAh Q3 fund only (read)
   { 0x80420003, 0x0101,  10252,  10910,  11098,  11286,  11474,  10346,  11004,  11192,  11380,  11568 },// Phasor Apparent VAh Q4 fund only (read)

   { 0x0028000A, 0x0000,   3358,   3791,   4221,   4651,   5081,   3572,   4006,   4436,   4866,   5296 },// V2h Element A, fund + harmonics (read)
   { 0x0030000A, 0x0000,   3359,   3792,   4222,   4652,   5082,   3573,   4007,   4437,   4867,   5297 },// V2h Element B, fund + harmonics (read)
   { 0x0038000A, 0x0000,   3360,   3793,   4223,   4653,   5083,   3574,   4008,   4438,   4868,   5298 },// V2h Element C, fund + harmonics (read)
   { 0x0068000A, 0x0000,   3361,   3794,   4224,   4654,   5084,   3575,   4009,   4439,   4869,   5299 },// V2h Element A, fund (read)
   { 0x0070000A, 0x0000,   3362,   3795,   4225,   4655,   5085,   3576,   4010,   4440,   4870,   5300 },// V2h Element B, fund (read)
   { 0x0078000A, 0x0000,   3363,   3796,   4226,   4656,   5086,   3577,   4011,   4441,   4871,   5301 },// V2h Element C, fund (read)

   { 0x0058000A, 0x0000,   3364,   3797,   4227,   4657,   5087,   3578,   4012,   4442,   4872,   5302 },// V2hAC, fund (read)
   { 0x0050000A, 0x0000,   3365,   3798,   4228,   4658,   5088,   3579,   4013,   4443,   4873,   5303 },// V2hCB, fund (read)
   { 0x0048000A, 0x0000,   3366,   3799,   4229,   4659,   5089,   3580,   4014,   4444,   4874,   5304 },// V2hBA, fund (read)
   { 0x0018000A, 0x0000,   3367,   3800,   4230,   4660,   5090,   3581,   4015,   4445,   4875,   5305 },// V2hAC, fund + harmonics (read)
   { 0x0010000A, 0x0000,   3368,   3801,   4231,   4661,   5091,   3582,   4016,   4446,   4876,   5306 },// V2hCB, fund + harmonics (read)
   { 0x0008000A, 0x0000,   3369,   3802,   4232,   4662,   5092,   3583,   4017,   4447,   4877,   5307 },// V2hBA, fund + harmonics (read)

   { 0x0068000E, 0x0000,   3370,   3803,   4233,   4663,   5093,   3584,   4018,   4448,   4878,   5308 },// I2h, Element A, fund (read)
   { 0x0070000E, 0x0000,   3371,   3804,   4234,   4664,   5094,   3585,   4019,   4449,   4879,   5309 },// I2h, Element B, fund (read)
   { 0x0078000E, 0x0000,   3372,   3805,   4235,   4665,   5095,   3586,   4020,   4450,   4880,   5310 },// I2h, Element C, fund (read)
   { 0x0028000E, 0x0000,   3373,   3806,   4236,   4666,   5096,   3587,   4021,   4451,   4881,   5311 },// I2h, Element A, fund + harmonics (read)
   { 0x0030000E, 0x0000,   3374,   3807,   4237,   4667,   5097,   3588,   4022,   4452,   4882,   5312 },// I2h, Element B, fund + harmonics (read)
   { 0x0038000E, 0x0000,   3375,   3808,   4238,   4668,   5098,   3589,   4023,   4453,   4883,   5313 },// I2h, Element C, fund + harmonics (read)
   { 0x0020000E, 0x0000,   3376,   3809,   4239,   4669,   5099,   3590,   4024,   4454,   4884,   5314 },// In2h (read)

   { 0x0003C004, 0x0000,     18,   3814,   4244,   4674,   5104,   3595,   4029,   4459,   4889,   5319 },// Qh (read)
   { 0x80000022, 0x0000,   3381,   3815,   4245,   4675,   5105,   3596,   4030,   4460,   4890,   5320 },// Count (Pulses - reading)
   { 0x80000022, 0x0001,   3382,   3816,   4246,   4676,   5106,   3597,   4031,   4461,   4891,   5321 },// Count (Pulses - reading)
   { 0x80000022, 0x0002,   3383,   3817,   4247,   4677,   5107,   3598,   4032,   4462,   4892,   5322 },// Count (Pulses - reading)
   { 0x80000022, 0x0003,   3384,   3818,   4248,   4678,   5108,   3599,   4033,   4463,   4893,   5323 },// Count (Pulses - reading)
   { 0x80000221, 0x0001,   9976,  12821,  13019,  13217,  13415,  12722,  12920,  13118,  13316,  13514 },// Frequency
   { 0x80000221, 0x0002,   9977,  12822,  13020,  13218,  13416,  12723,  12921,  13119,  13317,  13515 },// Frequency
   { 0x80000221, 0x0003,  12625,  12823,  13021,  13219,  13417,  12724,  12922,  13120,  13318,  13516 },// Frequency
   { 0x8003C218, 0x0001,   9999,  12791,  12989,  13187,  13385,  12686,  12884,  13082,  13280,  13478 },// Distortion PF
   { 0x8003C218, 0x0002,  10000,  12792,  12990,  13188,  13386,  12690,  12888,  13086,  13284,  13482 },// Distortion PF
   { 0x8003C218, 0x0003,  12618,  12793,  12991,  13189,  13387,  12694,  12892,  13090,  13288,  13486 },// Distortion PF
   { 0x80080208, 0x0001,   9942,  12733,  12931,  13129,  13327,  12634,  12832,  13030,  13228,  13426 },// VBA fund + harmonics
   { 0x80080208, 0x0002,   9943,  12745,  12943,  13141,  13339,  12646,  12844,  13042,  13240,  13438 },// VBA fund + harmonics
   { 0x80080208, 0x0003,  12607,  12757,  12955,  13153,  13351,  12658,  12856,  13054,  13252,  13450 },// VBA fund + harmonics
   { 0x80100208, 0x0001,   9940,  12732,  12930,  13128,  13326,  12633,  12831,  13029,  13227,  13425 },// VCB fund + harmonics
   { 0x80100208, 0x0002,   9941,  12744,  12942,  13140,  13338,  12645,  12843,  13041,  13239,  13437 },// VCB fund + harmonics
   { 0x80100208, 0x0003,  12606,  12756,  12954,  13152,  13350,  12657,  12855,  13053,  13251,  13449 },// VCB fund + harmonics
   { 0x80180208, 0x0001,   9938,  12731,  12929,  13127,  13325,  12632,  12830,  13028,  13226,  13424 },// VAC fund + harmonics
   { 0x80180208, 0x0002,   9939,  12743,  12941,  13139,  13337,  12644,  12842,  13040,  13238,  13436 },// VAC fund + harmonics
   { 0x80180208, 0x0003,  12605,  12755,  12953,  13151,  13349,  12656,  12854,  13052,  13250,  13448 },// VAC fund + harmonics
   { 0x8020020C, 0x0001,   9950,  12761,  12959,  13157,  13355,  12662,  12860,  13058,  13256,  13454 },// In fund + harmonics
   { 0x8020020C, 0x0002,   9951,  12762,  12960,  13158,  13356,  12663,  12861,  13059,  13257,  13455 },// In fund + harmonics
   { 0x8020020C, 0x0003,  12611,  12763,  12961,  13159,  13357,  12664,  12862,  13060,  13258,  13456 },// In fund + harmonics
   { 0x80280208, 0x0001,   9926,  12725,  12923,  13121,  13319,  12626,  12824,  13022,  13220,  13418 },// VA fund + harmonics
   { 0x80280208, 0x0002,   9927,  12737,  12935,  13133,  13331,  12638,  12836,  13034,  13232,  13430 },// VA fund + harmonics
   { 0x80280208, 0x0003,  12599,  12749,  12947,  13145,  13343,  12650,  12848,  13046,  13244,  13442 },// VA fund + harmonics
   { 0x8028020C, 0X0001,    669,  12764,  12962,  13160,  13358,  12665,  12863,  13061,  13259,  13457 },// IA fund + harmonics
   { 0x8028020C, 0X0002,    670,  12765,  12963,  13161,  13359,  12666,  12864,  13062,  13260,  13458 },// IA fund + harmonics
   { 0x8028020C, 0X0003,  12612,  12766,  12964,  13162,  13360,  12667,  12865,  13063,  13261,  13459 },// IA fund + harmonics
   { 0x80280210, 0x0001,   9970,  12794,  12992,  13190,  13388,  12695,  12893,  13091,  13289,  13487 },// VTHD, Element A
   { 0x80280210, 0x0002,   9971,  12803,  13001,  13199,  13397,  12704,  12902,  13100,  13298,  13496 },// VTHD, Element A
   { 0x80280210, 0x0003,  12619,  12812,  13010,  13208,  13406,  12713,  12911,  13109,  13307,  13505 },// VTHD, Element A
   { 0x80280211, 0x0001,   9978,  12797,  12995,  13193,  13391,  12698,  12896,  13094,  13292,  13490 },// ITHD, Element A
   { 0x80280211, 0x0002,   9979,  12806,  13004,  13202,  13400,  12707,  12905,  13103,  13301,  13499 },// ITHD, Element A
   { 0x80280211, 0x0003,  12622,  12815,  13013,  13211,  13409,  12716,  12914,  13112,  13310,  13508 },// ITHD, Element A
   { 0x80280211, 0X0101,   9980,  12800,  12998,  13196,  13394,  12701,  12899,  13097,  13295,  13493 },// ITHD, Element A
   { 0x80280211, 0x0102,   9981,  12809,  13007,  13205,  13403,  12710,  12908,  13106,  13304,  13502 },// ITHD, Element A
   { 0x80280211, 0x0103,   9982,  12818,  13016,  13214,  13412,  12719,  12917,  13115,  13313,  13511 },// ITHD, Element A
   { 0x802BC218, 0x0001,   9990,  12782,  12980,  13178,  13376,  12683,  12881,  13079,  13277,  13475 },// Distortion PF, Element A
   { 0x802BC218, 0x0002,   9991,  12783,  12981,  13179,  13377,  12687,  12885,  13083,  13281,  13479 },// Distortion PF, Element A
   { 0x802BC218, 0x0003,   9992,  12784,  12982,  13180,  13378,  12691,  12889,  13087,  13285,  13483 },// Distortion PF, Element A
   { 0x80300208, 0x0001,   9928,  12726,  12924,  13122,  13320,  12627,  12825,  13023,  13221,  13419 },// VB fund + harmonics
   { 0x80300208, 0x0002,   9929,  12738,  12936,  13134,  13332,  12639,  12837,  13035,  13233,  13431 },// VB fund + harmonics
   { 0x80300208, 0x0003,  12600,  12750,  12948,  13146,  13344,  12651,  12849,  13047,  13245,  13443 },// VB fund + harmonics
   { 0x8030020C, 0x0001,   9954,  12767,  12965,  13163,  13361,  12668,  12866,  13064,  13262,  13460 },// IB fund + harmonics
   { 0x8030020C, 0x0002,   9955,  12768,  12966,  13164,  13362,  12669,  12867,  13065,  13263,  13461 },// IB fund + harmonics
   { 0x8030020C, 0x0003,  12613,  12769,  12967,  13165,  13363,  12670,  12868,  13066,  13264,  13462 },// IB fund + harmonics
   { 0x80300210, 0x0001,   9972,  12795,  12993,  13191,  13389,  12696,  12894,  13092,  13290,  13488 },// VTHD, Element B
   { 0x80300210, 0x0002,   9973,  12804,  13002,  13200,  13398,  12705,  12903,  13101,  13299,  13497 },// VTHD, Element B
   { 0x80300210, 0x0003,  12620,  12813,  13011,  13209,  13407,  12714,  12912,  13110,  13308,  13506 },// VTHD, Element B
   { 0x80300211, 0x0001,   9983,  12798,  12996,  13194,  13392,  12699,  12897,  13095,  13293,  13491 },// ITHD, Element B
   { 0x80300211, 0x0002,   9984,  12807,  13005,  13203,  13401,  12708,  12906,  13104,  13302,  13500 },// ITHD, Element B
   { 0x80300211, 0x0003,  12623,  12816,  13014,  13212,  13410,  12717,  12915,  13113,  13311,  13509 },// ITHD, Element B
   { 0x80300211, 0X0101,   9985,  12801,  12999,  13197,  13395,  12702,  12900,  13098,  13296,  13494 },// ITHD, Element B
   { 0x80300211, 0x0102,   9986,  12810,  13008,  13206,  13404,  12711,  12909,  13107,  13305,  13503 },// ITHD, Element B
   { 0x80300211, 0x0103,   9987,  12819,  13017,  13215,  13413,  12720,  12918,  13116,  13314,  13512 },// ITHD, Element B
   { 0x8033C218, 0x0001,   9993,  12785,  12983,  13181,  13379,  12684,  12882,  13080,  13278,  13476 },// Distortion PF, Element B
   { 0x8033C218, 0x0002,   9994,  12786,  12984,  13182,  13380,  12688,  12886,  13084,  13282,  13480 },// Distortion PF, Element B
   { 0x8033C218, 0x0003,   9995,  12787,  12985,  13183,  13381,  12692,  12890,  13088,  13286,  13484 },// Distortion PF, Element B
   { 0x80380208, 0x0001,   9930,  12727,  12925,  13123,  13321,  12628,  12826,  13024,  13222,  13420 },// VC fund + harmonics
   { 0x80380208, 0x0002,   9931,  12739,  12937,  13135,  13333,  12640,  12838,  13036,  13234,  13432 },// VC fund + harmonics
   { 0x80380208, 0x0003,  12601,  12751,  12949,  13147,  13345,  12652,  12850,  13048,  13246,  13444 },// VC fund + harmonics
   { 0x8038020C, 0x0001,    672,  12770,  12968,  13166,  13364,  12671,  12869,  13067,  13265,  13463 },// IC fund + harmonics
   { 0x8038020C, 0x0002,    673,  12771,  12969,  13167,  13365,  12672,  12870,  13068,  13266,  13464 },// IC fund + harmonics
   { 0x8038020C, 0x0003,  12614,  12772,  12970,  13168,  13366,  12673,  12871,  13069,  13267,  13465 },// IC fund + harmonics
   { 0x80380210, 0x0001,   9974,  12796,  12994,  13192,  13390,  12697,  12895,  13093,  13291,  13489 },// VTHD, Element C
   { 0x80380210, 0x0002,   9975,  12805,  13003,  13201,  13399,  12706,  12904,  13102,  13300,  13498 },// VTHD, Element C
   { 0x80380210, 0x0003,  12621,  12814,  13012,  13210,  13408,  12715,  12913,  13111,  13309,  13507 },// VTHD, Element C
   { 0x80380211, 0x0001,  10001,  12799,  12997,  13195,  13393,  12700,  12898,  13096,  13294,  13492 },// ITHD, Element C
   { 0x80380211, 0x0002,  10002,  12808,  13006,  13204,  13402,  12709,  12907,  13105,  13303,  13501 },// ITHD, Element C
   { 0x80380211, 0x0003,  12624,  12817,  13015,  13213,  13411,  12718,  12916,  13114,  13312,  13510 },// ITHD, Element C
   { 0x80380211, 0X0101,  10003,  12802,  13000,  13198,  13396,  12703,  12901,  13099,  13297,  13495 },// ITHD, Element C
   { 0x80380211, 0x0102,  10004,  12811,  13009,  13207,  13405,  12712,  12910,  13108,  13306,  13504 },// ITHD, Element C
   { 0x80380211, 0x0103,  10005,  12820,  13018,  13216,  13414,  12721,  12919,  13117,  13315,  13513 },// ITHD, Element C
   { 0x803BC218, 0x0001,   9996,  12788,  12986,  13184,  13382,  12685,  12883,  13081,  13279,  13477 },// Distortion PF, Element C
   { 0x803BC218, 0x0002,   9997,  12789,  12987,  13185,  13383,  12689,  12887,  13085,  13283,  13481 },// Distortion PF, Element C
   { 0x803BC218, 0x0003,   9998,  12790,  12988,  13186,  13384,  12693,  12891,  13089,  13287,  13485 },// Distortion PF, Element C
   { 0x80480208, 0x0001,   9948,  12736,  12934,  13132,  13330,  12637,  12835,  13033,  13231,  13429 },// VBA fund only
   { 0x80480208, 0x0002,   9949,  12748,  12946,  13144,  13342,  12649,  12847,  13045,  13243,  13441 },// VBA fund only
   { 0x80480208, 0x0003,  12610,  12760,  12958,  13156,  13354,  12661,  12859,  13057,  13255,  13453 },// VBA fund only
   { 0x80500208, 0x0001,   9946,  12735,  12933,  13131,  13329,  12636,  12834,  13032,  13230,  13428 },// VCB fund only
   { 0x80500208, 0x0002,   9947,  12747,  12945,  13143,  13341,  12648,  12846,  13044,  13242,  13440 },// VCB fund only
   { 0x80500208, 0x0003,  12609,  12759,  12957,  13155,  13353,  12660,  12858,  13056,  13254,  13452 },// VCB fund only
   { 0x80580208, 0x0001,   9944,  12734,  12932,  13130,  13328,  12635,  12833,  13031,  13229,  13427 },// VAC fund only
   { 0x80580208, 0x0002,   9945,  12746,  12944,  13142,  13340,  12647,  12845,  13043,  13241,  13439 },// VAC fund only
   { 0x80580208, 0x0003,  12608,  12758,  12956,  13154,  13352,  12659,  12857,  13055,  13253,  13451 },// VAC fund only
   { 0x80680208, 0x0001,   9932,  12728,  12926,  13124,  13322,  12629,  12827,  13025,  13223,  13421 },// VA fund only
   { 0x80680208, 0x0002,   9933,  12740,  12938,  13136,  13334,  12641,  12839,  13037,  13235,  13433 },// VA fund only
   { 0x80680208, 0x0003,  12602,  12752,  12950,  13148,  13346,  12653,  12851,  13049,  13247,  13445 },// VA fund only
   { 0x8068020C, 0x0001,   9958,  12773,  12971,  13169,  13367,  12674,  12872,  13070,  13268,  13466 },// IA fund only
   { 0x8068020C, 0x0002,   9959,  12774,  12972,  13170,  13368,  12675,  12873,  13071,  13269,  13467 },// IA fund only
   { 0x8068020C, 0x0003,  12615,  12775,  12973,  13171,  13369,  12676,  12874,  13072,  13270,  13468 },// IA fund only
   { 0x80700208, 0x0001,   9934,  12729,  12927,  13125,  13323,  12630,  12828,  13026,  13224,  13422 },// VB fund only
   { 0x80700208, 0x0002,   9935,  12741,  12939,  13137,  13335,  12642,  12840,  13038,  13236,  13434 },// VB fund only
   { 0x80700208, 0x0003,  12603,  12753,  12951,  13149,  13347,  12654,  12852,  13050,  13248,  13446 },// VB fund only
   { 0x8070020C, 0x0001,   9960,  12776,  12974,  13172,  13370,  12677,  12875,  13073,  13271,  13469 },// IB fund only
   { 0x8070020C, 0x0002,   9961,  12777,  12975,  13173,  13371,  12678,  12876,  13074,  13272,  13470 },// IB fund only
   { 0x8070020C, 0x0003,  12616,  12778,  12976,  13174,  13372,  12679,  12877,  13075,  13273,  13471 },// IB fund only
   { 0x80780208, 0x0001,   9936,  12730,  12928,  13126,  13324,  12631,  12829,  13027,  13225,  13423 },// VC fund only
   { 0x80780208, 0x0002,   9937,  12742,  12940,  13138,  13336,  12643,  12841,  13039,  13237,  13435 },// VC fund only
   { 0x80780208, 0x0003,  12604,  12754,  12952,  13150,  13348,  12655,  12853,  13051,  13249,  13447 },// VC fund only
   { 0x8078020C, 0x0001,   9962,  12779,  12977,  13175,  13373,  12680,  12878,  13076,  13274,  13472 },// IC fund only
   { 0x8078020C, 0x0002,   9963,  12780,  12978,  13176,  13374,  12681,  12879,  13077,  13275,  13473 },// IC fund only
   { 0x8078020C, 0x0003,  12617,  12781,  12979,  13177,  13375,  12682,  12880,  13078,  13276,  13474 } // IC fund only
};


 // lookup table for maximum demand measurements
static const uomLookupEntry_t masterMaxDemandLookup [] =
{ //   ST12      ST14      pres   presA   presB   presC   presD    prev   prevA   prevB   prevC   prevD
   { 0x00284400, 0x0000,   7380,   5324,   5838,   6352,   6866,   7634,   5581,   6095,   6609,   7123 },// W Element A Q1 fund + harmonics
   { 0x00288400, 0x0000,   7381,   5325,   5839,   6353,   6867,   7635,   5582,   6096,   6610,   7124 },// W Element A Q2 fund + harmonics
   { 0x00290400, 0x0000,   7382,   5326,   5840,   6354,   6868,   7636,   5583,   6097,   6611,   7125 },// W Element A Q3 fund + harmonics
   { 0x002A0400, 0x0000,   7383,   5327,   5841,   6355,   6869,   7637,   5584,   6098,   6612,   7126 },// W Element A Q4 fund + harmonics
   { 0x002A4400, 0x0000,   7384,   5328,   5842,   6356,   6870,   7638,   5585,   6099,   6613,   7127 },// W Element A Q1+Q4 fund + harmonics
   { 0x00298400, 0x0000,   7385,   5329,   5843,   6357,   6871,   7639,   5586,   6100,   6614,   7128 },// W Element A Q2+Q3 fund + harmonics
   { 0x002BC400, 0x0000,   7386,   5330,   5844,   6358,   6872,   7640,   5587,   6101,   6615,   7129 },// W Element A (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x002FC400, 0x0000,   7387,   5331,   5845,   6359,   6873,   7641,   5588,   6102,   6616,   7130 },// W Element A (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00304400, 0x0000,   7388,   5332,   5846,   6360,   6874,   7642,   5589,   6103,   6617,   7131 },// W Element B Q1 fund + harmonics
   { 0x00308400, 0x0000,   7389,   5333,   5847,   6361,   6875,   7643,   5590,   6104,   6618,   7132 },// W Element B Q2 fund + harmonics
   { 0x00310400, 0x0000,   7390,   5334,   5848,   6362,   6876,   7644,   5591,   6105,   6619,   7133 },// W Element B Q3 fund + harmonics
   { 0x00320400, 0x0000,   7391,   5335,   5849,   6363,   6877,   7645,   5592,   6106,   6620,   7134 },// W Element B Q4 fund + harmonics
   { 0x00324400, 0x0000,   7392,   5336,   5850,   6364,   6878,   7646,   5593,   6107,   6621,   7135 },// W Element B Q1+Q4 fund + harmonics
   { 0x00318400, 0x0000,   7393,   5337,   5851,   6365,   6879,   7647,   5594,   6108,   6622,   7136 },// W Element B Q2+Q3 fund + harmonics
   { 0x0033C400, 0x0000,   7394,   5338,   5852,   6366,   6880,   7648,   5595,   6109,   6623,   7137 },// W Element B (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x0037C400, 0x0000,   7395,   5339,   5853,   6367,   6881,   7649,   5596,   6110,   6624,   7138 },// W Element B (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00384400, 0x0000,   7396,   5340,   5854,   6368,   6882,   7650,   5597,   6111,   6625,   7139 },// W Element C Q1 fund + harmonics
   { 0x00388400, 0x0000,   7397,   5341,   5855,   6369,   6883,   7651,   5598,   6112,   6626,   7140 },// W Element C Q2 fund + harmonics
   { 0x00390400, 0x0000,   7398,   5342,   5856,   6370,   6884,   7652,   5599,   6113,   6627,   7141 },// W Element C Q3 fund + harmonics
   { 0x003A0400, 0x0000,   7399,   5343,   5857,   6371,   6885,   7653,   5600,   6114,   6628,   7142 },// W Element C Q4 fund + harmonics
   { 0x003A4400, 0x0000,   7400,   5344,   5858,   6372,   6886,   7654,   5601,   6115,   6629,   7143 },// W Element C Q1+Q4 fund + harmonics
   { 0x00398400, 0x0000,   7401,   5345,   5859,   6373,   6887,   7655,   5602,   6116,   6630,   7144 },// W Element C Q2+Q3 fund + harmonics
   { 0x003BC400, 0x0000,   7402,   5346,   5860,   6374,   6888,   7656,   5603,   6117,   6631,   7145 },// W Element C (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x003FC400, 0x0000,   7403,   5347,   5861,   6375,   6889,   7657,   5604,   6118,   6632,   7146 },// W Element C (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00684400, 0x0000,   7404,   5348,   5862,   6376,   6890,   7658,   5605,   6119,   6633,   7147 },// W Element A Q1 fund only
   { 0x00688400, 0x0000,   7405,   5349,   5863,   6377,   6891,   7659,   5606,   6120,   6634,   7148 },// W Element A Q2 fund only
   { 0x00690400, 0x0000,   7406,   5350,   5864,   6378,   6892,   7660,   5607,   6121,   6635,   7149 },// W Element A Q3 fund only
   { 0x006A0400, 0x0000,   7407,   5351,   5865,   6379,   6893,   7661,   5608,   6122,   6636,   7150 },// W Element A Q4 fund only
   { 0x006A4400, 0x0000,   7408,   5352,   5866,   6380,   6894,   7662,   5609,   6123,   6637,   7151 },// W Element A Q1+Q4 fund only
   { 0x00698400, 0x0000,   7409,   5353,   5867,   6381,   6895,   7663,   5610,   6124,   6638,   7152 },// W Element A Q2+Q3 fund only
   { 0x006BC400, 0x0000,   7410,   5354,   5868,   6382,   6896,   7664,   5611,   6125,   6639,   7153 },// W Element A (Q1+Q4)+(Q2+Q3) fund only
   { 0x006FC400, 0x0000,   7411,   5355,   5869,   6383,   6897,   7665,   5612,   6126,   6640,   7154 },// W Element A (Q1+Q4)-(Q2+Q3) fund only
   { 0x00704400, 0x0000,   7412,   5356,   5870,   6384,   6898,   7666,   5613,   6127,   6641,   7155 },// W Element B Q1 fund only
   { 0x00708400, 0x0000,   7413,   5357,   5871,   6385,   6899,   7667,   5614,   6128,   6642,   7156 },// W Element B Q2 fund only
   { 0x00710400, 0x0000,   7414,   5358,   5872,   6386,   6900,   7668,   5615,   6129,   6643,   7157 },// W Element B Q3 fund only
   { 0x00720400, 0x0000,   7415,   5359,   5873,   6387,   6901,   7669,   5616,   6130,   6644,   7158 },// W Element B Q4 fund only
   { 0x00724400, 0x0000,   7416,   5360,   5874,   6388,   6902,   7670,   5617,   6131,   6645,   7159 },// W Element B Q1+Q4 fund only
   { 0x00718400, 0x0000,   7417,   5361,   5875,   6389,   6903,   7671,   5618,   6132,   6646,   7160 },// W Element B Q2+Q3 fund only
   { 0x0073C400, 0x0000,   7418,   5362,   5876,   6390,   6904,   7672,   5619,   6133,   6647,   7161 },// W Element B (Q1+Q4)+(Q2+Q3) fund only
   { 0x0077C400, 0x0000,   7419,   5363,   5877,   6391,   6905,   7673,   5620,   6134,   6648,   7162 },// W Element B (Q1+Q4)-(Q2+Q3) fund only
   { 0x00784400, 0x0000,   7420,   5364,   5878,   6392,   6906,   7674,   5621,   6135,   6649,   7163 },// W Element C Q1 fund only
   { 0x00788400, 0x0000,   7421,   5365,   5879,   6393,   6907,   7675,   5622,   6136,   6650,   7164 },// W Element C Q2 fund only
   { 0x00790400, 0x0000,   7422,   5366,   5880,   6394,   6908,   7676,   5623,   6137,   6651,   7165 },// W Element C Q3 fund only
   { 0x007A0400, 0x0000,   7423,   5367,   5881,   6395,   6909,   7677,   5624,   6138,   6652,   7166 },// W Element C Q4 fund only
   { 0x007A4400, 0x0000,   7424,   5368,   5882,   6396,   6910,   7678,   5625,   6139,   6653,   7167 },// W Element C Q1+Q4 fund only
   { 0x00798400, 0x0000,   7425,   5369,   5883,   6397,   6911,   7679,   5626,   6140,   6654,   7168 },// W Element C Q2+Q3 fund only
   { 0x007BC400, 0x0000,   7426,   5370,   5884,   6398,   6912,   7680,   5627,   6141,   6655,   7169 },// W Element C (Q1+Q4)+(Q2+Q3) fund only
   { 0x007FC400, 0x0000,   7427,   5371,   5885,   6399,   6913,   7681,   5628,   6142,   6656,   7170 },// W Element C (Q1+Q4)-(Q2+Q3) fund only
   { 0x00004400, 0x0000,   7428,   5372,   5886,   6400,   6914,   7682,   5629,   6143,   6657,   7171 },// W Q1 fund + harmonics
   { 0x00008400, 0x0000,   7429,   5373,   5887,   6401,   6915,   7683,   5630,   6144,   6658,   7172 },// W Q2 fund + harmonics
   { 0x00010400, 0x0000,   7430,   5374,   5888,   6402,   6916,   7684,   5631,   6145,   6659,   7173 },// W Q3 fund + harmonics
   { 0x00020400, 0x0000,   7431,   5375,   5889,   6403,   6917,   7685,   5632,   6146,   6660,   7174 },// W Q4 fund + harmonics
   { 0x00024400, 0x0000,     45,    334,    335,    336,    337,     88,     48,     50,     93,    169 },// W Q1+Q4 fund + harmonics
   { 0x00018400, 0x0000,    183,    338,    339,    340,    341,    188,    173,    175,    177,    179 },// W Q2+Q3 fund + harmonics
   { 0x0003C400, 0x0000,    253,    346,    347,    344,    349,     55,     97,     98,     99,     96 },// W (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x0007C400, 0x0000,    250,    342,    343,    348,    345,     56,     89,     90,     91,     92 },// W (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00404400, 0x0000,   7432,   5376,   5890,   6404,   6918,   7686,   5633,   6147,   6661,   7175 },// W Q1 fund only
   { 0x00408400, 0x0000,   7433,   5377,   5891,   6405,   6919,   7687,   5634,   6148,   6662,   7176 },// W Q2 fund only
   { 0x00410400, 0x0000,   7434,   5378,   5892,   6406,   6920,   7688,   5635,   6149,   6663,   7177 },// W Q3 fund only
   { 0x00420400, 0x0000,   7435,   5379,   5893,   6407,   6921,   7689,   5636,   6150,   6664,   7178 },// W Q4 fund only
   { 0x00424400, 0x0000,   7436,   5380,   5894,   6408,   6922,     94,   5637,   6151,   6665,   7179 },// W Q1+Q4 fund only
   { 0x00418400, 0x0000,   7437,   5381,   5895,   6409,   6923,     95,   5638,   6152,   6666,   7180 },// W Q2+Q3 fund only
   { 0x0043C400, 0x0000,   7438,   5382,   5896,   6410,   6924,   7690,   5639,   6153,   6667,   7181 },// W (Q1+Q4)+(Q2+Q3) fund only
   { 0x0047C400, 0x0000,   7439,   5383,   5897,   6411,   6925,   7691,   5640,   6154,   6668,   7182 },// W (Q1+Q4)-(Q2+Q3) fund only

   { 0x00284401, 0x0000,   7440,   5384,   5898,   6412,   6926,   7692,   5641,   6155,   6669,   7183 },// var Element A Q1 fund + harmonics
   { 0x00288401, 0x0000,   7441,   5385,   5899,   6413,   6927,   7693,   5642,   6156,   6670,   7184 },// var Element A Q2 fund + harmonics
   { 0x00290401, 0x0000,   7442,   5386,   5900,   6414,   6928,   7694,   5643,   6157,   6671,   7185 },// var Element A Q3 fund + harmonics
   { 0x002A0401, 0x0000,   7443,   5387,   5901,   6415,   6929,   7695,   5644,   6158,   6672,   7186 },// var Element A Q4 fund + harmonics
   { 0x0028C401, 0x0000,   7444,   5388,   5902,   6416,   6930,   7696,   5645,   6159,   6673,   7187 },// var Element A Q1+Q2 fund + harmonics
   { 0x002B0401, 0x0000,   7445,   5389,   5903,   6417,   6931,   7697,   5646,   6160,   6674,   7188 },// var Element A Q3+Q4 fund + harmonics
   { 0x002BC401, 0x0000,   7446,   5390,   5904,   6418,   6932,   7698,   5647,   6161,   6675,   7189 },// var Element A (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x002FC401, 0x0000,   7447,   5391,   5905,   6419,   6933,   7699,   5648,   6162,   6676,   7190 },// var Element A (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00304401, 0x0000,   7448,   5392,   5906,   6420,   6934,   7700,   5649,   6163,   6677,   7191 },// var Element B Q1 fund + harmonics
   { 0x00308401, 0x0000,   7449,   5393,   5907,   6421,   6935,   7701,   5650,   6164,   6678,   7192 },// var Element B Q2 fund + harmonics
   { 0x00310401, 0x0000,   7450,   5394,   5908,   6422,   6936,   7702,   5651,   6165,   6679,   7193 },// var Element B Q3 fund + harmonics
   { 0x00320401, 0x0000,   7451,   5395,   5909,   6423,   6937,   7703,   5652,   6166,   6680,   7194 },// var Element B Q4 fund + harmonics
   { 0x0030C401, 0x0000,   7452,   5396,   5910,   6424,   6938,   7704,   5653,   6167,   6681,   7195 },// var Element B Q1+Q2 fund + harmonics
   { 0x00330401, 0x0000,   7453,   5397,   5911,   6425,   6939,   7705,   5654,   6168,   6682,   7196 },// var Element B Q3+Q4 fund + harmonics
   { 0x0033C401, 0x0000,   7454,   5398,   5912,   6426,   6940,   7706,   5655,   6169,   6683,   7197 },// var Element B (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x0037C401, 0x0000,   7455,   5399,   5913,   6427,   6941,   7707,   5656,   6170,   6684,   7198 },// var Element B (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00384401, 0x0000,   7456,   5400,   5914,   6428,   6942,   7708,   5657,   6171,   6685,   7199 },// var Element C Q1 fund + harmonics
   { 0x00388401, 0x0000,   7457,   5401,   5915,   6429,   6943,   7709,   5658,   6172,   6686,   7200 },// var Element C Q2 fund + harmonics
   { 0x00390401, 0x0000,   7458,   5402,   5916,   6430,   6944,   7710,   5659,   6173,   6687,   7201 },// var Element C Q3 fund + harmonics
   { 0x003A0401, 0x0000,   7459,   5403,   5917,   6431,   6945,   7711,   5660,   6174,   6688,   7202 },// var Element C Q4 fund + harmonics
   { 0x0038C401, 0x0000,   7460,   5404,   5918,   6432,   6946,   7712,   5661,   6175,   6689,   7203 },// var Element C Q1+Q2 fund + harmonics
   { 0x003B0401, 0x0000,   7461,   5405,   5919,   6433,   6947,   7713,   5662,   6176,   6690,   7204 },// var Element C Q3+Q4 fund + harmonics
   { 0x003BC401, 0x0000,   7462,   5406,   5920,   6434,   6948,   7714,   5663,   6177,   6691,   7205 },// var Element C (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x003FC401, 0x0000,   7463,   5407,   5921,   6435,   6949,   7715,   5664,   6178,   6692,   7206 },// var Element C (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00684401, 0x0000,   7464,   5408,   5922,   6436,   6950,   7716,   5665,   6179,   6693,   7207 },// var Element A Q1 fund only
   { 0x00688401, 0x0000,   7465,   5409,   5923,   6437,   6951,   7717,   5666,   6180,   6694,   7208 },// var Element A Q2 fund only
   { 0x00690401, 0x0000,   7466,   5410,   5924,   6438,   6952,   7718,   5667,   6181,   6695,   7209 },// var Element A Q3 fund only
   { 0x006A0401, 0x0000,   7467,   5411,   5925,   6439,   6953,   7719,   5668,   6182,   6696,   7210 },// var Element A Q4 fund only
   { 0x0068C401, 0x0000,   7468,   5412,   5926,   6440,   6954,   7720,   5669,   6183,   6697,   7211 },// var Element A Q1+Q2 fund only
   { 0x006B0401, 0x0000,   7469,   5413,   5927,   6441,   6955,   7721,   5670,   6184,   6698,   7212 },// var Element A Q3+Q4 fund only
   { 0x006BC401, 0x0000,   7470,   5414,   5928,   6442,   6956,   7722,   5671,   6185,   6699,   7213 },// var Element A (Q1+Q2)+(Q3+Q4) fund only
   { 0x006FC401, 0x0000,   7471,   5415,   5929,   6443,   6957,   7723,   5672,   6186,   6700,   7214 },// var Element A (Q1+Q2)-(Q3+Q4) fund only
   { 0x00704401, 0x0000,   7472,   5416,   5930,   6444,   6958,   7724,   5673,   6187,   6701,   7215 },// var Element B Q1 fund only
   { 0x00708401, 0x0000,   7473,   5417,   5931,   6445,   6959,   7725,   5674,   6188,   6702,   7216 },// var Element B Q2 fund only
   { 0x00710401, 0x0000,   7474,   5418,   5932,   6446,   6960,   7726,   5675,   6189,   6703,   7217 },// var Element B Q3 fund only
   { 0x00720401, 0x0000,   7475,   5419,   5933,   6447,   6961,   7727,   5676,   6190,   6704,   7218 },// var Element B Q4 fund only
   { 0x0070C401, 0x0000,   7476,   5420,   5934,   6448,   6962,   7728,   5677,   6191,   6705,   7219 },// Var Element B Q1+Q2 fund only
   { 0x00730401, 0x0000,   7477,   5421,   5935,   6449,   6963,   7729,   5678,   6192,   6706,   7220 },// Var Element B Q3+Q4 fund only
   { 0x0073C401, 0x0000,   7478,   5422,   5936,   6450,   6964,   7730,   5679,   6193,   6707,   7221 },// Var Element B (Q1+Q2)+(Q3+Q4) fund only
   { 0x0077C401, 0x0000,   7479,   5423,   5937,   6451,   6965,   7731,   5680,   6194,   6708,   7222 },// var Element B (Q1+Q2)-(Q3+Q4) fund only
   { 0x00784401, 0x0000,   7480,   5424,   5938,   6452,   6966,   7732,   5681,   6195,   6709,   7223 },// var Element C Q1 fund only
   { 0x00788401, 0x0000,   7481,   5425,   5939,   6453,   6967,   7733,   5682,   6196,   6710,   7224 },// var Element C Q2 fund only
   { 0x00790401, 0x0000,   7482,   5426,   5940,   6454,   6968,   7734,   5683,   6197,   6711,   7225 },// var Element C Q3 fund only
   { 0x007A0401, 0x0000,   7483,   5427,   5941,   6455,   6969,   7735,   5684,   6198,   6712,   7226 },// var Element C Q4 fund only
   { 0x0078C401, 0x0000,   7484,   5428,   5942,   6456,   6970,   7736,   5685,   6199,   6713,   7227 },// var Element C Q1+Q2 fund only
   { 0x007B0401, 0x0000,   7485,   5429,   5943,   6457,   6971,   7737,   5686,   6200,   6714,   7228 },// var Element C Q3+Q4 fund only
   { 0x007BC401, 0x0000,   7486,   5430,   5944,   6458,   6972,   7738,   5687,   6201,   6715,   7229 },// var Element C (Q1+Q2)+(Q3+Q4) fund only
   { 0x007FC401, 0x0000,   7487,   5431,   5945,   6459,   6973,   7739,   5688,   6202,   6716,   7230 },// var Element C (Q1+Q2)-(Q3+Q4) fund only
   { 0x00004401, 0x0000,   7488,   5432,   5946,   6460,   6974,   7740,   5689,   6203,   6717,   7231 },// var Q1 fund + harmonics
   { 0x00008401, 0x0000,   7489,   5433,   5947,   6461,   6975,   7741,   5690,   6204,   6718,   7232 },// var Q2 fund + harmonics
   { 0x00010401, 0x0000,   7490,   5434,   5948,   6462,   6976,   7742,   5691,   6205,   6719,   7233 },// var Q3 fund + harmonics
   { 0x00020401, 0x0000,   7491,   5435,   5949,   6463,   6977,   7743,   5692,   6206,   6720,   7234 },// var Q4 fund + harmonics
   { 0x0000C401, 0x0000,     47,    201,    449,    454,    459,    187,     49,     51,    168,    170 },// var Q1+Q2 fund + harmonics
   { 0x00030401, 0x0000,    228,    445,    450,    455,    460,    229,    174,    176,    178,    180 },// var Q3+Q4 fund + harmonics
   { 0x0003C401, 0x0000,    254,    446,    451,    456,    461,    257,    238,    239,    240,    241 },// var (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x0007C401, 0x0000,    252,    447,    452,    457,    462,    256,    234,    235,    236,    237 },// var (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00404401, 0x0000,   7492,   5436,   5950,   6464,   6978,   7744,   5693,   6207,   6721,   7235 },// var Q1 fund only
   { 0x00408401, 0x0000,   7493,   5437,   5951,   6465,   6979,   7745,   5694,   6208,   6722,   7236 },// var Q2 fund only
   { 0x00410401, 0x0000,   7494,   5438,   5952,   6466,   6980,   7746,   5695,   6209,   6723,   7237 },// var Q3 fund only
   { 0x00420401, 0x0000,   7495,   5439,   5953,   6467,   6981,   7747,   5696,   6210,   6724,   7238 },// var Q4 fund only
   { 0x0040C401, 0x0000,   7496,   5440,   5954,   6468,   6982,   7748,   5697,   6211,   6725,   7239 },// var Q1+Q2 fund only
   { 0x00430401, 0x0000,   7497,   5441,   5955,   6469,   6983,   7749,   5698,   6212,   6726,   7240 },// var Q3+Q4 fund only
   { 0x0043C401, 0x0000,   7498,   5442,   5956,   6470,   6984,   7750,   5699,   6213,   6727,   7241 },// var (Q1+Q2)+(Q3+Q4) fund only
   { 0x0047C401, 0x0000,   7499,   5443,   5957,   6471,   6985,   7751,   5700,   6214,   6728,   7242 },// var (Q1+Q2)-(Q3+Q4) fund only

   { 0x00284402, 0x0000,   7503,   5447,   5961,   6475,   6989,   7755,   5704,   6218,   6732,   7246 },// Apparent VA Element A Q1
   { 0x00288402, 0x0000,   7504,   5448,   5962,   6476,   6990,   7756,   5705,   6219,   6733,   7247 },// Apparent VA Element A Q2
   { 0x00290402, 0x0000,   7505,   5449,   5963,   6477,   6991,   7757,   5706,   6220,   6734,   7248 },// Apparent VA Element A Q3
   { 0x002A0402, 0x0000,   7506,   5450,   5964,   6478,   6992,   7758,   5707,   6221,   6735,   7249 },// Apparent VA Element A Q4

   { 0x00304402, 0x0000,   7507,   5451,   5965,   6479,   6993,   7759,   5708,   6222,   6736,   7250 },// Apparent VA Element B Q1
   { 0x00308402, 0x0000,   7508,   5452,   5966,   6480,   6994,   7760,   5709,   6223,   6737,   7251 },// Apparent VA Element B Q2
   { 0x00310402, 0x0000,   7509,   5453,   5967,   6481,   6995,   7761,   5710,   6224,   6738,   7252 },// Apparent VA Element B Q3
   { 0x00320402, 0x0000,   7510,   5454,   5968,   6482,   6996,   7762,   5711,   6225,   6739,   7253 },// Apparent VA Element B Q4

   { 0x00384402, 0x0000,   7511,   5455,   5969,   6483,   6997,   7763,   5712,   6226,   6740,   7254 },// Apparent VA Element C Q1
   { 0x00388402, 0x0000,   7512,   5456,   5970,   6484,   6998,   7764,   5713,   6227,   6741,   7255 },// Apparent VA Element C Q2
   { 0x00390402, 0x0000,   7513,   5457,   5971,   6485,   6999,   7765,   5714,   6228,   6742,   7256 },// Apparent VA Element C Q3
   { 0x003A0402, 0x0000,   7514,   5458,   5972,   6486,   7000,   7766,   5715,   6229,   6743,   7257 },// Apparent VA Element C Q4

   { 0x00004402, 0x0000,   7515,   5459,   5973,   6487,   7001,   7767,   5716,   6230,   6744,   7258 },// Apparent VA Q1
   { 0x80004402, 0x0001,   7556,   5503,   6017,   6531,   7045,   7808,   5760,   6274,   6788,   7302 },// Arithmetic Apparent VA Q1
   { 0x80004402, 0x0002,  14193,  15703,  15711,  15719,  15727,  14243,  14419,  14541,  14663,  14785 },// Distortion Apparent VA Q1
   { 0x80004402, 0x0100,  14194,  15704,  15712,  15720,  15728,  14244,  14420,  14542,  14664,  14786 },// Apparent () Q1
   { 0x80004402, 0x0101,   7557,   5504,   6018,   6532,   7046,   7809,   5761,   6275,   6789,   7303 },// Arithmetic Apparent VA Q1

   { 0x00008402, 0x0000,   7516,   5460,   5974,   6488,   7002,   7768,   5717,   6231,   6745,   7259 },// Apparent VA Q2
   { 0x80008402, 0x0001,   7558,   5505,   6019,   6533,   7047,   7810,   5762,   6276,   6790,   7304 },// Arithmetic Apparent VA Q2
   { 0x80008402, 0x0002,  14195,  15705,  15713,  15721,  15729,  14245,  14421,  14543,  14665,  14787 },// Distortion VA Q2
   { 0x80008402, 0x0100,  14196,  15706,  15714,  15722,  15730,  14246,  14422,  14544,  14666,  14788 },// Apparent () Q2
   { 0x80008402, 0x0101,   7559,   5506,   6020,   6534,   7048,   7811,   5763,   6277,   6791,   7305 },// Arithmetic Apparent VA Q2

   { 0x00010402, 0x0000,   7517,   5461,   5975,   6489,   7003,   7769,   5718,   6232,   6746,   7260 },// Apparent VA Q3
   { 0x80010402, 0x0001,   7560,   5507,   6021,   6535,   7049,   7812,   5764,   6278,   6792,   7306 },// Arithmetic Apparent VA Q3
   { 0x80010402, 0x0002,  14197,  15707,  15715,  15723,  15731,  14247,  14423,  14545,  14667,  14789 },// Distortion VA Q3
   { 0x80010402, 0x0100,  14198,  15708,  15716,  15724,  15732,  14248,  14424,  14546,  14668,  14790 },// Apparent () Q3
   { 0x80010402, 0x0101,   7561,   5508,   6022,   6536,   7050,   7813,   5765,   6279,   6793,   7307 },// Arithmetic Apparent VA Q3

   { 0x00020402, 0x0000,   7518,   5462,   5976,   6490,   7004,   7770,   5719,   6233,   6747,   7261 },// Apparent VA Q4
   { 0x80020402, 0x0001,   7562,   5509,   6023,   6537,   7051,   7814,   5766,   6280,   6794,   7308 },// Arithmetic Apparent VA Q4
   { 0x80020402, 0x0002,  14199,  15709,  15717,  15725,  15733,  14249,  14425,  14547,  14669,  14791},// Distortion VA Q4
   { 0x80020402, 0x0100,  14200,  15710,  15718,  15726,  15734,  14250,  14426,  14548,  14670,  14792},// Apparent () Q4
   { 0x80020402, 0x0101,   7563,   5510,   6024,   6538,   7052,   7815,   5767,   6281,   6795,   7309 },// Arithmetic Apparent VA Q4

   { 0x80024402, 0x0000,     46,   5463,   5977,   6491,   7005,    186,   5720,   6234,   6748,   7262 },// Apparent VA Q1+Q4
   { 0x80024402, 0x0001,  14121,  14305,  14427,  14549,  14671,  14201,  14377,  14499,  14621,  14743 },// Arithmetic Apparent VA Q1+Q4
   { 0x80024402, 0x0002,  14122,  14306,  14428,  14550,  14672,  14202,  14378,  14500,  14622,  14744 },// Distortion VA Q1+Q4
   { 0x80024402, 0x0100,   7519,   5464,   5978,   6492,   7006,   7771,   5721,   6235,   6749,   7263 },// Apparent () Q1+Q4
   { 0x80024402, 0x0101,  14123,  14307,  14429,  14551,  14673,  14203,  14379,  14501,  14623,  14745 },// Arithmetic Apparent () Q1+Q4

   { 0x80018402, 0x0000,    184,   5465,   5979,   6493,   7007,    189,   5722,   6236,   6750,   7264 },// Apparent VA Q2+Q3
   { 0x80018402, 0x0001,  14124,  14308,  14430,  14552,  14674,  14204,  14380,  14502,  14624,  14746 },// Arithmetic Apparent VA Q2+Q3
   { 0x80018402, 0x0002,  14125,  14309,  14431,  14553,  14675,  14205,  14381,  14503,  14625,  14747 },// Distortion VA Q2+Q3
   { 0x80018402, 0x0100,   7520,   5466,   5980,   6494,   7008,   7772,   5723,   6237,   6751,   7265 },// Apparent () Q2+Q3
   { 0x80018402, 0x0101,  14126,  14310,  14432,  14554,  14676,  14206,  14382,  14504,  14626,  14748 },// Arithmetic Apparent () Q2+Q3

   { 0x0003C402, 0x0000,    230,    448,    453,    458,    463,    231,    464,    465,    466,    467 },// Apparent VA
   { 0x8003C402, 0x0000,    230,    448,    453,    458,    463,    231,    464,    465,    466,    467 },// Apparent VA (Q1+Q4)+(Q2+Q3)
   { 0x8003C402, 0x0100,   7521,   5467,   5981,   6495,   7009,   7773,   5724,   6238,   6752,   7266 },// Apparent VA (Q1+Q4)+(Q2+Q3)
   { 0x8003C402, 0x0001,   7554,   5501,   6015,   6529,   7043,   7806,   5758,   6272,   6786,   7300 },// Arithmetic Apparent VA
   { 0x8003C402, 0x0101,   7555,   5502,   6016,   6530,   7044,   7807,   5759,   6273,   6787,   7301 },// Arithmetic Apparent VA
   { 0x8003C402, 0x0002,   7627,   5574,   6088,   6602,   7116,   7879,   5831,   6345,   6859,   7373 },// Distortion VA
   { 0x8003C402, 0x0102,   7628,   5575,   6089,   6603,   7117,   7880,   5832,   6346,   6860,   7374 },// Distortion VA

   { 0x8007C402, 0x0000,    251,   5468,   5982,   6496,   7010,    255,   5725,   6239,   6753,   7267 },// Apparent VA (Q1+Q4)-(Q2+Q3)
   { 0x8007C402, 0x0001,  14127,  14311,  14433,  14555,  14677,  14207,  14383,  14505,  14627,  14749 },// Arithmetic Apparent VA (Q1+Q4)-(Q2+Q3)
   { 0x8007C402, 0x0002,  14128,  14312,  14434,  14556,  14678,  14208,  14384,  14506,  14628,  14750 },// Distortion VA (Q1+Q4)-(Q2+Q3)
   { 0x8007C402, 0x0100,   7522,   5469,   5983,   6497,   7011,   7774,   5726,   6240,   6754,   7268 },// Apparent () (Q1+Q4)-(Q2+Q3)
   { 0x8007C402, 0x0101,  14129,  14313,  14435,  14557,  14679,  14209,  14385,  14507,  14629,  14751 },// Arithmetic Apparent () (Q1+Q4)-(Q2+Q3)

   { 0x802A4402, 0x0000,   7523,   5470,   5984,   6498,   7012,   7775,   5727,   6241,   6755,   7269 },// Apparent VA Element A Q1+Q4
   { 0x802A4402, 0x0001,  14130,  14314,  14436,  14558,  14680,  14210,  14386,  14508,  14630,  14752 },// Arithmetic Apparent VA Element A Q1+Q4
   { 0x802A4402, 0x0002,  14131,  14315,  14437,  14559,  14681,  14211,  14387,  14509,  14631,  14753 },// Distortion VA Element A Q1+Q4
   { 0x802A4402, 0x0100,   7524,   5471,   5985,   6499,   7013,   7776,   5728,   6242,   6756,   7270 },// Apparent () Element A Q1+Q4
   { 0x802A4402, 0x0101,  14132,  14316,  14438,  14560,  14682,  14212,  14388,  14510,  14632,  14754 },// Arithmetic Apparent () Element A Q1+Q4

   { 0x80298402, 0x0000,   7525,   5472,   5986,   6500,   7014,   7777,   5729,   6243,   6757,   7271 },// Apparent VA Element A Q2+Q3
   { 0x80298402, 0x0001,  14133,  14317,  14439,  14561,  14683,  14213,  14389,  14511,  14633,  14755 },// Arithmetic Apparent VA Element A Q2+Q3
   { 0x80298402, 0x0002,  14134,  14318,  14440,  14562,  14684,  14214,  14390,  14512,  14634,  14756 },// Distortion VA Element A Q2+Q3
   { 0x80298402, 0x0100,   7526,   5473,   5987,   6501,   7015,   7778,   5730,   6244,   6758,   7272 },// Apparent () Element A Q2+Q3
   { 0x80298402, 0x0101,  14135,  14319,  14441,  14563,  14685,  14215,  14391,  14513,  14635,  14757 },// Arithmetic Apparent () Element A Q2+Q3

   { 0x002BC402, 0x0000,   7500,   5444,   5958,   6472,   6986,   7752,   5701,   6215,   6729,   7243 },// Apparent VA Element A
   { 0x802BC402, 0x0000,   7500,   5444,   5958,   6472,   6986,   7752,   5701,   6215,   6729,   7243 },// Apparent VA Element A (Q1+Q4)+(Q2+Q3)
   { 0x802BC402, 0x0001,  14136,  14320,  14442,  14564,  14686,  14216,  14392,  14514,  14636,  14758 },// Arithmetic Apparent VA Element A (Q1+Q4)+(Q2+Q3)
   { 0x802BC402, 0x0002,   7621,   5568,   6082,   6596,   7110,   7873,   5825,   6339,   6853,   7367 },// Distortion VA Element A
   { 0x802BC402, 0x0100,   7527,   5474,   5988,   6502,   7016,   7779,   5731,   6245,   6759,   7273 },// Apparent () Element A (Q1+Q4)+(Q2+Q3)
   { 0x802BC402, 0x0101,  14137,  14321,  14443,  14565,  14687,  14217,  14393,  14515,  14637,  14759 },// Arithmetic Apparent () Element A (Q1+Q4)+(Q2+Q3)
   { 0x802BC402, 0x0102,   7622,   5569,   6083,   6597,   7111,   7874,   5826,   6340,   6854,   7368 },// Distortion () Element A

   { 0x802FC402, 0x0000,   7528,   5475,   5989,   6503,   7017,   7780,   5732,   6246,   6760,   7274 },// Apparent VA Element A (Q1+Q4)-(Q2+Q3)
   { 0x802FC402, 0x0001,  14138,  14322,  14444,  14566,  14688,  14218,  14394,  14516,  14638,  14760 },// Arithmetic Apparent VA Element A (Q1+Q4)-(Q2+Q3)
   { 0x802FC402, 0x0002,  14139,  14323,  14445,  14567,  14689,  14219,  14395,  14517,  14639,  14761 },// Distortion VA Element A (Q1+Q4)-(Q2+Q3)
   { 0x802FC402, 0x0100,   7529,   5476,   5990,   6504,   7018,   7781,   5733,   6247,   6761,   7275 },// Apparent VA Element A (Q1+Q4)-(Q2+Q3)
   { 0x802FC402, 0x0101,  14140,  14324,  14446,  14568,  14690,  14220,  14396,  14518,  14640,  14762 },// Arithmetic Apparent () Element A (Q1+Q4)-(Q2+Q3)

   { 0x80324402, 0x0000,   7530,   5477,   5991,   6505,   7019,   7782,   5734,   6248,   6762,   7276 },// Apparent VA Element B Q1+Q4
   { 0x80324402, 0x0001,  14141,  14325,  14447,  14569,  14691,  14221,  14397,  14519,  14641,  14763 },// Arithmetic Apparent VA Element B Q1+Q4
   { 0x80324402, 0x0002,  14142,  14326,  14448,  14570,  14692,  14222,  14398,  14520,  14642,  14764 },// Distortion VA Element B Q1+Q4
   { 0x80324402, 0x0100,   7531,   5478,   5992,   6506,   7020,   7783,   5735,   6249,   6763,   7277 },// Apparent () Element B Q1+Q4
   { 0x80324402, 0x0101,  14143,  14327,  14449,  14571,  14693,  14223,  14399,  14521,  14643,  14765 },// Arithmetic Apparent () Element B Q1+Q4

   { 0x80318402, 0x0000,   7532,   5479,   5993,   6507,   7021,   7784,   5736,   6250,   6764,   7278 },// Apparent VA Element B Q2+Q3
   { 0x80318402, 0x0001,  14144,  14328,  14450,  14572,  14694,  14224,  14400,  14522,  14644,  14766 },// Arithmetic Apparent VA Element B Q2+Q3
   { 0x80318402, 0x0002,  14145,  14329,  14451,  14573,  14695,  14225,  14401,  14523,  14645,  14767 },// Distortion VA Element B Q2+Q3
   { 0x80318402, 0x0100,   7533,   5480,   5994,   6508,   7022,   7785,   5737,   6251,   6765,   7279 },// Apparent () Element B Q2+Q3
   { 0x80318402, 0x0101,  14146,  14330,  14452,  14574,  14696,  14226,  14402,  14524,  14646,  14768 },// Arithmetic Apparent () Element B Q2+Q3

   { 0x0033C402, 0x0000,   7501,   5445,   5959,   6473,   6987,   7753,   5702,   6216,   6730,   7244 },// Apparent VA Element B
   { 0x8033C402, 0x0000,   7501,   5445,   5959,   6473,   6987,   7753,   5702,   6216,   6730,   7244 },// Apparent VA Element B (Q1+Q4)+(Q2+Q3)
   { 0x8033C402, 0x0001,  14147,  14331,  14453,  14575,  14697,  14227,  14403,  14525,  14647,  14769 },// Arithmetic Apparent VA Element B (Q1+Q4)+(Q2+Q3)
   { 0x8033C402, 0x0002,   7623,   5570,   6084,   6598,   7112,   7875,   5827,   6341,   6855,   7369 },// Distortion VA Element B
   { 0x8033C402, 0x0100,   7534,   5481,   5995,   6509,   7023,   7786,   5738,   6252,   6766,   7280 },// Apparent () Element B (Q1+Q4)+(Q2+Q3)
   { 0x8033C402, 0x0101,  14148,  14332,  14454,  14576,  14698,  14228,  14404,  14526,  14648,  14770 },// Arithmetic Apparent () Element B (Q1+Q4)+(Q2+Q3)
   { 0x8033C402, 0x0102,   7624,   5571,   6085,   6599,   7113,   7876,   5828,   6342,   6856,   7370 },// Distortion () Element B

   { 0x8037C402, 0x0000,   7535,   5482,   5996,   6510,   7024,   7787,   5739,   6253,   6767,   7281 },// Apparent VA Element B (Q1+Q4)-(Q2+Q3)
   { 0x8037C402, 0x0001,  14149,  14333,  14455,  14577,  14699,  14229,  14405,  14527,  14649,  14771 },// Arithmetic Apparent VA Element B (Q1+Q4)-(Q2+Q3)
   { 0x8037C402, 0x0002,  14150,  14334,  14456,  14578,  14700,  14230,  14406,  14528,  14650,  14772 },// Distortion VA Element B (Q1+Q4)-(Q2+Q3)
   { 0x8037C402, 0x0100,   7536,   5483,   5997,   6511,   7025,   7788,   5740,   6254,   6768,   7282 },// Apparent () Element B (Q1+Q4)-(Q2+Q3)
   { 0x8037C402, 0x0101,  14151,  14335,  14457,  14579,  14701,  14231,  14407,  14529,  14651,  14773 },// Arithmetic Apparent () Element B (Q1+Q4)-(Q2+Q3)

   { 0x803A4402, 0x0000,   7537,   5484,   5998,   6512,   7026,   7789,   5741,   6255,   6769,   7283 },// Apparent VA Element C Q1+Q4
   { 0x803A4402, 0x0001,  14152,  14336,  14458,  14580,  14702,  14232,  14408,  14530,  14652,  14774 },// Arithmetic Apparent VA Element C Q1+Q4
   { 0x803A4402, 0x0002,  14153,  14337,  14459,  14581,  14703,  14233,  14409,  14531,  14653,  14775 },// Distortion VA Element C Q1+Q4
   { 0x803A4402, 0x0100,   7538,   5485,   5999,   6513,   7027,   7790,   5742,   6256,   6770,   7284 },// Apparent () Element C Q1+Q4
   { 0x803A4402, 0x0101,  14154,  14338,  14460,  14582,  14704,  14234,  14410,  14532,  14654,  14776 },// Arithmetic Apparent () Element C Q1+Q4

   { 0x80398402, 0x0000,   7539,   5486,   6000,   6514,   7028,   7791,   5743,   6257,   6771,   7285 },// Apparent VA Element C Q2+Q3
   { 0x80398402, 0x0001,  14155,  14339,  14461,  14583,  14705,  14235,  14411,  14533,  14655,  14777 },// Arithmetic Apparent VA Element C Q2+Q3
   { 0x80398402, 0x0002,  14156,  14340,  14462,  14584,  14706,  14236,  14412,  14534,  14656,  14778 },// Distortion VA Element C Q2+Q3
   { 0x80398402, 0x0100,   7540,   5487,   6001,   6515,   7029,   7792,   5744,   6258,   6772,   7286 },// Apparent () Element C Q2+Q3
   { 0x80398402, 0x0101,  14157,  14341,  14463,  14585,  14707,  14237,  14413,  14535,  14657,  14779 },// Arithmetic Apparent () Element C Q2+Q3

   { 0x003BC402, 0x0000,   7502,   5446,   5960,   6474,   6988,   7754,   5703,   6217,   6731,   7245 },// Apparent VA Element C
   { 0x803BC402, 0x0000,   7502,   5446,   5960,   6474,   6988,   7754,   5703,   6217,   6731,   7245 },// Apparent VA Element C (Q1+Q4)+(Q2+Q3)
   { 0x803BC402, 0x0001,  14158,  14342,  14464,  14586,  14708,  14238,  14414,  14536,  14658,  14780 },// Arithmetic Apparent VA Element C (Q1+Q4)+(Q2+Q3)
   { 0x803BC402, 0x0002,   7625,   5572,   6086,   6600,   7114,   7877,   5829,   6343,   6857,   7371 },// Distortion VA  Element C
   { 0x803BC402, 0x0100,   7541,   5488,   6002,   6516,   7030,   7793,   5745,   6259,   6773,   7287 },// Apparent VA Element C (Q1+Q4)+(Q2+Q3)
   { 0x803BC402, 0x0101,  14159,  14343,  14465,  14587,  14709,  14239,  14415,  14537,  14659,  14781 },// Arithmetic Apparent () Element C (Q1+Q4)+(Q2+Q3)
   { 0x803BC402, 0x0102,   7626,   5573,   6087,   6601,   7115,   7878,   5830,   6344,   6858,   7372 },// Distortion ()  Element C

   { 0x803FC402, 0x0000,   7542,   5489,   6003,   6517,   7031,   7794,   5746,   6260,   6774,   7288 },// Apparent VA Element C (Q1+Q4)-(Q2+Q3)
   { 0x803FC402, 0x0001,  14160,  14344,  14466,  14588,  14710,  14240,  14416,  14538,  14660,  14782 },// Arithmetic Apparent VA Element C (Q1+Q4)-(Q2+Q3)
   { 0x803FC402, 0x0002,  14161,  14345,  14467,  14589,  14711,  14241,  14417,  14539,  14661,  14783 },// Distortion VA Element C (Q1+Q4)-(Q2+Q3)
   { 0x803FC402, 0x0100,   7543,   5490,   6004,   6518,   7032,   7795,   5747,   6261,   6775,   7289 },// Apparent () Element C (Q1+Q4)-(Q2+Q3)
   { 0x803FC402, 0x0101,  14162,  14346,  14468,  14590,  14712,  14242,  14418,  14540,  14662,  14784 },// Arithmetic Apparent () Element C (Q1+Q4)-(Q2+Q3)

   { 0x8003C403, 0x0000,   7544,   5491,   6005,   6519,   7033,   7796,   5748,   6262,   6776,   7290 },// Phasor Apparent VA, fund + harmonics
   { 0x8003C403, 0x0001,  14163,  14347,  14469,  14591,  14713,  15433,  15463,  15493,  15523,  15553 },// Phasor Apparent Fuzzy VA, fund + harmonics
   { 0x8003C403, 0x0100,  14173,  14357,  14479,  14601,  14723,  15443,  15473,  15503,  15533,  15563 },// Phasor Apparent (), fund + harmonics
   { 0x8003C403, 0x0101,  14183,  14367,  14489,  14611,  14733,  15453,  15483,  15513,  15543,  15573 },// Phasor Apparent Fuzzy (), fund + harmonics

   { 0x8043C403, 0x0000,   7545,   5492,   6006,   6520,   7034,   7797,   5749,   6263,   6777,   7291 },// Phasor Apparent VA, fund only
   { 0x8043C403, 0x0001,  14164,  14348,  14470,  14592,  14714,  15434,  15464,  15494,  15524,  15554 },// Phasor Apparent Fuzzy VA, fund only
   { 0x8043C403, 0x0100,  14174,  14358,  14480,  14602,  14724,  15444,  15474,  15504,  15534,  15564 },// Phasor Apparent (), fund only
   { 0x8043C403, 0x0101,  14184,  14368,  14490,  14612,  14734,  15454,  15484,  15514,  15544,  15574 },// Phasor Apparent Fuzzy (), fund only

   { 0x80004403, 0x0000,   7546,   5493,   6007,   6521,   7035,   7798,   5750,   6264,   6778,   7292 },// Phasor Apparent VA Q1 fund+harmonics
   { 0x80004403, 0x0001,  14165,  14349,  14471,  14593,  14715,  15435,  15465,  15495,  15525,  15555 },// Phasor Apparent Fuzzy VA Q1 fund+harmonics
   { 0x80004403, 0x0100,  14175,  14359,  14481,  14603,  14725,  15445,  15475,  15505,  15535,  15565 },// Phasor Apparent () Q1 fund+harmonics
   { 0x80004403, 0x0101,  14185,  14369,  14491,  14613,  14735,  15455,  15485,  15515,  15545,  15575 },// Phasor Apparent Fuzzy () Q1 fund+harmonics

   { 0x80008403, 0x0000,   7547,   5494,   6008,   6522,   7036,   7799,   5751,   6265,   6779,   7293 },// Phasor Apparent VA Q2 fund+harmonics
   { 0x80008403, 0x0001,  14166,  14350,  14472,  14594,  14716,  15436,  15466,  15496,  15526,  15556 },// Phasor Apparent Fuzzy VA Q2 fund+harmonics
   { 0x80008403, 0x0100,  14176,  14360,  14482,  14604,  14726,  15446,  15476,  15506,  15536,  15566 },// Phasor Apparent () Q2 fund+harmonics
   { 0x80008403, 0x0101,  14186,  14370,  14492,  14614,  14736,  15456,  15486,  15516,  15546,  15576 },// Phasor Apparent Fuzzy () Q2 fund+harmonics

   { 0x80010403, 0x0000,   7548,   5495,   6009,   6523,   7037,   7800,   5752,   6266,   6780,   7294 },// Phasor Apparent VA Q3 fund+harmonics
   { 0x80010403, 0x0001,  14167,  14351,  14473,  14595,  14717,  15437,  15467,  15497,  15527,  15557 },// Phasor Apparent Fuzzy VA Q3 fund+harmonics
   { 0x80010403, 0x0100,  14177,  14361,  14483,  14605,  14727,  15447,  15477,  15507,  15537,  15567 },// Phasor Apparent () Q3 fund+harmonics
   { 0x80010403, 0x0101,  14187,  14371,  14493,  14615,  14737,  15457,  15487,  15517,  15547,  15577 },// Phasor Apparent Fuzzy () Q3 fund+harmonics

   { 0x80020403, 0x0000,   7549,   5496,   6010,   6524,   7038,   7801,   5753,   6267,   6781,   7295 },// Phasor Apparent VA Q4 fund+harmonics
   { 0x80020403, 0x0001,  14168,  14352,  14474,  14596,  14718,  15438,  15468,  15498,  15528,  15558 },// Phasor Apparent Fuzzy VA Q4 fund+harmonics
   { 0x80020403, 0x0100,  14178,  14362,  14484,  14606,  14728,  15448,  15478,  15508,  15538,  15568 },// Phasor Apparent () Q4 fund+harmonics
   { 0x80020403, 0x0101,  14188,  14372,  14494,  14616,  14738,  15458,  15488,  15518,  15548,  15578 },// Phasor Apparent Fuzzy () Q4 fund+harmonics

   { 0x80404403, 0x0000,   7550,   5497,   6011,   6525,   7039,   7802,   5754,   6268,   6782,   7296 },// Phasor Apparent VA Q1 fund only
   { 0x80404403, 0x0001,  14169,  14353,  14475,  14597,  14719,  15439,  15469,  15499,  15529,  15559 },// Phasor Apparent Fuzzy VA Q1 fund only
   { 0x80404403, 0x0100,  14179,  14363,  14485,  14607,  14729,  15449,  15479,  15509,  15539,  15569 },// Phasor Apparent () Q1 fund only
   { 0x80404403, 0x0101,  14189,  14373,  14495,  14617,  14739,  15459,  15489,  15519,  15549,  15579 },// Phasor Apparent Fuzzy () Q1 fund only

   { 0x80408403, 0x0000,   7551,   5498,   6012,   6526,   7040,   7803,   5755,   6269,   6783,   7297 },// Phasor Apparent VA Q2 fund only
   { 0x80408403, 0x0001,  14170,  14354,  14476,  14598,  14720,  15440,  15470,  15500,  15530,  15560 },// Phasor Apparent Fuzzy VA Q2 fund only
   { 0x80408403, 0x0100,  14180,  14364,  14486,  14608,  14730,  15450,  15480,  15510,  15540,  15570 },// Phasor Apparent () Q2 fund only
   { 0x80408403, 0x0101,  14190,  14374,  14496,  14618,  14740,  15460,  15490,  15520,  15550,  15580 },// Phasor Apparent Fuzzy () Q2 fund only

   { 0x80410403, 0x0000,   7552,   5499,   6013,   6527,   7041,   7804,   5756,   6270,   6784,   7298 },// Phasor Apparent VA Q3 fund only
   { 0x80410403, 0x0001,  14171,  14355,  14477,  14599,  14721,  15441,  15471,  15501,  15531,  15561 },// Phasor Apparent Fuzzy VA Q3 fund only
   { 0x80410403, 0x0100,  14181,  14365,  14487,  14609,  14731,  15451,  15481,  15511,  15541,  15571 },// Phasor Apparent () Q3 fund only
   { 0x80410403, 0x0101,  14191,  14375,  14497,  14619,  14741,  15461,  15491,  15521,  15551,  15581 },// Phasor Apparent Fuzzy () Q3 fund only

   { 0x80420403, 0x0000,   7553,   5500,   6014,   6528,   7042,   7805,   5757,   6271,   6785,   7299 },// Phasor Apparent VA Q4 fund only
   { 0x80420403, 0x0001,  14172,  14356,  14478,  14600,  14722,  15442,  15472,  15502,  15532,  15562 },// Phasor Apparent Fuzzy VA Q4 fund only
   { 0x80420403, 0x0100,  14182,  14366,  14488,  14610,  14732,  15452,  15482,  15512,  15542,  15572 },// Phasor Apparent (0 Q4 fund only
   { 0x80420403, 0x0101,  14192,  14376,  14498,  14620,  14742,  15462,  15492,  15522,  15552,  15582 },// Phasor Apparent Fuzzy () Q4 fund only

   { 0x0058040A, 0x0000,  12489,  11569,  11677,  11785,  11893,  12544,  11623,  11731,  11839,  11947 },// V2AC, fund
   { 0x0050040A, 0x0000,  12490,  11570,  11678,  11786,  11894,  12545,  11624,  11732,  11840,  11948 },// V2CB, fund
   { 0x0048040A, 0x0000,  12491,  11571,  11679,  11787,  11895,  12546,  11625,  11733,  11841,  11949 },// V2BA, fund
   { 0x0018040A, 0x0000,  12492,  11572,  11680,  11788,  11896,  12547,  11626,  11734,  11842,  11950 },// V2AC, fund+harmonics
   { 0x0010040A, 0x0000,  12493,  11573,  11681,  11789,  11897,  12548,  11627,  11735,  11843,  11951 },// V2CB, fund+harmonics
   { 0x0008040A, 0x0000,  12494,  11574,  11682,  11790,  11898,  12549,  11628,  11736,  11844,  11952 },// V2BA, fund+harmonics
   { 0x0068040A, 0x0000,  12495,  11575,  11683,  11791,  11899,  12550,  11629,  11737,  11845,  11953 },// V2 Element A, fund+harmonics
   { 0x0070040A, 0x0000,  12496,  11576,  11684,  11792,  11900,  12551,  11630,  11738,  11846,  11954 },// V2 Element B, fund+harmonics
   { 0x0078040A, 0x0000,  12497,  11577,  11685,  11793,  11901,  12552,  11631,  11739,  11847,  11955 },// V2 Element C, fund+harmonics

   { 0x80280208, 0x0001,   7564,   5511,   6025,   6539,   7053,   7816,   5768,   6282,   6796,   7310 },// VA fund + harmonics MAX
   { 0x80280208, 0x0002,   7565,   5512,   6026,   6540,   7054,   7817,   5769,   6283,   6797,   7311 },// VA fund + harmonics MIN
   { 0x80280208, 0x0003,   7566,   5513,   6027,   6541,   7055,   7818,   5770,   6284,   6798,   7312 },// VA fund + harmonics SNAPSHOT

   { 0x80300208, 0x0001,   7567,   5514,   6028,   6542,   7056,   7819,   5771,   6285,   6799,   7313 },// VB fund + harmonics MAX
   { 0x80300208, 0x0002,   7568,   5515,   6029,   6543,   7057,   7820,   5772,   6286,   6800,   7314 },// VB fund + harmonics MIN
   { 0x80300208, 0x0003,   7569,   5516,   6030,   6544,   7058,   7821,   5773,   6287,   6801,   7315 },// VB fund + harmonics SNAPSHOT

   { 0x80380208, 0x0001,   7570,   5517,   6031,   6545,   7059,   7822,   5774,   6288,   6802,   7316 },// VC fund + harmonics MAX
   { 0x80380208, 0x0002,   7571,   5518,   6032,   6546,   7060,   7823,   5775,   6289,   6803,   7317 },// VC fund + harmonics MIN
   { 0x80380208, 0x0003,   7572,   5519,   6033,   6547,   7061,   7824,   5776,   6290,   6804,   7318 },// VC fund + harmonics

   { 0x80680208, 0x0001,   7573,   5520,   6034,   6548,   7062,   7825,   5777,   6291,   6805,   7319 },// VA fund only MAX
   { 0x80680208, 0x0002,   7574,   5521,   6035,   6549,   7063,   7826,   5778,   6292,   6806,   7320 },// VA fund only MIN
   { 0x80680208, 0x0003,   7575,   5522,   6036,   6550,   7064,   7827,   5779,   6293,   6807,   7321 },// VA fund only SNAPSHOT

   { 0x80700208, 0x0001,   7576,   5523,   6037,   6551,   7065,   7828,   5780,   6294,   6808,   7322 },// VB fund only MAX
   { 0x80700208, 0x0002,   7577,   5524,   6038,   6552,   7066,   7829,   5781,   6295,   6809,   7323 },// VB fund only MIN
   { 0x80700208, 0x0003,   7578,   5525,   6039,   6553,   7067,   7830,   5782,   6296,   6810,   7324 },// VB fund only

   { 0x80780208, 0x0001,   7579,   5526,   6040,   6554,   7068,   7831,   5783,   6297,   6811,   7325 },// VC fund only MAX
   { 0x80780208, 0x0002,   7580,   5527,   6041,   6555,   7069,   7832,   5784,   6298,   6812,   7326 },// VC fund only MIN
   { 0x80780208, 0x0003,   7581,   5528,   6042,   6556,   7070,   7833,   5785,   6299,   6813,   7327 },// VC fund only SNAPSHOT

   { 0x80180208, 0x0001,   7582,   5529,   6043,   6557,   7071,   7834,   5786,   6300,   6814,   7328 },// VAC fund + harmonics MAX
   { 0x80180208, 0x0002,   7583,   5530,   6044,   6558,   7072,   7835,   5787,   6301,   6815,   7329 },// VAC fund + harmonics MIN
   { 0x80180208, 0x0003,   7584,   5531,   6045,   6559,   7073,   7836,   5788,   6302,   6816,   7330 },// VAC fund + harmonics SNAPSHOT

   { 0x80100208, 0x0001,   7585,   5532,   6046,   6560,   7074,   7837,   5789,   6303,   6817,   7331 },// VCB fund + harmonics MAX
   { 0x80100208, 0x0002,   7586,   5533,   6047,   6561,   7075,   7838,   5790,   6304,   6818,   7332 },// VCB fund + harmonics MIN
   { 0x80100208, 0x0003,   7587,   5534,   6048,   6562,   7076,   7839,   5791,   6305,   6819,   7333 },// VCB fund + harmonics SNAPSHOT

   { 0x80080208, 0x0001,   7588,   5535,   6049,   6563,   7077,   7840,   5792,   6306,   6820,   7334 },// VBA fund + harmonics MAX
   { 0x80080208, 0x0002,   7589,   5536,   6050,   6564,   7078,   7841,   5793,   6307,   6821,   7335 },// VBA fund + harmonics MIN
   { 0x80080208, 0x0003,   7590,   5537,   6051,   6565,   7079,   7842,   5794,   6308,   6822,   7336 },// VBA fund + harmonics SNAPSHOT

   { 0x80580208, 0x0001,   7591,   5538,   6052,   6566,   7080,   7843,   5795,   6309,   6823,   7337 },// VAC fund only MAX
   { 0x80580208, 0x0002,   7592,   5539,   6053,   6567,   7081,   7844,   5796,   6310,   6824,   7338 },// VAC fund only MIN
   { 0x80580208, 0x0003,   7593,   5540,   6054,   6568,   7082,   7845,   5797,   6311,   6825,   7339 },// VAC fund only SNAPSHOT

   { 0x80500208, 0x0001,   7594,   5541,   6055,   6569,   7083,   7846,   5798,   6312,   6826,   7340 },// VCB fund only MAX
   { 0x80500208, 0x0002,   7595,   5542,   6056,   6570,   7084,   7847,   5799,   6313,   6827,   7341 },// VCB fund only MIN
   { 0x80500208, 0x0003,   7596,   5543,   6057,   6571,   7085,   7848,   5800,   6314,   6828,   7342 },// VCB fund only SNAPSHOT

   { 0x80480208, 0x0001,   7597,   5544,   6058,   6572,   7086,   7849,   5801,   6315,   6829,   7343 },// VBA fund only MAX
   { 0x80480208, 0x0002,   7598,   5545,   6059,   6573,   7087,   7850,   5802,   6316,   6830,   7344 },// VBA fund only MIN
   { 0x80480208, 0x0003,   7599,   5546,   6060,   6574,   7088,   7851,   5803,   6317,   6831,   7345 },// VBA fund only SNAPSHOT

   { 0x0028040E, 0x0000,  12498,  11578,  11686,  11794,  11902,  12553,  11632,  11740,  11848,  11956 },// I2, Element A, fund + harmonics
   { 0x0030040E, 0x0000,  12499,  11579,  11687,  11795,  11903,  12554,  11633,  11741,  11849,  11957 },// I2, Element B, fund + harmonics
   { 0x0038040E, 0x0000,  12500,  11580,  11688,  11796,  11904,  12555,  11634,  11742,  11850,  11958 },// I2, Element C, fund + harmonics
   { 0x0068040E, 0x0000,  12501,  11581,  11689,  11797,  11905,  12556,  11635,  11743,  11851,  11959 },// I2, Element A, fund
   { 0x0070040E, 0x0000,  12502,  11582,  11690,  11798,  11906,  12557,  11636,  11744,  11852,  11960 },// I2, Element B, fund
   { 0x0078040E, 0x0000,  12503,  11583,  11691,  11799,  11907,  12558,  11637,  11745,  11853,  11961 },// I2, Element C, fund
   { 0x0020040E, 0x0000,  12504,  12001,  12003,  12005,  12007,  12559,  12002,  12004,  12006,  12008 },// I2, Element N

   { 0x8020020C, 0x0001,   7600,   5547,   6061,   6575,   7089,   7852,   5804,   6318,   6832,   7346 },// In fund + harmonics MAX
   { 0x8020020C, 0x0002,   7601,   5548,   6062,   6576,   7090,   7853,   5805,   6319,   6833,   7347 },// In fund + harmonics MIN
   { 0x8020020C, 0x0003,   7602,   5549,   6063,   6577,   7091,   7854,   5806,   6320,   6834,   7348 },// In fund + harmonics SNAPSHOT

   { 0x8028020C, 0x0001,   7603,   5550,   6064,   6578,   7092,   7855,   5807,   6321,   6835,   7349 },// IA fund + harmonics MAX
   { 0x8028020C, 0x0002,   7604,   5551,   6065,   6579,   7093,   7856,   5808,   6322,   6836,   7350 },// IA fund + harmonics MIN
   { 0x8028020C, 0x0003,   7605,   5552,   6066,   6580,   7094,   7857,   5809,   6323,   6837,   7351 },// IA fund + harmonics SNAPSHOT

   { 0x8030020C, 0x0001,   7606,   5553,   6067,   6581,   7095,   7858,   5810,   6324,   6838,   7352 },// IB fund + harmonics MAX
   { 0x8030020C, 0x0002,   7607,   5554,   6068,   6582,   7096,   7859,   5811,   6325,   6839,   7353 },// IB fund + harmonics MIN
   { 0x8030020C, 0x0003,   7608,   5555,   6069,   6583,   7097,   7860,   5812,   6326,   6840,   7354 },// IB fund + harmonics SNAPSHOT

   { 0x8038020C, 0x0001,   7609,   5556,   6070,   6584,   7098,   7861,   5813,   6327,   6841,   7355 },// IC fund + harmonics MAX
   { 0x8038020C, 0x0002,   7610,   5557,   6071,   6585,   7099,   7862,   5814,   6328,   6842,   7356 },// IC fund + harmonics MIN
   { 0x8038020C, 0x0003,   7611,   5558,   6072,   6586,   7100,   7863,   5815,   6329,   6843,   7357 },// IC fund + harmonics SNAPSHOT

   { 0x8068020C, 0x0001,   7612,   5559,   6073,   6587,   7101,   7864,   5816,   6330,   6844,   7358 },// IA fund only MAX
   { 0x8068020C, 0x0002,   7613,   5560,   6074,   6588,   7102,   7865,   5817,   6331,   6845,   7359 },// IA fund only MIN
   { 0x8068020C, 0x0003,   7614,   5561,   6075,   6589,   7103,   7866,   5818,   6332,   6846,   7360 },// IA fund only SNAPSHOT

   { 0x8070020C, 0x0001,   7615,   5562,   6076,   6590,   7104,   7867,   5819,   6333,   6847,   7361 },// IB fund only MAX
   { 0x8070020C, 0x0002,   7616,   5563,   6077,   6591,   7105,   7868,   5820,   6334,   6848,   7362 },// IB fund only MIN
   { 0x8070020C, 0x0003,   7617,   5564,   6078,   6592,   7106,   7869,   5821,   6335,   6849,   7363 },// IB fund only SNAPSHOT

   { 0x8078020C, 0x0001,   7618,   5565,   6079,   6593,   7107,   7870,   5822,   6336,   6850,   7364 },// IC fund only MAX
   { 0x8078020C, 0x0002,   7619,   5566,   6080,   6594,   7108,   7871,   5823,   6337,   6851,   7365 },// IC fund only MIN
   { 0x8078020C, 0x0003,   7620,   5567,   6081,   6595,   7109,   7872,   5824,   6338,   6852,   7366 },// IC fund only SNAPSHOT

   { 0x802BC218, 0x0001,  12505,  11584,  11692,  11800,  11908,  12560,  11638,  11746,  11854,  11962 },// Distortion PF, Element A MAX
   { 0x802BC218, 0x0002,  12506,  11585,  11693,  11801,  11909,  12561,  11639,  11747,  11855,  11963 },// Distortion PF, Element A
   { 0x802BC218, 0x0003,  12507,  11586,  11694,  11802,  11910,  12562,  11640,  11748,  11856,  11964 },// Distortion PF, Element A

   { 0x8033C218, 0x0001,  12508,  11587,  11695,  11803,  11911,  12563,  11641,  11749,  11857,  11965 },// Distortion PF, Element B MAX
   { 0x8033C218, 0x0002,  12509,  11588,  11696,  11804,  11912,  12564,  11642,  11750,  11858,  11966 },// Distortion PF, Element B
   { 0x8033C218, 0x0003,  12510,  11589,  11697,  11805,  11913,  12565,  11643,  11751,  11859,  11967 },// Distortion PF, Element B

   { 0x803BC218, 0x0001,  12511,  11590,  11698,  11806,  11914,  12566,  11644,  11752,  11860,  11968 },// Distortion PF, Element C MAX
   { 0x803BC218, 0x0002,  12512,  11591,  11699,  11807,  11915,  12567,  11645,  11753,  11861,  11969 },// Distortion PF, Element C
   { 0x803BC218, 0x0003,  12513,  11592,  11700,  11808,  11916,  12568,  11646,  11754,  11862,  11970 },// Distortion PF, Element C

   { 0x8003C218, 0x0001,  12514,  11593,  11701,  11809,  11917,  12569,  11647,  11755,  11863,  11971 },// Distortion PF MAX
   { 0x8003C218, 0x0002,  12515,  11594,  11702,  11810,  11918,  12570,  11648,  11756,  11864,  11972 },// Distortion PF MIN
   { 0x8003C218, 0x0003,  12516,  11595,  11703,  11811,  11919,  12571,  11649,  11757,  11865,  11973 },// Distortion PF SNAPSHOT

   { 0x80280210, 0x0001,  12517,  11596,  11704,  11812,  11920,  12572,  11650,  11758,  11866,  11974 },// VTHD, Element A MAX
   { 0x80280210, 0x0002,  12518,  11597,  11705,  11813,  11921,  12573,  11651,  11759,  11867,  11975 },// VTHD, Element A MIN
   { 0x80280210, 0x0003,  12519,  11598,  11706,  11814,  11922,  12574,  11652,  11760,  11868,  11976 },// VTHD, Element A SNAPSHOT

   { 0x80300210, 0x0001,  12520,  11599,  11707,  11815,  11923,  12575,  11653,  11761,  11869,  11977 },// VTHD, Element B MAX
   { 0x80300210, 0x0002,  12521,  11600,  11708,  11816,  11924,  12576,  11654,  11762,  11870,  11978 },// VTHD, Element B MIN
   { 0x80300210, 0x0003,  12522,  11601,  11709,  11817,  11925,  12577,  11655,  11763,  11871,  11979 },// VTHD, Element B SNAPSHOT

   { 0x80380210, 0x0001,  12523,  11602,  11710,  11818,  11926,  12578,  11656,  11764,  11872,  11980 },// VTHD, Element C MAX
   { 0x80380210, 0x0002,  12524,  11603,  11711,  11819,  11927,  12579,  11657,  11765,  11873,  11981 },// VTHD, Element C MIN
   { 0x80380210, 0x0003,  12525,  11604,  11712,  11820,  11928,  12580,  11658,  11766,  11874,  11982 },// VTHD, Element C SNAPSHOT

   { 0x80280211, 0x0001,  12526,  11605,  11713,  11821,  11929,  12581,  11659,  11767,  11875,  11983 },// ITHD, Element A MAX
   { 0x80280211, 0x0002,  12527,  11606,  11714,  11822,  11930,  12582,  11660,  11768,  11876,  11984 },// ITHD, Element A MIN
   { 0x80280211, 0x0003,  12528,  11607,  11715,  11823,  11931,  12583,  11661,  11769,  11877,  11985 },// ITHD, Element A SNAPSHOT

   { 0x80300211, 0x0001,  12529,  11608,  11716,  11824,  11932,  12584,  11662,  11770,  11878,  11986 },// ITHD, Element B MAX
   { 0x80300211, 0x0002,  12530,  11609,  11717,  11825,  11933,  12585,  11663,  11771,  11879,  11987 },// ITHD, Element B MIN
   { 0x80300211, 0x0003,  12531,  11610,  11718,  11826,  11934,  12586,  11664,  11772,  11880,  11988 },// ITHD, Element B SNAPSHOT

   { 0x80380211, 0x0001,  12532,  11611,  11719,  11827,  11935,  12587,  11665,  11773,  11881,  11989 },// ITHD, Element C MAX
   { 0x80380211, 0x0002,  12533,  11612,  11720,  11828,  11936,  12588,  11666,  11774,  11882,  11990 },// ITHD, Element C MIN
   { 0x80380211, 0x0003,  12534,  11613,  11721,  11829,  11937,  12589,  11667,  11775,  11883,  11991 },// ITHD, Element C SNAPSHOT

   { 0x80280211, 0x0001,  12526,  11605,  11713,  11821,  11929,  12581,  11659,  11767,  11875,  11983 },// ITHD, Element A MAX
   { 0x80280211, 0x0002,  12527,  11606,  11714,  11822,  11930,  12582,  11660,  11768,  11876,  11984 },// ITHD, Element A MIN
   { 0x80280211, 0x0003,  12528,  11607,  11715,  11823,  11931,  12583,  11661,  11769,  11877,  11985 },// ITHD, Element A SNAPSHOT

   { 0x80300211, 0x0001,  12529,  11608,  11716,  11824,  11932,  12584,  11662,  11770,  11878,  11986 },// ITHD, Element B MAX
   { 0x80300211, 0x0002,  12530,  11609,  11717,  11825,  11933,  12585,  11663,  11771,  11879,  11987 },// ITHD, Element B MIN
   { 0x80300211, 0x0003,  12531,  11610,  11718,  11826,  11934,  12586,  11664,  11772,  11880,  11988 },// ITHD, Element B SNAPSHOT

   { 0x80380211, 0x0001,  12532,  11611,  11719,  11827,  11935,  12587,  11665,  11773,  11881,  11989 },// ITHD, Element C MAX
   { 0x80380211, 0x0002,  12533,  11612,  11720,  11828,  11936,  12588,  11666,  11774,  11882,  11990 },// ITHD, Element C MIN
   { 0x80380211, 0x0003,  12534,  11613,  11721,  11829,  11937,  12589,  11667,  11775,  11883,  11991 },// ITHD, Element C SNAPSHOT

   { 0x0003C404, 0x0000,   7629,   5576,   6090,   6604,   7118,   7881,   5833,   6347,   6861,   7375 },// Q
   { 0x80000422, 0x0000,   7630,   5577,   6091,   6605,   7119,   7882,   5834,   6348,   6862,   7376 },// Count (Pulses - demand)
   { 0x80000422, 0x0001,   7631,   5578,   6092,   6606,   7120,   7883,   5835,   6349,   6863,   7377 },// Count (Pulses - demand)
   { 0x80000422, 0x0002,   7632,   5579,   6093,   6607,   7121,   7884,   5836,   6350,   6864,   7378 },// Count (Pulses - demand)
   { 0x80000422, 0x0003,   7633,   5580,   6094,   6608,   7122,   7885,   5837,   6351,   6865,   7379 } // Count (Pulses - demand)
};


 // lookup table for cumulative demand measurements
static const uomLookupEntry_t masterCumulativeDemandLookup [] =
{ //   ST12      ST14      pres   presA   presB   presC   presD    prev   prevA   prevB   prevC   prevD
   { 0x00284400, 0x0000,   7886,   8286,   8686,   9086,   9486,   8086,   8486,   8886,   9286,   9686 },// W Element A Q1 fund + harmonics
   { 0x00288400, 0x0000,   7887,   8287,   8687,   9087,   9487,   8087,   8487,   8887,   9287,   9687 },// W Element A Q2 fund + harmonics
   { 0x00290400, 0x0000,   7888,   8288,   8688,   9088,   9488,   8088,   8488,   8888,   9288,   9688 },// W Element A Q3 fund + harmonics
   { 0x002A0400, 0x0000,   7889,   8289,   8689,   9089,   9489,   8089,   8489,   8889,   9289,   9689 },// W Element A Q4 fund + harmonics
   { 0x002A4400, 0x0000,   7890,   8290,   8690,   9090,   9490,   8090,   8490,   8890,   9290,   9690 },// W Element A Q1+Q4 fund + harmonics
   { 0x00298400, 0x0000,   7891,   8291,   8691,   9091,   9491,   8091,   8491,   8891,   9291,   9691 },// W Element A Q2+Q3 fund + harmonics
   { 0x002BC400, 0x0000,   7892,   8292,   8692,   9092,   9492,   8092,   8492,   8892,   9292,   9692 },// W Element A (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x002FC400, 0x0000,   7893,   8293,   8693,   9093,   9493,   8093,   8493,   8893,   9293,   9693 },// W Element A (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00304400, 0x0000,   7894,   8294,   8694,   9094,   9494,   8094,   8494,   8894,   9294,   9694 },// W Element B Q1 fund + harmonics
   { 0x00308400, 0x0000,   7895,   8295,   8695,   9095,   9495,   8095,   8495,   8895,   9295,   9695 },// W Element B Q2 fund + harmonics
   { 0x00310400, 0x0000,   7896,   8296,   8696,   9096,   9496,   8096,   8496,   8896,   9296,   9696 },// W Element B Q3 fund + harmonics
   { 0x00320400, 0x0000,   7897,   8297,   8697,   9097,   9497,   8097,   8497,   8897,   9297,   9697 },// W Element B Q4 fund + harmonics
   { 0x00324400, 0x0000,   7898,   8298,   8698,   9098,   9498,   8098,   8498,   8898,   9298,   9698 },// W Element B Q1+Q4 fund + harmonics
   { 0x00318400, 0x0000,   7899,   8299,   8699,   9099,   9499,   8099,   8499,   8899,   9299,   9699 },// W Element B Q2+Q3 fund + harmonics
   { 0x0033C400, 0x0000,   7900,   8300,   8700,   9100,   9500,   8100,   8500,   8900,   9300,   9700 },// W Element B (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x0037C400, 0x0000,   7901,   8301,   8701,   9101,   9501,   8101,   8501,   8901,   9301,   9701 },// W Element B (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00384400, 0x0000,   7902,   8302,   8702,   9102,   9502,   8102,   8502,   8902,   9302,   9702 },// W Element C Q1 fund + harmonics
   { 0x00388400, 0x0000,   7903,   8303,   8703,   9103,   9503,   8103,   8503,   8903,   9303,   9703 },// W Element C Q2 fund + harmonics
   { 0x00390400, 0x0000,   7904,   8304,   8704,   9104,   9504,   8104,   8504,   8904,   9304,   9704 },// W Element C Q3 fund + harmonics
   { 0x003A0400, 0x0000,   7905,   8305,   8705,   9105,   9505,   8105,   8505,   8905,   9305,   9705 },// W Element C Q4 fund + harmonics
   { 0x003A4400, 0x0000,   7906,   8306,   8706,   9106,   9506,   8106,   8506,   8906,   9306,   9706 },// W Element C Q1+Q4 fund + harmonics
   { 0x00398400, 0x0000,   7907,   8307,   8707,   9107,   9507,   8107,   8507,   8907,   9307,   9707 },// W Element C Q2+Q3 fund + harmonics
   { 0x003BC400, 0x0000,   7908,   8308,   8708,   9108,   9508,   8108,   8508,   8908,   9308,   9708 },// W Element C (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x003FC400, 0x0000,   7909,   8309,   8709,   9109,   9509,   8109,   8509,   8909,   9309,   9709 },// W Element C (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00684400, 0x0000,   7910,   8310,   8710,   9110,   9510,   8110,   8510,   8910,   9310,   9710 },// W Element A Q1 fund only
   { 0x00688400, 0x0000,   7911,   8311,   8711,   9111,   9511,   8111,   8511,   8911,   9311,   9711 },// W Element A Q2 fund only
   { 0x00690400, 0x0000,   7912,   8312,   8712,   9112,   9512,   8112,   8512,   8912,   9312,   9712 },// W Element A Q3 fund only
   { 0x006A0400, 0x0000,   7913,   8313,   8713,   9113,   9513,   8113,   8513,   8913,   9313,   9713 },// W Element A Q4 fund only
   { 0x006A4400, 0x0000,   7914,   8314,   8714,   9114,   9514,   8114,   8514,   8914,   9314,   9714 },// W Element A Q1+Q4 fund only
   { 0x00698400, 0x0000,   7915,   8315,   8715,   9115,   9515,   8115,   8515,   8915,   9315,   9715 },// W Element A Q2+Q3 fund only
   { 0x006BC400, 0x0000,   7916,   8316,   8716,   9116,   9516,   8116,   8516,   8916,   9316,   9716 },// W Element A (Q1+Q4)+(Q2+Q3) fund only
   { 0x006FC400, 0x0000,   7917,   8317,   8717,   9117,   9517,   8117,   8517,   8917,   9317,   9717 },// W Element A (Q1+Q4)-(Q2+Q3) fund only
   { 0x00704400, 0x0000,   7918,   8318,   8718,   9118,   9518,   8118,   8518,   8918,   9318,   9718 },// W Element B Q1 fund only
   { 0x00708400, 0x0000,   7919,   8319,   8719,   9119,   9519,   8119,   8519,   8919,   9319,   9719 },// W Element B Q2 fund only
   { 0x00710400, 0x0000,   7920,   8320,   8720,   9120,   9520,   8120,   8520,   8920,   9320,   9720 },// W Element B Q3 fund only
   { 0x00720400, 0x0000,   7921,   8321,   8721,   9121,   9521,   8121,   8521,   8921,   9321,   9721 },// W Element B Q4 fund only
   { 0x00724400, 0x0000,   7922,   8322,   8722,   9122,   9522,   8122,   8522,   8922,   9322,   9722 },// W Element B Q1+Q4 fund only
   { 0x00718400, 0x0000,   7923,   8323,   8723,   9123,   9523,   8123,   8523,   8923,   9323,   9723 },// W Element B Q2+Q3 fund only
   { 0x0073C400, 0x0000,   7924,   8324,   8724,   9124,   9524,   8124,   8524,   8924,   9324,   9724 },// W Element B (Q1+Q4)+(Q2+Q3) fund only
   { 0x0077C400, 0x0000,   7925,   8325,   8725,   9125,   9525,   8125,   8525,   8925,   9325,   9725 },// W Element B (Q1+Q4)-(Q2+Q3) fund only
   { 0x00784400, 0x0000,   7926,   8326,   8726,   9126,   9526,   8126,   8526,   8926,   9326,   9726 },// W Element C Q1 fund only
   { 0x00788400, 0x0000,   7927,   8327,   8727,   9127,   9527,   8127,   8527,   8927,   9327,   9727 },// W Element C Q2 fund only
   { 0x00790400, 0x0000,   7928,   8328,   8728,   9128,   9528,   8128,   8528,   8928,   9328,   9728 },// W Element C Q3 fund only
   { 0x007A0400, 0x0000,   7929,   8329,   8729,   9129,   9529,   8129,   8529,   8929,   9329,   9729 },// W Element C Q4 fund only
   { 0x007A4400, 0x0000,   7930,   8330,   8730,   9130,   9530,   8130,   8530,   8930,   9330,   9730 },// W Element C Q1+Q4 fund only
   { 0x00798400, 0x0000,   7931,   8331,   8731,   9131,   9531,   8131,   8531,   8931,   9331,   9731 },// W Element C Q2+Q3 fund only
   { 0x007BC400, 0x0000,   7932,   8332,   8732,   9132,   9532,   8132,   8532,   8932,   9332,   9732 },// W Element C (Q1+Q4)+(Q2+Q3) fund only
   { 0x007FC400, 0x0000,   7933,   8333,   8733,   9133,   9533,   8133,   8533,   8933,   9333,   9733 },// W Element C (Q1+Q4)-(Q2+Q3) fund only
   { 0x00004400, 0x0000,   7934,   8334,   8734,   9134,   9534,   8134,   8534,   8934,   9334,   9734 },// W Q1 fund + harmonics
   { 0x00008400, 0x0000,   7935,   8335,   8735,   9135,   9535,   8135,   8535,   8935,   9335,   9735 },// W Q2 fund + harmonics
   { 0x00010400, 0x0000,   7936,   8336,   8736,   9136,   9536,   8136,   8536,   8936,   9336,   9736 },// W Q3 fund + harmonics
   { 0x00020400, 0x0000,   7937,   8337,   8737,   9137,   9537,   8137,   8537,   8937,   9337,   9737 },// W Q4 fund + harmonics
   { 0x00024400, 0x0000,    468,    477,    486,    495,    504,    513,    522,    531,    540,    549 },// W Q1+Q4 fund + harmonics
   { 0x00018400, 0x0000,    469,    478,    487,    496,    505,    514,    523,    532,    541,    550 },// W Q2+Q3 fund + harmonics
   { 0x0003C400, 0x0000,    470,    479,    488,    497,    506,    515,    524,    533,    542,    551 },// W (Q1+Q4)+(Q2+Q3) fund + harmonics
   { 0x0007C400, 0x0000,    471,    480,    489,    498,    507,    516,    525,    534,    543,    552 },// W (Q1+Q4)-(Q2+Q3) fund + harmonics
   { 0x00404400, 0x0000,   7938,   8338,   8738,   9138,   9538,   8138,   8538,   8938,   9338,   9738 },// W Q1 fund only
   { 0x00408400, 0x0000,   7939,   8339,   8739,   9139,   9539,   8139,   8539,   8939,   9339,   9739 },// W Q2 fund only
   { 0x00410400, 0x0000,   7940,   8340,   8740,   9140,   9540,   8140,   8540,   8940,   9340,   9740 },// W Q3 fund only
   { 0x00420400, 0x0000,   7941,   8341,   8741,   9141,   9541,   8141,   8541,   8941,   9341,   9741 },// W Q4 fund only
   { 0x00424400, 0x0000,   7942,   8342,   8742,   9142,   9542,   8142,   8542,   8942,   9342,   9742 },// W Q1+Q4 fund only
   { 0x00418400, 0x0000,   7943,   8343,   8743,   9143,   9543,   8143,   8543,   8943,   9343,   9743 },// W Q2+Q3 fund only
   { 0x0043C400, 0x0000,   7944,   8344,   8744,   9144,   9544,   8144,   8544,   8944,   9344,   9744 },// W (Q1+Q4)+(Q2+Q3) fund only
   { 0x0047C400, 0x0000,   7945,   8345,   8745,   9145,   9545,   8145,   8545,   8945,   9345,   9745 },// W (Q1+Q4)-(Q2+Q3) fund only

   { 0x00284401, 0x0000,   7946,   8346,   8746,   9146,   9546,   8146,   8546,   8946,   9346,   9746 },// var Element A Q1 fund + harmonics
   { 0x00288401, 0x0000,   7947,   8347,   8747,   9147,   9547,   8147,   8547,   8947,   9347,   9747 },// var Element A Q2 fund + harmonics
   { 0x00290401, 0x0000,   7948,   8348,   8748,   9148,   9548,   8148,   8548,   8948,   9348,   9748 },// var Element A Q3 fund + harmonics
   { 0x002A0401, 0x0000,   7949,   8349,   8749,   9149,   9549,   8149,   8549,   8949,   9349,   9749 },// var Element A Q4 fund + harmonics
   { 0x0028C401, 0x0000,   7950,   8350,   8750,   9150,   9550,   8150,   8550,   8950,   9350,   9750 },// var Element A Q1+Q2 fund + harmonics
   { 0x002B0401, 0x0000,   7951,   8351,   8751,   9151,   9551,   8151,   8551,   8951,   9351,   9751 },// var Element A Q3+Q4 fund + harmonics
   { 0x002BC401, 0x0000,   7952,   8352,   8752,   9152,   9552,   8152,   8552,   8952,   9352,   9752 },// var Element A (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x002FC401, 0x0000,   7953,   8353,   8753,   9153,   9553,   8153,   8553,   8953,   9353,   9753 },// var Element A (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00304401, 0x0000,   7954,   8354,   8754,   9154,   9554,   8154,   8554,   8954,   9354,   9754 },// var Element B Q1 fund + harmonics
   { 0x00308401, 0x0000,   7955,   8355,   8755,   9155,   9555,   8155,   8555,   8955,   9355,   9755 },// var Element B Q2 fund + harmonics
   { 0x00310401, 0x0000,   7956,   8356,   8756,   9156,   9556,   8156,   8556,   8956,   9356,   9756 },// var Element B Q3 fund + harmonics
   { 0x00320401, 0x0000,   7957,   8357,   8757,   9157,   9557,   8157,   8557,   8957,   9357,   9757 },// var Element B Q4 fund + harmonics
   { 0x0030C401, 0x0000,   7958,   8358,   8758,   9158,   9558,   8158,   8558,   8958,   9358,   9758 },// var Element B Q1+Q2 fund + harmonics
   { 0x00330401, 0x0000,   7959,   8359,   8759,   9159,   9559,   8159,   8559,   8959,   9359,   9759 },// var Element B Q3+Q4 fund + harmonics
   { 0x0033C401, 0x0000,   7960,   8360,   8760,   9160,   9560,   8160,   8560,   8960,   9360,   9760 },// var Element B (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x0037C401, 0x0000,   7961,   8361,   8761,   9161,   9561,   8161,   8561,   8961,   9361,   9761 },// var Element B (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00384401, 0x0000,   7962,   8362,   8762,   9162,   9562,   8162,   8562,   8962,   9362,   9762 },// var Element C Q1 fund + harmonics
   { 0x00388401, 0x0000,   7963,   8363,   8763,   9163,   9563,   8163,   8563,   8963,   9363,   9763 },// var Element C Q2 fund + harmonics
   { 0x00390401, 0x0000,   7964,   8364,   8764,   9164,   9564,   8164,   8564,   8964,   9364,   9764 },// var Element C Q3 fund + harmonics
   { 0x003A0401, 0x0000,   7965,   8365,   8765,   9165,   9565,   8165,   8565,   8965,   9365,   9765 },// var Element C Q4 fund + harmonics
   { 0x0038C401, 0x0000,   7966,   8366,   8766,   9166,   9566,   8166,   8566,   8966,   9366,   9766 },// var Element C Q1+Q2 fund + harmonics
   { 0x003B0401, 0x0000,   7967,   8367,   8767,   9167,   9567,   8167,   8567,   8967,   9367,   9767 },// var Element C Q3+Q4 fund + harmonics
   { 0x003BC401, 0x0000,   7968,   8368,   8768,   9168,   9568,   8168,   8568,   8968,   9368,   9768 },// var Element C (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x003FC401, 0x0000,   7969,   8369,   8769,   9169,   9569,   8169,   8569,   8969,   9369,   9769 },// var Element C (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00684401, 0x0000,   7970,   8370,   8770,   9170,   9570,   8170,   8570,   8970,   9370,   9770 },// var Element A Q1 fund only
   { 0x00688401, 0x0000,   7971,   8371,   8771,   9171,   9571,   8171,   8571,   8971,   9371,   9771 },// var Element A Q2 fund only
   { 0x00690401, 0x0000,   7972,   8372,   8772,   9172,   9572,   8172,   8572,   8972,   9372,   9772 },// var Element A Q3 fund only
   { 0x006A0401, 0x0000,   7973,   8373,   8773,   9173,   9573,   8173,   8573,   8973,   9373,   9773 },// var Element A Q4 fund only
   { 0x0068C401, 0x0000,   7974,   8374,   8774,   9174,   9574,   8174,   8574,   8974,   9374,   9774 },// var Element A Q1+Q2 fund only
   { 0x006B0401, 0x0000,   7975,   8375,   8775,   9175,   9575,   8175,   8575,   8975,   9375,   9775 },// var Element A Q3+Q4 fund only
   { 0x006BC401, 0x0000,   7976,   8376,   8776,   9176,   9576,   8176,   8576,   8976,   9376,   9776 },// var Element A (Q1+Q2)+(Q3+Q4) fund only
   { 0x006FC401, 0x0000,   7977,   8377,   8777,   9177,   9577,   8177,   8577,   8977,   9377,   9777 },// var Element A (Q1+Q2)-(Q3+Q4) fund only
   { 0x00704401, 0x0000,   7978,   8378,   8778,   9178,   9578,   8178,   8578,   8978,   9378,   9778 },// var Element B Q1 fund only
   { 0x00708401, 0x0000,   7979,   8379,   8779,   9179,   9579,   8179,   8579,   8979,   9379,   9779 },// var Element B Q2 fund only
   { 0x00710401, 0x0000,   7980,   8380,   8780,   9180,   9580,   8180,   8580,   8980,   9380,   9780 },// var Element B Q3 fund only
   { 0x00720401, 0x0000,   7981,   8381,   8781,   9181,   9581,   8181,   8581,   8981,   9381,   9781 },// var Element B Q4 fund only
   { 0x0070C401, 0x0000,   7982,   8382,   8782,   9182,   9582,   8182,   8582,   8982,   9382,   9782 },// Var Element B Q1+Q2 fund only
   { 0x00730401, 0x0000,   7983,   8383,   8783,   9183,   9583,   8183,   8583,   8983,   9383,   9783 },// Var Element B Q3+Q4 fund only
   { 0x0073C401, 0x0000,   7984,   8384,   8784,   9184,   9584,   8184,   8584,   8984,   9384,   9784 },// Var Element B (Q1+Q2)+(Q3+Q4) fund only
   { 0x0077C401, 0x0000,   7985,   8385,   8785,   9185,   9585,   8185,   8585,   8985,   9385,   9785 },// var Element B (Q1+Q2)-(Q3+Q4) fund only
   { 0x00784401, 0x0000,   7986,   8386,   8786,   9186,   9586,   8186,   8586,   8986,   9386,   9786 },// var Element C Q1 fund only
   { 0x00788401, 0x0000,   7987,   8387,   8787,   9187,   9587,   8187,   8587,   8987,   9387,   9787 },// var Element C Q2 fund only
   { 0x00790401, 0x0000,   7988,   8388,   8788,   9188,   9588,   8188,   8588,   8988,   9388,   9788 },// var Element C Q3 fund only
   { 0x007A0401, 0x0000,   7989,   8389,   8789,   9189,   9589,   8189,   8589,   8989,   9389,   9789 },// var Element C Q4 fund only
   { 0x0078C401, 0x0000,   7990,   8390,   8790,   9190,   9590,   8190,   8590,   8990,   9390,   9790 },// var Element C Q1+Q2 fund only
   { 0x007B0401, 0x0000,   7991,   8391,   8791,   9191,   9591,   8191,   8591,   8991,   9391,   9791 },// var Element C Q3+Q4 fund only
   { 0x007BC401, 0x0000,   7992,   8392,   8792,   9192,   9592,   8192,   8592,   8992,   9392,   9792 },// var Element C (Q1+Q2)+(Q3+Q4) fund only
   { 0x007FC401, 0x0000,   7993,   8393,   8793,   9193,   9593,   8193,   8593,   8993,   9393,   9793 },// var Element C (Q1+Q2)-(Q3+Q4) fund only
   { 0x00004401, 0x0000,   7994,   8394,   8794,   9194,   9594,   8194,   8594,   8994,   9394,   9794 },// var Q1 fund + harmonics
   { 0x00008401, 0x0000,   7995,   8395,   8795,   9195,   9595,   8195,   8595,   8995,   9395,   9795 },// var Q2 fund + harmonics
   { 0x00010401, 0x0000,   7996,   8396,   8796,   9196,   9596,   8196,   8596,   8996,   9396,   9796 },// var Q3 fund + harmonics
   { 0x00020401, 0x0000,   7997,   8397,   8797,   9197,   9597,   8197,   8597,   8997,   9397,   9797 },// var Q4 fund + harmonics
   { 0x0000C401, 0x0000,    472,    481,    490,    499,    508,    517,    526,    535,    544,    553 },// var Q1+Q2 fund + harmonics
   { 0x00030401, 0x0000,    473,    482,    491,    500,    509,    518,    527,    536,    545,    554 },// var Q3+Q4 fund + harmonics
   { 0x0003C401, 0x0000,    474,    483,    492,    501,    510,    519,    528,    537,    546,    555 },// var (Q1+Q2)+(Q3+Q4) fund + harmonics
   { 0x0007C401, 0x0000,    475,    484,    493,    502,    511,    520,    529,    538,    547,    556 },// var (Q1+Q2)-(Q3+Q4) fund + harmonics
   { 0x00404401, 0x0000,   7998,   8398,   8798,   9198,   9598,   8198,   8598,   8998,   9398,   9798 },// var Q1 fund only
   { 0x00408401, 0x0000,   7999,   8399,   8799,   9199,   9599,   8199,   8599,   8999,   9399,   9799 },// var Q2 fund only
   { 0x00410401, 0x0000,   8000,   8400,   8800,   9200,   9600,   8200,   8600,   9000,   9400,   9800 },// var Q3 fund only
   { 0x00420401, 0x0000,   8001,   8401,   8801,   9201,   9601,   8201,   8601,   9001,   9401,   9801 },// var Q4 fund only
   { 0x0040C401, 0x0000,   8002,   8402,   8802,   9202,   9602,   8202,   8602,   9002,   9402,   9802 },// var Q1+Q2 fund only
   { 0x00430401, 0x0000,   8003,   8403,   8803,   9203,   9603,   8203,   8603,   9003,   9403,   9803 },// var Q3+Q4 fund only
   { 0x0043C401, 0x0000,   8004,   8404,   8804,   9204,   9604,   8204,   8604,   9004,   9404,   9804 },// var (Q1+Q2)+(Q3+Q4) fund only
   { 0x0047C401, 0x0000,   8005,   8405,   8805,   9205,   9605,   8205,   8605,   9005,   9405,   9805 },// var (Q1+Q2)-(Q3+Q4) fund only

   { 0x00284402, 0x0000,   8009,   8409,   8809,   9209,   9609,   8209,   8609,   9009,   9409,   9809 },// Apparent VA Element A Q1
   { 0x00288402, 0x0000,   8010,   8410,   8810,   9210,   9610,   8210,   8610,   9010,   9410,   9810 },// Apparent VA Element A Q2
   { 0x00290402, 0x0000,   8011,   8411,   8811,   9211,   9611,   8211,   8611,   9011,   9411,   9811 },// Apparent VA Element A Q3
   { 0x002A0402, 0x0000,   8012,   8412,   8812,   9212,   9612,   8212,   8612,   9012,   9412,   9812 },// Apparent VA Element A Q4
   { 0x00304402, 0x0000,   8013,   8413,   8813,   9213,   9613,   8213,   8613,   9013,   9413,   9813 },// Apparent VA Element B Q1
   { 0x00308402, 0x0000,   8014,   8414,   8814,   9214,   9614,   8214,   8614,   9014,   9414,   9814 },// Apparent VA Element B Q2
   { 0x00310402, 0x0000,   8015,   8415,   8815,   9215,   9615,   8215,   8615,   9015,   9415,   9815 },// Apparent VA Element B Q3
   { 0x00320402, 0x0000,   8016,   8416,   8816,   9216,   9616,   8216,   8616,   9016,   9416,   9816 },// Apparent VA Element B Q4
   { 0x00384402, 0x0000,   8017,   8417,   8817,   9217,   9617,   8217,   8617,   9017,   9417,   9817 },// Apparent VA Element C Q1
   { 0x00388402, 0x0000,   8018,   8418,   8818,   9218,   9618,   8218,   8618,   9018,   9418,   9818 },// Apparent VA Element C Q2
   { 0x00390402, 0x0000,   8019,   8419,   8819,   9219,   9619,   8219,   8619,   9019,   9419,   9819 },// Apparent VA Element C Q3
   { 0x003A0402, 0x0000,   8020,   8420,   8820,   9220,   9620,   8220,   8620,   9020,   9420,   9820 },// Apparent VA Element C Q4

   { 0x00004402, 0x0000,   8021,   8421,   8821,   9221,   9621,   8221,   8621,   9021,   9421,   9821 },// Apparent VA Q1
   { 0x80004402, 0x0001,   8065,   8465,   8865,   9265,   9665,   8265,   8665,   9065,   9465,   9865 },// Arithmetic Apparent VA Q1
   { 0x80004402, 0x0002,  14865,  14993,  15121,  15249,  15377,  14913,  15041,  15169,  15297,  15425 },// Distortion VA Q1
   { 0x80004402, 0x0100,  14866,  14994,  15122,  15250,  15378,  14914,  15042,  15170,  15298,  15426 },// Apparent () Q1
   { 0x80004402, 0x0101,   8066,   8466,   8866,   9266,   9666,   8266,   8666,   9066,   9466,   9866 },// Arithmetic Apparent () Q1

   { 0x00008402, 0x0000,   8022,   8422,   8822,   9222,   9622,   8222,   8622,   9022,   9422,   9822 },// Apparent VA Q2
   { 0x80008402, 0x0001,   8067,   8467,   8867,   9267,   9667,   8267,   8667,   9067,   9467,   9867 },// Arithmetic Apparent VA Q2
   { 0x80008402, 0x0002,  14867,  14995,  15123,  15251,  15379,  14915,  15043,  15171,  15299,  15427 },// Distortion VA Q2
   { 0x80008402, 0x0100,  14868,  14996,  15124,  15252,  15380,  14916,  15044,  15172,  15300,  15428 },// Apparent () Q2
   { 0x80008402, 0x0101,   8068,   8468,   8868,   9268,   9668,   8268,   8668,   9068,   9468,   9868 },// Arithmetic Apparent () Q2

   { 0x00010402, 0x0000,   8023,   8423,   8823,   9223,   9623,   8223,   8623,   9023,   9423,   9823 },// Apparent VA Q3
   { 0x80010402, 0x0001,   8069,   8469,   8869,   9269,   9669,   8269,   8669,   9069,   9469,   9869 },// Arithmetic Apparent VA Q3
   { 0x80010402, 0x0002,  14869,  14997,  15125,  15253,  15381,  14917,  15045,  15173,  15301,  15429 },// Distortion VA Q3
   { 0x80010402, 0x0100,  14870,  14998,  15126,  15254,  15382,  14918,  15046,  15174,  15302,  15430 },// Apparent () Q3
   { 0x80010402, 0x0101,   8070,   8470,   8870,   9270,   9670,   8270,   8670,   9070,   9470,   9870 },// Arithmetic Apparent () Q3

   { 0x00020402, 0x0000,   8024,   8424,   8824,   9224,   9624,   8224,   8624,   9024,   9424,   9824 },// Apparent VA Q4
   { 0x80020402, 0x0001,   8071,   8471,   8871,   9271,   9671,   8271,   8671,   9071,   9471,   9871 },// Arithmetic Apparent VA Q4
   { 0x80020402, 0x0002,  14871,  14999,  15127,  15255,  15383,  14919,  15047,  15175,  15303,  15431 },// Distortion VA Q4
   { 0x80020402, 0x0100,  14872,  15000,  15128,  15256,  15384,  14920,  15048,  15176,  15304,  15432 },// Apparent () Q4
   { 0x80020402, 0x0101,   8072,   8472,   8872,   9272,   9672,   8272,   8672,   9072,   9472,   9872 },// Arithmetic Apparent VA Q4

   { 0x80024402, 0x0000,   8025,   8425,   8825,   9225,   9625,   8225,   8625,   9025,   9425,   9825 },// Apparent VA Q1+Q4
   { 0x80024402, 0x0001,  14793,  14921,  15049,  15177,  15305,  14873,  15001,  15129,  15257,  15385 },// Arithmetic Apparent VA Q1+Q4
   { 0x80024402, 0x0002,  14794,  14922,  15050,  15178,  15306,  14874,  15002,  15130,  15258,  15386},//  Distortion VA Q1+Q4
   { 0x80024402, 0x0100,   8026,   8426,   8826,   9226,   9626,   8226,   8626,   9026,   9426,   9826 },// Apparent () Q1+Q4
   { 0x80024402, 0x0101,  14795,  14923,  15051,  15179,  15307,  14875,  15003,  15131,  15259,  15387},// Arithmetic Apparent () Q1+Q4

   { 0x80018402, 0x0000,   8027,   8427,   8827,   9227,   9627,   8227,   8627,   9027,   9427,   9827 },// Apparent VA Q2+Q3
   { 0x80018402, 0x0001,   14796, 14924,  15052,  15180,  15308,  14876,  15004,  15132,  15260,  15388 },//  Arithmetic Apparent VA Q2+Q3
   { 0x80018402, 0x0002,   14797, 14925,  15053,  15181,  15309,  14877,  15005,  15133,  15261,  15389 },// Distortion VA Q2+Q3
   { 0x80018402, 0x0100,   8028,   8428,   8828,   9228,   9628,   8228,   8628,   9028,   9428,   9828 },// Apparent () Q2+Q3
   { 0x80018402, 0x0101,   14798, 14926,  15054,  15182,  15310,  14878,  15006,  15134,  15262,  15390 },// Arithmetic Apparent () Q2+Q3

   { 0x0003C402, 0x0000,    476,    485,    494,    503,    512,    521,    530,    539,    548,    557 },// Apparent VA
   { 0x8003C402, 0x0000,    476,    485,    494,    503,    512,    521,    530,    539,    548,    557 },// Apparent VA (Q1+Q4)+(Q2+Q3)
   { 0x8003C402, 0x0001,   8063,   8463,   8863,   9263,   9663,   8263,   8663,   9063,   9463,   9863 },// Arithmetic Apparent VA
   { 0x8003C402, 0x0002,   8079,   8479,   8879,   9279,   9679,   8279,   8679,   9079,   9479,   9879 },// Distortion VA
   { 0x8003C402, 0x0100,   8029,   8429,   8829,   9229,   9629,   8229,   8629,   9029,   9429,   9829 },// Apparent () (Q1+Q4)+(Q2+Q3)
   { 0x8003C402, 0x0101,   8064,   8464,   8864,   9264,   9664,   8264,   8664,   9064,   9464,   9864 },// Arithmetic Apparent ()
   { 0x8003C402, 0x0102,   8080,   8480,   8880,   9280,   9680,   8280,   8680,   9080,   9480,   9880 },// Distortion ()

   { 0x8007C402, 0x0000,   8030,   8430,   8830,   9230,   9630,   8230,   8630,   9030,   9430,   9830 },// Apparent VA (Q1+Q4)-(Q2+Q3)
   { 0x8007C402, 0x0001,  14799,  14927,  15055,  15183,  15311,  14879,  15007,  15135,  15263,  15391 },// Arithmetic Apparent VA (Q1+Q4)-(Q2+Q3)
   { 0x8007C402, 0x0002,  14800,  14928,  15056,  15184,  15312,  14880,  15008,  15136,  15264,  15392 },// Distortion VA (Q1+Q4)-(Q2+Q3)
   { 0x8007C402, 0x0100,   8031,   8431,   8831,   9231,   9631,   8231,   8631,   9031,   9431,   9831 },// Apparent () (Q1+Q4)-(Q2+Q3)
   { 0x8007C402, 0x0101,  14801,  14929,  15057,  15185,  15313,  14881,  15009,  15137,  15265,  15393 },// Arithmetic Apparent () (Q1+Q4)-(Q2+Q3)

   { 0x802A4402, 0x0000,   8032,   8432,   8832,   9232,   9632,   8232,   8632,   9032,   9432,   9832 },// Apparent VA Element A Q1+Q4
   { 0x802A4402, 0x0001,  14802,  14930,  15058,  15186,  15314,  14882,  15010,  15138,  15266,  15394 },// Arithmetic Apparent VA Element A Q1+Q4
   { 0x802A4402, 0x0002,  14803,  14931,  15059,  15187,  15315,  14883,  15011,  15139,  15267,  15395 },// Distortion VA Element A Q1+Q4
   { 0x802A4402, 0x0100,   8033,   8433,   8833,   9233,   9633,   8233,   8633,   9033,   9433,   9833 },// Apparent () Element A Q1+Q4
   { 0x802A4402, 0x0101,  14804,  14932,  15060,  15188,  15316,  14884,  15012,  15140,  15268,  15396 },// Arithmetic Apparent () Element A Q1+Q4

   { 0x80298402, 0x0000,   8034,   8434,   8834,   9234,   9634,   8234,   8634,   9034,   9434,   9834 },// Apparent VA Element A Q2+Q3
   { 0x80298402, 0x0001,  14805,  14933,  15061,  15189,  15317,  14885,  15013,  15141,  15269,  15397 },// Arithmetic Apparent VA Element A Q2+Q3
   { 0x80298402, 0x0002,  14806,  14934,  15062,  15190,  15318,  14886,  15014,  15142,  15270,  15398 },// Distortion VA Element A Q2+Q3
   { 0x80298402, 0x0100,   8035,   8435,   8835,   9235,   9635,   8235,   8635,   9035,   9435,   9835 },// Apparent () Element A Q2+Q3
   { 0x80298402, 0x0101,  14807,  14935,  15063,  15191,  15319,  14887,  15015,  15143,  15271,  15399 },// Arithmetic Apparent () Element A Q2+Q3

   { 0x002BC402, 0x0000,   8006,   8406,   8806,   9206,   9606,   8206,   8606,   9006,   9406,   9806 },// Apparent VA Element A
   { 0x802BC402, 0x0000,   8006,   8406,   8806,   9206,   9606,   8206,   8606,   9006,   9406,   9806 },// Apparent VA Element A (Q1+Q4)+(Q2+Q3)
   { 0x802BC402, 0x0001,  14808,  14936,  15064,  15192,  15320,  14888,  15016,  15144,  15272,  15400 },// Arithmetic Apparent VA Element A (Q1+Q4)+(Q2+Q3)
   { 0x802BC402, 0x0002,   8073,   8473,   8873,   9273,   9673,   8273,   8673,   9073,   9473,   9873 },// Distortion VA, Element A
   { 0x802BC402, 0x0100,   8036,   8436,   8836,   9236,   9636,   8236,   8636,   9036,   9436,   9836 },// Apparent VA Element A (Q1+Q4)+(Q2+Q3)
   { 0x802BC402, 0x0101,  14809,  14937,  15065,  15193,  15321,  14889,  15017,  15145,  15273,  15401 },// Arithmetic Apparent VA Element A (Q1+Q4)+(Q2+Q3)
   { 0x802BC402, 0x0102,   8074,   8474,   8874,   9274,   9674,   8274,   8674,   9074,   9474,   9874 },// Distortion VA, Element A

   { 0x802FC402, 0x0000,   8037,   8437,   8837,   9237,   9637,   8237,   8637,   9037,   9437,   9837 },// Apparent VA Element A (Q1+Q4)-(Q2+Q3)
   { 0x802FC402, 0x0001,  14810,  14938,  15066,  15194,  15322,  14890,  15018,  15146,  15274,  15402 },// Arithmetic Apparent VA Element A (Q1+Q4)-(Q2+Q3)
   { 0x802FC402, 0x0002,  14811,  14939,  15067,  15195,  15323,  14891,  15019,  15147,  15275,  15403 },// Distortion VA Element A (Q1+Q4)-(Q2+Q3)
   { 0x802FC402, 0x0100,   8038,   8438,   8838,   9238,   9638,   8238,   8638,   9038,   9438,   9838 },// Apparent () Element A (Q1+Q4)-(Q2+Q3)
   { 0x802FC402, 0x0101,  14812,  14940,  15068,  15196,  15324,  14892,  15020,  15148,  15276,  15404 },// Arithmetic Apparent () Element A (Q1+Q4)-(Q2+Q3)

   { 0x80324402, 0x0000,   8039,   8439,   8839,   9239,   9639,   8239,   8639,   9039,   9439,   9839 },// Apparent VA Element B Q1+Q4
   { 0x80324402, 0x0001,  14813,  14941,  15069,  15197,  15325,  14893,  15021,  15149,  15277,  15405 },// Arithmetic Apparent VA Element B Q1+Q4
   { 0x80324402, 0x0002,  14814,  14942,  15070,  15198,  15326,  14894,  15022,  15150,  15278,  15406 },// Distortion VA Element B Q1+Q4
   { 0x80324402, 0x0100,   8040,   8440,   8840,   9240,   9640,   8240,   8640,   9040,   9440,   9840 },// Apparent () Element B Q1+Q4
   { 0x80324402, 0x0101,  14815,  14943,  15071,  15199,  15327,  14895,  15023,  15151,  15279,  15407 },// Arithmetic Apparent () Element B Q1+Q4

   { 0x80318402, 0x0000,   8041,   8441,   8841,   9241,   9641,   8241,   8641,   9041,   9441,   9841 },// Apparent VA Element B Q2+Q3
   { 0x80318402, 0x0001,  14816,  14944,  15072,  15200,  15328,  14896,  15024,  15152,  15280,  15408 },// Arithmetic Apparent VA Element B Q2+Q3
   { 0x80318402, 0x0002,  14817,  14945,  15073,  15201,  15329,  15735,  15737,  15739,  15741,  15743 },// Distortion VA Element B Q2+Q3
   { 0x80318402, 0x0100,   8042,   8442,   8842,   9242,   9642,   8242,   8642,   9042,   9442,   9842 },// Apparent () Element B Q2+Q3
   { 0x80318402, 0x0101,  14818,  14946,  15074,  15202,  15330,  14897,  15025,  15153,  15281,  15409 },// Arithmetic Apparent () Element B Q2+Q3

   { 0x0033C402, 0x0000,   8007,   8407,   8807,   9207,   9607,   8207,   8607,   9007,   9407,   9807 },// Apparent VA Element B
   { 0x8033C402, 0x0000,   8007,   8407,   8807,   9207,   9607,   8207,   8607,   9007,   9407,   9807 },// Apparent VA Element B (Q1+Q4)+(Q2+Q3)
   { 0x8033C402, 0x0001,  14819,  14947,  15075,  15203,  15331,  14898,  15026,  15154,  15282,  15410 },// Arithmetic Apparent VA Element B (Q1+Q4)+(Q2+Q3)
   { 0x8033C402, 0x0002,   8075,   8475,   8875,   9275,   9675,   8275,   8675,   9075,   9475,   9875 },// Distortion VA Element B (Q1+Q4)+(Q2+Q3)
   { 0x8033C402, 0x0100,   8043,   8443,   8843,   9243,   9643,   8243,   8643,   9043,   9443,   9843 },// Apparent () Element B (Q1+Q4)+(Q2+Q3)
   { 0x8033C402, 0x0101,  14820,  14948,  15076,  15204,  15332,  14899,  15027,  15155,  15283,  15411 },// Arithmetic Apparent () Element B (Q1+Q4)+(Q2+Q3)

   { 0x8037C402, 0x0000,   8044,   8444,   8844,   9244,   9644,   8244,   8644,   9044,   9444,   9844 },// Apparent VA Element B (Q1+Q4)-(Q2+Q3)
   { 0x8037C402, 0x0001,  14821,  14949,  15077,  15205,  15333,  14900,  15028,  15156,  15284,  15412 },// Arithmetic Apparent VA Element B (Q1+Q4)-(Q2+Q3)
   { 0x8037C402, 0x0002,  14822,  14950,  15078,  15206,  15334,  14901,  15029,  15157,  15285,  15413 },// Distortion VA Element B (Q1+Q4)-(Q2+Q3)
   { 0x8037C402, 0x0100,   8045,   8445,   8845,   9245,   9645,   8245,   8645,   9045,   9445,   9845 },// Apparent () Element B (Q1+Q4)-(Q2+Q3)
   { 0x8037C402, 0x0101,  14823,  14951,  15079,  15207,  15335,  15736,  15738,  15740,  15742,  15744 },// Arithmetic Apparent () Element B (Q1+Q4)-(Q2+Q3)

   { 0x803A4402, 0x0000,   8046,   8446,   8846,   9246,   9646,   8246,   8646,   9046,   9446,   9846 },// Apparent VA Element C Q1+Q4
   { 0x803A4402, 0x0001,  14824,  14952,  15080,  15208,  15336,  14902,  15030,  15158,  15286,  15414 },// Arithmetic Apparent VA Element C Q1+Q4
   { 0x803A4402, 0x0002,  14825,  14953,  15081,  15209,  15337,  14903,  15031,  15159,  15287,  15415 },// Distortion VA Element C Q1+Q4
   { 0x803A4402, 0x0100,   8047,   8447,   8847,   9247,   9647,   8247,   8647,   9047,   9447,   9847 },// Apparent () Element C Q1+Q4
   { 0x803A4402, 0x0101,  14826,  14954,  15082,  15210,  15338,  14904,  15032,  15160,  15288,  15416 },// Arithmetic Apparent () Element C Q1+Q4

   { 0x80398402, 0x0000,   8048,   8448,   8848,   9248,   9648,   8248,   8648,   9048,   9448,   9848 },// Apparent VA Element C Q2+Q3
   { 0x80398402, 0x0001,  14827,  14955,  15083,  15211,  15339,  14905,  15033,  15161,  15289,  15417 },// Arithmetic Apparent VA Element C Q2+Q3
   { 0x80398402, 0x0002,  14828,  14956,  15084,  15212,  15340,  14906,  15034,  15162,  15290,  15418 },// Distortion VA Element C Q2+Q3
   { 0x80398402, 0x0100,   8049,   8449,   8849,   9249,   9649,   8249,   8649,   9049,   9449,   9849 },// Apparent VA Element C Q2+Q3
   { 0x80398402, 0x0101,  14829,  14957,  15085,  15213,  15341,  14907,  15035,  15163,  15291,  15419 },// Arithmetic Apparent VA Element C Q2+Q3

   { 0x003BC402, 0x0000,   8008,   8408,   8808,   9208,   9608,   8208,   8608,   9008,   9408,   9808 },// Apparent VA Element C
   { 0x803BC402, 0x0000,   8008,   8408,   8808,   9208,   9608,   8208,   8608,   9008,   9408,   9808 },// Apparent VA Element C (Q1+Q4)+(Q2+Q3)
   { 0x803BC402, 0x0001,  14830,  14958,  15086,  15214,  15342,  14908,  15036,  15164,  15292,  15420 },// Arithmetic Apparent VA Element C (Q1+Q4)+(Q2+Q3)
   { 0x803BC402, 0x0002,   8077,   8477,   8877,   9277,   9677,   8277,   8677,   9077,   9477,   9877 },// Distortion VA Element C (Q1+Q4)+(Q2+Q3)
   { 0x803BC402, 0x0100,   8050,   8450,   8850,   9250,   9650,   8250,   8650,   9050,   9450,   9850 },// Apparent () Element C (Q1+Q4)+(Q2+Q3)
   { 0x803BC402, 0x0101,  14831,  14959,  15087,  15215,  15343,  14909,  15037,  15165,  15293,  15421 },// Arithmetic Apparent () Element C (Q1+Q4)+(Q2+Q3)

   { 0x803FC402, 0x0000,   8051,   8451,   8851,   9251,   9651,   8251,   8651,   9051,   9451,   9851 },// Apparent VA Element C (Q1+Q4)-(Q2+Q3)
   { 0x803FC402, 0x0001,  14832,  14960,  15088,  15216,  15344,  14910,  15038,  15166,  15294,  15422 },// Arithmetic Apparent VA Element C (Q1+Q4)-(Q2+Q3)
   { 0x803FC402, 0x0002,  14833,  14961,  15089,  15217,  15345,  14911,  15039,  15167,  15295,  15423 },// Distortion VA Element C (Q1+Q4)-(Q2+Q3)
   { 0x803FC402, 0x0100,   8052,   8452,   8852,   9252,   9652,   8252,   8652,   9052,   9452,   9852 },// Apparent () Element C (Q1+Q4)-(Q2+Q3)
   { 0x803FC402, 0x0101,  14834,  14962,  15090,  15218,  15346,  14912,  15040,  15168,  15296,  15424 },// Arithmetic Apparent () Element C (Q1+Q4)-(Q2+Q3)

   { 0x8003C403, 0x0000,   8053,   8453,   8853,   9253,   9653,   8253,   8653,   9053,   9453,   9853 },// Phasor Apparent VA, fund + harmonics
   { 0x8003C403, 0x0001,  14835,  14963,  15091,  15219,  15347,  15583,  15613,  15643,  15673,  15703 },// Phasor Apparent Fuzzy VA, fund + harmonics
   { 0x8003C403, 0x0100,  14845,  14973,  15101,  15229,  15357,  15593,  15623,  15653,  15683,  15713 },// Phasor Apparent (), fund + harmonics
   { 0x8003C403, 0x0101,  14855,  14983,  15111,  15239,  15367,  15603,  15633,  15663,  15693,  15723 },// Phasor Apparent Fuzzy (), fund + harmonics

   { 0x8043C403, 0x0000,   8054,   8454,   8854,   9254,   9654,   8254,   8654,   9054,   9454,   9854 },// Phasor Apparent VA, fund only
   { 0x8043C403, 0x0001,  14836,  14964,  15092,  15220,  15348,  15584,  15614,  15644,  15674,  15704 },// Phasor Apparent Fuzzy VA, fund only
   { 0x8043C403, 0x0100,  14846,  14974,  15102,  15230,  15358,  15594,  15624,  15654,  15684,  15714 },// Phasor Apparent (), fund only
   { 0x8043C403, 0x0101,  14856,  14984,  15112,  15240,  15368,  15604,  15634,  15664,  15694,  15724 },// Phasor Apparent Fuzzy (), fund only

   { 0x80004403, 0x0000,   8055,   8455,   8855,   9255,   9655,   8255,   8655,   9055,   9455,   9855 },// Phasor Apparent VA Q1 fund+harmonics
   { 0x80004403, 0x0001,  14837,  14965,  15093,  15221,  15349,  15585,  15615,  15645,  15675,  15705 },// Phasor Apparent Fuzzy VA Q1 fund+harmonics
   { 0x80004403, 0x0100,  14847,  14975,  15103,  15231,  15359,  15595,  15625,  15655,  15685,  15715 },// Phasor Apparent () Q1 fund+harmonics
   { 0x80004403, 0x0101,  14857,  14985,  15113,  15241,  15369,  15605,  15635,  15665,  15695,  15725 },// Phasor Apparent Fuzzy () Q1 fund+harmonics

   { 0x80008403, 0x0000,   8056,   8456,   8856,   9256,   9656,   8256,   8656,   9056,   9456,   9856 },// Phasor Apparent VA Q2 fund+harmonics
   { 0x80008403, 0x0001,  14838,  14966,  15094,  15222,  15350,  15586,  15616,  15646,  15676,  15706 },// Phasor Apparent Fuzzy VA Q2 fund+harmonics
   { 0x80008403, 0x0100,  14848,  14976,  15104,  15232,  15360,  15596,  15626,  15656,  15686,  15716 },// Phasor Apparent () Q2 fund+harmonics
   { 0x80008403, 0x0101,  14858,  14986,  15114,  15242,  15370,  15606,  15636,  15666,  15696,  15726 },// Phasor Apparent Fuzzy () Q2 fund+harmonics

   { 0x80010403, 0x0000,   8057,   8457,   8857,   9257,   9657,   8257,   8657,   9057,   9457,   9857 },// Phasor Apparent VA Q3 fund+harmonics
   { 0x80010403, 0x0001,  14839,  14967,  15095,  15223,  15351,  15587,  15617,  15647,  15677,  15707 },// Phasor Apparent Fuzzy VA Q3 fund+harmonics
   { 0x80010403, 0x0100,  14849,  14977,  15105,  15233,  15361,  15597,  15627,  15657,  15687,  15717 },// Phasor Apparent () Q3 fund+harmonics
   { 0x80010403, 0x0101,  14859,  14987,  15115,  15243,  15371,  15607,  15637,  15667,  15697,  15727 },// Phasor Apparent Fuzzy () Q3 fund+harmonics

   { 0x80020403, 0x0000,   8058,   8458,   8858,   9258,   9658,   8258,   8658,   9058,   9458,   9858 },// Phasor Apparent VA Q4 fund+harmonics
   { 0x80020403, 0x0001,  14840,  14968,  15096,  15224,  15352,  15588,  15618,  15648,  15678,  15708 },// Phasor Apparent Fuzzy VA Q4 fund+harmonics
   { 0x80020403, 0x0100,  14850,  14978,  15106,  15234,  15362,  15598,  15628,  15658,  15688,  15718 },// Phasor Apparent () Q4 fund+harmonics
   { 0x80020403, 0x0101,  14860,  14988,  15116,  15244,  15372,  15608,  15638,  15668,  15698,  15728 },// Phasor Apparent Fuzzy () Q4 fund+harmonics

   { 0x80404403, 0x0000,   8059,   8459,   8859,   9259,   9659,   8259,   8659,   9059,   9459,   9859 },// Phasor Apparent VA Q1 fund only
   { 0x80404403, 0x0001,  14841,  14969,  15097,  15225,  15353,  15589,  15619,  15649,  15679,  15709 },// Phasor Apparent Fuzzy VA Q1 fund only
   { 0x80404403, 0x0100,  14851,  14979,  15107,  15235,  15363,  15599,  15629,  15659,  15689,  15719 },// Phasor Apparent () Q1 fund only
   { 0x80404403, 0x0101,  14861,  14989,  15117,  15245,  15373,  15609,  15639,  15669,  15699,  15729 },// Phasor Apparent Fuzzy () Q1 fund only

   { 0x80408403, 0x0000,   8060,   8460,   8860,   9260,   9660,   8260,   8660,   9060,   9460,   9860 },// Phasor Apparent VA Q2 fund only
   { 0x80408403, 0x0001,  14842,  14970,  15098,  15226,  15354,  15590,  15620,  15650,  15680,  15710 },// Phasor Apparent FuzzyVA Q2 fund only
   { 0x80408403, 0x0100,  14852,  14980,  15108,  15236,  15364,  15600,  15630,  15660,  15690,  15720 },// Phasor Apparent () Q2 fund only
   { 0x80408403, 0x0101,  14862,  14990,  15118,  15246,  15374,  15610,  15640,  15670,  15700,  15730 },// Phasor Apparent Fuzzy () Q2 fund only

   { 0x80410403, 0x0000,   8061,   8461,   8861,   9261,   9661,   8261,   8661,   9061,   9461,   9861 },// Phasor Apparent VA Q3 fund only
   { 0x80410403, 0x0001,  14843,  14971,  15099,  15227,  15355,  15591,  15621,  15651,  15681,  15711 },// Phasor Apparent Fuzzy VA Q3 fund only
   { 0x80410403, 0x0100,  14853,  14981,  15109,  15237,  15365,  15601,  15631,  15661,  15691,  15721 },// Phasor Apparent () Q3 fund only
   { 0x80410403, 0x0101,  14863,  14991,  15119,  15247,  15375,  15611,  15641,  15671,  15701,  15731 },// Phasor Apparent Fuzzy () Q3 fund only

   { 0x80420403, 0x0000,   8062,   8462,   8862,   9262,   9662,   8262,   8662,   9062,   9462,   9862 },// Phasor Apparent VA Q4 fund only
   { 0x80420403, 0x0001,  14844,  14972,  15100,  15228,  15356,  15592,  15622,  15652,  15682,  15712 },// Phasor Apparent Fuzzy VA Q4 fund only
   { 0x80420403, 0x0100,  14854,  14982,  15110,  15238,  15366,  15602,  15632,  15662,  15692,  15722 },// Phasor Apparent () Q4 fund only
   { 0x80420403, 0x0101,  14864,  14992,  15120,  15248,  15376,  15612,  15642,  15672,  15702,  15732 },// Phasor Apparent Fuzzy () Q4 fund only

   { 0x0058040A, 0x0000,  12009,  12105,  12201,  12297,  12393,  12057,  12153,  12249,  12345,  12441 },// V2AC, fund
   { 0x0050040A, 0x0000,  12010,  12106,  12202,  12298,  12394,  12058,  12154,  12250,  12346,  12442 },// V2CB, fund
   { 0x0048040A, 0x0000,  12011,  12107,  12203,  12299,  12395,  12059,  12155,  12251,  12347,  12443 },// V2BA, fund
   { 0x0018040A, 0x0000,  12012,  12108,  12204,  12300,  12396,  12060,  12156,  12252,  12348,  12444 },// V2AC, fund+harmonics
   { 0x0010040A, 0x0000,  12013,  12109,  12205,  12301,  12397,  12061,  12157,  12253,  12349,  12445 },// V2CB, fund+harmonics
   { 0x0008040A, 0x0000,  12014,  12110,  12206,  12302,  12398,  12062,  12158,  12254,  12350,  12446 },// V2BA, fund+harmonics
   { 0x0068040A, 0x0000,  12015,  12111,  12207,  12303,  12399,  12063,  12159,  12255,  12351,  12447 },// V2 Element A, fund+harmonics
   { 0x0070040A, 0x0000,  12016,  12112,  12208,  12304,  12400,  12064,  12160,  12256,  12352,  12448 },// V2 Element B, fund+harmonics
   { 0x0078040A, 0x0000,  12017,  12113,  12209,  12305,  12401,  12065,  12161,  12257,  12353,  12449 },// V2 Element C, fund+harmonics
   { 0x80280208, 0x0001,  12018,  12114,  12210,  12306,  12402,  12066,  12162,  12258,  12354,  12450 },// VA fund + harmonics
   { 0x80300208, 0x0001,  12019,  12115,  12211,  12307,  12403,  12067,  12163,  12259,  12355,  12451 },// VB fund + harmonics
   { 0x80380208, 0x0001,  12020,  12116,  12212,  12308,  12404,  12068,  12164,  12260,  12356,  12452 },// VC fund + harmonics
   { 0x80680208, 0x0001,  12021,  12117,  12213,  12309,  12405,  12069,  12165,  12261,  12357,  12453 },// VA fund only
   { 0x80700208, 0x0001,  12022,  12118,  12214,  12310,  12406,  12070,  12166,  12262,  12358,  12454 },// VB fund only
   { 0x80780208, 0x0001,  12023,  12119,  12215,  12311,  12407,  12071,  12167,  12263,  12359,  12455 },// VC fund only
   { 0x80180208, 0x0001,  12024,  12120,  12216,  12312,  12408,  12072,  12168,  12264,  12360,  12456 },// VAC fund + harmonics
   { 0x80100208, 0x0001,  12025,  12121,  12217,  12313,  12409,  12073,  12169,  12265,  12361,  12457 },// VCB fund + harmonics
   { 0x80080208, 0x0001,  12026,  12122,  12218,  12314,  12410,  12074,  12170,  12266,  12362,  12458 },// VBA fund + harmonics
   { 0x80580208, 0x0001,  12027,  12123,  12219,  12315,  12411,  12075,  12171,  12267,  12363,  12459 },// VAC fund only
   { 0x80500208, 0x0001,  12028,  12124,  12220,  12316,  12412,  12076,  12172,  12268,  12364,  12460 },// VCB fund only
   { 0x80480208, 0x0001,  12029,  12125,  12221,  12317,  12413,  12077,  12173,  12269,  12365,  12461 },// VBA fund only
   { 0x0028040E, 0x0000,  12030,  12126,  12222,  12318,  12414,  12078,  12174,  12270,  12366,  12462 },// I2, Element A, fund + harmonics
   { 0x0030040E, 0x0000,  12031,  12127,  12223,  12319,  12415,  12079,  12175,  12271,  12367,  12463 },// I2, Element B, fund + harmonics
   { 0x0038040E, 0x0000,  12032,  12128,  12224,  12320,  12416,  12080,  12176,  12272,  12368,  12464 },// I2, Element C, fund + harmonics
   { 0x0068040E, 0x0000,  12033,  12129,  12225,  12321,  12417,  12081,  12177,  12273,  12369,  12465 },// I2, Element A, fund
   { 0x0070040E, 0x0000,  12034,  12130,  12226,  12322,  12418,  12082,  12178,  12274,  12370,  12466 },// I2, Element B, fund
   { 0x0078040E, 0x0000,  12035,  12131,  12227,  12323,  12419,  12083,  12179,  12275,  12371,  12467 },// I2, Element C, fund
   { 0x0020040E, 0x0000,  12036,  12132,  12228,  12324,  12420,  12084,  12180,  12276,  12372,  12468 },// I2, Element N
   { 0x8020020C, 0x0001,  12037,  12133,  12229,  12325,  12421,  12085,  12181,  12277,  12373,  12469 },// In fund + harmonics
   { 0x8028020C, 0x0001,  12038,  12134,  12230,  12326,  12422,  12086,  12182,  12278,  12374,  12470 },// IA fund + harmonics
   { 0x8030020C, 0x0001,  12039,  12135,  12231,  12327,  12423,  12087,  12183,  12279,  12375,  12471 },// IB fund + harmonics
   { 0x8038020C, 0x0001,  12040,  12136,  12232,  12328,  12424,  12088,  12184,  12280,  12376,  12472 },// IC fund + harmonics
   { 0x8068020C, 0x0001,  12041,  12137,  12233,  12329,  12425,  12089,  12185,  12281,  12377,  12473 },// IA fund only
   { 0x8070020C, 0x0001,  12042,  12138,  12234,  12330,  12426,  12090,  12186,  12282,  12378,  12474 },// IB fund only
   { 0x8078020C, 0x0001,  12043,  12139,  12235,  12331,  12427,  12091,  12187,  12283,  12379,  12475 },// IC fund only

   { 0x8033C402, 0x0002,   8075,   8475,   8875,   9275,   9675,   8275,   8675,   9075,   9475,   9875 },// Distortion VA, Element B
   { 0x8033C402, 0x0102,   8076,   8476,   8876,   9276,   9676,   8276,   8676,   9076,   9476,   9876 },// Distortion VA, Element B
   { 0x803BC402, 0x0002,   8077,   8477,   8877,   9277,   9677,   8277,   8677,   9077,   9477,   9877 },// Distortion VA, Element C
   { 0x803BC402, 0x0102,   8078,   8478,   8878,   9278,   9678,   8278,   8678,   9078,   9478,   9878 },// Distortion VA, Element C

   { 0x802BC218, 0x0001,  12044,  12140,  12236,  12332,  12428,  12092,  12188,  12284,  12380,  12476 },// Distortion PF, Element A
   { 0x8033C218, 0x0001,  12045,  12141,  12237,  12333,  12429,  12093,  12189,  12285,  12381,  12477 },// Distortion PF, Element B
   { 0x803BC218, 0x0001,  12046,  12142,  12238,  12334,  12430,  12094,  12190,  12286,  12382,  12478 },// Distortion PF, Element C
   { 0x8003C218, 0x0001,  12047,  12143,  12239,  12335,  12431,  12095,  12191,  12287,  12383,  12479 },// Distortion PF

   { 0x80280210, 0x0001,  12048,  12144,  12240,  12336,  12432,  12096,  12192,  12288,  12384,  12480 },// VTHD, Element A
   { 0x80300210, 0x0001,  12049,  12145,  12241,  12337,  12433,  12097,  12193,  12289,  12385,  12481 },// VTHD, Element B
   { 0x80380210, 0x0001,  12050,  12146,  12242,  12338,  12434,  12098,  12194,  12290,  12386,  12482 },// VTHD, Element C
   { 0x80280211, 0x0001,  12051,  12147,  12243,  12339,  12435,  12099,  12195,  12291,  12387,  12483 },// ITHD, Element A
   { 0x80300211, 0x0001,  12052,  12148,  12244,  12340,  12436,  12100,  12196,  12292,  12388,  12484 },// ITHD, Element B
   { 0x80380211, 0x0001,  12053,  12149,  12245,  12341,  12437,  12101,  12197,  12293,  12389,  12485 },// ITHD, Element C
   { 0x80280211, 0x0001,  12051,  12147,  12243,  12339,  12435,  12099,  12195,  12291,  12387,  12483 },// ITHD, Element A
   { 0x80300211, 0x0001,  12052,  12148,  12244,  12340,  12436,  12100,  12196,  12292,  12388,  12484 },// ITHD, Element B
   { 0x80380211, 0x0001,  12053,  12149,  12245,  12341,  12437,  12101,  12197,  12293,  12389,  12485 },// ITHD, Element C
   { 0x0003C404, 0x0000,   8081,   8481,   8881,   9281,   9681,   8281,   8681,   9081,   9481,   9881 },// Q
   { 0x80000422, 0x0000,   8082,   8482,   8882,   9282,   9682,   8282,   8682,   9082,   9482,   9882 },// Count (Pulses - demand)
   { 0x80000422, 0x0001,   8083,   8483,   8883,   9283,   9683,   8283,   8683,   9083,   9483,   9883 },// Count (Pulses - demand)
   { 0x80000422, 0x0002,   8084,   8484,   8884,   9284,   9684,   8284,   8684,   9084,   9484,   9884 },// Count (Pulses - demand)
   { 0x80000422, 0x0003,   8085,   8485,   8885,   9285,   9685,   8285,   8685,   9085,   9485,   9885 } // Count (Pulses - demand)
};


/* Lookup table for coincident PF measurements.  ST12 and ST14 data will not be used when evaluating which
   row to access in the table.  Instead, strictly the index (60 or 61) will be used.  These values are supplied
   in the table to maintain UOM lookup interface design. */
static const uomLookupEntry_t masterCoincidentPfLookup [] =
{ //   ST12      ST14      pres  presA  presB  presC  presD       prev  prevA  prevB  prevC  prevD
  { 0x8003C419,	0x003C, {   711,   712,   713,   714,   715 }, {   722,   723,   724,   725,   726 } }, // Coincident PF #1
  { 0x8003C419,	0x003D, {   716,   717,   718,   719,   720 }, {   727,   728,   729,   730,   731 } }, // Coincident PF #2
};


static const uomLpLookupEntry_t uomLoadProfileLookupTable [] =
{ //   ST12      ST14     1Min    5Min    15Min    30Min    60Min

   // All Phases Total KWH
   { 0x00004500, 0x0000,    0,     311,     315,     319,     323 },// Wh Q1 fund + harmonics (LP)
   { 0x00404500, 0x0000,    0,    2296,    2295,    2294,    2293 },// Wh Q1 fund only (LP)
   { 0x00008500, 0x0000,    0,     312,     316,     320,     324 },// Wh Q2 fund + harmonics (LP)
   { 0x00408500, 0x0000,    0,    2301,    2300,    2299,    2298 },// Wh Q2 fund only (LP)
   { 0x00010500, 0x0000,    0,     313,     317,     321,     325 },// Wh Q3 fund + harmonics (LP)
   { 0x00410500, 0x0000,    0,    2306,    2305,    2304,    2303 },// Wh Q3 fund only (LP)
   { 0x00020500, 0x0000,    0,     314,     318,     322,     326 },// Wh Q4 fund + harmonics (LP)
   { 0x00420500, 0x0000,    0,    2311,    2310,    2309,    2308 },// Wh Q4 fund only (LP)
   { 0x00024500, 0x0000,    0,      73,      63,     119,     110 },// Wh Q1+Q4 fund + harmonics (LP)
   { 0x00424500, 0x0000,    0,      74,      65,     121,     112 },// Wh Q1+Q4 fund only (LP)
   { 0x00018500, 0x0000,    0,      75,      66,     122,     113 },// Wh Q2+Q3 fund + harmonics (LP)
   { 0x00418500, 0x0000,    0,    2317,    2316,    2315,    2314 },// Wh Q2+Q3 fund only (LP)
   { 0x0003C500, 0x0000,    0,      78,      69,     125,     116 },// Wh (Q1+Q4)+(Q2+Q3) fund + harmonics (LP)
   { 0x0043C500, 0x0000,    0,    2322,    2321,    2320,    2319 },// Wh (Q1+Q4)+(Q2+Q3) fund only (LP)
   { 0x0007C500, 0x0000,    0,      79,      70,     126,     117 },// Wh (Q1+Q4)-(Q2+Q3) fund + harmonics (LP)
   { 0x0047C500, 0x0000,    0,    2327,    2326,    2325,    2324 },// Wh (Q1+Q4)-(Q2+Q3) fund only (LP)
   { 0x00284500, 0x0000,    0,    2052,    2051,    2050,    2049 },// Wh Element A Q1 fund + harmonics (LP)
   { 0x00288500, 0x0000,    0,    2057,    2056,    2055,    2054 },// Wh Element A Q2 fund + harmonics (LP)
   { 0x00290500, 0x0000,    0,    2062,    2061,    2060,    2059 },// Wh Element A Q3 fund + harmonics (LP)
   { 0x002A0500, 0x0000,    0,    2067,    2066,    2065,    2064 },// Wh Element A Q4 fund + harmonics (LP)
   { 0x002A4500, 0x0000,    0,    2072,    2071,    2070,    2069 },// Wh Element A Q1+Q4 fund + harmonics (LP)
   { 0x00298500, 0x0000,    0,    2077,    2076,    2075,    2074 },// Wh Element A Q2+Q3 fund + harmonics (LP)
   { 0x002BC500, 0x0000,    0,    2082,    2081,    2080,    2079 },// Wh Element A (Q1+Q4)+(Q2+Q3) fund + harmonics (LP)
   { 0x002FC500, 0x0000,    0,    2087,    2086,    2085,    2084 },// Wh Element A (Q1+Q4)-(Q2+Q3) fund + harmonics (LP)
   { 0x00304500, 0x0000,    0,    2092,    2091,    2090,    2089 },// Wh Element B Q1 fund + harmonics (LP)
   { 0x00308500, 0x0000,    0,    2097,    2096,    2095,    2094 },// Wh Element B Q2 fund + harmonics (LP)
   { 0x00310500, 0x0000,    0,    2102,    2101,    2100,    2099 },// Wh Element B Q3 fund + harmonics (LP)
   { 0x00320500, 0x0000,    0,    2107,    2106,    2105,    2104 },// Wh Element B Q4 fund + harmonics (LP)
   { 0x00324500, 0x0000,    0,    2112,    2111,    2110,    2109 },// Wh Element B Q1+Q4 fund + harmonics (LP)
   { 0x00318500, 0x0000,    0,    2117,    2116,    2115,    2114 },// Wh Element B Q2+Q3 fund + harmonics (LP)
   { 0x0033C500, 0x0000,    0,    2122,    2121,    2120,    2119 },// Wh Element B (Q1+Q4)+(Q2+Q3) fund + harmonics (LP)
   { 0x0037C500, 0x0000,    0,    2127,    2126,    2125,    2124 },// Wh Element B (Q1+Q4)-(Q2+Q3) fund + harmonics (LP)
   { 0x00384500, 0x0000,    0,    2132,    2131,    2130,    2129 },// Wh Element C Q1 fund + harmonics (LP)
   { 0x00388500, 0x0000,    0,    2137,    2136,    2135,    2134 },// Wh Element C Q2 fund + harmonics (LP)
   { 0x00390500, 0x0000,    0,    2142,    2141,    2140,    2139 },// Wh Element C Q3 fund + harmonics (LP)
   { 0x003A0500, 0x0000,    0,    2147,    2146,    2145,    2144 },// Wh Element C Q4 fund + harmonics (LP)
   { 0x003A4500, 0x0000,    0,    2152,    2151,    2150,    2149 },// Wh Element C Q1+Q4 fund + harmonics (LP)
   { 0x00398500, 0x0000,    0,    2157,    2156,    2155,    2154 },// Wh Element C Q2+Q3 fund + harmonics (LP)
   { 0x003BC500, 0x0000,    0,    2162,    2161,    2160,    2159 },// Wh Element C (Q1+Q4)+(Q2+Q3) fund + harmonics (LP)
   { 0x003FC500, 0x0000,    0,    2167,    2166,    2165,    2164 },// Wh Element C (Q1+Q4)-(Q2+Q3) fund + harmonics (LP)
   { 0x00684500, 0x0000,    0,    2172,    2171,    2170,    2169 },// Wh Element A Q1 fund only (LP)
   { 0x00688500, 0x0000,    0,    2177,    2176,    2175,    2174 },// Wh Element A Q2 fund only (LP)
   { 0x00690500, 0x0000,    0,    2182,    2181,    2180,    2179 },// Wh Element A Q3 fund only (LP)
   { 0x006A0500, 0x0000,    0,    2187,    2186,    2185,    2184 },// Wh Element A Q4 fund only (LP)
   { 0x006A4500, 0x0000,    0,    2192,    2191,    2190,    2189 },// Wh Element A Q1+Q4 fund only (LP)
   { 0x00698500, 0x0000,    0,    2197,    2196,    2195,    2194 },// Wh Element A Q2+Q3 fund only (LP)
   { 0x006BC500, 0x0000,    0,    2202,    2201,    2200,    2199 },// Wh Element A (Q1+Q4)+(Q2+Q3) fund only (LP)
   { 0x006FC500, 0x0000,    0,    2207,    2206,    2205,    2204 },// Wh Element A (Q1+Q4)-(Q2+Q3) fund only (LP)
   { 0x00704500, 0x0000,    0,    2212,    2211,    2210,    2209 },// Wh Element B Q1 fund only (LP)
   { 0x00708500, 0x0000,    0,    2217,    2216,    2215,    2214 },// Wh Element B Q2 fund only (LP)
   { 0x00710500, 0x0000,    0,    2222,    2221,    2220,    2219 },// Wh Element B Q3 fund only (LP)
   { 0x00720500, 0x0000,    0,    2227,    2226,    2225,    2224 },// Wh Element B Q4 fund only (LP)
   { 0x00724500, 0x0000,    0,    2232,    2231,    2230,    2229 },// Wh Element B Q1+Q4 fund only (LP)
   { 0x00718500, 0x0000,    0,    2237,    2236,    2235,    2234 },// Wh Element B Q2+Q3 fund only (LP)
   { 0x0073C500, 0x0000,    0,    2242,    2241,    2240,    2239 },// Wh Element B (Q1+Q4)+(Q2+Q3) fund only (LP)
   { 0x0077C500, 0x0000,    0,    2247,    2246,    2245,    2244 },// Wh Element B (Q1+Q4)-(Q2+Q3) fund only (LP)
   { 0x00784500, 0x0000,    0,    2252,    2251,    2250,    2249 },// Wh Element C Q1 fund only (LP)
   { 0x00788500, 0x0000,    0,    2257,    2256,    2255,    2254 },// Wh Element C Q2 fund only (LP)
   { 0x00790500, 0x0000,    0,    2262,    2261,    2260,    2259 },// Wh Element C Q3 fund only (LP)
   { 0x007A0500, 0x0000,    0,    2267,    2266,    2265,    2264 },// Wh Element C Q4 fund only (LP)
   { 0x007A4500, 0x0000,    0,    2272,    2271,    2270,    2269 },// Wh Element C Q1+Q4 fund only (LP)
   { 0x00798500, 0x0000,    0,    2277,    2276,    2275,    2274 },// Wh Element C Q2+Q3 fund only (LP)
   { 0x007BC500, 0x0000,    0,    2282,    2281,    2280,    2279 },// Wh Element C (Q1+Q4)+(Q2+Q3) fund only (LP)
   { 0x007FC500, 0x0000,    0,    2287,    2286,    2285,    2284 },// Wh Element C (Q1+Q4)-(Q2+Q3) fund only (LP)

   // IEEE measurements
   { 0x00284501, 0x0000,    0,    2332,    2331,    2330,    2329 },// varh Element A Q1 fund + harmonics (LP)
   { 0x00288501, 0x0000,    0,    2337,    2336,    2335,    2334 },// varh Element A Q2 fund + harmonics (LP)
   { 0x00290501, 0x0000,    0,    2342,    2341,    2340,    2339 },// varh Element A Q3 fund + harmonics (LP)
   { 0x002A0501, 0x0000,    0,    2347,    2346,    2345,    2344 },// varh Element A Q4 fund + harmonics (LP)
   { 0x0028C501, 0x0000,    0,    2352,    2351,    2350,    2349 },// varh Element A Q1+Q2 fund + harmonics (LP)
   { 0x002B0501, 0x0000,    0,    2357,    2356,    2355,    2354 },// varh Element A Q3+Q4 fund + harmonics (LP)
   { 0x002BC501, 0x0000,    0,    2362,    2361,    2360,    2359 },// varh Element A (Q1+Q2)+(Q3+Q4) fund + harmonics (LP)
   { 0x002FC501, 0x0000,    0,    2367,    2366,    2365,    2364 },// varh Element A (Q1+Q2)-(Q3+Q4) fund + harmonics (LP)

   { 0x00304501, 0x0000,    0,    2372,    2371,    2370,    2369 },// varh Element B Q1 fund + harmonics (LP)
   { 0x00308501, 0x0000,    0,    2377,    2376,    2375,    2374 },// varh Element B Q2 fund + harmonics (LP)
   { 0x00310501, 0x0000,    0,    2382,    2381,    2380,    2379 },// varh Element B Q3 fund + harmonics (LP)
   { 0x00320501, 0x0000,    0,    2387,    2386,    2385,    2384 },// varh Element B Q4 fund + harmonics (LP)
   { 0x0030C501, 0x0000,    0,    2392,    2391,    2390,    2389 },// varh Element B Q1+Q2 fund + harmonics (LP)
   { 0x00330501, 0x0000,    0,    2397,    2396,    2395,    2394 },// varh Element B Q3+Q4 fund + harmonics (LP)
   { 0x0033C501, 0x0000,    0,    2402,    2401,    2400,    2399 },// varh Element B (Q1+Q2)+(Q3+Q4) fund + harmonics (LP)
   { 0x0037C501, 0x0000,    0,    2407,    2406,    2405,    2404 },// varh Element B (Q1+Q2)-(Q3+Q4) fund + harmonics (LP)

   { 0x00384501, 0x0000,    0,    2412,    2411,    2410,    2409 },// varh Element C Q1 fund + harmonics (LP)
   { 0x00388501, 0x0000,    0,    2417,    2416,    2415,    2414 },// varh Element C Q2 fund + harmonics (LP)
   { 0x00390501, 0x0000,    0,    2422,    2421,    2420,    2419 },// varh Element C Q3 fund + harmonics (LP)
   { 0x003A0501, 0x0000,    0,    2427,    2426,    2425,    2424 },// varh Element C Q4 fund + harmonics (LP)
   { 0x0038C501, 0x0000,    0,    2432,    2431,    2430,    2429 },// varh Element C Q1+Q2 fund + harmonics (LP)
   { 0x003B0501, 0x0000,    0,    2437,    2436,    2435,    2434 },// varh Element C Q3+Q4 fund + harmonics (LP)
   { 0x003BC501, 0x0000,    0,    2442,    2441,    2440,    2439 },// varh Element C (Q1+Q2)+(Q3+Q4) fund + harmonics (LP)
   { 0x003FC501, 0x0000,    0,    2447,    2446,    2445,    2444 },// varh Element C (Q1+Q2)-(Q3+Q4) fund + harmonics (LP)

   { 0x00684501, 0x0000,    0,    2452,    2451,    2450,    2449 },// varh Element A Q1 fund only (LP)
   { 0x00688501, 0x0000,    0,    2457,    2456,    2455,    2454 },// varh Element A Q2 fund only (LP)
   { 0x00690501, 0x0000,    0,    2462,    2461,    2460,    2459 },// varh Element A Q3 fund only (LP)
   { 0x006A0501, 0x0000,    0,    2467,    2466,    2465,    2464 },// varh Element A Q4 fund only (LP)
   { 0x0068C501, 0x0000,    0,    2472,    2471,    2470,    2469 },// varh Element A Q1+Q2 fund only (LP)
   { 0x006B0501, 0x0000,    0,    2477,    2476,    2475,    2474 },// varh Element A Q3+Q4 fund only (LP)
   { 0x006BC501, 0x0000,    0,    2482,    2481,    2480,    2479 },// varh Element A (Q1+Q2)+(Q3+Q4) fund only (LP)
   { 0x006FC501, 0x0000,    0,    2487,    2486,    2485,    2484 },// varh Element A (Q1+Q2)-(Q3+Q4) fund only (LP)

   { 0x00704501, 0x0000,    0,    2492,    2491,    2490,    2489 },// varh Element B Q1 fund only (LP)
   { 0x00708501, 0x0000,    0,    2497,    2496,    2495,    2494 },// varh Element B Q2 fund only (LP)
   { 0x00710501, 0x0000,    0,    2502,    2501,    2500,    2499 },// varh Element B Q3 fund only (LP)
   { 0x00720501, 0x0000,    0,    2507,    2506,    2505,    2504 },// varh Element B Q4 fund only (LP)
   { 0x0070C501, 0x0000,    0,    2512,    2511,    2510,    2509 },// varh Element B Q1+Q2 fund only (LP)
   { 0x00730501, 0x0000,    0,    2517,    2516,    2515,    2514 },// varh Element B Q3+Q4 fund only (LP)
   { 0x0073C501, 0x0000,    0,    2522,    2521,    2520,    2519 },// varh Element B (Q1+Q2)+(Q3+Q4) fund only (LP)
   { 0x0077C501, 0x0000,    0,    2527,    2526,    2525,    2524 },// varh Element B (Q1+Q2)-(Q3+Q4) fund only (LP)

   { 0x00784501, 0x0000,    0,    2532,    2531,    2530,    2529 },// varh Element C Q1 fund only (LP)
   { 0x00788501, 0x0000,    0,    2537,    2536,    2535,    2534 },// varh Element C Q2 fund only (LP)
   { 0x00790501, 0x0000,    0,    2542,    2541,    2540,    2539 },// varh Element C Q3 fund only (LP)
   { 0x007A0501, 0x0000,    0,    2547,    2546,    2545,    2544 },// varh Element C Q4 fund only (LP)
   { 0x0078C501, 0x0000,    0,    2552,    2551,    2550,    2549 },// varh Element C Q1+Q2 fund only (LP)
   { 0x007B0501, 0x0000,    0,    2557,    2556,    2555,    2554 },// varh Element C Q3+Q4 fund only (LP)
   { 0x007BC501, 0x0000,    0,    2562,    2561,    2560,    2559 },// varh Element C (Q1+Q2)+(Q3+Q4) fund only (LP)
   { 0x007FC501, 0x0000,    0,    2567,    2566,    2565,    2564 },// varh Element C (Q1+Q2)-(Q3+Q4) fund only (LP)

   { 0x00004501, 0x0000,    0,     269,     273,     277,     281 },// varh Q1 fund + harmonics (LP)
   { 0x00008501, 0x0000,    0,     270,     274,     278,     282 },// varh Q2 fund + harmonics (LP)
   { 0x00010501, 0x0000,    0,     271,     275,     279,     283 },// varh Q3 fund + harmonics (LP)
   { 0x00020501, 0x0000,    0,     272,     276,     280,     284 },// varh Q4 fund + harmonics (LP)
   { 0x0000C501, 0x0000,    0,      72,      64,     120,     111 },// varh Q1+Q2 fund + harmonics (LP)
   { 0x00030501, 0x0000,    0,      76,      67,     123,     114 },// varh Q3+Q4 fund + harmonics (LP)
   { 0x0003C501, 0x0000,    0,     353,     352,     351,     350 },// varh (Q1+Q2)+(Q3+Q4) fund + harmonics (LP)
   { 0x0007C501, 0x0000,    0,      80,      71,     127,     118 },// varh (Q1+Q2)-(Q3+Q4) fund + harmonics (LP)

   { 0x00404501, 0x0000,    0,    2576,    2575,    2574,    2573 },// varh Q1 fund only (LP)
   { 0x00408501, 0x0000,    0,    2581,    2580,    2579,    2578 },// varh Q2 fund only (LP)
   { 0x00410501, 0x0000,    0,    2586,    2585,    2584,    2583 },// varh Q3 fund only (LP)
   { 0x00420501, 0x0000,    0,    2591,    2590,    2589,    2588 },// varh Q4 fund only (LP)
   { 0x0040C501, 0x0000,    0,    2596,    2595,    2594,    2593 },// varh Q1+Q2 fund only (LP)
   { 0x00430501, 0x0000,    0,    2601,    2600,    2599,    2598 },// varh Q3+Q4 fund only (LP)
   { 0x0043C501, 0x0000,    0,    2606,    2605,    2604,    2603 },// varh (Q1+Q2)+(Q3+Q4) fund only (LP)
   { 0x0047C501, 0x0000,    0,    2611,    2610,    2609,    2608 },// varh (Q1+Q2)-(Q3+Q4) fund only (LP)

   // fuzzy measurements start here
   { 0x80284501, 0x0000,    0,   10350,   10349,   10348,   10347 },// varh Element A Q1 fund + harmonics (LP)
   { 0x80288501, 0x0000,    0,   10355,   10354,   10353,   10352 },// varh Element A Q2 fund + harmonics (LP)
   { 0x80290501, 0x0000,    0,   10360,   10359,   10358,   10357 },// varh Element A Q3 fund + harmonics (LP)
   { 0x802A0501, 0x0000,    0,   10365,   10364,   10363,   10362 },// varh Element A Q4 fund + harmonics (LP)
   { 0x8028C501, 0x0000,    0,   10370,   10369,   10368,   10367 },// varh Element A Q1+Q2 fund + harmonics (LP)
   { 0x802B0501, 0x0000,    0,   10375,   10374,   10373,   10372 },// varh Element A Q3+Q4 fund + harmonics (LP)
   { 0x802BC501, 0x0000,    0,   10380,   10379,   10378,   10377 },// varh Element A (Q1+Q2)+(Q3+Q4) fund + harmonics (LP)
   { 0x802FC501, 0x0000,    0,   10385,   10384,   10383,   10382 },// varh Element A (Q1+Q2)-(Q3+Q4) fund + harmonics (LP)
   { 0x80304501, 0x0000,    0,   10390,   10389,   10388,   10387 },// varh Element B Q1 fund + harmonics (LP)
   { 0x80308501, 0x0000,    0,   10395,   10394,   10393,   10392 },// varh Element B Q2 fund + harmonics (LP)
   { 0x80310501, 0x0000,    0,   10400,   10399,   10398,   10397 },// varh Element B Q3 fund + harmonics (LP)
   { 0x80320501, 0x0000,    0,   10405,   10404,   10403,   10402 },// varh Element B Q4 fund + harmonics (LP)
   { 0x8030C501, 0x0000,    0,   10410,   10409,   10408,   10407 },// varh Element B Q1+Q2 fund + harmonics (LP)
   { 0x80330501, 0x0000,    0,   10415,   10414,   10413,   10412 },// varh Element B Q3+Q4 fund + harmonics (LP)
   { 0x8033C501, 0x0000,    0,   10420,   10419,   10418,   10417 },// varh Element B (Q1+Q2)+(Q3+Q4) fund + harmonics (LP)
   { 0x8037C501, 0x0000,    0,   10425,   10424,   10423,   10422 },// varh Element B (Q1+Q2)-(Q3+Q4) fund + harmonics (LP)
   { 0x80384501, 0x0000,    0,   10430,   10429,   10428,   10427 },// varh Element C Q1 fund + harmonics (LP)
   { 0x80388501, 0x0000,    0,   10435,   10434,   10433,   10432 },// varh Element C Q2 fund + harmonics (LP)
   { 0x80390501, 0x0000,    0,   10440,   10439,   10438,   10437 },// varh Element C Q3 fund + harmonics (LP)
   { 0x803A0501, 0x0000,    0,   10445,   10444,   10443,   10442 },// varh Element C Q4 fund + harmonics (LP)
   { 0x8038C501, 0x0000,    0,   10450,   10449,   10448,   10447 },// varh Element C Q1+Q2 fund + harmonics (LP)
   { 0x803B0501, 0x0000,    0,   10455,   10454,   10453,   10452 },// varh Element C Q3+Q4 fund + harmonics (LP)
   { 0x803BC501, 0x0000,    0,   10460,   10459,   10458,   10457 },// varh Element C (Q1+Q2)+(Q3+Q4) fund + harmonics (LP)
   { 0x803FC501, 0x0000,    0,   10465,   10464,   10463,   10462 },// varh Element C (Q1+Q2)-(Q3+Q4) fund + harmonics (LP)

   { 0x80684501, 0x0000,    0,   10470,   10469,   10468,   10467 },// varh Element A Q1 fund only (LP)
   { 0x80688501, 0x0000,    0,   10475,   10474,   10473,   10472 },// varh Element A Q2 fund only (LP)
   { 0x80690501, 0x0000,    0,   10480,   10479,   10478,   10477 },// varh Element A Q3 fund only (LP)
   { 0x806A0501, 0x0000,    0,   10485,   10484,   10483,   10482 },// varh Element A Q4 fund only (LP)
   { 0x8068C501, 0x0000,    0,   10490,   10489,   10488,   10487 },// varh Element A Q1+Q2 fund only (LP)
   { 0x806B0501, 0x0000,    0,   10495,   10494,   10493,   10492 },// varh Element A Q3+Q4 fund only (LP)
   { 0x806BC501, 0x0000,    0,   10500,   10499,   10498,   10497 },// varh Element A (Q1+Q2)+(Q3+Q4) fund only (LP)
   { 0x806FC501, 0x0000,    0,   10505,   10504,   10503,   10502 },// varh Element A (Q1+Q2)-(Q3+Q4) fund only (LP)
   { 0x80704501, 0x0000,    0,   10510,   10509,   10508,   10507 },// varh Element B Q1 fund only (LP)
   { 0x80708501, 0x0000,    0,   10515,   10514,   10513,   10512 },// varh Element B Q2 fund only (LP)
   { 0x80710501, 0x0000,    0,   10520,   10519,   10518,   10517 },// varh Element B Q3 fund only (LP)
   { 0x80720501, 0x0000,    0,   10525,   10524,   10523,   10522 },// varh Element B Q4 fund only (LP)
   { 0x8070C501, 0x0000,    0,   10530,   10529,   10528,   10527 },// varh Element B Q1+Q2 fund only (LP)
   { 0x80730501, 0x0000,    0,   10535,   10534,   10533,   10532 },// varh Element B Q3+Q4 fund only (LP)
   { 0x8073C501, 0x0000,    0,   10540,   10539,   10538,   10537 },// varh Element B (Q1+Q2)+(Q3+Q4) fund only (LP)
   { 0x8077C501, 0x0000,    0,   10545,   10544,   10543,   10542 },// varh Element B (Q1+Q2)-(Q3+Q4) fund only (LP)
   { 0x80784501, 0x0000,    0,   10550,   10549,   10548,   10547 },// varh Element C Q1 fund only (LP)
   { 0x80788501, 0x0000,    0,   10555,   10554,   10553,   10552 },// varh Element C Q2 fund only (LP)
   { 0x80790501, 0x0000,    0,   10560,   10559,   10558,   10557 },// varh Element C Q3 fund only (LP)
   { 0x807A0501, 0x0000,    0,   10565,   10564,   10563,   10562 },// varh Element C Q4 fund only (LP)
   { 0x8078C501, 0x0000,    0,   10570,   10569,   10568,   10567 },// varh Element C Q1+Q2 fund only (LP)
   { 0x807B0501, 0x0000,    0,   10575,   10574,   10573,   10572 },// varh Element C Q3+Q4 fund only (LP)
   { 0x807BC501, 0x0000,    0,   10580,   10579,   10578,   10577 },// varh Element C (Q1+Q2)+(Q3+Q4) fund only (LP)
   { 0x807FC501, 0x0000,    0,   10585,   10584,   10583,   10582 },// varh Element C (Q1+Q2)-(Q3+Q4) fund only (LP)

   { 0x80004501, 0x0000,    0,   10590,   10589,   10588,   10587 },// varh Q1 Fuzzy fund + harmonics (LP)
   { 0x80008501, 0x0000,    0,   10595,   10594,   10593,   10592 },// varh Q2 fund + harmonics (LP)
   { 0x80010501, 0x0000,    0,   10600,   10599,   10598,   10597 },// varh Q3 fund + harmonics (LP)
   { 0x80020501, 0x0000,    0,   10605,   10604,   10603,   10602 },// varh Q4 fund + harmonics (LP)
   { 0x8000C501, 0x0000,    0,   10610,   10609,   10608,   10607 },// varh Q1+Q2 fund + harmonics (LP)
   { 0x80030501, 0x0000,    0,   10615,   10614,   10613,   10612 },// varh Q3+Q4 fund + harmonics (LP)
   { 0x8003C501, 0x0000,    0,   10620,   10619,   10618,   10617 },// varh (Q1+Q2)+(Q3+Q4) fund + harmonics (LP)
   { 0x8007C501, 0x0000,    0,   10625,   10624,   10623,   10622 },// varh (Q1+Q2)-(Q3+Q4) fund + harmonics (LP)
   { 0x80404501, 0x0000,    0,   10630,   10629,   10628,   10627 },// varh Q1 fund only (LP)
   { 0x80408501, 0x0000,    0,   10635,   10634,   10633,   10632 },// varh Q2 fund only (LP)
   { 0x80410501, 0x0000,    0,   10640,   10639,   10638,   10637 },// varh Q3 fund only (LP)
   { 0x80420501, 0x0000,    0,   10645,   10644,   10643,   10642 },// varh Q4 fund only (LP)
   { 0x8040C501, 0x0000,    0,   10650,   10649,   10648,   10647 },// varh Q1+Q2 fund only (LP)
   { 0x80430501, 0x0000,    0,   10655,   10654,   10653,   10652 },// varh Q3+Q4 fund only (LP)
   { 0x8043C501, 0x0000,    0,   10660,   10659,   10658,   10657 },// varh (Q1+Q2)+(Q3+Q4) fund only (LP)
   { 0x8047C501, 0x0000,    0,   10665,   10664,   10663,   10662 },// varh (Q1+Q2)-(Q3+Q4) fund only (LP)

   { 0x002BC502, 0x0000,    0,    2616,    2615,    2614,    2613 },// Apparent VAh Element A (LP)
   { 0x0033C502, 0x0000,    0,    2621,    2620,    2619,    2618 },// Apparent VAh Element B (LP)
   { 0x003BC502, 0x0000,    0,    2626,    2625,    2624,    2623 },// Apparent VAh Element C (LP)
   { 0x0003C502, 0x0000,    0,     132,     133,     134,     135 },// Apparent VAh (LP)
   { 0x00284502, 0x0000,    0,    2631,    2630,    2629,    2628 },// Apparent VAh Element A Q1 (LP)
   { 0x00288502, 0x0000,    0,    2636,    2635,    2634,    2633 },// Apparent VAh Element A Q2 (LP)
   { 0x00290502, 0x0000,    0,    2641,    2640,    2639,    2638 },// Apparent VAh Element A Q3 (LP)
   { 0x002A0502, 0x0000,    0,    2646,    2645,    2644,    2643 },// Apparent VAh Element A Q4 (LP)
   { 0x00304502, 0x0000,    0,    2651,    2650,    2649,    2648 },// Apparent VAh Element B Q1 (LP)
   { 0x00308502, 0x0000,    0,    2656,    2655,    2654,    2653 },// Apparent VAh Element B Q2 (LP)
   { 0x00310502, 0x0000,    0,    2661,    2660,    2659,    2658 },// Apparent VAh Element B Q3 (LP)
   { 0x00320502, 0x0000,    0,    2666,    2665,    2664,    2663 },// Apparent VAh Element B Q4 (LP)
   { 0x00384502, 0x0000,    0,    2671,    2670,    2669,    2668 },// Apparent VAh Element C Q1 (LP)
   { 0x00388502, 0x0000,    0,    2676,    2675,    2674,    2673 },// Apparent VAh Element C Q2 (LP)
   { 0x00390502, 0x0000,    0,    2681,    2680,    2679,    2678 },// Apparent VAh Element C Q3 (LP)
   { 0x003A0502, 0x0000,    0,    2686,    2685,    2684,    2683 },// Apparent VAh Element C Q4 (LP)
   { 0x00004502, 0x0000,    0,    2691,    2690,    2689,    2688 },// Apparent VAh Q1 (LP)
   { 0x80004502, 0x0000,    0,    2691,    2690,    2689,    2688 },// Apparent VAh Q1 (LP)
   { 0x80004502, 0x0001,    0,    2981,    2980,    2979,    2978 },// Arithmetic Apparent VAh Q1 (LP)
   { 0x80004502, 0x0002,    0,   13741,   13740,   13739,   13738 },// Distortion VAh Q1 (LP)
   { 0x80004502, 0x0100,    0,   13686,   13685,   13684,   13683 },// Apparent () Q1 (LP)
   { 0x80004502, 0x0101,    0,   13691,   13690,   13689,   13688 },// Arithmetic Apparent () Q1 (LP)
   { 0x00008502, 0x0000,    0,    2696,    2695,    2694,    2693 },// Apparent VAh Q2 (LP)
   { 0x80008502, 0x0000,    0,    2696,    2695,    2694,    2693 },// Apparent VAh Q2 (LP)
   { 0x80008502, 0x0001,    0,    2986,    2985,    2984,    2983 },// Arithmetic Apparent VAh Q2 (LP)
   { 0x80008502, 0x0002,    0,   13696,   13695,   13694,   13693 },// Distortion VAh Q2 (LP)
   { 0x80008502, 0x0100,    0,   13701,   13700,   13699,   13698 },// Apparent () Q2 (LP)
   { 0x80008502, 0x0101,    0,   13706,   13705,   13704,   13703 },// Arithmetic Apparent () Q2 (LP)
   { 0x00010502, 0x0000,    0,    2701,    2700,    2699,    2698 },// Apparent VAh Q3 (LP)
   { 0x80010502, 0x0000,    0,    2701,    2700,    2699,    2698 },// Apparent VAh Q3 (LP)
   { 0x80010502, 0x0001,    0,    2991,    2990,    2989,    2988 },// Arithmetic Apparent VAh Q3 (LP)
   { 0x80010502, 0x0002,    0,   13711,   13710,   13709,   13708 },// Distortion VAh Q3 (LP)
   { 0x80010502, 0x0100,    0,   13716,   13715,   13714,   13713 },// Apparent () Q3 (LP)
   { 0x80010502, 0x0101,    0,   13721,   13720,   13719,   13718 },// Arithmetic Apparent () Q3 (LP)
   { 0x00020502, 0x0000,    0,    2706,    2705,    2704,    2703 },// Apparent VAh Q4 (LP)
   { 0x80020502, 0x0000,    0,    2706,    2705,    2704,    2703 },// Apparent VAh Q4 (LP)
   { 0x80020502, 0x0001,    0,    2996,    2995,    2994,    2993 },// Arithmetic Apparent VAh Q4 (LP)
   { 0x80020502, 0x0002,    0,   13726,   13725,   13724,   13723 },// Distortion VAh Q4 (LP)
   { 0x80020502, 0x0100,    0,   13731,   13730,   13729,   13728 },// Apparent () Q4 (LP)
   { 0x80020502, 0x0101,    0,   13736,   13735,   13734,   13733 },// Arithmetic Apparent () Q4 (LP)
   { 0x80024502, 0x0000,    0,    2711,    2710,    2709,    2708 },// Apparent VAh Q1+Q4 (LP)
   { 0x80024502, 0x0001,    0,    2716,    2715,    2714,    2713 },// Arithmetic Apparent VAh Q1+Q4 (LP)
   { 0x80024502, 0x0002,    0,    2721,    2720,    2719,    2718 },// Distoration VAh Q1+Q4 (LP)
   { 0x80024502, 0x0100,    0,   13626,   13625,   13624,   13623 },// Apparent () Q1+Q4 (LP)
   { 0x80024502, 0x0101,    0,   13631,   13630,   13629,   13628 },// Arithmetic Apparent () Q1+Q4 (LP)
   { 0x80018502, 0x0000,    0,    2726,    2725,    2724,    2723 },// Apparent VAh Q2+Q3 (LP)
   { 0x80018502, 0x0001,    0,    2731,    2730,    2729,    2728 },// Arithmetic Apparent VAh Q2+Q3 (LP)
   { 0x80018502, 0x0002,    0,    2736,    2735,    2734,    2733 },// Distortion VAh Q2+Q3 (LP)
   { 0x80018502, 0x0100,    0,   13636,   13635,   13634,   13633 },// Apparent () Q2+Q3 (LP)
   { 0x80018502, 0x0101,    0,   13641,   13640,   13639,   13638 },// Arithmetic Apparent () Q2+Q3 (LP)
   { 0x8003C502, 0x0000,    0,     132,     133,     134,     135 },// Apparent VAh (Q1+Q4)+(Q2+Q3) (LP)
   { 0x8003C502, 0x0001,    0,    2741,    2740,    2739,    2738 },// Arithmetic Apparent VAh (Q1+Q4)+(Q2+Q3) (LP)
   { 0x8003C502, 0x0002,    0,    2746,    2745,    2744,    2743 },// Distoration VAh (Q1+Q4)+(Q2+Q3) (LP)
   { 0x8003C502, 0x0100,    0,   13646,   13645,   13644,   13643 },// Apparent () (Q1+Q4)+(Q2+Q3) (LP)
   { 0x8003C502, 0x0101,    0,   13651,   13650,   13649,   13648 },// Arithmetic Apparent () (Q1+Q4)+(Q2+Q3) (LP)
   { 0x8007C502, 0x0000,    0,    2751,    2750,    2749,    2748 },// Apparent VAh (Q1+Q4)-(Q2+Q3) (LP)
   { 0x8007C502, 0x0001,    0,    2756,    2755,    2754,    2753 },// Arithmetic Apparent VAh (Q1+Q4)-(Q2+Q3) (LP)
   { 0x8007C502, 0x0002,    0,    2761,    2760,    2759,    2758 },// Distoration VAh (Q1+Q4)-(Q2+Q3) (LP)
   { 0x8007C502, 0x0100,    0,   13656,   13655,   13654,   13653 },// Apparent () (Q1+Q4)-(Q2+Q3) (LP)
   { 0x8007C502, 0x0101,    0,   13661,   13660,   13659,   13658 },// Arithmetic Apparent () (Q1+Q4)-(Q2+Q3) (LP)
   { 0x802A4502, 0x0000,    0,    2766,    2765,    2764,    2763 },// Apparent VAh Element A Q1+Q4 (LP)
   { 0x802A4502, 0x0001,    0,    2771,    2770,    2769,    2768 },// Arithmetic Apparent VAh Element A Q1+Q4 (LP)
   { 0x802A4502, 0x0002,    0,    2776,    2775,    2774,    2773 },// Distortion VAh Element A Q1+Q4 (LP)
   { 0x802A4502, 0x0100,    0,   13666,   13665,   13664,   13663 },// Apparent () Element A Q1+Q4 (LP)
   { 0x802A4502, 0x0101,    0,   13671,   13670,   13669,   13668 },// Arithmetic Apparent () Element A Q1+Q4 (LP)
   { 0x80298502, 0x0000,    0,    2781,    2780,    2779,    2778 },// Apparent VAh Element A Q2+Q3 (LP)
   { 0x80298502, 0x0001,    0,    2786,    2785,    2784,    2783 },// Arithmetic Apparent VAh Element A Q2+Q3 (LP)
   { 0x80298502, 0x0002,    0,    2791,    2790,    2789,    2788 },// Distortion VAh Element A Q2+Q3 (LP)
   { 0x80298502, 0x0100,    0,   13676,   13675,   13674,   13673 },// Apparent () Element A Q2+Q3 (LP)
   { 0x80298502, 0x0101,    0,   13681,   13680,   13679,   13678 },// Arithmetic Apparent () Element A Q2+Q3 (LP)
   { 0x802BC502, 0x0000,    0,    2616,    2615,    2614,    2613 },// Apparent VAh Element A (Q1+Q4)+(Q2+Q3) (LP)
   { 0x802BC502, 0x0001,    0,    2796,    2795,    2794,    2793 },// Arithmetic Apparent VAh Element A (Q1+Q4)+(Q2+Q3) (LP)
   { 0x802BC502, 0x0002,    0,    2801,    2800,    2799,    2798 },// Distortion VAh Element A (Q1+Q4)+(Q2+Q3) (LP)
   { 0x802BC502, 0x0100,    0,     758,     757,     756,     755 },// Apparent () Element A (Q1+Q4)+(Q2+Q3) (LP)
   { 0x802BC502, 0x0101,    0,     763,     762,     761,     760 },// Arithmetic Apparent () Element A (Q1+Q4)+(Q2+Q3) (LP)
   { 0x802FC502, 0x0000,    0,    2806,    2805,    2804,    2803 },// Apparent VAh Element A (Q1+Q4)-(Q2+Q3) (LP)
   { 0x802FC502, 0x0001,    0,    2811,    2810,    2809,    2808 },// Arithmetic Apparent VAh Element A (Q1+Q4)-(Q2+Q3) (LP)
   { 0x802FC502, 0x0002,    0,    2816,    2815,    2814,    2813 },// Distortion VAh Element A (Q1+Q4)-(Q2+Q3) (LP)
   { 0x802FC502, 0x0100,    0,     768,     767,     766,     765 },// Apparent () Element A (Q1+Q4)-(Q2+Q3) (LP)
   { 0x802FC502, 0x0101,    0,     773,     772,     771,     770 },// Arithmetic Apparent () Element A (Q1+Q4)-(Q2+Q3) (LP)
   { 0x80324502, 0x0000,    0,    2821,    2820,    2819,    2818 },// Apparent VAh Element B Q1+Q4 (LP)
   { 0x80324502, 0x0001,    0,    2826,    2825,    2824,    2823 },// Apparent VAh Element B Q1+Q4 (LP)
   { 0x80324502, 0x0002,    0,    2831,    2830,    2829,    2828 },// Apparent VAh Element B Q1+Q4 (LP)
   { 0x80324502, 0x0100,    0,     778,     777,     776,     775 },// Apparent VAh Element B Q1+Q4 (LP)
   { 0x80324502, 0x0101,    0,     783,     782,     781,     780 },// Apparent VAh Element B Q1+Q4 (LP)
   { 0x80318502, 0x0000,    0,    2836,    2835,    2834,    2833 },// Apparent VAh Element B Q2+Q3 (LP)
   { 0x80318502, 0x0001,    0,    2841,    2840,    2839,    2838 },// Apparent VAh Element B Q2+Q3 (LP)
   { 0x80318502, 0x0002,    0,    2846,    2845,    2844,    2843 },// Apparent VAh Element B Q2+Q3 (LP)
   { 0x80318502, 0x0100,    0,     788,     787,     786,     785 },// Apparent VAh Element B Q2+Q3 (LP)
   { 0x80318502, 0x0101,    0,     793,     792,     791,     790 },// Apparent VAh Element B Q2+Q3 (LP)
   { 0x8033C502, 0x0000,    0,    2621,    2620,    2619,    2618 },// Apparent VAh Element B (Q1+Q4)+(Q2+Q3) (LP)
   { 0x8033C502, 0x0001,    0,    2851,    2850,    2849,    2848 },// Apparent VAh Element B (Q1+Q4)+(Q2+Q3) (LP)
   { 0x8033C502, 0x0002,    0,    2856,    2855,    2854,    2853 },// Apparent VAh Element B (Q1+Q4)+(Q2+Q3) (LP)
   { 0x8033C502, 0x0100,    0,     798,     797,     796,     795 },// Apparent VAh Element B (Q1+Q4)+(Q2+Q3) (LP)
   { 0x8033C502, 0x0101,    0,     803,     802,     801,     800 },// Apparent VAh Element B (Q1+Q4)+(Q2+Q3) (LP)
   { 0x8037C502, 0x0000,    0,    2861,    2860,    2859,    2858 },// Apparent VAh Element B (Q1+Q4)-(Q2+Q3) (LP)
   { 0x8037C502, 0x0001,    0,    2866,    2865,    2864,    2863 },// Apparent VAh Element B (Q1+Q4)-(Q2+Q3) (LP)
   { 0x8037C502, 0x0002,    0,    2871,    2870,    2869,    2868 },// Apparent VAh Element B (Q1+Q4)-(Q2+Q3) (LP)
   { 0x8037C502, 0x0100,    0,     808,     807,     806,     805 },// Apparent VAh Element B (Q1+Q4)-(Q2+Q3) (LP)
   { 0x8037C502, 0x0101,    0,     813,     812,     811,     810 },// Apparent VAh Element B (Q1+Q4)-(Q2+Q3) (LP)
   { 0x803A4502, 0x0000,    0,    2876,    2875,    2874,    2873 },// Apparent VAh Element C Q1+Q4 (LP)
   { 0x803A4502, 0x0001,    0,    2881,    2880,    2879,    2878 },// Apparent VAh Element C Q1+Q4 (LP)
   { 0x803A4502, 0x0002,    0,    2886,    2885,    2884,    2883 },// Apparent VAh Element C Q1+Q4 (LP)
   { 0x803A4502, 0x0100,    0,     818,     817,     816,     815 },// Apparent VAh Element C Q1+Q4 (LP)
   { 0x803A4502, 0x0101,    0,     823,     822,     821,     820 },// Apparent VAh Element C Q1+Q4 (LP)
   { 0x80398502, 0x0000,    0,    2891,    2890,    2889,    2888 },// Apparent VAh Element C Q2+Q3 (LP)
   { 0x80398502, 0x0001,    0,    2896,    2895,    2894,    2893 },// Apparent VAh Element C Q2+Q3 (LP)
   { 0x80398502, 0x0002,    0,    2901,    2900,    2899,    2898 },// Apparent VAh Element C Q2+Q3 (LP)
   { 0x80398502, 0x0100,    0,     828,     827,     826,     825 },// Apparent VAh Element C Q2+Q3 (LP)
   { 0x80398502, 0x0101,    0,     833,     832,     831,     830 },// Apparent VAh Element C Q2+Q3 (LP)
   { 0x803BC502, 0x0000,    0,    2626,    2625,    2624,    2623 },// Apparent VAh Element C (Q1+Q4)+(Q2+Q3) (LP)
   { 0x803BC502, 0x0001,    0,    2906,    2905,    2904,    2903 },// Apparent VAh Element C (Q1+Q4)+(Q2+Q3) (LP)
   { 0x803BC502, 0x0002,    0,    2911,    2910,    2909,    2908 },// Apparent VAh Element C (Q1+Q4)+(Q2+Q3) (LP)
   { 0x803BC502, 0x0100,    0,     838,     837,     836,     835 },// Apparent VAh Element C (Q1+Q4)+(Q2+Q3) (LP)
   { 0x803BC502, 0x0101,    0,     843,     842,     841,     840 },// Apparent VAh Element C (Q1+Q4)+(Q2+Q3) (LP)
   { 0x803FC502, 0x0000,    0,    2916,    2915,    2914,    2913 },// Apparent VAh Element C (Q1+Q4)-(Q2+Q3) (LP)
   { 0x803FC502, 0x0001,    0,    2921,    2920,    2919,    2918 },// Apparent VAh Element C (Q1+Q4)-(Q2+Q3) (LP)
   { 0x803FC502, 0x0002,    0,    2926,    2925,    2924,    2923 },// Apparent VAh Element C (Q1+Q4)-(Q2+Q3) (LP)
   { 0x803FC502, 0x0100,    0,     848,     847,     846,     845 },// Apparent VAh Element C (Q1+Q4)-(Q2+Q3) (LP)
   { 0x803FC502, 0x0101,    0,     853,     852,     851,     850 },// Apparent VAh Element C (Q1+Q4)-(Q2+Q3) (LP)

   { 0x0003C503, 0x0000,    0,    2931,    2930,    2929,    2928 },// Phasor Apparent VAh, fund + harmonics (LP)
   { 0x0043C503, 0x0000,    0,    2936,    2935,    2934,    2933 },// Phasor Apparent VAh, fund only (LP)
   { 0x00004503, 0x0000,    0,    2941,    2940,    2939,    2938 },// Phasor Apparent VAh Q1 fund+harmonics (LP)
   { 0x00008503, 0x0000,    0,    2946,    2945,    2944,    2943 },// Phasor Apparent VAh Q2 fund+harmonics (LP)
   { 0x00010503, 0x0000,    0,    2951,    2950,    2949,    2948 },// Phasor Apparent VAh Q3 fund+harmonics (LP)
   { 0x00020503, 0x0000,    0,    2956,    2955,    2954,    2953 },// Phasor Apparent VAh Q4 fund+harmonics (LP)
   { 0x00404503, 0x0000,    0,    2961,    2960,    2959,    2958 },// Phasor Apparent VAh Q1 fund only (LP)
   { 0x00408503, 0x0000,    0,    2966,    2965,    2964,    2963 },// Phasor Apparent VAh Q2 fund only (LP)
   { 0x00410503, 0x0000,    0,    2971,    2970,    2969,    2968 },// Phasor Apparent VAh Q3 fund only (LP)
   { 0x00420503, 0x0000,    0,    2976,    2975,    2974,    2973 },// Phasor Apparent VAh Q4 fund only (LP)

   { 0x8003C503, 0x0000,    0,    2931,    2930,    2929,    2928 },// Phasor Apparent VAh, fund + harmonics (LP)
   { 0x8043C503, 0x0000,    0,    2936,    2935,    2934,    2933 },// Phasor Apparent VAh, fund only (LP)
   { 0x80004503, 0x0000,    0,    2941,    2940,    2939,    2938 },// Phasor Apparent VAh Q1 fund+harmonics (LP)
   { 0x80008503, 0x0000,    0,    2946,    2945,    2944,    2943 },// Phasor Apparent VAh Q2 fund+harmonics (LP)
   { 0x80010503, 0x0000,    0,    2951,    2950,    2949,    2948 },// Phasor Apparent VAh Q3 fund+harmonics (LP)
   { 0x80020503, 0x0000,    0,    2956,    2955,    2954,    2953 },// Phasor Apparent VAh Q4 fund+harmonics (LP)
   { 0x80404503, 0x0000,    0,    2961,    2960,    2959,    2958 },// Phasor Apparent VAh Q1 fund only (LP)
   { 0x80408503, 0x0000,    0,    2966,    2965,    2964,    2963 },// Phasor Apparent VAh Q2 fund only (LP)
   { 0x80410503, 0x0000,    0,    2971,    2970,    2969,    2968 },// Phasor Apparent VAh Q3 fund only (LP)
   { 0x80420503, 0x0000,    0,    2976,    2975,    2974,    2973 },// Phasor Apparent VAh Q4 fund only (LP)

   { 0x8003C503, 0x0001,    0,   10670,   10669,   10668,   10667 },// Phasor Apparent VAh, fund + harmonics (LP)
   { 0x8043C503, 0x0001,    0,   10675,   10674,   10673,   10672 },// Phasor Apparent VAh, fund only (LP)
   { 0x80004503, 0x0001,    0,   10680,   10679,   10678,   10677 },// Phasor Apparent VAh Q1 fund+harmonics (LP)
   { 0x80008503, 0x0001,    0,   10685,   10684,   10683,   10682 },// Phasor Apparent VAh Q2 fund+harmonics (LP)
   { 0x80010503, 0x0001,    0,   10690,   10689,   10688,   10687 },// Phasor Apparent VAh Q3 fund+harmonics (LP)
   { 0x80020503, 0x0001,    0,   10695,   10694,   10693,   10692 },// Phasor Apparent VAh Q4 fund+harmonics (LP)
   { 0x80404503, 0x0001,    0,   10700,   10699,   10698,   10697 },// Phasor Apparent VAh Q1 fund only (LP)
   { 0x80408503, 0x0001,    0,   10705,   10704,   10703,   10702 },// Phasor Apparent VAh Q2 fund only (LP)
   { 0x80410503, 0x0001,    0,   10710,   10709,   10708,   10707 },// Phasor Apparent VAh Q3 fund only (LP)
   { 0x80420503, 0x0001,    0,   10715,   10714,   10713,   10712 },// Phasor Apparent VAh Q4 fund only (LP)
   { 0x8003C503, 0x0100,    0,   10720,   10719,   10718,   10717 },// Phasor Apparent VAh, fund + harmonics (LP)
   { 0x8043C503, 0x0100,    0,   10725,   10724,   10723,   10722 },// Phasor Apparent VAh, fund only (LP)
   { 0x80004503, 0x0100,    0,   10730,   10729,   10728,   10727 },// Phasor Apparent VAh Q1 fund+harmonics (LP)
   { 0x80008503, 0x0100,    0,   10735,   10734,   10733,   10732 },// Phasor Apparent VAh Q2 fund+harmonics (LP)
   { 0x80010503, 0x0100,    0,   10740,   10739,   10738,   10737 },// Phasor Apparent VAh Q3 fund+harmonics (LP)
   { 0x80020503, 0x0100,    0,   10745,   10744,   10743,   10742 },// Phasor Apparent VAh Q4 fund+harmonics (LP)
   { 0x80404503, 0x0100,    0,   10750,   10749,   10748,   10747 },// Phasor Apparent VAh Q1 fund only (LP)
   { 0x80408503, 0x0100,    0,   10755,   10754,   10753,   10752 },// Phasor Apparent VAh Q2 fund only (LP)
   { 0x80410503, 0x0100,    0,   10760,   10759,   10758,   10757 },// Phasor Apparent VAh Q3 fund only (LP)
   { 0x80420503, 0x0100,    0,   10765,   10764,   10763,   10762 },// Phasor Apparent VAh Q4 fund only (LP)
   { 0x8003C503, 0x0101,    0,   10770,   10769,   10768,   10767 },// Phasor Apparent VAh, fund + harmonics (LP)
   { 0x8043C503, 0x0101,    0,   10775,   10774,   10773,   10772 },// Phasor Apparent VAh, fund only (LP)
   { 0x80004503, 0x0101,    0,   10780,   10779,   10778,   10777 },// Phasor Apparent VAh Q1 fund+harmonics (LP)
   { 0x80008503, 0x0101,    0,   10785,   10784,   10783,   10782 },// Phasor Apparent VAh Q2 fund+harmonics (LP)
   { 0x80010503, 0x0101,    0,   10790,   10789,   10788,   10787 },// Phasor Apparent VAh Q3 fund+harmonics (LP)
   { 0x80020503, 0x0101,    0,   10795,   10794,   10793,   10792 },// Phasor Apparent VAh Q4 fund+harmonics (LP)
   { 0x80404503, 0x0101,    0,   10800,   10799,   10798,   10797 },// Phasor Apparent VAh Q1 fund only (LP)
   { 0x80408503, 0x0101,    0,   10805,   10804,   10803,   10802 },// Phasor Apparent VAh Q2 fund only (LP)
   { 0x80410503, 0x0101,    0,   10810,   10809,   10808,   10807 },// Phasor Apparent VAh Q3 fund only (LP)
   { 0x80420503, 0x0101,    0,   10815,   10814,   10813,   10812 },// Phasor Apparent VAh Q4 fund only (LP)
   { 0x0028050A, 0x0000,    0,    3001,    3000,    2999,    2998 },// V2h Element A, fund + harmonics (LP)
   { 0x0030050A, 0x0000,    0,    3006,    3005,    3004,    3003 },// V2h Element B, fund + harmonics (LP)
   { 0x0038050A, 0x0000,    0,    3011,    3010,    3009,    3008 },// V2h Element C, fund + harmonics (LP)
   { 0x0068050A, 0x0000,    0,    3016,    3015,    3014,    3013 },// V2h Element A, fund (LP)
   { 0x0070050A, 0x0000,    0,    3021,    3020,    3019,    3018 },// V2h Element B, fund (LP)
   { 0x0078050A, 0x0000,    0,    3026,    3025,    3024,    3023 },// V2h Element C, fund (LP)
   { 0x0058050A, 0x0000,    0,    3031,    3030,    3029,    3028 },// V2hAC, fund (LP)
   { 0x0050050A, 0x0000,    0,    3036,    3035,    3034,    3033 },// V2hCB, fund (LP)
   { 0x0048050A, 0x0000,    0,    3041,    3040,    3039,    3038 },// V2hBA, fund (LP)
   { 0x0018050A, 0x0000,    0,    3046,    3045,    3044,    3043 },// V2hAC, fund + harmonics (LP)
   { 0x0010050A, 0x0000,    0,    3051,    3050,    3049,    3048 },// V2hCB, fund + harmonics (LP)
   { 0x0008050A, 0x0000,    0,    3056,    3055,    3054,    3053 },// V2hBA, fund + harmonics (LP)
   { 0x80280208, 0x0001,    0,     382,     381,     380,     379 },// VA fund + harmonics MAX
   { 0x80300208, 0x0001,    0,   10009,   10008,   10007,   10006 },// VB fund + harmonics MAX
   { 0x80380208, 0x0001,    0,     443,     442,     441,     440 },// VC fund + harmonics MAX
   { 0x80680208, 0x0001,    0,   10014,   10013,   10012,   10011 },// VA fund only MAX
   { 0x80700208, 0x0001,    0,   10019,   10018,   10017,   10016 },// VB fund only MAX
   { 0x80780208, 0x0001,    0,   10024,   10023,   10022,   10021 },// VC fund only MAX
   { 0x80180208, 0x0001,    0,   10029,   10028,   10027,   10026 },// VAC fund + harmonics MAX
   { 0x80100208, 0x0001,    0,   10034,   10033,   10032,   10031 },// VCB fund + harmonics MAX
   { 0x80080208, 0x0001,    0,   10039,   10038,   10037,   10036 },// VBA fund + harmonics MAX
   { 0x80580208, 0x0001,    0,   10044,   10043,   10042,   10041 },// VAC fund only MAX
   { 0x80500208, 0x0001,    0,   10049,   10048,   10047,   10046 },// VCB fund only MAX
   { 0x80480208, 0x0001,    0,   10054,   10053,   10052,   10051 },// VBA fund only MAX
   { 0x80280208, 0x0002,    0,     387,     386,     385,     384 },// VA fund + harmonics MIN
   { 0x80300208, 0x0002,    0,   10059,   10058,   10057,   10056 },// VB fund + harmonics MIN
   { 0x80380208, 0x0002,    0,     438,     437,     436,     435 },// VC fund + harmonics MIN
   { 0x80680208, 0x0002,    0,   10064,   10063,   10062,   10061 },// VA fund only MIN
   { 0x80700208, 0x0002,    0,   10069,   10068,   10067,   10066 },// VB fund only MIN
   { 0x80780208, 0x0002,    0,   10074,   10073,   10072,   10071 },// VC fund only MIN
   { 0x80180208, 0x0002,    0,   10079,   10078,   10077,   10076 },// VAC fund + harmonics MIN
   { 0x80100208, 0x0002,    0,   10084,   10083,   10082,   10081 },// VCB fund + harmonics MIN
   { 0x80080208, 0x0002,    0,   10089,   10088,   10087,   10086 },// VBA fund + harmonics MIN
   { 0x80580208, 0x0002,    0,   10094,   10093,   10092,   10091 },// VAC fund only MIN
   { 0x80500208, 0x0002,    0,   10099,   10098,   10097,   10096 },// VCB fund only MIN
   { 0x80480208, 0x0002,    0,   10104,   10103,   10102,   10101 },// VBA fund only MIN
   { 0x80280208, 0x0003,    0,      34,      34,      34,      34 },// VA fund + harmonics SNAPSHOT
   { 0x80300208, 0x0003,    0,      36,      36,      36,      36 },// VB fund + harmonics SNAPSHOT
   { 0x80380208, 0x0003,    0,      35,      35,      35,      35 },// VC fund + harmonics SNAPSHOT
   { 0x80680208, 0x0003,    0,   10106,   10106,   10106,   10106 },// VA fund only SNAPSHOT
   { 0x80700208, 0x0003,    0,   10107,   10107,   10107,   10107 },// VB fund only SNAPSHOT
   { 0x80780208, 0x0003,    0,   10108,   10108,   10108,   10108 },// VC fund only SNAPSHOT
   { 0x80180208, 0x0003,    0,     692,     692,     692,     692 },// VAC fund + harmonics SNAPSHOT
   { 0x80100208, 0x0003,    0,     694,     694,     694,     694 },// VCB fund + harmonics SNAPSHOT
   { 0x80080208, 0x0003,    0,     693,     693,     693,     693 },// VBA fund + harmonics SNAPSHOT
   { 0x80580208, 0x0003,    0,     695,     695,     695,     695 },// VAC fund only SNAPSHOT
   { 0x80500208, 0x0003,    0,     697,     697,     697,     697 },// VCB fund only SNAPSHOT
   { 0x80480208, 0x0003,    0,     696,     696,     696,     696 },// VBA fund only SNAPSHOT
   { 0x8020020C, 0x0001,    0,   10112,   10111,   10110,   10109 },// In fund + harmonics MAX
   { 0x8028020C, 0x0001,    0,     402,     401,     400,     399 },// IA fund + harmonics MAX
   { 0x8030020C, 0x0001,    0,   10117,   10116,   10115,   10114 },// IB fund + harmonics MAX
   { 0x8038020C, 0x0001,    0,     417,     416,     415,     414 },// IC fund + harmonics MAX
   { 0x8068020C, 0x0001,    0,   10122,   10121,   10120,   10119 },// IA fund only MAX
   { 0x8070020C, 0x0001,    0,   10127,   10126,   10125,   10124 },// IB fund only MAX
   { 0x8078020C, 0x0001,    0,   10132,   10131,   10130,   10129 },// IC fund only MAX
   { 0x8020020C, 0x0002,    0,   10137,   10136,   10135,   10134 },// In fund + harmonics MIN
   { 0x8028020C, 0x0002,    0,     407,     406,     405,     404 },// IA fund + harmonics MIN
   { 0x8030020C, 0x0002,    0,   10142,   10141,   10140,   10139 },// IB fund + harmonics MIN
   { 0x8038020C, 0x0002,    0,     422,     421,     420,     419 },// IC fund + harmonics MIN
   { 0x8068020C, 0x0002,    0,   10147,   10146,   10145,   10144 },// IA fund only MIN
   { 0x8070020C, 0x0002,    0,   10152,   10151,   10150,   10149 },// IB fund only MIN
   { 0x8078020C, 0x0002,    0,   10157,   10156,   10155,   10154 },// IC fund only MIN
   { 0x8020020C, 0x0003,    0,      26,      26,      26,      26 },// In fund + harmonics SNAPSHOT
   { 0x8028020C, 0x0003,    0,      25,      25,      25,      25 },// IA fund + harmonics SNAPSHOT
   { 0x8030020C, 0x0003,    0,      28,      28,      28,      28 },// IB fund + harmonics SNAPSHOT
   { 0x8038020C, 0x0003,    0,      27,      27,      27,      27 },// IC fund + harmonics SNAPSHOT
   { 0x8068020C, 0x0003,    0,     698,     698,     698,     698 },// IA fund only SNAPSHOT
   { 0x8070020C, 0x0003,    0,     699,     699,     699,     699 },// IB fund only SNAPSHOT
   { 0x8078020C, 0x0003,    0,     700,     700,     700,     700 },// IC fund only SNAPSHOT
   { 0x80280209, 0x0004,    0,     164,     161,     375,     374 },// Average VA fund + harmonics
   { 0x80300209, 0x0004,    0,     166,     163,    3059,    3058 },// Average VB fund + harmonics
   { 0x80380209, 0x0004,    0,     165,     162,     433,     434 },// Average VC fund + harmonics
   { 0x80680209, 0x0004,    0,    3063,    3062,    3061,    3060 },// Average VA fund only
   { 0x80700209, 0x0004,    0,    3068,    3067,    3066,    3065 },// Average VB fund only
   { 0x80780209, 0x0004,    0,    3073,    3072,    3071,    3070 },// Average VC fund only
   { 0x80180209, 0x0004,    0,    3078,    3077,    3076,    3075 },// Average VAC fund + harmonics
   { 0x80100209, 0x0004,    0,    3083,    3082,    3081,    3080 },// Average VCB fund + harmonics
   { 0x80080209, 0x0004,    0,    3088,    3087,    3086,    3085 },// Average VBA fund + harmonics
   { 0x80580209, 0x0004,    0,    3093,    3092,    3091,    3090 },// Average VAC fund only
   { 0x80500209, 0x0004,    0,    3098,    3097,    3096,    3095 },// Average VCB fund only
   { 0x80480209, 0x0004,    0,    3103,    3102,    3101,    3100 },// Average VBA fund only
   { 0x0068050E, 0x0000,    0,    3108,    3107,    3106,    3105 },// I2h, Element A, fund (LP)
   { 0x0070050E, 0x0000,    0,    3113,    3112,    3111,    3110 },// I2h, Element B, fund (LP)
   { 0x0038050E, 0x0000,    0,    3118,    3117,    3116,    3115 },// I2h, Element C, fund (LP)
   { 0x0028050E, 0x0000,    0,    3123,    3122,    3121,    3120 },// I2h, Element A, fund + harmonics (LP)
   { 0x0030050E, 0x0000,    0,    3128,    3127,    3126,    3125 },// I2h, Element B, fund + harmonics (LP)
   { 0x0078050E, 0x0000,    0,    3133,    3132,    3131,    3130 },// I2h, Element C, fund + harmonics (LP)
   { 0x0020050E, 0x0000,    0,    3138,    3137,    3136,    3135 },// In2h (LP)
   { 0x8028020D, 0x0004,    0,     412,     411,     410,     409 },// Average IA fund + harmonics
   { 0x8030020D, 0x0004,    0,    3143,    3142,    3141,    3140 },// Average IB fund + harmonics
   { 0x8038020D, 0x0004,    0,     427,     426,     425,     424 },// Average IC fund + harmonics
   { 0x8068020D, 0x0004,    0,    3148,    3147,    3146,    3145 },// Average IA fund only
   { 0x8070020D, 0x0004,    0,    3153,    3152,    3151,    3150 },// Average IB fund only
   { 0x8078020D, 0x0004,    0,    3158,    3157,    3156,    3155 },// Average IC fund only
   { 0x802BC218, 0x0001,    0,     858,     857,     856,     855 },// Distortion PF, Element A MAX
   { 0x802BC218, 0x0002,    0,     863,     862,     861,     860 },// Distortion PF, Element A MIN
   { 0x802BC218, 0x0003,    0,     868,     867,     866,     865 },// Distortion PF, Element A SNAPSHOT
   { 0x8033C218, 0x0001,    0,     873,     872,     871,     870 },// Distortion PF, element B MAX
   { 0x8033C218, 0x0002,    0,     878,     877,     876,     875 },// Distortion PF, element B MIN
   { 0x8033C218, 0x0003,    0,     883,     882,     881,     880 },// Distortion PF, element B SNAPSHOT
   { 0x803BC218, 0x0001,    0,     888,     887,     886,     885 },// Distortion PF, Element C MAX
   { 0x803BC218, 0x0002,    0,     893,     892,     891,     890 },// Distortion PF, Element C MIN
   { 0x803BC218, 0x0003,    0,     898,     897,     896,     895 },// Distortion PF, Element C SNAPSHOT
   { 0x8003C218, 0x0001,    0,     903,     902,     901,     900 },// Distortion PF MAX
   { 0x8003C218, 0x0002,    0,     908,     907,     906,     905 },// Distortion PF MIN
   { 0x8003C218, 0x0003,    0,     913,     912,     911,     910 },// Distortion PF SNAPSHOT
   { 0x80280210, 0x0001,    0,     918,     917,     916,     915 },// VTHD, Element A MAX
   { 0x80280210, 0x0002,    0,     923,     922,     921,     920 },// VTHD, Element A MIN
   { 0x80280210, 0x0003,    0,     928,     927,     926,     925 },// VTHD, Element A SNAPSHOT
   { 0x80300210, 0x0001,    0,     933,     932,     931,     930 },// VTHD, Element B MAX
   { 0x80300210, 0x0002,    0,     938,     937,     936,     935 },// VTHD, Element B MIN
   { 0x80300210, 0x0003,    0,     943,     942,     941,     940 },// VTHD, Element B SNAPSHOT
   { 0x80380210, 0x0001,    0,     948,     947,     946,     945 },// VTHD, Element C MAX
   { 0x80380210, 0x0002,    0,     953,     952,     951,     950 },// VTHD, Element C MIN
   { 0x80380210, 0x0003,    0,     958,     957,     956,     955 },// VTHD, Element C SNAPSHOT
   { 0x80280211, 0x0001,    0,     963,     962,     961,     960 },// ITHD, Element A MAx
   { 0x80280211, 0x0002,    0,     968,     967,     966,     965 },// ITHD, Element A MIN
   { 0x80280211, 0x0003,    0,     973,     972,     971,     970 },// ITHD, Element A SNAPSHOT
   { 0x80300211, 0x0001,    0,     978,     977,     976,     975 },// ITHD, Element B MAX
   { 0x80300211, 0x0002,    0,     983,     982,     981,     980 },// ITHD, Element B MIN
   { 0x80300211, 0x0003,    0,     988,     987,     986,     985 },// ITHD, Element B SNAPSHOT
   { 0x80380211, 0x0001,    0,     993,     992,     991,     990 },// ITHD, Element C MAX
   { 0x80380211, 0x0002,    0,     998,     997,     996,     995 },// ITHD, Element C MIN
   { 0x80380211, 0x0003,    0,    1003,    1002,    1001,    1000 },// ITHD, Element C SNAPSHOT
   { 0x80280211, 0x0101,    0,    1008,    1007,    1006,    1005 },// TDD, Element A MAX
   { 0x80280211, 0x0102,    0,    1013,    1012,    1011,    1010 },// TDD, Element A MIN
   { 0x80280211, 0x0103,    0,    1018,    1017,    1016,    1015 },// TDD, Element A SNAPSHOT
   { 0x80300211, 0x0101,    0,    9889,    9888,    9887,    9886 },// TDD, Element B MAX
   { 0x80300211, 0x0102,    0,    9894,    9893,    9892,    9891 },// TDD, Element B MIN
   { 0x80300211, 0x0103,    0,    9899,    9898,    9897,    9896 },// TDD, Element B SNAPSHOT
   { 0x80380211, 0x0101,    0,    9904,    9903,    9902,    9901 },// TDD, Element C MAX
   { 0x80380211, 0x0102,    0,    9909,    9908,    9907,    9906 },// TDD, Element C MIN
   { 0x80380211, 0x0103,    0,    9914,    9913,    9912,    9911 },// TDD, Element C SNAPSHOT
   { 0x0003C504, 0x0000,    0,      77,      68,     124,     115 },// Qh (LP)
   { 0x80000221, 0x0001,    0,    9919,    9918,    9917,    9916 },// Frequency MAX
   { 0x80000221, 0x0002,    0,    9924,    9923,    9922,    9921 },// Frequency MIN
   { 0x80000221, 0x0003,    0,      23,      23,      23,      23 },// Frequency SNAPSHOT
   { 0x80000522, 0x0000,    0,    3164,    3163,    3162,    3161 },// Count (Pulses - LP)
   { 0x80000522, 0x0001,    0,    3169,    3168,    3167,    3166 },// Count (Pulses - LP)
   { 0x80000522, 0x0002,    0,    3174,    3173,    3172,    3171 },// Count (Pulses - LP)
   { 0x80000522, 0x0003,    0,    3179,    3178,    3177,    3176 },// Count (Pulses - LP)
   { 0x80000246, 0x0001,    0,     392,     391,     390,     389 },// Temperature
   { 0x80000246, 0x0002,    0,     397,     396,     395,     394 },// Temperature
   { 0x80000246, 0x0003,    0,      32,      32,      32,      32 },// Temperature
   { 0x80000246, 0x0004,    0,     369,     368,     367,     366 },// Temperature
   { 0x800000C3, 0x0003,    0,    3181,    3181,    3181,    3181 } // RSSI dBm
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
#endif // #if HMC_KV

