/***********************************************************************************************************************

   Filename: ansi_tables.h

   Contents: Defines many of the standard ANSI tables.

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2013-2021 Aclara. All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara). This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara. Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   $Log$ kad Created 073107

 ***********************************************************************************************************************
   Revision History:
   v1.0 - Initial Release

 **********************************************************************************************************************/

#ifndef ansi_tables_H
#define ansi_tables_H
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "psem.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* Ansi Table Names */
/* Note: Many of these are larger than 31 characters. */
#define STD_TBL_GENERAL_CONFIGURATION             ((uint16_t)0)
#define STD_TBL_GENERAL_MANUFACTURE_ID            ((uint16_t)1)
#define STD_TBL_DEVICE_NAMEPLATE                  ((uint16_t)2)
#define STD_TBL_END_DEVICE_MODE_STATUS            ((uint16_t)3)
#define STD_TBL_PENDING_STATUS                    ((uint16_t)4)
#define STD_TBL_DEVICE_IDENTIFICATION             ((uint16_t)5)
#define STD_TBL_UTILITY_INFORMATION               ((uint16_t)6)
#define STD_TBL_PROCEDURE_INITIATE                ((uint16_t)7)
#define STD_TBL_PROCEDURE_RESPONSE                ((uint16_t)8)
#define STD_TBL_DATA_SOURCE_DIMENSION_LIMITS      ((uint16_t)10)
#define STD_TBL_ACTUAL_DATA_SOURCES_LIMITING      ((uint16_t)11)
#define STD_TBL_UNITS_OF_MEASURE_ENTRY            ((uint16_t)12)
#define STD_TBL_DEMAND_CONTROL                    ((uint16_t)13)
#define STD_TBL_DATA_CONTROL                      ((uint16_t)14)
#define STD_TBL_CONSTANTS                         ((uint16_t)15)
#define STD_TBL_SOURCE_DEFINTION                  ((uint16_t)16)
#define STD_TBL_TRANSFORMER_LOSS_COMPENSATION     ((uint16_t)17)
#define STD_TBL_REGISTER_DIMENSION_LIMITS         ((uint16_t)20)
#define STD_TBL_ACTUAL_REGISTER                   ((uint16_t)21)
#define STD_TBL_DATA_SELECTION                    ((uint16_t)22)
#define STD_TBL_CURRENT_REGISTER_DATA             ((uint16_t)23)
#define STD_TBL_PREVIOUS_SEASON_DATA              ((uint16_t)24)
#define STD_TBL_PREVIOUS_DEMAND_RESET_DATA        ((uint16_t)25)
#define STD_TBL_SELF_READ_DATA                    ((uint16_t)26)
#define STD_TBL_PRESENT_REGISTER_SELECTION        ((uint16_t)27)
#define STD_TBL_PRESENT_REGISTER_DATA             ((uint16_t)28)
#define STD_TBL_DISPLAY_DIMENSION_LIMITS          ((uint16_t)30)
#define STD_TBL_ACTUAL_DISPLAY_LIMITING           ((uint16_t)31)
#define STD_TBL_DISPLAY_SOURCE                    ((uint16_t)32)
#define STD_TBL_PRIMARY_DISPLAY_LIST              ((uint16_t)33)
#define STD_TBL_SECONDARY_DISPLAY_LIST            ((uint16_t)34)
#define STD_TBL_SECURITY_DIMENSIONS_LIMITS        ((uint16_t)40)
#define STD_TBL_ACTUAL_SECURITY_LIMITING          ((uint16_t)41)
#define STD_TBL_SECURITY                          ((uint16_t)42)
#define STD_TBL_DEFAULT_ACCESS_CONTROL            ((uint16_t)43)
#define STD_TBL_ACCESS_CONTROL                    ((uint16_t)44)
#define STD_TBL_KEY                               ((uint16_t)45)
#define STD_TBL_TIME_AND_TOU_DIMINSION_LIMITS     ((uint16_t)50)
#define STD_TBL_ACTUAL_TIME_AND_TOU_LIMING        ((uint16_t)51)
#define STD_TBL_CLOCK                             ((uint16_t)52)
#define STD_TBL_TIME_OFFSET                       ((uint16_t)53)
#define STD_TBL_CALENDAR                          ((uint16_t)54)
#define STD_TBL_CLOCK_STATE                       ((uint16_t)55)
#define STD_TBL_TIME_REMAINING                    ((uint16_t)56)
#define STD_TBL_PRECISION_CLOCK_STATE             ((uint16_t)57)
#define STD_TBL_LOAD_PROFILE_DIMINSION_LIMITS     ((uint16_t)60)
#define STD_TBL_ACTUAL_LOAD_PROFILE_LIMITING      ((uint16_t)61)
#define STD_TBL_LOAD_PROFILE_CONTROL              ((uint16_t)62)
#define STD_TBL_LOAD_PROFILE_STATUS               ((uint16_t)63)
#define STD_TBL_LOAD_PROFILE_DATASET_1            ((uint16_t)64)
#define STD_TBL_LOAD_PROFILE_DATASET_2            ((uint16_t)65)
#define STD_TBL_LOAD_PROFILE_DATASET_3            ((uint16_t)66)
#define STD_TBL_LOAD_PROFILE_DATASET_4            ((uint16_t)67)
#define STD_TBL_LOG_DIMINSION_LIMITS              ((uint16_t)70)
#define STD_TBL_ACTUAL_LOG_LIMITING               ((uint16_t)71)
#define STD_TBL_EVENTS_IDENTIFICATION             ((uint16_t)72)
#define STD_TBL_HISTORY_LOG_CONTROL               ((uint16_t)73)
#define STD_TBL_HISTORY_LOG                       ((uint16_t)74)
#define STD_TBL_EVENT_LOG_CONTROL                 ((uint16_t)75)
#define STD_TBL_EVENT_LOG_DATA                    ((uint16_t)76)
#define STD_TBL_EVENT_LOG_AND_SIGNATURES_ENABLE   ((uint16_t)77)
#define STD_TBL_END_DEVICE_PROGRAM_STATE          ((uint16_t)78)
#define STD_TBL_EVENT_COUNTERS                    ((uint16_t)79)
#define INVALID_TABLE_INDICATOR                   ((uint16_t)0xFFFF) /*Indicates invalid table ID.  This is a valid table ID valid per PSEM, but we will never use */

/* Ansi Procedure Names */
#define STD_PROC_COLD_START                       ((uint16_t)0)
#define STD_PROC_WARM_START                       ((uint16_t)1)
#define STD_PROC_SAVE_CONFIGURATION               ((uint16_t)2)
#define STD_PROC_CLEAR_DATA                       ((uint16_t)3)
#define STD_PROC_RESET_LIST_POINTERS              ((uint16_t)4)
#define STD_PROC_UPDATE_LAST_READ_ENTRY           ((uint16_t)5)
#define STD_PROC_CHANGE_END_DEVICE_MODE           ((uint16_t)6)
#define STD_PROC_CLEAR_STANDARD_STATUS_FLAGS      ((uint16_t)7)
#define STD_PROC_CLEAR_MANUFACTURER_STATUS_FLAGS  ((uint16_t)8)
#define STD_PROC_REMOTE_RESET                     ((uint16_t)9)
#define STD_PROC_SET_DATE_AND_OR_TIME             ((uint16_t)10)
#define STD_PROC_EXECUTE_DIAG_PROCEDURE           ((uint16_t)11)
#define STD_PROC_ACTIVATE_ALL_PENDING_TABLES      ((uint16_t)12)
#define STD_PROC_ACTIVATE_SPECIFIC_PENDING_TABLES ((uint16_t)13)
#define STD_PROC_CLEAR_ALL_PENDING_TABLES         ((uint16_t)14)
#define STD_PROC_CLEAR_SPECIFIC_PENDING_TABLES    ((uint16_t)15)
#define STD_PROC_START_LOAD_PROFILE               ((uint16_t)16)
#define STD_PROC_STOP_LOAD_PROFILE                ((uint16_t)17)
#define STD_PROC_LOG_IN                           ((uint16_t)18)
#define STD_PROC_LOG_OUT                          ((uint16_t)19)
#define STD_PROC_INITIATE_AN_IMMEDIATE_CALL       ((uint16_t)20)
#define STD_PROC_DIRECT_LOAD_CONTROL              ((uint16_t)21)
#define STD_PROC_MODIFY_CREDIT                    ((uint16_t)22)
#define STD_PROC_CLEAR_PENDING_CALL_STATUS        ((uint16_t)27)
#define STD_PROC_START_QUALITY_OF_SERVICE_MONITORS ((uint16_t)28)
#define STD_PROC_STOP_QUALITY_OF_SERVICE_MONITORS ((uint16_t)29)
#define STD_PROC_START_SECURED_REGISTER           ((uint16_t)30)
#define STD_PROC_STOP_SECURED_REGISTER            ((uint16_t)31)
#define STD_PROC_SET_PRECISION_DATE_AND_OR_TIME   ((uint16_t)32)

/* Procedure RESULT_CODE Code which identifies the status of the proc execution. The codes are defined as follows: */
#define PROC_RESULT_COMPLETED                ((uint8_t)0) /* Procedure completed. */
#define PROC_RESULT_NOT_COMPLETED            ((uint8_t)1) /* Procedure accepted but not fully completed. */
#define PROC_RESULT_INV_PARAM                ((uint8_t)2) /* Invalid parameter for known procedure, procedure was ignored. */
#define PROC_RESULT_CONFLICT_SET_UP          ((uint8_t)3) /* Procedure conflicts with current device setup, proc was ignored. */
#define PROC_RESULT_TIMING_CONSTRAINT        ((uint8_t)4) /* Timing constraint, procedure was ignored. */
#define PROC_RESULT_AUTHORIZATION            ((uint8_t)5) /* No authorization for requested procedure, procedure was ignored. */
#define PROC_RESULT_UNRECOGNIZED             ((uint8_t)6) /* Un-recognized procedure, procedure was ignored. */

