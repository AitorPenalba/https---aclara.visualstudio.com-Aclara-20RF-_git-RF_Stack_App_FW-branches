/***********************************************************************************************************************

   Filename: heep.h

   Contents:

************************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2020-2021 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
************************************************************************************************************************
   $Log$

***********************************************************************************************************************/

#ifndef HEEP_H
#define HEEP_H

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "portable_freescale.h"
/*#include "time_util.h" */
/*#include "PHY_Protocol.h" */


/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */

/* #DEFINE DEFINITIONS */
/*lint -emacro(755,CIM_APP_MSG_READING_VALUE_SIZE,DFW_APP_MSG_READING_VALUE_SIZE,VALUES_INTERVAL_END_SIZE,VALUES_INTERVAL_END_SIZE_BITS) not referenced */
/*lint -emacro(755,VALUES_INTERVAL_START_SIZE,VALUES_INTERVAL_START_SIZE_BITS,FULL_MTR_RDG_SIZE_SHIFT) not referenced */
/*lint -emacro(755,COMPACT_MTR_RDG_SIZE_SHIFT,COMPACT_MTR_RDG_TS_PRESENT,COMPACT_MTR_RDG_QC_PRESENT,COMPACT_MTR_POWER10_SHIFT) not referenced */
/*lint -emacro(755,EVENT_AND_READING_QTY_SIZE,EVENT_AND_READING_QTY_SIZE_BITS,READING_QTY_MAX,ED_EVENT_TYPE_SIZE,ED_EVENT_TYPE_SIZE_BITS) not referenced */
/*lint -emacro(755,NVP_DET_VALUE_SIZE,NVP_DET_VALUE_SIZE_BITS,NVP_QTY_SIZE_BITS,EVENT_DETAIL_VALUE_SIZE_BITS,ED_EVENT_DETAILS_NAME_SIZE) not referenced */
/*lint -emacro(755,ED_EVENT_DETAILS_NAME_SIZE_BITS,ED_EVENT_CREATED_DATE_TIME_SIZE,READING_INFO_SIZE,READING_INFO_SIZE_BITS,COINC_READING_INFO_SIZE) not referenced */
/*lint -emacro(755,READING_TYPE_SIZE,READING_TYPE_SIZE_BITS,READING_TIME_STAMP_SIZE,READING_TIME_STAMP_SIZE_BITS,READING_QUALITY_SIZE) not referenced */
/*lint -emacro(755,READING_QUALITY_SIZE_BITS,COMP_RDG_VAL_SIZE_MASK,COMP_RDG_VAL_SIZE_SHIFT,RDG_VAL_SIZE_L_MASK,RDG_VAL_SIZE_U_MASK,RDG_VAL_SIZE_U_SHIFT) not referenced */
/*lint -emacro(755,APP_HDR_SIZE,EXCH_WITH_ID_RDG_TS_PRESENT,EXCH_WITH_ID_RDG_QC_PRESENT) not referenced */
#define CIM_APP_MSG_READING_VALUE_SIZE   ((uint8_t)8)
#define DFW_APP_MSG_READING_VALUE_SIZE   ((uint8_t)25)
#define VALUES_INTERVAL_END_SIZE         ((uint8_t)4)    /*Size of valuesInterval.end field */
#define VALUES_INTERVAL_END_SIZE_BITS    (VALUES_INTERVAL_END_SIZE * 8)
#define VALUES_INTERVAL_START_SIZE       ((uint8_t)4)    /*Size of valuesInterval.start field */
#define VALUES_INTERVAL_START_SIZE_BITS  (VALUES_INTERVAL_START_SIZE * 8)
#define FULL_MTR_RDG_SIZE_SHIFT          ((uint8_t)4)

#define COMPACT_MTR_RDG_SIZE_SHIFT       ((uint8_t)0)

#define COMPACT_MTR_RDG_TS_PRESENT       ((uint8_t)0x80)
#define COMPACT_MTR_RDG_QC_PRESENT       ((uint8_t)0x40)

#define COMPACT_MTR_POWER10_SHIFT        ((uint8_t)3)

#define EVENT_AND_READING_QTY_SIZE        ((uint8_t)1)    /*Size of EDEventQty and ReadingQty fields */
#define EVENT_AND_READING_QTY_SIZE_BITS   (EVENT_AND_READING_QTY_SIZE *8)
#define READING_QTY_MAX                   ((uint8_t)15)  /* Max number of readings per bit structure */
#define ED_EVENT_TYPE_SIZE                ((uint8_t)2)
#define ED_EVENT_TYPE_SIZE_BITS           (ED_EVENT_TYPE_SIZE * 8)
#define NVP_DET_VALUE_SIZE                ((uint8_t)1)
#define NVP_DET_VALUE_SIZE_BITS           (NVP_DET_VALUE_SIZE * 8)
#define NVP_QTY_SIZE_BITS                 ((uint8_t)4)
#define EVENT_DETAIL_VALUE_SIZE_BITS      ((uint8_t)4)
#define ED_EVENT_DETAILS_NAME_SIZE        ((uint8_t)2)
#define ED_EVENT_DETAILS_NAME_SIZE_BITS   (ED_EVENT_DETAILS_NAME_SIZE * 8)
#define ED_EVENT_CREATED_DATE_TIME_SIZE   ((uint8_t)4)
#define READING_INFO_SIZE                 ((uint8_t)3)   /* Contains bit fields for the Reading */
#define READING_INFO_SIZE_BITS            (READING_INFO_SIZE * 8)
#define COINC_READING_INFO_SIZE           ((uint8_t)1)   /* Special case for GetCoincidentReads bit structure */
#define COINC_READING_INFO_SIZE_BITS      (COINC_READING_INFO_SIZE * 8)
#define READING_TYPE_SIZE                 ((uint8_t)sizeof(meterReadingType))
#define READING_TYPE_SIZE_BITS            (READING_TYPE_SIZE * 8)

#define READING_TIME_STAMP_SIZE           ((uint8_t)4)
#define READING_TIME_STAMP_SIZE_BITS      (READING_TIME_STAMP_SIZE * 8)
#define READING_QUALITY_SIZE              ((uint8_t)1)
#define READING_QUALITY_SIZE_BITS         (READING_QUALITY_SIZE * 8)

#define COMP_RDG_VAL_SIZE_MASK            ((uint8_t)0x07)
#define COMP_RDG_VAL_SIZE_SHIFT           ((uint8_t)0)

/* The reading size fields within the Reading Info bit structure */
#define RDG_VAL_SIZE_L_MASK               ((uint8_t)0x0F)
#define RDG_VAL_SIZE_U_MASK               ((uint16_t)0xFFF0)
#define RDG_VAL_SIZE_U_SHIFT              ((uint8_t)4)

#define APP_HDR_SIZE                      ((uint16_t)4)

#define EXCH_WITH_ID_RDG_TS_PRESENT       ((uint8_t)0x40)
#define EXCH_WITH_ID_RDG_QC_PRESENT       ((uint8_t)0x20)

/* TYPE DEFINITIONS */

/* Transaction Types */
typedef enum
{
   TT_REQUEST     = 0,
   TT_RESPONSE    = 1,
   TT_ASYNC       = 2,
   LAST_VALID_TRANS_TYPE = TT_ASYNC,
   TT_RESERVED    = 3
} enum_TransactionType;

/* From the HEEP document - enumerated Resources*/
typedef enum
{
   bu_am    = 1,    /* Bubble up alarm */
   bu_ds    = 2,    /* Bubble up of the daily shifted data */
   bu_lp    = 3,    /* Bubble up of load profile (interval) data */
   bu_pd    = 4,    /* Bubble up of phase detection data */
   pd       = 5,    /* Phase detect survey command */
   cf_idc   = 6,    /* Read/Write interval data recording configuration */
   rg_cf    = 7,    /* Write configuration data */
   cf_idp   = 8,    /* Set interval data publication configuration */
   cf_mr    = 9,    /* Set on-request meter reading response */
   tr       = 10,   /* Trace Route */
   co_st    = 11,   /* DC status report */
   df_ap    = 12,   /* DFW apply */
   df_ca    = 13,   /* DFW cancellation */
   df_co    = 14,   /* DFW complete */
   df_dp    = 15,   /* DFW download packet */
   df_in    = 16,   /* DFW initialize */
   df_vr    = 17,   /* DFW verify */
   dr       = 18,   /* Demand Reset */
   lc       = 19,   /* Load Control */
   mm       = 20,   /* Read/Write memory */
   mm_c     = 21,   /* Clear memory */
   mm_re    = 22,   /* Reset to initial values */
   or_am    = 23,   /* On-request alarm status */
   or_ha    = 24,   /* Get historical alarms */
   or_ds    = 25,   /* Request/reply daily shifted data */
   or_lp    = 26,   /* Get historical LP readings */
   rg_md    = 27,   /* New EP device metadata */
   or_mr    = 28,   /* Request/reply meter reading */
   or_mt    = 29,   /* Request/reply meter table */
   or_pm    = 30,   /* Request/reply parameter R/W */
   rc       = 31,   /* Switch RCD open/close/arm-for-close */
   st       = 32,   /* Execute Mfg self-test */
   st_cmt   = 33,   /* Configure manufacturing self-test */
   st_cpt   = 34,   /* Check/Configure periodic self-test */
   st_x     = 35,   /* Execute specified tests */
   or_cr    = 36,   /* Get coincident reads */
   dc_rg_md = 37,   /* New DCU device metadata */
   bu_en    = 38,   /* Bubble up engineering data */
   dc_bu_am = 39,   /* Bubble-up DCU alarm messages (proposed) */
   dc_or_pm = 40,   /* read/write DCU parameter (proposed) */
   tn_tw_mp = 41,   /* Tunnel/TWACS/manufacturing port */
   tn_st_dc = 42,   /* Tunnel/STAR/DCU command */
   LAST_VALID_RESOURCE = tn_st_dc   /* Must be set to last available resouce */

} enum_MessageResource;

/* Method/Status byte enumerations */
typedef enum
{
   method_get     = 1,
   method_post    = 2,
   method_put     = 3,
   method_deleted = 4,
   LAST_VALID_METHOD = method_deleted  /* Must be set to last valid Method */
} enum_MessageMethod;

/* Contiunation (in HEEP) of Method/Status - status return values */
typedef enum
{
   Continue                = (uint8_t)32,
   Processing              = 34,
   OK                      = 64,
   Created                 = 65,
   PartialSuccess          = 67,
   NoContent               = 68,
   ResetContent            = 69,
   PartialContent          = 70,
   NotModified             = 100,
   BadRequest              = 128,
   NotAuthorized           = 129,
   NotFound                = 132,
   MethodNotAllowed        = 133,
   Timeout                 = 136,
   Conflict                = 137,
   Gone                    = 138,
   RequestedEntityTooLarge = 141,
   UnsupportedMediaType    = 143,
   InternalServerError     = 160,
   BadGateway              = 162,
   ServiceUnavailable      = 163,
   GatewayTimeout          = 164
} enum_status;

/* Time range structure */
PACK_BEGIN
typedef struct PACK_MID
{
   uint32_t       startTime; /* starting event time range (seconds since epoch) */
   uint32_t       endTime;   /* starting event time range (seconds since epoch) */
} timeRange_t, *pTimeRange_t;
PACK_END

/* Compact Meter read (response) structure */
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t        eventInfo;        /* time stamp present      1 bit
                                       Quality code present    1 bit
                                       Pending Power of 10 adj 3 bits
                                       Readings value size     3 bits */
   uint16_t       eventType;
   uint32_t       eventTime;        /* Only present if time stamp present bit (above) set */
   uint8_t        ReadingQuality;   /* Reading quality code, if any, plus msb set if more qual codes */
   uint8_t        ReadingValue[CIM_APP_MSG_READING_VALUE_SIZE];  /* Actual reading */
} MeterRead, *pMeterRead;
PACK_END

typedef enum
{
   CIM_QUALCODE_SUCCESS                   =  0,
   CIM_QUALCODE_SERVICE_DISCONNECT        =  1, /* 1.2.3 */
   CIM_QUALCODE_POWER_FAIL                =  2, /* 1.2.32 */
   CIM_QUALCODE_OVERFLOW                  =  3, /* 1.4.1 */
   CIM_QUALCODE_PARTIAL_INTERVAL          =  4, /* 1.4.2 */
   CIM_QUALCODE_LONG_INTERVAL             =  5, /* 1.4.3 */
   CIM_QUALCODE_SKIPPED_INTERVAL          =  6, /* 1.4.4 */
   CIM_QUALCODE_TEST_DATA                 =  7, /* 1.4.5 */
   CIM_QUALCODE_CONFIG_CHANGE             =  8, /* 1.4.6 */
   CIM_QUALCODE_NOT_RECORDING             =  9, /* 1.4.7 */
   CIM_QUALCODE_RESET_OCCURRED            = 10, /* 1.4.8 */
   CIM_QUALCODE_HMC_CLOCK_CHANGE          = 11, /* 1.4.9 */
   CIM_QUALCODE_LOAD_CONTROL              = 12, /* 1.4.10 */
   CIM_QUALCODE_DST_TIME_CHANGE           = 13, /* 1.4.259 */
   CIM_QUALCODE_SIGNIFICANT_TIME_BIAS     = 28, /* 2.4.63 */
   CIM_QUALCODE_DATA_OUTSIDE_RANGE        = 29, /* 2.5.256 */
   CIM_QUALCODE_ERROR_CODE                = 30, /* 2.5.257 */
   CIM_QUALCODE_SUSPECT                   = 31, /* 2.5.258 */
   CIM_QUALCODE_KNOWN_MISSING_READ        = 32, /* 2.5.259 */
   CIM_QUALCODE_ESTIMATED                 = 33, /* 2.8.0 */
   CIM_QUALCODE_CODE_INDETERMINATE        = 34, /* 2.10.0 */
   CIM_QUALCODE_CODE_READ_ONLY            = 37, /* 2.6.1002 */
   CIM_QUALCODE_CODE_OVERFLOW             = 38, /* 2.4.1 */
   CIM_QUALCODE_CODE_UNKNOWN_READING_TYPE = 39, /* 2.6.1001 */
   CIM_QUALCODE_CODE_TYPECAST_MISMATCH    = 40,  /* 2.6.1003 */
   CIM_QUALCODE_NOT_TESTABLE              = 90 // RCZ Added for Meter Programming
} enum_CIM_QualityCode;

