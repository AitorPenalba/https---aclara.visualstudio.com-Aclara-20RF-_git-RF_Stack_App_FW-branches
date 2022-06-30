/***********************************************************************************************************************
 *
 * Filename: HEEP_UTIL.c
 *
 * Global Designator: HEEP_
 *
 * Contents: General HEEP message retrieval/storage functions
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************
 *
 * $Log$ cmusial Created May 27, 2015
 *
 **********************************************************************************************************************/

/* INCLUDE FILES */
#include "math.h"
#include "project.h"
#include "buffer.h"
#include "byteswap.h"
#include "pack.h"
#include "STACK.h"
#include "STACK_Protocol.h"
#include "DBG_SerialDebug.h"
#include "MFG_Port.h"
#include "dfwtd_config.h"
#include "dfw_app.h"
#include "Mac.h"
#include "cfg_app.h"
#include "PHY.h"
#include "dvr_intFlash_cfg.h"
#include "Temperature.h"
#if (USE_DTLS == 1)
#include "dtls.h"
#endif
#include "EVL_event_log.h"
#include "ALRM_Handler.h"
#include "time_util.h"
#include "time_DST.h"
#include "MIMT_info.h"
#include "SELF_test.h"
#include "eng_res.h"

/*lint -esym(750,HEEP_UTIL_GLOBALS) */
#define HEEP_UTIL_GLOBALS
#include "HEEP_Util.h"
#undef HEEP_UTIL_GLOBALS

#include "APP_MSG_Handler.h"
#include "time_sync.h"
#include "smtd_config.h"
#include "radio.h"
#if ( EP == 1 )
#include "hmc_eng.h"
#if ( ANSI_SECURITY == 1 )
#include "hmc_start.h"
#endif
#include "time_sys.h"
#include "historyd.h"
#include "demand.h"
#include "pwr_config.h"
#include "ID_intervalTask.h"
#include "demand.h"
#include "OR_MR_Handler.h"
#include "mode_config.h"
#include "ed_config.h"
#include "BSP_aclara.h"
#include "endpt_cim_cmd.h"
#include "pwr_task.h"
#if ( PHASE_DETECTION == 1 )
#include "PhaseDetect.h"
#endif
#else
#include "DCU_cim_cmd.h"
#endif
#if ( USE_DTLS == 1)
#include "dtls.h"
#include "ecc108_apps.h"
#include "ecc108_lib_return_codes.h"
#endif
#if ( USE_MTLS == 1)
#include "mtls.h"
#endif
#if(ACLARA_DA == 1)
#include "da_srfn_reg.h"
#endif
#define TM_HEEP_UNIT_TEST 1

/* TYPE DEFINITIONS */

/* FILE VARIABLE DEFINITIONS */
static const char * const heepTransactionTypes[] =
{
   "Request",
   "Response",
   "Asynchronous",
   "RSVD"
};
static const char * const heepMethods[] =
{
   "get",
   "post",
   "put",
   "delete"
};
static const char * const heepResourceNames[] =
{
   "bu/am",
   "bu/ds",
   "bu/lp",
   "bu/pd",
   "pd",
   "cf/idc",
   "rg/cf",
   "cf/idp",
   "cf/mr",
   "tr",
   "co/st",
   "df/ap",
   "df/ca",
   "df/co",
   "df/dp",
   "df/in",
   "df/vr",
   "dr",
   "lc",
   "mm",
   "mm/c",
   "mm/re",
   "or/am",
   "or/ha",
   "or/ds",
   "or/lp",
   "rg/md",
   "or/mr",
   "or/mt",
   "or/pm",
   "rc",
   "st",
   "st/cmt",
   "st/cpt",
   "st/x",
   "or/cr",
   "dc/rg/md",
   "bu/en",
   "dc/bu/am",
   "dc/or/pm",
   "tn/tw/mp",
   "tn/st/dc"
};

static uint8_t HEEP_msgID;             /* Value reported in response ID field */
static bool OTAtestenabled_ = false;   /* Value to report whether OTA Testing is enabled allowing OR_PM frequency changes */

/* MACRO DEFINITIONS */

/* CONSTANTS */
/*lint -esym(750,OR_PM_MAX_READINGS) not referenced */
#define OR_PM_MAX_READINGS        (1) //Only security enable/disable is supported

#define HEEP_APP_INTERFACE_REVISION  (0)    //Supported interface revision

/* Application header Transaction Type/Rsource bit definitions */
#define HEEP_APPHDR_TT_MASK            ((uint8_t)3)
#define HEEP_APPHDR_TT_SHIFT           ((uint8_t)6)

#define HEEP_APPHDR_RESOURCE_MASK      ((uint8_t)0x3f)
#define HEEP_APPHDR_RESOURCE_SHIFT     ((uint8_t)0)

#define MAX_OR_PM_WRITE_SIZE  ( 256 )

#define CMSA_PVALUE_POWER_TEN ((uint8_t)2) // power of ten adjustment for macCmsaPValue parameter

// the minimum size in bytes of an or_pm payload, used to validate against a malformed payload condition
#define MIN_PAYLOAD_ORPM_WRITE_BYTES  (EVENT_AND_READING_QTY_SIZE + READING_INFO_SIZE + READING_TYPE_SIZE + sizeof(uint8_t))


typedef returnStatus_t (* paramHandler )( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );

typedef struct
{
  paramHandler          pHandler;
  meterReadingType      rType;
} OR_PM_HandlersDef;

static const OR_PM_HandlersDef OR_PM_Handlers[] =
{
  // OR_PM for all devices:
  { APP_MSG_SecurityHandler,   appSecurityAuthMode },
  { DFWA_OR_PM_Handler,        dfwStatus },
  { DFWA_OR_PM_Handler,        dfwStreamID },
  { DFWA_OR_PM_Handler,        dfwFileType },
  { DFWA_OR_PM_Handler,        capableOfEpPatchDFW },
  { DFWA_OR_PM_Handler,        capableOfEpBootloaderDFW },
  { DFWA_OR_PM_Handler,        capableOfMeterBasecodeDFW },
  { DFWA_OR_PM_Handler,        capableOfMeterPatchDFW },
  { DFWA_OR_PM_Handler,        capableOfMeterReprogrammingOTA },
  { PHY_OR_PM_Handler,         phyFrontEndGain },
  { PHY_OR_PM_Handler,         phyNumChannels },
  { PHY_OR_PM_Handler,         phyAvailableChannels },
  { PHY_OR_PM_Handler,         phyTxChannels },
  { PHY_OR_PM_Handler,         phyRxChannels },
  { PHY_OR_PM_Handler,         phySyncDetectCount },
  { PHY_OR_PM_Handler,         phyAvailableFrequencies },
  { PHY_OR_PM_Handler,         phyTxFrequencies },
  { PHY_OR_PM_Handler,         phyRxFrequencies },
  { PHY_OR_PM_Handler,         phyCcaThreshold},
  { PHY_OR_PM_Handler,         phyRcvrCount },
  { PHY_OR_PM_Handler,         phyRxDetection },
  { PHY_OR_PM_Handler,         phyRxFraming },
  { PHY_OR_PM_Handler,         phyRxMode },
  { PHY_OR_PM_Handler,         phyCcaAdaptiveThresholdEnable },
  { PHY_OR_PM_Handler,         phyCcaOffset },
  { PHY_OR_PM_Handler,         phyNoiseEstimate },
  { PHY_OR_PM_Handler,         phyNoiseEstimateRate },
  { PHY_OR_PM_Handler,         phyThermalControlEnable },
  { PHY_OR_PM_Handler,         phyThermalProtectionCount },
  { PHY_OR_PM_Handler,         phyThermalProtectionEnable },
  { PHY_OR_PM_Handler,         phyThermalProtectionEngaged },
  { PHY_OR_PM_Handler,         phyFailedFrameDecodeCount },
  { PHY_OR_PM_Handler,         phyFailedHcsCount },
  { PHY_OR_PM_Handler,         phyFramesReceivedCount },
  { PHY_OR_PM_Handler,         phyFramesTransmittedCount },
  { PHY_OR_PM_Handler,         phyFailedHeaderDecodeCount },
  { PHY_OR_PM_Handler,         phySyncDetectCount },
  { PHY_OR_PM_Handler,         phyDemodulator },
  { PHY_OR_PM_Handler,         phyMaxTxPayload },
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
  { PHY_OR_PM_Handler,         fngVSWR },
  { PHY_OR_PM_Handler,         fngVswrNotificationSet },
  { PHY_OR_PM_Handler,         fngVswrShutdownSet },
  { PHY_OR_PM_Handler,         fngForwardPower },
  { PHY_OR_PM_Handler,         fngReflectedPower },
  { PHY_OR_PM_Handler,         fngForwardPowerSet },
  { PHY_OR_PM_Handler,         fngFowardPowerLowSet },
#endif
//{ PHY_OR_PM_Handler,         engData1 }, // 1319 Handled by BU/EN
//{ PHY_OR_PM_Handler,         engData2 }, // 1365 Handled by BU/EN
//{ PHY_OR_PM_Handler,         engData3 }, // 1367 Not defined
#if  ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
  { PHY_OR_PM_Handler,         receivePowerMargin },
#endif
  { EVL_OR_PM_Handler,         opportunisticThreshold },
  { EVL_OR_PM_Handler,         realtimeThreshold },
  { EVL_OR_PM_Handler,         realTimeAlarm },
  { EVL_OR_PM_Handler,         amBuMaxTimeDiversity },
  { EVL_OR_PM_Handler,         opportunisticAlarmIndexID },
  { EVL_OR_PM_Handler,         realTimeAlarmIndexID },
  { SELF_OR_PM_Handler,        stRTCFailCount },
  { SELF_OR_PM_Handler,        stSecurityFailCount },
  { SELF_OR_PM_Handler,        stNvmRWFailCount },
  { TIME_UTIL_OR_PM_Handler,   dateTime },
  { NWK_OR_PM_Handler,         ipHEContext },
  { NWK_OR_PM_Handler,         iPifInDiscards },
  { NWK_OR_PM_Handler,         iPifInErrors },
  { NWK_OR_PM_Handler,         iPifInMulticastPkts },
  { NWK_OR_PM_Handler,         iPifInOctets },
  { NWK_OR_PM_Handler,         iPifInUcastPkts },
  { NWK_OR_PM_Handler,         iPifLastResetTime },
  { NWK_OR_PM_Handler,         invalidAddressMode},
  { NWK_OR_PM_Handler,         iPifOutDiscards },
  { NWK_OR_PM_Handler,         iPifOutErrors },
  { NWK_OR_PM_Handler,         iPifOutMulticastPkts },
  { NWK_OR_PM_Handler,         iPifOutOctets },
  { NWK_OR_PM_Handler,         iPifOutUcastPkts },
  { SMTDCFG_OR_PM_Handler,     engBuEnabled },
  { SMTDCFG_OR_PM_Handler,     engBuTrafficClass },
  { HEEP_util_OR_PM_Handler,   flashSecurityEnabled },
  { HEEP_util_OR_PM_Handler,   debugPortEnabled },
  { MIMTINFO_OR_PM_Handler,    FctModuleTestDate },
  { MIMTINFO_OR_PM_Handler,    FctEnclosureTestDate },
  { MIMTINFO_OR_PM_Handler,    IntegrationSetupDate },
  { MIMTINFO_OR_PM_Handler,    FctModuleProgramVersion },
  { MIMTINFO_OR_PM_Handler,    FctEnclosureProgramVersion },
  { MIMTINFO_OR_PM_Handler,    IntegrationProgramVersion },
  { MIMTINFO_OR_PM_Handler,    FctModuleDatabaseVersion },
  { MIMTINFO_OR_PM_Handler,    FctEnclosureDatabaseVersion },
  { MIMTINFO_OR_PM_Handler,    IntegrationDatabaseVersion },
  { MIMTINFO_OR_PM_Handler,    dataConfigurationDocumentVersion },
  { MIMTINFO_OR_PM_Handler,    manufacturerNumber },
  { MIMTINFO_OR_PM_Handler,    repairInformation },
  { MAC_OR_PM_Handler,         macAckWaitDuration },
  { MAC_OR_PM_Handler,         macCsmaQuickAbort },
  { MAC_OR_PM_Handler,         macPacketId },
  { MAC_OR_PM_Handler,         macPacketTimeout },

  { MAC_OR_PM_Handler,         macAckDelayDuration },
  { MAC_OR_PM_Handler,         macState },
  { MAC_OR_PM_Handler,         macChannelsAccessConstrained},
  { MAC_OR_PM_Handler,         macIsFNG },
  { MAC_OR_PM_Handler,         macIsIAG },
  { MAC_OR_PM_Handler,         macIsRouter },


  { MAC_OR_PM_Handler,         macNetworkId },
  { MAC_OR_PM_Handler,         macRSSI },
  { MAC_OR_PM_Handler,         macChannelSets },
  { MAC_OR_PM_Handler,         macChannelSetsSTAR },
  { MAC_OR_PM_Handler,         macChannelSetsCount },
  { MAC_OR_PM_Handler,         macTransactionTimeout },
  { MAC_OR_PM_Handler,         macTransactionTimeoutCount },
  { MAC_OR_PM_Handler,         macTxLinkDelayCount },
  { MAC_OR_PM_Handler,         macTxLinkDelayTime },
  { MAC_OR_PM_Handler,         macCsmaMaxAttempts },
  { MAC_OR_PM_Handler,         macCsmaMinBackOffTime },
  { MAC_OR_PM_Handler,         macCsmaMaxBackOffTime },
  { MAC_OR_PM_Handler,         macCsmaPValue },
  { MAC_OR_PM_Handler,         macReassemblyTimeout },
  { MAC_OR_PM_Handler,         inboundFrameCount },
  { MAC_OR_PM_Handler,         macPingCount },
  { MAC_OR_PM_Handler,         macReliabilityHighCount },
  { MAC_OR_PM_Handler,         macReliabilityMedCount },
  { MAC_OR_PM_Handler,         macReliabilityLowCount },
#if  ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
  { MAC_OR_PM_Handler,         macLinkParametersMaxOffset },
  { MAC_OR_PM_Handler,         macLinkParametersPeriod },
  { MAC_OR_PM_Handler,         macLinkParametersStart },
  { MAC_OR_PM_Handler,         macCommandResponseMaxTimeDiversity },
#endif
  { TIME_SYS_OR_PM_Handler,    timeRequestMaxTimeout },
  { TIME_SYS_OR_PM_Handler,    dateTimeLostCount },
  { TIME_SYS_OR_PM_Handler,    timeLastUpdated },
  { TIME_SYS_OR_PM_Handler,    rtcDateTime },
  { TIME_SYS_OR_PM_Handler,    timeAcceptanceDelay },
  { TIME_SYS_OR_PM_Handler,    timeState },
  { TIME_SYS_OR_PM_Handler,    installationDateTime },
  { TIME_SYS_OR_PM_Handler,    timeSigOffset},
  { TIME_SYNC_OR_PM_Handler,   timePrecision },
  { TIME_SYNC_OR_PM_Handler,   timeSetMaxOffset },
  { TIME_SYNC_OR_PM_Handler,   timeSetPeriod },
  { TIME_SYNC_OR_PM_Handler,   timeSetStart },
  { TIME_SYNC_OR_PM_Handler,   timeSource },
  { TIME_SYNC_OR_PM_Handler,   timeDefaultAccuracy },
  { TIME_SYNC_OR_PM_Handler,   timeQueryResponseMode },
  { TEMPERATURE_OR_PM_Handler, temperature },
  { TEMPERATURE_OR_PM_Handler, vswr },

#if ( EP == 1 )
  /* Required for all EP's */
  { APP_MSG_initialRegistrationTimeoutHandler,  initialRegistrationTimeout },
  { APP_MSG_maxRegistrationTimeoutHandler,      maxRegistrationTimeout },
  { APP_MSG_minRegistrationTimeoutHandler,      minRegistrationTimeout },
  { DFWTDCFG_OR_PM_Handler,                     dfwApplyConfirmTimeDiversity },
  { DFWTDCFG_OR_PM_Handler,                     dfwDowloadConfirmTimeDiversity },
#if 0 // TODO: RA6E1 Enable once DFW ported
  { DFWA_OR_PM_Handler,                         dfwDupDiscardPacketQty},
#endif
  { DST_timeZoneOffsetHandler,                  timeZoneOffset },
  { DST_dstOffsetHandler,                       dstOffset },
  { DST_dstEnabledHandler,                      dstEnabled },
  { DST_dstStartRuleHandler,                    dstStart },
  { DST_dstEndRuleHandler,                      dstEnd },
  { DST_OR_PM_timeZoneDstHashHander,            timeZoneDstHash },
  { EDCFG_OR_PM_Handler,                        edInfo },
  { EDCFG_OR_PM_Handler,                        edFwVersion },
  { EDCFG_OR_PM_Handler,                        edHwVersion },
  { EDCFG_OR_PM_Handler,                        edModel },
  { EDCFG_OR_PM_Handler,                        edMfgSerialNumber },
  { EDCFG_OR_PM_Handler,                        edUtilitySerialNumber },
  { EDCFG_OR_PM_Handler,                        configGet},
  { EDCFG_OR_PM_Handler,                        avgIndPowerFactor },
  { ENDPT_CIM_CMD_OR_PM_Handler,                newRegistrationRequired },
  { ENDPT_CIM_CMD_OR_PM_Handler,                comDeviceFirmwareVersion },
  { ENDPT_CIM_CMD_OR_PM_Handler,                comDeviceHardwareVersion },
  { ENDPT_CIM_CMD_OR_PM_Handler,                comDeviceType },
  { ENDPT_CIM_CMD_OR_PM_Handler,                comDeviceMACAddress },
#if ( ANSI_SECURITY == 1 )
  { HEEP_util_passordPort0_Handler,             passwordPort0 },
  { HEEP_util_passordPort0Master_Handler,       passwordPort0Master },
#endif
  { MODECFG_decommissionModeHandler,            decommissionMode },
  { PWRCFG_OutageDeclarationDelayHandler,       outageDeclarationDelay },
  { PWRCFG_RestorationDeclarationDelayHandler,  restorationDeclarationDelay },
  { PWRCFG_PowerQualityEventDurationHandler,    powerQualityEventDuration },
  { PWRCFG_LastGaspMaxNumAttemptsHandler,       lastGaspMaxNumAttempts },
#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
  { PWRCFG_SIMLG_OR_PM_Handler,                 simulateLastGaspStart },
  { PWRCFG_SIMLG_OR_PM_Handler,                 simulateLastGaspDuration },
  { PWRCFG_SIMLG_OR_PM_Handler,                 simulateLastGaspTraffic },
  { PWRCFG_SIMLG_OR_PM_Handler,                 simulateLastGaspMaxNumAttempts },
  { PWRCFG_SIMLG_OR_PM_Handler,                 simulateLastGaspStatCcaAttempts },
  { PWRCFG_SIMLG_OR_PM_Handler,                 simulateLastGaspStatPPersistAttempts },
  { PWRCFG_SIMLG_OR_PM_Handler,                 simulateLastGaspStatMsgsSent },
#endif
#if 0 // TODO: RA6E1 OR_PM handlers
  { PWR_OR_PM_Handler,                          watchdogResetCount },
  { PWR_OR_PM_Handler,                          spuriousResetCount },
  { PWR_OR_PM_Handler,                          PowerFailCount },
#endif
  { SMTDCFG_OR_PM_Handler,                      smLogTimeDiversity },
#if 0 // TODO: RA6E1 OR_PM handlers
  { PHY_OR_PM_Handler,                          stTxCWTest },
  { PHY_OR_PM_Handler,                          stRx4GFSK },
  { PHY_OR_PM_Handler,                          stTxBlurtTest },
#endif
  { TEMPERATURE_OR_PM_Handler,                  epMaxTemperature },
  { TEMPERATURE_OR_PM_Handler,                  epMinTemperature },
  { TEMPERATURE_OR_PM_Handler,                  epTempHysteresis },
  { TEMPERATURE_OR_PM_Handler,                  epTempMinThreshold },
  { TEMPERATURE_OR_PM_Handler,                  highTempThreshold },
  /* Metering devices (Electric) */
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#if ( ENABLE_DEMAND_TASKS == 1 )
#if ( DEMAND_IN_METER == 0 )
  { APP_MSG_SetDmdResetDateHandler,             scheduledDemandResetDay },
#endif
  { DEMAND_DrReadListHandler,                   drReadList},
  { DEMAND_demandResetLockoutPeriodHandler,     demandResetLockoutPeriod},
  { DEMAND_demandFutureConfigurationHandler,    demandFutureConfiguration },
  { DEMAND_demandPresentConfigurationHandler,   demandPresentConfiguration },
#endif
  { EDCFG_OR_PM_Handler,                        meterDateTime },
#if ( REMOTE_DISCONNECT == 1 )
  { EDCFG_OR_PM_Handler,                        switchPositionStatus },
  { EDCFG_OR_PM_Handler,                        disconnectCapable },
#endif
  { EDCFG_OR_PM_Handler,                        ansiTableOID },
  { HD_dailySelfReadTimeHandler,                dailySelfReadTime },
  { HD_dsBuDataRedundancyHandler,               dsBuDataRedundancy },
  { HD_dsBuMaxTimeDiversityHandler,             dsBuMaxTimeDiversity },
  { HD_dsBuTrafficClassHandler,                 dsBuTrafficClass },
  { HD_dsBuReadingTypesHandler,                 dsBuReadingTypes },
  { HD_dsBuEnabledHandler,                      dsBuEnabled },
  { HD_historicalRecoveryHandler,               historicalRecovery },
#if 0 // TODO: RA6E1 OR_PM handlers
  { HMC_OR_PM_Handler,                          meterCommLockout },
  { HMC_OR_PM_Handler,                          meterSessionFailureCount },
#endif
  { ID_LpBuDataRedundancyHandler,               lpBuDataRedundancy},
  { ID_LpBuMaxTimeDiversityHandler,             lpBuMaxTimeDiversity},
  { ID_LpBuEnabledHandler,                      lpBuEnabled},
  { ID_LpBuScheduleHandler,                     lpBuSchedule},
  { ID_LpBuTrafficClassHandler,                 lpBuTrafficClass},
  { OR_MR_OrReadListHandler,                    orReadList},
#if ( LP_IN_METER == 0 )
  { ID_lpSamplePeriod,                          lpSamplePeriod},
  { ID_lpBuChannel1Handler,                     lpBuChannel1},
  { ID_lpBuChannel2Handler,                     lpBuChannel2},
  { ID_lpBuChannel3Handler,                     lpBuChannel3},
  { ID_lpBuChannel4Handler,                     lpBuChannel4},
  { ID_lpBuChannel5Handler,                     lpBuChannel5},
  { ID_lpBuChannel6Handler,                     lpBuChannel6},
#if ( ID_MAX_CHANNELS > 6 )
  { ID_lpBuChannel7Handler,                     lpBuChannel7},
  { ID_lpBuChannel8Handler,                     lpBuChannel8},
#endif   /* end of #if ( ID_MAX_CHANNELS > 6 ) */
#endif   /* end of #if ( LP_IN_METER == 0 ) */

#endif   /* endif of (ACLARA_LC == 0) && ( ACLARA_DA == 0 ) */

#if ( ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
  { EDCFG_OR_PM_Handler, edProgrammerName },
  { EDCFG_OR_PM_Handler, edProgramID },
  { EDCFG_OR_PM_Handler, edProgrammedDateTime },
#endif   /* end of #if ( ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 ) ) */

#if ( ( ANSI_STANDARD_TABLES == 1 ) || (ACLARA_DA ==1) )
  { EDCFG_OR_PM_Handler,  edManufacturer },
#endif   /* end of #if ( ANSI_STANDARD_TABLES == 1 ) || (ACLARA_DA ==1) */

#if ( ACLARA_DA == 1)
  {DA_SRFN_REG_OR_PM_Handler, edBootVersion},
  {DA_SRFN_REG_OR_PM_Handler, edBspVersion},
  {DA_SRFN_REG_OR_PM_Handler, edKernelVersion},
  {DA_SRFN_REG_OR_PM_Handler, edCarrierHwVersion},
#endif

#if( SAMPLE_METER_TEMPERATURE == 1 )
  { EDCFG_OR_PM_Handler, edTempSampleRate },
  { EDCFG_OR_PM_Handler, edTempHysteresis },
#endif

   /* Bootloader */
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
#else
  { ENDPT_CIM_CMD_OR_PM_Handler, comDeviceBootloaderVersion },
#endif

  /* Meter programming EP's */
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
  { DFWA_OR_PM_Handler, dfwCompatibilityTestStatus },
  { DFWA_OR_PM_Handler, dfwProgramScriptStatus },
  { DFWA_OR_PM_Handler, dfwAuditTestStatus },
#endif

   /* Non-EP's */
#else   /* else for #if ( EP == 1 ) */
  { DCU_CIM_CMD_OR_PM_Handler, comDeviceFirmwareVersion },
  { DCU_CIM_CMD_OR_PM_Handler, comDeviceHardwareVersion },
  { DCU_CIM_CMD_OR_PM_Handler, comDeviceGatewayConfigRdType },
  { DCU_CIM_CMD_OR_PM_Handler, comDeviceType },
  { DCU_CIM_CMD_OR_PM_Handler, comDeviceMACAddress },
  { DCU_CIM_CMD_OR_PM_Handler, installationDateTime },
#endif   /* end of #if ( EP == 1 ) */

#if ( USE_DTLS == 1 )
  { DTLS_OR_PM_Handler, maxAuthenticationTimeout },
  { DTLS_OR_PM_Handler, minAuthenticationTimeout },
  { DTLS_OR_PM_Handler, initialAuthenticationTimeout },
  { DTLS_OR_PM_Handler, dtlsNetworkRootCA },
  { DTLS_OR_PM_Handler, dtlsSecurityRootCA },
  { DTLS_OR_PM_Handler, dtlsNetworkHESubject },
  { DTLS_OR_PM_Handler, dtlsNetworkMSSubject },
  { DTLS_OR_PM_Handler, dtlsMfgSubject1 },
  { DTLS_OR_PM_Handler, dtlsMfgSubject2 },
  { DTLS_OR_PM_Handler, dtlsServerCertificateSerialNum },
  { DTLS_OR_PM_Handler, dtlsDeviceCertificate },
#endif
#if ( USE_MTLS == 1 )
  { MTLS_OR_PM_Handler, mtlsAuthenticationWindow },
  { MTLS_OR_PM_Handler, mtlsNetworkTimeVariation },
#endif
#if ( PHASE_DETECTION == 1 )
  { PD_OR_PM_Handler, capableOfPhaseSelfAssessment},
  { PD_OR_PM_Handler, capableOfPhaseDetectSurvey },
  { PD_OR_PM_Handler, pdSurveyQuantity           },
  { PD_OR_PM_Handler, pdSurveyMode               },
  { PD_OR_PM_Handler, pdSurveyStartTime          },
  { PD_OR_PM_Handler, pdZCProductNullingOffset   },
  { PD_OR_PM_Handler, pdLcdMsg                   },
  { PD_OR_PM_Handler, pdSurveyPeriod             },
  { PD_OR_PM_Handler, pdBuMaxTimeDiversity       },
  { PD_OR_PM_Handler, pdBuDataRedundancy         },
  { PD_OR_PM_Handler, pdSurveyPeriodQty          },

#endif
  { NULL , invalidReadingType }
};