// ST23 specific items
#define ST23_DEMANDS_EVENTTIME_ENABLED       1
#define ST23_DEMANDS_EVENTTIME_CNT           ((uint8_t)25)
#define ST23_DEMANDS_CONTCUMDEMAND_ENABLED   0
#define ST23_DEMANDS_CONTCUMDEMAND_CNT       ((uint8_t)30)
#define ST23_DEMANDS_CONTCUMDEMAND_SIZE      6
#define ST23_DEMANDS_DEMAND_CNT              ((uint8_t)20)
#define ST23_SUMMATIONS_CNT                  ((uint8_t)5)
#define ST23_SUMMATIONS_SIZE                 6
#define ST23_DEMANDS_CNT                     ((uint8_t)5)
#define ST23_DEMANDS_SIZE                    4
#define ST23_COINCIDENTS_CNT                 ((uint8_t)10)

#define  WILD     ((bool)true)
#define  MATCH    ((bool)false)

#define INVALID_TBL_22_INDEX ((uint8_t)0xFF) /* This is per the ANSI C12 Standard */

#define METER_PROGRAM_INFO_SIZE (17)         /* Number of bytes in Meter Programmed information (KV2C)   */
#define ACLARA_STDTBL14_MAX   1
#define ACLARA_STDTBL14_MIN   2
#define ACLARA_STDTBL14_SNAP  3
#define ACLARA_STDTBL14_AVG   4

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef returnStatus_t( *eFptrAnsi_pup )( uint8_t *, uint8_t, uint8_t * );

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t year;    /* 00..89 Years 2000..2089, 90..99 Years 1990..1999, > 100 Reserved. */
   uint8_t month;   /* 0 Reserved, 1..12 Month of year, > 12 Reserved. */
   uint8_t day;     /* 0 Unassigned, 1..31 Day of month, > 31 Reserved. */
   uint8_t hour;    /* 00..23 Hour of day, 24 hour basis, > 23 Reserved. */
   uint8_t minute;  /* 00..59 Minute of hour, > 59 Reserved. */
} MtrTimeFormatLowPrecision_t;  /* Low-precision (minutes) time stamp. This is the prototype definition of the built-in type */
PACK_END
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t year;                      /* Year to send to the host meter. The year starts at 2000, 0 = year 2000. */
   uint8_t month;                     /* Month to send to the host meter. */
   uint8_t day;                       /* Day of the month to send to the host meter. */
   uint8_t hours;                     /* Hour of the day to send to the host meter. */
   uint8_t minutes;                   /* Minutes of the day to send to the host meter. */
   uint8_t seconds;                   /* Seconds of the day to send to the host meter. */
} MtrTimeFormatHighPrecision_t;
PACK_END
/* ------------------------------------------------------------------------------------------------------------------ */
/* Used by STD Table #0 */

/* Element to define order of multi-byte numeric data transfer. 0 Least significant byte 1st. 1 Most significant byte
   1st. */
typedef enum
{
   eDATA_ORDER_LITTLE_ENDIAN = (uint8_t)0,
   eDATA_ORDER_BIG_ENDIAN
} dataOrder_t;
/* 6.3 Date and Time Formats - This selection is used to determine the structure of dates and times used in the tables.
   The Element GEN_CONFIG_TBL.FORMAT_CONTROL_2.TM_FORMAT is the controlling selector for HTIME_DATE, LTIME_DATE,
   STIME_DATE, TIME, STIME and HTIME. */
typedef enum
{
   eCLKFMT_NOT_PRESENT = (uint8_t) 0,  /* No clock in the End Device */
   eCLKFMT_BCD,                        /* BCD type with discrete fields for year, month, day, hour, minute seconds and fractional seconds. */
   eCLKFMT_UINT8,                      /* UINT8 type with discrete fields for year, month, day, hour, minute seconds and fractional seconds. */
   eCLKFMT_UINT32_MIN,                 /* UINT32 counters where HTIME_DATE, LTIME_DATE and STIME_DATE types are encoded relative to 01/01/1970 @
                                          00:00:00 UTC, with discrete fields for minutes and fraction of a minute. */
   eCLKFMT_UINT32,                     /* UINT32 counters where HTIME_DATE, LTIME_DATE and STIME_DATE types are encoded relative to 01/01/1970 @
                                          00:00:00 UTC), with discrete fields for seconds and fraction of a second. */
   eCLK_ERROR5,                        /* */
   eCLK_ERROR6,
   eCLK_ERROR7,
} tmFormatEnum_t;    /* Values that may be assumed by GEN_CONFIG_TBL.FORMAT_CONTROL_2. TM_FORMAT (which controls the
                        interpretation of built-in types HTIME_DATE, LTIME_DATE, STIME_DATE, TIME, STIME and HTIME). */
typedef enum
{
   eDAM_FULL  = (uint8_t)0,   /* Only complete tables can be mapped into user tables. Partial tables cannot be mapped. */
   eDAM_PARTIAL,              /* Offset-count access method is supported. */
   eDAM_IDX_CNT,              /* Index-count access method is supported. */
   eDAM_FULL_PARTIAL          /* Access methods 2 (FULL) and 3 (PARTIAL) are supported. */
} dataAccessMethod_t;         /* Element used to designate possible methods of selecting table entries for placement in a user- defined table. */
/* See ANSI C12.19, Section 6.2. */
typedef enum
{
   eNIFMT_FLOAT64 = (uint8_t)0,  /* */
   eNIFMT_FLOAT32,               /* */
   eNIFMT_CHAR12,                /* FLOAT_CHAR12 (A STRING Number) */
   eNIFMT_CHAR6,                 /* FLOAT_CHAR6 (A STRING Number) */
   eNIFMT_INT32DP,               /* INT32 (Implied decimal point between fourth and fifth digits from least significant digit. For example 0.0001 is
                                    represented as 1) */
   eNIFMT_FIXED_BCD6,            /* */
   eNIFMT_FIXED_BCD4,            /* */
   eNIFMT_INT24,                 /* */
   eNIFMT_INT32,                 /* */
   eNIFMT_INT40,                 /* */
   eNIFMT_INT48,                 /* */
   eNIFMT_INT64,                 /* */
   eNIFMT_FIXED_BCD8,            /* */
   eNIFMT_FLOAT_CHAR21,          /* FLOAT_CHAR21 (A STRING Number) */
   eNIFMT_RSVD14,                /* */
   eNIFMT_RSVD15                 /* */
} niFormatEnum_t;                /* Values that may be assumed by GEN_CONFIG_TBL.FORMAT_CONTROL_3.N I_FORMAT1 (which controls the interpretation of
                                    the built-in type NI_FMAT1) and GEN_CONFIG_TBL.FORMAT_CONTROL_3.N I_FORMAT2 (which controls the interpretation of
                                    the built-in type NI_FMAT2). */

/* When the ID_CODE field represents the electric utility industry unit of measures this bit indicates phase measurement
   associations. When the ID_CODE does not represent an electric utility unit of measure then this field represents an a
   yet to be defined quantity, except for the value 0, which stands for all sources and flows. In this case the value
   should be set to zero. */
typedef enum
{
   eALL_PHASES = ( (uint8_t)0 ), /* Measurement is not a phase related or no phase information is applicable. e.g.,
                                    All phases on a polyphase End Device */
   ePHASE_AB,                    /* Phase A to B. i.e. A-B. */
   ePHASE_BC,                    /* Phase B to C. i.e. B-C. */
   ePHASE_CA,                    /* Phase C to A. i.e. C-A. */
   eNEUTRAL_TO_GND,              /* Neutral to ground, or no phase information. e.g., Neutral current in a 4Y wire sys. */
   ePHASE_AN,                    /* Phase A to Neutral. i.e. A-N. */
   ePHASE_BN,                    /* Phase B to Neutral. i.e. B-N. */
   ePHASE_CN                     /* Phase C to Neutral. i.e. C-N. */
   /* When the ID_CODE does not represent an electric utility unit of measure then this field represents an as yet to be
      defined quantity, except for the value 0, which stands for all sources and flows. */
} segmentation_t;
/* ------------------------------------------------------------------------------------------------------------------ */
/* Used by STD Table #12 */