/* From HEEP document - enumerated Reading Type Codes, Y84000-42-DSD_ReadingTypeIDsRevD20201130.xlsx */
typedef enum
{
   invalidReadingType               =     0, /* Not supported */
   meterDateTime                    =     1, /* The local time (from the meter's clock) uncorrected, and expressed as
                                                the number of seconds since midnight 1/1/70. */
/*                                       2-3    Reserved for future use. */
   minDeviceTemperature             =     4, /* Minimum indicating device temperature (degC). Over life of EndDevice (as
                                                measured using ED thermometer.)*/
   maxDeviceTemperature             =     5, /* Maximum indicating device temperature (degC). Over life of EndDevice (as
                                                measured using ED thermometer.)*/
   energization                     =     6, /* bulkQuantity electricitySecondaryMetered energization (count). Power up
                                                counter*/
   demandResetCount                 =     7, /* bulkQuantity electricitySecondaryMetered demandReset (count) */
   tamper                           =     8, /* bulkQuantity electricitySecondaryMetered tamper (count). Inversion
                                                counter */
   sag                              =     9, /* bulkQuantity electricitySecondaryMetered sag (count) */
   swell                            =    10, /* bulkQuantity electricitySecondaryMetered swell (count) */
   fwdkVAh                          =    11, /* bulkQuantity forward electricitySecondaryMetered energy (kVAh) */
   fwdkWh                           =    12, /* bulkQuantity forward electricitySecondaryMetered energy (kWh) */
   fwdkVArh                         =    13, /* bulkQuantity forward electricitySecondaryMetered energy (kVArh) */
   /* 14 reserved for future use.  */
   revkWh                           =    15, /* bulkQuantity reverse electricitySecondaryMetered energy (kWh) */
   revkVArh                         =    16, /* bulkQuantity reverse electricitySecondaryMetered energy (kVArh) */
   totkVAh                          =    17, /* bulkQuantity total electricitySecondaryMetered energy (kVAh) */
   totkQh                           =    18, /* bulkQuantity total electricitySecondaryMetered energy (kQh) */
   totkWh                           =    19, /* bulkQuantity total electricitySecondaryMetered energy (kWh) */
   netkWh                           =    20, /* bulkQuantity net electricitySecondaryMetered energy (kWh) */
   netkVArh                         =    21, /* bulkQuantity net electricitySecondaryMetered energy (kVArh) */
   fwdCumKW                         =    22, /* cumulative forward electricitySecondaryMetered demand (kW) */
   Hz                               =    23, /* indicating electricitySecondaryMetered frequency (Hz) */
   pF                               =    24, /* indicating electricitySecondaryMetered powerFactor (cos theta) */
   iA                               =    25, /* indicating electricitySecondaryMetered current phaseA (A) */
   iPhaseN                          =    26, /* indicating electricitySecondaryMetered current phaseN (A) */
   iC                               =    27, /* indicating electricitySecondaryMetered current phaseC (A) */
   iB                               =    28, /* indicating electricitySecondaryMetered current phaseB (A) */
   iPhAtoAv                         =    29, /* indicating electricitySecondaryMetered currentAngle phaseAtoAv (deg) */
   iPhCtoAv                         =    30, /* indicating electricitySecondaryMetered currentAngle phaseCtoAv (deg) */
   iPhBtoAv                         =    31, /* indicating electricitySecondaryMetered currentAngle phaseBtoAv (deg) */
   edTemperature                    =    32, /* indicating electricitySecondaryMetered temperature (degC) */
   vRMS                             =    33, /* indicating electricitySecondaryMetered voltage-rms (V) */
   vRMSA                            =    34, /* indicating electricitySecondaryMetered voltage-rms phaseA (V) */
   vRMSC                            =    35, /* indicating electricitySecondaryMetered voltage-rms phaseC (V) */
   vRMSB                            =    36, /* indicating electricitySecondaryMetered voltage-rms phaseB (V) */
   vPhAtoAv                         =    37, /* indicating electricitySecondaryMetered voltageAngle phaseAtoAv (deg) */
   vPhCtoAv                         =    38, /* indicating electricitySecondaryMetered voltageAngle phaseCtoAv (deg) */
   vPhBtoAv                         =    39, /* indicating electricitySecondaryMetered voltageAngle phaseBtoAv (deg) */
   vImbalance                       =    40, /* bulkQuantity electricitySecondaryMetered voltageImbalance (count) */
   switchPosition                   =    41, /* Switching cause code as reported by meter events table with i210+c EndDeviceEvents 53 and 54. */
   fwdkW                            =    42, /* indicating forward electricitySecondaryMetered power (kW) */
   fwdkVA                           =    43, /* indicating forward electricitySecondaryMetered power (kVA) */
   fwdkVAr                          =    44, /* indicating forward electricitySecondaryMetered power (kVAr) */
   maxPresFwdDmdKW                  =    45, /* maximum present indicating forward electricitySecondaryMetered demand (kW) */
   maxPresFwdDmdKVA                 =    46, /* maximum present indicating forward electricitySecondaryMetered demand (kVA) */
   maxPresFwdDmdKVAr                =    47, /* maximum present indicating forward electricitySecondaryMetered demand (kVAr) */
   maxPrevFwdDmdTouAkW              =    48, /* maximum previous indicating forward electricitySecondaryMetered demand TouA (kW) */
   maxPrevFwdDmdTouAkVAr            =    49, /* maximum previous indicating forward electricitySecondaryMetered demand TouA (kVAr) */
   maxPrevFwdDmdTouBkW              =    50, /* maximum previous indicating forward electricitySecondaryMetered demand TouB (kW) */
   maxPrevFwdDmdTouBkVAr            =    51, /* maximum previous indicating forward electricitySecondaryMetered demand TouB (kVAr) */
   revI                             =    52, /* bulkQuantity reverse electricitySecondaryMetered current (count) */
   crossPhaseDetected               =    53, /* bulkQuantity reverse electricitySecondaryMetered current phasesABC
                                                (count)*/
   zeroFlowDuration                 =    54, /* bulkQuantity electricitySecondaryMetered zeroFlowDuration (count) */
   maxPrevTotDmdKW                  =    55, /* Maximum previous indicating total electricitySecondaryMetered demand (kW) */
   maxPrevNetDmdKW                  =    56, /* Max previous indicating net electricitySecondaryMetered demand (kW) */
   fwdTouAkWh                       =    57, /* summation forward electricitySecondaryMetered energy TouA (kWh) */
   fwdTouBkWh                       =    58, /* summation forward electricitySecondaryMetered energy TouB (kWh) */
   fwdTouCkWh                       =    59, /* summation forward electricitySecondaryMetered energy TouC (kWh) */
   totTouAkWh                       =    60, /* summation total electricitySecondaryMetered energy TouA (kWh) */
   totTouBkWh                       =    61, /* summation total electricitySecondaryMetered energy TouB (kWh) */
   totTouCkWh                       =    62, /* summation total electricitySecondaryMetered energy TouC (kWh) */
   qhDeltaFwdkWh                    =    63, /* fifteenMinute deltaData forward electricitySecondaryMetered energy (kWh) */
   qhDeltaFwdkVArh                  =    64, /* fifteenMinute deltaData forward electricitySecondaryMetered energy (kVArh) */
   qhDeltaFwdFundkWh                =    65, /* fifteenMinute deltaData forward electricitySecondaryMetered energy fundamental (kWh) */
   qhDeltaRevkWh                    =    66, /* fifteenMinute deltaData reverse electricitySecondaryMetered energy (kWh) */
   qhDeltaRevkVArh                  =    67, /* fifteenMinute deltaData reverse electricitySecondaryMetered energy (kVArh) */
   qhDeltaTotkQh                    =    68, /* fifteenMinute deltaData total electricitySecondaryMetered energy (kQh) */
   qhDeltaTotkWh                    =    69, /* fifteenMinute deltaData total electricitySecondaryMetered energy (kWh) */
   qhDeltaNetkWh                    =    70, /* fifteenMinute deltaData net electricitySecondaryMetered energy (kWh) */
   qhDeltaNetkVArh                  =    71, /* fifteenMinute deltaData net electricitySecondaryMetered energy (kVArh) */
   fiveDeltaFwdkVArh                =    72, /* fiveMinute deltaData forward electricitySecondaryMetered energy (kVArh) */
   fiveDeltaFwdkWh                  =    73, /* fiveMinute deltaData forward electricitySecondaryMetered energy (kWh) */
   fiveDeltaFwdFundkWh              =    74, /* fiveMinute deltaData forward electricitySecondaryMetered energy fundamental (kWh) */
   fiveDeltaRevkWh                  =    75, /* fiveMinute deltaData reverse electricitySecondaryMetered energy (kWh) */
   fiveDeltaRevkVArh                =    76, /* fiveMinute deltaData reverse electricitySecondaryMetered energy (kVArh) */
   fiveDeltaTotkQh                  =    77, /* fiveMinute deltaData total electricitySecondaryMetered energy (kQh) */
   fiveDeltaTotkWh                  =    78, /* fiveMinute deltaData total electricitySecondaryMetered energy (kWh) */
   fiveDeltaNetkWh                  =    79, /* fiveMinute deltaData net electricitySecondaryMetered energy (kWh) */
   fiveDeltaNetkVArh                =    80, /* fiveMinute deltaData net electricitySecondaryMetered energy (kVArh) */
   qhIndPf                          =    81, /* average fifteenMinute indicating electricitySecondaryMetered powerFactor
                                                (cos?) */
   thd                              =    82, /* high electricitySecondaryMetered totalHarmonicDistortion (count) */
   thdA                             =    83, /* high electricitySecondaryMetered totalHarmonicDistortion phA (count) */
   thdC                             =    84, /* high electricitySecondaryMetered totalHarmonicDistortion phC (count) */
   thdB                             =    85, /* high electricitySecondaryMetered totalHarmonicDistortion phB (count) */
   vAngle                           =    86, /* excess bulkQuantity electricitySecondaryMetered voltageAngle (count) */
   phN                              =    87, /* excess bulkQuantity (highThreshold?) electricitySecondaryMetered current phaseN (count) */
   maxPrevFwdDmdKW                  =    88, /* maximum previous indicating forward electricitySecondaryMetered demand(kW)*/
   maxPrevNetDmdTouAkW              =    89, /* maximum previous indicating net electricitySecondaryMetered demand touA (kW) */
   maxPrevNetDmdTouBkW              =    90, /* maximum previous indicating net electricitySecondaryMetered demand touB (kW) */
   maxPrevNetDmdTouCkW              =    91, /* maximum previous indicating net electricitySecondaryMetered demand touC (kW) */
   maxPrevNetDmdTouDkW              =    92, /* maximum previous indicating net electricitySecondaryMetered demand touD (kW) */
   maxPrevFwdDmdTouCkW              =    93, /* maximum previous indicating forward electricitySecondaryMetered demand TouC (kW) */
   maxPrevFwdDmdFundkW              =    94, /* maximum previous indicating forward electricitySecondaryMetered demand fundamental (kW) */
   maxPrevRevDmdFundkW              =    95, /* maximum previous indicating reverse electricitySecondaryMetered demand fundamental (kW) */
   maxPrevTotDmdTouDkW              =    96, /* maximum previous indicating total electricitySecondaryMetered demand touD (kW) */
   maxPrevTotDmdTouAkW              =    97, /* maximum previous indicating total electricitySecondaryMetered demand TouA (kW) */
   maxPrevTotDmdTouBkW              =    98, /* maximum previous indicating total electricitySecondaryMetered demand TouB (kW) */
   maxPrevTotDmdTouCkW              =    99, /* maximum previous indicating total electricitySecondaryMetered demand TouC (kW) */
   dailyMaxFwdDmdKW                 =   100, /* daily maximum indicating forward electricitySecondaryMetered demand (kW) */
   netTouCkVarh                     =   101, /* summation net electricitySecondaryMetered energy touC (kVArh) */
   netTouBkVarh                     =   102, /* summation net electricitySecondaryMetered energy TouB (kVArh) */
   netTouAkVArh                     =   103, /* summation net electricitySecondaryMetered energy TouA (kVArh) */
   netTouDkVArh                     =   104, /* summation net electricitySecondaryMetered energy TouD (kVArh) */
   netTouAkWh                       =   105, /* summation net electricitySecondaryMetered energy touA (kWh) */
   netTouBkWh                       =   106, /* summation net electricitySecondaryMetered energy touB (kWh) */
   netTouCkWh                       =   107, /* summation net electricitySecondaryMetered energy touC (kWh) */
   netTouDkWh                       =   108, /* summation net electricitySecondaryMetered energy touD (kWh) */
   /* 109 reserved */
   sixtyDeltaFwdkWh                 =   110, /* sixtyMinute deltaData forward electricitySecondaryMetered energy (kWh) */
   sixtyDeltaFwdkVArh               =   111, /* sixtyMinute deltaData forward electricitySecondaryMetered energy (kVArh) */
   sixtyDeltaFwdFundkWh             =   112, /* sixtyMinute deltaData forward electricitySecondaryMetered energy fundamental (kWh) */
   sixtyDeltaRevkWh                 =   113, /* sixtyMinute deltaData reverse electricitySecondaryMetered energy (kWh) */
   sixtyDeltaRevkVArh               =   114, /* sixtyMinute deltaData reverse electricitySecondaryMetered energy (kVArh) */
   sixtyDeltaTotkQh                 =   115, /* sixtyMinute deltaData total electricitySecondaryMetered energy (kQh) */
   sixtyDeltaTotkWh                 =   116, /* sixtyMinute deltaData total electricitySecondaryMetered energy (kWh) */
   sixtyDeltaNetkWh                 =   117, /* sixtyMinute deltaData net electricitySecondaryMetered energy (kWh) */
   sixtyDeltaNetkVArh               =   118, /* sixtyMinute deltaData net electricitySecondaryMetered energy (kVArh) */
   thirtyDeltaFwdkWh                =   119, /* thirtyMinute deltaData forward electricitySecondaryMetered energy (kWh) */
   thirtyDeltaFwdkVArh              =   120, /* thirtyMinute deltaData forward electricitySecondaryMetered energy (kVArh) */
   thirtyDeltaFwdFundkWh            =   121, /* thirtyMinute deltaData forward electricitySecondaryMetered energy fundamental (kWh) */
   thirtyDeltaRevkWh                =   122, /* thirtyMinute deltaData reverse electricitySecondaryMetered energy (kWh) */
   thirtyDeltaRevkVArh              =   123, /* thirtyMinute deltaData reverse electricitySecondaryMetered energy (kVArh) */
   thirtyDeltaTotkQh                =   124, /* thirtyMinute deltaData total electricitySecondaryMetered energy (kQh) */
   thirtyDeltaTotkWh                =   125, /* thirtyMinute deltaData total electricitySecondaryMetered energy (kWh) */
   thirtyDeltaNetkWh                =   126, /* thirtyMinute deltaData net electricitySecondaryMetered energy (kWh) */
   thirtyDeltaNetkVArh              =   127, /* thirtyMinute deltaData net electricitySecondaryMetered energy (kVArh) */
   powerQuality                     =   128, /* bulkQuantity electricitySecondaryMetered powerQuality (count) */
   switchPositionStatus             =   129, /* electricitySecondaryMetered switchPosition (status) */
   energizationLoadSide             =   130, /* electricitySecondaryMetered energizationLoadSide (status) */
   switchPositionCount              =   131, /* bulkQuantity electricitySecondaryMetered switchPosition (count) */
   fiveDeltaTotkVAh                 =   132, /* fiveMinute deltaData total electricitySecondaryMetered energy (kVAh) */
   qhDeltaTotkVAh                   =   133, /* fifteenMinute deltaData total electricitySecondaryMetered energy (kVAh) */
   thirtyDeltaTotkVAh               =   134, /* thirtyMinute deltaData total electricitySecondaryMetered energy (kVAh) */
   sixtyDeltaTotkVAh                =   135, /* sixtyMinute deltaData total electricitySecondaryMetered energy (kVAh) */
   fwdTouAkVArh                     =   136, /* summation forward electricitySecondaryMetered energy TouA (kVArh)    */
   fwdTouBkVArh                     =   137, /* summation forward electricitySecondaryMetered energy TouB (kVArh)    */
   fwdTouCkVArh                     =   138, /* summation forward electricitySecondaryMetered energy TouC (kVArh)    */
   fwdTouDkWh                       =   139, /* summation forward electricitySecondaryMetered energy TouD (kWh)   */
   fwdTouDkVArh                     =   140, /* summation forward electricitySecondaryMetered energy TouD (kVArh)    */
   fwdTouEkWh                       =   141, /* summation forward electricitySecondaryMetered energy TouE (kWh)   */
   fwdTouEkVArh                     =   142, /* summation forward electricitySecondaryMetered energy TouE (kVArh)    */
   revTouAkWh                       =   143, /* summation reverse electricitySecondaryMetered energy TouA (kWh)   */
   revTouAkVArh                     =   144, /* summation reverse electricitySecondaryMetered energy TouA (kVArh)    */
   revTouBkWh                       =   145, /* summation reverse electricitySecondaryMetered energy TouB (kWh)   */
   revTouBkVArh                     =   146, /* summation reverse electricitySecondaryMetered energy TouB (kVArh)    */
   revTouCkWh                       =   147, /* summation reverse electricitySecondaryMetered energy TouC (kWh)   */
   revTouCkVArh                     =   148, /* summation reverse electricitySecondaryMetered energy TouC (kVArh)    */
   revTouDkWh                       =   149, /* summation reverse electricitySecondaryMetered energy TouD (kWh)   */
   revTouDkVArh                     =   150, /* summation reverse electricitySecondaryMetered energy TouD (kVArh)    */
   revTouEkWh                       =   151, /* summation reverse electricitySecondaryMetered energy TouE (kWh)   */
   revTouEkVArh                     =   152, /* summation reverse electricitySecondaryMetered energy TouE (kVArh)    */
   qhDeltaDataFwdkVAh               =   153, /* fifteenMinute deltaData forward electricitySecondaryMetered energy (kVAh)  */
   qhDeltaDataRevkVAh               =   154, /* fifteenMinute deltaData reverse electricitySecondaryMetered energy (kVAh)  */
   fiveDeltaDataFwdkVAh             =   155, /* fiveMinute deltaData forward electricitySecondaryMetered energy (kVAh) */
   fiveDeltaDataRevkVAh             =   156, /* fiveMinute deltaData reverse electricitySecondaryMetered energy (kVAh) */
   secondMaxPrevFwdDmdKW            =   157, /* secondMaximum previous indicating forward electricitySecondaryMetered demand (kW) */
   secondMaxPrevFwdDmdKVAr          =   158, /* secondMaximum previous indicating forward electricitySecondaryMetered demand (kVAr) */
   secondMaxPrevRevDmdKW            =   159, /* secondMaximum previous indicating reverse electricitySecondaryMetered demand (kW) */
   secondMaxPrevRevDmdKVAr          =   160, /* secondMaximum previous indicating reverse electricitySecondaryMetered demand (kVAr) */
   avgQhIndvoltageRMSA              =   161, /* average fifteenMinute indicating electricitySecondaryMetered voltage-rms phaseA (V) */
   avgQhIndvoltageRMSC              =   162, /* average fifteenMinute indicating electricitySecondaryMetered voltage-rms phaseC (V) */
   avgQhIndvoltageRMSB              =   163, /* average fifteenMinute indicating electricitySecondaryMetered voltage-rms phaseB (V) */
   avgFiveIndvoltageRMSA            =   164, /* average fiveMinute indicating electricitySecondaryMetered voltage-rms phaseA (V) */
   avgFiveIndvoltageRMSC            =   165, /* average fiveMinute indicating electricitySecondaryMetered voltage-rms phaseC (V) */
   avgFiveIndvoltageRMSB            =   166, /* average fiveMinute indicating electricitySecondaryMetered voltage-rms phaseB (V) */
/*                                      167,    Not used */
   maxPrevFwdDmdTouCkVAr            =   168, /* Maximum previous indicating forward electricitySecondaryMetered demand TouC (kVAr) */
   maxPrevFwdDmdTouDkW              =   169, /* maximum previous indicating forward electricitySecondaryMetered demand TouD (kW) */
   maxPrevFwdDmdTouDkVAr            =   170, /* Maximum previous indicating forward electricitySecondaryMetered demand TouD (kVAr) */
   maxPrevFwdDmdTouEkW              =   171, /* Maximum previous indicating forward electricitySecondaryMetered demand TouE (kW) */
   maxPrevFwdDmdTouEkVAr            =   172, /* Maximum previous indicating forward electricitySecondaryMetered demand TouE (kVAr) */
   maxPrevRevDmdTouAkW              =   173, /* Maximum previous indicating reverse electricitySecondaryMetered demand TouA (kW) */
   maxPrevRevDmdTouAkVAr            =   174, /* maximum previous indicating reverse electricitySecondaryMetered demand TouA (kVAr) */
   maxPrevRevDmdTouBkW              =   175, /* maximum previous indicating reverse electricitySecondaryMetered demand TouB (kW) */
   maxPrevRevDmdTouBkVAr            =   176, /* maximum previous indicating reverse electricitySecondaryMetered demand TouB (kVAr) */
   maxPrevRevDmdTouCkW              =   177, /* maximum previous indicating reverse electricitySecondaryMetered demand TouC (kW) */
   maxPrevRevDmdTouCkVAr            =   178, /* Maximum previous indicating reverse electricitySecondaryMetered demand TouC (kVAr) */
   maxPrevRevDmdTouDkW              =   179, /* maximum previous indicating reverse electricitySecondaryMetered demand TouD (kW) */
   maxPrevRevDmdTouDkVAr            =   180, /* maximum previous indicating reverse electricitySecondaryMetered demand TouD (kVAr) */
   maxPrevRevDmdTouEkW              =   181, /* maximum previous indicating reverse electricitySecondaryMetered demand TouE (kW) */
   maxPrevRevDmdTouEkVAr            =   182, /* maximum previous indicating reverse electricitySecondaryMetered demand TouE (kVAr) */
   maxPresRevDmdKW                  =   183, /* maximum present indicating reverse electricitySecondaryMetered demand (kW) */
   maxPresRevDmdKVA                 =   184, /* maximum present indicating reverse electricitySecondaryMetered demand (kVA) */
   dailyMaxTotDmdKW                 =   185, /* daily maximum indicating total electricitySecondaryMetered demand (kW)*/
   maxPrevFwdDmdKVA                 =   186, /* maximum previous indicating forward electricitySecondaryMetered demand (kVA) */
   maxPrevFwdDmdKVAr                =   187, /* maximum previous indicating forward electricitySecondaryMetered demand (kVAr) */
   maxPrevRevDmdKW                  =   188, /* maximum previous indicating reverse electricitySecondaryMetered demand (kW) */
   maxPrevRevDmdKVA                 =   189, /* maximum previous indicating reverse electricitySecondaryMetered demand (kVA) */
   maxIndVRMSA                      =   190, /* maximum indicating electricitySecondaryMetered voltage-rms phaseA (V) */
   maxIndVRMSC                      =   191, /* maximum indicating electricitySecondaryMetered voltage-rms phaseC (V) */
   maxIndVRMSB                      =   192, /* maximum indicating electricitySecondaryMetered voltage-rms phaseB (V) */
   dailyMaxNetDmdKW                 =   193, /* daily maximum indicating net electricitySecondaryMetered demand (kW)*/
   dailyMaxFwdDmdkVAr               =   194, /* daily maximum indicating forward electricitySecondaryMetered demand (kVAr)*/
   dailyMaxTotDmdkVAr               =   195, /* daily maximum indicating total electricitySecondaryMetered demand (kVAr)*/
   minIndVRMSA                      =   196, /* minimum indicating electricitySecondaryMetered voltage-rms phaseA (V) */
   minIndVRMSC                      =   197, /* minimum indicating electricitySecondaryMetered voltage-rms phaseC (V) */
   minIndVRMSB                      =   198, /* minimum indicating electricitySecondaryMetered voltage-rms phaseB (V) */
   dailyMaxNetDmdkVAr               =   199, /* daily maximum indicating net electricitySecondaryMetered demand (kVAr)*/
   dailyMaxTotDmdkVA                =   200, /* daily maximum indicating total electricitySecondaryMetered demand (kVA) */
   maxPresFwdDmdTouAkVAr            =   201, /* maximum present indicating forward electricitySecondaryMetered demand TouA (kVAr) */
   dailyMaxFwdDmdkVA                =   202, /* daily maximum indicating forward electricitySecondaryMetered demand (kVA) */
   dailyMaxFwdDmdTouAkW             =   203, /* daily maximum indicating forward electricitySecondaryMetered demand TouA (kW) */
   dailyMaxFwdDmdTouAkVAr           =   204, /* daily maximum indicating forward electricitySecondaryMetered demand TouA (kVAr) */
   dailyMaxFwdDmdTouBkW             =   205, /* daily maximum indicating forward electricitySecondaryMetered demand TouB (kW) */
   dailyMaxFwdDmdTouBkVAr           =   206, /* daily maximum indicating forward electricitySecondaryMetered demand TouB (kVAr) */
   dailyMaxFwdDmdTouCkW             =   207, /* daily maximum indicating forward electricitySecondaryMetered demand TouC (kW) */
   dailyMaxFwdDmdTouCkVAr           =   208, /* daily maximum indicating forward electricitySecondaryMetered demand TouC (kVAr) */
   dailyMaxFwdDmdTouDkW             =   209, /* daily maximum indicating forward electricitySecondaryMetered demand TouD (kW) */
   dailyMaxFwdDmdTouDkVAr           =   210, /* daily maximum indicating forward electricitySecondaryMetered demand TouD (kVAr) */
   dailyMaxFwdDmdTouEkW             =   211, /* daily maximum indicating forward electricitySecondaryMetered demand TouE (kW) */
   dailyMaxFwdDmdTouEkVAr           =   212, /* daily maximum indicating forward electricitySecondaryMetered demand TouE (kVAr) */
   dailyMaxRevDmdKW                 =   213, /* daily max indicating reverse electricitySecondaryMetered demand (kW) */
   dailyMaxRevDmdKVA                =   214, /* daily maximum indicating reverse electricitySecondaryMetered demand (kVA) */
   dailyMaxRevDmdKVAr               =   215, /* daily maximum indicating reverse electricitySecondaryMetered demand (kVAr) */
   dailyMaxRevDmdTouAkW             =   216, /* daily maximum indicating reverse electricitySecondaryMetered demand TouA (kW) */
   dailyMaxRevDmdTouAkVAr           =   217, /* daily maximum indicating reverse electricitySecondaryMetered demand TouA (kVAr) */
   dailyMaxRevDmdTouBkW             =   218, /* daily maximum indicating reverse electricitySecondaryMetered demand TouB (kW) */
   dailyMaxRevDmdTouBkVAr           =   219, /* daily maximum indicating reverse electricitySecondaryMetered demand TouB (kVAr) */
   dailyMaxRevDmdTouCkW             =   220, /* daily maximum indicating reverse electricitySecondaryMetered demand TouC (kW) */
   dailyMaxRevDmdTouCkVAr           =   221, /* daily maximum indicating reverse electricitySecondaryMetered demand TouC (kVAr) */
   dailyMaxRevDmdTouDkW             =   222, /* daily maximum indicating reverse electricitySecondaryMetered demand TouD (kW) */
   dailyMaxRevDmdTouDkVAr           =   223, /* daily maximum indicating reverse electricitySecondaryMetered demand TouD (kVAr) */
   dailyMaxRevDmdTouEkW             =   224, /* daily maximum indicating reverse electricitySecondaryMetered demand TouE (kW) */
   dailyMaxRevDmdTouEkVAr           =   225, /* daily maximum indicating reverse electricitySecondaryMetered demand TouE (kVAr) */
   instFwdkW                        =   226, /* instantaneous forward electricitySecondaryMetered power (kW) */
   instFwdkVAr                      =   227, /* instantaneous forward electricitySecondaryMetered power (kVAr) */
   maxPresRevDmdKVAr                =   228, /* maximum present indicating reverse electricitySecondaryMetered demand (kVAr) */
   maxPrevRevDmdKVAr                =   229, /* maximum previous indicating reverse electricitySecondaryMetered demand (kVAr) */
   maxPresTotDmdKVA                 =   230, /* maximum present indicating total electricitySecondaryMetered demand (kVa) */
   maxPrevTotDmdKVA                 =   231, /* maximum previous indicating total electricitySecondaryMetered demand (kVa) */
   highThreshIndDeviceTemp          =   232, /* The high temperature threshold for the meter to identify a "hot socket".
                                                In some meters this may be set as part of the meter program. In such a
                                                case this parameter is read-only.*/
                                             /* highThreshold indicating device temperature (degC) */
/*                                      233,    Reserved for future use. */
   maxPrevNetDmdTouAkVAr            =   234, /* maximum previous indicating net electricitySecondaryMetered demand touA (kVAr) */
   maxPrevNetDmdTouBkVAr            =   235, /* maximum previous indicating net electricitySecondaryMetered demand touB (kVAr) */
   maxPrevNetDmdTouCkVAr            =   236, /* maximum previous indicating net electricitySecondaryMetered demand touC (kVAr) */
   maxPrevNetDmdTouDkVAr            =   237, /* maximum previous indicating net electricitySecondaryMetered demand touD (kVAr) */
   maxPrevTotDmdTouAkVAr            =   238, /* maximum previous indicating total electricitySecondaryMetered demand touA (kVAr) */
   maxPrevTotDmdTouBkVAr            =   239, /* maximum previous indicating total electricitySecondaryMetered demand touB (kVAr) */
   maxPrevTotDmdTouCkVAr            =   240, /* maximum previous indicating total electricitySecondaryMetered demand touC (kVAr) */
   maxPrevTotDmdTouDkVAr            =   241, /* maximum previous indicating total electricitySecondaryMetered demand touD (kVAr) */
   totTouDkWh                       =   242, /* summation total electricitySecondaryMetered energy touD (kWh) */
   totTouAkVArh                     =   243, /* summation total electricitySecondaryMetered energy touA (kVArh) */
   totTouBkVArh                     =   244, /* summation total electricitySecondaryMetered energy touB (kVArh) */
   totTouCkVArh                     =   245, /* summation total electricitySecondaryMetered energy touC (kVArh) */
   totTouDkVArh                     =   246, /* summation total electricitySecondaryMetered energy touD (kVArh) */
   totkVArh                         =   247, /* bulkQuantity total electricitySecondaryMetered energy (kVArh) */
   revkVAh                          =   248, /* bulkQuantity reverse electricitySecondaryMetered energy (kVAh) */
   netkVAh                          =   249, /* bulkQuantity net electricitySecondaryMetered energy (kVAh) */
   maxPresNetDmdKW                  =   250, /* maximum present indicating net electricitySecondaryMetered demand (kW) */
   maxPresNetDmdKVA                 =   251, /* maximum present indicating net electricitySecondaryMetered demand (kVA) */
   maxPresNetDmdKVAr                =   252, /* maximum present indicating net electricitySecondaryMetered demand (kVAr) */
   maxPresTotDmdKW                  =   253, /* maximum present indicating total electricitySecondaryMetered demand (kW) */
   maxPresTotDmdKVAr                =   254, /* maximum present indicating total electricitySecondaryMetered demand (kVAr) */
   maxPrevNetDmdKVA                 =   255, /* maximum previous indicating net electricitySecondaryMetered demand (kVA) */
   maxPrevNetDmdKVAr                =   256, /* maximum previous indicating net electricitySecondaryMetered demand (kVAr) */
   maxPrevTotDmdKVAr                =   257, /* maximum previous indicating total electricitySecondaryMetered demand (kVAr) */
   highThresholdswell               =   258, /* highThreshold indicating electricitySecondaryMetered swell (%) */
   lowThresholdsag                  =   259, /* lowThreshold indicating electricitySecondaryMetered sag (%) */
   lowThresholdDuration             =   260, /* lowThreshold indicating electricitySecondaryMetered voltageExcursion (s) */
   sagSwellDuration                 =   261, /* electricitySecondaryMetered voltageExcursion (s) (Actual sag/swell duration?)  */
   revkW                            =   262, /* indicating reverse electricitySecondaryMetered power (kW) */
   revkVA                           =   263, /* indicating reverse electricitySecondaryMetered power (kVA) */
   revkVAr                          =   264, /* indicating reverse electricitySecondaryMetered power (kVAr) */
   q1kVArh                          =   265, /* bulkQuantity quadrant1 electricitySecondaryMetered energy (kVArh) */
   q2kVArh                          =   266, /* bulkQuantity quadrant2 electricitySecondaryMetered energy (kVArh) */
   q3kVArh                          =   267, /* bulkQuantity quadrant3 electricitySecondaryMetered energy (kVArh) */
   q4kVArh                          =   268, /* bulkQuantity quadrant4 electricitySecondaryMetered energy (kVArh) */
   fivedeltaq1kVArh                 =   269, /* fiveMinute deltaData quadrant1 electricitySecondaryMetered energy (kVArh) */
   fivedeltaq2kVArh                 =   270, /* fiveMinute deltaData quadrant2 electricitySecondaryMetered energy (kVArh) */
   fivedeltaq3kVArh                 =   271, /* fiveMinute deltaData quadrant3 electricitySecondaryMetered energy (kVArh) */
   fivedeltaq4kVArh                 =   272, /* fiveMinute deltaData quadrant4 electricitySecondaryMetered energy (kVArh) */
   fifteendeltaq1kVArh              =   273, /* fifteenMinute deltaData quadrant1 electricitySecondaryMetered energy (kVArh) */
   fifteendeltaq2kVArh              =   274, /* fifteenMinute deltaData quadrant2 electricitySecondaryMetered energy (kVArh) */
   fifteendeltaq3kVArh              =   275, /* fifteenMinute deltaData quadrant3 electricitySecondaryMetered energy (kVArh) */
   fifteendeltaq4kVArh              =   276, /* fifteenMinute deltaData quadrant4 electricitySecondaryMetered energy (kVArh) */
   thirtydeltaq1kVArh               =   277, /* thirtyMinute deltaData quadrant1 electricitySecondaryMetered energy (kVArh) */
   thirtydeltaq2kVArh               =   278, /* thirtyMinute deltaData quadrant2 electricitySecondaryMetered energy (kVArh) */
   thirtydeltaq3kVArh               =   279, /* thirtyMinute deltaData quadrant3 electricitySecondaryMetered energy (kVArh) */
   thirtydeltaq4kVArh               =   280, /* thirtyMinute deltaData quadrant4 electricitySecondaryMetered energy (kVArh) */
   sixtydeltaq1kVArh                =   281, /* sixtyMinute deltaData quadrant1 electricitySecondaryMetered energy (kVArh) */
   sixtydeltaq2kVArh                =   282, /* sixtyMinute deltaData quadrant2 electricitySecondaryMetered energy (kVArh) */
   sixtydeltaq3kVArh                =   283, /* sixtyMinute deltaData quadrant3 electricitySecondaryMetered energy (kVArh) */
   sixtydeltaq4kVArh                =   284, /* sixtyMinute deltaData quadrant4 electricitySecondaryMetered energy (kVArh) */
   thdphaseAV                       =   285, /* indicating electricitySecondaryMetered totalHarmonicDistortion phaseA (V) */
   thdphaseBV                       =   286, /* indicating electricitySecondaryMetered totalHarmonicDistortion phaseB (V) */
   thdphaseCV                       =   287, /* indicating electricitySecondaryMetered totalHarmonicDistortion phaseC (V) */
   thdphaseAA                       =   288, /* indicating electricitySecondaryMetered totalHarmonicDistortion phaseA (A) */
   thdphaseBA                       =   289, /* indicating electricitySecondaryMetered totalHarmonicDistortion phaseB (A) */
   thdphaseCA                       =   290, /* indicating electricitySecondaryMetered totalHarmonicDistortion phaseC (A) */
   q1kVAr                           =   291, /* indicating quadrant1 electricitySecondaryMetered power (kVAr) */
   q2kVAr                           =   292, /* indicating quadrant2 electricitySecondaryMetered power (kVAr) */
   q3kVAr                           =   293, /* indicating quadrant3 electricitySecondaryMetered power (kVAr) */
   q4kVAr                           =   294, /* indicating quadrant4 electricitySecondaryMetered power (kVAr) */
   phaseAcos                        =   295, /* indicating electricitySecondaryMetered powerFactor phaseA (cos?) */
   phaseBcos                        =   296, /* indicating electricitySecondaryMetered powerFactor phaseB (cos?) */
   phaseCcos                        =   297, /* indicating electricitySecondaryMetered powerFactor phaseC (cos?) */
   vRMSphAtoBV                      =   298, /* indicating electricitySecondaryMetered voltage-rms phaseAtoB (V) */
   vRMSphBtoCV                      =   299, /* indicating electricitySecondaryMetered voltage-rms phaseBtoC (V) */
   vRMSphCtoAV                      =   300, /* indicating electricitySecondaryMetered voltage-rms phaseCtoA (V) */
   totkVAr                          =   301, /* indicating total electricitySecondaryMetered power (kVAr) */
   netkVAr                          =   302, /* indicating net electricitySecondaryMetered power (kVAr) */
   q1kW                             =   303, /* indicating quadrant1 electricitySecondaryMetered power (kW) */
   q2kW                             =   304, /* indicating quadrant2 electricitySecondaryMetered power (kW) */
   q3kW                             =   305, /* indicating quadrant3 electricitySecondaryMetered power (kW) */
   q4kW                             =   306, /* indicating quadrant4 electricitySecondaryMetered power (kW) */
   q1kWh                            =   307, /* bulkQuantity quadrant1 electricitySecondaryMetered energy (kWh) */
   q2kWh                            =   308, /* bulkQuantity quadrant2 electricitySecondaryMetered energy (kWh) */
   q3kWh                            =   309, /* bulkQuantity quadrant3 electricitySecondaryMetered energy (kWh) */
   q4kWh                            =   310, /* bulkQuantity quadrant4 electricitySecondaryMetered energy (kWh) */
   fivedeltaq1kWh                   =   311, /* fiveMinute deltaData quadrant1 electricitySecondaryMetered energy (kWh) */
   fivedeltaq2kWh                   =   312, /* fiveMinute deltaData quadrant2 electricitySecondaryMetered energy (kWh) */
   fivedeltaq3kWh                   =   313, /* fiveMinute deltaData quadrant3 electricitySecondaryMetered energy (kWh) */
   fivedeltaq4kWh                   =   314, /* fiveMinute deltaData quadrant4 electricitySecondaryMetered energy (kWh) */
   fifteendeltaq1kWh                =   315, /* fifteenMinute deltaData quadrant1 electricitySecondaryMetered energy (kWh) */
   fifteendeltaq2kWh                =   316, /* fifteenMinute deltaData quadrant2 electricitySecondaryMetered energy (kWh) */
   fifteendeltaq3kWh                =   317, /* fifteenMinute deltaData quadrant3 electricitySecondaryMetered energy (kWh) */
   fifteendeltaq4kWh                =   318, /* fifteenMinute deltaData quadrant4 electricitySecondaryMetered energy (kWh) */
   thirtydeltaq1kWh                 =   319, /* thirtyMinute deltaData quadrant1 electricitySecondaryMetered energy (kWh) */
   thirtydeltaq2kWh                 =   320, /* thirtyMinute deltaData quadrant2 electricitySecondaryMetered energy (kWh) */
   thirtydeltaq3kWh                 =   321, /* thirtyMinute deltaData quadrant3 electricitySecondaryMetered energy (kWh) */
   thirtydeltaq4kWh                 =   322, /* thirtyMinute deltaData quadrant4 electricitySecondaryMetered energy (kWh) */
   sixtydeltaq1kWh                  =   323, /* sixtyMinute deltaData quadrant1 electricitySecondaryMetered energy (kWh) */
   sixtydeltaq2kWh                  =   324, /* sixtyMinute deltaData quadrant2 electricitySecondaryMetered energy (kWh) */
   sixtydeltaq3kWh                  =   325, /* sixtyMinute deltaData quadrant3 electricitySecondaryMetered energy (kWh) */
   sixtydeltaq4kWh                  =   326, /* sixtyMinute deltaData quadrant4 electricitySecondaryMetered energy (kWh) */
   KYZcount                         =   327, /* bulkQuantity electricitySecondaryMetered energy (KYZ count) */
   maxPrevIndKYZdmd                 =   328, /* maximum previous indicating electricitySecondaryMetered demand (KYZ) */
   maxPresIndKYZdmd                 =   329, /* maximum present indicating electricitySecondaryMetered demand (KYZ) */
   fiveDeltaKYZ                     =   330, /* fiveMinute deltaData electricitySecondaryMetered energy (KYZ) */
   fifteenDeltaKYZ                  =   331, /* fifteenMinute deltaData electricitySecondaryMetered energy (KYZ) */
   thirtyDeltaKYZ                   =   332, /* thirtyMinute deltaData electricitySecondaryMetered energy (KYZ) */
   sixtyDeltaKYZ                    =   333, /* sixtyMinute deltaData electricitySecondaryMetered energy (KYZ) */
   maxPresFwdDmdTouAkW              =   334, /* maximum present indicating forward electricitySecondaryMetered demand touA (kW) */
   maxPresFwdDmdTouBkW              =   335, /* maximum present indicating forward electricitySecondaryMetered demand touB (kW) */
   maxPresFwdDmdTouCkW              =   336, /* maximum present indicating forward electricitySecondaryMetered demand touC (kW) */
   maxPresFwdDmdTouDkW              =   337, /* maximum present indicating forward electricitySecondaryMetered demand touD (kW) */
   maxPresRevDmdTouAkW              =   338, /* maximum present indicating reverse electricitySecondaryMetered demand touA (kW) */
   maxPresRevDmdTouBKW              =   339, /* maximum present indicating reverse electricitySecondaryMetered demand touB (kW) */
   maxPresRevDmdTouCkW              =   340, /* maximum present indicating reverse electricitySecondaryMetered demand touC (kW) */
   maxPresRevDmdTouDkW              =   341, /* maximum present indicating reverse electricitySecondaryMetered demand touD (kW) */
   maxPresNetDmdTouAkW              =   342, /* maximum present indicating net electricitySecondaryMetered demand touA (kW) */
   maxPresNetDmdTouBkW              =   343, /* maximum present indicating net electricitySecondaryMetered demand touB (kW) */
   maxPresNetDmdTouCkW              =   344, /* maximum present indicating net electricitySecondaryMetered demand touC (kW) */
   maxPresNetDmdTouDkW              =   345, /* maximum present indicating net electricitySecondaryMetered demand touD (kW) */
   maxPresTotDmdTouAkW              =   346, /* maximum present indicating total electricitySecondaryMetered demand touA (kW) */
   maxPresTotDmdTouBkW              =   347, /* maximum present indicating total electricitySecondaryMetered demand touB (kW) */
   maxPresTotDmdTouCkW              =   348, /* maximum present indicating total electricitySecondaryMetered demand touC (kW) */
   maxPresTotDmdTouDkW              =   349, /* maximum present indicating total electricitySecondaryMetered demand touD (kW) */
   sixtyDeltaTotkVArh               =   350, /* sixtyMinute deltaData total electricitySecondaryMetered energy (kVArh) */
   thirtyDeltaTotkVArh              =   351, /* thirtyMinute deltaData total electricitySecondaryMetered energy (kVArh)*/
   qhDeltaTotkVArh                  =   352, /* fifteenMinute deltaData total electricitySecondaryMetered energy(kVArh)*/
   fiveDeltaTotkVArh                =   353, /* fiveMinute deltaData total electricitySecondaryMetered energy (kVArh)  */
   totkW                            =   354, /* indicating total electricitySecondaryMetered power (kW) */
   netkW                            =   355, /* indicating net electricitySecondaryMetered power (kW) */
   totkVA                           =   356, /* indicating total electricitySecondaryMetered power (kVA) */
   oneDeltaFwdkWh                   =   357, /* oneMinute deltaData forward electricitySecondaryMetered energy (kWh)  */
   oneDeltaRevkWh                   =   358, /* oneMinute deltaData reverse electricitySecondaryMetered energy (kWh)  */
   oneDeltaNetkWh                   =   359, /* oneMinute deltaData net electricitySecondaryMetered energy (kWh)      */
   oneDeltaTotkWh                   =   360, /* oneMinute deltaData total electricitySecondaryMetered energy (kWh)    */
   oneDeltaFwdkVArh                 =   361, /* oneMinute deltaData forward electricitySecondaryMetered energy(kVArh) */
   oneDeltaRevkVArh                 =   362, /* oneMinute deltaData reverse electricitySecondaryMetered energy(kVArh) */
   oneDeltaNetkVArh                 =   363, /* oneMinute deltaData net electricitySecondaryMetered energy (kVArh)    */
   oneDeltaTotkVArh                 =   364, /* oneMinute deltaData total electricitySecondaryMetered energy (kVArh)  */
   avgIndTempC                      =   365, /* average indicating electricitySecondaryMetered temperature (°C)*/
   sixtyAvgIndTempC                 =   366, /* average sixtyMinute indicating electricitySecondaryMetered temperature (°C)*/
   thirtyAvgIndTempC                =   367, /* average thirtyMinute indicating electricitySecondaryMetered temperature (°C)*/
   qhAvgIndTempC                    =   368, /* average fifteenMinute indicating electricitySecondaryMetered temperature (°C)*/
   fiveAvgIndTempC                  =   369, /* average fiveMinute indicating electricitySecondaryMetered temperature (°C)*/
   oneAvgIndTempC                   =   370, /* average oneMinute indicating electricitySecondaryMetered temperature (°C)*/
   nominalSwitchPositionStatus      =   371, /* nominal electricitySecondaryMetered switchPosition (status) */
/*                                      372     Reserved for future use. */
   oneDeltaTotkVAh                  =   373, /* oneMinute deltaData total electricitySecondaryMetered energy (kVAh)   */
   sixtyAvgVRMSA                    =   374, /* average sixtyMinute indicating electricitySecondaryMetered voltage-rms phaseA(V)  */
   thirtyAvgVRMSA                   =   375, /* average thirtyMinute indicating electricitySecondaryMetered voltage-rms phaseA (V)*/
   oneAvgIndVRMSC                   =   376, /* average oneMinute indicating electricitySecondaryMetered voltage-rms phaseC (V)*/
   oneAvgIndVRMSB                   =   377, /* average oneMinute indicating electricitySecondaryMetered voltage-rms phaseB (V)  */
   oneAvgVRMSA                      =   378, /* average oneMinute indicating electricitySecondaryMetered voltage-rms phaseA (V)   */
   sixtyMaxVRMSA                    =   379, /* maximum sixtyMinute electricitySecondaryMetered voltage-rms phaseA (V)*/
   thirtyMaxVRMSA                   =   380, /* maximum thirtyMinute electricitySecondaryMetered voltage-rms phaseA(V)*/
   qhMaxVRMSA                       =   381, /* maximum fifteenMinute electricitySecondaryMetered voltage-rms phaseA(V)*/
   fiveMaxVRMSA                     =   382, /* maximum fiveMinute electricitySecondaryMetered voltage-rms phaseA (V) */
   oneMaxVRMSA                      =   383, /* maximum oneMinute electricitySecondaryMetered voltage-rms phaseA (V)  */
   sixtyMinVRMSA                    =   384, /* minimum sixtyMinute electricitySecondaryMetered voltage-rms phaseA (V)*/
   thirtyMinVRMSA                   =   385, /* minimum thirtyMinute electricitySecondaryMetered voltage-rms phaseA(V)*/
   qhMinVRMSA                       =   386, /* minimum fifteenMinute electricitySecondaryMetered voltage-rms phaseA(V)*/
   fiveMinVRMSA                     =   387, /* minimum fiveMinute electricitySecondaryMetered voltage-rms phaseA (V) */
   oneMinVRMSA                      =   388, /* minimum oneMinute electricitySecondaryMetered voltage-rms phaseA (V)  */
   sixtyMaxTemp                     =   389, /* maximum sixtyMinute indicating electricitySecondaryMetered temperature(°C)  */
   thirtyMaxTemp                    =   390, /* maximum thirtyMinute indicating electricitySecondaryMetered temperature(°C) */
   qhMaxTemp                        =   391, /* maximum fifteenMinute indicating electricitySecondaryMetered temperature(°C)*/
   fiveMaxTemp                      =   392, /* maximum fiveMinute indicating electricitySecondaryMetered temperature(°C)   */
   oneMaxTemp                       =   393, /* maximum oneMinute indicating electricitySecondaryMetered temperature(°C)    */
   sixtyMinTemp                     =   394, /* minimum sixtyMinute indicating electricitySecondaryMetered temperature(°C)  */
   thirtyMinTemp                    =   395, /* minimum thirtyMinute indicating electricitySecondaryMetered temperature(°C) */
   qhMinTemp                        =   396, /* minimum fifteenMinute indicating electricitySecondaryMetered temperature(°C)*/
   fiveMinTemp                      =   397, /* minimum fiveMinute indicating electricitySecondaryMetered temperature(°C)   */
   oneMinTemp                       =   398, /* minimum oneMinute indicating electricitySecondaryMetered temperature(°C)    */
   sixtyMaxCurPhA                   =   399, /* maximum sixtyMinute electricitySecondaryMetered current phaseA (A)  */
   thirtyMaxCurPhA                  =   400, /* maximum thirtyMinute electricitySecondaryMetered current phaseA (A) */
   qhMaxCurPhA                      =   401, /* maximum fifteenMinute electricitySecondaryMetered current phaseA (A)*/
   fiveMaxCurPhA                    =   402, /* maximum fiveMinute electricitySecondaryMetered current phaseA (A)   */
   oneMaxCurPhA                     =   403, /* maximum oneMinute electricitySecondaryMetered current phaseA (A)    */
   sixtyMinCurPhA                   =   404, /* minimum sixtyMinute electricitySecondaryMetered current phaseA (A)  */
   thirtyMinCurPhA                  =   405, /* minimum thirtyMinute electricitySecondaryMetered current phaseA (A) */
   qhMinCurPhA                      =   406, /* minimum fifteenMinute electricitySecondaryMetered current phaseA (A)*/
   fiveMinCurPhA                    =   407, /* minimum fiveMinute electricitySecondaryMetered current phaseA (A)   */
   oneMinCurPhA                     =   408, /* minimum oneMinute electricitySecondaryMetered current phaseA (A)    */
   sixtyAvgCurPhA                   =   409, /* average sixtyMinute indicating electricitySecondaryMetered current phaseA (A)  */
   thirtyAvgCurPhA                  =   410, /* average thirtyMinute indicating electricitySecondaryMetered current phaseA (A) */
   qhAvgCurPhA                      =   411, /* average fifteenMinute indicating electricitySecondaryMetered current phaseA (A)*/
   fiveAvgCurPhA                    =   412, /* average fiveMinute indicating electricitySecondaryMetered current phaseA (A)   */
   oneAvgCurPhA                     =   413, /* average oneMinute indicating electricitySecondaryMetered current phaseA (A)    */
   sixtyMaxCurPhC                   =   414, /* maximum sixtyMinute electricitySecondaryMetered current phaseC (A)  */
   thirtyMaxCurPhC                  =   415, /* maximum thirtyMinute electricitySecondaryMetered current phaseC (A) */
   qhMaxCurPhC                      =   416, /* maximum fifteenMinute electricitySecondaryMetered current phaseC (A)*/
   fiveMaxCurPhC                    =   417, /* maximum fiveMinute electricitySecondaryMetered current phaseC (A)   */
   oneMaxCurPhC                     =   418, /* maximum oneMinute electricitySecondaryMetered current phaseC (A)    */
   sixtyMinCurPhC                   =   419, /* minimum sixtyMinute electricitySecondaryMetered current phaseC (A)  */
   thirtyMinCurPhC                  =   420, /* minimum thirtyMinute electricitySecondaryMetered current phaseC (A) */
   qhMinCurPhC                      =   421, /* minimum fifteenMinute electricitySecondaryMetered current phaseC (A)*/
   fiveMinCurPhC                    =   422, /* minimum fiveMinute electricitySecondaryMetered current phaseC (A)   */
   oneMinCurPhC                     =   423, /* minimum oneMinute electricitySecondaryMetered current phaseC (A)    */
   sixtyAvgCurPhC                   =   424, /* average sixtyMinute electricitySecondaryMetered current phaseC (A)  */
   thirtyAvgCurPhC                  =   425, /* average thirtyMinute electricitySecondaryMetered current phaseC (A) */
   qhAvgCurPhC                      =   426, /* average fifteenMinute electricitySecondaryMetered current phaseC (A)*/
   fiveAvgCurPhC                    =   427, /* average fiveMinute electricitySecondaryMetered current phaseC (A)   */
   oneAvgCurPhC                     =   428, /* average oneMinute electricitySecondaryMetered current phaseC (A)    */
   totTouAkVAh                      =   429, /* summation total electricitySecondaryMetered energy touA (kVAh) */
   totTouBkVAh                      =   430, /* summation total electricitySecondaryMetered energy touB (kVAh) */
   totTouCkVAh                      =   431, /* summation total electricitySecondaryMetered energy touC (kVAh) */
   totTouDkVAh                      =   432, /* summation total electricitySecondaryMetered energy touD (kVAh) */
   avgThirtyVRmsC                   =   433, /* average thirtyMinute indicating electricitySecondaryMetered voltage-rms phaseC (V) */
   avgSixtyVRmsC                    =   434, /* average sixtyMinute indicating electricitySecondaryMetered voltage-rms phaseC(V) */
   minSixtyVRmsC                    =   435, /* minimum sixtyMinute electricitySecondaryMetered voltage-rms phaseC (V) */
   minThirtyVRmsC                   =   436, /* minimum thirtyMinute electricitySecondaryMetered voltage-rms phaseC (V) */
   minQhVRmsC                       =   437, /* minimum fifteenMinute electricitySecondaryMetered voltage-rms phaseC (V) */
   minFiveVRmsC                     =   438, /* minimum fiveMinute electricitySecondaryMetered voltage-rms phaseC (V) */
   minOneVRmsC                      =   439, /* minimum oneMinute electricitySecondaryMetered voltage-rms phaseC (V) */
   maxSixtyVRmsC                    =   440, /* maximum sixtyMinute electricitySecondaryMetered voltage-rms phaseC (V) */
   maxThirtyVRmsC                   =   441, /* maximum thirtyMinute electricitySecondaryMetered voltage-rms phaseC (V) */
   maxQhVRmsC                       =   442, /* maximum fifteenMinute electricitySecondaryMetered voltage-rms phaseC (V) */
   maxFiveVRmsC                     =   443, /* maximum fiveMinute electricitySecondaryMetered voltage-rms phaseC (V) */
   maxOneVRmsC                      =   444, /* maximum oneMinute electricitySecondaryMetered voltage-rms phaseC (V) */
   maxPresRevDmdTouAkVAr            =   445, /* maximum present indicating reverse electricitySecondaryMetered demand touA (kVAr) */
   maxPresTotDmdTouAkVAr            =   446, /* maximum present indicating total electricitySecondaryMetered demand touA (kVAr) */
   maxPresNetDmdTouAkVAr            =   447, /* maximum present indicating net electricitySecondaryMetered demand touA (kVAr) */
   maxPresTotDmdTouAkVA             =   448, /* maximum present indicating total electricitySecondaryMetered demand touA (kVA) */
   maxPresFwdDmdTouBkVAr            =   449, /* maximum present indicating forward electricitySecondaryMetered demand touB (kVAr) */
   maxPresRevDmdTouBkVAr            =   450, /* maximum present indicating reverse electricitySecondaryMetered demand touB (kVAr) */
   maxPresTotDmdTouBkVAr            =   451, /* maximum present indicating total electricitySecondaryMetered demand touB (kVAr) */
   maxPresNetDmdTouBkVAr            =   452, /* maximum present indicating net electricitySecondaryMetered demand touB (kVAr) */
   maxPresTotDmdTouBkVA             =   453, /* maximum present indicating total electricitySecondaryMetered demand touB (kVA) */
   maxPresFwdDmdTouCkVAr            =   454, /* maximum present indicating forward electricitySecondaryMetered demand touC (kVAr) */
   maxPresRevDmdTouCkVAr            =   455, /* maximum present indicating reverse electricitySecondaryMetered demand touC (kVAr) */
   maxPresTotDmdTouCkVAr            =   456, /* maximum present indicating total electricitySecondaryMetered demand touC (kVAr) */
   maxPresNetDmdTouCkVAr            =   457, /* maximum present indicating net electricitySecondaryMetered demand touC (kVAr) */
   maxPresTotDmdTouCkVA             =   458, /* maximum present indicating total electricitySecondaryMetered demand touC (kVA) */
   maxPresFwdDmdTouDkVAr            =   459, /* maximum present indicating forward electricitySecondaryMetered demand touD (kVAr) */
   maxPresRevDmdTouDkVAr            =   460, /* maximum present indicating reverse electricitySecondaryMetered demand touD (kVAr) */
   maxPresTotDmdTouDkVAr            =   461, /* maximum present indicating total electricitySecondaryMetered demand touD (kVAr) */
   maxPresNetDmdTouDkVAr            =   462, /* maximum present indicating net electricitySecondaryMetered demand touD (kVAr) */
   maxPresTotDmdTouDkVA             =   463, /* maximum present indicating total electricitySecondaryMetered demand touD (kVA) */
   maxPrevTotDmdTouAkVA             =   464, /* maximum previous indicating total electricitySecondaryMetered demand touA (kVA) */
   maxPrevTotDmdTouBkVA             =   465, /* maximum previous indicating total electricitySecondaryMetered demand touB (kVA) */
   maxPrevTotDmdTouCkVA             =   466, /* maximum previous indicating total electricitySecondaryMetered demand touC (kVA) */
   maxPrevTotDmdTouDkVA             =   467, /* maximum previous indicating total electricitySecondaryMetered demand touD (kVA) */
   presFwdCumlDmdkW                 =   468, /* present cumulative forward electricitySecondaryMetered demand (kW) */
   presRevCumlDmdkW                 =   469, /* present cumulative reverse electricitySecondaryMetered demand (kW) */
   presTotCumlDmdkW                 =   470, /* present cumulative total electricitySecondaryMetered demand (kW) */
   presNetCumlDmdkW                 =   471, /* present cumulative net electricitySecondaryMetered demand (kW) */
   presFwdCumlDmdkVAR               =   472, /* present cumulative forward electricitySecondaryMetered demand (kVAr) */
   presRevCumlDmdkVAR               =   473, /* present cumulative reverse electricitySecondaryMetered demand (kVAr) */
   presTotCumlDmdkVAR               =   474, /* present cumulative total electricitySecondaryMetered demand (kVAr) */
   presNetCumlDmdkVAR               =   475, /* present cumulative net electricitySecondaryMetered demand (kVAr) */
   presTotCumlDmdkVA                =   476, /* present cumulative total electricitySecondaryMetered demand (kVA) */
   presFwdCumlDmdTouAkW             =   477, /* present cumulative forward electricitySecondaryMetered demand touA (kW) */
   presRevCumlDmdTouAkW             =   478, /* present cumulative reverse electricitySecondaryMetered demand touA (kW) */
   presTotCumlDmdTouAkW             =   479, /* present cumulative total electricitySecondaryMetered demand touA (kW) */
   presNetCumlDmdTouAkW             =   480, /* present cumulative net electricitySecondaryMetered demand touA (kW) */
   presFwdCumlDmdTouAkVAR           =   481, /* present cumulative forward electricitySecondaryMetered demand touA (kVAr) */
   presRevCumlDmdTouAkVAR           =   482, /* present cumulative reverse electricitySecondaryMetered demand touA (kVAr) */
   presTotCumlDmdTouAkVAR           =   483, /* present cumulative total electricitySecondaryMetered demand touA (kVAr) */
   presNetCumlDmdTouAkVAR           =   484, /* present cumulative net electricitySecondaryMetered demand touA (kVAr) */
   presTotCumlDmdTouAkVA            =   485, /* present cumulative total electricitySecondaryMetered demand touA (kVA) */
   presFwdCumlDmdTouBkW             =   486, /* present cumulative forward electricitySecondaryMetered demand touB (kW) */
   presRevCumlDmdTouBkW             =   487, /* present cumulative reverse electricitySecondaryMetered demand touB (kW) */
   presTotCumlDmdTouBkW             =   488, /* present cumulative total electricitySecondaryMetered demand touB (kW) */
   presNetCumlDmdTouBkW             =   489, /* present cumulative net electricitySecondaryMetered demand touB (kW) */
   presFwdCumlDmdTouBkVAR           =   490, /* present cumulative forward electricitySecondaryMetered demand touB (kVAr) */
   presRevCumlDmdTouBkVAR           =   491, /* present cumulative reverse electricitySecondaryMetered demand touB (kVAr) */
   presTotCumlDmdTouBkVAR           =   492, /* present cumulative total electricitySecondaryMetered demand touB (kVAr) */
   presNetCumlDmdTouBkVAR           =   493, /* present cumulative net electricitySecondaryMetered demand touB (kVAr) */
   presTotCumlDmdTouBkVA            =   494, /* present cumulative total electricitySecondaryMetered demand touB (kVA) */
   presFwdCumlDmdTouCkW             =   495, /* present cumulative forward electricitySecondaryMetered demand touC (kW) */
   presRevCumlDmdTouCkW             =   496, /* present cumulative reverse electricitySecondaryMetered demand touC (kW) */
   presTotCumlDmdTouCkW             =   497, /* present cumulative total electricitySecondaryMetered demand touC (kW) */
   presNetCumlDmdTouCkW             =   498, /* present cumulative net electricitySecondaryMetered demand touC (kW) */
   presFwdCumlDmdTouCkVAR           =   499, /* present cumulative forward electricitySecondaryMetered demand touC (kVAr) */
   presRevCumlDmdTouCkVAR           =   500, /* present cumulative reverse electricitySecondaryMetered demand touC (kVAr) */
   presTotCumlDmdTouCkVAR           =   501, /* present cumulative total electricitySecondaryMetered demand touC (kVAr) */
   presNetCumlDmdTouCkVAR           =   502, /* present cumulative net electricitySecondaryMetered demand touC (kVAr) */
   presTotCumlDmdTouCkVA            =   503, /* present cumulative total electricitySecondaryMetered demand touC (kVA) */
   presFwdCumlDmdTouDkW             =   504, /* present cumulative forward electricitySecondaryMetered demand touD (kW) */
   presRevCumlDmdTouDkW             =   505, /* present cumulative reverse electricitySecondaryMetered demand touD (kW) */
   presTotCumlDmdTouDkW             =   506, /* present cumulative total electricitySecondaryMetered demand touD (kW) */
   presNetCumlDmdTouDkW             =   507, /* present cumulative net electricitySecondaryMetered demand touD (kW) */
   presFwdCumlDmdTouDkVAR           =   508, /* present cumulative forward electricitySecondaryMetered demand touD (kVAr) */
   presRevCumlDmdTouDkVAR           =   509, /* present cumulative reverse electricitySecondaryMetered demand touD (kVAr) */
   presTotCumlDmdTouDkVAR           =   510, /* present cumulative total electricitySecondaryMetered demand touD (kVAr) */
   presNetCumlDmdTouDkVAR           =   511, /* present cumulative net electricitySecondaryMetered demand touD (kVAr) */
   presTotCumlDmdTouDkVA            =   512, /* present cumulative total electricitySecondaryMetered demand touD (kVA) */
   prevFwdCumlDmdkW                 =   513, /* previous cumulative forward electricitySecondaryMetered demand (kW) */
   prevRevCumlDmdkW                 =   514, /* previous cumulative reverse electricitySecondaryMetered demand (kW) */
   prevTotCumlDmdkW                 =   515, /* previous cumulative total electricitySecondaryMetered demand (kW) */
   prevNetCumlDmdkW                 =   516, /* previous cumulative net electricitySecondaryMetered demand (kW) */
   prevFwdCumlDmdkVAR               =   517, /* previous cumulative forward electricitySecondaryMetered demand (kVAr) */
   prevRevCumlDmdkVAR               =   518, /* previous cumulative reverse electricitySecondaryMetered demand (kVAr) */
   prevTotCumlDmdkVAR               =   519, /* previous cumulative total electricitySecondaryMetered demand (kVAr) */
   prevNetCumlDmdkVAR               =   520, /* previous cumulative net electricitySecondaryMetered demand (kVAr) */
   prevTotCumlDmdkVA                =   521, /* previous cumulative total electricitySecondaryMetered demand (kVA) */
   prevFwdCumlDmdTouAkW             =   522, /* previous cumulative forward electricitySecondaryMetered demand touA (kW) */
   prevRevCumlDmdTouAkW             =   523, /* previous cumulative reverse electricitySecondaryMetered demand touA (kW) */
   prevTotCumlDmdTouAkW             =   524, /* previous cumulative total electricitySecondaryMetered demand touA (kW) */
   prevNetCumlDmdTouAkW             =   525, /* previous cumulative net electricitySecondaryMetered demand touA (kW) */
   prevFwdCumlDmdTouAkVAR           =   526, /* previous cumulative forward electricitySecondaryMetered demand touA (kVAr) */
   prevRevCumlDmdTouAkVAR           =   527, /* previous cumulative reverse electricitySecondaryMetered demand touA kVAr) */
   prevTotCumlDmdTouAkVAR           =   528, /* previous cumulative total electricitySecondaryMetered demand touA (kVAr) */
   prevNetCumlDmdTouAkVAR           =   529, /* previous cumulative net electricitySecondaryMetered demand touA (kVAr) */
   prevTotCumlDmdTouAkVA            =   530, /* previous cumulative total electricitySecondaryMetered demand touA (kVA) */
   prevFwdCumlDmdTouBkW             =   531, /* previous cumulative forward electricitySecondaryMetered demand touB (kW) */
   prevRevCumlDmdTouBkW             =   532, /* previous cumulative reverse electricitySecondaryMetered demand touB (kW) */
   prevTotCumlDmdTouBkW             =   533, /* previous cumulative total electricitySecondaryMetered demand touB (kW) */
   prevNetCumlDmdTouBkW             =   534, /* previous cumulative net electricitySecondaryMetered demand touB (kW) */
   prevFwdCumlDmdTouBkVAR           =   535, /* previous cumulative forward electricitySecondaryMetered demand touB (kVAr) */
   prevRevCumlDmdTouBkVAR           =   536, /* previous cumulative reverse electricitySecondaryMetered demand touB (kVAr) */
   prevTotCumlDmdTouBkVAR           =   537, /* previous cumulative total electricitySecondaryMetered demand touB (kVAr) */
   prevNetCumlDmdTouBkVAR           =   538, /* previous cumulative net electricitySecondaryMetered demand touB (kVAr) */
   prevTotCumlDmdTouBkVA            =   539, /* previous cumulative total electricitySecondaryMetered demand touB (kVA) */
   prevFwdCumlDmdTouCkW             =   540, /* previous cumulative forward electricitySecondaryMetered demand touC (kW) */
   prevRevCumlDmdTouCkW             =   541, /* previous cumulative reverse electricitySecondaryMetered demand touC (kW) */
   prevTotCumlDmdTouCkW             =   542, /* previous cumulative total electricitySecondaryMetered demand touC (kW) */
   prevNetCumlDmdTouCkW             =   543, /* previous cumulative net electricitySecondaryMetered demand touC (kW) */
   prevFwdCumlDmdTouCkVAR           =   544, /* previous cumulative forward electricitySecondaryMetered demand touC (kVAr) */
   prevRevCumlDmdTouCkVAR           =   545, /* previous cumulative reverse electricitySecondaryMetered demand touC (kVAr) */
   prevTotCumlDmdTouCkVAR           =   546, /* previous cumulative total electricitySecondaryMetered demand touC (kVAr) */
   prevNetCumlDmdTouCkVAR           =   547, /* previous cumulative net electricitySecondaryMetered demand touC (kVAr) */
   prevTotCumlDmdTouCkVA            =   548, /* previous cumulative total electricitySecondaryMetered demand touC (kVA) */
   prevFwdCumlDmdTouDkW             =   549, /* previous cumulative forward electricitySecondaryMetered demand touD (kW) */
   prevRevCumlDmdTouDkW             =   550, /* previous cumulative reverse electricitySecondaryMetered demand touD (kW) */
   prevTotCumlDmdTouDkW             =   551, /* previous cumulative total electricitySecondaryMetered demand touD (kW) */
   prevNetCumlDmdTouDkW             =   552, /* previous cumulative net electricitySecondaryMetered demand touD (kW) */
   prevFwdCumlDmdTouDkVAR           =   553, /* previous cumulative forward electricitySecondaryMetered demand touD (kVAr) */
   prevRevCumlDmdTouDkVAR           =   554, /* previous cumulative reverse electricitySecondaryMetered demand touD (kVAr) */
   prevTotCumlDmdTouDkVAR           =   555, /* previous cumulative total electricitySecondaryMetered demand touD (kVAr) */
   prevNetCumlDmdTouDkVAR           =   556, /* previous cumulative net electricitySecondaryMetered demand touD (kVAr) */
   prevTotCumlDmdTouDkVA            =   557, /* previous cumulative total electricitySecondaryMetered demand touD (kVA) */
   dailyMaxTotDmdTouAkW             =   558, /* daily maximum indicating total electricitySecondaryMetered demand touA (kW) */
   dailyMaxNetDmdTouAkW             =   559, /* daily maximum indicating net   electricitySecondaryMetered demand touA (kW) */
   dailyMaxTotDmdTouAkVAr           =   560, /* daily maximum indicating total electricitySecondaryMetered demand touA (kVAr) */
   dailyMaxNetDmdTouAkVAr           =   561, /* daily maximum indicating net   electricitySecondaryMetered demand touA (kVAr) */
   dailyMaxTotDmdTouAkVA            =   562, /* daily maximum indicating total electricitySecondaryMetered demand touA (kVA) */
   dailyMaxTotDmdTouBkW             =   563, /* daily maximum indicating total electricitySecondaryMetered demand touB (kW) */
   dailyMaxNetDmdTouBkW             =   564, /* daily maximum indicating net   electricitySecondaryMetered demand touB (kW) */
   dailyMaxTotDmdTouBkVAr           =   565, /* daily maximum indicating total electricitySecondaryMetered demand touB (kVAr) */
   dailyMaxNetDmdTouBkVAr           =   566, /* daily maximum indicating net   electricitySecondaryMetered demand touB (kVAr) */
   dailyMaxTotDmdTouBkVA            =   567, /* daily maximum indicating total electricitySecondaryMetered demand touB (kVA) */
   dailyMaxTotDmdTouCkW             =   568, /* daily maximum indicating total electricitySecondaryMetered demand touC (kW) */
   dailyMaxNetDmdTouCkW             =   569, /* daily maximum indicating net   electricitySecondaryMetered demand touC (kW) */
   dailyMaxTotDmdTouCkVAr           =   570, /* daily maximum indicating total electricitySecondaryMetered demand touC (kVAr) */
   dailyMaxNetDmdTouCkVAr           =   571, /* daily maximum indicating net   electricitySecondaryMetered demand touC (kVAr) */
   dailyMaxTotDmdTouCkVA            =   572, /* daily maximum indicating total electricitySecondaryMetered demand touC (kVA) */
   dailyMaxTotDmdTouDkW             =   573, /* daily maximum indicating total electricitySecondaryMetered demand touD (kW) */
   dailyMaxNetDmdTouDKW             =   574, /* daily maximum indicating net   electricitySecondaryMetered demand touD (kW) */
   dailyMaxTotDmdTouDkVAr           =   575, /* daily maximum indicating total electricitySecondaryMetered demand touD (kVAr) */
   dailyMaxNetDmdTouDkVAr           =   576, /* daily maximum indicating net   electricitySecondaryMetered demand touD (kVAr) */
   dailyMaxTotDmdTouDkVA            =   577, /* daily maximum indicating total electricitySecondaryMetered demand touD (kVA) */
   prevFwdkWh                       =   578, /* previous bulkQuantity forward electricitySecondaryMetered energy(kWh) */
   prevRevkWh                       =   579, /* previous bulkQuantity reverse electricitySecondaryMetered energy (kWh) */
   prevTotkWh                       =   580, /* previous bulkQuantity total   electricitySecondaryMetered energy (kWh) */
   prevNetkWh                       =   581, /* previous bulkQuantity net     electricitySecondaryMetered energy (kWh) */
   prevFwdkVARh                     =   582, /* previous bulkQuantity forward electricitySecondaryMetered energy (kVArh) */
   prevRevkVARh                     =   583, /* previous bulkQuantity reverse electricitySecondaryMetered energy (kVArh) */
   prevTotkVARh                     =   584, /* previous bulkQuantity total   electricitySecondaryMetered energy (kVArh) */
   prevNetkVARh                     =   585, /* previous bulkQuantity net     electricitySecondaryMetered energy (kVArh) */
   prevTotkVAh                      =   586, /* previous bulkQuantity total   electricitySecondaryMetered energy (kVAh) */
   prevFwdTouAkWh                   =   587, /* previous summation forward electricitySecondaryMetered energy touA (kWh) */
   prevRevTouAkWh                   =   588, /* previous summation reverse electricitySecondaryMetered energy touA (kWh) */
   prevTotTouAkWh                   =   589, /* previous summation total   electricitySecondaryMetered energy touA (kWh) */
   prevNetTouAkWh                   =   590, /* previous summation net     electricitySecondaryMetered energy touA (kWh) */
   prevFwdTouAkVArh                 =   591, /* previous summation forward electricitySecondaryMetered energy touA (kVArh) */
   prevRevTouAkVArh                 =   592, /* previous summation reverse electricitySecondaryMetered energy touA (kVArh) */
   prevTotTouAkVArh                 =   593, /* previous summation total   electricitySecondaryMetered energy touA (kVArh) */
   prevNetTouAkVArh                 =   594, /* previous summation net     electricitySecondaryMetered energy touA (kVArh) */
   prevTotTouAkVAh                  =   595, /* previous summation total   electricitySecondaryMetered energy touA (kVAh) */
   prevFwdTouBkWh                   =   596, /* previous summation forward electricitySecondaryMetered energy touB (kWh) */
   prevRevTouBkWh                   =   597, /* previous summation reverse electricitySecondaryMetered energy touB (kWh) */
   prevTotTouBkWh                   =   598, /* previous summation total   electricitySecondaryMetered energy touB (kWh) */
   prevNetTouBkWh                   =   599, /* previous summation net     electricitySecondaryMetered energy touB (kWh) */
   prevFwdTouBkVArh                 =   600, /* previous summation forward electricitySecondaryMetered energy touB(kVArh) */
   prevRevTouBkVArh                 =   601, /* previous summation reverse electricitySecondaryMetered energy touB(kVArh) */
   prevTotTouBkVArh                 =   602, /* previous summation total   electricitySecondaryMetered energy touB (kVArh) */
   prevNetTouBkVArh                 =   603, /* previous summation net     electricitySecondaryMetered energy touB (kVArh) */
   prevTotTouBkVAh                  =   604, /* previous summation total   electricitySecondaryMetered energy touB (kVAh) */
   prevFwdTouCkWh                   =   605, /* previous summation forward electricitySecondaryMetered energy touC (kWh) */
   prevRevTouCkWh                   =   606, /* previous summation reverse electricitySecondaryMetered energy touC (kWh) */
   prevTotTouCkWh                   =   607, /* previous summation total   electricitySecondaryMetered energy touC (kWh) */
   prevNetTouCkWh                   =   608, /* previous summation net     electricitySecondaryMetered energy touC (kWh) */
   prevFwdTouCkVArh                 =   609, /* previous summation forward electricitySecondaryMetered energy touC(kVArh) */
   prevRevTouCkVArh                 =   610, /* previous summation reverse electricitySecondaryMetered energy touC(kVArh) */
   prevTotTouCkVArh                 =   611, /* previous summation total   electricitySecondaryMetered energy touC (kVArh) */
   prevNetTouCkVArh                 =   612, /* previous summation net     electricitySecondaryMetered energy touC (kVArh) */
   prevTotTouCkVAh                  =   613, /* previous summation total   electricitySecondaryMetered energy touC (kVAh) */
   prevFwdTouDkWh                   =   614, /* previous summation forward electricitySecondaryMetered energy touD (kWh) */
   prevRevTouDkWh                   =   615, /* previous summation reverse electricitySecondaryMetered energy touD (kWh) */
   prevTotTouDkWh                   =   616, /* previous summation total   electricitySecondaryMetered energy touD (kWh) */
   prevNetTouDkWh                   =   617, /* previous summation net     electricitySecondaryMetered energy touD (kWh) */
   prevFwdTouDkVArh                 =   618, /* previous summation forward electricitySecondaryMetered energy touD(kVArh) */
   prevRevTouDkVArh                 =   619, /* previous summation reverse electricitySecondaryMetered energy touD(kVArh) */
   prevTotTouDkVArh                 =   620, /* previous summation total   electricitySecondaryMetered energy touD (kVArh) */
   prevNetTouDkVArh                 =   621, /* previous summation net     electricitySecondaryMetered energy touD (kVArh) */
   prevTotTouDkVAh                  =   622, /* previous summation total   electricitySecondaryMetered energy touD (kVAh) */
   dailyCumlFwdDmdkW                =   623, /* daily cumulative forward electricitySecondaryMetered demand (kW) */
   dailyCumlRevDmdkW                =   624, /* daily cumulative reverse electricitySecondaryMetered demand (kW) */
   dailyCumlTotDmdkW                =   625, /* daily cumulative total   electricitySecondaryMetered demand (kW) */
   dailyCumlNetDmdkW                =   626, /* daily cumulative net     electricitySecondaryMetered demand (kW) */
   dailyCumlFwdDmdkVar              =   627, /* daily cumulative forward electricitySecondaryMetered demand (kVAr) */
   dailyCumlRevDmdkVar              =   628, /* daily cumulative reverse electricitySecondaryMetered demand (kVAr) */
   dailyCumlTotDmdkVar              =   629, /* daily cumulative total   electricitySecondaryMetered demand (kVAr) */
   dailyCumlNetDmdkVar              =   630, /* daily cumulative net     electricitySecondaryMetered demand (kVAr) */
   dailyCumlTotDmdkVA               =   631, /* daily cumulative total   electricitySecondaryMetered demand (kVA) */
   dailyCumlFwdDmdTouAkW            =   632, /* daily cumulative forward electricitySecondaryMetered demand touA (kW) */
   dailyCumlRevDmdTouAkW            =   633, /* daily cumulative reverse electricitySecondaryMetered demand touA (kW) */
   dailyCumlTotDmdTouAkW            =   634, /* daily cumulative total   electricitySecondaryMetered demand touA (kW) */
   dailyCumlNetDmdTouAkW            =   635, /* daily cumulative net     electricitySecondaryMetered demand touA (kW) */
   dailyCumlFwdDmdTouAkVAr          =   636, /* daily cumulative forward electricitySecondaryMetered demand touA (kVAr) */
   dailyCumlRevDmdTouAkVAr          =   637, /* daily cumulative reverse electricitySecondaryMetered demand touA (kVAr) */
   dailyCumlTotDmdTouAkVAr          =   638, /* daily cumulative total   electricitySecondaryMetered demand touA (kVAr) */
   dailyCumlNetDmdTouAkVAr          =   639, /* daily cumulative net     electricitySecondaryMetered demand touA (kVAr) */
   dailyCumlTotDmdTouAkVA           =   640, /* daily cumulative total   electricitySecondaryMetered demand touA (kVA) */
   dailyCumlFwdDmdTouBkW            =   641, /* daily cumulative forward electricitySecondaryMetered demand touB (kW) */
   dailyCumlRevDmdTouBkW            =   642, /* daily cumulative reverse electricitySecondaryMetered demand touB (kW) */
   dailyCumlTotDmdTouBkW            =   643, /* daily cumulative total   electricitySecondaryMetered demand touB (kW) */
   dailyCumlNetDmdTouBkW            =   644, /* daily cumulative net     electricitySecondaryMetered demand touB (kW) */
   dailyCumlFwdDmdTouBkVAr          =   645, /* daily cumulative forward electricitySecondaryMetered demand touB (kVAr) */
   dailyCumlRevDmdTouBkVAr          =   646, /* daily cumulative reverse electricitySecondaryMetered demand touB (kVAr) */
   dailyCumlTotDmdTouBkVAr          =   647, /* daily cumulative total   electricitySecondaryMetered demand touB (kVAr) */
   dailyCumlNetDmdTouBkVAr          =   648, /* daily cumulative net     electricitySecondaryMetered demand touB (kVAr) */
   dailyCumlTotDmdTouBkVA           =   649, /* daily cumulative total   electricitySecondaryMetered demand touB (kVA) */
   dailyCumlFwdDmdTouCkW            =   650, /* daily cumulative forward electricitySecondaryMetered demand touC (kW) */
   dailyCumlRevDmdTouCkW            =   651, /* daily cumulative reverse electricitySecondaryMetered demand touC (kW) */
   dailyCumlTotDmdTouCkW            =   652, /* daily cumulative total   electricitySecondaryMetered demand touC (kW) */
   dailyCumlNetDmdTouCkW            =   653, /* daily cumulative net     electricitySecondaryMetered demand touC (kW) */
   dailyCumlFwdDmdTouCkVAr          =   654, /* daily cumulative forward electricitySecondaryMetered demand touC (kVAr) */
   dailyCumlRevDmdTouCkVAr          =   655, /* daily cumulative reverse electricitySecondaryMetered demand touC (kVAr) */
   dailyCumlTotDmdTouCkVAr          =   656, /* daily cumulative total   electricitySecondaryMetered demand touC (kVAr) */
   dailyCumlNetDmdTouCkVAr          =   657, /* daily cumulative net     electricitySecondaryMetered demand touC (kVAr) */
   dailyCumlTotDmdTouCkVA           =   658, /* daily cumulative total   electricitySecondaryMetered demand touC (kVA) */
   dailyCumlFwdDmdTouDkW            =   659, /* daily cumulative forward electricitySecondaryMetered demand touD (kW) */
   dailyCumlRevDmdTouDkW            =   660, /* daily cumulative reverse electricitySecondaryMetered demand touD (kW) */
   dailyCumlTotDmdTouDkW            =   661, /* daily cumulative total   electricitySecondaryMetered demand touD (kW) */
   dailyCumlNetDmdTouDkW            =   662, /* daily cumulative net     electricitySecondaryMetered demand touD (kW) */
   dailyCumlFwdDmdTouDkVAr          =   663, /* daily cumulative forward electricitySecondaryMetered demand touD (kVAr) */
   dailyCumlRevDmdTouDkVAr          =   664, /* daily cumulative reverse electricitySecondaryMetered demand touD (kVAr) */
   dailyCumlTotDmdTouDkVAr          =   665, /* daily cumulative total   electricitySecondaryMetered demand touD (kVAr) */
   dailyCumlNetDmdTouDkVAr          =   666, /* daily cumulative net     electricitySecondaryMetered demand touD (kVAr) */
   dailyCumlTotDmdTouDkVA           =   667, /* daily cumulative total   electricitySecondaryMetered demand touD (kVA) */

   avgIndVRMSA                      =   668, /* average indicating electricitySecondaryMetered voltage-rms phaseA(V) */
   maxCurPhA                        =   669, /* maximum electricitySecondaryMetered current phaseA (A) */
   minCurPhA                        =   670, /* minimum electricitySecondaryMetered current phaseA (A) */
   avgIndCurPhA                     =   671, /* average indicating electricitySecondaryMetered current phaseA (A) */
   maxCurPhC                        =   672, /* maximum electricitySecondaryMetered current phaseC (A) */
   minCurPhC                        =   673, /* minimum electricitySecondaryMetered current phaseC (A) */
   avgIndCurPhC                     =   674, /* average indicating electricitySecondaryMetered current phaseC (A) */
#if 0
// These are in Y84000 as of 4/30/2020
                                    =   675, /* bulkQuantity forward electricitySecondaryMetered funds (kWh)  */
                                    =   676, /* bulkQuantity reverse electricitySecondaryMetered funds (kWh)  */
                                    =   677, /* bulkQuantity net electricitySecondaryMetered funds (kWh)      */
                                    =   678, /* bulkQuantity total electricitySecondaryMetered funds (kWh)    */
                                    =   679, /* electricitySecondaryMetered funds (code)                      */
                                    =   680, /* electricitySecondaryMetered emergencyLimit (code)             */
                                    =   681, /* electricitySecondaryMetered demandLimit (code)                */
                                    =   682, /* electricitySecondaryMetered voltageAngle (code)               */
                                    =   683, /* electricitySecondaryMetered voltageAngle (deg)                */
#endif
   pdBeaconHandle                   =   684, /* Phase Detection beacon handle, aka communication broadcastAddress code */
   pdPhaseAngle                     =   685, /* Phase Detection angle          aka communication voltageAngle          */
   indVoltRmsPhaseAtoNFundPlus      =   686, /* indicating electricitySecondaryMetered voltage-rms phaseAtoN (V) */
   indVoltRmsPhaseBtoNFundPlus      =   687, /* indicating electricitySecondaryMetered voltage-rms phaseBtoN (V) */
   indVoltRmsPhaseCtoNFundPlus      =   688, /* indicating electricitySecondaryMetered voltage-rms phaseCtoN (V) */
   indVoltRmsPhaseAtoNFundOnly      =   689, /* indicating electricitySecondaryMetered voltage-rms fundamental phaseAtoN (V) */
   indVoltRmsPhaseBtoNFundOnly      =   690, /* indicating electricitySecondaryMetered voltage-rms fundamental phaseBtoN (V) */
   indVoltRmsPhaseCtoNFundOnly      =   691, /* indicating electricitySecondaryMetered voltage-rms fundamental phaseCtoN (V) */
   indVoltRmsPhaseAtoCFundPlus      =   692, /* indicating electricitySecondaryMetered voltage-rms phaseAtoC (V) */
   indVoltRmsPhaseBtoAFundPlus      =   693, /* indicating electricitySecondaryMetered voltage-rms phaseBtoAv (V) */
   indVoltRmsPhaseCtoBFundPlus      =   694, /* indicating electricitySecondaryMetered voltage-rms phaseCtoB (V) */
   indVoltRmsPhaseAtoCFundOnly      =   695, /* indicating electricitySecondaryMetered voltage-rms fundamental phaseAtoC (V) */
   indVoltRmsPhaseBtoAFundOnly      =   696, /* indicating electricitySecondaryMetered voltage-rms fundamental phaseBtoAv (V) */
   indVoltRmsPhaseCtoBFundOnly      =   697, /* indicating electricitySecondaryMetered voltage-rms fundamental phaseCtoB (V) */
   indCurrentPhaseAFundOnly         =   698, /* indicating electricitySecondaryMetered current fundamental phaseA (A) */
   indCurrentPhaseBFundOnly         =   699, /* indicating electricitySecondaryMetered current fundamental phaseB (A) */
   indCurrentPhaseCFundOnly         =   700, /* indicating electricitySecondaryMetered current fundamental phaseC (A) */
   indTotDistorationDemandPhaseA    =   701, /* indicating electricitySecondaryMetered totalDistortionDemand phaseA (V) */
   indTotDistorationDemandPhaseB    =   702, /* indicating electricitySecondaryMetered totalDistortionDemand phaseB (V) */
   indTotDistorationDemandPhaseC    =   703, /* indicating electricitySecondaryMetered totalDistortionDemand phaseC (V) */
   indDeviceBatteryVoltage          =   704, /* indicating device batteryVoltage (V) */

   energizationLoadSidePhaseA       =   706, /* electricitySecondaryMetered energizationLoadSide phaseA (V) */
   energizationLoadSidePhaseB       =   707, /* electricitySecondaryMetered energizationLoadSide phaseB (V) */
   energizationLoadSidePhaseC       =   708, /* electricitySecondaryMetered energizationLoadSide phaseC (V) */
   amiCommunSignalStrength          =   709, /* communication signalStrength (dBm) */
   maxAmiCommunSignalStrength       =   710, /* maximum communication signalStrength (dBm) */
   avgIndPowerFactor                =   721, /* average indicating electricitySecondaryMetered powerFactor (cos?) */
   indCurrentPhaseAngleA            =   732, /* indicating electricitySecondaryMetered currentAngle phaseA (deg) */
   indVoltagePhaseAngleA            =   733, /* indicating electricitySecondaryMetered voltageAngle phaseA (deg) */
   indCurrentPhaseAngleB            =   734, /* indicating electricitySecondaryMetered currentAngle phaseB (deg) */
   indVoltagePhaseAngleB            =   735, /* indicating electricitySecondaryMetered voltageAngle phaseB (deg) */
   indCurrentPhaseAngleC            =   736, /* indicating electricitySecondaryMetered currentAngle phaseC (deg) */
   indVoltagePhaseAngleC            =   737, /* indicating electricitySecondaryMetered voltageAngle phaseC (deg) */
   indDistortionPf                  =   738, /* indicating electricitySecondaryMetered distortionPowerFactor (cos?) */
   indMeterDiagN1                   =   739, /* indicating electricitySecondaryMetered diagnostic n1 (count) */
   indMeterDiagN2                   =   740, /* indicating electricitySecondaryMetered diagnostic n2 (count) */
   indMeterDiagN3                   =   741, /* indicating electricitySecondaryMetered diagnostic n3 (count) */
   indMeterDiagN4                   =   742, /* indicating electricitySecondaryMetered diagnostic n4 (count) */
   indMeterDiagN5PhaseA             =   743, /* indicating electricitySecondaryMetered diagnostic n5 phaseA (count) */
   indMeterDiagN5PhaseB             =   744, /* indicating electricitySecondaryMetered diagnostic n5 phaseB (count) */
   indMeterDiagN5PhaseC             =   745, /* indicating electricitySecondaryMetered diagnostic n5 phaseC (count) */
   indMeterDiagN5PhaseTotal         =   746, /* indicating electricitySecondaryMetered diagnostic n5 phasesABC (count) */
   indMeterDiagN6PhaseA             =   747, /* indicating electricitySecondaryMetered diagnostic n6 phaseA (count) */
   indMeterDiagN7PhaseA             =   748, /* indicating electricitySecondaryMetered diagnostic n7 phaseA (count) */
   indMeterDiagN8                   =   749, /* indicating electricitySecondaryMetered diagnostic n8 (count) */
   indMeterDiagN6PhaseB             =   750, /* indicating electricitySecondaryMetered diagnostic n6 phaseB (count) */
   indMeterDiagN7PhaseB             =   751, /* indicating electricitySecondaryMetered diagnostic n7 phaseB (count) */
   indMeterDiagN6PhaseC             =   752, /* indicating electricitySecondaryMetered diagnostic n6 phaseC (count) */
   indMeterDiagN7PhaseC             =   753, /* indicating electricitySecondaryMetered diagnostic n7 phaseC (count) */
   indMeterTamperCount              =   754, /* indicating electricitySecondaryMetered tamper (count) */
                                             /* 754-1023 reserved for future use. */
   addressModeSupportedCount        =  1024, /* The number of messages received by the EP which were decoded, validated,
                                                and contained an address mode that was supported. */
   ansiTableData                    =  1025, /* Data transported from a meter table */
   ansiTableLength                  =  1026, /* ANSI table length in bytes (if not specified the entire table is
                                                returned). */
   ansiTableNumber                  =  1027, /* ANSI table number per ANSI C12.19 or meter programming guide. */
   ansiTableOffset                  =  1028, /* ANSI table offset number of bytes from start of table at which to start
                                                accessing data. */
   appSecurityAuthMode              =  1029, /* Specifies allowed security authentication modes for application
                                                security. */
   appSecurityError                 =  1030, /* Total number of receive security errors. */
   AuthenticationErrorCount         =  1031, /* A message was received that did not authenticate */
   channel                          =  1032, /* The interval data channel number */
   code                             =  1033, /* code.26.11.83.79 A location code related to
                                                comDevice.Firmware.Program.Error */
   collectorDeviceType              =  1034, /* The type of data concentrator */
   comDeviceFirmwareVersion         =  1035, /* Firmware version with the format: <Major>.<Minor>.<Engineering build> */
   comDeviceHardwareVersion         =  1036, /* Hardware version with the format: <Rev Letter>.<Dash
                                                Number>.<BasePartNumber> */
   comDeviceID                      =  1037, /* A unique identifier for the EP hardware. */
   comDeviceMACAddress              =  1038, /* Factory assigned, long MAC address of the Field Equipment */
   comDeviceResourceSet             =  1039, /* Resource Set, used in combination with Type to define function set of an
                                                endpoint. */
   comDeviceType                    =  1040, /* The official Field Equipment name assigned by new product development or
                                                systems */
   commDeviceAssyVer                =  1041, /* Comm device assembly version */
   count                            =  1042, /* count.10.6.5.58 number of times collector.Installation.
                                                autoRegistration.succeeded has occurred. */
   VLFloopbackFailures              =  1043, /* count.26.1.122.85 The total number of times the VLF loopback test has
                                                failed. */
   NVRAMfailures                    =  1044, /* count.26.1.72.79 Number of times that communication with NVRAM
                                                officially failed. */
   meterCommFailures                =  1045, /* count.26.1.76.79 Comm. with End Device (meter) error count (raised after
                                                5 retries have failed in succession.) */
   HMCloopBackFailures              =  1046, /* count.26.1.76.85 Number of times Port 0 or Host Meter Loopback test has
                                                failed. */
   FWupdateErrors                   =  1047, /* count.26.11.83.79 number of times that comDevice.Firmware. Program.Error
                                                has occurred. */
   isc                              =  1048, /* count.26.12.1.38 Number of times that
                                                comDevice.Security.Access.notAuthorized has occurred. */
   PasswdChg                        =  1049, /* count.26.12.24.24 No. password changes from
                                                comDevice.Security.Password.Changed event. */
   emaCRC                           =  1050, /* count.26.18.31.43 CRC errors data memory and counted in
                                                comDevice.memory.data.Corrupted event. */
   SelfTestFailCount                =  1051, /* count.26.18.31.85 selftest errors memory from
                                                comDevice.memory.data.failed event. */
   NVCommFailCount                  =  1052, /* count.26.18.72.160 external NVRAM comm failures.
                                                comDevice.Memory.NVRAM.NotFound */
   stNvmRWFailCount                 =  1053, /* count.26.18.72.85 NVRAM selftest failures */
   stNvmChksumFailCount             =  1054, /* count.26.18.72.43 comDevice.Memory.NVRAM. corrupted count (counting of
                                                checksum errors). */
   stRomChksumFailCount             =  1055, /* count.26.18.83.43 comDevice.Memory.Program.Corrupted (PGM checksum
                                                error) has occurred. */
   PGMfailCount                     =  1056, /* count.26.18.83.85 comDevice.Memory.Program.failed (PGM self test
                                                failure) has occurred. */
   stRamRWFailCount                 =  1057, /* count.26.18.85.85 comDevice.Memory.RAM.Failed */
   ROMfailCount                     =  1058, /* count.26.18.92.85 comDevice.Memory.ROM.Failed */
   meterTimeSetCount                =  1059, /* count.26.21.117.139 comDevice.Metrology.TimeVariance.Exceeded
                                                EndDeviceEvent */
   meterDateSetCount                =  1060, /* count.26.21.34.24 comDevice.metrology.Date.Changed EndDeviceEvent has
                                                occurred. */
   PowerFailCount                   =  1061, /* count.26.26.0.85 powerdown counter corresponds to Reg #40 in TWACS */
   NOTC24count                      =  1062, /* count26.26.25.100 comDevice.Power.Phase.Inactive count */
   HiTempCount                      =  1063, /* count.26.35.261.93 high temperature exceeded count */
   RTCpwrFailCount                  =  1064, /* count.26.36.109.79 RTC data lost. This might occur after an extended
                                                powerdown. */
   DiscTimeCount                    =  1065, /* count.26.36.114.24 Count of discontinuous time settings. */
   UnsecureTimeSyncCount            =  1066, /* count.26.36.114.63 comDevice.clock.time.unsecure count. Untrusted
                                                date/time source. */
   DiscDateCount                    =  1067, /* count.26.36.34.24 Discontinuous date alarm comDevice.clock.date.changed
                                                has occurred. */
   stRTCFailCount                   =  1068, /* count.26.36.60.85 RTC failure counter */
   CDLcommFailCount                 =  1069, /* count.26.39.60.79 comDevice.Associated.device.IO counter. EMTR failed to
                                                communicate. */
   RegistrationFailureCount         =  1070, /* count.26.6.5.58 comDevice.Installation.autoRegistration.succeeded
                                                counter */
   CoverTamperCount                 =  1071, /* count.3.12.29.212 ElectricMeter.Security.Cover.Removed counter */
   ansiTableOID                     =  1072, /* ANSI C12.22 requires reference to the Table Object ID.  For convenience,
                                                this value is being passed as a parameter rather than a table value,
                                                however it is always found in Table 00, offset 3, length 4.
                                                When this value is supplied as part of a PUT or POST command, rather
                                                than being written to the meter, the value is compared to the value in
                                                the meter.  If they agree, the PUT is successful.  If they disagreee,
                                                the Endpoint is to respond with an error. */
   MeterCalibCount                  =  1073, /* count.3.21.18.24 electricMeter.Metrology.Calibration.Changed counter */
   excessPhaseCount                 =  1074, /* count.3.26.130.40 electricMeter.power.PhaseAngle.OutOfRange counter */
   SagStartCount                    =  1075, /* count.3.26.131.223 electricMeter.power.PhaseAVoltage.SagStarted counter */
   SwellStartCount                  =  1076, /* count.3.26.131.248 electricMeter.power.PhaseAngle.SwellStarted counter */
   excessNeutralCurrentCount        =  1077, /* count.3.26.137.93 electricMeter.power.neutralCurrent.MaxLimitReached
                                                count */
   phaseDeadCount                   =  1078, /* count.3.26.25.100 electricMeter.power.phase.inactive count */
   crossPhaseCount                  =  1079, /* count.3.26.25.45 electricMeter.power.phase.CrossPhaseDetected count */
   highTHDcount                     =  1080, /* count.3.26.28.69 electric.meter.power.powerQuality.HighDistortion count */
   RevkVarCount                     =  1081, /* count.3.26.294.219 ElectricMeter.Power.reactivePower.Reversed count */
   VImbalanceCount                  =  1082, /* count.3.26.38.98 electricMeter.power.Voltage.Imbalanced count */
   meterConfigCount                 =  1083, /* count.3.7.0.24 electricMeter.Configuration.changed count */
   meterParamCount                  =  1084, /* count.3.7.75.24 ElectricMeter.Configuration.Parameter.Changed count */
   meterProgramCount                =  1085, /* count.3.7.83.24 ElectricMeter.Configuration.Program.Changed count */
   msgCrcErrorCount                 =  1087, /* A message was received that authenticated but failed an internal CRC
                                                check */
   dailySelfReadTime                =  1088, /* The time of day, every day, at which the daily readings are to occur. */
   UTCdate                          =  1089, /* The year, month, and day in UTC format and in the Zulu timezone */
   dateTime                         =  1090, /* The current date and time in the EP. */
   dateTimeLostCount                =  1091, /* RTC has lost date and time at powerup due to an extended outage */
   demandResetLockoutPeriod         =  1092, /* period of time after receiving a demand reset command that the EP is to
                                                ignore any further attempts to perform a demand reset */
   dfwApplyGracePeriod              =  1093, /* time in minutes that EP has to process pending inbound messages prior to
                                                executing the DFW apply command. If there are no pending commands in the
                                                inbound queue, the Apply will execute immediately. If the grace period
                                                proves insufficent and unprocessed inbound messages remain in the queue,
                                                they will be flushed, and the Apply will occur at the end of the grace
                                                period. */
   dfwApplySchedule                 =  1094, /* In the context of a given streamID, this value indicates the desired
                                                start time for the DFW apply command to occur. A dfwApplyGracePeriod may
                                                cause the Apply to be somewhat delayed. */
   dfwCRCErrorPatch                 =  1095, /* Indicates that the stream is initialized, and all packets have been
                                                received, but the CRC in the patch does not match the computed CRC for
                                                the patch. */
   dfwCRCErrorPackets               =  1096, /* In the verfication of a given streamID, this value will be True when the
                                                DFW status indicates that stream is initialized, and all packets have
                                                been received, but there is a CRC error on the received packets. */
   dfwCRCErrorImage                 =  1097, /* Indicates that the stream is initialized, and all packets have been
                                                received, the patch commands were executed, but the computed CRC of the
                                                resultant image does not match the CRC for the image given in the patch.
                                                */
   dfwExpiry                        =  1098, /* The date and time at which a given download stream will expire. After
                                                which, any attempt to apply the download or accept additional packets
                                                will fail. */
   dfwInternalError                 =  1099, /* A temporary error has prevented the initialization from taking effect.
                                                The HE may retry the initialize command. */
   dfwInvalidPatchCmd               =  1100, /* In the verfication of a given streamID, this value will be True when the
                                                DFW status indicates that the stream is initialized, and all packets
                                                have been received, but an error was detected in that one or more patch
                                                commands were not valid. */
   dfwInvalidSchedule               =  1101, /* In the verfication or application of a given streamID, this value will
                                                be present in the response when the intended "apply" time for the patch
                                                is invalid. This can occur (for example) if the requested apply time is
                                                past the official expiration time for the stream. */
   dfwMaxMissing                    =  1102, /* PacketBlocks The number of missing packet blocks to return to the sender
   */
   dfwMissingPacketID               =  1103, /* The ID of a single missing download packet */
   dfwMissingPackets                =  1104, /* In the verfication or application of a given streamID, this value will
                                                be True when the DFW status indicates that the stream has been
                                                initialized but all of the required packets have not been received. */
   dfwMissingPacketQty              =  1105, /* The number of missing DFW packets for a given stream. The IDs of the
                                                missing packets are expressed using the dfwMissing PacketID parameter.
                                                */
   dfwMissingPacketRange            =  1106, /* A range of consecutive packet ids that are missing (e.g. 4-11) */
   dfwPacket                        =  1107, /* The DFW packet (content.) */
   dfwPacketCount                   =  1108, /* The number of packets to be expected in this download. */
   dfwPacketID                      =  1109, /* The DFW packet ID. */
   dfwPacketSize                    =  1110, /* The size of each DFW packet. */
   dfwPHYMACUpgrade                 =  1111, /* This DFW initialization parameter indicates that the patch affects the
                                                PHY/MAC protocol. */
   dfwRegisterDevice                =  1112, /* This DFW initialization parameter indicates that the EP re-register with
                                                the HeadEnd at the end of the DFW application process. */
   dfwStatus                        =  1113, /* This parameter returns a string which indicates the state of the DFW
                                                state machine. The following states (strings) are possible:
                                                ApplyingDownload InitializedWithError InitializedWaitForApply
                                                InitializedWaitForPackets NotInitialized */
   dfwStreamID                      =  1114, /* A DFW stream ID. Every DFW transaction occurs in the context of a
                                                stream.  */
   dfwXSDVersion                    =  1115, /* This request parameter indicates that the EP is to supply the XSD
                                                version with every inbound message commencing with the application of a
                                                new DFW version. This parameter may be set by a DFW intitization
                                                command, and cleared by the HE with a CHANGE /or/pm command. */
   dsBuEnabled                      =  1116, /* Defines if the daily shifted data is allowed to bubble in. The daily
                                                shift time is defined by dailySelfReadTime. */
   dstEnabled                       =  1117, /* When set to true Daylight Saving Time is enabled and the conversion from
                                                UTC (system) time to local time will add one hour if the current system
                                                time is between the dstStart and dstEnd dateTimes. */
   dstEnd                           =  1118, /* The UTC date and time in the Fall at which Daylight Saving Time will
                                                end.  The EP must consider the year that it is in, and add 365 days for
                                                each year relative to the stated date (plus one day for each leap year
                                                encountered.) Leap year occur approximately every 4 years: 2016, 2020,
                                                2024, 2028, 2032, 2036, etc. For use in the US, the time supplied should
                                                be 02:00:00. (Other countries may end summertime at midnight or some
                                                other time.) */
   dstStart                         =  1119, /* The UTC date and time in the Spring at which Daylight Saving Time will
                                                begin. DST will remain in effect all Summer and end in the Fall. The EP
                                                must consider the year that it is in, and add 365 days for each year
                                                relative to the stated date (plus one day for each leap year
                                                encountered.) Leap year occur approximately every 4 years: 2016, 2020,
                                                2024, 2028, 2032, 2036, etc. For use in the US, the time supplied should
                                                be 02:00:00. (Other countries may start summertime at midnight or some
                                                other time.) */
#if 0    //Deprecated, use ansiTableOID instead
   edConfiguration                  =  1120, /* A host meter configuration obtained from the ANSI tables. The
                                                configuration value is meant to represent a given meter program and
                                                operating configuration for the meter. */
#endif
   edFwVersion                      =  1121, /* Host meter firmware version as obtained from ANSI Table 01
                                                (FW_VERSION_NUMBER.FW_REVISION_NUMBER) */
   edHwVersion                      =  1122, /* Host meter hardware version as obtained from ANSI Table 01
                                                (HW_VERSION_NUMBER.HW_REVISION_NUMBER) */
   edManufacturer                   =  1123, /* Host meter manufacturer name as obtained from ANSI Tables */
   edMfgSerialNumber                =  1124, /* Host meter serial number as obtained from ANSI Tables. (Note: the
                                                MeterID element is computed by concatenating edManufacturer with
                                                edMfgSerialNumber.) */
   edModel                          =  1125, /* Host meter model name as obtained from ANSI Tables */
   edProgramID                      =  1126, /* Host meter program ID. In KV2 this is obtained from Mfg Table 72.  */
   edProgrammedDateTime             =  1127, /* The date and time that the host meter was last programmed. In the KV2
                                                this is from Mfg Table 78. */
   edProgrammerName                 =  1128, /* Host meter programmer name. In KV2 this is from Mfg. Table 78. */
   emtrDiagnosticErrorCount         =  1129, /* An EMTR associated with this EP has registered a diagnostic error */
   framTestFailureCount             =  1130, /* The number of times that FRAM memory has failed a self test. */
   highTempCount                    =  1131, /* See count.26.35.261.93 */
   highTempThreshold                =  1132, /* The temperature threshold (in degrees Centigrade) which if exceeded,
                                                will trip a high temperature alarm by the EP. */
   historicalRecovery               =  1133, /* Defines if historical recovery is supported as a resource. */
   ifInUnknownProtos                =  1134, /* For packet-oriented interfaces, the number of packets received via the
                                                interface which were discarded because of an unknown or unsupported
                                                protocol. For character-oriented or fixed-length interfaces that support
                                                protocol multiplexing the number of transmission units received via the
                                                interface which were discarded because of an unknown or unsupported
                                                protocol. For any interface that does not support protocol multiplexing,
                                                this counter will always be 0. Discontinuities in the value of this
                                                counter can occur at re-initialization of the management system. */
   inboundFrameCount                =  1135, /* The number of VLF inbound frames transmitted since the last powerup. */
   installationDateTime             =  1136, /* The official moment of installation of the device as determined by its
                                                first powerup in the field and connection to an upstream network which
                                                supplies timestamps. */
   invalidAddressMode               =  1137, /* Count The number of messages received by the EP which decoded,
                                                validated, and contained an address mode that was NOT supported. */
   iPifAddrEdgeRouter               =  1138, /* Edge router IPv6 address. */
   iPifAddrGP64                     =  1139, /* Global IPv6 address based on prefix and 64-bit EUI-64. */
   iPifAddrLL64                     =  1140, /* Link Local IPv6 address based on EUI-64. */
   iPifInBroadcastPkts              =  1141, /* The number of packets, delivered by this sub-layer to a higher
                                                (sub-)layer, which were addressed to a broadcast address at this
                                                sub-layer. Discontinuities in the value of this counter can occur at
                                                re-initialization of the management system. */
   iPifInDiscards                   =  1142, /* The number of inbound packets which were chosen to be discarded even
                                                though no errors had been detected to prevent their being delivered to a
                                                higher-layer protocol. One possible reason for discarding such a packet
                                                could be to free up buffer space. */
   iPifInErrors                     =  1143, /* For packet-oriented interfaces, the number of inbound packets that
                                                contained errors preventing them from being deliverable to a
                                                higher-layer protocol. For character- oriented or fixed-length
                                                interfaces, the number of inbound transmission units that contained
                                                errors preventing them from being deliverable to a higher-layer
                                                protocol. */
   iPifInMulticastPkts              =  1144, /* The number of packets, delivered by this sub-layer to a higher
                                                (sub-)layer, which were addressed to a multicast address at this
                                                sub-layer. For a MAC layer protocol, this includes both Group and
                                                Functional addresses. */
   iPifInOctets                     =  1145, /* The total number of octets received on the interface, including framing
                                                characters. */
   iPifInUcastPkts                  =  1146, /* The number of packets, delivered by this sub-layer to a higher
                                                (sub-)layer, which were not addressed to a multicast or broadcast
                                                address at this sub-layer. */
   iPifLastResetTime                =  1147, /* The value of sysUpTime at the time of the re-initialization of the IP
                                                management system. */
   iPifMTU                          =  1148, /* The size of the largest packet which can be sent/received on the
                                                interface, specified in octets. For interfaces that are used for
                                                transmitting network datagrams, this is the size of the largest network
                                                datagram that can be sent on the interface. */
   iPifOutBroadcastPkts             =  1149, /* The total number of packets that higher-level protocols requested be
                                                transmitted, and which were addressed to a broadcast address at this
                                                sub-layer, including those that were discarded or not sent. */
   iPifOutDiscards                  =  1150, /* The number of outbound packets which were chosen to be discarded even
                                                though no errors had been detected to prevent their being transmitted.
                                                One possible reason for discarding such a packet could be to free up
                                                buffer space. */
   iPifOutErrors                    =  1151, /* For packet-oriented interfaces, the number of outbound packets that
                                                could not be transmitted because of errors. For character-oriented or
                                                fixed-length interfaces, the number of outbound transmission units that
                                                could not be transmitted because of errors. */
   iPifOutMulticastPkts             =  1152, /* The total number of packets that higher-level protocols requested be
                                                transmitted, and which were addressed to a multicast address at this
                                                sub-layer, including those that were discarded or not sent. For a MAC
                                                layer protocol, this includes both Group and Functional addresses. */
   iPifOutOctets                    =  1153, /* The total number of octets transmitted out of the interface, including
                                                framing characters. */
   iPifOutUcastPkts                 =  1154, /* The total number of packets that higher-level protocols requested be
                                                transmitted, and which were not addressed to a multicast or broadcast
                                                address at this sub-layer, including those that were discarded or not
                                                sent. */
   iPifPromiscuousMode              =  1155, /* This object has a value of false if this interface only accepts
                                                packets/frames that are addressed to this station. This object has a
                                                value of true when the station accepts all packets/frames transmitted on
                                                the media. The value true is only legal on certain types of media. If
                                                legal, setting this object to a value of true may require the interface
                                                to be reset before becoming effective. */
   locationCode                     =  1156, /* A number that represents drill down detail as to the location of an
                                                error within the firmware. When an internal error is detected within the
                                                code or operation of the hardware, this code is generated by the
                                                firmware and supplied with the alarm to provide additional detail. (Some
                                                might describe this as a magic numbe which the firmware produces as part
                                                of an alarm to indicate the proximate cause of the error.) */
   lpBuEnabled                      =  1157, /* Defines if the LP data is allowed to bubble in. */
   lpBuSchedule                     =  1158, /* Defines the periodicity at which LP data is allowed to bubble in
                                                measured in minutes. If 0, the bubble up is disabled. If lpBuEnabled =
                                                false the bubble up is disabled. System 2014 supports only one bubble-in
                                                schedule 15 minutes. Future systems may identify multiple schedules (for
                                                example, 5, 15, 60, and/or 1440.) */
   macAckNotRx                      =  1159, /* Number of times an ACK was not received for a frame transmitted (when
                                                ACK was requested). */
   macAckWaitDuration               =  1160, /* The maximum amount of time in seconds to wait for an acknowledgement to
                                                arrive following a transmitted data or command frame. This value is
                                                dependent on the PHY settings. The calculated value is the time to
                                                commence transmitting the acknowledgement plus the expected length of
                                                the frame containing the acknowledgement. */
   macAssociatedPANs                =  1161, /* List of all PANs to which the end point is associated. A PAN object is
                                                simply any data structure that maintains the following data items as a
                                                complete set; PAN ID End point short address Coordinator EUI-64 Sync
                                                pulse phase sequence OB spreading codes */
   macChnAllocation                 =  1162, /* 3 payload parameters; band count (3 bits) indicates number of freq
                                                bands, wire count (2 bits) indicates number of wires, channel break list
                                                (variable - 192 bits max) list of channel numbers where each value is
                                                the last channel in the RAC list, the rest of the channels are
                                                scheduled. The list goes band 1 / wire 1 , band 1 / wire 2 , Etc. */
   macChnMatrix                     =  1163, /* RNG seed or table lookup that defines the permutation vector for a
                                                Hadamard matrix. */
   macCoord                         =  1164, /* ExtendedAddress PAN coordinator MAC layer 64-bit long address. */
   macCRCErrors                     =  1165, /* The number of CRC errors since reset. */
   macCSMAFail                      =  1166, /* Number of times CSMA failed. */
   macExpiredPktsTx                 =  1167, /* Number of expired packets. */
   macExtendedAddress               =  1168, /* MAC layer 64-bit long address. */
   macFramesDropRx                  =  1169, /* Number of dropped receive frames. */
   macFramesDropTx                  =  1170, /* Number of dropped transmit frames. */
   macFramesRx                      =  1171, /* Number of link layer frames received. */
   macFramesTx                      =  1172, /* Number of link layer frames transmitted. */
   macMaxFrameRetries               =  1173, /* The maximum number of retries allowed after a transmission failure. */
   macMediaAccessFail               =  1174, /* Number of times access to media failed. */
   macOctetsRx                      =  1175, /* Number of Bytes received. */
   macOctetsTx                      =  1176, /* Number of Bytes transmitted. */
   macPANId                         =  1177, /* For coordinators this is the PAN ID that was created. For end points,
                                                this is the PAN ID for which it is an active member. 0 implies no
                                                association, or no PAN created. */
   macPktsFragErrorRx               =  1178, /* Number of errors receiving fragments. */
   macPromiscuousMode               =  1179, /* Indication of whether the MAC sublayer is in a promiscuous (receive all)
                                                mode. A value of TRUE indicates that the MAC sublayer accepts all frames
                                                received from the PHY. */
   macResponseWaitTime              =  1180, /* The maximum time, in milliseconds, a device will wait for a response
                                                command frame to be available following a request command frame. */
   macRetryCount                    =  1181, /* Number of MAC transmit retries. */
   macRSSI                          =  1182, /* Received signal strength indication. */
   macScanTime                      =  1183, /* Period of time in seconds that the MAC will tell the PHY to scan for
                                                sync pulses. */
   macSecurityAuthMode              =  1184, /* Specifies the allowed security authentication modes for network
                                                security. Values are below:
                                                0 = None Required
                                                1 = Pre-Shared Key Mode
                                                2 = Certificate Mode
                                                3 = Pre-Shared Key or Certificate Mode */
   macSecurityCounterErrorRx        =  1185, /* Number of security errors on received packets with status of
                                                "COUNTER_ERROR". */
   macSecurityErrorRx               =  1186, /* Total number of security errors on received packets. */
   macSecurityKeyErrorRx            =  1187, /* Number of security errors on received packets with status of
                                                "UNAVAILABLE_KEY". */
   macSecurityLevel                 =  1188, /* Specifies the required network security level. Values are below.
                                                0 = No Network Security
                                                1 = 32-bit MIC without encryption
                                                2 = 64-bit MIC without encryption
                                                3 = 128-bit MIC without encryption
                                                4 = Encryption without MIC
                                                5 = 32-bit MIC with encryption
                                                6 = 64-bit MIC with encryption
                                                7 = 128-bit MIC with encryption */
   macSecurityLevelErrorRx          =  1189, /* Number of security errors on received packets with status of
                                               "IMPROPER_SECURITY_LEVEL". */
   macShortAddress                  =  1190, /* The address that the device uses to communicate in the PAN. If the
                                                device is the PAN coordinator, this value is 0. Otherwise, the short
                                                address is allocated by a coordinator during association. A value of
                                                8,191 indicates that the device does not have a short address. */
   macSyncDetectionWindow           =  1191, /* The width of the sync pulse detection window in seconds. */
   macSyncLossCount                 =  1192, /* The number of sync pulses that must be missed before the MAC losses
                                                synchronization with the PAN. */
   macSyncPhaseSequence             =  1193, /* The sync pulse phase sequence used for the current PAN. This value is
                                                null if not an active member or coordinator of a PAN. */
   macTransactionPersistenceTime    =  1194, /* The maximum amount of time (in seconds) that a transaction is stored in
                                                the MAC before the transaction is automatically purged. */
   macType                          =  1195, /* Specifies the type of link layer interface associated with the
                                                IP Interface. Values are below:
                                                0 = Unspecified
                                                1 = IEEE 802.3 (Ethernet)
                                                2 = IEEE 802.11 (WLAN)
                                                3 = IEEE 802.15 (PAN)
                                                4 = IEEE 1901 (PLC)
                                                5-255 = Reserved */
   memoryFailureCount               =  1196, /* The number of times a self-test has detected any memory problem in the
                                                EP */
   meterCommLockout                 =  1197, /* LockoutCount Total number of times the transponder is locked out from
                                                communicating with the meter. */
   epMaxTemperature                 =  1198, /* The highest temp ever experienced by EP (comm module radio) over its
                                                life time. */
   meterID                          =  1199, /* The meterID is a string that uniquely identifies the end device. ANSI
                                                meters will have Table01. Table01 will contain a field called
                                                MANUFACTURER and another called MFG_SERIAL_NUMBER. To form the meterID,
                                                the trailing spaces will be trimmed from MANUFACTURER, and the leading
                                                spaces and/or zeroes will be trimmed from MFG_SERIAL_NUMBER. These two
                                                fields will then be concatenated and expressed as a string. */
   meterSessionFailureCount         =  1200, /* This register is incremented every time the transponder fails to perform
                                                a valid communication session with the meter for any of the following
                                                reasons:
                                                1. The meter sends a password failure message Insufficient
                                                   Security Clearance (ISC)
                                                2. A table could not be read because of one of the following:
                                                   a. Service Not Supported (SNS)
                                                   b. Operation Not Possible (ONP)
                                                   c. Inappropriate Action Requested (IAR)
                                                3. Retries have been exhausted (session failed for whatever reason) */
   mfgSelfTestCount                 =  1201, /* This counter represents the total number of times that the manufacturing
                                                self test suite has been ran (either in series by the decrement counter
                                                or individually by command.) */
   mfgSelfTestDecrementCount        =  1202, /* When a non-zero number is written to this counter, it will wait 10
                                                minutes then begin to execute the manufacturing self test. After
                                                completion of one round of tests, the counter will be self-decremented,
                                                and if non-zero, the mfg. self test suite will be ran again. The loop
                                                repeats until the counter counts down to zero. */
   oneWayInboundCommandCount        =  1203, /* The number of messages received by the EP which decoded, and passed
                                                security checks, and which were intended to be one-way inbound bubble-up
                                                transmissions of some sort. (Interval data, daily reads, Alarms, */
   oneWayOutboundCommandCount       =  1204, /* The number of messages received by the EP which decoded, and passed
                                                security checks, and which were intended to be one-way outbound
                                                broadcasts of some sort. The counter is incremented regardless of if the
                                                parameters were valid or not. */
   outboundPacketsTotalCount        =  1205, /* The number of messages received by the EP which decoded, and passed
                                                security checks, whether they were addressed to this EP or not. */
   passwordPort0                    =  1206, /* The password the EP uses to start an ANSI C12.18 session with the ED.
                                                Note: Due to the need to support non-human-readable strings, and various
                                                endian types, the password is transported as a binary object. Note: The
                                                parameter is write only for security purposes, so that if one device
                                                were compromised, it does not at the same time compromise all others. */
   publishedReadingType             =  1207, /* The ReadingType ID code by which a channel of interval data will be
                                                called upon publication. */
   stSecurityFailCount              =  1208, /* The number of times a test of hardware security chip has resulted in a
                                                self test failure */
   realTimeAlarm                    =  1209, /* Defines if real time alarms are supported as a resource. */
   resetInstructionCount            =  1210, /* The total number of times the microcontroller has reset due to a reset
                                                instruction being received. */
   stSecurityFailTest               =  1211, /* Invokes the security chip self test */
   runTimeInitCount                 =  1212, /* The number of times the microcontroller has had to reset the constants
                                                in memory due to corruption. */
   samplePeriod                     =  1213, /* The period of time at which a source of interval data is sampled. For
                                                example, fifteenMinute interval data will be sampled every 00:15:00. */
   dmdIntrvlClkAllowance            =  1214, /* The amount of time in seconds that the clock maybe adjusted before the
                                                sub-interval has to be disallowed for use in the computation of maximum
                                                demand. */
   selfTestFailureCount             =  1215, /* The number of times an error has been detected while running the mfg or
                                                periodic self test suite. */
   sourceReadingType                =  1216, /* The ReadingType ID code which will serve as the source from which to
                                                build a channel of interval data. */
   spuriousResetCount               =  1217, /* Incremented when the unit does a reset or power-up without having
                                                previously done a valid power-down. */
   stackResetCount                  =  1218, /* The total number of times the microcontroller has reset due to stack
                                                overflow and/or stack underflow conditions. */
   temperature                      =  1219, /* The current temperature of the EP radio. This value is expressed in
                                                degrees Centigrade. It may be a negative value. */
   timeAfterCorrection              =  1220, /* The date and time on the clock after a major correction */
   timeBeforeCorrection             =  1221, /* The date and time on the clock prior to a major correction */
   timeBiasAlrmThrshldPct           =  1222, /* Time bias occurs when a reading is not taken at the planned moment. This
                                                may occur in the production of LP data when the sample cannot be
                                                obtained exactly at the top of the interval. Or in daily shifted data
                                                when an outage occurred during the target capture time. It can also
                                                occur in systems which maintain uncorrected or uncorrectable differences
                                                in time.  The time bias alarm threshold parameter identifies the amount
                                                of time difference (or bias) between systems which must be considered
                                                significant. The value is expressed as a percentage. Each channel of
                                                interval data will be examined with respect to the time error to
                                                determine if the difference between the desired capture time and the
                                                actual capture time is significant at the time of data capture. If so, a
                                                quality code is included for that channel of data in the interval which
                                                experienced the time error.[1] */
   timeChngAlrmThrshldPPM           =  1223, /* A time change could occur which is metrologically significant. The time
                                                change alarm threshold parameter identifies the amount of time change
                                                which must be considered significant. The value is expressed as a
                                                percentage. Each channel of interval data will be examined with respect
                                                to the time change to determine if the amount of time change, for that
                                                channel's interval size, is considered significant. If so, a quality code
                                                is included for that channel of data in the interval which experienced
                                                the timechange. */
   timeZoneOffset                   =  1224, /* The number of hours to offset local time relative to the Zulu timezone
                                                in the UTC timekeeping system. Note1: North America is in a negative
                                                offset.  Note2: Some regions (e.g. St. John's Canada) operate in a
                                                timezone which is not an integer offset (e.g. -3.5). */
   totalCommandsCount               =  1225, /* The number of messages received by the EP which decoded and passed
                                                security checks (regardless of the correctness of the arguments or the
                                                ability to perform the command.) */
   totalCommandsExecutedCount       =  1226, /* The number of messages received by the EP which decoded, passed security
                                                checks, and contained a complete and correct set of arguments which were
                                                executable (and supported) by this EP. */
   totalCommandsNotExecutedCount    =  1227, /* The number of messages received by the EP which decoded, and passed
                                                security checks, but could not be executed because the command was
                                                invalid, or one or more of the arguments were invalid (or not supported
                                                by this EP.) */
   twoWayCommandCount               =  1228, /* The number of messages received by the EP which decoded, and passed
                                                security checks, and which were intended to be on-request reads of some
                                                sort. The counter is incremented regardless of if the parameters were
                                                valid or not. */
   untrustedDateTimeReceivedCount   =  1229, /* A timesync identifying itself as untrusted was received by the EP and
                                                processed. */
   vlfBeaconCount                   =  1230, /* The number of VLF beacons that have been received since the last
                                                rollover, reset, or powerup. */
   watchdogResetCount               =  1231, /* The total number of times the microcontroller has experienced a
                                                watchdog-timer triggered reset. */
   xsdVersion                       =  1232, /* The Aclara schema version number being used by the firmware. */
   priority                         =  1233, /* The priority of the given alarm maybe adjusted using this name in a
                                                name/value pair for the /cf/am resource. range 0-255 */
   demandPresentConfiguration       =  1234, /* This parameter defines how the EndPoint or EndDevice is to formulate its
                                                demand calculation. This parameter is sent to the EndPoint. The EndPoint
                                                will know if the EndPoint, or if the EndDevice is responsible to compute
                                                demand. The EP is expected to make the necessary adjustments.
                                                Valid values are:
                                                0x01 =  5-minute fixed block (IEC 61968 Annex C attrib 2 enum 55)
                                                0x02 = 10-minute fixed block (IEC 61968 Annex C attrib 2 enum 54)
                                                0x03 = 15-minute fixed block (IEC 61968 Annex C attrib 2 enum 53)
                                                0x04 = 30-minute fixed block (IEC 61968 Annex C attrib 2 enum 51)
                                                0x05 = 60-minute fixed block (IEC 61968 Annex C attrib 2 enum 50)
                                                0x06 = 10-minute block with 2 x 5-minute rolling sub-intervals
                                                       (IEC 61968 Annex C attrib 2 enum 74)
                                                0x07 = 15-minute block with 3 x 5-minute rolling sub-intervals
                                                       (IEC 61968 Annex C attrib 2 enum 71)
                                                0x08 = 30-minute block with 6 x 5-minute rolling sub-intervals
                                                       (IEC 61968 Annex C attrib 2 enum 63)
                                                0x09 = 30-minute block with 3 x 10-minute rolling sub-intervals
                                                       (IEC 61968 Annex C attrib 2 enum 66)
                                                0x0A = 30-minute block with 2 x 15-minute rolling sub-intervals
                                                       (IEC 61968 Annex C attrib 2 enum 65)
                                                0x0B = 60-minute block with 12 x 5-minute rolling sub-intervals
                                                       (IEC 61968 Annex C attrib 2 enum 63)
                                                0x0C = 60-minute block with 6 x 10-minute rolling sub-intervals
                                                       (IEC 61968 Annex C attrib 2 enum 61)
                                                0x0D = 60-minute block with 4 x 15-minute rolling sub-intervals
                                                       (IEC 61968 Annex C attrib 2 enum 59)
                                                0x0E = 60-minute block with 3 x 20-minute rolling sub-intervals
                                                       (IEC 61968 Annex C attrib 2 enum 58)
                                                0x0F = 60-minute block with 2 x 30-minute rolling sub-intervals
                                                       (IEC 61968 Annex C attrib 2 enum 57
                                                All other values imply 15 minute fixed block */
   demandFutureConfiguration        =  1235, /* This parameter defines the formulation of demand in the next billing
                                                period (after the next billing demand reset.) If daily max demand is
                                                being collected, it will defer changeover of its formulation until the
                                                monthly billing demand reset occurs. When a PUT /dr demand reset action
                                                occurs, the demandFutureConfiguration parameter will be copied over to
                                                the demandCurrentConfiguration. Data collected unter the
                                                demandCurrentConfiguration must be normalized and reported properly as a
                                                result of the demand reset. New and ongoing demand calculations will
                                                occur under the demandFutureConfiguration and will require a suitable
                                                normalization based on the whole or fixed block interval size. Valid
                                                values are:
                                                0x01 =  5-minute fixed block (IEC 61968 Annex C attrib 2 enum 55)
                                                0x02 = 10-minute fixed block (IEC 61968 Annex C attrib 2 enum 54)
                                                0x03 = 15-minute fixed block (IEC 61968 Annex C attrib 2 enum 53)
                                                0x04 = 30-minute fixed block (IEC 61968 Annex C attrib 2 enum 51)
                                                0x05 = 60-minute fixed block (IEC 61968 Annex C attrib 2 enum 50)
                                                0x06 = 10-minute block with 2 x 5-minute rolling blocks
                                                       (IEC 61968 Annex C attrib 2 enum 74)
                                                0x07 = 15-minute block with 3 x 5-minute rolling blocks
                                                       (IEC 61968 Annex C attrib 2 enum 71)
                                                0x08 = 30-minute block with 6 x 5-minute rolling blocks
                                                       (IEC 61968 Annex C attrib 2 enum 63)
                                                0x09 = 30-minute block with 3 x 10-minute rolling blocks
                                                       (IEC 61968 Annex C attrib 2 enum 66)
                                                0x0A = 30-minute block with 2 x 15-minute rolling blocks
                                                       (IEC 61968 Annex C attrib 2 enum 65)
                                                0x0B = 60-minute block with 12 x 5-minute rolling blocks
                                                       (IEC 61968 Annex C attrib 2 enum 63)
                                                0x0C = 60-minute block with 6 x 10-minute rolling blocks
                                                       (IEC 61968 Annex C attrib 2 enum 61)
                                                0x0D = 60-minute block with 4 x 15-minute rolling blocks
                                                       (IEC 61968 Annex C attrib 2 enum 59)
                                                0x0E = 60-minute block with 3 x 20-minute rolling blocks
                                                       (IEC 61968 Annex C attrib 2 enum 58)
                                                0x0F = 60-minute block with 2 x 30-minute rolling blocks
                                                       (IEC 61968 Annex C attrib 2 enum 57)
                                                All other values imply 15 minute fixed block */
   dsBuDataRedundancy               =  1236, /* This parameter controls the number of historical bundles that are
                                                included in a load profile bubble-up transmission. This parameter
                                                controls the historical depth which is included in the formation of the
                                                application layer message. Daily shifted data is treated has similar
                                                parameter to control its redundancy. A MAC parameter is also available
                                                in the form of the traffic class to control how many times an inbound
                                                will be repeated. */
   dsBuMaxTimeDiversity             =  1237, /* dsBuMaxTimeDiversity defines a window of time in which a /bu/ds message
                                                may bubble in subsequent to the daily shift. This parameter defines the
                                                width of the window in minutes. The window starts immediately after the
                                                dailySelfReadTime and ends dsBuMaxTimeDiversity minutes later. EPs will
                                                select a fine-grained, uniformly-random delay within this window before
                                                transmitting data to the DCU. Additional delays may occur due the
                                                carrier sense capability of the radio. */
   dsBuTrafficClass                 =  1238, /* The MAC level traffic class awarded to move data generated by the /bu/lp
                                                resource. */
   endDeviceEventCode               =  1239, /* The supplied code refers to the EndDeviceEventCode itself. This is much
                                                like IEC 61968-9 Annex C Attribute #17, UOM=EndDeviceEvent=118. However
                                                in this design, it is meant to say that the value refers to the
                                                afformentioned EndDeviceEventType rather than to a ReadingType. */
   isDeltaData                      =  1240, /* A T/F Boolean value that indicates if samples are to be subtracted from
                                                each other in the formation of the interval data business value.
                                                Default =FALSE. */
   intervalDigitReduction           =  1241, /* The number of Least Significant digits to be trimmed from the
                                                decimalized business value of a deltaData item before storage as an
                                                integer value.  Default =0 Note: bulkQuantities and indicating values
                                                are to be stored at full resolution. Only deltaData is governed by this
                                                parameter. */
   lpBuDataRedundancy               =  1242, /* This parameter controls the number of historical bundles that are
                                                included in a load profile bubble-up transmission. This parameter
                                                controls the historical depth which is included in the formation of the
                                                application layer message. The allowed range is from 0 to 2. */
   lpBuMaxTimeDiversity             =  1243, /* The Load Profile Maximum Time Diversity parameter defines the window of
                                                time in which a /bu/lp message may bubble in after the completion of an
                                                lpBuSchedule period. This parameter defines the width of the window in
                                                minutes. The window starts immediately upon completion of an an
                                                lpBuSchedule period and ends lpBuMaxTimeDiversity minutes later. EPs
                                                will select a fine-grained, uniformly-random delay within this window
                                                before transmitting data to the DCU. Additional delays may occur due the
                                                carrier sense capability of the radio. */
   lpBuTrafficClass                 =  1244, /* This parameter sets the traffic class for a load profile bubble-up
                                                transmission. Traffic class attributes such as priority or reliability
                                                can be used to determine system behavior. Range: 0 - 63 */
   lpSamplePeriod                   =  1245, /* If non-zero, lpSamplePeriod represents a (global) period of time at
                                                which all sources of interval data will be sampled. If zero, a global
                                                period does not apply, and each channel operates according to its
                                                individual configuration (as set by /cf/idp). The sample period is
                                                expressed in minutes. Supported values include:
                                                0 = no global sample period
                                                5 = 5 minute interval data for all channels
                                                15 =15 minute interval data for all channels
                                                60 = hourly interval data for all channels */
   maxAuthenticationTimeout         =  1246, /* The maximum delay following DTLS SSL_FATAL_ERROR before DTLS
                                                Authentication is retried with the head end. The
                                                maxAuthenticationTimeout is expressed in seconds. Range: 0 - 65,535 */
   maxRegistrationTimeout           =  1247, /* The maximum delay following a failed attempt to publish Metadata before
                                                Registration is retried with the head end. The maxRegistrationTimeout is
                                                expressed in seconds. Range: 0 - 65,535 */
   minAuthenticationTimeout         =  1248, /* The delay following an initial DTLS SSL_FATAL_ERROR before DTLS
                                                Authentication is retried with the head end. The authentication timeout
                                                delay for subsequent consecutive retries is doubled for each attempt
                                                until the maxAuthenticationTimeout is reached. The
                                                minAuthenticationTimeout is expressed in seconds. Range: 0 - 65,535 */
   minRegistrationTimeout           =  1249, /* The delay following an initial failed attempt to publish Metadata before
                                                Registration is retried with the head end. The registration timeout
                                                delay for subsequent consecutive retries is doubled for each attempt
                                                until the maxRegistrationTimeout is reached. The minRegistrationTimeout
                                                is expressed in seconds. Range: 0 - 65,535 */
   newRegistrationRequired          =  1250, /* A Boolean value which identifies an EndPoint as newly installed. The
                                                factory default setting is TRUE. This value is updated by the EP to
                                                FALSE upon successful processing of a configuration message from the HE.
                                                If the EP receives a configuration message which is not perfectly
                                                understood and perfectly processed, it will not change the value to
                                                FALSE. When set to TRUE, loadprofile and dailyshift bubbleup data will
                                                not be published.  */
   outageDeclarationDelay           =  1251, /* This parameter determines the amount of time following the loss of power
                                                to an EndPoint before an outage condition can be declared. If power has
                                                not been restored after waiting the delay then an outage condition is
                                                declared. The outage declaration delay is expressed in seconds. Range: 0
                                                - 65,535 Note: Some utilities only want sustained interruptions to be
                                                reported. */
   timeFractionalSeconds            =  1252, /* The fractional portion of the current date and time (see the dateTime
                                                parameter). It is expressed in units of 1/232 seconds of clock time.
                                                (Ref: ANSI C12.19 U_SEC_FRACTION under HTIME_DATE_RCD) Note: 100 ms -
                                                429,496,730 10 ms - 42,949,673 1 ms - 4,294,967 1 µs - 4,295 */
   macPingCount                     =  1253, /* This counter may be cleared or read by the /or/pm resource and
                                                incremented by use of the /iq resource. Note: The
                                                oneWayOutboundCommandCount is incremented every time an asynchronous
                                                outbound command is received by the EP. The commEvalOneWayCommandCount
                                                is incremented only when asked to be incremented. */
   disconnectCapable                =  1254, /* This value is TRUE if RCD hardware is present (and mfg. softkey enabled)
                                                and FALSE otherwise. */
   timeZoneDstHash                  =  1255, /* This value is derived by the value contained in other parameters. It is
                                                used to test the setting of the DST related parameters to ensure their
                                                correctness. It is computed as the SHA-246 hash of the timeZoneOffset,
                                                dstEnabled, dstOffset, dstStartRule, and dstEndRule. (In the first
                                                release it is allowable to make the value be read-writeable,
                                                pre-computed, and stored in non-volatile memory. */
   dstOffset                        =  1256, /* This is the amount of time to be added to the standard time to create
                                                the shift to daylight saving time. This offset is expressed as a
                                                positive or negative number of seconds. In the US this is always +3600
                                                seconds (1hr), but some regions of the world may use other values. */
   opportunisticAlarmIndexID        =  1257, /* EndDeviceEvents will be logged in two different circular buffers.  High
                                                priority alarms are sent real-time and logged in the real-time-alarm
                                                log. Medium priority alarms are sent opportunistically and logged in the
                                                opportunistic alarm log.  The current index into the opportunistic alarm
                                                log (pointing to the newest entry) is defined by this
                                                opportunisticAlarmIndexID.  The opportunisticAlarmIndexID is presented
                                                to the HE in the ApplicationHeader as the requestID during bubble-up
                                                messages.  The opportunisticAlarmIndexID is incremented every time an
                                                EndDeviceEvent is added to the opportunistic alarm buffer. The HE may
                                                use the opportunisticAlarmIndexID values to request lost historical
                                                opportunistic alarms.  */
   dtlsNetworkRootCA                =  1258, /* An X.509v3 security certificate which is used to validate HeadEnd
                                                certificates.  The Network Root certificate is conveyed in a proprietary
                                                comprepessed format. The certificate is expected to be 100's of bytes in
                                                size and in the DER format. */
   dtlsSecurityRootCA               =  1259, /* An X.509v3 security certificate which is used to substantiate membership
                                                in the utility's security system.  EndPoints would allow DTLS
                                                connections under this certificate chain for security sensitive
                                                operations (like updating a networkRoot CA in the field.) The
                                                certificate is expected to be 100's of bytes in size and in the DER
                                                format. */
   ipHEContext                      =  1260, /* Specifies the IP Context for the HeadEnd */
   timeRequestMaxTimeout            =  1261, /* The maximum amount of random delay following a request for network time before the time request is retried.
                                                The timeRequestMaxTimeout is expressed in seconds. Range: 30 - 65,535 */
   macNetworkId                     =  1262, /* The identifier for the network on which the device is permitted to
                                                communicate. Range 0 - 15. */
   phyAvailableFrequencies          =  1263, /* List of licenced frequencies in Hz that can be used for transmission
                                                or reception. The list is presented as a comma separated string of
                                                frequency values. The list ends with the last frequency. The list need
                                                not be in ascending order.  Note: The EP has a frequency-agile radio. It
                                                can freely tune itself to any available frequency in the 450-470 MHz
                                                range.  Note: Frequency must be maintained within 2ppm. (This implies
                                                900 Hz for 450MHz.) Note: The transmitter cannot increment a frequency
                                                setting smaller than the step size (which is typically 2/3 ppm.) Note:
                                                The specified frequency represents the center (carrier) frequency around
                                                which modulation occurs.  */
   trReqRSSI                        =  1264, /* Request RSSI: the measurement of the RSSI of the received MAC Ping Request */
   trReqChannel                     =  1265, /* Request Channel: the outbound channel that the used for the MAC Ping Request */
   trRspRSSI                        =  1266, /* Response RSSI: the measurement of the RSSI of the received MAC Ping Response*/
   trRspChannel                     =  1267, /* Response Channel: the inbound channel that the used for the MAC Ping Response*/
   trHeTxOfReqTS                    =  1268, /* Time Stamp when HeadEnd transmitted the request to DCU */
   trObRxOfReqTS                    =  1269, /* Time Stamp - DCU reception of request from HE */
   trObTxOfReqTS                    =  1270, /* Time Stamp - DCU transmission of request on to Target */
   trTargetRxOfReqTS                =  1271, /* Time Stamp when the Target Received the Ping Request Command Request */
   trTargetTxOfRspTS                =  1272, /* Time Stamp when the Target Transmitted the Ping Response Command Response */
   trIbRxOfRspTS                    =  1273, /* Time Stamp when the DCU Received the Ping Response from the Target Device */
   trIbTxOfRspTS                    =  1274, /* Time Stamp when the DCU Send the the Ping Response from the Head End */
   trHeRxOfRspTS                    =  1275, /* The HeadEnd reception of response from DCU */
   trRequestorMacAddress            =  1276, /* MAC Address of the device the initiated the Ping Request Command */
   trTargetMacAddress               =  1277, /* The MAC Address of the target device (destination) */
   trHandle                         =  1278, /* The Handle used in the Ping Request Command (assigned by the requestor,
                                                (originator) */
   trPingCountReset                 =  1279, /* The Reset Flag used in the Ping Request Command */
   trRspAddrMode                    =  1280, /* The Address Mode used in the Ping Request Command */
   trReqPNLevel                     =  1281, /* The perceived noise level (as measured by the ping target) just after
                                                receipt of a ping request. To capture this noise sample, the receiver
                                                measures the "signal strength" when no signal is intentionally present.
                                                The noise level is expressed in dBm. This value complements the
                                                trReqRSSI measurement which is also expressed in dBm. */
   trRspPNLevel                     =  1282, /* The perceived noise level (as measured by the DCU) just after receipt of
                                                a ping response. To capture this noise sample, the receiver measures the
                                                "signal strength" on the same channel that the ping response arrived on.
                                                The noise level is expressed in dBm. This value complements the
                                                trRspRSSI measurement which is also expressed in dBm. */
   trReqPNState                     =  1283, /* Indicates if the trReqPNLevel measurement is believed to contain signal
                                                or not. TRUE if the measurement is believed to contain signal. FALSE
                                                implies noise only. */
   trRspPNState                     =  1284, /* Indicates if the trRspPNLevel measurement is believed to contain signal
                                                or not. TRUE if the measurement is believed to contain signal. FALSE
                                                implies noise only. */
/*                                    1285-1291 unused */
   configGet                        =  1292, /* Computes a CRC32 over particular files in memory (that are considered to
                                                be configuration files). The resulting CRC "signature" then printed out
                                                over the mfg serial port. */
   configUpdate                     =  1293, /* Updates the CRC32 snapshot being kept for each file. */
   configTest                       =  1294, /* Computes a CRC32 for each file and compares it to the snapshot on file.
                                                It will then print an "OK" or "Fail" message to the mfg serial port. */
   phyAfcDuration                   =  1295, /* The duration in milliseconds that the channel center frequency will be
                                                transmitted immediately following the PHY header.
                                                Allowable range: 1-255 */
   phyAfcPeriod                     =  1296, /* The periodicity, in hours, that the AFC function on a DCU will transmit
                                                the AFC signal.  If set to 0, the AFC function is effectively turned
                                                off.  Allowable range: 0-255 */
   phyCcaThreshold                  =  1297, /* The threshold against which the current RSSI is compared to determine if
                                                the channel is idle or busy. */
   phyFailedFrameDecodeCount        =  1298, /* The number of received PHY frames that failed FEC decoding (i.e. no
                                                valid code word was found).   */
   phyFailedHcsCount                =  1299, /* The number of received PHY frame headers that failed the header check
                                                sequence validation.   */
   phyFramesReceivedCount           =  1300, /* The number of data frames correctly received.   */
   phyFramesTransmittedCount        =  1301, /* The number of data frames transmitted.  This includes any frame that
                                                transmitted at a minimum the preamble and sync word */
   phyFailedHeaderDecodeCount       =  1302, /* The number of received PHY frame headers that failed FEC decoding (i.e.
                                                no valid code word was found).  This count value will roll over. */
   phyRcvrCount                     =  1303, /* The number of receivers defined within the PHY. */
   phyRssiJumpCount1                =  1304, /* The number of times each respective receiver has jumped to a stronger
                                                RSSI detection while in process of receiving a frame.  This count will
                                                roll over. The length of the list is equal to phyRcvCount and each entry
                                                corresponds to entries in phyRxChannel. */
   phyRxChannels                    =  1305, /* List of channels corresponding to the frequencies defined by
                                                phyRxFrequencies. The conversion between a channel number and frequency
                                                is given by: Ch = (f - 450,000,000)/6250   Channel 3201 is reserved to
                                                indicate an invalid channel. Valid channel numbers are 0 to 3200.*/
   phySyncDetectCount               =  1306, /* The number of times a valid sync word is detected.*/
   stNvmRWFailTest                  =  1307, /* A counter, that when written, initiates the NV Memory R/W walking ‘0’
                                                and walking ‘1’ failure test. The counter will decrement after each test
                                                cycle. Testing will stop when the counter reaches zero. With each
                                                failure the stNvmRWFailCount parameter is incremented.  Note: When a
                                                value of ‘1’ is written, the user wants to perform a quick test. This
                                                test will write a pattern to an unused section of NVRAM, then attempt to
                                                read the pattern back. If the read pattern does not match what was
                                                written, the test fails.  When a value larger than one is written, the
                                                user is interested in a burn-in test. Every bit of every byte should be
                                                tested. When a zero is written, the user would like to stop any test in
                                                progress. */
   stRamRWFailTest                  =  1308, /* A counter, that when written, initiates the RAM R/W walking ‘0’ and
                                                walking ‘1’ failure test. The counter will decrement after each test
                                                cycle. Testing will stop when the counter reaches zero. With each
                                                failure the stRamRWFailCount parameter is incremented.  Note: When a
                                                value of ‘1’ is written, the user wants to perform a quick test. The
                                                firmware should merely exercise every address line and data line into
                                                RAM to ensure it can accept a ‘1’ and ‘0’. When a larger value is
                                                written, the user is interested in a burn-in test. Every bit of every
                                                byte should be tested.  When a zero is written, the user would like to
                                                stop any test in progress.  */
   stNvmChksumFailTest              =  1309, /* A counter, that when written, initiates the NonVolatileMemory Checksum
                                                test. The counter will decrement after each test cycle. Testing will
                                                stop when the counter reaches zero. With each failure the
                                                stNvmChksumFailCount parameter is incremented. */
   stRomChksumFailTest              =  1310, /* A counter, that when written, initiates the Read Only Memory Checksum
                                                test. The counter will decrement after each test cycle. Testing will
                                                stop when the counter reaches zero. With each failure the
                                                stRomChksumFailCount parameter is incremented. */
   stP0LoopbackFailTest             =  1311, /* A counter, that when written, initiates the port zero meter
                                                communication loopback test. An external cable must be attached which
                                                provides the loop between the meter TX and RX UART lines. The counter
                                                will decrement after each test cycle. Testing will stop when the counter
                                                reaches zero. With each failure the stP0LoopbackFailCount parameter is
                                                incremented. */
   stRTCFailTest                    =  1312, /* A counter, that when written, initiates the Real Time Clock test. If the
                                                RTC has invalid time, the test will load a valid time, observe time
                                                increasing, and then restore the original invalid time. If the RTC
                                                originally has a reasonable time, the test will simply observe that time
                                                is incrementing.  The counter will decrement after each test cycle.
                                                Testing will stop when the counter reaches zero. With each failure the
                                                stRTCFailCount parameter is incremented.  */
   stTxCWTest                       =  1313, /* This parameter defines the transmitter test frequency.  Writing a
                                                non-zero value to this parameter causes an unmodulated carrier wave to
                                                be transmitted at the supplied frequency. The supplied frequency must be
                                                between 450 and 470 MHz for the transmission to occur. If a zero is
                                                supplied as the parameter value transmission ceases and the board
                                                returns to its normal operating mode. The test equipment can control the
                                                duration of the transmission by first writing a value between 450 and
                                                470 million to initiate the test, and a value of zero to stop the test.
                                                The equipment will self-limit the test duration to be less than 10
                                                seconds to avoid part damage. Tests occur at the maximum allowed
                                                transmit power level. Use of this command may affect the
                                                phyAvailableFrequencies and phyTxFrequencies lists.  */
   stRx2GFSKTest                    =  1314, /* This parameter defines the 2GFSK BER test frequency.  Writing a non-zero
                                                value to this ToolEP parameter will place the board in a special mode in
                                                which all of the Si4460 recievers on the board are (re)configured to
                                                accept 2GFSK format, tuned to the commanded frequency, and send raw
                                                received data out the Si4460 GPIO pins. An external system must supply
                                                the 2GFSK modulated radio signal into the antenna port. The external
                                                system can then confirm reception of the data through the radio at the
                                                clock and data GPIO pins. The value written to this parameter represents
                                                the frequency the radio will be tuned to. The system will remain in the
                                                2GFSK Test mode until a zero is written to this parameter, or until the
                                                board is reset. Use of this command may affect the
                                                phyAvailableFrequencies and phyRxFrequencies list. */
   stRx4GFSK                        =  1315, /* This parameter defines the 4GFSKtest frequency.  An external system must
                                                supply the 4GFSK modulated radio signal into the antenna port. If a sync
                                                word is detected, the word "Sync" followed by the radio number is
                                                printed out the serial port. If a message is captured, the word "Raw"
                                                followed by the radio number and data is printed out the serial port;
                                                followed by the word "Decoded" followed by the radio number, followed by
                                                the decoded data. The system will remain in this mode until a zero is
                                                written to this parameter, or until the board is reset.  Use of this
                                                command may affect the phyAvailableFrequencies and phyRxFrequencies
                                                list.  */
   phyTxChannels                    =  1316, /* List of channels corresponding to the frequencies defined by
                                                phyTxFrequencies. The conversion between a channel number and frequency
                                                is given by: Ch = (f - 450,000,000) / 6250 Channel 3201 is reserved to
                                                indicate an invalid channel. Valid channel numbers are 0 to 3200.*/
   dfwPatchSize                     =  1317, /* This is the total size in bytes of the download patch */
   stTxBlurtTest                    =  1318, /* Writing to this parameter serves as a command to test inbound
                                                transmission. The command does not accept parameters.  The module will
                                                construct a bubble-up message which has a POST method, and /bu/en
                                                resource, in a FullMeterReads structure, carrying a canned message,
                                                addressed to a device with a MAC address of 11.22.33.44.55.  The canned
                                                message repeats {0x01, 0x23, 0x45, 0x67, 0x89} for 104 bytes.
                                                Transmission will occur on a frequency which is randomly selected from
                                                the Tx channel list.  The test may be used to capture RF output for
                                                analysis, or to force a command to bubble into a live STAR network. */
   engData1                         =  1319, /* engineering data (PHY/MAC/IP statistics) */
   phyTxFrequencies                 =  1320, /* List of frequencies which have been designated as legitimate transmit
                                                frequencies. The list is not positionally sensitive in relation to the
                                                hardware available. The list is formed as a string of space separated
                                                integers. The list may contain up to phyNumChannels of frequencies.  See
                                                also the equivalent scheme under phyTxChannels.*/
   phyRxFrequencies                 =  1321, /* List of frequencies which have been designated as legitimate receive
                                                frequencies. The list is positionally sensitive in relation to the
                                                hardware, so that the first frequency (in Hz) corresponds to the Rx
                                                frequency of the first receiver chip. Skipping a position (either by
                                                means of an empty comma separated value, or by use of 0 as an invalid
                                                frequency) causes the corresponding chip to not participate. The list is
                                                formed as a string of comma-separated integers. The list may contain up
                                                to phyRcvrCount of frequencies.  See also the equivalent scheme under
                                                phyRxChannels. */
   phyAvailableChannels             =  1322, /* List of channels corresponding to the frequencies defined by
                                                phyAvailableFrequencies. The conversion between a channel number and
                                                frequency is given by: Ch=(f-450,000,000)/6250 Valid channel numbers
                                                range from 0 to 3200. */
   phyNumChannels                   =  1323, /* The actual number of channels programmed into and usable by the PHY.
                                                (This parameter should agree with the length of the "list" found in the
                                                phyAvailableChannels parameter, which in turn should agree with the
                                                length of the list in the phyAvailableFrequencies parameter. */
   comDeviceSlot                    =  1324, /* The instance, slot, or port number that a given communication device is
                                                connected to. */
   timePrecision                    =  1325, /* The +/- resolution of the system clock on the device in units of log 2
                                                (seconds). Range: -127 to 3 */
   timeSetPeriod                    =  1326, /* The period, in seconds, for which the time set MAC command broadcast
                                                schedule is computed. A value of zero will prevent the DCU from
                                                automatically transmitting the time set MAC command. Range: 0 - 86,400 */
   timeSetStart                     =  1327, /* The start time in seconds relative to the DCU system time of midnight
                                                UTC for starting the DCU time set broadcast schedule. For example,
                                                starting the schedule at 1am (01:00:00) would yield a value of 3,600.
                                                Range: 0 - 86,399 */
   stRxTest                         =  1328, /* This parameter defines the receiver test frequency in Hz.  The test
                                                frequency must be in the range of 450,000,000 to 470,000,000 Hz.
                                                Writing a valid value to this parameter will place the device in a
                                                special mode and cause it to listen only at the prescribed frequency and
                                                not attempt any RF transmissions. Any RF messages received will be
                                                processed and also converted to ASCII HEX (a.k.a. hex binary) and sent
                                                out the mfg serial port. This test may be cancelled (returning the
                                                product to its normal mode) by supplying zero as a parameter, or by
                                                powering down the device.  */
   shipMode                         =  1329, /* When set to TRUE (shipMode > 0), this parameter temporarily suppresses
                                                the ability of the device to transmit RF messages; and declare outage,
                                                restoration, and power quality events.
                                                The device will not:
                                                - Transmit RF
                                                - Receive and process RF commands
                                                - Declare outage, restoration, or power quality events
                                                The device will (provided mains power is present):
                                                -  Communicate over the mfg serial port and process ToolEP commands
                                                - Attempt to communicate with the host meter
                                                If (shipMode > 0) at powerup, the device will:
                                                - Set the newRegistrationRequired flag
                                                - Clear shipMode
                                                - Clear the powerQuality counter ReadingType #128
                                                (new for f/w ver. 1.40)
                                                If (shipMode == 1) at powerup, the device will additionally:
                                                - Clear installationDateTime
                                                - Clear network stack statistics
                                                - Clear all events and metering values stored in the device (previous
                                                  demand values and historical data)
                                                - Invalidate System time*/
   dfwPatchFormatVersion            =  1330, /* The Patch Format in the DFW Patch header */
   edInfo                           =  1331, /* EndDevice information written at time of integration which likely
                                                includes the meter form, class, and voltage. The information is stored
                                                as a string of ASCII characters which are comma separated (e.g.
                                                2S,CL200,240V). */
   dtlsNetworkHESubject             =  1332, /* The subject field from the utility head end server's certificate. The
                                                value is expected to be in DER format. */
   dtlsNetworkMSSubject             =  1333, /* The subject field from the utility meter shop server's certificate. The
                                                value is expected to be in DER format. */
   timeSigOffset                    =  1334, /* A metrologically significant time offset. Stored in milliseconds. Valid
                                                Range: 1 - 60,000. */
   timeAcceptanceDelay              =  1335, /* The period of time following the acceptance of a MAC Time Set Indication
                                                before further MAC Time Set Indications may be accepted. Stored in
                                                seconds. Valid Range: 0 - 3600. */
   timeState                        =  1336, /* The state of the system clock:
                                                0 = INVALID
                                                1 = VALID_NO_SYNC
                                                2 = VALID_SYNC */
   timeLastUpdated                  =  1337, /* The time of the last time update. */
   dsBuReadingTypes                 =  1338, /* List daily shift bubble up readingType HEEP enumerations. The list is
                                                formed as a string of comma separated integers. Allowable readingType
                                                values and quantity of readingType values is product specific. Valid
                                                Range: 0 - 6 comma separated integers i-210: 12,19,20,15,128,129
                                                                                       kV2c: 12,15,13,16,17,128 */
   lpBuChannel1                     =  1339, /* List of the following load profile bubble up attributes for channel 1:
                                                -  sourceReadingType
                                                - Source readingType HEEP enumeration
                                                - samplePeriod - The time in minutes between sampling of the source.
                                                - isDeltaData - A Boolean value that indicates if samples are to be
                                                  subtracted from each other in the formation of the business value.
                                                - trimDigits - The number of digits to lose in order to convert the
                                                  computed integer business value to the stored integer business value.
                                                - StorageBytes - The number of bytes of storage allocated to this
                                                  channel. The list is formed as a string of space separated integers.
                                                  Allowable values are product specific.
                                                   i-210: 12,15,1,0,3
                                                   kV2c: 12,5,1,5,3 */
   lpBuChannel2                     =  1340, /* List of the following load profile bubble up attributes for channel 2:
                                                -  sourceReadingType - Source readingType HEEP enumeration
                                                -  samplePeriod - The time in minutes between sampling of the source.
                                                -  isDeltaData - A Boolean value that indicates if samples are to be
                                                   subtracted from each
                                                   other in the formation of the business value.
                                                -  trimDigits - The number of digits to lose in order to convert the
                                                   computed integer
                                                   business value to the stored integer business value.
                                                -  StorageBytes
                                                - The number of bytes of storage allocated to this channel. The list is
                                                  formed as a string of space separated integers. Allowable values are
                                                  product specific.
                                                   i-210: 15,15,1,0,3
                                                   kV2c: 15,5,1,5,3 */
   lpBuChannel3                     =  1341, /* List of the following load profile bubble up attributes for channel 3:
                                                - sourceReadingType - Source readingType HEEP enumeration
                                                - samplePeriod - The time in minutes between sampling of the source.
                                                - isDeltaData - A Boolean value that indicates
                                                  if samples are to be subtracted from each other in the formation of
                                                  the business value.
                                                - trimDigits - The number of digits to lose in order to convert the
                                                  computed integer business value to the stored integer business value.
                                                - StorageBytes
                                                - The number of bytes of storage allocated to this channel. The list is
                                                  formed as a string of space separated integers. Allowable values are
                                                  product specific.
                                                   i-210: 33,15,0,0,3
                                                   kV2c: 13,5,1,5,3 */
   lpBuChannel4                     =  1342, /* List of the following load profile bubble up attributes for channel 4:
                                                - sourceReadingType - Source readingType HEEP enumeration
                                                - samplePeriod - The time in minutes between sampling of the source.
                                                - isDeltaData - A Boolean value that indicates if samples are to be
                                                  subtracted from each other in the formation of the business value.
                                                - trimDigits - The number of digits to lose in order to convert the
                                                  computed integer business value to the stored integer business value.
                                                - StorageBytes - The number of bytes of storage allocated to this
                                                  channel. The list is formed as a string of space separated integers.
                                                  Allowable values are product specific.
                                                   i-210: 32,15,0,0,3
                                                   kV2c: 34,5,0,0,3 */
   lpBuChannel5                     =  1343, /* List of the following load profile bubble up attributes for channel 5:
                                                - sourceReadingType - Source readingType HEEP enumeration
                                                - samplePeriod - The time in minutes between sampling of the source.
                                                - isDeltaData - A Boolean value that indicates if samples are to be
                                                  subtracted from each other in the formation of the business value.
                                                - trimDigits - The number of digits to lose in order to convert the
                                                  computed integer business value to the stored integer business value.
                                                - StorageBytes - The number of bytes of storage allocated to this
                                                  channel. The list is formed as a string of space separated integers.
                                                  Allowable values are product specific.
                                                   i-210: --
                                                   kV2c: 36,5,0,0,3 */
   lpBuChannel6                     =  1344, /* List of the following load profile bubble up attributes for channel 6:
                                                - sourceReadingType - Source readingType HEEP enumeration
                                                - samplePeriod - The time in minutes between sampling of the source.
                                                - isDeltaData - A Boolean value that indicates if samples are to be
                                                  subtracted from each other in the formation of the business value.
                                                - trimDigits - The number of digits to lose in order to convert the
                                                  computed integer business value to the stored integer business value.
                                                - StorageBytes - The number of bytes of storage allocated to this
                                                  channel. The list is formed as a string of space separated integers.
                                                  Allowable values are product specific.
                                                   i-210: --
                                                   kV2c: 35,5,0,0,3 */
   applyFrequencies                 =  1345, /* A parameter, that when written, commands the MAC stack to be selectively
                                                reset and frequencies from the phyavailableFrequencies list to be
                                                applied. Valid range 0 to 0. */
   quietMode                        =  1346, /* A parameter, that when written to TRUE causes the EndPoint to stop all
                                                RF transmissions and also stop all communication with the host meter.
                                                Communication over the mfg serial port is not prohibited. This parameter
                                                clears itself upon powerup. Note: Compare this to the shipMode in which
                                                only RF communication is prohibited. */
   dfwApplyConfirmTimeDiversity     =  1347, /* Maximum time in minutes over which a uniform distribution for time
                                                diversity can be drawn for the apply confirmation message.
                                                Range 0 - 255. */
   dfwDowloadConfirmTimeDiversity   =  1348, /* Maximum time in minutes over which a uniform distribution for time
                                                diversity can be drawn for the download confirmation message.
                                                Range 0 - 255. */
   smLogTimeDiversity               =  1349, /* Maximum time in minutes over which a uniform distribution for time
                                                diversity can be drawn for the stack manager log message.
                                                Range 0 - 255. */
   opportunisticThreshold           =  1350, /* An endDeviceEvent priority value which serves as the threshold for
                                                opportunistic alarms. If the endDeviceEvent has a priority less than
                                                this threshold it is discarded. If the priority is greater than or equal
                                                to this threshold but less than the realtimeThreshold it is processed as
                                                an opportunistic event, and it will ride on the next regularly scheduled
                                                bubble-up message which has space available. */
   realtimeThreshold                =  1351, /* An endDeviceEvent priority value which serves as the threshold for
                                                realtime alarms. If the event priority is greater than or equal to this
                                                threshold the event is processed immediately. A new message will be
                                                generated just to carry it to the subscriber. */
   orReadList                       =  1352, /* List of enumerations which are returned by the EndPoint when it
                                                receives an empty on-request read. I210+: 12,15,33 I210+RD 12,15,33,129
                                                KV2c: 12,15,34,35,36 */
   eventAssociations                =  1353, /* When read, the single item on the list contains the HEEP of an
                                                EndDeviceEvent which the requestor would like to learn about. The
                                                response should echo-back the EndDeviceEvent number followed by any
                                                associated readings. When written, the first item on the list must be
                                                the enum of the EndDeviceEvent being configured, and the following enums
                                                are ReadingTypeIDs which are to be associated with the event. For
                                                example, to change the parameters associated with a successful demand
                                                reset, one could write: 74,7,88,90. Example events which are supported
                                                include: - Power restoration -  Successful demand reset -  RCD Switch
                                                opening -  New meter metadata Note: This mechanism does NOT include
                                                bubble-up readings or response definitions: - Load profile channel
                                                configurations -  Daily shifted values -  On request read response
                                                Defaults are described in Table 15. */
   comDeviceInterfaceIdentifier     =  1354, /* The EndDevice port number associated with a particular action or event.
                                                Port 0   = communication with host meter Port 1..127 = communication
                                                           with RMTR
                                                Port 128 = Mfg serial port
                                                Port 129 = Debug port
                                                Port 254 = DCUII
                                                Hack Interface Port 255 = Tangwar MAC RF interface */
   dfwDecryptionErrorPatch          =  1355, /* Indicates that the stream is initialized, all packets have been
                                                received, and the signature is valid, but the decryption of the patch
                                                failed.  Default = FALSE if not sent, TRUE if ID sent but value not
                                                populated */
   dfwSignatureErrorPatch           =  1356, /* Indicates that the stream is initialized, and all packets have been
                                                received, but the computed digital signature of the resultant image does
                                                not match the digital signature for the image given in the initialize
                                                command. Default = FALSE if not sent, TRUE if ID sent but value not
                                                populated */
   dfwToVersion                     =  1357, /* A string which describes the Field Equipment's firmware version after
                                                patch application with the format: <Major build>.<Minor
                                                build>.<Engineering build> (e.g. 1.0.0).  Maximum length = 20
                                                characters.  */
   dfwCipherSuite                   =  1358, /* The cipher suite used for securing the patch.
                                                0 = unsecured
                                                1 = AES-CCM encryption starting at the 14th byte with 16 byte MAC using
                                                    security chip, Digital Signature ECDSA P-256. */
   dtlsMfgSubject1                  =  1359, /* The subject field from the first manufacturing certificate. The value is
                                                expected to be in DER format. */
   dtlsMfgSubject2                  =  1360, /* The subject field from the second manufacturing certificate. The value
                                                is expected to be in DER format. */
   debugPortEnabled                 =  1361, /* Indicates that the debug port is enabled when set to true. */
   dtlsServerCertificateSerialNum   =  1362, /* The serial number is an integer assigned by the CA to each certificate.
                                                It MUST be unique for each certificate issued by a given CA (i.e., the
                                                issuer name and serial number identify a unique certificate).] */
   dtlsDeviceCertificate            =  1363, /* An X.509v3 security certificate which is tied to the comDeiceMACAddress
                                                of the Field Equipment. This certificate is used for establishing DTLS
                                                connectinos with the Head End. The certificate is expected to be
                                                slightly less than 400 bytes in size and in the DER format. */
   edUtilitySerialNumber            =  1364, /* Serial number for the meter assembly as assigned by the utility.
                                                Note 1: This value may be written at time of integration, or after
                                                        deployment in the field.
                                                Note 2: This serial number is related to the edMfgSerialNumber (which
                                                        originates with the manufacturer of the meter and is obtained
                                                        electronically from the meter hardware). */
   engData2                         =  1365, /* engineering data (OB RSSI statistics ) */
   amBuMaxTimeDiversity             =  1366, /* This parameter defines a window of time in minutes during which a /bu/am
                                                message may bubble-in subsequent to the event occurring. The window
                                                starts immediately after the event occurs and ends amBuMaxTimeDiversity
                                                minutes later. EPs will select a fine-grained, uniformly-random delay
                                                within this window before transmitting data to the DCU. Additional
                                                delays may occur due the carrier sense capability of the radio. Allowed
                                                range: 0-15. */
   engData3                         =  1367, /* This parameter contains a blob of engineering data. Its format (and
                                                potentially its content) is different from engData1 and engData2. */
   eventPriority                    =  1368, /* When read, the eventPriority should carry the HEEP enum of an
                                                EndDeviceEvent which the requestor would like to learn about. The
                                                response should echo-back the EndDeviceEvent enum followed by its
                                                priority. When written, the first item on the list must be the enum of
                                                the EndDeviceEvent being configured, followed by a positive integer
                                                representing the new priority. (<EndDeviceEventTypeEnum>,<priority>).
                                                For example, to set the priority for a successful demand reset event to
                                                200 one could write: 74,200. Defaults are described in Table 17. */
   flashSecurityEnabled             =  1369, /* Indicates that both the Flash Protection (FPROT) and JTAG/Flash security
                                                are enabled when set to true. Note 1: this may be a derived rather than
                                                a stored value. Note 2: the device will reset when this is set. True
                                                when the device leaves the factory */
   realTimeAlarmIndexID             =  1370, /* EndDeviceEvents will be logged in two different circular buffers. High
                                                priority alarms are sent real-time and logged in the real-time-alarm
                                                log.  Medium priority alarms are sent opportunistically and logged in
                                                the opportunistic alarm log. The current index into the real-time alarm
                                                log (pointing to the newest entry) is defined by this
                                                realTimeAlarmIndexID.  The realTimeAlarmIndexID is presented to the HE
                                                in the ApplicationHeader as the requestID during bubble-up messages.
                                                The realTimeAlarmIndexID is incremented every time an EndDeviceEvent is
                                                added to the real-time alarm buffer. HE may use the realTimeAlarmIndexID
                                                values to request lost historical real-time alarms. */
   reboot                           =  1371, /* This is a ToolEP command that performs a soft restart of the
                                                microcomputer. There are no parameters. The action is taken immediately
                                                upon reception. */
   phyRxDetection                   =  1372, /* The phyRxDetectionConfig parameter is defined in the Tengwar PHY Spec
                                                v1.  This parameter defines the preamble and sync word in operation. One
                                                entry in the list is provided for each receiver. The length of the list
                                                is phyRcvrCount. Value Use 0 Synergize Electric RF. 1 Gen 1. 2 Zero
                                                preamble with tolerance of a missing bit in the sync word. Other
                                                Reserved. */
   phyRxFraming                     =  1373, /* The phyRxFramingConfig parameter is defined in the Tengwar PHY Spec
                                                (v1). One entry in the list is provided for each receiver. The length
                                                of the list is phyRcvrCount.
                                                Value    Use
                                                 0       Standard format with MSB tx first.
                                                 1       Gen 1 format with LSB tx first.
                                                 2       Gen 2 format with LSB tx first. */
   phyRxMode                        =  1374, /* This parameter defines the PHY mode used on the corresponding receiver
                                                when in the ready state (not transmitting.) The parameter consists of a
                                                list with one entry for each receiver radio. The length of the list is
                                                equal to phyRcvrCount. Valid range of each entry: 0-15. */
   phyFrontEndGain                  =  1375, /* This parameter sets the amount of receiver gain between the output of
                                                the antenna and the input to the receiver radio in 0.5 dB increments.
                                                This is used to properly calibrate the RSSI measurements given in dBm.
                                                Range -128 to +127. */
   virgin                           =  1376, /* This command is used over the mfg and debug serial ports to reinitialize
                                                a device to its newly-manufactured virgin state. It does not accept any
                                                arguments */
   drReadList                       =  1377, /* List of readingType enumerations which are returned by the EndPoint
                                                after it performs a successful demand reset (POST /dr) or in response to
                                                a request for demand data (GET /dr). A special rule is in place for
                                                coincident reads: Any reading identified in the meter program as a
                                                coincident value that is associated with and based on a reading
                                                identified on the drReadList will be automatically included in the
                                                message as well. */
   restorationDeclarationDelay      =  1378, /* This parameter determines the amount of time following the restoration
                                                of power to an EndPoint after a sustained outage before a restoration
                                                condition can be declared. If power has not been lost after waiting the
                                                delay then a restoration condition is declared. The restoration
                                                declaration delay is expressed in seconds. Range: 0 - 300 Note: Some
                                                utilities only want restorations to be reported when power stays on for
                                                a period of time. */
   initialRegistrationTimeout       =  1379, /* The initial value for registrationTimeout following the first failed
                                                attempt to publish Metadata. The actual timeout is a uniform random
                                                delay from minRegistrationTimeout to registrationTimeout seconds. The
                                                registration timeout delay for subsequent consecutive retries is doubled
                                                for each attempt until the maxRegistrationTimeout is reached. The
                                                initialRegistrationTimeout is expressed in seconds.  Range: 1,800 -
                                                43,200 */
   initialAuthenticationTimeout     =  1380, /* The initial value for authenticationTimeout following the first DTLS
                                                SSL_FATAL_ERROR before DTLS Authentication is retried with the head end.
                                                The authentication timeout delay for subsequent consecutive retries is
                                                doubled for each attempt until the maxAuthenticationTimeout is reached.
                                                The initialAuthenticationTimeout is expressed in seconds.
                                                Range: 1,800 - 43,200 */
   rtcDateTime                      =  1381, /* The dateTime fetched from the module's RTC hardware. (Ordinarily this is
                                                fetched only upon powerup.) */
   edTempSampleRate                 =  1382, /* The period (in seconds) between temperature samples of the meter's
                                                thermometer. Note 1: If the meter does its own sampling, this parameter
                                                may not be writeable, or may not even be readable. Note 2: If the EP
                                                must read ED's temperature, this value will be read/writeable, stored in
                                                the EP, and used to govern EP behavior as it monitors the ED. */
   edTempHysteresis                 =  1383, /* The hysteresis (in degrees C) to be subtracted from a maximum
                                                temperature threshold before issuing a meter high temperature cleared
                                                alarm. */
   epTempHysteresis                 =  1384, /* The hysteresis (in degrees C) to be subtracted from a maximum
                                                temperature threshold before issuing a radio high temperature cleared
                                                alarm; or to be added to a minimum temperature threshold before issuing
                                                a radio low temperature cleared alarm. */
   epTempMinThreshold               =  1385, /* The minimum temperature threshold that when reached, triggers a low
                                                temperature event for the radio. */
   epMinTemperature                 =  1386, /* The lowest temperature ever witnessed by the EndPoint (comm module
                                                radio) over its lifetime. */
   decommissionMode                 =  1387, /* When (decommissionMode > 0) this parameter temporarily suppresses the
                                                ability of the device to declare outage, restoration, or power quality
                                                events. The device will not: - Declare or log outage, restoration, or
                                                power quality events If (decommissionMode  > 0) at powerup, the device
                                                will:  Set the newRegistrationRequired flag Clear decommissionMode If
                                                (decommissionMode == 1) at powerup, the device will additionally: Clear
                                                installationDateTime Clear network stack statistics Clear all events and
                                                metering values stored in the device (previous demand values and
                                                historical data including daily shifted reads and interval data.) Invalidate
                                                system and RTC time*/
   BERTest                          =  1388, /* Refer to the Tangwar Stack Manager document for definition of this
                                                value.  The argument for this command takes the form: { test frame,
                                                mode, mode parameters, framing, detection,txChannel}. For example:
                                                BERTest 0x01020304,TEST,0,1,2,1000 */
   deviceEnergization               =  1389, /* Device energization (state). Status of events #119-120 '1' = 'live', '0'
                                                ='dead' */
   meterDemandResetStatus           =  1390, /* '1' = demand reset lockout is in effect '0' = demand reset lockout is
                                                not in effect */
   comDeviceSecuritySessionStatus   =  1391, /* Status of events #160-161 True if session active False if session failed */
   comDeviceFWSignatureStatus       =  1392, /* Status of event #164 True if signature succeeded */
   comDeviceFWDecryptionStatus      =  1393, /* Status of event #163 True if Decryption succeeded False if decryption
                                                failed */
   comDeviceFWReplacedStatus        =  1394, /* Status of event #87 True if firmware has been successfully replaced
                                                False if apply has never occurred or was unsuccessful */
   meterProgramChangedStatus        =  1395, /* Status of event #65 True if the host meter indicates that it has been
                                                reprogrammed */
   metrologyReadingsResetStatus     =  1396, /* Status of event #162 TRUE if the host meter indicates that the energy
                                                registers have been zeroed. */
   meterTempThreshMaxLimitReached   =  1397, /* status of events #182-183 TRUE if the host meter indicates that it has a
                                                temperature above its high temp threshold */
   meterCurrentMaxLimitReached      =  1398, /* Excess device current (status) TRUE when class amperage of meter has
                                                (ever) been exceeded. (Taking the amperage averaged over several minutes
                                                and comparing it to the meter class amperage rating.) */
   meterSecurityAccessNotAuthorized =  1399, /* Device tamper (status) TRUE = Host meter tamper indiciation */
   meterSecurityRotationReversed    =  1400, /* Device ReverseRotationTamper (status) TRUE = Host meter tamper
                                                indiciation */
   comDeviceConfigCorruptStatus     =  1401, /* Status of event #174-175 TRUE = comDevice Configuration is corrupted
                                                (according to CRC failure) */
   meterUnprogrammedStatus          =  1402, /* Status of events #69-70 TRUE when host meter is unprogrammed */
   meterProgramLostState            =  1403, /* Status of events #67-68 TRUE when host meter loses its program FALSE
                                                when host meter regains a program */
   comDeviceMeterIOErrorStatus      =  1404, /* Status of events #83-84 TRUE when comDevice cannot communicate with host
                                                meter FALSE when host meter responds to comDevice */
   meterDiagProcessorErrorStatus    =  1405, /* Status of events #5-6 TRUE when host meter indicates a diagnostic error
                                                due to a DSP or microprocessor fault */
   meterBatteryStatus               =  1406, /* Status of events #23-24 TRUE when host meter indicates a low battery */
   meterDemandOverflow              =  1407, /* Status of events #71-72 TRUE when host meter indicates a demand overflow
                                                (since the last demand reset) */
   meterDiagFlashMemoryErrorStatus  =  1408, /* Status of events #17-18 TRUE when the host meter indicates a diagnostic
                                                FLASH memory error */
   meterDiagRamMemoryErrorStatus    =  1409, /* Status of events #19-20 TRUE when the host meter indicates a diagnostic
                                                RAM memory error */
   meterDiagRomMemoryErrorStatus    =  1410, /* Status of events #21-22 TRUE when the host meter indicates a diagnostic
                                                ROM memory error */
   meterError                       =  1411, /* Status of events #1-2 Device diagnostic (status) TRUE when Host meter
                                                indicates any generic diagnostic error */
   reverseReactivePowerflowStatus   =  1412, /* Status of events #45-46 TRUE whenhost meter indicates that reactive
                                                power is flowing in the reverse direction (contributing VArs to the
                                                grid). */
   meterVoltageLossDetected         =  1413, /* Status of events #154-155 TRUE when host meter indicates that mains
                                                voltage is so low that it may lose accuracy or it may powerdown. */
   meterSagPhAStarted               =  1414, /* Status of event #33 electricitySecondaryMetered sag phaseA (status) TRUE
                                                when host meter indicates that phaseA voltage is currently sustained
                                                below the sag threshold */
   meterSagPhBStarted               =  1415, /* Status of event #165 electricitySecondaryMetered sag phaseB (status)
                                                TRUE when host meter indicates that phaseB voltage is currently
                                                sustained below the sag threshold */
   meterSagPhCStarted               =  1416, /* Status of event #167 electricitySecondaryMetered sag phaseC (status)
                                                TRUE when host meter indicates that phaseC voltage is currently
                                                sustained below the sag threshold */
   meterSwellPhAStarted             =  1417, /* Status of event #35 electricitySecondaryMetered swell phaseA (status)
                                                TRUE when host meter indicates that phaseA voltage is currently
                                                sustained above the swell threshold */
   meterSwellPhBStarted             =  1418, /* electricitySecondaryMetered swell phaseB (status) TRUE when host meter
                                                indicates that phaseB voltage is currently sustained above the swell
                                                threshold */
   meterSwellPhCStarted             =  1419, /* electricitySecondaryMetered swell phaseC (status) TRUE when host meter
                                                indicates that phaseC voltage is currently sustained above the swell
                                                threshold */
   meterQualityExceeded             =  1420, /* Excess electricitySecondaryMetered powerQuality (status) TRUE when any
                                                of the power quality counters defined by the event are excessive */
   meterVoltageImbalanced           =  1421, /* Excess electricitySecondaryMetered voltageImbalance phasesABC (status)
                                                TRUE when the meter indicates a phase imbalance (where one phase is more
                                                than a few percentage different than the other two.) */
   lowCommunicationTemperature      =  1422, /* low communication temperature (status) TRUE if the comDevice indicates
                                                that it has a temperature below its low temp threshold */
   highCommunicationTemperature     =  1423, /* high communication temperature (status) TRUE if the comDevice indicates
                                                that it has a temperature above its high temp threshold */
   comDeviceMagneticFieldStatus     =  1424, /* Status of events #184-185 TRUE when the comDevice detects that an
                                                unusually strong magnetic field is present. */
   comDeviceMotionStatus            =  1425, /* Status of events #186-187 TRUE when the comDevice detects that the meter
                                                has been shaken or moved and powered down. FALSE after powerup (which
                                                implies that this status will always read FALSE when queried and
                                                powered.) */
   comDeviceTiltStatus              =  1426, /* TRUE when the comDevice detects that it is upside down (a.k.a.
                                                "tilted"). FALSE when the comDevice detects that it is rightside up. */
   lpBuChannel7                     =  1427, /* Ch 7     See Below */
   lpBuChannel8                     =  1428, /* Ch 8     See Below */
   lpBuChannel9                     =  1429, /* Ch 9     See Below */
   lpBuChannel10                    =  1430, /* Ch 10    See Below */
   lpBuChannel11                    =  1431, /* Ch 11    See Below */
   lpBuChannel12                    =  1432, /* Ch 12    See Below */
   lpBuChannel13                    =  1433, /* Ch 13    See Below */
   lpBuChannel14                    =  1434, /* Ch 14    See Below */
   lpBuChannel15                    =  1435, /* Ch 15    See Below */
   lpBuChannel16                    =  1436, /* Ch 16    See Below */
   lpBuChannel17                    =  1437, /* Ch 17    See Below */
   lpBuChannel18                    =  1438, /* Ch 18    See Below */
   lpBuChannel19                    =  1439, /* Ch 19    See Below */
   lpBuChannel20                    =  1440, /* Ch 20    See Below */
   lpBuChannel21                    =  1441, /* Ch 21    See Below */
   lpBuChannel22                    =  1442, /* Ch 22    See Below */
   lpBuChannel23                    =  1443, /* Ch 23    See Below */
   lpBuChannel24                    =  1444, /* List of the following load profile bubble up attributes for Channel 24:
                                                -  sourceReadingType: Source readingType HEEP enumeration
                                                -  samplePeriod:      The time in minutes between sampling of the source.
                                                -  isDeltaData:       A Boolean value that indicates if samples are to be
                                                                      subtracted from each other in the formation of the
                                                                      business value.
                                                -  trimDigits:        The number of digits to lose in order to convert the
                                                                      computed integer business value to the stored integer
                                                                      business value.
                                                -  StorageBytes:      The number of bytes of storage allocated to this
                                                                      channel.
                                                The list is formed as a string of space separated integers. Allowable
                                                values are product specific. */

   resourceID                       =  1445, /* When the "resource discovery" resource is used, this parameter allows
                                                the subject resource to be passed as a parameter. */
   virginDelay                      =  1446, /* Erases the signature but allows the device to continue to run. The
                                                device will be virgined on the next powerup. (See enum #1376) */
   scheduledDemandResetDay          =  1447, /* This value defines the occurrence of the next scheduled autonomous
                                                demand reset. This value represents a day of the month. If it is set to
                                                a value ranging from 1 to 31, the EndPoint will look for this day of the
                                                month to occur. When this day occurs, and the dailySelfReadTime occurs,
                                                the Endpoint will issue a demand reset to the meter and report the
                                                results to the HeadEnd.  If the scheduledDemandResetDay is set to a
                                                value outside the allowed range, the reset does not occur.  The demand
                                                reset will recur every month when this day number occurs.  Note: See
                                                MS/FE Register #334 for a similar feature called "Billing Cycle Date".*/
   meterSagPhAStopped               =  1448, /* Status of event #34 TRUE when host meter indicates that phaseA voltage
                                                is no longer below the sag threshold.  See also ReadingType #1414.  */
   meterSagPhBStopped               =  1449, /* Status of event #166 TRUE when host meter indicates that phaseB voltage
                                                is no longer below the sag threshold.  See also ReadingType #1415.  */
   meterSagPhCStopped               =  1450, /* Status of event #168 TRUE when host meter indicates that phaseC voltage
                                                is no longer below the sag threshold.  See also ReadingType #1416.  */
   meterSwellPhAStopped             =  1451, /* Status of event #36 TRUE when host meter indicates that phaseA voltage
                                                is no longer above the swell threshold.  See also ReadingType #1417. */
   meterSwellPhBStopped             =  1452, /* Status of events #170 TRUE when host meter indicates that phaseB voltage
                                                is no longer above the swell threshold.  See also ReadingType #1418.  */
   meterSwellPhCStopped             =  1453, /* Status of events #172 TRUE when host meter indicates that phaseC voltage
                                                is no longer above the swell threshold.  See also ReadingType #1419.  */
   FctModuleTestDate                =  1454, /* The date that the communication module was FCT (Functional Certification
                                                Test) tested.  This register is updated by the FCT program. The date is
                                                expressed as the number of days since Jan 1, 1990.  */
   FctEnclosureTestDate             =  1455, /* The date that the communication module was FCT (Functional Certification
                                                Test) tested.  This register is updated by the FCT program. The date is
                                                expressed as the number of days since Jan 1, 1990.  */
   IntegrationSetupDate             =  1456, /* The date that the communication module was integrated into the meter.
                                                This value is updated by the Integration program. */
   FctModuleProgramVersion          =  1457, /* The software version (major number, minor number, and build number) of
                                                the FCT application used when the communication module was FCT tested.
                                                */
   FctEnclosureProgramVersion       =  1458, /* The software version (major number, minor number, and build number) of
                                                the FCT application used when the communication module was FCT tested.
                                                */
   IntegrationProgramVersion        =  1459, /* This parameter contains the software version (major number, minor
                                                number, and build number) of the integration application used when the
                                                EndPoint enclosure was integrated. This parameter is updated by the
                                                integration program when the module is integrated into a meter. */
   FctModuleDatabaseVersion         =  1460, /* This parameter contains the date and revision of the database update
                                                that was used by the FCT program when testing EndPoint modules. This
                                                register is updated by the FCT application. */
   FctEnclosureDatabaseVersion      =  1461, /* This parameter contains the date and revision of the database update
                                                that was used by the FCT program when testing EndPoint enclosures.  This
                                                value is updated by the FCT application. */
   IntegrationDatabaseVersion       =  1462, /* The date and revision of the database update that was used by the
                                                integration program when integrating a module into a meter. This value
                                                is updated by the integration application. */
   dataConfigurationDocumentVersion =  1463, /* The version of the EEPROM map, or equivalent reference used during the
                                                FCT test.  This register is updated by the FCT application. */
   manufacturerNumber               =  1464, /* This parameter contains the information found on the Manufacturer Number
                                                / Date Code barcode label of the EndPoint communication module. This
                                                parameter is updated by the FCT application. */
   repairInformation                =  1465, /* This parameter contains the date the assembly was repaired and FCT
                                                tested along with the location of where the rework was performed. This
                                                register is updated by the FCT program.  */
   meterCrossPhaseStarted           =  1466, /* Status of event #41 TRUE when host meter indicates that a cross phase
                                                event has started (GE KV2c Diagnostic 1). */
   meterCrossPhaseStopped           =  1467, /* Status of event #42 TRUE when host meter indicates that a cross phase
                                                event has stopped (GE KV2c Diagnostic 1). */
   meterVoltageImbalanceStarted     =  1468, /* Status of event #49 TRUE when host meter indicates that a voltage
                                                imbalance event has started (GE KV2c Diagnostic 2). */
   meterVoltageImbalanceStopped     =  1469, /* Status of event #50 TRUE when host meter indicates that a voltage
                                                imbalance event has stopped (GE KV2c Diagnostic 2). */
   meterInactivePhaseStarted        =  1470, /* Status of event #39 TRUE when host meter indicates that an iniactive
                                                phase event has started (GE KV2c Diagnostic 3). */
   meterInactivePhaseStopped        =  1471, /* Status of event #40 TRUE when host meter indicates that an inactive
                                                phase event has stopped (GE KV2c Diagnostic 3). */
   meterPhaseAngleAlertStarted      =  1472, /* Status of event #31 TRUE when host meter indicates that a phase angle
                                                alert event has started (GE KV2c Diagnostic 4). */
   meterPhaseAngleAlertStopped      =  1473, /* Status of event #32 TRUE when host meter indicates that a phase angle
                                                alert event has stopped (GE KV2c Diagnostic 4). */
   meterHighDistortionAlertStarted  =  1474, /* Status of event #43 TRUE when host meter indicates that a high
                                                distortion alert event has started (GE KV2c Diagnostic 5). */
   meterHighDistortionAlertStopped  =  1475, /* Status of event #44 TRUE when host meter indicates that a high
                                                distortion alert event has stopped (GE KV2c Diagnostic 5). */
   meterUnderVoltagePhAStarted      =  1476, /* Status of event #33 TRUE when host meter indicates that an undervoltage
                                                phase A event has started (GE KV2c Diagnostic 6). */
   meterUnderVoltagePhAStopped      =  1477, /* Status of event #34 TRUE when host meter indicates that an undervoltage
                                                phase A event has stopped (GE KV2c Diagnostic 6). */
   meterOverVoltagePhAStarted       =  1478, /* Status of event #35 TRUE when host meter indicates that an overvoltage
                                                phase A event has started (GE KV2c Diagnostic 7). */
   meterOverVoltagePhAStopped       =  1479, /* Status of event #36 TRUE when host meter indicates that an overvoltage
                                                phase A event has stopped (GE KV2c Diagnostic 7). */
   meterHighNeutCurrentStarted      =  1480, /* Status of event #37 TRUE when host meter indicates that a high neutral
                                                current event has started (GE KV2c Diagnostic 8). */
   meterHighNeutCurrentStopped      =  1481, /* Status of event #38 TRUE when host meter indicates that a high neutral
                                                current event has stopped (GE KV2c Diagnostic 8). */
   powerQualityEventDuration        =  1482, /* This parameter determines the maximum amount of time following the loss
                                                of power to an EndPoint that momentary interruptions can be grouped into
                                                a momentary interruption event. According to IEEE 1366, all switching
                                                operations must be completed within five minutes of the first
                                                interruption. The power quality event duration is expressed in seconds.
                                                Range: 60 - 300   */
   securityConfigHash               =  1483, /* This value is derived by the value contained in other parameters. It is
                                                used to test the setting of the security related parameters to ensure
                                                their correctness. It is computed as the SHA-256 hash of the
                                                appSecurityAuthMode, dtlsNetworkHESubect, dtlsNetworkMSSubject,
                                                dtlsNetworkRootCA, and flashSecurityEnabled.  */
   networkConfigHash                =  1484, /* This value is derived by the value contained in other parameters. It is
                                                used to test the setting of the network related parameters to ensure
                                                their correctness. It is computed as the SHA-256 hash of the
                                                ipHEContext, macNetworkId, phyAvailableChannels, phyRxChannels, and
                                                phyTxChannels.  */
   meteringConfigHash               =  1485, /* This value is derived by the value contained in other parameters. It is
                                                used to test the setting of the network related parameters to ensure
                                                their correctness. It is computed as the SHA-256 hash of the
                                                demandFutureConfiguration, demandPresentConfiguration, drReadList,
                                                dailySelfReadTime, dsBuDataRedundancy, dsBuMaxTimeDiversity,
                                                dsBuReadingTypes, dsBuTrafficClass, and orReadList.  */
   eventConfigHash                  =  1486, /* This value is derived by the value contained in other parameters. It is
                                                used to test the setting of the network related parameters to ensure
                                                their correctness. It is computed as the SHA-256 hash of the
                                                opportunisticThreshold, outageDeclarationDelay, realtimeThreshold, and
                                                resotrationDeclarationDelay.  */
   lpConfigHash                     =  1487, /* This value is derived by the value contained in other parameters. It is
                                                used to test the setting of the network related parameters to ensure
                                                their correctness. It is computed as the SHA-256 hash of the
                                                lpBuChannel1, lpBuChannel2, lpBuChannel3, lpBuChannel4, lpBuChannel5,
                                                lpBuChannel6, lpBuDataRedundancy, lpBuMaxTimeDiversity, lpBuSchedule,
                                                and lpBuTrafficClass.  */
   macChannelSetsSTAR               =  1488, /* Lists of STAR channel pairs with values ranging from 0 to 3200. Refer to
                                                the Tengwar MAC specification for more information. */
   timeSource                       =  1489, /* Allows this device to broadcast time. */
   macChannelSets                   =  1490, /* Lists of SRFN channel pairs with values ranging from 0 to 3200. Refer to
                                                the Tengwar MAC specification for more information. */
   phyTxMaxPower                    =  1491, /* The maximum transmit power level in dBm under the control of the PHY
                                                layer. This parameter has a granularity of 0.1 dB, but the actual
                                                granularity of changes to the transmit power level are product specific
                                                and may not follow a linear distribution. Valid Range: 10.0 – 40.0 dBm */
   stOptLoopbackTest                =  1492, /* A counter, that when written, initiates the optical port communication
                                                loopback test. An external cable must be attached which provides the
                                                loop between the optical TX and RX UART lines. The counter will
                                                decrement after each test cycle. Testing will stop when the counter
                                                reaches zero. With each failure the stOptLoopbackFailCount parameter is
                                                incremented. */
   stOptLoopbackFailCount           =  1493, /* The number of times the “stOptLoopbackTest” has failed. */
   stDebugLoopbackTest              =  1494, /* A counter, that when written, initiates the debug port communication
                                                loopback test. An external cable must be attached which provides the
                                                loop between the debug TX and RX UART lines. The counter will decrement
                                                after each test cycle. Testing will stop when the counter reaches zero.
                                                With each failure the stDebugLoopbackFailCount parameter is incremented.
                                                */
   stDebugLoopbackFailCount         =  1495, /* The number of times the “stDebugLoopbackTest” has failed. */
   locationLatitude                 =  1496, /* The latitude, measured by a GPS triangulation, and expressed in decimal
                                                degrees. We would like to see accuracy expressed to 6 digits past the
                                                decimal point. */
   locationLongitude                =  1497, /* The longitude, measured by a GPS triangulation, and expressed in decimal
                                                degrees. */
   locationElevation                =  1498, /* The elevation, measured by a GPS triangulation, and expressed in decimal
                                                degrees. */
   timeSetMaxOffset                 =  1499, /* Maximum amount of random offset in milliseconds from timeSetStart for
                                                which the source will begin transmitting the time set MAC command.  */
   devPhyTamperStatus               =  1500, /* Device physicalTamper (status)
                                                Status of events #2014-2015
                                                TRUE = DCU enclosure open
                                                FALSE = DCU enclosure closed */
   comDeviceBootloaderVersion       =  1501, /* Firmware version applies to the Bootloader */
   comDeviceGatewayConfigRdType     =  1502, /* The configuration of the MAC layer (SRFN, STAR or NONE */
   engBuEnabled                     =  1503, /* Enable the engineering statistics to bubble up */
   mtlsAuthenticationWindow         =  1504, /* The amount of time in seconds that a received message can lag behind the
                                                current device time and still pass message validation. */
   mtlsNetworkTimeVariation         =  1505, /* The amount of time in seconds that a received message timestamp can exceed
                                                the current device time and still pass message validation. */
   dfwFileType                      =  1506, /* An enumerated value which describes the type of file being sent using
                                                DFW process.  This information will be used by the EP to interpret
                                                and apply the file contents */
   passwordPort0Master              =  1507, /* This password escalates the rights of the communication module as it
                                                accesses the meter via Port0.  This password is only to be used by the
                                                module for commands that require elevated privledges. */
   dfwApply                         =  1508, /* This command performs the /df/ap operation to the streamID identified
                                                in the parameter value.  This command, written using the /or/pm resource
                                                may be used as an alternative to the /df/ap command to apply meter and
                                                module configuration changes together. */
   dfwCompatibilityTestStatus       =  1509, /* An enumerated value which describes the status of an optional
                                                compatibility test section in a transferred file.
                                                Value    Meaning
                                                 0       Test section provided and passed.
                                                 1       Test section provided and failed.
                                                 2       Test section not testable or not applicable. */
   dfwProgramScriptStatus           =  1510, /* An enumerated value which describes the status of an executable section
                                                in a transferred file.
                                                Value    Meaning
                                                 0       Program section provided and passed.
                                                 1       Program section provided and failed.
                                                 2       Program section not supplied or not applicable. */
   dfwAuditTestStatus               =  1511, /* An enumerated value which describes the status of an optional audit
                                                test section in a transferred file.
                                                Value    Meaning
                                                 0       Audit section provided and passed.
                                                 1       Audit section provided and failed.
                                                 2       Audit section not testable or not applicable. */
   dfwDupDiscardPacketQty           =  1512, /* Counter to indicate the number of dfw packets that have already
                                                been received. */
   simulateLastGaspStart            =  1513, /* Writing this value with a DateTime value causes the EndPoint to initiate
                                                a last gasp simulation at the specified date and time. The EndPoint will
                                                send up the last gasp test alarm (EndDeviceEvent #191) in a pattern just
                                                like the last gasp alarm. Any value which occurs in the past will cause
                                                the simulation to be cancelled.*/
   simulateLastGaspDuration         =  1514, /* Defines length (in seconds) a last gasp simulation is to endure. The
                                                Power Quality (EndDeviceEvent #xxx) or Power Restored (EndDeviceEvent #192)
                                                test message is transmitted at the end of the simulation in a manner
                                                just like the normal alarms.*/
   simulateLastGaspTraffic          =  1515, /* If set to TRUE, during last gasp simulation the EndPoint shall only
                                                transmit simulation traffic and high priority alarm messages. Other
                                                traffic during this period is discarded (rather than suspended). */
   edBootVersion                    =  1516, /* The host board boot loader version. */
   edBspVersion                     =  1517, /* The host board BSP version. */
   edKernelVersion                  =  1518, /* The host board Linux kernel version. */
   edCarrierHwVersion               =  1519, /* The carrier board hardware version. */
   phyCcaAdaptiveThresholdEnable    =  1520, /* Indicates if the phyCcaAdaptiveThreshold is used (TRUE) or
                                                if phyCcaThreshold (FALSE) is used for the CCA algorithm. */
   phyCcaOffset                     =  1521, /* The offset in dB from phyNoiseEstimate used to obtain phyCcaAdaptiveThreshold. */
   phyNoiseEstimate                 =  1522, /* The noise floor estimate for each channel in phyAvailableChannels.
                                                For entries in phyAvailableChannels that are invalid channels, the value here is set to 55.
                                                The length of the list is aMaxChannel. */
   phyThermalControlEnable          =  1523, /* Indicates if the thermal control feature is enabled (TRUE) or disabled (FALSE). */
   phyThermalProtectionCount        =  1524, /* The number of times the thermal protection feature has detected a peripheral temperature exceeding a given limit. */
   phyThermalProtectionEnable       =  1525, /* Indicates if the thermal protection feature is enabled (TRUE) or disabled (FALSE). */
   phyThermalProtectionEngaged      =  1526, /* Indicates if a PHY peripheral has exceeded an allowed temperature limit.  If phyThermalProtectionEnable is set to FALSE, this field will also be set to FALSE. */

   // 02/20/2021 - For ffuture support when assinged in in the DSD
   // macPacketTimeout               =  9999, /* The number of milliseconds to maintain a given device’s packet ID pair before clearing the information from the MAC. */

   macTransactionTimeout            =  1527, /* The number of seconds that a transaction is allowed to be in process before being aborted. */
   macTransactionTimeoutCount       =  1528, /* The number of times the MAC had to abort a transaction for taking too long. */
   macTxLinkDelayCount              =  1529, /* The number of times the MAC had to delay a frame due to the PHY indicating a delay was necessary. */
   macTxLinkDelayTime               =  1530, /* The total amount of time in milliseconds that the MAC has delayed due to the PHY indicating a delay was necessary. */
   phyNoiseEstimateRate             =  1531, /* The frequency in minutes that noise measurements are taken while in receive mode.A value of 0 disables the noise estimation process. */
   macCsmaMaxAttempts               =  1532, /* The maximum number of attempts by the MAC to gain access to the channel before failing the transaction request. */
   macCsmaMinBackOffTime            =  1533, /* The minimum amount of back off time in milliseconds for the CSMA algorithm. */
   macCsmaMaxBackOffTime            =  1534, /* The maximum amount of back off time in milliseconds for the CSMA algorithm. */
   macCsmaPValue                    =  1535, /* The probability that the CSMA algorithm will decide to transmit when an idle channel is found. */
   macReassemblyTimeout             =  1536, /* The amount of time in seconds allowed to elapse between received segments of a single packet before the MAC
                                                will discard the entire packet and request any associated ARQ windows. */
   timeDefaultAccuracy              =  1537, /* The default accuracy of the system clock on the device in units of log2(seconds). Range: -20 to 5 */
   timeQueryResponseMode            =  1538, /* When receiving a time request, send the time in broadcast (0, default), unicast(1) or ignore request (2). */
   engBuTrafficClass                =  1539, /* This parameter sets the traffic class for a Eng Stats bubble-up
                                                transmission. Traffic class attributes such as priority or reliability
                                                can be used to determine system behavior. Range: 0 - 63 */
   hostEchoTest                     =  1540, /* Intracommunication echo test within the EP. */
   nwActiveActTimeout               =  1541, /* Defines the active network activity timeout in seconds for the EP board-to-board communication. */
   nwPassActTimeout                 =  1542, /* Defines the passive network activity timeout in seconds for the EP board-to-board communication. */
   vswr                             =  1543, /* This command retrieves the last cached vswr value measured. */
   capableOfEpPatchDFW              =  1544, /* Indicates that the EndPoint device supports the Download FirmWare feature for its bootloader. */
   capableOfEpBootloaderDFW         =  1545, /* Indicates that the EndPoint device supports the Download FirmWare feature for its code. */
   capableOfMeterPatchDFW           =  1546, /* Indicates that the EndPoint device and meter (together) support the Download FirmWare feature for meter firmware basecode. */
   capableOfMeterBasecodeDFW        =  1547, /* Indicates that the EndPoint device and meter (together) support the Download FirmWare feature for meter firmware patches. */
   capableOfMeterReprogrammingOTA   =  1548, /* Indicates that the EndPoint device and meter (together) support the ability to reconfigure (reprogram) the meter over the air. */
   enableOTATest                    =  1549, /* Temporarily allows over the air testing of certain tests which ordinarily are restricted to the mfg serial port.
                                                The value written represents the time in minutes OTA testing is enabled. The allowed range is from 0 to 255 minutes.
                                                (The parameter is essentially a countdown timer.) The following commands are to have their accessibility promoted
                                                from "2" to "RW": stTxCwTest, stRx2gfskTest, stRx4gfskTest, stTxBlurtTest, phyTxFrequencies, phyRxFrequencies,
                                                phyAvailableFrequencies, phyTxChannels, phyRxChannels, and phyAvailableChannels. */
   phyDemodulator                   =  1550, /* The demodulator used by the each respective receiver. Range 0-255 */
   capableOfPhaseSelfAssessment     =  1551, /* Indicates that the EndPoint device supports the ability to determine its own relative system phasor based on timesync synchrophasor
                                                publications */
   capableOfPhaseDetectSurvey       =  1552, /* Indicates that the EndPoint device supports the phase detect survey (GEN1) process.   True  1  Bool */
   pdSurveyQuantity                 =  1553, /* The number of PD surveys which are to be included in the daily or LP interval report
                                                Range 1? qty ? 3  1  1  Uint */
   pdSurveyMode                     =  1554, /* Used to Enable/Display Phase Detection Survey Mode
                                                  0 : Phase Detect Surveys disabled
                                                  1 : Phase Detect Surveys Enabled
                                                 >1 : reserved.  1 bytes, default to 1 */
   pdSurveyStartTime                =  1555, /* Defines the starting time at which phase detect surveys will begin. The EndPoint will use timesyncs received after this moment as PD Survey triggers.
                                                The survey will continue for the pdSurveyPeriod until the pdSurveyQuantity is exhausted and then automatically restart a new SurveyPeriod after
                                                completion of the current one. The value is expressed as the number of seconds since Midnight local time. */
   pdZCProductNullingOffset         =  1556, /* The amount (in hundredths of a degree) to be added to the calculation of the local phasor angle to null the end device design's zero cross latency.
                                                Range -36000 to 36000   Default = -1200,  3  Int */
   pdLcdMsg                         =  1560, /* A 6 character message displayed on the meter's LCD to indicate the meter's system phase.
                                                Note: In the I210+c this is mapped to MT119 offset 0. Character restrictions apply. Supported characters include: AbCdEFghIJ L nOPqrStU y 0123456789 -= */
   pdSurveyPeriod                   =  1561, /* The period at which the EndPoint listens after the pdSurveyStartTime to perform time-sync triggered surveys.
                                                Unit of measure: seconds
                                                Valid settings: 120, 300, 900, 1800, 3600 (1 hr), 7200, 10800, 14400, 21600, 28800, 43200, 86400 (24 hr)
                                                The periodicity, in seconds, that phase detect will repeat.  By defauly this is 86400 (daily) */
   pdDistortionPFThreshold          =  1562, /* The level of PF found in the meter which cautions against use of data from a PD survey. If PD surveys are running, event #242 is to be sent instead of
                                                #241 when the absolute value of PF from the meter is less than this threshold, and yet higher than pdHighDistortionPFThreshold. */
   pdHighDistortionPFThreshold      =  1563, /* The level of PF found in the meter which disqualifies the EndPoint from taking a PD survey. If PD surveys are running, event #243 is to be sent
                                                instead of #241 when thie absolute value of PF from the meter is less than this threshold. */
   pdBuMaxTimeDiversity             =  1564, /* Phase Detection Maximum Time Diversity in minutes 0 < pdPeriod */
   pdBuDataRedundancy               =  1565, /* This parameter controls the number of times that the application layer resends the phase detect survey data. 0-3 */
   lastGaspMaxNumAttempts           =  1566, /* The maximum number of attempts that the EndPoint is to attempt within the 20 minute period subsequent to a sustained outage declaration.*/
   pdSurveyPeriodQty                =  1567, /* The number of survey periods to be included in the response  0-1 */
   simulateLastGaspMaxNumAttempts   =  1568, /* The maximum number of attempts that the EndPoint is to attempt within the simulateLastGaspDuration period subesquent to simulateLastGaspStart
                                                Allowed range is 0-6 */
   simulateLastGaspStatCcaAttempts      = 1569, /* A statistic counter representing the number of clear channel assessments performed by the EP in an attempt to send last gasp simulation messages.
                                                   This is an array of six integers. Each entry representing the count on the corresponding last-gasp transmission. Each entry is zeroed at the beginning of a simulation.*/
   simulateLastGaspStatPPersistAttempts = 1570, /* A statistic counter representing the number of p-persistant attempts performed by the EP in an attempt to send last gasp simulation messages.
                                                   This is an array of six integers. Each entry representing the count on the corresponding last-gasp transmission. Each entry is zeroed at the beginning of a simulation.*/
   simulateLastGaspStatMsgsSent         = 1571, /* A statistic counter representing the number of last gasp simulation messages actually sent by the EP.
                                                   This is an array of six integers. Each entry representing the count on the corresponding last-gasp transmission.  Each entry is zeroed at the beginning of a simulation.*/
   phyMaxTxPayload                  =  1572, /* The maximum allowed TC payload size */

/* 1573-1597 */

   fngVSWR                          =  1598, /* For Field Network Gateway equipped with VSWR measurement circuitry, this
                                                command retrieves the last cached vswr value measured. When displayed,
                                                it should be represented as "x:1". 'x' will be the calculated VSWR */
   fngVswrNotificationSet           =  1599, /* Allows the network operator to set/read the VSWR Notification Set point.
                                                The VSWR Notification (Warning) will be sent to the Headend, allowing
                                                the network operator to take action if required at the site experiencing
                                                high VSWR while transmitting. Valid set point range is 2 to 10.
                                                Note: The VSWR notification setpoint should be set less than the VSWR
                                                Shutdown set point. */
   fngVswrShutdownSet               =  1600, /* Allows the network operator to set/read the VSWR Shutdown Set point.
                                                The VSWR shutdown (fault) will be sent to the Headend notifying the
                                                network operator to take action as there is a problem with the RF
                                                network while transmitting.  When this shutdown threshold is reached,
                                                there will be no downlink from this FNG. Valid set point range is 2-10.
                                                Note: The VSWR shutdown setpoint should be set greater than the Shutdown
                                                set point. */
   fngForwardPower                  =  1601, /* For radios equipped with Forward Power measurement circuitry, this
                                                command retrieves the last cached Forward Power value measured. */
   fngReflectedPower                =  1602, /* For radios equipped with Reflected Power measurement circuitry, this
                                                command retrieves the last cached Reflected Power value measured. */
   fngForwardPowerSet               =  1603, /* The Forward Power set point sets the desired forward power of the FNG.
                                                The valid range of this set point is 28 to 36 dBm.  The default value
                                                for the FNG forward power is 32 dBm */
   fngFowardPowerLowSet             =  1604, /* Allows the network operator to set/read the Forward Power Low
                                                notification set point. A Forward Power Low Notification (Warning) will
                                                be sent to the Headend once this setpoint has been reached, allowing the
                                                network operator to take action at the site that is experiencing low
                                                power while transmitting. Valid set point range is 27-35.  Note: The
                                                Forward Power Low notification setpoint should be at least 1 dB lower
                                                than the desired forward power setting. */
   macLinkParametersPeriod          =  1611, /* The period, in seconds, for which the MAC Link Parameters command broadcast
                                                schedule is computed.  A value of zero will prevent the DCU from automatically
                                                transmitting the MAC Link Parameters command. Range: 0 - 86,400
                                                Possible values 0, 900, 1800, 3600, 7200, 10800, 14400, 21600, 28800, 43200, 86400.*/
   macLinkParametersStart           =  1612, /* The start time in seconds relative to the DCU system time of midnight UTC for starting
                                                the DCU MAC Link Parameters broadcast schedule. For example, starting the schedule
                                                at 1am (01:00:00) would yield a value of 3,600. Range: 0 -- 86,399*/
   macLinkParametersMaxOffset       =  1613, /* The maximum amount of random offset in milliseconds from macLinkParameterStart
                                                for which the source will begin transmitting the MAC Link Parameters command.
                                                Maximum value = 20,000*/
   receivePowerMargin                 = 1627, /* A node specific margin added to the calculation of receive power thresholds(RPT) that can be
                                                 tuned by network support engineers to optimize performance on a per receiver node resolution.
                                                 This is a DCU or FNG only parameter. 0.5 dB resolution Range 0 to 127 dB */
   macCommandResponseMaxTimeDiversity = 1628, /* When the DCU receives a broadcast MAC command from the air interface, it shall set a
                                                flag so that a time diversity can be used to delay putting the response MAC command message
                                                in the transmit queue. This is to avoid the situation where multiple responses from different
                                                DCUs may collide if time diversity was not applied. A uniform random value between 0 and
                                                macCommandResponseMaxTimeDiversity shall be used to avoid these collisions. Range 0 to 10000 msecs*/
   macReliabilityHighCount            = 1631, /* The number of retries to satisfy the QOS reliability level of high */
   macReliabilityMedCount             = 1632, /* The number of retries to satisfy the QOS reliability level of medium */
   macReliabilityLowCount             = 1633, /* The number of retries to satisfy the QOS reliability level of low */

   macPacketTimeout                   = 1634, /* The number of milliseconds to maintain a given device’s packet ID pair before clearing the information from the MAC. */
   macCsmaQuickAbort                  = 1635, /* Indicates if the CSMA algorithm will abort frame transmission upon the first p-persistence choice to not transmit. */
   macPacketId                        = 1636, /* The ID of the current in-process packet. */
   macChannelSetsCount                = 1637, /* The number of defined channel sets within macChannelSets. */
                                              /* */
   macAckDelayDuration                = 1639, /* The maximum amount of time in milliseconds to wait for an acknowledgement to arrive following a transmitted data
                                                 or command frame where AR was set to TRUE. */
   macChannelsAccessConstrained       = 1640, /* Indicates if the device’s ability to access the communications channel is constrained/limited in some fashion.
                                                 A value of TRUE indicates constrained access and a value of FALSE indicates no constraint on channel access. */
   macIsFNG                           = 1641, /* Indicates if this device serves the role of an FNG (Field Network Gateway */
   macIsIAG                           = 1642, /* Indicates if this device serves the role of an IAG (Intre-Access Network Gateway) */
   macIsRouter                        = 1643, /* Indicates if this device serves the role of an router */
   macState                           = 1644, /* The current state of the MAC layer. */
   comDeviceMicroMPN                  = 1659, /* Microcontroller PartNumber. Need to send the partnumber over the air*/
// macTxFrames                        = 9996, /* Check if needed */

   edPwrRelSustainedIntrpMin          = 3600, /* bulkQuantity electricitySecondaryMetered ieee1366SustainedInterruption (min)*/
   edPwrRelSustainedIntrpCnt          = 3601, /* bulkQuantity electricitySecondaryMetered ieee1366SustainedInterruption (count)*/
   edPwrRelMomentaryIntrpCnt          = 3602, /* bulkQuantity electricitySecondaryMetered ieee1366MomentaryInterruption (count)*/
   edPwrRelMomentaryIntrpEventCnt     = 3603  /* bulkQuantity electricitySecondaryMetered ieee1366MomentaryInterruptionEvent (count)*/

} meterReadingType;

