/**********************************************************************************************************************

   Filename:   MFG_Port.c

   Global Designator: MFGP_

   Contents: routines to support the manufacturing port

 **********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 *********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "project.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <fio.h>
#endif

#if ( MCU_SELECTED == RA6E1 )
#include "hal_data.h"
#endif
#include "radio_hal.h"

#define MFGP_GLOBALS
#include "MFG_Port.h"    // Include self
#undef  MFGP_GLOBALS

#include "SELF_test.h"
#include "ascii.h"
#include "buffer.h"

#include "partition_cfg.h"
#include "mode_config.h"
#include "EVL_event_log.h"
#include "time_util.h"
#include "timer_util.h"
#include "MIMT_info.h"
#include "COMM.h"
#include "limits.h"
#if ( DCU == 1 )
#include "STAR.h"
#include "DCU_cim_cmd.h"
#endif
#if ( EP == 1 )
#include "time_DST.h"
#include "endpt_cim_cmd.h"
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#include "hmc_eng.h"
#endif // ACLARA_LC == 0
#endif
#include "version.h"
#include "dfw_interface.h"
#include "dvr_intFlash_cfg.h"
#include "dvr_extflash.h"
#include "file_io.h"
#include "pwr_task.h"
#include "eng_res.h"
#include "ecc108_lib_return_codes.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include "ecc108_mqx.h"
#elif ( RTOS_SELECTION == FREE_RTOS )
#include "ecc108_freertos.h"
#endif
#include "ecc108_apps.h"

#include "MAC.h"
#include "PHY.h"
#include "PHY_Protocol.h"
#include "STACK.h"
#include "radio.h"
#include "radio_hal.h"
#include "APP_MSG_Handler.h"
#include "Temperature.h"
#include "smtd_config.h"
#if ( EP == 1 )
#include "historyd.h"
#include "OR_MR_Handler.h"
#include "demand.h"
#include "pwr_config.h"
#include "dfwtd_config.h"
#include "ed_config.h"
#include "hmc_app.h"
#include "ID_intervalTask.h"
#include "intf_cim_cmd.h"
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#include "hmc_start.h"
#endif // ACLARA_LC == 0
#include "RG_MD_Handler.h"
#include "pwr_config.h"
#include "psem.h"
#include "time_sys.h"
#endif
#include "DBG_CommandLine.h"

#if ( ENABLE_B2B_COMM == 1 )
#include "hdlc.h"
#endif
#if ( MCU_SELECTED == NXP_K24 )
#include "psp_cpudef.h"
#endif
#ifndef __GNUC__
#include "dfw_app.h"
#endif

#include "DBG_SerialDebug.h"
/*lint -esym(715,argc,argv,cmd,pData,ptr) -esym(818,argc,argv) */

#if (USE_DTLS == 1)
#if ( RTOS_SELECTION == MQX_RTOS )
#include "serial.h"
#endif
#include "dtls.h"
#endif
#if (USE_MTLS == 1)
#include "mtls.h"
#endif
#if (ACLARA_LC ==1)
#include "ilc_srfn_reg.h"
#endif
#include "SM_Protocol.h"
#include "sm.h"
#include "time_sync.h"

#if (ACLARA_DA == 1)
#include "da_srfn_reg.h"
#include "b2bMessage.h"
#endif
#if (USE_USB_MFG == 1)
#include "virtual_com.h"
#include "usb_descriptor.h"
#include "usb_error.h"
#include "usb_class.h"
#endif

#if (PHASE_DETECTION == 1)
#include "PhaseDetect.h"
#endif
#if ( ( !USE_USB_MFG && ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A ) ) || ( TM_ROUTE_UNKNOWN_MFG_CMDS_TO_DBG == 1 ) )
#include "DBG_CommandLine.h"
#endif
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
/*lint -esym(750,ESCAPE_CHAR,LINE_FEED_CHAR,CARRIAGE_RETURN_CHAR,BACKSPACE_CHAR,TILDA_CHAR,WHITE_SPACE_CHAR)   */
#define ECHO_OPTICAL_PORT 0
#if ( ECHO_OPTICAL_PORT != 0 )
#warning "Do not release with ECHO_OPTICAL_PORT set non-zero"
#endif
#define MAX_DTLS_DISCONNECT_TRIES (5)      /* Maximum times to wait for a buffer to be freed */
#define MFGP_MAX_MFG_COMMAND_CHARS 1024    /* Root certificate may be up to 512 bytes  */
#define MFGP_MAX_CMDLINE_ARGS 6
#define CTRL_C_CHAR 0x03
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
#define UART_SWITCH_CHAR 0x81
#define OPTO_PKT_TIMEOUT 2       /* Number seconds allowed between STP and packet received.  */
#endif
#define ESCAPE_CHAR           0x1B
#define LINE_FEED_CHAR        0x0A
#define CARRIAGE_RETURN_CHAR  0x0D
#define BACKSPACE_CHAR        0x08
#define TILDA_CHAR            0x7E
#define WHITE_SPACE_CHAR      0x20
#define DELETE_CHAR           0x7f
#if ( EP == 1 )
#define RFTEST_MODE_TIMEOUT (8U * TIME_TICKS_PER_HR)  /* time out of 8 hours   */
#endif

#if USE_USB_MFG
#define MFG_puts( port, msg, len ) usb_puts( (char *)msg )
#else
#define MFG_puts( port, msg, len ) (void)UART_write( port, msg, len )
#endif

#if( RTOS_SELECTION == FREE_RTOS )
#define MFG_NUM_MSGQ_ITEMS 20 //NRJ: TODO Figure out sizing
#else
#define MFG_NUM_MSGQ_ITEMS 0
#endif

/* Temporary definition of MFG_logPrintf()   */
#if (USE_DTLS == 1)
#define MFG_logPrintf MFG_printf
#define MFG_printf(fmt, args...) \
{ \
   if ((_MfgPortState == DTLS_SERIAL_IO_e) && (DTLS_GetSessionState() == DTLS_SESSION_CONNECTED_e)) \
   { \
      (void)memset(MFGP_CommandBuffer, 0, sizeof(MFGP_CommandBuffer)); \
      MFGP_CmdLen = (uint16_t)snprintf(MFGP_CommandBuffer, (int32_t)sizeof(MFGP_CommandBuffer), fmt, ##args); \
      MFGP_EncryptBuffer(MFGP_CommandBuffer, MFGP_CmdLen); \
   } \
   else \
   { \
      MFGP_CmdLen = (uint16_t)snprintf(MFGP_CommandBuffer, (int32_t)sizeof(MFGP_CommandBuffer), fmt, ##args); \
      MFG_puts( mfgUart, (uint8_t *)MFGP_CommandBuffer, MFGP_CmdLen ); \
   } \
}
#else
#define MFG_logPrintf MFG_printf
#define MFG_printf (void)DBG_printfNoCr
#endif  // USE_DTLS

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static uint16_t         MFGP_CmdLen;
static char             MFGP_CommandBuffer[MFGP_MAX_MFG_COMMAND_CHARS + 1];
static uint16_t         MFGP_numBytes = 0;               /* Number of bytes currently in the command buffer */
static char           * argvar[MFGP_MAX_CMDLINE_ARGS + 1];
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
static enum_UART_ID mfgUart = UART_MANUF_TEST;           /* UART used for MFG port operations   */
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
#if USE_USB_MFG
static const enum_UART_ID mfgUart = MAX_UART_ID;         /* Dummy value when USE_USB_MFG is used */
static OS_EVNT_Obj      CMD_events;                      /* Used to suspend task requesting command execution  */
static uint32_t         event_flags;                     /* Event flags returned command handler.                 */
#else
#error "USE_USB_MFG need to be set to 1 for this board"
#endif
#else
static const enum_UART_ID mfgUart = UART_MANUF_TEST;     /* UART used for MFG port operations   */
#endif
#if ( EP == 1 )
static uint16_t         stP0LoopbackFailCount = 0;       /* HEEP required port 0 loopback test failure counter */
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
static bool             loggedOn = (bool)false;
static uint16_t         OptoTimerID;                     /* Optical port timeout timer ID  */
static timer_t          OptoTimerCfg;                    /* Optical port timeout Timer configuration */
#endif
#endif
static OS_EVNT_Obj    * SELF_notify;                     /* Event handler used to request a test   */
static OS_EVNT_Obj      MFG_notify;                      /* Event handler to "notify" when test results are done.  */
#if (USE_DTLS == 1)
static mfgPortState_e   _MfgPortState;                   /* Port state */
static const char       mfgpLockInEffect[] = {"LOCK IN EFFECT\n\r"}; /* When port is in tarpit this msg is returned  */
#endif
static OS_EVNT_Obj      _MfgpUartEvent;
static OS_MSGQ_Obj      _CommandReceived_MSGQ;
#if ( EP == 1 )
static timer_t          rfTestModeTimerCfg;              /* rfTestMode timeout Timer configuration */
#endif

static uint16_t         uStTxCwTestCompleteTimerId = 0;  /* Time out timer for StTxCwTest */

#if ( !USE_USB_MFG && ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A ) )
extern char             DbgCommandBuffer[];
void                    DBG_CommandLine_Process ( void );
#endif
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
typedef void ( *Fxn_CmdLine )( uint32_t argc, char *argv[] ); /*lint !e452 differs from DBG_CommandLine.c  */
/* lint --e{452} --e{631} differs from DBG_CommandLine.c  */
/*lint -esym(452,631,struct_CmdLineEntry) differs from DBG_CommandLine.c  */
typedef struct
{
   const char *pcCmd;   //! A pointer to a string containing the name of the command.
   Fxn_CmdLine pfnCmd;  //! A function pointer to the implementation of the command.
   const char *pcHelp;  //! A pointer to a string of brief help text for the command.
} struct_CmdLineEntry;
/*lint -esym(749,menuType::menuEp, menuType::menuStd) may not be referenced   */
typedef enum menuType
{
#if ( EP == 1 )
   menuQuiet,
   menuEp,
#endif
   menuStd,
   menuHidden,
#if (USE_DTLS == 1)
   menuSecure,
#endif
   menuLastValid
} menuType;

typedef enum
{
   channelUnits,
   frequencyUnits
} enum_radioUnits_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
typedef enum
{
   U8_VALUE_TYPE,
   I8_VALUE_TYPE ,
   U16_VALUE_TYPE,
   I16_VALUE_TYPE,
   U32_VALUE_TYPE,
   I32_VALUE_TYPE,
   TIME_VALUE_TYPE,
   BOOL_VALUE_TYPE,
   FLOAT_VALUE_TYPE
} ValueType_e;

typedef union{
   uint8_t       u8;
   int8_t        i8;
   uint16_t      u16;
   int16_t       i16;
   uint32_t      u32;
   int32_t       i32;
   TIMESTAMP_t   time;
   bool          bValue;
   float         fValue;
}value_t;

static bool strto_uint8(const char* argv, uint8_t *u8);
static bool strto_uint16(const char* argv, uint16_t *u16);
static bool strto_uint32(const char* argv, uint32_t *u32);
static bool strto_int8(const char* argv, int8_t *i8);
static bool strto_int16(const char* argv, int16_t *i16);
static bool strto_int32(const char* argv, int32_t *i32);
static bool strto_float(const char* argv, float *fValue);
static bool strto_bool(const char* argv, bool *bValue);
static bool strto_time(const char* argv, TIMESTAMP_t *timeStamp);

static void print_parameter_count_error(void);
static bool strto_value(const char* argv, ValueType_e valueType, value_t * pValue);
static void print_value(const char* argv, ValueType_e valueType, const value_t * pValue);

//static void NwkAttrSetGet(uint32_t argc, char *argv[], NWK_ATTRIBUTES_e eAttribute, ValueType_e valueType);
static void MacAttrSetGet(uint32_t argc, char *argv[], MAC_ATTRIBUTES_e eAttribute, ValueType_e valueType);
//static void PhyAttrSetGet(uint32_t argc, char *argv[], PHY_ATTRIBUTES_e eAttribute, ValueType_e valueType);

#if ( EP == 1 )
static void MFGP_watchDogResetCount( uint32_t argc, char *argv[] );
static void MFG_PhyDemodulator( uint32_t argc, char *argv[] );
#endif

static void MFGP_macReliabilityHighCount     ( uint32_t argc, char *argv[] );
static void MFGP_macReliabilityMedCount      ( uint32_t argc, char *argv[] );
static void MFGP_macReliabilityLowCount      ( uint32_t argc, char *argv[] );
static void MFGP_capableOfEpBootloaderDFW    ( uint32_t argc, char *argv[] );
static void MFGP_capableOfEpPatchDFW         ( uint32_t argc, char *argv[] );
static void MFGP_capableOfMeterBasecodeDFW   ( uint32_t argc, char *argv[] );
static void MFGP_capableOfMeterPatchDFW      ( uint32_t argc, char *argv[] );
static void MFGP_capableOfMeterReprogrammingOTA( uint32_t argc, char *argv[] );
static void MFG_printHex( char* msg, uint8_t const *pData, uint16_t numBytes);
static void MFG_appSecAuthMode               ( uint32_t argc, char *argv[] );
static void MFG_CommandLine_MacAddr          ( uint32_t argc, char *argv[] );
static void MFG_DateTime                     ( uint32_t argc, char *argv[] );
static void MFG_enableDebug                  ( uint32_t argc, char *argv[] );
static void MFGP_enableOTATest               ( uint32_t argc, char *argv[] );

#if ( PHASE_DETECTION == 1 )
static void PD_SurveySelfAssessment          ( uint32_t argc, char *argv[] );
static void PD_SurveyCapable                 ( uint32_t argc, char *argv[] );
static void PD_SurveyPeriod                  ( uint32_t argc, char *argv[] );
static void PD_SurveyStartTime               ( uint32_t argc, char *argv[] );
static void PD_SurveyQuantity                ( uint32_t argc, char *argv[] );
static void PD_SurveyReportMode              ( uint32_t argc, char *argv[] );
static void PD_ZCProductNullingOffset        ( uint32_t argc, char *argv[] );
static void PD_LcdMsg                        ( uint32_t argc, char *argv[] );
static void PD_BuMaxTimeDiversity            ( uint32_t argc, char *argv[] );
static void PD_BuDataRedundancy              ( uint32_t argc, char *argv[] );
static void PD_SurveyPeriodQty               ( uint32_t argc, char *argv[] );
#endif

static void MFG_PhyAvailableFrequencies      ( uint32_t argc, char *argv[] );
static void MFG_PhyAvailableChannels         ( uint32_t argc, char *argv[] );
static void MFG_PhyRxFrequencies             ( uint32_t argc, char *argv[] );
static void MFG_PhyRxChannels                ( uint32_t argc, char *argv[] );
static void MFG_PhyTxFrequencies             ( uint32_t argc, char *argv[] );
static void MFG_PhyFailedFrameDecodeCount    ( uint32_t argc, char *argv[] );
static void MFG_PhyFailedHcsCount            ( uint32_t argc, char *argv[] );
static void MFG_PhyFramesReceivedCount       ( uint32_t argc, char *argv[] );
static void MFG_PhyFramesTransmittedCount    ( uint32_t argc, char *argv[] );
static void MFG_PhyFailedHeaderDecodeCount   ( uint32_t argc, char *argv[] );
static void MFG_PhySyncDetectCount           ( uint32_t argc, char *argv[] );
static void MFG_PhyTxChannels                ( uint32_t argc, char *argv[] );
static void MFG_PhyCcaThreshold              ( uint32_t argc, char *argv[] );
static void MFG_PhyCcaAdaptiveThresholdEnable( uint32_t argc, char *argv[] );
static void MFG_PhyCcaOffset                 ( uint32_t argc, char *argv[] );
static void MFG_PhyFrontEndGain              ( uint32_t argc, char *argv[] );
static void MFG_PhyMaxTxPayload              ( uint32_t argc, char *argv[] );
static void MFG_PhyNumchannels               ( uint32_t argc, char *argv[] );
static void MFG_PhyRcvrCount                 ( uint32_t argc, char *argv[] );
static void MFGP_macPacketTimeout            ( uint32_t argc, char *argv[] );

static void MFGP_macChannelSetsCount         ( uint32_t argc, char *argv[] );
static void MFGP_macState                    ( uint32_t argc, char *argv[] );
static void MFGP_macTxFrames                 ( uint32_t argc, char *argv[] );
static void MFGP_macAckWaitDuration          ( uint32_t argc, char *argv[] );
static void MFGP_macAckDelayDuration         ( uint32_t argc, char *argv[] );
static void MFGP_macPacketId                 ( uint32_t argc, char *argv[] );
static void MFGP_macIsChannelAccessConstrained ( uint32_t argc, char *argv[] );
static void MFGP_macIsFNG                    ( uint32_t argc, char *argv[] );
static void MFGP_macIsIAG                    ( uint32_t argc, char *argv[] );
static void MFGP_macIsRouter                 ( uint32_t argc, char *argv[] );

static void MFGP_macPingCount                ( uint32_t argc, char *argv[] );
static void MFGP_macCsmaMaxAttempts          ( uint32_t argc, char *argv[] );
static void MFGP_macCsmaMinBackOffTime       ( uint32_t argc, char *argv[] );
static void MFGP_macCsmaMaxBackOffTime       ( uint32_t argc, char *argv[] );
static void MFGP_macCsmaPValue               ( uint32_t argc, char *argv[] );
static void MFGP_macCsmaQuickAbort           ( uint32_t argc, char *argv[] );
static void MFGP_macReassemblyTimeout        ( uint32_t argc, char *argv[] );
static void MFGP_inboundFrameCount           ( uint32_t argc, char *argv[] );
static void MFGP_amBuMaxTimeDiversity        ( uint32_t argc, char *argv[] );
static void MFGP_fctModuleTestDate           ( uint32_t argc, char *argv[] );
static void MFGP_fctEnclosureTestDate        ( uint32_t argc, char *argv[] );
static void MFGP_integrationSetupDate        ( uint32_t argc, char *argv[] );
static void MFGP_fctModuleProgramVersion     ( uint32_t argc, char *argv[] );
static void MFGP_fctEnclosureProgramVersion  ( uint32_t argc, char *argv[] );
static void MFGP_integrationProgramVersion   ( uint32_t argc, char *argv[] );
static void MFGP_fctModuleDatabaseVersion    ( uint32_t argc, char *argv[] );
static void MFGP_fctEnclosureDatabaseVersion ( uint32_t argc, char *argv[] );
static void MFGP_integrationDatabaseVersion  ( uint32_t argc, char *argv[] );
static void MFGP_dataConfigurationDocumentVersion( uint32_t argc, char *argv[] );
static void MFGP_manufacturerNumber          ( uint32_t argc, char *argv[] );
static void MFGP_repairInformation           ( uint32_t argc, char *argv[] );
static void MFGP_rtcDateTime                 ( uint32_t argc, char *argv[] );
static void MFG_PhyNoiseEstimate             ( uint32_t argc, char *argv[] );
static void MFG_PhyNoiseEstimateRate         ( uint32_t argc, char *argv[] );
static void MFG_PhyRxDetection               ( uint32_t argc, char *argv[] );
static void MFG_PhyRxFraming                 ( uint32_t argc, char *argv[] );
static void MFG_PhyRxMode                    ( uint32_t argc, char *argv[] );
static void MFG_PhyThermalControlEnable      ( uint32_t argc, char *argv[] );
static void MFG_PhyThermalProtectionEnable   ( uint32_t argc, char *argv[] );
static void MFG_PhyThermalProtectionCount    ( uint32_t argc, char *argv[] );
static void MFG_PhyThermalProtectionEngaged  ( uint32_t argc, char *argv[] );

#if ( DCU == 1 )
static void MFGP_DCUVersion                  ( uint32_t argc, char *argv[] );
static void MFG_ToolEP_SDRAMTest             ( uint32_t argc, char *argv[] );
static void MFG_ToolEP_SDRAMCount            ( uint32_t argc, char *argv[] );
static void MFGP_DeviceGatewayConfig         ( uint32_t argc, char *argv[] );
static void MFG_StTxBlurtTest_Star           ( void );
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
static void MFGP_fngFemMfg                   ( uint32_t argc, char *argv[] );
static void MFGP_fngForwardPower             ( uint32_t argc, char *argv[] );
static void MFGP_fngForwardPowerSet          ( uint32_t argc, char *argv[] );
static void MFGP_fngFowardPowerLowSet        ( uint32_t argc, char *argv[] );
static void MFGP_fngReflectedPower           ( uint32_t argc, char *argv[] );
static void MFGP_fngVSWR                     ( uint32_t argc, char *argv[] );
static void MFGP_fngVswrNotificationSet      ( uint32_t argc, char *argv[] );
static void MFGP_fngVswrShutdownSet          ( uint32_t argc, char *argv[] );
#endif
#endif
#if (TX_THROTTLE_CONTROL == 1 )
static void MFG_TxThrottleDisable            ( uint32_t argc, char *argv[] );
#endif

static void MFG_TimePrecision                ( uint32_t argc, char *argv[] );
static void MFG_TimeDefaultAccuracy          ( uint32_t argc, char *argv[] );
static void MFG_TimeQueryResponseMode        ( uint32_t argc, char *argv[] );

#if ( EP == 1 )
static void MFG_TimeSetStart                 ( uint32_t argc, char *argv[] );
static void MFG_TimeSetMaxOffset             ( uint32_t argc, char *argv[] );
static void MFG_TimeSetPeriod                ( uint32_t argc, char *argv[] );
static void MFG_TimeSource                   ( uint32_t argc, char *argv[] );
static void MFGP_timeSigOffset               ( uint32_t argc, char *argv[] );
static void MFGP_powerQuality                ( uint32_t argc, char *argv[] );
#endif

#if  ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
static void MFG_LinkParametersPeriod         ( uint32_t argc, char *argv[] );
static void MFG_LinkParametersStart          ( uint32_t argc, char *argv[] );
static void MFG_LinkParametersMaxOffset      ( uint32_t argc, char *argv[] );
static void MFG_ReceivePowerMargin           ( uint32_t argc, char *argv[] );
#endif
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
static void MFG_MacCommandResponseMaxTimeDiversity      ( uint32_t argc, char *argv[] );
#endif

static void MFG_TransactionTimeout           ( uint32_t argc, char *argv[] );
static void MFG_TransactionTimeoutCount      ( uint32_t argc, char *argv[] );
static void MFG_macTxLinkDelayCount          ( uint32_t argc, char *argv[] );
static void MFG_macTxLinkDelayTime           ( uint32_t argc, char *argv[] );
static void MFG_reboot                       ( uint32_t argc, char *argv[] );
static void MFG_stRTCFailCount               ( uint32_t argc, char *argv[] );
static void MFG_stRTCFailTest                ( uint32_t argc, char *argv[] );
static void MFG_StRx4GFSK                    ( uint32_t argc, char *argv[] );
static void MFG_StTxBlurtTest                ( uint32_t argc, char *argv[] );
static void MFG_StTxCwTest                   ( uint32_t argc, char *argv[] );
static void MFGP_MacChannelSets( uint32_t argc, char *argv[], MAC_ATTRIBUTES_e channelSetsAttribute, MAC_ATTRIBUTES_e channelSetsCountAttribute, uint8_t maxChannelSets);
static void MFGP_MacChannelSetsSRFN          ( uint32_t argc, char *argv[] );
#if ( DCU == 1 )
static void MFGP_MacChannelSetsSTAR          ( uint32_t argc, char *argv[] );
#endif

static void MFGP_CommandLine_Help            ( uint32_t argc, char *argv[] );
static void MFGP_DeviceType                  ( uint32_t argc, char *argv[] );
static void MFGP_dtlsDeviceCertificate       ( uint32_t argc, char *argv[] );
static void MFGP_dtlsMfgSubject1             ( uint32_t argc, char *argv[] );
static void MFGP_dtlsMfgSubject2             ( uint32_t argc, char *argv[] );
static void MFGP_dtlsNetworkHESubject        ( uint32_t argc, char *argv[] );
static void MFGP_dtlsNetworkMSSubject        ( uint32_t argc, char *argv[] );
static void MFGP_dtlsNetworkRootCA           ( uint32_t argc, char *argv[] );
static void MFGP_engBuEnabled                ( uint32_t argc, char *argv[] );
static void MFGP_engData1                    ( uint32_t argc, char *argv[] );
static void MFGP_engBuTrafficClass           ( uint32_t argc, char *argv[] );
#if ( EP == 1 )
static void MFGP_engData2                    ( uint32_t argc, char *argv[] );
#endif
static void MFGP_eventThreshold              ( uint32_t argc, char *argv[] );
static void MFGP_firmwareVersion             ( uint32_t argc, char *argv[] );
static void MFGP_hardwareVersion             ( uint32_t argc, char *argv[] );
static void MFGP_ipHEContext                 ( uint32_t argc, char *argv[] );
static void MFGP_macNetworkId                ( uint32_t argc, char *argv[] );
static void MFGP_nvFailCount                 ( uint32_t argc, char *argv[] );
static void MFGP_nvtest                      ( uint32_t argc, char *argv[] );

static void MFGP_ProcessCommand( char *command, uint16_t numBytes );

static void MFGP_shipMode                    ( uint32_t argc, char *argv[] );
static void MFGP_SpuriousResetCount          ( uint32_t argc, char *argv[] );
static void MFGP_stSecurityFailCount         ( uint32_t argc, char *argv[] );
static void MFGP_stSecurityFailTest          ( uint32_t argc, char *argv[] );
static void MFGP_FlashSecurity               ( uint32_t argc, char *argv[] );
static void MFGP_virgin                      ( uint32_t argc, char *argv[] );
static void MFGP_virginDelay                 ( uint32_t argc, char *argv[] );
static void MFGP_opportunisticAlarmIndexId   ( uint32_t argc, char *argv[] );
static void MFGP_realTimeAlarmIndexId        ( uint32_t argc, char *argv[] );
static void MFGP_temperature                 ( uint32_t argc, char *argv[] );
#if ( EP == 1 )
static void MFGP_epMaxTemperature            ( uint32_t argc, char *argv[] );
static void MFGP_epMinTemperature            ( uint32_t argc, char *argv[] );
static void MFGP_EpTempHysteresis            ( uint32_t argc, char *argv[] );
static void MFGP_HighTempThreshold           ( uint32_t argc, char *argv[] );
static void MFGP_EpTempMinThreshold          ( uint32_t argc, char *argv[] );
#endif
static void MFGP_installationDateTime        ( uint32_t argc, char *argv[] );
static void MFGP_getRealTimeAlarm            ( uint32_t argc, char *argv[] );
static void MFGP_timeLastUpdated             ( uint32_t argc, char *argv[] );
static void MFGP_timeState                   ( uint32_t argc, char *argv[] );

#if (EP == 1)
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
static void MFGP_dfwCompatibilityTestStatus  ( uint32_t argc, char *argv[] );
static void MFGP_dfwProgramScriptStatus      ( uint32_t argc, char *argv[] );
static void MFGP_dfwAuditTestStatus          ( uint32_t argc, char *argv[] );
#endif
static void MFGP_timeZoneDSTHash             ( uint32_t argc, char *argv[] );
static void MFGP_timeZoneOffset              ( uint32_t argc, char *argv[] );
static void MFGP_dfwDupDiscardPacketQty      ( uint32_t argc, char *argv[] );
static void MFGP_dstEnabled                  ( uint32_t argc, char *argv[] );
static void MFGP_dstEndRule                  ( uint32_t argc, char *argv[] );
static void MFGP_dstOffset                   ( uint32_t argc, char *argv[] );
static void MFGP_dstStartRule                ( uint32_t argc, char *argv[] );
#if ( SAMPLE_METER_TEMPERATURE == 1 )
static void MFGP_edTemperatureHystersis      ( uint32_t argc, char *argv[] );
static void MFGP_edTempSampleRate            ( uint32_t argc, char *argv[] );
#endif
static void MFGP_dateTimeLostCount           ( uint32_t argc, char *argv[] );
static void MFGP_timeRequestMaxTimeout       ( uint32_t argc, char *argv[] );
static void MFGP_initialRegistrationTimemout ( uint32_t argc, char *argv[] );
static void MFGP_minRegistrationTimemout     ( uint32_t argc, char *argv[] );
static void MFGP_maxRegistrationTimemout     ( uint32_t argc, char *argv[] );
static void MFGP_stP0LoopbackFailCount       ( uint32_t argc, char *argv[] );
static void MFGP_stP0LoopbackFailTest        ( uint32_t argc, char *argv[] );
static void MFGP_quietMode                   ( uint32_t argc, char *argv[] );
static void MFGP_rfTestMode                  ( uint32_t argc, char *argv[] );
static void MFGP_newRegistrationRequired     ( uint32_t argc, char *argv[] );
static void MFGP_smLogTimeDiversity          ( uint32_t argc, char *argv[] );
static void MFGP_timeAcceptanceDelay         ( uint32_t argc, char *argv[] );
#if ( ENABLE_HMC_TASKS == 1 )
static void MFG_bulkQuantity                 ( uint32_t argc, char *argv[] );
#endif
static void MFGP_outageDeclarationDelay      ( uint32_t argc, char *argv[] );
static void MFGP_restorationDelay            ( uint32_t argc, char *argv[] );
static void MFGP_powerQualityEventDuration   ( uint32_t argc, char *argv[] );
static void MFGP_lastGaspMaxNumAttempts      ( uint32_t argc, char *argv[] );
#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
static void MFGP_simulateLastGaspStart       ( uint32_t argc, char *argv[] );
static void MFGP_simulateLastGaspDuration    ( uint32_t argc, char *argv[] );
static void MFGP_SimulateLastGaspTraffic     ( uint32_t argc, char *argv[] );
static void MFGP_simulateLastGaspMaxNumAttempts( uint32_t argc, char *argv[] );
static void MFGP_simulateLastGaspStatCcaAttempts( uint32_t argc, char *argv[] );
static void MFGP_simulateLastGaspStatPPersistAttempts( uint32_t argc, char *argv[] );
static void MFGP_simulateLastGaspStatMsgsSent( uint32_t argc, char *argv[] );
#endif
static void MFGP_edInfo                      ( uint32_t argc, char *argv[] );
static void MFGP_edFwVersion                 ( uint32_t argc, char *argv[] );
static void MFGP_invalidAddressModeCount     ( uint32_t argc, char *argv[] );
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
static void MFG_logoff                       ( uint32_t argc, char *argv[] );
static void OptoPortTimerReset ( void );
#endif
#if ( ANSI_STANDARD_TABLES == 1 ) || ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 )
static void MFGP_edHwVersion                 ( uint32_t argc, char *argv[] );
static void MFGP_edModel                     ( uint32_t argc, char *argv[] );
#endif
#if ( ANSI_STANDARD_TABLES == 1 )
static void MFGP_AnsiTblOID                  ( uint32_t argc, char *argv[] );
static void MFGP_edManufacturer              ( uint32_t argc, char *argv[] );
static void MFGP_edProgrammedDateTime        ( uint32_t argc, char *argv[] );
static void MFGP_edProgramId                 ( uint32_t argc, char *argv[] );
static void MFGP_edProgrammerName            ( uint32_t argc, char *argv[] );
#elif ( ACLARA_DA == 1 )
static void MFGP_edManufacturer              ( uint32_t argc, char *argv[] );
#endif
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
static void MFGP_meterCommunicationLockoutCount( uint32_t argc, char *argv[] );
#if ( LP_IN_METER == 0 )
static void MFGP_lpBuChannel                 ( uint32_t argc, char *argv[] );
#endif
static void MFGP_lpBuDataRedundancy          ( uint32_t argc, char *argv[] );
static void MFGP_lpBuMaxTimeDiversity        ( uint32_t argc, char *argv[] );
static void MFGP_lpBuSchedule                ( uint32_t argc, char *argv[] );
static void MFGP_lpBuTrafficClass            ( uint32_t argc, char *argv[] );
static void MFGP_orReadList                  ( uint32_t argc, char *argv[] );
static void MFG_meterPassword                ( uint32_t argc, char *argv[] );
static void MFGP_passwordPort0Master         ( uint32_t argc, char *argv[] );
static void MFGP_lpBuEnabled                 ( uint32_t argc, char *argv[] );
static void MFGP_dsBuEnabled                 ( uint32_t argc, char *argv[] );
static void MFGP_meterSessionFailureCount    ( uint32_t argc, char *argv[] );
static void MFGP_historicalRecovery          ( uint32_t argc, char *argv[] );
static void MFGP_dsBuDataRedundancy          ( uint32_t argc, char *argv[] );
static void MFGP_dsBuMaxTimeDiversity        ( uint32_t argc, char *argv[] );
static void MFGP_dsBuReadingTypes            ( uint32_t argc, char *argv[] );
static void MFGP_dsBuTrafficClass            ( uint32_t argc, char *argv[] );
static void MFGP_dailySelfReadTime           ( uint32_t argc, char *argv[] );
#if ( ENABLE_DEMAND_TASKS == 1 )
static void MFGP_drReadList                  ( uint32_t argc, char *argv[] );
static void MFGP_demandFutureConfiguration   ( uint32_t argc, char *argv[] );
static void MFGP_demandPresentConfiguration  ( uint32_t argc, char *argv[] );
#if ( DEMAND_IN_METER == 0 )
static void MFGP_scheduledDemandResetDay     ( uint32_t argc, char *argv[] );
#endif
static void MFGP_demandResetLockoutPeriod    ( uint32_t argc, char *argv[] );
#endif //#if ( ENABLE_DEMAND_TASKS == 1 )
#endif   /* endif of (ACLARA_LC == 0)  */
static void MFGP_edMfgSerialNumber           ( uint32_t argc, char *argv[] );
static void MFGP_edUtilitySerialNumber       ( uint32_t argc, char *argv[] );
static void MFGP_dfwApplyConfirmTimeDiversity( uint32_t argc, char *argv[] );
static void MFGP_dfwDownloadConfirmTimeDiversity( uint32_t argc, char *argv[] );
static void MFGP_dfwStatus                   ( uint32_t argc, char *argv[] );
static void MFGP_dfwStreamId                 ( uint32_t argc, char *argv[] );
static void MFGP_dfwFileType                 ( uint32_t argc, char *argv[] );
static void MFGP_ConfigGet                   ( uint32_t argc, char *argv[] );
static void MFGP_PhyAfcEnable                ( uint32_t argc, char *argv[] );
static void MFGP_PhyAfcRSSIThreshold         ( uint32_t argc, char *argv[] );
static void MFGP_PhyAfcTemperatureRange      ( uint32_t argc, char *argv[] );
static void MFGP_meterShopMode               ( uint32_t argc, char *argv[] );

#if (ACLARA_DA == 1)
static void MFGP_hostBootVersion             ( uint32_t argc, char *argv[] );
static void MFGP_hostKernelVersion           ( uint32_t argc, char *argv[] );
static void MFGP_hostBspVersion              ( uint32_t argc, char *argv[] );
static void MFGP_carrierHwVersion            ( uint32_t argc, char *argv[] );
static void MFGP_hostEchoTest                ( uint32_t argc, char *argv[] );
#endif

#if ( REMOTE_DISCONNECT == 1 )
static void MFGP_switchPosition              ( uint32_t argc, char *argv[] );
static void MFGP_disconnectCapable           ( uint32_t argc, char *argv[] );
#endif
#if ( CLOCK_IN_METER == 1 )
static void MFGP_meterDateTime               ( uint32_t argc, char *argv[] );
#endif
#if ( ENABLE_FIO_TASKS == 1 )
static void MFGP_ConfigTest                  ( uint32_t argc, char *argv[] );
static void MFGP_ConfigUpdate                ( uint32_t argc, char *argv[] );
#endif

static void MFGP_NwPassActTimeout            ( uint32_t argc, char *argv[] );
static void MFGP_NwActiveActTimeout          ( uint32_t argc, char *argv[] );

#endif   //#if ( EP == 1 )

#if (USE_DTLS == 1)
static void MFG_disconnectDtls               ( uint32_t argc, char *argv[] );
static void MFG_startDTLSsession             ( uint32_t argc, char *argv[] );
static void MFGP_maxAuthenticationTimeout    ( uint32_t argc, char *argv[] );
static void MFGP_minAuthenticationTimeout    ( uint32_t argc, char *argv[] );
static void MFGP_initialAuthenticationTimeout( uint32_t argc, char *argv[] );
static void MFGP_dtlsServerCertificateSN     ( uint32_t argc, char *argv[] );
#endif
#if (USE_MTLS == 1)
static void MFGP_mtlsAuthenticationWindow    ( uint32_t argc, char *argv[] );
static void MFGP_mtlsNetworkTimeVariation    ( uint32_t argc, char *argv[] );
#endif
static void MFGP_macRSSI                     ( uint32_t argc, char *argv[] );

#if (VSWR_MEASUREMENT == 1)
extern float cachedVSWR;
static void MFGP_Vswr                        ( uint32_t argc, char *argv[] );
#endif
#if ( TM_UART_ECHO_COMMAND == 1 )
static void MFGP_CommandLine_EchoComment     ( uint32_t argc, char *argv[] );
#endif
static void StTxCwTestCompletedTimerExpired(uint8_t cmd, void *pData);

static const struct_CmdLineEntry MFGP_CmdTable[] =
{
   {  "help",                       MFGP_CommandLine_Help,           "Display list of commands" },
   {  "h",                          MFGP_CommandLine_Help,           "Alias for help" },
   {  "?",                          MFGP_CommandLine_Help,           "Alias for help" },
// { "alarmMaskProfile",            MFGP_alarmMaskProfile,           "xxx" },  // Was commented in base code (K24)
   {  "amBuMaxTimeDiversity",       MFGP_amBuMaxTimeDiversity,       "Get/Set window of time in minutes during which a /bu/am message may bubble-in" },
   {  "capableOfEpBootloaderDFW",   MFGP_capableOfEpBootloaderDFW,   "Indicates if the device supports the Download Firmware feature for its code"},
   {  "capableOfEpPatchDFW",        MFGP_capableOfEpPatchDFW,        "Indicates if the device supports the Download Firmware feature for its bootloader"},
   {  "capableOfMeterBasecodeDFW",  MFGP_capableOfMeterBasecodeDFW,  "Indicates if the device supports the Download Firmware feature for its meter base code"},
   {  "capableOfMeterPatchDFW",     MFGP_capableOfMeterPatchDFW,     "Indicates if the device supports the Download Firmware feature for its meter patch code"},
   {  "capableOfMeterReprogrammingOTA", MFGP_capableOfMeterReprogrammingOTA, "Indicates if the device supports the Download Firmware feature for meter configuration change" },
   {  "comdevicefirmwareversion",   MFGP_firmwareVersion,            "Get firmware version" },
   {  "comDeviceBootloaderVersion", MFGP_firmwareVersion,            "Get firmware version" },
   {  "comdevicehardwareversion",   MFGP_hardwareVersion,            "Get hardware version" },
   {  "comdevicemacaddress",        MFG_CommandLine_MacAddr,         "Read MAC address" },
   {  "comDeviceType",              MFGP_DeviceType,                 "Get Device Type" },
#if ( DCU == 1 )
   {  "comDeviceGatewayConfig",     MFGP_DeviceGatewayConfig,        "Get/Set Device Gateway Configuration" },
#endif
#if ( ENABLE_FIO_TASKS == 1 )
   {  "config_test",                MFGP_ConfigTest,                 "Test the configuration"},
   {  "config_update",              MFGP_ConfigUpdate,               "Update the Configuration Check Information"},
#endif
   {  "dataConfigurationDocumentVersion", MFGP_dataConfigurationDocumentVersion, "(MIMT)Get/Set the data Configuration document version" },
   {  "DateTime",                   MFG_DateTime,                    "Get/Set system date/time" },
   {  "dtlsDeviceCertificate",      MFGP_dtlsDeviceCertificate,      "Read Device cert" },                               // 1363
   {  "dtlsMfgSubject1",            MFGP_dtlsMfgSubject1,            "Read Mfg1 Subject partial cert" },                 // 1359
   {  "dtlsMfgSubject2",            MFGP_dtlsMfgSubject2,            "Read/Write Mfg2 Subject partial cert" },           // 1360
   {  "dtlsNetworkHESubject",       MFGP_dtlsNetworkHESubject,       "Read/Write Head End Subject partial cert" },       // 1332
   {  "dtlsNetworkMSSubject",       MFGP_dtlsNetworkMSSubject,       "Read/Write Meter Shop Subject partial cert" },     // 1333
#if 0 // Turned off in K24
//These are defined in the HEEP, but not sure if they are valid or not!
//   {  "dtlsSecurityRootCA",         MFGP_dtlsSecurityRootCA,         "Read/Write Security Root CA " },                   // 1259
#endif
   {  "dtlsServerCertificateSerialNum", MFGP_dtlsServerCertificateSN, "Read Network Root CA cert (DER format)" },        // 1362
   {  "dtlsNetworkRootCA",          MFGP_dtlsNetworkRootCA,          "Read/Write Network Root CA cert (DER format)" },   // 1258
#if ( TM_UART_ECHO_COMMAND == 1 )
   {  "echo",                       MFGP_CommandLine_EchoComment,    "Echo back comment for testing UART" },
#endif
   {  "engBuEnabled",               MFGP_engBuEnabled,               "Get/Set the engBuStats" },
   {  "engBuTrafficClass",          MFGP_engBuTrafficClass,          "Get/Set Eng Stats bubble-up Traffic Class" },
   {  "engData1",                   MFGP_engData1,                   "Get the engData1 stats" },
#if ( EP == 1 )
   {  "engData2",                   MFGP_engData2,                   "Get the engData2 stats" },
   {  "epMaxTemperature",           MFGP_epMaxTemperature,           "Get/Set EP Max Temperature" },
   {  "epMinTemperature",           MFGP_epMinTemperature,           "Get/Set EP Min Temperature" },
   {  "epTempHysteresis",           MFGP_EpTempHysteresis,           "Get/Set EP Temperature Hysteresis" },
   {  "epTempMinThreshold",         MFGP_EpTempMinThreshold,         "Get/Set EP Min Temperature Threshold" },
#endif
   {  "fctEnclosureDatabaseVersion",MFGP_fctEnclosureDatabaseVersion,"(MIMT)Get/Set the fct enclosure database version" },
   {  "fctEnclosureProgramVersion", MFGP_fctEnclosureProgramVersion, "(MIMT)Get/Set the fct enclosure program version" },
   {  "fctEnclosureTestDate",       MFGP_fctEnclosureTestDate,       "(MIMT)Get/Set the fct enclosure test date" },
   {  "fctModuleDatabaseVersion",   MFGP_fctModuleDatabaseVersion,   "(MIMT)Get/Set the fct module database version" },
   {  "fctModuleProgramVersion",    MFGP_fctModuleProgramVersion,    "(MIMT)Get/Set the fct module program version" },
   {  "fctModuleTestDate",          MFGP_fctModuleTestDate,          "(MIMT)Get/Set the fct module test date" },
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
   {  "fngFemMfg",                  MFGP_fngFemMfg,                  "Get/Set FEM manufacturer (0 = Mini Circuits/1 = MicroAnt)" },
   {  "fngForwardPower",            MFGP_fngForwardPower,            "Get Forward Power" },
   {  "fngForwardPowerSet",         MFGP_fngForwardPowerSet,         "Get/Set Forward Power Set Point" },
   {  "fngFowardPowerLowSet",       MFGP_fngFowardPowerLowSet,       "Get/Set Foward Power Low Set Point" },
   {  "fngReflectedPower",          MFGP_fngReflectedPower,          "Get Reflected Power" },
   {  "fngVSWR",                    MFGP_fngVSWR,                    "Get VSWR" },
   {  "fngVswrNotificationSet",     MFGP_fngVswrNotificationSet,     "Get/Set VSWR Notification Set Point" },
   {  "fngVswrShutdownSet",         MFGP_fngVswrShutdownSet,         "Get/Set VSWR Shutdown Set Point" },
#endif
#if ( EP == 1 )
   {  "HighTempThreshold",          MFGP_HighTempThreshold,          "Get/Set EP High Temperature Threshold" },
#endif
#if (USE_DTLS == 1)
   {  "initialAuthenticationTimeout", MFGP_initialAuthenticationTimeout, "Get/Set the maximum authentication timout value" },
#endif
   {  "inboundFrameCount",          MFGP_inboundFrameCount,          "Display MAC Layer inbound frame counter" },
   {  "installationDateTime",       MFGP_installationDateTime,       "Get the installation dateTime of the device." },
   {  "integrationDatabaseVersion", MFGP_integrationDatabaseVersion, "(MIMT)Get/Set the integration database version" },
   {  "integrationProgramVersion",  MFGP_integrationProgramVersion,  "(MIMT)Get/Set the integration program version" },
   {  "integrationSetupDate",       MFGP_integrationSetupDate,       "(MIMT)Get/Set the integration setup date" },
   {  "ipHEContext",                MFGP_ipHEContext,                "Get/Set IP Head End Context" },

   {  "macState",                   MFGP_macState,                   "Get MAC State" },
   {  "macTxFrames",                MFGP_macTxFrames,                "Get MAC TxFrames" },
   {  "macAckWaitDuration",         MFGP_macAckWaitDuration,         "Get/Set AckWaitDuration" },
   {  "macAckDelayDuration",        MFGP_macAckDelayDuration,        "Get/Set AckDelayDuration" },
   {  "macChannelSetsCount",        MFGP_macChannelSetsCount,        "Get MAC ChannelSetsCount" },
   {  "macPacketId",                MFGP_macPacketId,                "Get MAC PacketId" },
   {  "macTxFrames",                    MFGP_macTxFrames,                    "Get TxFrames"},
   {  "macIsChannelAccessConstrained",  MFGP_macIsChannelAccessConstrained,  "Get/Set macIsChannelAccessConstrained"},
   {  "macIsFNG",                       MFGP_macIsFNG,                       "Get/Set macIsFNG"},
   {  "macIsIAG",                       MFGP_macIsIAG,                       "Get/Set macIsIAG"},
   {  "macIsRouter",                    MFGP_macIsRouter,                    "Get/Set macIsRouter"},

   {  "macChannelSets",             MFGP_MacChannelSetsSRFN,         "Get/Set SRFN MAC channel sets" },
#if ( DCU == 1 )
   {  "macChannelSetsSTAR",         MFGP_MacChannelSetsSTAR,         "Get/Set STAR MAC channel sets" },
#endif
   {  "macCsmaMaxAttempts",         MFGP_macCsmaMaxAttempts,         "Get/Set MAC CSMA max attempts" },
   {  "macCsmaMinBackOffTime",      MFGP_macCsmaMinBackOffTime,      "Get/Set MAC CSMA minimum backoff time" },
   {  "macCsmaMaxBackOffTime",      MFGP_macCsmaMaxBackOffTime,      "Get/Set MAC CSMA maximum backoff time" },
   {  "macCsmaPValue",              MFGP_macCsmaPValue,              "Get/Set MAC CSMA PV value (float between 0-1)" },
   {  "macCsmaQuickAbort",          MFGP_macCsmaQuickAbort,          "Get/Set MAC CSMA QuickAbort" },
   {  "macNetworkId",               MFGP_macNetworkId,               "Get/Set MAC Netork ID" },
   {  "macPacketTimeout",           MFGP_macPacketTimeout,           "Get/Set MAC PacketTimeout" },
   {  "macPingCount",               MFGP_macPingCount,               "Display MAC Layer ping counter" },
   {  "macReassemblyTimeout",       MFGP_macReassemblyTimeout,       "Get/Set MAC reassembly timeout" },
   {  "macReliabilityHighCount",    MFGP_macReliabilityHighCount,    "Get/Set the number of retries to satisfy the QOS reliability level of high" },
   {  "macReliabilityMedCount",     MFGP_macReliabilityMedCount,     "Get/Set the number of retries to satisfy the QOS reliability level of medium" },
   {  "macReliabilityLowCount",     MFGP_macReliabilityLowCount,     "Get/Set the number of retries to satisfy the QOS reliability level of low" },
   {  "macRSSI",                    MFGP_macRSSI,                    "Display the RSSI of a specified radio" },
#if ( EP == 1 )
   {  "macTimeSetMaxOffset",        MFG_TimeSetMaxOffset,            "Get/Set the TimeSet MaxOffset"},
   {  "macTimeSetPeriod",           MFG_TimeSetPeriod,               "Get/Set the TimeSet Period" },
   {  "macTimeSetStart",            MFG_TimeSetStart,                "Get/Set the TimeSet Start"},
   {  "macTimeSource",              MFG_TimeSource,                  "Get/Set the Time Source"},
#endif
#if  ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
   {  "macLinkParametersPeriod",    MFG_LinkParametersPeriod,       "Get/Set the Link Parameter Period"},
   {  "macLinkParametersStart",     MFG_LinkParametersStart,        "Get/Set the Link Parameter Start"},
   {  "macLinkParametersMaxOffset", MFG_LinkParametersMaxOffset,    "Get/Set the Link Parameter MaxOffset"},
   {  "receivePowerMargin",         MFG_ReceivePowerMargin,         "Get/Set the Link Parameter receivePowerMargin"},
#endif
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
   {  "macCommandResponseMaxTimeDiversity", MFG_MacCommandResponseMaxTimeDiversity,    "Get/Set the Link Parameter MaxOffset"},
#endif
   {  "macTransactionTimeout",      MFG_TransactionTimeout,          "Get/Set MAC transaction timeout" },
   {  "macTransactionTimeoutCount", MFG_TransactionTimeoutCount,     "Get the number of times there was a MAC transaction timeout"},
   {  "macTxLinkDelayCount",        MFG_macTxLinkDelayCount,         "Amount of MAC delays incurred"},
   {  "macTxLinkDelayTime",         MFG_macTxLinkDelayTime,          "Total time (ms) in delay"},
   {  "manufacturerNumber",         MFGP_manufacturerNumber,         "(MIMT)Get/Set the manufacturerNumber of the endpoint" },
#if (USE_DTLS == 1)
   {  "maxAuthenticationTimeout",   MFGP_maxAuthenticationTimeout,   "Get/Set the maximum authentication timeout value" },
   {  "minAuthenticationTimeout",   MFGP_minAuthenticationTimeout,   "Get/Set the maximum authentication timeout value" },
#endif
#if (USE_MTLS == 1)
   {  "mtlsAuthenticationWindow",   MFGP_mtlsAuthenticationWindow,   "Get/Set the mtls Authentication Window value" },
   {  "mtlsNetworkTimeVariation",   MFGP_mtlsNetworkTimeVariation,   "Get/Set the mtls Network Time Variationvalue" },
#endif
   {  "opportunisticAlarmIndexId",  MFGP_opportunisticAlarmIndexId,  "Get current index into the opportunistic alarm log" },
   {  "opportunisticThreshold",     MFGP_eventThreshold,             "xxx" },

#if ( PHASE_DETECTION == 1 )
   {  "capableOfPhaseSelfAssessment",  PD_SurveySelfAssessment,        "Is device capable of Phase Detect Self Assessment" },
   {  "capableOfPhaseDetectSurvey",    PD_SurveyCapable,               "Is device capable of Phase Detect" },
   {  "pdSurveyStartTime",             PD_SurveyStartTime,             "Get/Set Survey StartTime" },
   {  "pdSurveyPeriod",                PD_SurveyPeriod,                "Get/Set Survey Period" },
   {  "pdSurveyQuantity",              PD_SurveyQuantity,              "Get/Set Survey Quantity" },
   {  "pdSurveyMode",                  PD_SurveyReportMode,            "Get/Set Survey Mode" },
   {  "pdZCProductNullingOffset",      PD_ZCProductNullingOffset,      "Get/Set ZC Product Nulling Offset" },
   {  "pdLcdMsg",                      PD_LcdMsg,                      "Get/Set Lcd Msg" },
   {  "pdBuMaxTimeDiversity",          PD_BuMaxTimeDiversity,          "Get/Set Time Diversity" },
   {  "pdBuDataRedundancy",            PD_BuDataRedundancy,            "Get/Set DataRedundancy" },
   {  "pdSurveyPeriodQty",             PD_SurveyPeriodQty,             "Get/Set Survey Period Quantity" },
#endif
   {  "phyAvailableChannels",          MFG_PhyAvailableChannels,        "Get/Set the available radio channels" },
   {  "PhyAvailableFrequencies",       MFG_PhyAvailableFrequencies,     "Get/Set available radio frequencies" },
   {  "phyCcaThreshold",               MFG_PhyCcaThreshold,             "Get the CCA threshold" },
   {  "phyCcaAdaptiveThresholdEnable", MFG_PhyCcaAdaptiveThresholdEnable,"Enable/Disable the adaptive CCA threshold" },
   {  "phyCcaOffset",                  MFG_PhyCcaOffset,                "The offset in dB from phyNoiseEstimate used to obtain phyCcaAdaptiveThreshold. Range: 0-30."},
#if ( EP == 1 )
   {  "phyDemodulator",             MFG_PhyDemodulator,              "Get/Set the demodulator used by the each receiver" },
#endif
   {  "phyFrontEndGain",            MFG_PhyFrontEndGain,             "Get/Set the front end gain (half dBm steps)" },
   {  "phyMaxTxPayload",            MFG_PhyMaxTxPayload,             "Get/Set the maximum transmitable PHY payload size" },
   {  "phyNoiseEstimate",           MFG_PhyNoiseEstimate,            "Get the PHY noise for each channel" },
   {  "phyNoiseEstimateRate",       MFG_PhyNoiseEstimateRate,        "Get/Set the PHY noise estimate rate" },
   {  "phyNumChannels",             MFG_PhyNumchannels,              "Get the number of channels programmed into and usable by the PHY" },
   {  "phyRcvrCount",               MFG_PhyRcvrCount,                "Get the number receivers" },
   {  "phyRxChannels",              MFG_PhyRxChannels,               "Get/Set radio Receiver channels" },
   {  "phyRxDetection",             MFG_PhyRxDetection,              "Get/Set the detection configuration" },
   {  "phyRxFraming",               MFG_PhyRxFraming,                "Get/Set the framing configuration" },
   {  "phyRxFrequencies",           MFG_PhyRxFrequencies,            "Get/Set radio Receiver frequencies" },
   {  "phyRxMode",                  MFG_PhyRxMode,                   "Get/Set the PHY mode configuration" },
   {  "phyThermalControlEnable",    MFG_PhyThermalControlEnable,     "Get/Set PHY thermal control feature" },
   {  "phyThermalProtectionEnable", MFG_PhyThermalProtectionEnable,  "Get/Set PHY thermal protection feature" },
   {  "phyThermalProtectionCount",  MFG_PhyThermalProtectionCount,   "Get The number of times a peripheral temperature exceeding a given limit."},
   {  "phyThermalProtectionEngaged",MFG_PhyThermalProtectionEngaged, "Get if a PHY peripheral has exceeded an allowed temperature limit"},
   {  "phyTxChannels",              MFG_PhyTxChannels,               "Get/Set radio Transmitter channels" },
   {  "phyTxFrequencies",           MFG_PhyTxFrequencies,            "Get/Set radio Transmitter frequencies" },
   {  "phyFailedFrameDecodeCount",  MFG_PhyFailedFrameDecodeCount,   "Get the number of received PHY frames that failed FEC decoding"},
   {  "phyFailedHcsCount",          MFG_PhyFailedHcsCount,           "Get the number of RX PHY frame headers that failed the header check for each  reciever"},
   {  "phyFramesReceivedCount",     MFG_PhyFramesReceivedCount,      "Get the number of data frames received for each  reciever"},
   {  "phyFramesTransmittedCount",  MFG_PhyFramesTransmittedCount,   "Get the number of frames transmitted"},
   {  "phyFailedHeaderDecodeCount", MFG_PhyFailedHeaderDecodeCount,  "Get the number of received PHY frame headers that failed FEC decoding"},
   {  "phySyncDetectCount",         MFG_PhySyncDetectCount,          "Get the number of times a valid sync word was detected on each reciever"},
#if ( EP == 1 )
   {  "powerQuality",               MFGP_powerQuality,               "Get power quality counter reading"},
#endif
#if (VSWR_MEASUREMENT == 1)
   {  "vswr",                       MFGP_Vswr,                       "Get last VSWR reading." },
#endif
   {  "realTimeAlarm",              MFGP_getRealTimeAlarm,           "Get whether the endpoint supports real time alarms" },
   {  "realTimeAlarmIndexId",       MFGP_realTimeAlarmIndexId,       "Get current index into the real time alarm log" },
   {  "realtimeThreshold",          MFGP_eventThreshold,             "Get the realtime event threshold" },
   {  "reboot",                     MFG_reboot,                      "Reboot device"   },
   {  "repairInformation",          MFGP_repairInformation,          "(MIMT)Get/Set the repair information of the endpoint of the endpoint" },
   {  "rtcDateTime",                MFGP_rtcDateTime,                "Get/Set the RTC value"   },
   {  "shipMode",                   MFGP_shipMode,                   "Set Ship Mode" },
   {  "spuriousresetcount",         MFGP_SpuriousResetCount,         "Get/Set spurious reset count" },
   {  "stnvmrwfailcount",           MFGP_nvFailCount,                "Get/Set NV failure count" },
   {  "stnvmrwfailtest",            MFGP_nvtest,                     "Run external NV memory test" },
   {  "stRTCFailCount",             MFG_stRTCFailCount,              "Get/Set Real Time Clock test Fail Count" },
   {  "stRTCFailTest",              MFG_stRTCFailTest,               "Start a Real Time Clock test" },
   {  "strx4gfsk",                  MFG_StRx4GFSK,                   "Start a Receiver BER test using normal radio processing" },
#if ( DCU == 1 )
   {  "stRamRWFailCount",           MFG_ToolEP_SDRAMCount,           "Get/Set SDRAM test failure count" },
   {  "stRamRWFailTest",            MFG_ToolEP_SDRAMTest,            "Run the SDRAM test" },
#endif
   {  "stSecurityFailCount",        MFGP_stSecurityFailCount,        "Get/Set security device test failure count" },
   {  "stSecurityFailTest",         MFGP_stSecurityFailTest,         "Test security device" },
#if ( DCU == 1 )
   {  "sttxblurttest",              MFG_StTxBlurtTest,               "Transmit a message to test TX. Specify SRFN or STAR" },
#else
   {  "sttxblurttest",              MFG_StTxBlurtTest,               "Transmit a message to test TX" },
#endif
   {  "sttxcwtest",                 MFG_StTxCwTest,                  "Start a CW transmitter test" },
#if ( DCU == 1 )
   {  "TBImageTarget",              MFGP_DCUVersion,                 "Get/Set DCU operating mode {DCU2|DCU2+}" },
#endif
   {  "temperature",                MFGP_temperature,                "Get device temperature" },
   {  "timeDefaultAccuracy",        MFG_TimeDefaultAccuracy,         "Get/Set the Default Time Accuracy"},
   {  "timeLastUpdated",            MFGP_timeLastUpdated,            "The time that date/time was last updated" },
   {  "timePrecision",              MFG_TimePrecision,               "Get the Time Precision"},
   {  "macTimePrecision",           MFG_TimePrecision,               "Alias for timePrecision"},
   {  "timeQueryResponseMode",      MFG_TimeQueryResponseMode,       "Get/Set the time query response behavior to broadcast(0), unicast(1) or ignore the request (2)" },
   {  "timeState",                  MFGP_timeState,                  "State of system clock: 0=INVALID 1=VALID_NO_SYNC 2=VALID"},
   {  "virgin",                     MFGP_virgin,                     "Virgin unit" },
   {  "virginDelay",                MFGP_virginDelay,                "Erases signature; continues. Allows new code load and then virgin"},
   { 0, 0, 0 }
};

#if ( EP == 1 )
/* The following commands are only valid on and endpoint not on frodo */
static const struct_CmdLineEntry MFGP_EpCmdTable[] =
{
   {  "config_get",                 MFGP_ConfigGet,                  "Get the configuration value" },
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   {  "dailySelfReadTime",          MFGP_dailySelfReadTime,          "time daily ready happens(seconds after midnight" },
#endif
   {  "dateTimeLostCount",          MFGP_dateTimeLostCount,          "Get/Set the number of times RTC was lost due to extended power down" },
   {  "decommissionMode",           MFGP_meterShopMode,              "Get/Set decommission Mode" },
#if ( ENABLE_DEMAND_TASKS == 1 )
   {  "demandFutureConfiguration",  MFGP_demandFutureConfiguration,  "xxx" },
   {  "demandPresentConfiguration", MFGP_demandPresentConfiguration, "xxx" },
   {  "demandResetLockoutPeriod",   MFGP_demandResetLockoutPeriod,   "Get/Set the demand reset lockout period" },
#endif // ( ENABLE_DEMAND_TASKS == 1 )
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
   {  "dfwAuditTestStatus",         MFGP_dfwAuditTestStatus,         "Get the dfwAuditTestStatus parameter value"},
   {  "dfwCompatibilityTestStatus", MFGP_dfwCompatibilityTestStatus, "Get the dfwCompatibilityTestStatus parameter value"},
   {  "dfwProgramScriptStatus",     MFGP_dfwProgramScriptStatus,     "Get the dfwProgramScriptStatus parameter value"},
#endif  // endif ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
   {  "dfwApplyConfirmTimeDiversity", MFGP_dfwApplyConfirmTimeDiversity, "xxx" },
   {  "dfwDownloadConfirmTimeDiversity", MFGP_dfwDownloadConfirmTimeDiversity, "xxx" },
   {  "dfwDupDiscardPacketQty",     MFGP_dfwDupDiscardPacketQty,     "Get the number of discarded during dfw packet download process." },
   {  "dfwFileType",                MFGP_dfwFileType,                "Get the file type set by the DFW_INIT"},
   {  "dfwStatus",                  MFGP_dfwStatus,                  "Get the DFW status" },
   {  "dfwStreamID",                MFGP_dfwStreamId,                "Get/Set DFW Stream Id" },
#if ( REMOTE_DISCONNECT == 1 )
   {  "disconnectCapable",          MFGP_disconnectCapable,          "Display whether the meter is capable of remote disconnect ability" },
#endif
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#if ( ENABLE_DEMAND_TASKS == 1 )
   {  "drReadList",                 MFGP_drReadList,                 "Get/Set Demand read list" },
#endif
   {  "dsBuDataRedundancy",         MFGP_dsBuDataRedundancy,         "xxx" },
   {  "dsBuEnabled",                MFGP_dsBuEnabled,                "Get/Set the whether daily shifted data is allowed to bubble up" },
   {  "dsBuMaxTimeDiversity",       MFGP_dsBuMaxTimeDiversity,       "xxx" },
   {  "dsBuReadingTypes",           MFGP_dsBuReadingTypes,           "xxx" },
   {  "dsBuTrafficClass",           MFGP_dsBuTrafficClass,           "xxx" },
#endif // ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   {  "dstEnabled",                 MFGP_dstEnabled,                 "xxx" },
   {  "dstEndRule",                 MFGP_dstEndRule,                 "xxx" },
   {  "dstOffset",                  MFGP_dstOffset,                  "xxx" },
   {  "dstStartRule",               MFGP_dstStartRule,               "xxx" },
#if ( ANSI_STANDARD_TABLES == 1 )
   {  "ansiTableOID",               MFGP_AnsiTblOID,                 "Get the meter's DEVICE_CLASS per ANSI Table 00" },
#endif
#if ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 )
   {  "edFwVersion",                MFGP_edFwVersion,                "Get the host's firmware version.revision.build" },
#else
   {  "edFwVersion",                MFGP_edFwVersion,                "Get the meter's firmware version.revision" },
#endif
#if ( ANSI_STANDARD_TABLES == 1 ) || ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 )
   {  "edHwVersion",                MFGP_edHwVersion,                "Get the host's hardware version.revision" },
#endif
#if ( HMC_KV == 1 ) || ( HMC_I210_PLUS == 1 )
   {  "edInfo",                     MFGP_edInfo,                     "xxx" },
#else // for SRNFI-210+C
   {  "edInfo",                     MFGP_edInfo,                     "Get the end device info" },
#endif

#if ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 )
   {  "edManufacturer",             MFGP_edManufacturer,             "Get the meter manufacturer" },
#elif ( ACLARA_DA == 1 )
   {  "edManufacturer",             MFGP_edManufacturer,             "Get the host manufacturer" },
#endif
   {  "edMfgSerialNumber",          MFGP_edMfgSerialNumber,          "Get the host's serial number" },
#if ( ANSI_STANDARD_TABLES == 1 )
   {  "edModel",                    MFGP_edModel,                    "Get the host's model name" },
#elif ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 )
   {  "edModel",                    MFGP_edModel,                    "Get/Set the host's type/model number" },
#endif
#if ACLARA_DA == 1
   {  "edBootVersion",              MFGP_hostBootVersion,            "Get the host's boot loader version" },
   {  "edBspVersion",               MFGP_hostBspVersion,             "Get the host's BSP version" },
   {  "edKernelVersion",            MFGP_hostKernelVersion,          "Get the host's kernel version" },
   {  "edCarrierHwVersion",         MFGP_carrierHwVersion,           "Get the carrier board's hardware version" },
   {  "hostEchoTest",               MFGP_hostEchoTest,               "Test the host communication port" },
#endif
#if ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 )
   {  "edProgrammedDateTime",       MFGP_edProgrammedDateTime,       "Get the meters's last program datetime in seconds" },
   {  "edProgramId",                MFGP_edProgramId,                "Get the meter's program ID number" },
   {  "edProgrammerName",           MFGP_edProgrammerName,           "Get the name of the last programmer of the meter" },
#endif
   {  "edUtilitySerialNumber",      MFGP_edUtilitySerialNumber,      "xxx" },
#if ( SAMPLE_METER_TEMPERATURE == 1 )
   {  "edTemperatureHystersis",     MFGP_edTemperatureHystersis,     "Get/Set The hysteresis from a maximum temperature threshold before a high temp alarm clears" },
   {  "edTempSampleRate",           MFGP_edTempSampleRate,           "Get/Set The period (in seconds) between temperature samples of the meter's thermometer" },
#endif
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   {  "fwdkWh",                     MFG_bulkQuantity,                "Read forward kWh from meter" },
   {  "historicalRecovery",         MFGP_historicalRecovery,         "Get whether the endpoint supports historical data recovery" },
#endif
   {  "initialRegistrationTimeout", MFGP_initialRegistrationTimemout,"Get/Set the initial registration timeout" },
   {  "invalidAddressModeCount",    MFGP_invalidAddressModeCount,    "Get the invalid address mode count." },
   {  "lastGaspMaxNumAttempts",     MFGP_lastGaspMaxNumAttempts,     "Get/Set maximum number of last gasps" },
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#if ( LP_IN_METER == 0 )
   {  "lpBuChannel1",               MFGP_lpBuChannel,                "xxx" },
   {  "lpBuChannel2",               MFGP_lpBuChannel,                "xxx" },
   {  "lpBuChannel3",               MFGP_lpBuChannel,                "xxx" },
   {  "lpBuChannel4",               MFGP_lpBuChannel,                "xxx" },
   {  "lpBuChannel5",               MFGP_lpBuChannel,                "xxx" },
   {  "lpBuChannel6",               MFGP_lpBuChannel,                "xxx" },
#if ( ID_MAX_CHANNELS > 6 )
   {  "lpBuChannel7",               MFGP_lpBuChannel,                "xxx" },
   {  "lpBuChannel8",               MFGP_lpBuChannel,                "xxx" },
#endif
#endif
   {  "lpBuEnabled",                MFGP_lpBuEnabled,                "Get/Set whether LP Data is allowed to Bubble up" },
   {  "lpBuDataRedundancy",         MFGP_lpBuDataRedundancy,         "xxx" },
   {  "lpBuMaxTimeDiversity",       MFGP_lpBuMaxTimeDiversity,       "xxx" },
   {  "lpBuSchedule",               MFGP_lpBuSchedule,               "xxx" },
   {  "lpBuTrafficClass",           MFGP_lpBuTrafficClass,           "xxx" },
#endif
   {  "maxRegistrationTimeout",     MFGP_maxRegistrationTimemout,    "Get/Set the maximum registration timeout" },
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   {  "meterCommunicationLockoutCount", MFGP_meterCommunicationLockoutCount,
      "Get Total number of times the transponder is locked out from communicating with the meter." },
#endif
   {  "minRegistrationTimeout",     MFGP_minRegistrationTimemout,    "Get/Set the minimum registration timeout" },
#if ( CLOCK_IN_METER == 1 )
   {  "meterDateTime",              MFGP_meterDateTime,              "Get meter's local date/time - seconds since midnight 1/1/70" },
#endif
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   {  "meterSessionFailureCount",   MFGP_meterSessionFailureCount,   "Get/Set the meterSessionFailureCount parameter" },
#endif
   {  "meterShopMode",              MFGP_meterShopMode,              "Alias for decommissionMode" },
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   {  "netkWh",                     MFG_bulkQuantity,                "Read net kWh from meter" },
#endif
   {  "newRegistrationRequired",    MFGP_newRegistrationRequired,    "Get/Set registration state" },
   {  "nwActiveActTimeout",         MFGP_NwActiveActTimeout,         "Get or set active network activity timeout" },
   {  "nwPassActTimeout",           MFGP_NwPassActTimeout,           "Get or set passive network activity timeout" },
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   {  "orReadList",                 MFGP_orReadList,                 "Get/Set on demand read list" },
#endif
   {  "PhyAfcEnable",               MFGP_PhyAfcEnable,               "Enable/disable AFC" },
   {  "PhyAfcRssiThreshold",        MFGP_PhyAfcRSSIThreshold,        "Get/Set AFC RSSI threshold" },
   {  "PhyAfcTemperaturerange",     MFGP_PhyAfcTemperatureRange,     "Get/Set AFC temperature range" },
   {  "outageDeclarationDelay",     MFGP_outageDeclarationDelay,     "Get/Set outage declaration delay in seconds" },
   {  "restorationDeclarationDelay",MFGP_restorationDelay,           "Get/Set restoration delay in seconds" },
   {  "powerQualityEventDuration",  MFGP_powerQualityEventDuration,  "Get/Set power quality event duration in seconds" },
#if ( ENABLE_DEMAND_TASKS == 1 )
#if ( DEMAND_IN_METER == 0 )
   {  "scheduledDemandResetDay",    MFGP_scheduledDemandResetDay,    "xxx" },
#endif
#endif
#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
   {  "simulateLastGaspStart",      MFGP_simulateLastGaspStart,      "Get/Set simulateLastGaspStart parameter" },
   {  "simulateLastGaspDuration",   MFGP_simulateLastGaspDuration,   "Get/Set simulateLastGaspDuration parameter" },
   {  "SimulateLastGaspTraffic",    MFGP_SimulateLastGaspTraffic,    "Get/Set simulateLastGaspDuration parameter" },
   {  "simulateLastGaspMaxNumAttempts",       MFGP_simulateLastGaspMaxNumAttempts,        "Get/Set simulateLastGaspMaxNumAttempts parameter" },
   {  "simulateLastGaspStatCcaAttempts",      MFGP_simulateLastGaspStatCcaAttempts,       "Get simulateLastGaspStatCcaAttempts stats" },
   {  "simulateLastGaspStatPPersistAttempts", MFGP_simulateLastGaspStatPPersistAttempts,  "Get simulateLastGaspStatPPersistAttempts stats" },
   {  "simulateLastGaspStatMsgsSent",         MFGP_simulateLastGaspStatMsgsSent,          "Get simulateLastGaspStatMsgsSent stats" },
#endif
   {  "smLogTimeDiversity",         MFGP_smLogTimeDiversity,         "Get/Set Stack Manager Log Time Diversity" },
   {  "stP0LoopbackFailCount",      MFGP_stP0LoopbackFailCount,      "Get/Set P0 Loopback Fail Count" },
//   {  "stp0loopbackfailtest",       MFGP_stP0LoopbackFailTest,       "Test P0 (HMC port) - requires loopback cable" },
#if ( REMOTE_DISCONNECT == 1 )
   {  "switchPosition",             MFGP_switchPosition,             "Possible Values returned: 0 = disconnected / 1 = connected" },
#endif
   {  "timeSigOffset",              MFGP_timeSigOffset,              "Get/Set timeSigOffset"},
   {  "timeAcceptanceDelay",        MFGP_timeAcceptanceDelay,        "Time Acceptance Delay for MAC Time Set" },
   {  "timeRequestMaxTimeout",      MFGP_timeRequestMaxTimeout,      "Get/Set the maximum amount of random delay following a request for network time before the time request is retried" },
   {  "timeZoneDSTHash",            MFGP_timeZoneDSTHash,            "SHA-256 hash of local time settings" },
   {  "timeZoneOffset",             MFGP_timeZoneOffset,             "time zone offset (signed, in hours)" },
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   {  "totkWh",                     MFG_bulkQuantity,                "Read total kWh from meter" },
#endif
   {  "watchdogResetCount",         MFGP_watchDogResetCount,         "Get/Set watchdoge reset counter" },
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   {  "0.0.0.1.1.1.12.0.0.0.0.0.0.0.0.3.72.0",  MFG_bulkQuantity,    "Read forward kWh from meter" },
   {  "0.0.0.1.20.1.12.0.0.0.0.0.0.0.0.3.72.0", MFG_bulkQuantity,    "Read total kWh from meter" },
   {  "0.0.0.1.4.1.12.0.0.0.0.0.0.0.0.3.72.0",  MFG_bulkQuantity,    "Read net kWh from meter" },
#endif
#if ( CLOCK_IN_METER == 1 )
   {  "0.0.0.6.0.41.7.0.0.0.0.0.0.0.0.0.108.0", MFGP_meterDateTime,  "Get meter's local date/time - seconds since midnight 1/1/70" },
#endif
   { 0, 0, 0 }
};
#endif   //end of #if ( EP == 1 )

#if ( EP == 1 )
/* The following command(s) are the ONLY commands accepted while in "quiet" mode.   */
static const struct_CmdLineEntry MFGP_QuietModeTable[] =
{
   {  "quietMode",                  MFGP_quietMode,                  "Get/Set quiet mode" },
   { 0, 0, 0 }
};
#endif

/* The following commands are not displayed by the help command and are therefore "hidden"   */
static const struct_CmdLineEntry MFGP_HiddenCmdTable[] =
{
#if ( EP == 1 )
   {  "rfTestMode",                 MFGP_rfTestMode,                 "Get/Set rfTest mode" },
#endif
   {  "appSecurityAuthMode",        MFG_appSecAuthMode,              "Set system security mode" },
   {  "debugPortEnabled",           MFG_enableDebug,                 "Enable debug port" },
   {  "enableOTATest",              MFGP_enableOTATest,              "Temporarily allows over the air testing of certain tests" },
   {  "flashSecurityEnabled",       MFGP_FlashSecurity,              "Lock/Unlock Flash and JTAG Security" },
#if ( EP == 1 )
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
   {  "logoff",                     MFG_logoff,                      "Reset port for mfg commands" },
#endif
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   {  "passwordPort0",              MFG_meterPassword,               "Set password for AMR port" },
   {  "masterpasswordPort0",        MFGP_passwordPort0Master,        "Set Master password for AMR port" },
#endif // endif of (ACLARA_LC == 0)
#endif
#if (TX_THROTTLE_CONTROL == 1 )
   {  "txThrottleDisable",          MFG_TxThrottleDisable,           "Disable transmitter throttling for 255 hours" },
#endif
   { 0, 0, 0 }
};

#if (USE_DTLS == 1)
/* If appsecurityauthmode is non-zero, this is the only command available  */
static const struct_CmdLineEntry MFGP_SecureTable[] =
{
   {  "752732DA-47CD-49C3-B8F4-DA8F23BE0708", MFG_startDTLSsession,  "Start the DTLS session" },
   {  "disconnectDtls",             MFG_disconnectDtls,              "CMD: shutdown the dtls" },
   { 0, 0, 0 }
};
#endif

// TODO: RA6 [name_Balaji]: Integrate Different Modes Tables
static const struct_CmdLineEntry * const cmdTables[ ( uint16_t )menuLastValid ] =
{
#if ( EP == 1 )
   MFGP_QuietModeTable,
   MFGP_EpCmdTable,
#endif
   MFGP_CmdTable,
   MFGP_HiddenCmdTable
#if (USE_DTLS == 1)
   ,
   MFGP_SecureTable
#endif
};
static const char CRLF[] = { '\r', '\n' };

/* TODO: RA6E1 Re-insert MFGP_dtlsServerCertificateSN as shown:
   ,MFGP_dtlsNetworkMSSubject \
   ,MFGP_dtlsServerCertificateSN \
   ,MFGP_dtlsNetworkRootCA \
  once DTLS is in place */
//lint -e750    Lint is complaining about macro not referenced
#define MFG_COMMON_CALLS \
   MFGP_CommandLine_Help \
   ,MFGP_amBuMaxTimeDiversity \
   ,MFGP_capableOfEpBootloaderDFW \
   ,MFGP_capableOfEpPatchDFW \
   ,MFGP_capableOfMeterBasecodeDFW \
   ,MFGP_capableOfMeterPatchDFW \
   ,MFGP_capableOfMeterReprogrammingOTA \
   ,MFGP_firmwareVersion \
   ,MFGP_firmwareVersion \
   ,MFGP_hardwareVersion \
   ,MFG_CommandLine_MacAddr \
   ,MFGP_DeviceType \
   ,MFGP_dataConfigurationDocumentVersion \
   ,MFG_DateTime \
   ,MFGP_dtlsDeviceCertificate \
   ,MFGP_dtlsMfgSubject1 \
   ,MFGP_dtlsMfgSubject2 \
   ,MFGP_dtlsNetworkHESubject \
   ,MFGP_dtlsNetworkMSSubject \
   ,MFGP_dtlsNetworkRootCA \
   ,MFGP_engBuEnabled \
   ,MFGP_engData1 \
   ,MFGP_fctEnclosureDatabaseVersion \
   ,MFGP_fctEnclosureProgramVersion \
   ,MFGP_fctEnclosureTestDate \
   ,MFGP_fctModuleDatabaseVersion \
   ,MFGP_fctModuleProgramVersion \
   ,MFGP_fctModuleTestDate \
   ,MFGP_inboundFrameCount \
   ,MFGP_installationDateTime \
   ,MFGP_integrationDatabaseVersion \
   ,MFGP_integrationProgramVersion \
   ,MFGP_integrationSetupDate \
   ,MFGP_ipHEContext \
   ,MFGP_MacChannelSetsSRFN \
   ,MFGP_macCsmaMaxAttempts \
   ,MFGP_macCsmaMinBackOffTime \
   ,MFGP_macCsmaMaxBackOffTime \
   ,MFGP_macCsmaPValue \
   ,MFGP_macCsmaQuickAbort \
   ,MFGP_macNetworkId \
   ,MFGP_macPacketTimeout \
   ,MFGP_macPingCount \
   ,MFGP_macReassemblyTimeout \
   ,MFGP_macReliabilityHighCount \
   ,MFGP_macReliabilityMedCount \
   ,MFGP_macReliabilityLowCount \
   ,MFGP_macRSSI \
   ,MFG_TransactionTimeout \
   ,MFGP_manufacturerNumber \
   ,MFGP_opportunisticAlarmIndexId \
   ,MFGP_eventThreshold \
   ,MFG_PhyAvailableChannels \
   ,MFG_PhyAvailableFrequencies \
   ,MFG_PhyCcaThreshold \
   ,MFG_PhyFrontEndGain \
   ,MFG_PhyMaxTxPayload \
   ,MFG_PhyNoiseEstimateRate \
   ,MFG_PhyNumchannels \
   ,MFG_PhyRcvrCount \
   ,MFG_PhyRxChannels \
   ,MFG_PhyRxDetection \
   ,MFG_PhyRxFraming \
   ,MFG_PhyRxFrequencies \
   ,MFG_PhyRxMode \
   ,MFG_PhyThermalControlEnable \
   ,MFG_PhyThermalProtectionEnable \
   ,MFG_PhyTxChannels \
   ,MFG_PhyTxFrequencies \
   ,MFGP_getRealTimeAlarm \
   ,MFGP_realTimeAlarmIndexId \
   ,MFGP_eventThreshold \
   ,MFG_reboot \
   ,MFGP_repairInformation \
   ,MFGP_rtcDateTime \
   ,MFGP_shipMode \
   ,MFGP_SpuriousResetCount \
   ,MFGP_nvFailCount \
   ,MFGP_nvtest \
   ,MFG_stRTCFailCount \
   ,MFG_stRTCFailTest \
   ,MFG_StRx4GFSK \
   ,MFGP_stSecurityFailCount \
   ,MFGP_stSecurityFailTest \
   ,MFG_StTxBlurtTest \
   ,MFG_StTxCwTest \
   ,MFGP_temperature \
   ,MFG_TimeDefaultAccuracy \
   ,MFG_TimePrecision \
   ,MFG_TimeQueryResponseMode \
   ,MFGP_virgin \
   ,MFGP_virginDelay \
   ,MFG_appSecAuthMode \
   ,MFG_enableDebug \
   ,MFGP_FlashSecurity \
   ,MFGP_enableOTATest \
   ,MFGP_engBuTrafficClass \

#if ( DCU == 1 )
#define MFG_DCU_CALLS \
   ,MFGP_DCUVersion \
   ,MFGP_DeviceGatewayConfig \
   ,MFGP_MacChannelSetsSTAR \
   ,MFG_ToolEP_SDRAMCount \
   ,MFG_ToolEP_SDRAMTest
#else
#define MFG_DCU_CALLS
#endif

#if ( ENABLE_FIO_TASKS == 1 )
#define MFG_FIO_CALLS \
   ,MFGP_ConfigTest \
   ,MFGP_ConfigUpdate
#else
#define MFG_FIO_CALLS
#endif

#if ( EP == 1 )
#define MFG_EP_CALLS \
   ,MFGP_engData2 \
   ,MFGP_epMaxTemperature \
   ,MFGP_epMinTemperature \
   ,MFGP_EpTempHysteresis \
   ,MFGP_EpTempMinThreshold \
   ,MFGP_HighTempThreshold \
   ,MFG_TimeSetMaxOffset \
   ,MFG_TimeSetPeriod \
   ,MFG_TimeSetStart \
   ,MFG_TimeSource \
   ,MFGP_ConfigGet \
   ,MFGP_dateTimeLostCount \
   ,MFGP_meterShopMode \
   ,MFGP_dfwApplyConfirmTimeDiversity \
   ,MFGP_dfwDownloadConfirmTimeDiversity \
   ,MFGP_dfwDupDiscardPacketQty \
   ,MFGP_dfwStatus \
   ,MFGP_dfwStreamId \
   ,MFGP_dstEnabled \
   ,MFGP_dstEndRule \
   ,MFGP_dstOffset \
   ,MFGP_dstStartRule \
   ,MFGP_edFwVersion \
   ,MFGP_edInfo \
   ,MFGP_edMfgSerialNumber \
   ,MFGP_edUtilitySerialNumber \
   ,MFGP_initialRegistrationTimemout \
   ,MFGP_invalidAddressModeCount \
   ,MFGP_maxRegistrationTimemout \
   ,MFGP_minRegistrationTimemout \
   ,MFGP_meterShopMode \
   ,MFGP_newRegistrationRequired \
   ,MFGP_NwActiveActTimeout \
   ,MFGP_NwPassActTimeout \
   ,MFGP_PhyAfcEnable \
   ,MFGP_PhyAfcRSSIThreshold \
   ,MFGP_PhyAfcTemperatureRange \
   ,MFG_PhyDemodulator \
   ,MFGP_powerQuality \
   ,MFGP_outageDeclarationDelay \
   ,MFGP_restorationDelay \
   ,MFGP_powerQualityEventDuration \
   ,MFGP_lastGaspMaxNumAttempts \
   ,MFGP_smLogTimeDiversity \
   ,MFGP_stP0LoopbackFailCount \
   ,MFGP_stP0LoopbackFailTest \
   ,MFGP_timeState \
   ,MFGP_timeSigOffset \
   ,MFGP_timeAcceptanceDelay \
   ,MFGP_timeLastUpdated \
   ,MFGP_timeRequestMaxTimeout \
   ,MFGP_timeZoneDSTHash \
   ,MFGP_timeZoneOffset \
   ,MFGP_watchDogResetCount \
   ,MFGP_quietMode \
   ,MFGP_rfTestMode
#else
#define MFG_EP_CALLS
#endif

#if (USE_DTLS == 1)
#define MFG_DTLS_CALLS \
   ,MFGP_initialAuthenticationTimeout \
   ,MFGP_maxAuthenticationTimeout \
   ,MFGP_minAuthenticationTimeout \
   ,MFG_startDTLSsession \
   ,MFG_disconnectDtls
#else
#define MFG_DTLS_CALLS
#endif

#if (USE_MTLS == 1)
#define MFG_MTLS_CALLS \
   ,MFGP_mtlsAuthenticationWindow \
   ,MFGP_mtlsNetworkTimeVariation
#else
#define MFG_MTLS_CALLS
#endif

#if (VSWR_MEASUREMENT == 1)
#define MFG_VSWR_CALLS \
   ,MFGP_Vswr
#else
#define MFG_VSWR_CALLS
#endif

#if ( EP == 1) && ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#define MFG_NOT_LC_AND_NOT_DA_CALLS \
   ,MFGP_dailySelfReadTime \
   ,MFGP_drReadList \
   ,MFGP_dsBuDataRedundancy \
   ,MFGP_dsBuEnabled \
   ,MFGP_dsBuMaxTimeDiversity \
   ,MFGP_dsBuReadingTypes \
   ,MFGP_dsBuTrafficClass \
   ,MFG_bulkQuantity \
   ,MFGP_historicalRecovery \
   ,MFGP_lpBuEnabled \
   ,MFGP_lpBuDataRedundancy \
   ,MFGP_lpBuMaxTimeDiversity \
   ,MFGP_lpBuSchedule \
   ,MFGP_lpBuTrafficClass \
   ,MFGP_meterCommunicationLockoutCount \
   ,MFGP_meterSessionFailureCount \
   ,MFGP_orReadList \
   ,MFG_meterPassword \
   ,MFGP_passwordPort0Master
#else
#define MFG_NOT_LC_AND_NOT_DA_CALLS
#endif

#if ( EP == 1) && ( ENABLE_DEMAND_TASKS == 1 )
#if ( DEMAND_IN_METER == 1 )
#define MFG_DEMAND_CALLS \
   ,MFGP_demandFutureConfiguration \
   ,MFGP_demandPresentConfiguration \
   ,MFGP_demandResetLockoutPeriod
#else
#define MFG_DEMAND_CALLS \
   ,MFGP_demandFutureConfiguration \
   ,MFGP_demandPresentConfiguration \
   ,MFGP_demandResetLockoutPeriod \
   ,MFGP_scheduledDemandResetDay
#endif
#else
#define MFG_DEMAND_CALLS
#endif

#if ( EP == 1) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
#define MFG_END_DEVICE_PROG_CALLS \
   ,MFGP_dfwAuditTestStatus \
   ,MFGP_dfwCompatibilityTestStatus \
   ,MFGP_dfwProgramScriptStatus
#else
#define MFG_END_DEVICE_PROG_CALLS
#endif

#if ( EP == 1) && ( REMOTE_DISCONNECT == 1 )
#define MFG_REMOTE_DISCONNET_CALLS \
   ,MFGP_disconnectCapable
#else
#define MFG_REMOTE_DISCONNET_CALLS
#endif

#if ( EP == 1) && ( ANSI_STANDARD_TABLES == 1 )
#define MFG_ANSI_TABLES_CALLS \
   ,MFGP_AnsiTblOID
#else
#define MFG_ANSI_TABLES_CALLS
#endif

#if ( EP == 1) && (( ANSI_STANDARD_TABLES == 1 ) || ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 ))
#define MFG_ANSI_TABLES_OR_LC_OR_DA_CALLS \
   ,MFGP_edHwVersion
#else
#define MFG_ANSI_TABLES_OR_LC_OR_DA_CALLS
#endif

#if ( ACLARA_DA == 1 )
#define MFG_DA_CALLS \
   ,MFGP_hostBootVersion \
   ,MFGP_hostBspVersion \
   ,MFGP_hostKernelVersion \
   ,MFGP_carrierHwVersion \
   ,MFGP_hostEchoTest
#else
#define MFG_DA_CALLS
#endif

#if ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 )
#define MFG_HMC_KV_OR_HMC_I210_PLUS_C_CALLS \
   ,MFGP_edProgrammedDateTime \
   ,MFGP_edProgramId \
   ,MFGP_edProgrammerName
#else
#define MFG_HMC_KV_OR_HMC_I210_PLUS_C_CALLS
#endif

#if ( EP == 1 ) && ( SAMPLE_METER_TEMPERATURE == 1 )
#define MFG_SAMPLE_METER_TEMP_CALLS \
   ,MFGP_edTemperatureHystersis \
   ,MFGP_edTempSampleRate
#else
#define MFG_SAMPLE_METER_TEMP_CALLS
#endif

#if ( EP == 1 ) && ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 ) && ( LP_IN_METER == 0 )
#define MFG_LP_IN_METER_CALLS \
   ,MFGP_lpBuChannel
#else
#define MFG_LP_IN_METER_CALLS
#endif

#if ( EP == 1) && ( CLOCK_IN_METER == 1 )
#define MFG_CLOCK_IN_METER_CALLS \
   ,MFGP_meterDateTime
#else
#define MFG_CLOCK_IN_METER_CALLS
#endif

#if ( EP == 1) && ( REMOTE_DISCONNECT == 1 )
#define MFG_REMOTE_DISCONNECT_CALLS \
   ,MFGP_switchPosition
#else
#define MFG_REMOTE_DISCONNECT_CALLS
#endif

#if ( EP == 1) && ( LAST_GASP_SIMULATION == 1 )
#define MFG_EP_LGSIM_CALLS \
   ,MFGP_simulateLastGaspStart \
   ,MFGP_simulateLastGaspDuration \
   ,MFGP_SimulateLastGaspTraffic \
   ,MFGP_simulateLastGaspMaxNumAttempts \
   ,MFGP_simulateLastGaspStatCcaAttempts \
   ,MFGP_simulateLastGaspStatPPersistAttempts \
   ,MFGP_simulateLastGaspStatMsgsSent
#else
#define MFG_EP_LGSIM_CALLS
#endif

/***********************************************************************************************************************
   Function Name: mfgp_QuietModeTimeout_

   Purpose: Callback function invoked on expiration of quiet mode timer.

   Arguments: Standard timer call back parameters - not used.

   Returns: void
***********************************************************************************************************************/
static void mfgp_QuietModeTimeout_( uint8_t param, void *ptr ) /*lint !e818 ptr could be ptr to const  */
{
   (void)param;
   (void)ptr;

   ( void )MODECFG_set_quiet_mode( 0 );
   MFG_printf( "quietmode Exiting quiet mode\n" );
   OS_TASK_Sleep( 200 );         /* Wait long enough for the printing to complete  */
   PWR_SafeReset();
}
#if ( EP == 1 )
/***********************************************************************************************************************
   Function Name: mfgp_rfTestModeTimeout_

   Purpose: Callback function invoked on expiration of rfTest mode timer.

   Arguments: Standard timer call back parameters - not used.

   Returns: void
***********************************************************************************************************************/
static void mfgp_rfTestModeTimeout_( uint8_t param, void *ptr )   /*lint !e818 ptr could be ptr to const  */
{
   (void)param;
   (void)ptr;

   ( void )MODECFG_set_rfTest_mode( 0 );
   MFG_printf( "Exiting rf test mode: timed out.\n" );
   OS_TASK_Sleep( 200 );         /* Wait long enough for the printing to complete  */
   PWR_SafeReset();
}
/***********************************************************************************************************************
   Function Name: MFGP_rfTestTimerReset

   Purpose: Resets rfTestMode timer

   Arguments: none

   Returns: void
***********************************************************************************************************************/
void MFGP_rfTestTimerReset( void )
{
   ( void )TMR_ResetTimer( rfTestModeTimerCfg.usiTimerId, RFTEST_MODE_TIMEOUT );
}

/***********************************************************************************************************************
   Function Name: MFGP_init

   Purpose: Initializes the Manufacturing port

   Arguments: none

   Returns: eSUCCESS if all went well.  eFAILURE if not
***********************************************************************************************************************/
returnStatus_t MFGP_init( void )
{
   timer_t tmrSettings;            // Timer for re-issuing metadata registration
   returnStatus_t retVal = eFAILURE;

   if ( OS_EVNT_Create (&MFG_notify) )
   {
      retVal = eSUCCESS;
      if ( MODECFG_get_quiet_mode() != 0 )   /* If in "quiet mode", set failsafe timer.   */
      {
         (void)memset( &tmrSettings, 0, sizeof(tmrSettings) );
         tmrSettings.ulDuration_mS = TIME_TICKS_PER_HR;
         tmrSettings.pFunctCallBack = mfgp_QuietModeTimeout_;
         retVal = TMR_AddTimer(&tmrSettings);
      }
#if ( EP == 1 )
      if ( MODECFG_get_rfTest_mode() != 0 )   /* If in "rfTest mode", set failsafe timer.   */
      {
         (void)memset( &rfTestModeTimerCfg, 0, sizeof(tmrSettings) );
         rfTestModeTimerCfg.ulDuration_mS = RFTEST_MODE_TIMEOUT;
         rfTestModeTimerCfg.pFunctCallBack = mfgp_rfTestModeTimeout_;
         retVal = TMR_AddTimer( &rfTestModeTimerCfg );
      }
#endif
#if ( USE_USB_MFG != 0 )
      if ( retVal == eSUCCESS )
      {
         if ( !OS_EVNT_Create( &CMD_events ) )
         {
            retVal = eFAILURE;
         }
      }
#endif
   }

   return(retVal);
}
#endif //if MQX
/***********************************************************************************************************************
   Function Name: MFGP_cmdInit

   Purpose: Initializes the Manufacturing command task

   Arguments: none

   Returns: eSUCCESS if all went well.  eFAILURE if not
***********************************************************************************************************************/
returnStatus_t MFGP_cmdInit( void )
{
   returnStatus_t retVal = eFAILURE;
   if ( OS_EVNT_Create(&_MfgpUartEvent) &&
#if ( MCU_SELECTED == NXP_K24 )
      OS_MSGQ_Create(&_CommandReceived_MSGQ, MFG_NUM_MSGQ_ITEMS) )
#elif ( MCU_SELECTED == RA6E1 )
      OS_MSGQ_Create( &_CommandReceived_MSGQ, MFG_NUM_MSGQ_ITEMS, "MFGP" )
#endif
   )
   {
      retVal = eSUCCESS;
   }
   return(retVal);
}

/***********************************************************************************************************************
   Function Name: mfgpReadByte

   Purpose: This function is called to process a byte received from the uart in MFG serial mode

   Arguments:  uint8_t rxByte      - Byte read from the port

   Returns: None
***********************************************************************************************************************/
static void mfgpReadByte( uint8_t rxByte )
{
   buffer_t *commandBuf;

   if (  ( rxByte == ESCAPE_CHAR ) ||  /* User pressed ESC key */
         ( rxByte == CTRL_C_CHAR ) ||  /* User pressed CRTL-C key */
         ( rxByte == 0xC0 ))           /* Left over SLIP protocol characters in buffer */
   {
      /* user canceled the in progress command */
      MFGP_numBytes = 0;
#if !USE_USB_MFG
      (void)UART_write( mfgUart, (uint8_t*)CRLF, sizeof( CRLF ) );
#if ( MCU_SELECTED == NXP_K24 )   // TODO: RA6E1 This UART_flush not needed now for the debug and mfg port. Might be added in the future.
      (void)UART_flush ( mfgUart  );
#endif
#else
      usb_send( CRLF, sizeof( CRLF ) );
      usb_flush();
#endif
   }
   else if( rxByte == BACKSPACE_CHAR || rxByte == 0x7f )
   {
      if( MFGP_numBytes != 0 )
      {
         /* buffer contains at least one character, remove the last one entered */
         MFGP_numBytes -= 1;
#if ( MCU_SELECTED == RA6E1 )
         /* Gives GUI effect in Terminal for Backspace */
         ( void )UART_echo( mfgUart, (uint8_t*)"\b\x20\b" , 3 );
#endif
      }
   }
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
   else if( rxByte == UART_SWITCH_CHAR ) /* Signal to switch to optical port.   */
   {
      int32_t flags;                /* Settings for the UART */
      if ( mfgUart == UART_MANUF_TEST )
      {
         flags = IO_SERIAL_TRANSLATION | IO_SERIAL_ECHO; /* Settings for the UART */
      }
      else
      {
         flags = IO_SERIAL_TRANSLATION; /* Settings for the UART */
      }

#if !USE_USB_MFG
      ( void )UART_ioctl( mfgUart, (int32_t)IO_IOCTL_SERIAL_SET_FLAGS, &flags );
#else
      usb_ioctl( (MQX_FILE_PTR)(uint32_t)mfgUart, IO_IOCTL_SERIAL_SET_FLAGS, &flags );
#endif
      commandBuf = ( buffer_t * )BM_alloc(  1 );
      if (commandBuf != NULL)
      {
         commandBuf->data[ 0 ] = rxByte;
         OS_MSGQ_Post( &_CommandReceived_MSGQ, commandBuf ); // Function will not return if it fails
      }
   }
#endif
   else
   {
#if ( MCU_SELECTED == NXP_K24 )
      if( (rxByte == LINE_FEED_CHAR) || (rxByte == CARRIAGE_RETURN_CHAR) )
#elif ( MCU_SELECTED == RA6E1 )
      if( rxByte == CARRIAGE_RETURN_CHAR )
#endif
      {
#if (USE_DTLS == 1)
         if ( !MFGP_AllowDtlsConnect() )
         {
#if !USE_USB_MFG
            (void)UART_write( mfgUart, (uint8_t*)mfgpLockInEffect, sizeof( mfgpLockInEffect ) );
#if ( MCU_SELECTED == NXP_K24 )   // TODO: RA6E1 This UART_flush not needed now for the debug and mfg port. Might be added in the future.
            (void)UART_flush ( mfgUart  );
#endif
#else
            MFG_printf( mfgpLockInEffect );
#endif
            return;
         }
#endif
#if ( RTOS_SELECTION == FREE_RTOS )
         OS_TASK_Sleep( (uint32_t)10 );  // Make sure the allocated buffer has been processed and freed in other task.
#endif
         commandBuf = ( buffer_t * )BM_alloc( MFGP_numBytes + 1 );
         if ( commandBuf != NULL )
         {
            commandBuf->data[MFGP_numBytes] = 0; /* Null terminating string */

#if ( DTLS_DEBUG == 1 )
            INFO_printHex( "MFG_buffer: ", MFGP_CommandBuffer, MFGP_numBytes );
            OS_TASK_Sleep( 50U );
#endif
#if ( MCU_SELECTED == RA6E1 )
            ( void )UART_echo( mfgUart, (uint8_t*)CRLF, sizeof( CRLF ) );
#endif
            ( void )memcpy( commandBuf->data, MFGP_CommandBuffer, MFGP_numBytes );
            /* Call the command */
            OS_MSGQ_Post( &_CommandReceived_MSGQ, commandBuf ); // Function will not return if it fails
#if ( USE_USB_MFG != 0 )
            event_flags = OS_EVNT_Wait ( &CMD_events, 0xffffffff, (bool)false, ONE_SEC );
            if ( event_flags == 0 )    /* Check for time-out.  */
            {
               /* Unblock task(s) waiting on USB output  */
               USB_resumeSendQueue( (bool)true );  /* Wake all tasks waiting for USB output completion. */
            }
#endif
            MFGP_numBytes = 0;
         }
         else
         {
            ERR_printf( "mfgpReadByte failed to create command buffer, ignoring command" );
            MFGP_numBytes = 0;
         }

      }
#if ( MCU_SELECTED == RA6E1 )
      else if ( rxByte == LINE_FEED_CHAR )
      {
         rxByte = 0x0;
      }
#endif
      else if( ( MFGP_numBytes ) >= MFGP_MAX_MFG_COMMAND_CHARS )
      {
         /* buffer is full */
         MFGP_numBytes = 0;
      }
      else
      {
         // Save character in buffer (space is available)
         MFGP_CommandBuffer[ MFGP_numBytes++] = rxByte;
#if ( MCU_SELECTED == RA6E1 )
         (void)UART_echo( mfgUart, &rxByte, sizeof( rxByte ) );
#endif
      }
   }
}

/***********************************************************************************************************************
   Function Name: MFGP_uartTask

   Purpose: collects incoming manufacturing serial port characters and calls appropriate command handler

   Arguments: Arg0 - an unused MQX required argument for tasks

   Returns: none
***********************************************************************************************************************/
/*lint -esym(715,Arg0) not referenced but required by API   */
void MFGP_uartCmdTask( taskParameter )
{
   buffer_t *commandBuf;
#if ( MCU_SELECTED == NXP_K24 )
#if !USE_USB_MFG
   FILE_PTR fh_ptr;

   if ( NULL != ( fh_ptr = fopen( MFG_PORT_IO_CHANNEL, NULL ) ) )
   {
      ( void )_io_set_handle( IO_STDOUT, fh_ptr );
   }
#endif
#endif
   for( ;; )
   {
      if ( OS_MSGQ_Pend( &_CommandReceived_MSGQ, ( void * )&commandBuf, OS_WAIT_FOREVER ) == true )
      {
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
         if ( ( commandBuf->data[ 0 ] == UART_SWITCH_CHAR ) && ( commandBuf->x.dataLen == 1 ) )
         {
            /* Command to change the standard out port.  */
            ( void )_io_set_handle( IO_STDOUT, (FILE_PTR)UART_getHandle( mfgUart ) );
         }
         else
#endif
         {
            MFGP_ProcessCommand( ( char * )commandBuf->data, commandBuf->x.dataLen );
#if ( USE_USB_MFG != 0 )
            OS_EVNT_Set( (OS_EVNT_Handle)&CMD_events, 1U );
#endif
         }
         BM_free( commandBuf );
      }
   }
}

/***********************************************************************************************************************
   Function Name: MFGP_uartRecvTask

   Purpose: collects incoming manufacturing serial port characters and calls appropriate command handler

   Arguments: Arg0 - an unused MQX required argument for tasks

   Returns: none
***********************************************************************************************************************/
/*lint -esym(715,Arg0) not referenced but required by API   */
void MFGP_uartRecvTask( taskParameter )
{
   uint8_t rxByte;
#if ( RTOS_SELECTION == MQX_RTOS )
   ( void )Arg0;
#endif

   SELF_notify = (OS_EVNT_Obj *)SELF_getEventHandle();   /* Find out the handle used to request a test.  */
   SELF_setEventNotify( &MFG_notify );    /* Set event handler to "notify" when test results are completed.  */
#if ( RTOS_SELECTION == MQX_RTOS )
#if USE_USB_MFG
   USB_SetEventNotify( &MFG_notify );     /* Set event handler to "notify" when USB reconnected.  */
   USB_App_Init();
   (void)OS_EVNT_Wait ( &MFG_notify, 1U, ( bool )false, OS_WAIT_FOREVER ); /* Wait for initial self test to complete */

#else
   FILE_PTR fh_ptr;
   if ( NULL != ( fh_ptr = fopen( MFG_PORT_IO_CHANNEL, NULL ) ) )
   {
      ( void )_io_set_handle( IO_STDOUT, fh_ptr );
      //MFG_printf("MFGP_uartTask: _io_set_handle SUCCESS!\n");
   }
#endif   // USE_USB_MFG
#endif   // RTOS_SELECTION
#if (USE_DTLS == 1)
   _MfgPortState = MFG_SERIAL_IO_e;
   MFGP_DtlsInit( mfgUart );

   for( ;; )
   {
#if ( USE_USB_MFG != 0 )
      rxByte = usb_getc(); /* Task will suspend until input available.  */
#else
#if ( MCU_SELECTED == NXP_K24 )
      while ( 0 != UART_read ( mfgUart, &rxByte, sizeof( rxByte ) ) )
#elif ( MCU_SELECTED == RA6E1 )
      while ( 0 != UART_getc ( mfgUart, &rxByte, sizeof( rxByte ), OS_WAIT_FOREVER ) )
#endif
#endif   // USE_USB_MFG
      {
         if ( _MfgPortState == DTLS_SERIAL_IO_e )
         {
            MFGP_UartRead( rxByte );
         }/* end of if() */
         else
         {
#if ( ECHO_OPTICAL_PORT != 0 )
            if ( ( mfgUart == UART_OPTICAL_PORT ) && ( rxByte != UART_SWITCH_CHAR ) )
            {
               UART_Transmit( mfgUart, &rxByte, 1 );
            }/* end of if() */
#endif   // ECHO_OPTICAL_PORT
#if ( ENABLE_B2B_COMM == 1 )
            if ( _MfgPortState == MFG_SERIAL_IO_e )
            {
               // If receive a 0x7E then switch to B2B framing mode!!!
               if(rxByte == 0x7E)
               {
                   _MfgPortState = B2B_SERIAL_IO_e;
                  {
                     int32_t flags;                /* Settings for the UART */
                     flags = IO_SERIAL_RAW_IO;     /* Settings for the UART */
                     (void)UART_ioctl( mfgUart, IO_IOCTL_SERIAL_SET_FLAGS, &flags );
                  }
               }/* end of if() */
            }/* end of if() */
            if ( _MfgPortState == B2B_SERIAL_IO_e )
            {
               hdlc_on_rx_u8(rxByte);
            }/* end of if() */
            else
            {
               mfgpReadByte( rxByte );
            }/* end of else() */
#else
            mfgpReadByte( rxByte );
#endif   // ENABLE_B2B_COMM

         }/* end of else() */
      }/* end of while() */
   }/* end of for () */
#else
   for( ;; )
   {
#if ( MCU_SELECTED == NXP_K24 )
      while ( 0 != UART_read ( mfgUart, &rxByte, sizeof( rxByte ) ) )
#elif ( MCU_SELECTED == RA6E1 )
      while ( 0 != UART_getc ( mfgUart, &rxByte, sizeof( rxByte ), OS_WAIT_FOREVER ) )
#endif
      {
         mfgpReadByte( rxByte );
      }
   }/* end for () */
#endif   // USE_DTLS
}

#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
/*******************************************************************************

   Function name: OptoPortTimer_Reset

   Purpose:  Used to reset the port timer

   Arguments: none

   Returns: bool

*******************************************************************************/
static void OptoPortTimerReset ( void )
{
   ( void )TMR_ResetTimer( OptoTimerID, TIME_TICKS_PER_HR );
}

/*******************************************************************************

   Function name: OptoTimer_CallBack

   Purpose: This function is called when the debug Opto timer expires.
            If the timer expires, the port will be disabled.

   Arguments:  uint8_t cmd - Not used, but required due to generic API
               void *pData - Not used, but required due to generic API

   Returns: none

   Notes:

*******************************************************************************/
static void  OptoTimerCallBack( uint8_t cmd, void *pData )
{
   MFG_logoff ( 0, NULL );
}  /*lint !e818 !e715 pData could be pointer to const, args not used; required by API */

/*******************************************************************************

   Function name: OptoTimer_Init

   Purpose: Used to initialize the Opto UART port Timer

   Arguments: none

   Returns: none

*******************************************************************************/
static void OptoTimer_Init ( void )
{
   /* Initialize/update timer used to automatically disable optical port if no activity is detected before the timer
      expires  */
   if ( OptoTimerCfg.pFunctCallBack != OptoTimerCallBack )
   {
      ( void )memset( &OptoTimerCfg, 0, sizeof( OptoTimerCfg ) ); /* Clear the timer values */
      OptoTimerCfg.bOneShot = true;
      OptoTimerCfg.bExpired = true;
      OptoTimerCfg.bStopped = true;
      OptoTimerCfg.ulDuration_mS = TIME_TICKS_PER_HR;
      OptoTimerCfg.pFunctCallBack = ( vFPtr_u8_pv )OptoTimerCallBack;
      ( void )TMR_AddTimer( &OptoTimerCfg );           /* Create the status update timer   */
      OptoTimerID = OptoTimerCfg.usiTimerId;
      OptoTimerCfg.bStopped = false;             /* Start the timer   */
   }
}
/***********************************************************************************************************************
   Function Name: processOptoBuf

   Purpose: Handles incoming psem message and determines whether to switch the UART used for MFG commands to the optical
            port.

   Arguments: uint8_t *optoMsg - buffer of characters received from the optical port.

   Returns: none
***********************************************************************************************************************/
static void processOptoBuf( sC1218Packet *optoMsg )
{
   sC1218Packet   *msg = optoMsg;
   uint32_t       ioctlFlags;

   if ( msg->cmd == CMD_LOG_ON_REQ )
   {
      HMC_COMM_SENSE_ON();                // Let meter know we have the port
      mfgUart = UART_OPTICAL_PORT;
      MFGP_DtlsInit( mfgUart );           /* Let dtls serial task know to use optical port.  */
      (void)UART_write( mfgUart, (uint8_t *)"OK\n", 3 );

      ioctlFlags = (uint32_t)UART_SWITCH_CHAR;  /* Send a UART_SWITCH_CHAR to the MFG Uart port. */
      ( void )UART_ioctl( UART_MANUF_TEST, (int32_t)IO_IOCTL_SERIAL_FORCE_RECEIVE, &ioctlFlags );

      loggedOn = (bool)true;
      OptoTimer_Init();
   }
}

typedef enum
{
   waitSTP,    /* No characters received, yet.  */
   waitID,     /* Got STP, waiting for ID byte. */
   waitCtrl,   /* Got ID, waiting for control byte. */
   waitSeq,    /* Got control, waiting for sequence byte. */
   waitLen1,   /* Waiting for message length MSB.   */
   waitLen2,   /* Waiting for message length LSB.   */
   readMsg,    /* Reading balance of message.   */
   readCRC1,   /* Two bytes of CRC follow message. */
   readCRC2
} optoTaskState;
/***********************************************************************************************************************
   Function Name: MFGP_optoTask

   Purpose: collects incoming meter's optical port characters and calls appropriate command handler

   Arguments: Arg0 - an unused MQX required argument for tasks

   Returns: none
***********************************************************************************************************************/
void MFGP_optoTask( uint32_t Arg0 )
{
   sC1218Packet      optoMsg;                         /* Buffer for incoming ANSI packet. */
   MQX_TICK_STRUCT   stpTime;                         /* Time when start of packet (STP) received. */
   MQX_TICK_STRUCT   curTime;                         /* Used to calc time since STP received.  */
   uint8_t          *pOptoBuf = (uint8_t *)&optoMsg;  /* Pointer to optoMsg buffer. */
   int32_t           flags;                           /* UART ioctl flags. */
   optoTaskState     optoState = waitSTP;             /* state machine variable. */
   uint16_t          msgLen = 0;                      /* Length of message retrieved from message. */
   uint8_t           rxByte;                          /* Incoming ANSI character.   */

   for( ;; )
   {
      while ( !loggedOn )
      {
#if ( MCU_SELECTED == NXP_K24 )
         if ( 0 != UART_read( UART_OPTICAL_PORT, &rxByte, sizeof( rxByte ) ) )
#elif ( MCU_SELECTED == RA6E1 )
         if ( 0 != UART_getc ( UART_OPTICAL_PORT, &rxByte, sizeof( rxByte ), OS_WAIT_FOREVER ) )
#endif
         {
            if ( ( uint32_t )( ( uint8_t * )pOptoBuf - ( uint8_t * )&optoMsg ) >= sizeof( optoMsg ) )
            {
               /* Receiving garbage? About to overflow the buffer. Reset machine state!   */
               optoState = waitSTP;             /* Set state machine to waiting for PSEM_STP. */
            }
            else
            {
               *pOptoBuf++ = rxByte;
            }

#if 0 /* Useful for debugging optical port reception only.  */
            DBG_logPrintf( 'O', "%02x", rxByte );
#endif

            switch ( optoState )
            {
               case waitSTP:     /* Waiting for start of packet.  */
               {
                  ( void )memset( &optoMsg, 0, sizeof( optoMsg ) );
                  pOptoBuf = (uint8_t *)&optoMsg;

                  if ( rxByte == PSEM_STP )
                  {
                     /* Got start of packet. Switch to UART non-blocking mode.   */
                     flags = IO_SERIAL_NON_BLOCKING;
                     ( void )UART_ioctl( UART_OPTICAL_PORT, (int32_t)IO_IOCTL_SERIAL_SET_FLAGS, &flags );
                     OS_TICK_Get_CurrentElapsedTicks( &stpTime ); /* Get "current" time   */

                     *pOptoBuf++ = rxByte;   /* Save received byte.  */
                     optoState = (optoTaskState)((uint8_t)optoState + 1);  /* Bump to next state.  */
                  }
                  break;
               }
               case waitID:      /* Waiting for ID byte. */
               {
                  if ( rxByte == 0x80 )
                  {
                     optoState = (optoTaskState)((uint8_t)optoState + 1);  /* Bump to next state.  */
                  }
                  else
                  {
                     optoState = waitSTP;       /* Set state machine to waiting for PSEM_STP. */
                  }
                  break;
               }
               case waitCtrl:    /* Next 2 bytes are "don't care".   */
               case waitSeq:
               {
                  optoState = (optoTaskState)((uint8_t)optoState + 1);  /* Bump to next state.  */
                  break;
               }
               case waitLen1:    /* Get MSB of message length. */
               {
                  msgLen = rxByte;
                  optoState = (optoTaskState)((uint8_t)optoState + 1);  /* Bump to next state.  */
                  break;
               }
               case waitLen2:    /* Get LSB of message length. */
               {
                  msgLen = (uint16_t)( msgLen << 8 ) + rxByte;
                  optoState = (optoTaskState)((uint8_t)optoState + 1);  /* Bump to next state.  */
                  break;
               }
               case readMsg:  /* Receiving balance of message. */
               {
                  if ( --msgLen == 0 ) /* Got entire message.  */
                  {
                     optoState = (optoTaskState)((uint8_t)optoState + 1);  /* Bump to next state.  */
                  }
                  break;
               }
               case readCRC1:  /* Receiving CRC, 1st byte. */
               {
                  optoState = (optoTaskState)((uint8_t)optoState + 1);  /* Bump to next state.  */
                  break;
               }
               case readCRC2:  /* Receiving CRC, 2nd byte. */
               {
                  processOptoBuf( &optoMsg );
                  optoState = waitSTP;
                  break;
               }
               default:
               {
                  optoState = waitSTP;
                  ( void )memset( &optoMsg, 0, sizeof( optoMsg ) );
                  pOptoBuf = (uint8_t *)&optoMsg;
                  break;
               }
            }
         }
         else  /* In non-blocking mode. Check time elapsed since receiving STP.  */
         {
            int32_t  nOutageSeconds;      /* Used to compute elapsed time since stp received.  */
            bool     bOverflow;           /* Used in elapsed time calculation.   */

            OS_TICK_Get_CurrentElapsedTicks( &curTime ); /* Get "current" time   */
            nOutageSeconds = _time_diff_seconds( &curTime, &stpTime, &bOverflow );
            if ( nOutageSeconds >= OPTO_PKT_TIMEOUT )
            {
               /* Timed out. Switch to UART blocking mode.   */
               flags = IO_SERIAL_RAW_IO;
               ( void )UART_ioctl( UART_OPTICAL_PORT, (int32_t)IO_IOCTL_SERIAL_SET_FLAGS, &flags );

               optoState = waitSTP; /* Reset the state machine.   */
               ( void )memset( &optoMsg, 0, sizeof( optoMsg ) );
               pOptoBuf = (uint8_t *)&optoMsg;
            }
            else
            {
               OS_TASK_Sleep( 100 );
            }
         }
      }
      /* If loop exits, optical port has been "passed off" to the mfg command task. */
      OS_TASK_Sleep( 1000 );
   }
}
#endif

/***********************************************************************************************************************
   Function Name: MFGP_ProcessCommand

   Purpose: accepts a space delimited string and converts it to an array of arguments (argv[])
            and an argument count (argc) for ease of processing, and then calls any functions registered as a handler
            for the first argument.

   Arguments:
      command - pointer to a space delimited, null terminated string
      numbytes - number of bytes that are considered valid at *command

   Returns: none
***********************************************************************************************************************/
static void MFGP_ProcessCommand ( char *command, uint16_t numBytes )
{
   struct_CmdLineEntry  const *CmdEntry;
   char                 *CmdString;
   uint32_t             argc = 0;
   uint16_t             i;
   menuType             table;
   bool                 NextArg = ( bool )true;
#if  ( TM_ROUTE_UNKNOWN_MFG_CMDS_TO_DBG == 1 )
   char                 tempBuffer[50];
#endif
   /* sanitize input - should be null terminated string of numBytes */
   if ( strnlen( command, MFGP_MAX_MFG_COMMAND_CHARS ) <= numBytes )
   {
      CmdString = command;
#if ( !USE_USB_MFG && ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A ) )
      // Copy of the original command for later
      strcpy( DbgCommandBuffer, command );
#elif  ( TM_ROUTE_UNKNOWN_MFG_CMDS_TO_DBG == 1 )
      if ( strlen( command ) < sizeof( tempBuffer ) )
      {
         strcpy( tempBuffer, command );
      }
      else
      {
         tempBuffer[0] = 0;
      }
#endif
      while ( *CmdString != 0 )
      {
         if ( *CmdString == ' ' )
         {
            *CmdString = 0;
            NextArg = ( bool )true;
         }
         else if ( NextArg == ( bool )true )
         {
            if ( argc < MFGP_MAX_CMDLINE_ARGS )
            {
               argvar[argc] = CmdString;
               argc++;
               NextArg = ( bool )false;
            }
            else
            {
               break;
            }
         }

         CmdString++;
      }
#if 0
      INFO_printHex( "command: ", command, numBytes );
#endif

      if ( argc > 0 )
      {
         uint8_t  securityMode;

         ( void )APP_MSG_SecurityHandler( method_get, appSecurityAuthMode, &securityMode, NULL );

#if (USE_DTLS == 1)
         if ( ( securityMode == 0 ) || ( _MfgPortState == DTLS_SERIAL_IO_e ) )
         {
#if ( EP == 1 )
            table = menuQuiet;
#else
            table = menuStd;
#endif //EP
         }
         else
         {
            table = menuSecure;
         }
#else    /* DTLS disabled  */
#endif
#if ( EP == 1 )
         table = menuQuiet;
#else
         table = menuStd;
#endif //EP
         CmdEntry = cmdTables[ table ];

         while ( CmdEntry->pcCmd != 0 )
         {
            if ( !strcasecmp( argvar[0], CmdEntry->pcCmd ) )
            {
               /* This is the line of code that is actually calling the handler function
                  for the command that was received */
_Pragma ( "calls = \
                 MFG_COMMON_CALLS \
                 MFG_DCU_CALLS \
                 MFG_FIO_CALLS \
                 MFG_EP_CALLS \
                 MFG_DTLS_CALLS \
                 MFG_MTLS_CALLS \
                 MFG_VSWR_CALLS \
                 MFG_NOT_LC_AND_NOT_DA_CALLS \
                 MFG_DEMAND_CALLS \
                 MFG_END_DEVICE_PROG_CALLS \
                 MFG_REMOTE_DISCONNET_CALLS \
                 MFG_ANSI_TABLES_CALLS \
                 MFG_DA_CALLS \
                 MFG_HMC_KV_OR_HMC_I210_PLUS_C_CALLS \
                 MFG_SAMPLE_METER_TEMP_CALLS \
                 MFG_LP_IN_METER_CALLS \
                 MFG_CLOCK_IN_METER_CALLS \
                 MFG_REMOTE_DISCONNECT_CALLS \
                 MFG_EP_LGSIM_CALLS \
                 " )
                   CmdEntry->pfnCmd( argc, argvar );
#if ( EP == 1 )
               /***************************************************************************
                  Reset the rfTestmode timer upon execution of a valid command
               ****************************************************************************/
               if ( MODECFG_get_rfTest_mode() != 0 )
               {
                  MFGP_rfTestTimerReset();
               }
#endif
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
               if ( loggedOn )
               {
                  OptoPortTimerReset();
               }
#endif
               break;
            }

            CmdEntry++;
            if ( CmdEntry->pcCmd == 0 )
            {
               /* Reached end of current table; bump to next, if any left  */
               if ( table < ( menuType )( ( uint16_t )menuLastValid - 1 ) )
               {
                  table = (menuType)((uint8_t)table + 1);
                  if ( MODECFG_get_quiet_mode() != 0 )   /* Only one menu available in "quiet mode".  */
                  {
                     break;
                  }
                  /* Check for secured mode command sent in non-secured operating mode and disallow processing.   */
#if ( USE_DTLS == 1 )
                  else if (  ( securityMode == 2 )      ||        /* All commands allowed - OR                    */
                        ( table != menuSecure )    ||             /* commands in this menu allowed in any mode.   */
                        ( _MfgPortState == DTLS_SERIAL_IO_e ) )   /* OR, still in DTLS_SERIAL_IO_e mode           */
#endif
                  {
                     CmdEntry = cmdTables[ ( uint16_t )table ];
                  }/* Else, CmdEntry points to a NULL and the command is considered invalid...   */
               }
            }
         }

         if ( CmdEntry->pcCmd == 0 )
         {
            /* We reached the end of the list and did not find a valid command */
            if ( MODECFG_get_quiet_mode() == 0 )
            {
#if ( !USE_USB_MFG && ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A ) )
               // Send unknown Mfg commands to debug command line interpreter instead
               DBG_CommandLine_Process ();
#elif ( TM_ROUTE_UNKNOWN_MFG_CMDS_TO_DBG == 1 )
                /* This makes DBG commands accessible from MFG port */
               if ( tempBuffer[0] )
               {
                  DBG_CommandLine_InvokeDebugCommandFromManufacturingPort ( tempBuffer );
               }
#else
#if ( MCU_SELECTED == NXP_K24 )
               MFG_logPrintf( "%s is not a valid command!\n", argvar[0] );
#elif ( MCU_SELECTED == RA6E1 )
               // TODO: RA6 [name_Balaji]: Integrate \r inside MFG_logPrintf
               MFG_logPrintf( "%s is not a valid command!\r\n", argvar[0] );
#endif   // MCU_SELECTED
#endif   // USE_USB_MFG
            }
            else
            {
               MFG_logPrintf( "%s is not available in quietmode\n", argvar[0] );
            }
            for ( i = 0; i < ( uint16_t )strlen( argvar[0] ); i++ )
            {
               if ( argvar[0][i] == 0x7F )
               {
                  DBG_logPrintf( 'R', "character %d was a backspace!", ( i + 1 ) );
               }
            }
         }
      }
   }
} /* end  */

/***********************************************************************************************************************
   Function Name: Service_Unavailable

   Purpose: Print PHY error message

   Arguments: none

   Returns: none
***********************************************************************************************************************/
static void Service_Unavailable( void )
{
   MFG_logPrintf( "Service unavailable\n" );
}

/***********************************************************************************************************************
   Function Name: MFGP_CommandLine_Help

   Purpose: Print the help menu for the command line interface

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_CommandLine_Help ( uint32_t argc, char *argv[] )
{
   struct_CmdLineEntry const  *CmdLineEntry;
   menuType                   menu;

#if ( EP == 1 )
   menu = menuQuiet;
#else
   menu = menuStd;
#endif
#if ( MCU_SELECTED == NXP_K24 )
   MFG_printf( "\nCommand List:\n" );
#elif ( MCU_SELECTED == RA6E1 )
   /* Added carriage return to follow printing standard */
   MFG_printf( "\r\nCommand List:\r\n" );
#endif
   CmdLineEntry = cmdTables[ menu ];
   while ( CmdLineEntry->pcCmd )
   {
#if ( MCU_SELECTED == NXP_K24 )
      MFG_printf( "%32s: %s\n", CmdLineEntry->pcCmd, CmdLineEntry->pcHelp );
#elif ( MCU_SELECTED == RA6E1 )
      /* Added Carriage return to follow Printing standard */
      MFG_printf( "%32s: %s\r\n", CmdLineEntry->pcCmd, CmdLineEntry->pcHelp );
#endif
      CmdLineEntry++;
      OS_TASK_Sleep( TEN_MSEC );    /* Delay of 10millisecond makes all prints visible */
      if ( CmdLineEntry->pcCmd == NULL )
      {
         menu = (menuType)((uint8_t)menu + 1);
         if ( menu < menuHidden )
         {
            CmdLineEntry = cmdTables[ menu ];
         }
      }/* end if() */
   }/* end while() */
}/* end MFGP_CommandLine_Help () */

/***********************************************************************************************************************
   Function Name: MFGP_firmwareVersion

   Purpose: Print the MTU firmware

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_firmwareVersion( uint32_t argc, char *argv[] )
{
   firmwareVersion_u ver;
   if ( strcasecmp( "comdevicebootloaderversion", argv[ 0 ] ) == 0)
   {
      ver = VER_getFirmwareVersion(eFWT_BL);
   }
   else
   {
      ver = VER_getFirmwareVersion(eFWT_APP);
   }
   MFG_printf( "%s %02d.%02d.%04d\n", argv[ 0 ], ver.field.version, ver.field.revision, ver.field.build );
}

/***********************************************************************************************************************
   Function Name: MFGP_hardwareVersion

   Purpose: Print the MTU firmware

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_hardwareVersion( uint32_t argc, char *argv[] )
{
   uint8_t  string[VER_HW_STR_LEN];

   if ( argc == 2 )           /* User wants to set the context */
   {
      ( void )VER_setHardwareVersion ( (uint8_t *)argv[1] );
   }

   /* Always print the actual value   */
   ( void )VER_getHardwareVersion ( &string[0], sizeof(string) );
   MFG_printf( "%s %s\n", argv[ 0 ], &string[0] );
}

/***********************************************************************************************************************
   Function Name: MFGP_DeviceType

   Purpose: Print the MTU Device Type

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_DeviceType( uint32_t argc, char *argv[] )
{
   ( void )argc;

   MFG_printf( "%s %s\n", argv[ 0 ], VER_getComDeviceType() );
}

/***********************************************************************************************************************
   Function Name: MFGP_ipHEContext

   Purpose: Get/Set the IP HE Context

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_ipHEContext( uint32_t argc, char *argv[] )
{
   NWK_GetConf_t     GetConf;
   char              *endptr;
   uint8_t           val;

   if ( argc == 2 )           /* User wants to set the context */
   {
      val = ( uint8_t )strtoul( argv[1], &endptr, 10 );
      (void)NWK_SetRequest( eNwkAttr_ipHEContext, &val);
   }

   /* Always print the actual value   */
   GetConf = NWK_GetRequest( eNwkAttr_ipHEContext );
   MFG_printf( "%s %u\n", argv[0], GetConf.val.ipHEContext );
}

/***********************************************************************************************************************
   Function Name: MFGP_macNetworkId

   Purpose: Get/Set the MAC Network ID

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_macNetworkId( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_NetworkId, U8_VALUE_TYPE);

}

/** **********************************************************************************************************************
 *
 *  **********************************************************************************************************************/
static void MFGP_macTxFrames          ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_TxFrameCount, U32_VALUE_TYPE);
}

/** **********************************************************************************************************************
 *
 *  **********************************************************************************************************************/
static void MFGP_macChannelSetsCount( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_ChannelSetsCountSRFN, U8_VALUE_TYPE);
}

/** **********************************************************************************************************************
 *
 *  **********************************************************************************************************************/
static void MFGP_macPacketId          ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_PacketId, U8_VALUE_TYPE);
}

/** **********************************************************************************************************************
 *
 *  **********************************************************************************************************************/
static void MFGP_macAckWaitDuration          ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_AckWaitDuration, U16_VALUE_TYPE);
}

/** **********************************************************************************************************************
 *
 *  **********************************************************************************************************************/
static void MFGP_macAckDelayDuration         ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_AckDelayDuration, U16_VALUE_TYPE);
}

/** **********************************************************************************************************************
 *
 *  **********************************************************************************************************************/
static void MFGP_macState                    ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_State, U8_VALUE_TYPE);
}

/** **********************************************************************************************************************
 *
 *  **********************************************************************************************************************/
static void MFGP_macIsChannelAccessConstrained ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_IsChannelAccessConstrained, BOOL_VALUE_TYPE);
}

/** **********************************************************************************************************************
 *
 *  **********************************************************************************************************************/
static void MFGP_macIsFNG                    ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_IsFNG, BOOL_VALUE_TYPE);
}

/** **********************************************************************************************************************
 *
 *  **********************************************************************************************************************/
static void MFGP_macIsIAG                    ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_IsIAG, BOOL_VALUE_TYPE);
}

/** **********************************************************************************************************************
 *
 *  **********************************************************************************************************************/
static void MFGP_macIsRouter                 ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_IsRouter, BOOL_VALUE_TYPE);
}


#if PHASE_DETECTION == 1
/*******************************************************************************

   Function Name:

   Purpose: Get the Phase Detect ...

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none

*******************************************************************************/
static void PD_SurveySelfAssessment   ( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %d\n", argv[0], PD_SurveySelfAssessment_Get() );
}

/*******************************************************************************

   Function Name:

   Purpose: Get the Phase Detect ...

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none

*******************************************************************************/
static void PD_SurveyCapable          ( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %d\n", argv[0], PD_SurveyCapable_Get() );
}

/*******************************************************************************

   Function Name: PD_SurveyStartTime

   Purpose: Get/Set the Phase Detect SurveyStartTime

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none

*******************************************************************************/
static void PD_SurveyStartTime ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      char *endptr;
      uint32_t u32_val = ( uint32_t )strtoul( argv[1], &endptr, 10 );
      (void)PD_SurveyStartTime_Set(u32_val);
   }
   MFG_printf( "%s %d\n", argv[0], PD_SurveyStartTime_Get() );
}

/*******************************************************************************

   Function Name: PD_SurveyPeriod

   Purpose: Get/Set the Phase Detect SurveyPeriod

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none

*******************************************************************************/
static void PD_SurveyPeriod ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      char *endptr;
      uint32_t u32_val = ( uint32_t )strtoul( argv[1], &endptr, 10 );
      (void)PD_SurveyPeriod_Set(u32_val);
   }
   MFG_printf( "%s %d\n", argv[0], PD_SurveyPeriod_Get() );
}

/*******************************************************************************

   Function Name: PD_SurveyQuantity

   Purpose: Get/Set the Phase Detect SurveyQuantity

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none

*******************************************************************************/
static void PD_SurveyQuantity ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      char *endptr;
      uint8_t u8_val = ( uint8_t )strtoul( argv[1], &endptr, 10 );
      (void) PD_SurveyQuantity_Set(u8_val);
   }
   MFG_printf( "%s %d\n", argv[0], PD_SurveyQuantity_Get() );
}

/*******************************************************************************

   Function Name: PD_SurveyMode

   Purpose: Get/Set the Phase Detect SurveyMode

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none

   Example:
      pdSurveyMode 0     Disabled
      pdSurveyMode 1     Daily
      pdSurveyMode 2     Interval
      pdSurveyMode 3     >2 is invalid

*******************************************************************************/
static void PD_SurveyReportMode ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      char *endptr;
      uint8_t u8_val = ( uint8_t )strtoul( argv[1], &endptr, 10 );
      (void) PD_SurveyMode_Set(u8_val);
   }
   MFG_printf( "%s %d\n", argv[0], PD_SurveyMode_Get());
}

/*******************************************************************************

   Function Name: PD_ZCProductNullingOffset

   Purpose: Get/Set the Phase Detect ZCProductNullingOffset

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none

   Example:
      pdZCProductNullingOffset -30000
      pdZCProductNullingOffset  30000

*******************************************************************************/
static void PD_ZCProductNullingOffset ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      char *endptr;
      int32_t i32_val = ( int32_t )strtoul( argv[1], &endptr, 10 );
      (void) PD_ZCProductNullingOffset_Set(i32_val);
   }
   MFG_printf( "%s %d\n", argv[0], PD_ZCProductNullingOffset_Get());
}

/*******************************************************************************

   Function Name: PD_TimeDiversity

   Purpose: Get/Set the Time Maximum Diversity

   Arguments:

   Returns: none

*******************************************************************************/
static void PD_BuMaxTimeDiversity            ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      uint16_t value = ( uint16_t )atol( argv[1] );
      (void) PD_BuMaxTimeDiversity_Set(value);
   }
   MFG_printf( "%s %d\n", argv[0], PD_BuMaxTimeDiversity_Get());
}

/*******************************************************************************

   Function Name: PD_DataRedundancy

   Purpose: Get/Set the Data Redundancy

   Arguments:

   Returns: none

*******************************************************************************/
static void PD_BuDataRedundancy           ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      uint8_t value = ( uint8_t )atol( argv[1] );
      (void) PD_BuDataRedundancy_Set(value);
   }
   MFG_printf( "%s %d\n", argv[0], PD_BuDataRedundancy_Get());
}

/*******************************************************************************

   Function Name:

   Purpose: Get/Set the Phase Detect LCD Message

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none

   Example:
      pdLcdMsg PH----

   Notes:
      From the MFG port entering spaces is not supported. This is because the
      input with spoaces is treated as multiple arguments

*******************************************************************************/
static void PD_LcdMsg ( uint32_t argc, char *argv[] )
{
   if ( argc > 1 )
   {
      uint8_t size = ( uint8_t )strlen( argv[1] );
      for(uint8_t i = 0; i < size; i++)
      {  // replace each tilda character in the string with whitespace
         if( TILDA_CHAR == argv[1][i])
         {
            argv[1][i] = WHITE_SPACE_CHAR;
         }
      }
      (void)PD_LcdMsg_Set((uint8_t const *) argv[1]);
   }

   char result[PD_LCD_MSG_SIZE+1];
   memset(result,0,sizeof(result));
   (void)PD_LcdMsg_Get(result);
   MFG_printf( "%s %s\n", argv[0], result);
}

/*******************************************************************************

   Function Name: PD_SurveyPeriodQty

   Purpose: Get/Set the Phase Detect Survey Period Quantity

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none

*******************************************************************************/
static void PD_SurveyPeriodQty ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      char *endptr;
      uint8_t u8_val = ( uint8_t )strtoul( argv[1], &endptr, 10 );
      (void)PD_SurveyPeriodQty_Set(u8_val);
   }
   MFG_printf( "%s %d\n", argv[0], PD_SurveyPeriodQty_Get() );
}
#endif   // PHASE_DETECTION == 1


/*******************************************************************************

   Function name: MFGP_newRegistrationRequired

   Purpose: This function will print out or set the registration state

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
#if ( EP == 1 )
static void MFGP_newRegistrationRequired ( uint32_t argc, char *argv[] )
{
   uint8_t currRegState;

   if ( argc == 2 )  /* Set new value  */
   {
      currRegState = ( uint8_t )strtoull( argv[1], NULL, 0 );
      if ( currRegState == 1 )            /* Registration required; map to NOT SENT   */
      {
         APP_MSG_ReEnableRegistration();  /* Restart the registration process */
      }
      else if ( currRegState == 0 )       /* Registration NOT required; map to CONFIRMED   */
      {
         RegistrationInfo.registrationStatus       = REG_STATUS_CONFIRMED;
         RegistrationInfo.nextRegistrationTimeout  = RegistrationInfo.initialRegistrationTimeout;
         APP_MSG_updateRegistrationTimeout();   // sets active Registration Timeout to a random value
         APP_MSG_UpdateRegistrationFile();
      }
      else
      {
         MFG_logPrintf( "Invalid Registration State value - must be { 0 | 1 }\n" );
      }
   }
   currRegState = RegistrationInfo.registrationStatus;
   MFG_printf( "%s %d\n", argv[ 0 ], ( currRegState == REG_STATUS_CONFIRMED ) ? 0 : 1 );
}

static void MFGP_initialRegistrationTimemout( uint32_t argc, char *argv[] )
{
   uint16_t uInitialRegistrationTimeout;

   /* if new value was provided, set new value */
   if ( argc == 2 )  /* set new value */
   {
      uInitialRegistrationTimeout = ( uint16_t )atol( argv[1] );
      (void)APP_MSG_setInitialRegistrationTimeout(uInitialRegistrationTimeout);
   }

   /* always print out value */
   MFG_printf( "%s %d\n", argv[ 0 ], RegistrationInfo.initialRegistrationTimeout );
}

static void MFGP_minRegistrationTimemout( uint32_t argc, char *argv[] )
{
   uint16_t uMinRegistrationTimeout;

   /* if new value was provided, set new value */
   if ( argc == 2 )  /* set new value */
   {
      uMinRegistrationTimeout = ( uint16_t )atol( argv[1] );
      (void)APP_MSG_setMinRegistrationTimeout(uMinRegistrationTimeout);
   }

   /* always print out value */
   MFG_printf( "%s %d\n", argv[ 0 ], RegistrationInfo.minRegistrationTimeout );
}

static void MFGP_maxRegistrationTimemout( uint32_t argc, char *argv[] )
{
   uint32_t uMaxRegistrationTimeout;

   /* if new value was provided, set new value */
   if ( argc == 2 )  /* set new value */
   {
      uMaxRegistrationTimeout = ( uint32_t )atol( argv[1] );
      (void)APP_MSG_setMaxRegistrationTimeout(uMaxRegistrationTimeout);
   }

   /* always print out value */
   MFG_printf( "%s %d\n", argv[ 0 ], RegistrationInfo.maxRegistrationTimeout );
}

/*******************************************************************************

   Function name: MFGP_quietMode

   Purpose: This function will print out or set the MTU's "quiet" mode
            On change of mode, system will reboot.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static void MFGP_quietMode( uint32_t argc, char *argv[] )
{
   uint8_t  quietState;                   /* User entered value   */
   uint8_t  qMode;                        /* Current system setting  */
   bool     rebootNeeded = (bool)false;

   qMode = MODECFG_get_quiet_mode();
   if ( argc == 2 )  /* Set new value  */
   {
      quietState = ( uint8_t )strtoull( argv[1], NULL, 0 );
      if ( qMode != quietState )  /* Actually changing mode? */
      {
         if ( quietState == 1 )              /* Entering quiet mode  */
         {
            /* Cannot enter quiet mode when ship or metershop mode is enabled - mutexes needed for clearing data do not exist */
            if ( ( 0 != MODECFG_get_ship_mode() ) || ( 0 != MODECFG_get_decommission_mode() ) )
            {
               MFG_printf( "Cannot enter quietmode while in shipMode or meterShopMode\n" );
            }
            else
            {
               /* Make sure to end dtls mode, if active! */
               uint8_t val = 0;
               (void)APP_MSG_SecurityHandler( method_put, appSecurityAuthMode, &val, NULL );
               if ( MODECFG_get_rfTest_mode() != 0 )
               {
                  MFG_printf( "%s: Exiting rf test mode\n", argv[ 0 ] );
                  (void)MODECFG_set_rfTest_mode( 0 );
               }
               rebootNeeded = (bool)true;
            }
         }
         else if ( quietState == 0 )         /* Exiting quiet mode   */
         {
            rebootNeeded = (bool)true;
         }
         if ( rebootNeeded )                 /* Valid value entered, different from current value. */
         {
            MFG_printf( "%s %s quiet mode\n", argv[ 0 ], quietState == 1 ? "Entering" : "Exiting" );

            qMode = quietState;
            ( void )MODECFG_set_quiet_mode( qMode );
            OS_TASK_Sleep( 200 );         /* Wait long enough for the printing to complete  */
            PWR_SafeReset();
         }
      }
   }
   MFG_printf( "%s %d\n", argv[ 0 ], qMode );
}

#if ( EP == 1 )
/*******************************************************************************

   Function name: MFGP_rfTestMode

   Purpose: This function will print out or set the MTU's "rfTest" mode
            On change of mode, system will reboot.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static void MFGP_rfTestMode( uint32_t argc, char *argv[] )
{
   uint8_t  rfTestState;                   /* User entered value   */
   uint8_t  qMode;                        /* Current system setting  */
   bool     rebootNeeded = (bool)false;

   qMode = MODECFG_get_rfTest_mode();
   if ( argc == 2 )  /* Set new value  */
   {
      rfTestState = ( uint8_t )strtoull( argv[1], NULL, 0 );
      if ( qMode != rfTestState )  /* Actually changing mode? */
      {
         if ( rfTestState == 1 )              /* Entering rfTest mode  */
         {
            /* Make sure to end dtls mode, if active! */
            uint8_t val = 0;
            (void)APP_MSG_SecurityHandler( method_put, appSecurityAuthMode, &val, NULL );
            rebootNeeded = (bool)true;
         }
         else if ( rfTestState == 0 )         /* Exiting rfTest mode   */
         {
            rebootNeeded = (bool)true;
         }
         if ( rebootNeeded )                 /* Valid value entered, different from current value. */
         {
            MFG_printf( "%s %s rf test mode\n", argv[ 0 ], rfTestState == 1 ? "Entering" : "Exiting" );

            qMode = rfTestState;
            ( void )MODECFG_set_rfTest_mode( qMode );
            OS_TASK_Sleep( 200 );         /* Wait long enough for the printing to complete  */
            PWR_SafeReset();
         }
      }
   }
   MFG_printf( "%s %d\n", argv[ 0 ], qMode );
}
#endif

/***********************************************************************************************************************
Function Name: MFGP_smLogTimeDiversity

Purpose: Set or Print the Stack Manager Log Time Diversity

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_smLogTimeDiversity( uint32_t argc, char *argv[] )
{
   uint8_t uSmLogTimeDiversity;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write smLogTimeDiversity
         uSmLogTimeDiversity = ( uint8_t )atol( argv[1] );
         ( void )SMTDCFG_set_log( uSmLogTimeDiversity );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], SMTDCFG_get_log() );
}

#if ( EP == 1 )
/***********************************************************************************************************************
Function Name: MFGP_timeSigOffset

Purpose: Set or Print the timeSigOffset.

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_timeSigOffset( uint32_t argc, char *argv[] )
{
   uint16_t utimeSigOffset;

   if( argc <= 2 )
   {
      if( 2 == argc)
      {

        utimeSigOffset = (uint16_t) atol(argv[1] );
        (void)TIME_SYS_setTimeSigOffset(utimeSigOffset);  //setting value of timeSigOffset
      }
   }

   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
   MFG_printf( "%s %u\n", argv[ 0 ], TIME_SYS_getTimeSigOffset()); //always get the current value of timesigoffset
}
#endif

/***********************************************************************************************************************
Function Name: MFGP_timeAcceptanceDelay

Purpose: Set or Print the Time Acceptance Delay for MAC Time Set.

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_timeAcceptanceDelay( uint32_t argc, char *argv[] )
{
   uint16_t uTimeAcceptanceDelay;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write timeAcceptanceDelay
         uTimeAcceptanceDelay = ( uint16_t )atol( argv[1] );
         ( void )TIME_SYS_SetTimeAcceptanceDelay( uTimeAcceptanceDelay );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], TIME_SYS_GetTimeAcceptanceDelay() );
}
#endif

#if ( EP == 1 )
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
/***********************************************************************************************************************
   Function Name: MFGP_lpBuEnabled

   Purpose: Set or Print the lpBuEnabled parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_lpBuEnabled( uint32_t argc, char *argv[] )
{
   uint8_t uLpBuEnabled;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uLpBuEnabled = ( uint8_t )atoi( argv[1] );
         if (  uLpBuEnabled >= 1 )
         {
            ( void )ID_setLpBuEnabled( (uint8_t)true );
         }
         else
         {
            ( void )ID_setLpBuEnabled( (uint8_t)false );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], ID_getLpBuEnabled() );
}
#endif // endif of (ACLARA_LC == 0)
#endif

/***********************************************************************************************************************
   Function Name: MFGP_getRealTimeAlarm

   Purpose: Dispaly whether the endpoint supports real time alarms.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_getRealTimeAlarm( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %d\n", argv[ 0 ], EVL_getRealTimeAlarm() );
}

/***********************************************************************************************************************
   Function Name: MFGP_fctModuleTestDate

   Purpose: Set or Print the fctModuleTestDate.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_fctModuleTestDate( uint32_t argc, char *argv[] )
{
   uint16_t ufctModuleTestDate;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         ufctModuleTestDate = ( uint16_t )atoi( argv[1] );
         MIMTINFO_setFctModuleTestDate( ufctModuleTestDate );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], MIMTINFO_getFctModuleTestDate() );
}


/***********************************************************************************************************************
   Function Name: MFGP_fctEnclosureTestDate

   Purpose: Set or Print the fctEnclosureTestDate.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_fctEnclosureTestDate( uint32_t argc, char *argv[] )
{
   uint16_t ufctEnclosureTestDate;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         ufctEnclosureTestDate = ( uint16_t )atoi( argv[1] );
         MIMTINFO_setFctEnclosureTestDate( ufctEnclosureTestDate );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], MIMTINFO_getFctEnclosureTestDate() );
}


/***********************************************************************************************************************
   Function Name: MFGP_integrationSetupDate

   Purpose: Set or Print the integrationSetupDate.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_integrationSetupDate( uint32_t argc, char *argv[] )
{
   uint16_t uIntegrationSetupDate;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         uIntegrationSetupDate = ( uint16_t )atoi( argv[1] );
         MIMTINFO_setIntegrationSetupDate( uIntegrationSetupDate );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ],MIMTINFO_getIntegrationSetupDate() );
}


/***********************************************************************************************************************
   Function Name: MFGP_fctModuleProgramVersion

   Purpose: Set or Print the fctModuleProgramVersion.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_fctModuleProgramVersion( uint32_t argc, char *argv[] )
{
   uint32_t uFctModuleProgramVersion;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         uFctModuleProgramVersion = ( uint32_t )atoi( argv[1] );
         MIMTINFO_setFctModuleProgramVersion( uFctModuleProgramVersion );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ],MIMTINFO_getFctModuleProgramVersion() );
}


/***********************************************************************************************************************
   Function Name: MFGP_fctEnclosureProgramVersion

   Purpose: Set or Print the fctEnclosureProgramVersion.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_fctEnclosureProgramVersion( uint32_t argc, char *argv[] )
{
   uint32_t uFctEnclosureProgramVersion;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         uFctEnclosureProgramVersion = ( uint32_t )atoi( argv[1] );
         MIMTINFO_setFctEnclosureProgramVersion( uFctEnclosureProgramVersion );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ],MIMTINFO_getFctEnclosureProgramVersion() );
}


/***********************************************************************************************************************
   Function Name: MFGP_integrationProgramVersion

   Purpose: Set or Print the integrationProgramVersion.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_integrationProgramVersion( uint32_t argc, char *argv[] )
{
   uint32_t uIntegrationProgramVersion;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         uIntegrationProgramVersion = ( uint32_t )atoi( argv[1] );
         MIMTINFO_setIntegrationProgramVersion( uIntegrationProgramVersion );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ],MIMTINFO_getIntegrationProgramVersion() );
}


/***********************************************************************************************************************
   Function Name: MFGP_fctModuleDatabaseVersion

   Purpose: Set or Print the fctModuleDatabaseVersion.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_fctModuleDatabaseVersion( uint32_t argc, char *argv[] )
{
   uint32_t uFctModuleDatabaseVersion;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         uFctModuleDatabaseVersion = ( uint32_t )atoi( argv[1] );
         MIMTINFO_setFctModuleDatabaseVersion( uFctModuleDatabaseVersion );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ],MIMTINFO_getFctModuleDatabaseVersion() );
}


/***********************************************************************************************************************
   Function Name: MFGP_fctEnclosureDatabaseVersion

   Purpose: Set or Print the fctEnclosureDatabaseVersion.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_fctEnclosureDatabaseVersion( uint32_t argc, char *argv[] )
{
   uint32_t uFctEnclosureDatabaseVersion;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         uFctEnclosureDatabaseVersion = ( uint32_t )atoi( argv[1] );
         MIMTINFO_setFctEnclosureDatabaseVersion( uFctEnclosureDatabaseVersion );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ],MIMTINFO_getFctEnclosureDatabaseVersion() );
}


/***********************************************************************************************************************
   Function Name: MFGP_integrationDatabaseVersion

   Purpose: Set or Print the integrationDatabaseVersion.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_integrationDatabaseVersion( uint32_t argc, char *argv[] )
{
   uint32_t uIntegrationDatabaseVersion;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         uIntegrationDatabaseVersion = ( uint32_t )atoi( argv[1] );
         MIMTINFO_setIntegrationDatabaseVersion( uIntegrationDatabaseVersion );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ],MIMTINFO_getIntegrationDatabaseVersion() );
}


/***********************************************************************************************************************
   Function Name: MFGP_dataConfigurationDocumentVersion

   Purpose: Set or Print the dataConfigurationDocumentVersion.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_dataConfigurationDocumentVersion( uint32_t argc, char *argv[] )
{
   uint64_t uDataConfigurationDocumentVersion;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         uDataConfigurationDocumentVersion = ( uint64_t )atoll( argv[1] );
         MIMTINFO_setDataConfigurationDocumentVersion( uDataConfigurationDocumentVersion );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %llu\n", argv[ 0 ],MIMTINFO_getDataConfigurationDocumentVersion() );
}

/***********************************************************************************************************************
   Function Name: MFGP_macPacketTimeout

   Purpose: Get/Set MAC packet timeout

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_macPacketTimeout( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_PacketTimeout, U16_VALUE_TYPE);
}

/***********************************************************************************************************************
   Function Name: MFG_TransactionTimeout

   Purpose: Get/Set MAC transaction timeout

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_TransactionTimeout( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_TransactionTimeout, U16_VALUE_TYPE);
}

/***********************************************************************************************************************
   Function Name: MFG_TransactionTimeoutCount

   Purpose: Get the number of MAC timeouts

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_TransactionTimeoutCount( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_TransactionTimeout, U32_VALUE_TYPE);
}

/***********************************************************************************************************************
   Function Name: MFG_TransactionTimeoutCount

   Purpose: Get the number of times a delay was necesary

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_macTxLinkDelayCount(uint32_t argc, char *argv[])
{
   MAC_GetConf_t GetConf;

   MAC_ATTRIBUTES_e eAttribute = eMacAttr_TxLinkDelayCount;

   /* Always print read back value  */
   GetConf = MAC_GetRequest( eAttribute );
   if ( GetConf.eStatus == eMAC_GET_SUCCESS )
   {
      MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.TxLinkDelayCount );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFG_TransactionTimeoutCount

   Purpose: Get the total amount of delay incurred in Milliseconds.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_macTxLinkDelayTime(uint32_t argc, char *argv[])
{
   MAC_GetConf_t GetConf;

   MAC_ATTRIBUTES_e eAttribute = eMacAttr_TxLinkDelayTime;

   /* Always print read back value  */
   GetConf = MAC_GetRequest( eAttribute );
   if ( GetConf.eStatus == eMAC_GET_SUCCESS )
   {
      MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.TxLinkDelayTime );
   }
   else
   {
      Service_Unavailable();
   }
}


/***********************************************************************************************************************
   Function Name: MFGP_manufacturerNumber

   Purpose: Set or Print the manufacturerNumber.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_manufacturerNumber( uint32_t argc, char *argv[] )
{
   uint64_t uManufacturerNumber;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         uManufacturerNumber = ( uint64_t )atoll( argv[1] );
         MIMTINFO_setManufacturerNumber( uManufacturerNumber );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %llu\n", argv[ 0 ],MIMTINFO_getManufacturerNumber() );
}


/***********************************************************************************************************************
   Function Name: MFGP_repairInformation

   Purpose: Set or Print the repairInformation.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_repairInformation( uint32_t argc, char *argv[] )
{
   uint32_t uRepairInformation;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         uRepairInformation = ( uint32_t )atoi( argv[1] );
         MIMTINFO_setRepairInformation(  uRepairInformation );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ],MIMTINFO_getRepairInformation() );
}

#if ( EP == 1 )
/***********************************************************************************************************************
   Function Name: MFGP_dateTimeLostCount

   Purpose: Set or Print the dateTimeLostCount parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_dateTimeLostCount( uint32_t argc, char *argv[] )
{
   uint16_t uDateTimeLostCount;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {

         uDateTimeLostCount = ( uint16_t )atoi( argv[1] );
         TIME_SYS_SetDateTimeLostCount(  uDateTimeLostCount );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ],TIME_SYS_GetDateTimeLostCount() );
}
#endif
/***********************************************************************************************************************
   Function Name: MFGP_timeLastUpdated

   Purpose: Set or Print the dateTimeLostCount parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_timeLastUpdated( uint32_t argc, char *argv[] )
{
   sysTime_t             sysTime;
   sysTime_dateFormat_t  sysDateTime;
   uint32_t TimeLastUpdated = TIME_SYS_GetTimeLastUpdated();

   TIME_UTIL_ConvertSecondsToSysFormat( TimeLastUpdated, 0, &sysTime);
   (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sysTime, &sysDateTime);

   /* Always print read back value  */
   MFG_printf( "%s %02u/%02u/%04u %02u:%02u:%02u (%u) \n", argv[ 0 ], sysDateTime.month, sysDateTime.day, sysDateTime.year, sysDateTime.hour, sysDateTime.min, sysDateTime.sec, TimeLastUpdated );
}
#if (EP == 1)
/***********************************************************************************************************************
   Function Name: MFGP_PhyAfcEnable

   Purpose: Set or Print the AFC enable state

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_PhyAfcEnable( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   bool enable;

   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         enable = ( bool )atoi( argv[1] );
         (void)PHY_SetRequest( ePhyAttr_AfcEnable, &enable);
      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( ePhyAttr_AfcEnable );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %u\n", argv[ 0 ], GetConf.val.AfcEnable );
   }
}

/***********************************************************************************************************************
   Function Name: MFGP_PhyAfcRSSIThreshold

   Purpose: Set or Print the AFC RSSI threshold

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_PhyAfcRSSIThreshold( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   int16_t threshold;

   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         threshold = ( int16_t )atoi( argv[1] );
         (void)PHY_SetRequest( ePhyAttr_AfcRssiThreshold, &threshold);

      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( ePhyAttr_AfcRssiThreshold );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.AfcRssiThreshold );
   }
   else
   {
      Service_Unavailable();
   }
}


/***********************************************************************************************************************
   Function Name: MFGP_PhyAfcTemperatureRange

   Purpose: Set or Print the AFC temperature range

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_PhyAfcTemperatureRange( uint32_t argc, char *argv[] )
{
   uint32_t i;
   char *token;                           /* argv[1] tokens */
   PHY_GetConf_t GetConf;
   int8_t AfcTemperatureRange[2];
   int32_t temperature_C;

   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         // Load current values
         GetConf = PHY_GetRequest( ePhyAttr_AfcTemperatureRange );
         if ( GetConf.eStatus == ePHY_GET_SUCCESS )
         {
            AfcTemperatureRange[0] = GetConf.val.AfcTemperatureRange[0];
            AfcTemperatureRange[1] = GetConf.val.AfcTemperatureRange[1];

            /* Loop through temperatures */
            for ( i = 0, token = argv[ 1 ]; i < 2; i++ )
            {
               if( ( i == 0 ) || ( token != NULL ) )  /* There's something in slot 0, because argc > 1. */
               {
                  if (sscanf( token, "%ld,", &temperature_C )) /* Convert value at token  */
                  {
                     AfcTemperatureRange[i] = (int8_t)temperature_C;
                  }
                  token = strpbrk( token, "," );      /* Find next token         */
                  if ( token != NULL )                   /*  If another comma is present, bump past it */
                  {
                     token++;
                  }
               }
            }
            (void)PHY_SetRequest( ePhyAttr_AfcTemperatureRange, &AfcTemperatureRange[0]);
         }
      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( ePhyAttr_AfcTemperatureRange );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %d,%d\n", argv[ 0 ], GetConf.val.AfcTemperatureRange[0], GetConf.val.AfcTemperatureRange[1] );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFGP_timeZoneDSTHash

   Purpose: Set or Print the the time zone hash

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_timeZoneDSTHash( uint32_t argc, char *argv[] )
{
   uint32_t uTimeZoneDSTHash;

   if ( argc > 1 )
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   DST_getTimeZoneDSTHash( &uTimeZoneDSTHash );
   MFG_printf( "%s %u\n", argv[ 0 ], uTimeZoneDSTHash );
}

/***********************************************************************************************************************
   Function Name: MFGP_timeZoneOffset

   Purpose: Set or Print the Time Zone offset

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_timeZoneOffset( uint32_t argc, char *argv[] )
{
   int32_t offset;

   if ( argc <=  2)
   {
      if ( 2 == argc )
      {
         /* write time zone offset */
         offset = ( int32_t )atol( argv[1] );
         ( void )DST_setTimeZoneOffset( offset );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   DST_getTimeZoneOffset( &offset );
   MFG_printf( "%s %ld\n", argv[ 0 ], offset );
}

/***********************************************************************************************************************
   Function Name: MFGP_dstEnabled

   Purpose: Set or Print the DST enabled flag

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_dstEnabled( uint32_t argc, char *argv[] )
{
   int8_t nDstEnabled;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write dstEnabled
         nDstEnabled = ( int8_t )atol( argv[1] );
         ( void )DST_setDstEnable( nDstEnabled );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   DST_getDstEnable( &nDstEnabled );
   MFG_printf( "%s %d\n", argv[ 0 ], nDstEnabled );
}

/***********************************************************************************************************************
   Function Name: MFGP_dstOffset

   Purpose: Set or Print the DST offset

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_dstOffset( uint32_t argc, char *argv[] )
{
   int32_t nDstOffset; //for convience actual parameter is int16_T

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write dstOffset
         nDstOffset = atol( argv[1] );
         if(SHRT_MIN <= nDstOffset && nDstOffset <= SHRT_MAX) //make input within param size
         {
            ( void )DST_setDstOffset( ( int16_t ) nDstOffset );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   DST_getDstOffset( (int16_t *) &nDstOffset );
   MFG_printf( "%s %d\n", argv[ 0 ], nDstOffset );
}

/***********************************************************************************************************************
   Function Name: MFGP_dstStartRule

   Purpose: Set or Print the DST start rule

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_dstStartRule( uint32_t argc, char *argv[] )
{
   uint64_t uDstStartRule = 0;
   uint8_t uDayOccurence;
   uint8_t uDayOfWeek;
   uint8_t uHour;
   uint8_t uMinute;
   uint8_t uMonth;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write dstStartRule
         uDstStartRule = ( uint64_t )strtoull( argv[1], NULL, 0 );

         ( void )DST_setDstStartRule(
                     ( uint8_t ) ( ( uDstStartRule >> 32 ) & 0xffu ), // Month
                     ( uint8_t ) ( ( uDstStartRule >> 24 ) & 0xffu ), // Day of Week
                     ( uint8_t ) ( ( uDstStartRule >> 16 ) & 0xffu ), // Day Occurence
                     ( uint8_t ) ( ( uDstStartRule >> 8 ) & 0xffu ), // Hour
                     ( uint8_t ) ( uDstStartRule & 0xffu ) );       // Minute
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   DST_getDstStartRule( &uMonth, &uDayOfWeek, &uDayOccurence, &uHour, &uMinute );

   uDstStartRule =   ( ( ( uint64_t ) uMonth )        << 32 ) |
                     ( ( ( uint64_t ) uDayOfWeek )    << 24 ) |
                     ( ( ( uint64_t ) uDayOccurence ) << 16 ) |
                     ( ( ( uint64_t ) uHour )         <<  8 ) |
                     (   ( uint64_t ) uMinute );

   MFG_printf( "%s %llu\n", argv[ 0 ], uDstStartRule );
}
/***********************************************************************************************************************
   Function Name: MFGP_dstEndRule

   Purpose: Set or Print the DST end rule

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_dstEndRule( uint32_t argc, char *argv[] )
{
   uint64_t uDstEndRule = 0;
   uint8_t uDayOccurence;
   uint8_t uDayOfWeek;
   uint8_t uHour;
   uint8_t uMinute;
   uint8_t uMonth;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write dstEndRule
         uDstEndRule = ( uint64_t )strtoull( argv[1], NULL, 0 );

         ( void )DST_setDstStopRule(
                     ( uint8_t ) ( ( uDstEndRule >> 32 ) & 0xffu ), // Month
                     ( uint8_t ) ( ( uDstEndRule >> 24 ) & 0xffu ), // Day of Week
                     ( uint8_t ) ( ( uDstEndRule >> 16 ) & 0xffu ), // Day Occurence
                     ( uint8_t ) ( ( uDstEndRule >>  8 ) & 0xffu ), // Hour
                     ( uint8_t ) ( uDstEndRule & 0xffu ) );         // Minute
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   DST_getDstStopRule( &uMonth, &uDayOfWeek, &uDayOccurence, &uHour, &uMinute );

   uDstEndRule =  ( ( ( uint64_t ) uMonth )        << 32 ) |
                  ( ( ( uint64_t ) uDayOfWeek )    << 24 ) |
                  ( ( ( uint64_t ) uDayOccurence ) << 16 ) |
                  ( ( ( uint64_t ) uHour )         <<  8 ) |
                  ( (   uint64_t ) uMinute );

   MFG_printf( "%s %llu\n", argv[ 0 ], uDstEndRule );

}
#endif //#if (EP == 1)

/***********************************************************************************************************************
   Function Name: MFGP_nvtest

   Purpose: Test the external NV memory device

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_nvtest( uint32_t argc, char *argv[] )
{
   uint32_t          LoopCount = 1;    /* Assume 1 pass through the test   */
   uint32_t          errCount = 0;     /* Number of failures reported by DVR_EFL_unitTest */
   char              *endptr;
   uint32_t          event_flags;      /*lint !e578 local variable same name as global is OK here. */
   SELF_TestData_t   *SelfTestData;
   uint16_t          initFailCount;    /* Failure count at start of test pass    */

   SelfTestData = SELF_GetTestFileHandle()->Data;
   if ( argc == 2 )
   {
      LoopCount = strtoul( argv[1], &endptr, 0 );
   }
   while ( LoopCount )
   {
      initFailCount = SelfTestData->nvFail;
      OS_EVNT_Set ( SELF_notify, ( uint32_t )( 1 << ( uint16_t )e_nvFail ) ); /* Request the test  */
      event_flags = OS_EVNT_Wait ( &MFG_notify, ( uint32_t )( 1 << ( uint16_t )e_nvFail ), ( bool )false, ONE_SEC * 20 );
      if ( event_flags != 0 )    /* Did the test complete?  */
      {
         if ( initFailCount != SelfTestData->nvFail ) /* Did the test pass?   */
         {
            initFailCount = SelfTestData->nvFail;
            errCount++;
         }
         LoopCount--;
      }
   }

   if( errCount != 0 )  /* Need to update the failure counter? */
   {
      ( void )SELF_UpdateTestResults();
   }
   MFG_logPrintf( "%s %d\n", argv[0], SelfTestData->nvFail );
}

/***********************************************************************************************************************
   Function Name: MFGP_nvFailCount

   Purpose: Read/Write the nv test fail counter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_nvFailCount( uint32_t argc, char *argv[] )
{
   uint16_t errCount;         /* Number of failures reported by DVR_EFL_unitTest */
   char *endptr;
   SELF_file_t *pSELF_test;

   pSELF_test = SELF_GetTestFileHandle();
   ( void )FIO_fread( &pSELF_test->handle, ( uint8_t * )pSELF_test->Data, 0, pSELF_test->Size );
   if ( argc == 2 )           /* User wants to set the counter */
   {
      errCount = ( uint16_t )strtoul( argv[1], &endptr, 10 );
      pSELF_test->Data->nvFail = errCount;
      ( void )SELF_UpdateTestResults();
   }
   /* Always print the counter value   */
   ( void )FIO_fread( &pSELF_test->handle, ( uint8_t * )pSELF_test->Data, 0, pSELF_test->Size );
   MFG_logPrintf( "%s %d\n", argv[0], pSELF_test->Data->nvFail );
}

/***********************************************************************************************************************
   Function Name: MFGP_SpuriousResetCount

   Purpose: Read/Write the Spurious Reset counter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_SpuriousResetCount( uint32_t argc, char *argv[] )
{
   uint16_t SpuriousResetCount;  /* Number of Spurios Resets recorded or update value  */
   char *endptr;

   if ( argc == 2 )           /* User wants to set the counter */
   {
      SpuriousResetCount = ( uint16_t )strtoul( argv[1], &endptr, 10 );
      PWR_setSpuriousResetCnt( SpuriousResetCount );
   }
   /* Read and display the failure counter. */
   SpuriousResetCount =  ( uint16_t )PWR_getSpuriousResetCnt();
   MFG_logPrintf( "%s %d\n", argv[0], SpuriousResetCount );
}

#if ( EP == 1 )
/***********************************************************************************************************************
Function Name: MFGP_dfwApplyConfirmTimeDiversity

Purpose: Set or Print the DFW Apply Confirm Time Diversity

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_dfwApplyConfirmTimeDiversity( uint32_t argc, char *argv[] )
{
   uint8_t uDfwApplyConfirmTimeDiversity;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write dsBuMaxTimeDiversity
         uDfwApplyConfirmTimeDiversity = ( uint8_t )atol( argv[1] );
         ( void )DFWTDCFG_set_apply_confirm( uDfwApplyConfirmTimeDiversity );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], DFWTDCFG_get_apply_confirm() );
}

/***********************************************************************************************************************
Function Name: MFGP_dfwDownloadConfirmTimeDiversity

Purpose: Set or Print the DFW Download Confirm Time Diversity

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_dfwDownloadConfirmTimeDiversity( uint32_t argc, char *argv[] )
{
   uint8_t uDfwDownloadConfirmTimeDiversity;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write dsBuMaxTimeDiversity
         uDfwDownloadConfirmTimeDiversity = ( uint8_t )atol( argv[1] );
         ( void )DFWTDCFG_set_download_confirm( uDfwDownloadConfirmTimeDiversity );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], DFWTDCFG_get_download_confirm() );
}

#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
/***********************************************************************************************************************
   Function Name: MFGP_lpBuSchedule

   Purpose: Set or Print the LP Bubble Up Schedule

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_lpBuSchedule( uint32_t argc, char *argv[] )
{
   uint16_t uLpBuSchedule;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write lpBuSchedule
         uLpBuSchedule = ( uint16_t )atol( argv[1] );
         ( void )ID_setLpBuSchedule( uLpBuSchedule );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], ID_getLpBuSchedule() );
}

/***********************************************************************************************************************
   Function Name: MFGP_lpBuTrafficClass

   Purpose: Set or Print the LP Bubble Up Traffic Class

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_lpBuTrafficClass( uint32_t argc, char *argv[] )
{
   uint8_t uLpBuTrafficClass;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write lpBuTrafficClass
         uLpBuTrafficClass = ( uint8_t )atoi( argv[1] );
         ( void )ID_setQos( uLpBuTrafficClass );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], ID_getQos() );
}

/***********************************************************************************************************************
Function Name: MFGP_lpBuDataRedundancy

Purpose: Set or Print the LP Bubble Data Redundancy

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_lpBuDataRedundancy( uint32_t argc, char *argv[] )
{
   uint8_t uLpBuDataRedundancy;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write lpBuDataRedundancy
         uLpBuDataRedundancy = ( uint8_t )atoi( argv[1] );
         ( void )ID_setLpBuDataRedundancy( uLpBuDataRedundancy );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[0], ID_getLpBuDataRedundancy() );
}

/***********************************************************************************************************************
Function Name: MFGP_lpBuMaxTimeDiversity

Purpose: Set or Print the LP Bubble Max Time Diversity

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_lpBuMaxTimeDiversity( uint32_t argc, char *argv[] )
{
   uint8_t uLpBuMaxTimeDiversity;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write lpBuMaxTimeDiversity
         uLpBuMaxTimeDiversity = ( uint8_t )atoi( argv[1] );
         ( void )ID_setLpBuMaxTimeDiversity( uLpBuMaxTimeDiversity );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], ID_getLpBuMaxTimeDiversity() );
}

/***********************************************************************************************************************
Function Name: MFGP_lpBuChannel

Purpose: Set or Print the LP Bubble Channel (1-6)

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
#if ( LP_IN_METER == 0 )
static void MFGP_lpBuChannel( uint32_t argc, char *argv[] )
{
   idChCfg_t chCfg;
   char cIndexChar;
   int8_t nIndex;

   cIndexChar = argv[0][11];
   //TODO: Need to modify next statement to account for channel larger than a single digit
   nIndex = ( int8_t ) ( cIndexChar - '1' );

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uint32_t scanValues[5];

         if ( 5 == sscanf(  argv[1],
                            "%lu,%lu,%lu,%lu,%lu",
                            &scanValues[0],
                            &scanValues[1],
                            &scanValues[2],
                            &scanValues[3],
                            &scanValues[4] ) )
         {
            (void)memset( (uint8_t *)&chCfg, 0, sizeof(chCfg) );
            chCfg.rdgTypeID = ( uint16_t )scanValues[0];
            chCfg.sampleRate_mS = ( uint32_t )scanValues[1] * TIME_TICKS_PER_MIN;
            chCfg.mode = ( mode_e )scanValues[2];
            chCfg.trimDigits = ( uint8_t )scanValues[3];
            chCfg.storageLen = ( uint8_t )scanValues[4];
            (void)ID_cfgIntervalData( ( uint8_t )nIndex, &chCfg );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
   ( void )ID_rdIntervalDataCfg( &chCfg, ( uint8_t )nIndex );

   MFG_printf( "%s %u,%u,%u,%u,%u\n",
               argv[0],
               chCfg.rdgTypeID,
               chCfg.sampleRate_mS / TIME_TICKS_PER_MIN,
               chCfg.mode,
               chCfg.trimDigits,
               chCfg.storageLen );

}
#endif // endif of (LP_IN_METER == 0)
#endif // endif of (ACLARA_LC == 0)
#endif

/***********************************************************************************************************************
Function Name: MFGP_eventThreshold

Purpose: Set or Print the opportunistic or realtime Threshold

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_eventThreshold( uint32_t argc, char *argv[] )
{
   uint8_t bOpportunisticThreshold = false;
   uint8_t uOpportunisticThreshold;
   uint8_t uRealtimeThreshold;

   /* Which command was invoked? */
   if ( 0 == strcasecmp( argv[0], "opportunisticThreshold" ) )
   {
      bOpportunisticThreshold = true;
   }

   if ( argc <= 2 )  /* Valid parameter count?  */
   {
      EVL_GetThresholds( &uRealtimeThreshold, &uOpportunisticThreshold );  /* Get current values   */

      if ( argc == 2 )
      {
         if ( true == bOpportunisticThreshold )
         {
            uOpportunisticThreshold = ( uint8_t )atoi( argv[1] ); /* User wants to set opportunistic  */
         }
         else
         {
            uRealtimeThreshold = ( uint8_t )atoi( argv[1] );   /* User wants to set realtime  */
         }

         /* Update NV   */
         ( void )EVL_SetThresholds( uRealtimeThreshold, uOpportunisticThreshold );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "%s : Invalid number of parameters", argv[0] );
   }

   /* Always print out the value from NV */
   EVL_GetThresholds( &uRealtimeThreshold, &uOpportunisticThreshold );
   MFG_printf( "%s %u\n", argv[0], bOpportunisticThreshold ? uOpportunisticThreshold : uRealtimeThreshold );
}

#if ( EP == 1 )
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
/***********************************************************************************************************************
Function Name: MFGP_dailySelfReadTime

Purpose: Set or Print the Daily Self Read Time

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_dailySelfReadTime( uint32_t argc, char *argv[] )
{
   uint32_t uDailySelfReadTime;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write dailySelfReadTime
         uDailySelfReadTime = ( uint32_t )atol( argv[1] );
         ( void )HD_setDailySelfReadTime( uDailySelfReadTime );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print the read back value */
   MFG_printf( "%s %u\n", argv[ 0 ], HD_getDailySelfReadTime() );
}

/***********************************************************************************************************************
Function Name: MFGP_dsBuReadingTypes

Purpose: Set or Print the DS Bubble Up Reading Types

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_dsBuReadingTypes( uint32_t argc, char *argv[] )
{
   uint16_t uDsBuReadingType[HD_TOTAL_CHANNELS] = {0};

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         if ( HD_TOTAL_CHANNELS == sscanf( argv[1],
                           "%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,"
                           "%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,"
                           "%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,"
                           "%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu",
                           &uDsBuReadingType[0],
                           &uDsBuReadingType[1],
                           &uDsBuReadingType[2],
                           &uDsBuReadingType[3],
                           &uDsBuReadingType[4],
                           &uDsBuReadingType[5],
                           &uDsBuReadingType[6],
                           &uDsBuReadingType[7],
                           &uDsBuReadingType[8],
                           &uDsBuReadingType[9],
                           &uDsBuReadingType[10],
                           &uDsBuReadingType[11],
                           &uDsBuReadingType[12],
                           &uDsBuReadingType[13],
                           &uDsBuReadingType[14],
                           &uDsBuReadingType[15],
                           &uDsBuReadingType[16],
                           &uDsBuReadingType[17],
                           &uDsBuReadingType[18],
                           &uDsBuReadingType[19],
                           &uDsBuReadingType[20],
                           &uDsBuReadingType[21],
                           &uDsBuReadingType[22],
                           &uDsBuReadingType[23],
                           &uDsBuReadingType[24],
                           &uDsBuReadingType[25],
                           &uDsBuReadingType[26],
                           &uDsBuReadingType[27],
                           &uDsBuReadingType[28],
                           &uDsBuReadingType[29],
                           &uDsBuReadingType[30],
                           &uDsBuReadingType[31],
                           &uDsBuReadingType[32],
                           &uDsBuReadingType[33],
                           &uDsBuReadingType[34],
                           &uDsBuReadingType[35],
                           &uDsBuReadingType[36],
                           &uDsBuReadingType[37],
                           &uDsBuReadingType[38],
                           &uDsBuReadingType[39],
                           &uDsBuReadingType[40],
                           &uDsBuReadingType[41],
                           &uDsBuReadingType[42],
                           &uDsBuReadingType[43],
                           &uDsBuReadingType[44],
                           &uDsBuReadingType[45],
                           &uDsBuReadingType[46],
                           &uDsBuReadingType[47],
                           &uDsBuReadingType[48],
                           &uDsBuReadingType[49],
                           &uDsBuReadingType[50],
                           &uDsBuReadingType[51],
                           &uDsBuReadingType[52],
                           &uDsBuReadingType[53],
                           &uDsBuReadingType[54],
                           &uDsBuReadingType[55],
                           &uDsBuReadingType[56],
                           &uDsBuReadingType[57],
                           &uDsBuReadingType[58],
                           &uDsBuReadingType[59],
                           &uDsBuReadingType[60],
                           &uDsBuReadingType[61],
                           &uDsBuReadingType[62],
                           &uDsBuReadingType[63] ) )
         {
            HD_setDsBuReadingTypes( uDsBuReadingType );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   // Now read and print out the current values.
   HD_getDsBuReadingTypes( uDsBuReadingType );
   MFG_printf( "%s %u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
              "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
               argv[0],
               uDsBuReadingType[0],
               uDsBuReadingType[1],
               uDsBuReadingType[2],
               uDsBuReadingType[3],
               uDsBuReadingType[4],
               uDsBuReadingType[5],
               uDsBuReadingType[6],
               uDsBuReadingType[7],
               uDsBuReadingType[8],
               uDsBuReadingType[9],
               uDsBuReadingType[10],
               uDsBuReadingType[11],
               uDsBuReadingType[12],
               uDsBuReadingType[13],
               uDsBuReadingType[14],
               uDsBuReadingType[15],
               uDsBuReadingType[16],
               uDsBuReadingType[17],
               uDsBuReadingType[18],
               uDsBuReadingType[19],
               uDsBuReadingType[20],
               uDsBuReadingType[21],
               uDsBuReadingType[22],
               uDsBuReadingType[23],
               uDsBuReadingType[24],
               uDsBuReadingType[25],
               uDsBuReadingType[26],
               uDsBuReadingType[27],
               uDsBuReadingType[28],
               uDsBuReadingType[29],
               uDsBuReadingType[30],
               uDsBuReadingType[31],
               uDsBuReadingType[32],
               uDsBuReadingType[33],
               uDsBuReadingType[34],
               uDsBuReadingType[35],
               uDsBuReadingType[36],
               uDsBuReadingType[37],
               uDsBuReadingType[38],
               uDsBuReadingType[39],
               uDsBuReadingType[40],
               uDsBuReadingType[41],
               uDsBuReadingType[42],
               uDsBuReadingType[43],
               uDsBuReadingType[44],
               uDsBuReadingType[45],
               uDsBuReadingType[46],
               uDsBuReadingType[47],
               uDsBuReadingType[48],
               uDsBuReadingType[49],
               uDsBuReadingType[50],
               uDsBuReadingType[51],
               uDsBuReadingType[52],
               uDsBuReadingType[53],
               uDsBuReadingType[54],
               uDsBuReadingType[55],
               uDsBuReadingType[56],
               uDsBuReadingType[57],
               uDsBuReadingType[58],
               uDsBuReadingType[59],
               uDsBuReadingType[60],
               uDsBuReadingType[61],
               uDsBuReadingType[62],
               uDsBuReadingType[63] );
}

/***********************************************************************************************************************
Function Name: MFGP_dsBuTrafficClass

Purpose: Set or Print the DS Bubble Up Traffic Class

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_dsBuTrafficClass( uint32_t argc, char *argv[] )
{
   uint8_t uDsBuTrafficClass;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write dsBuTrafficClass
         uDsBuTrafficClass = ( uint8_t )atoi( argv[1] );
         ( void )HD_setDsBuTrafficClass( uDsBuTrafficClass );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], HD_getDsBuTrafficClass() );
}

/***********************************************************************************************************************
Function Name: MFGP_orReadList

Purpose: Set or Print the On-demand Read Reading Types

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_orReadList( uint32_t argc, char *argv[] )
{
   uint16_t uReadingType[OR_MR_MAX_READINGS] = {0};

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         if ( 32 == sscanf( argv[1],
                           "%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu",
                           &uReadingType[0],
                           &uReadingType[1],
                           &uReadingType[2],
                           &uReadingType[3],
                           &uReadingType[4],
                           &uReadingType[5],
                           &uReadingType[6],
                           &uReadingType[7],
                           &uReadingType[8],
                           &uReadingType[9],
                           &uReadingType[10],
                           &uReadingType[11],
                           &uReadingType[12],
                           &uReadingType[13],
                           &uReadingType[14],
                           &uReadingType[15],
                           &uReadingType[16],
                           &uReadingType[17],
                           &uReadingType[18],
                           &uReadingType[19],
                           &uReadingType[20],
                           &uReadingType[21],
                           &uReadingType[22],
                           &uReadingType[23],
                           &uReadingType[24],
                           &uReadingType[25],
                           &uReadingType[26],
                           &uReadingType[27],
                           &uReadingType[28],
                           &uReadingType[29],
                           &uReadingType[30],
                           &uReadingType[31] ) )
         {
            OR_MR_setReadingTypes( uReadingType );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   OR_MR_getReadingTypes( uReadingType );

   MFG_printf( "%s %u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
              argv[0],
              uReadingType[0],
              uReadingType[1],
              uReadingType[2],
              uReadingType[3],
              uReadingType[4],
              uReadingType[5],
              uReadingType[6],
              uReadingType[7],
              uReadingType[8],
              uReadingType[9],
              uReadingType[10],
              uReadingType[11],
              uReadingType[12],
              uReadingType[13],
              uReadingType[14],
              uReadingType[15],
              uReadingType[16],
              uReadingType[17],
              uReadingType[18],
              uReadingType[19],
              uReadingType[20],
              uReadingType[21],
              uReadingType[22],
              uReadingType[23],
              uReadingType[24],
              uReadingType[25],
              uReadingType[26],
              uReadingType[27],
              uReadingType[28],
              uReadingType[29],
              uReadingType[30],
              uReadingType[31] );

}

#if ( ENABLE_DEMAND_TASKS == 1 )
/***********************************************************************************************************************
Function Name: MFGP_drReadList

Purpose: Set or Print the Demand Reading Types

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_drReadList( uint32_t argc, char *argv[] )
{
   uint16_t uReadingType[DMD_MAX_READINGS] = {0};

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         if ( 32 == sscanf( argv[1],
                           "%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu",
                           &uReadingType[0],
                           &uReadingType[1],
                           &uReadingType[2],
                           &uReadingType[3],
                           &uReadingType[4],
                           &uReadingType[5],
                           &uReadingType[6],
                           &uReadingType[7],
                           &uReadingType[8],
                           &uReadingType[9],
                           &uReadingType[10],
                           &uReadingType[11],
                           &uReadingType[12],
                           &uReadingType[13],
                           &uReadingType[14],
                           &uReadingType[15],
                           &uReadingType[16],
                           &uReadingType[17],
                           &uReadingType[18],
                           &uReadingType[19],
                           &uReadingType[20],
                           &uReadingType[21],
                           &uReadingType[22],
                           &uReadingType[23],
                           &uReadingType[24],
                           &uReadingType[25],
                           &uReadingType[26],
                           &uReadingType[27],
                           &uReadingType[28],
                           &uReadingType[29],
                           &uReadingType[30],
                           &uReadingType[31] ) )
         {
            DEMAND_setReadingTypes( uReadingType );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   DEMAND_getReadingTypes( uReadingType );
   MFG_printf( "%s %u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
               argv[0],
               uReadingType[0],
               uReadingType[1],
               uReadingType[2],
               uReadingType[3],
               uReadingType[4],
               uReadingType[5],
               uReadingType[6],
               uReadingType[7],
               uReadingType[8],
               uReadingType[9],
               uReadingType[10],
               uReadingType[11],
               uReadingType[12],
               uReadingType[13],
               uReadingType[14],
               uReadingType[15],
               uReadingType[16],
               uReadingType[17],
               uReadingType[18],
               uReadingType[19],
               uReadingType[20],
               uReadingType[21],
               uReadingType[22],
               uReadingType[23],
               uReadingType[24],
               uReadingType[25],
               uReadingType[26],
               uReadingType[27],
               uReadingType[28],
               uReadingType[29],
               uReadingType[30],
               uReadingType[31] );
}
#endif // ( ENABLE_DEMAND_TASKS == 1 )

/***********************************************************************************************************************
Function Name: MFGP_dsBuDataRedundancy

Purpose: Set or Print the DS Bubble Data Redundancy

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_dsBuDataRedundancy( uint32_t argc, char *argv[] )
{
   uint8_t uDsBuDataRedundancy;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write dsBuDataRedundancy
         uDsBuDataRedundancy = ( uint8_t )atoi( argv[1] );
         ( void )HD_setDsBuDataRedundancy( uDsBuDataRedundancy );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[0], HD_getDsBuDataRedundancy() );
}

/***********************************************************************************************************************
Function Name: MFGP_dsBuMaxTimeDiversity

Purpose: Set or Print the DS Bubble Max Time Diversity

Arguments:
   argc - Number of Arguments passed to this function
   argv - pointer to the list of arguments passed to this function

Returns: void
***********************************************************************************************************************/
static void MFGP_dsBuMaxTimeDiversity( uint32_t argc, char *argv[] )
{
   uint8_t uDsBuMaxTimeDiversity;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write dsBuMaxTimeDiversity
         uDsBuMaxTimeDiversity = ( uint8_t )atoi( argv[1] );
         ( void )HD_setDsBuMaxTimeDiversity( uDsBuMaxTimeDiversity );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], HD_getDsBuMaxTimeDiversity() );
}
#endif // #if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )

#if ( ENABLE_DEMAND_TASKS == 1 )
/***********************************************************************************************************************
Function Name: MFGP_demandFutureConfiguration

   Purpose: Set or Print the Demand Future Configuration

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_demandFutureConfiguration( uint32_t argc, char *argv[] )
{
#if ( DEMAND_IN_METER == 1 )
   if ( argc != 1 )
#else
   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uint8_t demand;

         // Write Future Demand Configuration
         demand = ( uint8_t )atol( argv[1] );
         ( void )DEMAND_SetFutureConfig( demand );
      }
   }
   else
#endif
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], DEMAND_GetFutureConfig() );
}

/***********************************************************************************************************************
Function Name: MFGP_demandPresentConfiguration

   Purpose: Set or Print the Demand Present Configuration

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_demandPresentConfiguration( uint32_t argc, char *argv[] )
{
#if ( DEMAND_IN_METER == 1 )
   if ( argc != 1 )
#else
   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uint8_t demand;

         // Write Present Demand Configuration
         demand = ( uint8_t )atol( argv[1] );
         ( void )DEMAND_SetConfig( demand );
      }
   }
   else
#endif
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], DEMAND_GetConfig() );
}

/***********************************************************************************************************************
Function Name: MFGP_scheduledDemandResetDay

   Purpose: Set or Print the Scheduled Demand Reset Day

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
          Notes: SCHD DMD RESET DEPRICATED
**********************************************************************************************************************/
#if ( DEMAND_IN_METER == 0 )
static void MFGP_scheduledDemandResetDay( uint32_t argc, char *argv[] )
{
   uint8_t dateOfMonth;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write ScheduledDemandResetDay
         dateOfMonth = ( uint8_t )atol( argv[1] );
         ( void )DEMAND_SetResetDay( dateOfMonth );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   //Always print read back value
   DEMAND_GetResetDay( &dateOfMonth );
   MFG_printf( "%s %u\n", argv[ 0 ], dateOfMonth );
}
#endif
#endif // #if ( ENABLE_DEMAND_TASKS == 1 )

/***********************************************************************************************************************
Function Name: MFGP_outageDeclarationDelay

   Purpose: Set or Print the Outage Declaration Delay

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_outageDeclarationDelay( uint32_t argc, char *argv[] )
{
   uint16_t delay;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write outageDeclarationDelay
         delay = ( uint16_t )atol( argv[1] );
         ( void )PWRCFG_set_outage_delay_seconds( delay );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], PWRCFG_get_outage_delay_seconds() );
}

/***********************************************************************************************************************
Function Name: MFGP_restorationDelay

   Purpose: Set or Print the Restoration Delay

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_restorationDelay( uint32_t argc, char *argv[] )
{
   uint16_t delay;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write restorationDelay
         delay = ( uint16_t )atol( argv[1] );
         ( void )PWRCFG_set_restoration_delay_seconds( delay );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], PWRCFG_get_restoration_delay_seconds() );
}

/***********************************************************************************************************************
Function Name: MFGP_powerQualityEventDuration

   Purpose: Set or Print the Power Quality Event Duration

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_powerQualityEventDuration( uint32_t argc, char *argv[] )
{
   uint16_t delay;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write Power Quality Event Duration
         delay = ( uint16_t )atol( argv[1] );
         ( void )PWRCFG_set_PowerQualityEventDuration( delay );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], PWRCFG_get_PowerQualityEventDuration() );
}

/***********************************************************************************************************************
Function Name: MFGP_lastGaspMaxNumAttempts

   Purpose: Set or Print the Maximum number of last gasps attempts

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_lastGaspMaxNumAttempts( uint32_t argc, char *argv[] )
{
   uint8_t  uMaxAttempts;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write max number of last gasp attempts
         uMaxAttempts = ( uint8_t )atol( argv[1] );
         ( void )PWRCFG_set_LastGaspMaxNumAttempts( uMaxAttempts );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], PWRCFG_get_LastGaspMaxNumAttempts() );
}

#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
/***********************************************************************************************************************
Function Name: MFGP_simulateLastGaspStart

   Purpose: Set or Print the LG Simulation time in seconds

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_simulateLastGaspStart( uint32_t argc, char *argv[] )
{
   uint32_t  SimulateLastGaspStart;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write the LG Simulation start time value
         SimulateLastGaspStart = ( uint32_t )atol( argv[1] );
         ( void )PWRCFG_set_SimulateLastGaspStart( SimulateLastGaspStart );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], PWRCFG_get_SimulateLastGaspStart() );
}

/***********************************************************************************************************************
Function Name: MFGP_simulateLastGaspDuration

   Purpose: Set or Print the duration of the LG Simulation in secods

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_simulateLastGaspDuration( uint32_t argc, char *argv[] )
{
   uint16_t  SimulateLastGaspDuration;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write the LG Simulation duration value
         SimulateLastGaspDuration = ( uint16_t )atol( argv[1] );
         ( void )PWRCFG_set_SimulateLastGaspDuration( SimulateLastGaspDuration );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], PWRCFG_get_SimulateLastGaspDuration() );
}
/***********************************************************************************************************************
Function Name: MFGP_SimulateLastGaspTraffic

   Purpose: Set or Print the traffic allow/block status

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_SimulateLastGaspTraffic( uint32_t argc, char *argv[] )
{
   uint8_t  uSimulateLastGaspTraffic;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write the SimulateLastGaspTraffic status
         uSimulateLastGaspTraffic = ( uint8_t )atol( argv[1] );
         if ( eFAILURE == PWRCFG_set_SimulateLastGaspTraffic( uSimulateLastGaspTraffic ) )
         {
            MFG_printf( "No change, LG Sim in progress (or) write failed\n" );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], PWRCFG_get_SimulateLastGaspTraffic() );
}

/***********************************************************************************************************************
Function Name: MFGP_simulateLastGaspMaxNumAttempts

   Purpose: Set or Print the Maximum number of last gasps attempts for LG Simulation

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_simulateLastGaspMaxNumAttempts( uint32_t argc, char *argv[] )
{
   uint8_t  uMaxAttempts;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write max number of last gasp attempts for the LG Simulation
         uMaxAttempts = ( uint8_t )atol( argv[1] );
         ( void )PWRCFG_set_SimulateLastGaspMaxNumAttempts( uMaxAttempts );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], PWRCFG_get_SimulateLastGaspMaxNumAttempts() );
}

/***********************************************************************************************************************
   Function Name: MFGP_simulateLastGaspStatCcaAttempts

   Purpose: Get the CCA attempts statistics.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_simulateLastGaspStatCcaAttempts(uint32_t argc, char *argv[])
{
   SimStats_t *getLGSimStats;
   uint8_t i;

   MFG_printf( "%s ", argv[ 0 ]);

   getLGSimStats = PWRCFG_get_SimulateLastGaspStats();

   for( i = 0; i < 6; i++ )
   {
      MFG_printf( "%d", getLGSimStats[i].CCA_Attempts);

      if( i < 6 - 1 )
      {
         MFG_printf( "," );
      }
   }

   MFG_printf( "\n" );
}

/***********************************************************************************************************************
   Function Name: MFGP_simulateLastGaspStatPPersistAttempts

   Purpose: Get the P-Persist statistics for LG Simulation.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_simulateLastGaspStatPPersistAttempts(uint32_t argc, char *argv[])
{
   SimStats_t *getLGSimStats;
   uint8_t i;

   MFG_printf( "%s ", argv[ 0 ]);

   getLGSimStats = PWRCFG_get_SimulateLastGaspStats();

   for( i = 0; i < 6; i++ )
   {
      MFG_printf( "%d", getLGSimStats[i].pPersistAttempts);

      if( i < 6 - 1 )
      {
         MFG_printf( "," );
      }
   }

   MFG_printf( "\n" );
}

/***********************************************************************************************************************
   Function Name: MFGP_simulateLastGaspStatMsgsSent

   Purpose: Get the message sent or not status for the LG Simulation.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_simulateLastGaspStatMsgsSent(uint32_t argc, char *argv[])
{
   SimStats_t *getLGSimStats;
   uint8_t i;

   MFG_printf( "%s ", argv[ 0 ]);

   getLGSimStats = PWRCFG_get_SimulateLastGaspStats();

   for( i = 0; i < 6; i++ )
   {
      MFG_printf( "%hhu", (uint8_t)getLGSimStats[i].MessageSent);

      if( i < 6 - 1 )
      {
         MFG_printf( "," );
      }
   }

   MFG_printf( "\n" );
}
#endif
#endif

/***********************************************************************************************************************
Function Name: MFGP_shipMode

   Purpose: Set or Print the Outage Declaration Delay

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_shipMode( uint32_t argc, char *argv[] )
{
   uint8_t locShipMode = 0;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         locShipMode = ( uint8_t )atol( argv[1] );
         if ( locShipMode != 0 )
         {
            ( void )SM_StartRequest( eSM_START_LOW_POWER, NULL );
         }
         else
         {
            ( void )SM_StartRequest( eSM_START_STANDARD, NULL );
         }
         ( void )MODECFG_set_ship_mode( locShipMode );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Read and display ship mode */
   MFG_printf( "%s %u\n", argv[0], MODECFG_get_ship_mode() );
}

/***********************************************************************************************************************
Function Name: MFGP_rtcDateTime

   Purpose: Set or Print the RTC time

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_rtcDateTime( uint32_t argc, char *argv[] )
{

   sysTime_t sysTime;
   sysTime_dateFormat_t rtcTime;
   uint32_t fracSec, uRtcDateTime;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uRtcDateTime = ( uint32_t )atol( argv[1] );
         TIME_UTIL_ConvertSecondsToSysFormat(uRtcDateTime, (uint32_t) 0, &sysTime);
         (void)TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &rtcTime);  //convert sys time to RTC time format
         (void)RTC_SetDateTime (&rtcTime);
      }
   }

   RTC_GetDateTime ( &rtcTime );
   (void)TIME_UTIL_ConvertDateFormatToSysFormat(&rtcTime, &sysTime);
   TIME_UTIL_ConvertSysFormatToSeconds(&sysTime, &uRtcDateTime, &fracSec);
   MFG_printf( "%s %u\n", argv[0], uRtcDateTime );
}


#if ( EP == 1 )
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
/***********************************************************************************************************************
Function Name: MFGP_meterSessionFailurecount

   Purpose: Set or Print the Meter Shop Mode

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_meterSessionFailureCount( uint32_t argc, char *argv[] )
{
   uint16_t uMeterSessionFailureCount;

   if (argc <= 2 )
   {
      if (argc == 2 )
      {
         uMeterSessionFailureCount = ( uint8_t )atol( argv[1] );
         (void)HMC_ENG_setMeterSessionFailureCount( uMeterSessionFailureCount );
      }
   }

   /* Read and display Meter Shop mode */
   (void)HMC_ENG_getMeterSessionFailureCount( &uMeterSessionFailureCount );
   MFG_printf( "%s %u\n", argv[0], uMeterSessionFailureCount );

}
#endif

/***********************************************************************************************************************
Function Name: MFGP_meterShopMode

   Purpose: Set or Print the Meter Shop Mode

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_meterShopMode( uint32_t argc, char *argv[] )
{
   uint8_t meterShopMode = 0;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         meterShopMode = ( uint8_t )atol( argv[1] );

         /* Write Meter Shop Mode */
         ( void )MODECFG_set_decommission_mode( meterShopMode );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Read and display Meter Shop mode */
   MFG_printf( "%s %u\n", argv[0], MODECFG_get_decommission_mode() );
}
#endif

/***********************************************************************************************************************
Function Name: MFGP_FlashSecurity

   Purpose: Report status of Flash/JTAG security or change them Lock/Unlock Flash and JTAG Security.
            A second argument requests the security settings to be updated:
               1 or "true"    - secure the code flash and lock the JTAG interface
               0 or "false"   - unsecure the code flash and UNlock the JTAG interface
   NOTE: An update results in a system reset and this function never returns.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_FlashSecurity( uint32_t argc, char *argv[] )
{
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
   PartitionData_t const   *pImagePTbl;            // Image partition for PGM memory
   dSize                   destAddr = 0x408u;      // Destination address of the data to move
   uint8_t                 fprot[ 5 ];             // Values read
   uint8_t                 Lock = false;           // Use chose "lock" option
   uint8_t                 Valid = false;          // Arguments valid indicator
   bool                    written = (bool)false;  // Partition written flag
   returnStatus_t          retVal = eFAILURE;

   //                                       FPROT3 FPROT2 FPROT1 FPROT0 FSEC
   static const uint8_t    LockMask[5] =   {0x00u, 0x80u, 0xFFu, 0xFFu, 0xFFu};
   static const uint8_t    UnlockMask[5] = {0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFEu};

   if ( argc <= 2 )
   {
      // Open the alternate program partition for writing.
      if ( eSUCCESS == PAR_partitionFptr.parOpen( &pImagePTbl, ePART_DFW_PGM_IMAGE, 0L ) )
      {
         (void)PAR_partitionFptr.parRead( fprot, destAddr, ( lCnt ) sizeof( fprot ), pImagePTbl );
         if ( 2 == argc )
         {
            if ( ( 0 == strcasecmp( argv[1], "true" ) ) || ( 0 == strcmp( argv[1], "1" ) ) )
            {
               Lock = true;     // Lock desired
               Valid = true;    // Arguments are valid
            }
            else if ( ( 0 == strcasecmp( argv[1], "false" ) ) || ( 0 == strcmp( argv[1], "0" ) ) )
            {
               Valid = true;    // Arguments valid; default state is "unlock"
            }
            else
            {
               MFG_printf( "Invalid argument\n" );
            }

            if ( Valid )  // Valid arguments?
            {
               // Write the appropriate pattern to lock or unlock the flash and JTAG, if actually changing.
               // Dev NOTE: This is directly reading Internal Flash, where above is using the partiton read - Why use different methods?
               if (  (  Lock && memcmp( (uint8_t *)destAddr,   LockMask, sizeof( fprot ) ) != 0 )   || // Locking
                     ( !Lock && memcmp( (uint8_t *)destAddr, UnlockMask, sizeof( fprot ) ) != 0 ) )    // Unlocking
               {
                  // Copy the running program image to the alternate partition.
                  retVal = DFWA_CopyPgmToPgmImagePartition(true, eFWT_APP);

                  if ( eSUCCESS == retVal )
                  {
                     retVal = PAR_partitionFptr.parWrite( destAddr, Lock ? LockMask : UnlockMask,
                                                         ( lCnt ) sizeof( LockMask ), pImagePTbl );
                     written = (bool)true;
                  }

                  if ( written ) // Only if the device write was attempted
                  {
                     if ( eSUCCESS == retVal  ) // And the write was successful
                     {
                        // Swap the program flash partitions and reset.
                        (void)DFWA_WaitForSysIdle( 3000 );   //No need to unlock when done since resetting
                        MFG_printf( "Swapping Flash.\n" );
                        ( void )DFWA_SetSwapState( eFS_COMPLETE );
                        MFG_printf( "Resetting Processor.\n" );

                        /* Keep all other tasks from running.
                           Increase the priority of the power and idle tasks  */
                        ( void )OS_TASK_Set_Priority( pTskName_Pwr, 10 );
                        ( void )OS_TASK_Set_Priority( pTskName_MfgUartCmd, 11 );
                        ( void )OS_TASK_Set_Priority( pTskName_Idle, 12 );
                        //Does not return from here
                        PWR_SafeReset();
                     }
                     else
                     {
                        MFG_printf( "Flash Partition Write FAILED\n" );
                     }
                  }
               }
#if 0 /* Debug only  */
               else
               {
                  MFG_printf( "No change required.\n" );
               }
#endif
            }
         }  /* End of request to update settings   */

#if 0    /* Debug code only   */
         MFG_printf( "fprot: " );
         for ( uint8_t i = 0; i < ARRAY_IDX_CNT( fprot ); i++ )
         {
            MFG_printf( "0x%02x, ", fprot [ i ] );
         }
         MFG_printf( "\n" );
#endif
      }
      else
      {
         MFG_printf( "Flash Partition Open FAILED\n" );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* At end of process, report the results as 1 or 0, based on the registers matching the Lock or Unlock mask.   */
   (void)memcpy( fprot, (uint8_t *)destAddr, sizeof( fprot ) );
   MFG_printf( "%s %d\n", argv[0], (  memcmp( fprot, LockMask, sizeof( fprot ) ) == 0 ? 1 : 0 ) );
#elif ( MCU_SELECTED == NXP_K24 ) //K24 target
   PartitionData_t const   *pImagePTbl;            // Image partition for PGM memory
   dSize                   destAddr = 0x40Cu;      // Destination address of the FSEC field
   uint8_t                 fprot;                  // Values read
   uint8_t                 Lock     = false;       // Use chose "lock" option
   uint8_t                 Valid    = false;       // Arguments valid indicator
   returnStatus_t          retVal   = eFAILURE;

   //Bit fields in FSEC:
   // BackDoorEkyEn   7:6(En=b'10)
   // MassEraseEn     5:4(En=!b'10)
   // NXPAccessEn     3:2(En=!b'10)
   // FlashSecurityEn 1:0(En=!b'10)
   // 11 11 11 10
   uint8_t flashSecEnabled  = (uint8_t)0xFF;
   uint8_t flashSecDisabled = (uint8_t)0xFE;

   if ( argc <= 2 )
   {
      // Open the alternate program partition for writing.
      if ( eSUCCESS == PAR_partitionFptr.parOpen( &pImagePTbl, ePART_BL_CODE, 0L ) )
      {
         (void)PAR_partitionFptr.parRead( &fprot, destAddr, ( lCnt ) sizeof( fprot ), pImagePTbl );
         if ( 2 == argc )
         {
            if ( ( 0 == strcasecmp( argv[1], "true" ) ) || ( 0 == strcmp( argv[1], "1" ) ) )
            {
               Lock = true;     // Lock desired
               Valid = true;    // Arguments are valid
            }
            else if ( ( 0 == strcasecmp( argv[1], "false" ) ) || ( 0 == strcmp( argv[1], "0" ) ) )
            {
               Valid = true;    // Arguments valid; default state is "unlock"
            }
            else
            {
               MFG_printf( "Invalid argument\n" );
            }

            if ( Valid )  // Valid arguments?
            {

               if (  (  Lock && (flashSecEnabled  != fprot)  ) || // Locking
                     ( !Lock && (flashSecDisabled != fprot)) )    // Unlocking
               {
                  /********************************************************************************
                   CAUTION:  !!!!!!    This is a critical section.   !!!!!
                   ALL interrupts MUST be disabled since the sector containing the FSEC also
                   contains the Interrupt Vector Table.  The sector will be erased when updating
                   the FSEC field.  MUST use the PRIMASK form of disabling the interrupts.
                   TODO: When interrupts are disabled, then verify NO RTOS scheduling calls can be made - seems likely in parWrite
                  ********************************************************************************/
                  uint32_t primask = __get_PRIMASK();
                  __disable_interrupt();
                  retVal = PAR_partitionFptr.parWrite( destAddr, Lock ? &flashSecEnabled : &flashSecDisabled,
                                                        ( lCnt ) sizeof( fprot ), pImagePTbl );
                  __set_PRIMASK(primask); // Restore interrupts
                  if ( eSUCCESS == retVal  ) // And the write was successful
                  {
                     (void)DFWA_WaitForSysIdle( 3000 );   //No need to unlock when done since resetting
                     MFG_printf( "Resetting Processor.\n" );

                     /* Keep all other tasks from running.
                        Increase the priority of the power and idle tasks  */
                     ( void )OS_TASK_Set_Priority( pTskName_Pwr, 10 );
                     ( void )OS_TASK_Set_Priority( pTskName_MfgUartCmd, 11 );
                     ( void )OS_TASK_Set_Priority( pTskName_Idle, 12 );
                     //Does not return from here
                     PWR_SafeReset();
                  }
#if 0
                  else
                  {
                     MFG_printf( "Flash Partition Write FAILED\n" );
                  }
#endif
               }
            }
         }  /* End of request to update settings   */
#if 0    /* Debug code only   */
         MFG_printf( "fprot was: 0x%02x\n", fprot );
#endif
      }
      else
      {
         MFG_printf( "Flash Partition Open FAILED\n" );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* At end of process, report the results as 1 or 0, based on the registers matching the Lock or Unlock mask.   */
   (void)memcpy( &fprot, (uint8_t *)destAddr, sizeof( fprot ) );  //This directly reads from internal Flash
   MFG_printf( "%s %d\n", argv[0], ( flashSecEnabled == fprot ? 1 : 0 ) );
#elif ( MCU_SELECTED == RA6E1 )
   uint32_t dlmmon = ( R_PSCU->DLMMON & R_PSCU_DLMMON_DLMMON_Msk ); /* Get the Device Lifecycle Monitor value */
   #define DLM_DPL 4 /* Device Lifecycle Monitor state = Deployed (unable to find a #define in renesas.h */
   MFG_printf( "%s %d\n", argv[0], ( dlmmon == DLM_DPL ? 1 : 0 ) );
#endif // endif to #if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
}

/***********************************************************************************************************************
Function Name: MFGP_installationDateTime

   Purpose: Print the installation dateTime in seconds since epoch

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_installationDateTime( uint32_t argc, char *argv[] )
{

   uint32_t dateTimeVal = 0;
#if ( EP == 1 )
   (void)ENDPT_CIM_CMD_getDateTimeValue( installationDateTime, &dateTimeVal );
#else
   (void)DCU_CIM_CMD_getDateTimeValue( installationDateTime, &dateTimeVal );
#endif
   MFG_logPrintf( "%s %lu\n", argv[ 0 ],  dateTimeVal);
}


#if ( EP == 1 )
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
/***********************************************************************************************************************
Function Name: MFGP_dsBuEnabled

   Purpose: Get/Set dsBuEnabled parameter in the history module

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_dsBuEnabled( uint32_t argc, char *argv[] )
{

   uint8_t uDsBuEnabled;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uDsBuEnabled = ( uint8_t )atol( argv[1] );

         if ( uDsBuEnabled >= 1)
         {
            ( void )HD_setDsBuEnabled( (uint8_t)true );
         }
         else
         {
            ( void )HD_setDsBuEnabled( (uint8_t)false );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Read and display time request timeout */
   MFG_printf( "%s %u\n", argv[0], HD_getDsBuEnabled() );
}
#endif // endif of (ACLARA_LC == 0)

/***********************************************************************************************************************
Function Name: MFGP_timeRequestMaxTimeout

   Purpose: Get/Set the maximum amount of random delay following a request for network time before the time request is retried

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_timeRequestMaxTimeout( uint32_t argc, char *argv[] )
{

   uint16_t uTimeRequestMaxTimeout = 0;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uTimeRequestMaxTimeout = ( uint16_t )atol( argv[1] );
         ( void )TIME_SYS_SetTimeRequestMaxTimeout( uTimeRequestMaxTimeout );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Read and display time request timeout */
   MFG_printf( "%s %u\n", argv[0], TIME_SYS_GetTimeRequestMaxTimeout() );

}


/***********************************************************************************************************************
Function Name: MFGP_edInfo

   Purpose: Set or Print the End Device Information.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:   If more than one argument is given, the strings will be concatenated together with a
            single space separating them.
***********************************************************************************************************************/
static void MFGP_edInfo( uint32_t argc, char *argv[] )
{
   char     strEdInfo[EDCFG_INFO_LENGTH + 1];
#if ( HMC_KV == 1 ) || ( HMC_I210_PLUS == 1 )
   uint8_t  bTooLong = false;
   uint8_t  i;
   uint8_t  nCount = 0;
   uint8_t  nLength;


   if ( argc >= 2 )
   {
      /* Check for "" as input. If found - set strEdInfo to all NULLS.  */
      if ( strcmp( argv[ 1 ], "\"\"" ) == 0 )
      {
         ( void )memset( strEdInfo, 0, sizeof( strEdInfo ) );  /* User wants to clear this value   */
      }
      else
      {
         ( void )memset( strEdInfo, 0, sizeof( strEdInfo ) );  /* Clear the buffer */
         for ( i = 1; ( i < argc ) && ( nCount < EDCFG_INFO_LENGTH ); ++i )
         {
            /* If this is not the first argument being concatenated together, add a space to separate them. */
            if ( nCount > 0 )
            {
               strEdInfo[nCount] = ' ';
               ++nCount;
            }

            nLength = ( uint8_t )strlen( argv[i] );

            /* Make sure the new field will fit into the allowed string length when concatenated with the fields already
               copied in.  */
            if ( ( nCount + nLength ) > EDCFG_INFO_LENGTH )
            {
               bTooLong = true;
            }

            if ( false == bTooLong )
            {
               ( void )memcpy( ( char * ) &strEdInfo[nCount], argv[i], nLength );
               nCount += nLength;
            }

            /* If there is not room for at least 2 characters in the string being built, just give up because there is
               not room for the space charater and any part of the next argument.   */
            if ( ( nCount + 1 ) >= EDCFG_INFO_LENGTH )
            {
               // If there are still more arguments, the string entered is too long.
               if ( ( i + 1 ) < argc )
               {
                  bTooLong = true;
               }
            }

            /* If the combined arguments for were longer than the allowed field length, just give up.  */
            if ( true == bTooLong )
            {
               break;
            }
         }
      }

      if ( false == bTooLong )
      {
         // Write strEdInfo
         ( void )EDCFG_set_info( strEdInfo );
      }
   }
#else
   if ( argc > 1 )
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
#endif
   /* Always print read back value  */
   ( void )memset( strEdInfo, 0, sizeof( strEdInfo ) );
   (void)EDCFG_get_info( strEdInfo, sizeof( strEdInfo ) );
   MFG_printf( "%s %s\n", argv[0], strEdInfo);
}

/***********************************************************************************************************************
Function Name: MFGP_edMfgSerialNumber

   Purpose: Set or Print the End Device Mfg Serial Number.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:   For the I210+, this is written through the MFG port at integration.
            For the kV2C, it is read from the meter tables.
***********************************************************************************************************************/
static void MFGP_edMfgSerialNumber( uint32_t argc, char *argv[] )
{
#if ( ACLARA_LC == 1 )
   uint32_t serialNumber_u32;

   if(argc == 2)
   {

      serialNumber_u32 = ( uint32_t )atol( argv[1] );

      uint8_t data[ILC_DRU_DRIVER_REG_SERIAL_NUMBER_SIZE];
      data[0] = (serialNumber_u32 >> 24) & 0xFF;
      data[1] = (serialNumber_u32 >> 16) & 0xFF;
      data[2] = (serialNumber_u32 >>  8) & 0xFF;
      data[3] = (serialNumber_u32      ) & 0xFF;

      // In the DRU the Serial Number Register = 24, length = 4
      // Wait 2 seconds
      if ( CIM_QUALCODE_SUCCESS != ILC_DRU_DRIVER_writeDruRegister( ILC_DRU_DRIVER_REG_SERIAL_NUMBER, ILC_DRU_DRIVER_REG_SERIAL_NUMBER_SIZE, data, 2000 ) )
      {
         INFO_printf( "Unsuccessful DRU Write Reg" );
      }
   }
   /* Always print read back value  */

   ValueInfo_t  reading;
   INTF_CIM_CMD_getMeterReading( edMfgSerialNumber, &reading );
   MFG_logPrintf("%s %s\n", argv[0], reading.Value.buffer);
#elif (ACLARA_DA == 1)
   ValueInfo_t  reading;
   INTF_CIM_CMD_getMeterReading( edMfgSerialNumber, &reading );
   MFG_logPrintf("%s %s\n", argv[0], reading.Value.buffer);
#else
   char strEdMfgSerialNumber[EDCFG_MFG_SERIAL_NUMBER_LENGTH + 1];

   if ( argc <= 2 )
   {
#if HMC_I210_PLUS
      /* Serial number can only be set through the MFG port for the I210+.  For the kV2c it is read from the meter. */
      if ( 2 == argc )
      {
         uint8_t bTooLong = false;
         uint8_t nLength;

         ( void )memset( strEdMfgSerialNumber, 0, sizeof( strEdMfgSerialNumber ) );
         nLength = ( uint8_t )strlen( argv[1] );

         /* Make sure the new field will fit into the allowed string length when concatenated with the fields already
            copied in.  */
         if ( nLength > EDCFG_MFG_SERIAL_NUMBER_LENGTH )
         {
            bTooLong = true;
         }
         else
         {
            ( void )memcpy( ( void* ) strEdMfgSerialNumber, argv[1], nLength );
         }

         if ( false == bTooLong )
         {
            // Write strEdInfo
            ( void )EDCFG_set_mfg_serial_number( strEdMfgSerialNumber );
         }
      }
#endif
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %s\n", argv[0], EDCFG_get_mfg_serial_number( strEdMfgSerialNumber,
               sizeof( strEdMfgSerialNumber ) ) );
#endif
}

/***********************************************************************************************************************
Function Name: MFGP_edUtilitySerialNumber

   Purpose: Set or Print the End Device Utility Serial Number.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_edUtilitySerialNumber( uint32_t argc, char *argv[] )
{
   char strEdUtilitySerialNumber[EDCFG_UTIL_SERIAL_NUMBER_LENGTH + 1];

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uint8_t bTooLong = false;
         uint8_t nLength;

         ( void )memset( strEdUtilitySerialNumber, 0, sizeof( strEdUtilitySerialNumber ) );
         nLength = ( uint8_t )strlen( argv[1] );

         /* Make sure the new field will fit into the allowed string length when concatenated with the fields already
            copied in.  */
         if ( nLength > EDCFG_UTIL_SERIAL_NUMBER_LENGTH )
         {
            bTooLong = true;
         }
         else
         {
            ( void )memcpy( strEdUtilitySerialNumber, argv[1], nLength );
         }

         if ( false == bTooLong )
         {
            // Write strEdInfo
            ( void )EDCFG_set_util_serial_number( strEdUtilitySerialNumber );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %s\n", argv[0], EDCFG_get_util_serial_number( strEdUtilitySerialNumber,
               sizeof( strEdUtilitySerialNumber ) ) );
}

/***********************************************************************************************************************
   Function Name: MFGP_HMC_PortTest

   Purpose: Test the HMC serial port by transmitting a stream of bytes and reading them back.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_stP0LoopbackFailTest( uint32_t argc, char *argv[] )
{
   uint16_t       bytesReceived;
   uint16_t       LoopCount = 1;
   char           *endptr;
   const uint8_t  tx_string[] = { 'A', 'c', 'l', 'a', 'r', 'a', '\n' };
   uint8_t        rx_string[ sizeof( tx_string ) ];

   /* Disable the HMC main application */
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   ( void )HMC_APP_main( ( uint8_t )HMC_APP_CMD_INIT, NULL );
   ( void )HMC_APP_main( ( uint8_t )HMC_APP_CMD_DISABLE, NULL );
   while ( HMC_APP_main( ( uint8_t )HMC_APP_CMD_STATUS, NULL )  != ( uint8_t )HMC_APP_STATUS_LOGGED_OFF )
   {}
#endif   /* end of ACLARA_LC == 0   */
   if ( argc == 2 )
   {
      LoopCount = ( uint16_t )strtoul( argv[1], &endptr, 0 );
   }

   while( LoopCount )
   {
      LoopCount--;
      ( void )memset( rx_string, 0, sizeof( rx_string ) );
      ( void )UART_write( UART_HOST_COMM_PORT, ( uint8_t * )tx_string, sizeof( tx_string ) );
#if 0 // TODO: RA6E1 Implement tx empty functionality
      while( UART_isTxEmpty ( UART_HOST_COMM_PORT ) == 0 )
      {}
#endif

      OS_TASK_Sleep( 5 );     /* Wait long enough for the transmission to complete  */

#if (ACLARA_DA == 1)
      bytesReceived = 0;

      // DA requires UART in blocking mode so need to check for each byte
      while (UART_gotChar(UART_HOST_COMM_PORT) && bytesReceived < sizeof(rx_string))
      {
#if ( MCU_SELECTED == NXP_K24 )
         (void)UART_read(UART_HOST_COMM_PORT, &rx_string[bytesReceived], 1);
#elif ( MCU_SELECTED == RA6E1 )
         ( void ) UART_getc ( UART_HOST_COMM_PORT, &rx_string[bytesReceived], 1, OS_WAIT_FOREVER );
#endif
         bytesReceived++;
      }
#else
#if ( MCU_SELECTED == NXP_K24 )
      bytesReceived = UART_read( UART_HOST_COMM_PORT, rx_string, sizeof( rx_string ) );
#elif ( MCU_SELECTED == RA6E1 )
      bytesReceived = 0;
      while( TRUE )
      {
         ( void ) UART_getc ( UART_HOST_COMM_PORT, &rx_string[ bytesReceived ], 1, 10 );
         if( rx_string[ bytesReceived ] == '\n')
         {
            break;
         }
         bytesReceived++;
      }

#endif
#endif

      if( ( bytesReceived != sizeof( rx_string ) ) || ( memcmp( tx_string, rx_string, sizeof( tx_string ) ) != 0 ) )
      {
         if( stP0LoopbackFailCount  < ( ( 1 << ( 8 * sizeof( stP0LoopbackFailCount ) ) ) - 1 ) )
         {
            stP0LoopbackFailCount++;
         }
      }
      ( void )UART_flush ( UART_HOST_COMM_PORT ); /* Wait until all characters sent before re-enabling HMC!   */
   }
   MFG_logPrintf( "%s %d\n", argv[0], stP0LoopbackFailCount );
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
   ( void )HMC_APP_main( ( uint8_t )HMC_APP_CMD_ENABLE, NULL );
#endif   /* end of ACLARA_LC == 0   */
}

/***********************************************************************************************************************
   Function Name: MFGP_stP0LoopbackFailCount

   Purpose: Read/Write the HMC serial port loopback test failure counter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_stP0LoopbackFailCount( uint32_t argc, char *argv[] )
{
   char *endptr;

   if ( argc == 2 )           /* User wants to set the counter */
   {
      stP0LoopbackFailCount = ( uint16_t )strtoul( argv[1], &endptr, 10 );
   }
   MFG_logPrintf( "%s %d\n", argv[0], stP0LoopbackFailCount );
}
#endif

/***********************************************************************************************************************
   Function Name: MFGP_stSecurityFailCount

   Purpose: Read/Write the ECC security device test failure counter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_stSecurityFailCount( uint32_t argc, char *argv[] )
{
   SELF_TestData_t   *SelfTestData;
   char *endptr;

   SelfTestData = SELF_GetTestFileHandle()->Data;
   if ( argc == 2 )           /* User wants to set the counter */
   {
      SelfTestData->securityFail = ( uint16_t )strtoul( argv[1], &endptr, 10 );
      ( void )SELF_UpdateTestResults();   /* Update the file values  */
   }
   MFG_logPrintf( "%s %d\n", argv[0], SelfTestData->securityFail );
}

/***********************************************************************************************************************
   Function Name: MFGP_stSecurityFailTest

   Purpose: Test the ECC security device

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_stSecurityFailTest( uint32_t argc, char *argv[] )
{
   uint32_t          LoopCount;           /* User supplied number of runs           */
   char              *endptr;             /* Used with strtoul                      */
   uint32_t          event_flags;         /*lint !e578 local variable same name as global is OK here. */
   SELF_TestData_t   *SelfTestData;       /* Access to self-test failure counters   */
   uint16_t          initFailCount;       /* Failure count at start of test pass    */
   uint16_t          failCount = 0;       /* Number of failures during this run     */

   SelfTestData = SELF_GetTestFileHandle()->Data;
   initFailCount = SelfTestData->securityFail;

   if ( argc == 2 )  /* User wants multiple runs of this test  */
   {
      LoopCount = strtoul( argv[1], &endptr, 0 );
   }
   else
   {
      LoopCount = 1;
   }
   while( LoopCount-- )
   {
      OS_EVNT_Set ( SELF_notify, ( uint32_t )( 1 << ( uint16_t )e_securityFail ) );
      event_flags = OS_EVNT_Wait ( &MFG_notify, ( uint32_t )( 1 << ( uint16_t )e_securityFail ), ( bool )false, 1003 );
      if ( event_flags != 0 )
      {
         /* Test ran; get results   */
         if ( initFailCount != SelfTestData->securityFail )
         {
            failCount++;
            initFailCount = SelfTestData->securityFail;
         }
      }
   }

   if ( failCount != 0 )
   {
      ( void )SELF_UpdateTestResults();   /* Update the file values  */
   }
   MFG_logPrintf( "%s %d\n", argv[0], SelfTestData->securityFail );
}

#if ( EP == 1 )
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
/*******************************************************************************

   Function name: MFGP_meterCommunicationLockoutCount

   Purpose: This function set/get the meterCommunicationLockoutCount
            parameter from the hmc_eng module.

   Arguments: argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:

*******************************************************************************/
static void MFGP_meterCommunicationLockoutCount( uint32_t argc, char *argv[] )
{
   uint16_t uMeterCommunicationLockoutCount;

   if( argc <= 2)
   {
      if( argc == 2 )
      {
         char *endptr;
         uMeterCommunicationLockoutCount = (uint16_t)strtoul( argv[1], &endptr, 0 );
         (void)HMC_ENG_setMeterCommunicationLockoutCount( uMeterCommunicationLockoutCount );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   (void)HMC_ENG_getMeterCommunicationLockoutCount(&uMeterCommunicationLockoutCount);
   MFG_logPrintf( "%s %d\n", argv[ 0 ], uMeterCommunicationLockoutCount );
}
#endif // endif of (ACLARA_LC == 0)
#endif
/*******************************************************************************

   Function name: MFGP_temperature

   Purpose: This function will print out the device temperature

   Arguments: argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:

*******************************************************************************/
static void MFGP_temperature( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;

   GetConf = PHY_GetRequest( ePhyAttr_Temperature );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_logPrintf( "%s %d\n", argv[0], GetConf.val.Temperature );
   }
   else
   {
      Service_Unavailable();
   }
}
#if ( EP == 1 )
/*******************************************************************************

   Function name: MFGP_epMaxTemperature

   Purpose: This function will print out the device Maximum temperature

   Arguments: argc - Number of Arguments passed to this function
              argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:

*******************************************************************************/
static void MFGP_epMaxTemperature( uint32_t argc, char *argv[] )
{
   if( argc == 2 )
   { /* User wants to set the value */
      (void)TEMPERATURE_setEpMaxTemperature( ( uint8_t )atoi( argv[1] ) );
   }

   /* Always print read back value  */
   MFG_logPrintf( "%s %i\n", argv[0], TEMPERATURE_getEpMaxTemperature() );
}

/*******************************************************************************

   Function name: MFGP_epMinTemperature

   Purpose: This function will print out the device Maximum temperature

   Arguments: argc - Number of Arguments passed to this function
              argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:

*******************************************************************************/
static void MFGP_epMinTemperature( uint32_t argc, char *argv[] )
{
   int32_t val;
   if( argc == 2 ) /* User wants to set the value */
   {
      val = atol( argv[1] );
      if( SCHAR_MIN <= val && val <= SCHAR_MAX ) //val must be 1 byte [-128, 127]
      {
         (void)TEMPERATURE_setEpMinTemperature( ( int8_t )val );
      }
   }
   /* Always print read back value  */
   MFG_logPrintf( "%s %d\n", argv[0], TEMPERATURE_getEpMinTemperature() );
}

/*******************************************************************************

   Function name: MFGP_EpTempHysteresis

   Purpose: This function will print the Temperature Hysteresis

   Arguments: argc - Number of Arguments passed to this function
              argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:

*******************************************************************************/
static void MFGP_EpTempHysteresis( uint32_t argc, char *argv[] )
{
   if( argc == 2 ) /* User wants to set the value */
   {
      (void)TEMPERATURE_setEpTempHysteresis( ( uint8_t )atoi( argv[1] ) );
   }
   /* Always print read back value  */
   MFG_logPrintf( "%s %d\n", argv[0], TEMPERATURE_getEpTempHysteresis() );
}

/*******************************************************************************

   Function name: MFGP_HighTempThreshold

   Purpose: This function will print out the device high Temperature Threshold

   Arguments: argc - Number of Arguments passed to this function
              argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:

*******************************************************************************/
static void MFGP_HighTempThreshold( uint32_t argc, char *argv[] )
{
   if( argc == 2 ) /* User wants to set the value */
   {
      (void)TEMPERATURE_setHighTempThreshold( ( uint8_t )atoi( argv[1] ) );
   }
   /* Always print read back value  */
   MFG_logPrintf( "%s %d\n", argv[0], TEMPERATURE_getHighTempThreshold() );
}

/*******************************************************************************

   Function name: MFGP_EpTempMinThreshold

   Purpose: This function will print out the device Low Temperature Threshold

   Arguments: argc - Number of Arguments passed to this function
              argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:

*******************************************************************************/
static void MFGP_EpTempMinThreshold( uint32_t argc, char *argv[] )
{
   int32_t val;
   if( argc == 2 ) /* User wants to set the value */
   {
      val = atol( argv[1] );
      if( SCHAR_MIN <= val && val <= SCHAR_MAX ) //val must be 1 byte [-128, 127]
      {
         (void)TEMPERATURE_setEpTempMinThreshold( ( int8_t ) val);
      }
   }
   /* Always print read back value  */
   MFG_logPrintf( "%s %d\n", argv[0], TEMPERATURE_getEpTempMinThreshold() );
}
#endif /* endif of ( EP == 1 )*/
/*******************************************************************************

   Function name: MFG_DateTime

   Purpose: This function will print out the current time or set the time

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static void MFG_DateTime ( uint32_t argc, char *argv[] )
{
   uint32_t    locdateTime;
   TIMESTAMP_t sTime;

   if ( argc <= 2 )
   {
      if ( 2 == argc ) /* Need 1 additional parameter to set the RTC and System time. */
      {
         char *endptr;
         locdateTime = strtoul( argv[1], &endptr, 0 );

         ( void )TIME_UTIL_SetTimeFromSeconds( locdateTime, 0 );

#if ( EP == 1 )
         if ( 0 == locdateTime )
         {
            if ( !MAC_TimeQueryReq() )
            {
               {
                  DBG_printf( "MAC_TimeReq Failed" );
               }
            }
         }
#endif
      }
   }

   /* Always print read back value  */
   ( void )TIME_UTIL_GetTimeInSecondsFormat( &sTime );
   locdateTime = sTime.seconds;
   MFG_logPrintf( "%s %d\n", argv[ 0 ], locdateTime );
}
/*******************************************************************************

   Function name: MFG_stRTCFailTest

   Purpose: This function verifies that the RTC increments in approximately 1 second.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static void MFG_stRTCFailTest ( uint32_t argc, char *argv[] )
{
   uint32_t          event_flags;   /*lint !e578 local variable same name as global is OK here. */
   SELF_TestData_t   *SelfTestData; /* Access to self-test failure counters   */
   uint16_t          numberOfTests = 1;
   bool              updateTestResults = false;

   SelfTestData = SELF_GetTestFileHandle()->Data;
   if( 2 >= argc)
   {
       if( 2 == argc)
       {
          numberOfTests = ( uint16_t )atol( argv[1]);
       }
       while(numberOfTests != 0)
       {
          OS_EVNT_Set ( SELF_notify, ( uint32_t )( 1 << ( uint16_t )e_RTCFail ) ); /* Request the test  */
          event_flags = OS_EVNT_Wait ( &MFG_notify, ( uint32_t )( 1 << ( uint16_t )e_RTCFail ), ( bool )false, ONE_SEC * 3 );
          if ( event_flags != 0 ) /* Did the test run? */
          {
             updateTestResults = (bool) true;
          }
          numberOfTests--;
       }
       if( (bool) true == updateTestResults )
       {
          ( void )SELF_UpdateTestResults();   /* Update the file values if at least one test was run*/
       }
   }
   else
   {
      DBG_logPrintf( 'E', "Invalid number of parameters" );
   }

   MFG_logPrintf( "%s %d\n", argv[ 0 ], SelfTestData->RTCFail );
}
/*******************************************************************************

   Function name: MFG_stRTCFailCount

   Purpose: This function sets/gets the RTC failure counter.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static void MFG_stRTCFailCount ( uint32_t argc, char *argv[] )
{
   SELF_TestData_t   *SelfTestData;

   SelfTestData = SELF_GetTestFileHandle()->Data;
   if( argc > 1 )
   {
      char     *endptr;
      SelfTestData->RTCFail = ( uint16_t )strtoul( argv[1], &endptr, 10 );
      ( void )SELF_UpdateTestResults();   /* Update the file values  */
   }
   MFG_logPrintf( "%s %d\n", argv[ 0 ], SelfTestData->RTCFail );
}
/******************************************************************************

   Function Name: MFG_CommandLine_MacAddr

   Purpose: This function is used to get the mac address
            Usage: comdevicemacaddress

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

******************************************************************************/
static void MFG_CommandLine_MacAddr ( uint32_t argc, char *argv[] )
{
   macAddr_t         mac;

   ( void )memset( ( uint8_t * )&mac, 0, sizeof( mac ) );

   /*   Read the value from the security chip, if possible. Also gets the OUI.         */
   if ( ECC108_SUCCESS == ecc108_open() )
   {
      ( void )ecc108e_read_mac( mac.eui );
      ecc108_close();
   }

   /* MAC address is kept in external NV, updated to match security device at boot-up  */
   MAC_EUI_Address_Get( ( eui_addr_t * )mac.eui );

   MFG_logPrintf( "%s ", argv[ 0 ] );
   for( uint32_t i = 0; i < sizeof( mac.eui ); i++ )
   {
      MFG_logPrintf( "%02X", mac.eui[ i ] );
   }
   MFG_logPrintf( "\n" );

}
/******************************************************************************

   Function Name:  MFGP_MacChannelSets

   Purpose: This function is used to get/set the channel sets.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function
               channelSetsAttribute      - Channel Sets attribute
               channelSetsCountAttribute - Channel Sets Count attribute
               macChannelSets            - Maximum nuber of channel set in channelSets list

   Returns: none

   Notes:

******************************************************************************/
static void MFGP_MacChannelSets( uint32_t argc, char *argv[], MAC_ATTRIBUTES_e channelSetsAttribute, MAC_ATTRIBUTES_e channelSetsCountAttribute, uint8_t maxChannelSets)
{
   uint32_t i;                          /* loop counter   */
   uint32_t frequency;
   CH_SETS_t ChannelSets[max(MAC_MAX_CHANNEL_SETS_SRFN, MAC_MAX_CHANNEL_SETS_STAR)] = {0}; //lint !e506
   char *token;                           /* argv[1] tokens */
   uint8_t channelSetsCount;
   MAC_GetConf_t GetConf;

   if ( argc == 2 )      /* Setting the list of frequencies. Enforce 1 comma separated list.  */
   {
      // Initialize channel sets list
      for ( i = 0; i < maxChannelSets; i++)
      {
         ChannelSets[i].start = PHY_INVALID_CHANNEL;
         ChannelSets[i].stop  = PHY_INVALID_CHANNEL;
      }

      /* Loop through MAC_MAX_CHANNEL_SETS */
      for ( i = 0, token = argv[ 1 ]; i < maxChannelSets*2; i++ )
      {
         if( ( i == 0 ) || (( token != NULL ) && ( *token != 0)) )  /* There's something in slot 0, because argc > 1. */
         {
            if (sscanf( token, "%lu,", &frequency )) /* Convert value at token  */
            {
               if ((i%2) == 0)
               {
                  ChannelSets[i/2].start = ( uint16_t )FREQ_TO_CHANNEL( frequency );
               } else {
                  ChannelSets[i/2].stop  = ( uint16_t )FREQ_TO_CHANNEL( frequency );
               }
            }
            token = strpbrk( token, "," );      /* Find next token         */
            if ( token != NULL )                /*  If another comma is present, bump past it */
            {
               token++;
            }
         }
      }
      (void) MAC_SetRequest( channelSetsAttribute, ChannelSets);
   }

   /* Print out the list   */
   MFG_printf( "%s ", argv[ 0 ] );
   GetConf = MAC_GetRequest( channelSetsCountAttribute );
   channelSetsCount = GetConf.val.ChannelSetsCount;

   GetConf = MAC_GetRequest( channelSetsAttribute );
   for ( i = 0; i < channelSetsCount; i++ )
   {
      MFG_printf( "%u,%u", CHANNEL_TO_FREQ( GetConf.val.ChannelSets[i].start ),
                  CHANNEL_TO_FREQ( GetConf.val.ChannelSets[i].stop ) );
      if( i < (uint32_t)(channelSetsCount - 1) )
      {
         MFG_printf( "," );
      }
   }
   MFG_logPrintf( "\n" );
}

/******************************************************************************

   Function Name:  MFGP_MacChannelSetsSRFN

   Purpose: This function is used to get/set SRFN channel sets.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFGP_MacChannelSetsSRFN( uint32_t argc, char *argv[] )
{
   MFGP_MacChannelSets( argc, argv, eMacAttr_ChannelSetsSRFN, eMacAttr_ChannelSetsCountSRFN, MAC_MAX_CHANNEL_SETS_SRFN);
}

#if ( DCU == 1 )
/******************************************************************************

   Function Name:  MFGP_MacChannelSetsSTAR

   Purpose: This function is used to get/set STAR channel sets.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFGP_MacChannelSetsSTAR( uint32_t argc, char *argv[] )
{
   MFGP_MacChannelSets( argc, argv, eMacAttr_ChannelSetsSTAR, eMacAttr_ChannelSetsCountSTAR, MAC_MAX_CHANNEL_SETS_STAR);
}
#endif

/******************************************************************************

   Function Name:  MFG_GetSetFrequencies

   Purpose: This function is a helper function to all the get/set radio frequencies.
            It lists or sets the available frequencies of the type requested, based on the
            low-level get/set functions passed in.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function
               channelLimit - number of channels associated with the frequency type
               SetFunc() - low level set frequency function
               GetFunc() - low lever get frequency function
               rdUnits - indicate whether frequency list or channel list used

   Returns: none

   Notes: The low level get/set functions must all use the same API/parameters!

******************************************************************************/
static void MFG_GetSetFrequencies ( uint32_t argc, char *argv[], uint32_t channelLimit,
                                    bool ( *SetFunc )( uint8_t index, uint16_t chan ),
                                    bool ( *GetFunc )( uint8_t index, uint16_t *chan ),
                                    bool ( *isValid )( uint16_t chan ),
                                    enum_radioUnits_t rdUnits )
{
   uint32_t i;                            /* loop counter   */
   uint32_t frequency[PHY_MAX_CHANNEL];
   char *token;                           /* argv[1] tokens */
   uint16_t radio_channel;
   bool  valid;
   uint16_t Channel;

   if ( argc == 2 )      /* Setting the list of frequencies. Enforce 1 comma separated list.  */
   {
      ( void )memset( frequency, 0, sizeof( frequency ) );
      /* Loop through PHY_MAX_CHANNELs */
      for ( i = 0, token = argv[ 1 ]; i < channelLimit; i++ )
      {
         if( ( i == 0 ) || ( token != NULL ) )  /* There's something in slot 0, because argc > 1. */
         {

            /* if radio units are frequency, scan next token */
            if( rdUnits == frequencyUnits )
            {
               ( void )sscanf( token, "%ld,", &frequency[i] ); /* Convert value at token  */
            }

            /* else if radio units are channels, check return value since zero to verify int */
            else
            {
               frequency[i] = PHY_INVALID_CHANNEL;
               if( !sscanf( token, "%ld,", &frequency[i] ) )
               {
                  frequency[ i ] = PHY_INVALID_CHANNEL; /* set to invalid since zero is valid entry for channels */
               }
            }

            token = strpbrk( token, "," );      /* Find next token         */

            if ( token != NULL )                   /*  If another comma is present, bump past it */
            {
               token++;
            }
         }
         else /* no valid token, set to invalid values */
         {
            if ( rdUnits == frequencyUnits )
            {
               frequency[ i ] = 0;
            }
            else
            {
               frequency[ i ] = PHY_INVALID_CHANNEL;
            }
         }
#if 0
         MFG_printf( "chan: %d, freq: %d\n", i, frequency[ i ] );
#endif

         /* Make sure it's a valid frequency */
         if ( (rdUnits == frequencyUnits) && ( frequency[ i ] >= PHY_MIN_FREQ ) && ( frequency[ i ] <= PHY_MAX_FREQ ) )
         {
            radio_channel = ( uint16_t )FREQ_TO_CHANNEL( frequency[ i ] ); /* Convert frequency into a channel */
         }
         /* Make sure it's a valid channel if channel units are used */
         else if( (rdUnits == channelUnits) && (frequency[ i ] <= PHY_NUM_CHANNELS) )
         {
            radio_channel = ( uint16_t )frequency[ i ];
         }
         else  /* Otherwise, invalidate this channel  */
         {
            radio_channel = PHY_INVALID_CHANNEL;
         }
         /* Validate the choice. If this is the PhyAvailable list, no check needed. Otherwise make sure there's an
            entry in the PHY_config list corresponding to the rx or tx frequency */
         valid = ( isValid == NULL ) || isValid( radio_channel );
         (void)SetFunc( ( uint8_t )i, valid ? radio_channel : PHY_INVALID_CHANNEL ); /* Set the results for this channel */
      }
   }

   /* Print out the list   */
   if ( eSUCCESS == PHY_configRead() )               /* First read the file from NV!  */
   {
      MFG_printf( "%s ", argv[ 0 ] );
      for ( i = 0; i < channelLimit; i++ )
      {
         if ( GetFunc( ( uint8_t )i, &Channel ) )
         {
            if ( Channel != PHY_INVALID_CHANNEL )
            {
               if (rdUnits == frequencyUnits )
               {
                  MFG_printf( "%u", CHANNEL_TO_FREQ( Channel ) ); /*lint !e666 value sent to macro OK */
               }
               else
               {
                  MFG_printf( "%u", Channel ); /*lint !e666 value sent to macro OK */
               }
            }
            if( i < channelLimit - 1 )
            {
               MFG_printf( "," );
            }
         }
      }
      MFG_logPrintf( "\n" );
   }
   else
   {
      MFG_logPrintf( "PHY config file read failed!\n" );
   }
}
/******************************************************************************

   Function Name:  MFG_PhyAvailableFrequencies

   Purpose: Get/Set Phy frequencies

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyAvailableFrequencies( uint32_t argc, char *argv[] )
{
   MFG_GetSetFrequencies ( argc, argv, PHY_MAX_CHANNEL, PHY_Channel_Set, PHY_Channel_Get, NULL, frequencyUnits );
}
/******************************************************************************

   Function Name:  MFG_PhyAvailableChannels

   Purpose: Get/Set Phy frequencies

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyAvailableChannels( uint32_t argc, char *argv[] )
{
   MFG_GetSetFrequencies ( argc, argv, PHY_MAX_CHANNEL, PHY_Channel_Set, PHY_Channel_Get, NULL, channelUnits);
}
/******************************************************************************

   Function Name:  MFG_PhyRxFrequencies

   Purpose: Get/Set Receiver frequencies

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyRxFrequencies ( uint32_t argc, char *argv[] )
{
   MFG_GetSetFrequencies ( argc, argv, PHY_RCVR_COUNT, PHY_RxChannel_Set,
                           PHY_RxChannel_Get, PHY_IsChannelValid, frequencyUnits );
}
/******************************************************************************

   Function Name:  MFG_PhyRxChannels

   Purpose: Get/Set Receiver channels

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyRxChannels ( uint32_t argc, char *argv[] )
{
   MFG_GetSetFrequencies ( argc, argv, PHY_RCVR_COUNT, PHY_RxChannel_Set,
                           PHY_RxChannel_Get, PHY_IsChannelValid, channelUnits );
}
/******************************************************************************

   Function Name:  MFG_PhyTxFrequencies

   Purpose: Get/Set Transmitter frequencies

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyTxFrequencies ( uint32_t argc, char *argv[] )
{
   MFG_GetSetFrequencies ( argc, argv, PHY_MAX_CHANNEL, PHY_TxChannel_Set,
                           PHY_TxChannel_Get, PHY_IsChannelValid, frequencyUnits );
}

#if VSWR_MEASUREMENT == 1
/**
Gets the last VSWR reading.

@param argc Number of Arguments passed to this function.
@param argv Pointer to the list of arguments passed to this function.
*/
static void MFGP_Vswr(uint32_t argc, char *argv[])
{
   char floatStr[PRINT_FLOAT_SIZE];

   MFG_printf("%s %s\n", argv[0], DBG_printFloat(floatStr, cachedVSWR, 2));
}
#endif

/******************************************************************************

   Function Name:  MFG_PhyTxChannels

   Purpose: Get/Set Transmitter channels

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyTxChannels ( uint32_t argc, char *argv[] )
{
   MFG_GetSetFrequencies ( argc, argv, PHY_MAX_CHANNEL, PHY_TxChannel_Set,
                           PHY_TxChannel_Get, PHY_IsChannelValid, channelUnits );
}

/******************************************************************************

   Function Name:  MFG_PhyFailedFrameDecodeCount

   Purpose: The number of received PHY frames that failed FEC decoding

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
 static void MFG_PhyFailedFrameDecodeCount ( uint32_t argc, char *argv[] )
{

  PHY_GetConf_t GetConf;
  uint8_t i; // loop var
  if( argc == 1 )
  {
    GetConf = PHY_GetRequest( ePhyAttr_FailedFrameDecodeCount );
    if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
      MFG_printf( "%s ", argv[ 0 ]);
      for ( i = 0; i < PHY_RCVR_COUNT; i++)
      {
        MFG_printf( " %u,", GetConf.val.FailedFrameDecodeCount[i]);
      }
      MFG_printf( "\n");
    }
    else
    {
      Service_Unavailable();
    }
  }
  else
  {
    DBG_logPrintf( 'R', "Invalid number of parameters" );
  }
}

/******************************************************************************

   Function Name:  MFG_PhyFailedHcsCount

   Purpose: Get number of received PHY frame headers that failed the header check
   sequence validation.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyFailedHcsCount ( uint32_t argc, char *argv[] )
{
  PHY_GetConf_t GetConf;
  uint8_t i; // loop var
  if( argc == 1)
  {
    GetConf = PHY_GetRequest( ePhyAttr_FailedHcsCount );
    if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
      MFG_printf( "%s", argv[ 0 ]);
      for (i = 0; i < PHY_RCVR_COUNT; i++)
      {
        MFG_printf( " %u,", GetConf.val.FailedHcsCount[i] );
      }
      MFG_printf( "\n");
    }
    else
    {
      Service_Unavailable();
    }
  }
  else
  {
    DBG_logPrintf( 'R', "Invalid number of parameters" );
  }
}

/******************************************************************************

   Function Name:  MFG_PhyFramesReceivedCount

   Purpose: Get number of received PHY frame headers

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyFramesReceivedCount( uint32_t argc, char *argv[] )
{
  PHY_GetConf_t GetConf;
  uint8_t i; // loop var
  if( argc == 1 )
  {
    GetConf = PHY_GetRequest( ePhyAttr_FramesReceivedCount );
    if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
      MFG_printf( "%s", argv[ 0 ]);
      for (i = 0; i < PHY_RCVR_COUNT; i++)
      {
        MFG_printf( " %u,", GetConf.val.FramesReceivedCount[i] );
      }
      MFG_printf( "\n");
    }
    else
    {
      Service_Unavailable();
    }
  }
  else
  {
    DBG_logPrintf( 'R', "Invalid number of parameters" );
  }


}

/******************************************************************************

   Function Name:  MFG_PhyFramesTransmittedCount

   Purpose: Get number of transmitted PHY frame headers

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyFramesTransmittedCount( uint32_t argc, char *argv[] )
{
  PHY_GetConf_t GetConf;
  if( argc == 1 )
  {
    GetConf = PHY_GetRequest( ePhyAttr_FramesTransmittedCount );
    if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
      MFG_printf( "%s %u\n", argv[ 0 ], GetConf.val.FramesTransmittedCount );
    }
    else
    {
      Service_Unavailable();
    }
  }
  else
  {
    DBG_logPrintf( 'R', "Invalid number of parameters" );
  }
}

/******************************************************************************

   Function Name:  MFG_PhyFailedHeaderDecodeCount

   Purpose: The number of received PHY frame headers that failed FEC decoding

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyFailedHeaderDecodeCount( uint32_t argc, char *argv[] )
{
  PHY_GetConf_t GetConf;
  uint8_t i; // loop var
  if( argc == 1 )
  {
    GetConf = PHY_GetRequest( ePhyAttr_FailedHeaderDecodeCount );
    if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
      MFG_printf( "%s ", argv[ 0 ]);
      for (i = 0; i < PHY_RCVR_COUNT; i++)
      {
        MFG_printf( " %u,", GetConf.val.FailedHeaderDecodeCount[i] );
      }
      MFG_printf( "\n");
    }
    else
    {
      Service_Unavailable();
    }
  }
  else
  {
    DBG_logPrintf( 'R', "Invalid number of parameters" );
  }

}

/******************************************************************************

   Function Name:  MFG_PhySyncDetectCount

   Purpose: The number of times a valid sync word is detected

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhySyncDetectCount( uint32_t argc, char *argv[] )
{
  PHY_GetConf_t GetConf;
  uint8_t i; // loop var
  if( argc == 1 )
  {
    GetConf = PHY_GetRequest( ePhyAttr_SyncDetectCount );
    if ( GetConf.eStatus == ePHY_GET_SUCCESS )
    {
      MFG_printf( "%s ", argv[ 0 ]);
      for (i = 0; i < PHY_RCVR_COUNT; i++)
      {
        MFG_printf( " %u,", GetConf.val.SyncDetectCount[i] );
      }
      MFG_printf( "\n");
    }
    else
    {
      Service_Unavailable();
    }
  }
  else
  {
    DBG_logPrintf( 'R', "Invalid number of parameters" );
  }
}

/******************************************************************************

   Function Name:  MFG_PhyCcaThreshold

   Purpose: Get/Set PHY Cca Threshold

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyCcaThreshold ( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   int16_t Value;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_CcaThreshold;

   if ( argc == 2 )
   {
      Value = ( int8_t )atoi( argv[1] );
      (void)PHY_SetRequest( eAttribute, &Value);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.CcaThreshold );
   }
   else
   {
      Service_Unavailable();
   }
}

/******************************************************************************

   Function Name:  MFG_PhyCcaAdaptiveThresholdEnable

   Purpose: Enable/DisAble PHY Cca Adaptive Threshold

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyCcaAdaptiveThresholdEnable ( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   bool enable;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_CcaAdaptiveThresholdEnable;
   if ( argc == 2 )
   {
      enable = ( bool )atoi( argv[1] );
      (void)PHY_SetRequest( eAttribute, &enable);
   }
   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.CcaAdaptiveThresholdEnable );
   }
   else
   {
      Service_Unavailable();
   }
}

/******************************************************************************

   Function Name:  MFG_PhyCcaAdaptiveThresholdEnable

   Purpose: Enable/DisAble PHY Cca Adaptive Threshold

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyCcaOffset( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint8_t offset;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_CcaOffset;

   if ( argc == 2 )
   {
      offset = ( uint8_t )strtoull( argv[1], NULL, 0 );
     (void)PHY_SetRequest( eAttribute, &offset);

   }

   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %u\n", argv[ 0 ], GetConf.val.CcaOffset );
   }
   else
   {
      Service_Unavailable();
   }

}
/******************************************************************************

   Function Name:  MFG_PhyDemodulator

   Purpose: Get/Set PHY demodulator

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
#if ( EP == 1 )
static void MFG_PhyDemodulator ( uint32_t argc, char *argv[] )
{
#if ( EP == 0 )
#error Update this call for DCU to handle a list instead of just one element
#endif
   PHY_GetConf_t GetConf;
   uint8_t Value;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_Demodulator;
   if ( argc == 2 )
   {
      Value = ( uint8_t )atoi( argv[1] );
      (void)PHY_SetRequest( eAttribute, &Value);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %u\n", argv[ 0 ], GetConf.val.Demodulator[0] );
   }
   else
   {
      Service_Unavailable();
   }
}
#endif
/******************************************************************************

   Function Name: MFG_PhyFrontEndGain

   Purpose: Get/Set the front end gain

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyFrontEndGain ( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   int32_t gain;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_FrontEndGain;

   if ( argc == 2 )
   {
      gain = atol( argv[1] );
      if( SCHAR_MIN <= gain && gain <= SCHAR_MAX ) //val must be 1 byte [-128, 127]
      {
         (void)PHY_SetRequest( eAttribute, (int8_t *) &gain);
      }
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
      MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.FrontEndGain );
   }
   else
   {
      Service_Unavailable();
   }
}

/******************************************************************************

   Function Name: MFG_PhyMaxTxPayload

   Purpose: Get/Set the maximum tansmitable TX payload size

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

******************************************************************************/
static void MFG_PhyMaxTxPayload ( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint16_t maxTxPayload;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_MaxTxPayload;

   if ( argc == 2 )
   {
      maxTxPayload = ( uint16_t )atoi( argv[1] );
      (void)PHY_SetRequest( eAttribute, &maxTxPayload);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %u\n", argv[ 0 ], GetConf.val.MaxTxPayload );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFG_TimePrecision

   Purpose: Get/Set the TimePrecision

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_TimePrecision ( uint32_t argc, char *argv[] )
{
   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], TIME_SYNC_TimePrecision_Get());
}

/***********************************************************************************************************************
   Function Name: MFG_TimeDefaultAccuracy

   Purpose: Get/Set the Time Default Accuracy

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_TimeDefaultAccuracy ( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         int8_t value;
         value = ( int8_t )atoi( argv[1] );
         (void)TIME_SYNC_TimeDefaultAccuracy_Set(value);
      }
   }
   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], TIME_SYNC_TimeDefaultAccuracy_Get());
}

/***********************************************************************************************************************
   Function Name: MFG_TimeQueryResponseMode

   Purpose: "Get/Set the time query response behavior to broadcast(0), unicast(1) or ignore the request (2)

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_TimeQueryResponseMode ( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         uint8_t value;
         value = ( uint8_t )atoi( argv[1] );
         (void)TIME_SYNC_TimeQueryResponseMode_Set(value);
      }
   }
   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], TIME_SYNC_TimeQueryResponseMode_Get());
}

#if ( EP == 1)
/***********************************************************************************************************************
   Function Name: MFG_TimeSetStart

   Purpose: Get/Set the TimeSetStart

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_TimeSetStart ( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         uint32_t value;
         value = ( uint32_t )atoi( argv[1] );
         (void)TIME_SYNC_TimeSetStart_Set(value);
      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], TIME_SYNC_TimeSetStart_Get());

}

/***********************************************************************************************************************
   Function Name: MFG_TimeSetMaxOffset

   Purpose: Get/Set the TimeSetMaxOffset

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_TimeSetMaxOffset  ( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         uint32_t value;
         value = ( uint32_t )atoi( argv[1] );
         (void)TIME_SYNC_TimeSetMaxOffset_Set((uint16_t)value);
      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], TIME_SYNC_TimeSetMaxOffset_Get() );

}

/***********************************************************************************************************************
   Function Name: MFG_TimeSetPeriod

   Purpose: Get/Set the TimeSetPeriod

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_TimeSetPeriod ( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         uint32_t value;
         value = ( uint32_t )atoi( argv[1] );
         (void)TIME_SYNC_TimeSetPeriod_Set(value);
      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], TIME_SYNC_TimeSetPeriod_Get() );

}

/***********************************************************************************************************************
   Function Name: MFG_TimeSource

   Purpose: Get/Set the TimeSource

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_TimeSource ( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         uint32_t value;
         value = ( uint32_t )atoi( argv[1] );
         (void)TIME_SYNC_TimeSource_Set((uint8_t)value);
      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], TIME_SYNC_TimeSource_Get() );

}
#endif

#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
/***********************************************************************************************************************
   Function Name: MFG_LinkParametersPeriod

   Purpose: Get/Set the macLinkParametersPeriod

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_LinkParametersPeriod ( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         uint32_t value;
         value = ( uint32_t )atoi( argv[1] );
        ( void )MAC_SetRequest( eMacAttr_LinkParamPeriod, &value);
      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   MAC_GetConf_t GetConf;
   GetConf = MAC_GetRequest( eMacAttr_LinkParamPeriod );

   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.LinkParamPeriod );
}

/***********************************************************************************************************************
   Function Name: MFG_LinkParametersStart

   Purpose: Get/Set the Link parameter Start

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_LinkParametersStart ( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         uint32_t value;
         value = ( uint32_t )atoi( argv[1] );
         ( void )MAC_SetRequest( eMacAttr_LinkParamStart, &value);
      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   MAC_GetConf_t GetConf;
   GetConf = MAC_GetRequest( eMacAttr_LinkParamStart );

   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.LinkParamStart );
}

/***********************************************************************************************************************
   Function Name: MFG_LinkParametersMaxOffset

   Purpose: Get/Set the Link parameter Max Offset

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_LinkParametersMaxOffset ( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         uint32_t value;
         value = ( uint16_t )atoi( argv[1] );
         ( void )MAC_SetRequest( eMacAttr_LinkParamMaxOffset, &value);
      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   MAC_GetConf_t GetConf;
   GetConf = MAC_GetRequest( eMacAttr_LinkParamMaxOffset );

   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.LinkParamMaxOffset );
}

/***********************************************************************************************************************
   Function Name: MFG_ReceivePowerMargin

   Purpose: Get/Set the Link parameter RPT Margin

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_ReceivePowerMargin ( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         uint32_t value;
         value = ( uint8_t )atoi( argv[1] );
         ( void )PHY_SetRequest( ePhyAttr_ReceivePowerMargin, &value );
      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   PHY_GetConf_t GetConf;
   GetConf = PHY_GetRequest( ePhyAttr_ReceivePowerMargin );

   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.ReceivePowerMargin );
}
#endif // end of ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) ) section

#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
/***********************************************************************************************************************
   Function Name: MFG_MacCommandResponseMaxTimeDiversity

   Purpose: Get/Set the Link parameter RPT Margin

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments

   Returns: none
***********************************************************************************************************************/
static void MFG_MacCommandResponseMaxTimeDiversity ( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         uint32_t value;
         value = ( uint16_t )atoi( argv[1] );
         ( void )MAC_SetRequest( eMacAttr_CmdRespMaxTimeDiversity, &value );
      }
   }
   else  // More than one parameter
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   MAC_GetConf_t GetConf;
   GetConf = MAC_GetRequest( eMacAttr_CmdRespMaxTimeDiversity );

   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.macCmdRespMaxTimeDiversity );
}
#endif

/***********************************************************************************************************************
   Function Name: MFGP_DeviceGatewayConfig

   Purpose: Get/Set the comDeviceGatewayConfig

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
#if ( DCU == 1 )
static void MFGP_DeviceGatewayConfig( uint32_t argc, char *argv[] )
{
   ( void )argc;
   uint8_t comDeviceGatewayConfigString[COM_DEVICE_GATEWAY_CONFIG_LEN];

   if ( argc == 2 )
   {
      /* update saved comDeviceType */
      ( void )VER_setComDeviceGatewayConfig ( (uint8_t *)argv[1] );
   }

   /* echo current configured value */
   if (eFAILURE == VER_getComDeviceGatewayConfig ( &comDeviceGatewayConfigString[0], sizeof(comDeviceGatewayConfigString) ) )
   {
      comDeviceGatewayConfigString[0] = '\0';
   }
   MFG_printf( "%s %s\n", argv[ 0 ], &comDeviceGatewayConfigString[0] );
}
#endif
/***********************************************************************************************************************
   Function Name:  MFG_GetSetConfig

   Purpose: This function is a helper function to all the get/set radio frequencies.
            It lists or sets the available frequencies of the type requested, based on the
            low-level get/set functions passed in.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function
               channelLimit - number of channels associated with the frequency type
               SetFunc() - low level set frequency function
               GetFunc() - low lever get frequency function

   Returns: none

   Notes: The low level get/set functions must all use the same API/parameters!
***********************************************************************************************************************/
static void MFG_GetSetConfig ( uint32_t argc, char *argv[],
                               bool ( *SetFunc )( uint8_t const *list ), bool ( *GetFunc )( uint8_t *list ),
                               uint32_t min_value, uint32_t max_value )
{
   uint32_t i;                   /* loop counter   */
   uint8_t list[PHY_RCVR_COUNT];
   uint32_t element;
   char *token;                  /* argv[1] tokens */

   // Retrieve current list
   (void)GetFunc( list );

   // Only execute this if the SetFunc is not NULL
   if (SetFunc && ( argc == 2 ))   /* Setting the list. Enforce 1 comma separated list.  */
   {
      for ( i = 0, token = argv[ 1 ]; i < PHY_RCVR_COUNT; i++ )
      {
         if( ( i == 0 ) || (( token != NULL ) && ( *token != 0 )) ) /* There's something in slot 0, because argc > 1. */
         {
            if (sscanf( token, "%lu", &element )) /* Convert value at token  */
            {
               /* Make sure it's valid */
               if ( (element >= min_value ) && ( element < max_value ) )
               {
                  list[ i ] = (uint8_t)element;
               }
               token = strpbrk( token, "," );      /* Find next token         */
            }
            if (( token != NULL ) && (*token != 0) ) /*  If another comma is present, bump past it */
            {
               token++;
            }
         }
      }
      // Save updated list
      (void)SetFunc( list );

      // Retrieve current list to print what list really is.
      (void)GetFunc( list );
   }

   /* Print out the list */
   MFG_printf( "%s ", argv[ 0 ] );
   for ( i = 0; i < PHY_RCVR_COUNT; i++ )
   {
      MFG_printf( "%u", list[i] );
#if ( DCU == 1)
      if ( i < (PHY_RCVR_COUNT-1) )
      {
         MFG_printf( "," );
      }
#endif
   }
   MFG_printf( "\n" );
}
/***********************************************************************************************************************
   Function Name: MFG_PhyRxDetection

   Purpose: Set/Get available rx detection config

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_PhyRxDetection( uint32_t argc, char *argv[] )
{
   MFG_GetSetConfig ( argc, argv, PHY_RxDetectionConfig_Set, PHY_RxDetectionConfig_Get,
                     (uint32_t)ePHY_DETECTION_0, (uint32_t)ePHY_DETECTION_LAST );
}

/***********************************************************************************************************************
   Function Name: MFG_PhyRxFraming

   Purpose: Set/Get available rx framing config

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_PhyRxFraming( uint32_t argc, char *argv[] )
{
   MFG_GetSetConfig ( argc, argv, PHY_RxFramingConfig_Set, PHY_RxFramingConfig_Get,
                     (uint32_t)ePHY_FRAMING_0, (uint32_t)ePHY_FRAMING_LAST );
}

/***********************************************************************************************************************
   Function Name: MFG_PhyRxMode

   Purpose: Set/Get available rx mode

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_PhyRxMode( uint32_t argc, char *argv[] )
{
   MFG_GetSetConfig ( argc, argv, PHY_RxMode_Set, PHY_RxMode_Get, (uint32_t)ePHY_MODE_0, (uint32_t)ePHY_MODE_LAST );
}

/***********************************************************************************************************************
   Function Name: MFG_PhyThermalControlEnable

   Purpose: Get/Set PHY thermal control feature

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_PhyThermalControlEnable( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   bool state;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_ThermalControlEnable;

   if ( argc == 2 )
   {
      state = ( bool )atoi( argv[1] );
      (void)PHY_SetRequest( eAttribute, &state);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.ThermalControlEnable );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFG_PhyThermalProtectionEnable

   Purpose: Get/Set PHY thermal protection feature

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_PhyThermalProtectionEnable( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   bool state;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_ThermalProtectionEnable;

   if ( argc == 2 )
   {
      state = ( bool )atoi( argv[1] );
      (void)PHY_SetRequest( eAttribute, &state);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.ThermalProtectionEnable );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFG_PhyThermalProtectionEnable

   Purpose: Get count of times peripheral has exceed thermal protections

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_PhyThermalProtectionCount(uint32_t argc, char* argv[])
{
   PHY_GetConf_t GetConf;
   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_ThermalProtectionCount;

   GetConf = PHY_GetRequest( eAttribute );
   /* Always print read back value  */
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.ThermalProtectionCount );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFG_PhyThermalProtectionEngaged

   Purpose: Get if peripheral has exceed thermal protections

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_PhyThermalProtectionEngaged(uint32_t argc, char* argv[])
{
   PHY_GetConf_t GetConf;
   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_ThermalProtectionEngaged;

   GetConf = PHY_GetRequest( eAttribute );
   /* Always print read back value  */
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %d\n", argv[ 0 ], GetConf.val.ThermalProtectionEngaged );
   }
   else
   {
      Service_Unavailable();
   }
}
/***********************************************************************************************************************
   Function Name: MFG_PhyNoiseEstimate

   Purpose: Get PHY noise estimate for each channel

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_PhyNoiseEstimate( uint32_t argc, char* argv [])
{
   PHY_GetConf_t GetConf;
   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_NoiseEstimate;
   GetConf = PHY_GetRequest( eAttribute );

   if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
      //uint32_t i;
      MFG_printf( "%s\n", argv[0]);
      for (uint32_t i = 0; i < PHY_MAX_CHANNEL; i ++)
      {
        MFG_printf( "Channel #%d : %d\n", i, (int32_t)GetConf.val.NoiseEstimate[i]);
      }
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFG_PhyNoiseEstimateRate

   Purpose: Get/Set PHY noise estimate rate

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_PhyNoiseEstimateRate( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint8_t       rate;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_NoiseEstimateRate;

   if ( argc == 2 )
   {
      rate = ( uint8_t )atoi( argv[1] );
      (void)PHY_SetRequest( eAttribute, &rate);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %u\n", argv[ 0 ], GetConf.val.NoiseEstimateRate );
   }
   else
   {
      Service_Unavailable();
   }
}

#if ( DCU == 1 )
/******************************************************************************

   Function Name: MFGP_DCUVersion

   Purpose: Get/Set the DCU operating mode / version

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:

******************************************************************************/
static void MFGP_DCUVersion( uint32_t argc, char *argv[] )
{
   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         (void)VER_strSetDCUVersion(argv[1]);
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %s\n", argv[ 0 ], VER_strGetDCUVersion());
}

/******************************************************************************

   Function Name: MFG_ToolEP_SDRAMTest

   Purpose: Run the SDRAM test

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
static void MFG_ToolEP_SDRAMTest( uint32_t argc, char *argv[] )
{
   uint32_t          LoopCount = 1;    /* Assume 1 pass through the test   */
   uint32_t          errCount = 0;     /* Number of failures reported by DVR_EFL_unitTest */
   char              *endptr;
   uint32_t          event_flags;      /*lint !e578 local variable same name as global is OK here. */
   SELF_TestData_t   *SelfTestData;
   uint16_t          initFailCount;    /* Failure count at start of test pass    */

   SelfTestData = SELF_GetTestFileHandle()->Data;
   if ( argc == 2 )
   {
      LoopCount = strtoul( argv[1], &endptr, 0 );
   }
   while ( LoopCount )
   {
      initFailCount = SelfTestData->SDRAMFail;
      OS_EVNT_Set ( SELF_notify, (uint32_t)( 1 << (uint16_t)e_sdRAMFail ) );  /* Request the test  */
      event_flags = OS_EVNT_Wait ( &MFG_notify, (uint32_t)( 1 << (uint16_t)e_sdRAMFail ), (bool)false, ONE_SEC * 20 );
      if ( event_flags != 0 )    /* Did the test complete?  */
      {
         if ( initFailCount != SelfTestData->SDRAMFail ) /* Did the test pass?   */
         {
            initFailCount = SelfTestData->SDRAMFail;
            errCount++;
         }
         LoopCount--;
      }
   }

   if( errCount != 0 )  /* Need to update the failure counter? */
   {
      (void)SELF_UpdateTestResults();
   }
   MFG_printf( "%s %d\n", argv[0], SelfTestData->SDRAMFail );
}

/***********************************************************************************************************************
   Function Name: MFG_ToolEP_SDRAMCount

   Purpose: Read/Write the SDRAM test failure counter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_ToolEP_SDRAMCount( uint32_t argc, char *argv[] )
{
   SELF_TestData_t   *SelfTestData;
   char *endptr;

   SelfTestData = SELF_GetTestFileHandle()->Data;
   if ( argc == 2 )           /* User wants to set the counter */
   {
      SelfTestData->SDRAMFail = ( uint16_t )strtoul( argv[1], &endptr, 10 );
      ( void )SELF_UpdateTestResults();   /* Update the file values  */
   }
   MFG_logPrintf( "%s %d\n", argv[0], SelfTestData->SDRAMFail );
}

#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
/***********************************************************************************************************************
   Function Name: MFGP_fngVSWR

   Purpose: Read the VSWR

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_fngVSWR( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint8_t       loc_vswr;
   uint16_t      integerPart = 0;
   uint16_t      fracPart = 0;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_fngVSWR;

   if ( argc == 2 )
   {
      (void)sscanf( argv[1], "%hu.%1hu", &integerPart, &fracPart );
      loc_vswr = ( integerPart * 10U ) + fracPart;
      (void)PHY_SetRequest( eAttribute, &loc_vswr);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %hu.%hu:1\n", argv[ 0 ], GetConf.val.fngVSWR/10U, GetConf.val.fngVSWR%10U );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFGP_fngVswrNotificationSet

   Purpose: Get/Set VSWR Notification Set Point

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_fngVswrNotificationSet( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint16_t      vswrnotificationset;
   uint16_t      integerPart = 0;
   uint16_t      fracPart = 0;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_fngVswrNotificationSet;

   if ( argc == 2 )
   {
      (void)sscanf( argv[1], "%hu.%1hu", &integerPart, &fracPart );
      vswrnotificationset = ( integerPart * 10U ) + fracPart;
      (void)PHY_SetRequest( eAttribute, &vswrnotificationset);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %hu.%1hu:1\n", argv[ 0 ], GetConf.val.fngVswrNotificationSet/10U, GetConf.val.fngVswrNotificationSet%10U );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFGP_fngVswrShutdownSet

   Purpose: Get/Set VSWR Shutdown Set Point

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_fngVswrShutdownSet( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint16_t      vswrshutdownset;
   uint16_t      integerPart = 0;
   uint16_t      fracPart = 0;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_fngVswrShutdownSet;

   if ( argc == 2 )
   {
      (void)sscanf( argv[1], "%hu.%1hu", &integerPart, &fracPart );
      vswrshutdownset = ( integerPart * 10U ) + fracPart;
      (void)PHY_SetRequest( eAttribute, &vswrshutdownset);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %hu.%hu:1\n", argv[ 0 ], GetConf.val.fngVswrShutdownSet/10U, GetConf.val.fngVswrShutdownSet%10U );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFGP_fngForwardPower

   Purpose: Get Forward Power

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_fngForwardPower( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint16_t      forwardpower;
   uint16_t      integerPart = 0;
   uint16_t      fracPart = 0;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_fngForwardPowerSet;

   if ( argc == 2 )
   {
      (void)sscanf( argv[1], "%hu.%1hu", &integerPart, &fracPart );
      forwardpower = ( integerPart * 10U ) + fracPart;
      (void)PHY_SetRequest( eAttribute, &forwardpower);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %hu.%hu\n", argv[ 0 ], GetConf.val.fngForwardPower/10U, GetConf.val.fngForwardPower%10U );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFGP_fngReflectedPower

   Purpose: Get Reflected Power

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_fngReflectedPower( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint16_t      reflectedpower;
   uint16_t      integerPart = 0;
   uint16_t      fracPart = 0;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_fngReflectedPower;

   if ( argc == 2 )
   {
      (void)sscanf( argv[1], "%hu.%1hu", &integerPart, &fracPart );
      reflectedpower = ( integerPart * 10U ) + fracPart;
      (void)PHY_SetRequest( eAttribute, &reflectedpower);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %hu.%hu\n", argv[ 0 ], GetConf.val.fngReflectedPower/10U, GetConf.val.fngReflectedPower%10U );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFGP_fngForwardPowerSet

   Purpose: Get/Set Forward Power Set Point

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_fngForwardPowerSet( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint16_t      forwardpowerset;
   uint16_t      integerPart = 0;
   uint16_t      fracPart = 0;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_fngForwardPowerSet;

   if ( argc == 2 )
   {
      (void)sscanf( argv[1], "%hu.%1hu", &integerPart, &fracPart );
      forwardpowerset = ( integerPart * 10U ) + fracPart;
      (void)PHY_SetRequest( eAttribute, &forwardpowerset);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %hu.%hu dBm\n", argv[ 0 ], GetConf.val.fngForwardPowerSet/10U, GetConf.val.fngForwardPowerSet%10U );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFGP_fngFowardPowerLowSet

   Purpose: Get/Set Foward Power Low Set Point

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_fngFowardPowerLowSet( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint16_t      fowardpowerlowset;
   uint16_t      integerPart = 0;
   uint16_t      fracPart = 0;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_fngFowardPowerLowSet;

   if ( argc == 2 )
   {
      (void)sscanf( argv[1], "%hu.%1hu", &integerPart, &fracPart );
      fowardpowerlowset = ( integerPart * 10U ) + fracPart;
      (void)PHY_SetRequest( eAttribute, &fowardpowerlowset);
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %hu.%hu dBm\n", argv[ 0 ], GetConf.val.fngFowardPowerLowSet/10U, GetConf.val.fngFowardPowerLowSet%10U );
   }
   else
   {
      Service_Unavailable();
   }
}
/***********************************************************************************************************************
   Function Name: MFGP_fngFemMfg

   Purpose: Get/Set external FEM manufacturer

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_fngFemMfg( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t  GetConf;
   uint32_t       input;
   uint8_t        mfg;

   PHY_ATTRIBUTES_e eAttribute = ePhyAttr_fngfemMfg;

   if ( argc == 2 )
   {
      input = strtoul( argv[1], NULL, 0 );
      if ( input <= MICRO_ANT )
      {
         mfg = (uint8_t)input;
         (void)PHY_SetRequest( eAttribute, &mfg);
      }
   }

   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %hhu\n", argv[ 0 ], GetConf.val.fngfemMfg );
   }
   else
   {
      Service_Unavailable();
   }
}
#endif   /* #if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) ) */
#endif

/***********************************************************************************************************************
   Function Name: MFG_StTxCwTest

   Purpose: MFG port interface to put the transmitter into CW mode, at the requested frequency

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_StTxCwTest( uint32_t argc, char *argv[] )
{
   char     *endptr;
   uint32_t  CWFreq;
   uint16_t  radio_channel;

   if( argc == 2 )      /* There MUST be one additional argument on the cmd line */
   {
      CWFreq = strtoul( argv[1], &endptr, 10 );
      if ( ( CWFreq >= PHY_MIN_FREQ ) && ( CWFreq <= PHY_MAX_FREQ ) )
      {
         radio_channel = ( uint16_t )FREQ_TO_CHANNEL( CWFreq ); /* Convert frequency into a channel */
      }
      else
      {
         radio_channel = PHY_INVALID_CHANNEL;
      }
      (void)MFG_PhyStTxCwTest( radio_channel );
   }
}

static void StTxCwTestCompletedTimerExpired(uint8_t cmd, void *pData)   /*lint !e818 pData could be ptr to const  */
{
   (void)cmd;
   (void)pData;

   uStTxCwTestCompleteTimerId = 0;
   RADIO_Mode_Set( eRADIO_MODE_NORMAL );
}

/***********************************************************************************************************************
   Function Name: MFG_PhyStTxCwTest

   Purpose: General function to put the transmitter into CW mode, at the requested frequency

   Arguments:
      radio_channel - Channel to use for CW Tx

   Returns: returnStatus_t
   **********************************************************************************************************/
returnStatus_t MFG_PhyStTxCwTest( uint16_t radio_channel )
{
   timer_t        tmrSettings;
   returnStatus_t retVal = eFAILURE;

   // Delete timer if running
   if ( uStTxCwTestCompleteTimerId )
   {
      (void)TMR_DeleteTimer(uStTxCwTestCompleteTimerId);
      uStTxCwTestCompleteTimerId = 0;
   }

   (void)PHY_Channel_Set( 0, radio_channel );      /* Make sure this frequency is in slot 0 of PHY available channels */
   (void)PHY_TxChannel_Set( 0, radio_channel );    /* Make sure this frequency is in slot 0 of tx channels  */
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
   (void) MAC_StopRequest( NULL );
   OS_TASK_Sleep ( QTR_SEC );
#endif
   RADIO_Mode_Set( radio_channel == PHY_INVALID_CHANNEL ? eRADIO_MODE_NORMAL : eRADIO_MODE_CW );

   // Run test 10 seconds when valid channel
   if ( radio_channel != PHY_INVALID_CHANNEL )
   {
      (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
      tmrSettings.bOneShot       = true;
      tmrSettings.bOneShotDelete = true;
      tmrSettings.bExpired       = false;
      tmrSettings.bStopped       = false;
      tmrSettings.ulDuration_mS  = 10 * ONE_SEC;
      tmrSettings.pFunctCallBack = StTxCwTestCompletedTimerExpired;
      if (eSUCCESS == TMR_AddTimer(&tmrSettings))
      {
         uStTxCwTestCompleteTimerId = (uint16_t)tmrSettings.usiTimerId;
         retVal = eSUCCESS;
      }
   }
   return (retVal);
}

/***********************************************************************************************************************
   Function Name: MFG_StRx4GFSK

   Purpose: Mfg Port interface to put the receiver into RX mode at the requested frequency

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_StRx4GFSK( uint32_t argc, char *argv[] )
{
   char     *endptr;
   uint32_t CWFreq;
   uint16_t radio_channel;

   if( argc == 2 )      /* There MUST be one additional argument on the cmd line */
   {
      CWFreq = strtoul( argv[1], &endptr, 10 );
      if ( ( CWFreq >= PHY_MIN_FREQ ) && ( CWFreq <= PHY_MAX_FREQ ) )
      {
         radio_channel = ( uint16_t )FREQ_TO_CHANNEL( CWFreq ); /* Convert frequency into a channel */
      }
      else
      {
         radio_channel = PHY_INVALID_CHANNEL;
      }
      MFG_PhyStRx4GFSK( radio_channel );
   }
}

/***********************************************************************************************************************
   Function Name: MFG_PhyStRx4GFSK

   Purpose: General function to put the receiver into RX mode at the requested frequency

   Arguments:
      radio_channel - Radio channel to Rx

   Returns: none
***********************************************************************************************************************/
void MFG_PhyStRx4GFSK( uint16_t radio_channel )
{
   uint8_t  radioNum;

   (void)PHY_Channel_Set( 0, radio_channel );      /* Make sure this frequency is in slot 0 of PHY available channels  */
   /* Populate all RX radios */
   for (radioNum=(uint8_t)RADIO_FIRST_RX; radioNum<(uint8_t)MAX_RADIO; radioNum++)
   {
      (void)PHY_RxChannel_Set( radioNum, radio_channel );
   }
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
   (void)MAC_StopRequest( NULL );
   OS_TASK_Sleep ( QTR_SEC );
#endif
   RADIO_Mode_Set( radio_channel == PHY_INVALID_CHANNEL ? eRADIO_MODE_NORMAL : eRADIO_MODE_NORMAL_BER );
}

/***********************************************************************************************************************
   Function Name: MFG_StTxBlurtTest_Srfn

   Purpose: Send a message to test Tx

   Arguments:
      none

   Returns: void
***********************************************************************************************************************/
void MFG_StTxBlurtTest_Srfn( void )
{
   uint8_t   data[] =
   {
      0x00, 0xA6, 0x02, 0x00,
      0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89,
      0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89,
      0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89,
      0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89,
      0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89,
      0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89,
      0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89,
      0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89,
      0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89,
      0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89
   };

   COMM_Tx_t tx_data;

   tx_data.dst_address.addr_type         = eEXTENSION_ID;
   tx_data.dst_address.ExtensionAddr[0]  = 0x11;
   tx_data.dst_address.ExtensionAddr[1]  = 0x22;
   tx_data.dst_address.ExtensionAddr[2]  = 0x33;
   tx_data.dst_address.ExtensionAddr[3]  = 0x44;
   tx_data.dst_address.ExtensionAddr[4]  = 0x55;
   tx_data.length                        = sizeof( data );
   tx_data.data                          = data;
   tx_data.callback                      = NULL;
   tx_data.qos                           = 0;
   tx_data.override.linkOverride         = eNWK_LINK_OVERRIDE_NULL;
   tx_data.override.linkSettingsOverride = eNWK_LINK_SETTINGS_OVERRIDE_NULL;
   COMM_transmit( &tx_data );
}

#if ( DCU == 1 )
/***********************************************************************************************************************
   Function Name: MFG_StTxBlurtTest_Star

   Purpose: Sends a Star Blur Message

   Returns: void
***********************************************************************************************************************/
static void MFG_StTxBlurtTest_Star( void )
{
   // Equivalent to !1A8031B3150000AAAA00 which is a ping command to 5555 MTU.
   uint8_t data[] = { '1',  'A',  '8',  '0',  '3',  '1',  'B',  '3',  '1',  '5',  '0',  '0',  '0',  '0',  'A',  'A',  'A',  'A',  '0' };
// uint8_t data[] = { 0x31, 0x61, 0x38, 0x30, 0x33, 0x31, 0x62, 0x33, 0x31, 0x35, 0x30, 0x30, 0x30, 0x30, 0x61, 0x61, 0x61, 0x61, 0 };
   (void)STAR_mod_now( data );
}
#endif

/***********************************************************************************************************************
   Function Name: MFG_StTxBlurtTest

   Purpose: Handles a Star Blurt Request
            sttxblurttest STAR
            sttxblurttest SRFN
   Returns: void
***********************************************************************************************************************/
static void MFG_StTxBlurtTest( uint32_t argc, char *argv[] )
{
#if ( DCU == 1 )
   if ( argc == 2 )
   {
      if ( strncasecmp((const char*)argv[1], ComDeviceGatewayConfigSrfn, COM_DEVICE_GATEWAY_CONFIG_LEN) == 0)
      {  // SRFN
         MFG_StTxBlurtTest_Srfn();
      }
      else if ( strncasecmp((const char*)argv[1], ComDeviceGatewayConfigStar, COM_DEVICE_GATEWAY_CONFIG_LEN) == 0)
      {  // STAR
         MFG_StTxBlurtTest_Star();
      }
   } else
#endif
   {
      // Default to SRFN
      MFG_StTxBlurtTest_Srfn();
   }
}

#if ( ENABLE_HMC_TASKS == 1 )
/***********************************************************************************************************************
   Function Name: MFG_bulkQuantity

   Purpose: Read the fwdkWh from the host meter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFG_bulkQuantity ( uint32_t argc, char *argv[] )
{
   ValueInfo_t  reading;
   //CimValueInfo_t       rdgPair;
   double               vald;
   uint64_t             integer;       /* Integer portion of rdgPair.Value */
   uint32_t             len;
   uint32_t             power10;       /* pow10 converted to 10 ^ pow10 */
   uint32_t             logVal;        /* (int)log( rdgPair.Value ) */
   uint8_t              pow10;
   enum_CIM_QualityCode rdgStatus;
   char                 outInt[21];

   // initialize rdgPair
   (void)memset( (uint8_t *)&reading, 0, sizeof(reading) );

   /*Read the host meter for the current fwdkWh value */
   if( 0 == strcasecmp( argv[0], "0.0.0.1.1.1.12.0.0.0.0.0.0.0.0.3.72.0" ) ||
       0 == strcasecmp( argv[0], "fwdkWh" ) )
   {
      rdgStatus = INTF_CIM_CMD_getMeterReading( fwdkWh, &reading );
   }
   else if( 0 == strcasecmp( argv[0], "0.0.0.1.20.1.12.0.0.0.0.0.0.0.0.3.72.0" ) ||
            0 == strcasecmp( argv[0], "totkWh" ) )
   {
      rdgStatus = INTF_CIM_CMD_getMeterReading( totkWh, &reading );
   }
   else if( 0 == strcasecmp( argv[0], "0.0.0.1.4.1.12.0.0.0.0.0.0.0.0.3.72.0" ) ||
            0 == strcasecmp( argv[0], "netkWh" ) )
   {
      rdgStatus = INTF_CIM_CMD_getMeterReading( netkWh, &reading );
   }
   else
   {
      rdgStatus = CIM_QUALCODE_ERROR_CODE;
   }

   if ( CIM_QUALCODE_SUCCESS == rdgStatus )
   {
      pow10 = reading.power10;
      power10 = ( uint32_t )pow( ( double )10, ( double )pow10 );
      integer = ( uint64_t )( ( double )reading.Value.intValue / power10 );
      MFG_logPrintf( "%s ", argv[ 0 ] );
      len  = ( uint32_t )sprintf( outInt, "%llu.", integer );
      MFG_logPrintf( "%s", outInt );
      logVal = ( uint32_t )log10( ( double )reading.Value.intValue ) + 1;
      if( logVal >= pow10 )
      {
         len = logVal - ( len - 1 );   /* Take the '.' out of the length   */
      }
      else
      {
         len = pow10;   /* All the digits are after the decimal point. Need leading zeros */
      }
      /* Find the fractional portion of rdgPair.ValTime.Value  */
      vald = ( double )reading.Value.intValue;
      reading.Value.intValue = ( int64_t )( vald - ( ( double )integer * power10 ) );
      MFG_logPrintf( "%0.*llu\n", len, reading.Value.intValue );
   }
   else
   {
      MFG_logPrintf( "%s failed: %d\n", argv[ 0 ], ( int32_t )rdgStatus );
   }
}
#endif

/*******************************************************************************

   Function name: printCert

   Purpose: Helper function to print out cert data from ROM

   Arguments:  argv - pointer to the list of arguments passed to this function
               uint8_t *cert    - pointer to cert data (in ROM)
               uint16_t len     - maximum length

               Expect a fully formed ASCII representation of a .der format certificate.

   Returns: none

   Notes:

*******************************************************************************/
static void printCert( char **argv, uint8_t *cert, uint16_t len )
{
   uint8_t           numBytes;      /* Use to compute length of cert for printing */
   uint8_t           *pCert;        /* Pointer to next byte in cert */
   uint32_t          certLen = 0;   /* Computed length of cert for printing */
   buffer_t          *certBuf;      /* Holds full cert, ready to print as one string. */

   MFG_logPrintf( "%s ", argv[ 0 ] );
   if ( cert[ 0 ] == 0x30 )
   {
      if ( ( cert[ 1 ] & 0x80 ) != 0 )
      {
         numBytes = cert[ 1 ] & (uint8_t)~0x80;
         pCert = &cert[ 2 ];
      }
      else
      {
         numBytes = 1;
         pCert = &cert[ 1 ];
      }
      while ( numBytes-- != 0 )    /* Loop through the length field of the cert */
      {
         certLen = ( certLen << 8 ) + *pCert++;
      }
      certLen += ( uint32_t )( pCert - cert ); /* Account for tag/tag-length fields */

      len = ( uint16_t )min( len, certLen ); /* Finalize the count to the cert length, if that is appropriate. */
      certBuf = BM_alloc( (uint16_t)( len * 2U ) + 1U );
      if ( certBuf != NULL )
      {
         pCert = certBuf->data;
         for( uint16_t j = 0; j < len; j++ )
         {
            pCert += snprintf( (char *)pCert, certBuf->x.dataLen - ( pCert - certBuf->data ), "%02x", cert[ j ] );
         }
         MFG_logPrintf( "%s", certBuf->data );
         BM_free( certBuf );
      }
   }
   MFG_logPrintf( "\n" );
}
/*******************************************************************************

   Function name: MFGP_dtlsHelper

   Purpose: Helper function to set/get various dtls information kept in NV memory

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function
               uint16_t offset  - offset into file
               uint16_t len     - file length

               Expect a fully formed ASCII representation of a .der format certificate. Store in proper file, which
               currently defined as being in the micro's internal flash.

   Returns: none

   Notes:

*******************************************************************************/
static void MFGP_dtlsHelper( uint32_t argc, char *argv[], uint16_t offset, uint16_t len )
{
   uint8_t           cert[ sizeof( NetWorkRootCA_t ) ]; /* Upper limit of storage for certificate */
   char              *pASCII;                           /* Pointer to next ASCII character pair in argv[1] */
   uint8_t           *pCert;                            /* Pointer to next byte in cert                    */
   uint16_t          i;                                 /* Loop counter, used to limit number of characters received  */
   PartitionData_t   const *pPart;                      /* Used to access the security info partition  */

   pPart = SEC_GetSecPartHandle();
   // Read the data from secROM
   if ( eSUCCESS == PAR_partitionFptr.parRead( cert, ( dSize )offset, ( lCnt )len, pPart ) )
   {
      i = len;
      if( argc == 2 )   /* User wants to update the certificate   */
      {
         if ( ( strlen( argv[1] ) % 2 ) == 0 )
         {
            ( void )memset( cert, 0, sizeof( cert ) );
            /* Loop through the ASCII character pairs in argv[1]  */
            for( i = 0, pASCII = argv[ 1 ], pCert = cert; i < sizeof( cert ); i++ )
            {
               if ( *pASCII == '\0' )
               {
                  break;      /* End of input reached */
               }
               ( void )sscanf( pASCII, "%02hhx", pCert );
               pCert++;
               pASCII += 2;
            }
         }
         /* Update the file in NV   */
         //TODO: Take Pwr_mutex
         if ( eSUCCESS != PAR_partitionFptr.parWrite( ( dSize )offset, cert, ( lCnt )len, pPart ) )
         //TODO: Release Pwr_mutex
         {
            MFG_logPrintf( "Error writing NV\n" );
         }
      }
      /* Print results
         At this point, i is either the number of bytes read originally, or the number of bytes written, up to
         sizeof(NetWorkRootCA_t)
      */
      printCert( argv, ( uint8_t * )( pPart->PhyStartingAddress + offset ), i );
   }
}
/*******************************************************************************

   Function name: MFGP_dtlsNetworkRootCA

   Purpose: Read/Store the dtlsNetworkRootCA

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

               Expect a fully formed ASCII representation of a .der format certificate. Store in proper file, which
               currently defined as being in the micro's internal flash.

   Returns: none

   Notes:

*******************************************************************************/
static void MFGP_dtlsNetworkRootCA( uint32_t argc, char *argv[] )
{
   MFGP_dtlsHelper( argc, argv, ( uint16_t )offsetof( intFlashNvMap_t, dtlsNetworkRootCA ), sizeof( NetWorkRootCA_t ) );
}
/*******************************************************************************

   Function name: MFGP_dtlsDeviceCertificate

   Purpose: Read/Store the dtlsNetworkRootCA

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

               Dump the computed device certificate;

   Returns: none

   Notes:

*******************************************************************************/
static void MFGP_dtlsDeviceCertificate( uint32_t argc, char *argv[] )
{
   uint32_t certLen;
   uint8_t  response[ 512 ];     /* Value returned by some ecc108e routines   */

   if( ECC108_SUCCESS == ecc108e_GetDeviceCert( DeviceCert_e, response, &certLen ) )
   {
      printCert( argv, response, ( uint16_t )certLen );
   }
}
/*******************************************************************************

   Function name: MFGP_dtlsNetworkMSSubject

   Purpose: Read/Store the dtlsNetworkMSSubject

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

               Expect a fully formed ASCII representation of a .der format certificate. Store in proper file, which
               currently defined as being in the micro's internal flash.

   Returns: none
*******************************************************************************/
static void MFGP_dtlsNetworkMSSubject( uint32_t argc, char *argv[] )
{
   MFGP_dtlsHelper( argc, argv, ( uint16_t )offsetof( intFlashNvMap_t, dtlsMSSubject ), sizeof( MSSubject_t ) );
}
/*******************************************************************************

   Function name: MFGP_dtlsMfgSubject1

   Purpose: Read the dtlsMfgSubject1

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none
*******************************************************************************/
static void MFGP_dtlsMfgSubject1( uint32_t argc, char *argv[] )
{
   printCert( argv, ( uint8_t * )ROM_dtlsMfgSubject1, sizeof( MSSubject_t ) );
}
/*******************************************************************************

   Function name: MFGP_dtlsMfgSubject2

   Purpose: Read/Store the dtlsMfgSubject2

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

               Expect a fully formed ASCII representation of a .der format certificate. Store in proper file, which
               currently defined as being in the micro's internal flash.

   Returns: none
*******************************************************************************/
static void MFGP_dtlsMfgSubject2( uint32_t argc, char *argv[] )
{
   MFGP_dtlsHelper( argc, argv, ( uint16_t )offsetof( intFlashNvMap_t, dtlsMfgSubject2 ), sizeof( MSSubject_t ) );
}
/*******************************************************************************

   Function name: MFGP_dtlsNetworkHESubject

   Purpose: Read/Store the dtlsNetworkHESubject

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

               Expect a fully formed ASCII representation of a .der format certificate. Store in proper file, which
               currently defined as being in the micro's internal flash.

   Returns: none

   Notes:

*******************************************************************************/
static void MFGP_dtlsNetworkHESubject( uint32_t argc, char *argv[] )
{
   MFGP_dtlsHelper( argc, argv, ( uint16_t )offsetof( intFlashNvMap_t, dtlsHESubject ), sizeof( HESubject_t ) );
}

/*******************************************************************************

   Function name: MFG_appSecAuthMode

   Purpose: Read/set the system level application security mode

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

*******************************************************************************/
static void MFG_appSecAuthMode ( uint32_t argc, char *argv[] )
{
   char              *endptr;
   uint8_t           val;

   if( argc == 2 ) /* User wants to set the security mode */
   {
      /* Get the user's input value */
      val = ( uint8_t )strtoul( argv[1], &endptr, 10 );
      (void)APP_MSG_SecurityHandler( method_put, appSecurityAuthMode, &val, NULL );
   }
   (void)APP_MSG_SecurityHandler( method_get, appSecurityAuthMode, &val, NULL );
   MFG_logPrintf( "%s %d\n", argv[ 0 ], val );
}

/*******************************************************************************

   Function name: MFG_enableDebug

   Purpose: Enable/disable the debug port

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

*******************************************************************************/
static void MFG_enableDebug ( uint32_t argc, char *argv[] )
{
   char * endptr;
   uint32_t val;
   bool     echo=true;
   if ( argc > 1 )   /* User wants to change the setting */
   {
      // Get the port timeout value
      // It has a range of 0 to 255
      val = strtoul( argv[1], &endptr, 10 );
      if ( val < 256)
      {
         DBG_PortTimer_Set(( uint8_t) val);
      }
      if ( argc > 2 )
      {
         echo = (bool)strtoul( argv[2], &endptr, 10 );
      }
      DBG_PortEcho_Set( echo );
   }
   /* Always print read back value  */
   MFG_logPrintf( "%s %u %u\n", argv[ 0 ], DBG_PortTimer_Get(), DBG_PortEcho_Get() );
}

#if (TX_THROTTLE_CONTROL == 1 )
/*******************************************************************************

   Function name: MFG_TxThrottleDisable

   Purpose: Enable/disable the debug port

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

*******************************************************************************/
static void MFG_TxThrottleDisable ( uint32_t argc, char *argv[] )
{
   uint32_t val;
   if ( argc > 1 )   /* User wants to change the setting */
   {
      // Get the requested timeout value
      // It has a range of 0 to 255
      val = strtoul( argv[1], NULL, 0 );
      if ( val < 256)
      {
         MAC_TxThrottleTimerManage( (uint8_t)val );
      }
   }
   /* Always print read back value  */
   MFG_logPrintf( "%s %u minutes\n", argv[ 0 ], MAC_GetTxThrottleTimer() );
}
#endif


#if ( EP == 1 )
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
/*******************************************************************************

   Function name: MFG_logoff

   Purpose: Reset the UART used for manufacturing port (i.e., log off the optical port. )

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

*******************************************************************************/
static void MFG_logoff ( uint32_t argc, char *argv[] )
{
   uint32_t ioctlFlags;

   if ( loggedOn )   /* No action necessary if not logged on to optical port. */
   {
      (void)UART_write( mfgUart, (uint8_t *)"OK\n", 3 );
      HMC_COMM_SENSE_OFF();               // Let meter know we are done with the optical port
      mfgUart = UART_MANUF_TEST;
      MFGP_DtlsInit( mfgUart );           /* Let dtls serial task know to use optical port.  */

      ioctlFlags = (uint32_t)UART_SWITCH_CHAR;  /* Send a UART_SWITCH_CHAR to the MFG Uart port. */
      ( void )UART_ioctl( UART_OPTICAL_PORT, (int32_t)IO_IOCTL_SERIAL_FORCE_RECEIVE, &ioctlFlags );

      loggedOn = (bool) false;
   }
}
#endif
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
/*******************************************************************************

   Function name: MFG_meterPassword

   Purpose: Set the password used to log onto the meter

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

*******************************************************************************/
static void MFG_meterPassword ( uint32_t argc, char *argv[] )
{
   uint8_t  password[ 20 ];      /* Raw password - converted from ASCII */
   uint8_t  textStr[ 41 ];       /* Text password - converted from Hex, includes terminator */
   uint32_t len;                 /* Length of incoming password in hex bytes */
   uint8_t  *pwd;                /* Pointer for storing converted bytes */
   uint16_t i;                   /* Loop */

   if( argc > 1 ) /* User wants to set the security mode */
   {
      len = strlen( argv[ 1 ] )/2;  /* Allow for 2 ASCII characters per hex byte  */
      if ( len <= sizeof( password ) )
      {
         ( void )memset( password, 0, sizeof( password ) );
         pwd = password;
         for ( i = 0; i < len; i++ )
         {
            ( void )sscanf( &argv[ 1 ][ i * 2 ], "%02hhx", pwd );
            pwd++;
         }
         ( void )HMC_STRT_SetPassword( password, (uint16_t)sizeof(password) );
         ( void )memset( password, 0, sizeof( password ) );
         ( void )memset( textStr, 0, sizeof( textStr ) ); // Ensures a string terminator
         ( void )HMC_STRT_GetPassword( password );
         pwd = password;
         for ( i = 0; i < len; i++ )
         {
            ASCII_htoa(&textStr[ i * 2 ], *pwd);
            pwd++;
         }
         MFG_logPrintf( "%s %s\n", argv[ 0 ], &textStr[0] );
      }
   }
}

/*******************************************************************************

   Function name: MFGP_passwordPort0Master

   Purpose: Set the master password used to log onto the meter for elevated
            access rights

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

*******************************************************************************/
static void MFGP_passwordPort0Master ( uint32_t argc, char *argv[] )
{
   uint8_t  password[ 20 ];      /* Raw password - converted from ASCII */
   uint8_t  textStr[ 41 ];       /* Text password - converted from Hex, includes terminator */
   uint32_t len;                 /* Length of incoming password in hex bytes */
   uint8_t  *pwd;                /* Pointer for storing converted bytes */
   uint16_t i;                   /* Loop */

   if( argc > 1 ) /* User wants to set the security mode */
   {
      len = strlen( argv[ 1 ] )/2;  /* Allow for 2 ASCII characters per hex byte  */
      if ( len <= sizeof( password ) )
      {
         ( void )memset( password, 0, sizeof( password ) );
         pwd = password;
         for ( i = 0; i < len; i++ )
         {
            ( void )sscanf( &argv[ 1 ][ i * 2 ], "%02hhx", pwd );
            pwd++;
         }
         ( void )HMC_STRT_SetMasterPassword( password, (uint16_t)sizeof(password) );
         ( void )memset( password, 0, sizeof( password ) );
         ( void )memset( textStr, 0, sizeof( textStr ) ); // Ensures a string terminator
         ( void )HMC_STRT_GetMasterPassword( password );
         pwd = password;
         for ( i = 0; i < len; i++ )
         {
            ASCII_htoa(&textStr[ i * 2 ], *pwd);
            pwd++;
         }
         MFG_logPrintf( "%s %s\n", argv[ 0 ], &textStr[0] );
      }
   }
}
#endif // endif of (ACLARA_LC == 0)
#endif

/***********************************************************************************************************************
 *
 * Function Name: MFG_reboot
 *
 * Purpose: Reboot the unit
 *
 * Arguments: None
 *
 * Returns: None
 *
 * Side Effects: None
 *
 * Reentrant Code: NO
 *
 **********************************************************************************************************************/
static void MFG_reboot ( uint32_t argc, char *argv[] )
{
   PWR_SafeReset();  /* Execute Software Reset, with cache flush */
}

/***********************************************************************************************************************
 *
 * Function Name: MFGP_virgin
 *
 * Purpose: Virgin the unit
 *
 * Arguments: None
 *
 * Returns: None
 *
 * Side Effects: None
 *
 * Reentrant Code: NO
 *
 **********************************************************************************************************************/
static void MFGP_virgin( uint32_t argc, char *argv[] )
{
   ( void ) DBG_CommandLine_virgin ( 0, NULL );
}
/*******************************************************************************

   Function name: MFGP_virginDelay

   Purpose: Erases signature and continues to run. Allows new code load and automatic virgin on next reboot.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static void MFGP_virginDelay ( uint32_t argc, char *argv[] )
{
   PartitionData_t const * partitionData;    /* Pointer to partition information   */
   uint8_t                 erasedSignature[ 8 ];

   /* Open the partition with the signature  */
   if ( eSUCCESS ==  PAR_partitionFptr.parOpen( &partitionData, ePART_NV_VIRGIN_SIGNATURE, 0xffffffff ) )
   {
      // Erase signature
      ( void )memset( erasedSignature, 0, sizeof( erasedSignature ) );
      if ( eSUCCESS == PAR_partitionFptr.parWrite( 0, erasedSignature, sizeof( erasedSignature ), partitionData ) )
      {
         MFG_printf( "Signature erased. Device will be made virgin on next reboot!\n" );
      }
   }
}

#if (USE_DTLS == 1)
/***********************************************************************************************************************
 *
 * Function Name: MFGP_DtlsConnectionClosed
 *
 * Purpose: This function is called when the DTLS server closes the connection.
 *
 * Arguments: None
 *
 * Returns: None
 *
 * Side Effects: None
 *
 * Reentrant Code: NO
 *
 **********************************************************************************************************************/
void MFGP_DtlsConnectionClosed( void )
{
   buffer_t *commandBuf;
   uint16_t numBytes;
   const uint8_t cmd[] = "disconnectDtls";
   uint8_t tries;

   numBytes = sizeof( cmd ) + 1;

   for ( tries = 0; tries < MAX_DTLS_DISCONNECT_TRIES; tries++ )
   {
      commandBuf = ( buffer_t * )BM_alloc( numBytes );
      if ( commandBuf )
      {
         commandBuf->data[numBytes] = 0; /* Null terminating string */
         ( void )memcpy( commandBuf->data, cmd, sizeof( cmd ) );

         /* Call the command */
         OS_MSGQ_Post( &_CommandReceived_MSGQ, commandBuf ); // Function will not return if it fails
         /* Free up any pending DTLS command */
         OS_EVNT_Set( &_MfgpUartEvent, MFGP_EVENT_COMMAND );
         break;
      }
      else
      {
         ERR_printf( "MFGP_DtlsConnectionClosed failed to create command buffer, ignoring command" );
      }
      OS_TASK_Sleep( 1000 * 10 );
   }
}

/*******************************************************************************

   Function name: MFG_disconnectDtls

   Purpose: The purpose of this function will notify the MFG task to
            disconnect the DTLS and reset the port state.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

*******************************************************************************/
static void MFG_disconnectDtls ( uint32_t argc, char *argv[] )
{
   if ( _MfgPortState == DTLS_SERIAL_IO_e )
   {
#if ( MCU_SELECTED == NXP_K24 )// TODO: RA6E1 Enable UART ioctl (check if required)
      int32_t flags = IO_SERIAL_TRANSLATION | IO_SERIAL_ECHO; /* Settings for the UART */
#endif
      /* This could be caused by the user closing the connection, or the time out value exceeded. If timing out, there's
         a DTLS message sent (close notify) that isn't being handled. Pause this task to allow the UART to receive the
         entire message, before flusing the UART.  */
      OS_TASK_Sleep( 30 );       /* Wait for close notify message to be received.   */

#if ( MCU_SELECTED == NXP_K24 )// TODO: RA6E1 Enable UART ioctl and UART rx (check if required)
#if !USE_USB_MFG
      UART_RX_flush( mfgUart );      // TODO: RA6E1 This UART_flush not needed now for the debug and mfg port. Might be added in the future.
      ( void )UART_ioctl( mfgUart, (int32_t)IO_IOCTL_SERIAL_SET_FLAGS, &flags );
#else
      usb_flush();
      usb_ioctl( ( MQX_FILE_PTR )( uint32_t )mfgUart, ( int32_t )IO_IOCTL_SERIAL_SET_FLAGS, &flags );
#endif
#endif
      ( void )memset( MFGP_CommandBuffer, 0, sizeof( MFGP_CommandBuffer ) );
      MFGP_numBytes = 0;

#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
      MFG_logoff ( 0, NULL );
#endif

      _MfgPortState = MFG_SERIAL_IO_e;
      INFO_printf( "MFGP_DtlsConnectionClosed called cleaning up port." );
   }
}

/*******************************************************************************

   Function name: MFG_startDTLSsession

   Purpose: The purpose of this function is to connect the MFG port to a DTLS
            client.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none

   Notes:

*******************************************************************************/
static void MFG_startDTLSsession ( uint32_t argc, char *argv[] )
{
   /* Make sure that only one start is active */
   if ( _MfgPortState == MFG_SERIAL_IO_e )
   {
//      uint32_t flags = 0;
#if !USE_USB_MFG
#if 0 // TODO: RA6E1 Enable UART ioctl (check if required)
      (void)UART_ioctl( mfgUart, (int32_t)IO_IOCTL_SERIAL_SET_FLAGS, &flags );
#endif
#else
      usb_ioctl( (MQX_FILE_PTR)(uint32_t)mfgUart, IO_IOCTL_SERIAL_SET_FLAGS, &flags );
#endif
      if ( eSUCCESS == DTLS_UartConnect( MFGP_DtlsResultsCallback ) )
      {
         _MfgPortState = DTLS_SERIAL_IO_e;
      }
   }
   else
   {
      ERR_printf( "DTLS Already connected ignoring request" );
   }
}

/*******************************************************************************

   Function name: MFGP_ProcessDecryptedCommand

   Purpose: This function is called to process a command that was decrypted
            by the DTLS.

   Arguments: buffer_t *bfr        - Buffer with decrypted command.

   Returns: none

   Notes: Calling function will free up *bfr

*******************************************************************************/
/*lint -esym(818,bfr) */
void MFGP_ProcessDecryptedCommand( buffer_t *bfr )
{
   buffer_t *commandBuf;

   /* Make sure the buffer exists */
   if ( !bfr )
   {
      ERR_printf( "Buffer supplied is NULL while trying to process a decrypted command" );
      return;
   }

   commandBuf = ( buffer_t * )BM_alloc( bfr->x.dataLen + 1 );
   if ( commandBuf != NULL )
   {
      commandBuf->data[bfr->x.dataLen] = 0; /* Null terminating string */
      ( void )memcpy( commandBuf->data, bfr->data, bfr->x.dataLen );
#if ( DTLS_DEBUG == 1 )
      DBG_logPrintf( 'I', "%s: %s", __FUNCTION__, commandBuf->data );
#endif

      /* Call the command */
      OS_MSGQ_Post( &_CommandReceived_MSGQ, commandBuf ); // Function will not return if it fails
   } else {
      ERR_printf( "Failed to create command buffer" );
   }

   return;
}
/*lint +esym(818,bfr) */

/*******************************************************************************

   Function name: MFGP_GetEventHandle

   Purpose: This function is called to get the event handle.

   Arguments: None

   Returns: OS_EVNT_Obj the event handle

   Notes: The reason for this function is to make as much of the DTLS processing
         in the MFG_Dtls.c file.

*******************************************************************************/
OS_EVNT_Obj *MFGP_GetEventHandle( void )
{
   return &_MfgpUartEvent;
}

#endif
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#if ( EP == 1 )
/*******************************************************************************
 *
 *  Function name: MFGP_historicalRecovery
 *
 *  Purpose: Get status of whether endpoint supports historical recovery
 *
 *  Arguments:
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_historicalRecovery( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %d\n", argv[ 0 ], HD_getHistoricalRecovery() );
}
#endif
#endif // endif of (ACLARA_LC == 0)
#if EP == 1
/*******************************************************************************
 *
 *  Function name: MFGP_ConfigGet
 *
 *  Purpose: Used to compute the crc32 for the device configuration
 *
 *  Arguments:
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_ConfigGet( uint32_t argc, char *argv[] )
{
   (void) argc;
   (void) argv;

   uint32_t crc32 = FIO_ConfigGet( ) ;
   MFG_printf( "%s 0x%08lx\n", argv[ 0 ], crc32 );
}

#if ( ENABLE_DEMAND_TASKS == 1 )
/*******************************************************************************
 *
 *  Function name: MFGP_demandResetLockoutPeriod
 *
 *  Purpose: Get/Set the demandResetLockoutPeriod
 *
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_demandResetLockoutPeriod( uint32_t argc, char *argv[] )
{
   uint32_t resetTimeout;

   if ( argc <= 2 )
   {
      if (argc == 2)
      {
         resetTimeout = ( uint32_t )atol( argv[1] );
         ( void )DEMAND_SetTimeout( resetTimeout );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   DEMAND_GetTimeout( &resetTimeout );
   MFG_logPrintf( "%s %d\n", argv[ 0 ], resetTimeout );
}
#endif // endif of ( ENABLE_DEMAND_TASKS == 1 )

/*******************************************************************************
 *
 *  Function name: MFGP_edFwVersion
 *
 *  Purpose: Get the firmware version of the endDevice(meter)
 *
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_edFwVersion( uint32_t argc, char *argv[] )
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edFwVersion, &readingInfo);
   MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
}

#if ( ANSI_STANDARD_TABLES == 1 ) || ( ACLARA_LC == 1 )
/*******************************************************************************
 *
 *  Function name: MFGP_edHwVersion
 *
 *  Purpose: Get the hardware version of the endDevice(meter)
 *
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_edHwVersion( uint32_t argc, char *argv[] )
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edHwVersion, &readingInfo );
   MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
}
/*******************************************************************************
 *
 *  Function name: MFGP_edModel
 *
 *  Purpose: Get the model of the meter
 *
 *  Arguments:
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_edModel( uint32_t argc, char *argv[] )
{
#if ( ANSI_STANDARD_TABLES == 1 )
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edModel, &readingInfo );
   MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
#else /* ACLARA_LC   */

   if ( argc == 2 )
   {
      // Example edModel 255.255

      uint8_t data[2] = {0};

      char *pt;
      pt = strtok (argv[1],".");
      int numValues = 0;
      while (pt != NULL)
      {
         int a = atoi(pt);

         data[numValues] = (uint8_t) a;
         numValues++;
         pt = strtok (NULL, ".");
      }

      if ( CIM_QUALCODE_SUCCESS != ILC_DRU_DRIVER_writeDruRegister( ILC_DRU_DRIVER_REG_MODEL, ILC_DRU_DRIVER_REG_MODEL_SIZE, data, 2000 ) )
      {
         INFO_printf( "Unsuccessful DRU Write Reg" );
      }
   }

   {
      // Get the Type/Model as a string
      ValueInfo_t readingInfo;
      (void)INTF_CIM_CMD_getMeterReading( edModel, &readingInfo );
      MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
   }
#endif
}

#elif (ACLARA_DA == 1)

/**
Get the hardware version of the endDevice(host)

@param argc Number of Arguments passed to this function
@param argv pointer to the list of arguments passed to this function
*/
static void MFGP_edHwVersion(uint32_t argc, char *argv[])
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edHwVersion, &readingInfo );
   MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
}

/**
Get the model of the host

@param argc Number of Arguments passed to this function
@param argv pointer to the list of arguments passed to this function
*/
static void MFGP_edModel(uint32_t argc, char *argv[])
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edModel, &readingInfo );
   MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
}

/**
Get the software version of the host boot loader

@param argc Number of Arguments passed to this function
@param argv pointer to the list of arguments passed to this function
*/
static void MFGP_hostBootVersion(uint32_t argc, char *argv[])
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edBootVersion, &readingInfo );
   MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
}

/**
Get the software version of the host kernel

@param argc Number of Arguments passed to this function
@param argv pointer to the list of arguments passed to this function
*/
static void MFGP_hostKernelVersion(uint32_t argc, char *argv[])
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edKernelVersion, &readingInfo );
   MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
}

/**
Get the software version of the host BSP

@param argc Number of Arguments passed to this function
@param argv pointer to the list of arguments passed to this function
*/
static void MFGP_hostBspVersion(uint32_t argc, char *argv[])
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edBspVersion, &readingInfo );
   MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
}

/**
Get the hardware version of the carrier board

@param argc Number of Arguments passed to this function
@param argv pointer to the list of arguments passed to this function
*/
static void MFGP_carrierHwVersion(uint32_t argc, char *argv[])
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edCarrierHwVersion, &readingInfo );
   MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
}

/**
Get the manufacturer of the meter

@param argc Number of Arguments passed to this function
@param argv pointer to the list of arguments passed to this function
*/
static void MFGP_edManufacturer(uint32_t argc, char *argv[])
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edManufacturer, &readingInfo );
   MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
}

/**
Test the host communication port by sending an echo request

@param argc Number of Arguments passed to this function
@param argv pointer to the list of arguments passed to this function
*/
static void MFGP_hostEchoTest(uint32_t argc, char *argv[])
{
   (void)argc;

   MFG_logPrintf("%s %s\n", argv[0], B2BSendEchoReq() ? "pass" : "fail");
}
#endif

#if ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 )
/*******************************************************************************
 *
 *  Function name: MFGP_edManufacturer
 *
 *  Purpose: Get the manufacturer of the meter
 *
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_edManufacturer( uint32_t argc, char *argv[] )
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading(edManufacturer, &readingInfo);

   MFG_logPrintf( "%s %s\n", argv[ 0 ], (char *)readingInfo.Value.buffer );
}


/*******************************************************************************
 *
 *  Function name: MFGP_AnsiTblOID
 *
 *  Purpose: Get the manufacturer of the meter
 *
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_AnsiTblOID( uint32_t argc, char *argv[] )
{
   uint8_t manufacturer[EDCFG_ANSI_TBL_OID_LENGTH];

   if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getConfigMfr( &manufacturer[0] ) )
   {
      MFG_printHex( argv[ 0 ], &manufacturer[0], EDCFG_ANSI_TBL_OID_LENGTH );
   }
   else
   {  //Not available or waiting on initial meter comms
      MFG_logPrintf( "%s\n", argv[ 0 ] );
   }
}

/*******************************************************************************
 *
 *  Function name: MFGP_edProgrammerName
 *
 *  Purpose: Get the programmer name of the meter
 *
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_edProgrammerName( uint32_t argc, char *argv[] )
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edProgrammerName, &readingInfo );
   MFG_logPrintf( "%s %s\n", argv[ 0 ], readingInfo.Value.buffer );
}
/*******************************************************************************
 *
 *  Function name: MFGP_edProgramId
 *
 *  Purpose: Get the program id of the meter
 *
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_edProgramId( uint32_t argc, char *argv[] )
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edProgramID, &readingInfo);
   MFG_logPrintf( "%s %llu\n", argv[ 0 ], readingInfo.Value.uintValue );
}
/*******************************************************************************
 *
 *  Function name: MFGP_edProgrammedDateTime
 *
 *  Purpose: Get the meter's last program datetime expressed in seonds since
 *           epoch.
 *
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_edProgrammedDateTime( uint32_t argc, char *argv[] )
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( edProgrammedDateTime, &readingInfo );
   MFG_logPrintf( "%s %llu\n", argv[ 0 ], readingInfo.Value.uintValue );
}
#endif   /* endif of ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 )  */

#if ( CLOCK_IN_METER == 1 )
/*******************************************************************************
 *
 *  Function name: MFGP_meterDateTime
 *
 *  Purpose: Get the current datetime from the meter
 *
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_meterDateTime( uint32_t argc, char *argv[] )
{
   ValueInfo_t readingInfo;

   (void)INTF_CIM_CMD_getMeterReading( meterDateTime, &readingInfo );
   MFG_logPrintf( "%s %llu\n", argv[ 0 ], readingInfo.Value.uintValue );
}
#endif // endif for (CLOCK_IN_METER == 1)

#if ( REMOTE_DISCONNECT == 1 )
/*******************************************************************************
 *
 *  Function name: MFGP_switchPosition
 *
 *  Purpose: Get the current status of disconnect/switch position.
 *
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_switchPosition( uint32_t argc, char *argv[] )
{
   ValueInfo_t readingInfo;
   INTF_CIM_CMD_getMeterReading( switchPositionStatus, &readingInfo );
   MFG_logPrintf( "%s %llu\n", argv[ 0 ], readingInfo.Value.uintValue );
}
/*******************************************************************************
 *
 *  Function name: MFGP_switchPosition
 *
 *  Purpose: Get the current status of disconnect/switch position.
 *
 *  Arguments: argc - Number of Arguments passed to this function
 *             argv - pointer to the list of arguments passed to this function
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_disconnectCapable( uint32_t argc, char *argv[] )
{
   ValueInfo_t readingInfo;
   (void)INTF_CIM_CMD_getMeterReading( disconnectCapable, &readingInfo );
   MFG_logPrintf( "%s %llu\n", argv[ 0 ], readingInfo.Value.intValue );

}
#endif // endif for (REMOTE_DISCONNECT == 1)
#endif // endif of (EP == 1)

#if ENABLE_FIO_TASKS == 1
/*******************************************************************************
 *
 *  Function name: MFGP_ConfigTest
 *
 *  Purpose: Used to test the device configuration.
 *
 *  Arguments:
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_ConfigTest( uint32_t argc, char *argv[] )
{
   (void) argc;
   (void) argv;

   bool status;

   status = FIO_ConfigCheck( false );

   MFG_printf( "%s %s \n", argv[ 0 ], status ?  "OK" : "Error" );
}

/*******************************************************************************
 *
 *  Function name: MFGP_ConfigUpdate
 *
 *  Purpose: Used to update the device configuration reference .
 *
 *  Arguments:
 *
 *  Returns: void
 *
 *******************************************************************************/
static void MFGP_ConfigUpdate( uint32_t argc, char *argv[] )
{
   (void) argc;
   (void) argv;

   bool status;

   status = FIO_ConfigCheck( true );

   MFG_printf( "%s %s \n", argv[ 0 ], status ?  "OK" : "Error" );
}
#endif
/******************************************************************************

   Function Name: MFGP_macRSSI

   Purpose: This function displays the RSSI value of a radio

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
static void MFGP_macRSSI ( uint32_t argc, char *argv[] )
{
   CCA_RSSI_TYPEe processingType;
   uint8_t        radioNum;
   uint8_t        rssi=0;
   char           floatStr[PRINT_FLOAT_SIZE];

#if (EP == 1)
   if ( argc <= 2 ) /* No parameters   */
   {
      radioNum = 0;
      PHY_Lock(); // Function will not return if it fails
      rssi = (uint8_t)RADIO_Filter_CCA( radioNum, &processingType, CCA_SLEEP_TIME );
      PHY_Unlock(); // Function will not return if it fails
      MFG_printf( "%s %s\n", argv[ 0 ], DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( rssi ), 1 ) );
   }
   else
   {
     DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
#else
   if ( argc == 2 )  /* Need radio number */
   {
      radioNum = ( uint8_t )atoi( argv[1] );

      if (radioNum < (uint8_t)MAX_RADIO)
      {
         PHY_Lock(); // Function will not return if it fails
         rssi = (uint8_t)RADIO_Filter_CCA( radioNum, &processingType, CCA_SLEEP_TIME );
         PHY_Unlock(); // Function will not return if it fails
         MFG_printf( "%s %d %s\n", argv[ 0 ], radioNum,
                     DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( rssi ), 1 ) );
      }
   }
   else
   {
     DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
#endif
}

/***********************************************************************************************************************
Function Name: MFGP_macPingCount

   Purpose: Get the current mac ping count from the mac module

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_macPingCount( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_PingCount, U16_VALUE_TYPE);
}

/***********************************************************************************************************************
   Function Name: MFGP_macCsmaMaxAttempts

   Purpose: Get/Set MAC CSMA max attempts

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_macCsmaMaxAttempts( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_CsmaMaxAttempts , U8_VALUE_TYPE);
}

/***********************************************************************************************************************
   Function Name: MFGP_macCsmaMaxBackOffTime

   Purpose: Get/Set MAC CSMA maximum backoff time

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_macCsmaMaxBackOffTime( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_CsmaMaxBackOffTime, U16_VALUE_TYPE);
}

/***********************************************************************************************************************
   Function Name: MFGP_macCsmaMinBackOffTime

   Purpose: Get/Set MAC CSMA minimum backoff time

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_macCsmaMinBackOffTime( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_CsmaMinBackOffTime, U16_VALUE_TYPE);
}

/***********************************************************************************************************************
   Function Name: MFGP_macCsmaPValue

   Purpose: Get/Set MAC CSMA PV value

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_macCsmaPValue( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_CsmaPValue, FLOAT_VALUE_TYPE);
}


/***********************************************************************************************************************
   Function Name: MFGP_macCsmaQuickAbort

   Purpose: Get/Set MAC CSMA QuickAbort value

      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_macCsmaQuickAbort( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_CsmaQuickAbort, BOOL_VALUE_TYPE);
}


/***********************************************************************************************************************
   Function Name: MFGP_macReassemblyTimeout

   Purpose: Get/Set MAC reassembly timeout

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_macReassemblyTimeout( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_ReassemblyTimeout, U8_VALUE_TYPE);
}


/***********************************************************************************************************************
Function Name: MFGP_inboundFrameCount

   Purpose: Get the current inbound frame count from the mac module

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_inboundFrameCount( uint32_t argc, char *argv[] )
{

   MAC_GetConf_t GetConf;
   GetConf = MAC_GetRequest( eMacAttr_RxFrameCount );
   MFG_printf( "%s %u\n", argv[0], GetConf.val.RxFrameCount );

}

/***********************************************************************************************************************
Function Name: MFGP_amBuMaxTimeDiversity

   Purpose: Get/Set the time in minutes during which a /bu/am message may bubble-in subsequent to the event

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_amBuMaxTimeDiversity( uint32_t argc, char *argv[] )
{

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uint8_t uAmBuMaxTimeDiversity;
         uAmBuMaxTimeDiversity = ( uint8_t )atol( argv[1] );
         ( void )EVL_setAmBuMaxTimeDiversity( uAmBuMaxTimeDiversity );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Read and display time request timeout */
   MFG_printf( "%s %u\n", argv[0], EVL_getAmBuMaxTimeDiversity());
}
#if ( EP == 1 )
#if ( SAMPLE_METER_TEMPERATURE == 1 )
/***********************************************************************************************************************
Function Name: MFGP_edTemperatureHystersis

   Purpose: Get/Set the edTemperatureHystersis parameter in the ed_config module

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_edTemperatureHystersis( uint32_t argc, char *argv[] )
{

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uint8_t uEdTemperatureHystersis;
         uEdTemperatureHystersis = ( uint32_t )atol( argv[1] );
         ( void )EDCFG_setEdTemperatureHystersis( uEdTemperatureHystersis );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Read and display time request timeout */
   MFG_printf( "%s %u\n", argv[0], EDCFG_getEdTemperatureHystersis());
}
/***********************************************************************************************************************
Function Name: MFGP_edTempSampleRate

   Purpose: Get/Set the edTempSampleRate parameter in the ed_config module

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_edTempSampleRate( uint32_t argc, char *argv[] )
{

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uint16_t uEdTempSampleRate;
         uEdTempSampleRate = ( uint16_t )atol( argv[1] );
         ( void )EDCFG_setEdTempSampleRate( uEdTempSampleRate );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Read and display time request timeout */
   MFG_printf( "%s %u\n", argv[0], EDCFG_getEdTempSampleRate());
}
#endif // endif for SAMPLE_METER_TEMPERATURE == 1
#endif // endif fo EP == 1
/***********************************************************************************************************************
Function Name: MFGP_engBuTrafficClass

   Purpose: Get/Set the engBuTrafficClass parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_engBuTrafficClass( uint32_t argc, char *argv[] )
{

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uint8_t uEngBuTrafficClass;
         uEngBuTrafficClass = ( uint8_t )atol( argv[1] );
         ( void )SMTDCFG_setEngBuTrafficClass( uEngBuTrafficClass );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Read and display time request timeout */
   MFG_printf( "%s %u\n", argv[0], SMTDCFG_getEngBuTrafficClass());
} /*lint !e715 !e818 */


/***********************************************************************************************************************
Function Name: MFGP_opportunisticAlarmIndexId

   Purpose: Get the current index in the opportunistic alarm log

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_opportunisticAlarmIndexId( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %u\n", argv[0], EVL_GetNormalIndex());
}

/***********************************************************************************************************************
Function Name: MFGP_realTimeAlarmIndexId

   Purpose: Get the current index in the real time alarm log

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_realTimeAlarmIndexId( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %u\n", argv[0], EVL_GetRealTimeIndex());
}

#if (USE_DTLS == 1)
/***********************************************************************************************************************
Function Name: MFGP_maxAuthenticationTimeout

   Purpose: Get/Set the maximum wait time before retrying DTLS Authentication

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_maxAuthenticationTimeout( uint32_t argc, char *argv[] )
{
   uint32_t uMaxAuthenticationTimeout = 0;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uMaxAuthenticationTimeout = ( uint32_t )atol( argv[1] );
         ( void )DTLS_setMaxAuthenticationTimeout( uMaxAuthenticationTimeout );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Read and display time request timeout */
   MFG_printf( "%s %u\n", argv[0], DTLS_getMaxAuthenticationTimeout() );
}


/***********************************************************************************************************************
Function Name: MFGP_minAuthenticationTimeout

   Purpose: Get/Set the minimum wait time before retrying DTLS Authentication

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_minAuthenticationTimeout( uint32_t argc, char *argv[] )
{
   uint16_t uMinAuthenticationTimeout = 0;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uMinAuthenticationTimeout = ( uint16_t )atol( argv[1] );
         ( void )DTLS_setMinAuthenticationTimeout( uMinAuthenticationTimeout );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Read and display time request timeout */
   MFG_printf( "%s %u\n", argv[0], DTLS_getMinAuthenticationTimeout() );
}


/***********************************************************************************************************************
Function Name: MFGP_initialAuthenticationTimeout

   Purpose: Get/Set the minimum wait time before retrying DTLS Authentication

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_initialAuthenticationTimeout( uint32_t argc, char *argv[] )
{
   uint16_t uInitialAuthenticationTimeout = 0;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         uInitialAuthenticationTimeout = ( uint16_t )atol( argv[1] );
         ( void )DTLS_setInitialAuthenticationTimeout( uInitialAuthenticationTimeout );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Read and display time request timeout */
   MFG_printf( "%s %u\n", argv[0], DTLS_getInitialAuthenticationTimeout() );
}

/***********************************************************************************************************************
Function Name: MFGP_dtlsServerCertificateSN

   Purpose: Get the server certificate serial number

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_dtlsServerCertificateSN(uint32_t argc, char *argv[] ){
   uint16_t size = 0;
   uint8_t certBuffer[DTLS_SERVER_CERTIFICATE_SERIAL_MAX_SIZE] = {0};

   if(1 == argc)
   {
      size = DTLS_GetServerCertificateSerialNumber(certBuffer);
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   MFG_printHex(argv[0], certBuffer, size);
}

#endif // USE_DTLS endif
#if (USE_MTLS == 1)
/***********************************************************************************************************************
Function Name: MFGP_mtlsAuthenticationWindow

   Purpose: Get/Set the MTLS authenticaion window value

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_mtlsAuthenticationWindow( uint32_t argc, char *argv[] )
{
   uint32_t AuthenticationWindow = 0;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         AuthenticationWindow = ( uint32_t )atol( argv[1] );
         ( void )MTLS_setMTLSauthenticationWindow( AuthenticationWindow );
      }
   }

   /* Read and display time request timeout */
   MFG_printf( "%s %u\n", argv[0], MTLS_setMTLSauthenticationWindow( (uint32_t)-1 ) );
}


/***********************************************************************************************************************
Function Name: MFGP_mtlsNetworkTimeVariation

   Purpose: Get/Set the MTLS network Time Variation value

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void

   Notes:
***********************************************************************************************************************/
static void MFGP_mtlsNetworkTimeVariation( uint32_t argc, char *argv[] )
{
   uint16_t NetworkTimeVariation = 0;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         NetworkTimeVariation = ( uint16_t )atol( argv[1] );
         ( void )MTLS_setMTLSnetworkTimeVariation( NetworkTimeVariation );
      }
   }

   /* Read and display time request timeout */
   MFG_printf( "%s %u\n", argv[0], MTLS_setMTLSnetworkTimeVariation( (uint32_t)-1 ) );
}

#endif // USE_MTLS endif

/***********************************************************************************************************************
   Function Name: MFG_PhyNumchannels

   Purpose: Get the number of valid channels configured

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFG_PhyNumchannels( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   GetConf = PHY_GetRequest( ePhyAttr_NumChannels );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %u\n", argv[ 0 ], GetConf.val.NumChannels );
   }
   else
   {
      Service_Unavailable();
   }
}

/***********************************************************************************************************************
   Function Name: MFG_PhyRcvrCount

   Purpose: Get the number of receivers

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFG_PhyRcvrCount( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;

   GetConf = PHY_GetRequest( ePhyAttr_RcvrCount );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      MFG_printf( "%s %u\n", argv[ 0 ], GetConf.val.RcvrCount );
   }
   else
   {
      Service_Unavailable();
   }
}

#if (EP == 1)
/***********************************************************************************************************************
   Function Name: MFGP_invalidAddressModeCount

   Purpose: Get the number of valid channels configured

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_invalidAddressModeCount( uint32_t argc, char *argv[] )
{
   NWK_GetConf_t GetConf;
   GetConf = NWK_GetRequest( eNwkAttr_invalidAddrModeCount );
   MFG_printf( "%s %u\n", argv[0], GetConf.val.invalidAddrModeCount );
}
#endif

#if ( EP == 1)
/***********************************************************************************************************************
 * Get/Set the Watch Dog reset Count
 ***********************************************************************************************************************/
static void MFGP_watchDogResetCount( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      uint16_t value;
      value = ( uint16_t )atoi( argv[1] );
      PWR_setWatchDogResetCnt( value );
   }
   MFG_printf( "%s %d\n", argv[ 0 ], PWR_getWatchDogResetCnt() );
}
#endif


#if EP == 1
/***********************************************************************************************************************
 * Get/Set the DFW Status
***********************************************************************************************************************/
static void MFGP_dfwStatus( uint32_t argc, char *argv[] )
{
   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], DFW_Status() );
}

/***********************************************************************************************************************
 * Get/Set the DFW Filetype
***********************************************************************************************************************/
static void MFGP_dfwFileType( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %d\n", argv[ 0 ], DFW_GetFileType());
}

/***********************************************************************************************************************
 * Get/Set the DFW StreamId
***********************************************************************************************************************/
static void MFGP_dfwStreamId( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      uint8_t value;
      value = ( uint8_t )atoi( argv[1] );
      DFW_SetStreamId( value );
   }

   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], DFW_StreamId() );
}
#endif

/***********************************************************************************************************************
 * Get/Set the timeState
***********************************************************************************************************************/
static void MFGP_timeState( uint32_t argc, char *argv[] )
{
   /* Always print read back value  */
   if ( argc == 2 )
   {
      uint8_t value;
      value = ( uint8_t )atoi( argv[1] );
      TIME_SYS_SetTimeState(value);
   }

   /* Always print read back value  */
   MFG_printf( "%s %u\n", argv[ 0 ], TIME_SYS_TimeState());
}

#if ( TM_UART_ECHO_COMMAND == 1 )
/*******************************************************************************

   Function name: MFGP_CommandLine_EchoComment

   Purpose: Echos the command string in arg[0] for testing UART cut/paste

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: Always successful

   Notes:

*******************************************************************************/
static void MFGP_CommandLine_EchoComment( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      MFG_printf( "ECHO\r\n" );
   }
   else if ( argc == 2 )
   {
      MFG_printf( "ECHO %s\r\n", argv[1] );
   }
   else if ( argc == 3 )
   {
      MFG_printf( "ECHO %s %s\r\n", argv[1], argv[2] );
   }
}
#endif // TM_UART_ECHO_COMMAND

/***********************************************************************************************************************
   Function Name: MFGP_engBuEnabled

   Purpose: Set or Print the Eng Bubbleup Stats enabled flag

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
static void MFGP_engBuEnabled( uint32_t argc, char *argv[] )
{
   uint8_t nEngBuEnabled;

   if ( argc <= 2 )
   {
      if ( 2 == argc )
      {
         // Write engBuEnabled
         nEngBuEnabled = ( uint8_t )atol( argv[1] );
         ( void )SMTDCFG_setEngBuEnabled( nEngBuEnabled );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   /* Always print read back value  */
   MFG_printf( "%s %d\n", argv[ 0 ], SMTDCFG_getEngBuEnabled( ) );
}

/***********************************************************************************************************************
 * Handler for engData1 command
***********************************************************************************************************************/
static void MFGP_engData1( uint32_t argc, char *argv[] )
{
   buffer_t *pBuf;
   uint16_t numBytes;

   numBytes = ENG_RES_statsReading( (bool) true, NULL ); // First call with NULL buffer will return the buffer size needed to hold data

   pBuf = BM_allocStack(numBytes);   // Allocate the buffer
   if(pBuf)
   {
      numBytes = ENG_RES_statsReading( (bool) true, pBuf->data); // Second call builds actual payload
      MFG_printHex( argv[ 0 ], pBuf->data, numBytes);
      BM_free( pBuf );
   }
}

#if ( EP == 1 )
/***********************************************************************************************************************
 * Handler for engData2 command
***********************************************************************************************************************/
static void MFGP_engData2( uint32_t argc, char *argv[] )
{
   buffer_t *pBuf;
   uint16_t numBytes;

   numBytes = ENG_RES_rssiLinkReading( NULL ); // First call with NULL buffer will return the buffer size needed to hold data

   pBuf = BM_allocStack(numBytes);   // Allocate the buffer
   if(pBuf)
   {
      numBytes = ENG_RES_rssiLinkReading( pBuf->data );
      MFG_printHex( argv[ 0 ], pBuf->data, numBytes);
      BM_free( pBuf );
   }
}
#endif

/*! ********************************************************************************************************************
 *
 * \fn void MFG_printHex(char *msg, uint8_t *pData, uint16_t numBytes)
 *
 * \brief used to print an buffer to the MFG port
 *
 * \param  msg
 * \param  pData
 * \param  numBytes
 *
 * \return  none
 *
 * \details
 *
 ********************************************************************************************************************
 */
static void MFG_printHex( char* msg, uint8_t const *pData, uint16_t numBytes)
{
   MFG_printf( "%s ", msg);
   if( numBytes != 0 )
   {
       do
       {
          MFG_printf( "%02X", (*pData & 0xff));
          pData++;
       } while (--numBytes > 0);
   }
   MFG_printf( "\n");
}

#if ( EP == 1)
/***********************************************************************************************************************
   Function Name: MFGP_dfwDupDiscardPacketQty

   Purpose: Get the current number duplicate discarded dfw packets

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_dfwDupDiscardPacketQty( uint32_t argc, char *argv[] )
{
   uint16_t discardedPacketCount;
   discardedPacketCount = DFWA_getDuplicateDiscardedPacketQty();
   MFG_printf( "%s %u\n", argv[ 0 ], discardedPacketCount );
}

#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
/***********************************************************************************************************************
   Function Name: MFGP_dfwCompatibilityTestStatus

   Purpose: Get the value of the dfwCompatibilityTestStatus parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_dfwCompatibilityTestStatus( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %u\n", argv[ 0 ], DFWA_getDfwCompatibilityTestStatus() );
}

/***********************************************************************************************************************
   Function Name: MFGP_dfwProgramScriptStatus

   Purpose: Get the value of the dfwProgramScriptStatus parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_dfwProgramScriptStatus( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %u\n", argv[ 0 ], DFWA_getDfwProgramScriptStatus() );
}

/***********************************************************************************************************************
   Function Name: MFGP_dfwAuditTestStatus

   Purpose: Get the value of the dfwAuditTestStatus parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_dfwAuditTestStatus( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %u\n", argv[ 0 ], DFWA_getDfwAuditTestStatus() );
}
/*lint +esym(715,argc,argv) +esym(818,argc,argv) */
#endif // endif ( END_DEVICE_PROGRAMMING_CONFIG == 1 )

/**
Gets or sets the passive network activity timeout.

@param argc Number of arguments
@param argv List of arguments
*/
static void MFGP_NwPassActTimeout(uint32_t argc, char *argv[])
{
   if (argc > 1)
   {
      SM_ATTRIBUTES_u val;

      val.SmActivityTimeout = strtoul(argv[1], NULL, 10);

      (void)SM_SetRequest(eSmAttr_NetworkActivityTimeoutPassive, &val);
   }

   SM_GetConf_t resp = SM_GetRequest(eSmAttr_NetworkActivityTimeoutPassive);

   if (resp.eStatus == eSM_GET_SUCCESS)
   {
      MFG_printf("%s %u\n", argv[0], resp.val.SmActivityTimeout);
   }
   else
   {
      MFG_printf("%s Error\n", argv[0]);
   }
} /*lint !e818 */

/**
Gets or sets the active network activity timeout.

@param argc Number of arguments
@param argv List of arguments
*/
static void MFGP_NwActiveActTimeout(uint32_t argc, char *argv[])
{
   if (argc > 1)
   {
      SM_ATTRIBUTES_u val;

      val.SmActivityTimeout = strtoul(argv[1], NULL, 10);

      (void)SM_SetRequest(eSmAttr_NetworkActivityTimeoutActive, &val);
   }

   SM_GetConf_t resp = SM_GetRequest(eSmAttr_NetworkActivityTimeoutActive);

   if (resp.eStatus == eSM_GET_SUCCESS)
   {
      MFG_printf("%s %u\n", argv[0], resp.val.SmActivityTimeout);
   }
   else
   {
      MFG_printf("%s Error\n", argv[0]);
   }
} /*lint !e818 */
#endif // endif ( EP == 1 )

/**
Sets the OTA Testing Status, either enabled or disabled.

@param argc Number of arguments
@param argv List of arguments
*/
static void MFGP_enableOTATest( uint32_t argc, char *argv[] )
{
   bool  printInputParam;

   printInputParam = (bool)false;
   if ( argc == 2 )  /* Set new value  */
   {
      if ( ( 0 == strcasecmp( argv[1], "true" ) ) || ( 0 == strcmp( argv[1], "1" ) ) )
      {
         HEEP_setEnableOTATest((bool)true);
         printInputParam = (bool)true;
      }
      else if ( ( 0 == strcasecmp( argv[1], "false" ) ) || ( 0 == strcmp( argv[1], "0" ) ) )
      {
         HEEP_setEnableOTATest((bool)false);
         printInputParam = (bool)true;
      }
   }
   if ( printInputParam )
   {
      MFG_printf("%s %s\n", argv[0], argv[1] );
   }
   else
   {
      MFG_printf("%s %u\n", argv[0], HEEP_getEnableOTATest() == true? 1: 0 );
   }
} /*lint !e818 */

/***********************************************************************************************************************
   Function Name: MFGP_capableOfEpBootloaderDFW

   Purpose: Get the value of the capableOfEpBootloaderDFW parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_capableOfEpBootloaderDFW( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %u\n", argv[ 0 ], DFWA_getCapableOfEpBootloaderDFW() );
} /*lint !e818 !e715 pData could be pointer to const, args not used; required by API */

/***********************************************************************************************************************
   Function Name: MFGP_capableOfEpPatchDFW

   Purpose: Get the value of the capableOfEpPatchDFW parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_capableOfEpPatchDFW( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %u\n", argv[ 0 ], DFWA_getCapableOfEpPatchDFW() );
} /*lint !e818 !e715 pData could be pointer to const, args not used; required by API */

/***********************************************************************************************************************
   Function Name: MFGP_capableOfMeterBasecodeDFW

   Purpose: Get the value of the capableOfMeterBasecodeDFW parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_capableOfMeterBasecodeDFW( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %u\n", argv[ 0 ], DFWA_getCapableOfMeterBasecodeDFW() );
} /*lint !e818 !e715 pData could be pointer to const, args not used; required by API */

/***********************************************************************************************************************
   Function Name: MFGP_capableOfMeterPatchDFW

   Purpose: Get the value of the capableOfMeterPatchDFW parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_capableOfMeterPatchDFW( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %u\n", argv[ 0 ], DFWA_getCapableOfMeterPatchDFW() );
} /*lint !e818 !e715 pData could be pointer to const, args not used; required by API */

/***********************************************************************************************************************
   Function Name: MFGP_capableOfMeterReprogrammingOTA

   Purpose: Get the value of the capableOfMeterReprogrammingOTA parameter

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
static void MFGP_capableOfMeterReprogrammingOTA( uint32_t argc, char *argv[] )
{
   MFG_printf( "%s %u\n", argv[ 0 ], DFWA_getCapableOfMeterReprogrammingOTA() );
} /*lint !e818 !e715 pData could be pointer to const, args not used; required by API */

/***********************************************************************************************************************
   Function Name: MFGP_powerQuality

   Purpose:  Return the value for the power quality reading

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
#if ( EP == 1 )
static void MFGP_powerQuality ( uint32_t argc, char *argv[] )
{
   ValueInfo_t  reading;
   (void)INTF_CIM_CMD_getMeterReading( powerQuality, &reading );
   MFG_printf( "%s %llu\n", argv[ 0 ], reading.Value.uintValue );
} /*lint !e818 !e715 pData could be pointer to const, args not used; required by API */
#endif

/******************************************************************************

   Function Name: MFGP_macReliabilityHighCount

   Purpose: Get/Set the macReliabilityHighCount parameter

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

******************************************************************************/
static void MFGP_macReliabilityHighCount  ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_ReliabilityHighCount, U8_VALUE_TYPE);
}

/******************************************************************************

   Function Name: MFGP_macReliabilityMedCount

   Purpose: Get/Set the macReliabilityMedCount parameter

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

******************************************************************************/
static void MFGP_macReliabilityMedCount   ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_ReliabilityMediumCount, U8_VALUE_TYPE);
}


/******************************************************************************

   Function Name: MFGP_macReliabilityLowCount

   Purpose: Get/Set the macReliabilityLowCount parameter

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

******************************************************************************/
static void MFGP_macReliabilityLowCount   ( uint32_t argc, char *argv[] )
{
   MacAttrSetGet(argc, argv, eMacAttr_ReliabilityLowCount, U8_VALUE_TYPE);
}

/*!
 *********************************************************************************************************************
 *
 * This is a helper function for Setting and Reading MAC Attributes.
 *
 * If no parameters are passed it will just read the attribute
 *
 * If one parameters is passed it will parse the value,
 * check for a valid value based upon the type
 * and if valid it will call the set function.
 *
 **********************************************************************************************************************/
static void MacAttrSetGet(uint32_t argc, char *argv[], MAC_ATTRIBUTES_e eAttribute, ValueType_e valueType)
{
   if ( argc <= 2 ){
      if ( argc == 2 )
      {
         value_t value;
         if( strto_value(argv[1], valueType, &value))
         {
            (void)MAC_SetRequest( eAttribute, &value);
         }
      }
   }
   else{
      print_parameter_count_error();
   }

   /* Always print read back value  */
   MAC_GetConf_t GetConf;
   GetConf = MAC_GetRequest( eAttribute );
   if ( GetConf.eStatus == eMAC_GET_SUCCESS )
   {
      value_t *pValue = (value_t *)&GetConf.val;
      print_value(argv[ 0 ], valueType, pValue);
   }
   else
   {
      Service_Unavailable();
   }
}

#if 0
/*!
 *********************************************************************************************************************
 *
 * This is a helper function for Setting and Reading PHY Attributes.
 *
 * If no parameters are passed it will just read the attribute
 *
 * If one parameters is passed it will parse the value,
 * check for a valid value based upon the type
 * and if valid it will call the set function.
 *
 **********************************************************************************************************************/
static void PhyAttrSetGet(uint32_t argc, char *argv[], PHY_ATTRIBUTES_e eAttribute, ValueType_e valueType)
{
   if ( argc <= 2 )
   {
      if ( argc == 2 )
      {
         value_t value;
         if( strto_value(argv[1], valueType, &value))
         {
            (void)PHY_SetRequest( eAttribute, &value);
         }
      }
   }
   else
   {
      print_parameter_count_error();
   }

   PHY_GetConf_t GetConf;
   /* Always print read back value  */
   GetConf = PHY_GetRequest( eAttribute );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS )
   {
      value_t *pValue = (value_t *)&GetConf.val;
      print_value(argv[ 0 ], valueType, pValue);
   }
   else
   {
      Service_Unavailable();
   }
}
#endif


/*! *********************************************************************************************************************
 * Used to print error message when the incorrect number of paramters are entered
 **********************************************************************************************************************/
static void print_parameter_count_error(void)
{
   DBG_logPrintf( 'R', "Invalid number of parameters" );
}

/*! *********************************************************************************************************************
 * Used to print a value using the value type
 **********************************************************************************************************************/
static void print_value(const char* argv, ValueType_e valueType, const value_t * pValue)
{
   if      ( valueType == U8_VALUE_TYPE )    { MFG_printf( "%s %u\n",    argv, pValue->u8);  }
   else if ( valueType == I8_VALUE_TYPE )    { MFG_printf( "%s %d\n",    argv, pValue->i8);  }
   else if ( valueType == U16_VALUE_TYPE )   { MFG_printf( "%s %u\n",    argv, pValue->u16); }
   else if ( valueType == I16_VALUE_TYPE )   { MFG_printf( "%s %d\n",    argv, pValue->i16); }
   else if ( valueType == U32_VALUE_TYPE )   { MFG_printf( "%s %u\n",    argv, pValue->u32); }
   else if ( valueType == I32_VALUE_TYPE )   { MFG_printf( "%s %d\n",    argv, pValue->i32); }
   else if ( valueType == BOOL_VALUE_TYPE )  { MFG_printf( "%s %u\n",    argv, pValue->bValue); }
   else if ( valueType == TIME_VALUE_TYPE )  { MFG_printf( "%s %u %u\n", argv, pValue->time.seconds, pValue->time.QFrac); }
   else if ( valueType == FLOAT_VALUE_TYPE ) {
      char floatStr[PRINT_FLOAT_SIZE];
      // Since we are only showing 2 decimal places, round up by 0.005
      MFG_printf( "%s %s\n", argv, DBG_printFloat(floatStr, (pValue->fValue + 0.005F), 2) );
   }
}

/*! *********************************************************************************************************************
 * Used to parse a value where the value type is specified
 **********************************************************************************************************************/
static bool strto_value(const char* argv, ValueType_e valueType, value_t * pValue)
{
   bool isValid = false;
   switch (valueType)
   {
      case U8_VALUE_TYPE:    {  isValid = strto_uint8(  argv, &pValue->u8 );     break; }
      case I8_VALUE_TYPE:    {  isValid = strto_int8(   argv, &pValue->i8 );     break; }
      case U16_VALUE_TYPE:   {  isValid = strto_uint16( argv, &pValue->u16 );    break; }
      case I16_VALUE_TYPE:   {  isValid = strto_int16(  argv, &pValue->i16 );    break; }
      case U32_VALUE_TYPE:   {  isValid = strto_uint32( argv, &pValue->u32 );    break; }
      case I32_VALUE_TYPE:   {  isValid = strto_int32(  argv, &pValue->i32 );    break; }
      case TIME_VALUE_TYPE:  {  isValid = strto_time(   argv, &pValue->time );   break; }
      case BOOL_VALUE_TYPE:  {  isValid = strto_bool(   argv, &pValue->bValue ); break; }
      case FLOAT_VALUE_TYPE: {  isValid = strto_float(  argv, &pValue->fValue ); break; }
      default:         {  isValid = false;                               break; }
   }
   return isValid;
}


/*! *********************************************************************************************************************
 * Used to parse an uint8
 **********************************************************************************************************************/
static
bool strto_uint8(const char* argv, uint8_t *u8)
{
   uint32_t temp = strtoul(argv, NULL, 10);
   if( temp <= UCHAR_MAX){
      *u8 = (uint8_t)temp;
      return true;
   }
   return false;
}

/*! *********************************************************************************************************************
 * Used to parse an uint16
 **********************************************************************************************************************/
static
bool strto_uint16(const char* argv, uint16_t *u16)
{
   uint32_t temp = strtoul(argv, NULL, 10);
   if( temp <= USHRT_MAX){
      *u16 = (uint16_t)temp;
      return true;
   }
   return false;
}

/*! *********************************************************************************************************************
 * Used to parse an uint32
 **********************************************************************************************************************/
static
bool strto_uint32(const char* argv, uint32_t *u32)
{
   *u32 = strtoul(argv, NULL, 10);
   return true;
}

/*! *********************************************************************************************************************
 * Used to parse an int8
 **********************************************************************************************************************/
static
bool strto_int8(const char* argv, int8_t *i8)
{  // i8
   int32_t temp = strtol(argv, NULL, 10);
   if( (temp >= SCHAR_MIN) && (temp <= SCHAR_MAX)){
      *i8 = (int8_t) temp ;
      return true;
   }
   return false;
}

/*! *********************************************************************************************************************
 * Used to parse an int16
 **********************************************************************************************************************/
static
bool strto_int16(const char* argv, int16_t *i16)
{
   int32_t temp = strtol(argv, NULL, 10);
   if( (temp >= SHRT_MIN) && (temp <= SHRT_MAX)){
      *i16 = (int16_t) temp ;
      return true;
   }
   return false;
}

/*! *********************************************************************************************************************
 * Used to parse an int32
 **********************************************************************************************************************/
static
bool strto_int32(const char* argv, int32_t *i32)
{
   int32_t temp = strtol(argv, NULL, 10);
   *i32 = temp ;
   return true;
}

/*! *********************************************************************************************************************
 * Used to parse a float
 **********************************************************************************************************************/
static
bool strto_float(const char* argv, float *fValue)
{
   *fValue = (float)atof(argv);
   return true;
}

/*! *********************************************************************************************************************
 * Used to parse a bool
 **********************************************************************************************************************/
static
bool strto_bool(const char* argv, bool *bValue)
{
   int32_t temp = strtol(argv, NULL, 10);
   if( (temp >= 0) && (temp <= 1)){
      *bValue = (bool)temp;
      return true;
   }
   return false;
}

/*! *********************************************************************************************************************
 * Used to parse a TIMESTAMP_t
 **********************************************************************************************************************/
static
bool strto_time(const char* argv, TIMESTAMP_t *value)
{
   uint32_t temp = strtoul(argv, NULL, 10);
   value->seconds    = temp;
   value->QSecFrac = 0;
   return true;
}