// TODO: RA6E1 PHY_OR_PM handler,SELF_OR_PM_Handler, MAC_OR_PM_Handler has to be added with this common call
//lint -e750    Lint is complaining about macro not referenced
#define HEEP_COMMON_CALLS APP_MSG_SecurityHandler \
                     ,DFWA_OR_PM_Handler \
                     ,EVL_OR_PM_Handler \
                     ,TIME_UTIL_OR_PM_Handler \
                     ,NWK_OR_PM_Handler \
                     ,SMTDCFG_OR_PM_Handler \
                     ,HEEP_util_OR_PM_Handler \
                     ,MIMTINFO_OR_PM_Handler \
                     ,TIME_SYNC_OR_PM_Handler \
                     ,TEMPERATURE_OR_PM_Handler

#if ( DCU == 1 )
#define HEEP_DCU_CALLS ,DCU_CIM_CMD_OR_PM_Handler
#else
#define HEEP_DCU_CALLS
#endif

// TODO: RA6E1 PWR_OR_PM_Handler has to be added with this HEEP_EP_CALLS
#if ( EP == 1 )
#define HEEP_EP_CALLS ,APP_MSG_initialRegistrationTimeoutHandler \
                 ,APP_MSG_maxRegistrationTimeoutHandler \
                 ,APP_MSG_minRegistrationTimeoutHandler \
                 ,DFWTDCFG_OR_PM_Handler \
                 ,DFWA_OR_PM_Handler \
                 ,DST_timeZoneOffsetHandler \
                 ,DST_dstOffsetHandler \
                 ,DST_dstEnabledHandler \
                 ,DST_dstStartRuleHandler \
                 ,DST_dstEndRuleHandler \
                 ,DST_OR_PM_timeZoneDstHashHander \
                 ,EDCFG_OR_PM_Handler \
                 ,ENDPT_CIM_CMD_OR_PM_Handler \
                 ,MODECFG_decommissionModeHandler \
                 ,PWRCFG_OutageDeclarationDelayHandler \
                 ,PWRCFG_RestorationDeclarationDelayHandler \
                 ,PWRCFG_PowerQualityEventDurationHandler \
                 ,PWRCFG_LastGaspMaxNumAttemptsHandler \
                 ,TIME_SYS_OR_PM_Handler \
                 ,SMTDCFG_OR_PM_Handler
#else
#define HEEP_EP_CALLS
#endif

#if ( USE_DTLS == 1 )
#define HEEP_DTLS_CALLS ,DTLS_OR_PM_Handler
#else
#define HEEP_DTLS_CALLS
#endif

#if ( USE_MTLS == 1 )
#define HEEP_MTLS_CALLS ,MTLS_OR_PM_Handler
#else
#define HEEP_MTLS_CALLS
#endif

// TODO: RA6E1 HMC_OR_PM_Handler has to be added with this HEEP_NOT_LC_NOT_DA_CALLS
#if ( EP == 1 ) && ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#define HEEP_NOT_LC_NOT_DA_CALLS ,EDCFG_OR_PM_Handler \
                                  ,HD_dailySelfReadTimeHandler \
                                  ,HD_dsBuDataRedundancyHandler \
                                  ,HD_dsBuMaxTimeDiversityHandler \
                                  ,HD_dsBuTrafficClassHandler \
                                  ,HD_dsBuReadingTypesHandler \
                                  ,HD_dsBuEnabledHandler \
                                  ,HD_historicalRecoveryHandler \
                                  ,ID_LpBuDataRedundancyHandler \
                                  ,ID_LpBuMaxTimeDiversityHandler \
                                  ,ID_LpBuEnabledHandler \
                                  ,ID_LpBuScheduleHandler \
                                  ,ID_LpBuTrafficClassHandler \
                                  ,OR_MR_OrReadListHandler
#else
#define HEEP_NOT_LC_NOT_DA_CALLS
#endif

#if ( ENABLE_DEMAND_TASKS == 1 )
#define HEEP_DEMAND_CALLS ,APP_MSG_SetDmdResetDateHandler \
                            ,DEMAND_DrReadListHandler \
                            ,DEMAND_demandResetLockoutPeriodHandler \
                            ,DEMAND_demandFutureConfigurationHandler \
                            ,DEMAND_demandPresentConfigurationHandler
#else
#define HEEP_DEMAND_CALLS
#endif

#if ( EP == 1 ) && ( LP_IN_METER == 0 )
#define HEEP_LP_CALLS ,ID_lpSamplePeriod \
                 ,ID_lpBuChannel1Handler \
                 ,ID_lpBuChannel2Handler \
                 ,ID_lpBuChannel3Handler \
                 ,ID_lpBuChannel4Handler \
                 ,ID_lpBuChannel5Handler \
                 ,ID_lpBuChannel6Handler
#else
#define HEEP_LP_CALLS
#endif

#if ( EP == 1 ) && ( LP_IN_METER == 0 ) && ( ID_MAX_CHANNELS > 6 )
#define HEEP_ID_MAX_CALLS ,ID_lpBuChannel7Handler \
                     ,ID_lpBuChannel8Handler
#else
#define HEEP_ID_MAX_CALLS
#endif

#if ( ACLARA_DA == 1)
#define HEEP_DA_CALLS ,DA_SRFN_REG_OR_PM_Handler
#else
#define HEEP_DA_CALLS
#endif

#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
#define HEEP_BOOTLOADER_CALLS
#else
#define HEEP_BOOTLOADER_CALLS ,ENDPT_CIM_CMD_OR_PM_Handler
#endif

#define EVAL_APP_HDR_TUPPLE 0
#if ( EVAL_APP_HDR_TUPPLE == 1 ) //Laying ground work for additional evaluation of App header
// Combination of the Transaction Type, Resource and Method - MUST be in same member order as App Header
typedef struct
{
   enum_TransactionType TransactionType;
   enum_MessageResource Resource;
   enum_MessageMethod   Method;        // For a Request this is Method
}TransRescMethodTuple_t;

// Create tupple for simpler comparson
typedef union
{
   TransRescMethodTuple_t  TRM;
   uint8_t                 Tupple[3];
}TRMTupple_u;

static const TRMTupple_u TRM[]=
{  // Table 2-11 From HEEP identifying the valid TransactionType, Resource and Method combinations.
   // All TT_RESPONSE type commented out since the App generates these messages, not receive them.
   //{.TransactionType, .Resource,   .Method              }  //Bit Structure
                                                             //Bubble-up load profile readings
   {TT_ASYNC,           bu_lp,       method_post          }, //CompactMeterReads
                                                             //Bubble-up load profile readings
   {TT_ASYNC,           bu_lp,       method_post          }, //CompactMeterReads
                                                             //Bubble-up daily shift readings
   {TT_ASYNC,           bu_ds,       method_post          }, //CompactMeterReads
                                                             //Bubble-up engineering data
   {TT_ASYNC,           bu_en,       method_post          }, //FullMeterReads
   {TT_REQUEST,         bu_en,       method_get           }, //GetMeterReads
// {TT_RESPONSE,        bu_en,       OK                   }, //FullMeterReads
                                                             //Bubble-up real-time alarm
   {TT_ASYNC,           bu_am,       method_post          }, //FullMeterReads
   {TT_REQUEST,         bu_am,       method_post          }, //ExchangeWithID
// {TT_RESPONSE,        bu_am,       OK                   }, //FullMeterReads
                                                             //New EP device metadata
   {TT_REQUEST,         dc_rg_md,    method_get           }, //None (header only)
// {TT_RESPONSE,        dc_rg_md,    OK                   }, //FullMeterReads
   {TT_ASYNC,           dc_rg_md,    method_post          }, //FullMeterReads
                                                             //DFW apply
   {TT_REQUEST,         df_ap,       method_put           }, //ExchangeWithID
// {TT_RESPONSE,        df_ap,       OK                   }, //FullMeterReads
                                                             //DFW complete
   {TT_ASYNC,           df_co,       method_post          }, //FullMeterReads
                                                             //DFW download packet
   {TT_REQUEST,         df_dp,       method_put           }, //ExchangeWithID
// {TT_RESPONSE,        df_dp,       OK                   }, //FullMeterReads
                                                             //DFW initialize for download
   {TT_REQUEST,         df_in,       method_put           }, //ExchangeWithID
// {TT_RESPONSE,        df_in,       OK                   }, //FullMeterReads
                                                             //DFW verify
   {TT_REQUEST,         df_vr,       method_get           }, //ExchangeWithID
// {TT_RESPONSE,        df_vr,       OK                   }, //FullMeterReads
                                                             //Get historical alarms
   {TT_REQUEST,         or_ha,       method_get           }, //GetLogEntries
// {TT_RESPONSE,        or_ha,       OK                   }, //CompactMeterReads
                                                             //Get demand readings
   {TT_REQUEST,         dr,          method_get           }, //ExchangeWithID
// {TT_RESPONSE,        dr,          OK                   }, //FullMeterReads
                                                             //Get historical (daily shifted) readings
   {TT_REQUEST,         or_ds,       method_get           }, //GetMeterReads
// {TT_RESPONSE,        or_ds,       OK                   }, //CompactMeterReads
                                                             //Get historical LP readings
   {TT_REQUEST,         or_lp,       method_get           }, //GetMeterReads
// {TT_RESPONSE,        or_lp,       OK                   }, //CompactMeterReads
                                                             //New EP device metadata
   {TT_REQUEST,         rg_md,       method_get           }, //None (header only)
// {TT_RESPONSE,        rg_md,       OK                   }, //FullMeterReads
   {TT_ASYNC,           rg_md,       method_post          }, //FullMeterReads
                                                             //On-request readings
   {TT_REQUEST,         or_mr,       method_get           }, //GetMeterReads
// {TT_RESPONSE,        or_mr,       OK                   }, //CompactMeterReads
                                                             //On-request alarm status
   {TT_REQUEST,         or_am,       method_get           }, //GetAlarms
// {TT_RESPONSE,        or_am,       OK                   }, //CompactMeterReads
                                                             //Read ANSI table
   {TT_REQUEST,         or_mt,       method_get           }, //ExchangeWithID
// {TT_RESPONSE,        or_mt,       OK                   }, //FullMeterReads
                                                             //Read parameter
   {TT_REQUEST,         or_pm,       method_get           }, //GetMeterReads
// {TT_RESPONSE,        or_pm,       OK                   }, //FullMeterReads
                                                             //Reset Demand
   {TT_REQUEST,         dr,          method_post          }, //ExchangeWithID (optional) or None
// {TT_RESPONSE,        dr,          OK                   }, //FullMeterReads
                                                             //Load Control
   {TT_REQUEST,         lc,          method_put           }, //CompactMeterReads
// {TT_RESPONSE,        lc,          OK                   }, //FullMeterReads
                                                             //Reset to initial values
   {TT_REQUEST,         mm_re,       method_put           }, //ExchangeWithID
// {TT_RESPONSE,        mm_re,       OK                   }, //None (header only)
                                                             //Resource Discovery
   {TT_REQUEST,         rd,          method_get           }, //ExchangeWithID
// {TT_RESPONSE,        rd,          OK                   }, //FullMeterReads
                                                             //Set interval data construction configuration
   {TT_REQUEST,         or_pm,       method_put           }, //ExchangeWithID
// {TT_RESPONSE,        or_pm,       OK                   }, //None (header only)
                                                             //Switch RCD open/close/arm-for-close
   {TT_REQUEST,         rc,          method_put           }, //ExchangeWithID
// {TT_RESPONSE,        rc,          OK                   }, //FullMeterReads
                                                             //Trace Route
   {TT_REQUEST,         tr,          method_post          }, //ExchangeWithID
// {TT_RESPONSE,        tr,          OK                   }, //FullMeterReads
   {TT_ASYNC,           tr,          method_post          }, //FullMeterReads
                                                             //Tunnel STAR DCU Command
   {TT_REQUEST,         tn_st_dc,    method_put           }, //None
// {TT_RESPONSE,        tn_st_dc,    OK                   }, //None
                                                             //Tunneling TWACS Manufacturing/Programming Port – Serial Protocol (MPP-SP)
   {TT_ASYNC,           tn_tw_mp,    method_post          }, // None (MPP-SP Request Packet)
   {TT_REQUEST,         tn_tw_mp,    method_get           }, // None (MPP-SP Request Packet)
// {TT_RESPONSE,        tn_tw_mp,    OK                   }, // None (MPP-SP Response Packet)
// {TT_RESPONSE,        tn_tw_mp,    Timeout              }, // None (MPP-SP Response Packet)
// {TT_RESPONSE,        tn_tw_mp,    BadRequest           }, // None (MPP-SP Response Packet)
// {TT_RESPONSE,        tn_tw_mp,    ServiceUnavailable   }, // None (MPP-SP Response Packet)
                                                             //Write ANSI table
   {TT_REQUEST,         or_mt,       method_put           }, // ExchangeWithID
// {TT_RESPONSE,        or_mt,       OK                   }, // FullMeterReads
                                                             //Write parameter
   {TT_REQUEST,         or_pm,       method_put           }, // ExchangeWithID
// {TT_RESPONSE,        or_pm,       OK                   }  // FullMeterReads
};

#endif


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static void ReadingQty_Inc(struct readings_s *p);
static void ReadingHeader_Set(struct   readings_s *p,ReadingsValueTypecast Type,uint16_t  Size,meterReadingType ReadingType );
static uint8_t getMinSize(uint8_t const *pData, uint8_t size);



/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************
 *
 * Function Name: HEEP_MSG_Tx(APP_MSG_Tx_t const *, buffer_t const *)
 *
 * Purpose: This function is called to send the bubble-up data or response back to the head-end.
 *          The calling function will allocate a buffer. This buffer will be freed by this or the lower layers. Once the
 *          buffer is passed to  this function, the calling module will not touch the buffer.
 *
 * Arguments: APP_MSG_Tx_t * - Contains information regarding the QOS and Message.
 *            buffer_t * - Payload and the length of the payload
 *
 * Returns: Success/Failure
 *
 * Side Effects: None
 *
 * Reentrant Code: NO
 *
 **********************************************************************************************************************/