/* From HEEP document - Enumerated Readings Value Typecast */
typedef enum
{
   ASCIIStringValue  = 0,  /* A one-byte per character extended ASCII representation */
   Boolean           = 1,  /* The least significant bit is '0' to represent 'false', and '1' to represent 'true'. */
   dateTimeValue     = 2,  /* A date and time expressed as the number of seconds since 1970-01-01T00:00:00 UTC in the
                              Zulu timezone. */
   dateValue         = 3,  /* A date expressed as the number of seconds since 1/1/1970. The useful result of the
                              conversion is the year, month, and day. Any hours, minutes, or seconds implied during the
                              conversion is to be ignored. */
   hexBinary         = 4,  /* A value which, when transmitted contains a binary object of the specified size. If the
                              binary object is from an EndDevice, it is unaltered and retains the original Endianness of
                              the device that created it. If the binary object is from an EndPoint, it is in the
                              customary Endian format: Big Endian. */
   intValue          = 5,  /* Signed integer value in which the most significant bit is interpreted as the sign bit. */
   timeValue         = 6,  /* A time of day expressed as the number of seconds since midnight. */
   uintValue         = 7,  /* An unsigned integer value in which the most significant bit is a data bit just like all of
                              the other bits. */
   uint16_list_type  = 8,  /* unsigned int list (16 bits) */
   int32_list_type   = 9   /* signed int list (32 bits) */

} ReadingsValueTypecast;