typedef enum
{
   /* Power */
   eActivePower_W = (uint8_t)0,     /*   (0) Active power W. */
   eReactivePower_VAR,              /*   (1) Reactive power VAR. */
   eApparentPower_VA,               /*   (2) Apparent power VA, e.g., Result of IRMS x VRMS. */
   ePhasorPower_VA,                 /*   (3) Phasor power - VA = sqrt (W2+VAR2). */
   eQuantityPower_Q60,              /*   (4) Quantity power - Q(60). */
   eQuantityPower_Q45,              /*   (5) Quantity power - Q(45). */
   ePhasorReactivePower_RMS,        /*   (6) Phasor Reactive Power - RMS. This is computed as square root (VA2 - W2), where W is associated with
                                             UOM.ID_CODE 0 and VA is associated with UOM.ID_CODE 2. This term (UOM.ID_CODE 6) includes the distortion
                                             voltamperes quantity D defined by VA2 = W2 + VAR2 + D2. */
   eDistortionVoltAmps_RMS,         /*   (7) Distortion volt-amperes - RMS. Defined as VA2 = W2 + VAR2 + D2, where VA is selected by UOM.ID_CODE 2
                                             (VRMS x IRMS), W is selected by UOM.ID_CODE 0 and VAR is selected by UOM.ID_CODE 1. The distortion D can
                                             also be computed (when D is not reported directly by the meter via (new) UOM.ID_CODE 7) from UOM.ID_CODE
                                             2 and UOM.ID_CODE 3 as follows: D2 = VA2 (UOM.ID_CODE 2) - VA2 (UOM.ID_CODE 3). */

   /* Volts */
   eRmsVolts,                       /*   (8) RMS Volts. */
   eAveVolts,                       /*   (9) Average Volts (average of |V|). */
   eRmsVoltsSquard,                 /*  (10) RMS Volts Squared (V2). */
   eInstantaneousVolts,             /*  (11) Instantaneous volts. */

   /* Current */
   eRmsAmps,                        /*  (12) RMS Amps. */
   eAverageCurrent,                 /*  (13) Average Current (average of |I|). */
   eRmsAmpsSquared,                 /*  (14) RMS Amps Squared (I2). */
   eInstantaneousCurrent,           /*  (15) Instantaneous current. */

   /* Percent Total Harmonic Distortion */
   eThdV_IEEE,                      /*  (16) T.H.D. V (IEEE Institute of Electrical and Electronics Engineers). */
   eThdI_IEEE,                      /*  (17) T.H.D. I (IEEE). */
   eThdV_IC,                        /*  (18) T.H.D. V (IC Industry Canada). */
   eThdI_IC,                        /*  (19) T.H.D. I (IC). */

   /* Phase Angle */
   ePa_VVA,                         /*  (20) V-VA, Voltage phase angle. */
   ePa_VxVy,                        /*  (21) Vx-Vy, where x and y are phases defined in phase selector. */
   ePa_IVA,                         /*  (22) I-VA, Current phase angle. */
   ePa_IxIy,                        /*  (23) Ix-Iy, where x and y are phases defined in phase selector. */
   ePa_PwrFactorAP,                 /*  (24) Power factor computed using apparent power, UOM.ID_CODE=2. */
   ePa_PwrFactorPP,                 /*  (25) Power factor computed using phasor power, UOM.ID_CODE=3. */
   ePa_AvePwrFactor,                /*  (26) Average Power Factor. */

   /* Time */
   eTimeOfDay = (uint8_t)29,        /*  (29) Time of Day */
   eDate,                           /*  (30) */
   eTimeOfDayandDate,               /*  (31) */
   eIntervalTimer,                  /*  (32) */
   eFrequency,                      /*  (33) */
   eCounter,                        /*  (34) */
   eSenseInput,                     /*  (35) Sense input (true/on or false/off). */
   ePulseOutputFormA,               /*  (36) Pulse output Form A. Single Pole Single Throw (SPST), Normally Open. */
   ePulseOutputFormB,               /*  (37) Pulse output Form B. Single Pole Single Throw (SPST), Normally Closed. */
   ePulseOutputFormC,               /*  (38) Pulse output Form C. Single Pole Double Throw (SPDT), also referred to as a changeover switch. The form C
                                             has 2 contacts, one that opens and one that closes. */
   eZeroFlowDuration,               /*  (39) Zero Flow Duration (Condition which is normally timed for diagnostics). */
   eVoltageSag,                     /*  (40) */
   eVoltageSwell,                   /*  (41) */
   ePowerOutage,                    /*  (42) Power outage (system mains). */
   eVoltageExcursionLow,            /*  (43) */
   eVoltageExcursionHigh,           /*  (44) */
   eNormalVoltageLevel,             /*  (45) */
   ePhaseVoltagePhaseImbalance,     /*  (46) */
   eVoltageThdExcess,               /*  (47) */
   eCurrentThdExcess,               /*  (48) */
   ePowerOutageEndDevice,           /*  (49) Pwr outage (End Device) device may be pwrd separately from pwr being monitored. */

   /* Event Counters */
   eNumOfPwrOutages,                /*  (50) */
   eNumOfDemandResets,              /*  (51) */
   eNumOfTimesProg,                 /*  (52) */
   eNumOfMinOnBatCarryover,         /*  (53) */
   eNumOfInversionTampers,          /*  (54) */
   eNumOfRemovalTampers,            /*  (55) */
   eNumOfReprogTampers,             /*  (56) */
   eNumOfPwrLossTampers,            /*  (57) */
   eNumOfRevRotationTampers,        /*  (58) */
   eNumOfPhysicalTampers,           /*  (59) */
   eNumOfEncoderTampers,            /*  (60) */
   eNumOfWdtRecoveriesED,           /*  (61) Number of watchdog timeout-recoveries the End Device has encountered. */
   eNumOfDiagAlarms,                /*  (62) */

   /* Gas Industry Units */
   /* Volume */
   /* Temperature */
   eTempDegreesC = (uint8_t)70,     /*  (70) */
   /* Energy */
   /* Pressure */
   /* Other */
   /* Water Industry Units */

   /* Generic Industry Units */
   eLocalCurrency = (uint8_t)190,   /* (190) */
   eInch,                           /* (191) */
   eFoot,                           /* (192) */
   eMeter,                          /* (193) */
   eDifferentialValue,              /* (194) */
   eDBm,                            /* (195) */
   ePercentCapacity,                /* (196) */
   eSeconds,                        /* (197) */
   eAngleInDegrees,                 /* (198) */
   eFrequencyInHertz,               /* (199) */
   eBandwidthInHertz,               /* (200) */
   eLastIdCode = eBandwidthInHertz

   /* HVAC Industry */

} uomIdCodes_t;
typedef enum
{
   eBulkQuantityOfCom = (uint8_t)0, /* (0) Bulk Quantity of Commodity (Dial Reading). Quantity of commodity; integral of commodity usage rate.  Values
                                           have the units stated in the ID_CODE x Hour (Energy units). */
   eInstantaneous,                  /* (1) Sampled; This is the fastest rate at which a measurement is acquired. */
   ePeriodBased,                    /* (2) Period based. This is a time period based upon the period of a fundamental frequency (Power, RMS). */
   eSubBlockAveDemand,              /* (3) Sub-block Average Demand. Sub-block Average Demand values are the most recent averaging demand subinterval
                                           values (Demand). */
   eBlockAveDemand,                 /* (4) Block Average Demand. Block Average Demand values may be either the average over a number of Sub-block
                                           Average Demand subintervals or the average over a single interval. The averaging period is typically
                                           greater or equal to the Sub-block Average Demand period (Demand). */
   eNetBulkQuantityOfCom,           /* (5) Net Bulk Quantity of Commodity (Relative Dial Reading). Quantity of commodity; integral of commodity usage
                                           rate over a specified period of time T1 to T2. T1 and T2 are quite arbitrary and defined by mechanisms and
                                           table driven schedules. Values have the units stated in the ID_CODE x Hour, e.g., W x h = Wh (Energy) */
   eThermalQuantity,                /* (6) Thermal quantity (Demand). */
   eEventQuantity                   /* (7) Event quantity (Number of occurrences of an identified item). */
} timeBase_t;
/* ------------------------------------------------------------------------------------------------------------------ */
/* Standard Table Definitions */
/* ------------------------------------------------------------------------------------------------------------------ */
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  dataOrder:        1; /* Transfer order of multibyte numeric data (0 = LSB first, 1 = MSB first) */
   uint8_t  charFormat:       3; /* char set to be used for tbl char data (0 = unassigned, 1 = 7-bit ISO, 2 = ISO
                                    Latin1) */
   uint8_t  modelSelect:      3; /* 0 = I/O model selects sources as an 8-bit index into the data control table (14)*/
   uint8_t  filler:           1;
   uint8_t  tmFormat:         3; /* Time fmt used (0 = no clk, 1= BCD, 2 = UINT8, 3 = UNIT32, sec since ref time) */
   uint8_t  dataAccessMethod: 2; /* Tbl access methods supported (0 = full tbl only, 1 = offset/cnt, 2 = index/count, 3
                                    = both 1 and 2) */
   uint8_t  idForm:           1; /* Format of meter ID (0 = character, 1 = BCD) */
   uint8_t  intFormat:        2; /* Fmt of signed integers (0 = twos complement, 1 = ones complement, 2 = sign/magnitude) */
   uint8_t  niFormat1:        4; /* Non-integer format specified in tables as NI_FORMAT1; 10 = INT48 */
   uint8_t  niFormat2:        4; /* Non-integer format specified in tables as NI_FORMAT2; 8 = INT32 */
   uint8_t  manufacturer[4];     /* Four-byte manufacturer code */
   uint8_t  nameplateType;       /* Used to select record strct for Std tbl 2, which is not supported. Defaults to 2 = Electric */
   uint8_t  defaultSetUsed;      /* Indicates which, if any, default sets are used in decade tables (0 = none) */
   uint8_t  maxProcParamLen;     /* Indicates which, if any, default sets are used in decade tables (0 = none) */
   uint8_t  maxRespDataLen;      /* Manufacturer-defined maximum length of procedure response data (Std. Table 8) */
   uint8_t  stdVersionNo;        /* Unsigned bin # designating ver of std tbls implemented (0 = pre-release, 1 = first released doc) */
   uint8_t  stdRevisionNo;       /* Unsigned binary number designating revision of standard tables implemented */
   uint8_t  dimStdTblsUsed;      /* # of bytes of btflds required to indicate which std tbls are present */
   uint8_t  dimMfgTblsUsed;      /* # of bytes of btflds required to indicate which Mfg. tlbs are present */
   uint8_t  dimStdProcUsed;      /* # of bytes of btflds required to indicate which Std. Procedures are present */
   uint8_t  dimMfgProcUsed;      /* # of bytes of btflds required to indicate which Mfg. Procedures are supported */
   uint8_t  dimMfgStatusUsed;    /* Number of bytes allocated for indicating manufacturer-defined status conditions */
   uint8_t  nbrPending;          /* Number of pending status sets in Pending Status Table  (Table 4) */
   uint8_t  stdTblsUsed[11];     /* Bit fields indicating which std tbls are supported in this device (0 = !supported, 1 = supported)  stdTblsUsed[0]
                                    bit 0 is ST0 */
   uint8_t  mfgTblsUsed[15];     /* Bit fields indicating which Mfg. tbls are supported in this device (0 = !supported, 1 = supported) mfgTblsUsed[0]
                                    bit 0 is MT0 */
   uint8_t  stdProcUsed[3];      /* Bit fields indicating which Std. Procedures are supported in this device 0 = !supported, 1 = supported) */
   uint8_t  mfgProcUsed[12];     /* Bit fields indicating which Mfg. Procedures are supported in this device (0 = !supported, 1 = supported) */
   uint8_t  stdTblsWrite[11];    /* Bit fields indicating which of the Standard Tables can be written in this device (0
                                    = !supported, 1 = supported) */
   uint8_t  mfgTblsWrite[15];    /* Bit fields indicating which of the Mfg. Tables can be written in this device (0 = !supported, 1 = supported) */
} stdTbl0_t;                     /* Standard Table #0 - General Configuration */
PACK_END
/* ------------------------------------------------------------------------------------------------------------------ */
PACK_BEGIN
typedef struct PACK_MID
{
   uint32_t manufacturer;        /* Four-byte manufacturer code, from ANSI C12.19, Annex A */
   uint8_t  edModel[8];          /* Model identifier of this device */
   uint8_t  hwVersionNumber;     /* Hardware version number; implies functional change; factory-programmed */
   uint8_t  hwRevisionNumber;    /* Hardware revision number; factory-programmed */
   uint8_t  fwVersionNumber;     /* Firmware version number; implies functional differences */
   uint8_t  fwRevisionNumber;    /* Firmware revision number; implies product corrections or improvements */
   uint8_t  mfgSerialNumber[16]; /* Factory-programmed unique serial number */
} stdTbl1_t;                     /* Standard Table #1 - General Manufacturer Identification */
PACK_END
/* ------------------------------------------------------------------------------------------------------------------ */

PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint8_t unprogrammedFlag:        1; /* Indicates device is in default (unprogrammed) state (0 = programmed, 1 = unprogrammed). */
      uint8_t configurationErrorFlag:  1; /* Not supported  */
      uint8_t selfChkErrorFlag:        1; /* Not supported  */
      uint8_t ramFailureFlag:          1; /* Device did/did not detect a RAM error (0 = no RAM error, 1 = RAM error) */
      uint8_t romFailureFlag:          1; /* Device did/did not detect a ROM error (0 = no ROM error, 1 = ROM error) */
      uint8_t nonvolMemFailureFlag:    1; /* Device did/did not detect a NVRAM error (0 = no error, 1 = error) */
      uint8_t clockErrorFlag:          1; /* Device did/did not detect a clock error (0 = no error, 1 = error) */
      uint8_t measurementErrorFlag:    1; /* Device did/did not detect a bad voltage reference. 0 = no error, 1 = error */
      uint8_t lowBatteryFlag:          1; /* Set when battery test shows low battery (0 = battery OK, 1 = low battery) */
      uint8_t lowLossPotentialFlag:    1; /* Device did/did not detect potential below predetermined value 0=False, 1=True */
      uint8_t demandOverloadFlag:      1; /* Device did/did not detect a demand threshold overload 0 = False, 1 = True */
      uint8_t powerFailureFlag:        1; /* Device did/did not detect a power failure. */
      uint8_t tamperDetectFlag:        1; /* Device did/did not detect possible meter inversion tamper   */
      uint8_t coverDetectFlag:         1; /* reserved */
      uint8_t fill:                    1; /* - */
      uint8_t highLineCurrDetectFlag:  1; /* Device did detect possible high line current event (0=False, 1 = True) */
   } bits;
   uint8_t bytes[ 2 ];
} edStdStatus1Bfld_t;                  /* */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t unused;
} edStdStatus2Bfld_t;                  /* */
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint8_t dspError:       1; /* See MT 69 for cause of error */
      uint8_t timeChanged:    1; /* Set whenever time/date is set in the device. Applies only to external events, not DST time changes.  */
      uint8_t systemError:    1; /* Device did/did not detect expiration of the watchdog timer or some other abnormal execution sequence. (0 = False,
                                    1 = True)  Also see MT14. */
      uint8_t receivedKwh:    1; /* Device did/did not detect received kWh (0 = False, 1 = True) */
      uint8_t leadingKvarh:   1; /* Device did/did not detect leading kvarh (0 = False, 1 = True) */
      uint8_t lossOfProgram:  1; /* Device was interrupted during programming; the new program was lost. 0=False, 1=True */
      uint8_t flashCodeError: 1; /* Device did/did not detect an error in the code section of flash memory (0 = False, 1 = True) */
      uint8_t flashDataError: 1; /* Device did/did not detect a checksum error in the data section of flash memory (0 = False, 1 = True) */
   } bits;
   uint8_t byte;
} edMfgStatus1_t;             /* */
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint8_t  meteringFlag:        1; /* Indicates device is/is not in metering mode (0 = False, 1 = True) */
      uint8_t  testModeFlag:        1; /* Indicates device is/is not in test mode (0 = False, 1 = True) */
      uint8_t  meterShopModeFlag:   1; /* */
      uint8_t  fill:                5; /* */
   } bits;
   uint8_t     byte;
} edModeBfld_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   edModeBfld_t         edModeBfld;
   edStdStatus1Bfld_t   edStdStatus1Bfld;       /* */
   edStdStatus2Bfld_t   edStdStatus2Bfld;       /* */
   edMfgStatus1_t       edMfgStatus1;           /* */
} stdTbl3_t;                                    /* Standard Table #3 - End Device Mode and Status */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t meterId[20]; /* ID of the Meter. */
} stdTbl5_t;            /* Standard Table #5 - Standard Device Identification Table */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t pfExcludeFlag:     1;    /* Ctrls whether or not device ignores max demand for some time after a pwr outage. (0 = False, 1 = True) */
   uint8_t resetExcludeFlag:  1;    /* Ctrls whether or not device inhibits demand rsts for some period after a rst. (0 = False, 1 = True) */
   uint8_t blockDemandFlag:   1;    /* Device is configured for block demand. (0 = False, 1 = True) */
   uint8_t slidingDemandFlag: 1;    /* Device is configured for sliding (rolling) demand. (0 = False, 1 = True) */
   uint8_t thermalDemandFlag: 1;    /* Device is configured for thermal demand. (0 = False, 1 = True) */
   uint8_t set1PresentFlag:   1;    /* Device is using the 1st set of optional const in the electric record of the CONSTANTS_TABLE #15. (0 = False, 1
                                       = True) */
   uint8_t set2PresentFlag:   1;    /* Device is using the 2nd set of optional const in the electric record of the CONSTANTS_TABLE #15. (0 = False, 1
                                       = True) */
   uint8_t fill:              1;    /* Fill */
   uint8_t    nbrUomEntries;        /* Actual number of entries in the UOM_ENTRY_TBL #12 */
   uint8_t    nbrDemandCtrlEntries; /* Actual number of entries in the DEMAND_CONTROL_TBL #13 */
   uint8_t    dataCtrlLength;       /* Width in bytes of an entry in the first array of the DATA_CONTROL_TBL #14. */
   uint8_t    nbrDataCtrlEntries;   /* Actual number of entries in DATA_CONTROL_TBL #14. */
   uint8_t    nbrConstantsEntries;  /* Actual number of entries in the CONSTANTS_TBL #15. */
   uint8_t    constantsSelector;    /* Selector for the record strct used in the CONSTANTS_TBL #15. 2 = ELECTRIC_CONSTANTS_RCD. */
   uint8_t    nbrSources;           /* Actual number of entries in the SOURCES_TBL #16. */
} stdTbl11_t;                       /* Standard Table #11 - Actual Sources Limiting Table */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t idCode;                     /* ID for physical quantity of interest. */
   uint8_t timeBase:                3; /* Measurement method with respect to time */
   uint8_t multiplier:              3; /* Scaling value to be applied to reported value after delivery of the tagged item. If ID_CODE = 0 (Watts) and
                                          MULTIPLIER = 2 (103) then UOM is kW.*/
   uint8_t q1Accountability:        1; /* Indication of whether quantity lies in quadrant 1. 0 = False, 1 = True */
   uint8_t q2Accountability:        1; /* Indication of whether quantity lies in quadrant 2. 0 = False, 1 = True */
   uint8_t q3Accountability:        1; /* Indication of whether quantity lies in quadrant 3. 0 = False, 1 = True */
   uint8_t q4Accountability:        1; /* Indication of whether quantity lies in quadrant 4. 0 = False, 1 = True */
   uint8_t netFlowAccountability:   1; /* Indication of manner in which specified quadrants are summed; 0 = added positively, regardless of direction
                                          of flow, 1 = net of delivered minus received. */
   uint8_t segmentation:            3; /* Phase measurement association, where applicable. */
   uint8_t harmonic:                1; /* 0 = entire signal, unfiltered, 1 = harmonic component of associated src */
   uint8_t reserved1:               1;
   uint8_t reserved2:               7;
   uint8_t nfs:                     1; /* Not fully supported; indicates UOM entry does not follow the definition in some way. */
} stdTbl12_t;                          /* Standard Table #12 - Unit of Measure Entry Table */
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint8_t    subInt;         /* For sliding demand, size of subinterval. */
      uint8_t    intMultiplier;  /* For sliding demand, subinterval multiplier. */
   };
   uint16_t intLength;           /* For block demand, size of demand interval */
} intervalValue_t;               /* Standard Table #13 - Interval Value Structure */

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t             resetExclusion;       /* # of min after demand reset to inhibit additional resets. */
   uint8_t             pFailRecognitionTm;   /* # of sec after a power failure occurs before a valid pwr failure is recorded and a specified action is
                                                taken. See Mfg. Table 67. */
   uint8_t             pFailExclusion;       /* # of min after a valid pwr outage occurs to inhibit demand calc. */
   uint8_t             coldLoadPickup;       /* # of min after a valid pwr failure occurs to provide cold load pickup function. */
   intervalValue_t   intervalValue[5];       /* Standard Table #13 - Interval Value Structure Array */
} stdTbl13_t;                                /* Standard Table #13 - Demand Control Table */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t  sourceId[62]; /* Contains entry-specific information which further qualifies the source. */
} stdTbl14_t;              /* Standard Table #14 - Data Control Table */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  multiplier[6];       /* Value to be used in multiplication/division adjustment. */
   uint8_t  offset[6];           /* Value to be used in addition/subtraction adjustment. */
   uint8_t  setAppliedFlag:   1; /* Indicates if the RATIO_F1 has been applied to the associated source. 0/1 False/True) */
   uint8_t  filler:           7; /* */
   uint8_t  ratioF1[6];          /* Current transformer ratio for electricity metering. More generally, ratio of intermediary device to allow
                                    interface of commodity flow to utility meters. */
   uint8_t  ratioP1[6];          /* Voltage transformer ratio for electricity metering. More generally, ratio of intermediary device to allow
                                    interface of commodity pressure to utility meters. */
} stdTbl15_t;                    /* Standard Table #15 - Constants Table */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t uomEntryFlag:         1; /* Indicates if this source has an entry in Table 12. (0 = False, 1 = True) */
   uint8_t demandCtrlFlag:       1; /* Indicates if this source has an entry in Table 13. (0 = False, 1 = True) */
   uint8_t dataCtrlFlag:         1; /* Indicates if this source has an entry in Table 14. (0 = False, 1 = True) */
   uint8_t constantsFlag:        1; /* Indicates if this source has an entry in Table 15. (0 = False, 1 = True) */
   uint8_t pulseEngrFlag:        1; /* Indicates if this source is in pulses or engineering units. (0 = pulses, 1 = engineering units) */
   uint8_t constantToBeApplied:  1; /* Indicates if the entry in Table 15 (Constants Table) if present, has been applied to the source. (0 = False, 1
                                       = True) */
   uint8_t filler:               2; /* - */
} stdTbl16_t;                       /* Standard Table #16 - Source Definition Table */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t seasonInfoFieldFlag:  1; /* Indicates whether device is capable of reporting the season in tables in this decade. (0 = False, 1 = True) */
   uint8_t dateTimeFieldFlag:    1; /* Indicates whether device is capable of reporting date/time in tables in this decade. (0 = False, 1 = True) */
   uint8_t demandResetCtrFlag:   1; /* Indicates if the device is capable of counting demand resets. (0 = False, 1 = True) */
   uint8_t demandResetLockFlag:  1; /* Indicates whether device supports demand reset lockout. (0 = False, 1 = True) */
   uint8_t cumDemandFlag:        1; /* Indicates whether the device is recording cumulative demand. (0 = False, 1 = True) */
   uint8_t contCumDemandFlag:    1; /* Indicates whether the device supports continuous cumulative demand. (0 = False, 1 = True) */
   uint8_t timeRemainingFlag:    1; /* Indicates if the device is capable of reporting the time remaining in the demand interval. (0 = False, 1 =
                                       True) */
   uint8_t filler:               1; /* - */
} regFunc1Bfld_t;                   /*  */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t selfReadInhibitOverflowFlag:   1; /* Indicates if the device is capable of inhibiting self-reads once an overflow occurs. (0 = False, 1 =
                                                True) */
   uint8_t selfReadSeqNbrFlag:            1; /* Indicates if the device is capable of providing a self-read sequential number for each entry. (0 =
                                                False, 1 = True) */
   uint8_t dailySelfReadFlag:             1; /* Indicates whether daily self-reads are supported. 0= False, 1= True */
   uint8_t weeklySelfReadFlag:            1; /* Indicates whether weekly self-reads are supported. 0= False, 1= True */
   uint8_t selfReadFlag:                  2; /* Specifies whether device is capable of performing a self-read whenever a demand reset is performed,
                                                and vice versa. (0 = None, 1 = self-read on demand reset, 2 = demand reset on self-read, 3 = both) */
   uint8_t filler:                        2; /* - */
} regFunc2Bfld_t;                            /*  */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t seasonInfoFieldFlag:           1; /* Indicates whether device is capable of reporting the season in tables in this decade. (0 = False, 1 =
                                                True) */
   uint8_t dateTimeFieldFlag:             1; /* Indicates whether device is capable of reporting date/time in tables in this decade. (0 = False, 1 =
                                                True) */
   uint8_t demandResetCtrFlag:            1; /* Indicates if the device is capable of counting demand resets. (0 = False, 1 = True) */
   uint8_t demandResetLockFlag:           1; /* Indicates whether device supports demand reset lockout. (0 = False, 1 = True) */
   uint8_t cumDemandFlag:                 1; /* Indicates whether device supports demand reset lockout. (0 = False, 1 = True) */
   uint8_t contCumDemandFlag:             1; /* Indicates whether the device supports continuous cumulative demand. (0 = False, 1 = True) */
   uint8_t timeRemainingFlag:             1; /* Indicates if the device is capable of reporting the time remaining in the demand interval. (0 =
                                                False, 1 = True) */
   uint8_t filler:                        1; /* - */