returnStatus_t HEEP_MSG_Tx ( HEEP_APPHDR_t const *heepMsgInfo, buffer_t const *payloadBuf )
{
   returnStatus_t    RetVal = eFAILURE;      //Return status
   EventMarkSent_s   markedSent;             //Used to mark events that have been sent to head end
   uint8_t           numOfAlarms;            //Number of alarms added by the alarm module
   uint16_t          alarmBytes = 0;         //Number of bytes added by the alarm module
   uint16_t          offset;                 //Offset into the buffer
   bool              sent;                   //Indicator of whether message can be sent

#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
   if ( eFAILURE == EVL_LGSimAllowMsg(heepMsgInfo, payloadBuf) )
   {
      INFO_printf( "Message dropped while in LG Simulation");
      RetVal = eSUCCESS;
   }
   else
#endif
   {

      if (  ( payloadBuf != NULL ) &&
            ( payloadBuf->x.dataLen >= ( VALUES_INTERVAL_END_SIZE + EVENT_AND_READING_QTY_SIZE ) || // Size needs to be at least 5 bytes long
            ( heepMsgInfo->Resource == (uint8_t)tn_st_dc ) ) )                                      // Except for Tunnel/STAR/DCU. It can be as small as 0 byte
      {
         //At least 1 byte of payload
         NWK_Address_t  dst_address;
         NWK_Override_t override = { eNWK_LINK_OVERRIDE_NULL, eNWK_LINK_SETTINGS_OVERRIDE_NULL };
         buffer_t      *pBuf;
         uint16_t       opportunisticAlarmSize = 0;
         if ( (   ( heepMsgInfo->Resource    == (uint8_t)bu_lp )
               || ( heepMsgInfo->Resource    == (uint8_t)bu_ds )
               || ( heepMsgInfo->Resource    == (uint8_t)bu_en )
#if 0   //TODO: Enable these when HE is ready to accept
               || ( heepMsgInfo->Resource    == (uint8_t)df_dp )
               || ( heepMsgInfo->Resource    == (uint8_t)df_co )
#endif
                                                                      ) &&
              ( heepMsgInfo->TransactionType == (uint8_t)TT_ASYNC ) )
         {  //Max number of bytes needed for opportunistic alarms
            opportunisticAlarmSize = ( uint16_t )min((APP_MSG_MAX_DATA - payloadBuf->x.dataLen), MAX_ALARM_MEMORY);
         }
         // Allocate a temporary buffer to build the payload
         pBuf = BM_allocStack( payloadBuf->x.dataLen + HEEP_APP_HEADER_SIZE + opportunisticAlarmSize);
         if( pBuf != NULL )
         {
            uint8_t       *data = ( uint8_t* )pBuf->data;
            NWK_GetConf_t  GetConf;

            // Retrieve Head End context ID
            GetConf = NWK_GetRequest( eNwkAttr_ipHEContext );
            dst_address.addr_type = eCONTEXT;

            if (GetConf.eStatus != eNWK_GET_SUCCESS) {
               dst_address.context = DEFAULT_HEAD_END_CONTEXT; // Use default in case of error
            }
            else if( ( (uint8_t) TT_ASYNC == heepMsgInfo->TransactionType ) && ((uint8_t)tr != heepMsgInfo->Resource))
            {
               dst_address.context = GetConf.val.ipHEContext;
            }
            else //sync rsp reply with the same context as request
            {
              dst_address.context = heepMsgInfo->TransactionContext;
            }

            // In the case of a remote DCU-DCU response, we need to send the response over the air instead of sending it back to the main board.
            if ( ( heepMsgInfo->Resource        == (uint8_t)tn_st_dc )           &&
                 ( heepMsgInfo->TransactionType == (uint8_t)TT_RESPONSE ) ) {
               override.linkOverride         = eNWK_LINK_OVERRIDE_TENGWAR_MAC;
               override.linkSettingsOverride = eNWK_LINK_SETTINGS_OVERRIDE_BROADCAST_OB;
            }


            offset = 0;
            data[offset++] = HEEP_APP_INTERFACE_REVISION;
            data[offset++] =  ( ( heepMsgInfo->TransactionType & HEEP_APPHDR_TT_MASK ) << HEEP_APPHDR_TT_SHIFT ) |
                              ( ( heepMsgInfo->Resource & HEEP_APPHDR_RESOURCE_MASK ) << HEEP_APPHDR_RESOURCE_SHIFT );
            data[offset++] = heepMsgInfo->Method_Status;
            data[offset++] = heepMsgInfo->Req_Resp_ID;

            if ( ( heepMsgInfo->Resource        == (uint8_t)tn_st_dc )     &&
                 ( heepMsgInfo->TransactionType == (uint8_t)TT_RESPONSE ) ) {
               // Copy payload
               ( void )memcpy( &data[offset], &payloadBuf->data[0], payloadBuf->x.dataLen );
            } else {
               // Copy the timestamp
               ( void )memcpy( &data[offset], &payloadBuf->data[0], VALUES_INTERVAL_END_SIZE );
               offset += VALUES_INTERVAL_END_SIZE; //Skip past the timestamp

               //Save the events that are already present in the payload
               numOfAlarms = payloadBuf->data[VALUES_INTERVAL_END_SIZE] >> 4;

               /************************************************************************************************************
                  If the message is LP, DS, EN or DFW bubble up, check for and add opportunistic alarms.
                  Call alarm module to add alarms. Alarms will be added after app header, timestamp, event and reading qty
               ************************************************************************************************************/
               if ( (   ( heepMsgInfo->Resource    == (uint8_t)bu_lp )
                     || ( heepMsgInfo->Resource    == (uint8_t)bu_ds )
                     || ( heepMsgInfo->Resource    == (uint8_t)bu_en )
#if 0   //TODO: Enable these when HE is ready to accept
                     || ( heepMsgInfo->Resource    == (uint8_t)df_dp )
                     || ( heepMsgInfo->Resource    == (uint8_t)df_co )
#endif
                                                                            ) &&
                    ( heepMsgInfo->TransactionType == (uint8_t)TT_ASYNC ) )
               {
                  numOfAlarms += ALRM_AddOpportunisticAlarms( payloadBuf->x.dataLen, &alarmBytes, &data[ offset + 1 ],
                                                               &HEEP_msgID, &markedSent );
               }

               //Update the event quantity
               data[offset++] = ( uint8_t )( ( payloadBuf->data[VALUES_INTERVAL_END_SIZE] & 0x0F ) | ( numOfAlarms << 4 ) );

               //Skip past the Alarms data added
               offset += alarmBytes;

               //Copy the application payload, to the temp buffer
               ( void )memcpy(   &data[offset], &payloadBuf->data[VALUES_INTERVAL_END_SIZE + EVENT_AND_READING_QTY_SIZE],
                                 ( size_t )( payloadBuf->x.dataLen - ( VALUES_INTERVAL_END_SIZE + EVENT_AND_READING_QTY_SIZE ) ) );
            }

            /* If this is a bubble-up message, we need to insert one of the alarm index id parameter values
               in place of the response ID of the application header. */
            if( (uint8_t)TT_ASYNC == heepMsgInfo->TransactionType )
            {
               if( ( (uint8_t)bu_lp == heepMsgInfo->Resource ) || ( (uint8_t)bu_am == heepMsgInfo->Resource ) )
               {  // insert the real time alarm index ID for both bu_lp and bu_am resource messages
                  data[ offsetof( AppHdrBitStructure_t, requestResponseId ) ] = EVL_GetRealTimeIndex();
               }
               else
               { // for all other bubble up messages, insert the opportunistic alarm index ID
                  data[ offsetof( AppHdrBitStructure_t, requestResponseId ) ] = EVL_GetNormalIndex();
               }
            }

#if (USE_DTLS == 1)
#if ( DTLS_FIELD_TRIAL == 0 )
            if ( heepMsgInfo->appSecurityAuthMode == 0 )
#else
            /* Allow unsecured communications until DTLS session established */
            if (  ( heepMsgInfo->appSecurityAuthMode == 0 ) ||
                  ( !DTLS_IsSessionEstablished() ) )
#endif
#endif
            {
               ( void )NWK_DataRequest(  0,
                                         heepMsgInfo->qos,
                                         payloadBuf->x.dataLen + alarmBytes + HEEP_APP_HEADER_SIZE,
                                         data,
                                         &dst_address,
                                         &override,
                                         heepMsgInfo->callback,
                                         NULL );

               sent = (bool)true;
               RetVal = eSUCCESS;
            }
#if (USE_DTLS == 1)
            else
            {
               if( DTLS_IsSessionEstablished() )   /* Make sure DTLS session has been established. */
               {
                  ( void )DTLS_DataRequest( (uint8_t)UDP_DTLS_PORT,
                                            heepMsgInfo->qos,
                                            payloadBuf->x.dataLen + alarmBytes + HEEP_APP_HEADER_SIZE,
                                            data,
                                            &dst_address,
                                            &override,
                                            heepMsgInfo->callback,
                                            NULL );

                  sent = (bool)true;
                  RetVal = eSUCCESS;
               }
               else  /* No session established. */
               {
                  DTLS_CounterInc( eDTLS_IfOutNoSessionErrors );
                  sent = (bool)false;
                  RetVal = eFAILURE;
               }
            }
#endif

            /* Now mark event(s) as sent, as appropriate   */
#if (USE_DTLS == 1)
            if ( sent && ( alarmBytes != 0 ) )
#else
            if ( alarmBytes != 0 )
#endif
            {
               EVL_MarkSent( &markedSent );  /*lint !e645 if alarmBytes is non-zero, markedSent has been init'd */
            }

#if (TM_HEEP_UNIT_TEST != 0)
            INFO_printHex( sent ? "Sent: " : "Not sent: " ,
                           data, payloadBuf->x.dataLen + alarmBytes + HEEP_APP_HEADER_SIZE );
#endif
            // free the temp buffer
            BM_free( pBuf );
         }
         BM_free( ( buffer_t * )payloadBuf ); //Free the app buffer
      }
      else
      {
         if (payloadBuf != NULL)
         {
            BM_free( ( buffer_t * )payloadBuf ); //Free the app buffer
            DBG_printf( "ERROR: Invalid length passed to HEEP_MSG_Tx function" );  //Invalid length
         }
         else
         {
            DBG_printf( "ERROR: NULL BUFFER passed to HEEP_MSG_Tx function" );  //Invalid buffer
         }
      }
   }

   return ( RetVal );
}

/***********************************************************************************************************************
 *
 * Function Name: HEEP_MSG_TxHeepHdrOnly(APP_MSG_Tx_t const *)
 *
 * Purpose: This function is called to send a response consisting of just the
 *          heep header back to the head-end - no application payload
 *
 * Arguments: HEEP_MSG_Tx_t * - Contains information regarding the QOS and Message.
 *
 * Returns: Success/Failure
 *
 * Side Effects: None
 *
 * Reentrant Code: NO
 *
 **********************************************************************************************************************/
returnStatus_t HEEP_MSG_TxHeepHdrOnly ( HEEP_APPHDR_t const *heepHdr )
{
   returnStatus_t    RetVal = eSUCCESS;         //Return status
   uint16_t          offset;                    //Offset into the buffer
   NWK_Address_t     dst_address;
   NWK_Override_t    override = { eNWK_LINK_OVERRIDE_NULL, eNWK_LINK_SETTINGS_OVERRIDE_NULL };
   uint8_t           data[HEEP_APP_HEADER_SIZE];
   bool              sent;
   NWK_GetConf_t     GetConf;

#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
   if ( eSUCCESS == EVL_LGSimActive() )
   {
      INFO_printf( "Message dropped while in LG Simulation");
      RetVal = eSUCCESS;
   }
   else
#endif
   {
      // Retrieve Head End context ID
      GetConf = NWK_GetRequest( eNwkAttr_ipHEContext );
      dst_address.addr_type = eCONTEXT;

      if (GetConf.eStatus != eNWK_GET_SUCCESS)
      {
         dst_address.context = DEFAULT_HEAD_END_CONTEXT; // Use default in case of error
      }
      else if( ((uint8_t)TT_ASYNC == heepHdr->TransactionType) && ((uint8_t)tr != heepHdr->Resource))
      {
         dst_address.context = GetConf.val.ipHEContext;
      }
      else //sync rsp reply with the same context as request
      {
         dst_address.context = heepHdr->TransactionContext;
      }

      // Prepare the Application header
      offset = 0;
      data[offset++] = HEEP_APP_INTERFACE_REVISION;
      data[offset++] =  ( ( heepHdr->TransactionType & HEEP_APPHDR_TT_MASK )       << HEEP_APPHDR_TT_SHIFT ) |
                        ( ( heepHdr->Resource        & HEEP_APPHDR_RESOURCE_MASK ) << HEEP_APPHDR_RESOURCE_SHIFT );
      data[offset++] = heepHdr->Method_Status;
      data[offset  ] = heepHdr->Req_Resp_ID;

#if (USE_DTLS == 1)
#if ( DTLS_FIELD_TRIAL == 0 )
      if ( heepHdr->appSecurityAuthMode == 0 )
#else
      /* Allow unsecured communications until DTLS session established */
      if (  ( heepHdr->appSecurityAuthMode == 0 ) ||
            ( !DTLS_IsSessionEstablished() ) )
#endif
#endif
      {
         ( void )NWK_DataRequest(  0,
                                   heepHdr->qos,
                                   HEEP_APP_HEADER_SIZE,
                                   data,
                                   &dst_address,
                                   &override,
                                   heepHdr->callback,
                                   NULL );
         sent = (bool)true;
      }
#if (USE_DTLS == 1)
      else
      {
         if( DTLS_IsSessionEstablished() )   /* Make sure DTLS session has been established. */
         {
            ( void )DTLS_DataRequest( (uint8_t)UDP_DTLS_PORT,
                                      heepHdr->qos,
                                      HEEP_APP_HEADER_SIZE,
                                      data,
                                      &dst_address,
                                      &override,
                                      heepHdr->callback,
                                      NULL );

            sent = (bool)true;
            RetVal = eSUCCESS;
         }
         else  /* No session established  */
         {
            DTLS_CounterInc( eDTLS_IfOutNoSessionErrors );
            sent = (bool)false;
            RetVal = eFAILURE;
         }
      }
#endif
#if (TM_HEEP_UNIT_TEST != 0)
      INFO_printHex( sent ? "Sent: " : "Not sent: " ,
                     data, HEEP_APP_HEADER_SIZE );
#endif
   }
   return ( RetVal );
}

/***********************************************************************************************************************
 *
 * Function Name: HEEP_UTIL_unpackHeader(STACK_DataInd_t *indication, uint16_t bitNo, HEEP_MSG_RX_t appHdr)
 *
 * Purpose: This function will unpack the HEEP header from a received HEEP message buffer
 *
 * Arguments: STACK_DataInd_t *indication - the buffer passed up from the Stack layer
 *            uint16_t *bitNo - the current bit position in the indication payload
 *            HEEP_MSG_RX_t *appHdr - The HEEP header unpacked into a structure
 *
 * Returns: True - header was successfully parsed; False - unrecognized interface revision
 *
 * Side Effects: None
 *
 * Reentrant Code: Yes
 *
 **********************************************************************************************************************/
bool HEEP_UTIL_unpackHeader ( NWK_DataInd_t *indication, uint16_t *bitNo, HEEP_APPHDR_t *appHdr )  /*lint !e818 could be pointer to const */
{
   bool validHeader = false;
   uint8_t interfaceRevision;

   *bitNo = 0;
   interfaceRevision = UNPACK_bits2_uint8( indication->payload, bitNo, HEEP_APPHDR_INTERFACE_REVISION_SIZE_BITS );
   DBG_logPrintf( 'I', "HEEP Revision: %d", interfaceRevision );

   /* Header Validation
    * The InterfaceRevision field of the header must be set to 0, otherwise header validation fails.
    * The TransactionType   field of the header must be set to a non-RSVD value, otherwise header validation fails.
    * The Resource          field of the header must be set to a non-RSVD value, otherwise header validation fails.
    * The Method            field of the header must be set to a non-RSVD value, otherwise header validation fails.
    * The TransactionType, Resource, Method field tupple from the header must be valid, otherwise header validation fails.
    * If at any time header validation fails, then the packet is discarded and appIfInErrors is incremented.
    */
   //Validate the interface revision
   if ( HEEP_APP_INTERFACE_REVISION == interfaceRevision )
   {
      appHdr->TransactionType = UNPACK_bits2_uint8( indication->payload, bitNo, HEEP_APPHDR_TRANSACTION_TYPE_SIZE_BITS );
      appHdr->Resource        = UNPACK_bits2_uint8( indication->payload, bitNo, HEEP_APPHDR_RESOURCE_SIZE_BITS );
      appHdr->Method_Status   = UNPACK_bits2_uint8( indication->payload, bitNo, HEEP_APPHDR_METHOD_STATUS_SIZE_BITS );
      appHdr->Req_Resp_ID     = UNPACK_bits2_uint8( indication->payload, bitNo, HEEP_APPHDR_REQUEST_RESPONSE_ID_SIZE_BITS );
      appHdr->qos             = indication->qos;
      if( indication->srcAddr.addr_type == eCONTEXT)
      {
          appHdr->TransactionContext = indication->srcAddr.context;
      }
      else
      {
          appHdr->TransactionContext = DEFAULT_HEAD_END_CONTEXT;
      }
      if ( ( appHdr->TransactionType <= (uint8_t)LAST_VALID_TRANS_TYPE ) &&
           ( appHdr->Resource        <= (uint8_t)LAST_VALID_RESOURCE )   &&
           ( appHdr->Method_Status   <= (uint8_t)LAST_VALID_METHOD ) )
      {
#if ( EVAL_APP_HDR_TUPPLE == 1 ) //Laying ground work for additional evaluation of App header
         uint8_t  i;
         for ( i = 0; i < ARRAY_IDX_CNT(TRM); i++ )
         {
            if ( 0 == memcmp(&TRM[i].Tupple[0], &appHdr->TransactionType, sizeof(TRMTupple_u) ) )
            {
#endif
               DBG_logPrintf( 'I', "Transaction Type: %s, Resource: %s, Method: %s, Response ID: %d, qos: 0x%02x",
                           heepTransactionTypes[ appHdr->TransactionType ], heepResourceNames[ appHdr->Resource - 1 ],
                           heepMethods[ appHdr->Method_Status - 1 ], appHdr->Req_Resp_ID, appHdr->qos );

               validHeader = true;
#if ( EVAL_APP_HDR_TUPPLE == 1 ) //Laying ground work for additional evaluation of App header
               break;
            }
         }
#endif
      }
   }
   return validHeader;
}  /*lint !e818 indication could be pointer to const   */

/***********************************************************************************************************************
 *
 * Function Name: HEEP_MSG_GetNextReadingInfo(void *payloadBuf, uint16_t *bitNo)
 *
 * Purpose: Retrieves information on the next reading in the request buffer.
 *
 * Arguments: void *payloadBuf - The application payload buffer.
 *            uint16_t *bitNo - The current offset in the application payload buffer.  On entry, this is the index
 *                              within payloadBuff of the start of the next reading information.  On exit, it will
 *                              be the index of the start of the reading value.
 *            uint16_t *readingValNumBytes - Number of bytes in the reading value
 *            uint8_t *readingValTypecast - Enumeration describing the data type of the reading value
 *            uint8_t *readingValPow10 - Power of 10 shift to be applied to the reading value.  Note that
 *                                       this will be the actual shift value, not the enumeration.
 *            uint8_t *readingFlags - Bit flags associated with the reading:
 *                                    10000000 - is coincident trigger
 *                                    01000000 - time stamp present
 *                                    00100000 - reading qualities present
 *                                    00010000 - use
 *                                    00001000 - reserved
 *                                    00000111 - Bits masked off and never used
 *
 * Returns: uint16_t readingType enumeration
 *
 * Side Effects: None
 *
 * Reentrant Code: Yes
 *
 **********************************************************************************************************************/
#if 0
static uint16_t HEEP_MSG_GetNextReadingInfo( void *payloadBuf, uint16_t *bitNo, uint16_t *readingValNumBytes,
                                             uint8_t *readingValTypecast, int8_t *readingValPow10, uint8_t *readingFlags )
{
   uint16_t sizeAndType, readingType;

   *readingFlags = UNPACK_bits2_uint8( payloadBuf, bitNo, 8 );

   /***
   * Pull out the power of ten adjustment and convert from the enumeration to the actual adjustment value.
   ***/
   *readingValPow10 = -( int8_t )( *readingFlags & 0x07 );
   if ( *readingValPow10 == -7 )
   {
      *readingValPow10 = -9;
   }
   *readingFlags &= ~0x07;   // mask off the power of ten adjustment from hte flags.

   sizeAndType = UNPACK_bits2_uint16( payloadBuf, bitNo, 16 );
   *readingValNumBytes = sizeAndType >> 4;
   *readingValTypecast = sizeAndType & 0x07;

   readingType = UNPACK_bits2_uint16( payloadBuf, bitNo, 16 );

   return readingType;
}  /*lint !e818 payloadBuf could be pointer to const   */
#endif
/*******************************************************************************

   Function name: HEEP_getPowerOf10Code

   Purpose: Returns power of 10 as per HEEP document and Normalize the value

   Arguments: uint8_t ch: Channel number
               int64_t *val

   Returns: Power of 10 coded (as per HEEP document)

   Notes:

*******************************************************************************/
uint8_t HEEP_getPowerOf10Code( uint8_t powerOf10, int64_t *val )
{
   uint8_t powerOf10Code; //Code value for power of 10
   int64_t valNew;        //Used to compress the val by removing 0's at the end and then adjusting the power of 10

   while ( powerOf10 > 0 )
   {
      valNew = *val / 10;
      if ( ( valNew * 10 ) == *val )
      {
         powerOf10--;
         *val = valNew;
      }
      else
      {
         break;
      }
   }

   switch( powerOf10 ) //Convert to code that will be transmitted
   {
      case 7:
         {
            *val *= 100;
            powerOf10Code = 7;
            break;
         }
      case 8:
         {
            *val *= 10;
            powerOf10Code = 7;
            break;
         }
      case 9:
         {
            powerOf10Code = 7;
            break;
         }
      default:
         {
            powerOf10Code = powerOf10;
            break;
         }
   }
   return powerOf10Code;
}

/*******************************************************************************

   Function name: HEEP_getMinByteNeeded

   Purpose: Returns minimum number of bytes needed to transmit this int64_t

   Arguments: int64_t

   Returns: Minimum number of bytes needed to transmit the argument

   Notes:

*******************************************************************************/
uint8_t HEEP_getMinByteNeeded( int64_t val, ReadingsValueTypecast typecast, uint16_t valueSizeInBytes )
{
   uint8_t str[sizeof( int64_t )];
   uint8_t decimal;
   uint8_t minNumBytesNeeded = 0;

   ( void )memcpy( &str[0], ( uint8_t * )&val, sizeof( int64_t ) );
   Byte_Swap( &str[0], sizeof( int64_t ) );

   if( intValue == typecast )
   {
      if ( val >= 0 )
      {
         for ( decimal = 0; decimal < ( sizeof( int64_t ) - 1 ); decimal++ )
         {
            if ( !( str[decimal] == 0 && ( str[decimal + 1] & 0x80 ) == 0 ) )
            {
               break;
            }
         }
      }
      else
      {
         for ( decimal = 0; decimal < ( sizeof( int64_t ) - 1 ); decimal++ )
         {
            if ( !( str[decimal] == 0xFF && ( str[decimal + 1] & 0x80 ) == 0x80 ) )
            {
               break;
            }
         }
      }

      minNumBytesNeeded = sizeof( int64_t ) - decimal; //number of bytes needed
   }
   else if( uintValue == typecast )
   {
      for ( decimal = 0; decimal < ( sizeof( uint64_t ) - 1 ); decimal++ )
      {
         if ( (str[decimal] != 0 ) || ( str[decimal] == 0 && (str[decimal + 1] > 0x7F) ) )
         {
            break;
         }
      }

      minNumBytesNeeded = sizeof( int64_t ) - decimal; //number of bytes needed
   }
   else if( valueSizeInBytes )
   { //if typecast cannot be compressed (non integer) then the minimum bytes needed is the size of the value
      minNumBytesNeeded = (uint8_t)valueSizeInBytes;
   }

   return minNumBytesNeeded;
}

/*******************************************************************************

   Function name: HEEP_initHeader

   Purpose: Initialize HEEP header structure with system wide default values

   Arguments: HEEP_APPHDR_t *

   Returns: none

   Notes:

*******************************************************************************/
void HEEP_initHeader( HEEP_APPHDR_t *hdr )
{
   uint8_t  securityMode;

   ( void )APP_MSG_SecurityHandler( method_get, appSecurityAuthMode, &securityMode, NULL );

   ( void )memset( ( uint8_t * )hdr, 0, sizeof( HEEP_APPHDR_t ) );
   hdr->appSecurityAuthMode = securityMode;
}