/* From HEEP document - Enumerated End Device Event/Control Types, Y84000-41-DSD-RevC-EndDeviceEventIDs20200818.xlsx */
typedef enum
{
   invalidEventType                                                  = 0,
   electricMeterError                                                = 1,
   electricMeterErrorCleared                                         = 2,
   electricMeterSelfTestError                                        = 3,
   electricMeterSelfTestErrorCleared                                 = 4,
   electricMeterProcessorError                                       = 5,
   electricMeterProcessorErrorCleared                                = 6,
   electricMeterFirmwareStatusReplaced                               = 7,
   electricMeterFirmwareStatusNormal                                 = 8,
   electricMeterSecurityCoverRemoved                                 = 9,
   electricMeterSecurityCoverReplaced                                = 10,
   electricMeterSecurityRotationReversed                             = 11,
   electricMeterSecurityRotationNormal                               = 12,
   electricMeterMemoryDataError                                      = 13,
   electricMeterMemoryDataErrorCleared                               = 14,
   electricMeterMemoryNVramFailed                                    = 15,
   electricMeterMemoryNVramSucceeded                                 = 16,
   electricMeterMemoryProgramError                                   = 17,
   electricMeterMemoryProgramErrorCleared                            = 18,
   electricMeterMemoryRAMFailed                                      = 19,
   electricMeterMemoryRAMSucceeded                                   = 20,
   electricMeterMemoryROMFailed                                      = 21,
   electricMeterMemoryROMSucceeded                                   = 22,
   electricMeterBatteryChargeMinLimitReached                         = 23,
   electricMeterBatteryChargeMinLimitCleared                         = 24,
   electricMeterMetrologyCalibrationChanged                          = 25,
   electricMeterMetrologyCalibrationNormal                           = 26,
   electricMeterMetrologyMeasurementError                            = 27,
   electricMeterMetrologyMeasurementErrorCleared                     = 28,
   electricMeterPowerFailed                                          = 29,
   electricMeterPowerRestored                                        = 30,
   electricMeterPowerPhaseAngleOutOfRange                            = 31,
   electricMeterPowerPhaseAngleOutOfRangeCleared                     = 32,
   electricMeterPowerPhaseAVoltageSagStarted                         = 33,
   electricMeterPowerPhaseAVoltageSagStopped                         = 34,
   electricMeterPowerPhaseAVoltageSwellStarted                       = 35,
   electricMeterPowerPhaseAVoltageSwellStopped                       = 36,
   electricMeterPowerNeutralCurrentMaxLimitReached                   = 37,
   electricMeterPowerNeutralCurrentMaxLimitCleared                   = 38,
   electricMeterPowerPhaseInactive                                   = 39,
   electricMeterPowerPhaseInactiveCleared                            = 40,
   electricMeterPowerPhaseCrossPhaseDetected                         = 41,
   electricMeterPowerPhaseCrossPhaseCleared                          = 42,
   electricMeterPowerPowerQualityHighDistortion                      = 43,
   electricmeterPowerPowerQualityHighDistortionCleared               = 44,
   electricMeterPowerReactivePowerReversed                           = 45,
   electricMeterPowerReactivePowerNormal                             = 46,
   electricMeterPowerVoltageMinLimitReached                          = 47,
   electricMeterPowerVoltageMinLimitCleared                          = 48,
   electricMeterPowerVoltageImbalanced                               = 49,
   electricMeterPowerVoltageImbalanceCleared                         = 50,
   electricMeterRcdSwitchSwitchPostionChanged                        = 51,
   electricMeterRcdSwitchArmedForClosure                             = 52,
   electricMeterRcdSwitchConnected                                   = 53,
   electricMeterRcdSwitchDisconnected                                = 54,
   electricMeterClockError                                           = 55,
   electricMeterClockErrorCleared                                    = 56,
   electricMeterConfigurationChanged                                 = 57,
   electricMeterConfigurationNormal                                  = 58,
   electricMeterConfigurationError                                   = 59,
   electricMeterConfigurationErrorCleared                            = 60,
   electricMeterConfigurationIdentityChanged                         = 61,
   electricMeterConfigurationIdentityNormal                          = 62,
   electricMeterConfigurationParameterChanged                        = 63,
   electricMeterConfigurationParameterNormal                         = 64,
   electricMeterConfigurationProgramChanged                          = 65,
   electricMeterConfigurationProgramNormal                           = 66,
   electricMeterConfigurationProgramLossDetected                     = 67,
   electricMeterConfigurationProgramReestablished                    = 68,
   electricMeterConfigurationProgramUninitialized                    = 69,
   electricMeterConfigurationProgramInitialized                      = 70,
   electricMeterDemandOverflow                                       = 71,
   electricMeterDemandNormal                                         = 72,
   electricMeterDemandResetFailed                                    = 73,
   electricMeterDemandResetOccurred                                  = 74,
   collectorInstallationautoRegistrationSucceeded                    = 75,
   collectorInstallationautoRegistrationFailed                       = 76,
   comDeviceSelfTestEnabled                                          = 77,
   comDeviceSelfTestDisabled                                         = 78,
   comDeviceCommunicationTransceiverFailed                           = 79,
   comDeviceCommunicationTransceiverSucceeded                        = 80,
   comDeviceCommunicationNVramError                                  = 81,
   comDeviceCommunicationNVramErrorCleared                           = 82,
   comDeviceMetrologyIOerror                                         = 83,
   comDeviceMetrologyIOerrorCleared                                  = 84,
   comDeviceMetrologyIOfailed                                        = 85,
   comDeviceMetrologyIOsucceeded                                     = 86,
   comDeviceFirmwareStatusReplaced                                   = 87,
   comDeviceFirmwareStatusNormal                                     = 88,
   comDeviceFirmwareProgramError                                     = 89,
   comDeviceFirmwareProgramErrorCleared                              = 90,
   comDeviceSecurityAccessNotAuthorized                              = 91,
   comDeviceSecurityAccessAccessed                                   = 92,
   comDeviceSecurityPasswordInvalid                                  = 93,
   comDeviceSecurityPasswordConfirmed                                = 94,
   comDeviceSecurityPasswordChanged                                  = 95,
   comDeviceSecurityPasswordNormal                                   = 96,
   comDeviceMemoryDataCorrupted                                      = 97,
   comDeviceMemoryDataCorruptionCleared                              = 98,
   comDeviceMemoryDataFailed                                         = 99,
   comDeviceMemoryDataSucceeded                                      = 100,
   comDeviceMemoryNVramCorrupted                                     = 101,
   comDeviceMemoryNVramCorruptionCleared                             = 102,
   comDeviceMemoryNVramFailed                                        = 103,
   comDeviceMemoryNVramSucceeded                                     = 104,
   comDeviceMemoryProgramCorrupted                                   = 105,
   comDeviceMemoryProgramCorruptionCleared                           = 106,
   comDeviceMemoryProgramFailed                                      = 107,
   comDeviceMemoryProgramSucceeded                                   = 108,
   comDeviceMemoryRAMFailed                                          = 109,
   comDeviceMemoryRAMSucceeded                                       = 110,
   comDeviceMemoryROMFailed                                          = 111,
   comDeviceMemoryROMSucceeded                                       = 112,
   comDeviceMetrologyTimeVarianceExceeded                            = 113,
   comDeviceMetrologyTimeVarianceNormal                              = 114,
   comDeviceMetrologydateChanged                                     = 115,
   comDeviceMetrologydateNormal                                      = 116,
   comDeviceMetrologySeasonChangePending                             = 117,
   comDeviceMetrologySeasonChanged                                   = 118,
   comDevicePowerFailed                                              = 119,
   comDevicePowerRestored                                            = 120,
   comDevicePowerPhaseInactive                                       = 121,
   comDevicePowerPhaseactive                                         = 122,
   comDeviceTemperatureThresholdMaxLimitReached                      = 123,
   comDeviceTemperatureThresholdMaxLimitCleared                      = 124,
   comDeviceClockStorageError                                        = 125,
   comDeviceClockStorageErrorCleared                                 = 126,
   comDeviceClockTimeChanged                                         = 127,
   comDeviceClockTimeNormal                                          = 128,
   comDeviceClockTimeUnsecure                                        = 129,
   comDeviceClockTimeReestablished                                   = 130,
   comDeviceClockdateChanged                                         = 131,
   comDeviceClockdateNormal                                          = 132,
   comDeviceClockIOFailed                                            = 133,
   comDeviceClockIOSucceeded                                         = 134,
   comDeviceassociatedDeviceIOError                                  = 135,
   comDeviceassociatedDeviceIOErrorCleared                           = 136,
   comDeviceInstallationAutoRegistrationSucceeded                    = 137,
   comDeviceInstallationAutoRegistrationFailed                       = 138,
   comDeviceDemandResetOccurred                                      = 139,
   comDeviceDemandResetFailed                                        = 140,
   electricMeterDemandCalculationChanged                             = 141,
   comDeviceFirmwareStatusReadyForActivation                         = 142,
   electricMeterRCDSwitchArmForClosure                               = 143,
   electricMeterRCDSwitchArmForOpen                                  = 144,
   electricMeterRCDSwitchConnect                                     = 145,
   electricMeterRCDSwitchDisconnect                                  = 146,
   electricMeterDemandDemandReset                                    = 147,
   electricMeterRCDSwitchConnectFailed                               = 148,
   electricMeterRCDSwitchDisconnected                                = 149,
   electricMeterRCDSwitchDisconnectFailed                            = 150,
   comDeviceError                                                    = 151,
   comDeviceFirmwareStatusChangePending                              = 152,
   comDeviceSecurityTestFailed                                       = 153,
   electricMeterPowerVoltageLossDetected                             = 154,
   electricMeterPowerVoltageNormal                                   = 155,
   electricMeterSecurityAccessNotAuthorized                          = 156,
   electricMeterSecurityAccessSucceeded                              = 157,
   electricMeterMetrologyCalibrationReset                            = 158,
   comDeviceSecurityDecryptionFailed                                 = 159,
   comDeviceSecuritySessionFailed                                    = 160,
   comDeviceSecuritySessionSucceeded                                 = 161,
   electricMeterMetrologyReadingsResetOccurred                       = 162,
   comDeviceFirmwareDecryptionFailed                                 = 163,
   comDeviceFirmwareSignatureFailed                                  = 164,
   electricMeterPowerPhaseBVoltageSagStarted                         = 165,
   electricMeterPowerPhaseBVoltageSagStopped                         = 166,
   electricMeterPowerPhaseCVoltageSagStarted                         = 167,
   electricMeterPowerPhaseCVoltageSagStopped                         = 168,
   electricMeterPowerPhaseBVoltageSwellStarted                       = 169,
   electricMeterPowerPhaseBVoltageSwellStopped                       = 170,
   electricMeterPowerPhaseCVoltageSwellStarted                       = 171,
   electricMeterPowerPhaseCVoltageSwellStopped                       = 172,
   electricMeterPowerPowerQualityExceeded                            = 173,
   comDeviceConfigurationCorrupted                                   = 174,
   comDeviceConfigurationCorruptionCleared                           = 175,
   comDeviceConfigurationMismatched                                  = 176,
   comDevicePowerPowerQualityEventStarted                            = 177,
   comDevicePowerPowerQualityEventStopped                            = 178,
   electricMeterPowerCurrentMaxLimitReached                          = 179,
   comDeviceTemperatureThresholdMinLimitReached                      = 180,
   comDeviceTemperatureThresholdMinLimitCleared                      = 181,
   electricMeterTemperatureThresholdMaxLimitReached                  = 182,
   electricMeterTemperatureThresholdMaxLimitCleared                  = 183,
   comDeviceSecurityMagneticSwitchTamperDetected                     = 184,
   comDeviceSecurityMagneticSwitchTamperCleared                      = 185,
   comDeviceSecurityParentDeviceRemoved                              = 186,
   comDeviceSecurityParentDeviceReplaced                             = 187,
   comDeviceSecurityTilted                                           = 188,
   comDeviceSecurityTamperCleared                                    = 189,
   electricMeterPowerCurrentMaxLimitCleared                          = 190,
   comDevicepowertestfailed                                          = 191,
   comDevicePowertestrestored                                        = 192,
   electricMeterclocktimechanged                                     = 193,
   electricMetersecurityaccessuploaded                               = 194,
   electricMetersecurityaccesswritten                                = 195,
   electricMetermetrologyselfReadsucceeded                           = 196,
   electricMetermetrologyTestModestarted                             = 197,
   electricMetermetrologyTestModestopped                             = 198,
   electricMeterbatterychargefailed                                  = 199,
   electricMeterbatterychargerestored                                = 200,
   electricMeterbillingRTPstarted                                    = 201,
   electricMeterbillingRTPstopped                                    = 202,
   electricMeterConfigurationCalibrationstarted                      = 203,
   electricMeterpowervoltagefrozen                                   = 204,
   electricMeterpowervoltagereleased                                 = 205,
   electricMeterloadControlEmergencySupplyCapacityLimitEventStarted  = 206,
   electricMeterloadControlEmergencySupplyCapacityLimitEventStopped  = 207,
   electricMeterloadControlSupplyCapacityLimitEventStarted           = 208,
   electricMeterloadControlSupplyCapacityLimitEventStopped           = 209,
   electricMeterdemandthresholdmaxLimitReached                       = 210,
   electricMeterdemandthresholdmaxLimitReachedCleared                = 211,
   comDeviceConfigurationError                                       = 212,
   comDeviceConfigurationErrorCleared                                = 213,
   electricMeterBillingRTPActivated                                  = 214,
   electricMeterBillingRTPDeactivated                                = 215,
   electricMeterSecurityPasswordInvalid                              = 216,
   electricMeterSecurityPasswordUnlocked                             = 217,
   electricMeterIODisabled                                           = 218,
   electricMeterIOEnabled                                            = 219,
   electricMeterPowerCurrentImbalanced                               = 220,
   electricMeterPowerCurrentImbalanceCleared                         = 221,
   electricMeterPowerError                                           = 222,
   electricMeterPowerErrorCleared                                    = 223,
   electricMeterSecurityMagneticSwitchTamperDetected                 = 224,
   electricMeterSecurityMagneticSwitchTamperCleared                  = 225,
   electricMeterSecurityTilted                                       = 226,
   electricMeterSecurityTamperCleared                                = 227,
   electricMeterLoadControlStatusDisconnected                        = 228,
   electricMeterPowerStatusDisconnected                              = 229,
   electricMeterRCDSwitchSupplyCapacityLimitDisconnected             = 230,
   electricMeterRCDSwitchVoltageCharged                              = 231,
   electricMeterRCDSwitchVoltageCleared                              = 232,
   electricMeterRCDSwitchVoltageTamperDetected                       = 233,
   electricMeterRCDSwitchVoltageTamperCleared                        = 234,
   electricMeterRCDSwitchEmergencySupplyCapacityLimitDisconnected    = 235,
   electricMeterRCDSwitchPrepaymentCreditExpired                     = 236,
   electricMeterRCDSwitchInProgress                                  = 237,
   electricMeterFirmwareWriteFailed                                  = 238,
// comDevicePowerphaseAngleCalculated                                = 239,  /*  This alarm indicates that a survey has occurred. */
   comDeviceFirmwareInitializationError                              = 240,  /*  The EndPoint did not power up correctly. One or more initialization errors were detected. */
   comDeviceFirmwareInitializationSucceeded                          = 241,  /*  The EndPoint has recovered from an initialization error. */
   comDevicePowerPhaseAngleDistorted                                 = 242,  /*  A questionable phase angle was calculated at the time of the survey due to high PF or harmonic distortion. */
   comDevicePowerPhaseAngleHighDistortion                            = 243,  /*  A useful phase angle was not calculable at the time of the survey due to high PF or harmonic distortion. */
   comDevicePowerPowerQualityTestEventStarted                        = 244,  /*  This is a “test” version of alarm #177 */
   comDevicePowerPowerQualityTestEventStopped                        = 245,  /*  This is a “test” version of alarm #178 */
   electricMeterRcdSwitchDataLogConnected                            = 247,
   electricMeterRcdSwitchDataLogDisconnected                         = 248,
   electricMeterPowerPhaseBVoltageCrossPhaseDetected                 = 249,
   electricMeterPowerPhaseCVoltageCrossPhaseDetected                 = 250,
   electricMeterPowerPhaseACurrentCrossPhaseDetected                 = 251,
   electricMeterPowerPhaseBCurrentCrossPhaseDetected                 = 252,
   electricMeterPowerPhaseCCurrentCrossPhaseDetected                 = 253,
   electricMeterPowerPhaseBVoltageImbalanced                         = 254,
   electricMeterPowerPhaseCVoltageImbalanced                         = 255,
   electricMeterPowerPhaseACurrentInactive                           = 256,
   electricMeterPowerPhaseBCurrentInactive                           = 257,
   electricMeterPowerPhaseCCurrentInactive                           = 258,
   electricMeterPowerPhaseAngleAOutofRange                           = 259,
   electricMeterPowerPhaseAngleBOutofRange                           = 260,
   electricMeterPowerPhaseAngleCOutofRange                           = 261,
   electricMeterPowerPhaseAHighDistortion                            = 262,
   electricMeterPowerPhaseBHighDistortion                            = 263,
   electricMeterPowerPhaseCHighDistortion                            = 264,
   electricMeterPowerPhaseAVoltageMinLimitReached                    = 265,
   electricMeterPowerPhaseAVoltageMinLimitCleared                    = 266,
   electricMeterPowerPhaseBVoltageMinLimitReached                    = 267,
   electricMeterPowerPhaseBVoltageMinLimitCleared                    = 268,
   electricMeterPowerPhaseCVoltageMinLimitReached                    = 269,
   electricMeterPowerPhaseCVoltageMinLimitCleared                    = 270,
   electricMeterSecurityPhaseAVoltageLossDetected                    = 271,
   electricMeterSecurityPhaseBVoltageLossDetected                    = 272,
   electricMeterSecurityPhaseCVoltageLossDetected                    = 273,
   electricMeterSecurityVoltageRestored                              = 274,
   loadControlDeviceRCDSwitchEventLogDisconnected                    = 275,
   electricMeterRCDSwitchSupplyCapacityLimitConnected                = 276,
   loadControlDeviceRCDSwitchEventLogConnected                       = 277,
   electricMeterPowerPhaseACurrentLimitReached                       = 278,
   electricMeterPowerPhaseBCurrentLimitReached                       = 279,
   electricMeterPowerPhaseCCurrentLimitReached                       = 280,
   electricMeterDemandResetDatalogOccurred                           = 310,
   luminaireRcdSwitchTimeInvalid                                     = 311,
   luminaireRcdSwitchOpened                                          = 312,
   luminaireRcdSwitchClosed                                          = 313,
   comDeviceBatterChargeFailed                                       = 314,
   comDeviceEventLogCorrupted                                        = 315,
   comDeviceEventLogInitialized                                      = 316,
   collectorbatterycurrentmaxLimitReached                            = 2000,
   collectorbatterycurrentminLimitReached                            = 2001,
   collectorbatterycurrentinactive                                   = 2002,
   collectorbatteryNVRAMfailed                                       = 2003,
   collectorSecurityDoorclosed                                       = 2004,
   collectorSecurityDooraccessed                                     = 2005,
   collectorSecurityDoorexceeded                                     = 2006,
   collectorCommunicationHeadEndSystemfailed                         = 2007,
   collectorCommunicationqueuefailed                                 = 2008,
   collectorCommunicationfailed                                      = 2009,
   collectorSecurityCRCerror                                         = 2010,
   collectorFirmwareStatusreset                                      = 2011,
   collectorNetworkRadiounstable                                     = 2012,
   collectorVideoDisplayStatusaccessed                               = 2013,
   collectorSecurityEnclosureClosed                                  = 2014,
   collectorSecurityEnclosureOpened                                  = 2015,
   collectorCommunicationRadioMismatched                             = 2016,  /* VSWR Notification Limit Reached. (Indicates an impedence mismatch with the antenna was detected) */
   collectorCommunicationRadioFailed                                 = 2017,  /* VSWR max limit reached and the Field Network Gateway is not transmitting. The impedence mismatch
                                                                                 initially reported as event #2016 is now so severe that the transmitter has shut down. */
   collectorCommunicationRadioLossDetected                           = 2018,  /* Forward power min limit was reached. DCU outbound (downlink) transmitter operating at reduced power. */
   CollectorCommunicationRadioRestored                               = 2019,  /* Forward power now above minimum threshold  */
   collectorCommunicationRadioLossRestored                           = 2020,  /* VSWR limit condition cleared  */

   LastEndDeviceEventControlType                                     = 0xFFFF /* Forces this enumeration to be 16 bits */
} EDEventType;