//   regFunc1Bfld_t regFunc1Bfld;
   uint8_t selfReadInhibitOverflowFlag:   1; /* Indicates if the device is capable of inhibiting self-reads once an overflow occurs. (0 = False, 1 =
                                                True) */
   uint8_t selfReadSeqNbrFlag:            1; /* Indicates if the device is capable of providing a self-read sequential number for each entry. (0 =
                                                False, 1 = True) */
   uint8_t dailySelfReadFlag:             1; /* Indicates whether daily self-reads are supported. 0= False, 1= True */
   uint8_t weeklySelfReadFlag:            1; /* Indicates whether weekly self-reads are supported. 0= False, 1= True */
   uint8_t selfReadFlag:                  2; /* Specifies whether device is capable of performing a self-read whenever a demand reset is performed,
                                                and vice versa. (0 = None, 1 = self-read on demand reset, 2 = demand reset on self-read, 3 = both) */
   uint8_t filler2:                       2; /* - */
// regFunc2Bfld_t regFunc2Bfld;
   uint8_t nbrSelfReads;         /* Maximum number of self-reads supported. */
   uint8_t nbrSummations;        /* Maximum number of summations supported per data block. */
   uint8_t nbrDemands;           /* Maximum number of demands supported per data block. */
   uint8_t nbrCoinValues;        /* Maximum number of coincident values saved concurrently in each data block. */
   uint8_t nbrOccur;             /* Maximum number of occurrences stored for  each selection. */
   uint8_t nbrTiers;             /* Maximum number of tiers that data can be stored in. */
   uint8_t nbrPresentDemands;    /* Maximum number of present demand values that can be stored. */
   uint8_t nbrPresentValues;     /* Maximum number of present values that can be stored. */
} stdTbl20_t, stdTbl21_t;        /* Standard Table #20 & #21 - Dimension Register Table & Demand Version */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t summationSelect[5];         /* Source indices for summation quantities from Table 16. */
   uint8_t demandSelection[5];         /* Source indices for demand quantities from Table 16. */
   uint8_t minOrMaxFlags;              /* Bit flags, one for each selected demand quantity, which indicate whether the corresponding entry is a
                                          minimum or a maximum. (0 = minimum, 1 = maximum) */
   uint8_t coincidentSelection[10];    /* Source indices for coincident demand quantities from table 16. */
   uint8_t coincidentDemandAssoc[10];  /* For each coincident demand, index into DEMAND_SELECT array identifying the demand for which the coincident
                                          value is taken */
} stdTbl22_t;                          /* Standard Table #22 - Data Selection Table */
PACK_END

/* LP/TOU meter demand layout */
PACK_BEGIN
typedef struct PACK_MID
{
#if ST23_DEMANDS_EVENTTIME_ENABLED
   MtrTimeFormatLowPrecision_t eventTime;                      /* Date/time of max demand. */
#endif
   uint8_t cumulativeDemand[ST23_DEMANDS_CONTCUMDEMAND_SIZE];  /* Cumulative demand. */
#if ST23_DEMANDS_CONTCUMDEMAND_ENABLED
   uint8_t contCumDemand[ST23_DEMANDS_CONTCUMDEMAND_SIZE];
#endif
   uint8_t demand[ST23_DEMANDS_SIZE];                          /* Current max demand. */
} demands_t;   /* Standard Table #23 - Current Register Data Table */
PACK_END