/*******************************************************************************

   Function name: OR_PM_MsgHandler

   Purpose: Handle OR_PM requests

   Arguments: APP_MSG_Rx_t *appMsgRxInfo - Pointer to application header structure from head end
              void *payloadBuf           - pointer to data payload
              uint16_t length            - payload length

   Returns: None

   Notes:   GetMeterReads bit structure used to "get" the value. Response uses "FullMeterReads" structure.
            ExchangeWithID bit structure used to "put" the value. Response is "header only".

*******************************************************************************/
//lint -efunc(818, OR_PM_MsgHandler) arguments cannot be const because of API requirements
void OR_PM_MsgHandler(HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length)
{
   (void)length;
   HEEP_APPHDR_t      heepRespHdr;  // Application header/QOS info
   OR_PM_HandlersDef *pHandlers;    //Used to search for id
   OR_PM_Attr_t       OR_PM_Attr_s; //Info of the return string
   buffer_t          *lBuf;         // pointer to value

   //Application header/QOS info
   HEEP_initHeader( &heepRespHdr );
   heepRespHdr.TransactionType = (uint8_t)TT_RESPONSE;
   heepRespHdr.Resource        = (uint8_t)or_pm;
   heepRespHdr.Method_Status   = (uint8_t)OK;  //Default
   heepRespHdr.Req_Resp_ID     = (uint8_t)heepReqHdr->Req_Resp_ID;
   heepRespHdr.qos             = (uint8_t)heepReqHdr->qos;
   heepRespHdr.callback        = NULL;
   heepRespHdr.TransactionContext = heepReqHdr->TransactionContext;

   if( length < EVENT_AND_READING_QTY_SIZE)
   { //Invalid request,
      //Must have at least one mini header rdg qty bytes to declare no readings requested
      heepRespHdr.Method_Status = (uint8_t)BadRequest;
      (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
   }
   else if ( method_get == (enum_MessageMethod)heepReqHdr->Method_Status)
   {  //Process the get request, prepare Full Meter Read structure
      union TimeSchRdgQty_u qtys;
      uint16_t         bits=0;              //Keeps track of the bit position within the Bitfields
      uint16_t         bytesLeft = length;             //Keeps track of the Bytes read from the request buffer
      uint16_t         readingQtyIndex = 0; //index into FMR readingqty

      //Read the parameters requested by parsing the packet payload - an implied GetMeterReads bit-structure
      qtys.TimeSchRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS); //Parse the fields passed in
      bytesLeft          -= EVENT_AND_READING_QTY_SIZE;

      //the first mini header must have a non zero reading quantity
      if (qtys.bits.rdgQty == 0 || qtys.bits.timeSchQty > 0)
      { //Invalid request, send only header back
         heepRespHdr.Method_Status = (uint8_t)BadRequest;
         (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
      }
      else
      {
         buffer_t      *pBuf;     // pointer to message
         pBuf = BM_alloc( APP_MSG_MAX_DATA ); //Allocate a big buffer to accommodate worst case payload.
         lBuf = BM_alloc( MAX_OR_PM_PAYLOAD_SIZE ); //Allocate a buffer to accommodate biggest parameter size.
         if ( pBuf != NULL && lBuf != NULL )
         {
            meterReadingType readingType;             //Reading Type
            returnStatus_t retVal = eAPP_NOT_HANDLED; //status
            uint8_t       i;                          //loop counter
            bool          foundHandler;               // flag, handler found or not
            pack_t        packCfg;                    // Struct needed for PACK functions
            uint32_t      msgTime;                    // Time converted to seconds since epoch
            sysTimeCombined_t currentTime;            // Get current time
            uint8_t       eventCount_flags;           // Counter for alarms and readings
            uint8_t       numReadings = 0;            // total number of readings packed
            uint16_t      rdgDataLength = 0;          // data length of this reading
            uint16_t      rtype;

            (void)memset(lBuf->data, 0, MAX_OR_PM_PAYLOAD_SIZE);
            PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );
            //Add current timestamp
            (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
            msgTime = (uint32_t)(currentTime / TIME_TICKS_PER_SEC);
            //reset the read head to beginning of the buffer so that we can loop thorugh all mini headers
            bits = 0;
            bytesLeft = length;
            //no data has been packed yet so 0 data length
            pBuf->x.dataLen = 0;
            while ( bytesLeft )
            {
               if( heepRespHdr.Method_Status   == (uint8_t)PartialContent )
               {  //we've filled the buffer as much as possible and broken out of the for loop
                  // no point in reading more bytes or adding another FMR BS
                  break;
               }
               else if ( bytesLeft < EVENT_AND_READING_QTY_SIZE )
               {  //not enough bytes describing the reading quantity
                  heepRespHdr.Method_Status = (uint8_t)BadRequest;
                  break;
               }
               else if( (pBuf->x.dataLen + EVENT_AND_READING_QTY_SIZE + VALUES_INTERVAL_END_SIZE) > APP_MSG_MAX_DATA)
               {  //adding another FMR structure will overflow the message
                  //update status and break
                  heepRespHdr.Method_Status   = (uint8_t)PartialContent;
                  break;
               }
               else
               {  //the buffer can accommodate a new FMR structure
                  PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&msgTime, &packCfg );
                  pBuf->x.dataLen += VALUES_INTERVAL_END_SIZE;
                  readingQtyIndex = pBuf->x.dataLen;
                  numReadings = 0;
                  //These bytes will let us know how many more readings to process
                  qtys.TimeSchRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS);
                  bytesLeft          -= EVENT_AND_READING_QTY_SIZE;
                  eventCount_flags = qtys.bits.rdgQty;
                  PACK_Buffer( sizeof(eventCount_flags) * 8, (uint8_t *)&eventCount_flags, &packCfg );
                  pBuf->x.dataLen += sizeof(eventCount_flags);
                  if( qtys.bits.rdgQty == 0 || qtys.bits.timeSchQty > 0 )
                  {  //If the reading quantity is 0, we do not know how many more readings to process.
                     //Break out of loop so response can be sent to head end.
                     break;
                  }
               }

               if( bytesLeft <  ( qtys.bits.rdgQty *  READING_TYPE_SIZE ) )
               {  //the reading quanity described is too large for the message provided
                  //break to avoid the reading from beyond message buffers end
                  heepRespHdr.Method_Status = (uint8_t)BadRequest;
                  break;
               }

               for (i =0; i< qtys.bits.rdgQty; i++ ) //process all reading types
               {
                  foundHandler = false;
                  OR_PM_Attr_s.rValLen = 0;
                  OR_PM_Attr_s.rValTypecast = 0xF; //Invalid value
                  OR_PM_Attr_s.powerOfTen = 0;     //Initialize to zero
                  rdgDataLength = 0;
                  eventCount_flags = 0x20; //flags, needs to send quality code
                  readingType = (meterReadingType)UNPACK_bits2_uint16(payloadBuf, &bits, sizeof(readingType) * 8);
                  bytesLeft   -= READING_TYPE_SIZE;
                  (void)memset(lBuf->data, 0, MAX_OR_PM_PAYLOAD_SIZE); // initialize the destination buffer

                  for( pHandlers = (OR_PM_HandlersDef *)OR_PM_Handlers; pHandlers->pHandler != NULL;  pHandlers++ )
                  {
                     if (pHandlers->rType == readingType)
                     {  //found handler
                        foundHandler = true;
_Pragma ( "calls = \
                        HEEP_COMMON_CALLS \
                        HEEP_DCU_CALLS \
                        HEEP_EP_CALLS \
                        HEEP_DTLS_CALLS \
                        HEEP_MTLS_CALLS \
                        HEEP_NOT_LC_NOT_DA_CALLS \
                        HEEP_DEMAND_CALLS \
                        HEEP_LP_CALLS \
                        HEEP_ID_MAX_CALLS \
                        HEEP_DA_CALLS \
                        HEEP_BOOTLOADER_CALLS \
                        " )
                        retVal = (* pHandlers->pHandler )( method_get, readingType, lBuf->data, &OR_PM_Attr_s );
                        if (retVal == eSUCCESS )
                        {
                           if( (OR_PM_Attr_s.rValLen != 0) || (OR_PM_Attr_s.rValTypecast == (uint8_t)ASCIIStringValue)
                              || (OR_PM_Attr_s.rValTypecast == (uint8_t)hexBinary))
                           {
                              eventCount_flags = 0; //flags, all zero for now
                           }
                        }
                        break;
                     }
                  }

                  rdgDataLength = sizeof(eventCount_flags) +
                     sizeof(rtype) +
                        sizeof(readingType) +
                           sizeof(enum_CIM_QualityCode) +
                              OR_PM_Attr_s.rValLen;
                  if( APP_MSG_MAX_DATA < (rdgDataLength + pBuf->x.dataLen) )
                  {  //adding this reading will cause us to overflow the HEEP msg buffer
                     //update the rdgQty, set status as Partial Content, and break the out of the for loop
                     pBuf->data[readingQtyIndex] = numReadings;
                     heepRespHdr.Method_Status   = (uint8_t)PartialContent;
                     break;
                  }

                  eventCount_flags = ( eventCount_flags | OR_PM_Attr_s.powerOfTen ); // set power of ten attribute, updated in handler
                  PACK_Buffer( sizeof(eventCount_flags) * 8, (uint8_t *)&eventCount_flags, &packCfg ); //Insert flags
                  pBuf->x.dataLen += sizeof(eventCount_flags);

                  rtype = OR_PM_Attr_s.rValLen;
                  rtype = (rtype << RDG_VAL_SIZE_U_SHIFT) & RDG_VAL_SIZE_U_MASK;
                  rtype |= (OR_PM_Attr_s.rValTypecast & RDG_VAL_SIZE_L_MASK);
                  PACK_Buffer( -(int16_t)(sizeof(rtype) * 8), (uint8_t *)&rtype, &packCfg ); //Insert size and reading value typecast
                  pBuf->x.dataLen += sizeof(rtype);

                  PACK_Buffer( -(int16_t)(sizeof(readingType) * 8), (uint8_t *)&readingType, &packCfg ); //pack reading type
                  pBuf->x.dataLen += sizeof(readingType);

                  if (eventCount_flags & 0x20)
                  {  //Pack quality code
                     enum_CIM_QualityCode qCode = CIM_QUALCODE_ERROR_CODE; //default

                     if (false == foundHandler)
                     {  //No handler found
                        qCode = CIM_QUALCODE_CODE_UNKNOWN_READING_TYPE;
                     }
                     else if( eAPP_WRITE_ONLY == retVal )
                     {
                        qCode = CIM_QUALCODE_CODE_READ_ONLY;
                     }
                     PACK_Buffer( (sizeof(qCode) * 8), (uint8_t *)&qCode, &packCfg ); //Add quality code
                     pBuf->x.dataLen += sizeof(qCode);
                     heepRespHdr.Method_Status = (uint8_t)PartialSuccess;  //At least one reading type failed.
                  }

                  //Pack data correctly
                  switch (OR_PM_Attr_s.rValTypecast)
                  {
                  case uintValue: //Fall-thru, Byte swap
                  case dateValue: //Fall-thru, Byte swap
                  case intValue:  //Fall-thru, Byte swap
                  case timeValue: //Byte swap
                     {
                        PACK_Buffer( -(int16_t)(OR_PM_Attr_s.rValLen * 8), lBuf->data, &packCfg ); //pack the value in the response buffer
                        break;
                     }
                  case ASCIIStringValue: //Fall-thru
                  case hexBinary: // Pack each byte
                     {
                        uint16_t cnt;
                        for(cnt = 0; cnt < (OR_PM_Attr_s.rValLen / sizeof(uint8_t)); cnt++)
                        {
                           PACK_Buffer( (sizeof(uint8_t) * 8), lBuf->data + (cnt * sizeof(uint8_t)), &packCfg ); //pack the value in the response buffer
                        }
                        break;
                     }
                  case uint16_list_type: //Byte swap 16-bit values
                     {
                        uint8_t cnt;
                        for(cnt = 0; cnt < (OR_PM_Attr_s.rValLen / sizeof(uint16_t)); cnt++)
                        {
                           PACK_Buffer( -(int16_t)(sizeof(uint16_t) * 8), lBuf->data + (cnt * sizeof(uint16_t)), &packCfg ); //pack the value in the response buffer
                        }
                        break;
                     }
                  case dateTimeValue:   //Fall-thru. It can be 4 or 8 byte long. Need 32-bit swaps
                  case int32_list_type: //Byte swap 32-bit values
                     {
                        uint8_t cnt;
                        for(cnt = 0; cnt < (OR_PM_Attr_s.rValLen / sizeof(int32_t)); cnt++)
                        {
                           PACK_Buffer( -(int16_t)(sizeof(int32_t) * 8), lBuf->data + (cnt * sizeof(int32_t)), &packCfg ); //pack the value in the response buffer
                        }
                        break;
                     }

                  default:
                     {
                        PACK_Buffer( (int16_t)(OR_PM_Attr_s.rValLen * 8), lBuf->data, &packCfg ); //pack the value in the response buffer
                        break;
                     }
                  }
                  pBuf->x.dataLen += OR_PM_Attr_s.rValLen;
                  numReadings++;
               } // for rdg qty

            } // while ( bytesProcessed < length )
            if(  heepRespHdr.Method_Status == (uint8_t)BadRequest )
            {
               (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
               //must free the allocated buffer since 'Tx Header Only' does NOT!
               BM_free( ( buffer_t * )pBuf );

            }
            else
            {
               (void)HEEP_MSG_Tx (&heepRespHdr, pBuf);
            }
            //the parameter buffer must be freed indpendently
            BM_free(lBuf);
         } //if buff != null
         else
         {  //one or more of the buffers failed to allocate
            //make sure we free all allocated memory
            DBG_printf( "ERROR: FAILED to allocated Value Buffer" );
            if( lBuf )
            {
               BM_free(lBuf);
            }
            if( pBuf )
            {
               BM_free(pBuf);
            }
         }
      }
   } //End of get request
   else if ( method_put == (enum_MessageMethod)heepReqHdr->Method_Status)
   {  //Process the write parameter
      EventRdgQty_u        qtys;          // For EventTypeQty and ReadingTypeQty
      ExWId_ReadingInfo_u  readingInfo;   // Reading Info for the reading Type and Value
      uint16_t             bits=0;        // Keeps track of the bit position within the Bitfields
      uint16_t             bytes=0;       // Keeps track of byte position within Bitfields
      uint16_t             bytesLeft = length;
      uint16_t             readingQtyIndex = 0;

      qtys.EventRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS);
      bytes               += EVENT_AND_READING_QTY_SIZE;
      bytesLeft           -= EVENT_AND_READING_QTY_SIZE;

      if (qtys.bits.rdgQty == 0 || qtys.bits.eventQty > 0 || length < MIN_PAYLOAD_ORPM_WRITE_BYTES )
      { //Invalid request, send the header back
         heepRespHdr.Method_Status = (uint8_t)BadRequest;
         (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
      }
      else
      {
         buffer_t      *pBuf;     // pointer to message

         lBuf = BM_alloc( MAX_OR_PM_WRITE_SIZE );
         pBuf = BM_alloc( APP_MSG_MAX_DATA ); //Allocate a big buffer to accommodate worst case payload.
         if ( pBuf != NULL && lBuf != NULL )
         {
            meterReadingType readingType;    //Reading Type
            returnStatus_t retVal;           //status
            uint8_t       i;                 //loop counter
            bool          foundHandler;      // flag, handler found or not
            pack_t        packCfg;           // Struct needed for PACK functions
            uint32_t      msgTime;           // Time converted to seconds since epoch
            sysTimeCombined_t currentTime;   // Get current time
            uint8_t       eventflags;        // Flags
            uint8_t       eventCount;        // Counter readings
            uint16_t      rtype;

            PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );

            //Add timestamp
            (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
            msgTime = (uint32_t)(currentTime / TIME_TICKS_PER_SEC);
            PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&msgTime, &packCfg );
            pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;
            readingQtyIndex = pBuf->x.dataLen;

            eventCount = 0; //Fill this later, if error processing any entry
            PACK_Buffer( sizeof(eventCount) * 8, (uint8_t *)&eventCount, &packCfg );
            pBuf->x.dataLen += sizeof(eventCount);

            //reset message pointer to the beginning so that we can loop through all mini headers
            bytesLeft = length;
            bytes = 0;
            bits = 0;

            while( bytesLeft )
            {  // while we still have bytes to read from the write request
               if( heepRespHdr.Method_Status == (uint8_t)BadRequest )
               {  //we must have broken out of the for loop, now break out of while() loop
                  break;
               }
               else if(bytesLeft < ( EVENT_AND_READING_QTY_SIZE ) )
               {  //we still have bytes to read but not enought for a min header abort request
                  heepRespHdr.Method_Status = (uint8_t)BadRequest;
                  break;
               }
               else
               {  /* We have just completed a full set of parameter writes.  If there is any additional informaion
                  that needs to be read from the request message, the next byte will be a reading quantity value.
                  This bytes will let us know how many more readings to process in the next set of writes. */
                  qtys.EventRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS);
                  bytes           += EVENT_AND_READING_QTY_SIZE;
                  bytesLeft       -= EVENT_AND_READING_QTY_SIZE;

                  if( qtys.bits.rdgQty == 0 || qtys.bits.eventQty > 0 )
                  {  /* If the reading quantity is 0, we do not know how many more readings to process.
                     Break out of loop so response can be sent to head end. */
                     break;
                  }
               }


               for (i =0; (i< qtys.bits.rdgQty) ; i++ ) //process all reading types
               {
                  foundHandler = false;
                  if( bytesLeft < (READING_INFO_SIZE + READING_TYPE_SIZE) )
                  {
                     heepRespHdr.Method_Status = (uint8_t)BadRequest;
                     break;
                  }
                  //Todo: process the flags later, if needed. Expecting ReadingsValueSizeInBytes and ReadingsValueTypecast
                  (void)memcpy( &readingInfo.bytes[0], &((uint8_t *)payloadBuf)[bytes], sizeof(readingInfo.bytes));
                  bits       += READING_INFO_SIZE_BITS;
                  bytes      += READING_INFO_SIZE;
                  bytesLeft  -= READING_INFO_SIZE;
                  readingType = (meterReadingType)UNPACK_bits2_uint16(payloadBuf, &bits, sizeof(readingType) * 8);
                  bytes      += READING_TYPE_SIZE;
                  bytesLeft  -= READING_TYPE_SIZE;

                  OR_PM_Attr_s.rValLen      = readingInfo.bits.RdgValSizeU;  //lint !e530 readingInfo readingInfo populated by memcpy() above
                  OR_PM_Attr_s.rValLen      = ((OR_PM_Attr_s.rValLen << RDG_VAL_SIZE_U_SHIFT) & RDG_VAL_SIZE_U_MASK) | readingInfo.bits.RdgValSize;/*lint !e530 */
                  OR_PM_Attr_s.rValTypecast = readingInfo.bits.typecast; //lint !e530 readingInfo readingInfo populated by memcpy() above
                  OR_PM_Attr_s.powerOfTen   = readingInfo.bits.pendPowerof10;//lint !e530 readingInfo readingInfo populated by memcpy() above
                  if (readingInfo.bits.tsPresent)  //lint !e530 readingInfo readingInfo populated by memcpy() above
                  {
                     bits      += READING_TIME_STAMP_SIZE_BITS;   //Skip over since we do not use/need it
                     bytes     += READING_TIME_STAMP_SIZE;
                     bytesLeft -= READING_TIME_STAMP_SIZE;
                  }
                  if(bytesLeft < OR_PM_Attr_s.rValLen)
                  {  //not enough bytes left for the given value lenth
                     heepRespHdr.Method_Status = (uint8_t)BadRequest;
                     break;
                  }

                  (void)memset(lBuf->data, 0, lBuf->bufMaxSize);
                  (void)memcpy(lBuf->data, &((uint8_t *)payloadBuf)[bytes], OR_PM_Attr_s.rValLen);

                  //Byte-swap, if required
                  switch (OR_PM_Attr_s.rValTypecast)
                  {
                     case uintValue: //Fall-thru, Byte swap
                     case dateValue: //Fall-thru, Byte swap
                     case intValue:  //Fall-thru, Byte swap
                     case timeValue: //Byte swap
                     {
                        Byte_Swap(lBuf->data, (uint8_t)OR_PM_Attr_s.rValLen); //Byte swap
                        break;
                     }
                     case uint16_list_type: //Byte swap 16-bit values
                     {
                        uint8_t cnt;
                        for(cnt = 0; cnt < (OR_PM_Attr_s.rValLen / sizeof(uint16_t)); cnt++)
                        {
                           Byte_Swap(&lBuf->data[cnt * sizeof(uint16_t)], (uint8_t)sizeof(uint16_t));
                        }
                        break;
                     }
                     case dateTimeValue:   //Fall-thru. It can be 4 or 8 byte long. Need 32-bit swaps
                     case int32_list_type: //Byte swap 32-bit values
                     {
                        uint8_t cnt;
                        for(cnt = 0; cnt < (OR_PM_Attr_s.rValLen / sizeof(int32_t)); cnt++)
                        {
                           Byte_Swap(&lBuf->data[cnt * sizeof(uint32_t)], (uint8_t)sizeof(int32_t));
                        }
                        break;
                     }
                     default:
                     {
                        break;
                     }
                  }

                  for( pHandlers = (OR_PM_HandlersDef *)OR_PM_Handlers; pHandlers->pHandler != NULL;  pHandlers++ )
                  {
                     if (pHandlers->rType == readingType)
                     {  //found handler
                        foundHandler = true;
_Pragma ( "calls = \
                        HEEP_COMMON_CALLS \
                        HEEP_DCU_CALLS \
                        HEEP_EP_CALLS \
                        HEEP_DTLS_CALLS \
                        HEEP_MTLS_CALLS \
                        HEEP_NOT_LC_NOT_DA_CALLS \
                        HEEP_LP_CALLS \
                        HEEP_ID_MAX_CALLS \
                        HEEP_DA_CALLS \
                        HEEP_BOOTLOADER_CALLS \
                        " )
                        retVal = (* pHandlers->pHandler )( method_put, readingType, lBuf->data, &OR_PM_Attr_s );  //TODO: Oct 22 2018 SMG, Most of the handlers are not qualifying the typecast
                        break;
                     }
                  }

                  bits  += (OR_PM_Attr_s.rValLen * 8); //lint !e734 Max bits allowed in pack routine is less than uint16
                  bytes += OR_PM_Attr_s.rValLen;
                  bytesLeft -= OR_PM_Attr_s.rValLen;

                  if ( (false == foundHandler) || (true == foundHandler && retVal != eSUCCESS) )  //lint !e644 retVal will be initialized
                  { //Error case, add quality code

                     if( eventCount == READING_QTY_MAX )
                     {  /* We cannot fit anymore readings into the reponse.  Save the current number of readings,
                           reset the reading counter, start a new bit structure to be chained to the end of the
                           previous full bit structure. */
                        pBuf->data[readingQtyIndex] = eventCount; //Insert the correct number of readings

                        //Add timestamp for next group in response chain
                        PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&msgTime, &packCfg );
                        pBuf->x.dataLen += VALUES_INTERVAL_END_SIZE;
                        readingQtyIndex = pBuf->x.dataLen;

                        eventCount = 0; //reinitialize the event count
                        PACK_Buffer( sizeof(eventCount) * 8, (uint8_t *)&eventCount, &packCfg );
                        pBuf->x.dataLen += sizeof(eventCount);
                     }

                     enum_CIM_QualityCode qCode = CIM_QUALCODE_ERROR_CODE; //default

                     eventflags = 0x20;   //This is NOT an event, but is the Reading "header" with only ReadingQualityPresent set
                     PACK_Buffer( sizeof(eventflags) * 8, (uint8_t *)&eventflags, &packCfg ); //Insert flags
                     pBuf->x.dataLen += sizeof(eventflags);
                     rtype = OR_PM_Attr_s.rValTypecast;  //Default response: Value size of zero and typecast from request
                     PACK_Buffer( -(int16_t)(sizeof(rtype) * 8), (uint8_t *)&rtype, &packCfg ); //Insert size and reading value typecast
                     pBuf->x.dataLen += sizeof(rtype);
                     PACK_Buffer( -(int16_t)(sizeof(readingType) * 8), (uint8_t *)&readingType, &packCfg ); //pack reading type
                     pBuf->x.dataLen += sizeof(readingType);
                     if (false == foundHandler)
                     {  //No handler found
                        qCode = CIM_QUALCODE_CODE_UNKNOWN_READING_TYPE;
                     }
                     else
                     {  //Convert from APP error codes to QUALITY CODES in one place
                        if (eAPP_INVALID_VALUE == retVal)
                        {  //Couldn't this also happen on read only, or size too big?
                           qCode = CIM_QUALCODE_DATA_OUTSIDE_RANGE;
                        }
                        else if (eAPP_INVALID_TYPECAST == retVal)
                        {  // wrong typcast specified in write request
                           qCode = CIM_QUALCODE_CODE_TYPECAST_MISMATCH;
                        }
                        else if( eAPP_READ_ONLY == retVal )
                        {  // tried to read a write only or write a read only parameter
                           qCode = CIM_QUALCODE_CODE_READ_ONLY;
                        }
                        else if( eAPP_OVERFLOW_CONDITION == retVal )
                        { // tried to write a number of bytes larger than allocated for parameter
                           qCode = CIM_QUALCODE_CODE_OVERFLOW;
                        }
                     }
                     PACK_Buffer( (sizeof(qCode) * 8), (uint8_t *)&qCode, &packCfg ); //Add quality code
                     pBuf->x.dataLen += sizeof(qCode);
                     heepRespHdr.Method_Status = (uint8_t)PartialSuccess;  //At least one reading type failed.
                     eventCount++;
                  }
                  else //Foundhandler and retVal = eSUCCESS
                  {
                     if ( appSecurityAuthMode == readingType )
                     {  //Get the updated appSecurityAuthMode. Special handling needed
                        uint8_t  securityMode;
                        ( void )APP_MSG_SecurityHandler( method_get, appSecurityAuthMode, &securityMode, NULL );
                        heepRespHdr.appSecurityAuthMode = securityMode;
                     }
                  }
               } //end of for() rdg qty
               pBuf->data[readingQtyIndex] = eventCount; //Insert the correct number of readings

            } // while ( bytesLeft )

            if( heepRespHdr.Method_Status == (uint8_t)BadRequest )
            {
               //must free the allocated buffer since Tx Header Only does NOT!
               (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
               BM_free( (buffer_t *)pBuf);
            }
            else
            {
               (void)HEEP_MSG_Tx (&heepRespHdr, pBuf);
            }

         } //if buff != null
         else
         {  /* We were unable to acquire buffers to perform the write task, send header back to head end indicating
               service is unavailable. */
            heepRespHdr.Method_Status = (uint8_t)ServiceUnavailable;
            (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
            if( pBuf )
            { //one buffer may have been allocated and the other failed, make sure to
               //free in either case
               BM_free(pBuf);
            }
         }

         if( lBuf != NULL )
         {  // All writes have been completed, free the value buffer
            BM_free(lBuf);
         }
      }
   }
}