PACK_BEGIN
typedef struct PACK_MID
{ /*For the Application Header bit structure */
   uint8_t interfaceRevision;
   struct
   {
     uint8_t resource        : 6;
     uint8_t transactionType : 2;
   };
   uint8_t methodStatus;
   uint8_t requestResponseId;
}AppHdrBitStructure_t, *pAppHdrBitStructure_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{  /*For the CompactMeterRead bit structure */
   uint8_t  rdgValSize     : 3;  /* Reading value size in bytes */
   uint8_t  pendPowerof10  : 3;  /* enumerated power of 10 */
   uint8_t  rdgQualPres    : 1;  /* Reading quality codes present */
   uint8_t  tsPresent      : 1;  /* Time stamp present */
}CMR_readingInfo_t;
PACK_END

PACK_BEGIN
typedef union ewIdRdgInfo_u PACK_MID
{  /*For the ExchangeWithID bitstructure */
   struct ewIdrdgInfoBits
   {
      uint8_t  pendPowerof10  : 3;  /* enumerated power of 10 */
      uint8_t  rsvd2          : 1;
      uint8_t  use            : 1;  /* Use 1=ID, 0 = write */
      uint8_t  RdgQualPres    : 1;  /* Reading quality codes present */
      uint8_t  tsPresent      : 1;  /* Time stamp present */
      uint8_t  isCoinTrig     : 1;  /* Is coincident trigger flag */
      uint16_t RdgValSizeU    : 8;  /* Reading value size in bytes (upper 8 bits of 12) */
      uint16_t typecast       : 4;  /* enumerated typecast */
      uint16_t RdgValSize     : 4;  /* Reading value size (lowest 4 bits of 12) */
   }bits;
   uint8_t bytes[3];
}ExWId_ReadingInfo_u;
PACK_END