/* Table 23 in LP/TOU mode includes time stamps. Data laid out:
   NBR_DEMAND RESETS (1 byte)
   SUMMATIONS[5][6]
   DEMANDS[5] substructure, repeated 5 times
      EVENT_TIME (5 bytes)
      CUMULATIVE DEMAND (6 bytes)
      DEMAND (4 bytes)
   COINCIDENTS[10][4]
   TIER DATA - Same as above repeated for each tier (e.g. A, B, C, D, E)
*/
typedef struct
{
   MtrTimeFormatLowPrecision_t eventTime[ST23_DEMANDS_CNT];    /* Date/time of max demand. */
   uint8_t cumulativeDemand[ST23_DEMANDS_CONTCUMDEMAND_SIZE];  /* Cumulative demand. */
#if ( ST23_DEMANDS_CONTCUMDEMAND_ENABLED != 0 )
   uint8_t contCumDemand[ST23_DEMANDS_CONTCUMDEMAND_SIZE];
#endif
   uint8_t demand[ST23_DEMANDS_SIZE];                          /* Current max demand. */
}demandsAllFields_t;    /* Standard Table #23 - Worst case size of Demand block  */

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t     summations[ST23_SUMMATIONS_CNT][ST23_SUMMATIONS_SIZE];   /* Values of selected summation quantities. */
   demands_t   demands[ST23_DEMANDS_CNT];
   uint32_t    coincidents[ST23_COINCIDENTS_CNT];             /* Values of selected quantities coincident to the selected Two coincidents are
                                                                 available for each demand; coincidents are associated with demands based on the
                                                                 COINCIDENT_DEMAND_ASSOC in Table 22. */
} totDataBlock_t;       /* Standard Table #23 - Current Register Data Table */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t        nbrDemandResets;
   totDataBlock_t totDataBlock;
   totDataBlock_t tierDataBlock[4];
} stdTbl23_t;                       /* Standard Table #23 - Current Register Data Table */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   stdTbl23_t st23;  /* This is just the starting location of ST23 */
} stdTbl25Dmd_t;     /* Standard Table #25 - Data Selection Table */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   MtrTimeFormatLowPrecision_t   dmdResetDateTime; /* Date/Time of last demand reset   */
   uint8_t                       season;           /* Range 1 - 4 indicates current season. */
   stdTbl23_t                    st23;             /* This is just the starting location of ST23 */
} stdTbl25TOU_t;     /* Standard Table #25 - Data Selection Table */
PACK_END


PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t          dayOfWeek : 3; /* current day of the week */
   uint8_t          dstFlag   : 1; /* indicates whether or not device is in daylight savings */
   uint8_t            gmtFlag : 1; /* indicates if device time corresponds to Greenwich Mean */
   uint8_t    tmZnAppliedFlag : 1; /* indicates if time zone offset has been applied to the device time */
   uint8_t     dstAppliedFlag : 1; /* indicates if device time includes adjustment for daylight savings time */
   uint8_t             filler : 1; /* filler, unused bit */
} TimeDateQualBitField_t;
PACK_END  /* represent the time data qualification bit field in ST52 */


PACK_BEGIN
typedef struct PACK_MID
{
   MtrTimeFormatHighPrecision_t  clockCalendar;  /* Current Date and Time   */
   TimeDateQualBitField_t         qualBitField;  /* date and time qualification info */
} stdTbl52_t;     /* Standard Table #52 - Clock Table */
PACK_END


PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t lpSet1InhibitOvfFlag:   1;  /* Indicates if device is inhibiting LP set 1 once an overflow occurs. */
   uint8_t lpSet2InhibitOvfFlag:   1;  /* Indicates if device is inhibiting LP set 2 once an overflow occurs */
   uint8_t lpSet3InhibitOvfFlag:   1;  /* Indicates if device is inhibiting LP set 3 once an overflow occurs */
   uint8_t lpSet4InhibitOvfFlag:   1;  /* Indicates if device is inhibiting LP set 4 once an overflow occurs */
   uint8_t blkEndReadFlag:         1;  /* Indicates whether the device is providing block register readings. */
   uint8_t blkEndPulseFlag:        1;  /* Indicates if the device is using pulse accumulation. (0 = False,  1 = True) */
   uint8_t scalarDivisorFlagSet1:  1;  /* Indicates if the device is using scalars and divisors associated with set 1 LP data. */
   uint8_t scalarDivisorFlagSet2:  1;  /* Indicates if the device is using scalars and divisors associated with set 2 LP data. */
   uint8_t scalarDivisorFlagSet3:  1;  /* Indicates if the device is using scalars and divisors associated with set 3 LP data. */
   uint8_t scalarDivisorFlagSet4:  1;  /* Indicates if the device is using scalars and divisors associated with set 4 LP data. */
   uint8_t extendedIntStatusFlag:  1;  /* Indicates if the device is returning extended interval status with LP data. */
   uint8_t simpleIntStatusFlag:    1;  /* Indicates if the device is returning simple interval status with LP data. */
   uint8_t filler:                 4;
} lpFlags_t; /* This bit field specifies Load Profile list management capabilities. */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t invUnit8Flag:   1; /* Indicates whether device is using UINT8 format for intervals. */
   uint8_t invUnit16Flag:  1; /* Indicates whether device is using UINT16 format for intervals. */
   uint8_t invUnit32Flag:  1; /* Indicates whether device is using UINT32 format for intervals. */
   uint8_t invInt8Flag:    1; /* Indicates whether device is using INT8 format for intervals. */
   uint8_t invInt16Flag:   1; /* Indicates whether device is using INT16 format for intervals. */
   uint8_t invInt32Flag:   1; /* Indicates whether device is using INT32 format for intervals. */
   uint8_t invNiFmt1Flag:  1; /* Indicates if device is using NI_FMAT1 format for intervals. */
   uint8_t invNiFmt2Flag:  1; /* Indicates if device is using NI_FMAT2 format for intervals. */
} lpFormats_t;  /* This set of booleans specifies the format(s) acceptable for use in DIM_LP_TBL (Table 60) and ACT_LP_TBL (Table 61). */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t nbrBlksSet;       /* Max # of blocks that can be contained in LP_DATA_SET1_TBL (ST64). */
   uint16_t nbrBlksIntsSet;   /* Max # of intrvls per blck that can be contained in LP_DATA_SET1_TBL (ST64) */
   uint8_t  nbrChnsSet;       /* Max # of ch of LP data that can be contained in LP_DATA_SET1_TBL (ST64) */
   uint8_t  nbrMaxIntTimeSet; /* Max time (min) for LP intrvl duration contained in LP_DATA_SET1_TBL (ST64) */
} lpDataSet_t; /* This record contains information defining how each set of Load Profile information works. */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint32_t    lpMemoryLen; /* Maximum number of octets of storage available for Load Profile data. This reflects the combined sizes of tables
                               LP_DATA_SET1_TBL (Table 64), LP_DATA_SET2_TBL (Table 65),  LP_DATA_SET3_TBL (Table 66), and LP_DATA_SET4_TBL (Table
                               67). */
   lpFlags_t   lpFlags;       /* This bit field specifies Load Profile list management capabilities. */
   lpFormats_t lpFormats;     /* This set of booleans specifies the format(s) acceptable for use in DIM_LP_TBL (Table 60) and ACT_LP_TBL (Table 61).
   */
   lpDataSet_t lpDataSet1;    /* Contains info defining how each set of Load Profile info works. */
   lpDataSet_t lpDataSet2;    /* Contains info defining how each set of Load Profile info works. */
   lpDataSet_t lpDataSet3;    /* Contains info defining how each set of Load Profile info works. */
   lpDataSet_t lpDataSet4;    /* Contains info defining how each set of Load Profile info works. */
} stdTbl61_t;   /* Actual Load Profile Table - LP_MEMORY_LEN, NBR_BLKS_SET1 and NBR_BLK_INTS_SET1 will be dependent on the interval size programmed,
                   since the intent is to make the block size equal to one day's worth of interval data. Although there is a fixed amount of flash
                   memory available for LP data storage, the actual usable amount will depend on the programmed interval size, due to the flash memory
                   management scheme.*/
PACK_END