/*!
 **********************************************************************
 *
 * \fn HEEP_Put_Boolean
 *
 * \brief Put a Boolean value into the HEEP message
 *
 * \details This will add a reading and increment the ReadingQty
 *
 * \param p              pointer to readings structure
 * \param ReadingType    reading type to assign
 * \param value          value to add to the reading
 *
 * \return  void
 *
 **********************************************************************
 */
void HEEP_Put_Boolean(
   struct   readings_s *p,
   meterReadingType ReadingType,
   uint8_t  value)
{
   ReadingHeader_Set(p, Boolean, sizeof(value), ReadingType );

   (void) memcpy(&p->pData[p->size], (uint8_t*) &value, sizeof(value));
   p->size += sizeof(value);

   ReadingQty_Inc(p);

}


/*!
 **********************************************************************
 *
 * \fn HEEP_Put_HexBinary
 *
 * \brief Put a HexBinary  value into the HEEP message
 *
 * \details This will add a reading and increment the ReadingQty
 *
 * \param p              pointer to readings structure
 * \param ReadingType    reading type to assign
 * \param value          value to add to the reading
 * \param size           size of payload
 *
 * \return  void
 *
 **********************************************************************
 */
void HEEP_Put_HexBinary (
   struct readings_s *p,
   meterReadingType ReadingType,
   uint8_t  const Data[],
   uint16_t Size)
{

   ReadingHeader_Set(p, hexBinary, Size, ReadingType );

   (void) memcpy(&p->pData[p->size], Data, Size);
   p->size += Size;

   ReadingQty_Inc(p);

}

/*!
 **********************************************************************
 *
 * \fn HEEP_Put_DateTimeValue
 *
 * \brief Put a DateTime value into the HEEP message
 *
 * \details This will add a reading and increment the ReadingQty
 *
 * \param p              pointer to readings structure
 * \param ReadingType    reading type to assign
 * \param value          value to add to the reading
 *
 * \return  void
 *
 **********************************************************************
 */
void HEEP_Put_DateTimeValue(
   struct readings_s *p,
   meterReadingType ReadingType,
   TIMESTAMP_t const *DateTime)
{
   uint8_t Size = 8;
   uint32_t value;

   ReadingHeader_Set(p, dateTimeValue, Size, ReadingType );

   // Add the seconds since 1970-01-01T00:00:00
   value = htonl(DateTime->seconds );
   (void) memcpy(&p->pData[p->size], (uint8_t*) &value, sizeof(value));
   p->size += sizeof(value);

// if( Size == 8 )
   {
      // Add the fractional seconds (in units of 1/4294967296 (2^32) seconds) .
      value = htonl(DateTime->QFrac );
      (void) memcpy(&p->pData[p->size], (uint8_t*) &value, sizeof(value));
      p->size += sizeof(value);
   }

   ReadingQty_Inc(p);

}

/*!
 **********************************************************************
 *
 * \fn HEEP_Put_U8
 *
 * \brief Put a U8 value into the HEEP message
 *
 * \details This will add a reading and increment the ReadingQty
 *
 * \param p              pointer to readings structure
 * \param ReadingType    reading type to assign
 * \param value          value to add to the reading
 *
 * \return  void
 *
 **********************************************************************
 */
void HEEP_Put_U8(
   struct   readings_s *p,
   meterReadingType ReadingType,
   uint8_t  value)
{
   ReadingHeader_Set(p, uintValue, sizeof(value), ReadingType );

   (void) memcpy(&p->pData[p->size], (uint8_t*) &value, sizeof(value));
   p->size += sizeof(value);

   ReadingQty_Inc(p);

}



/*!
 **********************************************************************
 *
 * \fn HEEP_Put_U16
 *
 * \brief Put a U16 value into the HEEP message
 *
 * \details This will add a reading and increment the ReadingQty
 *
 * \param p              pointer to readings structure
 * \param ReadingType    reading type to assign
 * \param value          value to add to the reading
 * \param powerOfTen     power of ten of the reading
 *
 * \return  void
 *
 **********************************************************************
 */
void HEEP_Put_U16(
   struct   readings_s *p,
   meterReadingType ReadingType,
   uint16_t value,
   uint8_t powerOfTen)
{
   // Convert from host to network byte order
   value = htons((int16_t) value);

   uint8_t *pValue = (uint8_t *)&value;

   uint8_t size = getMinSize(pValue, sizeof(value));

   // Set the header info
   ReadingHeader_Set(p, uintValue, size, ReadingType );

   // Set the PendingPowerOf10 (3 lsb's of first byte of Reading Header)
   p->pData[p->size - 5] |= ( uint8_t )( powerOfTen & 0x07 );

   // copy the data
   (void) memcpy(&p->pData[p->size], &pValue[sizeof(value)-size], size);
   p->size += size;

   ReadingQty_Inc(p);
}

/*!
 **********************************************************************
 *
 * \fn HEEP_Put_I16
 *
 * \brief Put a I16 value into the HEEP message
 *
 * \details This will add a reading and increment the ReadingQty
 *
 * \param p              pointer to readings structure
 * \param ReadingType    reading type to assign
 * \param value          value to add to the reading
 * \param powerOfTen     power of ten of the reading
 *
 * \return  void
 *
 **********************************************************************
 */
void HEEP_Put_I16(
   struct   readings_s *p,
   meterReadingType ReadingType,
   int16_t value,
   uint8_t powerOfTen)
{
   // Convert from host to network byte order
   value = (int16_t)htons( value );

   uint8_t *pValue = (uint8_t *)&value;

   uint8_t size = getMinSize(pValue, sizeof(value));

   // Set the header info
   ReadingHeader_Set(p, intValue, size, ReadingType );

   // Set the PendingPowerOf10 (3 lsb's of first byte of Reading Header)
   p->pData[p->size - 5] |= ( uint8_t )( powerOfTen & 0x07 );

   // copy the data
   (void) memcpy(&p->pData[p->size], &pValue[sizeof(value)-size], size);
   p->size += size;

   ReadingQty_Inc(p);
}

/*!
 **********************************************************************
 *
 * \fn HEEP_Put_U32
 *
 * \brief Put a U32 value into the HEEP message
 *
 * \details This will add a reading and increment the ReadingQty
 *
 * \param p              pointer to readings structure
 * \param ReadingType    reading type to assign
 * \param value          value to add to the reading
 *
 * \return  void
 *
 **********************************************************************
 */
void HEEP_Put_U32(
   struct readings_s *p,
   meterReadingType ReadingType,
   uint32_t value)
{
   // Convert from host to network byte order
   value = htonl(value);

   uint8_t *pValue = (uint8_t *)&value;

   uint8_t size = getMinSize(pValue, sizeof(value));

   // Set the header info
   ReadingHeader_Set(p, uintValue, size, ReadingType );

   // copy the data
   (void) memcpy(&p->pData[p->size], &pValue[sizeof(value)-size], size);
   p->size += size;

   ReadingQty_Inc(p);

}


/*!
 **********************************************************************
 *
 * \fn HEEP_Put_U64
 *
 * \brief Put a U64 value into the HEEP message
 *
 * \details This will add a reading and increment the ReadingQty
 *
 * \param p              pointer to readings structure
 * \param ReadingType    reading type to assign
 * \param value          value to add to the reading
 *
 * \return  void
 *
 **********************************************************************
 */
void HEEP_Put_U64(
   struct readings_s *p,
   meterReadingType ReadingType,
   uint64_t value)
{

   // Convert from host to network byte order
   value = htonll(value);

   uint8_t *pValue = (uint8_t *)&value;

   uint8_t size = getMinSize(pValue, sizeof(value));

   // Set the header info
   ReadingHeader_Set(p, uintValue, size, ReadingType );

   // copy the data
   (void) memcpy(&p->pData[p->size], &pValue[sizeof(value)-size], size);
   p->size += size;

   ReadingQty_Inc(p);
}

#if OTA_CHANNELS_ENABLED
/*!
 **************************************************************************************************
 *
 * \fn HEEP_Put_ui_list
 *
 * \brief Put a ui_list into the HEEP message
 *
 * \details This function is used to add a list of uint16 values to a heep message
 *
 * \param p              pointer to readings structure
 * \param ReadingType    reading type to assign
 * \param value          a list of uint16_t values to add to the reading

 * \return  void
 *
 **************************************************************************************************
 */
void HEEP_Put_ui_list(
   struct readings_s *p,
   uint16_t ReadingType,    /* */
   struct ui_list_s const *ui_list)
{
   uint8_t  Type = 8;
   uint16_t Size = ui_list->num_elements * 2;

   uint8_t* pPayload    = &p->pData[p->size];

   uint8_t num_bytes = 0;

   *pPayload++ = 0;
   *pPayload++ = (uint8_t) (Size >> 4);
   *pPayload++ = (Size & 0x0F) << 4 | (Type & 0x0F);
   *pPayload++ = (ReadingType & 0xFF00) >> 8;
   *pPayload++ = (ReadingType & 0x00FF);
   num_bytes+=5;

   for (int i=0; i<ui_list->num_elements; i++)
   {
      *pPayload++ = ui_list->data[i] >> 8;
      *pPayload++ = ui_list->data[i] & 0xff;
//      *pPayload++ = ui_list->data[i];
      num_bytes+=2;
   }

   p->size += num_bytes;

   ReadingQty_Inc(p);

}
#endif

/*!
 **********************************************************************
 *
 * \fn HEEP_Add_Reading
 *
 * \brief Adds a reading header to the readings
 *
 * \details This will have a size of 0 bytes.
 *
 * \param p              pointer to readings structure
 * \param ReadingType    reading type to assign
 *
 * \return  void
 *
 **********************************************************************
 */
void HEEP_Add_ReadingType(
   struct readings_s *p,
   meterReadingType ReadingType)
{

   ReadingHeader_Set(p, uintValue, 0, ReadingType );

   ReadingQty_Inc(p);

}

/*!
 **********************************************************************
 *
 * \fn HEEP_ReadingsInit
 *
 * \brief Initialize the CompactMeterRead Header
 *
 * \details This function will initialize the readings structure
 *          Sets the pointer to the buffer provided
 *          Sets the size to 0
 *          Adds the current time to the buffer
 *          Sets the Reading Quantity = 0
 *          Sets the Event   Quantity = 0
 *
 * \param reading        pointer to readings structure
 * \param rtype          specified if there is a time stamp
 * \param buffer         pointer to the buffer
 *
 * \return  void
 *
 ***********************************************************************
 */
void HEEP_ReadingsInit(
   struct readings_s *readings,
   uint8_t  rtype,
   uint8_t  *buffer)
{
   readings->size = 0;
   readings->pData = buffer;


   if (rtype == 1)
   {
      TIMESTAMP_t CurrentTime;

      /* Get current time in seconds and fractional seconds */
      (void) TIME_UTIL_GetTimeInSecondsFormat(&CurrentTime);

      // Add current timestamp ( 32 bits)
      uint32_t value = htonl(CurrentTime.seconds);
      (void) memcpy(&readings->pData[readings->size], (uint8_t*) &value, sizeof(value));
      readings->size += sizeof(value);
   }

   // Insert the event quantity and reading count ( 8 bits ). This will be updated later!
   readings->pData[readings->size++] = 0;

   readings->rtype = rtype;

}


/*!
 **********************************************************************
 *
 * \fn ReadingQty_Inc
 *
 * \brief This increments the reading quantity field in the header.
 *
 * \details The ReadingQty is a 4 bit field
 *
 * \param p pointer to readings structure
 *
 * \return  void
 *
 **********************************************************************
 */
/*lint -esym(818,p) could be ponter to const   */
static void ReadingQty_Inc(struct readings_s *p)
{
   uint8_t *pReadingQty;

   if (p->rtype == 1)
   {
      pReadingQty = &p->pData[4];
   }
   else{
      pReadingQty = &p->pData[0];
   }

   uint8_t readingQty;
   readingQty =   (*pReadingQty & 0x0F);
   readingQty++;

   // Write the updated value
   *pReadingQty = (*pReadingQty & 0xF0) | readingQty;
}
/*lint +esym(818,p) could be ponter to const   */

/*!
 **********************************************************************************************************************
 *
 * \fn ReadingHeader_Set
 *
 * \brief This function updates the reading header.
 *
 * \details If the size > 0, add the Size and Type (3 bytes).
 *          Then always add the ReadingType (2 bytes).
 *
 * \param p           - pointer to a reading_s
 * \param Type        - ReadingsValueTypecast
 * \param Size        - The size of the element to add
 * \param ReadingType - meterReadingType
 *
 * \return  void
 *
 **********************************************************************************************************************
 */
static void ReadingHeader_Set(
   struct   readings_s   *p,
   ReadingsValueTypecast Type,
   uint16_t              Size,
   meterReadingType      ReadingType )
{
   uint8_t* pPayload    = &p->pData[p->size];

   if ( Size > 0 )
   {
      *pPayload++ = 0;
      *pPayload++ = (Size & 0x0FF0) >> 4;
      *pPayload++ = (Size & 0x000F) << 4 | ((uint8_t) Type & 0x0F);
      p->size += 3;
   }

   // Add the reading type ( 16 bits )
   *pPayload++ = ((uint16_t) ReadingType >> 8) & 0xFF;
   *pPayload   = ((uint16_t) ReadingType     ) & 0xFF;
   p->size += 2;
}


/*!
 **********************************************************************
 *
 * \fn getMinSize
 *
 * \brief Computes the minimum number of bytes required to send a uint value
 *
 * \details
 *
 * \param pData  - Pointer to the data
 * \param size   - The number of bytes in the data
 *
 * \return  uint64_t
 *
 **********************************************************************
 */
static uint8_t getMinSize(uint8_t const *pData, uint8_t size)
{
   // Only send the values starting with first non zero value
   uint8_t i;

   for ( i = 0; i < ( size-1 ); i++ )
   {
      if ( pData[i] != 0 ) /*lint !e2662 pointer arithmetic on pointer that may not refer to an array */
      {
         break;
      }
   }
   return ( uint8_t )(size - i);
}


/*!
 **********************************************************************
 *
 * \fn htonll
 *
 * \brief Converts a uint64_t from host to network byte order
 *
 * \details
 *
 * \param value
 *
 * \return  uint64_t
 *
 **********************************************************************
 */
uint64_t htonll(uint64_t value)
{
   uint32_t high_part = htonl((uint32_t)(value >> 32));
   uint32_t low_part  = htonl((uint32_t)(value & 0xFFFFFFFFLL));
   return (((uint64_t)low_part) << 32) | high_part;
}

/*!
 **********************************************************************
 *
 * \fn ntohll
 *
 * \brief Converts a uint64_t from network to host byte order
 *
 * \details
 *
 * \param value
 *
 * \return  uint64_t
 *
 **********************************************************************
 */
uint64_t ntohll(uint64_t value)
{
   uint32_t high_part = htonl((uint32_t)(value >> 32));
   uint32_t low_part  = htonl((uint32_t)(value & 0xFFFFFFFFLL));
   return (((uint64_t)low_part) << 32) | high_part;
}


/***********************************************************************************************************************

   Function Name: HEEP_util_OR_PM_Handler

   Purpose: Get/Set parameters

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t HEEP_util_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case flashSecurityEnabled :
         {
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
            PartitionData_t const *pImagePTbl; // Image partition for PGM memory
            dSize destAddr = 0x408u; // Address in partition of current flash security status
            uint8_t fprot[ 5 ] = {0}; // Values read from partition
            static const uint8_t LockMask[5] =   {0x00u, 0x80u, 0xFFu, 0xFFu, 0xFFu}; // security enabled mask

            if ( eSUCCESS == PAR_partitionFptr.parOpen( &pImagePTbl, ePART_APP_CODE, 0L ) )
            {
               (void)PAR_partitionFptr.parRead( fprot, destAddr, ( lCnt ) sizeof( fprot ), pImagePTbl );
            }

            *(uint8_t *)value = memcmp( fprot, LockMask, sizeof( fprot ) ) == 0 ? 1 : 0;
            retVal = eSUCCESS;
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(uint8_t);
               attr->rValTypecast = (uint8_t)Boolean;
            }
#elif ( MCU_SELECTED == RA6E1 )
            // TODO: RA6E1 Flash security enabled configuration
#else
#define FLASH_SECURITY_DISABLED (2)  /* All other values indicate Flash Security is enabled */
            *(uint8_t *)value = TRUE;  //Assume Flash Security is enabled
            retVal             = eSUCCESS;
            attr->rValLen      = (uint16_t)sizeof(uint8_t);
            attr->rValTypecast = (uint8_t)Boolean;
            if ( FLASH_SECURITY_DISABLED == FTFE_FSEC_SEC(FTFE_FSEC) )
            {
               *(uint8_t *)value  = FALSE;
            }
#endif
            break;
         }
         case debugPortEnabled :
         {
            static uint8_t dbgEnabled;
            retVal              = eSUCCESS;
            dbgEnabled          = ( DBG_PortTimer_Get() == 0 ) ? false : true;
            *(uint8_t *)value   = dbgEnabled;
            if (attr != NULL)
            {
              attr->rValLen      = (uint16_t)sizeof(uint8_t);
              attr->rValTypecast = (uint8_t)Boolean;
            }
           break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   return retVal;
}