PACK_BEGIN
typedef struct fmrRdgValInfo PACK_MID
{  /*For the FullMeterRead bit structure */
   uint8_t  pendPowerof10  : 3;  /* enumerated power of 10 */
   uint8_t  rsvd2          : 2;
   uint8_t  RdgQualPres    : 1;  /* Reading quality codes present */
   uint8_t  tsPresent      : 1;  /* Time stamp present */
   uint8_t  isCoinTrig     : 1;  /* Is coincident trigger flag */

   uint16_t RdgValSizeU    : 8;  /* Reading value size in bytes (upper 8 bits of 12) */
#if OTA_CHANNELS_ENABLED == 0
   // todo: 3/18/2016 4:39:51 PM [BAH] - Should be able to remove this
   uint16_t typecast       : 3;  /* enumerated typecast */
   uint16_t rsvd1          : 1;
#else
   uint16_t typecast       : 4;  /* enumerated typecast */
#endif
   uint16_t RdgValSize     : 4;  /* Reading value size (lowest 4 bits of 12) */

   uint16_t rdgType;             /* enumerated meterReadingType */
} fmrRdgValInfo_t;
PACK_END

PACK_BEGIN
struct coincRdgValInfo_t PACK_MID
{  /*For the GetCoincedentReads bit structure */
   uint8_t  rsvd           : 7;
   uint8_t  isCoinTrig     : 1;  /* Is coincident trigger flag */