#if (ID_MAX_CHANNELS) // Don't define the following section when ID_MAX_CHANNELS is 0 (i.e. DA and ILC) because it makes an array of size 0.
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t endRdgFlag: 1; /* Indicates whether the corresponding channel has an associated end reading. */
   uint8_t filler:     7;
} chnlFlag_t;  /* Flags associated with a particular channel */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   chnlFlag_t chnlFlag;
   uint8_t    lpSrcSel;         /* Index into Table 16 which identifies the data source for this channel. */
   uint8_t    endBlkRdgSrcSel;  /* Idx into tbl 16 which identifies the data src for block end reading for ch. */
} lpSelSet_t;    /* Array of records that identifies sources of data for each ch of ID in LP_DATA_SET1_TBL (ST64). */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   lpSelSet_t  lpSelSet[ID_MAX_CHANNELS];     /* Array of records that identifies sources of data for each channel of interval data in LP_DATA_SET1_TBL
                                                (Table 64). */
   uint8_t     intFmtCde;                    /* Code selecting the format for all ID in ST64. (16 = INT16) */
   uint16_t    scalarsSet[ID_MAX_CHANNELS];   /* Array of scalars applied to ID before recording pulse data. */
   uint16_t    divisors_set[ID_MAX_CHANNELS]; /* Array of divisors applied to ID before recording pulse data. */
} stdTbl62_t;
PACK_END
#endif

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t blockOrder:                1;  /* Indicates if LP data blocks are transported in ascending or descending order. (0 = ascending, 1 =
                                             descending) */
   uint8_t overflowFlag:              1;  /* Indicates when entering a new interval block would cause overflow, i.e., the number of unread blocks of
                                             LP data would exceed the maximum number of blocks for which storage is available. */
   uint8_t listType:                  1;  /* Indicates if a FIFO or circular list is used for LP data. (0 = FIFO, 1 = circular list) */
   uint8_t blockInhibitOverflowFlag:  1;  /* Indicates if LP recording is inhibited following an overflow, see Tbl 61 */
   uint8_t intervalOrder:             1;  /* Indicates if intervals in each block are transported in ascending or descending order. (0 = ascending, 1
                                             = descending) */
   uint8_t activeModeFlag:            1;  /* Indicates whether LP recording is active. (0 = False, 1 = True) This flag will be set False when the
                                             interval size specified in Table 61 is 0. */
   uint8_t testMode:                  1;  /* Indicates whether LP data set pertains to normal mode or test mode. (0 = normal, 1 = test) */
   uint8_t lpPowerFailRec:            1;  /* Indicates whether LP power fail recovery mode is recording temporary LP interval data. (0=False, 1=True)
   */
} lpSetStatusFlags_t; /* Status information for profile data set 1. */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   lpSetStatusFlags_t   lpSelSet1 PACK_MID;  /* Status information for profile data set 1. */
   uint16_t             nbrValidBlocks;      /* Number of valid load profile data blocks in Table 64. A block is considered valid when it contains at
                                                least one valid interval. */
   uint16_t             lastBlockElement;    /* Array index of the newest valid data block. This field is valid only if the NBR_VALID_BLOCKS is
                                                greater than zero. */
   uint32_t             lastBlockSeqNbr;     /* The sequence # of the last valid block in the LP data block array. */
   uint16_t             nbrUnreadBlocks;     /* The number of valid blocks of LP data which have not been read. This field can be updated through
                                                Standard Procedure 5. */
   uint16_t             nbrValidInt;         /* Number of valid intervals in the last load profile block. */
} stdTbl63_t;
PACK_END


PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  yr;
   uint8_t  mm;
   uint8_t  dd;
   uint8_t  hh;
   uint8_t  min;
} stdTbl64Time_t;
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint8_t b00:   1;
      uint8_t b01:   1;
      uint8_t b02:   1;
      uint8_t b03:   1;
      uint8_t b04:   1;
      uint8_t b05:   1;
      uint8_t b06:   1;
      uint8_t b07:   1;
      uint8_t b08:   1;
      uint8_t b09:   1;
      uint8_t b10:   1;
      uint8_t b11:   1;
      uint8_t b12:   1;
      uint8_t b13:   1;
      uint8_t b14:   1;
      uint8_t b15:   1;
      uint8_t b16:   1;
      uint8_t b17:   1;
      uint8_t b18:   1;
      uint8_t b19:   1;
      uint8_t b20:   1;
      uint8_t b21:   1;
      uint8_t b22:   1;
      uint8_t b23:   1;
      uint8_t b24:   1;
      uint8_t b25:   1;
      uint8_t b26:   1;
      uint8_t b27:   1;
   } bits;
   uint8_t bytes[ (ID_MAX_CHANNELS/2)+1 ];
} stdTbl64Flags_t;
PACK_END

PACK_BEGIN
typedef struct
{
   uint16_t tblProcNbr  : 11;       /* Event identifier  */
   uint16_t mfgFlag     :  1;       /* 0 -> standard event; 1 -> mfg event */
   uint16_t selector    :  4;       /* Not used */
} stdTbl76EventCode_t, *pStdTbl76EventCode_t;
PACK_END

PACK_BEGIN
typedef struct
{
   uint8_t             dateTime[ 6 ];
   uint16_t            seqNbr;
   uint16_t            userID;
   stdTbl76EventCode_t eventCode;
   uint8_t             argument;
} stdTbl76Event_t, *pStdTbl76Event_t;
PACK_END

PACK_BEGIN
typedef struct
{
   struct
   {
      uint8_t  order       :  1;
      uint8_t  Overflow    :  1;
      uint8_t  listType    :  1;
      uint8_t  inhOverflow :  1;
      uint8_t  fill        :  4;
   } eventFlags;
   uint16_t          nbrValid;
   uint16_t          lastEntry;
   uint32_t          lastSeqNbr;
   uint16_t          nbrUnread;
   stdTbl76Event_t   eventArray[];
} stdTbl76_t, *pStdTbl76_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   stdTbl76EventCode_t  code;       /* Event identifier  */
   EDEventType          eventType;  /* Associated heep event type */
   uint8_t              mask;       /* Value in event_argument from meter to identify sub event (bit fields). */
   uint8_t              priority;   /* Event priority */
} stdTbl76Lookup_t;
PACK_END

typedef enum
{
   eCRTT_TOT_SUMMATION,       /* Total Summation */
   eCRTT_TOT_DEMAND,          /* Total Demand */
   eCRTT_DAILY_DEMAND,        /* Daily Demand*/
   eCRTT_TOT_COINCIDENT,      /* Total Coincident */
   eCRTT_TIER_A_SUMMATION,    /* Tier A Summation */
   eCRTT_TIER_A_DAILY_DEMAND, /* Tier A Daily Demand*/
   eCRTT_TIER_A_DEMAND,       /* Tier A Demand */
   eCRTT_TIER_A_COINCIDENT,   /* Tier A Coincident */
   eCRTT_TIER_B_SUMMATION,    /* Tier B Summation */
   eCRTT_TIER_B_DEMAND,       /* Tier B Demand */
   eCRTT_TIER_B_DAILY_DEMAND, /* Tier B Daily Demand*/
   eCRTT_TIER_B_COINCIDENT,   /* Tier B Coincident */
   eCRTT_TIER_C_SUMMATION,    /* Tier C Summation */
#if ( DAILY_TOU_TIER_C_D_SUPPORT != 0 )
   eCRTT_TIER_C_DAILY_DEMAND, /* Tier C Daily Demand*/
#endif
   eCRTT_TIER_C_DEMAND,       /* Tier C Demand */
   eCRTT_TIER_C_COINCIDENT,   /* Tier C Coincident */
   eCRTT_TIER_D_SUMMATION,    /* Tier D Summation */
#if ( DAILY_TOU_TIER_C_D_SUPPORT != 0 )
   eCRTT_TIER_D_DAILY_DEMAND, /* Tier D Daily Demand*/
#endif
   eCRTT_TIER_D_DEMAND,       /* Tier D Demand */
   eCRTT_TIER_D_COINCIDENT,   /* Tier D Coincident */
#if ( TOU_TIER_E_SUPPORT != 0 )
   eCRTT_TIER_E_SUMMATION,    /* Tier E Summation */
   eCRTT_TIER_E_DEMAND,       /* Tier E Demand */
   eCRTT_TIER_E_COINCIDENT    /* Tier E Coincident */
#endif
} currentRegTblType_t;

typedef enum
{
   eDDT_NONE,
   eDDT_CUMULATIVE_DEMAND,
   eDDT_CONT_CUMULATIVE_DEMAND,
   eDDT_MAX_DEMAND
} enum_uomDescriptionInfo_t;

typedef struct
{
   uint8_t    q1:            1; /* 1 = Treat Q1 Accountability in STD Table #12 as a wild card */
   uint8_t    q2:            1; /* 1 = Treat Q2 Accountability in STD Table #12 as a wild card */
   uint8_t    q3:            1; /* 1 = Treat Q3 Accountability in STD Table #12 as a wild card */
   uint8_t    q4:            1; /* 1 = Treat Q4 Accountability in STD Table #12 as a wild card */
   uint8_t    netFlowAcnt:   1; /* 1 = Treat Net Flow Accountability in STD Table #12 as a wild card */
   uint8_t    segmentation:  1; /* 1 = Treat Segmentation in STD Table #12 as a wild card */
   uint8_t    harmonic:      1; /* 1 = Treat Harmonic in STD Table #12 as a wild card */
   uint8_t    rsvd:          1; /* Reserved */
} wildCard_t;

typedef struct
{
   int8_t        applyScalars;      /* 1 = Apply LP scalars. 0 = No scalers needed */
   int8_t        multiplierOveride; /* Multiplier for ST12. 0 = Use table 12's multiplier */
} multiplier_t;

typedef enum
{
   eCURRENT_DATA,    /* Data comes from ST23( Current Register Data Table ) or MT98( Cycle Insensitive Demand Data Table )  */
   eSHIFTED_DATA     /* Data comes from ST25, Previous Demand Reset Data Table */
} eMtrDataLoc_t;

typedef struct
{
   eMtrDataLoc_t  location;   /* Indicates current/shifted data   */
   wildCard_t     wild;       /* Which bits in tbl12 must match   */
   multiplier_t   multCfg;
   stdTbl12_t     tbl12;      /* This will be the match criteria to match a request in the host meter. */
   uint8_t        tbl14Byte0;
   uint8_t        tbl14Byte1;
} uomSignature_t;

typedef union
{
   niFormatEnum_t fmt;
   int8_t         exp;
} dataFmt_t;