/***********************************************************************************************************************

   Function Name: NWK_OR_PM_Handler

   Purpose: Get/Set parameters related to NWK

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t NWK_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   NWK_SetConf_t  confirm;
   NWK_GetConf_t  GetConf;
   returnStatus_t retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      GetConf.eStatus = eNWK_GET_UNSUPPORTED;
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case ipHEContext : // get ipHEContext paramter
         {
            if ( sizeof(GetConf.val.ipHEContext) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = NWK_GetRequest( eNwkAttr_ipHEContext );
               if ( GetConf.eStatus == eNWK_GET_SUCCESS ) {
                  *(uint8_t *)value = GetConf.val.ipHEContext;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.ipHEContext);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case iPifInDiscards :
         case iPifOutDiscards :
         {
            if ( sizeof(GetConf.val.ipIfInDiscards) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               switch ( id )  /*lint !e788 not all enums used within switch */
               {
                  case iPifInDiscards :
                     GetConf = NWK_GetRequest( eNwkAttr_ipIfInDiscards );
                     break;
                  case iPifOutDiscards :
                     GetConf = NWK_GetRequest( eNwkAttr_ipIfOutDiscards );
                     break;
                  default:
                     break;
               }  /*lint !e788 not all enums used within switch */
               if ( GetConf.eStatus == eNWK_GET_SUCCESS ) {
                  *(uint32_t *)value = GetConf.val.ipIfInDiscards;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.ipIfInDiscards);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case iPifInErrors :
         case iPifInOctets :
         case iPifOutErrors :
         case iPifOutOctets :
         {
            if ( sizeof(GetConf.val.ipIfInErrors) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               switch ( id )  /*lint !e788 not all enums used within switch */
               {
                  case iPifInErrors :
                     GetConf = NWK_GetRequest( eNwkAttr_ipIfInErrors );
                     break;
                  case iPifInOctets :
                     GetConf = NWK_GetRequest( eNwkAttr_ipIfInOctets );
                     break;
                  case iPifOutErrors :
                     GetConf = NWK_GetRequest( eNwkAttr_ipIfOutErrors );
                     break;
                  case iPifOutOctets :
                     GetConf = NWK_GetRequest( eNwkAttr_ipIfOutOctets );
                     break;
                  default:
                     break;
               }  /*lint !e788 not all enums used within switch */
               if ( GetConf.eStatus == eNWK_GET_SUCCESS ) {
                  (void)memcpy( (uint8_t*)value, (uint8_t*)GetConf.val.ipIfInErrors, sizeof(GetConf.val.ipIfInErrors) );
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.ipIfInErrors);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case iPifInMulticastPkts :
         case iPifInUcastPkts :
         case iPifOutMulticastPkts :
         case iPifOutUcastPkts :
         {
            if ( sizeof(GetConf.val.ipIfInMulticastPkts) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               switch ( id )  /*lint !e788 not all enums used within switch */
               {
                  case iPifInMulticastPkts :
                     GetConf = NWK_GetRequest( eNwkAttr_ipIfInMulticastPkts );
                     break;
                  case iPifInUcastPkts :
                     GetConf = NWK_GetRequest( eNwkAttr_ipIfInUcastPkts );
                     break;
                  case iPifOutMulticastPkts :
                     GetConf = NWK_GetRequest( eNwkAttr_ipIfOutMulticastPkts );
                     break;
                  case iPifOutUcastPkts :
                     GetConf = NWK_GetRequest( eNwkAttr_ipIfOutUcastPkts );
                     break;
                  default:
                     break;
               }  /*lint !e788 not all enums used within switch */
               if ( GetConf.eStatus == eNWK_GET_SUCCESS ) {
                  (void)memcpy( (uint8_t*)value, (uint8_t*)GetConf.val.ipIfInMulticastPkts, sizeof(GetConf.val.ipIfInMulticastPkts) );
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.ipIfInMulticastPkts);
                     attr->rValTypecast = (uint8_t)hexBinary;
                  }
               }
            }
            break;
         }
         case iPifLastResetTime :
         {
            if ( sizeof(GetConf.val.ipIfLastResetTime.seconds) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = NWK_GetRequest( eNwkAttr_ipIfLastResetTime );
               if ( GetConf.eStatus == eNWK_GET_SUCCESS ) {
                  *(uint32_t *)value = GetConf.val.ipIfLastResetTime.seconds;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.ipIfLastResetTime.seconds);
                     attr->rValTypecast = (uint8_t)dateTimeValue;
                  }
               }
            }
            break;
         }
#if (EP == 1)
         case invalidAddressMode :
         {
           if ( sizeof(GetConf.val.invalidAddrModeCount) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = NWK_GetRequest( eNwkAttr_invalidAddrModeCount );
               if ( GetConf.eStatus == eNWK_GET_SUCCESS ) {
                  *(uint32_t *)value = GetConf.val.invalidAddrModeCount;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.invalidAddrModeCount);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
           break;
         }
#endif
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
      if( GetConf.eStatus == eNWK_GET_SUCCESS )
      {
         retVal = eSUCCESS;
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      confirm.eStatus = eNWK_SET_UNSUPPORTED;
      retVal = eFAILURE;
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case ipHEContext : // set ipHEContext paramter
         {
            confirm = NWK_SetRequest( eNwkAttr_ipHEContext, (uint16_t *)value);
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
      if( confirm.eStatus == eNWK_SET_SUCCESS )
      {
         retVal = eSUCCESS;
      }
   }
   return ( retVal );
}


/***********************************************************************************************************************

   Function Name: MAC_OR_PM_Handler

   Purpose: Get/Set parameters over the air related to the MAC

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t MAC_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   MAC_GetConf_t   GetConf; // used to store MAC config values
   MAC_SetConf_t   confirm;
#if ( DCU == 1 )
   MAC_SetReq_t    SetReq;
#endif
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      GetConf.eStatus = eMAC_GET_UNSUPPORTED;
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case macNetworkId :
         {
            if ( sizeof(uint8_t) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_NetworkId );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint16_t *)value = GetConf.val.NetworkId;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.NetworkId );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case macChannelSetsSTAR :
         {
            if ( sizeof(GetConf.val.ChannelSets) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               uint8_t channelSetsCount; // used to store number of sets

               GetConf = MAC_GetRequest( eMacAttr_ChannelSetsCountSTAR ); // get current number of sets
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  channelSetsCount = GetConf.val.ChannelSetsCount;
                  GetConf = MAC_GetRequest( eMacAttr_ChannelSetsSTAR ); // get the current Channel Set values
                  if( GetConf.eStatus == eMAC_GET_SUCCESS ) // verify we successfully acquired channel sets
                  {
                     // copy the number of channels set values
                     (void)memcpy((uint8_t*)value, (uint8_t*)GetConf.val.ChannelSets, sizeof( CH_SETS_t ) * channelSetsCount);
                     if (attr != NULL)
                     {
                        attr->rValLen = (uint16_t)(sizeof( CH_SETS_t ) * channelSetsCount );
                        attr->rValTypecast = (uint8_t)uint16_list_type;
                     }
                  }
               }
            }
            break;
         }
         case macChannelSets :
         {
            if ( sizeof(GetConf.val.ChannelSets) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               uint8_t channelSetsCount; // used to store number of sets

               GetConf = MAC_GetRequest( eMacAttr_ChannelSetsCountSRFN ); // get current number of sets
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  channelSetsCount = GetConf.val.ChannelSetsCount;
                  GetConf = MAC_GetRequest( eMacAttr_ChannelSetsSRFN ); // get the current Channel Set values
                  if( GetConf.eStatus == eMAC_GET_SUCCESS ) // verify we successfully acquired channel sets
                  {
                     // copy the number of channels set values
                     (void)memcpy((uint8_t*)value, (uint8_t*)GetConf.val.ChannelSets, sizeof( CH_SETS_t ) * channelSetsCount);
                     if (attr != NULL)
                     {
                        attr->rValLen = (uint16_t)(sizeof( CH_SETS_t ) * channelSetsCount );
                        attr->rValTypecast = (uint8_t)uint16_list_type;
                     }
                  }
               }
            }
            break;
         }

         case macChannelSetsCount :
         {
            if ( sizeof(uint8_t) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               GetConf = MAC_GetRequest( eMacAttr_ChannelSetsCountSRFN ); // get current number of sets
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint8_t *)value = GetConf.val.ChannelSetsCount;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.ChannelSetsCount );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }

         case macPacketId :
         {
            if ( sizeof(uint8_t) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               GetConf = MAC_GetRequest( eMacAttr_PacketId ); // get current number of sets
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint8_t *)value = GetConf.val.PacketId;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.PacketId );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }

         case macPingCount :
         {
            if ( sizeof(GetConf.val.PingCount) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_PingCount );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint16_t *)value = GetConf.val.PingCount;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.PingCount );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }

         case macCsmaQuickAbort:
         {
            if ( sizeof(GetConf.val.TransactionTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_CsmaQuickAbort );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint8_t *)value = GetConf.val.CsmaQuickAbort;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(uint8_t);
                     attr->rValTypecast = (uint8_t)Boolean;
                  }
               }
            }
            break;
         }

         case macPacketTimeout :
         {
            if ( sizeof(GetConf.val.TransactionTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_PacketTimeout );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint16_t *)value = GetConf.val.PacketTimeout;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.PacketTimeout );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }

         case macAckWaitDuration :
         {
            if ( sizeof(GetConf.val.TransactionTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_AckWaitDuration );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint16_t *)value = GetConf.val.AckWaitDuration;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.AckWaitDuration );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }

         case macAckDelayDuration :
         {
            if ( sizeof(GetConf.val.TransactionTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_AckDelayDuration );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint16_t *)value = GetConf.val.AckDelayDuration;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.AckDelayDuration );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }

         case macState :
         {
            if ( sizeof(GetConf.val.State) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_State );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint8_t *)value = GetConf.val.State;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.State );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }

         case macChannelsAccessConstrained:
         {
            if ( sizeof(GetConf.val.TransactionTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_IsChannelAccessConstrained );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint8_t *)value = GetConf.val.IsChannelAccessConstrained;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(uint8_t);
                     attr->rValTypecast = (uint8_t)Boolean;
                  }
               }
            }
            break;
         }

         case macIsFNG :
         {
            if ( sizeof(GetConf.val.TransactionTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_IsFNG );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint8_t *)value = GetConf.val.IsFNG;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(uint8_t);
                     attr->rValTypecast = (uint8_t)Boolean;
                  }
               }
            }
            break;
         }

         case macIsIAG :
         {
            if ( sizeof(GetConf.val.TransactionTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_IsIAG );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint8_t *)value = GetConf.val.IsIAG;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(uint8_t);
                     attr->rValTypecast = (uint8_t)Boolean;
                  }
               }
            }
            break;
         }

         case macIsRouter :
         {
            if ( sizeof(GetConf.val.TransactionTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_IsRouter );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint8_t *)value = GetConf.val.IsRouter;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(uint8_t);
                     attr->rValTypecast = (uint8_t)Boolean;
                  }
               }
            }
            break;
         }

#if 0
         case macTxFrames :
         {
            if ( sizeof(GetConf.val.State) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_TxFrameCount );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint32_t *)value = GetConf.val.TxFrameCount;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.TxFrameCount );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
#endif

         case macTransactionTimeout :
         {
            if ( sizeof(GetConf.val.TransactionTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_TransactionTimeout );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint16_t *)value = GetConf.val.TransactionTimeout;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.TransactionTimeout );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case macTransactionTimeoutCount :
         {
            if ( sizeof(GetConf.val.TransactionTimeoutCount) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_TransactionTimeoutCount );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint32_t *)value = GetConf.val.TransactionTimeoutCount;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.TransactionTimeoutCount );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case macTxLinkDelayCount :
         {
            if ( sizeof(GetConf.val.TxLinkDelayCount) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_TxLinkDelayCount );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint32_t *)value = GetConf.val.TxLinkDelayCount;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.TxLinkDelayCount );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case macTxLinkDelayTime :
         {
            if ( sizeof(GetConf.val.TxLinkDelayTime) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_TxLinkDelayTime );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint32_t *)value = GetConf.val.TxLinkDelayTime;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.TxLinkDelayTime );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case macCsmaMaxAttempts :
         {
            if ( sizeof(GetConf.val.CsmaMaxAttempts) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_CsmaMaxAttempts );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint8_t *)value = GetConf.val.CsmaMaxAttempts;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.CsmaMaxAttempts );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case macCsmaMinBackOffTime :
         {
            if ( sizeof(GetConf.val.CsmaMinBackOffTime) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_CsmaMinBackOffTime );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint16_t *)value = GetConf.val.CsmaMinBackOffTime;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.CsmaMinBackOffTime );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case macCsmaMaxBackOffTime :
         {
            if ( sizeof(GetConf.val.CsmaMaxBackOffTime) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_CsmaMaxBackOffTime );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint16_t *)value = GetConf.val.CsmaMaxBackOffTime;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.CsmaMaxBackOffTime );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case macCsmaPValue :
         {
            if ( sizeof(GetConf.val.CsmaPValue) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_CsmaPValue );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(int32_t *)value = (int32_t)( (GetConf.val.CsmaPValue + 0.005f) * pow(10.0,(double)CMSA_PVALUE_POWER_TEN) );
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof(uint32_t);
                     attr->rValTypecast = (uint8_t)intValue;
                     attr->powerOfTen = (uint8_t)CMSA_PVALUE_POWER_TEN;
                  }
               }
            }
            break;
         }
         case macReassemblyTimeout :
         {
            if ( sizeof(GetConf.val.ReassemblyTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = MAC_GetRequest( eMacAttr_ReassemblyTimeout );
               if ( GetConf.eStatus == eMAC_GET_SUCCESS )
               {
                  *(uint8_t *)value = GetConf.val.ReassemblyTimeout;
                  if (attr != NULL)
                  {
                     attr->rValLen = sizeof( GetConf.val.ReassemblyTimeout );
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
        case inboundFrameCount :
        {
           if ( sizeof(GetConf.val.ReassemblyTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
           {  //The reading will fit in the buffer
              GetConf = MAC_GetRequest( eMacAttr_RxFrameCount );
              if ( GetConf.eStatus == eMAC_GET_SUCCESS )
              {
                 *(uint32_t *)value = GetConf.val.RxFrameCount;
                 if (attr != NULL)
                 {
                    attr->rValLen = sizeof( GetConf.val.RxFrameCount );
                    attr->rValTypecast = (uint8_t)uintValue;
                 }
              }
           }
           break;
         }
        case macReliabilityHighCount :
        {
           if ( sizeof( GetConf.val.ReliabilityHighCount ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
           {  //The reading will fit in the buffer
              GetConf = MAC_GetRequest( eMacAttr_ReliabilityHighCount );
              if ( GetConf.eStatus == eMAC_GET_SUCCESS )
              {
                 *(uint8_t *)value = GetConf.val.ReliabilityHighCount;
                 if (attr != NULL)
                 {
                    attr->rValLen = sizeof( GetConf.val.ReliabilityHighCount );
                    attr->rValTypecast = (uint8_t)uintValue;
                 }
              }
           }
           break;
         }
        case macReliabilityMedCount :
        {
           if ( sizeof( GetConf.val.ReliabilityMediumCount ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
           {  //The reading will fit in the buffer
              GetConf = MAC_GetRequest( eMacAttr_ReliabilityMediumCount );
              if ( GetConf.eStatus == eMAC_GET_SUCCESS )
              {
                 *(uint8_t *)value = GetConf.val.ReliabilityMediumCount;
                 if (attr != NULL)
                 {
                    attr->rValLen = sizeof( GetConf.val.ReliabilityMediumCount );
                    attr->rValTypecast = (uint8_t)uintValue;
                 }
              }
           }
           break;
         }
        case macReliabilityLowCount :
        {
           if ( sizeof( GetConf.val.ReliabilityLowCount ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
           {  //The reading will fit in the buffer
              GetConf = MAC_GetRequest( eMacAttr_ReliabilityLowCount );
              if ( GetConf.eStatus == eMAC_GET_SUCCESS )
              {
                 *(uint8_t *)value = GetConf.val.ReliabilityLowCount;
                 if (attr != NULL)
                 {
                    attr->rValLen = sizeof( GetConf.val.ReliabilityLowCount );
                    attr->rValTypecast = (uint8_t)uintValue;
                 }
              }
           }
           break;
         }
#if  ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
        case macLinkParametersPeriod :
        {
           if ( sizeof( GetConf.val.LinkParamPeriod ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
           {  //The reading will fit in the buffer
              GetConf = MAC_GetRequest( eMacAttr_LinkParamPeriod );
              if ( GetConf.eStatus == eMAC_GET_SUCCESS )
              {
                 *(uint32_t *)value = GetConf.val.LinkParamPeriod;
                 if (attr != NULL)
                 {
                    attr->rValLen = sizeof( GetConf.val.LinkParamPeriod );
                    attr->rValTypecast = (uint8_t)uintValue;
                 }
              }
           }
           break;
         }
        case macLinkParametersStart :
        {
           if ( sizeof( GetConf.val.LinkParamStart)  <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
           {  //The reading will fit in the buffer
              GetConf = MAC_GetRequest( eMacAttr_LinkParamStart );
              if ( GetConf.eStatus == eMAC_GET_SUCCESS )
              {
                 *(uint32_t *)value = GetConf.val.LinkParamStart;
                 if (attr != NULL)
                 {
                    attr->rValLen = sizeof( GetConf.val.LinkParamStart );
                    attr->rValTypecast = (uint8_t)uintValue;
                 }
              }
           }
           break;
         }
        case macLinkParametersMaxOffset :
        {
           if ( sizeof( GetConf.val.LinkParamMaxOffset ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
           {  //The reading will fit in the buffer
              GetConf = MAC_GetRequest( eMacAttr_LinkParamMaxOffset );
              if ( GetConf.eStatus == eMAC_GET_SUCCESS )
              {
                 *(uint16_t *)value = GetConf.val.LinkParamMaxOffset;
                 if (attr != NULL)
                 {
                    attr->rValLen = sizeof( GetConf.val.LinkParamMaxOffset );
                    attr->rValTypecast = (uint8_t)uintValue;
                 }
              }
           }
           break;
         }
#endif // end of ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) ) section
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
        case macCommandResponseMaxTimeDiversity :
        {
           if ( sizeof( GetConf.val.macCmdRespMaxTimeDiversity ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
           {  //The reading will fit in the buffer
              GetConf = MAC_GetRequest( eMacAttr_CmdRespMaxTimeDiversity );
              if ( GetConf.eStatus == eMAC_GET_SUCCESS )
              {
                 *(uint16_t *)value = GetConf.val.macCmdRespMaxTimeDiversity ;
                 if (attr != NULL)
                 {
                    attr->rValLen = sizeof( GetConf.val.macCmdRespMaxTimeDiversity );
                    attr->rValTypecast = (uint8_t)uintValue;
                 }
              }
           }
           break;
         }
#endif
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
      if( GetConf.eStatus == eMAC_GET_SUCCESS )
      {
         retVal = eSUCCESS;
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      confirm.eStatus = eMAC_SET_UNSUPPORTED;
      retVal = eFAILURE;
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
#if ( DCU == 1)
         case macChannelSetsSTAR :
         case macChannelSets :
         {
            uint16_t  numValues;
            uint16_t  maxSets;
            uint16_t *pChannel = (uint16_t *) value;
            uint16_t *pSet     = (uint16_t *)(void *)SetReq.val.ChannelSets;

            numValues = attr->rValLen/sizeof(uint16_t);

            if ( id == macChannelSetsSTAR )
            {
               maxSets = MAC_MAX_CHANNEL_SETS_STAR*2;
            }
            else
            {
               maxSets = MAC_MAX_CHANNEL_SETS_SRFN*2;
            }

            // Make sure there is at least one value and not more than allowed
            if (( numValues > 0) && (numValues <= maxSets))
            {  // There muast be at least one value
               for(uint32_t i=0;i<maxSets;i++)
               {
                  if (i < numValues)
                  {
                     pSet[i] = pChannel[i];
                  } else {
                     pSet[i] = PHY_INVALID_CHANNEL;
                  }
               }
               if ( id == macChannelSetsSTAR ) {
                  confirm = MAC_SetRequest( eMacAttr_ChannelSetsSTAR, (void*)&SetReq.val );
               } else {
                  confirm = MAC_SetRequest( eMacAttr_ChannelSetsSRFN, (void*)&SetReq.val );
               }
            }
            break;
         }
#if ( MAC_LINK_PARAMETERS == 1 )
         case macLinkParametersPeriod :
         {
            confirm = MAC_SetRequest( eMacAttr_LinkParamPeriod, (uint32_t *)value);
            break;
         }
         case macLinkParametersStart :
         {
            confirm = MAC_SetRequest( eMacAttr_LinkParamStart, (uint32_t *)value);
            break;
         }
         case macLinkParametersMaxOffset :
         {
            confirm = MAC_SetRequest( eMacAttr_LinkParamMaxOffset, (uint16_t *)value);
            break;
         }
#endif
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
         case macCommandResponseMaxTimeDiversity :
         {
            confirm = MAC_SetRequest( eMacAttr_CmdRespMaxTimeDiversity, (uint16_t *)value);
            break;
         }
#endif
#endif
         case macReliabilityHighCount :
         {
            confirm = MAC_SetRequest( eMacAttr_ReliabilityHighCount, (uint8_t *)value);
            break;
         }
         case macReliabilityMedCount :
         {
            confirm = MAC_SetRequest( eMacAttr_ReliabilityMediumCount, (uint8_t *)value);
            break;
         }
         case macReliabilityLowCount :
         {
            confirm = MAC_SetRequest( eMacAttr_ReliabilityLowCount, (uint8_t *)value);
            break;
         }
         case macPacketTimeout :
         {
            confirm = MAC_SetRequest( eMacAttr_PacketTimeout, (uint16_t *)value);
            break;
         }
         case macCsmaQuickAbort :
         {
            confirm = MAC_SetRequest( eMacAttr_CsmaQuickAbort, (uint8_t *)value);
            break;
         }
         case macIsFNG:
         {
            confirm = MAC_SetRequest( eMacAttr_IsFNG, (uint8_t *)value);
            break;
         }
         case macIsIAG:
         {
            confirm = MAC_SetRequest( eMacAttr_IsIAG, (uint8_t *)value);
            break;
         }
         case macIsRouter:
         {
            confirm = MAC_SetRequest( eMacAttr_IsRouter, (uint8_t *)value);
            break;
         }
         case macTransactionTimeout :
         {
            confirm = MAC_SetRequest( eMacAttr_TransactionTimeout, (uint16_t *)value);
            break;
         }
         case macCsmaMaxAttempts :
         {
            confirm = MAC_SetRequest( eMacAttr_CsmaMaxAttempts, (uint8_t *)value);
            break;
         }
         case macCsmaMinBackOffTime :
         {
            confirm = MAC_SetRequest( eMacAttr_CsmaMinBackOffTime, (uint16_t *)value);
            break;
         }
         case macCsmaMaxBackOffTime :
         {
            confirm = MAC_SetRequest( eMacAttr_CsmaMaxBackOffTime, (uint16_t *)value);
            break;
         }
         case macCsmaPValue :
         {
            if( intValue != (ReadingsValueTypecast)attr->rValTypecast )
            {
               retVal = eAPP_INVALID_TYPECAST;
            }
            else
            {
               float temp;
               // convert integer to float using provided power of ten qualifier from the request
               temp = (float)((float)*(int32_t*)value/(float)pow(10.0, (double)attr->powerOfTen));
               confirm = MAC_SetRequest( eMacAttr_CsmaPValue, (float *)&temp);
            }
            break;
         }
         case macReassemblyTimeout :
         {
            confirm = MAC_SetRequest( eMacAttr_ReassemblyTimeout, (uint8_t *)value);
            break;
         }

         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
      if( confirm.eStatus == eMAC_SET_SUCCESS )
      {
         retVal = eSUCCESS;
      }
      else if (confirm.eStatus == eMAC_SET_INVALID_PARAMETER )
      {
         retVal = eAPP_INVALID_VALUE;
      }
      else if( confirm.eStatus == eMAC_SET_READONLY )
      {
         retVal =  eAPP_READ_ONLY;
      }
   }
   return ( retVal );
}

/***********************************************************************************************************************

   Function Name: PHY_OR_PM_Handler

   Purpose: Get/Set parameters related to PHY module

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t PHY_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   PHY_SetConf_t  confirm;
   PHY_GetConf_t  GetConf;
   returnStatus_t retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      GetConf.eStatus = ePHY_GET_UNSUPPORTED_ATTRIBUTE;
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case phyFrontEndGain :
         {
            if ( sizeof(GetConf.val.FrontEndGain) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_FrontEndGain );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(int8_t *)value = GetConf.val.FrontEndGain;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.FrontEndGain);
                     attr->rValTypecast = (uint8_t)intValue;
                  }
               }
            }
            break;
         }
         case phyNumChannels :
         {
            if ( sizeof(GetConf.val.NumChannels) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_NumChannels );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint8_t *)value = GetConf.val.NumChannels;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.NumChannels);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case phyAvailableChannels :
         {
            if ( sizeof(GetConf.val.AvailableChannels) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_AvailableChannels );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  (void)memcpy((uint16_t *)value, GetConf.val.AvailableChannels, sizeof(GetConf.val.AvailableChannels));
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.AvailableChannels);
                     attr->rValTypecast = (uint8_t)uint16_list_type;
                  }
               }
            }
            break;
         }
         case phyAvailableFrequencies :
         {
            int32_t frequencies[PHY_MAX_CHANNEL];
            uint8_t i;
            if ( sizeof(frequencies) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_AvailableChannels );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  for( i = 0; i < PHY_MAX_CHANNEL; i++ )
                  {
                     if( GetConf.val.AvailableChannels[i] == PHY_INVALID_CHANNEL )
                     {
                        frequencies[i]  = 0;
                     }
                     else
                     {
                        frequencies[i] = (int32_t)CHANNEL_TO_FREQ( GetConf.val.AvailableChannels[i] );
                     }
                  }
                  (void)memcpy((int32_t *)value, frequencies, sizeof(frequencies));
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(frequencies);
                     attr->rValTypecast = (uint8_t)int32_list_type;
                  }
               }
            }
            break;
         }
         case phyTxFrequencies :
         {
            int32_t frequencies[PHY_MAX_CHANNEL];
            uint8_t i;
            if ( sizeof(frequencies) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_TxChannels );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  for( i = 0; i < PHY_MAX_CHANNEL; i++ )
                  {
                     if( GetConf.val.TxChannels[i] == PHY_INVALID_CHANNEL )
                     {
                        frequencies[i]  = 0;
                     }
                     else
                     {
                        frequencies[i] = (int32_t)CHANNEL_TO_FREQ( GetConf.val.TxChannels[i] );
                     }
                  }
                  (void)memcpy((int32_t *)value, frequencies, sizeof(frequencies));
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(frequencies);
                     attr->rValTypecast = (uint8_t)int32_list_type;
                  }
               }
            }
            break;
         }
         case phyRcvrCount :
         {
            if ( sizeof(GetConf.val.NumChannels) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_RcvrCount );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint8_t *)value = GetConf.val.RcvrCount;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.RcvrCount);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case phyTxChannels :
         {
            if ( sizeof(GetConf.val.TxChannels) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_TxChannels );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  (void)memcpy((uint16_t *)value, GetConf.val.TxChannels, sizeof(GetConf.val.TxChannels));
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.TxChannels);
                     attr->rValTypecast = (uint8_t)uint16_list_type;
                  }
               }
            }
            break;
         }
         case phyRxFrequencies :
         {
            int32_t frequencies[PHY_RCVR_COUNT];
            uint8_t i;
            if ( sizeof(frequencies) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_RxChannels );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  for( i = 0; i < PHY_RCVR_COUNT; i++ )
                  {
                     if( GetConf.val.RxChannels[i] == PHY_INVALID_CHANNEL )
                     {
                        frequencies[i]  = 0;
                     }
                     else
                     {
                        frequencies[i] = (int32_t)CHANNEL_TO_FREQ( GetConf.val.RxChannels[i] );
                     }
                  }
                  (void)memcpy((int32_t *)value, frequencies, sizeof(frequencies));
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(frequencies);
                     attr->rValTypecast = (uint8_t)int32_list_type;
                  }
               }
            }
            break;
         }
         case phyRxChannels :
         {
            if ( sizeof(GetConf.val.RxChannels) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_RxChannels );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  (void)memcpy((uint16_t *)value, GetConf.val.RxChannels, sizeof(GetConf.val.RxChannels));
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.RxChannels);
                     attr->rValTypecast = (uint8_t)uint16_list_type;
                  }
               }
            }
            break;
         }
#if ( EP == 1)
         // todo MKD 11-25-2019 11:45 AM - This code needs to be modified to handle a list of more than one entry
         case phyDemodulator :
         {
            if ( sizeof(GetConf.val.Demodulator) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_Demodulator );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint16_t *)value = (uint16_t)GetConf.val.Demodulator[0];
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)(sizeof(GetConf.val.Demodulator)*sizeof(uint16_t)); // Convert the uint8_t list size into an equivalent uint16_t list size
                     attr->rValTypecast = (uint8_t)uint16_list_type;
                  }
               }
            }
            break;
         }
#endif
         case phyCcaThreshold :
         {
            if ( sizeof(GetConf.val.CcaThreshold) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_CcaThreshold );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS )
               {
                  (void)memcpy((int16_t *)value, &GetConf.val.CcaThreshold, sizeof(GetConf.val.CcaThreshold));
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.CcaThreshold);
                     attr->rValTypecast = (uint8_t)intValue;
                  }
               }
            }
            break;
         }
         case phyMaxTxPayload :
         {
            if ( sizeof(GetConf.val.MaxTxPayload) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_MaxTxPayload );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint16_t *)value = GetConf.val.MaxTxPayload;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.MaxTxPayload);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         // All the following counters are all uint32_t so we can re-use some code
         case phyFailedFrameDecodeCount :
         case phyFailedHcsCount :
         case phyFramesReceivedCount :
         case phyFailedHeaderDecodeCount :
         case phySyncDetectCount :
         {
            if ( sizeof(GetConf.val.FailedFrameDecodeCount) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               switch ( id )  /*lint !e788 not all enums used within switch */
               {
                  case phyFailedFrameDecodeCount:
                     GetConf = PHY_GetRequest( ePhyAttr_FailedFrameDecodeCount );
                     break;
                  case phyFailedHcsCount:
                     GetConf = PHY_GetRequest( ePhyAttr_FailedHcsCount );
                     break;
                  case phyFramesReceivedCount:
                     GetConf = PHY_GetRequest( ePhyAttr_FramesReceivedCount );
                     break;
                  case phyFailedHeaderDecodeCount:
                     GetConf = PHY_GetRequest( ePhyAttr_FailedHeaderDecodeCount );
                     break;
                  case phySyncDetectCount:
                     GetConf = PHY_GetRequest( ePhyAttr_SyncDetectCount );
                     break;
                  default:
                     break;
               }  /*lint !e788 not all enums used within switch */
               if ( GetConf.eStatus == ePHY_GET_SUCCESS )
               {
                  (void)memcpy((uint32_t *)value, GetConf.val.FailedFrameDecodeCount, sizeof(GetConf.val.FailedFrameDecodeCount)); // FailedFrameDecodeCount maps the other counters too
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.FailedFrameDecodeCount);
                     attr->rValTypecast = (uint8_t)hexBinary;
                  }
               }
            }
            break;
         }
         case phyFramesTransmittedCount :
         {
            if ( sizeof(GetConf.val.FramesTransmittedCount) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_FramesTransmittedCount );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS )
               {
                  (void)memcpy((uint32_t *)value, &GetConf.val.FramesTransmittedCount, sizeof(GetConf.val.FramesTransmittedCount)); // FailedFrameDecodeCount maps the other counters too
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)GetConf.val.FramesTransmittedCount, uintValue, 0);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case phyRxDetection :
         {
            uint16_t list[PHY_RCVR_COUNT];
            if ( sizeof(list) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_RxDetectionConfig );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS )
               {
                  uint32_t i;
                  for (i = 0; i < PHY_RCVR_COUNT; i ++)
                  {
                     list[i] = (uint16_t)GetConf.val.RxDetectionConfig[i];
                  }
                  (void)memcpy((uint16_t *)value, list, sizeof(list) );
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)( sizeof(list) );
                     attr->rValTypecast = (uint8_t)uint16_list_type;
                  }
               }
            }
            break;
         }
         case phyRxFraming :
         {
            uint16_t list[PHY_RCVR_COUNT];
            if ( sizeof(list) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_RxFramingConfig );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS )
               {
                  uint32_t i;
                  for (i = 0; i < PHY_RCVR_COUNT; i ++)
                  {
                     list[i] = (uint16_t)GetConf.val.RxFramingConfig[i];
                  }
                  (void)memcpy((uint16_t *)value, list, sizeof(list) );
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)( sizeof(list) );
                     attr->rValTypecast = (uint8_t)uint16_list_type;
                  }
               }
            }
            break;
         }
         case phyRxMode :
         {
            uint16_t list[PHY_RCVR_COUNT];
            if ( sizeof(list) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_RxMode );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS )
               {
                  uint32_t i;
                  for (i = 0; i < PHY_RCVR_COUNT; i ++)
                  {
                     list[i] = (uint16_t)GetConf.val.RxMode[i];
                  }
                  (void)memcpy((uint16_t *)value, list, sizeof(list) );
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)( sizeof(list) );
                     attr->rValTypecast = (uint8_t)uint16_list_type;
                  }
               }
            }
            break;
         }
         case phyCcaAdaptiveThresholdEnable :
         {
            if ( sizeof(GetConf.val.CcaAdaptiveThresholdEnable) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_CcaAdaptiveThresholdEnable );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint8_t *)value = GetConf.val.CcaAdaptiveThresholdEnable;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.CcaAdaptiveThresholdEnable);
                     attr->rValTypecast = (uint8_t)Boolean;
                  }
               }
            }
            break;
         }
         case phyCcaOffset :
         {
            if ( sizeof(GetConf.val.CcaOffset) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_CcaOffset );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint8_t *)value = GetConf.val.CcaOffset;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.CcaOffset);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case phyNoiseEstimate :
         {
            int32_t list[PHY_MAX_CHANNEL];
            if ( sizeof(list) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_NoiseEstimate );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  uint32_t i;
                  for (i = 0; i < PHY_MAX_CHANNEL; i ++)
                  {
                     list[i] = (int32_t)GetConf.val.NoiseEstimate[i];
                  }
                  (void)memcpy((uint8_t *)value, (uint8_t *)list, sizeof(list) );
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)( sizeof(list) );
                     attr->rValTypecast = (uint8_t)int32_list_type;
                  }
               }
            }
            break;
         }
         case phyNoiseEstimateRate :
         {
            if ( sizeof(GetConf.val.NoiseEstimateRate) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_NoiseEstimateRate );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint8_t *)value = GetConf.val.NoiseEstimateRate;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.NoiseEstimateRate);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case phyThermalControlEnable :
         {
            if ( sizeof(GetConf.val.ThermalControlEnable) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_ThermalControlEnable );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint8_t *)value = GetConf.val.ThermalControlEnable;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.ThermalControlEnable);
                     attr->rValTypecast = (uint8_t)Boolean;
                  }
               }
            }
            break;
         }
         case phyThermalProtectionCount :
         {
            if ( sizeof(GetConf.val.ThermalProtectionCount) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_ThermalProtectionCount );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint32_t *)value = GetConf.val.ThermalProtectionCount;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.ThermalProtectionCount);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
         case phyThermalProtectionEnable :
         {
            if ( sizeof(GetConf.val.ThermalProtectionEnable) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_ThermalProtectionEnable );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint8_t *)value = GetConf.val.ThermalProtectionEnable;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.ThermalProtectionEnable);
                     attr->rValTypecast = (uint8_t)Boolean;
                  }
               }
            }
            break;
         }
         case phyThermalProtectionEngaged :
         {
            if ( sizeof(GetConf.val.ThermalProtectionEngaged) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_ThermalProtectionEngaged );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint8_t *)value = GetConf.val.ThermalProtectionEngaged;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.ThermalProtectionEngaged);
                     attr->rValTypecast = (uint8_t)Boolean;
                  }
               }
            }
            break;
         }