   uint16_t rdgType;             /* enumerated meterReadingType */
};
PACK_END

PACK_BEGIN
union nvpQtyDetValSz_u PACK_MID
{
   struct nvpQtyDetValSzBits
   {
      uint8_t  nvpQty   : 4;     /* Reading Qty */
      uint8_t  DetValSz : 4;     /* Event Qty */
   } bits;
   uint8_t nvpQtyDetValSz;      /* 4 bits for each */
};
PACK_END

PACK_BEGIN
typedef union EvRdgQty PACK_MID
{
   struct EventRdqQtyBits
   {
      uint8_t  rdgQty   : 4;     /* Reading Qty */
      uint8_t  eventQty : 4;     /* Event Qty */
   } bits;
   uint8_t     EventRdgQty;      /* 4 bits for each */
} EventRdgQty_u;
PACK_END

PACK_BEGIN
typedef union TimeSchRdgQty_u PACK_MID
{
   struct TimeSchRdgBits
   {
      uint8_t  rdgQty       : 4;       /* Reading Qty */
      uint8_t  timeSchQty   : 4;       /* Time Schedule Qty */
   } bits;
   uint8_t        TimeSchRdgQty;       /* 4 bits for each */
} TimeSchRdgQty_t, *pTimeSchRdgQty_t;
PACK_END