typedef struct
{
   tableOffsetCnt_t  tblInfo;       /* Table ID, Offset and Cnt of quantity */
   uint32_t          multiplier15;  /* Multiplier - Same as ANSI ST15, Multiplier field. Use NI_FMAT1. */
   int8_t            multiplier12;  /* Multiplier - Same as ANSI ST12, MULTIPLIER field.
                                       0 = 10^0,
                                       1 = 10^-1,
                                       2 = 10^-2,
                                       3 = 10^-3,
                                       4 = 10^-4,
                                       5 = 10^-5,
                                       6 = 10^-6,
                                       7 = 10^-9 */
   niFormatEnum_t    dataFmt;       /* Data format as defined by Std Table #0 */
   eFptrAnsi_pup     *pFunct;       /* Function pointer to process the results, NULL if not used */
   bool              stdFmt;        /* 1 = Use standard format, 0 = use value in the dataFmt field as actual */
} tableLoc_t;

typedef enum
{
   eANSI_MTR_LOOKUP,                /* Look the data up from the tables read from the host meter. */
   eANSI_DIRECT                     /* Use a direct table read defined in tableLoc_t. */
} dataAccess_t;

typedef struct
{
   meterReadingType  quantity;   /* Unit Of Measure */
   dataAccess_t      eAccess;    /* Access method, look up or direct read. */
   union
   {
      uomSignature_t sig;        /* Define the signature of the data requested */
      tableLoc_t     tbl;        /* Use direct table read */
   };
} quantitySig_t;

/* ------------------------------------------------------------------------------------------------------------------ */
/* Standard Procedure Parameter Definitions */

PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t procNum;   /* Procedure Number */
   uint8_t procId;     /* Procedure ID - a number assigned at the time the procedure is executed. */
} procNumId_t;       /* The first 2 procedure data byte that goes with every procedure. */
PACK_END

typedef enum
{
   eLIST_TYPE_EVENT_LOG = (uint8_t)1,
   eLIST_TYPE_SELF_READ,
   eLIST_TYPE_LP_DATA_SET
} rstLstPtrsProcParam_t; /* This procedure will reset list control variables for the selected list to their initial states. To execute this procedure
                            successfully, the user must have access rights to the procedure and write access to the affected table(s). If the
                            selected list is not active (upgraded enabled, feature configured), this procedure will appear to execute successfully,
                            but will not actually affect any data. Supported lists are Load Profile, Self-reads and Event Log. If an attempt is made
                            to reset any other list, an Invalid Parameter response will be returned in Standard Table 8. This procedure should not be
                            executed when the meter is in Unconfigured Mode, Uncalibrated Mode, Test Mode or Calibration Mode. Procedure must NOT be
                            bracketed by Start/Stop Programming. */
typedef enum
{
   PrimaryPowerDown           = 1,
   PrimaryPowerUp             = 2,
   TimeChanged                = 3,
   EndDeviceAccessedforRead   = 7,
   EndDeviceAccessedforWrite  = 8,
   EndDeviceProgrammed        = 11,
   DemandResetOccurred        = 20,
   SelFreadOccurred           = 21,
   TestModeEnter              = 32,
   TestModeExit               = 33
} stdEventsLogged;

#if ( LOG_IN_METER == 1 )
typedef enum
{
#if ( HMC_KV == 1 )
   Diagnostic1                      = 0,
   Diagnostic1Cleared               = 1,
   Diagnostic2                      = 2,
   Diagnostic2Cleared               = 3,
   Diagnostic3                      = 4,
   Diagnostic3Cleared               = 5,
   Diagnostic4                      = 6,
   Diagnostic4Cleared               = 7,
   Diagnostic5                      = 8,
   Diagnostic5Cleared               = 9,
#endif
   Diagnostic6                      = 10,
   Diagnostic6Cleared               = 11,
   Diagnostic7                      = 12,
   Diagnostic7Cleared               = 13,
   Diagnostic8                      = 14,
   Diagnostic8Cleared               = 15,
   Caution000400                    = 16,
   Caution000400Cleared             = 17,
   Caution004000                    = 18,
   Caution004000Cleared1            = 19,
   Caution400000                    = 20,
   Caution400000Cleared1            = 21,
   Caution040000                    = 22,
   Caution040000Cleared1            = 23,
   RealTimePricingActivation        = 24,
   RealTimePricingDeactivation      = 25,
   CalibrationModeActivated         = 28,
   RevenueGuardPlusEvent            = 30,
#if ( HMC_KV == 1 )
   BadPasswordEvent                 = 32,
#endif
#if(HMC_I210_PLUS_C==1)
   DCdetectioncondition             = 33,
   DCdetectionconditioncleared      = 34,
   ECPActivation                    = 35,
   ECPDeactivation                  = 36,
   DLPActivation                    = 37,
   DLPDeactivation                  = 38,
   RCDCSwitchOpen                   = 39,
   RCDCSwitchClose                  = 40,
   TamperDetect                     = 41,
   HighTempThresholdAlertSet        = 56,
   HighTempThresholdAlertClear      = 57,
   BadSecurityCodeLockoutSet        = 58,
   BadSecurityCodeLockoutCleared    = 59,
   OpticalPortCommunicationDisabled = 60,
   OpticalPortCommunicationEnabled  = 61,
   HighLineCurrentCautionSet        = 66,
   HighLineCurrentCautionCleared    = 67
#endif
}mfgEventsLogged;
#endif

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t demandRst:     1;  /* 1 = True, 0 = False (bit 0) */
   uint8_t selfRead:      1;  /* NOTE:  Not supported in KV2 (bit 1) */
   uint8_t seasonChange:  1;  /* 0 = False, 1 = True (bit 2) */
   uint8_t newSeason:     4;  /* Valid: 0-3 (values 4-15 are not supported) */
} remoteResetProcParam_t;     /* This procedure may be used to perform a demand reset or season change only. This procedure should be used to do only
                                 one reset at a time.*/
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t                       setTime:          1; /* 1 = Apply the Time (bit 0) */
   uint8_t                       setDate:          1; /* 1 = Apply the Date (bit 1) */
   uint8_t                       setTimeDateQual:  1; /* 1 = Apply the information in timeDateQual field (bit 2) */
   uint8_t                       rsvd:             5; /* Reserved fields and must be set to 0. (bits 3-7) */
   MtrTimeFormatHighPrecision_t  DateTime;            /* Meter's date time structure   */
   uint8_t                       dayOfWeek:        3; /* 0 = Sunday, 1 = Monday, 2 - Tuesday, etc... Valid Range: 0-6 */
   uint8_t                       dst:              1; /* 1 = End Device time is in daylight saving time (bit 3) */
   uint8_t                       gmt:              1; /* 1 = End Device time is Greenwich Mean Time (bit 4) */
   uint8_t                       tmZnApplied:      1; /* 1 = Time zone offset been applied to end device time (bit 5) */
   uint8_t                       dstApplied:       1; /* 1 = End device time includes DST adjustment (bit 6) */
   /* If GEN_CONFIG_TBL.STD_VERSION_NO < 2 Then dstSupported is not used, else dstSupported is used. (bit 7) */
   uint8_t                      dstSupported:     1; /* 1 = ED supports DST_FLAG. DST_FLAG returned is meaningful. */
} setDateTimeProcParam_t;              /* Data field to send to the host meter when setting the date/time */
PACK_END
#if 0  // Packing doesn't appear to be working properly on the IAR compiler.
PACK_BEGIN
typedef struct PACK_MID
{
   union __packed
   {
      struct __packed PACK_MID
      {
         uint8_t setTime:          1;  /* 1 = Apply the Time (bit 0) */
         uint8_t setDate:          1;  /* 1 = Apply the Date (bit 1) */
         uint8_t setTimeDateQual:  1;  /* 1 = Apply the information in timeDateQual field (bit 2) */
         uint8_t rsvd:             5;  /* Reserved fields and must be set to 0. (bits 3-7) */
      } setMaskBfld;                   /* Individual fields */
      uint8_t setMaskAllFields;        /* Used to clear all fields */
   } setMask;                          /* Set Mask Bitfield */
   struct PACK_MID
   {
      uint8_t year;                    /* Year to send to the host meter. The year starts at 2000, 0 = year 2000. */
      uint8_t month;                   /* Month to send to the host meter. */
      uint8_t day;                     /* Day of the month to send to the host meter. */
      uint8_t hours;                   /* Hour of the day to send to the host meter. */
      uint8_t minutes;                 /* Minutes of the day to send to the host meter. */
      uint8_t seconds;                 /* Seconds of the day to send to the host meter. */
   } dateTime;                         /* New Date and Time */
   union __packed PACK_MID
   {
      struct __packed PACK_MID
      {
         uint8_t dayOfWeek:        3;  /* 0 = Sunday, 1 = Monday, 2 - Tuesday, etc... Valid Range: 0-6 */
         uint8_t dst:              1;  /* 1 = End Device time is in daylight saving time (bit 3) */
         uint8_t gmt:              1;  /* 1 = End Device time corresponds to Greenwich Mean Time (bit 4) */
         uint8_t tmZnApplied:      1;  /* 1 = Time zone offset has been applied to the end device time (bit 5) */
         uint8_t dstApplied:       1;  /* 1 = End device time includes daylight saving adjustment (bit 6) */
         /* If GEN_CONFIG_TBL.STD_VERSION_NO < 2 Then dstSupported is not used, else dstSupported is used. (bit 7) */
         uint8_t dstSupported:     1;  /* 1 = End Device supports the DST_FLAG. The DST_FLAG returned is meaningful. */
      } timeDateQualBfld;              /* Individual fields */
      uint8_t timeDateQualAllFields;   /* Used to clear all fields */
   } timeDateQual;                     /* Daylight saving time status */
} setDateTimeProcParam_t;              /* Data field to send to the host meter when setting the date/time */
PACK_END
#endif
/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */
/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
#endif