#if  ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
         case receivePowerMargin :
         {
            if ( sizeof(GetConf.val.ReceivePowerMargin) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  // The reading will fit in the buffer
               GetConf = PHY_GetRequest( ePhyAttr_ReceivePowerMargin );
               if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
                  *(uint8_t *)value = GetConf.val.ReceivePowerMargin;
                  if (attr != NULL)
                  {
                     attr->rValLen = (uint16_t)sizeof(GetConf.val.ReceivePowerMargin);
                     attr->rValTypecast = (uint8_t)uintValue;
                  }
               }
            }
            break;
         }
#endif
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
      if( GetConf.eStatus == ePHY_GET_SUCCESS )
      {
         retVal = eSUCCESS;
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      confirm.eStatus = ePHY_SET_UNSUPPORTED_ATTRIBUTE;
      retVal = eFAILURE;
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case phyFrontEndGain :
         {
            confirm = PHY_SetRequest( ePhyAttr_FrontEndGain, (uint8_t *)value);
            break;
         }
         case phyCcaThreshold :
         {
            confirm = PHY_SetRequest( ePhyAttr_CcaThreshold, (uint8_t *)value);
            break;
         }
         case phyCcaAdaptiveThresholdEnable :
         {
            confirm = PHY_SetRequest( ePhyAttr_CcaAdaptiveThresholdEnable, (uint8_t *)value);
            break;
         }
         case phyCcaOffset :
         {
            confirm = PHY_SetRequest( ePhyAttr_CcaOffset, (uint8_t *)value);
            break;
         }
         case phyThermalControlEnable :
         {
            confirm = PHY_SetRequest( ePhyAttr_ThermalControlEnable, (uint8_t *)value);
            break;
         }
         case phyThermalProtectionEnable :
         {
            confirm = PHY_SetRequest( ePhyAttr_ThermalProtectionEnable, (uint8_t *)value);
            break;
         }
         case phyNoiseEstimateRate :
         {
            confirm = PHY_SetRequest( ePhyAttr_NoiseEstimateRate, (uint8_t *)value);
            break;
         }
         case phyMaxTxPayload :
         {
            confirm = PHY_SetRequest( ePhyAttr_MaxTxPayload, (uint8_t *)value);
            break;
         }
#if ( EP == 1 )
         // todo MKD 11-25-2019 11:45 AM - This code needs to be modified to handle a list of more than one entry
         case phyDemodulator :
         {
            confirm = PHY_SetRequest( ePhyAttr_Demodulator, (uint8_t *)value);
            break;
         }
#endif
         case phyAvailableFrequencies :
         case phyAvailableChannels :
         {
            // If the number of values = 0,  the command is invalid
            // If the number of values < 32 (PHY_MAX_CHANNEL) then the remaining values will be invalid
            // If the number of values > 32 (PHY_MAX_CHANNEL) then the extras values are ignored
            // If the number of values < 32, then the remaining values will be set invalid
            // If a frequency is invalid, then the channel will be set to invalid.  This is done in the PHY.

            // Removing frequencies/channels that are used for Rx/Tx does not clear those.
            // This can leave the system in an invalid state.
            if ( OTAtestenabled_ )
            {
               PHY_ATTRIBUTES_u phy_attr;

               uint8_t   numValues;
               uint8_t   valueType;
               uint32_t *pFrequency = (uint32_t *) value;
               uint16_t *pChannel   = (uint16_t *) value;

               if(id == phyAvailableFrequencies)
               {
                  valueType = 0;
                  numValues = (uint8_t)(attr->rValLen/sizeof(uint32_t));
               }
               else{
                  valueType = 1;
                  numValues = (uint8_t)(attr->rValLen/sizeof(uint16_t));
               }
               // Make sure there is at least one value and not more than allowed
               if (( numValues > 0) && (numValues <= PHY_MAX_CHANNEL) )
               {  // There muast be at least one value
                  for(uint8_t i=0;i<PHY_MAX_CHANNEL;i++)
                  {
                     if (i < numValues)
                     {
                        if(valueType == 0)
                        {
                           // Need to sanitize inputs when using frequency
                           if ( ( pFrequency[i] == 0) ||
                                ( pFrequency[i] == PHY_INVALID_CHANNEL) )
                           {
                              pFrequency[i] = PHY_INVALID_FREQ_TO_CHANNEL;
                           }

                           /* Convert frequency into channel */
                           phy_attr.AvailableChannels[i] =  ( uint16_t )FREQ_TO_CHANNEL( pFrequency[i] );
                        }
                        else{
                           phy_attr.AvailableChannels[i] =  pChannel[i];
                        }
                     }else
                     {
                        phy_attr.AvailableChannels[i] = PHY_INVALID_CHANNEL;
                     }
                  }
                  confirm = PHY_SetRequest( ePhyAttr_AvailableChannels, (uint8_t *)phy_attr.AvailableChannels);
               }
            }
            else
            {
               confirm.eStatus = ePHY_SET_READONLY;
            }
            break;
         }
         case phyTxFrequencies :
         case phyTxChannels :
         {
#if ( EP == 1 )
            if ( OTAtestenabled_ )
#endif
            {
               PHY_ATTRIBUTES_u phy_attr;

               uint16_t  numValues;
               uint8_t   valueType;

               uint32_t *pFrequency = (uint32_t *) value;
               uint16_t *pChannel   = (uint16_t *) value;

               if(id == phyTxFrequencies)
               {
                  valueType = 0; // Frequency
                  numValues = attr->rValLen/sizeof(uint32_t);
               }
               else{
                  valueType = 1; // Channel
                  numValues = attr->rValLen/sizeof(uint16_t);
               }

               // Make sure there is at least one value and not more than allowed
               if (( numValues > 0) && (numValues <= PHY_MAX_CHANNEL) )
               {  // There must be at least one value
                  // For each channel
                  for(uint32_t i=0;i<PHY_MAX_CHANNEL;i++)
                  {
                     if (i < numValues)
                     {
                        if(valueType == 0)
                        {
                           // Need to sanitize inputs when using frequency
                           if ( ( pFrequency[i] == 0) ||
                                ( pFrequency[i] == PHY_INVALID_CHANNEL) )
                           {
                              pFrequency[i] = PHY_INVALID_FREQ_TO_CHANNEL;
                           }

                           /* Convert frequency into channel */
                           phy_attr.TxChannels[i] =  ( uint16_t )FREQ_TO_CHANNEL( pFrequency[i] );
                        }
                        else{
                           phy_attr.TxChannels[i] =  pChannel[i];
                        }
                     }else
                     {
                        phy_attr.TxChannels[i] = PHY_INVALID_CHANNEL;
                     }
                  }
                  confirm = PHY_SetRequest( ePhyAttr_TxChannels, (uint8_t *)phy_attr.TxChannels);
               }
            }
#if ( EP == 1 )
            else
            {
               confirm.eStatus = ePHY_SET_READONLY;
            }
#endif
            break;
         }
         case phyRxFrequencies :
         case phyRxChannels :
         {
#if ( EP == 1 )
            if ( OTAtestenabled_ )
#endif
            {
               PHY_ATTRIBUTES_u phy_attr;

               uint16_t  numValues;
               uint8_t   valueType;

               uint32_t *pFrequency = (uint32_t *) value;
               uint16_t *pChannel   = (uint16_t *) value;

               if(id == phyRxFrequencies)
               {
                  valueType = 0; // Frequency
                  numValues = attr->rValLen/sizeof(uint32_t);
               }
               else{
                  valueType = 1; // Channel
                  numValues = attr->rValLen/sizeof(uint16_t);
               }

               // Make sure there is at least one value and not more than allowed
               if (( numValues > 0) && (numValues <= PHY_RCVR_COUNT))
               {  // There muast be at least one value
                  // For each receiver
                  for(uint32_t i=0;i<PHY_RCVR_COUNT;i++)
                  {
                     if (i < numValues)
                     {
                        if(valueType == 0)
                        {
                           // Need to sanitize inputs when using frequency
                           if ( ( pFrequency[i] == 0) ||
                                ( pFrequency[i] == PHY_INVALID_CHANNEL) )
                           {
                              pFrequency[i] = PHY_INVALID_FREQ_TO_CHANNEL;
                           }

                           /* Convert frequency into channel */
                           phy_attr.RxChannels[i] =  ( uint16_t )FREQ_TO_CHANNEL( pFrequency[i] );
                        }
                        else{
                           phy_attr.RxChannels[i] =  pChannel[i];
                        }
                     }else
                     {
                        phy_attr.RxChannels[i] = PHY_INVALID_CHANNEL; //lint !e661
                     }
                  }
                  confirm = PHY_SetRequest( ePhyAttr_RxChannels, (uint8_t *)phy_attr.RxChannels);
               }
            }
#if ( EP == 1 )
            else
            {
               confirm.eStatus = ePHY_SET_READONLY;
            }
#endif
            break;
         }
#if ( DCU == 1 )
         case phyRxDetection :
         {
            PHY_ATTRIBUTES_u phy_attr;
            uint16_t *pValue = (uint16_t *)value;
            uint16_t numValues = attr->rValLen/sizeof(uint16_t);

            for(uint32_t i=0;i<PHY_RCVR_COUNT;i++)
            {
               if (i < numValues)
               {
                  phy_attr.RxDetectionConfig[i] = (PHY_DETECTION_e) pValue[i];
               }else
               {
                  phy_attr.RxDetectionConfig[i] = ePHY_DETECTION_0;
               }
            }
            confirm = PHY_SetRequest( ePhyAttr_RxDetectionConfig, (uint8_t *)phy_attr.RxDetectionConfig);
            break;
         }

         case phyRxFraming :
         {
            PHY_ATTRIBUTES_u phy_attr;
            uint16_t *pValue = (uint16_t *)value;
            uint16_t numValues = attr->rValLen/sizeof(uint16_t);

            for(uint32_t i=0;i<PHY_RCVR_COUNT;i++)
            {
               if (i < numValues)
               {
                  phy_attr.RxFramingConfig[i] = (PHY_FRAMING_e) pValue[i];
               }else
               {
                  phy_attr.RxFramingConfig[i] = ePHY_FRAMING_0;
               }
            }
            confirm = PHY_SetRequest( ePhyAttr_RxFramingConfig, (uint8_t *)phy_attr.RxFramingConfig);
            break;
         }
         case phyRxMode :
         {
            PHY_ATTRIBUTES_u phy_attr;
            uint16_t *pValue = (uint16_t *)value;
            uint16_t numValues = attr->rValLen/sizeof(uint16_t);

            for(uint32_t i=0;i<PHY_RCVR_COUNT;i++)
            {
               if (i < numValues)
               {
                  phy_attr.RxMode[i] = (PHY_MODE_e) pValue[i];
               }else
               {
                  phy_attr.RxMode[i] = ePHY_MODE_0;
               }
            }
            confirm = PHY_SetRequest( ePhyAttr_RxMode, (uint8_t *)phy_attr.RxMode);
            break;
         }
#if  ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
         case receivePowerMargin :
         {
            confirm = PHY_SetRequest( ePhyAttr_ReceivePowerMargin, (uint8_t *)value);
            break;
         }
#endif
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
         case fngVSWR:
         {
            confirm = PHY_SetRequest( ePhyAttr_fngVSWR, (uint8_t *)value);
            break;
         }
         case fngVswrNotificationSet:
         {
            confirm = PHY_SetRequest( ePhyAttr_fngVswrNotificationSet, (uint8_t *)value);
            break;
         }
         case fngVswrShutdownSet:
         {
            confirm = PHY_SetRequest( ePhyAttr_fngVswrShutdownSet, (uint8_t *)value);
            break;
         }
         case fngForwardPower:
         {
            confirm = PHY_SetRequest( ePhyAttr_fngForwardPower, (uint8_t *)value);
            break;
         }
         case fngReflectedPower:
         {
            confirm = PHY_SetRequest( ePhyAttr_fngReflectedPower, (uint8_t *)value);
            break;
         }
         case fngForwardPowerSet:
         {
            confirm = PHY_SetRequest( ePhyAttr_fngForwardPowerSet, (uint8_t *)value);
            break;
         }
         case fngFowardPowerLowSet:
         {
            confirm = PHY_SetRequest( ePhyAttr_fngFowardPowerLowSet, (uint8_t *)value);
            break;
         }
#endif
#else //else for #if ( DCU == 0 )
         case stTxCWTest :
         {
            if ( OTAtestenabled_ )
            {
               uint16_t numValues = attr->rValLen/sizeof(uint32_t);

               // Make sure there is only one value
               if ( 1 == numValues )
               {  /* Convert frequency into channel */
                  if ( eSUCCESS == MFG_PhyStTxCwTest( ( uint16_t )FREQ_TO_CHANNEL( *(uint32_t *)value ) ) )
                  {
                     confirm.eStatus = ePHY_SET_SUCCESS;
                  }
               }
            }
            break;
         }
         case stRx4GFSK :
         {
            if ( OTAtestenabled_ )
            {
               uint16_t numValues = attr->rValLen/sizeof(uint32_t);

               // Make sure there is only one value
               if ( 1 == numValues )
               {  /* Convert frequency into channel */
                  MFG_PhyStRx4GFSK( ( uint16_t )FREQ_TO_CHANNEL( *(uint32_t *)value ) );
                  confirm.eStatus = ePHY_SET_SUCCESS;
               }
            }
            break;
         }
         case stTxBlurtTest :
         {
            if ( OTAtestenabled_ )
            {
               MFG_StTxBlurtTest_Srfn();
               confirm.eStatus = ePHY_SET_SUCCESS;
            }
            break;
         }
#endif
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
      if( confirm.eStatus == ePHY_SET_SUCCESS )
      {
         retVal = eSUCCESS;
      }
      else if( confirm.eStatus == ePHY_SET_READONLY )
      {  // an attempt was made to write a read only value
         retVal = eAPP_READ_ONLY;
      }
   }

   return ( retVal );
}