PACK_BEGIN
typedef struct timeSheduleQty PACK_MID
{
   uint32_t    startTime;              /* time in seconds from epoch */
   uint32_t    endTime;                /* time in seconds from epoch */
} timeSheduleQty_t , *pTimeSheduleQty_t ;
PACK_END

PACK_BEGIN
typedef struct eventHdr PACK_MID
{
   uint32_t       intervalTime;  /* Interval time associated with requested qty */
   EventRdgQty_u  eventRdgQty;   /* Event/Reading quantity  */
} eventHdr_t  , *peventHdr_t  ;
PACK_END

/* GetAlarms structure */
PACK_BEGIN
typedef struct PACK_MID
{
   EventRdgQty_u  TimeSchedQty;  /* Time Schedule Qty and EDEvent Qty   */
   timeRange_t    time;          /* List of time start/end ranges */
   EDEventType    eventID;       /* List of event IDs requested   */
} getAlarms_t, *pGetAlarms_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   EventRdgQty_u     eventRdgQty;      /* End Device event qty 4 bits
                                          Reading          qty 4 bits */
   EDEventType       eventType;        /* Requested info/action (outbound); event description (inbound) */
   uint8_t           nvp;              /* Name/value pair info */
   uint16_t          EDDetailsName;    /* End Device Event Details Name */
   uint8_t           EDDetailsValue;   /* Variable length! */
   uint8_t           CoincidentInfo;   /* Coincident qty information */
   meterReadingType  readingType;      /* enumerated reading type */
   uint8_t           value;            /* Reading value */

} ExchangeWithID, *pExchangeWithID;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   fmrRdgValInfo_t   rdgValInfo;       /* Readings value size in bits 12 bits
                                          rsvd                         1 bit
                                          readings value typecase      3 bits */
   uint32_t          timeStamp;        /* Only present if bit set in eventInfo */
   uint8_t           ReadingQuality;   /* Reading quality code, if any, plus msb set if more qual codes */
   uint8_t           ReadingValue[CIM_APP_MSG_READING_VALUE_SIZE];  /* Actual reading */
} readingQty_t;
PACK_END

PACK_BEGIN
typedef struct NVP PACK_MID
{
   uint16_t       nvpName;       /* End Device Event Details Name */
   int8_t         nvpValue[15];  /* End Device Event Details Value */
} nvp_t , *pNvp_t ;
PACK_END

PACK_BEGIN
typedef struct edEventQty PACK_MID
{
   uint32_t       createdTime;   /* time in seconds from epoch */
   EDEventType    eventType;     /* Requested info/action (outbound); event description (inbound) */
} EDEventQty_t, *pEDEventQty_t ;
PACK_END

/* Full Meter read (response) structure */
PACK_BEGIN
typedef struct PACK_MID
{
   eventHdr_t     eventHeader;   /* Interval time associated and event/reading qty */
   uint32_t       createdTime;   /* time in seconds from epoch */
   EDEventType    eventType;     /* Requested info/action (outbound); event description (inbound) */
   uint8_t        nvp;           /* Name/value pair information   */
   readingQty_t   readingQty;
} FullMeterRead, *pFullMeterRead;
PACK_END

/* Get Meter read structure */
PACK_BEGIN
typedef struct PACK_MID
{
   TimeSchRdgQty_t            SchedQty;         /* Time schedule qty & Reading qty */
   timeSheduleQty_t           timeSchedule;     /* Start/end times */
   meterReadingType           readingType;      /* enumerated reading type (16 bits) */
} GetMeterRead, *pGetMeterRead;
PACK_END

/*!
   Generic file access information
   Each module that manages OTA parameters has an instance of this for its paramaters.
*/
typedef void (*fptr_notify)( void );
PACK_BEGIN
typedef struct PACK_MID
{
   meterReadingType  elementID;        /*!< HEEP enumeration for value */
   uint16_t          vOffset;          /*!< Offset into file for elementID */
   uint8_t           vSize;            /*!< Size of value in file */
   fptr_notify       notify;           /* Notify routine, if any */
} HEEP_FileInfo_t;
PACK_END

/* Get Log Entries bit structure - used by the /or/ha resource to gather event history */
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t startIndexId;    /* alarmIndex value to start search */
   uint8_t endIndexId;      /* alarmIndex value to end search   */
   uint8_t severity;        /* alarm severity for search, which buffer to read from */
} GetLogEntries_t;
PACK_END

#ifdef __cplusplus
}
#endif
#endif /* HEEP_H */