#if ( USE_DTLS == 1 )
/*!
 **********************************************************************************************************************
 *
 * \fn         DTLS_OR_PM_Handler
 *
 * \brief      Get/Set parameters related to DTLS
 *
 * \param      action-> set or get
 *             id    -> HEEP enum associated with the value
 *             value -> pointer to new value to be placed in file
 *             attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL
 *
 * \return     returnStatus_t
 *
 * \details    If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
returnStatus_t DTLS_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case maxAuthenticationTimeout :
         {
            if ( sizeof( uint32_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint32_t *)value = DTLS_getMaxAuthenticationTimeout();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded(*(int64_t *)value, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case minAuthenticationTimeout :
         {
            if ( sizeof( uint16_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint16_t *)value = DTLS_getMinAuthenticationTimeout();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded(*(int64_t *)value, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case initialAuthenticationTimeout :
         {
            if ( sizeof( uint16_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint16_t *)value = DTLS_getInitialAuthenticationTimeout();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded(*(int64_t *)value, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case dtlsNetworkRootCA :
         {
            if ( sizeof( NetWorkRootCA_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               retVal = DTLS_getDtlsValue(dtlsNetworkRootCA, (uint8_t *)value);
               if (attr != NULL)
               {
                  attr->rValLen = computeCertLength(value);
                  attr->rValTypecast = (uint8_t)hexBinary;
               }
            }
            break;
         }
#if 0
      case dtlsSecurityRootCA :
      {
         if ( sizeof( SecurityRootCA_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
         {  //The reading will fit in the buffer
            retVal = DTLS_getDtlsValue(dtlsNetworkRootCA, (uint8_t *)value);
            if (attr != NULL)
            {
               attr->rValLen = computeCertLength(value);
               attr->rValTypecast = (uint8_t)hexBinary;
            }
         }
         break;
      }
#endif
         case dtlsNetworkHESubject :
         {
            if ( sizeof( HESubject_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               retVal = DTLS_getDtlsValue(dtlsNetworkHESubject, (uint8_t *)value);
               if (attr != NULL)
               {
                  attr->rValLen = computeCertLength(value);
                  attr->rValTypecast = (uint8_t)hexBinary;
               }
            }
            break;
         }
         case dtlsNetworkMSSubject :
         {
            if ( sizeof( MSSubject_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               retVal = DTLS_getDtlsValue(dtlsNetworkMSSubject, (uint8_t *)value);
               if (attr != NULL)
               {
                  attr->rValLen = computeCertLength(value);
                  attr->rValTypecast = (uint8_t)hexBinary;
               }
            }
            break;
         }
         case dtlsMfgSubject1 :
         {
            if ( sizeof( ROM_dtlsMfgSubject1 ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               retVal = DTLS_getDtlsValue(dtlsMfgSubject1, (uint8_t *)value);
               if (attr != NULL)
               {
                  attr->rValLen = computeCertLength(value);
                  attr->rValTypecast = (uint8_t)hexBinary;
               }
            }
            break;
         }
         case dtlsMfgSubject2 :
         {
            if ( sizeof( MSSubject_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               retVal = DTLS_getDtlsValue(dtlsMfgSubject2, (uint8_t *)value);
               if (attr != NULL)
               {
                  attr->rValLen = computeCertLength(value);
                  attr->rValTypecast = (uint8_t)hexBinary;
               }
            }
            break;
         }
         case dtlsServerCertificateSerialNum :
         {
            if ( DTLS_SERVER_CERTIFICATE_SERIAL_MAX_SIZE <= MAX_OR_PM_PAYLOAD_SIZE ) {/*lint !e506 !e774 Boolean within 'if' always evaluates to True */
               uint16_t size;
               retVal = eSUCCESS;
               size = DTLS_GetServerCertificateSerialNumber(value);
               if (attr != NULL)
               {
                  attr->rValLen = size;
                  attr->rValTypecast = (uint8_t)hexBinary;
               }
            }
            break;
         }
         case dtlsDeviceCertificate :
         {
            uint32_t certLen = 0;
            if( ECC108_SUCCESS == ecc108e_GetDeviceCert( DeviceCert_e, (uint8_t *)value, &certLen ) )
            {
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)certLen;
                  attr->rValTypecast = (uint8_t)hexBinary;
               }
            }
            else
            {
               retVal = eFAILURE;
            }
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case maxAuthenticationTimeout :
         {
            retVal = DTLS_setMaxAuthenticationTimeout( *(uint32_t *)value );
            break;
         }
         case minAuthenticationTimeout :
         {
            retVal = DTLS_setMinAuthenticationTimeout( *(uint16_t *)value );
            break;
         }
         case initialAuthenticationTimeout :
         {
            retVal = DTLS_setInitialAuthenticationTimeout( *(uint16_t *)value );
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   return ( retVal );
}
#endif

#if( EP == 1)
/***********************************************************************************************************************

   Function Name: HMC_OR_PM_Handler

   Purpose: Get/Set parameters over the air related to the HMC

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t HMC_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{

   returnStatus_t  retVal; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case meterCommLockout :
         {
            retVal = HMC_ENG_getMeterCommunicationLockoutCount((uint16_t *)value);
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(uint16_t);
               attr->rValTypecast = (uint8_t)uintValue;
            }
            break;
         }
         case meterSessionFailureCount :
         {
            retVal = HMC_ENG_getMeterSessionFailureCount((uint16_t *)value);
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(uint16_t);
               attr->rValTypecast = (uint8_t)uintValue;
            }
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case meterCommLockout :
         {
            retVal = HMC_ENG_setMeterCommunicationLockoutCount( *(uint16_t *)value );
            break;
         }
         case meterSessionFailureCount :
         {
            retVal = HMC_ENG_setMeterSessionFailureCount( *(uint16_t *)value );
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   return ( retVal );
}
#endif // endif for EP == 1

/***********************************************************************************************************************

   Function Name: HEEP_util_passordPort0_Handler

   Purpose: Get/Set parameters over the air related to the HMC

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -esym(715,id) ID matched before function is called  */
#if ( ANSI_SECURITY == 1 )
returnStatus_t HEEP_util_passordPort0_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED;

   if ( method_put == action )
   {
      if( (uint8_t)hexBinary != attr->rValTypecast )
      {  // invalid typecast provided in write request, return invalid typecast
         retVal = eAPP_INVALID_TYPECAST;
      }
      else
      {  // set passwordPort0
         retVal = HMC_STRT_SetPassword( (uint8_t *)value, attr->rValLen );
      }
   }
   else
   {  // The request is to read, but parameter is write only.  Return write only status.
      if( NULL != attr )
      {
         attr->rValLen = 0;
         attr->rValTypecast = (uint8_t)hexBinary;
      }
      retVal = eAPP_WRITE_ONLY;
   }
   return retVal;
}
#endif

/***********************************************************************************************************************

   Function Name: HEEP_util_passordPort0Master_Handler

   Purpose: Get/Set parameters over the air related to the HMC

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -esym(715,id) ID matched before function is called  */
#if ( ANSI_SECURITY == 1 )
returnStatus_t HEEP_util_passordPort0Master_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED;

   if ( method_put == action )
   {  // This is a write
      if( (uint8_t)hexBinary != attr->rValTypecast )
      {  // invalid typecast provided in write request, return invalid typecast
         retVal = eAPP_INVALID_TYPECAST;
      }
      else
      {  // set passwordPort0Master
         retVal = HMC_STRT_SetMasterPassword( (uint8_t *)value, attr->rValLen );
      }
   }
   else
   {  // The request is to read, but parameter is write only.  Return write only status.
      if( NULL != attr )
      {
         attr->rValLen = 0;
         attr->rValTypecast = (uint8_t)hexBinary;
      }
      retVal = eAPP_WRITE_ONLY;
   }
   return retVal;
}
#endif

/***********************************************************************************************************************

   Function Name: HEEP_setEnableOTATest

   Purpose: Set the enableOTATest parameter

   Arguments:  enabledStatus -> enable or disable OTA Testing

   Returns: None

   Side Effects: Allows the programming of frequencies/channels over the air

   Reentrant Code: Yes

 **********************************************************************************************************************/
void HEEP_setEnableOTATest( bool enabledStatus)
{
   OTAtestenabled_ = enabledStatus;
}

/***********************************************************************************************************************

   Function Name: HEEP_getEnableOTATest

   Purpose: Get the enableOTATest parameter

   Arguments:  None

   Returns: Current state of parameter

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
bool HEEP_getEnableOTATest( void )
{
   return ( OTAtestenabled_ );
}

#if ( PHASE_DETECTION == 1 )
/***********************************************************************************************************************

   Function Name:

   Purpose: Get/Set parameters over the air related to Phase Detection

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t PD_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t retVal = eAPP_NOT_HANDLED; // Success/failure
   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e787 not all enums used within switch */
      {
      case capableOfPhaseSelfAssessment:
         //INFO_printf("capableOfPhaseSelfAssessment");
         if ( sizeof( bool ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
         {
            *(uint8_t *)value = false;
            retVal = eSUCCESS;
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(bool);
               attr->rValTypecast = (uint8_t)Boolean;
            }
         }
         break;


      case capableOfPhaseDetectSurvey:
         //INFO_printf("capableOfPhaseDetectSurvey");

         if ( sizeof( bool ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
         {
            retVal = eSUCCESS;
            *(uint8_t *)value = true;
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(bool);
               attr->rValTypecast = (uint8_t)Boolean;
            }
         }
         break;

      case pdSurveyQuantity:
         // type = "uint"
         if ( sizeof( uint8_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
         {
            retVal = eSUCCESS;
            *(uint8_t *)value = PD_SurveyQuantity_Get();
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(uint8_t);
               attr->rValTypecast = (uint8_t)uintValue;
            }
         }
         break;

      case pdSurveyMode:
         // type = "uint"
         if ( sizeof( uint8_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
         {
            retVal = eSUCCESS;
            *(uint8_t *)value = PD_SurveyMode_Get();
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(uint8_t);
               attr->rValTypecast = (uint8_t)uintValue;
            }
         }
         break;


      case pdSurveyStartTime:
         // type = "timeValue"
         if ( sizeof( uint32_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
         {
            retVal = eSUCCESS;
            *(uint32_t *)value = PD_SurveyStartTime_Get();

            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(uint32_t);
               attr->rValTypecast = (uint8_t)timeValue;
            }
         }
         break;

      case pdSurveyPeriod:
         // type = "uint"
         if ( sizeof( uint32_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
         {
            retVal = eSUCCESS;
            *(uint32_t *)value = PD_SurveyPeriod_Get();

            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(uint32_t);
               attr->rValTypecast = (uint8_t)uintValue;
            }
         }
         break;

      case pdZCProductNullingOffset:
         // type = "int"
         if ( sizeof( int32_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
         {
            retVal = eSUCCESS;
            *(int32_t *)value = PD_ZCProductNullingOffset_Get();
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(int32_t);
               attr->rValTypecast = (uint8_t)intValue;
            }
         }
         break;

      case pdLcdMsg:
         {
            /* type = "str" */
            retVal = eSUCCESS;
            uint16_t num_bytes = PD_LcdMsg_Get(value);
            if ( sizeof( uint32_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)num_bytes;
                  attr->rValTypecast = (uint8_t)ASCIIStringValue;
               }
            }
         }
         break;

      case pdBuMaxTimeDiversity:
         // type = "uint"
         if ( sizeof( uint8_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
         {
            retVal = eSUCCESS;
            *(uint16_t *)value = PD_BuMaxTimeDiversity_Get();
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(uint16_t);
               attr->rValTypecast = (uint8_t)uintValue;
            }
         }
         break;

      case pdBuDataRedundancy:
         // type = "uint"
         if ( sizeof( uint8_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
         {
            retVal = eSUCCESS;
            *(uint8_t *)value = PD_BuDataRedundancy_Get();
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(uint8_t);
               attr->rValTypecast = (uint8_t)uintValue;
            }
         }
         break;

      case pdSurveyPeriodQty:
         // type = "uint"
         if ( sizeof( uint8_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
         {
            retVal = eSUCCESS;
            *(uint8_t *)value = PD_SurveyPeriodQty_Get();
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(uint8_t);
               attr->rValTypecast = (uint8_t)uintValue;
            }
         }
         break;


      }  /*lint !e787 not all enums used within switch */
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      switch ( id )  /*lint !e787 not all enums used within switch */
      {
      case pdSurveyQuantity:
         if(attr->rValTypecast == (uint8_t)uintValue)
         {
            if(attr->rValLen == 1)
            {  // uint 1 byte
               retVal = PD_SurveyQuantity_Set(*(uint8_t *)value);
            }
         }
         else
         {
            retVal = eAPP_INVALID_TYPECAST;
         }
         break;

      case pdSurveyMode:
         if(attr->rValTypecast == (uint8_t)uintValue)
         {
            if(attr->rValLen == 1)
            {  // uint 1 byte
               retVal = PD_SurveyMode_Set(*(uint8_t *)value);
            }
         }
         else
         {
            retVal = eAPP_INVALID_TYPECAST;
         }
         break;

      case pdSurveyStartTime:
         if(attr->rValTypecast == (uint8_t)timeValue)
         {
            if(attr->rValLen == 3)
            {
               // Convert from 3 bytes value to 4
               // timeValues are byte reversed before this is called
               // As a result the bytes are reversed from how they were transmitted (Big endian)
               uint8_t *pValue = (uint8_t *)value;
               uint32_t u32_value = (pValue[2] <<  16) | (pValue[1] <<  8)  | pValue[0] ;
               retVal = PD_SurveyStartTime_Set(u32_value);
            }
         }
         else
         {
            retVal = eAPP_INVALID_TYPECAST;
         }
         break;

      case pdSurveyPeriod:
         if( (attr->rValTypecast == (uint8_t)uintValue) )
         {
            if( attr->rValLen == 3 )
            {
               // Convert from 3 bytes value to 4
               // timeValues are byte reversed before this is called
               // As a result the bytes are reversed from how they were transmitted (Big endian)
               uint8_t *pValue = (uint8_t *)value;
               uint32_t u32_value = (pValue[2] <<  16) | (pValue[1] <<  8)  | pValue[0] ;
               retVal = PD_SurveyPeriod_Set(u32_value);
            }
         }
         else
         {
            retVal = eAPP_INVALID_TYPECAST;
         }
         break;

      case pdZCProductNullingOffset:
         // Signed value from -35999 to +35999
         if(attr->rValTypecast == (uint8_t)intValue)
         {
            if(attr->rValLen == 3)
            {
               // Convert from 3 bytes value to 4
               // intValues are byte reversed before this is called
               // As a result the bytes are reversed from how they were transmitted (Big endian)
               uint8_t *pValue = (uint8_t *)value;
               int32_t i32_value = (pValue[2] <<  24) | (pValue[1] <<  16)  | (pValue[0] <<  8) ; /*lint !e701 */
               i32_value = i32_value / 256;
               retVal = PD_ZCProductNullingOffset_Set(i32_value);
            }
         }
         else
         {
            retVal = eAPP_INVALID_TYPECAST;
         }
         break;


      case pdLcdMsg:
         // Require that the length is 6 bytes
         if(attr->rValTypecast == (uint8_t)ASCIIStringValue)
         {
            retVal = PD_LcdMsg_Set(value);
         }
         else
         {
            retVal = eAPP_INVALID_TYPECAST;
         }
         break;

      case pdBuMaxTimeDiversity:
         // uint
         if( attr->rValTypecast == (uint8_t)uintValue )
         {
            if( attr->rValLen == 2 )
            {
               uint16_t u16_value = *(uint16_t *)value;
               retVal = PD_BuMaxTimeDiversity_Set(u16_value);
            }
         }
         else
         {
            retVal = eAPP_INVALID_TYPECAST;
         }
         break;

      case pdBuDataRedundancy:
         // uint
         if(attr->rValTypecast == (uint8_t)uintValue)
         {
            if(attr->rValLen == 1)
            {  // size of 1
               uint8_t u8_value = *(uint8_t *)value;
               retVal = PD_BuDataRedundancy_Set(u8_value);
            }
         }
         else
         {
            retVal = eAPP_INVALID_TYPECAST;
         }
         break;

      case pdSurveyPeriodQty:
         // uint
         if(attr->rValTypecast == (uint8_t)uintValue)
         {  // size of 1
            if(attr->rValLen == 1)
            {
               uint8_t u8_value = *(uint8_t *)value;
               retVal = PD_SurveyPeriodQty_Set(u8_value);
            }
         }
         else
         {
            retVal = eAPP_INVALID_TYPECAST;
         }
         break;

      }  /*lint !e787 not all enums used within switch */
   }
   return retVal;
}
#endif

#if ( MAC_LINK_PARAMETERS == 1 ) // Only used in MAC-LINK Parameters for now
/***********************************************************************************************************************

   Function Name: HEEP_scaleNumber

   Purpose: To convert a number from one scale to unsigned integer scale based on the Input

   Arguments:

   Returns: uint8_t scaled Number

   Note: Output range is 0 to 255.

 **********************************************************************************************************************/
uint8_t HEEP_scaleNumber( double Input, int16_t InputLow, int16_t InputHigh )
{
   uint8_t result = 0;

   if( InputLow != InputHigh )
   {
      /* Formula: ((Input - InputLow) / (InputHigh - InputLow))* (OutputHigh - OutputLow) + OutputLow; */
      result = (uint8_t)round( ((Input-InputLow)/(InputHigh-InputLow ))*255.0 + 0 );
   }
   return result;
}

/***********************************************************************************************************************

   Function Name: HEEP_UnscaleNumber

   Purpose: To convert a Scaled number to

   Arguments:

   Returns: uint8_t scaled Number

 **********************************************************************************************************************/
float HEEP_UnscaleNumber( uint8_t ScaledNum, int16_t InputLow, int16_t InputHigh )
{
   float result = (float)InputLow;

   if( InputHigh != InputLow )
   {
      /* Formula: ( ((ScaledInput-OutputLow)/(OutputHigh-OutputLow))*(InputHigh-InputLow) )+InputLow
                  where OutputLow = 0, OutputHigh=256 Note: Using 256 for this give more accurate result*/
      result = (float)( (((double)ScaledNum)/256)*(double)(InputHigh-InputLow)+InputLow );
   }

   return result;
}
#endif // end of ( MAC_LINK_PARAMETERS == 1 ) section