/******************************************************************************

   Filename: DBG_CommandLine.c

   Global Designator: DBG_

   Contents:

 ******************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 *****************************************************************************/

/*lint -esym(818, argc, argv) argc, argv could be const */
/*lint -esym(715,argc, argv, Arg0)  */
/* INCLUDE FILES */
#include "project.h"
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "DBG_SerialDebug.h"
#include "DBG_CommandLine.h"
#if ( MCU_SELECTED == NXP_K24 )
#include <bsp.h>
#endif
#include "file_io.h"
#include "SELF_test.h"
#include "version.h"
#include "dfw_app.h"

#include "pwr_task.h"
#include "dfw_interface.h"
//#include "MIMT_info.h"

#if ( EP == 1 )
#include "pwr_last_gasp.h"
#include "vbat_reg.h"
#include "time_DST.h"
#include "crc16_m.h"
#endif

#include "mode_config.h"
#include "MFG_Port.h"
//#include "MAC_Protocol.h"
//#include "STACK_Protocol.h"
#include "COMM.h"
//#include "time_util.h"
#include "dvr_DAC0.h"
#include "radio.h"
#include "radio_hal.h"
#include "PHY.h"
#include "PHY_Protocol.h"
#include "MAC_Protocol.h"
#include "SM_Protocol.h"     // Stack Manager
#include "SM.h"              // Stack Manager
#include "MAC.h"
#include "STACK.h"
#if ( USE_DTLS == 1 )
#include "dtls.h"
#endif
#if ( USE_MTLS == 1 )
#include "mtls.h"
#endif
#include "time_util.h"
#include "partitions.h"
#if ( DCU == 1 )
#include "MAINBD_Handler.h"
#include "HEEP_util.h"
#endif //DCU
//#include "dfw_interface.h"
#include "STRT_Startup.h"
//#include "APP_MSG_Handler.h"
#ifdef TM_PARTITION_USAGE
#include "filenames.h"
#endif
#include "ecc108_apps.h"
#include "ecc108_config.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include "ecc108_mqx.h"
#elif ( RTOS_SELECTION == FREE_RTOS )
#include "ecc108_freertos.h"
#include "crc.h"
#endif
#include "ecc108_lib_return_codes.h"
//#include "compiler_types.h"
#include "si446x_api_lib.h"
#include "si446x_cmd.h"
#include "si446x_prop.h"
#include "time_sync.h"
//#include "SELF_test.h"
#include "mode_config.h"
#include "EVL_event_log.h"
#include "ascii.h"
#if (USE_DTLS==1)
#include "dtls.h"
#include "wolfssl/wolfcrypt/aes.h"
#include "wolfssl/wolfcrypt/sha256.h"
#endif

#if (PHASE_DETECTION==1)
#include "PhaseDetect.h"
uint32_t DBG_CommandLine_PhaseDetectCmd( uint32_t argc, char *argv[] );
#endif

//uint32_t DBG_CommandLine_SM_Start(  uint32_t argc, char *argv[] ); // RA6E1 Bob: This command was removed from original K24 code
//uint32_t DBG_CommandLine_SM_Stop(   uint32_t argc, char *argv[] ); // RA6E1 Bob: This command was removed from original K24 code
uint32_t DBG_CommandLine_SM_Stats(  uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_SM_Config( uint32_t argc, char *argv[] );
//uint32_t DBG_CommandLine_SM_Set(    uint32_t argc, char *argv[] );
//uint32_t DBG_CommandLine_SM_Get(    uint32_t argc, char *argv[] );

#if (EP == 1)
#include "smtd_config.h"
#endif  /* end of EP */
#include "dfwtd_config.h"
#if (EP == 1)
#include "pwr_config.h"
#include "ed_config.h"
#include "RG_MD_Handler.h"
#include "OR_MR_Handler.h"
#if (ACLARA_LC == 0 ) && (ACLARA_DA == 0)
#include "demand.h"
#include "hmc_mtr_info.h"
#include "historyd.h"
#include "ID_intervalTask.h"
#include "hmc_ds.h"
#include "hmc_request.h"
#include "hmc_eng.h"
#endif // (ACLARA_LC == 0 ) && (ACLARA_DA == 0)
#include "intf_cim_cmd.h"
#if ( DEMAND_IN_METER == 1 )
#include "hmc_demand.h"
#endif
#if ( CLOCK_IN_METER == 1 )
#include "hmc_time.h"
#endif
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 ) // Required for creating HMC traffic during noiseband testing
#include "hmc_display.h"
#include "hmc_start.h"
#include "hmc_finish.h"
#include "ID_intervalTask.h"
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
#endif // (EP == 1)
#if ( NOISE_HIST_ENABLED == 1 )
#include "NH_NoiseHistData.h"
#endif
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
#include "MK66F18.h"
#endif
#if ( TM_BSP_SW_DELAY == 1 )
#include "sys_clock.h"
#endif
#if( ( RTOS_SELECTION == FREE_RTOS ) && ( TM_OS_EVENT_TEST == 1) )
#include "event_groups.h"
#endif

/* #DEFINE DEFINITIONS */
#if ( ( EP == 1 ) || ( PORTABLE_DCU == 0 ) )
#define MAX_DBG_COMMAND_CHARS     1602
#define MAX_ATOH_CHARS            270
#else
#define MAX_DBG_COMMAND_CHARS     2602
#define MAX_ATOH_CHARS            1301
#endif
#define MAX_CMDLINE_ARGS          34
#define DELETE_CHANNEL            9999

#define SOH 1
#define EOT 4

#define WRITE_KEY_ALLOWED  0
#define READ_KEY_ALLOWED   0

#define NOISEBAND_MAX_NSAMPLES 20000 // Each channels can gather up to this amount of samples to compute the statistics
#define NOISEBAND_MAX_CHANNELS 1601  // Maximum number of channels we can sample at once
#define NOISEBAND_SCALE_STDDEV 3.0   // Used to increased to dynamic range of small values

#if ( SIMULATE_POWER_DOWN == 1 )
#define DEFAULT_NUM_SECTORS   (uint8_t)4
#define INTERNAL_NV           (bool)true
#define EXTERNAL_NV           (bool)false
#endif

#if ( MCU_SELECTED == RA6E1 )  //To process the input byte in RA6E1
#define ESCAPE_CHAR           0x1B
#define LINE_FEED_CHAR        0x0A
#define CARRIAGE_RETURN_CHAR  0x0D
#define BACKSPACE_CHAR        0x08
#define TILDA_CHAR            0x7E
#define WHITE_SPACE_CHAR      0x20
#define CTRL_C_CHAR 0x03
#endif

#if ( TM_TIME_COMPOUND_TEST == 1 )
#define TICKCOUNT_SEC            200                             /* Number of TickCount per second */
#define TICKCOUNT_MIN            TICKCOUNT_SEC*60                /* Number of TickCount per minute */
#define TICKCOUNT_HOUR           TICKCOUNT_MIN*60                /* Number of TickCount per hour */
#define MAX_SAMPLE_LOOP_COUNT    5                               /* Maximum Number of Sample for DBG_CommandLine_TimeElapsed */
#endif

#if ( TM_LINKED_LIST == 1)
#define MAX_LINKEDLIST_DATA      5
#endif


/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
typedef uint32_t ( *Fxn_CmdLine )( uint32_t argc, char *argv[] );
typedef struct
{
   const char *pcCmd; //! A pointer to a string containing the name of the command.
   Fxn_CmdLine pfnCmd; //! A function pointer to the implementation of the command.
   const char *pcHelp; //! A pointer to a string of brief help text for the command.
} struct_CmdLineEntry;

typedef struct {
   uint8_t min;
   uint8_t med;
   uint8_t max;
   uint8_t avg;
   uint8_t stddev;
   uint8_t P90;
   uint8_t P95;
   uint8_t P99;
   uint8_t P995;
   uint8_t P999;
} NOISEBAND_Stats_t;

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

#if ENABLE_HMC_TASKS
static OS_SEM_Obj HMC_CMD_SEM;
static bool       HmcCmdSemCreated = ( bool )false;
#endif

#if ( SIMULATE_POWER_DOWN == 1 )
static OS_TICK_Struct         DBG_startTick_ ={0};
static bool                   pwrDnSimulation_Enable_ = false;
static PartitionData_t const  *pTestPartition_;    /* Pointer to partition information   */
#endif

static char                   DbgCommandBuffer[MAX_DBG_COMMAND_CHARS + 1];
static char                   *argvar[MAX_CMDLINE_ARGS + 1];
static void PrintECC_error( uint8_t ECCstatus );

static uint32_t DBG_CommandLine_Comment( uint32_t argc, char *argv[] );
static uint32_t DBG_CommandLine_DebugFilter( uint32_t argc, char *argv[] );
#if ( TM_UART_ECHO_COMMAND == 1 )
static uint32_t DBG_CommandLine_EchoComment( uint32_t argc, char *argv[] );
#endif
static uint32_t DBG_CommandLine_GenDFWkey( uint32_t argc, char *argv[] );
static uint32_t DBG_CommandLine_SendHeepMsg( uint32_t argc, char *argv[] );
static uint32_t DBG_CommandLine_SM_NwActiveActTimeout(uint32_t argc, char *argv[]);
static uint32_t DBG_CommandLine_SM_NwPassiveActTimeout(uint32_t argc, char *argv[]);
static uint32_t DBG_CommandLine_virginDelay( uint32_t argc, char *argv[] );
#if ( DEBUG_PORT_BAUD_RATE == 1 )
static uint32_t DBG_CommandLine_DebugBaudRate( uint32_t argc, char *argv[] );
#endif
#if ( DCU == 1 )
static uint32_t DBG_CommandLine_sdtest( uint32_t argc, char *argv[] );
static uint32_t DBG_CommandLine_getTBslot( uint32_t argc, char *argv[] );
//static uint32_t CommandLine_ReadResource ( uint32_t argc, char *argv[] ); // RA6E1 Bob: This command was removed from original K24 code
//static uint32_t DBG_TraceRoute( uint32_t argc, char *argv[] ); // RA6E1 Bob: This command was removed from original K24 code

#if OTA_CHANNELS_ENABLED == 1
static uint32_t CommandLine_OTA_Cmd( uint32_t argc, char *argv[] );
static uint32_t SetChannels( uint16_t ReadingType, uint32_t argc, char *argv[] );
#endif
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
static uint32_t DBG_CommandLine_clockswtest( uint32_t argc, char *argv[] );
#endif
#endif //( DCU == 1 )

#if ( EP == 1 )
static uint32_t DBG_CommandLine_crc16m( uint32_t argc, char *argv[] );
static uint32_t DBG_CommandLine_PWR_BoostMode( uint32_t argc, char *argv[] );
#if ( TEST_TDMA == 1 )
static uint32_t DBG_CommandLine_CsmaSkip( uint32_t argc, char *argv[] );
static uint32_t DBG_CommandLine_TxSlot( uint32_t argc, char *argv[] );
#endif
#if ( ACLARA_LC != 1 ) && (ACLARA_DA != 1) /* meter specific code */
static uint32_t DBG_CommandLine_HmcwCmd( uint32_t argc, char *argv[] );
#endif
#if ( LP_IN_METER != 0 )
static uint32_t DBG_CommandLine_lpstats( uint32_t argc, char *argv[] );
#endif
#if ( MULTIPLE_MAC != 0 )
static uint32_t DBG_CommandLine_NumMac( uint32_t argc, char *argv[] );
#endif
#if ( ENABLE_DEMAND_TASKS == 1 )
#if ( DEMAND_IN_METER == 1 )
static void DBGC_DemandResetCallBack( returnStatus_t result );
static uint32_t DBG_CommandLine_DemandReset( uint32_t argc, char *argv[] );
#endif
#endif
#if ( CLOCK_IN_METER == 1 )
static uint32_t DBG_CommandLine_HMC_time( uint32_t argc, char *argv[] );
static uint32_t DBG_CommandLine_mtrTime( uint32_t argc, char *argv[] );
#endif
#endif //( EP == 1 )

#if ( READ_KEY_ALLOWED != 0 )
static uint32_t DBG_CommandLine_readKey( uint32_t argc, char *argv[] );
static uint32_t DBG_CommandLine_dumpKeys( uint32_t argc, char *argv[] );
#endif
#if ( WRITE_KEY_ALLOWED != 0 )
static uint32_t DBG_CommandLine_writeKey( uint32_t argc, char *argv[] );
#endif

#if ( EVL_UNIT_TESTING == 1 )
uint32_t DBG_CommandLine_EVL_UNIT_TESTING( uint32_t argc, char *argv[] );
#endif

#if ( PORTABLE_DCU == 1)
static uint32_t DBG_CommandLine_dfwMonMode( uint32_t argc, char *argv[] );
#endif

#if 0  // RA6E1 Bob: This command was removed from original K24 code
static uint32_t DBG_CommandLine_hardfault( uint32_t argc, char *argv[] );
#endif

#if ( USE_IPTUNNEL == 1 )
static uint32_t DBG_CommandLine_SendIpTunMsg(uint32_t argc, char *argv[]);
#endif
#if ( USE_USB_MFG != 0 )
static uint32_t DBG_CommandLine_BDT(uint32_t argc, char *argv[]);
static uint32_t DBG_CommandLine_usbaddr( uint32_t argc, char *argv[] );
#endif

#if ( TEST_SYNC_ERROR == 1 )
static uint32_t DBG_CommandLine_SyncError( uint32_t argc, char *argv[] );
#endif
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
uint32_t DBG_CommandLine_NoiseBandClkOn ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NoiseBandClkOff( uint32_t argc, char *argv[] );
#define GPIO_PIN_TRISTATE 1
typedef struct
{
   uint8_t GPIO0;
   uint8_t GPIO1;
   uint8_t GPIO2;
   uint8_t GPIO3;
   uint8_t gen_config;
} ArgStream_s;
static ArgStream_s sArgStream
             = {SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_TRISTATE,           /* GPIO0 */
                SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_TRISTATE,           /* GPIO1 */
                SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_TRISTATE,           /* GPIO2 */
                SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_TRISTATE,           /* GPIO3 */
                SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5}; /* Low Drive Strength */
#endif
#if ( TM_MEASURE_SLEEP_TIMES == 1 )
static uint32_t DBG_CommandLine_TestOsTaskSleep( uint32_t argc, char *argv[] );
#endif
#if ( MCU_SELECTED == RA6E1 )
static uint32_t DBG_CommandLine_CoreClocks( uint32_t argc, char *argv[] );
static uint32_t DBG_CommandLine_TestSWDelay( uint32_t argc, char *argv[] );
static uint32_t DBG_CommandLine_FlashSecurity( uint32_t argc, char *argv[] );
#endif

#if ( TM_OS_EVENT_TEST == 1)
const char pTskName_OSEVNTTest[]      = "TMEVT";
static OS_EVNT_Obj eventTestObj;
static OS_EVNT_Handle eventTestHandle = &eventTestObj;
static bool osEventTaskCreated = FALSE;
static bool osEventCreated = FALSE;
static uint32_t waitBit;
static bool waitForAll;
#endif

#if ( TM_LINKED_LIST == 1)
static OS_List_Obj osLinkedListTestObj;
static OS_List_Handle osLinkedListTestHandle = &osLinkedListTestObj;
static OS_Linked_List_Element LinkedListdata[MAX_LINKEDLIST_DATA];
#endif

static const struct_CmdLineEntry DBG_CmdTable[] =
{
   /*lint --e{786}    String concatenation within initializer OK   */
   { "help",         DBG_CommandLine_Help,            "Display list of commands" },
   { "h",            DBG_CommandLine_Help,            "help" },
   { "?",            DBG_CommandLine_Help,            "help" },
   { "watchDogTest", DBG_CommandLine_wdTest,          "1 for refreshing the watchdog timer 2 for validating the watchdog timer" },
#if ( DAC_CODE_CONFIG == 1 )
   { "setdac0step",               DBG_CommandLine_DAC_SetDacStep,      "Set the DAC Step, range for steps is 0 - 4096 eg.) setdac0step 2200" },
   { "pwrsel",                    DBG_CommandLine_setPwrSel,           "Set/reset pwrsel pin" },
#endif
#if ( FAKE_TRAFFIC == 1 )
   { "bhgencount",   DBG_CommandLine_ipBhaulGenCount, "get (no args) or set (arg1) backhaul fake record gen count" },
#endif
#if ( EP == 1 )
#if ( MCU_SELECTED == RA6E1 ) // RA6E1 Bob: these commands were removed from K24 starting point but maybe we need them for testing of RA6E1
   { "afcenable",    DBG_CommandLine_AfcEnable,       "Enable/disable AFC" }, // RA6E1 Bob: This command was removed from original K24 code
   { "afcrssithreshold", DBG_CommandLine_AfcRSSIThreshold, "display/Set AFC RSSI threshold" }, // RA6E1 Bob: This command was removed from original K24 code
   { "afctemperaturerange", DBG_CommandLine_AfcTemperatureRange, "display/Set AFC temperature range" }, // RA6E1 Bob: This command was removed from original K24 code
#endif // ( MCU_SELECTED == RA6E1 )
#endif // ( EP == 1 )
#if ( USE_USB_MFG != 0 )
   {  "bdt",         DBG_CommandLine_BDT,             "Print USB buffer descriptor table(s)" },
#endif
#if ( EP == 1 )
   { "boost",        DBG_CommandLine_PWR_BoostTest,   "PWR - Boost Test" },
   { "boostmode",    DBG_CommandLine_PWR_BoostMode,   "Turn boost supply on/off" },
#endif
#if ENABLE_PWR_TASKS
#if 0 // RA6E1 Bob: This command was removed from original K24 code
   { "brown",        DBG_CommandLine_PWR_BrownOut,    "PWR - Signal Brown Out" },
#endif
#endif
   { "buffers",      DBG_CommandLine_Buffers,         "Display buffers allocated and statitics" },
#if ( EP == 1 )
   { "cap",          DBG_CommandLine_PWR_SuperCap,    "PWR - Read Super Cap Voltage" },
#endif
   { "cca",          DBG_CommandLine_CCA,             "Display/Set the CCA threshold" },
   { "ccaadaptivethreshold", DBG_CommandLine_CCAAdaptiveThreshold, "Display the CCA adaptive threshold" },
   { "ccaadaptivethresholdenable", DBG_CommandLine_CCAAdaptiveThresholdEnable, "Display/Set the CCA adaptive threshold enable" },
   { "ccaoffset",    DBG_CommandLine_CCAOffset,       "Display/Set the CCA offset" },
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
   { "clockswtest",  DBG_CommandLine_clockswtest,     "Count cycle counter at 2 different clock settings over 5 seconds" },
#endif
   { "clocktst",     DBG_CommandLine_clocktst,        "1/0 Turn clkout signal on/off" },
   { "comment",      DBG_CommandLine_Comment,         "No action; allows comment in log" },
#if ( MCU_SELECTED == RA6E1 )
   { "coreClocks",   DBG_CommandLine_CoreClocks,      "Display the CPU core clocks in Hz" },
#endif
#if ( EP == 1 )
   { "counters",     DBG_CommandLine_Counters,        "Display the current counters like reset counts etc" },
#endif
   { "cpuloaddis",   DBG_CommandLine_CpuLoadDisable,  "Disable CPU Load Output" },
   { "cpuloaden",    DBG_CommandLine_CpuLoadEnable,   "Enable CPU Load Output" },
   { "cpuloadsmart", DBG_CommandLine_CpuLoadSmart,    "Print CPU Load only when > 30%" },
   { "crc",          DBG_CommandLine_crc,             "calculate crc over hex string" },
//   { "crc16",        DBG_CommandLine_crc16,           "calculate DCU hack 16 bit crc over hex string" }, // RA6E1 Bob: This command was removed from original K24 code
#if ( EP == 1 )
   { "crcm",         DBG_CommandLine_crc16m,          "calculate ANSI psem 16 bit crc over hex string" },
#if ( TEST_TDMA == 1 )
   { "csmaskip",     DBG_CommandLine_CsmaSkip,        "Skip CCA" },
#endif
#endif
//   { "dbgdis",       DBG_CommandLine_DebugDisable,    "Disable Debug Output" }, // RA6E1 Bob: This command was removed from original K24 code
//   { "dbgen",        DBG_CommandLine_DebugEnable,     "Enable Debug Output" },  // RA6E1 Bob: This command was removed from original K24 code
   { "dbgfilter",    DBG_CommandLine_DebugFilter,     "Set debug port print filter task id" },
#if ( DEBUG_PORT_BAUD_RATE == 1 )
   { "dbgbaudrate",  DBG_CommandLine_DebugBaudRate,   "Set debug port baud rate to 11520 or 38400" }, // New for RA6E1 to support Uplink Power Control testing */
#endif
#if ( EP == 1 )
#if ( TEST_TDMA == 1 )
   { "Delay",        DBG_CommandLine_Delay,           "Set/Get the Slot Time Delay betwen 0 and 100 ms" },
#endif
#if ( ENABLE_DEMAND_TASKS == 1 )
#if ( DEMAND_IN_METER == 1 )
   { "demandreset",  DBG_CommandLine_DemandReset,     "Execute a demand reset (in the meter)" },
#endif
#if( DEMAND_IN_METER == 0 )
   { "schDmdRst",    DBG_CommandLine_SchDmdRst,       "Get (no args) or set (arg1) the scheduled demand reset date of month" },
#endif
//   { "demandto",     DBG_CommandLine_DMDTO,           "Get (no args) or set (arg1) the demand reset timeout" }, // RA6E1 Bob: This command was removed from original K24 code
#endif
#endif
#if ENABLE_DFW_TASKS
   { "dfw",          DBG_CommandLine_DFW_Status,      "DFW - Status" },
#endif   //End of #if EP
#if ( EP == 1 )
   { "dfwtd",        DBG_CommandLine_DfwTimeDv,       "Get/Set DFW TimeDiversity 0-255 minutes (DLConfirm ApplyConfirm)" },
#if ( REMOTE_DISCONNECT == 1 )
#if ( ACLARA_LC != 1 ) && ( ACLARA_DA != 1 ) /* meter specific code */
   { "ds",           DBG_CommandLine_ds,              "Open/Close switch. Format ds close or ds open [ msgID ]" },
#endif
#endif
//   { "dstimedv",     DBG_CommandLine_DsTimeDv,        "Get (no args) or set (one arg) Daily Shift Time Diversity" }, // RA6E1 Bob: This command was removed from original K24 code
#endif
#if ( USE_DTLS == 1 )
   { "dtls",         DBG_CommandLine_Dtls,            "Gets or resets DTLS session state" },
   { "dtlsstats",    DBG_CommandLine_DtlsStats,       "Print the DTLS stats" },
#endif
#if ( EP == 1 )
#if ( ENABLE_DEMAND_TASKS == 1 ) /* If Demand Feature is enabled */
   { "dumpdemand",   DBG_CommandLine_DMDDump,         "Prints the demand file/variables" },
#endif
#if ( TM_UART_ECHO_COMMAND == 1 )
   { "echo",         DBG_CommandLine_EchoComment,     "Echos what was typed" },
#endif
#endif
   { "evladd",       DBG_CommandLine_EVLADD,          "Add an event to the log" },
   { "evlcmd",       DBG_CommandLine_EVLCMD,          "Send Commands to EVL" },
   { "evlgetlog",    DBG_CommandLine_EVLGETLOG,       "Get an event log" },
   { "evlq",         DBG_CommandLine_EVLQ,            "Query event log" },
   { "evlsend",      DBG_CommandLine_EVLSEND,         "Mark Events as Sent" },
#if ( EVL_UNIT_TESTING == 1 )
   { "evlUnitTest",  DBG_CommandLine_EVL_UNIT_TESTING, "Run and event log unit test" },
#endif
   { "file",         DBG_CommandLine_PrintFiles,      "Print File list or file content (optional param, filename)" },
   { "filedump",     DBG_CommandLine_DumpFiles,       "Print the contents of files in the un-named partitions" },
#if ( MCU_SELECTED == RA6E1 )
   { "flashsecurity",DBG_CommandLine_FlashSecurity,   "Display the Device Lifecycle Management state for RA6" },
#endif
   { "freeram",      DBG_CommandLine_FreeRAM,         "Display remaining free internal RAM" },
#if ( DCU == 1 )
   { "frontendgain", DBG_CommandLine_FrontEndGain,    "Display/Set the front end gain" },
#endif
#if ( BUILD_DFW_TST_CMD_CHANGE == 0 )
   { "GenDFWkey",    DBG_CommandLine_GenDFWkey,       "ComDeviceType FromVer ToVer" },
#else
   { "DfwKeyGen",    DBG_CommandLine_GenDFWkey,       "ComDeviceType FromVer ToVer" },
#endif
#if ( EP == 1 )
//   { "getdstenable", DBG_CommandLine_getDstEnable,    "Prints the DST enable" },             // RA6E1 Bob: This command was removed from original K24 code
//   { "getdsthash",   DBG_CommandLine_getDstHash,      "Prints the DST hash" },               // RA6E1 Bob: This command was removed from original K24 code
//   { "getdstoffset", DBG_CommandLine_getDstOffset,    "Prints the DST offset" },             // RA6E1 Bob: This command was removed from original K24 code
#endif
   { "getfreq",      DBG_CommandLine_GetFreq,         "print list of available frequencies" },
#if ( EP == 1 )
//   { "getstartrule", DBG_CommandLine_getDstStartRule, "Prints the DST start rule" },         // RA6E1 Bob: This command was removed from original K24 code
//   { "getstoprule",  DBG_CommandLine_getDstStopRule,  "Prints the DST stop rule" },          // RA6E1 Bob: This command was removed from original K24 code
//   { "gettimezoneoffset", DBG_CommandLine_getTimezoneOffset, "Prints the timezone offset" }, // RA6E1 Bob: This command was removed from original K24 code
#endif
#if ENABLE_HMC_TASKS
#if ( ACLARA_LC != 1 ) && ( ACLARA_DA != 1 ) /* meter specific code */
   { "hmc",          DBG_CommandLine_HmcCmd,          "Host Meter Table Read, format is: hmc [m] id offset len" },
   { "hmcc",         DBG_CommandLine_HmcCurrent,      "Host Meter Read Current, format is: hmcc uom" },
   { "hmcd",         DBG_CommandLine_HmcDemand,       "Host Meter Demand, format is: hmcd uom" },
   { "hmceng",       DBG_CommandLine_HmcEng,          "print Host Meter Communication engineering stats"},
#endif
#if 0
   { "hmcdc",        DBG_CommandLine_HmcDemandCoin,   "Host Meter Demand Coincident, format is: hmcd uom coin" }, // RA6E1 Bob: This command was removed from original K24 code
#endif
   { "hmcs",         DBG_CommandLine_HmcHist,         "Host Meter Read Shift, format is: hmcs uom" },
#if ( CLOCK_IN_METER == 1 )
   { "hmctime",      DBG_CommandLine_HMC_time,        "Get/Set Host Meter time" },
#endif
#if ( ACLARA_LC != 1 ) && ( ACLARA_DA != 1 ) /* meter specific code */
   { "hmcw",         DBG_CommandLine_HmcwCmd,         "Host Meter Table Write, format is: hmcw [m] id offset data" },
#endif
#endif
   { "hwinfo",       DBG_CommandLine_GetHWInfo,       "Display the HW Info" },
#ifdef TM_ID_TEST_CODE
   { "idbufr",       DBG_CommandLine_IdBufRd,         "Reads from the interval data test buffer: bufIndex "},
   { "idbufw",       DBG_CommandLine_IdBufWr,         "Reads from the interval data test buffer: bufIndex floatVal"},
   { "idcfg",        DBG_CommandLine_IdCfg,           "Configure Interval Data: ch smpl_mS store mult div mode rti" },
   { "idcfgr",       DBG_CommandLine_IdCfgR,          "Read Interval Data Configuration" },
   { "idparams",     DBG_CommandLine_IdParams,        "Read/Write Parameters\n"
                   "                                   Read: No Params, Set: SamplePeriod BuSchedule" },
   { "idrd",         DBG_CommandLine_IdRd,            "Read Interval: ch yy mm dd hh mm" },
   { "idtimedv",     DBG_CommandLine_IdTimeDv,        "Get (no args) or set (one arg) Interval Data Time Diversity" },
#endif
#ifdef TM_ID_UNIT_TEST
//   { "idut",         DBG_CommandLine_IdUt,            "Run interval data self test: idut ch"},                         // RA6E1 Bob: This command was removed from original K24 code
#endif

   { "insertappmsg", DBG_CommandLine_InsertAppMsg,    "send NWK payload (arg1) into the bottom of the APP layer" },
#if ( RTOS_SELECTION == MQX_RTOS )
   { "insertmacmsg", DBG_CommandLine_InsertMacMsg,    "send phy payload (arg1) into the bottom of the MAC layer using\n"
                   "                                   rxFraming (arg2) and RxMode (arg3). No arg2/3 assumes SRFN" },
#elif ( RTOS_SELECTION == FREE_RTOS)
   { "insertmacmsg", DBG_CommandLine_InsertMacMsg,    "send phy payload (arg1) into the bottom of the MAC layer using\r\n"
                   "                                   rxFraming (arg2) and RxMode (arg3). No arg2/3 assumes SRFN" },
#endif // ( RTOS_SELECTION )
#if ( EP == 1 )
   { "led",          DBG_CommandLine_LED,             "Turn LEDs on/off" },
#if ( LP_IN_METER != 0 )
   { "lpstats",      DBG_CommandLine_lpstats,         "Dump Load Profile info. Usage: lptstats [first_block [last_block]]" },
#endif
#endif
   { "mac_time",     DBG_CommandLine_MacTimeCmd,      "Send a 'mac_time set' (DCU) or 'mac_time req' (EP) command" },
   { "mac_tsync",    DBG_CommandLine_TimeSync,        "Set/Get the TimeSync parameters" },
   { "macaddr",      DBG_CommandLine_MacAddr,         "Set/Get the MAC address" },
   { "macconfig",    DBG_CommandLine_MacConfig,       "Print the MAC Configuration" },
//   { "macflush",     DBG_CommandLine_MacFlush,        "mac flush" }, // RA6E1 Bob: This command was removed from original K24 code
//   { "macget",       DBG_CommandLine_MacGet,          "mac get"   }, // RA6E1 Bob: This command was removed from original K24 code
//   { "macpurge",     DBG_CommandLine_MacPurge,        "mac purge" }, // RA6E1 Bob: This command was removed from original K24 code
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
   { "mac_lp",       DBG_CommandLine_MAC_LinkParameters,"Get/Send MAC Link Parameters Config/Command" },
#endif
   { "macreset",     DBG_CommandLine_MacReset,        "mac reset" },
//   { "macset",       DBG_CommandLine_MacSet,          "mac set"   }, // RA6E1 Bob: This command was removed from original K24 code
//   { "macstart",     DBG_CommandLine_MacStart,        "mac start" }, // RA6E1 Bob: This command was removed from original K24 code
   { "macstats",     DBG_CommandLine_MacStats,        "Print the MAC stats" },
#if ( DCU == 1 )
   { "macpacketdelay",DBG_CommandLine_TxPacketDelay,   "get (no args) or set (arg1) MAC TxPacketDelay attribute" },
#endif
//   { "macstop",      DBG_CommandLine_MacStop,         "mac stop"  }, // RA6E1 Bob: This command was removed from original K24 code
   { "macrxtimeout", DBG_CommandLine_RxTimeout,       "get (no args) or set (arg1) MAC Reassembly Timeout in seconds" },
   { "macrelhigh",   DBG_CommandLine_RelHighCnt,      "get (no args) or set (arg1) MAC ReliabilityHighCount attribute" },
   { "macrelmed",    DBG_CommandLine_RelMedCnt,       "get (no args) or set (arg1) MAC ReliabilityMediumCount attribute" },
   { "macrellow",    DBG_CommandLine_RelLowCnt,       "get (no args) or set (arg1) MAC ReliabilityLowCount attribute" },
   { "mactxpause",   DBG_CommandLine_MacTxPause,      "pause transmit of mac for QoS testing" },
   { "mactxunpause", DBG_CommandLine_MacTxUnPause,    "unpause transmit of mac for QoS testing" },
#if ( OVERRIDE_TEMPERATURE == 1 )
   { "manTemp",      DBG_CommandLine_ManualTemperature, "Set/Get manual (temporary override) temperture" },
#endif
#if ( USE_MTLS == 1 )
   { "mtlsstats",    DBG_CommandLine_mtlsStats,       "Get/Reset the MTLS stats 'mtlsstats reset' to reset" },
#endif
#if ( CLOCK_IN_METER == 1 )
   { "mtrtime",      DBG_CommandLine_mtrTime,         "Convert meter time mm dd yy hh mm to system time" },
#endif
   { "networkid",    DBG_CommandLine_NetworkId,       "get (no args) or set (arg1) Network ID" },
   { "noiseband",    DBG_CommandLine_NoiseBand,       "Display/compute the noise for a range of channels" },
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
   { "noisebandClkOn",  DBG_CommandLine_NoiseBandClkOn,     "Display/compute the noise for a range of frequencies with 1MHz clock out" },
   { "noisebandClkOff", DBG_CommandLine_NoiseBandClkOff,    "Display/compute the noise for a range of frequencies without 1MHz clock out" },
#endif
   { "noiseestimate", DBG_CommandLine_NoiseEstimate,  "Display the noise estimate" },
   { "noiseestimatecount", DBG_CommandLine_NoiseEstimateCount, "Display/Set the noise estimate count" },
   { "noiseestimaterate", DBG_CommandLine_NoiseEstimateRate, "Display/Set the noise estimate rate" },
#if ( NOISE_HIST_ENABLED == 1 )
   { "noisehist",    DBG_CommandLine_NoiseHistogram,  "Display a noise histogram for channel(s)" },
   { "noiseburst",   DBG_CommandLine_NoiseBurst,      "Display noise bursts for channel(s)" },
   { "bursthist",    DBG_CommandLine_BurstHistogram,  "Display histgram of noise bursts for channel(s)" },
#endif
#if ( ( MULTIPLE_MAC != 0 ) && ( EP == 1 ) )
   { "NumMac",       DBG_CommandLine_NumMac,          "Set/Get number of mac addresses emulated (1-50)" },
#endif
   { "nvr",          DBG_CommandLine_NvRead,          "NV memory Read: Params - Partition Addr Len" },
#ifdef TM_DVR_EXT_FL_UNIT_TEST
   { "nvtest",       DBG_CommandLine_NvTest,          "[count (default=100)] Test a sector of external NV" },
#endif
   { "nwkget",       DBG_CommandLine_NwkGet,          "nwk get"  },
   { "nwkreset",     DBG_CommandLine_NwkReset,        "nwk reset"},
   { "nwkset",       DBG_CommandLine_NwkSet,          "nwk set"  },
   { "nwkstart",     DBG_CommandLine_NwkStart,        "nwk start"},
   { "nwkstats",     DBG_CommandLine_NwkStats,        "Print the NWK stats" },
   { "nwkstop",      DBG_CommandLine_NwkStop,         "nwk stop" },
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#if ( EP == 1 )
   { "ondemandread", DBG_CommandLine_OnDemandRead,    "Does on demand read of pre-set quantities" },
#endif
#endif
#if OTA_CHANNELS_ENABLED == 1
#if DCU == 1
   { "ota",          CommandLine_OTA_Cmd,             "Over the air command"},
#endif
#endif
#if ( PHASE_DETECTION == 1 )
   { "pd",           DBG_CommandLine_PhaseDetectCmd,  "Phase Detect Info : pd <info> <freq>" },
#endif
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
   { "padac",        DBG_CommandLine_PaDAC,           "Prints Power Amplifier DAC output" },
#if ( ENGINEERING_BUILD == 1 )
   { "PaDBMtoDAC",   DBG_CommandLine_PaCoeff,         "Get/Set Power Amplifier output" },
   { "PaVFWDtoDBM",  DBG_CommandLine_PaCoeff,         "Get/Set Power Amplifier output" },
#endif   //( ENGINEERING_BUILD == 1 )
#endif   //( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
//   { "packettimeout", DBG_CommandLine_PacketTimeout,  "get (no args) or set (arg1) MAC Packet Timeout in ms" }, // RA6E1 Bob: This command was removed from original K24 code
   { "part",         DBG_CommandLine_Partition,       "Prints partition data" },
//   { "phymaxtxlen",  DBG_CommandLine_GetPhyMaxTxPayload, "Usage:  'phymaxtxlen' get phy maximum TX payload" },  // RA6E1 Bob: This command was removed from original K24 code
   { "phyreset",     DBG_CommandLine_PhyReset,        "phy reset" },
//   { "phystart",     DBG_CommandLine_PhyStart,        "phy start" },                                            // RA6E1 Bob: This command was removed from original K24 code
   { "phystats",     DBG_CommandLine_PhyStats,        "Print the PHY stats" },
   { "phyconfig",    DBG_CommandLine_PhyConfig,       "Print the PHY Configuration" },
//   { "phystop",      DBG_CommandLine_PhyStop,         "phy stop"  },                                            // RA6E1 Bob: This command was removed from original K24 code
   { "ping",         DBG_CommandLine_MacPingCmd,      "Send a 'ping' request to EP"},
   { "power",        DBG_CommandLine_Power,           "Get power level when no arguments. Set power level (0-5)" },
#if ( EP == 1 )
   { "printdst",     DBG_CommandLine_getDstParams,    "Prints the DST params" },
#endif
   { "radiostatus",  DBG_CommandLine_RadioStatus,     "Get radio status" },
   { "reboot",       DBG_CommandLine_Reboot,          "Reboot device" },
#if ( EP == 1 )
   { "regstate",     DBG_CommandLine_RegState,        "Get or Set the Registration State" },
   { "regtimeouts",  DBG_CommandLine_RegTimeouts,     "Get or Set the Registration Timeouts" },
#endif
#if BOB_IS_LAZY == 1
   { "reset",        DBG_ResetAllStats,               "Reset PHY, MAC and NWK stats" },
#endif
#if ( CLOCK_IN_METER == 1 )
   { "revmtrtime",   DBG_CommandLine_mtrTime,         "Convert system time to meter time mm dd yy hh mm" },
#endif
#if ( FAKE_TRAFFIC == 1 )
   { "rfdutycycle",  DBG_CommandLine_ipRfDutyCycle,   "get (no args) or set (arg1) RF dutycycle" },
#endif
   { "rssi",         DBG_CommandLine_RSSI,            "Display the RSSI of a specified radio" },
//   { "rssijumpthreshold", DBG_CommandLine_RSSIJumpThreshold, "Get or Set the RSSI Jump Threshold" },            // RA6E1 Bob: This command was removed from original K24 code
#if ( MCU_SELECTED == NXP_K24 )
   { "rtcCap",       DBG_CommandLine_rtcCap,          "Get/Set Cap Load in RTC_CR" }, /* There is no analogous feature on the RA6E1 MCU */
#endif
#if ( MCU_SELECTED == NXP_K24 )
   { "rtctime",      DBG_CommandLine_rtcTime,         "Operates on RTC only. Read: No Params,\n"
                   "                                   Set: Params - yy mm dd hh mm ss" },
   { "rxchannels",   DBG_CommandLine_RxChannels,      "set/print the RX channel list.\n"
                   "                                   Type ""rxchannels"" with no arguments for more help" },
#elif ( MCU_SELECTED == RA6E1 )
   { "rtctime",      DBG_CommandLine_rtcTime,         "Operates on RTC only. Read: No Params,\r\n"
                   "                                   Set: Params - yy mm dd hh mm ss" },
   { "rxchannels",   DBG_CommandLine_RxChannels,      "set/print the RX channel list.\r\n"
                   "                                   Type ""rxchannels"" with no arguments for more help" },
#endif // ( MCU_SELECTED )
   { "rxdetection",  DBG_CommandLine_RxDetection,     "Set/print the detection configuration" },
   { "rxframing",    DBG_CommandLine_RxFraming,       "Set/print the framing configuration" },
   { "rxmode",       DBG_CommandLine_RxMode,          "Set/print the PHY mode configuration" },
#if ( DCU == 1 )
   { "sdtest",       DBG_CommandLine_sdtest,          "[count (default=1)] Exercise SDRAM" },
#endif
   { "secconfig",    DBG_CommandLine_secConfig,       "Set ECCX08 device configuration to Aclara defaults\r\n"
                   "                                   Plus unpublished functions - see source code for details" },
   { "sectest",      DBG_CommandLine_sectest,         "[count (default=1)] Exercise Security Device" },
   { "sendappmsg",   DBG_CommandLine_SendAppMsg,      "send data (arg1) to address (arg2) with qos (arg3)" },
#if (USE_IPTUNNEL == 1)
   { "sendtunmsg",   DBG_CommandLine_SendIpTunMsg,    "send data (arg1) to address (arg2) with qos (arg3)" },
#endif
   { "sendheadendmsg", DBG_CommandLine_SendHeadEndMsg,  "send data (arg1) to head end with qos (arg2)" },
   { "sendheepmsg",  DBG_CommandLine_SendHeepMsg,     "send dummy alarm header HEEP_MSG_TxHeepHdrOnly" },
   { "sendmtlsmsg",  DBG_CommandLine_SendAppMsg,      "send data (arg1) to address (arg2) with qos (arg3)" },
//   { "sendmeta",     DBG_CommandLine_SendMetadata,    "Trigger transmission of a Metadata message" }, // RA6E1 Bob: This command was removed from original K24 code
#if ( EP == 1 )
#if ( RTOS_SELECTION == MQX_RTOS )
   { "setdstenable", DBG_CommandLine_setDstEnable,   "Updates the DST enable:\n"
                   "                                   Params - enable flag (0 - disable or 1 - enable)" },
#elif ( RTOS_SELECTION == FREE_RTOS )
   { "setdstenable", DBG_CommandLine_setDstEnable,   "Updates the DST enable:\r\n"
                   "                                   Params - enable flag (0 - disable or 1 - enable)" },
#endif // ( RTOS_SELECTION )
   { "setdstoffset", DBG_CommandLine_setDstOffset,    "Updates the DST offset: Params - offset in seconds" },
#endif // ( EP == 1 )
#if ( RTOS_SELECTION == MQX_RTOS )
   { "setfreq",      DBG_CommandLine_SetFreq,         "set list of available frequencies.\n"
                   "                                   Type ""setfreq"" with no arguments for more help" },
#elif ( RTOS_SELECTION == FREE_RTOS )
   { "setfreq",      DBG_CommandLine_SetFreq,         "set list of available frequencies.\r\n"
                   "                                   Type ""setfreq"" with no arguments for more help" },
#endif // ( RTOS_SELECTION )
#if ( EP == 1 )
   { "setstartrule", DBG_CommandLine_setDstStartRule, "Updates the DST start rule: Params - mm dow ood hh mm" },
   { "setstoprule",  DBG_CommandLine_setDstStopRule,  "Updates the DST stop rule: Params - mm dow ood hh mm" },
   { "settimezoneoffset", DBG_CommandLine_setTimezoneOffset, "Updates the timezone offset: Params - offset in seconds" },
#endif
   // SM - Stack Manager
//   { "smstart",      DBG_CommandLine_SM_Start,         "SM start" },                                            // RA6E1 Bob: This command was removed from original K24 code
//   { "smstop",       DBG_CommandLine_SM_Stop,          "SM stop"  },                                            // RA6E1 Bob: This command was removed from original K24 code
//   { "smget",        DBG_CommandLine_SM_Get,           "SM get"   },                                            // RA6E1 Bob: This command was removed from original K24 code
//   { "smset",        DBG_CommandLine_SM_Set,           "SM set"   },                                            // RA6E1 Bob: This command was removed from original K24 code
   { "nwPassActTimeout",   DBG_CommandLine_SM_NwPassiveActTimeout, "Get or set passive network activity timeout" },
   { "nwActiveActTimeout", DBG_CommandLine_SM_NwActiveActTimeout,  "Get or set active network activity timeout" },
   { "smstats",      DBG_CommandLine_SM_Stats,        "Print the SM stats" },
   { "smconfig",     DBG_CommandLine_SM_Config,       "Print the SM Configuration" },

   { "stackusage",   DBG_CommandLine_StackUsage,      "print the stack of active tasks" },
#if ( TEST_SYNC_ERROR == 1 )
   { "syncerror",    DBG_CommandLine_SyncError,       "Set SYNC bit error " },
#endif
   { "tasksummary",  DBG_CommandLine_TaskSummary,     "print tasks summary" },
#if ( RTOS_SELECTION == MQX_RTOS )
   { "time",         DBG_CommandLine_time,            "RTC and SYS time.\n"
                   "                                   Read: No Params, Set: Params - yy mm dd hh mm ss" },
#elif ( RTOS_SELECTION == FREE_RTOS )
   { "testBSPSWDelay", DBG_CommandLine_TestSWDelay,   "Test Renesas BSP R_BSP_SoftwareDelay function" },
   { "testOsTaskSleep", DBG_CommandLine_TestOsTaskSleep, "Test OS_TASK_Sleep function" },
   { "time",         DBG_CommandLine_time,            "RTC and SYS time.\r\n"
                   "                                   Read: No Params, Set: Params - yy mm dd hh mm ss" },
#endif // ( RTOS_SELECTION )
#if ( DCU == 1 )
//   { "read_res",     CommandLine_ReadResource,       "Read a resource and value."},                             // RA6E1 Bob: This command was removed from original K24 code
   { "slot",         DBG_CommandLine_getTBslot,        "Get the slot of this TB." },

//   { "tr",           DBG_TraceRoute,                 "Trace Route"},                                            // RA6E1 Bob: This command was removed from original K24 code
#endif
   { "tunnel",       DBG_CommandLine_tunnel,          "Convert TWACS command to tunnel format" },
#if ( RTOS_SELECTION == MQX_RTOS )
   { "txchannels",   DBG_CommandLine_TxChannels,      "set/print the TX channel list.\n"
                   "                                   Type ""txchannels"" with no arguments for more help" },
   { "txmode",       DBG_CommandLine_TXMode,          "Choose TX mode: 0 = Normal (default), 1 = PN9,\n"
                   "                                   2 = Unmodulated Carrier Wave,\n"
                   "                                   3 = Deprecated,\n"
#if ( PORTABLE_DCU == 1)
                   "                                   4 = 4GFSK BER with manufacturing print out,\n"
                   "                                   5 = Old PHY (freq dev 600Hz, RS(63,59)" },
#else
                   "                                   4 = 4GFSK BER with manufacturing print out," },
#endif
#elif ( RTOS_SELECTION == FREE_RTOS )
   { "txchannels",   DBG_CommandLine_TxChannels,      "set/print the TX channel list.\r\n"
                   "                                   Type ""txchannels"" with no arguments for more help" },
   { "txmode",       DBG_CommandLine_TXMode,          "Choose TX mode: 0 = Normal (default), 1 = PN9,\r\n"
                   "                                   2 = Unmodulated Carrier Wave,\r\n"
                   "                                   3 = Deprecated,\r\n"
#if ( PORTABLE_DCU == 1)
                   "                                   4 = 4GFSK BER with manufacturing print out,\r\n"
                   "                                   5 = Old PHY (freq dev 600Hz, RS(63,59)" },
#else
                   "                                   4 = 4GFSK BER with manufacturing print out," },
#endif
#endif // ( RTOS_SELECTION )
#if ( EP == 1 ) && ( TEST_TDMA == 1 )
   { "txslot",       DBG_CommandLine_TxSlot,           "Specify which time slot to use for TDMA test\n" },
#endif
#if ( PORTABLE_DCU == 1)
   { "dfwmonitormode",  DBG_CommandLine_dfwMonMode,    "enable (1) or disable (0) mode to only print DFW responses\n" },
#endif
   { "ver",          DBG_CommandLine_Versions,        "Display the current Versions of components" },
   { "virgin",       DBG_CommandLine_virgin,          "Erases flash chip and resets the micro" },
   { "virgindelay",  DBG_CommandLine_virginDelay,     "Erases signature; continues. Allows new code load and then virgin" },
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
   { "vswr",         DBG_CommandLine_VSWR,            "Displays VSWR measurement" },
#endif
#if ( READ_KEY_ALLOWED != 0 )
   { "dumpKeys",     DBG_CommandLine_dumpKeys ,       "Dump 3 keys automatically replace on flash security change" },
   { "rkey",         DBG_CommandLine_readKey ,        "Read specifed key" },
   { "rkeyenc",      DBG_CommandLine_readKey ,        "Read specifed encrypted key" },
#endif
#if USE_USB_MFG
   { "usb",          DBG_CommandLine_usbaddr ,        "Display USB address" },
#endif
#if ( WRITE_KEY_ALLOWED != 0 )
   { "wkey",         DBG_CommandLine_writeKey ,       "Write specifed key with specified data" },
#endif
#if 0  // RA6E1 Bob: This command was removed from original K24 code
   { "x",            DBG_CommandLine_hardfault,       "Force a write to flash; test fault handler" },
#endif
#if ( SIMULATE_POWER_DOWN == 1 )
   { "PowerDownTest",DBG_CommandLine_SimulatePowerDown,"Simulate Power Down to measure the Worst Case Power Down Time" },
#endif
#if ( TM_CRC_UNIT_TEST == 1)
   { "crcunittest",          DBG_CommandLine_CrcCalculate,     "Displays CRC results" },
#endif
#if ( TM_TIME_COMPOUND_TEST == 1)
   { "ticktestelapsed",      DBG_CommandLine_TimeElapsed,    "Displays Time compound's difference in ElapsedMilliseconds" },
   { "ticktestnanosec",      DBG_CommandLine_TimeNanoSec,    "Displays Time compound's difference in nanosecond results" },
   { "ticktestmicrosec",     DBG_CommandLine_TimeMicroSec,   "Displays Time compound's difference in microsecond results" },
   { "ticktestmillisec",     DBG_CommandLine_TimeMilliSec,   "Displays Time compound's difference in millisecond results" },
   { "ticktestsec",          DBG_CommandLine_TimeSec,        "Displays Time compound's difference in second results" },
   { "ticktestmin",          DBG_CommandLine_TimeMin,        "Displays Time compound's difference in minute results" },
   { "ticktesthour",         DBG_CommandLine_TimeHour,       "Displays Time compound's difference in hour results" },
   { "ticktestmsectoticks",  DBG_CommandLine_TimeTicks,      "Displays Time compound's add millisecond to tickcount results" },
   { "ticktestfuture",       DBG_CommandLine_TimeFuture,     "Displays Time compound's difference between two tick struct results" },
#endif
#if ( TM_OS_EVENT_TEST == 1)
   { "eventtestset",         DBG_CommandLine_OS_EventSet,    "Sets the Event Bits argument is SetBit (hex) " },
   { "createtesteventandwait",  DBG_CommandLine_OS_EventCreateWait,     "Create and Wait for Event Set Bits argument\r\n"
                                     "                                   are WaitBit (hex) and WaitForAll (bool)" },
   { "deleteeventtesttask",  DBG_CommandLine_OS_EventTaskDelete,        "Deletes the Event Test Task" },
#endif
#if ( TM_LINKED_LIST == 1)
   { "oslinkedlistcreate",   DBG_CommandLine_OS_LinkedList_Create,      "Creates test LinkedList" },
   { "oslinkedlistremove",   DBG_CommandLine_OS_LinkedList_Remove,      "Removes a element from test LinkedList" },
   { "oslinkedlistenq",      DBG_CommandLine_OS_LinkedList_Enqueue,     "Enqueues a element to test LinkedList" },
   { "oslinkedlistdeq",      DBG_CommandLine_OS_LinkedList_Dequeue,     "Dequeue a element from test LinkedList" },
   { "oslinkedlistinsert",   DBG_CommandLine_OS_LinkedList_Insert,      "Insert a element at a specific index in test LinkedList" },
   { "oslinkedlistnext",     DBG_CommandLine_OS_LinkedList_Next,        "Returns the Next element from test LinkedList" },
   { "oslinkedlisthead",     DBG_CommandLine_OS_LinkedList_Head,        "Returns the Head element from test LinkedList" },
   { "oslinkedlistnumele",   DBG_CommandLine_OS_LinkedList_NumElements, "Adds LinkedList element and checks for the count" },
#endif
   { 0, 0, 0 }
};

/* FUNCTION PROTOTYPES */

static void DBG_CommandLine_Process ( void );
static returnStatus_t atoh( uint8_t *pHex, char const *pAscii );
#if ( TM_OS_EVENT_TEST == 1)
void OS_EVNTTestTask ( taskParameter );         /* A Task to test Event wait */
#endif

/* FUNCTION DEFINITIONS */
/*******************************************************************************

   Function name: DBG_CommandLineTask

   Purpose: This function is a Task and will parse the Serial Received data and
            take action on the commands that are received.

   Arguments: taskParameter - Not used, but required here because this is a task

   Returns:void

   Notes:

*******************************************************************************/
void DBG_CommandLineTask ( taskParameter )
{
#if ( BUILD_DFW_TST_LARGE_PATCH == 1 )/* Enabling this in CompileSwitch.h will shift the code down resulting in a large patch */
   NOP();
   NOP();
   NOP();
#endif

   /* Print out the version information one time at power up */
   OS_TASK_Sleep ( 100 ); /* Correct a display issue with STRT trying to print reset reason */
#if ( ( EP == 1 ) || ( PORTABLE_DCU == 0 ) )
   DBG_logPrintf( 'I', "############## APP STARTING ##############" );
   (void)DBG_CommandLine_Versions ( 0, argvar );
#else
   DBG_logPrintf('R', "#####################MOBILE DCU TEST UNIT FIRMWARE#####################");
   (void)DBG_CommandLine_Versions ( 0, argvar );
   DBG_logPrintf('R', "#####################MOBILE DCU TEST UNIT FIRMWARE#####################");
#endif
   for ( ;; )
   {
#if (TM_SEMAPHORE == 1)
      if( OS_SEM_TestPend() )
      {
         vTaskDelay(pdMS_TO_TICKS(1000));
         vTaskSuspend(NULL);
         break; /* Exit */
      }

#endif
#if (TM_MSGQ == 1)
      if( OS_MSGQ_TestPend() )
      {
         vTaskDelay(pdMS_TO_TICKS(1000));
         vTaskSuspend(NULL);
         break; /* Exit */
      }

#endif
#if( TM_EVENTS == 1 )
      if( OS_EVENT_TestWait() )
      {
         vTaskDelay(pdMS_TO_TICKS(1000));
         vTaskSuspend(NULL);
         break; /* Exit */
      }
#endif
#if ( !USE_USB_MFG && ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A ) )
      OS_TASK_Sleep(OS_WAIT_FOREVER);
#else
      UART_fgets( UART_DEBUG_PORT, DbgCommandBuffer, MAX_DBG_COMMAND_CHARS );
#endif
      /* Check before executing; may have been disable while waiting for input   */
      if ( DBG_IsPortEnabled () ) /* Only process input if the port is enabled */
      {
         DBG_CommandLine_Process();
#if ( MCU_SELECTED == RA6E1 )
         memset( DbgCommandBuffer, 0, sizeof( DbgCommandBuffer ) );
#endif
      }
      else
      {
         (void)DBG_CommandLine_DebugDisable ( 0, NULL ); /*lint !e413 NULL OK; not used   */
#if ( MCU_SELECTED == NXP_K24 )
#if 0
         (void)UART_flush( UART_DEBUG_PORT );                   /* Drop any input queued up while debug disabled   */
#endif
#endif
      }/* end if() */
   } /* end for() */
} /* end DBG_CommandLineTask () */

/*******************************************************************************

   Function name: DBG_CommandLine_Process

   Purpose: This function processes a command line that was received and matches
            it up to the pre-defined list of commands in this file.  if a match
            is found, the corresponding function handler is called

   Arguments: None

   Returns: Void

   Notes:

*******************************************************************************/
static void DBG_CommandLine_Process ( void )
{
   char     *CmdString;
   uint32_t commandLen = 0;
   uint32_t argc = 0;
   bool     NextArg = ( bool )true;
   uint16_t i;
   struct_CmdLineEntry const * CmdEntry;
#if (DCU == 1)
   uint8_t slot;
#endif

   CmdString = &DbgCommandBuffer[0];

   /* handle backspaces */
   if ( *CmdString != 0 )
   {
      commandLen = ( uint32_t )strlen( CmdString );
      commandLen = min( commandLen, ( uint32_t )MAX_DBG_COMMAND_CHARS );
      //lint -e{850} intentional manipulation of i within the for loop
      for ( i = 0; i < commandLen; i++ )
      {
         if ( *( CmdString + i ) == 0x7F )
         {
            if ( ( i >= 1 ) && ( i < ( sizeof( DbgCommandBuffer ) - 2 ) ) )
            {
               /* remove 2 chars.  both the backspace and the char before it */
               /* Note: The memory in memcpy cannot overlap inorder to avoid undefined behaviour use memmove instead */
               ( void )memmove( ( CmdString + ( i - 1 ) ), ( CmdString + ( i + 1 ) ), ( commandLen - ( i + 1 ) ) );   /*lint !e669 !e670  out of bounds access  */
               commandLen -= 2;
               i -= 2;

               *( CmdString + commandLen ) = 0; //lint !e661 !e662 this will not point out of bounds
            }
            else
            {
               /* remove a leading backspace */
               /* Note: The memory in memcpy cannot overlap inorder to avoid undefined behaviour use memmove instead */
               ( void )memmove( CmdString, ( CmdString + 1 ), ( commandLen - 1 ) );
               commandLen--;
               i--;
               *( CmdString + commandLen ) = 0;
            }
         }
      }
   }

   while ( *CmdString != 0 )
   {
      if ( *CmdString == ' ' )
      {
         *CmdString = 0;
         NextArg = ( bool )true;
      } /* end if() */
      else if ( NextArg )
      {
         if ( argc < MAX_CMDLINE_ARGS )
         {
            argvar[argc] = CmdString;
            argc++;
            NextArg = ( bool )false;
         } /* end if() */
         else
         {
            break;
         } /* end else() */
      } /* end if() */

      CmdString++;
   } /* end while() */

   if ( argc > 0 )
   {
      CmdEntry = DBG_CmdTable;

      while ( CmdEntry->pcCmd != 0 )
      {
         if ( !strcasecmp( argvar[0], CmdEntry->pcCmd ) )
         {
            /* This is the line of code that is actually calling the handler function
               for the command that was received */
#if 0 // TODO: RA6E1 Bob: making the #pragma into _pragma with conditional compile is a big job.  Deferred for now.
#if ( EP == 1 )
#if ( ACLARA_LC != 1 ) && ( ACLARA_DA != 1 ) /* meter specific code */
/* TODO: Make a group of the calls by feature set ( similar to HEEP_Util.c pragma calls ) */
#pragma  calls=DBG_CommandLine_Help,DBG_CommandLine_PWR_BoostTest, DBG_CommandLine_Buffers, \
               DBG_CommandLine_PWR_SuperCap, DBG_CommandLine_CCA, DBG_CommandLine_CCAAdaptiveThreshold, \
               DBG_CommandLine_CCAAdaptiveThresholdEnable, DBG_CommandLine_CCAOffset, DBG_CommandLine_clocktst, \
               DBG_CommandLine_Comment, DBG_CommandLine_Counters, DBG_CommandLine_CpuLoadDisable, \
               DBG_CommandLine_CpuLoadEnable, DBG_CommandLine_CpuLoadSmart, DBG_CommandLine_crc, \
               DBG_CommandLine_DebugDisable, \
               DBG_CommandLine_DebugFilter, DBG_CommandLine_SchDmdRst, DBG_CommandLine_DMDTO, \
               DBG_CommandLine_DFW_Status, DBG_CommandLine_DfwTimeDv, DBG_CommandLine_ds, DBG_CommandLine_DsTimeDv, \
               DBG_CommandLine_Dtls, DBG_CommandLine_DtlsStats, DBG_CommandLine_DMDDump, DBG_CommandLine_EVLADD, \
               DBG_CommandLine_EVLCMD, DBG_CommandLine_EVLGETLOG, DBG_CommandLine_EVLQ, DBG_CommandLine_EVLSEND, \
               DBG_CommandLine_PrintFiles, DBG_CommandLine_DumpFiles, DBG_CommandLine_FreeRAM, \
               DBG_CommandLine_FrontEndGain, DBG_CommandLine_GenDFWkey, DBG_CommandLine_getDstEnable, \
               DBG_CommandLine_getDstHash, DBG_CommandLine_getDstOffset, DBG_CommandLine_GetFreq, \
               DBG_CommandLine_getDstStartRule, DBG_CommandLine_getDstStopRule, DBG_CommandLine_getTimezoneOffset, \
               DBG_CommandLine_HmcCmd, DBG_CommandLine_HmcCurrent, DBG_CommandLine_HmcDemand, \
               DBG_CommandLine_tunnel,DBG_CommandLine_HmcDemandCoin, \
               DBG_CommandLine_HmcHist, DBG_CommandLine_IdCfg, DBG_CommandLine_IdCfgR, DBG_CommandLine_IdParams, \
               DBG_CommandLine_IdRd, DBG_CommandLine_IdTimeDv,  DBG_CommandLine_InsertAppMsg, DBG_CommandLine_LED, DBG_CommandLine_InsertMacMsg, \
               DBG_CommandLine_MacTimeCmd, DBG_CommandLine_MacAddr, \
               DBG_CommandLine_MacReset, \
               DBG_CommandLine_MacStats, DBG_CommandLine_MacConfig, \
               DBG_CommandLine_MacTxPause, DBG_CommandLine_MacTxUnPause, DBG_CommandLine_mtlsStats, DBG_CommandLine_NetworkId, \
               DBG_CommandLine_mtrTime, DBG_CommandLine_NoiseBand, DBG_CommandLine_NoiseEstimate, DBG_CommandLine_NoiseEstimateCount, \
               DBG_CommandLine_NoiseEstimateRate, DBG_CommandLine_NvRead, DBG_CommandLine_NvTest, \
               DBG_CommandLine_NwkGet, DBG_CommandLine_NwkReset, DBG_CommandLine_NwkSet, DBG_CommandLine_NwkStart, \
               DBG_CommandLine_NwkStats, DBG_CommandLine_NwkStop, DBG_CommandLine_OnDemandRead, \
               DBG_CommandLine_Partition, \
               DBG_CommandLine_PhyReset, DBG_CommandLine_PhyStats, \
               DBG_CommandLine_PhyConfig, DBG_CommandLine_MacPingCmd, DBG_CommandLine_Power, DBG_CommandLine_PWR_BoostMode, \
               DBG_CommandLine_getDstParams, DBG_CommandLine_RadioStatus, DBG_CommandLine_Reboot, \
               DBG_CommandLine_RegState, DBG_CommandLine_RegTimeouts, DBG_CommandLine_RSSI, \
               DBG_CommandLine_rtcCap, DBG_CommandLine_rtcTime, \
               DBG_CommandLine_RxChannels, DBG_CommandLine_RxDetection, DBG_CommandLine_RxFraming, \
               DBG_CommandLine_RxMode, DBG_CommandLine_RxTimeout, DBG_CommandLine_secConfig, DBG_CommandLine_sectest, \
               DBG_CommandLine_SendAppMsg, DBG_CommandLine_SendHeadEndMsg, DBG_CommandLine_SendHeepMsg, \
               DBG_CommandLine_setDstEnable, DBG_CommandLine_setDstOffset, \
               DBG_CommandLine_SetFreq, DBG_CommandLine_setDstStartRule, DBG_CommandLine_setDstStopRule, \
               DBG_CommandLine_setTimezoneOffset, DBG_CommandLine_SM_NwActiveActTimeout, DBG_CommandLine_SM_NwPassiveActTimeout, \
               DBG_CommandLine_StackUsage, DBG_CommandLine_TaskSummary, \
               DBG_CommandLine_time,  DBG_CommandLine_TimeSync, DBG_CommandLine_TxChannels, DBG_CommandLine_TXMode, DBG_CommandLine_Versions, \
               DBG_CommandLine_virgin, DBG_CommandLine_virginDelay, DBG_CommandLine_RelHighCnt, \
               DBG_CommandLine_RelMedCnt, DBG_CommandLine_RelLowCnt, DBG_CommandLine_SM_Config, \
               DBG_CommandLine_SM_Stats, DBG_CommandLine_HmcwCmd,  DBG_CommandLine_NumMac, DBG_CommandLine_crc16m, \
               DBG_CommandLine_AfcEnable, DBG_CommandLine_AfcTemperatureRange, DBG_CommandLine_AfcRSSIThreshold, \
               DBG_CommandLine_ManualTemperature, \
               DBG_CommandLine_GetHWInfo, DBG_CommandLine_NumMac,  DBG_CommandLine_crc16m, \
               DBG_CommandLine_AfcEnable, DBG_CommandLine_AfcRSSIThreshold,  DBG_CommandLine_AfcTemperatureRange
#if ( USE_USB_MFG != 0 )
#pragma  calls=DBG_CommandLine_BDT
#endif // ( USE_USB_MFG != 0 )
#else // LC or DA
#pragma  calls=DBG_CommandLine_Help,DBG_CommandLine_PWR_BoostTest, DBG_CommandLine_Buffers, \
               DBG_CommandLine_PWR_SuperCap, DBG_CommandLine_CCA, DBG_CommandLine_CCAAdaptiveThreshold, \
               DBG_CommandLine_CCAAdaptiveThresholdEnable, DBG_CommandLine_CCAOffset, DBG_CommandLine_clocktst, \
               DBG_CommandLine_Comment, DBG_CommandLine_Counters, DBG_CommandLine_CpuLoadDisable, \
               DBG_CommandLine_CpuLoadEnable, DBG_CommandLine_CpuLoadSmart, DBG_CommandLine_crc, \
               DBG_CommandLine_DebugDisable, \
               DBG_CommandLine_DebugFilter, \
               DBG_CommandLine_DFW_Status, DBG_CommandLine_DfwTimeDv, \
               DBG_CommandLine_Dtls, DBG_CommandLine_DtlsStats, DBG_CommandLine_EVLADD, \
               DBG_CommandLine_EVLCMD, DBG_CommandLine_EVLGETLOG, DBG_CommandLine_EVLQ, DBG_CommandLine_EVLSEND, \
               DBG_CommandLine_PrintFiles, DBG_CommandLine_DumpFiles, DBG_CommandLine_FreeRAM, \
               DBG_CommandLine_FrontEndGain, DBG_CommandLine_GenDFWkey, DBG_CommandLine_getDstEnable, \
               DBG_CommandLine_getDstHash, DBG_CommandLine_getDstOffset, DBG_CommandLine_GetFreq, \
               DBG_CommandLine_getDstStartRule, DBG_CommandLine_getDstStopRule, DBG_CommandLine_getTimezoneOffset, \
               DBG_CommandLine_LED, DBG_CommandLine_InsertMacMsg, \
               DBG_CommandLine_MacTimeCmd, DBG_CommandLine_MacAddr, \
               DBG_CommandLine_MacReset, \
               DBG_CommandLine_MacStats, DBG_CommandLine_MacConfig, \
               DBG_CommandLine_MacTxPause, DBG_CommandLine_MacTxUnPause, DBG_CommandLine_NetworkId, \
               DBG_CommandLine_NoiseBand, DBG_CommandLine_NoiseEstimate, DBG_CommandLine_NoiseEstimateCount, \
               DBG_CommandLine_NoiseEstimateRate, DBG_CommandLine_NvRead, DBG_CommandLine_NvTest, \
               DBG_CommandLine_NwkGet, DBG_CommandLine_NwkReset, DBG_CommandLine_NwkSet, DBG_CommandLine_NwkStart, \
               DBG_CommandLine_NwkStats, DBG_CommandLine_NwkStop, \
               DBG_CommandLine_Partition, \
               DBG_CommandLine_PhyReset, DBG_CommandLine_PhyStats, \
               DBG_CommandLine_PhyConfig, DBG_CommandLine_MacPingCmd, DBG_CommandLine_Power, \
               DBG_CommandLine_getDstParams, DBG_CommandLine_RadioStatus, DBG_CommandLine_Reboot, \
               DBG_CommandLine_RegState, DBG_CommandLine_RegTimeouts, DBG_CommandLine_RSSI, \
               DBG_CommandLine_rtcCap, DBG_CommandLine_rtcTime, \
               DBG_CommandLine_RxChannels, DBG_CommandLine_RxDetection, DBG_CommandLine_RxFraming, \
               DBG_CommandLine_RxDetection, DBG_CommandLine_RxFraming, DBG_CommandLine_RxMode, \
               DBG_CommandLine_RxMode, DBG_CommandLine_RxTimeout, DBG_CommandLine_secConfig, DBG_CommandLine_sectest, \
               DBG_CommandLine_SendAppMsg, DBG_CommandLine_SendHeadEndMsg, DBG_CommandLine_SendHeepMsg, \
               DBG_CommandLine_setDstEnable, DBG_CommandLine_setDstOffset, \
               DBG_CommandLine_SetFreq, DBG_CommandLine_setDstStartRule, DBG_CommandLine_setDstStopRule, \
               DBG_CommandLine_setTimezoneOffset, DBG_CommandLine_StackUsage, DBG_CommandLine_TaskSummary, \
               DBG_CommandLine_time,  DBG_CommandLine_TxChannels, DBG_CommandLine_TXMode, DBG_CommandLine_Versions, \
               DBG_CommandLine_virgin, DBG_CommandLine_virginDelay, DBG_CommandLine_RelHighCnt, \
               DBG_CommandLine_RelMedCnt, DBG_CommandLine_RelLowCnt, DBG_CommandLine_SM_Config, \
               DBG_CommandLine_SM_Stats, DBG_CommandLine_NumMac, DBG_CommandLine_crc16m, \
               DBG_CommandLine_AfcEnable,  DBG_CommandLine_AfcTemperatureRange, DBG_CommandLine_AfcRSSIThreshold, \
               DBG_CommandLine_ManualTemperature, \
               DBG_CommandLine_GetHWInfo,  DBG_CommandLine_NumMac,  DBG_CommandLine_crc16m, \
               DBG_CommandLine_AfcEnable, DBG_CommandLine_AfcRSSIThreshold,  DBG_CommandLine_AfcTemperatureRange, \
               DBG_CommandLine_tunnel
#if ( USE_USB_MFG != 0 )
#pragma  calls=DBG_CommandLine_BDT
#endif // ( USE_USB_MFG != 0 )
#endif // #if ( ACLARA_LC != 1 ) && ( ACLARA_DA != 1 )
#else  // DCU
#pragma  calls=DBG_CommandLine_Help,DBG_CommandLine_PWR_BoostTest, DBG_CommandLine_Buffers, \
               DBG_CommandLine_PWR_SuperCap, DBG_CommandLine_CCA, DBG_CommandLine_CCAAdaptiveThreshold, \
               DBG_CommandLine_CCAAdaptiveThresholdEnable, DBG_CommandLine_CCAOffset, DBG_CommandLine_clocktst, \
               DBG_CommandLine_Comment, DBG_CommandLine_Counters, DBG_CommandLine_CpuLoadDisable, \
               DBG_CommandLine_CpuLoadEnable, DBG_CommandLine_CpuLoadSmart, DBG_CommandLine_crc, \
               DBG_CommandLine_DebugDisable, \
               DBG_CommandLine_DebugFilter, DBG_CommandLine_SchDmdRst, DBG_CommandLine_DMDTO, \
               DBG_CommandLine_DFW_Status, DBG_CommandLine_DfwTimeDv, DBG_CommandLine_ds, DBG_CommandLine_DsTimeDv, \
               DBG_CommandLine_Dtls, DBG_CommandLine_DtlsStats, DBG_CommandLine_DMDDump, DBG_CommandLine_EVLADD, \
               DBG_CommandLine_EVLCMD, DBG_CommandLine_EVLGETLOG, DBG_CommandLine_EVLQ, DBG_CommandLine_EVLSEND, \
               DBG_CommandLine_PrintFiles, DBG_CommandLine_DumpFiles, DBG_CommandLine_FreeRAM, \
               DBG_CommandLine_FrontEndGain, DBG_CommandLine_GenDFWkey, DBG_CommandLine_getDstEnable, \
               DBG_CommandLine_getDstHash, DBG_CommandLine_getDstOffset, DBG_CommandLine_GetFreq, \
               DBG_CommandLine_getDstStartRule, DBG_CommandLine_getDstStopRule, DBG_CommandLine_getTimezoneOffset, \
               DBG_CommandLine_HmcCmd, DBG_CommandLine_HmcCurrent, DBG_CommandLine_HmcDemand, \
               DBG_CommandLine_tunnel,DBG_CommandLine_HmcDemandCoin, \
               DBG_CommandLine_HmcHist, DBG_CommandLine_IdCfg, DBG_CommandLine_IdCfgR, DBG_CommandLine_IdParams, \
               DBG_CommandLine_IdRd, DBG_CommandLine_IdTimeDv,  DBG_CommandLine_InsertAppMsg, DBG_CommandLine_LED, DBG_CommandLine_InsertMacMsg, \
               DBG_CommandLine_MacTimeCmd, DBG_CommandLine_MacAddr, \
               DBG_CommandLine_MacReset, \
               DBG_CommandLine_MacStats, DBG_CommandLine_MacConfig, \
               DBG_CommandLine_MacTxPause, DBG_CommandLine_MacTxUnPause, DBG_CommandLine_NetworkId, \
               DBG_CommandLine_NoiseBand, DBG_CommandLine_NoiseEstimate, DBG_CommandLine_NoiseEstimateCount, \
               DBG_CommandLine_NoiseEstimateRate, DBG_CommandLine_NvRead, DBG_CommandLine_NvTest, \
               DBG_CommandLine_NwkGet, DBG_CommandLine_NwkReset, DBG_CommandLine_NwkSet, DBG_CommandLine_NwkStart, \
               DBG_CommandLine_NwkStats, DBG_CommandLine_NwkStop, \
               DBG_CommandLine_Partition, \
               DBG_CommandLine_PhyReset, DBG_CommandLine_PhyStats, \
               DBG_CommandLine_PhyConfig, DBG_CommandLine_MacPingCmd, DBG_CommandLine_Power, \
               DBG_CommandLine_getDstParams, DBG_CommandLine_RadioStatus, DBG_CommandLine_Reboot, \
               DBG_CommandLine_RegTimeouts, DBG_CommandLine_RSSI, \
               DBG_CommandLine_rtcCap, DBG_CommandLine_rtcTime, \
               DBG_CommandLine_RxChannels, DBG_CommandLine_RxDetection, DBG_CommandLine_RxFraming, \
               DBG_CommandLine_RxMode, DBG_CommandLine_RxTimeout, DBG_CommandLine_secConfig, DBG_CommandLine_sectest, \
               DBG_CommandLine_SendAppMsg, DBG_CommandLine_SendHeadEndMsg, DBG_CommandLine_SendHeepMsg, \
               DBG_CommandLine_setDstEnable, DBG_CommandLine_setDstOffset, \
               DBG_CommandLine_SetFreq, DBG_CommandLine_setDstStartRule, DBG_CommandLine_setDstStopRule, \
               DBG_CommandLine_setTimezoneOffset, DBG_CommandLine_SM_NwActiveActTimeout, DBG_CommandLine_SM_NwPassiveActTimeout, \
               DBG_CommandLine_StackUsage, DBG_CommandLine_TaskSummary, \
               DBG_CommandLine_time,  DBG_CommandLine_TimeSync, DBG_CommandLine_TxChannels, DBG_CommandLine_TXMode, DBG_CommandLine_Versions, \
               DBG_CommandLine_virgin, DBG_CommandLine_virginDelay, DBG_CommandLine_RelHighCnt, \
               DBG_CommandLine_RelMedCnt, DBG_CommandLine_RelLowCnt, DBG_CommandLine_SM_Config, \
               DBG_CommandLine_SM_Stats, \
               DBG_CommandLine_AfcEnable, DBG_CommandLine_AfcTemperatureRange, DBG_CommandLine_AfcRSSIThreshold, \
               DBG_CommandLine_ManualTemperature, \
               DBG_CommandLine_GetHWInfo, \
               DBG_CommandLine_AfcEnable, DBG_CommandLine_AfcRSSIThreshold,  DBG_CommandLine_AfcTemperatureRange, \
               DBG_CommandLine_SendIpTunMsg, DBG_CommandLine_sdtest, DBG_CommandLine_getTBslot, DBG_CommandLine_TxPacketDelay
#if ( USE_USB_MFG != 0 )
#warning "Fix this, the list can't be built like this, which is why the whole list is cloned 3 times"
#pragma  calls=DBG_CommandLine_usbaddr
#endif // ( USE_USB_MFG != 0 )
#endif // ( EP == 1 )
#endif /* #if 0 // TODO: RA6E1 Bob: making the #pragma into _pragma with conditional compile is a big job.  Deferred for now.*/
            (void)CmdEntry->pfnCmd( argc, argvar );

            /***************************************************************************
               Reset the debug enable timer upon execution of a valid command
            ****************************************************************************/
            if ( DBG_IsPortEnabled () )
            {
               DBG_PortTimer_Manage ( );
            }
            /***************************************************************************
               Reset the rfTestmode timer upon execution of a valid command
            ****************************************************************************/
#if ( EP == 1 )
            if ( MODECFG_get_rfTest_mode() != 0 )
            {
               MFGP_rfTestTimerReset();
            }
#endif
            break;
         } /* end if() */

         CmdEntry++;
      } /* end while() */

      if ( CmdEntry->pcCmd == 0 )
      {
#if (DCU == 1)
         if ( VER_isComDeviceStar() == (bool)true )
         {
            /* set slot to 0xFF (everyone) so bang command will be treated as STAR version */
            slot = 0xFF;
         }
         else
         {
            SLOT_ADDR(slot);
         }
         if ( ! MAINBD_ProcessCmd( UART_DEBUG_PORT,
                                   (uint8_t*)DbgCommandBuffer,
                                   sizeof(DbgCommandBuffer),
                                   (uint16_t)commandLen,
                                   slot) )
#endif
         {
            /* We reached the end of the list and did not find a valid command */
            DBG_logPrintf( 'R', "%s is not a valid command!", argvar[0] );
         }
      } /* end if() */
   } /* end if() */
} /* end DBG_CommandLine_Process () */

/*******************************************************************************

   Function name: DBG_CommandLine_Help

   Purpose: Print the help menu for the command line interface

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_Help ( uint32_t argc, char *argv[] )
{
   struct_CmdLineEntry  const *CmdLineEntry;
#if ( RTOS_SELECTION == MXQ_RTOS )
   DBG_printf( "\n[M]Command List:" );
#elif ( RTOS_SELECTION == FREE_RTOS )
/* Added carriage return to follow printing standard */
   DBG_printf( "\r\n[M]Command List:" );
#endif
   CmdLineEntry = DBG_CmdTable;
   while ( CmdLineEntry->pcCmd )
   {
      DBG_printf( "[M]%30s: %s", CmdLineEntry->pcCmd, CmdLineEntry->pcHelp );
      CmdLineEntry++;
      OS_TASK_Sleep( TEN_MSEC );
   } /* end while() */
   return ( 0 );
} /* end DBG_CommandLine_Help () */


/***********************************************************************************************************************
   Function Name: DBG_CommandLine_wdTest

   Purpose: Test the IWDT

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/

uint32_t DBG_CommandLine_wdTest ( uint32_t argc, char *argv[] )
{
   if ( 2 == argc )
   {
      int16_t wdtNo = atoi( argv[1] );
      if ( wdtNo == 1 )
      {
         /* Refresh counter value */
         WDOG_Kick();
      }
      else if ( wdtNo == 2 )
      {
         /* Test the IWDT */
         SELF_testIWDT();
      }
      else
      {
         DBG_logPrintf( 'R', "watchDogTest 1 for refreshing the watchdog timer , watchDogTest 2 (for validating the watchdog timer)" );
      }
   }
   return 0;
}

#if ( DAC_CODE_CONFIG == 1 )
/******************************************************************************

   Function Name: DBG_CommandLine_DAC_SetDacStep

   Purpose: Set values to the DAC0 Data registers (DAC0_DAT0H & DAC0_DAT0L)

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_DAC_SetDacStep ( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   // More than one parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
      OS_TASK_Sleep( TEN_MSEC );
   }
   else
   {
      // No parameters
      if ( argc == 1 )
      {
         DBG_logPrintf( 'R', "Set the DAC Step, range for steps is 0 - 4095 eg.) setdac0step 2200");
         OS_TASK_Sleep( TEN_MSEC );
      }
      else
      {
         uint16_t dac0StepVal;
         dac0StepVal = ( uint16_t )atoi( argv[1] );
         DBG_logPrintf( 'R', "setdac0step %d", dac0StepVal );
         OS_TASK_Sleep( TEN_MSEC );
         if ( dac0StepVal < 4096)
         {
            DVR_DAC0_setVoltageStep( dac0StepVal );

            retVal = eSUCCESS;
         }
         else
         {
            DBG_logPrintf( 'R', "Value out of bounds, the value range is 0 - 4095");
            OS_TASK_Sleep( TEN_MSEC );
         }
      }
   }

   return ( uint32_t )retVal;
}
/******************************************************************************

   Function Name: DBG_CommandLine_setPwrSel

   Purpose: Set/Rest the pwrsel pin

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_setPwrSel ( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   R_BSP_PinAccessEnable();

   // More than one parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
      OS_TASK_Sleep( TEN_MSEC );
   }
   else
   {
      // No parameters
      if ( argc == 1 )
      {
         DBG_logPrintf( 'R', "pwrsel 1/0");
         OS_TASK_Sleep( TEN_MSEC );
      }
      else
      {
         uint16_t pwrSelVal;
         pwrSelVal = ( uint16_t )atoi( argv[1] );
         DBG_logPrintf( 'R', "pwrsel %d", pwrSelVal );
         OS_TASK_Sleep( TEN_MSEC );
         if ( pwrSelVal == 1)
         {
            R_BSP_PinWrite(BSP_IO_PORT_07_PIN_08, BSP_IO_LEVEL_HIGH);
         }
         else
         {
            R_BSP_PinWrite(BSP_IO_PORT_07_PIN_08, BSP_IO_LEVEL_LOW);
         }

         retVal = eSUCCESS;
      }
   }

   return ( uint32_t )retVal;
}

#endif


#if ENABLE_DFW_TASKS
/*******************************************************************************

   Function name: DBG_CommandLine_DFW_Status

   Purpose: Display the current status of Download Firmware Application

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_DFW_Status( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      DFWI_DisplayStatus();
   }
   else
   {
      DBG_logPrintf( 'R', "ERROR - Invalid Param" );
   }
   return ( 0 );
}
#if ( EP == 1 )
/*******************************************************************************

   Function name: DBG_CommandLine_DfwTimeDv

   Purpose: This function will get or set the DFW time diversity.

   Arguments:  argc - Number of Arguments passed to this function
               argv - Pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_DfwTimeDv( uint32_t argc, char *argv[] )
{
   returnStatus_t        retVal = eFAILURE;
   dfwTdConfigFileData_t DfwTimeDiversity;

   if ( 1 == argc )
   {
      ( void )DFWTDCFG_GetTimeDiversity( &DfwTimeDiversity );
      DBG_logPrintf( 'R', "%s %u %u", argv[ 0 ], DfwTimeDiversity.uDfwDownloadConfirmTimeDiversity,
                     DfwTimeDiversity.uDfwApplyConfirmTimeDiversity );
   }
   else if ( 3 == argc )
   {
      // Write DfwTimeDiversity
      DfwTimeDiversity.uDfwDownloadConfirmTimeDiversity = ( uint8_t )atoi( argv[1] );
      DfwTimeDiversity.uDfwApplyConfirmTimeDiversity    = ( uint8_t )atoi( argv[2] );
      retVal = DFWTDCFG_SetTimeDiversity( &DfwTimeDiversity );

      if ( eSUCCESS != retVal )
      {
         DBG_logPrintf( 'R', "Error writing DFW Time Diversity. Error code %u", ( uint32_t )retVal );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   return( 0 );
} /* end DBG_CommandLine_DfwTimeDv () */
#endif   //End of #if EP
#endif   //End of #if ENABLE_DFW_TASKS

#if 0
/*******************************************************************************

   Function name: DBG_CommandLine_DebugEnable

   Purpose: This function will enable debug output

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_DebugEnable( uint32_t argc, char *argv[] )
{
   char * endptr;
   uint32_t val;

   if ( argc > 1 )   /* User wants to change the setting */
   {
      val = strtoul( argv[1], &endptr, 10 );
      if ( val < 256)

      {
         DBG_PortTimer_Set(( uint8_t) val);
      }
   }else
   {  // No arguments, so default to 1 hour
      DBG_PortTimer_Set ( 1 );
   }
   return ( 0 );
} /* end DBG_CommandLine_DebugEnable () */
#endif   /* end of #if 0 */
/*******************************************************************************

   Function name: DBG_CommandLine_DebugDisable

   Purpose: This function will disable debug output

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_DebugDisable( uint32_t argc, char *argv[] )
{
   // Disable the debug
   DBG_PortTimer_Set ( 0 );
   return ( 0 );
} /* end DBG_CommandLine_DebugDisable () */

#if ( TM_CRC_UNIT_TEST == 1)
/*******************************************************************************

   Function name: DBG_CommandLine_CrcCalculate

   Purpose: This function will test the CRC for the input string

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

*******************************************************************************/
uint32_t DBG_CommandLine_CrcCalculate( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eSUCCESS;
   uint16_t crc16;   /* Used to store CRC results */
   uint32_t crc32;   /* Used to store CRC results */
   uint8_t  dataBuf[100];

   if ( argc == 1 )
   {
      /* No arguments */
      DBG_logPrintf( 'R', "ERROR - Enter arguments" );
   }
   else if ( argc == 2 )
   {
      uint16_t srcIndex, bufIndex, srcLength;
      srcLength = strlen( argv[1] );
      if ( srcLength <= sizeof(dataBuf) )
      {
         if ( ( srcLength % 2 ) == 0 ) /* Any hex string must be an even number of hex digits */
         {
            for ( srcIndex = 0, bufIndex = 0; ( ( bufIndex < sizeof(dataBuf) ) && ( srcIndex < srcLength ) ); bufIndex++, srcIndex += 2 )
            {
               if ( eSUCCESS != ASCII_atohByte( argv[1][srcIndex], argv[1][srcIndex+1], &dataBuf[bufIndex] ) )
               {
                  retVal = eFAILURE;
               }
            }
            if ( eSUCCESS == retVal )
            {
               srcLength /= 2; /* Convert ASCII byte length to binary byte length */
               DBG_printfNoCr( "Byte string is %u long: ", srcLength );
               for ( bufIndex = 0 ; bufIndex < srcLength ; bufIndex++ )
               {
                  DBG_printfNoCr( "%02x", dataBuf[ bufIndex ] );
               }
               DBG_printf( " " );
               /* The number of arguments must be 1 */
               crc16 = CRC_16_Calculate( (uint8_t *)dataBuf, srcLength );
               DBG_logPrintf( 'R', "CRC_16_Calculate: %x", crc16 );
               /* Setting the Seed value to 0 and KeepLock value to false */
               CRC_ecc108_crc( srcLength, (uint8_t *)&dataBuf, (uint8_t *)&crc16, 0, false );
               DBG_logPrintf( 'R', "CRC_ecc108_crc: %x", crc16 );
               crc16 = CRC_16_PhyHeader( (uint8_t *)&dataBuf, srcLength );
               DBG_logPrintf( 'R', "CRC_16_PhyHeader: %x", crc16 );
               crc32 = CRC_32_Calculate( (uint8_t *)&dataBuf, srcLength );
               DBG_logPrintf( 'R', "CRC_32_Calculate: %x", crc32 );
            }
            else
            {
               DBG_logPrintf( 'R', "ERROR - invalid hex digit(s) in the input string %s", argv[1] );
            }
         }
         else
         {
            DBG_logPrintf( 'R', "ERROR - length of hex input string must be even but it is %u", srcLength );
         }
      }
      else
      {
         DBG_logPrintf( 'R', "ERROR - input hex string is too long, max size = %u", sizeof(dataBuf) );
      }
   }
   else
   {
      /* More arguments */
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   return ( 0 );
}/* end DBG_CommandLine_CrcCalculate() */
#endif

#if ( TM_TIME_COMPOUND_TEST == 1 )
/*******************************************************************************

   Function name: DBG_CommandLine_TimeElapsed

   Purpose: This function will test the OS_TICK_Get_ElapsedMilliseconds

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

*******************************************************************************/
uint32_t DBG_CommandLine_TimeElapsed( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   uint32_t delayMilliSec;
   uint32_t milliSec1;
   uint32_t milliSec2;
   uint32_t diffInSec[6];
   uint32_t diffAvg = 0;
   uint32_t diffMilliSec;
   if ( argc == 1 )
   {
      /* No arguments */
      DBG_logPrintf( 'R', "ERROR - Enter arguments" );
   }
   else if ( argc == 2 )
   {
      /* The number of arguments must be 1 */
      delayMilliSec = ( uint32_t )atoi( argv[1] );
      /* Sampling for 5 times */
      for( int loopCount = 0; loopCount < MAX_SAMPLE_LOOP_COUNT ; loopCount++ )
      {
         milliSec1 = OS_TICK_Get_ElapsedMilliseconds();
         /* BSP delay in milliseconds */
         R_BSP_SoftwareDelay( delayMilliSec,BSP_DELAY_UNITS_MILLISECONDS );
         milliSec2 = OS_TICK_Get_ElapsedMilliseconds();
         /* Storing the difference in array */
         diffInSec[ loopCount ] = milliSec2 - milliSec1;
         diffAvg += diffInSec[ loopCount ];
      }
      /* Average of Sampling */
      diffAvg = diffAvg / MAX_SAMPLE_LOOP_COUNT;
      diffMilliSec = milliSec2 - milliSec1;
      /* Considering success if difference is one */
      if ( ( diffAvg == diffMilliSec ) || ( diffAvg == ( diffMilliSec + 1 ) ) || ( diffAvg == ( diffMilliSec - 1 ) ) )
      {
         DBG_logPrintf( 'R', "Test Success!" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'R', "Test Failure!" );
      }
   }
   else
   {
      /* More arguments */
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   return ( uint32_t )retVal;
}/* end DBG_CommandLine_TimeElapsed() */

/*******************************************************************************

   Function name: DBG_CommandLine_TimeNanoSec

   Purpose: This function will test the OS_TICK_Get_Diff_InNanoseconds

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

*******************************************************************************/
uint32_t DBG_CommandLine_TimeNanoSec( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   OS_TICK_Struct tickValue1 ;
   OS_TICK_Struct tickValue2 ;
   uint32_t delayNanoSec;
   uint32_t diffInSec;
   if ( argc == 1 )
   {
      /* No arguments */
      DBG_logPrintf( 'R', "ERROR - Enter arguments" );
   }
   else if ( argc == 2 )
   {
      /* The number of arguments must be 1 */
      delayNanoSec = ( uint32_t )atoi( argv[1] );
      /* Set OS_TICK_Struct to zero */
      memset( &tickValue1, 0, sizeof tickValue1 );
      memset( &tickValue2, 0, sizeof tickValue2 );
      /* Gets Previous value */
      OS_TICK_Get_CurrentElapsedTicks( &tickValue1 );
      /* BSP delay in milliseconds */
      R_BSP_SoftwareDelay( delayNanoSec, BSP_DELAY_UNITS_MICROSECONDS );
      /* Gets current value */
      OS_TICK_Get_CurrentElapsedTicks( &tickValue2 );
      diffInSec = OS_TICK_Get_Diff_InNanoseconds( &tickValue1, &tickValue2 );
      /* If delay and difference are same */
      if ( diffInSec == delayNanoSec )
      {
         DBG_logPrintf( 'R', "Test Success!" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'R', "Test Failure!" );
      }
   }
   else
   {
      /* More arguments */
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   return ( uint32_t )retVal;
}/* end DBG_CommandLine_TimeNanoSec() */

/*******************************************************************************

   Function name: DBG_CommandLine_TimeMicroSec

   Purpose: This function will test the OS_TICK_Get_Diff_InMicroseconds

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

*******************************************************************************/
uint32_t DBG_CommandLine_TimeMicroSec( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   OS_TICK_Struct tickValue1 ;
   OS_TICK_Struct tickValue2 ;
   uint64_t delayMicroSec1;
   uint64_t delayMicroSec2;
   int64_t diffInArg;
   uint32_t diffInSec;
   uint32_t sysTickLoad;
   if ( argc == 1 )
   {
      /* No arguments */
      DBG_logPrintf( 'R', "ERROR - Enter arguments" );
   }
   else if ( argc == 3 )
   {
      /* The number of arguments must be 2 */
      delayMicroSec1 = ( uint64_t )atoll( argv[1] );
      delayMicroSec2 = ( uint64_t )atoll( argv[2] );
      /* Set OS_TICK_Struct to zero */
      memset( &tickValue1, 0, sizeof tickValue1 );
      memset( &tickValue2, 0, sizeof tickValue2 );
      diffInArg = 0;
      diffInArg = ( int64_t ) ( delayMicroSec2 - delayMicroSec1 ) ;
      if ( diffInArg < 0 )
      {
         diffInArg = 0;
      }
      taskENTER_CRITICAL( );
      sysTickLoad = SysTick->LOAD + 1;
      taskEXIT_CRITICAL( );
      delayMicroSec1 = delayMicroSec1 * ( ( sysTickLoad / portTICK_RATE_MS ) / 1000 );
      delayMicroSec2 = delayMicroSec2 * ( ( sysTickLoad / portTICK_RATE_MS ) / 1000 );
      tickValue1.tickCount = delayMicroSec1 / sysTickLoad ;
      delayMicroSec1 = delayMicroSec1 % sysTickLoad;
      tickValue2.tickCount = delayMicroSec2 / sysTickLoad ;
      delayMicroSec2 = delayMicroSec2 % sysTickLoad;
      tickValue1.HW_TICKS = sysTickLoad - delayMicroSec1;
      tickValue2.HW_TICKS = sysTickLoad - delayMicroSec2;
      diffInSec = OS_TICK_Get_Diff_InMicroseconds( &tickValue1, &tickValue2 );
      /* If delay and difference are same */
      if ( diffInSec == diffInArg )
      {
         DBG_logPrintf( 'R', "Test Success!  sysTickLoad=%lu, delayMicroSec1=%llu; delayMicroSec2=%llu; tickValue1 = [ %lu %lu ]; tickValue2 = [ %lu %lu ]; diffInSec=%lu, diffInArg=%lld",
                             sysTickLoad, delayMicroSec1, delayMicroSec2, tickValue1.tickCount, tickValue1.HW_TICKS, tickValue2.tickCount, tickValue2.HW_TICKS,
                             diffInSec, diffInArg);
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'R', "Test Failure!  sysTickLoad=%lu, delayMicroSec1=%llu; delayMicroSec2=%llu; tickValue1 = [ %lu %lu ]; tickValue2 = [ %lu %lu ]; diffInSec=%lu, diffInArg=%lld",
                             sysTickLoad, delayMicroSec1, delayMicroSec2, tickValue1.tickCount, tickValue1.HW_TICKS, tickValue2.tickCount, tickValue2.HW_TICKS,
                             diffInSec, diffInArg);
      }
   }
   else
   {
      /* More arguments */
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   return ( uint32_t )retVal;
}/* end DBG_CommandLine_TimeMicroSec() */

/*******************************************************************************

   Function name: DBG_CommandLine_TimeMilliSec

   Purpose: This function will test the OS_TICK_Get_Diff_InMilliseconds

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

*******************************************************************************/
uint32_t DBG_CommandLine_TimeMilliSec( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   OS_TICK_Struct tickValue1 ;
   OS_TICK_Struct tickValue2 ;
   uint64_t delayMilliSec1;
   uint64_t delayMilliSec2;
   int64_t diffInArg;
   uint32_t diffInSec;
   uint32_t sysTickLoad;
   if ( argc == 1 )
   {
      /* No arguments */
      DBG_logPrintf( 'R', "ERROR - Enter arguments" );
   }
   else if ( argc == 3 )
   {
      /* The number of arguments must be 2 */
      delayMilliSec1 = ( uint64_t )atoll( argv[1] );
      delayMilliSec2 = ( uint64_t )atoll( argv[2] );
      /* Set OS_TICK_Struct to zero */
      memset( &tickValue1, 0, sizeof tickValue1 );
      memset( &tickValue2, 0, sizeof tickValue2 );
      diffInArg = 0;
      diffInArg = ( int64_t ) ( delayMilliSec2 - delayMilliSec1 ) ;
      if ( diffInArg < 0 )
      {
         diffInArg = 0;
      }
      tickValue1.tickCount = delayMilliSec1 / 5;
      taskENTER_CRITICAL( );
      sysTickLoad = SysTick->LOAD + 1;
      taskEXIT_CRITICAL( );
      tickValue1.HW_TICKS = sysTickLoad - ( ( delayMilliSec1 % 5 ) * ( sysTickLoad / portTICK_RATE_MS ) );
      tickValue2.tickCount = delayMilliSec2 / 5;
      tickValue2.HW_TICKS = sysTickLoad - ( ( delayMilliSec2 % 5 ) * ( sysTickLoad / portTICK_RATE_MS ) );
      diffInSec = OS_TICK_Get_Diff_InMilliseconds(&tickValue1,&tickValue2);
      /* If delay and difference are same */
      if ( diffInSec == diffInArg )
      {
         DBG_logPrintf( 'R', "Test Success!" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'R', "Test Failure!" );
      }
   }
   else
   {
      /* More arguments */
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   return ( uint32_t )retVal;
}/* end DBG_CommandLine_TimeMilliSec() */

/*******************************************************************************

   Function name: DBG_CommandLine_TimeSec

   Purpose: This function will test the OS_TICK_Get_Diff_InSeconds

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

*******************************************************************************/
uint32_t DBG_CommandLine_TimeSec( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   OS_TICK_Struct tickValue1 ;
   OS_TICK_Struct tickValue2 ;
   OS_TICK_Struct diffTickValue ;
   uint64_t delaySec1;
   uint64_t delaySec2;
   int64_t diffInArg;
   uint32_t diffInSec;
   uint32_t diffInArg_Check;
   if ( argc < 3 )
   {
      /* No arguments */
      DBG_logPrintf( 'R', "ERROR - Enter arguments" );
   }
   else if ( argc == 3 )
   {
      /* The number of arguments must be 2 */
      delaySec1 = ( uint64_t )atoll( argv[1] );
      delaySec2 = ( uint64_t )atoll( argv[2] );
      /* Set OS_TICK_Struct to zero */
      memset( &tickValue1, 0, sizeof tickValue1 );
      memset( &tickValue2, 0, sizeof tickValue2 );
      memset( &diffTickValue, 0, sizeof diffTickValue );
      diffInArg = 0;
      diffInArg = ( int64_t ) ( delaySec2 - delaySec1 ) ;
      if ( diffInArg < 0 )
      {
         diffInArg = 0;
      }
      diffInArg_Check = diffInArg;
      delaySec1 = delaySec1 * TICKCOUNT_SEC;
      delaySec2 = delaySec2 * TICKCOUNT_SEC;
      tickValue1.xNumOfOverflows = delaySec1 / UINT32_MAX ;
      delaySec1 = delaySec1 % UINT32_MAX;
      tickValue2.xNumOfOverflows = delaySec2 / UINT32_MAX ;
      delaySec2 = delaySec2 % UINT32_MAX;
      tickValue1.tickCount = delaySec1;
      tickValue2.tickCount = delaySec2;
      diffInSec = OS_TICK_Get_Diff_InSeconds( &tickValue1,&tickValue2 );
      /* Convert the diffInArg to OS Tick */
      diffInArg = diffInArg * TICKCOUNT_SEC;
      diffTickValue.xNumOfOverflows = diffInArg / UINT32_MAX ;
      /* Check for xNumOfOverflows */
      if ( diffTickValue.xNumOfOverflows > 0 )
      {
         DBG_logPrintf( 'R', "Current tick count value is greater than Max uint32_t value or UINT32_MAX" );
         // TODO: RA6 [name_Balaji]:Currently we dont support  values greater than UINT32_MAX. Check if required
      }

      else
      {
         /* If delay and difference are same */
         if ( diffInSec == diffInArg_Check )
         {
            DBG_logPrintf( 'R', "Test Success!" );
            retVal = eSUCCESS;
         }
         else
         {
            DBG_logPrintf( 'R', "Test Failure!" );
         }
      }
   }
   else
   {
      /* More arguments */
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   return ( uint32_t )retVal;
}/* end DBG_CommandLine_TimeSec() */

/*******************************************************************************

   Function name: DBG_CommandLine_TimeMin

   Purpose: This function will test the OS_TICK_Get_Diff_InMinutes

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

*******************************************************************************/
uint32_t DBG_CommandLine_TimeMin( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   OS_TICK_Struct timeMin1 ;
   OS_TICK_Struct timeMin2 ;
   OS_TICK_Struct diffTickValue ;
   uint64_t delayMin1;
   uint64_t delayMin2;
   int64_t diffInArg;
   uint32_t diffInMin;
   uint32_t diffInArg_Check;
   if ( argc == 1 )
   {
      /* No arguments */
      DBG_logPrintf( 'R', "ERROR - Enter arguments" );
   }
   else if ( argc == 3 )
   {
      /* The number of arguments must be 2 */
      delayMin1 = ( uint64_t )atoll( argv[1] );
      delayMin2 = ( uint64_t )atoll( argv[2] );
      /* Set OS_TICK_Struct to zero */
      memset( &timeMin1, 0, sizeof timeMin1 );
      memset( &timeMin2, 0, sizeof timeMin2 );
      diffInArg = 0;
      diffInArg = ( int64_t ) ( delayMin2 - delayMin1 ) ;
      if ( diffInArg < 0 )
      {
         diffInArg = 0;
      }
      diffInArg_Check = diffInArg;
      delayMin1 = delayMin1 * TICKCOUNT_MIN;
      delayMin2 = delayMin2 * TICKCOUNT_MIN;
      timeMin1.xNumOfOverflows = delayMin1 / UINT32_MAX ;
      delayMin1 = delayMin1 % UINT32_MAX;
      timeMin2.xNumOfOverflows = delayMin2 / UINT32_MAX ;
      delayMin2 = delayMin2 % UINT32_MAX;
      timeMin1.tickCount = delayMin1;
      timeMin2.tickCount = delayMin2;
      diffInMin = OS_TICK_Get_Diff_InMinutes( &timeMin1,&timeMin2 );
      /* Convert the diffInArg to OS Tick */
      diffInArg = diffInArg * TICKCOUNT_MIN;
      diffTickValue.xNumOfOverflows = diffInArg / UINT32_MAX ;
      /* Check for xNumOfOverflows */
      if ( diffTickValue.xNumOfOverflows > 0)
      {
         DBG_logPrintf( 'R', "Current tick count value is greater than Max uint32_t value or UINT32_MAX" );
         // TODO: RA6 [name_Balaji]:Currently we dont support  values greater than UINT32_MAX. Check if required
      }
      else
      {
         /* If delay and difference are same */
         if ( diffInMin == diffInArg_Check )
         {
            DBG_logPrintf( 'R', "Test Success!" );
            retVal = eSUCCESS;
         }
         else
         {
            DBG_logPrintf( 'R', "Test Failure!" );
         }
      }
   }
   else
   {
      /* More arguments */
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   return ( uint32_t )retVal;
}/* end DBG_CommandLine_TimeMin() */


/*******************************************************************************

   Function name: DBG_CommandLine_TimeHour

   Purpose: This function will test the OS_TICK_Get_Diff_InHours

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

*******************************************************************************/
uint32_t DBG_CommandLine_TimeHour( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   OS_TICK_Struct timeHour1 ;
   OS_TICK_Struct timeHour2 ;
   OS_TICK_Struct diffTickValue ;
   uint64_t delayHour1;
   uint64_t delayHour2;
   int64_t diffInArg;
   uint32_t diffInHour;
   uint32_t diffInArg_Check;
   if ( argc == 1 )
   {
      /* No arguments */
      DBG_logPrintf( 'R', "ERROR - Enter arguments" );
   }
   else if ( argc == 3 )
   {
      /* The number of arguments must be 2 */
      delayHour1 = ( uint64_t )atoll( argv[1] );
      delayHour2 = ( uint64_t )atoll( argv[2] );
      /* Set OS_TICK_Struct to zero */
      memset( &timeHour1, 0, sizeof timeHour1 );
      memset( &timeHour2, 0, sizeof timeHour2 );
      diffInArg = 0;
      diffInArg = ( int64_t ) ( delayHour2 - delayHour1 );
      /* If negative value assign zero */
      if ( diffInArg < 0 )
      {
         diffInArg = 0;
      }
      diffInArg_Check = diffInArg;
      delayHour1 = delayHour1 * TICKCOUNT_HOUR;
      delayHour2 = delayHour2 * TICKCOUNT_HOUR;
      timeHour1.xNumOfOverflows = delayHour1 / UINT32_MAX ;
      delayHour1 = delayHour1 % UINT32_MAX;
      timeHour2.xNumOfOverflows = delayHour2 / UINT32_MAX ;
      delayHour2 = delayHour2 % UINT32_MAX;
      timeHour1.tickCount = delayHour1;
      timeHour2.tickCount = delayHour2;
      diffInHour = OS_TICK_Get_Diff_InHours( &timeHour1,&timeHour2 );
      /* Convert the diffInArg to OS Tick */
      diffInArg = diffInArg * TICKCOUNT_HOUR;
      diffTickValue.xNumOfOverflows = diffInArg / UINT32_MAX ;
      /* Check for xNumOfOverflows */
      if ( diffTickValue.xNumOfOverflows > 0 )
      {
         DBG_logPrintf( 'R', "Current tick count value is greater than Max uint32_t value or UINT32_MAX" );
         // TODO: RA6 [name_Balaji]:Currently we dont support  values greater than UINT32_MAX. Check if required
      }

      else
      {
         /* If delay and difference are same */
         if ( diffInHour == diffInArg_Check )
         {
            DBG_logPrintf( 'R', "Test Success!" );
            retVal = eSUCCESS;
         }
         else
         {
            DBG_logPrintf( 'R', "Test Failure!" );
         }
      }
   }
   else
   {
      /* More arguments */
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_TimeHour() */

/*******************************************************************************

   Function name: DBG_CommandLine_TimeTicks

   Purpose: This function will test the OS_TICK_Add_msec_to_ticks

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

*******************************************************************************/
uint32_t DBG_CommandLine_TimeTicks( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   OS_TICK_Struct timeMilliSec;
   OS_TICK_Struct_Ptr isdelayptr;
   uint32_t argvMilliSec;
   uint32_t prevTickValue;
   uint32_t diffInTick;
   if ( argc == 1 )
   {
      /* No arguments */
      DBG_logPrintf( 'R', "ERROR - Enter arguments" );
   }
   else if ( argc == 2 )
   {
      /* The number of arguments must be 1 */
      argvMilliSec = ( uint32_t )atoi( argv[1] );
      if( argvMilliSec < portTICK_RATE_MS )
      {
         /* Due to portTICK_RATE_MS accuracy values need to be 5 or greater than 5
          * as we are checking with millisec */
         DBG_logPrintf( 'R', "Value needs to be 5 or greater than 5 which is portTICK_RATE_MS " );
      }
      else
      {
         /* Sets timeMilliSec to zero */
         memset( &timeMilliSec, 0, sizeof timeMilliSec );
         /* Get Elapsed Ticks */
         OS_TICK_Get_CurrentElapsedTicks( &timeMilliSec );
         prevTickValue = timeMilliSec.tickCount;
         isdelayptr = OS_TICK_Add_msec_to_ticks( &timeMilliSec, argvMilliSec );
         /* Get the difference between tickCounts */
         diffInTick = ( isdelayptr->tickCount ) - prevTickValue;
         /* If delay and difference are same */
         if ( diffInTick == ( argvMilliSec/portTICK_RATE_MS ))
         {
            DBG_logPrintf( 'R', "Test Success!" );
            retVal = eSUCCESS;
         }
         else
         {
            DBG_logPrintf( 'R', "Test Failure!" );
         }
      }
   }
   else
   {
      /* More arguments */
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_TimeTicks() */

/*******************************************************************************

   Function name: DBG_CommandLine_TimeFuture

   Purpose: This function will test the OS_TICK_Is_FutureTime_Greater

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

*******************************************************************************/
uint32_t DBG_CommandLine_TimeFuture( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   OS_TICK_Struct tickValue1 ;
   OS_TICK_Struct tickValue2 ;
   uint32_t delayMilliSec;
   bool isTime2Greater;
   if ( argc == 1 )
   {
      /* No arguments */
      DBG_logPrintf( 'R', "ERROR - Enter arguments" );
   }
   else if ( argc == 2 )
   {
      /* The number of arguments must be 1 */
      delayMilliSec = ( uint32_t )atoi( argv[1] );
      /* Set OS_TICK_Struct to zero */
      memset( &tickValue1, 0, sizeof tickValue1 );
      memset( &tickValue2, 0, sizeof tickValue2 );
      /* Gets Previous value */
      OS_TICK_Get_CurrentElapsedTicks( &tickValue1 );
      /* BSP Delay in milliseconds */
      R_BSP_SoftwareDelay( delayMilliSec,BSP_DELAY_UNITS_MILLISECONDS );
      /* Gets current value */
      OS_TICK_Get_CurrentElapsedTicks( &tickValue2 );
      isTime2Greater = OS_TICK_Is_FutureTime_Greater( &tickValue1, &tickValue2 );
      if ( isTime2Greater == true )
      {
         DBG_logPrintf( 'R', "Test Success!" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'R', "Test Failure!" );
      }
   }
   else
   {
      /* More arguments */
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   return ( uint32_t )retVal;
}/* end DBG_CommandLine_TimeFuture() */
#endif

#if ( TM_OS_EVENT_TEST == 1)
/*******************************************************************************

   Function name: OS_EVNTTestTask

   Purpose: Creates the test event and waits forever

   Arguments:  taskParameter - Not used, but required here because this is a task

   Returns: void

*******************************************************************************/
void OS_EVNTTestTask ( taskParameter )
{
   bool isEvtCreateSuccess = false;
   isEvtCreateSuccess = OS_EVNT_Create( eventTestHandle );
   if( isEvtCreateSuccess == true )
   {
      DBG_logPrintf( 'I', "EventTest Create Success!" );
      uint32_t setEvents;
      osEventCreated = TRUE;
      for(;;)
      {
         setEvents = OS_EVNT_Wait( eventTestHandle, waitBit, waitForAll, OS_WAIT_FOREVER );
         DBG_logPrintf( 'R', "Test SetEvents %X",setEvents );
         if ( setEvents == waitBit )
         {
            DBG_logPrintf( 'R', "SetBits and WaitBits are equal" );
         }
         else
         {
            DBG_logPrintf( 'R', "SetBits and WaitBits are not equal" );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'E', "EventTest Create Failure!" );
      OS_TASK_Exit();
   }
}
/*******************************************************************************

   Function name: DBG_CommandLine_OS_EventSet

   Purpose: This function will set and test the OS_EVNT_Set

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

*******************************************************************************/
uint32_t DBG_CommandLine_OS_EventSet( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   uint32_t setBit;
   if ( argc == 2 )
   {
      /* The number of arguments must be 2 */
      setBit = ( uint32_t )strtol( argv[1], NULL, 16 );
      if( setBit > 0xFFFFFF || setBit < 1 )
      {
         DBG_logPrintf( 'R', "Enter values less than 3Bytes ( 1 to FFFFFF )" );
      }
      else
      {
         if ( osEventCreated == TRUE )
         {
            OS_EVNT_Set( eventTestHandle,setBit );
            DBG_logPrintf( 'R', "OS Test Event Set Successful" );
            retVal = eSUCCESS;
         }
         else
         {
            DBG_logPrintf( 'R', "OS Test Event not created" );
         }
      }
   }
   else if (argc < 2 )
   {
      DBG_logPrintf( 'E', "ERROR - Too few arguments" );
   }
   else
   {
      DBG_logPrintf( 'E', "ERROR - Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_OS_EventSet() */

/*******************************************************************************

   Function name: DBG_CommandLine_OS_EventCreateWait

   Purpose: This function will create OS_EVNTTestTask

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: retVal - Successful status of this function

*******************************************************************************/
uint32_t DBG_CommandLine_OS_EventCreateWait( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   uint8_t argvWaitForAll;
   if ( argc == 3 )
   {
      /* The number of arguments must be 2 */
      waitBit = ( uint32_t ) strtol ( argv[1], NULL, 16 );
      if( waitBit > 0xFFFFFF || waitBit < 1 )
      {
         DBG_logPrintf( 'R', "Enter values less than 3Bytes ( 1 to FFFFFF )" );
      }
      else
      {
         argvWaitForAll = ( uint32_t )atoi( argv[2] );
         if( ( argvWaitForAll == 0 ) || ( argvWaitForAll == 1 ) )
         {
            waitForAll = argvWaitForAll;
            if( osEventTaskCreated == FALSE )
            {
               const OS_TASK_Template_t  osEventTaskTemplate[1] =
               {
                  /* Task Index, Function,   Stack, Pri, Name,           tributes, Param, Time Slice */
                  { 1,      OS_EVNTTestTask, 500, 38, (char *)pTskName_OSEVNTTest, 0, 0, 0 },
               };
               OS_TASK_Template_t const *osEventTestTask;
               osEventTestTask = &osEventTaskTemplate[0];
#if( RTOS_SELECTION == MQX_RTOS )
               if ( MQX_NULL_TASK_ID == (taskID = OS_TASK_Create(osEventTestTask) ) )
#elif (RTOS_SELECTION == FREE_RTOS)
               if ( pdPASS != OS_TASK_Create(osEventTestTask) )
#endif
               {
                  DBG_logPrintf( 'R', "ERROR - Test Event Task Creation Failed" );
               }
               osEventTaskCreated = TRUE;
               retVal = eSUCCESS;
            }
            else
            {
               DBG_logPrintf( 'R', "ERROR - Test Event Task Already Created" );
            }
         }
         else
         {
            DBG_logPrintf( 'R', "WaitForAll must be 0 or 1" );
         }
      }
   }
   else if (argc < 3 )
   {
      DBG_logPrintf( 'E', "ERROR - Too few arguments" );
   }
   else
   {
      DBG_logPrintf( 'E', "ERROR - Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_OS_EventCreateWait() */

/*******************************************************************************

   Function name: DBG_CommandLine_OS_EventTaskDelete

   Purpose: This function will exit the OS_EVNTTest task and deletes the Event
            test group

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: retVal - Successful status of this function

*******************************************************************************/
uint32_t DBG_CommandLine_OS_EventTaskDelete( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   if ( argc == 1 )
   {
      if( osEventTaskCreated == TRUE )
      {
         OS_TASK_ExitId( pTskName_OSEVNTTest );
         osEventTaskCreated = FALSE;
         OS_EVNT_DELETE( eventTestObj );
         DBG_logPrintf( 'R', "Test Event Task Delete Success" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'R', "ERROR - Test Event Task Not Created" );
      }
   }
   else
   {
      DBG_logPrintf( 'E', "ERROR - Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_OS_EventTaskDelete() */
#endif

#if ( TM_LINKED_LIST == 1)
/*******************************************************************************

   Function name: DBG_CommandLine_OS_LinkedList_Create

   Purpose: This function will test OS_LINKEDLIST_Create function and
             create the Test event

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: retVal - Successful status of this function

*******************************************************************************/
uint32_t DBG_CommandLine_OS_LinkedList_Create( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   bool osEventTaskCreated;
   if ( argc == 1 )
   {
      /* Creates the LINKED_LIST handle*/
      osEventTaskCreated = OS_LINKEDLIST_Create(osLinkedListTestHandle);
      if( osEventTaskCreated == TRUE )
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Success Test Case Success" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'E', "Linkedlist_Test_Failure Test Case Failure" );
      }
   }
   else
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_OS_LinkedList_Create() */

/*******************************************************************************

   Function name: DBG_CommandLine_OS_LinkedList_NumElements

   Purpose: This function will test OS_LINKEDLIST_NumElements function by
             adding the number of elements from the user to the LinkedList

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: retVal - Successful status of this function

*******************************************************************************/
uint32_t DBG_CommandLine_OS_LinkedList_NumElements( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   uint8_t numElementToAdd;
   uint8_t numElementInsideList;
   if ( argc == 2 )
   {
      numElementToAdd = ( uint32_t )atoi( argv[1] );
      /* Validate input from user */
      if ( ( numElementToAdd > MAX_LINKEDLIST_DATA )
           || ( numElementToAdd == MAX_LINKEDLIST_DATA ) )
      {
         DBG_logPrintf( 'R', "ERROR - Linkedlist_Test_Failure Invalid Array Index, Should be less than 5" );
         return ( uint32_t )retVal;
      }

      /* Enqueue elements to the list */
      for( uint8_t index = 0; index < numElementToAdd; index++ )
      {
         OS_LINKEDLIST_Enqueue( osLinkedListTestHandle, &LinkedListdata[ index ] );
      }

      /* Get the Count from the API */
      numElementInsideList = OS_LINKEDLIST_NumElements( osLinkedListTestHandle );

      /* Validate the result */
      if ( numElementToAdd == numElementInsideList )
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Success Test Case Success" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Failure Test Case Failure" );
      }
   }
   else if ( argc < 2 )
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too few arguments" );
   }
   else
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_OS_LinkedList_NumElements() */

/*******************************************************************************

   Function name: DBG_CommandLine_OS_LinkedList_Remove

   Purpose: This function will test OS_LINKEDLIST_Remove function by
             removing the number of elements from the user to the LinkedList

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: retVal - Successful status of this function

*******************************************************************************/
uint32_t DBG_CommandLine_OS_LinkedList_Remove( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   uint8_t indexElementToRemove;
   uint8_t numElementBeforeRemove;
   uint8_t numElementAfterRemove;
   if ( argc == 2 )
   {
      indexElementToRemove = ( uint32_t )atoi( argv[1] );
      /* Validate input from user */
      if ( ( indexElementToRemove > MAX_LINKEDLIST_DATA )
           || ( indexElementToRemove == MAX_LINKEDLIST_DATA ) )
      {
         DBG_logPrintf( 'R', "ERROR - Linkedlist_Test_Failure Invalid Array Index, Should be less than 5" );
         return ( uint32_t )retVal;
      }

      if ( LinkedListdata[ indexElementToRemove ].NEXT == NULL )
      {
         DBG_logPrintf( 'R', "ERROR - Linkedlist_Test_Failure Invalid Array Index, Array Index already NULL" );
         return ( uint32_t )retVal;
      }

      /* Get the Count from the API */
      numElementBeforeRemove = OS_LINKEDLIST_NumElements( osLinkedListTestHandle );

      /* Remove elements from the list */
      OS_LINKEDLIST_Remove( osLinkedListTestHandle, &LinkedListdata[ indexElementToRemove ] );

      /* Get the Count from the API */
      numElementAfterRemove = OS_LINKEDLIST_NumElements( osLinkedListTestHandle );

      if ( ( numElementAfterRemove + 1 ) == numElementBeforeRemove )
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Success Test Case Success" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Failure Test Case Failure" );
      }

   }
   else if ( argc < 2 )
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too few arguments" );
   }
   else
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_OS_LinkedList_Remove() */

/*******************************************************************************

   Function name: DBG_CommandLine_OS_LinkedList_Enqueue

   Purpose: This function will test OS_LINKEDLIST_Enqueue function by
             adding the number of elements from the user to the LinkedList
             and validating the number of elements

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: retVal - Successful status of this function

*******************************************************************************/
uint32_t DBG_CommandLine_OS_LinkedList_Enqueue( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   uint8_t indexElementToEnq;
   uint8_t numElementBeforeEnq;
   uint8_t numElementAfterEnq;
   if ( argc == 2 )
   {
      indexElementToEnq = ( uint32_t )atoi( argv[1] );
      /* Validate input from user */
      if ( ( indexElementToEnq > MAX_LINKEDLIST_DATA )
           || ( indexElementToEnq == MAX_LINKEDLIST_DATA ) )
      {
         DBG_logPrintf( 'R', "ERROR - Linkedlist_Test_Failure Invalid Array Index, Should be less than 5" );
         return ( uint32_t )retVal;
      }

      /* Get the Count from the API */
      numElementBeforeEnq = OS_LINKEDLIST_NumElements( osLinkedListTestHandle );

      /* Remove elements from the list */
      OS_LINKEDLIST_Enqueue( osLinkedListTestHandle, &LinkedListdata[ indexElementToEnq ] );

      /* Get the Count from the API */
      numElementAfterEnq = OS_LINKEDLIST_NumElements( osLinkedListTestHandle );

      if ( numElementAfterEnq == ( numElementBeforeEnq + 1 ) )
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Success Test Case Success" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Failure Test Case Failure" );
      }

   }
   else if ( argc < 2 )
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too few arguments" );
   }
   else
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_OS_LinkedList_Enqueue() */

/*******************************************************************************

   Function name: DBG_CommandLine_OS_LinkedList_Dequeue

   Purpose: This function will test OS_LINKEDLIST_Dequeue function by
             removing the number of elements from the user to the LinkedList
             and validating with the count

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: retVal - Successful status of this function

*******************************************************************************/
uint32_t DBG_CommandLine_OS_LinkedList_Dequeue( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   uint8_t numElementBeforeDeq;
   uint8_t numElementAfterDeq;
   static OS_Linked_List_Element_Handle deqReturnElement;

   if ( argc == 1 )
   {

      /* Get the Count from the API */
      numElementBeforeDeq = OS_LINKEDLIST_NumElements( osLinkedListTestHandle );

      /* Dequeue elements from the list */
      deqReturnElement = OS_LINKEDLIST_Dequeue( osLinkedListTestHandle );

      if (deqReturnElement == NULL)
      {
         DBG_logPrintf( 'E', "Linkedlist_Test_Failure Test Case Failure. LinkedList is already NULL" );
         return ( uint32_t )retVal;
      }

      /* Get the Count from the API */
      numElementAfterDeq = OS_LINKEDLIST_NumElements( osLinkedListTestHandle );

      if ( ( numElementAfterDeq + 1 ) == numElementBeforeDeq )
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Success Test Case Success" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'E', "Linkedlist_Test_Failure Test Case Failure" );
      }

   }
   else
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_OS_LinkedList_Dequeue() */

/*******************************************************************************

   Function name: DBG_CommandLine_OS_LinkedList_Insert

   Purpose: This function will test OS_LINKEDLIST_Insert function by
             inserting the element in the LinkedList and validating
             the number of count

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: retVal - Successful status of this function

*******************************************************************************/
uint32_t DBG_CommandLine_OS_LinkedList_Insert( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   uint8_t indexElementToInsertAfter;
   uint8_t indexElementToInsert;
   uint8_t numElementBeforeInsert;
   uint8_t numElementAfterInsert;
   bool isValueInserted;
   if ( argc == 3 )
   {
      indexElementToInsertAfter = ( uint32_t )atoi( argv[1] );
      indexElementToInsert = ( uint32_t )atoi( argv[2] );
      /* Validate input from user */
      if ( ( indexElementToInsert > MAX_LINKEDLIST_DATA )
           || ( indexElementToInsert == MAX_LINKEDLIST_DATA )
           || ( indexElementToInsertAfter > MAX_LINKEDLIST_DATA )
           || ( indexElementToInsertAfter == MAX_LINKEDLIST_DATA )
              )
      {
         DBG_logPrintf( 'R', "ERROR - Linkedlist_Test_Failure Invalid Array Index, Should be less than 5" );
         return ( uint32_t )retVal;
      }


      if ( LinkedListdata[ indexElementToInsertAfter ].NEXT == NULL )
      {

         DBG_logPrintf( 'E', "Linkedlist_Test_Failure Test Case Failure. LinkedList is already NULL" );
         return ( uint32_t )retVal;
      }

      /* Get the Count from the API */
      numElementBeforeInsert = OS_LINKEDLIST_NumElements( osLinkedListTestHandle );

      /* Insert elements to the list */
      isValueInserted = OS_LINKEDLIST_Insert( osLinkedListTestHandle, &LinkedListdata[ indexElementToInsertAfter ], &LinkedListdata[ indexElementToInsert ] );

      /* Get the Count from the API */
      numElementAfterInsert = OS_LINKEDLIST_NumElements( osLinkedListTestHandle );

      if ( numElementAfterInsert == ( numElementBeforeInsert + 1 ) )
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Success Test Case Success; isValueInserted = %u", (uint16_t)isValueInserted );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Failure Test Case Failure; isValueInserted = %u", (uint16_t)isValueInserted );
      }

   }
   else if ( argc < 3 )
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too few arguments" );
   }
   else
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_OS_LinkedList_Insert() */

/*******************************************************************************

   Function name: DBG_CommandLine_OS_LinkedList_Head

   Purpose: This function will test OS_LINKEDLIST_Head function by
             validating the return from the function

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: retVal - Successful status of this function

*******************************************************************************/
uint32_t DBG_CommandLine_OS_LinkedList_Head( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   static OS_Linked_List_Element_Handle headElement;
   if ( argc == 1 )
   {

      /* Get Head elements from the list */
      headElement = OS_LINKEDLIST_Head( osLinkedListTestHandle );

      if ( headElement != NULL )
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Success Test Case Success" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'E', "Linkedlist_Test_Failure Test Case Failure" );
      }

   }
   else
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_OS_LinkedList_Head() */


/*******************************************************************************

   Function name: DBG_CommandLine_OS_LinkedList_Next

   Purpose: This function will test OS_LINKEDLIST_Next function by
             validating the return from the function

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: retVal - Successful status of this function

*******************************************************************************/
uint32_t DBG_CommandLine_OS_LinkedList_Next( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   uint8_t indexElement;
   static OS_Linked_List_Element_Handle nextElement;
   if ( argc == 2 )
   {
      indexElement = ( uint32_t )atoi( argv[1] );
      /* Validate input from user */
      if ( ( indexElement > MAX_LINKEDLIST_DATA )
           || ( indexElement == MAX_LINKEDLIST_DATA ) )
      {
         DBG_logPrintf( 'R', "ERROR - Linkedlist_Test_Failure Invalid Array Index, Should be less than 5" );
         return ( uint32_t )retVal;
      }

      /* Get next elements from the list */
      nextElement = OS_LINKEDLIST_Next( osLinkedListTestHandle, &LinkedListdata[ indexElement ] );

      if ( nextElement != NULL )
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Success Test Case Success" );
         retVal = eSUCCESS;
      }
      else
      {
         DBG_logPrintf( 'R', "Linkedlist_Test_Failure Test Case Failure" );
      }

   }
   else if ( argc < 2 )
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too few arguments" );
   }
   else
   {
      DBG_logPrintf( 'E', "ERROR - Linkedlist_Test_Failure Too many arguments" );
   }
   return ( uint32_t )retVal;
}/* end DBG_CommandLine_OS_LinkedList_Next() */

#endif   //TM_LINKED_LIST

/*******************************************************************************

   Function name: DBG_CommandLine_CpuLoadEnable

   Purpose: This function will enable CPU Load output

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_CpuLoadEnable( uint32_t argc, char *argv[] )
{
   STRT_CpuLoadPrint ( eSTRT_CPU_LOAD_PRINT_ON );

   return ( 0 );
} /* end DBG_CommandLine_CpuLoadEnable () */
/*******************************************************************************

   Function name: DBG_CommandLine_DebugFilter

   Purpose: This function sets the debug print filter task id

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static uint32_t DBG_CommandLine_DebugFilter( uint32_t argc, char *argv[] )
{
   OS_TASK_id tid;
   if ( argc > 1 )
   {
      tid = strtoul( argv[1], NULL, 0 );
      DBG_SetTaskFilter( tid );
   }

   return 0;
}

#if ( DEBUG_PORT_BAUD_RATE == 1 )
/*******************************************************************************

   Function name: DBG_CommandLine_DebugBaudRate

   Purpose: This function sets the debug port baud rate to 115200 or 38400

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static uint32_t DBG_CommandLine_DebugBaudRate( uint32_t argc, char *argv[] )
{
   /* The following definitions were copied from hal_data.c, as generated by the Renesas smart configurator */
   static const baud_setting_t baud115200 =
   {  /* Baud rate 115200 calculated with 1.725% error. */ .abcse = 0, .abcs = 0, .bgdm = 1, .cks = 0, .brr = 31, .mddr = (uint8_t) 256, .brme = false };
   static const baud_setting_t baud38400 =
   {  /* Baud rate  38400 calculated with 0.677% error. */ .abcse = 0, .abcs = 0, .bgdm = 1, .cks = 0, .brr = 96, .mddr = (uint8_t) 256, .brme = false };
   baud_setting_t *pBaudRate;

   if ( argc != 2 )
   {
      DBG_logPrintf( 'R', "Usage: dbgbaudrate {115200 | 38400}" );
   }
   else
   {
      uint32_t baudRate = atoi (argv[1] );
      if ( ( baudRate != 115200 ) && ( baudRate != 38400 ) )
      {
         DBG_logPrintf( 'R', "Command only accepts baud rates of 115200 or 38400" );
      }
      else
      {
         if (baudRate == 115200 )
         {
            pBaudRate = (void *)&baud115200; /* The cast is required because of the const attribute of the source */
         }
         else
         {
            pBaudRate = (void *)&baud38400;  /* The cast is required because of the const attribute of the source */
         }
         DBG_logPrintf( 'I', "Will change baud rate of '%s' UART to %u bps in 10 seconds; please change your terminal emulator", UART_getName(UART_DEBUG_PORT), baudRate );
         OS_TASK_Sleep ( (uint32_t) 10000 ); /* Make sure nothing is still echoing and wait for user to change baud rate */

         returnStatus_t retVal = UART_setBaud ( UART_DEBUG_PORT, pBaudRate);

         if ( eSUCCESS == retVal )
         {
            DBG_logPrintf( 'I', "Changed '%s' UART baud rate to %u bps", UART_getName(UART_DEBUG_PORT), baudRate );
         }
         else
         {
            OS_TASK_Sleep ( (uint32_t) 20000 ); /* Wait for the user to change the baud rate back to previous */
            DBG_logPrintf( 'R', "Attempt to set baud rate of '%s' UART to %u bps failed, retVal = %u", UART_getName(UART_DEBUG_PORT), baudRate, retVal );
         }
      }
   }
   return 0;
}
#endif
/*******************************************************************************

   Function name: DBG_CommandLine_CpuLoadSmart

   Purpose: This function will enable smart CPU Load output (only print when greater
            than 30%).

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_CpuLoadSmart( uint32_t argc, char *argv[] )
{
   STRT_CpuLoadPrint ( eSTRT_CPU_LOAD_PRINT_SMART );

   return ( 0 );
} /* end DBG_CommandLine_CpuLoadEnable () */

/*******************************************************************************

   Function name: DDBG_CommandLine_CpuLoadDisable

   Purpose: This function will disable CPU Load output

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_CpuLoadDisable( uint32_t argc, char *argv[] )
{
   STRT_CpuLoadPrint ( eSTRT_CPU_LOAD_PRINT_OFF );

   return ( 0 );
} /* end DBG_CommandLine_CpuLoadSmart () */
/*******************************************************************************

   Function name: DBG_CommandLine_Partition

   Purpose: This function will print out the Partition Table Information

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_Partition ( uint32_t argc, char *argv[] )
{
#ifdef TM_PARTITION_TBL
   ( void )PAR_ValidatePartitionTable();
#else
   DBG_logPrintf( 'R', "ERROR - Need to enable 'TM_PARTITION_TBL' in CompileSwitch.h" );
#endif
   return ( 0 );
} /* end DBG_CommandLine_Partition () */

/*******************************************************************************

   Function name: DBG_CommandLine_NvRead

   Purpose: Reads the NV memory

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_NvRead ( uint32_t argc, char *argv[] )
{
#if ( PARTITION_MANAGER == 1 )
   char     *endptr;
   uint8_t  buffer[ 64 ];        /* Data read from NV */
   uint8_t  respDataHex[ ( sizeof( buffer ) * 3 ) + ( sizeof( buffer ) / 16 ) + 1 ];
   uint32_t offset;
   uint16_t bytesLeft;           /* Running count of bytes read, decreasing   */
   uint16_t part;
   uint16_t cnt;

   if ( 4 == argc )
   {
      PartitionData_t const *pPTbl_;
      part = ( uint16_t )strtoul( argv[1], &endptr, 0 );

      if ( eSUCCESS == PAR_partitionFptr.parOpen( &pPTbl_, ( ePartitionName )part, 0L ) )
      {  //If this is NOT an internalFlash partition
         if ( 0 != memcmp (pPTbl_->PartitionType.pDevice, &_sIntFlashType[0], sizeof( pPTbl_->PartitionType.pDevice )) )
         {
            offset = strtoul( argv[2], &endptr, 0 );
            bytesLeft  = ( uint16_t )( strtoul( argv[3], &endptr, 0 ) );
            if ( offset < pPTbl_->lDataSize )
            {  //Offset is within partition data size
               if ( (offset + bytesLeft) > pPTbl_->lDataSize )
               {  //Correct for a length beyond the partitions data size
                  bytesLeft = ( uint16_t )( pPTbl_->lDataSize - offset );
                  DBG_logPrintf( 'I', "Limiting Read to Partition DataSize %d", bytesLeft );
               }
               while ( bytesLeft != 0 )
               {
                  cnt = min( bytesLeft, sizeof( buffer ) );
                  if ( eSUCCESS == PAR_partitionFptr.parRead( &buffer[0], offset, cnt, pPTbl_ ) )
                  {
                     uint8_t i;
                     uint8_t *pPtr;
                     for ( i = 0, pPtr = &respDataHex[ 0 ]; i < cnt; i++ )
                     {
                        if ( ( ( i % 16 ) == 0 ) && ( i != 0 ) )
                        {
                           pPtr += sprintf( ( char * )pPtr, "\n" );
                        }
                        pPtr += sprintf( ( char * )pPtr, "%02X ", buffer[ i ] );
                     }
                     *pPtr = 0;
                     DBG_logPrintf( 'R', "Partition: %d, Offset: 0x%x\n%s", part, offset, &respDataHex[0] );
                     OS_TASK_Sleep( 100 );
                     bytesLeft -= cnt;
                     offset += cnt;
                  }
                  else
                  {
                     DBG_logPrintf( 'R', "Error Reading Partition at %s", argv[2] );
                  }
               }
            }
            else
            {
               DBG_logPrintf( 'R', "Error, Offset beyond partition data size" );
            }
         }
         else
         {
            DBG_logPrintf( 'R', "Error, Partition is Read Protected" );
         }
      }
      else
      {
         DBG_logPrintf( 'R', "Error Opening Partition" );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "USAGE: nvr id start len" );
   }
#endif
   return ( 0 );
} /* end DBG_CommandLine_NvRead () */

/*******************************************************************************

   Function name: DBG_CommandLine_NvTest

   Purpose: Tests the NV memory

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_NvTest ( uint32_t argc, char *argv[] )
{
   uint32_t LoopCount = 100;
   char *endptr;
   if ( argc == 2 )
   {
      LoopCount = strtoul( argv[1], &endptr, 0 );
   }
   DBG_logPrintf( 'R', "Testing NV %d times...", LoopCount );
   if( 0 == DVR_EFL_UnitTest( LoopCount ) ) //External-FLash UnitTest
   {
      DBG_logPrintf( 'R', "NV Test Success!" );
   }
   else
   {
      DBG_logPrintf( 'R', "NV Test Failed!!!!" );
   }
   return ( 0 );
}
/* end DBG_CommandLine_NvTest() */

#if ( SIMULATE_POWER_DOWN == 1 )

/*******************************************************************************
*******************************************************************************/
OS_TICK_Struct DBG_CommandLine_GetStartTick ()
{
   return (DBG_startTick_);
}
/*******************************************************************************
*******************************************************************************/
bool DBG_CommandLine_isPowerDownSimulationFeatureEnabled()
{
   return pwrDnSimulation_Enable_;
}
/*******************************************************************************
*******************************************************************************/
#if 0
void DBG_CommandLine_isPowerDownSimulationFeatureDisable()
{
   pwrDnSimulation_Enable_ = false;
}
#endif
/*******************************************************************************

   Function name: DBG_CommandLine_SimulatePowerDown

   Purpose: Power Down Simulation Test. This function accepts the parameters as follows:
            (1) delay in miliseconds.
            (2) number of sectors to test on
            (3) ( 0 )External NV or Internal NV ( 1 )

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes: * NOT intended to be released to the production.
          * If you would like to cancel the test before powering down, use MFG port to reboot the device
*******************************************************************************/
uint32_t DBG_CommandLine_SimulatePowerDown ( uint32_t argc, char *argv[] )
{
   uint8_t  delaymSec       = 0;                    /* Max Wait time is 4.5 minutes*/
   uint8_t  numOfSectors   = DEFAULT_NUM_SECTORS;   /* Max 255 */
   bool     location       = EXTERNAL_NV;           /* false: External (default), true: Internal */
   bool     execute        = false;

#if 0 // DBG Test
   bool     eraseTestOnly = true;

   if( eraseTestOnly )
   {
      ( void )DBG_CommandLine_NvEraseTest ( 4, EXTERNAL_NV );
   }
   else
   {
#endif

   switch ( argc )
   {

      case ( 2 ):
      {
         delaymSec      = ( uint8_t )atoi( argv[1] );
         execute        = true;
         break;
      }
      case ( 3 ) :
      {
         delaymSec      = ( uint8_t )atoi( argv[1] );
         numOfSectors   = ( uint8_t )atoi( argv[2] );
         execute        = true;
         break;
      }
      case ( 4 ):
      {
         delaymSec      = ( uint8_t )atoi( argv[1] );
         numOfSectors   = ( uint8_t )atoi( argv[2] );
         location       = ( uint8_t )atoi( argv[3] );
         execute        = true;
         break;
      }
      default:
      {
         execute        = false;
         DBG_printf( "USAGE: PowerDownTest delay(ms) [no_of_sectors] [ 0: External NV; 1: Internal ]" );
         break;
      }
   }

   if( execute )
   {
      pwrDnSimulation_Enable_ = true; // Enable the feature

      if ( ( EXTERNAL_NV == location ) && ( 0 == numOfSectors ) )
      {
         numOfSectors = DEFAULT_NUM_SECTORS; // Update with default
         INFO_printf( " Using the defaults: Number of Sectors %d, Test Location: External NV !!!!" );
      }
      if ( ( INTERNAL_NV == location ) && ( numOfSectors > 4 ) )
      {
         numOfSectors = DEFAULT_NUM_SECTORS; // Update: Max value for internal NV
         INFO_printf( " Note: Max possible number of sectors for internal NV is 4 " );
      }
      DBG_logPrintf( 'R', " !!!! Power Down Simulation Test !!!!" );
      if( delaymSec != 0 )
      {
         INFO_printf( " !!!! Wait for %d msecs !!!!", delaymSec );
         ( void )DFWA_WaitForSysIdle(delaymSec );
      }

      INFO_printf( " !!!! Arming for Power Down !!!!" );

      /* Increase the priority of the DBG task and Print task */
      ( void )OS_TASK_Set_Priority( pTskName_Print, 11 ); // Increasing priority
      ( void )OS_TASK_Set_Priority( pTskName_Dbg, 11 );
      // Note: Increase the MFG port priority to have MFG port access to use "reboot" command
      ( void )OS_TASK_Set_Priority( pTskName_Mfg, 11 );
      ( void )OS_TASK_Set_Priority( pTskName_MfgUartRecv, 11 );
      ( void )OS_TASK_Set_Priority( pTskName_MfgUartCmd, 11 );
      ( void )OS_TASK_Set_Priority( pTskName_Idle, 13 );

      DBG_printf( " !!!! Now waiting for Power Down, Please TURN OFF the Power !!!!" );
      DBG_printf( " Note: If you would like to end this test NOW, use MFG port to send '''reboot''' command " );
      ( void )OS_SEM_Pend( &PWR_SimulatePowerDn_Sem, OS_WAIT_FOREVER ); // Wait for power down semaphore!
      OS_TICK_Get_CurrentElapsedTicks(&DBG_startTick_);
      // lock the PWR mutex
      PWR_lockMutex( PWR_MUTEX_ONLY ); // Function will not return if it fails
      ( void )DBG_CommandLine_NvEraseTest ( numOfSectors, location );
      PWR_unlockMutex( PWR_MUTEX_ONLY ); // Function will not return if it fails

      /* Never Returns */
   }
   return (0);
}
/* end DBG_CommandLine_SimulatePowerDown() */
/*******************************************************************************

   Function name: DBG_CommandLine_NvEraseTest

   Purpose: To perform R-E-W ( Read Erase Write ) test on Internal or External NV

   Arguments:  uint8_t - number of sectors
               bool - NV location to test ( 0: External 1: Internal )

   Returns: FuncStatus - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_NvEraseTest ( uint8_t sectors, bool nvLocation )
{
   uint32_t          sectorSize        = EXT_FLASH_SECTOR_SIZE;
   ePartitionName    partition         = ePART_DFW_PGM_IMAGE;
   uint32_t          bootLoaderOffset  = PART_BL_BACKUP_OFFSET;
   OS_TICK_Struct    startTick         = {0};
   OS_TICK_Struct    endTick           = {0};
   uint32_t          *pSomeBuffer;

   DBG_printf( " !!!! Welcome to NV Read Erase Write ( R-E-W ) Test !!!!" );
   INFO_printf( " !!!! Let's perform R-E-W on #%d Sectors ", sectors );

   if ( INTERNAL_NV == nvLocation )  // Internal
   {
      sectorSize        = INT_FLASH_ERASE_SIZE;  // same as sector erase size for Ext flash
      partition         = ePART_BL_BACKUP;
      bootLoaderOffset  = 0;
      INFO_printf(" Performing Test on Internal NV ");
   }
   else
   {
      INFO_printf(" Performing Test on External NV ");
   }
   // Read-Erase-Write for given no. of sectors. "One sector at a time"
   pSomeBuffer = malloc( sectorSize );

   if ( NULL != pSomeBuffer )
   {
#if 0
      //DG: DBG Prints
      INFO_printf( "Got  pSomeBuffer = 0x%08x", ( uint32_t )pSomeBuffer );
      INFO_printf( " Partition address :0x%08x", pTestPartition_->PhyStartingAddress );
#endif
      (void)memset( pSomeBuffer, 0 , sectorSize );
      // Fill the buffer with Random Number
      for ( uint16_t i = 0; i < ( uint16_t )( sectorSize/4) ; i++ )
      {
         pSomeBuffer[i] = ( uint32_t )aclara_rand();
      }
      /* Get the OS tick */
      OS_TICK_Get_CurrentElapsedTicks(&startTick);
      // TODO: Select the Partition based on the size? // RA6E1 Bob: This TODO was in the original K24 starting point
      if ( eSUCCESS == PAR_partitionFptr.parOpen(&pTestPartition_, partition, 0L) )
      {
         for ( ; sectors > 0 ; sectors-- )
         {
            // Write the pattern to the sector
            if ( eSUCCESS == PAR_partitionFptr.parWrite( bootLoaderOffset, (uint8_t *) pSomeBuffer, sectorSize, pTestPartition_ ) )
            {
               INFO_printf(" !!!!!!!! Successfully performed R-E-W on a sector !!!!!!!");
            }
            else
            {
               ERR_printf( "ERROR: Write FAILED!!!!!!!!!" );
            }
            bootLoaderOffset += sectorSize; // perform R-E-W on next sector
         } // End of for loop
         OS_TICK_Get_CurrentElapsedTicks(&endTick);
         INFO_printf(" Time to complete the NV Read-Write-Erase Test : %li usec", OS_TICK_Get_Diff_InMicroseconds ( &startTick, &endTick  ));
      }
      else
      {
         ERR_printf(" !!!!!!!! Failed to Open the partition !!!!!!! ");
      }
      free(pSomeBuffer);
   }
   else
   {
      ERR_printf( "ERROR: Falied to allocate memory ");
   }

   return ( 0 );
}
/* end DBG_CommandLine_NvTest() */
#endif /* End of #if ( SIMULATE_POWER_DOWN == 1 ) */
#if ( DCU == 1 )
/*******************************************************************************

   Function name: DBG_CommandLine_sdtest

   Purpose: Tests the external SDRAM memory

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
/* Kill tasks that are likely to try to use (malloc) RAM under test. */
static char const  *const tasks[] =
{
   "STRT",
   "APPMSG",
   "BPI",
   "SM",
   "PHY",
   "MAC",
   "NWK",
   "STAR",
   "DTLS",
   "BUAM",
   "DFW",
   "MFGC",
   "MFGU",
   "PAR",
   "TEST",
   NULL
};
extern unsigned char __EXTERNAL_MRAM_RAM_BASE[];  /* Defined in linker script   */
extern unsigned char __EXTERNAL_MRAM_RAM_SIZE[];  /* Defined in linker script   */
#define SDRAM_START __EXTERNAL_MRAM_RAM_BASE
#define SDRAM_SIZE  __EXTERNAL_MRAM_RAM_SIZE
static uint32_t DBG_CommandLine_sdtest ( uint32_t argc, char *argv[] )
{
   returnStatus_t    status; /*lint !e123 */
   char              **pTask;
   _task_id          id;
   uint32_t          LoopCount = 1;
   uint32_t          pattern;
   uint32_t          antipattern;
   uint32_t          readback;
   uint32_t          addressMask;
   uint32_t          offset;
   uint32_t          testOffset;
   char              *endptr;             /* Used with strtoul */
   volatile uint32_t *sdptr;              /* Pointer to SDRAM  */
   uint32_t          i;

   if ( argc == 2 )
   {
      LoopCount = strtoul( argv[1], &endptr, 0 );
   }
   pTask = (char **)tasks;
   do
   {
      id = OS_TASK_GetID_fromName( *pTask );
      if ( id != 0 )
      {
         (void)_task_destroy( id );
      }
      pTask++;
   } while ( *pTask != NULL );

   (void)printf( "Testing SDRAM %d times...\n", LoopCount );

   /*lint -e{443} status in first clause; not in third OK   */
   for ( status = eSUCCESS; LoopCount && ( status == eSUCCESS ); LoopCount-- )
   {
      (void)printf( "Loop: %lu\n", LoopCount );

      (void)printf( "Walking 1s test\n" );
      /* Perform walking ones test  */
      sdptr = (uint32_t *)(void*)SDRAM_START;
      for( i = 0; i < (uint32_t)SDRAM_SIZE / sizeof( uint32_t ) ; i++, sdptr++ )
      {
         for( pattern = 1U; pattern != 0; pattern <<= 1 )
         {
            *sdptr = pattern;       /* Write pattern  */

            readback = *sdptr;      /* Read back to variable, in case read/write not reliable.  */
            if ( readback != pattern )
            {
               (void)printf( "Walking 1 failed at location 0x%08p, s/b 0x%08lx, is 0x%08lx\n", sdptr, pattern, readback );
            }
         }
      }

      /* Perform address bus test   */
      (void)printf( "Address stuck high test\n" );
      sdptr = (uint32_t *)(void*)SDRAM_START;
      addressMask = ( (uint32_t)SDRAM_SIZE/sizeof( uint32_t ) - 1 );
      pattern     = (uint32_t) 0xAAAAAAAA;
      antipattern = (uint32_t) 0x55555555;

      /* Write the default pattern at each of the power-of-two offsets.  */
      for (offset = 1; (offset & addressMask) != 0; offset <<= 1)
      {
         sdptr[offset] = pattern;
      }

      /* Check for address bits stuck high.  */
      testOffset = 0;
      sdptr[testOffset] = antipattern;

      for (offset = 1; (offset & addressMask) != 0; offset <<= 1)
      {
         readback = sdptr[offset];
         if (readback != pattern)
         {
            (void)printf( "Address stuck high at location 0x%08p, s/b 0x%08lx, is 0x%08lx\n", sdptr+offset, pattern, readback );
            status = eFAILURE;
         }
      }

      sdptr[testOffset] = pattern;

      /* Check for address bits stuck low or shorted.  */
      (void)printf( "Address stuck low or shorted test\n" );
      for (testOffset = 1; (testOffset & addressMask) != 0; testOffset <<= 1)
      {
         sdptr[testOffset] = antipattern;

         readback = sdptr[0];
         if ( readback != pattern )
         {
            (void)printf( "Address stuck low at location 0x%08p, s/b 0x%08lx, is 0x%08lx\n", sdptr+offset, pattern, readback );
            status = eFAILURE;
         }

         for (offset = 1; (offset & addressMask) != 0; offset <<= 1)
         {
            if ((sdptr[offset] != pattern) && (offset != testOffset))
            {
               (void)printf( "Address stuck low at location 0x%08p, s/b 0x%08lx, is 0x%08lx\n", sdptr+offset, pattern, readback );
               status = eFAILURE;
            }
         }
         sdptr[testOffset] = pattern;
      }


      /* Fill sector with pattern (data = address low byte) */
      sdptr = (uint32_t *)(void*)SDRAM_START;
      for( i = 0; i < (uint32_t)SDRAM_SIZE / sizeof( uint32_t ) ; i++, sdptr++ )
      {
         *sdptr = ~( uint32_t )sdptr;
      }
      /* Check sector for pattern (data = address low byte) */
      sdptr = (uint32_t *)(void*)SDRAM_START;
      for( i = 0; i < (uint32_t)SDRAM_SIZE / sizeof( uint32_t ) ; i++, sdptr++ )
      {
         if ( *sdptr != ~( uint32_t )sdptr )
         {
            (void)printf( "Failed at location 0x%08p, s/b 0x%08x, is 0x%08x\n",
                           sdptr, ( uint32_t )sdptr, *sdptr );
            status = eFAILURE;
         }
      }
      WDOG_Kick();
   }

   if( eSUCCESS == status )
   {
      (void)printf( "SDRAM Test Success!\n" );
   }
   else
   {
      (void)printf( "SDRAM Test Failed!!!!\n" );
      OS_TASK_Sleep( ONE_SEC * 5 );
   }

   (void)printf( "Rebooting\n" );
   OS_TASK_Sleep( ONE_SEC );
   PWR_SafeReset();  /* Execute Software Reset, with cache flush */
   return ( 0 );
}
/* end DBG_CommandLine_sdtest() */
/*******************************************************************************

   Function name: DBG_CommandLine_getTBslot

   Purpose: Reports backplane slot sensed.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static uint32_t DBG_CommandLine_getTBslot( uint32_t argc, char *argv[] )
{
   uint8_t slotID;

   SLOT_ADDR( slotID );                            /* read slot ID      */
   slotID += '1';                                  /* Convert to ASCII and bias for base 1 reporting  */
   DBG_logPrintf( 'I', "Slot: %c (1 based)", (char)slotID ); /* Print result      */
   return 0;
}
#endif //#if ( DCU == 1 )

static const char checkmac_failed[] = "ECC108_CHECKMAC_FAILED";
static const char parse_error[]  = "ECC108_PARSE_ERROR";
static const char cmd_fail[]  = "ECC108_CMD_FAIL";
static const char status_crc[]  = "ECC108_STATUS_CRC";
static const char status_unknown[]  = "ECC108_STATUS_UNKNOWN";
static const char status_ecc[]  = "ECC108_STATUS_ECC";
static const char func_fail[]  = "ECC108_FUNC_FAIL";
static const char gen_fail[]  = "ECC108_GEN_FAIL";
static const char bad_param[]  = "ECC108_BAD_PARAM";
static const char invalid_id[]  = "ECC108_INVALID_ID";
static const char invalid_size[]  = "ECC108_INVALID_SIZE";
static const char bad_crc[]  = "ECC108_BAD_CRC";
static const char rx_fail[]  = "ECC108_RX_FAIL";
static const char rx_no_response[]  = "ECC108_RX_NO_RESPONSE";
static const char resync_with_wakeup[]  = "ECC108_RESYNC_WITH_WAKEUP";
static const char comm_fail[]  = "ECC108_COMM_FAIL";
static const char timeout[]  = "ECC108_TIMEOUT";
static const char unknown[]  = "Unknown";

/*******************************************************************************

   Function name: PrintECC_error

   Purpose: Local helper function to print (translation of) error codes returned
            by the security chip.
   Arguments: uint8_t error code returned by the security chip.

   Returns: None

   Notes:

*******************************************************************************/
static void PrintECC_error( uint8_t ECCstatus )
{
   char  const *msg;
   if ( ECCstatus != ECC108_SUCCESS )
   {
      switch ( ECCstatus )
      {
         case ECC108_CHECKMAC_FAILED:
         {
            msg = checkmac_failed;
            break;
         }
         case ECC108_PARSE_ERROR:
         {
            msg = parse_error;
            break;
         }
         case ECC108_CMD_FAIL:
         {
            msg = cmd_fail;
            break;
         }
         case ECC108_STATUS_CRC:
         {
            msg = status_crc;
            break;
         }
         case ECC108_STATUS_UNKNOWN:
         {
            msg = status_unknown;
            break;
         }
         case ECC108_STATUS_ECC:
         {
            msg = status_ecc;
            break;
         }
         case ECC108_FUNC_FAIL:
         {
            msg = func_fail;
            break;
         }
         case ECC108_GEN_FAIL:
         {
            msg = gen_fail;
            break;
         }
         case ECC108_BAD_PARAM:
         {
            msg = bad_param;
            break;
         }
         case ECC108_INVALID_ID:
         {
            msg = invalid_id;
            break;
         }
         case ECC108_INVALID_SIZE:
         {
            msg = invalid_size;
            break;
         }
         case ECC108_BAD_CRC:
         {
            msg = bad_crc;
            break;
         }
         case ECC108_RX_FAIL:
         {
            msg = rx_fail;
            break;
         }
         case ECC108_RX_NO_RESPONSE:
         {
            msg = rx_no_response;
            break;
         }
         case ECC108_RESYNC_WITH_WAKEUP:
         {
            msg = resync_with_wakeup;
            break;
         }
         case ECC108_COMM_FAIL:
         {
            msg = comm_fail;
            break;
         }
         case ECC108_TIMEOUT:
         {
            msg = timeout;
            break;
         }

         default:
            msg = unknown;
      }
      DBG_printf( "\nCode = 0x%02x, %s", ECCstatus, msg );
   }
}

/*******************************************************************************

   Function name: DBG_CommandLine_GenDFWkey

   Purpose: Derive the DFW encryption/decryption key

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
PACK_BEGIN
struct ver PACK_MID
{
   uint8_t  major;
   uint8_t  minor;
   uint16_t build;
};
PACK_END

static uint32_t DBG_CommandLine_GenDFWkey( uint32_t argc, char *argv[] )
{
   uint8_t     response[ 32 ];   /* Value ecc108e routine                  */
   struct
   {
      uint8_t     comDevType[ 20 ]; /* Com Device Type is 20 characters max   */
      struct ver  from;
      struct ver  to;
      uint8_t     zero[ 4 ];        /* 4 bytes of trailing zeroes             */
   } dfwChallenge;
   uint8_t     ECCstatus;

   if ( argc != 4 )
   {
      DBG_logPrintf( 'R', "Usage: GenDFWkey ComDeviceType FromVer(MM.mm.bbbb) ToVer(MM.mm.bbbb)" );
   }
   else
   {
      ( void )memset( ( uint8_t * )&dfwChallenge, 0, sizeof( dfwChallenge ) );
      ( void )strncpy( ( char * )dfwChallenge.comDevType, argv[ 1 ], sizeof( dfwChallenge.comDevType ) );
      ( void )sscanf( argv[ 2 ], "%hhu.%hhu.%hu", &dfwChallenge.from.major,
                      &dfwChallenge.from.minor, & dfwChallenge.from.build );
      ( void )sscanf( argv[ 3 ], "%hhu.%hhu.%hu", &dfwChallenge.to.major, &dfwChallenge.to.minor, & dfwChallenge.to.build );
      /* Convert build fields to big endian   */
      dfwChallenge.from.build = ( uint16_t )( dfwChallenge.from.build << 8 ) | ( dfwChallenge.from.build >> 8 );
      dfwChallenge.to.build = ( uint16_t )( dfwChallenge.to.build << 8 ) | ( dfwChallenge.to.build >> 8 );
      ECCstatus = ecc108e_DeriveFWDecryptionKey( ( uint8_t * )&dfwChallenge, response );
      if ( ECCstatus == ECC108_SUCCESS )
      {
         for ( uint32_t i = 0; i < sizeof( response ); i++ )
         {
            DBG_printfNoCr( "%02x", response[ i ] );
         }
         DBG_printf( "" );
      }
   }
   return 0;
}

/*******************************************************************************

   Function name: DBG_CommandLine_sectest

   Purpose: Exercise the security chip (I2C)

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
#if ( TM_TEST_SECURITY_CHIP == 1 )
#warning "Switch TM_TEST_SECURITY_CHIP is enabled!  The project will not compile!"
static const uint8_t verifyMsg[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };
static const uint8_t slot12sig[ 64 ] =
{
   0x5F, 0xA0, 0xFF, 0x1E, 0x82, 0xB4, 0x99, 0xC1, 0xA1, 0x55, 0xC1, 0x78, 0xEB, 0x66, 0x19, 0x67,
   0x37, 0x2F, 0x30, 0x15, 0x0A, 0x69, 0x46, 0x61, 0xCC, 0xA9, 0xE2, 0x12, 0xBF, 0x8D, 0x48, 0x87,
   0x9D, 0xEA, 0xA3, 0xAE, 0xF5, 0x31, 0xFA, 0x96, 0x59, 0xEF, 0x6C, 0x70, 0x3E, 0x68, 0x89, 0x0D,
   0xC5, 0x3C, 0xB1, 0x57, 0x32, 0x71, 0x75, 0x24, 0xEE, 0xDC, 0xA6, 0x50, 0x14, 0xF4, 0xDB, 0x62
};
static const uint8_t slot14sig[ 64 ] =
{
   0x26, 0x3E, 0xDD, 0x98, 0x14, 0xFC, 0x7F, 0xA9, 0xCE, 0x00, 0x41, 0x5F, 0xCB, 0x14, 0xF7, 0xB3,
   0xC7, 0x3B, 0xF7, 0x3B, 0x90, 0x0A, 0x64, 0x94, 0x67, 0x7A, 0xA4, 0xE1, 0x22, 0x19, 0x60, 0xF8,
   0x78, 0x8C, 0x37, 0x64, 0xBF, 0x98, 0xD1, 0xCC, 0xA6, 0x51, 0x0E, 0xC3, 0x44, 0x88, 0x6F, 0xE2,
   0xF9, 0x5E, 0x4B, 0xE7, 0x1F, 0x4F, 0x73, 0x31, 0xB4, 0x2C, 0x3F, 0x03, 0xE5, 0xD6, 0x2F, 0xB2
};
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
#if ( TM_TEST_SECURITY_CHIP == 1 )
static const uint8_t fwen[ 32 ] = { 0 };
#endif // ( TM_TEST_SECURITY_CHIP == 1 )

uint32_t DBG_CommandLine_sectest ( uint32_t argc, char *argv[] )
{
#if ( TM_TEST_SECURITY_CHIP == 1 )
   uint8_t        response[ VERIFY_256_SIGNATURE_SIZE ];
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
   char           *endptr;             /* Used with strtoul */
   uint32_t       LoopCount = 1;
   uint8_t        ECCstatus;
#if ( TM_TEST_SECURITY_CHIP == 1 )
   Sha256         sha;                 /* sha256 working area  */
   uint8_t        response[ 512 ];     /* Value returned by some ecc108e routines   */
   PartitionData_t const *pPart;       /* Used to access the security info partition  */
#endif // ( TM_TEST_SECURITY_CHIP == 1 )

   if ( argc == 2 )
   {
      LoopCount = strtoul( argv[1], &endptr, 0 );
   }
   DBG_logPrintf( 'R', "Exercising security device %d times...", LoopCount );
#if ( TM_TEST_SECURITY_CHIP == 1 )
   if ( ECC108_SUCCESS == ecc108_open() )
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
   {
      ECCstatus = ECC108_SUCCESS;
      do
      {
         ECCstatus = ecc108e_SelfTest();
#if ( TM_TEST_SECURITY_CHIP == 1 )
         ECCstatus = ecc108e_Verify( 14, sizeof( verifyMsg ), ( uint8_t * )verifyMsg, ( uint8_t * )slot14sig );
         /* sha256 the message */
         ( void )wc_InitSha256( &sha );
         ( void )wc_Sha256Update( &sha, verifyMsg, sizeof( verifyMsg ) ); /* sha256 on message */
         ( void )wc_Sha256Final( &sha, response );      /* Retrieve the sha256 results   */
         ECCstatus = ecc108e_Sign( SHA256_DIGEST_SIZE, response, response );
         for ( Cert_type i = AclaraPublicRootCert_e; i <= dtlsMSSubject_e; i++ )
         {
            ecc108e_GetDeviceCert( i, response, &LoopCount );
         }
         endptr = ( char * )ecc108e_SecureRandom();
         ECCstatus = ecc108e_Verify( 12, sizeof( verifyMsg ), ( uint8_t * )verifyMsg, ( uint8_t * )slot12sig );
         ECCstatus = ecc108e_EncryptedKeyWrite( PUB_KEY_SECRET, HOST_AUTH_KEY, ( bool )false, ( bool )false );
         ECCstatus = ecc108e_Verify( 12, sizeof( verifyMsg ), ( uint8_t * )verifyMsg, ( uint8_t * )slot12sig );
         ECCstatus = ecc108e_DeriveFWDecryptionKey( response, response );
         ECCstatus = ecc108e_EncryptedKeyWrite( NETWORK_PUB_KEY_SECRET, HOST_AUTH_KEY, ( bool )false, ( bool )false );
         ECCstatus = ecc108e_EncryptedKeyWrite( HOST_AUTH_KEY, KEY_UPDATE_WRITE_KEY, ( bool )true, ( bool )true );
         ECCstatus = ecc108e_Sign( sizeof( verifyMsg ), ( uint8_t * )verifyMsg, response );
         ECCstatus = ecc108e_DeriveFWDecryptionKey( ( uint8_t * )fwen, response );
         ECCstatus = ecc108e_EncryptedKeyRead( KEY_UPDATE_WRITE_KEY, HOST_AUTH_KEY, response );
         ecc108e_sleep();
         ECCstatus = ecc108e_Verify( 12, sizeof( verifyMsg ), ( uint8_t * )verifyMsg, ( uint8_t * )slot12sig );
         ECCstatus = ecc108e_Verify( 14, sizeof( verifyMsg ), ( uint8_t * )verifyMsg, ( uint8_t * )slot14sig );
         ECCstatus = ecc108e_read_mac( response );
         ECCstatus = ecc108e_Verify( 12, sizeof( verifyMsg ), ( uint8_t * )verifyMsg, ( uint8_t * )slot12sig );
         ECCstatus = ecc108e_Verify( 14, sizeof( verifyMsg ), ( uint8_t * )verifyMsg, ( uint8_t * )slot14sig );
         ECCstatus = ecc108e_EncryptedKeyWrite();
         ECCstatus = ecc108e_SelfTest();
         uint8_t        slotData[ 32 ];
         ECCstatus = ecc108e_EncryptedKeyRead( slotData );
         ecc108e_sleep();
         endptr = ( char * )ecc108e_SecureRandom();
         ecc108e_sleep();
         if( ECCstatus == ECC108_SUCCESS )
         {
            ECCstatus = ecc108e_readAllPublic();
         }
         ECCstatus = ecc108e_readAllPublic();
         ECCstatus = ecc108e_gen_public_key();
         PrintECC_error( ECCstatus );
         PrintECC_error( ECCstatus );
         ECCstatus = ecc108e_random( NULL );
         PrintECC_error( ECCstatus );
         ecc108e_write_config();
         break;
         ECCstatus = ecc108e_random( NULL );
         ECCstatus = ecc108e_checkmac_device();
         ECCstatus = ecc108e_checkmac_firmware();
         ECCstatus = ecc108e_verify_external();
         ECCstatus = ecc108e_verify_host();
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
         if( LoopCount )
         {
            LoopCount--;
         }
      }
      while( ( ECC108_SUCCESS == ECCstatus ) && ( LoopCount != 0 ) );
      PrintECC_error( ECCstatus );
      DBG_printf( "Security Device Test %s", ( ECC108_SUCCESS == ECCstatus ) ? "Success!" : "Failed!!!!" );
#if ( TM_TEST_SECURITY_CHIP == 1 )
      ecc108_close();
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
   }
#if ( TM_TEST_SECURITY_CHIP == 1 )
   else
   {
      DBG_logPrintf( 'R', "Failed to open device %s", i2cName );
   }
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
   return ( 0 );  /*lint !e438 endptr value not used; OK */
}

/*******************************************************************************

   Function name: DBG_CommandLine_secConfig

   Purpose: Configure the security chip (I2C)

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Note: This function can take an optional (unadvertised) parameter -
         if the string (case insensitive) "lockdata" is specified, the data zone will be locked.
            OR
         if the string (case insensitive) "lockconfig" is specified on the command line, the configuration zone will be
            locked.
            OR
         the word "customize", the Aclara customization (public key and signer cert) will be stored in the
            eccx08 device. The customization data must already be present in ecc108e_apps.c file.
            OR
         the word "genkey" - will generate a new private device key in slot 0, with the public key going to slot 9
            OR
         the word "verifykeys" - will attempt to verify all the public keys in the device.
            OR
         the word "crcconfig" will compute the CRC of the config zone. The user may also specify a SN as an
               additional parameter (up to 10 hex digits)

   Returns: FuncStatus - Successful status of this function.

*******************************************************************************/
uint32_t DBG_CommandLine_secConfig ( uint32_t argc, char *argv[] )
{
#if ( TM_TEST_SECURITY_CHIP == 1 )
   uint8_t        customize = 0;    /* Request to customize the data zone flag            */
   uint8_t        genkey = 0;       /* Request to generate a new private/public key pair  */
   uint8_t        verifykey = 0;    /* Request to verify all the public keys              */
   uint16_t       crc;              /* Computed CRC of config zone                        */
   uint64_t       *psn = NULL;
   uint64_t       sn = 0;
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
   uint8_t        crcconfig = 0;    /* Request to compute config zone CRC, with optional S/N   */
   uint8_t        ECCstatus;        /* Function return value                              */
   uint8_t        lockdata = 0;     /* Request to lock the data zone flag                 */
#if ( TM_TEST_SECURITY_CHIP == 1 )
   uint8_t        lockconfig = 0;   /* Request to lock the config zone flag               */
#endif // ( TM_TEST_SECURITY_CHIP == 1 )

   if ( argc >= 2 )     /* Extra parameter on command line - not advertised   */
   {
      if ( strcasecmp( "lockdata", argv[1] ) == 0 )
      {
         lockdata = 1;
      }
#if ( TM_TEST_SECURITY_CHIP == 1 )
      else if ( strcasecmp( "lockconfig", argv[1] ) == 0 )
      {
         lockconfig = 1;
      }
      else if ( strcasecmp( "customize", argv[1] ) == 0 )
      {
         customize = 1;
      }
      else if ( strcasecmp( "genkey", argv[1] ) == 0 )
      {
         genkey = 1;
      }
      else if ( strcasecmp( "verifykeys", argv[1] ) == 0 )
      {
         verifykey = 1;
      }
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
      else if ( strcasecmp( "crcconfig", argv[1] ) == 0 )
      {
#if ( TM_TEST_SECURITY_CHIP == 1 )
         crcconfig = 1;
         if( argc == 3 )      /* S/N provided?  */
         {
            char *endptr;
            /* Use should input as SN[0:3]SN[4-7]
               SN[8] is always 0xEE
               e.g. SN[0-3] = 01236B37
                     SN[4-7] = 69A1FF4F
               type
               secconfig crcconfig 0x01236B3769A1FF4F
            */
            sn = strtoull( argv[2], &endptr, 0 );
            psn = &sn;
         }
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
      }
   }
   if ( ECC108_SUCCESS == ecc108_open() )
   {
      if( lockdata != 0 )
      {
#if ( TM_TEST_SECURITY_CHIP == 1 )
         DBG_logPrintf( 'R', "Attempting to lock ECCx08 data." );
         ecc108e_sleep();
         ECCstatus = ecc108e_lock_data_zone();
         PrintECC_error( ECCstatus );
         DBG_logPrintf( 'R', "ECCx08 lock data %s.", ( ECCstatus == ECC108_SUCCESS ) ? "succeeded" :  "failed" );
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
      }
#if ( TM_TEST_SECURITY_CHIP == 1 )
      else if ( customize != 0 )
      {
         DBG_logPrintf( 'R', "Attempting to customize ECCx08." );
         ECCstatus = ecc108e_customize();
         PrintECC_error( ECCstatus );
         DBG_logPrintf( 'R', "ECCx08 customize %s.", ( ECCstatus == ECC108_SUCCESS ) ? "succeeded" :  "failed" );
      }
      else if ( verifykey != 0 )
      {
         DBG_logPrintf( 'R', "Attempting to verify ECCx08 keys." );
         ECCstatus = ecc108e_verify_public_keys();
         PrintECC_error( ECCstatus );
         DBG_logPrintf( 'R', "ECCx08 gen key %s.", ( ECCstatus == ECC108_SUCCESS ) ? "succeeded" :  "failed" );
      }
      else if ( genkey != 0 )
      {
         DBG_logPrintf( 'R', "Attempting to generate ECCx08 key." );
         ECCstatus = ecc108e_gen_public_key();
         PrintECC_error( ECCstatus );
         DBG_logPrintf( 'R', "ECCx08 gen key %s.", ( ECCstatus == ECC108_SUCCESS ) ? "succeeded" :  "failed" );
      }
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
      else if ( crcconfig != 0 ) /*lint !e774 crcconfig always 0; OK temporary code */
      {
#if ( TM_TEST_SECURITY_CHIP == 1 )
         DBG_logPrintf( 'R', "Computing ECCx08 config zone CRC." );
         PrintECC_error( ECCstatus );
         DBG_logPrintf( 'R', "Computing config CRC%s.", ( ECCstatus == ECC108_SUCCESS ) ? "succeeded" :  "failed" );
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
      }
      else
      {
//       ECCstatus = ecc108e_check_lock_status();  /* Returns failure on "UNlocked" */
#if ( TM_TEST_SECURITY_CHIP == 1 )
         if ( ECCstatus != ECC108_FUNC_FAIL )      /* Make sure not locked already  */
         {
            PrintECC_error( ECCstatus );
            DBG_logPrintf( 'R', "ECCx08 config already locked." );
         }
         else
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
         {
#if ( TM_TEST_SECURITY_CHIP == 1 )
            DBG_logPrintf( 'R', "Writing default values to the ECCx08 configuration zone." );
//          ECCstatus = ecc108e_write_config(); /* Attempt to write default config data   */
            PrintECC_error( ECCstatus );
            DBG_logPrintf( 'R', "ECCx08 config update %s.", ( ECCstatus == ECC108_SUCCESS ) ? "succeeded" : "failed" );
            if ( ECCstatus == ECC108_SUCCESS )
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
            {
               ECCstatus = ecc108e_verify_config();
               PrintECC_error( ECCstatus );
               DBG_logPrintf( 'R', "ECCx08 config verification %s.",
                              ( ECCstatus == ECC108_SUCCESS ) ? "succeeded" :  "failed" );
#if ( TM_TEST_SECURITY_CHIP == 1 )
               if ( ECCstatus == ECC108_SUCCESS )
               {
                  if( lockconfig != 0 ) /*lint !e774 lockconfig always 0; OK temporary code */
                  {
                     ecc108e_sleep();
//                   ECCstatus = ecc108e_lock_config_zone();
                     PrintECC_error( ECCstatus );
                     DBG_logPrintf( 'R', "ECCx08 lock config %s.",
                                    ( ECCstatus == ECC108_SUCCESS ) ? "succeeded" :  "failed" );
                  }
               }
#endif // ( TM_TEST_SECURITY_CHIP == 1 )
               ecc108e_sleep();
            }
         }
      }
      ecc108_close();
   }
   else
   {
      DBG_logPrintf( 'R', "Failed to open device %s", i2cName );
   }
   return ( 0 );
}
/* end DBG_CommandLine_secConfig() */

#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
/*******************************************************************************

   Function name: DBG_CommandLine_clockswtest

   Purpose: Capture cyclecounter difference at two different clock speed settings over 5 seconds

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static uint32_t DBG_CommandLine_clockswtest ( uint32_t argc, char *argv[] )
{
   char           floatStr[PRINT_FLOAT_SIZE];
   uint32_t       startCycles;
   uint32_t       endCycles;
   uint32_t       diffCycles;
   float          clockFreq;
   returnStatus_t retVal = eSUCCESS;

   if ( _bsp_get_clock_configuration() != BSP_CLOCK_CONFIGURATION_DEFAULT )
   {
      retVal = (returnStatus_t)_bsp_set_clock_configuration( BSP_CLOCK_CONFIGURATION_DEFAULT ); /* Set clock to 180MHz, high speed run mode.  */
   }

   if ( retVal == eSUCCESS )
   {
      DBG_printf( "Getting current cycle counter and sleeping 5 seconds" );
      startCycles = DWT_CYCCNT;

      OS_TASK_Sleep( ONE_SEC * 5 );

      endCycles = DWT_CYCCNT;
      diffCycles = endCycles - startCycles;
      clockFreq = (float)diffCycles / 5.0E6;

      DBG_printf( "Start cycles: %lu, End cycles: %lu, diff: %lu, freq: %s MHz", startCycles, endCycles, diffCycles, DBG_printFloat( floatStr, clockFreq, 3 ) );


      DBG_printf( "Switching clock to 120MHz" );

      retVal = (returnStatus_t)_bsp_set_clock_configuration( BSP_CLOCK_CONFIGURATION_3 ); /* Set clock to 120MHz, normal run mode.  */
      if ( retVal == eSUCCESS )
      {
         startCycles = DWT_CYCCNT;

         OS_TASK_Sleep( ONE_SEC * 5 );

         endCycles = DWT_CYCCNT;
         diffCycles = endCycles - startCycles;
         clockFreq = (float)diffCycles / 5.0E6;

         DBG_printf( "Start cycles: %lu, End cycles: %lu, diff: %lu, freq: %s MHz", startCycles, endCycles, diffCycles, DBG_printFloat( floatStr, clockFreq, 3 ) );
      }
      else
      {
         DBG_printf( "Unable to switch clock to 120MHz" );
      }
   }
   else
   {
      DBG_printf( "Unable to switch clock to default configuration" );
   }

   if ( _bsp_get_clock_configuration() != BSP_CLOCK_CONFIGURATION_DEFAULT )
   {
      (void)_bsp_set_clock_configuration( BSP_CLOCK_CONFIGURATION_DEFAULT ); /* Set clock to 180MHz, high speed run mode.  */
   }

   return 0;
}
#endif   /* HAL_TARGET_XCVR_9985_REV_A  */

/*******************************************************************************

   Function name: DBG_CommandLine_clocktst

   Purpose: This function allows user to turn the clkout signal on or off

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_clocktst( uint32_t argc, char *argv[] )
{
   uint32_t on;
   uint8_t string[VER_HW_STR_LEN];   /* Version string including the two '.' and a NULL */

   ( void )VER_getHardwareVersion ( &string[0], sizeof(string) );
#if ( MCU_SELECTED == NXP_K24 ) // The CLKOUT pin is grounded in the Y8409x K24 NIC, not in RA6E1
   /* Rev C HW, PTC3/CLKOUT pin is grounded */
   if ( 'C' == string[0] )
   {
      DBG_printf( "This is Rev C HW, PTC3/CLKOUT pin is grounded" );

      return( 0 );
   }
#endif // ( MCU_SELECTED == NXP_K24 )
   if( argc != 2 )      /* Must be at least on parameter */
   {
      DBG_logPrintf( 'R', "1/0 (on/off) parameter required" );
   }
   else
   {
      ( void )sscanf( argv[1], "%lu", &on ); /* Get user's request   */
      if( on == 0 )
      {
#if ( MCU_SELECTED == NXP_K24 )
         SIM_SOPT2 &= ~( SIM_SOPT2_CLKOUTSEL( 7 ) ); /* Disable clock              */
         OSC_CR &= ~( OSC_CR_ERCLKEN_MASK << OSC_CR_ERCLKEN_SHIFT ); /* Disable the clock output   */
#elif ( MCU_SELECTED == RA6E1 )
         DBG_logPrintf( 'R', "clocktst command is not currently supported for RA6E1"); // TODO: RA6E1 Bob: Support this?
#endif // MCU_SELECTED
#if ( DCU == 1 )
         PORTE_PCR0 &= ~PORT_PCR_MUX( 7 );
         PORTE_PCR0 |= PORT_PCR_MUX( 1 );          /* Set PTE0 as normal GPIO    */
#else
   #if ( MCU_SELECTED == NXP_K24 )
         PORTC_PCR3 &= ~PORT_PCR_MUX( 7 );
         PORTC_PCR3 |= PORT_PCR_MUX( 1 );          /* Set PTC3 as normal GPIO    */
   #elif ( MCU_SELECTED == RA6E1 )

   #endif // MCU_SELECTED
#endif // ( DCU == 1 )
      }
      else
      {
#if ( MCU_SELECTED == NXP_K24 )
         SIM_SOPT2 &= ~( SIM_SOPT2_CLKOUTSEL( 7 ) ); /* Disable clock              */
         SIM_SOPT2 |= ( SIM_SOPT2_CLKOUTSEL( 6 ) ); /* Select the external oscillator as the source clock */
#elif ( MCU_SELECTED == RA6E1 )
         DBG_logPrintf( 'R', "clocktst command is not currently supported for RA6E1"); // TODO: RA6E1 Bob: Support this?
#endif // MCU_SELECTED
#if ( DCU == 1 )
//         RTC_CR |= ( uint8_t )( OSC_CR_ERCLKEN_MASK << OSC_CR_ERCLKEN_SHIFT ); /* Enable the clock output    */
//         PORTE_PCR0 |= PORT_PCR_MUX( 7 );    /* Set PTE0 as RTC_CLKOUT signal  */
         PORTE_PCR0 |= PORT_PCR_MUX( 5 );    /* Set PTE0 as TRACE_CLKOUT signal (i.e. 60MHz) */
#else
   #if ( MCU_SELECTED == NXP_K24 )
         OSC_CR |= ( uint8_t )( OSC_CR_ERCLKEN_MASK << OSC_CR_ERCLKEN_SHIFT ); /* Enable the clock output    */
         PORTC_PCR3 |= PORT_PCR_MUX( 5 );    /* Set PTC3 as CLKOUT signal  */
   #elif ( MCU_SELECTED == RA6E1 )

   #endif // MCU_SELECTED
#endif // ( DCU == 1 )
      }
      DBG_logPrintf( 'R', "clock: %s", on == 0 ? "off" : "on" );
   }
   return 0;
}

/*******************************************************************************

   Function name: DBG_CommandLine_Comment

   Purpose: "Do nothing" routine. Allows comment to be in debug output log

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: Always successful

   Notes:

*******************************************************************************/
static uint32_t DBG_CommandLine_Comment( uint32_t argc, char *argv[] )
{
   return 0;
}

#if ( TM_UART_ECHO_COMMAND == 1 )
/*******************************************************************************

   Function name: DBG_CommandLine_EchoComment

   Purpose: Echos the command string in arg[0] for testing UART cut/paste

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: Always successful

   Notes:

*******************************************************************************/
static uint32_t DBG_CommandLine_EchoComment( uint32_t argc, char *argv[] )
{
   DBG_printfNoCr( "ECHO" );
   if ( argc > 1 )
   {
      for ( uint32_t i = 1; i < argc; i++ )
      {
         DBG_printfNoCr( " %s", argv[i] );
      }
   }
   DBG_printf( " " );
   return 0;
}
#endif // TM_UART_ECHO_COMMAND

/*******************************************************************************

   Function name: DBG_CommandLine_mtrTime

   Purpose: This function converts the input time in meter time format to system time, and converts it to UTC.
            meter time format is yy mm dd hh mm ss
            Enter arguments mm dd yy hh mm (more natural input format )

   Arguments:  if argv[0] is mtrtime then converts meter format time to TIMESTAMP_t seconds
                  Converts meter time to system time and applies local to UTC conversion.
               if argv[0] is revmtrtime the converts TIMESTAMP_t seconds to meter format date
                  Converts entered time (seconds since epoch) to system time, and displays that
                  applies UTC to local conversion and displays that.

               argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
#if ( CLOCK_IN_METER == 1 )
static uint32_t DBG_CommandLine_mtrTime( uint32_t argc, char *argv[] )
{
   sysTime_t                     sTime;      /* Meter time converted to system time */
   sysTime_dateFormat_t          sDateTime;  /* Seconds converted to date/time format  */
   uint32_t                      seconds;    /* System time converted to seconds since epoch */
   uint32_t                      fracSecs;   /* Unused, but required fractional seconds  */
   MtrTimeFormatHighPrecision_t  mTime;      /* Structure built with input values   */

   if ( strcasecmp( argv[ 0 ], "mtrtime" ) == 0 )
   {
      if ( 6 == argc )  /* Must be exactly 6 parameters  */
      {
         /* Get individual date/time quantities */
         mTime.month   = (uint8_t)strtoul( argv[1], NULL, 0 );  /* Get two digits of month  */
         mTime.day     = (uint8_t)strtoul( argv[2], NULL, 0 );  /* Get two digits of day  */
         mTime.year    = (uint8_t)strtoul( argv[3], NULL, 0 );  /* Get two digits of year  */
         mTime.hours   = (uint8_t)strtoul( argv[4], NULL, 0 );  /* Get two digits of hours  */
         mTime.minutes = (uint8_t)strtoul( argv[5], NULL, 0 );  /* Get two digits of minutes  */
         mTime.seconds = 0;

         (void)TIME_UTIL_ConvertMtrHighFormatToSysFormat( &mTime, &sTime);    /*  Convert meter local time to system time and convert to UTC  */
         TIME_UTIL_ConvertSysFormatToSeconds( &sTime, &seconds, &fracSecs);
         DBG_logPrintf( 'R', "UTC: %hhu/%hhu/%hhu %hhu:%02hhu:%02hhu 0x%08lX", mTime.month, mTime.day, mTime.year, mTime.hours, mTime.minutes,
                        mTime.seconds, seconds ); /*  Display system time */
      }
      else
      {
         DBG_logPrintf( 'E', "Enter date: mm dd yy hh mm" );
      }
   }
   else if ( ( argc == 2 ) && ( strcasecmp( argv[ 0 ], "revmtrtime" ) == 0 ) )
   {
      seconds = strtoul( argv[ 1 ], NULL, 0 );
      fracSecs = 0;

      TIME_UTIL_ConvertSecondsToSysFormat( seconds, fracSecs, &sTime );    /* Convert input seconds to sysTime_t format */
      (void)TIME_UTIL_ConvertSysFormatToMtrHighFormat( &sTime, &mTime );   /* Convert to meter format, local   */

      DBG_logPrintf( 'R', "mtr time (local):  %02hhu/%02hhu/%04hu %02hhu:%02hhu:%02hhu", mTime.month, mTime.day, mTime.year + 2000U,
                     mTime.hours, mTime.minutes, mTime.seconds );

      (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sDateTime );  /* Convert to date and time format */
      DBG_logPrintf( 'R', "mtr time (UTC)  :  %02hhu/%02hhu/%04hu %02hhu:%02hhu:%02hhu", sDateTime.month, sDateTime.day, sDateTime.year,
                     sDateTime.hour, sDateTime.min, sDateTime.sec );
   }
   return 0;
}
#endif

/*******************************************************************************

   Function name: DBG_CommandLine_time

   Purpose: This function will print out the current time or set the time

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_time ( uint32_t argc, char *argv[] )
{
   if ( 1 == argc ) /* No parameters will read the RTC time and system time */
   {
      sysTime_dateFormat_t sysTime;
      sysTime_dateFormat_t RTC_time;
      sysTime_t            sTime;
#if ( RTOS_SELECTION == MQX_RTOS )
      TIME_STRUCT          mqxTime;
#endif
      sysTime_t            pupTime;
      uint32_t             sysSeconds;
      uint32_t             sysFracSecs;
      char valid[]         = "";
      char invalid[]       = " (Invalid time)";
      returnStatus_t       retVal;

#if ( RTOS_SELECTION == MQX_RTOS )
      // Display MQX time
      _time_get( &mqxTime );
      TIME_UTIL_ConvertSecondsToSysFormat( mqxTime.SECONDS, mqxTime.MILLISECONDS, &sTime );
      ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );
      DBG_logPrintf( 'R', "MQX:   %02u/%02u/%04u %02u:%02u:%02u.%03u",
                     sysTime.month, sysTime.day, sysTime.year, sysTime.hour, sysTime.min, sysTime.sec, mqxTime.MILLISECONDS );
#endif
      // Display RTC time
      RTC_GetDateTime ( &RTC_time );
      DBG_logPrintf( 'R', "RTC:   %02u/%02u/%04u %02u:%02u:%02u.%03u",
                     RTC_time.month, RTC_time.day, RTC_time.year, RTC_time.hour, RTC_time.min, RTC_time.sec, RTC_time.msec);

      // Display system time
      retVal = TIME_UTIL_GetTimeInDateFormat( &sysTime );
      TIME_UTIL_ConvertSysFormatToSeconds( &sTime, &sysSeconds, &sysFracSecs);
      DBG_logPrintf( 'R', "Sys:   %02u/%02u/%04u %02u:%02u:%02u.%03u%s, combined: %08lX",
                     sysTime.month, sysTime.day, sysTime.year, sysTime.hour, sysTime.min, sysTime.sec, sysTime.msec,
                     retVal == eSUCCESS ? valid : invalid,
                     sysSeconds );
#if ( EP == 1 )
      // Display local time
      ( void )DST_getLocalTime( &sTime );
      ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );
      DBG_logPrintf( 'R', "Local: %02u/%02u/%04u %02u:%02u:%02u.%03u%s",
                     sysTime.month, sysTime.day, sysTime.year, sysTime.hour, sysTime.min, sysTime.sec, sysTime.msec,
                     retVal == eSUCCESS ? valid : invalid );
#endif
      TIME_SYS_GetPupDateTime( &pupTime );
      ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &pupTime, &sysTime );
      DBG_logPrintf( 'R', "Pup:   % 5u days %02u:%02u:%02u.%03u",
                     pupTime.date, sysTime.hour, sysTime.min, sysTime.sec, sysTime.msec );
   }
   else if ( 2 == argc )
   {
      char *endptr;
      uint32_t locdateTime;

      locdateTime = strtoul( argv[1], &endptr, 0 );

      if ( eSUCCESS == TIME_UTIL_SetTimeFromSeconds( locdateTime, 0 ) )
      {
         DBG_logPrintf( 'R', "%s %d", argv[ 0 ], locdateTime );
      }
   }
   else
   {
      if ( 7 == argc ) /* Need all six parameters to set the RTC and System time. */
      {
         sysTime_dateFormat_t rtcTime;
         rtcTime.year   = ( uint16_t )( atoi( argv[1] ) + 2000 ); /* Adjust the 2 digit date to 20xx */
         rtcTime.month  = ( uint8_t )( atoi( argv[2] ) );
         rtcTime.day    = ( uint8_t )( atoi( argv[3] ) );
         rtcTime.hour   = ( uint8_t )( atoi( argv[4] ) );
         rtcTime.min    = ( uint8_t )( atoi( argv[5] ) );
         rtcTime.sec    = ( uint8_t )( atoi( argv[6] ) );
         rtcTime.msec   = 0;  /* Not used, but ensure cleared */

         if ( eSUCCESS == TIME_UTIL_SetTimeFromDateFormat( &rtcTime ) )
         {
            DBG_logPrintf( 'R', "RTC and System time set successfully" );
         }
         else
         {
            DBG_logPrintf( 'R', "Failed to set system time!" );
         }
      }
      else
      {
         DBG_logPrintf( 'R', "Invalid number of parameters!  No params for read, 6 params to set date/time" );
      }
   }
   return ( 0 );
} /* end DBG_CommandLine_time () */

//#if ( EP == 1 ) && ( TEST_TDMA == 1 )
///*******************************************************************************
//
//   Function name: DBG_CommandLine_Delay
//
//   Purpose: This function will print out the current time or set the time
//
//   Arguments:  argc - Number of Arguments passed to this function
//               argv - pointer to the list of arguments passed to this function
//
//   Returns: FuncStatus - Successful status of this function - currently always 0 (success)
//
//   Notes:
//
//*******************************************************************************/
//extern uint32_t timeSlotDelay;
//uint32_t DBG_CommandLine_Delay ( uint32_t argc, char *argv[] )
//{
//   if ( 1 == argc ) /* No parameters will read the RTC time and system time */
//   {
//      DBG_logPrintf( 'R', "Delay = %u", timeSlotDelay );
//   }
//   else if ( 2 == argc )
//   {
//      timeSlotDelay = (uint32_t)atoi( argv[1] );
//      if (timeSlotDelay > 100) {
//         DBG_logPrintf( 'R', "Time Slot Delay must between 0 and 100" );
//      }
//   }
//   return ( 0 );
//} /* end DBG_CommandLine_time () */
//#endif

/*******************************************************************************

   Function name: DBG_CommandLine_rtcTime

   Purpose: This function will print out the current time or set the time (in RTC only)

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_rtcTime ( uint32_t argc, char *argv[] )
{
   if ( 1 == argc ) /* No parameters will read the RTC time */
   {
      sysTime_dateFormat_t RTC_time;

      RTC_GetDateTime ( &RTC_time );

      DBG_logPrintf( 'R', "RTC=%02d/%02d/%04d %02d:%02d:%02d",
                     RTC_time.month, RTC_time.day, RTC_time.year, RTC_time.hour, RTC_time.min, RTC_time.sec );

   }
   else
   {
      if ( 7 == argc ) /* Need all six parameters to set the RTC and System time. */
      {
         sysTime_dateFormat_t rtcTime;
         rtcTime.year   = ( uint16_t )( atoi( argv[1] ) + 2000 ); /* Adjust the 2 digit date to 20xx */
         rtcTime.month  = ( uint8_t )( atoi( argv[2] ) );
         rtcTime.day    = ( uint8_t )( atoi( argv[3] ) );
         rtcTime.hour   = ( uint8_t )( atoi( argv[4] ) );
         rtcTime.min    = ( uint8_t )( atoi( argv[5] ) );
         rtcTime.sec    = ( uint8_t )( atoi( argv[6] ) );

         if ( RTC_SetDateTime ( &rtcTime ) )
         {
            DBG_logPrintf( 'R', "RTC time set successfully" );
         }
         else
         {
            DBG_logPrintf( 'R', "Failed to set system time!" );
         }
      }
      else
      {
         DBG_logPrintf( 'R', "Invalid number of parameters!  No params for read, 6 params to set date/time" );
      }
   }
   return ( 0 );
} /* end DBG_CommandLine_rtcTime () */

#if ( MCU_SELECTED == NXP_K24 )
/***********************************************************************************************************************
   Function Name: DBG_CommandLine_rtcCap

   Purpose: Get/Set RTC Oscillator Cap Load in RTC_SR

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
uint32_t DBG_CommandLine_rtcCap( uint32_t argc, char *argv[] )
{
   uint32_t uValue;
   uint8_t uPF = 0;

   uValue = ( uint32_t ) RTC_CR;

   if ( 1 == argc )
   {
      DBG_printf( "RTC CAP: 0x%02x",
                  (  uValue & ( RTC_CR_SC16P_MASK | RTC_CR_SC8P_MASK | RTC_CR_SC4P_MASK | RTC_CR_SC2P_MASK ) ) >>
                  RTC_CR_SC16P_SHIFT );

      if ( 0 != ( uValue & RTC_CR_SC16P_MASK ) )
      {
         uPF += 16;
      }
      if ( 0 != ( uValue & RTC_CR_SC8P_MASK ) )
      {
         uPF += 8;
      }
      if ( 0 != ( uValue & RTC_CR_SC4P_MASK ) )
      {
         uPF += 4;
      }
      if ( 0 != ( uValue & RTC_CR_SC2P_MASK ) )
      {
         uPF += 2;
      }

      DBG_printf( "RTC CAP: %d pF", uPF );
   }
   else if ( 2 == argc )
   {
      uPF = ( uint8_t ) atoi( argv[1] );

      if ( ( uPF <= 30 ) && ( 0 == ( uPF & 0x01 ) ) )
      {
         // Disable RTC Oscillator.
         uValue &= ~RTC_CR_OSCE_MASK;
         RTC_CR = uValue;

         // Mask out all RTC_CR cap load values.
         uValue &= ~( RTC_CR_SC16P_MASK | RTC_CR_SC8P_MASK | RTC_CR_SC4P_MASK | RTC_CR_SC2P_MASK );

         if ( uPF >= 16 )
         {
            uValue |= RTC_CR_SC16P_MASK;
            uPF -= 16;
         }

         if ( uPF >= 8 )
         {
            uValue |= RTC_CR_SC8P_MASK;
            uPF -= 8;
         }

         if ( uPF >= 4 )
         {
            uValue |= RTC_CR_SC4P_MASK;
            uPF -= 4;
         }

         if ( uPF >= 2 )
         {
            uValue |= RTC_CR_SC2P_MASK;
         }

         // Enable RTC Oscillator.
         uValue |= RTC_CR_OSCE_MASK;
         RTC_CR = uValue;
      }
      else
      {
         DBG_logPrintf( 'R', "Invalid value, must be an even number between 0 and 30 inclusive" );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   return( 0 );
}
#endif // #if ( MCU_SELECTED == NXP_K24 )

#if (EP == 1)
/***********************************************************************************************************************
   Function Name: DBG_CommandLine_lpstats

   Purpose: Print out Load Profile configuration info and status

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
#if ( LP_IN_METER != 0 )
static uint32_t DBG_CommandLine_lpstats ( uint32_t argc, char *argv[] )
{
   stdTbl61_t                    tbl61;         /* Full table 61  (13 bytes)  */
   stdTbl63_t                    tbl63;         /* Full table 63  (13 bytes)  */
   MtrTimeFormatHighPrecision_t  tblTime;       /* Time from meter ( yy mm dd hh mm ) */
   sysTime_dateFormat_t          mtrTime;       /* Meter time in mm dd yy hh mm format                                                                             */
   sysTime_t                     sTime;         /* Meter time converted to system time */
   sysTimeCombined_t             startTime;     /* Meter block start time  */
   uint32_t                      offset;        /* byte offset into table 64  */
   uint32_t                      fracSecs= 0;   /* Unused, but required fractional seconds  */
   uint32_t                      seconds;       /* System time converted to seconds since epoch */
   uint16_t                      block;         /* tbl64 block index */
   uint16_t                      endBlock;      /* Last  block to report (0 based)   */
   uint16_t                      startBlock;    /* First block to report (0 based), default to last valid block   */
   uint16_t                      blockSize;     /* computed size of each block */
   uint8_t                       statusBytes;   /* Number of status bytes per interval (for all channels)   */
   uint8_t                       dataBytes;     /* Number of data bytes (for all channels)   */
   uint8_t                       readOffset;    /* offset where the intervals in the block start */

   DBG_logPrintf( 'R', "Reading tables 61 and 63" );
   if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &tbl61, STD_TBL_ACTUAL_LOAD_PROFILE_LIMITING, 0, (uint16_t)sizeof( stdTbl61_t ) ) )
   {
      DBG_logPrintf( 'R', "memory length: %u (0x%x), number of blocks: %hu, intervals per block: %hu, no. channels: %hhu, interval time: %hhu",
            tbl61.lpMemoryLen, tbl61.lpMemoryLen, tbl61.lpDataSet1.nbrBlksSet, tbl61.lpDataSet1.nbrBlksIntsSet, tbl61.lpDataSet1.nbrChnsSet,
            tbl61.lpDataSet1.nbrMaxIntTimeSet );
   }
   if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &tbl63, STD_TBL_LOAD_PROFILE_STATUS, 0, (uint16_t)sizeof( stdTbl63_t ) ) )
   {
      DBG_logPrintf( 'R', "no. valid blocks: %hu , last block element %hu, no. valid intervals: %hu",
            tbl63.nbrValidBlocks, tbl63.lastBlockElement, tbl63.nbrValidInt );
   }

   startBlock = tbl63.lastBlockElement;
   endBlock   = startBlock;

   if ( argc >= 2 )  /* Requested starting block;  */
   {
      startBlock  = (uint16_t)min( (uint16_t)strtoul( argv[1], NULL, 0), tbl63.lastBlockElement );
      endBlock    = startBlock;
   }
   if ( argc > 2 )  /* Requested ending block;  */
   {
      endBlock = (uint16_t)max( startBlock, (uint16_t)strtoul( argv[2], NULL, 0 ) );
      endBlock = (uint16_t)min( endBlock, tbl63.lastBlockElement );
   }

   statusBytes = ( tbl61.lpDataSet1.nbrChnsSet / 2U ) + 1U;                   /* Bytes for status  */
   dataBytes = tbl61.lpDataSet1.nbrChnsSet * 2U;                              /*lint !e734 (9 bits to 8 bits, bytes for data, 2 per channel) */
   readOffset = sizeof( stdTbl64Time_t );                                     /* Bytes for the date and time of the last read */
   readOffset += ( 6U * tbl61.lpDataSet1.nbrChnsSet );                        /*lint !e734 Bytes for end block reads */
   blockSize = ( statusBytes + dataBytes ) * tbl61.lpDataSet1.nbrBlksIntsSet; /*lint !e734 (Bytes for all the readings in block) */
   blockSize += readOffset;
   offset = startBlock * blockSize;

   /* Loop through all the valid blocks. Note tbl63.lastBlockElement is an index, so include that value in the loop.  */
   for ( block = startBlock; block <= endBlock; block++ )
   {
      if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &tblTime, STD_TBL_LOAD_PROFILE_DATASET_1, offset, ( uint16_t )sizeof( tblTime ) ) )
      {
         /* Print block end times in local and UTC */
         tblTime.seconds = 0;                                                 /* Meter doesn't record the seconds value */
         (void)TIME_UTIL_ConvertMtrHighFormatToSysFormat( &tblTime, &sTime);  /* sTime is meter UTC date in sysTime_t  */
         DST_ConvertUTCtoLocal( &sTime );                                     /* Convert meter UTC time to local time  */
         DBG_logPrintf( 'R', "block %hhu, @ 0x%p", block, offset );
         DBG_logPrintf( 'R', "block end   local date = %02hhu/%02hhu/%4hu %02hhu:%02hhu:00", tblTime.month, tblTime.day,
                        tblTime.year + 2000U, tblTime.hours, tblTime.minutes );

         DST_ConvertLocaltoUTC( &sTime );                                     /* Convert meter local time to UTC time  */
         TIME_UTIL_ConvertSysFormatToSeconds( &sTime, &seconds, &fracSecs);
         (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &mtrTime );    /* convert to date format  */
         DBG_logPrintf( 'R', "block end   UTC   date = %02hhu/%02hhu/%4hu %02hhu:%02hhu:00 (0x%08lX)", mtrTime.month, mtrTime.day, mtrTime.year,
                        mtrTime.hour, mtrTime.min, seconds );

         /* Print block start times in local and UTC */
         /* Compute and display block start time  */
         startTime = TIME_UTIL_ConvertSysFormatToSysCombined( &sTime );       /* Convert block end time (UTC) to seconds since epoch   */
         if ( block != tbl63.lastBlockElement )
         {
            /* Start time of first block = end time - ( number of intervals * number of minutes per block * TIME_TICKS_PER_MIN ).   */
            startTime -= ( tbl61.lpDataSet1.nbrBlksIntsSet * tbl61.lpDataSet1.nbrMaxIntTimeSet * TIME_TICKS_PER_MIN );
         }
         else
         {
            /* Start time of first block = end time - ( number of intervals in this block * number of minutes per block * TIME_TICKS_PER_MIN ).   */
            startTime -= ( tbl63.nbrValidInt * tbl61.lpDataSet1.nbrMaxIntTimeSet * TIME_TICKS_PER_MIN );

         }

         TIME_UTIL_ConvertSysCombinedToSysFormat( &startTime, &sTime );
         (void)TIME_UTIL_ConvertSysFormatToMtrHighFormat( &sTime, &tblTime ); /* convert start time to meter format (local)   */
         (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &mtrTime );    /* convert to date format (UTC)  */
         TIME_UTIL_ConvertSysCombinedToSeconds( &startTime, &seconds, &fracSecs);
         DBG_logPrintf( 'R', "block start local date = %02hhu/%02hhu/%4hu %02hhu:%02hhu:00", tblTime.month, tblTime.day,
                        tblTime.year + 2000U, tblTime.hours, tblTime.minutes );

         DST_ConvertLocaltoUTC( &sTime );                                     /* Convert meter system time to UTC time  */
         TIME_UTIL_ConvertSysFormatToSeconds( &sTime, &seconds, &fracSecs);
         DBG_logPrintf( 'R', "block start UTC   date = %02hhu/%02hhu/%4hu %02hhu:%02hhu:00 (0x%08lX)", mtrTime.month, mtrTime.day, mtrTime.year,
                        mtrTime.hour, mtrTime.min, seconds );
      }
      offset += blockSize;
   }
   return 0;
}
#endif //( LP_IN_METER != 0 )

/***********************************************************************************************************************
   Function Name: DBG_CommandLine_LED

   Purpose: Turn LEDs Off or On and set drive state to High or Low.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
uint32_t DBG_CommandLine_LED( uint32_t argc, char *argv[] )
{
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_A ) /* Are we running on the RA6E1 NIC for the I-210+c? */
   char *pHelp = "Usage: LED {on | off | 0 | 1 }";
   if ( 2 != argc )
   {
      DBG_printf( "%s", pHelp );
   }
   else if ( ( 0 == strcasecmp( argv[1], "on"  ) ) || ( 0 == strcmp( argv[1], "1" ) ) )
   {
      TEST_LED_TACKON_ON;
   }
   else if ( ( 0 == strcasecmp( argv[1], "off" ) ) || ( 0 == strcmp( argv[1], "0" ) ) )
   {
      TEST_LED_TACKON_OFF;
   }
   else
   {
      DBG_printf( "%s", pHelp );
   }
   return ( 0 );
}
#else
   uint8_t bHi    = false;
   uint8_t bOn    = false;
   uint8_t bValid = true;
   uint8_t uLed;
   uint8_t string[VER_HW_STR_LEN];   /* Version string including the two '.' and a NULL */

   ( void )VER_getHardwareVersion ( &string[0], sizeof(string) );

   /* Rev C HW does not support LED functionality */
   if ( 'C' == string[0] )
   {
      DBG_printf( "This is Rev C HW, LED operations are not available" );

      return( 0 );
   }

   if ( ( 3 == argc ) || ( argc == 4 ) )
   {


      /* set LED's to manual control */
      LED_enableManualControl();


      if ( ( 0 == strcmp( argv[1], "0" ) ) || ( 0 == strcmp( argv[1], "1" ) ) || ( 0 == strcmp( argv[1], "2" ) ) )
      {
         uLed = ( uint8_t )( argv[1][0] - '0' );
      }
      else
      {
         bValid = false;
      }

      if ( ( 0 == strcasecmp( argv[2], "on" ) ) || ( 0 == strcmp( argv[2], "1" ) ) )
      {
         bOn = true;
      }
      else if ( ( 0 == strcasecmp( argv[2], "off" ) ) || ( 0 == strcmp( argv[2], "0" ) ) )
      {
         bOn = false;
      }
      else
      {
         bValid = false;
      }

      if ( argc == 4 )
      {
         if ( ( 0 == strcasecmp( argv[3], "hi" ) ) || ( 0 == strcmp( argv[3], "1" ) ) )
         {
            bHi = true;
         }
         else if ( ( 0 == strcasecmp( argv[3], "lo" ) ) || ( 0 == strcmp( argv[3], "0" ) ) )
         {
            bHi = false;
         }
         else
         {
            bValid = false;
         }
      }

      if ( bValid )
      {
         if ( argc == 4 )
         {
            DBG_printf( "%s %s %s %s", argv[0], argv[1], argv[2], argv[3] );
         }
         else
         {
            DBG_printf( "%s %s %s", argv[0], argv[1], argv[2] );
         }

         switch( uLed ) /*lint !e644 if bValid, uLed is initialized  */
         {
            case 0:
            {
               if ( true == bOn )
               {
                  if ( true == bHi )
                  {
                     GRN_LED_PIN_DRV_HIGH();
                  }
                  else
                  {
                     GRN_LED_PIN_DRV_LOW();
                  }
                  GRN_LED_ON();
               }
               else
               {
                  GRN_LED_PIN_TRIS();
               }
               break;
            }
            case 1:
            {
               if ( true == bOn )
               {
                  if ( true == bHi )
                  {
                     RED_LED_PIN_DRV_HIGH();
                  }
                  else
                  {
                     RED_LED_PIN_DRV_LOW();
                  }
                  RED_LED_ON();
               }
               else
               {
                  RED_LED_PIN_TRIS();
               }
               break;
            }
            case 2:
            {
               if ( true == bOn )
               {
                  if ( true == bHi )
                  {
                     BLU_LED_PIN_DRV_HIGH();
                  }
                  else
                  {
                     BLU_LED_PIN_DRV_LOW();
                  }
                  BLU_LED_ON();
               }
               else
               {
                  BLU_LED_PIN_TRIS();
               }
               break;
            }
            default:
            {
               GRN_LED_OFF();
               RED_LED_OFF();
               BLU_LED_OFF();
               break;
            }
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   return( 0 );
}
#endif // ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_A )
#endif // (EP == 1)

#if (EP == 1)
/*******************************************************************************

   Function name: DBG_CommandLine_getDstHash

   Purpose: Prints the DST hash

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_getDstHash ( uint32_t argc, char *argv[] )
{
   uint32_t hash;
   DST_getTimeZoneDSTHash( &hash );
   DBG_logPrintf( 'R', "DST Hash = %lu", hash );
   return( 0 );
}

/*******************************************************************************

   Function name: DBG_CommandLine_setTimezoneOffset

   Purpose: Updates the timezone offset

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_setTimezoneOffset ( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;

   if ( 2 == argc )
   {
      int32_t offset =  ( int32_t )atol( argv[1] );
      retVal = DST_setTimeZoneOffset( offset );
      if ( eSUCCESS == retVal )
      {
         DBG_logPrintf( 'R', "Wrote timezone offset = %lu", offset );
      }
      else
      {
         DBG_logPrintf( 'R', "Error writing timezone offset. Error code %u", ( uint32_t )retVal );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
   return ( uint32_t )retVal;
}

/*******************************************************************************

   Function name: DBG_CommandLine_getTimezoneOffset

   Purpose: Prints the timezone offset

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_getTimezoneOffset ( uint32_t argc, char *argv[] )
{
   int32_t offset;
   DST_getTimeZoneOffset( &offset );
   DBG_logPrintf( 'R', "Timezone offset = %l", offset );
   return ( 0 );
}

/*******************************************************************************

   Function name: DBG_CommandLine_setDstEnable

   Purpose: Updates the DST enable

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_setDstEnable ( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;

   if ( 2 == argc )
   {
      int8_t dstEnable =  ( int8_t )atoi( argv[1] );
      retVal = DST_setDstEnable( dstEnable );
      if ( eSUCCESS == retVal )
      {
         DBG_logPrintf( 'R', "Wrote DST enable = %i", dstEnable );
      }
      else
      {
         DBG_logPrintf( 'R', "Error DST enable. Error code %u", ( uint32_t )retVal );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
   return ( uint32_t )retVal;
}

/*******************************************************************************
   Function name: DBG_CommandLine_getDstEnable

   Purpose: Prints the DST Enable

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_getDstEnable ( uint32_t argc, char *argv[] )
{
   int8_t dstEnable;
   DST_getDstEnable( &dstEnable );
   DBG_logPrintf( 'R', "DST enable = %i", dstEnable );
   return ( 0 );
}

/*******************************************************************************
   Function name: DBG_CommandLine_setDstOffset

   Purpose: Updates the DST offset

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_setDstOffset ( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;

   if ( 2 == argc )
   {
      int16_t offset =  ( int16_t )atoi( argv[1] );
      retVal = DST_setDstOffset( offset );
      if ( eSUCCESS == retVal )
      {
         DBG_logPrintf( 'R', "Wrote DST offset = %i", offset );
      }
      else
      {
         DBG_logPrintf( 'R', "Error writing DST offset. Error code %u", ( uint32_t )retVal );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
   return ( uint32_t )retVal;
}

/*******************************************************************************
   Function name: DBG_CommandLine_getDstOffset

   Purpose: Prints the DST offset

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_getDstOffset ( uint32_t argc, char *argv[] )
{
   int16_t offset;
   DST_getDstOffset( &offset );
   DBG_logPrintf( 'R', "DST offset = %i", offset );
   return ( 0 );
}

/*******************************************************************************
   Function name: DBG_CommandLine_setDstStartRule

   Purpose: Updates the DST start rule

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_setDstStartRule ( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;

   if ( 6 == argc )
   {
      uint8_t month     = ( uint8_t )atoi( argv[1] );
      uint8_t dayOfWeek = ( uint8_t )atoi( argv[2] );
      uint8_t occOfDay  = ( uint8_t )atoi( argv[3] );
      uint8_t hour      = ( uint8_t )atoi( argv[4] );
      uint8_t minute    = ( uint8_t )atoi( argv[5] );

      retVal = DST_setDstStartRule( month, dayOfWeek, occOfDay, hour, minute );
      if ( eSUCCESS == retVal )
      {
         DBG_logPrintf( 'R', "Wrote DST start rule: month=%u, dayOfWeek=%u, occOfDay=%u, hour=%u, minute=%u",
                        month, dayOfWeek, occOfDay, hour, minute );
      }
      else
      {
         DBG_logPrintf( 'R', "Error writing DST start rule. Error code %u", ( uint32_t )retVal );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
   return ( uint32_t )retVal;
}

/*******************************************************************************
   Function name: DBG_CommandLine_getDstStartRule

   Purpose: Prints the DST start rule

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_getDstStartRule ( uint32_t argc, char *argv[] )
{
   uint8_t month;
   uint8_t dayOfWeek;
   uint8_t occOfDay;
   uint8_t hour;
   uint8_t minute;

   DST_getDstStartRule( &month, &dayOfWeek, &occOfDay, &hour, &minute );
   DBG_logPrintf( 'R', "DST start rule: month=%u, dayOfWeek=%u, occOfDay=%u, hour=%u, minute=%u",
                  month, dayOfWeek, occOfDay, hour, minute );
   return ( 0 );
}

/*******************************************************************************
   Function name: DBG_CommandLine_setDstStopRule

   Purpose: Updates the DST stop rule

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_setDstStopRule ( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;

   if ( 6 == argc )
   {
      uint8_t month     = ( uint8_t )atoi( argv[1] );
      uint8_t dayOfWeek = ( uint8_t )atoi( argv[2] );
      uint8_t occOfDay  = ( uint8_t )atoi( argv[3] );
      uint8_t hour      = ( uint8_t )atoi( argv[4] );
      uint8_t minute    = ( uint8_t )atoi( argv[5] );

      retVal = DST_setDstStopRule( month, dayOfWeek, occOfDay, hour, minute );
      if ( eSUCCESS == retVal )
      {
         DBG_logPrintf( 'R', "Wrote DST stop rule: month=%u, dayOfWeek=%u, occOfDay=%u, hour=%u, minute=%u",
                        month, dayOfWeek, occOfDay, hour, minute );
      }
      else
      {
         DBG_logPrintf( 'R', "Error writing DST stop rule. Error code %u", ( uint32_t )retVal );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }
   return ( uint32_t )retVal;
}

/*******************************************************************************
   Function name: DBG_CommandLine_getDstStopRule

   Purpose: Prints the DST stop rule

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_getDstStopRule ( uint32_t argc, char *argv[] )
{
   uint8_t month;
   uint8_t dayOfWeek;
   uint8_t occOfDay;
   uint8_t hour;
   uint8_t minute;

   DST_getDstStopRule( &month, &dayOfWeek, &occOfDay, &hour, &minute );
   DBG_logPrintf( 'R', "DST stop rule: month=%u, dayOfWeek=%u, occOfDay=%u, hour=%u, minute=%u",
                  month, dayOfWeek, occOfDay, hour, minute );
   return ( 0 );
}

/*******************************************************************************
   Function name: DBG_CommandLine_getDstParams

   Purpose: Prints the DST params

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.

   Notes:
*******************************************************************************/
uint32_t DBG_CommandLine_getDstParams ( uint32_t argc, char *argv[] )
{
   DST_PrintDSTParams();
   return ( 0 );
}
#endif // (EP == 1)

/*******************************************************************************

   Function name: DBG_CommandLine_virgin

   Purpose: Erases NV chip and reset micro. Comes up as a virgin unit.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_virgin ( uint32_t argc, char *argv[] )
{

   /* Pass in argc of 2 to differentiate this call from a command line entry of virginDelay  */
   ( void )DBG_CommandLine_virginDelay ( 2, argv );   /* Erase the signature  */

   RESET();  /* Execute Software Reset. Just erased NV, PWR_SafeReset() not necessary */
}
/*******************************************************************************

   Function name: DBG_CommandLine_virginDelay

   Purpose: Erases signature and continues to run. Allows new code load and automatic virgin on next reboot.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static uint32_t DBG_CommandLine_virginDelay ( uint32_t argc, char *argv[] )
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
         if ( 1 == argc )  /* Invoked from command line directly? */
         {
            DBG_logPrintf( 'D', "Signature erased. Device will be made virgin on next reboot!" );
         }
      }
   }

   return 0;
}

#if ( WRITE_KEY_ALLOWED == 1)
#warning "Don't release with WRITE_KEY_ALLOWED"
static uint32_t DBG_CommandLine_writeKey ( uint32_t argc, char *argv[] )
{
   uint8_t  keyData[ 64 ]; /* Longest possible key */
   uint16_t keyID;
   uint8_t  ECCstatus;

   if ( argc < 3 )
   {
      DBG_logPrintf( 'D', "usage: wkey id data" );
   }
   else
   {
      keyID = ( uint16_t )strtoul( argv[1], NULL, 0 );
      if ( ( keyID < 4 ) || ( keyID > 15 ) )
      {
         DBG_logPrintf( 'D', "keyID must be > 3 and < 16" );
      }
      else
      {
         if ( strlen( argv[ 2 ] ) > 128 )
         {
            DBG_logPrintf( 'D', "keylength must be <= 128" );
         }
         else
         {
            char     *msg = argv[ 2 ];
            uint8_t  *data = keyData;
            uint8_t  keyLen = 0;

            while ( *msg != 0 )                          /* Loop thru input string, converting ASCII to hex.   */
            {
               ( void )sscanf( msg, "%02hhx", data );
               msg += 2;                                 /* Each sscanf consumes 2 input bytes.       */
               keyLen++;
               data++;
            }
            ECCstatus = ecc108e_WriteKey( keyID, keyLen, keyData );
            PrintECC_error( ECCstatus );
         }
      }
   }

   return 0;
}
#endif // ( WRITE_KEY_ALLOWED == 1)

#if ( READ_KEY_ALLOWED == 1 )
#warning "Don't release with READ_KEY_ALLOWED"
static uint32_t DBG_CommandLine_readKey ( uint32_t argc, char *argv[] )
{
   uint8_t  keyData[ 64 ]; /* Longest possible key */
   uint16_t keyID;
   uint16_t readKeyID;
   uint8_t  keyLen;
   uint8_t  ECCstatus = 1;

   if ( ( argc > 1 ) && ( argc <= 3 ) )
   {
      ( void )memset( keyData, 0, sizeof( keyData ) );
      keyID = ( uint16_t )strtoul( argv[1], NULL, 0 );

      readKeyID = ( uint16_t )strtoul( argv[ 1 ], NULL, 0 );
      if ( ( readKeyID < 4 ) || ( readKeyID > 15 ) )
      {
         DBG_logPrintf( 'D', "readKeyID must be > 3 and < 16" );
      }
      else
      {
         if ( strcasecmp( argv[ 0 ], "rkeyenc" ) == 0 )
         {
            ECCstatus = ecc108e_EncryptedKeyRead( keyID, keyData );
            if ( keyID < FIRST_SIGNATURE_KEY )
            {
               keyLen = ECC108_KEY_SIZE;
            }
            else
            {
               keyLen = VERIFY_256_KEY_SIZE;
            }
         }
         else
         {
            keyLen = ( uint8_t )strtoul( argv[ 2 ], NULL, 0 );
            ECCstatus = ecc108e_ReadKey( keyID, keyLen, keyData );
         }
         if ( ECCstatus == ECC108_SUCCESS )
         {
            DBG_logPrintHex ( 'D', "key: ", keyData, keyLen );
         }
         else
         {
            PrintECC_error( ECCstatus );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'D', "usage: rkey keyID keyLen" );
      DBG_logPrintf( 'D', "usage: rkeyenc keyID readkey" );
   }

   return 0;
}
static uint32_t DBG_CommandLine_dumpKeys ( uint32_t argc, char *argv[] )
{
   uint8_t *key;
#define secROM ((intFlashNvMap_t *)0x7e000)

   key = secROM->pKeys[ 0 ].key;
   DBG_logPrintHex ( 'D', "pKey         4: ", key, 32 );

   key = secROM->replPrivKeys[ 0 ].key;
   DBG_logPrintHex ( 'D', "replPrivKey  4: ", key, 32 );

   key = secROM->sigKeys[ 4 ].key;
   DBG_logPrintHex ( 'D', "pKey        13: ", key, 32 );

   key = secROM->replSigKeys[ 4 ].key;
   DBG_logPrintHex ( 'D', "replPrivKey 13: ", key, 32 );

   key = secROM->sigKeys[ 6 ].key;
   DBG_logPrintHex ( 'D', "pKey        15: ", key, 32 );

   key = secROM->replSigKeys[ 6 ].key;
   DBG_logPrintHex ( 'D', "replPrivKey 15: ", key, 32 );

   return 0;
}
#endif

#if ( PHASE_DETECTION == 1 )
/*******************************************************************************

   Function name: DBG_CommandLine_PhaseDetectCmd

   Purpose: Phase Detection debug commands

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_PhaseDetectCmd( uint32_t argc, char *argv[] )
{
   return PD_DebugCommandLine ( argc, argv );
}
#endif

#if (EP == 1) && ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
/*******************************************************************************

   Function name: DBG_CommandLine_OnDemandRead

   Purpose: Performs on demand read of the pre-selected values.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_OnDemandRead ( uint32_t argc, char *argv[] )
{
   HEEP_APPHDR_t heepHdr;
   union TimeSchRdgQty_u qtys;      // For Time Qty and Reading Qty
   uint16_t dummy=0;

   qtys.TimeSchRdgQty = 0; //No quantities, send default
   heepHdr.Method_Status = ( uint8_t )method_get;
   OR_MR_MsgHandler( &heepHdr, (uint8_t *)&qtys, dummy );
   return ( 0 );
}
#endif

#if ( OVERRIDE_TEMPERATURE == 1 )
/*******************************************************************************

   Function name: DBG_CommandLine_ManualTemperature

   Purpose: This function will temporarily override the current processor Temperature
            If temperature is less than 255 the measured temperature is overridden by this setting
            This is a volatile setting and will be disabled over a reset

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_ManualTemperature ( uint32_t argc, char *argv[] )
{
   char           floatStr[PRINT_FLOAT_SIZE];
   returnStatus_t retVal = eSUCCESS;

   if ( 2 == argc )
   {
      int16_t newTemperature = atoi( argv[1] );

      ADC_Set_Man_Temperature( (float)newTemperature );
   }
   DBG_logPrintf( 'R', "Manual Temperature: %8sC", DBG_printFloat(floatStr, ADC_Get_Man_Temperature(  ),  1) );
   return ( uint32_t )retVal;
}
#endif

/*******************************************************************************

   Function name: DBG_CommandLine_GetHWInfo

   Purpose: This function will print out the current processor Temperature

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_GetHWInfo ( uint32_t argc, char *argv[] )
{
   char           floatStr[PRINT_FLOAT_SIZE];
   int32_t        temperatureF, temperatureC;
   PHY_GetConf_t  GetConf;
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
   float          volts;
   int32_t        dBm;
#endif

   //NOTE: All conversion scaled up by 10 to get 0.1 degree resolution and avoid a floating point printf
   if ( argc != 0 )  /* Called by user command  */
   {
      GetConf = PHY_GetRequest( ePhyAttr_Temperature );
   }
   else  /* Assume called by PHY_TestMode - can't get radio temperature.   */
   {
      GetConf.eStatus = ePHY_GET_SUCCESS;
   }
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      DBG_logPrintf( 'R', "Functnl Rev = %8sV", DBG_printFloat(floatStr, ADC_GetHWRevVoltage(), 3) );
      DBG_logPrintf( 'R', "RevLtr      =   %c", ADC_GetHWRevLetter() );
      if ( argc != 0 )
      {
         temperatureC = (int32_t)( 10 * GetConf.val.Temperature );
         temperatureF = (int32_t)( (float)temperatureC * 9 / 5 + 320.5 ); //Add 320.5 since value is x10 and round up to 0.1
         //The space between the "%" and "4" is actually a flag that specifies the plus sign (+) is "invisible" so positicve and negative numbers are justified the same
         DBG_logPrintf( 'R', "Radio Temp  = % 4d.%1dF, % 4d.%1dC", temperatureF/10, temperatureF%10, temperatureC/10, temperatureC%10 );
      }

#if ( MCU_SELECTED == NXP_K24 )
   temperatureC = (int32_t)( 10 * ADC_Get_uP_Temperature( TEMP_IN_DEG_C ) );
   temperatureF = (int32_t)( (float)temperatureC * 9 / 5 + 320.5 ); //Add 320.5 since value is x10 and round up to 0.1
   DBG_logPrintf( 'R', "CPU Temp    = % 4d.%1dF, % 4d.%1dC", temperatureF/10, temperatureF%10, temperatureC/10, temperatureC%10 );
#elif ( MCU_SELECTED == RA6E1 )
   int16_t        Temperature;
   bool tempOK = RADIO_Get_Chip_Temperature( (uint8_t) RADIO_0, &Temperature );
   if( tempOK )
   {
      temperatureC = (int32_t)( 10 * Temperature );
      temperatureF = (int32_t)( (float)temperatureC * 9 / 5 + 320.5 ); //Add 320.5 since value is x10 and round up to 0.1
      DBG_logPrintf( 'R', "Radio Temp  = % 4d.%1dF, % 4d.%1dC", temperatureF/10, temperatureF%10, temperatureC/10, temperatureC%10 );
   }
   else
   {
      DBG_logPrintf( 'E', "Problem in RADIO_Get_Chip_Temperature");
   }
#endif

#if ( EP == 1 )
      DBG_logPrintf( 'R', "SuperCap    = %-6sV", DBG_printFloat( floatStr, ADC_Get_SC_Voltage(), 3 ) );
      DBG_logPrintf( 'R', "Vin         = %-6sV", DBG_printFloat( floatStr, ADC_Get_4V0_Voltage(), 3 ) );
#if ( MCU_SELECTED == RA6E1 )
      uint8_t string[VER_HW_STR_LEN];
      ( void )VER_getHardwareVersion ( &string[0], sizeof(string) );
      DBG_logPrintf( 'R', "%s %s", VER_getComDeviceType(), &string[0] ); /* No harm in providing a little more information */
#endif
#elif ( DCU == 1 )
      temperatureC = (int32_t)( 10 * ADC_Get_PS_Temperature( TEMP_IN_DEG_C ) );
      temperatureF = (int32_t)( (float)temperatureC * 9 / 5 + 320.5 ); //Add 320.5 since value is x10 and round up to 0.1
      DBG_logPrintf( 'R', "Pwr Sply T  = % 4d.%1dF, % 4d.%1dC", temperatureF/10, temperatureF%10, temperatureC/10, temperatureC%10 );
      temperatureC = (int32_t)( 10 * PHY_VirtualTemp_Get() );
      temperatureF = (int32_t)( (float)temperatureC * 9 / 5 + 320.5 ); //Add 320.5 since value is x10 and round up to 0.1
      DBG_logPrintf( 'R', "vTemp       = % 4d.%1dF, % 4d.%1dC", temperatureF/10, temperatureF%10, temperatureC/10, temperatureC%10 );
      DBG_logPrintf( 'R', "+12VA       = %8sV", DBG_printFloat(floatStr, ADC_Get_12VA_Monitor(), 3) );
      temperatureC = (int32_t)( 10 * ADC_Get_PA_Temperature( TEMP_IN_DEG_C ) );
      temperatureF = (int32_t)( (float)temperatureC * 9 / 5 + 320.5 ); //Add 320.5 since value is x10 and round up to 0.1
      DBG_logPrintf( 'R', "Pwr Amp T   = % 4d.%1dF, % 4d.%1dC", temperatureF/10, temperatureF%10, temperatureC/10, temperatureC%10 );
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )

      volts = ADC_Get_FEM_V_Forward();    /* Saved as volts */
      dBm   = (int32_t)( BSP_VFWD_to_DBM( volts ) * 10 );
      DBG_logPrintf( 'R', "FEM V Fwd   = %8sV, %2d.%1ddBm", DBG_printFloat( floatStr, volts, 3 ), dBm/10, dBm%10 );

      volts = ADC_Get_FEM_V_Reflected();  /* Saved as volts */
      DBG_logPrintf( 'R', "FEM V Refl  = %8sV", DBG_printFloat( floatStr, volts, 3 ) );

      volts = ADC_Get_VSWR() + 0.5;
      DBG_logPrintf( 'R', "FEM VSWR    = %8s:1", DBG_printFloat( floatStr, volts, 3 ) );
#if 1
      if ( argc != 0 )
      {
         GetConf = PHY_GetRequest( ePhyAttr_fngForwardPowerSet );
         dBm = BSP_dBm_Out_to_PaDAC( (float)GetConf.val.fngForwardPowerSet / 10.0f );
         DBG_logPrintf( 'R', "PA DAC(out) = %7d (computed)", dBm );
      }
#endif
#endif
#endif
   }
   return ( 0 );
} /* end DBG_CommandLine_GetHWInfo () */

/*******************************************************************************

   Function name: DBG_CommandLine_PaDAC

   Purpose: This function will print out the value in the DAC ouput register for the RF Power Amplifier

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes: In an engineering build, the function will also let you set the DAC register value

*******************************************************************************/
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
uint32_t DBG_CommandLine_PaDAC( uint32_t argc, char *argv[] )
{
   char    floatStr[PRINT_FLOAT_SIZE];

   if ( argc == 1) // No Parameters
   {
      // NOTE: All conversion scaled up by 10 to get 0.1 degree resolution and avoid a floating point printf
      DBG_printf( "%s %sdBm", argv[0], DBG_printFloat( floatStr, BSP_PaDAC_Get_dBm_Out(), 6 ) );
   }
#if ( ENGINEERING_BUILD == 1 )
   else if ( argc == 2 )
   {
#if 0  //Raw register input
      uint16_t output = (uint16_t)atoi( argv[1] );

      returnStatus_t retVal = BSP_PaDAC_SetValue(output); //<0xfff or 4095
      dBm = (int32_t)(10 * BSP_PaDAC_Get_dBm_Out());
      DBG_printf( "%s %d.%1ddBm", argv[0], dBm/10, dBm%10 );

      if ( retVal != eSUCCESS )
      {
         DBG_logPrintf( 'R', "ERROR - Invalid range. Must be 0-4095" );
      }
#else  //Float dBm input
      float output = (float)atof( argv[1] );

      if ( eSUCCESS == BSP_PaDAC_Set_dBm_Out( output ) )
      {
         DBG_printf( "%s %sdBm", argv[0], DBG_printFloat( floatStr, BSP_PaDAC_Get_dBm_Out(), 6 ) );
      }
      else
      {
         DBG_logPrintf( 'R', "ERROR - Invalid range. Must be 0.0, or 28.0 to 36.0" );
      }
#endif
   }
#endif
   else
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }
   return ( 0 );
}
#endif //( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )

/*******************************************************************************

   Function name: DBG_CommandLine_PaDACCoeff

   Purpose: This function will print out the value in the DAC ouput register for the RF Power Amplifier

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes: In an engineering build, the function will also let you set the DAC register value

*******************************************************************************/
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A ) && ( ENGINEERING_BUILD == 1 ) )
uint32_t DBG_CommandLine_PaCoeff( uint32_t argc, char *argv[] )
{
#define EXPONENT_STR_NONE ""
#define EXPONENT_STR_EM6  "E-6"
   uint8_t        type    = 0;
   uint8_t        device  = 0;
   char           *expStr = EXPONENT_STR_NONE;
   float          A;
   char           floatStrA[PRINT_FLOAT_SIZE];
   float          B;
   char           floatStrB[PRINT_FLOAT_SIZE];
   float          C;
   char           floatStrC[PRINT_FLOAT_SIZE];

   if ( !strcasecmp( argv[0], "PaDBMtoDAC" ) )
   {
      type = 1;
   }
   else if ( !strcasecmp( argv[0], "PaVFWDtoDBM" ) )
   {
      type = 2;
   }
   if ( argc == 1 )     // Print all devices
   {
      while ( eSUCCESS == BSP_PaDAC_GetCoeff( type, device, &A, &B, &C ) )
      {
         DBG_printf( "%s %d %s%s %s %s", argv[0], device, DBG_printFloat( floatStrA, A, 6 ), expStr,
                                                          DBG_printFloat( floatStrB, B, 6 ),
                                                          DBG_printFloat( floatStrC, C, 6 ) );
         device++;
      }
   }
   else if ( argc == 2 )   // Print specific device only
   {
      device = (uint8_t)atoi( argv[1] );
      if ( eSUCCESS == BSP_PaDAC_GetCoeff( type, device, &A, &B, &C ) )
      {
         if ( 1 == type )
         {
            A *= 1.0e6;
            expStr = EXPONENT_STR_EM6;
         }
         DBG_printf( "%s %d %s%s %s %s", argv[0], device, DBG_printFloat( floatStrA, A, 6 ), expStr,
                                                            DBG_printFloat( floatStrB, B, 6 ),
                                                            DBG_printFloat( floatStrC, C, 6 ) );
      }
      else
      {
         DBG_printf( "%d is an invalid device", device );
      }
   }
   else if ( argc == 5)    // Update and print specific device
   {
      float   a;
      float   b;
      float   c;

      device = (uint8_t)atoi( argv[1] );
      a      = (float)atof( argv[2] );
      b      = (float)atof( argv[3] );
      c      = (float)atof( argv[4] );

      if ( eSUCCESS == BSP_PaDAC_SetCoeff(type, device, a, b, c) )
      {
         BSP_PaDAC_GetCoeff( type, device, &A, &B, &C );
         if ( 1 == type )
         {
            A *= 1.0e6;
            expStr = EXPONENT_STR_EM6;
         }
         DBG_printf( "%s %d %s%s %s %s", argv[0], device, DBG_printFloat( floatStrA, A, 6 ), expStr,
                                                            DBG_printFloat( floatStrB, B, 6 ),
                                                            DBG_printFloat( floatStrC, C, 6 ) );
      }
      else
      {
         DBG_logPrintf( 'R', "ERROR - Invalid range" );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "ERROR - invalid arguments. Rqd: device a b c" );
   }
   return ( 0 );
}
#endif //( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A ) && ( ENGINEERING_BUILD == 1 )

/*******************************************************************************

   Function name: DBG_CommandLine_VSWR

   Purpose: This function will print out the measure Voltage Standing Wave Ratio

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
uint32_t DBG_CommandLine_VSWR( uint32_t argc, char *argv[] )
{
   char floatStr[PRINT_FLOAT_SIZE];

   if ( argc == 1) // No Parameters
   {
      DBG_printf( "%s %-6s", argv[0], DBG_printFloat( floatStr, ADC_Get_VSWR(), 3 ) );
   }
   else
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   return ( 0 );
}
#endif //( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )

#if (EP == 1)
/*******************************************************************************

   Function name: printMeterQty

   Purpose:

   Arguments:

   Returns:

   Notes:

*******************************************************************************/
static void printMeterQty( int64_t val, uint8_t pow10 )
{
   double   vald;
   uint64_t integer;       /* Integer portion of val              */
   uint32_t len;
   uint32_t power10;       /* pow10 converted to 10 ^ pow10       */
   uint32_t logVal;        /* (int)log( val )                     */
   char     outInt[21];


   if ( pow10 == 7 )    /* Special value; means 10^-9 */
   {
      pow10 = 9;
   }
   DBG_logPrintf( 'D', "Raw data: 0x%llx", val );
   power10 = ( uint32_t )pow( ( double )10, ( double )pow10 );
   integer = ( uint64_t )( ( double )val / power10 );
   len  = ( uint32_t )sprintf( outInt, "%llu.", integer );
   logVal = ( uint32_t )log10( ( double )val ) + 1;
   if( logVal >= pow10 )
   {
      len = logVal - ( len - 1 );   /* Take the '.' out of the length   */
   }
   else
   {
      len = pow10;   /* All the digits are after the decimal point. Need leading zeros */
   }
   /* Find the fractional portion of val  */
   vald = ( double )val;
   val = ( int64_t )( vald - ( ( double )integer * power10 ) );
   DBG_logPrintf( 'D', "%s%0*llu", outInt, len, val );
}
/*******************************************************************************

   Function name: DBG_CommandLine_HmcCurrent

   Purpose: This function will print out a quantity from the HMC CIM interface module

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_HmcCurrent ( uint32_t argc, char *argv[] )
{
   ValueInfo_t  readValue;

   enum_CIM_QualityCode rdgStatus;

   if ( argc == 2 )  /* The number of arguments must be 2 */
   {
      uint16_t reading = ( uint16_t )strtoul( argv[1], NULL, 0 );
      rdgStatus = INTF_CIM_CMD_getMeterReading( (meterReadingType)reading, &readValue );
      if ( CIM_QUALCODE_SUCCESS == rdgStatus )
      {
         printMeterQty( readValue.Value.intValue, readValue.power10 );
      }
      else
      {
         DBG_logPrintf( 'D', "%s failed: %d", argv[ 0 ], ( int32_t )rdgStatus );
      }
#if (ACLARA_LC == 0 ) && (ACLARA_DA == 0)
      uint8_t powerOfTen = HMC_MTRINFO_getMeterReadingPowerOfTen( (meterReadingType)reading );
      DBG_logPrintf( 'D', "Power Of Ten: %d", powerOfTen );
#endif
   }
   return ( 0 );
}
 /* end DBG_CommandLine_HmcCurrent() */
#endif

#if (EP == 1)
/*******************************************************************************

   Function name: DBG_CommandLine_HmcHist

   Purpose: This function will print out a quantity from the HMC CIM interface module, this is the shifted value

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_HmcHist ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )  /* The number of arguments must be 2 */
   {
      ValueInfo_t           readingInfo;
      sysTime_dateFormat_t  DateTime;      /* Converted date/time of associated value   */
      enum_CIM_QualityCode  resp;
      uint16_t              reading;

      reading = ( uint16_t )strtoul( argv[1], NULL, 0 );
      resp = INTF_CIM_CMD_getMeterReading( ( meterReadingType )reading, &readingInfo );
      if ( CIM_QUALCODE_SUCCESS == resp )
      {
         DBG_logPrintf( 'R', "HMC Shifted: Val: %lld", readingInfo.Value.intValue );
         printMeterQty( readingInfo.Value.intValue, readingInfo.power10 );
         if ( readingInfo.cimInfo.timePresent != 0 )
         {
            ( void )TIME_UTIL_ConvertSysFormatToDateFormat( ( sysTime_t * )&readingInfo.timeStamp, &DateTime );
            DBG_logPrintf( 'R', "Shifted time: %02d/%02d/%04d %02d:%02d",
                           DateTime.month, DateTime.day, DateTime.year, DateTime.hour, DateTime.min );
         }
      }
      else
      {
         DBG_logPrintf( 'R', "HMC Shifted Failed - %d", ( int32_t )resp );
      }
   }
   return ( 0 );
} /* end DBG_CommandLine_HmcHist() */
#endif

#if (EP == 1)
/*******************************************************************************

   Function name: DBG_CommandLine_HmcDemand

   Purpose: This function will print out a demand value, peak time, coincidents, demand reset count, and its time.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_HmcDemand ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )  /* The number of arguments must be 1 */
   {
      sysTime_dateFormat_t DateTime;      /* Converted date/time of associated value   */
      ValueInfo_t          readingInfo;
      meterReadingType     RdgType;       /* Demand reading type  */
      enum_CIM_QualityCode respReading;   /* Success/failure of reading request. */
      uint8_t              coincidentCount;

      RdgType = ( meterReadingType )( strtoul( argv[1], NULL, 0 ) ); /* Get demand qty of interest */

      /* Retrieve reading and metadata */
      respReading = INTF_CIM_CMD_getMeterReading( RdgType, &readingInfo );

      if ( CIM_QUALCODE_SUCCESS == respReading )
      {
         printMeterQty( ( int64_t )readingInfo.Value.intValue, readingInfo.power10 );
         if ( readingInfo.cimInfo.timePresent != 0 )
         {
            ( void )TIME_UTIL_ConvertSysFormatToDateFormat( (sysTime_t *)&readingInfo.timeStamp, &DateTime );
            DBG_logPrintf( 'R', "Peak time: %02d/%02d/%04d %02d:%02d",
                           DateTime.month, DateTime.day, DateTime.year, DateTime.hour, DateTime.min );
         }
         DBG_logPrintf( 'R', "No. coin: %d", readingInfo.cimInfo.coincidentCount );
         coincidentCount = readingInfo.cimInfo.coincidentCount;
         for ( uint8_t coinCnt = 0; coinCnt < coincidentCount; coinCnt++ )
         {
            DBG_logPrintf( 'R', "Coin Number: %d", coinCnt );
            respReading = INTF_CIM_CMD_getDemandCoinc( RdgType, coinCnt, &readingInfo );
            if ( CIM_QUALCODE_SUCCESS == respReading )
            {
               DBG_logPrintf( 'R', "coincident reading type: %u", (uint16_t)readingInfo.readingType );
               printMeterQty( ( int64_t )readingInfo.Value.intValue, readingInfo.power10 );
            }
            else
            {
               DBG_logPrintf( 'R', "Failed reading coincident - %d",  ( int32_t )respReading );
            }
         }
      }
      else
      {
         DBG_logPrintf( 'R', "Failed reading demand value - %d",  ( int32_t )respReading );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid parameters" );
   }
   return ( 0 );
} /* end DBG_CommandLine_HmcDemand() */
#if 0
/*******************************************************************************

   Function name: DBG_CommandLine_HmcDemandCoin

   Purpose: This function will print out a coincident quantity from the HMC CIM interface module

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_HmcDemandCoin ( uint32_t argc, char *argv[] )
{
   if ( argc == 3 )  /* The number of arguments must be 2 */
   {
      uint64_t             coinVal;
      meterReadingType     RdgType;
      meterReadingType     coinRdgType;
      uint8_t              pow10;
      uint8_t              whichCoin;
      enum_CIM_QualityCode respReading;

      RdgType = ( meterReadingType )( atoi( argv[1] ) ); /* Get demand qty of interest */

      whichCoin = ( uint8_t )strtoul( argv[2], NULL, 0 );
      respReading = INTF_CIM_CMD_getDemandCoincident( RdgType, whichCoin, &coinVal, &pow10, &coinRdgType );
      if ( CIM_QUALCODE_SUCCESS == respReading )
      {
         DBG_logPrintf( 'R', "coincident reading type: %d", coinRdgType );
         printMeterQty( (int64_t)coinVal, pow10 );
      }
      else
      {
         DBG_logPrintf( 'R', "Failed Reading Current - %d",  respReading );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Failed - Invalid Parameters" );
   }
   return ( 0 );
} /* end DBG_CommandLine_HmcDemandCoin() */
#endif
#endif

/*******************************************************************************

   Function name: DBG_CommandLine_PrintFiles

   Purpose: This function will print out the File Tables from each partition

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_PrintFiles  ( uint32_t argc, char *argv[] )
{
#ifdef TM_PARTITION_USAGE
   if ( argc == 1 )  /* The number of arguments must be 4 */
   {
      FIO_printFileInfo();
   }
   else if ( argc == 2 )  /* The number of arguments must be 4 */
   {
      FIO_printFile( ( filenames_t )atoi( argv[1] ) );
   }
#else
   DBG_logPrintf( 'R', "Error - Macro 'TM_PARTITION_USAGE' must be enabled in CompileSwitch.h" );
#endif
   return ( 0 );
} /* end DBG_CommandLine_PrintFiles () */

/*******************************************************************************

   Function name: DBG_CommandLine_DumpFiles

   Purpose: This function will print out the contents of all the files in un-named partitions

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_DumpFiles  ( uint32_t argc, char *argv[] )
{
#ifdef TM_PARTITION_USAGE
   if ( argc == 1 )
   {
      FIO_fileDump();
   }
#else
   DBG_logPrintf( 'R', "Error - Macro 'TM_PARTITION_USAGE' must be enabled in CompileSwitch.h" );
#endif
   return ( 0 );
}

#if (EP == 1)
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
/*******************************************************************************

   Function name: DBG_CommandLine_HmcCmd

   Purpose: This function will print out the HMC table information

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:   If first argument is 'm' or 'M', use manufacturing tables (id offset by 2048)

*******************************************************************************/
uint32_t DBG_CommandLine_HmcCmd  ( uint32_t argc, char *argv[] )
{
#define HMC_WAIT_TIME   ( 25 )            /* Number of seconds to wait for response */

   char     respDataHex[ ( HMC_CMD_MSG_MAX * 4 ) +                      /* 2 hex digits + space per byte read  */
                         ( 15 * 3 ) +                                   /* up to 15 .. + space  in 1st line */
                         ( ( ( HMC_REQ_MAX_BYTES / 16 ) + 2 ) * 7 ) +   /* leading address and space per line */
                         ( HMC_REQ_MAX_BYTES / 16 ) + 2 + 1 ];          /* \n per line + final '\0' */
   buffer_t *pBuffer;            /* table read data   */
   char     *pPtr;
   uint16_t tableID = 0;
   uint32_t tblOffset = 0;
   uint16_t totalBytes;          /* Running count of bytes read, decreasing   */
   uint16_t bytesRead;           /* Bytes requested each pass. */
   uint8_t  arg;                 /* index into argv[]  */
   uint8_t  i;

   // Allocate a buffer for a HMC data
   pBuffer = BM_alloc( HMC_CMD_MSG_MAX  );
   if ( pBuffer != NULL )
   {
      if ( ( argc == 4 ) || ( argc == 5 ) )  /* The number of arguments must be 4 or 5 */
      {
         arg = 1;
         if ( strcasecmp ( argv[ arg ], "m" ) == 0 )  /* Requesting Manufacturing table; offset ID by 2048. */
         {
            tableID = 2048U;
            arg++;
         }

         /* Retrieve table ID, offset, length   */
         tableID    += ( uint16_t )strtoul( argv[arg++], NULL, 0 );
         tblOffset   = ( uint32_t )strtoul( argv[arg++], NULL, 0 );
         totalBytes  = ( uint16_t )strtoul( argv[arg], NULL, 0 );

         pBuffer->x.dataLen  = HMC_CMD_MSG_MAX;

         pPtr = respDataHex;
         for ( arg = 0; arg < argc; arg++ )
         {
            pPtr += snprintf( pPtr, sizeof( respDataHex ) - ( pPtr - respDataHex ), "%s ", argv[ arg ] );
         }
         DBG_logPrintf( 'D', "%s", respDataHex );

         /* Loop until requested number of bytes have been read, or timed out. */
         while ( totalBytes != 0 )
         {
            bytesRead = min( totalBytes, HMC_CMD_MSG_MAX );
            if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( pBuffer->data, tableID, tblOffset, bytesRead ) )
            {

               DBG_logPrintf( 'R', "table data (hex) @Offset: %p", tblOffset );

               /* Print starting address                                */
               pPtr = respDataHex + snprintf( respDataHex, sizeof( respDataHex ), "%06x ", tblOffset - ( tblOffset % 16U ) );

               if ( ( tblOffset % 16U ) != 0 )                       /* Not starting on hex 10 boundary? */
               {
                  for ( uint8_t pad = tblOffset % 16U; pad; pad-- )  /* Print calclated number of .. */
                  {
                     pPtr += snprintf( pPtr, sizeof( respDataHex ) - ( pPtr - respDataHex ), ".. " );
                  }
               }

               for ( i = 0; i < bytesRead; i++ )
               {
                  if ( ( ( tblOffset % 16U ) == 0 ) && ( i != 0 ) )     /* If on hex 10 boundary, new line  */
                  {
#if ( RTOS_SELECTION == MQX_RTOS )
                     pPtr += snprintf( pPtr, sizeof( respDataHex ) - ( pPtr - respDataHex ), "\n%06x ", tblOffset );
#elif ( RTOS_SELECTION == FREE_RTOS )
                     pPtr += snprintf( pPtr, sizeof( respDataHex ) - ( pPtr - respDataHex ), "\r\n%06x ", tblOffset );
#endif
                  }
                  pPtr += snprintf( pPtr, sizeof( respDataHex ) - ( pPtr - respDataHex ), "%02X ", pBuffer->data[i] );
                  tblOffset++;
               }
               *pPtr = '\0';
               DBG_printf( "       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F" );
               DBG_printf( "%s", &respDataHex[0] );
            }
            else
            {
               DBG_logPrintf( 'R', "ANSI read failed" );
               break;   /* Exit loop on failure.  */
            }
            totalBytes -= bytesRead;
         }
      }
      else
      {
         DBG_logPrintf( 'R', "Invalid Syntax" );
      }
      BM_free( pBuffer );
   }

   return ( 0 );
} /* end DBG_CommandLine_HmcCmd() */
/*******************************************************************************

   Function name: DBG_CommandLine_HmcEng

   Purpose: This function will print out the hmc engineering stats.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_HmcEng ( uint32_t argc, char *argv[] )
{
  mtrEngFileData_t fileData;
  returnStatus_t retVal = eSUCCESS;
  uint32_t  numSessions = 0;
  retVal = HMC_ENG_getHmgEngStats( (void *) &fileData, sizeof(fileData) );

  if( retVal == eSUCCESS )
  {
    numSessions = (uint32_t) fileData.perfSum.sessions[0]; //number of sessions for Port0
    DBG_logPrintf( 'R', "meter comm lockout count %d:", fileData.meterCommunicationLockoutCount);
    DBG_logPrintf( 'R', "meter session failure count: %d", fileData.meterSessionFailureCount);
    DBG_logPrintf( 'R', "port 0 error: %d", fileData.uErrBits.Bits.Port0Err);
    DBG_logPrintf( 'R', "password error: %d", fileData.uErrBits.Bits.ISCErr);
    DBG_logPrintf( 'R', "error count summary:");
    DBG_logPrintf( 'R', "   ae:    0x%02x", fileData.errCntSum.ae);
    DBG_logPrintf( 'R', "   ansie: 0x%02x", fileData.errCntSum.ansie);
    DBG_logPrintf( 'R', "   ato:   0x%02x", fileData.errCntSum.ato);
    DBG_logPrintf( 'R', "   dle:   0x%02x", fileData.errCntSum.dle);
    DBG_logPrintf( 'R', "   hw:    0x%02x", fileData.errCntSum.hwe);
    DBG_logPrintf( 'R', "   ir:    0x%02x", fileData.errCntSum.ir);
    DBG_logPrintf( 'R', "performance summary");
    DBG_logPrintf( 'R', "   Aborts:     %lu", fileData.perfSum.abort);
    DBG_logPrintf( 'R', "   Packets:    %lu", fileData.perfSum.packets);
    DBG_logPrintf( 'R', "   Sessions:   %lu", numSessions);
    DBG_logPrintf( 'R', "exeception counts");
    DBG_logPrintf( 'R', "   hmb:     0x%02x", fileData.exeCnts.hwb);
    DBG_logPrintf( 'R', "   nakr:    0x%02x", fileData.exeCnts.nakr);
    DBG_logPrintf( 'R', "   nakt:    0x%02x", fileData.exeCnts.nakt);
    DBG_logPrintf( 'R', "   or:      0x%02x", fileData.exeCnts.or);
    DBG_logPrintf( 'R', "   pto:     0x%02x", fileData.exeCnts.pto);
    DBG_logPrintf( 'R', "   retry:   0x%02x", fileData.exeCnts.retry);
    DBG_logPrintf( 'R', "   tog:     0x%02x", fileData.exeCnts.tog);
  }
  return 0;
}
#endif /* ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 ) meter specific code */

/*******************************************************************************

   Function name: DBG_CommandLine_Cmd

   Purpose: This function will write supplied data to a meter table

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:   If first argument is 'm' or 'M', use manufacturing tables (id offset by 2048)

*******************************************************************************/
#if ( ACLARA_LC != 1 ) && ( ACLARA_DA != 1 ) /* meter specific code */
static uint32_t DBG_CommandLine_HmcwCmd  ( uint32_t argc, char *argv[] )
{
   HMC_REQ_queue_t   *req;                /* psem buffer */
   buffer_t          *pBuffer;            /* table read data   */
   buffer_t          *preq;               /* psem command buffer  */
   buffer_t          *writeData;
   char              *pData;              /* User input data      */
   uint16_t          tableID = 0;
   uint16_t          tblOffset = 0;
   uint16_t          bytesLeft;           /* Running count of bytes read, decreasing   */
   uint8_t           arg;                 /* index into argv[]  */

   DBG_logPrintf( 'D', "%s %s", argv[ 0 ], argv[ 1 ] );  /* Use for timing measurement, only */
   // Allocate a buffer for a HMC data
   pBuffer = BM_alloc( HMC_CMD_MSG_MAX  );
   if ( pBuffer != NULL )
   {
      // Allocate a buffer for a HMC command
      preq = BM_alloc( sizeof( HMC_REQ_queue_t ) );
      if ( preq != NULL )
      {
         if ( argc >= 4 )  /* The number of arguments must be at least 4   */
         {
            if ( !HmcCmdSemCreated )
            {
               if ( OS_SEM_Create ( &HMC_CMD_SEM, 10 ) ) /* Assume that up to 10 commands can be pending before HMC completes them */
               {
                  HmcCmdSemCreated = ( bool )true;
               } /* end if() */
            }
            arg = 1;
            if ( strcasecmp ( argv[ arg ], "m" ) == 0 )
            {
               tableID = 2048;
               arg++;
            }
            /* Retrieve table ID, offset, length   */
            tableID += ( uint16_t )strtoul( argv[arg++], NULL, 0 );
            tblOffset = ( uint16_t )strtoul( argv[arg++], NULL, 0 );
            writeData = BM_alloc( ( uint16_t )strlen( argv[ arg ] ) / 2 );
            if ( writeData != NULL )
            {
               for ( bytesLeft = 0, pData = argv[ arg ];
                     ( bytesLeft <  writeData->x.dataLen ) && *pData != 0;
                     pData += 2 )
               {
                  /* sscanf returns number of parameters successfully converted. */
                  bytesLeft += ( uint16_t )sscanf( pData, "%02hhx", &writeData->data[ bytesLeft ] );
               }
               pBuffer->x.dataLen = bytesLeft ;
               ( void )memset( pBuffer->data, 0, pBuffer->x.dataLen );

               preq->x.dataLen = sizeof( HMC_REQ_queue_t );
               ( void )memset( preq->data, 0, preq->x.dataLen );

               req = ( HMC_REQ_queue_t * )( void * )preq->data;

               /* Following fields don't change between requests  */
               req->pSem         = &HMC_CMD_SEM;
               req->bOperation   = eHMC_WRITE;
               req->maxDataLen   = (uint8_t)bytesLeft;
               req->pData        = writeData->data;
               req->tblInfo.id   = tableID;
               req->tblInfo.offset = tblOffset;
               req->tblInfo.cnt = min( bytesLeft, HMC_CMD_MSG_MAX  );
               OS_QUEUE_Enqueue ( &HMC_REQ_queueHandle, req );
               if ( OS_SEM_Pend ( &HMC_CMD_SEM, ONE_SEC * HMC_WAIT_TIME ) )   // Wait for 25s max
               {
                  DBG_logPrintf( 'R', "HMC Status: %i, Tbl Rsp: %i", ( int32_t )req->hmcStatus, ( int32_t )req->tblResp );
               }
               else
               {
                  ( void ) OS_QUEUE_Dequeue ( &HMC_REQ_queueHandle );   /* Attempt to remove "dropped" request from queue */
                  DBG_logPrintf( 'R', "HMC Timed out after %ds!", HMC_WAIT_TIME );
               }
               BM_free( writeData );
            }
         }
         else
         {
            DBG_logPrintf( 'R', "Invalid Syntax" );
         }
         BM_free( preq );
      }
      BM_free( pBuffer );
   }
   return ( 0 );
} /* end DBG_CommandLine_HmcwCmd() */
#endif /* ( ACLARA_LC != 1 ) && ( ACLARA_DA != 1 ) meter specific code */
#endif /* (EP == 1) */

#ifdef TM_ID_TEST_CODE
/*******************************************************************************

   Function name: DBG_CommandLine_IdBufRd

   Purpose: This function will Read data from the interval data test buffer

   Arguments:  argc - Number of Arguments passed to this function
               argv - Pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

 *******************************************************************************/
uint32_t DBG_CommandLine_IdBufRd ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )  /* The number of arguments must be 2 */
   {
      returnStatus_t retVal;
      char buf[20];
      retVal = ID_readTestBuffer( ( uint8_t )( atoi( argv[1] ) ), &buf[0] );
      if ( eSUCCESS == retVal )
      {
         DBG_logPrintf( 'R', "Data: %s", buf );
      }
      else
      {
         DBG_logPrintf( 'R', "Data Read Failed" );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid Syntax" );
   }

   return ( 0 );
} /* end DBG_CommandLine_IdBufRd () */
#endif // TM_ID_TEST_CODE

#ifdef TM_ID_TEST_CODE
/*******************************************************************************

   Function name: DBG_CommandLine_IdBufWr

   Purpose: This function will write data to the interval data test buffer

   Arguments:  argc - Number of Arguments passed to this function
               argv - Pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

 *******************************************************************************/
uint32_t DBG_CommandLine_IdBufWr ( uint32_t argc, char *argv[] )
{
   if ( argc == 3 )  /* The number of arguments must be 3 */
   {
      returnStatus_t retVal;
      char buf[20];
      ( void )snprintf( buf, sizeof( buf ), "%s", argv[2] );
      retVal = ID_writeTestBuffer( ( uint8_t )( atoi( argv[1] ) ), &buf[0] );
      if ( eSUCCESS == retVal )
      {
         DBG_logPrintf( 'R', "Data Written" );
      }
      else
      {
         DBG_logPrintf( 'R', "Data Write Failed" );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid Syntax" );
   }

   return ( 0 );
} /* end DBG_CommandLine_IdBufWr () */
#endif // TM_ID_UNIT_TEST

#ifdef TM_ID_UNIT_TEST
/*******************************************************************************

   Function name: DBG_CommandLine_IdUt

   Purpose: This function will write data to the interval data test buffer

   Arguments:  argc - Number of Arguments passed to this function
               argv - Pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

 *******************************************************************************/
uint32_t DBG_CommandLine_IdUt ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )  /* The number of arguments must be 2 */
   {
      ID_RunUnitTest( ( uint8_t )( atoi( argv[1] ) ) );
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid Syntax" );
   }
   return ( 0 );
} /* end DBG_CommandLine_IdUt () */
#endif // TM_ID_UNIT_TEST

#if (EP == 1)
#if (ACLARA_LC == 0) && ( ACLARA_DA == 0 )
#if ( LP_IN_METER == 0 )
/*******************************************************************************

   Function name: DBG_CommandLine_IdCfg

   Purpose: This function will configure interval data.

   Arguments:  argc - Number of Arguments passed to this function
               argv - Pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_IdCfg  ( uint32_t argc, char *argv[] )
{
   if ( argc == 7 )
   {
      idChCfg_t      chCfg;
      returnStatus_t retVal;

      (void)memset( (uint8_t *)&chCfg, 0, sizeof(chCfg) );
      chCfg.rdgTypeID = ( uint16_t )( atoi( argv[2] ) );
      chCfg.sampleRate_mS = ( uint32_t )( atoi( argv[3] ) ) * TIME_TICKS_PER_MIN;
      chCfg.mode = ( mode_e )( atoi( argv[4] ) );
      chCfg.trimDigits = ( uint8_t )( atoi( argv[5] ) );
      chCfg.storageLen = ( uint8_t )( atoi( argv[6] ) );
      retVal = ID_cfgIntervalData( ( uint8_t )( atoi( argv[1] ) ), &chCfg );
      if ( eSUCCESS == retVal )
      {
         DBG_logPrintf( 'R', "Success!" );
      }
      else if ( eNV_EXTERNAL_NV_ERROR == retVal )
      {
         DBG_logPrintf( 'R', "NV Error!" );
      }
      else if ( eAPP_ID_CFG_SAMPLE == retVal )
      {
         DBG_logPrintf( 'R', "Sample Rate Error!" );
      }
      else if ( eAPP_ID_CFG_PARAM == retVal )
      {
         DBG_logPrintf( 'R', "Config Param Error!" );
      }
      else
      {
         DBG_logPrintf( 'R', "Unknown Error!" );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid Syntax, Ch ReadingType samplePeriod isDeltaData trimDigits storageBytes" );
   }

   return ( 0 );
} /* end DBG_CommandLine_IdCfg () */
#endif // ( LP_IN_METER == 0 )
#endif // (ACLARA_LC == 0) && ( ACLARA_DA == 0 )
#endif // (EP == 1)

#if ENABLE_ID_TASKS
#if ( LP_IN_METER == 0 )
/*******************************************************************************

   Function name: DBG_CommandLine_IdCfgR

   Purpose: This function will read interval data configuration.

   Arguments:  argc - Number of Arguments passed to this function
               argv - Pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

 *******************************************************************************/
uint32_t DBG_CommandLine_IdCfgR  ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )  /* The number of arguments must be 1 */
   {
      returnStatus_t retVal;
      idChCfg_t      chCfg;

      retVal = ID_rdIntervalDataCfg( &chCfg, ( uint8_t )( atoi( argv[1] ) ) );
      if ( eSUCCESS == retVal )
      {
         DBG_logPrintf( 'R', "Cfg: ch = %d, smplRate = %d, store = %d, trimDigits = %d, mode = %d, RTI = %d",
                        ( uint16_t )( atoi( argv[1] ) ),
                        chCfg.sampleRate_mS / TIME_TICKS_PER_MIN,
                        chCfg.storageLen,
                        chCfg.trimDigits,
                        ( uint8_t )chCfg.mode,
                        chCfg.rdgTypeID );
      }
      else if ( eAPP_ID_CFG_CH == retVal )
      {
         DBG_logPrintf( 'R', "Invalid Channel Error!" );
      }
      else if ( eNV_EXTERNAL_NV_ERROR == retVal )
      {
         DBG_logPrintf( 'R', "NV Error!" );
      }
      else
      {
         DBG_logPrintf( 'R', "Unknown Error!" );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid Syntax" );
   }

   return ( 0 );
} /* end DBG_CommandLine_IdCfgR () */
#endif // ( LP_IN_METER == 0 )
#endif // ENABLE_ID_TASKS

#if ENABLE_ID_TASKS
#if ( LP_IN_METER == 0 )
/*******************************************************************************

   Function name: DBG_CommandLine_IdRd

   Purpose: This function will read interval data at a specific time/date.

   Arguments:  argc - Number of Arguments passed to this function
               argv - Pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

 *******************************************************************************/
uint32_t DBG_CommandLine_IdRd  ( uint32_t argc, char *argv[] )
{
   char floatStr[PRINT_FLOAT_SIZE];

   if ( argc == 7 )  /* The number of arguments must be 7 */
   {
      returnStatus_t retVal;
      uint8_t          qualCodes;
      double        reading;

      sysTime_t sysTime;
      sysTime_dateFormat_t rtcTime;
      rtcTime.year = ( uint16_t )( atoi( argv[2] ) + 2000 ); /* Adjust the 2 digit date to 20xx */
      rtcTime.month = ( uint8_t )( atoi( argv[3] ) );
      rtcTime.day = ( uint8_t )( atoi( argv[4] ) );
      rtcTime.hour = ( uint8_t )( atoi( argv[5] ) );
      rtcTime.min = ( uint8_t )( atoi( argv[6] ) );
      rtcTime.sec = 0;

      if ( eSUCCESS == TIME_UTIL_ConvertDateFormatToSysFormat( &rtcTime, &sysTime ) )
      {
         retVal = ID_rdIntervalData( &qualCodes, &reading, ( uint8_t )( atoi( argv[1] ) ), &sysTime );
         if ( eSUCCESS == retVal )
         {
            DBG_logPrintf( 'R', "ID Val = %s, Quality Code = 0x%02X",
                           DBG_printFloat( floatStr, ( float )reading, 6 ), qualCodes );
         }
         else if ( eNV_EXTERNAL_NV_ERROR == retVal )
         {
            DBG_logPrintf( 'R', "NV Error!" );
         }
         else if ( eAPP_ID_REQ_FUTURE == retVal )
         {
            DBG_logPrintf( 'R', "Req in Future Error!" );
         }
         else if ( eAPP_ID_REQ_TIME_BOUNDARY == retVal )
         {
            DBG_logPrintf( 'R', "Req not on boundary Error!" );
         }
         else if ( eAPP_ID_REQ_PAST == retVal )
         {
            DBG_logPrintf( 'R', "Req too far in past Error!" );
         }
         else
         {
            DBG_logPrintf( 'R', "Unknown Error!" );
         }
      }
      else
      {
         DBG_logPrintf( 'R', "Invalid time syntax" );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid Syntax" );
   }

   return ( 0 );
} /* end DBG_CommandLine_IdRd () */
#endif // ( LP_IN_METER == 0 )
#endif // ENABLE_ID_TASKS

#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#if (EP == 1)
/*******************************************************************************

   Function name: DBG_CommandLine_DsTimeDv

   Purpose: This function will get or set the daily shift time diversity.

   Arguments:  argc - Number of Arguments passed to this function
               argv - Pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_DsTimeDv( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   uint8_t uDsBuMaxTimeDiversity;

   if ( 1 == argc )
   {
      DBG_logPrintf( 'R', "%s %u", argv[ 0 ], HD_getDsBuMaxTimeDiversity() );
   }
   else if ( 2 == argc )
   {
      // Write dsBuMaxTimeDiversity
      uDsBuMaxTimeDiversity = ( uint8_t )atoi( argv[1] );
      retVal = HD_setDsBuMaxTimeDiversity( uDsBuMaxTimeDiversity );

      if ( eSUCCESS != retVal )
      {
         DBG_logPrintf( 'R', "Error writing dsBuMaxTimeDiversity. Error code %u", ( uint32_t )retVal );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   return( 0 );
} /* end DBG_CommandLine_DsTimeDv () */

#if ENABLE_ID_TASKS
/*******************************************************************************

   Function name: DBG_CommandLine_IdTimeDv

   Purpose: This function will get or set the interval data time diversity.

   Arguments:  argc - Number of Arguments passed to this function
               argv - Pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_IdTimeDv( uint32_t argc, char *argv[] )
{
   returnStatus_t retVal = eFAILURE;
   uint8_t uLpBuMaxTimeDiversity;

   if ( 1 == argc )
   {
      DBG_logPrintf( 'R', "%s %u", argv[ 0 ], ID_getLpBuMaxTimeDiversity() );
   }
   else if ( 2 == argc )
   {
      // Write dsBuMaxTimeDiversity
      uLpBuMaxTimeDiversity = ( uint8_t )atoi( argv[1] );
      retVal = ID_setLpBuMaxTimeDiversity( uLpBuMaxTimeDiversity );

      if ( eSUCCESS != retVal )
      {
         DBG_logPrintf( 'R', "Error writing uLpBuMaxTimeDiversity. Error code %u", ( uint32_t )retVal );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   return( 0 );
} /* end DBG_CommandLine_IdTimeDv () */

/*******************************************************************************

   Function name: DBG_CommandLine_IdParams

   Purpose: This function will read/write ID parameters

   Arguments:  argc - Number of Arguments passed to this function
               argv - Pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_IdParams ( uint32_t argc, char *argv[] )
{
   uint16_t buSchedule;

   if ( argc == 1 )  //Read the params
   {
      DBG_logPrintf( 'R', "buSchedule=%u minutes", ID_getLpBuSchedule() );
   }
   else if ( argc == 2 )  //Write the params
   {
      buSchedule = ( uint16_t )( atoi( argv[1] ) );
      ( void )ID_setLpBuSchedule( buSchedule );
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid Syntax, Read: No Params, Set: BuSchedule (in minutes)" );
   }

   return ( 0 );
} /* end DBG_CommandLine_IdRd () */
#endif   /* end of ENABLE_ID_TASKS */
#endif   /* end of EP == 1   */
#endif   /* end of if ( ACLARA_LC == 0 )  */

#if ( PORTABLE_DCU == 1)
/*******************************************************************************

   Function name: DBG_CommandLine_dfwMonMode

   Purpose: This function will enable or disable DFW reply monitor mode.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static uint32_t DBG_CommandLine_dfwMonMode ( uint32_t argc, char *argv[] )
{
   uint8_t mode;
   if ( argc > 3 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   else
   {
      if ( argc == 2 )
      {
         mode = ( uint8_t )atoi( argv[1] );
         if (mode == 1)
         {
            // enable DFW monitor mode
            DBG_DfwMonitorMode((bool)true);
         }
         else if (mode == 0)
         {
            // disable DFW monitor mode
            DBG_DfwMonitorMode((bool)false);
         }
      }

      /* always print current value */
      if (DBG_GetDfwMonitorMode() == (bool)true)
      {
         DBG_printf( "dfwmonitormode 1" );
      }
      else
      {
         DBG_printf( "dfwmonitormode 0" );
      }
   }

   return ( 0 );
} /* end DBG_CommandLine_dfwMonMode () */
#endif // ( PORTABLE_DCU == 1)

/*******************************************************************************

   Function name: DBG_CommandLine_Versions

   Purpose: This function will print out the current versions of several components
            in the project

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_Versions ( uint32_t argc, char *argv[] )
{
   uint8_t                    string[VER_HW_STR_LEN];
   firmwareVersion_u          ver;
   const firmwareVersionDT_s *dt;
   uint8_t                    index = 0;
   const uint8_t             *mcuVersion = ( uint8_t * )MCUVERSION_ADDR;

   ver = VER_getFirmwareVersion(eFWT_APP);
   dt  = VER_getFirmwareVersionDT();
   DBG_logPrintf( 'R', "RF Electric App V%02d.%02d.%04d Built %s %s",
                  ver.field.version, ver.field.revision, ver.field.build, &dt->date[0], &dt->time[0]);
// K22 EPs and 9975T don't have a bootloader
#if ( !(( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
   ver = VER_getFirmwareVersion(eFWT_BL);

   DBG_logPrintf( 'R', "RF Electric BL  V%02u.%02u.%04u",
                  ver.field.version, ver.field.revision, ver.field.build);
#endif
   ( void )VER_getHardwareVersion ( &string[0], sizeof(string) );
   DBG_logPrintf( 'R', "%s %s", VER_getComDeviceType(), &string[0] );
#if ( ( MCU_SELECTED == NXP_K24 ) || ( DCU == 1 ) )
   DBG_logPrintf( 'R', "BSP=%s BSPIO=%s PSP=%s IAR=%d",
                  BSP_Get_BspRevision(), BSP_Get_IoRevision(), BSP_Get_PspRevision(), __VER__ );
#endif

#if ( MCU_SELECTED == RA6E1 )
   const bsp_unique_id_t *uniqueId = R_BSP_UniqueIdGet();
   while( index < 4 )
   {
      DBG_logPrintf( 'R',"Unique ID %d - %x", index, uniqueId->unique_id_words[index] );
      index++;
   }
   DBG_logPrintf( 'R', "Part Numbering Info %s",VER_getComDeviceMicroMPN() );
   DBG_logPrintf( 'R', "MCU Version Register %d",*(mcuVersion));
#endif

#if ( RTOS_SELECTION == MQX_RTOS )
   DBG_logPrintf( 'R', "MQX=%s MQXgen=%s MQXLibraryDate=%s",
                  OS_Get_OsVersion(), OS_Get_OsGenRevision(), OS_Get_OsLibDate() );
#endif
#if ( ( MCU_SELECTED == NXP_K24 ) || ( DCU == 1 ) )
   DBG_logPrintf( 'R', "Silicon Info: 0x%04x", SIM_SDID & 0xffff );  // TODO: RA6: Support this line
#endif
#if ( MCU_SELECTED == RA6E1 )
   DBG_logPrintf( 'R', "BSP=%s IAR=%d", BSP_Get_BSPVersion(), __VER__ );
#endif
#if ( RTOS_SELECTION == FREE_RTOS )
   DBG_logPrintf( 'R', "FreeRTOS=%s", tskKERNEL_VERSION_NUMBER );
#endif

#if ( DCU == 1 )
   DBG_logPrintf( 'R', "TBImageTarget: %s", VER_strGetDCUVersion() );
#endif
#if ( PORTABLE_DCU == 1 )
   DBG_logPrintf( 'R', "Portable DCU" );
#endif
#if ( MFG_MODE_DCU == 1 )
   DBG_logPrintf( 'R', "Mfg Test DCU" );
#endif
#if ( DFW_TEST_KEY == 1 )
   DBG_logPrintf( 'R', "DFW Test Enabled" );
#endif
#if ( OTA_CHANNELS_ENABLED == 1 )
   DBG_logPrintf( 'R', "OTA Channels Enabled" );
#endif

#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )
   DBG_logPrintf ('R', "HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A");
#endif

#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
   DBG_logPrintf ('R', "HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_B");
#endif

   return ( 0 );
} /* end DBG_CommandLine_Versions () */


#if (EP == 1)
static const char * const ResetReasons[] =
{
   "Power on Reset",
   "External Reset Pin",
   "Watchdog",
   "Loss of Ext Clock",
   "Low Voltage",
   "Low Leakage Wakeup",
   "Stop Mode ACK Error",
   "EZPort Reset",
   "MDM-AP",
   "Software Reset",
   "Core Lockup",
   "JTAG",
   "Anomaly Count"
};
/*******************************************************************************

   Function name: DBG_CommandLine_Counters

   Purpose: This function will print out the current counters

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_Counters ( uint32_t argc, char *argv[] )
{
   uint32_t i;                         /* Loop counter/index into reset reason array               */
   uint32_t tib;                       /* Bit mask used to loop through all possible reset causes  */
   uint16_t const *counter;            /* Pointer to individual counters from power fail file      */
   const pwrFileData_t *pwrFileData;   /* Pointer to power fail file                               */

   pwrFileData = PWR_getpwrFileData();
   counter = (uint16_t const *)&pwrFileData->uPowerDownCount;
   if ( argc == 2 )
   {
      if ( strcasecmp( argv[1], "reset" ) == 0 )
      {
         PWR_resetCounters();
      }
   }

   for( i = 0, tib = RESET_SOURCE_POWER_ON_RESET; tib <= RESET_ANOMALY_COUNT; tib <<= 1, i++ )
   {
      DBG_logPrintf( 'I', "%s: %d", ResetReasons[i], *counter );
      counter++;
   }
   ( void )PWR_printResetCause();
   return ( 0 );
}
#endif

#if 0 // RA6E1 Bob: This command was removed from original K24 code
/*******************************************************************************
   Function name: DBG_CommandLine_SendMetadata

   Purpose: This command will trigger transmission of a Metadata (/or/md) message

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - status of this function - currently always 0 (success)
*******************************************************************************/
uint32_t DBG_CommandLine_SendMetadata ( uint32_t argc, char *argv[] )
{
   DBG_logPrintf( 'R', "Received DBG_CommandLine_SendMetadata command" );

   return ( 0 );
}
#endif // RA6E1 Bob: This command was removed from original K24 code

/***********************************************************************************************************************

   Function name: atoh

   Purpose:  Converts 2-bytes of ASCII to one byte of hex.  For example:  ASCII 0x31 0x32 is 0x12 in Hex

   Arguments: uint8_t *pHex - Location to store the converted ASCII Byte, char *pAscii - 2 bytes of ASCII

   Returns: returnStatus_t - eSUCCESS

   Side Effects: None

   Reentrant Code: No

   Notes:  Not much error checking here!!!

 **********************************************************************************************************************/
static returnStatus_t atoh( uint8_t *pHex, char const *pAscii )
{
   ( void )sscanf( ( char* )pAscii, "%02hhx", pHex );
   return( eSUCCESS );
}

/*******************************************************************************
   Function name: DBG_CommandLine_SendAppMsg

   Purpose: This command will trigger call to an application layer tx function

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - status of this function - currently always 0 (success)
   Note:    If invoked as sendMTLSmsg, will send a message through the appropriate port
*******************************************************************************/
uint32_t DBG_CommandLine_SendAppMsg ( uint32_t argc, char *argv[] )
{
   uint8_t        data[MAX_ATOH_CHARS];
   uint8_t        address[MULTICAST_ADDRESS_SIZE * 2];
   uint16_t       dataLen;
   uint16_t       bufIndex;
   uint16_t       srcIndex;
   uint32_t i;

   COMM_Tx_t tx_data;

   DBG_logPrintf( 'R', "Received DBG_CommandLine_SendAppMsg command" );
   if ((argc >= 4) && (argc <=6))  /* The number of arguments must be 3-5 */
   {
      dataLen = ( uint16_t )strlen( argv[1] );
      if ( ( dataLen % 2 ) != 0 )
      {
         DBG_logPrintf( 'R',
                        "data field must be divisible by 2 (2 chars per hex byte) 'sendappmsg 1354 1122334455 3F'" );
      }
      else if ( dataLen > ( MAX_ATOH_CHARS * 2 ) )
      {
         DBG_logPrintf( 'R', "data field must be less than or equal to %u", MAX_ATOH_CHARS );

      }
      else if ( (strlen(argv[2]) != 10) && (strlen(argv[2]) != 12) )
      {
         DBG_logPrintf('R', "Address must be 10 or 12 chars (5 or 6) hex bytes. 'sendappmsg 1354 1122334455(66) 3F'");
      }

      else if ( ( strlen( argv[3] ) != 1 ) && ( strlen( argv[3] ) != 2 ) )
      {
         DBG_logPrintf( 'R',
                        "QoS field must be 2 chars representing a hex byte. 'sendappmsg 1354 1122334455 3F'" );
      }
      else
      {
         /* convert ascii data to hex data */
         for ( bufIndex = 0, srcIndex = 0;
               ( bufIndex < sizeof( data ) ) && ( 0 != argv[1][srcIndex] );
               srcIndex += 2, bufIndex++ )
         {
            ( void )atoh( &data[bufIndex], &argv[1][srcIndex] );
         }
         /* convert ascii address to hex */
         for ( bufIndex = 0, srcIndex = 0;
               ( bufIndex < sizeof( address ) ) && ( 0 != argv[2][srcIndex] );
               srcIndex += 2, bufIndex++ )
         {
            ( void )atoh( &address[bufIndex], &argv[2][srcIndex] );
         }

         if ( strlen(argv[2]) == ( MAC_ADDRESS_SIZE * 2 ) )
         {
#if ( EP == 1 )
            tx_data.dst_address.addr_type = eCONTEXT; // On an EP, we send a message to the HE so we use context.
#else
            tx_data.dst_address.addr_type = eEXTENSION_ID; // On a DCU, we send a message to an EP so we use the MAC address.
#endif
            //lint -e{772} address initialized by atoh above
            ( void )memcpy( tx_data.dst_address.ExtensionAddr, address, MAC_ADDRESS_SIZE );
         }
         else if ( strlen( argv[2] ) == ( MULTICAST_ADDRESS_SIZE * 2 ) )
         {
            tx_data.dst_address.addr_type = eMULTICAST;
            //lint -e{772} address initialized by atoh above
            (void)memcpy(tx_data.dst_address.multicastAddr, address, MULTICAST_ADDRESS_SIZE);
         }

         tx_data.length = dataLen / 2;
         tx_data.data = data;
         tx_data.callback = NULL;
         tx_data.override.linkOverride = eNWK_LINK_OVERRIDE_NULL;

         dataLen = ( uint16_t )strlen( argv[3] );
         if ( dataLen  == 1 )
         {
            /* user entered a single digit, assume it represents a decimal number */
            tx_data.qos = ( uint8_t )atoi( argv[3] );
         }
         else if ( dataLen  == 2 )
         {
            /* user entered 2 ascii chars, assume it represents a hex byte (3F, 1A, etc)*/
            /* convert ascii data to hex data */
            for ( bufIndex = 0, srcIndex = 0;
                  ( bufIndex < sizeof( data ) ) && ( 0 != argv[3][srcIndex] );
                  srcIndex += 2, bufIndex++ )
            {
               ( void )atoh( &tx_data.qos, &argv[3][srcIndex] );
            }
            if ( !NWK_IsQosValueValid( tx_data.qos ) )
            {
               DBG_logPrintf( 'R', "QoS must be valid. 'sendappmsg 1354 1122334455 3F'" );
            }
            else
            {
               if ( strcasecmp( argv[0], "sendappmsg" ) == 0 )
               {
                  if(argc>=5)
                  {
                     for ( i = 0; i < (uint32_t)atoi(argv[4]); i++ )
                     {
                        COMM_transmit( &tx_data );
                        if ( argc == 6 )
                        {
                           OS_TASK_Sleep( (uint32_t)atoi(argv[5]) );
                        }
                     }
                  }
                  else
                  {
                     COMM_transmit( &tx_data );
                  }
               }
#if ( USE_MTLS == 1 )
               else
               {
                  MTLS_transmit( &tx_data );
               }
#endif
            }
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R',
                     "Invalid, expected format:  'sendappmsg <data> <address> [repeats] [delay(ms)]' : eg. 'sendappmsg 1354 1122334455'" );
   }

   return ( 0 );
}

#if (USE_IPTUNNEL == 1)
/**
This command will trigger call to the IP Tunneling layer tx function

@param argc Number of Arguments passed to this function
@param argv pointer to the list of arguments passed to this function

@return status of this function - currently always 0 (success)
*/
static uint32_t DBG_CommandLine_SendIpTunMsg(uint32_t argc, char *argv[])
{
   uint8_t        data[MAX_ATOH_CHARS];
   uint8_t        address[MULTICAST_ADDRESS_SIZE * 2];
   uint16_t       dataLen;
   uint16_t       bufIndex;
   uint16_t       srcIndex;
   COMM_Tx_t tx_data;

   DBG_logPrintf('R', "Received DBG_CommandLine_SendIpTunMsg command");
   if (argc == 4)  /* The number of arguments must be 3 */
   {
      dataLen = (uint16_t)strlen(argv[1]);
      if ((dataLen % 2) != 0)
      {
         DBG_logPrintf('R',
                       "data field must be divisible by 2 (2 chars per hex byte) 'sendtunmsg 1354 1122334455 3F'");
      }
      else if (dataLen > (MAX_ATOH_CHARS * 2))
      {
         DBG_logPrintf('R', "data field must be less than or equal to %u", MAX_ATOH_CHARS);
      }
      else if ((strlen(argv[2]) != 10) && (strlen(argv[2]) != 12))
      {
         DBG_logPrintf('R', "Address must be 10 or 12 chars (5 or 6) hex bytes. 'sendtunmsg 1354 1122334455(66) 3F'");
      }

      else if ((strlen(argv[3]) != 1) && (strlen(argv[3]) != 2))
      {
         DBG_logPrintf('R',
                       "QoS field must be 2 chars representing a hex byte. 'sendtunmsg 1354 1122334455 3F'");
      }
      else
      {
         /* convert ascii data to hex data */
         for (bufIndex = 0, srcIndex = 0;
              (bufIndex < sizeof(data)) && (0 != argv[1][srcIndex]);
              srcIndex += 2, bufIndex++)
         {
            (void)atoh(&data[bufIndex], &argv[1][srcIndex]);
         }

         /* convert ascii address to hex */
         for (bufIndex = 0, srcIndex = 0;
              (bufIndex < sizeof(address)) && (0 != argv[2][srcIndex]);
              srcIndex += 2, bufIndex++)
         {
            (void)atoh(&address[bufIndex], &argv[2][srcIndex]);
         }

         if (strlen(argv[2]) == (MAC_ADDRESS_SIZE * 2))
         {
            tx_data.dst_address.addr_type = eEXTENSION_ID;
            //lint -e{772} address initialized by atoh above
            (void)memcpy(tx_data.dst_address.ExtensionAddr, address, MAC_ADDRESS_SIZE);
         }
         else if (strlen(argv[2]) == (MULTICAST_ADDRESS_SIZE * 2))
         {
            tx_data.dst_address.addr_type = eMULTICAST;
            //lint -e{772} address initialized by atoh above
            (void)memcpy(tx_data.dst_address.multicastAddr, address, MULTICAST_ADDRESS_SIZE);
         }

         tx_data.length = dataLen / 2;
         tx_data.data = data;
         tx_data.callback = NULL;
         tx_data.override.linkOverride = eNWK_LINK_OVERRIDE_NULL;

         dataLen = (uint16_t)strlen(argv[3]);
         if (dataLen == 1)
         {
            /* user entered a single digit, assume it represents a decimal number */
            tx_data.qos = (uint8_t)atoi(argv[3]);
         }
         else if (dataLen == 2)
         {
            /* user entered 2 ascii chars, assume it represents a hex byte (3F, 1A, etc)*/
            /* convert ascii data to hex data */
            for (bufIndex = 0, srcIndex = 0;
                 (bufIndex < sizeof(data)) && (0 != argv[3][srcIndex]);
                 srcIndex += 2, bufIndex++)
            {
               (void)atoh(&tx_data.qos, &argv[3][srcIndex]);
            }

            if (!NWK_IsQosValueValid(tx_data.qos))
            {
               DBG_logPrintf('R', "QoS must be valid. 'sendtunmsg 1354 1122334455 3F'");
            }
            else
            {
               IP_transmit(&tx_data);
            }
         }
      }
   }
   else
   {
      DBG_logPrintf('R',
                    "Invalid, expected format:  'sendtunmsg <data> <address> <qos>' : eg. 'sendtunmsg 1354 1122334455 3F'");
   }

   return 0;
}
#endif // (USE_IPTUNNEL == 1)

/*******************************************************************************
   Function name: DBG_CommandLine_SendHeadEndMsg

   Purpose: This command will trigger call to an application layer tx function
   using context ID as destination address

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - status of this function - currently always 0 (success)
*******************************************************************************/
uint32_t DBG_CommandLine_SendHeadEndMsg ( uint32_t argc, char *argv[] )
{
   uint8_t        data[MAX_ATOH_CHARS];
   uint16_t       dataLen;
   uint16_t       bufIndex;
   uint16_t       srcIndex;
   COMM_Tx_t tx_data;

   DBG_logPrintf( 'R', "Received DBG_CommandLine_SendHeadEndMsg command" );
   if ( argc == 3 )  /* The number of arguments must be 3 */
   {
      dataLen = ( uint16_t )strlen( argv[1] );
      if ( ( dataLen % 2 ) != 0 )
      {
         DBG_logPrintf( 'R', "data field must be divisible by 2 (2 chars per hex byte) 'sendheadendmsg 1354 3F'" );
      }
      else if ( dataLen > ( MAX_ATOH_CHARS * 2 ) )
      {
         DBG_logPrintf( 'R', "data field must be less than or equal to %u", MAX_ATOH_CHARS );
      }
      else if ( ( strlen( argv[2] ) != 1 ) && ( strlen( argv[2] ) != 2 ) )
      {
         DBG_logPrintf( 'R',
                        "QoS field must be 2 chars representing a hex byte. 'sendheadendmsg 1354 3F'" );
      }

      else
      {
         /* convert ascii data to hex data */
         for ( bufIndex = 0, srcIndex = 0;
               ( bufIndex < sizeof( data ) ) && ( 0 != argv[1][srcIndex] );
               srcIndex += 2, bufIndex++ )
         {
            ( void )atoh( &data[bufIndex], &argv[1][srcIndex] );
         }

         NWK_GetConf_t GetConf;

         // Retrieve Head End context ID
         GetConf = NWK_GetRequest( eNwkAttr_ipHEContext );

         if (GetConf.eStatus != eNWK_GET_SUCCESS) {
            GetConf.val.ipHEContext = DEFAULT_HEAD_END_CONTEXT; // Use default in case of error
         }

         tx_data.dst_address.addr_type = eCONTEXT;
         //lint -e{772} address initialized by atoh above
         tx_data.dst_address.context = GetConf.val.ipHEContext;
         tx_data.length = dataLen / 2;
         tx_data.data = data;
         tx_data.callback = NULL;
         tx_data.override.linkOverride = eNWK_LINK_OVERRIDE_NULL;

         dataLen = ( uint16_t )strlen( argv[2] );
         if ( dataLen  == 1 )
         {
            /* user entered a single digit, assume it represents a decimal number */
            tx_data.qos = ( uint8_t )atoi( argv[2] );
         }
         else if ( dataLen  == 2 )
         {
            /* user entered 2 ascii chars, assume it represents a hex byte (3F, 1A, etc)*/
            /* convert ascii data to hex data */
            for ( bufIndex = 0, srcIndex = 0;
                  ( bufIndex < sizeof( data ) ) && ( 0 != argv[2][srcIndex] );
                  srcIndex += 2, bufIndex++ )
            {
               ( void )atoh( &tx_data.qos, &argv[2][srcIndex] );
            }
         }
         if ( !NWK_IsQosValueValid( tx_data.qos ) )
         {
            DBG_logPrintf( 'R', "QoS must be valid. 'sendheadendmsg 1354 3F'" );
         }
         else
         {
            COMM_transmit( &tx_data );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid, expected format:  'sendheadendmsg <data> <QoS>' : eg. 'sendappmsg 1354 00'" );
   }

   return ( 0 );
}
/*******************************************************************************
   Function name: DBG_CommandLine_SendHeepMsg

   Purpose: This command will send a HEEP header, only, to the head end

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - status of this function - currently always 0 (success)
*******************************************************************************/
static uint32_t DBG_CommandLine_SendHeepMsg ( uint32_t argc, char *argv[] )
{
   HEEP_APPHDR_t        heepMsgInfo;         /* HEEP header */
   static uint8_t       alarmID = 0;

   HEEP_initHeader( &heepMsgInfo );          /* Set up the header info with defaults   */
   heepMsgInfo.TransactionType   = ( uint8_t )TT_ASYNC;
   heepMsgInfo.Resource          = ( uint8_t )bu_am;
   heepMsgInfo.Method_Status     = ( uint8_t )method_post;
   heepMsgInfo.Req_Resp_ID       = alarmID++;
   heepMsgInfo.qos               = 0x39;     /* Per the RF Electric Phase 1 spreadsheet   */
   heepMsgInfo.callback          = NULL;
   ( void )HEEP_MSG_TxHeepHdrOnly( &heepMsgInfo );

   return ( 0 );
}

/*******************************************************************************
   Function name: DBG_CommandLine_InsertAppMsg

   Purpose: This command will insert a NWK payload into the bottom of the APP layer

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - status of this function - currently always 0 (success)
*******************************************************************************/
uint32_t DBG_CommandLine_InsertAppMsg ( uint32_t argc, char *argv[] )
{
   uint8_t        data[MAX_ATOH_CHARS];
   uint16_t       dataLen;
   uint16_t       bufIndex;
   uint16_t       srcIndex;
   buffer_t      *stack_buffer = NULL;
   IP_Frame_t     rx_frame;
   TIMESTAMP_t    timeStamp;

   DBG_logPrintf( 'R', "Received DBG_CommandLine_InsertAppMsg command" );
   if ( argc == 2 )  /* The number of arguments must be 2 */
   {
      dataLen = ( uint16_t )strlen( argv[1] );
      if ( ( dataLen % 2 ) != 0 )
      {
         DBG_logPrintf( 'R', "data field must be divisible by 2 (2 chars per hex byte) 'insertmacmsg 1354AB'" );
      }
      else if ( dataLen > ( MAX_ATOH_CHARS *  2) )
      {
         DBG_logPrintf( 'R', "data field must be less than or equal to %u", MAX_ATOH_CHARS );

      }
      else
      {
         (void)memset(data, 0, MAX_ATOH_CHARS);
         /* convert ascii data to hex data */
         for ( bufIndex = 0, srcIndex = 0;
               ( bufIndex < sizeof( data ) ) && ( 0 != argv[1][srcIndex] );
               srcIndex += 2, bufIndex++ )
         {
            ( void )atoh( &data[bufIndex], &argv[1][srcIndex] );
         }

         rx_frame.version              = 0;
         rx_frame.qualityOfService     = 0;
         rx_frame.nextHeaderPresent    = 0;
         rx_frame.srcAddress.addr_type = eCONTEXT;
         rx_frame.srcAddress.context   = 0;
         rx_frame.dstAddress.addr_type = eELIDED;
         rx_frame.nextHeader           = 0;
         rx_frame.src_port             = 0;   /* 4 bit. source port */
         rx_frame.dst_port             = 0;   /* 4 bit. destination port */
         rx_frame.length               = bufIndex;
         rx_frame.data                 = data;

         /* generate a stack indication */
         stack_buffer = BM_allocStack( (sizeof(NWK_DataInd_t) + rx_frame.length) );
         if (stack_buffer != NULL)
         {
            timeStamp.QSecFrac = TIME_UTIL_GetTimeInQSecFracFormat();
            NWK_SendDataIndication(stack_buffer, &rx_frame, timeStamp);
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid, expected format:  'insertappmsg 11223344'" );
   }

   return ( 0 );
}

/*******************************************************************************
   Function name: DBG_CommandLine_InsertMacMsg

   Purpose: This command will insert a phy payload into the bottom of the MAC
   layer

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - status of this function - currently always 0 (success)
*******************************************************************************/
uint32_t DBG_CommandLine_InsertMacMsg ( uint32_t argc, char *argv[] )
{
   uint8_t        data[MAX_ATOH_CHARS];
   uint16_t       dataLen;
   uint16_t       length;
   uint16_t       bufIndex;
   uint16_t       srcIndex;
   uint8_t        framing = (uint8_t)ePHY_FRAMING_0;
   uint8_t        mode = (uint8_t)ePHY_MODE_1;

   DBG_logPrintf( 'R', "Received DBG_CommandLine_InsertMacMsg command" );
   if ( ( argc == 2 ) || ( argc == 4 ) )  /* The number of arguments must be 1 or 3 */
   {
      dataLen = ( uint16_t )strlen( argv[1] );
      if ( ( dataLen % 2 ) != 0 )
      {
         DBG_logPrintf( 'R', "data field must be divisible by 2 (2 chars per hex byte) 'insertmacmsg 1354AB'" );
      }
      else if ( dataLen > ( MAX_ATOH_CHARS *  2) )
      {
         DBG_logPrintf( 'R', "data field must be less than or equal to %u", MAX_ATOH_CHARS );
      }
      else
      {
         (void)memset(data, 0, MAX_ATOH_CHARS);
         /* convert ascii data to hex data */
         for ( bufIndex = 0, srcIndex = 0;
               ( bufIndex < sizeof( data ) ) && ( 0 != argv[1][srcIndex] );
               srcIndex += 2, bufIndex++ )
         {
            ( void )atoh( &data[bufIndex], &argv[1][srcIndex] );
         }
         if ( argc == 2 )
         {
            /* framing/mode not provided, assume SRFN */
            PHY_InsertTestMsg( data, ( dataLen / 2 ), ePHY_FRAMING_0, ePHY_MODE_1);
         }
         else
         {
            /* convert ascii data to hex data (framing) */
            length = ( uint16_t )strlen( argv[2] );
            if ( length  == 1 )
            {
               /* user entered a single digit, assume it represents a decimal number */
               framing = ( uint8_t )atoi( argv[2] );
            }
            else if ( length  == 2 )
            {
               /* user entered 2 ascii chars, assume it represents a hex byte (01, 1A, etc)*/
               ( void )atoh( &framing, &argv[2][0] );
            }

            if ( ( (PHY_FRAMING_e)framing != ePHY_FRAMING_0 ) &&
                 ( (PHY_FRAMING_e)framing != ePHY_FRAMING_1 ) &&
                 ( (PHY_FRAMING_e)framing != ePHY_FRAMING_2 ) )
            {
               DBG_logPrintf( 'R', "Framing must be valid (0-2)" );
            }
            else
            {
               /* convert ascii data to hex data (mode)*/
               length = ( uint16_t )strlen( argv[3] );
               if ( length  == 1 )
               {
                  /* user entered a single digit, assume it represents a decimal number */
                  mode = ( uint8_t )atoi( argv[3] );
               }
               else if ( length  == 2 )
               {
                  /* user entered 2 ascii chars, assume it represents a hex byte (01, 1A, etc)*/
                  ( void )atoh( &mode, &argv[3][0] );
               }
               if ( ( (PHY_MODE_e)mode != ePHY_MODE_0 ) &&
                    ( (PHY_MODE_e)mode != ePHY_MODE_1 ) &&
                    ( (PHY_MODE_e)mode != ePHY_MODE_2 ) &&
                    ( (PHY_MODE_e)mode != ePHY_MODE_3 ) )
               {
                  DBG_logPrintf( 'R', "Mode must be valid (0-3)" );
               }
               else
               {
                  /*lint -e{772} Symbol 'data' initialized by call to strlen  */
                  PHY_InsertTestMsg( data, ( dataLen / 2 ), (PHY_FRAMING_e)framing, (PHY_MODE_e)mode );
               }
            }
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid, expected format:  'insertmacmsg 11223344' (SRFN) or 'insertmacmsg 11223344 2 2'" );
   }

   return ( 0 );
}

#if ( EP == 1 )
/*******************************************************************************

   Function name: DBG_CommandLine_RegTimeouts

   Purpose: This function will print out or set the reigstration timeout values

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_RegTimeouts ( uint32_t argc, char *argv[] )
{
   bool     allParamsValid = ( bool )true;
   uint32_t nextTimeout = RegistrationInfo.nextRegistrationTimeout;
   uint32_t maxTimeout  = RegistrationInfo.maxRegistrationTimeout;
   uint16_t minTimeout  = RegistrationInfo.minRegistrationTimeout;

   switch ( argc )
   {
      case 1:  /* No parameters will print the registration timeouts */
         DBG_logPrintf( 'R', "MinRegistrationTimeout (seconds): %hd", RegistrationInfo.minRegistrationTimeout );
         DBG_logPrintf( 'R', "MaxRegistrationTimeout (seconds): %d", RegistrationInfo.maxRegistrationTimeout );
         DBG_logPrintf( 'R', "NextRegistrationTimeout (seconds): %d", RegistrationInfo.nextRegistrationTimeout );
         DBG_logPrintf( 'R', "ActiveRegistrationTimeout (seconds): %d", RegistrationInfo.activeRegistrationTimeout );
         allParamsValid = ( bool )false; // so it won't try to save any values below
         break;
      case 4:
         nextTimeout = ( uint32_t )atoi( argv[3] );
         if ( nextTimeout == 0 )
         {
            DBG_logPrintf( 'R', "Invalid NextRegistrationTimeout value (must be > 0)" );
            allParamsValid = ( bool )false;
         }
      //lint -fallthrough
      case 3:
         maxTimeout = ( uint32_t )atoi( argv[2] );
         if ( maxTimeout < 30 )
         {
            DBG_logPrintf( 'R', "Invalid MaxRegistrationTimeout value (must be >= 30)" );
            allParamsValid = ( bool )false;
         }
      //lint -fallthrough
      case 2:
         minTimeout = ( uint16_t )atoi( argv[1] );
         if ( minTimeout < 30 )
         {
            DBG_logPrintf( 'R', "Invalid MinRegistrationTimeout value (must be >= 30)" );
            allParamsValid = ( bool )false;
         }
         break;
      default:
         DBG_logPrintf( 'R',
                        "Invalid number of parameters!"
                        " No params for read, 1, 2, or 3 to set min, max, next Registration Timeouts" );
   }

   if ( allParamsValid && ( maxTimeout < minTimeout ) )
   {
      DBG_logPrintf( 'R', "MaxRegistrationTimeout must be >= MinRegistrationTimeout" );
      allParamsValid = ( bool )false;
   }

   if ( allParamsValid )
   {
      RegistrationInfo.minRegistrationTimeout  = minTimeout;
      RegistrationInfo.maxRegistrationTimeout  = maxTimeout;
      RegistrationInfo.nextRegistrationTimeout = nextTimeout;
      APP_MSG_UpdateRegistrationFile();
   }
   return ( 0 );
} /* end DBG_CommandLine_RegTimeouts () */

/*******************************************************************************

   Function name: DBG_CommandLine_RegState

   Purpose: This function will print out or set the reigstration state

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_RegState ( uint32_t argc, char *argv[] )
{
   sysTime_dateFormat_t formattedDate;    /* Installation date/time in yy, mm, dd, hh, mm, dd, ss  */
   char                 *currRegState;    /* String representation of registration state  */
   uint32_t             installSeconds;   /* Installation date/time in seconds  */
   sysTime_t            installTime;      /* Installation date/time in system format   */

   switch ( argc )
   {
      case 1:  /* No parameters will print the registration state */
         switch ( RegistrationInfo.registrationStatus )
         {
            case REG_STATUS_NOT_SENT:
               currRegState = "NotSent";
               break;
            case REG_STATUS_NOT_CONFIRMED:
               currRegState = "Sent";
               break;
            case REG_STATUS_CONFIRMED:
               currRegState = "Complete";
               break;
            default:
               currRegState = "??";  //lint !e585 not a trigraph
               break;
         }
         DBG_logPrintf( 'R', "Current Registration State: %s", currRegState );
         installSeconds = TIME_SYS_GetInstallationDateTime();
         TIME_UTIL_ConvertSecondsToSysFormat( installSeconds, 0, &installTime );
         ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &installTime, &formattedDate );
         DBG_logPrintf( 'R', "Installation date/time: %02d/%02d/%02d %02d:%02d:%02d (%d)",
                         formattedDate.month, formattedDate.day, formattedDate.year, formattedDate.hour,
                         formattedDate.min, formattedDate.sec, installSeconds);
         break;
      case 2:
         if ( ( strcasecmp( argv[1], "NotSent" ) == 0 ) || ( strcasecmp( argv[1], "0" ) == 0 ) )
         {
            APP_MSG_ReEnableRegistration();
         }
         else if ( ( strcasecmp( argv[1], "Sent" ) == 0 ) || ( strcasecmp( argv[1], "1" ) == 0 ) )
         {
            sysTimeCombined_t currentTime;   // Get current time

            APP_MSG_initRegistrationInfo();
            RegistrationInfo.registrationStatus   = REG_STATUS_NOT_CONFIRMED;
            ( void )TIME_UTIL_GetTimeInSysCombined( &currentTime );
            RegistrationInfo.timeRegistrationSent = ( uint32_t )( currentTime / TIME_TICKS_PER_SEC );
            APP_MSG_UpdateRegistrationFile();
         }
         else if ( ( strcasecmp( argv[1], "Complete" ) == 0 ) || ( strcasecmp( argv[1], "2" ) == 0 ) )
         {
            RegistrationInfo.registrationStatus      = REG_STATUS_CONFIRMED;
            RegistrationInfo.nextRegistrationTimeout = RegistrationInfo.minRegistrationTimeout;
            APP_MSG_UpdateRegistrationFile();
         }
         else
         {
            DBG_logPrintf( 'R', "Invalid Registration State value - must be { NotSent | Sent | Complete }" );
         }
         break;
      default:
         DBG_logPrintf( 'R', "Invalid number of parameters!  No params for read; {NotSent | Sent | Complete} to set" );
   }
   return ( 0 );
} /* end DBG_CommandLine_RegState () */
#endif // (EP == 1)

/*******************************************************************************
   Function name: DBG_CommandLine_chksum

   Purpose: This command will calculate a checksum (TWACS serial port protocol) over a passed in hex string

   Arguments:  char *msg - pointer to message to convert

   Returns: checksum of message passed
*******************************************************************************/
uint8_t DBG_CommandLine_chksum ( char *msg )
{
   uint8_t        data;
   uint8_t        chksum = 0;

   while ( *msg != 0 )                          /* Loop thru input string, converting ASCII to hex.   */
   {
      ( void )sscanf( msg, "%02hhx", &data );
      chksum += data;                           /* Update checksum by simple 8 bit addition. */
      msg += 2;                                 /* Each sscanf consumes 2 input bytes.       */
   }

   return ( chksum );
}
static const char * const SRFNheader[2] =
{
   "00A90205",     /* No response expected */
   "00290105"      /* Response expected */
};
/*******************************************************************************
   Function name: DBG_CommandLine_tunnel

   Purpose: This command will convert a string of hex bytes into hex ASCII

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - status of this function - currently always 0 (success)
*******************************************************************************/
uint32_t DBG_CommandLine_tunnel ( uint32_t argc, char *argv[] )
{
   eui_addr_t  addr;
   uint8_t     data[MAX_ATOH_CHARS];
   char        const * hdr = SRFNheader[ 0 ];
   uint32_t    maxLen;
   uint8_t     chkstring[ 3 ];
   uint16_t    dataLen;
   uint16_t    bufIndex;
   uint16_t    srcIndex;
   uint8_t     chksum;
   uint8_t     qos;
   bool        valid = (bool)true;

   if ( argc >= 3 )  /* The number of arguments must be at least 2 */
   {
      if ( ( 0 == strcasecmp( argv[1], "true" ) ) || ( 0 == strcmp( argv[1], "1" ) ) )
      {
         hdr = SRFNheader[ 1 ];
      }

      /* Compute max input string: Size of data - strlen( "sendappmsg " ) - strlen( hdr ) - SOH - chksum - EOT - '\0 */
      maxLen = ( sizeof( data ) - strlen("sendappmsg ") ) - strlen( SRFNheader[ 0 ] );
      maxLen -= ( sizeof( char ) * 3 ) + sizeof( chksum ) * 4;    /* SOH, EOT, '\0 each take 2 bytes; chksum takes 4 */
      maxLen /= 2;                                                /* 2 characters output per input character.  */
      dataLen = ( uint16_t )strlen( argv[2] );
      if ( dataLen > maxLen ) /* Need room for SOH, checksum and EOT */
      {
         DBG_logPrintf( 'R', "data field must be less than or equal to %d", maxLen );

      }
      else
      {
         chksum = DBG_CommandLine_chksum( argv[ 2 ] );
         ( void )sprintf( ( char *)chkstring, "%02X", chksum );
         bufIndex = ( uint16_t )sprintf( ( char * )data, "sendappmsg  %s%02X", hdr, SOH );

         /* convert ascii data to hex data */
         for ( srcIndex = 0; ( bufIndex < sizeof(data) - 5 ) && ( 0 != argv[2][srcIndex] ); srcIndex++, bufIndex += 2 )
         {
            ASCII_htoa( &data[bufIndex], argv[2][srcIndex] );
         }

         for ( uint16_t i = 0; i < 2; i++ )                              /* Append the checksum  */
         {
            ASCII_htoa( &data[bufIndex], chkstring[i] );
            bufIndex += 2;
         }
         ( void )snprintf( ( char * )&data[ bufIndex ], ( int32_t )sizeof( data ) - bufIndex, "%02x", EOT );  /* Append the EOT */

         if ( argc >= 4 )  /* mac address supplied? */
         {
            if ( strlen( argv[ 3 ] ) != 10 )
            {
               DBG_logPrintf( 'R', "mac address must be 5 bytes in length" );
               valid = (bool)false;
            }
            else
            {
               for ( uint32_t i = 0; i < 4; i++ )
               {
                  ( void )sscanf( &argv[ 3 ][ i * 2 ], "%2hhx", &addr[ i + 3 ] );
               }
            }
         }
         else
         {

            /*lint -e{545} & operator needed by compiler in next statement */
            MAC_EUI_Address_Get( ( eui_addr_t * )&addr );
         }
         if ( argc == 5 )  /* qos provided? */
         {
            ( void )sscanf( argv[ 4 ], "%2hhx", &qos );
         }
         else
         {
            qos = 0;
         }

         if ( valid )
         {
            /*lint -e{644} addr initialized if valid is true.  */
            DBG_logPrintf( 'R', "%s %02X%02X%02X%02X%02X %02X", data, addr[3], addr[4], addr[5], addr[6], addr[7], qos );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R',
                  "Format error: 'tunnel 0|1 <data> [<macaddr>] [<qos>]', e.g. tunnel 1 AA010000000008FC040000C0" );
   }

   return ( 0 );
}

/*******************************************************************************
   Function name: DBG_CommandLine_crc

   Purpose: This command will calculate a crc over a passed in hex string

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - status of this function - currently always 0 (success)
*******************************************************************************/
uint32_t DBG_CommandLine_crc ( uint32_t argc, char *argv[] )
{
   uint8_t        data[MAX_ATOH_CHARS];
   uint16_t       dataLen;
   uint16_t       bufIndex;
   uint16_t       srcIndex;
   uint32_t       crc_value;

   DBG_logPrintf( 'R', "Received DBG_CommandLine_crc command" );
   if ( argc == 2 )  /* The number of arguments must be 1 */
   {
      dataLen = ( uint16_t )strlen( argv[1] );
      if ( ( dataLen % 2 ) != 0 )
      {
         DBG_logPrintf( 'R', "data field must be divisible by 2 (2 chars per hex byte) 'crc 135455'" );
      }
      else if ( dataLen > ( MAX_ATOH_CHARS * 2 ) )
      {
         DBG_logPrintf( 'R', "data field must be less than or equal to %u", MAX_ATOH_CHARS );

      }
      else
      {
         /* convert ascii data to hex data */
         for ( bufIndex = 0, srcIndex = 0;
               ( bufIndex < sizeof( data ) ) && ( 0 != argv[1][srcIndex] );
               srcIndex += 2, bufIndex++ )
         {
            ( void )atoh( &data[bufIndex], &argv[1][srcIndex] );
         }

         crc_value = CRC_32_Calculate( data, ( dataLen / 2 ) );
         DBG_logPrintf( 'R', "CRC value is : 0x%08x", crc_value );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid, expected format:  'crc <data>' : eg. 'crc 1354AB'" );
   }

   return ( 0 );
}
#if 0 // RA6E1 Bob: This command was removed from original K24 code
/*******************************************************************************
   Function name: DBG_CommandLine_crc16

   Purpose: This command will calculate a crc over a passed in hex string

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - status of this function - currently always 0 (success)
*******************************************************************************/
uint32_t DBG_CommandLine_crc16 ( uint32_t argc, char *argv[] )
{
   uint8_t        data[MAX_ATOH_CHARS];
   uint16_t       dataLen;
   uint16_t       bufIndex;
   uint16_t       srcIndex;
   uint32_t       crc_value;

   DBG_logPrintf( 'R', "Received DBG_CommandLine_crc command" );
   if ( argc == 2 )  /* The number of arguments must be 1 */
   {
      dataLen = ( uint16_t )strlen( argv[1] );
      if ( ( dataLen % 2 ) != 0 )
      {
         DBG_logPrintf( 'R', "data field must be divisible by 2 (2 chars per hex byte) 'crc 135455'" );
      }
      else if ( dataLen > ( MAX_ATOH_CHARS * 2 ) )
      {
         DBG_logPrintf( 'R', "data field must be less than or equal to %u", MAX_ATOH_CHARS );

      }
      else
      {
         /* convert ascii data to hex data */
         for ( bufIndex = 0, srcIndex = 0;
               ( bufIndex < sizeof( data ) ) && ( 0 != argv[1][srcIndex] );
               srcIndex += 2, bufIndex++ )
         {
            ( void )atoh( &data[bufIndex], &argv[1][srcIndex] );
         }

         crc_value = CRC_16_DcuHack( data, ( dataLen / 2 ) );
         DBG_logPrintf( 'R', "CRC value is : 0x%04x", crc_value );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid, expected format:  'crc <data>' : eg. 'crc 1354AB'" );
   }

   return ( 0 );
}
#endif // RA6E1 Bob: This command was removed from original K24 code
#if ( EP == 1 )
/*******************************************************************************
   Function name: DBG_CommandLine_crc16m

   Purpose: This command will calculate a ANSI psem crc over a passed in hex string

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - status of this function - currently always 0 (success)
*******************************************************************************/
static uint32_t DBG_CommandLine_crc16m ( uint32_t argc, char *argv[] )
{
   uint8_t        data[MAX_ATOH_CHARS];
   uint16_t       maxIndex;
   uint16_t       dataLen;
   uint16_t       bufIndex;
   uint16_t       srcIndex;

   DBG_logPrintf( 'R', "Received DBG_CommandLine_crc command" );
   if ( argc == 2 )  /* The number of arguments must be 1 */
   {
      dataLen = ( uint16_t )strlen( argv[1] );
      if ( ( dataLen % 2 ) != 0 )
      {
         DBG_logPrintf( 'R', "data field must be divisible by 2 (2 chars per hex byte) 'crcm 135455'" );
      }
      else if ( dataLen > ( MAX_ATOH_CHARS * 2 ) )
      {
         DBG_logPrintf( 'R', "data field must be less than or equal to %u", MAX_ATOH_CHARS );

      }
      else
      {
         /* convert ascii data to hex data */
         for ( bufIndex = 0, srcIndex = 0;
               ( bufIndex < sizeof( data ) ) && ( 0 != argv[1][srcIndex] );
               srcIndex += 2, bufIndex++ )
         {
            ( void )atoh( &data[bufIndex], &argv[1][srcIndex] );
         }

         CRC16_InitMtr( data );  /* "Consumes" first two bytes.   *//*lint !e772 data is initialized.  */
         maxIndex = dataLen  / 2;
         for ( bufIndex = 2; bufIndex < maxIndex; bufIndex++ )
         {
            CRC16_CalcMtr( data[ bufIndex ] );
         }
         CRC16_CalcMtr( 0 );
         CRC16_CalcMtr( 0 );
         DBG_logPrintf( 'R', "CRC value is : 0x%04x", CRC16_ResultMtr() );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid, expected format:  'crcm <data>' : eg. 'crc ee8020'" );
   }

   return ( 0 );
}
#endif

uint32_t DBG_CommandLine_NwkStart ( uint32_t argc, char *argv[] )
{
   ( void )NWK_StartRequest( NULL );
   return 0;
}

uint32_t DBG_CommandLine_NwkStop ( uint32_t argc, char *argv[] )
{
   ( void )NWK_StopRequest( NULL );
   return 0;
}

uint32_t DBG_CommandLine_NwkReset ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      // No parameters
      ( void )NWK_ResetRequest( eNWK_RESET_STATISTICS, NULL );
      return ( 0 );
   }
   else if ( argc == 2 )
   {
      // One parameter
      if ( strcasecmp( "all", argv[1] ) == 0 )
      {
         ( void )NWK_ResetRequest( eNWK_RESET_ALL,        NULL );
         return ( 0 );
      }
      if ( strcasecmp( "stats", argv[1] ) == 0 )
      {
         ( void )NWK_ResetRequest( eNWK_RESET_STATISTICS, NULL );
         return ( 0 );
      }
   }
   DBG_logPrintf( 'R', "Usage: 'nwkreset start|all'" );
   return ( 0 );
}

uint32_t DBG_CommandLine_NwkGet ( uint32_t argc, char *argv[] )
{
   int option = 0;

   if ( argc > 1 )
   {
      option = ( uint16_t )( atoi( argv[1] ) );
   }

   ( void )NWK_GetRequest( ( NWK_ATTRIBUTES_e )option );
#if 0 // RA6E1 Bob: This command was removed from original K24 code

   switch ( option )
   {
      case  eStackAttr_ipIfInDiscards                     :
         NWK_GetRequest( eStackAttr_ipIfInDiscards                       , NULL );
         break;
      case  eStackAttr_ipIfInErrors                       :
         NWK_GetRequest( eStackAttr_ipIfInErrors                         , NULL );
         break;
      case  eStackAttr_ipIfInMulticastPkts                :
         NWK_GetRequest( eStackAttr_ipIfInMulticastPkts                  , NULL );
         break;
      case  eStackAttr_ipIfInOctets                       :
         NWK_GetRequest( eStackAttr_ipIfInOctets                         , NULL );
         break;
      case  eStackAttr_ipIfInUcastPkts                    :
         NWK_GetRequest( eStackAttr_ipIfInUcastPkts                      , NULL );
         break;
      case  eStackAttr_ipIfInUnknownProtos                :
         NWK_GetRequest( eStackAttr_ipIfInUnknownProtos                  , NULL );
         break;
      case  eStackAttr_ipIfLastResetTime                  :
         NWK_GetRequest( eStackAttr_ipIfLastResetTime                    , NULL );
         break;
      case  eStackAttr_ipIfOutDiscards                    :
         NWK_GetRequest( eStackAttr_ipIfOutDiscards                      , NULL );
         break;
      case  eStackAttr_ipIfOutErrors                      :
         NWK_GetRequest( eStackAttr_ipIfOutErrors                        , NULL );
         break;
      case  eStackAttr_ipIfOutMulticastPkts               :
         NWK_GetRequest( eStackAttr_ipIfOutMulticastPkts                 , NULL );
         break;
      case  eStackAttr_ipIfOutOctets                      :
         NWK_GetRequest( eStackAttr_ipIfOutOctets                        , NULL );
         break;
      case  eStackAttr_ipIfOutUcastPkts                   :
         NWK_GetRequest( eStackAttr_ipIfOutUcastPkts                     , NULL );
         break;
      case  eStackAttr_ipLowerLinkCount                   :
         NWK_GetRequest( eStackAttr_ipLowerLinkCount                     , NULL );
         break;
#if ( EP == 1 )
      case  eStackAttr_ipRoutingMaxPacketCount            :
         NWK_GetRequest( eStackAttr_ipRoutingMaxPacketCount              , NULL );
         break;
      case  eStackAttr_ipRoutingEntryExpirationThreshold  :
         NWK_GetRequest( eStackAttr_ipRoutingEntryExpirationThreshold    , NULL );
         break;
#endif // ( EP == 1 )
      case  eStackAttr_ipHEContext                        :
         NWK_GetRequest( eStackAttr_ipHEContext                         , NULL );
         break;
#if ( HE == 1 )
      case  eStackAttr_ipPacketHistory                    :
         NWK_GetRequest( eStackAttr_ipPacketHistory                      , NULL );
         break;
      case  eStackAttr_ipRoutingHistory                   :
         NWK_GetRequest( eStackAttr_ipRoutingHistory                     , NULL );
         break;
      case  eStackAttr_ipRoutingMaxDcuCount               :
         NWK_GetRequest( eStackAttr_ipRoutingMaxDcuCount                 , NULL );
         break;
      case  eStackAttr_ipRoutingFrequency                 :
         NWK_GetRequest( eStackAttr_ipRoutingFrequency                   , NULL );
         break;
      case  eStackAttr_ipMultipathForwardingEnabled       :
         NWK_GetRequest( eStackAttr_ipMultipathForwardingEnabled         , NULL );
         break;
#endif // ( HE == 1 )
   }
#endif // RA6E1 Bob: This command was removed from original K24 code
   return ( 0 );
}

uint32_t DBG_CommandLine_NwkSet ( uint32_t argc, char *argv[] )
{
// todo: 7/31/2015 12:38:41 PM [BAH] - This is not completed yet
#if 0 // RA6E1 Bob: This command was removed from original K24 code
   int option = 0;
   if ( argc > 1 )
   {
      option = ( uint16_t )( atoi( argv[1] ) );
   }
   ( void )NWK_SetRequest( ( NWK_ATTRIBUTES_e )option , NULL );
#endif // RA6E1 Bob: This command was removed from original K24 code
   return 0;
}

#if 0 // RA6E1 Bob: This command was removed from original K24 code
uint32_t DBG_CommandLine_MacStart ( uint32_t argc, char *argv[] )
{
   ( void )MAC_StartRequest( NULL );
   return 0;
}

uint32_t DBG_CommandLine_MacStop ( uint32_t argc, char *argv[] )
{
   ( void )MAC_StopRequest( NULL );
   return 0;
}

uint32_t DBG_CommandLine_MacGet ( uint32_t argc, char *argv[] )
{
   int option = 0;

   if ( argc > 1 )
   {
      option = ( uint16_t )( atoi( argv[1] ) );
   }

   switch ( option )
   {
      case  0:
         ( void )MAC_GetRequest(    eMacAttr_AckDelayDuration          );
         break;
      case  1:
         ( void )MAC_GetRequest(    eMacAttr_ArqOverflowCount          );
         break;
      case  2:
         ( void )MAC_GetRequest(    eMacAttr_ChannelAccessFailureCount );
         break;
      case  3:
         ( void )MAC_GetRequest(    eMacAttr_CsmaMaxAttempts           );
         break;
      case  4:
         ( void )MAC_GetRequest(    eMacAttr_CsmaMaxBackOffTime        );
         break;
      case  5:
         ( void )MAC_GetRequest(    eMacAttr_CsmaMinBackOffTime        );
         break;
      case  6:
         ( void )MAC_GetRequest(    eMacAttr_CsmaPValue                );
         break;
      case  7:
         ( void )MAC_GetRequest(    eMacAttr_CsmaQuickAbort            );
         break;
      case  8:
         ( void )MAC_GetRequest(    eMacAttr_FailedFcsCount            );
         break;
      case  9:
         ( void )MAC_GetRequest(    eMacAttr_LastResetTime             );
         break;
      case 10:
         ( void )MAC_GetRequest(    eMacAttr_MalformedAckCount         );
         break;
      case 11:
         ( void )MAC_GetRequest(    eMacAttr_MalformedFrameCount       );
         break;
      case 12:
         ( void )MAC_GetRequest(    eMacAttr_NetworkId                 );
         break;
      case 13:
         ( void )MAC_GetRequest(    eMacAttr_PacketId                  );
         break;
      case 14:
         ( void )MAC_GetRequest(    eMacAttr_PacketTimeout             );
         break;
      case 15:
         ( void )MAC_GetRequest(    eMacAttr_AdjacentNwkFrameCount     );
         break;
      case 16:
         ( void )MAC_GetRequest(    eMacAttr_RxFrameCount              );
         break;
      case 17:
         ( void )MAC_GetRequest(    eMacAttr_RxOverflowCount           );
         break;
      case 18:
         ( void )MAC_GetRequest(    eMacAttr_ReassemblyTimeout         );
         break;
      case 19:
         ( void )MAC_GetRequest(    eMacAttr_ReliabilityHighCount      );
         break;
      case 21:
         ( void )MAC_GetRequest(    eMacAttr_ReliabilityMediumCount    );
         break;
      case 22:
         ( void )MAC_GetRequest(    eMacAttr_ReliabilityLowCount       );
         break;
      case 28:
         ( void )MAC_GetRequest(    eMacAttr_TransactionOverflowCount  );
         break;
      case 29:
         ( void )MAC_GetRequest(    eMacAttr_TxFrameCount              );
         break;
      default:
         break;
   }
   return 0;
}

uint32_t DBG_CommandLine_MacSet ( uint32_t argc, char *argv[] )
{
   DBG_logPrintf( 'R', "Not implemented'" );
   return 0;
}
#endif // RA6E1 Bob: This command was removed from original K24 code

uint32_t DBG_CommandLine_MacReset ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      // No parameters
      ( void )MAC_ResetRequest( eMAC_RESET_STATISTICS, NULL );
      return ( 0 );
   }
   else if ( argc == 2 )
   {
      // One parameter
      if ( strcasecmp( "all", argv[1] ) == 0 )
      {
         ( void )MAC_ResetRequest( eMAC_RESET_ALL,        NULL );
         return ( 0 );
      }
      if ( strcasecmp( "stats", argv[1] ) == 0 )
      {
         ( void )MAC_ResetRequest( eMAC_RESET_STATISTICS, NULL );
         return ( 0 );
      }
   }
   DBG_logPrintf( 'R', "Usage: 'macreset start|all'" );
   return ( 0 );
}

#if 0 // RA6E1 Bob: This command was removed from original K24 code
uint32_t DBG_CommandLine_MacFlush ( uint32_t argc, char *argv[] )
{
   ( void )MAC_FlushRequest( NULL );
   return 0;
}

uint32_t DBG_CommandLine_MacPurge ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      uint16_t handle = ( uint16_t )( atoi( argv[1] ) );
      ( void )MAC_PurgeRequest( handle, NULL );
      return 0;
   }
   DBG_logPrintf( 'R', "Usage: 'macpurge x'" );
   return 0;
}
#endif // RA6E1 Bob: This command was removed from original K24 code

#if 0 // RA6E1 Bob: This command was removed from original K24 code
/*!
 *
 * \fn DBG_CommandLine_SM_Start
 *
 * \brief This function is used to start the stack.
 *
 * \param option 0-N
 *
 * \return     none
 *
 * \details
 *
 *  smstart option <cr>
 *
 *    0 - eSM_START_STANDARD
 *    1 - eSM_START_MUTE
 *    2 - eSM_START_DEAF
 *    3 - eSM_START_LOW_POWER
 *
 */
uint32_t DBG_CommandLine_SM_Start ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      uint16_t option = ( uint16_t )( atoi( argv[1] ) );
      ( void )SM_StartRequest( (SM_START_e) option, NULL );
      return 0;
   }
   DBG_logPrintf( 'R', "Usage: 'smstart x'" );
   return 0;
}

/*!
 * \fn DBG_CommandLine_SM_Stop
 *
 * \brief This function is used to stop the stack.
 *
 * \param
 * \param
 *
 * \return     none
 *
 * \details
 *
 *  smstop <cr>
 *
 */
uint32_t DBG_CommandLine_SM_Stop ( uint32_t argc, char *argv[] )
{
   ( void )SM_StopRequest( NULL );
   return 0;
}


/*!
 * \fn DBG_CommandLine_SM_Set
 *
 * \brief This function is used to ...
 *
 * \param
 * \param
 *
 * \return
 *
 * \details
 *
 *  smget option value
 *
 */
uint32_t DBG_CommandLine_SM_Set ( uint32_t argc, char *argv[] )
{
   if ( argc > 2 )
   {
      SM_ATTRIBUTES_u val;

      SM_ATTRIBUTES_e option;
      option = ( SM_ATTRIBUTES_e )( atoi( argv[1] ) );

      val.SmMode = (SM_MODE_e) atoi( argv[2] );

      // todo: 5/27/2016 12:43:44 PM [BAH]
//      val.SmLinkCount                 =     uint8_t
//      val.SmMode                      =     uint16_t
//      val.SmModeMaxTransitionAttempts =     uint16_t
//      val.SmStartWaitTime             =     uint16_t
//      val.SmStatsCaptureTime          =     uint16_t
//      val.SmStatsConfig[1]            =     uint16_t
//      val.SmStatsEnable               =     bool
//      val.SmStopWaitTime              =     uint16_t
//      val.SmQueueLevel                =     uint16_t

      ( void )SM_SetRequest( ( SM_ATTRIBUTES_e )option, &val );
      return 0;
   }else{
      DBG_logPrintf( 'R', "Usage: 'smset option value'" );
   }
   return 0;
}

/*!
 * \fn DBG_CommandLine_SM_Get
 *
 * \brief This function is used to ...
 *
 * \param
 * \param
 *
 * \return     none
 *
 * \details
 *
 *  smget option <cr>
 *
 */
uint32_t DBG_CommandLine_SM_Get ( uint32_t argc, char *argv[] )
{
   if ( argc > 1 )
   {
      SM_ATTRIBUTES_e option;
      option = ( SM_ATTRIBUTES_e )( atoi( argv[1] ) );
      ( void )SM_GetRequest( option );
   }else{
      DBG_logPrintf( 'R', "Usage: 'smget value'" );
   }
   return 0;
}
#endif // RA6E1 Bob: This command was removed from original K24 code

/**
Gets or sets the passive network activity timeout.

@param argc Number of arguments
@param argv List of arguments

@return 0
*/
static uint32_t DBG_CommandLine_SM_NwPassiveActTimeout(uint32_t argc, char *argv[])
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
      DBG_logPrintf('R', "%s %u", argv[0], resp.val.SmActivityTimeout);
   }
   else
   {
      DBG_logPrintf('R', "%s Error", argv[0]);
   }

   return 0;
}

/**
Gets or sets the active network activity timeout.

@param argc Number of arguments
@param argv List of arguments

@return 0
*/
static uint32_t DBG_CommandLine_SM_NwActiveActTimeout(uint32_t argc, char *argv[])
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
      DBG_logPrintf('R', "%s %u", argv[0], resp.val.SmActivityTimeout);
   }
   else
   {
      DBG_logPrintf('R', "%s Error", argv[0]);
   }

   return 0;
}
#if ( BOB_IS_LAZY == 1 )
uint32_t DBG_ResetAllStats ( uint32_t argc, char *argv[] )
{
   (void) DBG_CommandLine_PhyReset ( (uint32_t)1, argv );
   (void) DBG_CommandLine_MacReset ( (uint32_t)1, argv );
   (void) DBG_CommandLine_NwkReset ( (uint32_t)1, argv );
   return ( 0 );
}
uint32_t DBG_ShowAllStats  ( uint32_t argc, char *argv[] )
{
   (void) DBG_CommandLine_PhyStats ( (uint32_t)1, argv );
   (void) DBG_CommandLine_MacStats ( (uint32_t)1, argv );
   return ( 0 );
}
#endif

#if 0 // RA6E1 Bob: This command was removed from original K24 code
/*!
 * \fn DBG_CommandLine_PhyStart
 *
 * \brief This function is used to start the phy
 *
 * \param
 * \param
 *
 * \return     none
 *
 * \details
 *
 *  phystart  option
 *     0 - ePHY_START_STANDBY
 *     1 - ePHY_START_READY
 *     2 - ePHY_START_READY_TX
 *     3 - ePHY_START_READY_RX
 *
 */
uint32_t DBG_CommandLine_PhyStart ( uint32_t argc, char *argv[] )
{
   if ( argc == 2 )
   {
      uint16_t mode = ( uint16_t )( atoi( argv[1] ) );
      ( void )PHY_StartRequest( (PHY_START_e) mode,   NULL );
      return 0;
   }
   DBG_logPrintf( 'R', "Usage: 'phystart mode'" );
   return 0;
}
/*!
 * \fn DBG_CommandLine_PhyStop
 *
 * \brief This function is used to stop the phy
 *
 */
uint32_t DBG_CommandLine_PhyStop ( uint32_t argc, char *argv[] )
{
   (void) PHY_StopRequest( NULL );
   return 0;
}
#endif // RA6E1 Bob: This command was removed from original K24 code

/*!
 * \fn DBG_CommandLine_PhyReset
 *
 * \brief This function is used to reset the phy
 *
 */
uint32_t DBG_CommandLine_PhyReset ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      // No parameters
      (void) PHY_ResetRequest( ePHY_RESET_STATISTICS, NULL );
      return ( 0 );
   }
   else if ( argc == 2 )
   {
      // One parameter
      if ( strcasecmp( "all", argv[1] ) == 0 )
      {
         (void) PHY_ResetRequest( ePHY_RESET_ALL,        NULL );
         return ( 0 );
      }
      if ( strcasecmp( "stats", argv[1] ) == 0 )
      {
         (void) PHY_ResetRequest( ePHY_RESET_STATISTICS, NULL );
         return ( 0 );
      }
   }
   DBG_logPrintf( 'R', "Usage: 'phyreset stats|all'" );
   return ( 0 );
}


/******************************************************************************

   Function Name: DBG_CommandLine_MacTimeCmd ( uint32_t argc, char *argv[] )

   Purpose: This function is used to send a time_set or time_req command

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:  time set <cr> (DCU)
           time req <cr> (EP)

******************************************************************************/
uint32_t DBG_CommandLine_MacTimeCmd ( uint32_t argc, char *argv[] )
{
   char* option = "mac_time";
   if ( argc > 1 )
   {
      if ( strcasecmp( "set", argv[1] ) == 0 )
      {
         if( eMAC_TIME_SUCCESS != MAC_TimePush_Request(BROADCAST_MODE, NULL, NULL, 0 ) )
         {
            INFO_printf( "mac_time set : Not executed or pending" );
         }
      }
      else
#if ( EP == 1 )
         if ( strcasecmp( "req", argv[1] ) == 0 )
         {
            if( !MAC_TimeQueryReq() )
            {
               INFO_printf( "mac_time req : Not executed" );
            }
         }
         else
#endif
         {
            INFO_printf( "mac_time : Invalid option" );
         }
   }
   else
   {
      DBG_logPrintf( 'R', "Usage:", option );
#if ( DCU == 1 )
      DBG_logPrintf( 'R', "%s set <cr>", option );
#endif
#if ( EP == 1 )
      DBG_logPrintf( 'R', "%s req <cr>", option );
#endif
   }
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_MacPingCmd ( uint32_t argc, char *argv[] )

   Purpose: This function is used to send a ping request to an EP via the RF link

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:   ping  handle reset_count rsp_mode dst_addr <cr> (DCU)
            ping  0 0 0 0.1.0.0.5 <CR>

******************************************************************************/
uint32_t DBG_CommandLine_MacPingCmd ( uint32_t argc, char *argv[] )
{
   uint16_t handle        = 0;
   uint8_t  rsp_mode      = 0; // 0=Unicast, 1=Broadcast
   uint8_t  reset_counter = 0; // Reset Count Option 0 = No, 1 = Yes
#if BOB_PADDED_PING == 1
   uint16_t extra_DL_bytes = 0, extra_UL_bytes = 0; // No extra bytes by default
   if ( argc > 5 )
   {
      extra_DL_bytes   = ( uint16_t )atoi( argv[5] );
   }
   if ( argc > 6 )
   {
      extra_UL_bytes   = ( uint16_t )atoi( argv[6] );
   }
   if ( ( 5 <= argc ) && ( argc <= 7 ) )
#else
   if( argc == 5 )
#endif
   {
      handle        = ( uint16_t )atoi( argv[1] );
      reset_counter = ( uint8_t ) atoi( argv[2] );
      rsp_mode      = ( uint8_t ) atoi( argv[3] );

      // Destination EUI Address
      eui_addr_t eui_addr = { 0 };
      char delimiter[] = ":.";   // Allows delimiter to be ':' or '.'
      uint32_t num_tokens = 0;
      char* pch;

      pch = strtok( argv[4], delimiter );
      while ( pch != NULL )
      {
         // Convert the token to address bytes
         ( void )sscanf( pch, "%02hhx", &eui_addr[num_tokens] );
         num_tokens++;
         if( num_tokens > sizeof( eui_addr ) )
         {
            // no more room!
            break;
         }
         pch = strtok ( NULL, delimiter );
      }

      if( num_tokens == sizeof( eui_addr_t ) )
      {
#if ( BOB_THIN_DEBUG == 0 )
         DBG_logPrintf( 'R', "rsp_mode=%d reset_counter=%d", rsp_mode, reset_counter );
#endif
#if BOB_PADDED_PING == 1
         ( void )MAC_PingRequest( handle, eui_addr, reset_counter, rsp_mode, extra_DL_bytes, extra_UL_bytes );
#else
         ( void )MAC_PingRequest( handle, eui_addr, reset_counter, rsp_mode );
#endif
      }
      else
      {
         // error
#if BOB_PADDED_PING == 1
         DBG_logPrintf( 'R', "ping <handle> <reset_flag> <rsp_mode> <dst_eui_addr> <extra_DL_bytes> <extra_UL_bytes>" );
#else
         DBG_logPrintf( 'R', "ping <handle> <reset_flag> <rsp_mode> <dst_eui_addr>" );
#endif
      }

   }
   else
   {
#if BOB_PADDED_PING == 1
      DBG_logPrintf( 'R', "ping <handle> <reset_flag> <rsp_mode> <dst_eui_addr> <extra_DL_bytes> <extra_UL_bytes>" );
#else
      DBG_logPrintf( 'R', "ping <handle> <reset_flag> <rsp_mode> <dst_eui_addr>" );
#endif
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_TimeSync ( uint32_t argc, char *argv[] )

   Purpose: This function is used to set/get the Time Sync Parameters

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_TimeSync ( uint32_t argc, char *argv[] )
{
   struct
   {
      char*            name;
   } field[] =
     {
      {"leap"},
      {"precision" },
      {"offset" },
      {"period" },
      {"start" },
      {"source" },
   };

   if ( argc > 1 )
   {
      if ( strcasecmp( "get", argv[1] ) == 0 )
      {
         TIME_SYNC_Parameters_Print();
      }
      else if ( strcasecmp( "set", argv[1] ) == 0 )
      {
         if( argc >= 4 )
         {
            int index = 0;
            do
            {
               if ( strcasecmp( field[index].name, argv[2] ) == 0 )
               {
                  uint32_t value;
                  value = ( uint32_t )atoi( argv[3] );
                  if ( index == 2 )
                  {
                     (void)TIME_SYNC_TimeSetMaxOffset_Set((uint16_t)value);
                  }
                  else if ( index == 3 )
                  {
                     (void)TIME_SYNC_TimeSetPeriod_Set(value);
                  }
                  else if ( index == 4 )
                  {
                     (void)TIME_SYNC_TimeSetStart_Set(value);
                  }
                  else if ( index == 5 )
                  {
                     (void)TIME_SYNC_TimeSource_Set((uint8_t)value);
                  }
                  TIME_SYNC_Parameters_Print();
                  break;
               }
            }
            while ( index++ < 5 );
            if( index > 5 )
            {
               INFO_printf( "error" );
            }
         }
         else
         {
            INFO_printf( "error" );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "mac_tsync get <cr>" );
      DBG_logPrintf( 'R', "mac_tsync set %s|%s|%s|%s|%s val<cr>",
                     field[0].name, field[1].name, field[2].name, field[3].name, field[4].name );
   }
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_MacAddr

   Purpose: This function is used to get/set the mac extension address (40-bits)
            Usage: macaddr get <cr>
            Usage: macaddr get 01:02:03:04:aa <cr>

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes: Changing the mac address will not be allowed when the security chip is enabled!

******************************************************************************/
uint32_t DBG_CommandLine_MacAddr ( uint32_t argc, char *argv[] )
{
   bool bStatus = ( bool )false;
   if ( argc > 1 )
   {
      ( void )ecc108_open();
      if ( strcasecmp( "set", argv[1] ) == 0 )
      {
         xid_addr_t xid_addr;
         char delimiter[] = ":.";   // Allows delimiter to be ':' or '.'
         uint16_t num_tokens = 0;
         char* pch;

         pch = strtok( argv[2], delimiter );
         while ( pch != NULL )
         {
            // Convert the token to address bytes
            ( void )sscanf( pch, "%02hhx", &xid_addr[num_tokens] );
            num_tokens++;
            if( num_tokens > ( uint16_t )sizeof( xid_addr ) )
            {
               // no more room!
               break;
            }
            pch = strtok ( NULL, delimiter );
         }

         if( num_tokens == ( uint16_t )sizeof( xid_addr_t ) )
         {
            /*lint -e{545} & operator needed by compiler in next statement */
            MAC_XID_Address_Set( ( const xid_addr_t * )&xid_addr ); /*lint !e772 num_tokens correct->xid_addr init'd */
            bStatus = ( bool )true;
         }
         else
         {
            // need to print the usage
            DBG_logPrintf( 'R', "parameter error!" );
         }
      }
      else if ( strcasecmp( "get", argv[1] ) == 0 )
      {
         bStatus = ( bool )true;
      }
      ecc108_close();
   }
   if( bStatus == ( bool )true )
   {
      // Successfull
      eui_addr_t addr;
      /*lint -e{545} & operator needed by compiler in next statement */
      MAC_EUI_Address_Get( ( eui_addr_t * )&addr );

      // Print the Extension Id
      DBG_logPrintf( 'R', "macaddr=%02x:%02x:%02x:%02x:%02x",
                     addr[3], addr[4], addr[5], addr[6], addr[7] );

      // The Full Extended Unique Identifier
      DBG_logPrintf( 'R', "EUI=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2],
                     addr[3], addr[4], addr[5], addr[6], addr[7] );
   }
   else
   {
      // Error, so print the usage message
      DBG_logPrintf( 'R', "usage: macaddr get" );
      DBG_logPrintf( 'R', "usage: macaddr set 00.00.00.00.00" );
   }
   return ( 0 );
} /* end DBG_CommandLine_MacAddr () */

#if 0 // RA6E1 Bob: This command was removed from original K24 code
/******************************************************************************

   Function Name: DBG_CommandLine_GetPhyMaxTxPayload

   Purpose: This function will get the maximum TX phy payload length

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_GetPhyMaxTxPayload ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      DBG_logPrintf( 'R', "phymaxlen = %u", PHY_GetMaxTxPayload() );
   }
   else
   {
      DBG_logPrintf( 'R', "Usage: 'phymaxlen' get phy maximum TX payload len" );
   }

   return ( 0 );
}
#endif // RA6E1 Bob: This command was removed from original K24 code

/******************************************************************************

   Function Name: DBG_CommandLine_MacTxPause

   Purpose: This function will pause transmission for packet QoS testing

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_MacTxPause ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      MAC_PauseTx ( ( bool )true );
   }
   else
   {
      DBG_logPrintf( 'R', "ERROR - too many arguments:  Usage:  'mactxpause'" );
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_MacTxUnPause

   Purpose: This function will unpause transmission (for packet QoS testing)

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_MacTxUnPause ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      MAC_PauseTx ( ( bool )false );
   }
   else
   {
      DBG_logPrintf( 'R', "ERROR - too many arguments:  Usage:  'mactxunpause'" );
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_PhyStats

   Purpose: Used to print the PHY Statistics

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_PhyStats ( uint32_t argc, char *argv[] )
{
   PHY_Stats ();
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_PhyConfig

   Purpose: Used to print the PHY Configuration

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_PhyConfig ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      // No parameters
      PHY_ConfigPrint();
      return ( 0 );
   }
   DBG_logPrintf( 'R', "Usage:  'phyconfig'" );
   return ( 0 );
}



/******************************************************************************

   Function Name: DBG_CommandLine_SM_Stats

   Purpose: Used to print the SM Statistics

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_SM_Stats ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      // No parameters
      SM_Stats ( ( bool )false );
      return ( 0 );
   }

   if ( argc == 2 )
   {
      // One parameter
      if ( strcasecmp( "y", argv[1] ) == 0 )
      {
         SM_Stats ( ( bool )true );
         return ( 0 );
      }
      if ( strcasecmp( "n", argv[1] ) == 0 )
      {
         SM_Stats ( ( bool )false );
         return ( 0 );
      }
   }
   DBG_logPrintf( 'R', "Usage:  'smstats y|n'" );
   return ( 0 );
}



/******************************************************************************

   Function Name: DBG_CommandLine_SM_Config

   Purpose: Used to print the SM Configuration

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_SM_Config ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      // No parameters
      SM_ConfigPrint();
      return ( 0 );
   }
   DBG_logPrintf( 'R', "Usage:  'smconfig'" );
   return ( 0 );
}


/******************************************************************************

   Function Name: DBG_CommandLine_MacStats

   Purpose: Used to print the MAC Statistics

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_MacStats ( uint32_t argc, char *argv[] )
{
   MAC_Stats ();
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_MacConfig

   Purpose: Used to print the MAC Configuration

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_MacConfig ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      // No parameters
      MAC_ConfigPrint();
      return ( 0 );
   }
   DBG_logPrintf( 'R', "Usage:  'macconfig'" );

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_NwkStats

   Purpose: Used to print the MAC Statistics

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_NwkStats ( uint32_t argc, char *argv[] )
{
   NWK_Stats ( );
   return ( 0 );
}

#if (USE_DTLS == 1)
/******************************************************************************

   Function Name: DBG_CommandLine_DtlsStats

   Purpose: Used to print the DTLS Statistics

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_DtlsStats ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      // No parameters
      DTLS_Stats ( ( bool )false );
      return ( 0 );
   }

   if ( argc == 2 )
   {
      // One parameter
      if ( strcasecmp( "y", argv[1] ) == 0 )
      {
         DTLS_Stats ( ( bool )true );
         return ( 0 );
      }
      if ( strcasecmp( "n", argv[1] ) == 0 )
      {
         DTLS_Stats ( ( bool )false );
         return ( 0 );
      }
   }
   DBG_logPrintf( 'R', "Usage:  'dtlsstats y|n'" );
   return ( 0 );
}
#endif // (USE_DTLS == 1)

#if (USE_MTLS == 1)
/******************************************************************************

   Function Name: DBG_CommandLine_mtlsStats

   Purpose: Used to print the MTLS Statistics

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_mtlsStats ( uint32_t argc, char *argv[] )
{

   if ( ( argc > 1 ) && ( strcasecmp( argv[ 1 ], "reset" ) == 0 ) )
   {
      if ( eSUCCESS != MTLS_Reset() )
      {
         DBG_logPrintf( 'E', "Failed to reset mtls readonly stats!" );
      }
   }
   MTLS_Stats();

   return ( 0 );
}
#endif // (USE_MTLS == 1)

/******************************************************************************
   Function Name: DBG_CommandLine_PWR_BrownOut

   Purpose: Signal a Brown Out to the Power Task

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

******************************************************************************/
#if 0 // RA6E1 Bob: This command was removed from original K24 code
uint32_t DBG_CommandLine_PWR_BrownOut( uint32_t argc, char *argv[] )
{
   isr_brownOut();
   return( 0 );
}
#endif

/******************************************************************************

   Function Name: DBG_CommandLine_usbaddr

   Purpose: Display USB address

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

******************************************************************************/
#if ( USE_USB_MFG != 0 )
static uint32_t DBG_CommandLine_usbaddr( uint32_t argc, char *argv[] )
{
   DBG_logPrintf( 'I', "usb_addr %hhu", USB0_ADDR );
   return 0;
}
#endif //  ( USE_USB_MFG != 0 )

/******************************************************************************

   Function Name: DBG_CommandLine_BDT

   Purpose: Display USB BDT, or specific buffer.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

******************************************************************************/
#if ( USE_USB_MFG != 0 )
static uint32_t DBG_CommandLine_BDT( uint32_t argc, char *argv[] )
{
   char           bufDesc[ 16 ];
   USB_MemMapPtr  usb0 = USB0_BASE_PTR;
   uint8_t        *bdt;       /* Pointer to USB BDT   */
   uint8_t        *buffer;    /* Pointer to endpoint buffer   */
   uint8_t        bdtCount;   /* Steps thru argv values  */
   uint8_t        bdtId;      /* argv value converted to hex   */
   uint8_t        byteCount;  /* Number of bytes sent/rec'd in each EP buffer */
   uint8_t        *rxTx;
   uint8_t        bdtPage1 = usb0->BDTPAGE1;    // Quiet compiler warning on usage of volatile usb0. BDT location does not change.
   uint8_t        bdtPage2 = usb0->BDTPAGE2;    // Quiet compiler warning on usage of volatile usb0. BDT location does not change.
   uint8_t        bdtPage3 = usb0->BDTPAGE3;    // Quiet compiler warning on usage of volatile usb0. BDT location does not change.
   bdt = (uint8_t *)( ( bdtPage3 << 24U ) | ( bdtPage2 << 16U ) | ( bdtPage1 << 8U ) );
   DBG_logPrintf( 'I', "BDT @ 0x%08X", bdt );

   if ( argc == 1 )  /* Dump BDT table */
   {
      DBG_logPrintf( 'I', "EP   BC OWN DATA0/1 KEEP NINC DTS BDT_STALL buffer" );
      for ( uint8_t ep = 0; ep < 16 * 2 * 2; ep++ ) /* 16 bidirectional end points with even/odd bdts for each direction   */
      {
         /*           EP     BC    OWN   D0/1  KEEP  NINC DTS   STALL buffer  */
         DBG_logPrintf( 'I', "%2hhu %4hu %3hhx %7hhx %4hhx %4hhx %3hhx %9hhx 0x%P",
               ( ep/4 ),
               (*(uint32_t *)(void *)bdt >> 16) & 0x3ff, /*  BC      bits 25:16 */
               (*bdt >>  7)                     & 0x001, /*  Own     bit  7 */
               (*bdt >>  6)                     & 0x001, /*  data0/1 bit  6 */
               (*bdt >>  5)                     & 0x001, /*  keep    bit  5 */
               (*bdt >>  4)                     & 0x001, /*  NINC    bit  4 */
               (*bdt >>  3)                     & 0x001, /*  DTS     bit  3 */
               (*bdt >>  2)                     & 0x001, /*  STALL   bit  2 */
               *( uint32_t *)(void *)( bdt + 4 ) );      /*  buffer address */
         bdt += 8U;                                      /* Skip to next BDT (2 uint32_t locations */
      }
   }
   else
   {
      for ( bdtCount = 0; bdtCount < ( argc - 1 ); bdtCount++ )
      {
         bdtId = (uint8_t)strtoul( argv[ bdtCount + 1 ], NULL, 0 );
         buffer = bdt + ( bdtId * 32 );   /* 4 8 byte buffers per EP ( TX, RX, EVEN, ODD )   */

         for ( uint8_t bdtBuf = 0; bdtBuf < 4; bdtBuf++ )
         {
            (void)snprintf( bufDesc, (int32_t)sizeof(bufDesc), "EP%hhu, %s, %-4s ", bdtId, ( bdtBuf < 2 ) ? "RX": "TX" , ( bdtBuf % 2 == 0 ) ? "EVEN": "ODD" );
            DBG_logPrintHex ( 'I', bufDesc, buffer, 8 );
            byteCount = (uint8_t)max( ( (*(uint32_t *)(void *)bdt >> 16) & 0x3ff ), 8 );
            buffer += 4;
            rxTx = (uint8_t *)(*(uint32_t *)(void *)buffer); /* Extract pointer to data buffer   */
            /* Verify buffer address in range of internal SRAM  */
            if (  ( rxTx >= (uint8_t *)__INTERNAL_SRAM_BASE ) &&
                  ( rxTx < (uint8_t *)(  (uint32_t)__INTERNAL_SRAM_BASE + (uint32_t)__INTERNAL_SRAM_SIZE ) ) )
            {
               DBG_logPrintHex( 'I', "buffer data:  ", rxTx, byteCount );
            }
            else
            {
               DBG_logPrintf( 'I', "Invalid buffer pointer (%P)", rxTx );
            }
            buffer += 4;
         }
         DBG_printf( "" );
      }
   }
   return( 0 );
}
#endif // ( USE_USB_MFG != 0 )

#if (EP == 1)
/******************************************************************************

   Function Name: DBG_CommandLine_PWR_BoostTest

   Purpose: Measure voltage drop on super cap during 1 second of boost.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_PWR_BoostTest( uint32_t argc, char *argv[] )
{
   float fSuperCapV[2];
   char floatStr[3][PRINT_FLOAT_SIZE];

   PWR_USE_BOOST();

   fSuperCapV[0] = ADC_Get_SC_Voltage();
   OS_TASK_Sleep( 1000 );
   fSuperCapV[1] = ADC_Get_SC_Voltage();

   PWR_USE_LDO();

   DBG_logPrintf( 'R', "Super Cap Voltage Drop: %s -> %s, %s Ws",
                  DBG_printFloat( floatStr[0], fSuperCapV[0], 6 ),
                  DBG_printFloat( floatStr[1], fSuperCapV[1], 6 ),
                  DBG_printFloat( floatStr[2],
                                  ( ( fSuperCapV[0]*fSuperCapV[0] ) - ( fSuperCapV[1]*fSuperCapV[1] ) ) * 5, 6 ) );

   return( 0 );
}
/******************************************************************************

   Function Name: DBG_CommandLine_PWR_BoostMode

   Purpose: Turn on/off the boost regulator to allow noise burst/Rx sensitivity testing.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:   External power supply must be provided or unit will fail.

******************************************************************************/
static uint32_t DBG_CommandLine_PWR_BoostMode( uint32_t argc, char *argv[] )
{
   bool boost;

   if ( argc < 2 )
   {
      DBG_logPrintf( 'R', "Usage boostmode on|off" );
   }
   else
   {
      if ( strcasecmp( argv[ 1 ], "on" ) == 0 )
      {
         boost = (bool)true;
         PWR_USE_BOOST();
      }
      else
      {
         boost = (bool)false;
         PWR_USE_LDO();
      }
      DBG_logPrintf( 'R', "Boost supply %s.", boost ? "on" : "off" );
   }
   return( 0 );
}
#endif // (EP == 1)

#if (EP == 1)
/******************************************************************************

   Function Name: DBG_CommandLine_PWR_SuperCap

   Purpose: Measure voltage on supercap.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_PWR_SuperCap( uint32_t argc, char *argv[] )
{
   float fSuperCapV;
   char floatStr[PRINT_FLOAT_SIZE];

   fSuperCapV = ADC_Get_SC_Voltage();
   DBG_logPrintf( 'R', "Super Cap Voltage: %s", DBG_printFloat( floatStr, fSuperCapV, 6 ) );

   return( 0 );
}
#endif // (EP == 1)

/******************************************************************************

   Function Name: DBG_CommandLine_ds

   Purpose: Exercise the remote disconnect switch

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function

   Notes:

******************************************************************************/
#if ENABLE_HMC_TASKS
#if ( REMOTE_DISCONNECT == 1 )
static const HEEP_APPHDR_t CloseRelay =
{
   .TransactionType      = ( uint8_t )TT_REQUEST,
   .Resource             = ( uint8_t )rc,
  {.Method_Status        = ( uint8_t )method_put },
  {.Req_Resp_ID          = ( uint8_t )0 },
   .qos                  = ( uint8_t )0,
   .callback             = NULL,
   .appSecurityAuthMode  = ( uint8_t )0
};
uint32_t DBG_CommandLine_ds( uint32_t argc, char *argv[] )
{
   uint16_t dummy=0;
   uint32_t retval = 1;
   if ( argc >= 2 )
   {
      HEEP_APPHDR_t   heepHdr;    /* Message header information */
      ExchangeWithID payloadBuf;    /* Request information  */    /* Request information  */
      ( void )memset( ( void * )&payloadBuf, 0, sizeof( payloadBuf ) );
      ( void )memcpy( ( uint8_t * )&heepHdr, ( uint8_t * )&CloseRelay, sizeof( heepHdr ) );
      if ( strcasecmp( "close", argv[1] ) == 0 )
      {
         payloadBuf.eventType = ( EDEventType )BIG_ENDIAN_16BIT( electricMeterRCDSwitchConnect );
         payloadBuf.eventRdgQty.bits.eventQty = 1;
         retval = 0;
      }
      else if ( strcasecmp( "open", argv[1] ) == 0 )
      {
         payloadBuf.eventType = ( EDEventType )BIG_ENDIAN_16BIT( electricMeterRCDSwitchDisconnect );
         payloadBuf.eventRdgQty.bits.eventQty = 1;
         retval = 0;
      }
      if ( argc > 2 )      /* User specify the message id?  */
      {
         heepHdr.Req_Resp_ID = ( uint8_t )atoi( argv[ 2 ] );
      }
      if ( 0 == retval )
      {
         HMC_DS_ExecuteSD( &heepHdr, &payloadBuf, dummy );
      }
   }
   return retval;
}
#endif // ( REMOTE_DISCONNECT == 1 )
#endif // ENABLE_HMC_TASKS

/******************************************************************************

   Function Name: printErr

   Purpose: Print the word ERROR. This is used to reduce code space

   Arguments: none

   Returns: none

   Notes:

******************************************************************************/
static void printErr(void)
{
   DBG_logPrintf( 'R', "ERROR");
}
/******************************************************************************

   Function Name: DBG_CommandLine_SetFreq

   Purpose: This function sets a list of available frequencies

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_SetFreq ( uint32_t argc, char *argv[] )
{
   uint32_t i; // loop counter
   uint32_t locationIndex = 0;
   uint32_t freq; // Frequency between 450 and 470 MHz
   uint16_t Channel; // Frequency translates into a channel between 0-3200 //lint !e578  Declaration of symbol 'channel' hides symbol 'channel'

   // No parameters
   if ( argc == 1 )
   {
#if ( FULL_USER_INTERFACE == 1 )
      DBG_printf( "setfreq sets a list of available frequencies:" );
      DBG_printf( "usage: setfreq locationIndex freq|channel [freq|channel] [...]" );
      DBG_printf( "       locationIndex (0-31) is a starting index into an array of frequencies" );
      DBG_printf( "       freq is betwen 450000000 and 470000000 Hz" );
      DBG_printf( "       channel is between 0-3200 (0 is 450 MHz, 1 is 450.00625 MHz, etc)" );
      DBG_printf( "       To delete a frequency from the list use frequency 999999999" );
      DBG_printf( "       or channel 9999." );
#endif
      return ( 0 );
   }

   // One parameter
   if ( argc == 2 )
   {
#if ( FULL_USER_INTERFACE == 1 )
      DBG_logPrintf( 'R', "ERROR - Not enough arguments. At least one frequency or channel must be provided" );
#else
      printErr();
#endif
      return ( 0 );
   }

   // Many parameters. Validate locationIndex
   if ( argc > 2 )
   {
      locationIndex = ( uint32_t )atoi( argv[1] );

      // Check for valid range
      if ( locationIndex >= PHY_MAX_CHANNEL )
      {
#if ( FULL_USER_INTERFACE == 1 )
         DBG_logPrintf( 'R', "ERROR - locationIndex is invalid. Must be between 0-31" );
#else
         printErr();
#endif
         return ( 0 );
      }
   }

   // Notice how locationIndex is incremented in the loop
   for ( i = 2; i < argc; i++, locationIndex++ )
   {
      // Validate locationIndex
      if ( locationIndex >= PHY_MAX_CHANNEL )
      {
#if ( FULL_USER_INTERFACE == 1 )
         DBG_logPrintf( 'R', "locationIndex is out of range. Further frequencies ignored" );
#else
         printErr();
#endif
         return ( 0 );
      }

      // Validate as frequency
      freq    = ( uint32_t )atoi( argv[i] );
      Channel = ( uint16_t )atoi( argv[i] );
      if ( ( freq >= PHY_MIN_FREQ ) && ( freq <= PHY_MAX_FREQ ) )
      {
         // Convert frequency into a channel
         Channel = FREQ_TO_CHANNEL( freq );

         if ( CHANNEL_TO_FREQ( Channel ) != freq )
         {
#if ( FULL_USER_INTERFACE == 1 )
            DBG_logPrintf( 'R', "ERROR - Invalid frequency %u for locationIndex %u", freq, locationIndex );
#else
            printErr();
#endif
            continue;
         }
         // Validate as delete frequency
      }
      else if ( freq == PHY_INVALID_FREQ )
      {
         Channel = DELETE_CHANNEL;
         // Validate as channel
      }
      else if ( ( Channel > PHY_NUM_CHANNELS ) && ( Channel != DELETE_CHANNEL ) )
      {
#if ( FULL_USER_INTERFACE == 1 )
         DBG_logPrintf( 'R', "ERROR - Invalid channel %u for locationIndex %u", Channel, locationIndex );
#else
         printErr();
#endif
         continue;
      }

      // Program that frequency
      if ( Channel != DELETE_CHANNEL )
      {
         DBG_printf( "Programmed locationIndex %u with %uMHz (channel %u)",
                     locationIndex, CHANNEL_TO_FREQ( Channel ), Channel );
      }
      else
      {
         // Check if current channel that we are going to delete is not in the TX or RX list.
         (void)PHY_Channel_Get( ( uint8_t )locationIndex, &Channel );
         if ( PHY_IsRXChannelValid( Channel ) )
         {
            DBG_logPrintf( 'R', "ERROR - Remove channel %u from RX list first.", Channel );
            continue;
         }

         if ( PHY_IsTXChannelValid( Channel ) )
         {
            DBG_logPrintf( 'R', "ERROR - Remove channel %u from TX list first.", Channel );
            continue;
         }
         DBG_printf( "Deleted channel %u at locationIndex %u", Channel, locationIndex );
         Channel = PHY_INVALID_CHANNEL;
      }
      (void)PHY_Channel_Set( ( uint8_t )locationIndex, Channel );
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_GetFreq

   Purpose: This function will print a list of available frequencies

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_GetFreq ( uint32_t argc, char *argv[] )
{
   uint32_t i; // loop counter
   uint16_t Channel = PHY_INVALID_CHANNEL;

   for ( i = 0; i < PHY_MAX_CHANNEL; i++ )
   {
      (void)PHY_Channel_Get( ( uint8_t )i , &Channel );
      if ( Channel == PHY_INVALID_CHANNEL )
      {
         DBG_printf( "Index %2u Freq Invalid", i );
      }
      else
      {
         DBG_printf( "Index %2u Freq %u (channel %4u)", i, CHANNEL_TO_FREQ( Channel ), Channel );
      }
   }

   return ( 0 );
}

#if DCU == 1
#if OTA_CHANNELS_ENABLED == 1
/*
 * Over the Air (OTA) Commands
 */
static uint32_t CommandLine_OTA_Cmd ( uint32_t argc, char *argv[] )
{
   if ( argc > 1 )
   {
      if (      strcasecmp( "phyTxChannels"       , argv[1] ) == 0 ){ return SetChannels ( phyTxChannels,        argc-1, &argv[1] ) ;}
      else if ( strcasecmp( "phyRxChannels"       , argv[1] ) == 0 ){ return SetChannels ( phyRxChannels,        argc-1, &argv[1] ) ;}
      else if ( strcasecmp( "phyAvailableChannels", argv[1] ) == 0 ){ return SetChannels ( phyAvailableChannels, argc-1, &argv[1] ) ;
      }
      else{
         DBG_logPrintf( 'R', "usage: ota phyTxChannels        xx.xx.xx.xx.xx 1,2,3,4,5 " );
         DBG_logPrintf( 'R', "usage: ota phyRxChannels        xx.xx.xx.xx.xx 1,2,3,4,5 " );
         DBG_logPrintf( 'R', "usage: ota phyAvailableChannels xx.xx.xx.xx.xx 1,2,3,4,5 " );
      }
   }
   return 0;
}

/*
 *
 * These functions are used to set the available channels, tx channels and rx channels
 *
 */

/* Command Line Interface for generic Set Channels */
static uint32_t SetChannels ( uint16_t ReadingType, uint32_t argc, char *argv[] )
{

   bool bStatus = ( bool )false;
   xid_addr_t xid_addr = {0,0,0,0,0};

   struct ui_list_s channel_list;
   channel_list.num_elements = 0;

   if ( argc > 1 )
   {
      // First get the MAC ID
      char delimiter[] = ":.";   // Allows delimiter to be ':' or '.'
      uint16_t num_tokens = 0;
      char* pch;

      pch = strtok( argv[1], delimiter );
      while ( pch != NULL )
      {
         // Convert the token to address bytes
         ( void )sscanf( pch, "%02hhx", &xid_addr[num_tokens] );
         num_tokens++;
         if( num_tokens > ( uint16_t )sizeof( xid_addr ) )
         {
            // no more room!
            break;
         }
         pch = strtok ( NULL, delimiter );
      }

      if( num_tokens == ( uint16_t )sizeof( xid_addr_t ) )
      {
         bStatus = ( bool )true;
      }
      else
      {
         // need to print the usage
         DBG_logPrintf( 'R', "parameter error!" );
         return 0;
      }

      {
         // Get a list of channel numbers
         char delimiter[] = ",";   // Allows delimiter to be ','
         char* pch;

         pch = strtok( argv[2], delimiter );
         while ( pch != NULL )
         {
            // Convert the token to channel
            /* h modifier since destination is a uint16_t */
           ( void )sscanf( pch, "%hd", &channel_list.data[channel_list.num_elements] );
            channel_list.num_elements++;
            if( channel_list.num_elements >= MAX_UI_LIST_ELEMENTS)
            {
               // no more room!
               break;
            }
            pch = strtok ( NULL, delimiter );
         }
      }

      // build the heep command and send it
      // todo: 1/8/2016 12:28:08 PM [BAH]
      uint8_t buffer[200];
      memset(buffer, 0, 200);

      // todo: 1/8/2016 9:20:31 AM [BAH]
      buffer[0] = 0x00;       // InterfaceRevision
      buffer[1] = or_pm;      // resource
      buffer[2] = method_put; // verb
      buffer[3] = 0x00;       // id

      struct readings_s readings;

      // Initialize the CompactMeterRead Header
      HEEP_ReadingsInit(&readings, 0, &buffer[4] );

      // Add this to to message
      HEEP_Put_ui_list(&readings, ReadingType, &channel_list );

      uint8_t offset = 4 + readings.size;

      {
         COMM_Tx_t tx_data;

         tx_data.dst_address.addr_type = eEXTENSION_ID;

         ( void )memcpy( tx_data.dst_address.ExtensionAddr, xid_addr, MAC_ADDRESS_SIZE );

          tx_data.data   = buffer;
          tx_data.length = offset;
          tx_data.qos    = 0;
          tx_data.callback = NULL;
          tx_data.override.linkOverride = eNWK_LINK_OVERRIDE_NULL;
          COMM_transmit( &tx_data );
      }
   }
   return 0;
}
#endif
#endif


#if ( DCU == 1 )
#if 0 // RA6E1 Bob: This code was already removed from the K24 starting point
// Kept in case it is needed later
/******************************************************************************

   Function Name: DBG_TraceRoute

   Purpose: This function will allow the user to send a TRACE ROUTE command

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

               usage tr xx.xx.xx.xx.xx.xx.xx.xx
               usage tr xx.xx.xx.xx.xx.xx.xx.xx | reset |
               usage tr xx.xx.xx.xx.xx.xx.xx.xx | reset | response mode

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
static uint32_t DBG_TraceRoute( uint32_t argc, char *argv[] )
{
   eui_addr_t eui_addr = {0}; // eui address

   if ( argc > 1 )
   {
      // Parse the paramters

      // First get the MAC ID
      char delimiter[] = ":.";   // Allows delimiter to be ':' or '.'
      uint16_t num_tokens = 0;
      char* pch;

      pch = strtok( argv[1], delimiter );
      while ( pch != NULL )
      {
         // Convert the token to address bytes
         ( void )sscanf( pch, "%02hhx", &eui_addr[num_tokens] );
         num_tokens++;
         if( num_tokens > ( uint16_t )sizeof( eui_addr ) )
         {
            // no more room!
            break;
         }
         pch = strtok ( NULL, delimiter );
      }

      if( num_tokens != ( uint16_t )sizeof( eui_addr_t ) )
      {
         // need to print the usage
         DBG_logPrintf( 'R', "parameter error!" );
         return 0;
      }

      buffer_t *pBuffer;

      // Allocate a buffer for a Network Indication
      pBuffer = BM_alloc(30 + sizeof(NWK_DataInd_t));
      if (pBuffer != NULL) {
         pBuffer->x.dataLen  = sizeof(NWK_DataInd_t);

         pBuffer->eSysFmt = eSYSFMT_NWK_INDICATION;

         NWK_DataInd_t *pIndication;

         pIndication = (NWK_DataInd_t *) pBuffer->data;   /*lint !e740 !e826 */
         pIndication->qos = 0x0;
         pIndication->dstAddr.addr_type = eEXTENSION_ID;
         memcpy( &pIndication->dstAddr.ExtensionAddr, &eui_addr[3], sizeof(pIndication->dstAddr.ExtensionAddr) );
         pIndication->dstUdpPort = 0;
         pIndication->srcAddr.addr_type = eCONTEXT;
         pIndication->srcAddr.context = 0;
         pIndication->srcUdpPort = 0;
         pIndication->payloadLength = 100;
         pIndication->payload =  pBuffer->data + sizeof(NWK_DataInd_t);

         uint8_t offset = 0;
         pIndication->payload[offset++] = 0x00;                 // InterfaceRevision
         pIndication->payload[offset++] = (uint8_t) tr;         // resource
         pIndication->payload[offset++] = (uint8_t)method_post; // verb
         pIndication->payload[offset++] = 0x00;                 // id

         pIndication->payloadLength = offset;

         struct readings_s readings;

         // ExchangeWithID
         HEEP_ReadingsInit(&readings, 0, &pIndication->payload[pIndication->payloadLength]);

         uint64_t u64_addr;
         (void) memcpy( &u64_addr, eui_addr, sizeof(eui_addr));
         u64_addr = ntohll(u64_addr);
         HEEP_Put_U64( &readings, trTargetMacAddress, u64_addr);

         // the next parameter will be the reset flag
         if ( argc > 2 )
         {
            uint8_t reset;
            reset = ( uint8_t )atoi( argv[2] );

            HEEP_Put_U8( &readings,trPingCountReset, reset);
         }

         // the next parameter will be the response mode
         if ( argc > 3 )
         {
            uint8_t rsp_mode;
            rsp_mode = ( uint8_t )atoi( argv[3] );

            HEEP_Put_U8( &readings,trRspAddrMode, rsp_mode );
         }

         pIndication->payloadLength += readings.size;

         pBuffer->x.dataLen += readings.size;

         // Send the message to the APPL Layer
         APP_MSG_PostMsg((void *)pBuffer);
      } else {
         DBG_logPrintf( 'R', "Can't allocate buffer");
      }
   }
   return 0;
}
#endif // RA6E1 Bob: This code was already removed from the K24 starting point
#endif // ( DCU == 1 )

#if ( DCU == 1 )
#if 0 // RA6E1 Bob: This code was already removed from the K24 starting point
// Kept in case it is needed later
/*!
 **********************************************************************
 *
 * \fn CommandLine_ReadResource(uint32_t argc, char *argv[])
 *
 * \brief This function allows the user to read a resource over the air.
 *
 * \details  The user must specify the XID, Resource and Meter Read Type
 *           p1 = mac_addr xx.xx.xx.xx.xx
 *           p2 = resource : 1-63
 *           p3 = meter read type : 1-65535
 *
 * \param argc
 * \param argv
 *
 * \return  uint32_t
 *
 **********************************************************************
 */
static uint32_t CommandLine_ReadResource ( uint32_t argc, char *argv[] )
{
   xid_addr_t xid_addr = {0,0,0,0,0};

   uint8_t  resource    = (uint8_t)  bu_en;
   uint16_t readingType = (uint16_t) engData2;

   if ( argc > 3 )
   {
      // First get the MAC ID
      char delimiter[] = ":.";   // Allows delimiter to be ':' or '.'
      uint16_t num_tokens = 0;
      char* pch;

      pch = strtok( argv[1], delimiter );
      while ( pch != NULL )
      {
         // Convert the token to address bytes
         ( void )sscanf( pch, "%02hhx", &xid_addr[num_tokens] );
         num_tokens++;
         if( num_tokens > ( uint16_t )sizeof( xid_addr ) )
         {
            // no more room!
            break;
         }
         pch = strtok ( NULL, delimiter );
      }

      if( num_tokens != ( uint16_t )sizeof( xid_addr_t ) )
      {
         DBG_logPrintf( 'R', "Invalid addr");
         return 0;
      }

      // Get Resource Id and Reading Type
      resource    = ( uint8_t ) atoi( argv[2] );
      if(!((resource > 0) && (resource < 64)))
      {
         DBG_logPrintf( 'R', "Invalid resource (1-63)");
         return 0;
      }

      // Reading Type
      readingType = ( uint16_t )atoi( argv[3] );
      if(readingType == 0)
      {
         DBG_logPrintf( 'R', "Invalid reading type (1-65535)");
         return 0;
      }

      // build the heep command and send it
      // todo: 1/8/2016 12:28:08 PM [BAH]
      uint8_t buffer[50];
      (void) memset(buffer, 0, 50);

      buffer[0] = 0x00;                 // InterfaceRevision
      buffer[1] = resource;             // resource
      buffer[2] = (uint8_t) method_get; // verb
      buffer[3] = 0x00;                 // id

      struct readings_s readings;

      // Initialize the CompactMeterRead Header
      HEEP_ReadingsInit(&readings, 1, &buffer[4] );

      HEEP_Add_ReadingType(&readings, (meterReadingType) readingType);

      {
         COMM_Tx_t tx_data;

         tx_data.dst_address.addr_type = eEXTENSION_ID;

         ( void )memcpy( tx_data.dst_address.ExtensionAddr, xid_addr, MAC_ADDRESS_SIZE );

          tx_data.data   = buffer;
          tx_data.length = 4 + readings.size;
          tx_data.qos    = 0x01;
          tx_data.callback = NULL;
          tx_data.override.linkOverride = eNWK_LINK_OVERRIDE_NULL;
          COMM_transmit( &tx_data );
      }
   }else
   {
      DBG_logPrintf( 'R', "usage:   resource_get xx.xx.xx.xx.xx resource readingType" );
      DBG_logPrintf( 'R', "example: resource_get 00.01.00.00.21 38 1319" );
   }
   return 0;
}
#endif // RA6E1 Bob: This code was already removed from the K24 starting point
#endif // ( DCU == 1 )

#if 0 // RA6E1 Bob: This code was already removed from the K24 starting point

#if 0 // RA6E1 Bob: This code was already removed from the K24 starting point
#if ( DCU == 1 )
/*
 *
 * These functions are used to set the available channels, tx channels and rx channels
 *
 */

// Set the Available Channels
static uint32_t CommandLine_SetChannels ( uint32_t argc, char *argv[] )
{
   return SetChannels ( phyAvailableChannels, argc, argv ) ;

}

/* Set the RX Channels */
static uint32_t CommandLine_SetRxChannels ( uint32_t argc, char *argv[] )
{
   return SetChannels ( phyRxChannels, argc, argv ) ;
}

/* Set the TX Channels */
static uint32_t CommandLine_SetTxChannels ( uint32_t argc, char *argv[] )
{
   return SetChannels ( phyTxChannels, argc, argv ) ;
}
#endif // ( DCU == 1 )
#endif // RA6E1 Bob: This code was already removed from the K24 starting point

/* Command Line Interface for generic Set Channels */
static uint32_t SetChannels ( uint16_t ReadingType, uint32_t argc, char *argv[] )
{

   #define MAX_CHANNELS 8

   bool bStatus = ( bool )false;
   xid_addr_t xid_addr = {0,0,0,0,0};

   struct ui_list_s channel_list;
   channel_list.num_elements = 0;

   if ( argc > 1 )
   {
      // First get the MAC ID
      char delimiter[] = ":.";   // Allows delimiter to be ':' or '.'
      uint16_t num_tokens = 0;
      char* pch;

      pch = strtok( argv[1], delimiter );
      while ( pch != NULL )
      {
         // Convert the token to address bytes
         ( void )sscanf( pch, "%02hhx", &xid_addr[num_tokens] );
         num_tokens++;
         if( num_tokens > ( uint16_t )sizeof( xid_addr ) )
         {
            // no more room!
            break;
         }
         pch = strtok ( NULL, delimiter );
      }

      if( num_tokens == ( uint16_t )sizeof( xid_addr_t ) )
      {
         bStatus = ( bool )true;
      }
      else
      {
         // need to print the usage
         DBG_logPrintf( 'R', "parameter error!" );
         return 0;
      }

      {
         // Get a list of channel numbers
         char delimiter[] = ",";   // Allows delimiter to be ','
         char* pch;

         pch = strtok( argv[2], delimiter );
         while ( pch != NULL )
         {
            // Convert the token to address bytes
           /* h modifier used even though looking for byte, destination is a word */
            ( void )sscanf( pch, "%02hx", &channel_list.data[channel_list.num_elements] );
            channel_list.num_elements++;
            if( channel_list.num_elements > MAX_CHANNELS )
            {
               // no more room!
               break;
            }
            pch = strtok ( NULL, delimiter );
         }
      }

      // build the heep command and send it
      // todo: 1/8/2016 12:28:08 PM [BAH]
      uint8_t buffer[50];
      memset(buffer, 0, 50);

      // todo: 1/8/2016 9:20:31 AM [BAH]
      buffer[0] = 0x00;       // InterfaceRevision
      buffer[1] = tr;         // resource
//    buffer[1] = or_pm;      // resource
      buffer[2] = method_put; // verb , put?
      buffer[3] = 0x00;       // id

      struct readings_s readings;

      // Initialize the CompactMeterRead Header
      HEEP_ReadingsInit(&readings, 0, &buffer[4] );

      // Add this to to message
      HEEP_Put_ui_list(&readings, ReadingType, &channel_list );

      uint8_t offset = 4 + readings.size;

      {
         COMM_Tx_t tx_data;

         tx_data.dst_address.addr_type = eEXTENSION_ID;

         ( void )memcpy( tx_data.dst_address.ExtensionAddr, xid_addr, MAC_ADDRESS_SIZE );

          tx_data.data   = buffer;
          tx_data.length = offset;
          tx_data.qos    = 0x01;
          tx_data.callback = NULL;
          tx_data.override.linkOverride = eNWK_LINK_OVERRIDE_NULL;
          COMM_transmit( &tx_data );
      }
   }
   return 0;
}
#endif // RA6E1 Bob: This code was already removed from the K24 starting point



/******************************************************************************

   Function Name: DBG_CommandLine_TxChannels

   Purpose: This function will set/print a list of TX channels

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_TxChannels ( uint32_t argc, char *argv[] )
{
   uint32_t i; // Loop counter
   uint32_t locationIndex = 0;
   uint32_t freq; // Frequency between 450 and 470 MHz
   uint16_t Channel; // Frequency translates into a channel between 0-3200 //lint !e578  Declaration of symbol 'channel' hides symbol 'channel'

   // No parameters
   if ( argc == 1 )
   {
#if ( FULL_USER_INTERFACE == 1 )
      DBG_printf( "TxChannels sets a list of available TX frequencies:" );
      DBG_printf( "usage: txchannels locationIndex freq|channel [freq|channel] [...]" );
      DBG_printf( "       locationIndex (0-31) is a starting index into an array of frequencies" );
      DBG_printf( "       freq is betwen 450000000 and 470000000 Hz" );
      DBG_printf( "       channel is between 0-3200 (0 is 450 MHz, 1 is 450.00625 MHz, etc)" );
      DBG_printf( "       To delete a frequency from the list use frequency 999999999" );
      DBG_printf( "       or channel 9999." );
#endif

      for ( i = 0; i < PHY_MAX_CHANNEL; i++ )
      {
         if ( PHY_TxChannel_Get( ( uint8_t )i, &Channel ) )
         {
            if ( Channel == PHY_INVALID_CHANNEL )
            {
#if ( FULL_USER_INTERFACE == 1 )
               DBG_printf( "Index %u not set", i );
#else
               printErr();
#endif
            }
            else if ( PHY_IsChannelValid( Channel ) )
            {
               DBG_printf( "Index %u set to channel %4u Freq %u", i, Channel, CHANNEL_TO_FREQ( Channel ) );
//             printErr();
            }
            else
            {
               DBG_printf( "Index %u set to channel %4u Freq %u (WARNING: Not in PHY channel list)",
                           i, Channel, CHANNEL_TO_FREQ( Channel ) );
//             printErr();
            }
         }
      }
      return ( 0 );
   }

   // One parameter
   if ( argc == 2 )
   {
#if ( FULL_USER_INTERFACE == 1 )
      DBG_logPrintf( 'R', "ERROR - Not enough arguments. At least one frequency or channel must be provided" );
#else
      printErr();
#endif
      return ( 0 );
   }

   // Many parameters. Validate locationIndex
   if ( argc > 2 )
   {
      locationIndex = ( uint32_t )atoi( argv[1] );

      // Check for valid range
      if ( locationIndex >= PHY_MAX_CHANNEL )
      {
#if ( FULL_USER_INTERFACE == 1 )
         DBG_logPrintf( 'R', "ERROR - locationIndex is invalid. Must be between 0-31" );
#else
         printErr();
#endif
         return ( 0 );
      }
   }

   // Notice how locationIndex is incremented in the loop
   for ( i = 2; i < argc; i++, locationIndex++ )
   {
      // Validate locationIndex
      if ( locationIndex >= PHY_MAX_CHANNEL )
      {
#if ( FULL_USER_INTERFACE == 1 )
         DBG_logPrintf( 'R', "locationIndex is out of range. Further frequencies ignored" );
#else
         printErr();
#endif
         return ( 0 );
      }

      freq    = ( uint32_t )atoi( argv[i] );
      Channel = ( uint16_t )atoi( argv[i] );
      if ( ( freq >= PHY_MIN_FREQ ) && ( freq <= PHY_MAX_FREQ ) )
      {
         // Convert frequency into a channel
         Channel = FREQ_TO_CHANNEL( freq );

         if ( CHANNEL_TO_FREQ( Channel ) != freq )
         {
#if ( FULL_USER_INTERFACE == 1 )
            DBG_logPrintf( 'R', "ERROR - Invalid frequency %u for locationIndex %u", freq, locationIndex );
#else
            printErr();
#endif
            continue;
         }
      // Validate as delete frequency
      }
      else if ( freq == PHY_INVALID_FREQ )
      {
         Channel = DELETE_CHANNEL;
      }

      // Validate as channel
      if ( ( Channel > PHY_NUM_CHANNELS ) && ( Channel != DELETE_CHANNEL ) )
      {
#if ( FULL_USER_INTERFACE == 1 )
         DBG_logPrintf( 'R', "ERROR - Invalid channel %u for locationIndex %u", channel, locationIndex );
#else
         printErr();
#endif
         // Delete channel
      }
      else if ( Channel == DELETE_CHANNEL )
      {
         DBG_printf( "Deleted index %u", locationIndex );
         (void)PHY_TxChannel_Set( ( uint8_t )locationIndex, ( uint16_t )PHY_INVALID_CHANNEL );
         // Make sure channel is in PHY channel list
      }
      else if ( !PHY_IsChannelValid( Channel ) )
      {
         DBG_logPrintf( 'R',
                        "ERROR - Channel %u is not present in PHY channel list. Add to PHY channel list first.",
                        Channel );
         printErr();
         // Program that channel
      }
      else
      {
         DBG_printf( "Programmed locationIndex %u with %uMHz (channel %u)",
                     locationIndex, CHANNEL_TO_FREQ( Channel ), Channel );
         (void)PHY_TxChannel_Set( ( uint8_t )locationIndex, Channel );
      }
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_RxChannels

   Purpose: This function will set/print a list of TX channels

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_RxChannels ( uint32_t argc, char *argv[] )
{
   uint32_t i; // Loop counter
   uint32_t RadioIndex = 0;
   uint32_t freq; // Frequency between 450 and 470 MHz
   uint16_t Channel; // Frequency translate into a channel between 0-3200 //lint !e578  Declaration of symbol 'channel' hides symbol 'channel'

   // No parameters
   if ( argc == 1 )
   {
#if ( FULL_USER_INTERFACE == 1 )
      DBG_printf( "RxChannels sets a list of available RX frequencies:" );
      DBG_printf( "usage: rxchannels RadioIndex freq|channel [freq|channel] [...]" );
      DBG_printf( "       RadioIndex (0-8) is the first radio to set" );
      DBG_printf( "       freq is betwen 450000000 and 470000000 Hz" );
      DBG_printf( "       channel is between 0-3200 (0 is 450 MHz, 1 is 450.00625 MHz, etc)" );
      DBG_printf( "       To delete a frequency from the list use frequency 999999999" );
      DBG_printf( "       or channel 9999." );
#endif
      for ( i = 0; i < PHY_RCVR_COUNT; i++ )
      {
         if ( PHY_RxChannel_Get( ( uint8_t )i, &Channel ) )
         {
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )
         if ( i == ( uint8_t )RADIO_0 )
         {
            DBG_printf( "Radio 0 has no receiver on this hardware." );
         }
         else
#endif
            if ( Channel == PHY_INVALID_CHANNEL )
            {
#if ( FULL_USER_INTERFACE == 1 ) // In the K24 code, nothing is printed inside this if { } clause
               DBG_printf( "Radio %u not set", i );
               printErr();
#endif
            }
            else if ( PHY_IsChannelValid( Channel ) )
            {
               DBG_printf( "Radio %u set to channel %4u Freq %u", i, Channel, CHANNEL_TO_FREQ( Channel ) );
            }
            else
            {
               DBG_printf( "Radio %u set to channel %4u Freq %u (WARNING: Not in PHY channel list)",
                              i, Channel, CHANNEL_TO_FREQ( Channel ) );
            }
         }
      }
      return ( 0 );
   }

   // One parameter
   if ( argc == 2 )
   {
#if ( FULL_USER_INTERFACE == 1 )
      DBG_logPrintf( 'R', "ERROR - Not enough arguments. At least one frequency or channel must be provided" );
#else
      printErr();
#endif
      return ( 0 );
   }

   // Many parameters. Validate RadioIndex
   if ( argc > 2 )
   {
      RadioIndex = ( uint32_t )atoi( argv[1] );

      // Check for valid range
      if ( RadioIndex >= PHY_RCVR_COUNT )
      {
#if ( FULL_USER_INTERFACE == 1 )
         DBG_logPrintf( 'R', "ERROR - RadioIndex is invalid. Must be between less than %u", PHY_RCVR_COUNT );
#else
         printErr();
#endif
         return ( 0 );
      }
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )
      if ( RadioIndex == ( uint8_t )RADIO_0 )
      {
#if ( FULL_USER_INTERFACE == 1 )
         DBG_logPrintf( 'R',
                       "ERROR - RadioIndex 0 is invalid because the hardware doesn't have a receiver for RADIO 0" );
#else
         printErr();
#endif
         return ( 0 );
      }
#endif
   }

   // Notice how RadioIndex is incremented in the loop
   for ( i = 2; i < argc; i++, RadioIndex++ )
   {
      // Validate RadioIndex
      if ( RadioIndex >= PHY_RCVR_COUNT )
      {
#if ( FULL_USER_INTERFACE == 1 )
         DBG_logPrintf( 'R', "RadioIndex is out of range. Further frequencies ignored" );
#else
         printErr();
#endif
         return ( 0 );
      }

      freq    = ( uint32_t )atoi( argv[i] );
      Channel = ( uint16_t )atoi( argv[i] );
      if ( ( freq >= PHY_MIN_FREQ ) && ( freq <= PHY_MAX_FREQ ) )
      {
         // Convert frequency into a channel
         Channel = FREQ_TO_CHANNEL( freq );

         if ( CHANNEL_TO_FREQ( Channel ) != freq )
         {
#if ( FULL_USER_INTERFACE == 1 )
            DBG_logPrintf( 'R', "ERROR - Invalid frequency %u for RadioIndex %d", freq, RadioIndex );
#else
            printErr();
#endif
            continue;
         }
      // Validate as delete frequency
      }
      else if ( freq == PHY_INVALID_FREQ )
      {
         Channel = DELETE_CHANNEL;
      }

      // Validate as channel
      if ( ( Channel > PHY_NUM_CHANNELS ) && ( Channel != DELETE_CHANNEL ) )
      {
#if ( FULL_USER_INTERFACE == 1 )
         DBG_logPrintf( 'R', "ERROR - Invalid channel %u for RadioIndex %d", channel, RadioIndex );
#else
         printErr();
#endif
         // Delete channel
      }
      else if ( Channel == DELETE_CHANNEL )
      {
         DBG_printf( "Deleted index %u", RadioIndex );
         (void)PHY_RxChannel_Set( ( uint8_t )RadioIndex, ( uint16_t )PHY_INVALID_CHANNEL );
         // Make sure channel is in PHY channel list
      }
      else if ( !PHY_IsChannelValid( Channel ) )
      {
         DBG_logPrintf( 'R',
                        "ERROR - Channel %u is not present in PHY channel list. Add to PHY channel list first.",
                        Channel );
         printErr();
         // Program that channel
      }
      else
      {
         DBG_printf( "Programmed RadioIndex %u with %uMHz (channel %u)",
                     RadioIndex, CHANNEL_TO_FREQ( Channel ), Channel );
         (void)PHY_RxChannel_Set( ( uint8_t )RadioIndex, Channel );
      }
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_RxDetection

   Purpose: This function will set/print the RX detection list

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_RxDetection ( uint32_t argc, char *argv[] )
{
   uint32_t i; // Loop counter
   uint32_t RadioIndex = 0;
   PHY_DETECTION_e detection; // Detection mode
   PHY_DETECTION_e detectionList[PHY_RCVR_COUNT]; // Detection list

   if ( PHY_RxDetectionConfig_Get((uint8_t*)detectionList) ) {
      // No parameters
      if ( argc == 1 )
      {
         DBG_printf( "RxDetection configure the detection used for each RX radio:" );
         DBG_printf( "usage: rxdetection RadioIndex detection [detection] [...]" );
         DBG_printf( "       RadioIndex is the first radio to set" );
         DBG_printf( "       Detection is the detection mode to use" );

         for ( i = 0; i < PHY_RCVR_COUNT; i++ )
         {
            {
               DBG_printf( "Radio %u set to detection mode %u", i, ( uint32_t )detectionList[i]);
            }
         }
         return ( 0 );
      }

      // One parameter
      if ( argc == 2 )
      {
         DBG_logPrintf( 'R', "ERROR - Not enough arguments. At least one detection mode must be provided" );
         return ( 0 );
      }

      // Many parameters. Validate RadioIndex
      if ( argc > 2 )
      {
         RadioIndex = ( uint32_t )atoi( argv[1] );

         // Check for valid range
         if ( RadioIndex >= PHY_RCVR_COUNT )
         {
            DBG_logPrintf( 'R', "ERROR - RadioIndex is invalid. Must be between less than %u", PHY_RCVR_COUNT );
            return ( 0 );
         }
      }

      // Notice how RadioIndex is incremented in the loop
      for ( i = 2; i < argc; i++, RadioIndex++ )
      {
         // Validate RadioIndex
         if ( RadioIndex >= PHY_RCVR_COUNT )
         {
            DBG_logPrintf( 'R', "RadioIndex is out of range. Further detection mode ignored" );
            return ( 0 );
         }

         detection = ( PHY_DETECTION_e )atoi( argv[i] );
         // Validate
         if ( detection < ePHY_DETECTION_LAST )
         {
            DBG_printf( "Programmed RadioIndex %u with %u", RadioIndex, ( uint32_t )detection );
            detectionList[RadioIndex] = detection;
            (void)PHY_RxDetectionConfig_Set( (uint8_t*)detectionList );
         }
         else
         {
            DBG_logPrintf( 'R', "ERROR - Invalid detection mode %u for RadioIndex %u", ( uint32_t )detection, RadioIndex );
         }
      }
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_RxFraming

   Purpose: This function will set/print the RX framing list

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_RxFraming ( uint32_t argc, char *argv[] )
{
   uint32_t i; // Loop counter
   uint32_t RadioIndex = 0;
   PHY_FRAMING_e frame; // Framing mode
   PHY_FRAMING_e frameList[PHY_RCVR_COUNT]; // Framing list

   if ( PHY_RxFramingConfig_Get((uint8_t*)frameList) ) {
      // No parameters
      if ( argc == 1 )
      {
         DBG_printf( "RxFraming configure the framing used for each RX radio:" );
         DBG_printf( "usage: rxframing RadioIndex framing [framing] [...]" );
         DBG_printf( "       RadioIndex is the first radio to set" );
         DBG_printf( "       Framing is the framing mode to use" );

         for ( i = 0; i < PHY_RCVR_COUNT; i++ )
         {
            {
               DBG_printf( "Radio %u set to framing mode %u", i, ( uint32_t )frameList[i]);
            }
         }
         return ( 0 );
      }

      // One parameter
      if ( argc == 2 )
      {
         DBG_logPrintf( 'R', "ERROR - Not enough arguments. At least one framing mode must be provided" );
         return ( 0 );
      }

      // Many parameters. Validate RadioIndex
      if ( argc > 2 )
      {
         RadioIndex = ( uint32_t )atoi( argv[1] );

         // Check for valid range
         if ( RadioIndex >= PHY_RCVR_COUNT )
         {
            DBG_logPrintf( 'R', "ERROR - RadioIndex is invalid. Must be between less than %u", PHY_RCVR_COUNT );
            return ( 0 );
         }
      }

      // Notice how RadioIndex is incremented in the loop
      for ( i = 2; i < argc; i++, RadioIndex++ )
      {
         // Validate RadioIndex
         if ( RadioIndex >= PHY_RCVR_COUNT )
         {
            DBG_logPrintf( 'R', "RadioIndex is out of range. Further framing mode ignored" );
            return ( 0 );
         }

         frame = ( PHY_FRAMING_e )atoi( argv[i] );
         // Validate
         if ( frame < ePHY_FRAMING_LAST )
         {
            DBG_printf( "Programmed RadioIndex %u with %u", RadioIndex, ( uint32_t )frame );
            frameList[RadioIndex] = frame;
            (void)PHY_RxFramingConfig_Set( (uint8_t*)frameList );
         }
         else
         {
            DBG_logPrintf( 'R', "ERROR - Invalid framing mode %u for RadioIndex %u", ( uint32_t )frame, RadioIndex );
         }
      }
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_RxMode

   Purpose: This function will set/print the RX mode list

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_RxMode ( uint32_t argc, char *argv[] )
{
   uint32_t i; // Loop counter
   uint32_t RadioIndex = 0;
   PHY_MODE_e mode; // mode
   PHY_MODE_e modeList[PHY_RCVR_COUNT]; // mode list

   if ( PHY_RxMode_Get((uint8_t*)modeList) ) {
      // No parameters
      if ( argc == 1 )
      {
         DBG_printf( "RxMode configure the PHY mode used for each RX radio:" );
         DBG_printf( "usage: rxmode RadioIndex mode [mode] [...]" );
         DBG_printf( "       RadioIndex is the first radio to set" );
         DBG_printf( "       Mode is the PHY mode to use" );

         for ( i = 0; i < PHY_RCVR_COUNT; i++ )
         {
            {
               DBG_printf( "Radio %u set to PHY mode %u", i, ( uint32_t )modeList[i]);
            }
         }
         return ( 0 );
      }

      // One parameter
      if ( argc == 2 )
      {
         DBG_logPrintf( 'R', "ERROR - Not enough arguments. At least one PHY mode must be provided" );
         return ( 0 );
      }

      // Many parameters. Validate RadioIndex
      if ( argc > 2 )
      {
         RadioIndex = ( uint32_t )atoi( argv[1] );

         // Check for valid range
         if ( RadioIndex >= PHY_RCVR_COUNT )
         {
            DBG_logPrintf( 'R', "ERROR - RadioIndex is invalid. Must be between less than %u", PHY_RCVR_COUNT );
            return ( 0 );
         }
      }

      // Notice how RadioIndex is incremented in the loop
      for ( i = 2; i < argc; i++, RadioIndex++ )
      {
         // Validate RadioIndex
         if ( RadioIndex >= PHY_RCVR_COUNT )
         {
            DBG_logPrintf( 'R', "RadioIndex is out of range. Further PHY mode ignored" );
            return ( 0 );
         }

         mode = ( PHY_MODE_e )atoi( argv[i] );
         // Validate
         if ( mode < ePHY_MODE_LAST )
         {
            DBG_printf( "Programmed RadioIndex %u with %u", RadioIndex, ( uint32_t )mode );
            modeList[RadioIndex] = mode;
            (void)PHY_RxMode_Set( (uint8_t*)modeList );
         }
         else
         {
            DBG_logPrintf( 'R', "ERROR - Invalid PHY mode %u for RadioIndex %u", ( uint32_t )mode, RadioIndex );
         }
      }
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_StackUsage

   Purpose: This function will print the stack of active tasks

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_StackUsage ( uint32_t argc, char *argv[] )
{
   DBG_printf("Command is deprecated. Use tasksummary instead.");
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_TaskSummary

   Purpose: This function will print the stack of active tasks

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_TaskSummary ( uint32_t argc, char *argv[] )
{
   OS_TASK_Summary((bool)true);
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_TXMode

   Purpose: This function sets the radio mode

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:  This is used to cause the PHY layer to either enter or exit test mode.
           In test mode the Radio can be configured to CW or PN9 Mode. In this mode it will
           transmit for T (msec), and and repeat every Period (msec).
           This will repeat for RepeatCnt times.

******************************************************************************/
uint32_t DBG_CommandLine_TXMode ( uint32_t argc, char *argv[] )
{
   RADIO_MODE_t mode = eRADIO_MODE_NORMAL;
   uint16_t T;                  // Time the signal is active
   uint16_t Period;             // Time for one full cycle
   uint16_t RepeatCnt;          // How many time the test will run
   uint16_t Delay;              // Delay in seconds before the test starts
   uint16_t Delay2=0;           // Delay between 2 loops
   uint16_t RepeatCnt2=0;       // How many time to repeat the 2nd loop (65535 is infinity)
   uint16_t UseMarksTxAlgo;     // Use Mark's PA temperature compensation algorithm
   uint16_t UseDennisTempCheck; // Enable/Disable Dennis' temperature check
   uint16_t UseDynamicBackoff;  // Use fixed backoff delay (0) or dynamic (1)

   if ( argc == 1 )
   {
      DBG_printf( "Txmode syntax: mode constantPower thermalProtection delayType delayStart dutyON period repeat delay2 repeat2" );
      DBG_printf( "mode:              0 = Normal (default), 1 = PN9, 2 = Unmodulated Carrier Wave, 3 = Deprecated, 4 = 4GFSK BER with manufacturing print out" );
      DBG_printf( "constantPower:     Use Mark's algorithm for constant TX power" );
      DBG_printf( "thermalProtection: Use Dennis' algorithm for PA protection" );
      DBG_printf( "delayType:         Use fixed backoff delay (0) or dynamic (1)" );
      DBG_printf( "delayStart:        Delay start of Txmode (in seconds)" );
      DBG_printf( "dutyON:            Duty cycle ON part (in msec)" );
      DBG_printf( "period:            Duty cycle ON + OFF part (in msec)"  );
      DBG_printf( "repeat:            Repeat first loop (65535 is forever)" );
      DBG_printf( "delay2:            Second delay (in second)" );
      DBG_printf( "repeat2:           Repeat second loop (65535 is forever)" );
   }

   if ( argc == 2 )
   {
      mode = ( RADIO_MODE_t )atoi( argv[1] );

      // Check for valid range
      if ( mode >= eRADIO_LAST_MODE )
      {
         DBG_logPrintf( 'R', "ERROR - Invalid range. Must be between 0-%u", ( uint16_t )eRADIO_LAST_MODE - 1 );
         return ( 0 );
      }

      // If the phy is in the test repeat mode, then it first needs to be taken out of that mode
      // then put the radio into the requested mode
      PHY_TestMode_Enable( 0, 0, 0, 0, 0, 0, 0, 0, 0, eRADIO_MODE_NORMAL );

#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
      (void)PHY_StartRequest( ( PHY_START_e )0, NULL );
#endif
      RADIO_Mode_Set( mode );
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
      if ( mode == 0 )  /* Restart the stack */
      {
         (void)SM_StopRequest( NULL );
         (void)SM_StartRequest( eSM_START_STANDARD, NULL );
      }
#endif

      return ( 0 );
   }

   /* -----------------------------------------------------------------------------------
    * This is used to put the PHY/Radio into the CW or PN9 Mode
    * In this mode, the radio will do a cycle of CW/PN9, delay, normal, delay, repeat
    * -----------------------------------------------------------------------------------*/
   if ( argc > 7 )
   {
      mode               = ( RADIO_MODE_t )atoi( argv[1] );
      UseMarksTxAlgo     = (uint16_t)atoi( argv[2] );
      UseDennisTempCheck = (uint16_t)atoi( argv[3] );
      UseDynamicBackoff  = (uint16_t)atoi( argv[4] );
      Delay              = (uint16_t)atoi( argv[5] );
      T                  = (uint16_t)atoi( argv[6] );
      Period             = (uint16_t)atoi( argv[7] );
      RepeatCnt          = (uint16_t)atoi( argv[8] );
      if (argc >= 10) {
         Delay2 = (uint16_t)atoi( argv[9] );
      }
      if (argc >= 11) {
         RepeatCnt2 = (uint16_t)atoi( argv[10] );
      }

      if ( (mode == eRADIO_MODE_PN9) || (mode == eRADIO_MODE_CW) )
      {
         // Check the input parameters
         if( RepeatCnt > 0 )
         {
            // Period and T must be > 0 and Period must be >= T
            if(((Period == 0) || ( T == 0 )) ||
                (Period < T))
            {  // Error.
               DBG_logPrintf( 'R', "ERROR - Invalid Parameter" );
               return 0;
            }

            // Enable the PHY Test Mode
            PHY_TestMode_Enable( Delay, UseMarksTxAlgo, UseDennisTempCheck, UseDynamicBackoff, T, Period, RepeatCnt, Delay2, RepeatCnt2, mode );

            // Cause the PHY to change state!
            (void)PHY_StartRequest( ( PHY_START_e )0, NULL );
            return 0;
         }else
         {
            DBG_logPrintf( 'R', "ERROR - Repeat Count" );
            return 0;
         }
      }else
      {
         DBG_logPrintf( 'R', "ERROR - Invalid mode" );
      }
   }else if ( argc == 1)
   {
      DBG_printf( "Current radio TXMode = %u", ( (uint16_t)RADIO_TxMode_Get() ) );
   }

   return ( 0 );
}

#if ( EP == 1 )
#if ( MCU_SELECTED == RA6E1 ) // RA6E1 Bob: This command was removed from original K24 code but enabling for RA6E1 testing
/******************************************************************************

   Function Name: DBG_CommandLine_AfcEnable

   Purpose: This function enables/disables AFC

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_AfcEnable ( uint32_t argc, char *argv[] )
{
   bool enable;

   // One parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      if ( argc == 1 )
      {
         PHY_GetConf_t GetConf;
         GetConf = PHY_GetRequest( ePhyAttr_AfcEnable );
         if (GetConf.eStatus == ePHY_GET_SUCCESS) {
            DBG_printf( "AFC enable is %u", ( uint32_t )GetConf.val.AfcEnable );
         }
      }
      else
      {
         PHY_SetConf_t SetConf;
         enable = ( bool )atoi( argv[1] );
         SetConf = PHY_SetRequest( ePhyAttr_AfcEnable, &enable);
         if (SetConf.eStatus == ePHY_SET_SUCCESS) {
            DBG_printf( "AFC enable set to %u", ( uint8_t )enable );
         } else {
            DBG_printf( "Failed to set AFC enable" );
         }
      }
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_AfcRSSIThreshold

   Purpose: This function displays/sets RSSI threshold

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_AfcRSSIThreshold ( uint32_t argc, char *argv[] )
{
   int16_t threshold;

   // One parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      if ( argc == 1 )
      {
         PHY_GetConf_t GetConf;
         GetConf = PHY_GetRequest( ePhyAttr_AfcRssiThreshold );
         if (GetConf.eStatus == ePHY_GET_SUCCESS) {
            DBG_printf( "AFC RSSI threshold is %d", GetConf.val.AfcRssiThreshold );
         }
      }
      else
      {
         PHY_SetConf_t SetConf;
         threshold = ( int16_t )atoi( argv[1] );
         SetConf = PHY_SetRequest( ePhyAttr_AfcRssiThreshold, &threshold);
         if (SetConf.eStatus == ePHY_SET_SUCCESS) {
            DBG_printf( "AFC RSSI threshold set to %d", threshold );
         } else {
            DBG_printf( "Failed to set AFC RSSI threshold" );
         }
      }
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_AfcTemperatureRange

   Purpose: This function displays/sets AFC teperature range

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_AfcTemperatureRange ( uint32_t argc, char *argv[] )
{
   int8_t AfcTemperatureRange[2];

   // Two parameters parameter
   if ( argc > 3 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else if ( argc == 2 ) {
      DBG_logPrintf( 'R', "ERROR - Need 0 or 2 arguments" );
   } else {
      if ( argc == 1 )
      {
         PHY_GetConf_t GetConf;
         GetConf = PHY_GetRequest( ePhyAttr_AfcTemperatureRange );
         if (GetConf.eStatus == ePHY_GET_SUCCESS) {
            DBG_printf( "AFC temperature range low %d high %d", GetConf.val.AfcTemperatureRange[0], GetConf.val.AfcTemperatureRange[1] );
         }
      }
      else
      {
         PHY_SetConf_t SetConf;
         AfcTemperatureRange[0] = ( int8_t )atoi( argv[1] );
         AfcTemperatureRange[1] = ( int8_t )atoi( argv[2] );
         SetConf = PHY_SetRequest( ePhyAttr_AfcTemperatureRange, &AfcTemperatureRange[0]);
         if (SetConf.eStatus == ePHY_SET_SUCCESS) {
            DBG_printf( "AFC temperature range set to low %d high %d", AfcTemperatureRange[0], AfcTemperatureRange[1] );
         } else {
            DBG_printf( "Failed to set AFC temperature range" );
         }
      }
   }

   return ( 0 );
}
#endif // ( MCU_SELECTED == RA6E1 ) // RA6E1 Bob: This command was removed from original K24 code
#endif // ( EP == 1 )

/******************************************************************************

   Function Name: DBG_CommandLine_Power

   Purpose: This function sets the TX power level mode

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_Power ( uint32_t argc, char *argv[] )
{
   uint8_t PowerSetting;

   // One parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      if ( argc == 1 )
      {
         PHY_GetConf_t GetConf;
         GetConf = PHY_GetRequest( ePhyAttr_PowerSetting );
         if (GetConf.eStatus == ePHY_GET_SUCCESS) {
            DBG_printf( "Current power level is %u", ( uint8_t )GetConf.val.PowerSetting );
         }
      }
      else
      {
         PowerSetting = ( uint8_t )atoi( argv[1] );

         // Check for valid range
         if ( PowerSetting > 127 )
         {
            DBG_logPrintf( 'R', "ERROR - Invalid range. Must be between 0-127" );
            return ( 0 );
         }
         PHY_SetConf_t SetConf;
         SetConf = PHY_SetRequest( ePhyAttr_PowerSetting, &PowerSetting);
         if (SetConf.eStatus == ePHY_SET_SUCCESS) {
            DBG_printf( "Power level is set to %u", PowerSetting );
         } else {
            DBG_printf( "Failed to set power" );
         }
      }
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_RadioStatus

   Purpose: This function reads radio status

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_RadioStatus ( uint32_t argc, char *argv[] )
{
   uint8_t radioNum;
   uint8_t state, int_pend, int_status, ph_pend, ph_status, modem_pend, modem_status, chip_pend, chip_status;
   char stateStr[][11] = { "Undefined" , "Sleep", "SPI Active", "Ready", "Ready2", "TX Tune", "RX Tune", "TX", "RX" };

   for ( radioNum = ( uint8_t )RADIO_0; radioNum < ( uint8_t )MAX_RADIO; radioNum++ )
   {
      if ( RADIO_Status_Get( radioNum, &state, &int_pend, &int_status, &ph_pend, &ph_status, &modem_pend, &modem_status,
                             &chip_pend, &chip_status ) )
      {
         DBG_printf( "Radio %u: state %11s, INT status 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
                     radioNum, stateStr[state], int_pend, int_status, ph_pend, ph_status, modem_pend, modem_status,
                     chip_pend, chip_status );
      }
   }

   return ( 0 );
}

#if (EP == 1)
#if ( ENABLE_DEMAND_TASKS == 1 )
/******************************************************************************

   Function Name: DBG_CommandLine_DMDDump

   Purpose: Prints the demand file/variables

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_DMDDump ( uint32_t argc, char *argv[] )
{
   DEMAND_DumpFile();
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_SchDmdRst

   Purpose: Get or set the scheduled demand reset date of month

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes: SCHD DMD RESET not in meters within meter demand

******************************************************************************/
uint32_t DBG_CommandLine_SchDmdRst ( uint32_t argc, char *argv[] )
{
   uint8_t              dateOfMonth=255;
   sysTime_t            sTime;
   sysTime_dateFormat_t sysTime;

   //Command only, get Scheduled date
   if ( 1 == argc )
   {
      DEMAND_GetResetDay( &dateOfMonth );
   }
   //Command and parameter, set scheduled date of month
   else if ( 2 == argc )
   {
         // Write ScheduledDemandResetDay
      dateOfMonth = ( uint8_t )atol( argv[1] );
      ( void )DEMAND_SetResetDay( dateOfMonth );
   }
   if ( 2 >= argc )
   {
      DEMAND_GetSchDmdRst( &sTime.date, &sTime.time );
      (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );
      DBG_printf( "Demand Reset Date: %u, Schedule(Local): %02d/%02d/%04d %02d:%02d:%02d", dateOfMonth,
                sysTime.month, sysTime.day, sysTime.year, sysTime.hour, sysTime.min, sysTime.sec);
   }
   else
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_DMDTO

   Purpose: Get or set the demand reset timeout

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_DMDTO ( uint32_t argc, char *argv[] )
{
   uint32_t timeout = 0;   /*lint !e578 local name same as enum OK   */

   //Command only, get Timeout
   if ( 1 == argc )
   {
      DEMAND_GetTimeout( &timeout );
   }
   //Command and parameter, set timeout
   else if ( 2 == argc )
   {
      timeout = ( uint32_t )atol( argv[1] );
      ( void )DEMAND_SetTimeout( timeout );
   }
   if ( 2 >= argc )
   {
      DBG_printf( "Demand Reset Timeout: %u", timeout );
   }
   else
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }
   return ( 0 );
}
#if ( DEMAND_IN_METER == 1 )
/******************************************************************************

   Function Name: DBGC_DemandResetCallBack

   Purpose: Called upon completion (or failure) of demand reset command

   Arguments:  result - success/failure of procedure.

   Returns: none

   Notes:

******************************************************************************/
static volatile bool             dmdResetDone;
static volatile returnStatus_t   dmdResetResult;
static void DBGC_DemandResetCallBack( returnStatus_t result )
{
   dmdResetDone = (bool)true;
   dmdResetResult = result;
}

/******************************************************************************

   Function Name: DBG_CommandLine_DemandReset

   Purpose: Execute a demand reset in the meter

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
static uint32_t DBG_CommandLine_DemandReset( uint32_t argc, char *argv[] )
{
   uint8_t retries;
   dmdResetDone = false;
   //Command only, get Timeout
   if ( 1 == argc )
   {
      /* Request a demand reset and set the callback function  */
      ( void )HMC_DMD_Reset( ( uint8_t )HMC_APP_API_CMD_ACTIVATE, (void *)DBGC_DemandResetCallBack ); /*lint !e611 cast
                                                                                                        is correct   */
      for( retries = 120; retries && !dmdResetDone; retries-- ) /* Wait for results              */
      {
         OS_TASK_Sleep( 250 );
      }
      /* Display results   */
      DBG_printf( "Demand Reset: %s.", dmdResetDone ? "completed" : "not complete" );
      if ( dmdResetDone )
      {
         if ( eSUCCESS == dmdResetResult )
         {
            DBG_printf( "Demand Reset: success." );
         }
         else
         {
            DBG_printf( "Demand Reset: %u.", ( uint32_t )dmdResetResult );
         }
      }
   }
   return 0;
}
#endif //#if ( DEMAND_IN_METER == 1 )
#endif // #if ( ENABLE_DEMAND_TASKS == 1 )
#if ( CLOCK_IN_METER == 1 )
/*******************************************************************************

   Function name: DBG_CommandLine_HMC_time

   Purpose: This function will get or set the KV2C time
   Usage:   hmc_time - reads meter time
            hmc_time yy mm dd hh mm ss - set meter's time

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
static uint32_t DBG_CommandLine_HMC_time( uint32_t argc, char *argv[] )
{
   ( void )HMC_TIME_Set_Time( ( uint8_t )HMC_APP_API_CMD_ACTIVATE, NULL );  // Read/Update meter's time

   return 0;
}
#endif

#if ( MULTIPLE_MAC != 0 )
/******************************************************************************

   Function Name: DBG_CommandLine_NumMac

   Purpose: This function sets/gets the number of mac addresses emulated by this device.

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
static uint32_t DBG_CommandLine_NumMac( uint32_t argc, char *argv[] )
{
   uint8_t           NumMac;


   if ( argc == 2 )
   {
      MAC_SetConf_t SetConf;
      NumMac = ( uint8_t )strtoul( argv[1], NULL, 0 );
      SetConf = MAC_SetRequest( eMacAttr_NumMac, &NumMac);
      if (SetConf.eStatus != eMAC_SET_SUCCESS) {
          DBG_printf( "MAC set API returned an error" );
      }
   }

   if ( argc <= 2 )
   {
      MAC_GetConf_t GetConf;
      GetConf = MAC_GetRequest( eMacAttr_NumMac );
      if (GetConf.eStatus == eMAC_GET_SUCCESS) {
         DBG_printf( "NumMac: %d", GetConf.val.NumMac );
      }
   }
   return ( 0 );
}
#endif // ( MULTIPLE_MAC != 0 )
#endif // (EP == 1)

/******************************************************************************

   Function Name: DBG_CommandLine_Buffers

   Purpose: This function displays the available free internal RAM

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_Buffers( uint32_t argc, char *argv[] )
{
   bufferStats_t bufferStats;
   uint8_t i;

   BM_getStats( &bufferStats );

   DBG_printf( "    Alloc Fail App: %u", bufferStats.allocFailApp );
   DBG_printf( "  Alloc Fail Debug: %u", bufferStats.allocFailDebug );
   DBG_printf( "  Buffer Free Fail: %u", bufferStats.freeFail );
   DBG_printf( "Buffer Double Free: %u", bufferStats.freeDoubleFree );

   for ( i = 0; i < BUFFER_N_POOLS; ++i )
   {
      DBG_printf( "Pool: %2u  "
                  "size/cnt/type: %4u / %4u / %u  "
                  "A-Ok/F-Ok:  %10u / %10u  "
                  "A-Fail/F-Fail: %6u / %6u  "
                  "Cur/Max: %4u / %4u",
                  i,
                  BM_bufferPoolParams[i].size,
                  BM_bufferPoolParams[i].cnt,
                  ( uint16_t ) BM_bufferPoolParams[i].type,
                  bufferStats.pool[i].allocOk,
                  bufferStats.pool[i].freeOk,
                  bufferStats.pool[i].allocFail,
                  bufferStats.pool[i].freeFail,
                  bufferStats.pool[i].currAlloc,
                  bufferStats.pool[i].highwater );
      // These lines are pretty long (about 130 char) and there might not be enough buffer avaiable to print all at once so throttle the output.
      OS_TASK_Sleep(TEN_MSEC); // About the time needed to print a line
   }

   BM_showAlloc((bool)true);

   return ( 0 );
}
#if ( RTOS_SELECTION == FREE_RTOS )
/******************************************************************************

   Function Name: freeRAMdump

   Purpose: This function retrieves and prints the FreeRTOS Heap statistics

   Arguments: None

   Returns:   None

   Notes:

******************************************************************************/
static uint32_t mallocFailures = 0; /* File global variable shared with vApplicationMallocFailedHook */
static void freeRAMdump( char * pString )
{
   HeapStats_t heepInfo;
   (void)vPortGetHeapStats( &heepInfo );
   DBG_printf( "Heap Statistics: %s \r\n"
            "  Availale Heap Space           %6u \r\n"
            "  Size of Largest Free Block    %6u \r\n"
            "  Size of Smallest Free Block   %6u \r\n"
            "  Number of Free Blocks         %6u \r\n"
            "  Minimum Ever Free Remaining   %6u \r\n"
            "  Number of Successful Allocs   %6u \r\n"
            "  Number of Successful Frees    %6u \r\n"
            "  Number of Unsuccessful Allocs %6u ",
            pString,
            heepInfo.xAvailableHeapSpaceInBytes,
            heepInfo.xSizeOfLargestFreeBlockInBytes,
            heepInfo.xSizeOfSmallestFreeBlockInBytes,
            heepInfo.xNumberOfFreeBlocks,
            heepInfo.xMinimumEverFreeBytesRemaining,
            heepInfo.xNumberOfSuccessfulAllocations,
            heepInfo.xNumberOfSuccessfulFrees,
            mallocFailures                           );
}

/******************************************************************************

   Function Name: vApplicationMallocFailedHook

   Purpose: This function is called by FreeRTOS if an malloc fails.  To enable this feature
            conditional compile switch configUSE_MALLOC_FAILED_HOOK needs to be set to (1)
            in FreeRTOSConfig.h

   Arguments: None

   Returns:   None

   Notes:

******************************************************************************/
void vApplicationMallocFailedHook( void )
{
   mallocFailures++;
   DBG_logPrintf( 'R', "FreeRTOS malloc failure hook was called, no other info available" );
   freeRAMdump( "after malloc failure");
}
#endif

/******************************************************************************

   Function Name: DBG_CommandLine_FreeRAM

   Purpose: This function displays the available free internal RAM

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_FreeRAM ( uint32_t argc, char *argv[] )
{
#if ( RTOS_SELECTION == MQX_RTOS )
   DBG_printf( "Free Internal RAM High Water Mark = %u bytes \n Current RAM Free = %u bytes ",
               ( uint32_t )BSP_DEFAULT_END_OF_KERNEL_MEMORY - ( uint32_t )_mem_get_highwater(),
               ( uint32_t )_mem_get_free() );
#elif ( RTOS_SELECTION == FREE_RTOS )
   if ( argc == 1 )
   {
      freeRAMdump( " " );
   }
   else if ( argc == 2 )
   {
      uint32_t bytes = atol( argv[1] );
      freeRAMdump( "before malloc call ");
      void * pBuffer = malloc( bytes );
      DBG_printf( "FreeRTOS malloc returned %08x for buffer of %u bytes", pBuffer, bytes );
      freeRAMdump( "after malloc call" );
      if ( pBuffer != NULL)
      {
         free( pBuffer );
         DBG_printf( "FreeRTOS free called to return buffer at %08x", pBuffer );
         freeRAMdump( "after free call" );
      }
   }
   else
   {
      DBG_printf( "Too many args!" );
   }
#endif // RTOS_SELECTION
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_Reboot

   Purpose: This function reboots the device

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
/*lint -efunc(533,DBG_CommandLine_Reboot) no return because the function doesn't return   */
uint32_t DBG_CommandLine_Reboot ( uint32_t argc, char *argv[] )
{
   PWR_SafeReset();  /* Execute Software Reset, with cache flush */
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_RSSI

   Purpose: This function displays the RSSI value of a radio

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_RSSI ( uint32_t argc, char *argv[] )
{
   CCA_RSSI_TYPEe processingType;
   uint8_t        radioNum;
   uint8_t        rssi;
   char           floatStr[PRINT_FLOAT_SIZE];

   // One parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
      return ( 0 );
   }
#if (EP == 1)
   radioNum = 0;
   PHY_Lock(); // Function will not return if it fails
   rssi = (uint8_t)RADIO_Filter_CCA( radioNum, &processingType, CCA_SLEEP_TIME );
   PHY_Unlock(); // Function will not return if it fails
   DBG_printf( "Compensated RSSI = %u, %sdBm", rssi, DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( rssi ), 1 ) );
#else
   if ( argc == 1 )
   {
      DBG_logPrintf( 'R', "ERROR - Not enough arguments" );
   }
   else
   {
      radioNum = ( uint8_t )atoi( argv[1] );

      if (radioNum < (uint8_t)MAX_RADIO) {
         PHY_Lock(); // Function will not return if it fails
         rssi = (uint8_t)RADIO_Filter_CCA( radioNum, &processingType, CCA_SLEEP_TIME );
         PHY_Unlock(); // Function will not return if it fails
         DBG_printf( "Compensated RSSI = %u, %sdBm", rssi, DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( rssi ), 1 ) );
      } else {
         DBG_logPrintf( 'R', "ERROR - Argument is out of range. Must be between 0 and %u", (uint32_t)MAX_RADIO-1);
      }
   }
#endif // (EP == 1)
   return ( 0 );
}
#if 0 // RA6E1 Bob: This command was removed from original K24 code
/******************************************************************************

   Function Name: DBG_CommandLine_RSSIJumpThreshold

   Purpose: This function gets or sets the RSSI Jump Threshold

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_RSSIJumpThreshold ( uint32_t argc, char *argv[] )
{
   uint8_t rssi;

   // More than one parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      // No parameters
      if ( argc == 1 )
      {
         // Get the RSSI Jump threshold
         PHY_GetConf_t GetConf;
         GetConf = PHY_GetRequest( ePhyAttr_RssiJumpThreshold );
         if (GetConf.eStatus == ePHY_GET_SUCCESS) {
            DBG_printf( "RSSI Jump Threshold is %u dBm", GetConf.val.RssiJumpThreshold );
         }
      } else {
         PHY_SetConf_t SetConf;
         rssi = ( uint8_t )atoi( argv[1] );
         SetConf = PHY_SetRequest( ePhyAttr_RssiJumpThreshold, &rssi);
         if (SetConf.eStatus == ePHY_SET_INVALID_PARAMETER) {
            DBG_logPrintf( 'R', "ERROR - Invalid range. Must be between %d and %d",
                           PHY_RSSI_JUMP_THRESHOLD_MIN, PHY_RSSI_JUMP_THRESHOLD_MAX );
         }
         if (SetConf.eStatus == ePHY_SET_SERVICE_UNAVAILABLE) {
            DBG_printf( "Failed to set RSSI jump threshold" );
         }
      }
   }

   return ( 0 );
}
#endif // RA6E1 Bob: This command was removed from original K24 code
/******************************************************************************

   Function Name: DBG_CommandLine_CCA

   Purpose: This function displays/Set the CCA threshold

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_CCA ( uint32_t argc, char *argv[] )
{
   int16_t threshold;

   // More than one parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      // No parameters
      if ( argc == 1 )
      {
         PHY_GetConf_t GetConf;
         // Get the CCA threshold
         GetConf = PHY_GetRequest( ePhyAttr_CcaThreshold );
         if (GetConf.eStatus == ePHY_GET_SUCCESS) {
            DBG_printf( "CCA Threshold is %d dBm", GetConf.val.CcaThreshold );
         }
      } else {
         PHY_SetConf_t SetConf;
         threshold = ( int16_t )atoi( argv[1] );
         SetConf = PHY_SetRequest( ePhyAttr_CcaThreshold, &threshold);
         if (SetConf.eStatus == ePHY_SET_INVALID_PARAMETER) {
            DBG_logPrintf( 'R', "ERROR - Invalid range. Must be between %d and %d",
                           PHY_CCA_THRESHOLD_MIN, PHY_CCA_THRESHOLD_MAX );
         }
         if (SetConf.eStatus == ePHY_SET_SERVICE_UNAVAILABLE) {
            DBG_printf( "Failed to set CCA" );
         }
      }
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_CCAAdaptiveThreshold

   Purpose: This function displays the CCA adaptive threshold

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_CCAAdaptiveThreshold ( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint8_t numChannels;
   uint32_t i;

   // Get the number of channels
   GetConf = PHY_GetRequest( ePhyAttr_NumChannels );
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      numChannels = GetConf.val.NumChannels;

      // Get the CCA adaptive threshold
      GetConf = PHY_GetRequest( ePhyAttr_CcaAdaptiveThreshold );
      if (GetConf.eStatus == ePHY_GET_SUCCESS) {
         for ( i = 0; i < numChannels; i++ )
         {
            DBG_printf( "%d dBm", GetConf.val.CcaAdaptiveThreshold[i] );
         }
      }
   }
   return ( 0 );
}
/******************************************************************************

   Function Name: DBG_CommandLine_CCAAdaptiveThresholdEnable

   Purpose: This function displays/Set the CCA adaptive threshold enable

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_CCAAdaptiveThresholdEnable ( uint32_t argc, char *argv[] )
{
   bool enable;

   // More than one parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }
   else
   {
      // No parameters
      if ( argc == 1 )
      {
         PHY_GetConf_t GetConf;
         // Get the CCA adaptive threshold enable
         GetConf = PHY_GetRequest( ePhyAttr_CcaAdaptiveThresholdEnable );
         if (GetConf.eStatus == ePHY_GET_SUCCESS)
         {
            DBG_printf( "CCA Threshold Enable is %u", ( uint32_t )GetConf.val.CcaAdaptiveThresholdEnable );
         }
      }
      else
      {
         PHY_SetConf_t SetConf;
         enable = ( bool )atoi( argv[1] );
         SetConf = PHY_SetRequest( ePhyAttr_CcaAdaptiveThresholdEnable, &enable);
         if (SetConf.eStatus == ePHY_SET_SERVICE_UNAVAILABLE)
         {
            DBG_printf( "Failed to set CCA adaptive threshold enable" );
         }
      }
   }
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_CCAOffset

   Purpose: This function displays/Set the CCA offset

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_CCAOffset ( uint32_t argc, char *argv[] )
{
   uint8_t CcaOffset;

   // More than one parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      // No parameters
      if ( argc == 1 )
      {
         PHY_GetConf_t GetConf;
         // Get the CCA offset
         GetConf = PHY_GetRequest( ePhyAttr_CcaOffset );
         if (GetConf.eStatus == ePHY_GET_SUCCESS) {
            DBG_printf( "CCA Offset is %u dBm", GetConf.val.CcaOffset );
         }
      } else {
         PHY_SetConf_t SetConf;
         CcaOffset = ( uint8_t )atoi( argv[1] );
         SetConf = PHY_SetRequest( ePhyAttr_CcaOffset, &CcaOffset);
         if (SetConf.eStatus == ePHY_SET_INVALID_PARAMETER) {
            DBG_logPrintf( 'R', "ERROR - Invalid range. Must be between %d and %d", 0, PHY_CCA_OFFSET_MAX );
         }
         if (SetConf.eStatus == ePHY_SET_SERVICE_UNAVAILABLE) {
            DBG_printf( "Failed to set CCA offset" );
         }
      }
   }
   return ( 0 );
}

#if ( EP == 1 ) && ( TEST_TDMA == 1 )
/******************************************************************************

   Function Name: DBG_CommandLine_CsmaSkip

   Purpose: This function enable/disable skipping CSMA

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_CsmaSkip ( uint32_t argc, char *argv[] )
{
   uint8_t CsmaSkip;

   // More than one parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      // No parameters
      if ( argc == 1 )
      {
         MAC_GetConf_t GetConf;
         // Get the CSMA skip
         GetConf = MAC_GetRequest( eMacAttr_CsmaSkip );
         if (GetConf.eStatus == eMAC_GET_SUCCESS) {
            DBG_printf( "CSMA skip is %u", GetConf.val.CsmaSkip );
         }
      } else {
         CsmaSkip = ( uint8_t )atoi( argv[1] );
         MAC_SetRequest( eMacAttr_CsmaSkip, &CsmaSkip);
      }
   }
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_TxSlot

   Purpose: This function configure the test TDMA slot

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_TxSlot ( uint32_t argc, char *argv[] )
{
   uint8_t TxSlot;

   // More than one parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      // No parameters
      if ( argc == 1 )
      {
         MAC_GetConf_t GetConf;
         // Get the TX slot
         GetConf = MAC_GetRequest( eMacAttr_TxSlot );
         if (GetConf.eStatus == eMAC_GET_SUCCESS) {
            DBG_printf( "TX slot is %u", GetConf.val.TxSlot+1 );
         }
      } else {
         TxSlot = ( uint8_t )atoi( argv[1] );
         if ( TxSlot > 0 && TxSlot < 6) {
            TxSlot--;
            MAC_SetRequest( eMacAttr_TxSlot, &TxSlot);
         } else {
            DBG_printf( "Invalid range 1-5" );
         }
      }
   }
   return ( 0 );
}

#endif

/******************************************************************************

   Function Name: DBG_CommandLine_FrontEndGain

   Purpose: This function displays/Set the front end gain

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
#if ( DCU == 1 )
uint32_t DBG_CommandLine_FrontEndGain ( uint32_t argc, char *argv[] )
{
   int8_t gain;
   char   floatStr[PRINT_FLOAT_SIZE];

   // More than one parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      // No parameters
      if ( argc == 1 )
      {
         // Get the front end gain
         if ( PHY_FrontEndGain_Get(&gain) ) {
            DBG_printf( "Front end gain is %s dBm", DBG_printFloat( floatStr, ( float )gain / 2, 1 ) );
         }
      } else {
         DBG_printf( "NOTE: the gain entered is treated as half steps (e.g. 20 would be 10dBm)" );
         gain = ( int8_t )atoi( argv[1] );
         if ( PHY_FrontEndGain_Set(&gain) ) {
            DBG_printf( "Front end gain set to %i (i.e. %s dBm)", gain, DBG_printFloat( floatStr, ( float )gain / 2, 1 ) );
         } else {
            DBG_printf( "Failed to set front end gain" );
         }
      }
   }
   return ( 0 );
}
#endif

/******************************************************************************

   Function Name: DBG_CommandLine_NoiseEstimate

   Purpose: This function displays the noise estimate for all channels

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_NoiseEstimate ( uint32_t argc, char *argv[] )
{
   PHY_GetConf_t GetConf;
   uint8_t i;
   uint16_t Channel;

   // Get the noise estimate
   GetConf = PHY_GetRequest( ePhyAttr_NoiseEstimate );
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      for ( i = 0; i < PHY_MAX_CHANNEL; i++ )
      {
         (void)PHY_Channel_Get( i, &Channel );
         if ( PHY_IsChannelValid( Channel )) {
            DBG_printf( "Channel %4u %4d dBm", Channel, GetConf.val.NoiseEstimate[i] );
         }
      }
   }
#if ( EP == 1 )
   // Get the noise estimate when boost is on
   GetConf = PHY_GetRequest( ePhyAttr_NoiseEstimateBoostOn );
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      for ( i = 0; i < PHY_MAX_CHANNEL; i++ )
      {
         (void)PHY_Channel_Get( i, &Channel );
         if ( PHY_IsChannelValid( Channel )) {
            DBG_printf( "Boost On Channel %4u %4d dBm", Channel, GetConf.val.NoiseEstimateBoostOn[i] );
         }
      }
   }
#endif
   return ( 0 );
}
/******************************************************************************

   Function Name: DBG_CommandLine_NoiseEstimateCount

   Purpose: This function displays/Set the noise estimate count

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_NoiseEstimateCount ( uint32_t argc, char *argv[] )
{
   uint8_t NoiseCount;

   // More than one parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      // No parameters
      if ( argc == 1 )
      {
         PHY_GetConf_t GetConf;
         // Get the noise estimate count
         GetConf = PHY_GetRequest( ePhyAttr_NoiseEstimateCount );
         if (GetConf.eStatus == ePHY_GET_SUCCESS) {
            DBG_printf( "Noise estimate count is %u", GetConf.val.NoiseEstimateCount );
         }
      } else {
         PHY_SetConf_t SetConf;
         NoiseCount = ( uint8_t )atoi( argv[1] );
         SetConf = PHY_SetRequest( ePhyAttr_NoiseEstimateCount, &NoiseCount);
         if (SetConf.eStatus == ePHY_SET_INVALID_PARAMETER) {
            DBG_logPrintf( 'R', "ERROR - Invalid range. Must be between %d and %d",
                           PHY_NOISE_ESTIMATE_COUNT_MIN, PHY_NOISE_ESTIMATE_COUNT_MAX );
         }
         if (SetConf.eStatus == ePHY_SET_SERVICE_UNAVAILABLE) {
            DBG_printf( "Failed to set noise estimate count" );
         }
      }
   }
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_NoiseEstimateRate

   Purpose: This function displays/Set the noise estimate rate

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_NoiseEstimateRate ( uint32_t argc, char *argv[] )
{
   uint8_t rate;

   // More than one parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      // No parameters
      if ( argc == 1 )
      {
         PHY_GetConf_t GetConf;
         // Get the noise estimate rate
         GetConf = PHY_GetRequest( ePhyAttr_NoiseEstimateRate );
         if (GetConf.eStatus == ePHY_GET_SUCCESS) {
            DBG_printf( "Noise estimate rate is %u minutes", GetConf.val.NoiseEstimateRate );
         }
      } else {
         PHY_SetConf_t SetConf;
         rate = ( uint8_t )atoi( argv[1] );
         SetConf = PHY_SetRequest( ePhyAttr_NoiseEstimateRate, &rate);
         if (SetConf.eStatus == ePHY_SET_SERVICE_UNAVAILABLE) {
            DBG_printf( "Failed to set noise estimate rate" );
         }
      }
   }
   return ( 0 );
}

static uint8_t scaleAverage( double val, uint8_t min, uint8_t max )
{
   if (min == max) {
      return 0;
   }
   return (uint8_t)((((val-min)/(max-min))*256.0)+0.5);
}

static float unscaleAverage( uint8_t val, uint8_t min, uint8_t max )
{
   float avg;

   if (min == max) {
      return min;
   }
   avg = (float)((((double)val/256)*((double)max-min))+(double)min);

   // Make sure avg is not larger than max. This can happen because of poor granularity.
   if ( avg > max ) {
      avg = max;
   }
   return avg;
}

static uint8_t scaleStddev( double val )
{
   return (uint8_t)((log10(1+(val*NOISEBAND_SCALE_STDDEV))*100)+0.5);
}

static float unscaleStddev( uint8_t val )
{
   return (float)((pow( 10.0, (double)val/100.0 ) - 1.0)/NOISEBAND_SCALE_STDDEV);
}

/******************************************************************************

   Function Name: computeAvgAndStddev

   Purpose: This function computes the average and stddev of a RSSI buffer

   Arguments:  buf - buffer of RSSI data
               length - number of samples in buf
               average - linear average of buf returned in dBm
               stddev - linear standard deviation

   Returns:

   Notes:

******************************************************************************/
static void computeAvgAndStddev( uint8_t const *buf, uint16_t length, double *average, double *stddev )
{
   double   sum, temp;
   uint32_t i; // loop counter

   // Compute average
   sum = 0;
   for (i=0; i<length; i++) {
      sum += pow(10.0, ((double)buf[i]/20.0)); // Convert to linear and average. Divide by 20 because the RSSI is in half dB step
   }
   *average = log10(sum/length)*20; // Average in dBm in half dB step

   // Compute stddev
   sum = 0;
   for (i=0; i<length; i++) {
      temp = (*average-(double)buf[i])/2.0;
      sum += temp*temp;
   }
   *stddev = sqrt(sum/length);
}

/******************************************************************************

   Function Name: DBG_CommandLine_NoiseBand

   Purpose: This function displays/compute the noise for a range of channels

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
static int cmpfunc( const void *a, const void *b);
static int cmpfunc( const void *a, const void *b) {
  return *(char*)a - *(char*)b;
}

#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
typedef struct
{
   char RadioSPI;
   char RadioSDN;
   char RadioCS;
   char RadioOSC;
   char RadioGPIO;
   char JtagPins;
   char Unused;
   char Meter;
} ports_s;

static char upperCase(char letter)
{
   if ( ( (uint8_t)letter >= (uint8_t)'a' ) && ( (uint8_t)letter <= (uint8_t)'z' ) )
   {
      return ( (char)((uint8_t)letter - (uint8_t)'a' + (uint8_t)'A' ) );
   } else {
      return ( letter );
   }
}

#warning "You have built the enhanced version of Noiseband that allows it to change many port configurations!"

static bool checkOptions( char inputByte, char *option, char pAllowed[] )
{
   char inputUpper = upperCase( inputByte );
   if ( inputByte == 0 )
   {
      return ( (bool)true ); // this option was defaulted so we use the previous one without updating it
   }
   else
   {
      for ( uint32_t j = 0; j < 10; j++ )
      {
         if ( pAllowed[j] == 0 )   // search through the string of allowed options until null termination
         {
            return( (bool)false ); // reached the end of our allowed options, no match
         }
         if ( pAllowed[j] == inputUpper )
         {
            break; // got a match, break out of loop
         }
      }
   }
   *option = inputUpper;  // return the matching option for use in this run
   return ( (bool)true ); // found a matching option
}
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )

uint32_t DBG_CommandLine_NoiseBand ( uint32_t argc, char *argv[] )
{
   uint32_t      time;
   PHY_GetConf_t GetConf;
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 0 ) // Just to keep .hex file same as K24
   uint16_t      waittime, nSamples, samplingRate, i, j;
   uint8_t       radioNum;
#else
   uint16_t      i, j;
#endif
   double        average, stddev;
   uint8_t       min, max;
   uint32_t      freeRam;
   NOISEBAND_Stats_t *stats;

   static uint16_t nbChannels = 0;
   static uint8_t *workBuf = NULL;
   static NOISEBAND_Stats_t *noiseResults = NULL;
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
   static uint8_t  radioNum = 0;
   static uint16_t waittime = 15, nSamples = 80, samplingRate = 1000, requestedSamplingRate = 1000;
   static uint16_t start = 0;
   static uint16_t end   = 3200;
   static uint16_t step  = 2;
   static uint8_t  boost = 0;
   static uint32_t channelsBeforeDelay = 3200 / 2;
   static uint8_t  testModeHMC = 0;
   static ports_s  portPins, ports = { '_', '_', '_', '_', '_', '_', '_', '_' };
   OS_TICK_Struct  time1, time2;
   uint32_t        TimeDiff;
   static float    lowestCapVoltage = 9.99;
#else
   static uint16_t start = 0;
   static uint16_t end   = 0;
   static uint16_t step  = 0;
   static uint8_t  boost = 0;
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )

#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
   static bool     listFreqs = (bool)false;

   if ( argc == 2 ) // Special handling for noiseband f = list frequencies.  Might want to make this permanent.
   {
      if ( ( *argv[1] == 'f' ) || ( *argv[1] == 'F' ) )
      {
         listFreqs = (bool)true;
         --argc;
      }
      if ( ( *argv[1] == 'v' ) || ( *argv[1] == 'V' ) )
      {
         DBG_printf("lowestCap=%d.%02dV", (uint32_t)(lowestCapVoltage), ((uint32_t)(lowestCapVoltage*100)) % 100U );
         return ( 0 );
      }
   }
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
   // No parameters
   if ( argc == 1 )
   {
      // Print data if available
      if ( noiseResults )
      {
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 0 )
         DBG_printf( "   min    med    max     avg stddev    P90    P95    P99   P995   P999");
#else
         if ( listFreqs ) {
            DBG_printf( "\r\nFrequency   min    med    max     avg stddev    P90    P95    P99   P995   P999 'noiseband %d %d %d %d %d %d %d %d %d' "
                        "'rSPI:%c rSDN:%c rCS:%c rOSC:%c rGPIO:%c JTAG:%c Unused:%c Meter:%c' 'lowestCap=%d.%02dV'"
                        " 'Radio GPIO=%02x%02x%02x%02x%02x'",
                          radioNum, waittime, nSamples, requestedSamplingRate, start, end, step, boost, testModeHMC, \
                          ports.RadioSPI, ports.RadioSDN, ports.RadioCS, ports.RadioOSC, ports.RadioGPIO, ports.JtagPins, ports.Unused, ports.Meter,
                          (uint32_t)(lowestCapVoltage), ((uint32_t)(lowestCapVoltage*100)) % 100U,
                          sArgStream.GPIO0, sArgStream.GPIO1, sArgStream.GPIO2, sArgStream.GPIO3, sArgStream.gen_config
                          );
         } else {
            DBG_printf( "\r\nmin    med    max     avg stddev    P90    P95    P99   P995   P999 'noiseband %d %d %d %d %d %d %d %d %d' "
                        "'rSPI:%c rSDN:%c rCS:%c rOSC:%c rGPIO:%c JTAG:%c Unused:%c Meter:%c' 'lowestCap=%d.%02dV' "
                        " 'Radio GPIO=%02x%02x%02x%02x%02x'",
                          radioNum, waittime, nSamples, requestedSamplingRate, start, end, step, boost, testModeHMC, \
                          ports.RadioSPI, ports.RadioSDN, ports.RadioCS, ports.RadioOSC, ports.RadioGPIO, ports.JtagPins, ports.Unused, ports.Meter,
                          (uint32_t)(lowestCapVoltage), ((uint32_t)(lowestCapVoltage*100)) % 100U,
                          sArgStream.GPIO0, sArgStream.GPIO1, sArgStream.GPIO2, sArgStream.GPIO3, sArgStream.gen_config
                          );
         }
#endif
         //List seperately so existing test programs are not affected
         for ( j=0, i = start; i <= end; i += step, j++ )
         {
           OS_TASK_Sleep( 10 );
           // Get data into the appropriate buffer
            stats = &noiseResults[j];
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
            if ( listFreqs )
            {
               DBG_printf( "%3d.%04d %4d.%1d %4d.%1d %4d.%1d %4d.%02d %3u.%02u %4d.%1d %4d.%1d %4d.%1d %4d.%1d %4d.%1d",
                      (int32_t)(450+(i*20/3200)), (int32_t)((450UL*10000UL+((uint32_t)i*10000UL*20UL/3200UL))%10000UL),
                      (int32_t)RSSI_RAW_TO_DBM(stats->min ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->min )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->med ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->med )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->max ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->max )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(unscaleAverage(stats->avg, stats->min, stats->max)), abs((int32_t)(RSSI_RAW_TO_DBM(unscaleAverage(stats->avg, stats->min, stats->max))*100)%100),
                      (uint32_t)unscaleStddev( stats->stddev ), (uint32_t)(unscaleStddev(stats->stddev)*100)%100,
                      (int32_t)RSSI_RAW_TO_DBM(stats->P90 ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->P90 )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->P95 ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->P95 )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->P99 ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->P99 )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->P995), abs((int32_t)(RSSI_RAW_TO_DBM(stats->P995)*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->P999), abs((int32_t)(RSSI_RAW_TO_DBM(stats->P999)*10)%10));
            }
            else
#endif
            {
               DBG_printf( "%4d.%1d %4d.%1d %4d.%1d %4d.%02d %3u.%02u %4d.%1d %4d.%1d %4d.%1d %4d.%1d %4d.%1d",
                      (int32_t)RSSI_RAW_TO_DBM(stats->min ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->min )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->med ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->med )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->max ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->max )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(unscaleAverage(stats->avg, stats->min, stats->max)), abs((int32_t)(RSSI_RAW_TO_DBM(unscaleAverage(stats->avg, stats->min, stats->max))*100)%100),
                      (uint32_t)unscaleStddev( stats->stddev ), (uint32_t)(unscaleStddev(stats->stddev)*100)%100,
                      (int32_t)RSSI_RAW_TO_DBM(stats->P90 ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->P90 )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->P95 ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->P95 )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->P99 ), abs((int32_t)(RSSI_RAW_TO_DBM(stats->P99 )*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->P995), abs((int32_t)(RSSI_RAW_TO_DBM(stats->P995)*10)%10),
                      (int32_t)RSSI_RAW_TO_DBM(stats->P999), abs((int32_t)(RSSI_RAW_TO_DBM(stats->P999)*10)%10));
            }
            OS_TASK_Sleep( 1 );
         }

         // Free buffers
         free( workBuf );
         workBuf = NULL;
         free( noiseResults );
         noiseResults = NULL;
      } else {
         DBG_printf( "NoiseBand  displays/compute the noise for a range of channels:" );
         DBG_printf( "usage: noiseband radio waittime nSamples samplingRate start end step boost" );
         DBG_printf( "       radio is the radio to use for RX (0-8)" );
         DBG_printf( "       waittime is the time in seconds that the test will be postponed" );
         DBG_printf( "       nSamples is the number of samples used for averaging and std deviation (1-20000)" );
         DBG_printf( "       samplingRate is the time in microseconds between RSSI reads" );
         DBG_printf( "       start is the first channel to get RSSI for" );
         DBG_printf( "       end is the last channel to get RSSI for" );
         DBG_printf( "       step is the increment between channels (minimum 2)" );
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 0 )
         DBG_printf( "       boost (opt.) 0=LDO only(default), 1=LDO and Boost" );
#else
         DBG_printf( "       (optional) boost 0=LDO only(default), 1=Boost, abort if Vcap is low, 2=Boost, ignore Vcap, 3=Boost, wait for Vcap" );
         DBG_printf( "       (optional) HMC traffic: 0=Normal, 1=One LogOff/LogOn, 2=One Update LCD, 3=Disable other tasks, 4=Update LP Data" );
         DBG_printf( "                               5=Update LCD Continuously" );
         DBG_printf( "       (optional) port pin configuration during test: 12345678 or ________" );
         DBG_printf( "           1=rSPIdrv:H|M|L 2=rSDNdrv:H|M|L|N|P 3=rCSdrv:H|M|L|N|P 4=rOSCdrv:H|M|L|N|P 5=MCUrGPIO:I|U 6=JTAG:H|L|J 7=Unused:L|H|I|P 8=Meter:L|H|I|P " );
         DBG_printf( "             For 1,2,3,4: H=High Drive, M=Mid Drive, L=Low Drive, N=NMOS, P=PMOS" );
         DBG_printf( "             For 5,6,7,8: H=High Output/Mid Drive, L=Low Output/Mid Drive, I=Input, U=Pulled-Up Input, J=JTAG" );
         DBG_printf( "             Do NOT use L or H for option 8 when a meter is connected as the output pins will conflict" );
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 0 )
      }
      return ( 0 );
   }
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
   if ( argc > 11 )
#else
   if ( argc > 9 ) // Keep code identical for K24
#endif
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
      return ( 0 );
   }
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
   if ( ( argc == 2 ) && ( ( *argv[1] == 'd' ) || ( *argv[1] == 'D' ) ) )
   {
      radioNum     = 0;
      waittime     = 5;
      nSamples     = 80;
      samplingRate = 1000;
      requestedSamplingRate = samplingRate;
      start        = 0;
      end          = 3200;
      step         = 2;
      boost        = 0;  //Default to Off
      testModeHMC  = 0;
      listFreqs    = (bool)false;
      portPins     = ports;
   }
   else
#endif
   if ( argc < 8 )
   {
      DBG_logPrintf( 'R', "ERROR - Not enough arguments" );
      return ( 0 );
   }
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
   else
   {
#endif
      radioNum     = ( uint8_t  )atoi( argv[1] );
      waittime     = ( uint16_t )atoi( argv[2] );
      nSamples     = ( uint16_t )atoi( argv[3] );
      samplingRate = ( uint16_t )atoi( argv[4] );
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
      requestedSamplingRate = samplingRate;
#endif
      start        = ( uint16_t )atoi( argv[5] );
      end          = ( uint16_t )atoi( argv[6] );
      step         = ( uint16_t )atoi( argv[7] );
      boost        = 0;  //Default to Off
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
      if ( argc == 9 ) // Keep code identical for K24
#else
      if ( argc >= 9 )
#endif
      {
         boost = ( uint8_t )atoi( argv[8] );
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 ) // Keep code identical for K24
         if ( ( boost == 1 ) && ( ADC_Get_SC_Voltage() < 2.3f ) )
         {
            DBG_printf( "You have requested boost mode but super-cap voltage < 2300 mV\r\n"
                        "  If you want to go ahead anyway, re-run and enter '2' for boost instead of 1\r\n"
                        "  If you want to wait until the voltage reaches 2300mV, enter '3' for boost" );
            return ( 0 );
         }
         float lv = ADC_Get_SC_Voltage();
         while ( ( boost == 3 ) && ( lv < 2.3f ) )
         {
            DBG_printf(" You have requested boost mode but super-cap is only up to %u mV, waiting to reach 2300 mV", (uint32_t)(lv * 1000.0) );
            OS_TASK_Sleep ( 2000 );
            lv = ADC_Get_SC_Voltage();
         }
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
      }
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
      if ( argc >= 10 )
      {
         testModeHMC = ( uint8_t )atoi( argv[9] ); // test mode for HMC traffic during noiseband test
      }

      portPins = ports; // Start off with the values from the previous run
      if ( argc >= 11 )
      {
         ports_s *pPorts = (ports_s *)argv[10]; /* Point to the string containing the 9th parameter to noiseband */
         bool optionsOK = (bool)false;
         if (                      checkOptions( pPorts->RadioSPI,  (char *)&portPins.RadioSPI,  "_HML"   ) )  {
            if (                   checkOptions( pPorts->RadioSDN,  (char *)&portPins.RadioSDN,  "_HMLNP" ) )  {
               if (                checkOptions( pPorts->RadioCS,   (char *)&portPins.RadioCS,   "_HMLNP" ) )  {
                  if (             checkOptions( pPorts->RadioOSC,  (char *)&portPins.RadioOSC,  "_HMLNP" ) )  {
                     if (          checkOptions( pPorts->RadioGPIO, (char *)&portPins.RadioGPIO, "_IU"    ) )  {
                        if (       checkOptions( pPorts->JtagPins,  (char *)&portPins.JtagPins,  "_HLJ"   ) )  {
                           if (    checkOptions( pPorts->Unused,    (char *)&portPins.Unused,    "_IUHL"  ) )  {
                              if ( checkOptions( pPorts->Meter,     (char *)&portPins.Meter,     "_IUHL"  ) )
                              {
                                 optionsOK = (bool)true;
                              } else { DBG_printf( "Error: Meter = %c, must be I|U|H|L to set the meter interface port pins or _ to leave them as is", portPins.Meter );    }
                           } else { DBG_printf( "Error: Unused pins = %c, must be I|U|H|L to set unused pins or _ to leave them as is", portPins.Unused );                  }
                        } else { DBG_printf( "Error: JtagPins = %c, must be H|L|J to set JTAG pins or _ to leave them as is", portPins.JtagPins );                          }
                    } else { DBG_printf( "Error: RadioGPIO = %c, must be I|U to set MCU pins connected to Radio GPIOn pins or _ to leave them as is", portPins.RadioGPIO ); }
                 } else { DBG_printf( "Error: RadioOSC = %c, must be H|M|L|N|P to set Radio SPI chip select drive strength or _ to leave them as is", portPins.RadioOSC );  }
               } else { DBG_printf( "Error: RadioCS = %c, must be H|M|L|N|P to set Radio SPI Chip Select drive strength or _ to leave them as is", portPins.RadioCS );      }
            } else { DBG_printf( "Error: RadioSDN = %c, must be H|M|L|N|P to set Radio SDN pin drive strength or _ to leave it as is", portPins.RadioSDN );                 }
         } else { DBG_printf( "Error: RadioSPI = %c, must be H|M|L to set Radio SPI pins drive strength or _ to leave them as is", portPins.RadioSPI );                 }
         if ( ! optionsOK )
         {
            return ( 0 );
         }
      }
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
      if ( radioNum >= (uint8_t)MAX_RADIO )
      {
         DBG_logPrintf( 'R', "ERROR - invalid radio" );
         return ( 0 );
      }
      if ( waittime > 600 ) /* Don't wait more than 10 minutes */
      {
         DBG_logPrintf( 'R', "ERROR - waittime too long" );
         return ( 0 );
      }
      if ( nSamples > NOISEBAND_MAX_NSAMPLES )
      {
         DBG_logPrintf( 'R', "ERROR - nSamples over %u", NOISEBAND_MAX_NSAMPLES );
         return ( 0 );
      }
      if ( nSamples == 0 )
      {
         nSamples = 1;
      }
      if ( start >= PHY_INVALID_CHANNEL )
      {
         DBG_logPrintf( 'R', "ERROR - start channel is invalid" );
         return ( 0 );
      }
      if ( end >= PHY_INVALID_CHANNEL )
      {
         DBG_logPrintf( 'R', "ERROR - end channel is invalid" );
         return ( 0 );
      }
      if ( end < start )
      {
         DBG_logPrintf( 'R', "ERROR - end channel must be greater than or equal to start channel" );
         return ( 0 );
      }
      if ( step < 2 )
      {
         step = 2;
      }
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
   }
#endif

   nbChannels = ( uint16_t )( ( end - start ) / step + 1 ); /* Number of channel to scan *//*lint !e573 mixed signed-unsigned with division  */
   if ( nbChannels > NOISEBAND_MAX_CHANNELS ) {
      DBG_printf("Can't process more than %u channels at once", NOISEBAND_MAX_CHANNELS);
      return ( 0 );
   }

#if ( RTOS_SELECTION == MQX_RTOS )
   freeRam = (uint32_t)_mem_get_free();
#elif (RTOS_SELECTION == FREE_RTOS)
   HeapStats_t hd;
   (void) vPortGetHeapStats ( &hd );              /* Get the statistics for the heap.  This assumes the use of heap_4.c */
   freeRam = hd.xSizeOfLargestFreeBlockInBytes;   /* We need a contiguous block so find size of largest one */
   DBG_printf("\n\rFreeRTOS vPortGetHeapStats returned %d bytes are available in the largest block", freeRam);
#endif

#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 ) // just to produce same .hex file
   // Free previous buffers, if any, prior to allocating new ones
   if ( workBuf      != NULL) { free( workBuf );      workBuf = NULL;      }
   if ( noiseResults != NULL) { free( noiseResults ); noiseResults = NULL; }
#endif
   // Reserve RAM for processing
   // 1) Reserve work area
   if ( (workBuf = (uint8_t*)malloc( nSamples )) == NULL) {
      DBG_printf("%u bytes needed out of %u available", nSamples, freeRam);
      DBG_printf("Reduce the number of samples and/or the number of channels");
      return ( 0 );
   }

   // 2) Reserve statistics buffer
   if ( (noiseResults = (NOISEBAND_Stats_t*)malloc( sizeof(NOISEBAND_Stats_t)*nbChannels )) == NULL) {
      DBG_printf("%u bytes needed out of %u available", nSamples+(sizeof(NOISEBAND_Stats_t)*nbChannels), freeRam);
      DBG_printf("Reduce the number of samples and/or the number of channels");
      free( workBuf );
      workBuf = NULL;
      return ( 0 );
   }

#if ( MCU_SELECTED == NXP_K24 )
#define MINIMUM_TIME 320 // microseconds
#define PAUSE_MSEC     5 // milliseconds
   // This minimum sampling rate is around 320usec per read.
   // RSSI read will take a minimum time which is bounded by the SPI rate and radio turn around time.
#elif ( MCU_SELECTED == RA6E1 )
#define MINIMUM_TIME 130 // microseconds based on OS_TICK_Get_Diff_InMicroseconds measurements on RA6E1/FreeRTOS
#define PAUSE_MSEC     5 // milliseconds
#endif
   if (samplingRate < MINIMUM_TIME) {
      samplingRate = MINIMUM_TIME;
   }
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 0 ) // Following is identical to K24
   time = ( uint32_t )( nbChannels * ((float)samplingRate/1000.0f) * ((float)nSamples/1000.0f) ); /* Rough time in seconds */
#else
   time  = (uint32_t)( nbChannels * ( (float)samplingRate/1000.0f) * ((float)nSamples ) ); /* Rough time in milliseconds */
   /* Add in Stats: 52.6usec/sample/channel, Qsort: 1.14usec/sample/channel, PHY_Lock+PHY_Unlock+tabulate: 133usec/channel */
   time += (uint32_t)( nbChannels * ( ( (float)nSamples * 0.06f ) + 0.150f ) );
   time += (uint32_t)( nbChannels * ( (float)( samplingRate - MINIMUM_TIME ) * 0.015f/1000.0f ) * ((float)nSamples )); /* OS_TICK_Get_Diff_InMicroseconds runs 1.5% high */
   #define ACTUAL_PAUSE_MSEC  ((float)PAUSE_MSEC + 3.184f) /* OS_TASK_Sleep has a maximum average delay of 8.184msec delay when 5msec is requested under FreeRTOS */
   #define DELAY_SEC          10 // How long to delay between batches of DELAY_CHANS channels
   #define ENERGY_IN_CAP 3600000 // Magic number to keep super-cap voltage above 2.1V
   time += ( (uint32_t) ( nbChannels * ACTUAL_PAUSE_MSEC ) ) + 1000 * (1 + waittime);     /* account for PAUSE_MSEC and requested delay at start */
   if ( boost != 0 )
   {
      time += (uint32_t)nbChannels * (PWR_3P6LDO_EN_ON_DLY_MS + PWR_3V6BOOST_EN_ON_DLY_MS ); // Boost/LDO time delays
   }
   if ( testModeHMC == 5 ) time = (uint32_t)( time * 1.15 ); /* account for HMC running at a much higher duty-cycle */
   bool delayDuringTest = (bool)false;
   if ( (boost != 0) && ( (uint32_t)nSamples * (uint32_t)samplingRate * (uint32_t)nbChannels > ENERGY_IN_CAP ) )
   {  /* At risk of depleting super-cap during this run.  Insert delays between every channelsBeforeDelay channels */
      delayDuringTest = (bool)true;
      channelsBeforeDelay = ENERGY_IN_CAP / ( (uint32_t)nSamples * (uint32_t)samplingRate );
      time += (uint32_t)( ( ( nbChannels - 2 ) / channelsBeforeDelay ) * DELAY_SEC * 1000 );
      DBG_printf( "A delay of %u msec will be inserted every %u channel steps for super-cap recharging", (uint32_t)(DELAY_SEC*1000), channelsBeforeDelay );
   }
   time = (uint32_t)( ( time + 500 ) /1000 );
#endif
   // Remove the time it takes to read RSSI from the sampling rate so that we read RSSI at the specified rate
   if (samplingRate > MINIMUM_TIME) {
      samplingRate -= MINIMUM_TIME;
   } else {
      samplingRate = 0;
   }
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
// R_BSP_PinCfg (BSP_IO_PORT_04_PIN_06, ((uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH | (uint32_t)IOPORT_CFG_DRIVE_MID )); // P406: TP121 (used for timing verification)
   uint32_t portConfig;
#define IOPORT_CFG_DRIVE_LOW 0UL

/* If non-blank, set up configurations for drive strength of the Radio SPI port pins */
   if ( ( portPins.RadioSPI != ' ' ) && ( portPins.RadioSPI != '_' ) )
   {
      portConfig = ((uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SPI );
      if        ( portPins.RadioSPI == 'H' ) {
         portConfig |= ((uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT );
      } else if ( portPins.RadioSPI == 'M' ) {
         portConfig |= ((uint32_t)IOPORT_CFG_DRIVE_LOW  | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT );
      } else if ( portPins.RadioSPI == 'L' ) {
         portConfig |= ((uint32_t)IOPORT_CFG_DRIVE_MID  | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT );
      } else if ( portPins.RadioSPI == 'N' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_LOW   | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_NMOS_ENABLE ); // Doesn't work in HW, causes radio errors
      } else if ( portPins.RadioSPI == 'P' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_LOW   | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PMOS_ENABLE ); // Doesn't work in HW, causes radio errors
      } else {
        return ( 0 ); // Shouldn't get here unless a mismatch exists between options allowed here and string passed to checkOptions()
      }
      ports.RadioSPI = portPins.RadioSPI;                 // remember valid configuration in static memory for next run
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_01, portConfig ); // P101: SPIB_MOSI
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_02, portConfig ); // P102: SPIB_SCK
   }
/* If non-blank, set up configurations for drive strength and output type of the SDN_SI4467 (radio shutdown) signal */
   if ( ( portPins.RadioSDN != ' ' ) && ( portPins.RadioSDN != '_' ) )
   {
      if        ( portPins.RadioSDN == 'H' ) {
         portConfig = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW );
      } else if ( portPins.RadioSDN == 'M' ) {
         portConfig = ((uint32_t) IOPORT_CFG_DRIVE_MID  | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW );
      } else if ( portPins.RadioSDN == 'L' ) {
         portConfig = ((uint32_t) IOPORT_CFG_DRIVE_LOW  | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW );
      } else if ( portPins.RadioSDN == 'N' ) {
         portConfig = ((uint32_t) IOPORT_CFG_DRIVE_LOW  | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW | (uint32_t)IOPORT_CFG_NMOS_ENABLE );
      } else if ( portPins.RadioSDN == 'P' ) {
         portConfig = ((uint32_t) IOPORT_CFG_DRIVE_LOW  | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW | (uint32_t)IOPORT_CFG_PMOS_ENABLE );
      }
      ports.RadioSDN = portPins.RadioSDN;                 // remember valid configuration in static memory for next run
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_04, portConfig ); // P104: SDN_SI4467
   }
/* If non-blank, set up configurations for drive strength and output type of the /SPIB_CS_SI4467 chip select signal */
   if ( ( portPins.RadioCS != ' ' ) && ( portPins.RadioCS != '_' ) )
   {
      if        ( portPins.RadioCS == 'H' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_HIGH  | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SPI );
      } else if ( portPins.RadioCS == 'M' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_LOW   | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SPI );
      } else if ( portPins.RadioCS == 'L' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_MID   | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SPI );
      } else if ( portPins.RadioCS == 'N' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_LOW   | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SPI | (uint32_t)IOPORT_CFG_NMOS_ENABLE );
      } else if ( portPins.RadioCS == 'P' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_LOW   | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SPI | (uint32_t)IOPORT_CFG_PMOS_ENABLE );
      }
      ports.RadioCS = portPins.RadioCS;                   // remember valid configuration in static memory for next run
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_03, portConfig ); // P103: /SPIB_CS_SI4467
   }
 /* If non-blank, set up configurations for drive strength of the Oscillator Enable port pin */
   if ( ( portPins.RadioOSC != ' ' ) && ( portPins.RadioOSC != '_' ) )
   {
      if        ( portPins.RadioOSC == 'H' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_HIGH  | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SPI );
      } else if ( portPins.RadioOSC == 'M' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_LOW   | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SPI );
      } else if ( portPins.RadioOSC == 'L' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_MID   | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SPI );
      } else if ( portPins.RadioOSC == 'N' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_LOW   | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SPI | (uint32_t)IOPORT_CFG_NMOS_ENABLE );
      } else if ( portPins.RadioOSC == 'P' ) {
         portConfig = ((uint32_t)IOPORT_CFG_DRIVE_LOW   | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SPI | (uint32_t)IOPORT_CFG_PMOS_ENABLE );
      }
      ports.RadioOSC = portPins.RadioOSC;                  // remember valid configuration in static memory for next run
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_05, portConfig );  // P105: /OSC_EN
   }
 /* If non-blank, then set up configurations for MCU pins connected to the Radio GPIOn_SI4467 pins; do not allow them to be outputs as the SI4467 drives them! */
   if ( ( portPins.RadioGPIO != ' ' ) && ( portPins.RadioGPIO != '_' ) )
   {
      if        ( portPins.RadioGPIO == 'U' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_INPUT  | (uint32_t)IOPORT_CFG_PULLUP_ENABLE                                                 ); // Input with pull-up enabled
      } else if ( portPins.RadioGPIO == 'I' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_INPUT                                                                                       ); // Input without pull-up enabled
      }
      ports.RadioGPIO = portPins.RadioGPIO;               // remember valid configuration in static memory for next run
      R_BSP_PinCfg ( BSP_IO_PORT_06_PIN_00, portConfig ); // P600: GPIO0_SI4467
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_06, portConfig ); // P106: GPIO1_SI4467
      R_BSP_PinCfg ( BSP_IO_PORT_00_PIN_00, portConfig ); // P000: GPIO2_SI4467
      R_BSP_PinCfg ( BSP_IO_PORT_00_PIN_03, portConfig ); // P003: GPIO3_SI4467
    }
/* If non-blank, set up configurations for the JTAG and MD_BOOT pins during test.  Restored to JTAG after test or JTAG will no longer function! */
   if ( ( portPins.JtagPins != ' ' ) && ( portPins.JtagPins != '_' ) )
   {
      /* P300 and P201 are tied together on the board.  Therefore, make P300 an input first, then set P201 to OUTPUT_xxx, then set P300 to OUTPUT_xxx */
      R_BSP_PinCfg ( BSP_IO_PORT_03_PIN_00, ((uint32_t)IOPORT_CFG_PORT_DIRECTION_INPUT )); // P300: TCLK/SWCLK
      if        ( portPins.JtagPins == 'L' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_LOW       | (uint32_t)IOPORT_CFG_DRIVE_MID        );
      } else if ( portPins.JtagPins == 'H' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH      | (uint32_t)IOPORT_CFG_DRIVE_MID        );
      } else if ( portPins.JtagPins == 'J' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PERIPHERAL_PIN        | (uint32_t)IOPORT_PERIPHERAL_DEBUG                                                  );
      }
      ports.JtagPins = portPins.JtagPins;                 // remember valid configuration in static memory for next run
      R_BSP_PinCfg ( BSP_IO_PORT_02_PIN_01, portConfig ); // P201: MD_BOOT
      R_BSP_PinCfg ( BSP_IO_PORT_03_PIN_00, portConfig ); // P300: TCLK/SWCLK
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_08, portConfig ); // P108: TMS/SWDIO
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_09, portConfig ); // P109: TDO
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_10, portConfig ); // P110: TDI
   }
/* If non-blank, then set up configurations for unused / unconnected pins.  Didn't bother with drive strength options since these signals do not toggle */
   if ( ( portPins.Unused != ' ' ) && ( portPins.Unused != '_' ) )
   {
      if        ( portPins.Unused == 'U' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_INPUT  | (uint32_t)IOPORT_CFG_PULLUP_ENABLE                                                 ); // Input with pull-up enabled
      } else if ( portPins.Unused == 'I' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_INPUT                                                                                       ); // Input without pull-up enabled
      } else if ( portPins.Unused == 'H' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH      | (uint32_t)IOPORT_CFG_DRIVE_MID        ); // Output in HIGH state
      } else if ( portPins.Unused == 'L' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_LOW       | (uint32_t)IOPORT_CFG_DRIVE_MID        ); // Output in LOW state
      }
      ports.Unused = portPins.Unused;                     // remember valid configuration in static memory for next run
      R_BSP_PinCfg ( BSP_IO_PORT_00_PIN_14, portConfig ); // P014: unused
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_11, portConfig ); // P111: unused
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_15, portConfig ); // P115: unused
      R_BSP_PinCfg ( BSP_IO_PORT_02_PIN_05, portConfig ); // P205: unused
      R_BSP_PinCfg ( BSP_IO_PORT_02_PIN_08, portConfig ); // P208: unused
      R_BSP_PinCfg ( BSP_IO_PORT_02_PIN_09, portConfig ); // P209: unused
      R_BSP_PinCfg ( BSP_IO_PORT_02_PIN_10, portConfig ); // P210: unused
      R_BSP_PinCfg ( BSP_IO_PORT_02_PIN_11, portConfig ); // P211: unused
      R_BSP_PinCfg ( BSP_IO_PORT_02_PIN_14, portConfig ); // P214: unused
      R_BSP_PinCfg ( BSP_IO_PORT_03_PIN_01, portConfig ); // P301: unused
      R_BSP_PinCfg ( BSP_IO_PORT_03_PIN_02, portConfig ); // P302: unused
      R_BSP_PinCfg ( BSP_IO_PORT_03_PIN_03, portConfig ); // P303: unused
      R_BSP_PinCfg ( BSP_IO_PORT_03_PIN_04, portConfig ); // P304: unused
      R_BSP_PinCfg ( BSP_IO_PORT_03_PIN_05, portConfig ); // P305: unused
      R_BSP_PinCfg ( BSP_IO_PORT_04_PIN_02, portConfig ); // P402: unused
      R_BSP_PinCfg ( BSP_IO_PORT_04_PIN_03, portConfig ); // P403: unused
      R_BSP_PinCfg ( BSP_IO_PORT_04_PIN_06, portConfig ); // P406: TP121
      R_BSP_PinCfg ( BSP_IO_PORT_04_PIN_07, portConfig ); // P407: unused
      R_BSP_PinCfg ( BSP_IO_PORT_04_PIN_10, portConfig ); // P410: unused
      R_BSP_PinCfg ( BSP_IO_PORT_04_PIN_11, portConfig ); // P411: unused
      R_BSP_PinCfg ( BSP_IO_PORT_04_PIN_12, portConfig ); // P412: unused
      R_BSP_PinCfg ( BSP_IO_PORT_04_PIN_13, portConfig ); // P413: unused
      R_BSP_PinCfg ( BSP_IO_PORT_04_PIN_14, portConfig ); // P414: unused
      R_BSP_PinCfg ( BSP_IO_PORT_04_PIN_15, portConfig ); // P415: unused
      R_BSP_PinCfg ( BSP_IO_PORT_06_PIN_01, portConfig ); // P601: unused
      R_BSP_PinCfg ( BSP_IO_PORT_06_PIN_02, portConfig ); // P602: unused
      R_BSP_PinCfg ( BSP_IO_PORT_06_PIN_08, portConfig ); // P608: unused
      R_BSP_PinCfg ( BSP_IO_PORT_06_PIN_10, portConfig ); // P610: unused
      R_BSP_PinCfg ( BSP_IO_PORT_07_PIN_08, portConfig ); // P708: unused
   }
/* If non-blank, set up onfigurations for meter interface port pins (use only when NIC is NOT integrated with a meter) */
   if ( ( portPins.Meter != ' ' ) && ( portPins.Meter != '_' )  )
   {
      if        ( portPins.Meter == 'U' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_INPUT  | (uint32_t)IOPORT_CFG_PULLUP_ENABLE                                                 ); // Input with pull-up enabled
      } else if ( portPins.Meter == 'I' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_INPUT                                                                                       ); // Input without pull-up enabled
      } else if ( portPins.Meter == 'H' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH      | (uint32_t)IOPORT_CFG_DRIVE_MID        ); // Output in HIGH state
      } else if ( portPins.Meter == 'L' ) {
         portConfig = ((uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_LOW       | (uint32_t)IOPORT_CFG_DRIVE_MID        ); // Output in LOW state
      }
      ports.Meter = portPins.Meter;                      // remember valid configuration in static memory for next run
      R_BSP_PinCfg ( BSP_IO_PORT_00_PIN_01, portConfig); // P001: CBS_I-210+C_METER
      R_BSP_PinCfg ( BSP_IO_PORT_00_PIN_02, portConfig); // P002: CBF_BI_I-210+C_METER
      R_BSP_PinCfg ( BSP_IO_PORT_00_PIN_04, portConfig); // P004: MC_I-210+C_METER
      R_BSP_PinCfg ( BSP_IO_PORT_00_PIN_06, portConfig); // P006: /PF_METER
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_07, portConfig); // P107: AD_I-210+C_METER
      R_BSP_PinCfg ( BSP_IO_PORT_01_PIN_14, portConfig); // P114: ZCD_METER
      R_BSP_PinCfg ( BSP_IO_PORT_05_PIN_05, portConfig); // P505; T_I-210+C_METER
   }

   DBG_printf( "test should take %02u:%02u, %u seconds", time / 60, time % 60, time );
   OS_TICK_Get_CurrentElapsedTicks(&time1); /* Get the starting time for the entire test */

   bool updateDisplayDuringNoiseband = (bool)false;
   switch( testModeHMC )
   {
      case 1:
      {
         // Log off
         DBG_printf(" HMC- Log off ");
         (void)HMC_STRT_LogOn((uint8_t)HMC_APP_API_CMD_ABORT, (uint8_t far *)0);
         (void)HMC_FINISH_LogOff((uint8_t)HMC_APP_API_CMD_ACTIVATE, (uint8_t far *)0);
         // Log on
         DBG_printf(" HMC- Log ON ");
         (void)HMC_STRT_LogOn((uint8_t)HMC_APP_API_CMD_ACTIVATE, (uint8_t far *)0);

         break;
      }
      case 2:
      {
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
         DBG_printf(" HMC- Write NETREC to the Display ");
         HMC_DISP_UpdateDisplayBuffer( HMC_DISP_MSG_VALID_TIME, HMC_DISP_POS_PD_LCD_MSG ); //Update the Display
#endif
         break;
      }
      case 3:
      {
         DBG_printf("Increasing the Priorities of the related tasks ");
         /* Increase the priority of the DBG task and Print task */

         ( void )OS_TASK_Set_Priority( pTskName_Dbg, 11 );
         ( void )OS_TASK_Set_Priority( pTskName_Phy, 11 );
         ( void )OS_TASK_Set_Priority( pTskName_Sm, 11 );
         ( void )OS_TASK_Set_Priority( pTskName_Print, 11 ); // Increasing priority
         ( void )OS_TASK_Set_Priority( pTskName_Time, 11 );
         ( void )OS_TASK_Set_Priority( pTskName_Tmr, 11 );
         ( void )OS_TASK_Set_Priority( pTskName_Strt, 12 );

         ( void )OS_TASK_Set_Priority( pTskName_Idle, 13 );

         break;
      }
      case 4:
      {
         (void)HMC_MTRINFO_app( (uint8_t)HMC_APP_API_CMD_ACTIVATE, (void *)NULL ); //Request a Meter Info Read
         break;
      }
      case 5:
      {
         updateDisplayDuringNoiseband = (bool)true;
         DBG_printf( "To keep the HMC UART busy, noiseband will write to the meter's display as fast as possible" );
         HMC_DISP_UpdateDisplayBuffer( HMC_DISP_MSG_VALID_TIME, HMC_DISP_POS_PD_LCD_MSG ); //Update the Display
         break;
      }
      default:
         break;
   }
#else
   DBG_printf( "test should take %02u:%02u", time / 60, time % 60 );
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )

   OS_TASK_Sleep( waittime * ONE_SEC );

#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )  // just so we produce the same .hex file
   DBG_printf( "Noiseband start; delay between samples = %u, starting super-cap voltage=%u mV", samplingRate, (uint32_t)(ADC_Get_SC_Voltage()*1000.0) );

   PWR_3P6LDO_EN_ON();    // RA6E1 Bob: This should already be in this condition, just being sure
   PWR_3V6BOOST_EN_OFF(); // RA6E1 Bob: This should already be in this condition, just being sure

   OS_TASK_Sleep( ONE_SEC ); /* Make sure debug printout of the above message has completed before we start */

   union si446x_cmd_reply_union Si446xCmd;
   PHY_Lock();      // Function will not return if it fails
   /* Configure the radio's GPIO pins per the settings in sArgStream, from the noisebandclkon command */
   (void)si446x_gpio_pin_cfg( 0, sArgStream.GPIO0, sArgStream.GPIO1, sArgStream.GPIO2, sArgStream.GPIO3,
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING,
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING,
                                 sArgStream.gen_config, &Si446xCmd);
    /* Enable the radio's GPIO0 pin as a divided clock output */
   (void)si446x_set_property( 0,
                              SI446X_PROP_GRP_ID_GLOBAL,
                              1,
                              SI446X_PROP_GRP_INDEX_GLOBAL_CLK_CFG,
                              SI446X_PROP_GLOBAL_CLK_CFG_DIVIDED_CLK_EN_TRUE_BIT |          // Enable divided system clock output
                              SI446X_PROP_GLOBAL_CLK_CFG_DIVIDED_CLK_SEL_ENUM_DIV_30 << 3); // Divide clock by 30
   PHY_Unlock();    // Function will not return if it fails
   lowestCapVoltage = 9.99;

   static uint32_t oldTaskPriorityDBG, oldTaskPriorityPRN, oldTaskPriorityHMC, oldTaskPriorityPHY, oldTaskPriorityIDL;
   if ( updateDisplayDuringNoiseband )
   {
      oldTaskPriorityDBG = OS_TASK_Set_Priority( pTskName_Dbg,   19 );
      oldTaskPriorityPRN = OS_TASK_Set_Priority( pTskName_Print, 19 );
      oldTaskPriorityHMC = OS_TASK_Set_Priority( pTskName_Hmc,   17 );
      oldTaskPriorityPHY = OS_TASK_Set_Priority( pTskName_Phy,   18 );
      oldTaskPriorityIDL = OS_TASK_Set_Priority( pTskName_Idle,  24 );
      HMC_APP_SetScanDelay( 0UL ); /* Cause HMC_app to run Applets at an accelerated speed, no deliberate delays */
      HMC_DISP_SetContinuousUpdate(); /* This will cause HMC_display to continuously update the meter's LCD */
      HMC_DISP_UpdateDisplayBuffer( HMC_DISP_MSG_NOISEBAND, HMC_DISP_POS_PD_LCD_MSG ); //Update the Display
   }
#else
   DBG_logPrintf( 'R', "noiseband start" );
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )

#if ( TM_INSTRUMENT_NOISEBAND_TIMING == 1 )
   #define CAPTURE_TIMES 8
   OS_TICK_Struct  times [CAPTURE_TIMES];
   uint32_t        deltas[CAPTURE_TIMES-1];
   uint64_t        totals[CAPTURE_TIMES-1] = { 0 };
   #define CAPTURE_TIME(x) OS_TICK_Get_CurrentElapsedTicks( &times[x] )
#else
   #define CAPTURE_TIME(x)
#endif // ( TM_INSTRUMENT_NOISEBAND_TIMING == 1 )
   for ( j=0, i = start; i <= end; i += step, j++ )
   {
      CAPTURE_TIME(0); /* Get the starting time */
      PHY_Lock();      // Function will not return if it fails
      CAPTURE_TIME(1); /* Measure duration of PHY_Lock */
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 0 )
      RADIO_Get_RSSI( radioNum, i, workBuf, nSamples, samplingRate, boost);
#else
      float lv = RADIO_Get_RSSI( radioNum, i, workBuf, nSamples, samplingRate, boost);
      if ( lv < lowestCapVoltage ) lowestCapVoltage = lv;
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 0 )
      CAPTURE_TIME(2); /* Measure duration of the RSSI vector acquisition */
      PHY_Unlock();    // Function will not return if it fails
      CAPTURE_TIME(3); /* Measure duration of the PHY_Unlock */
      qsort( workBuf, nSamples, sizeof(uint8_t), cmpfunc); // Sort RSSI array
      CAPTURE_TIME(4); /* Measure duration of the quick sort function */
      // Compute average and stddev
      computeAvgAndStddev( workBuf, nSamples, &average, &stddev );
      CAPTURE_TIME(5); /* Measure duration of the statistics */
      // Update stats
      min = workBuf[0];
      max = workBuf[nSamples-1];
      noiseResults[j].min = min;
      noiseResults[j].med = workBuf[(nSamples-1)/2];
      noiseResults[j].max = max;
      noiseResults[j].avg = scaleAverage( average, min, max );
      noiseResults[j].stddev = scaleStddev( stddev );
      noiseResults[j].P90  = workBuf[(uint32_t)((nSamples-1)*0.9)];
      noiseResults[j].P95  = workBuf[(uint32_t)((nSamples-1)*0.95)];
      noiseResults[j].P99  = workBuf[(uint32_t)((nSamples-1)*0.99)];
      noiseResults[j].P995 = workBuf[(uint32_t)((nSamples-1)*0.995)];
      noiseResults[j].P999 = workBuf[(uint32_t)((nSamples-1)*0.999)];

#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )  // just so we produce the same .hex file
      if ( delayDuringTest )
      {
         if ( ( i != start ) && ( i != end ) && ( j % channelsBeforeDelay == 0) ) /* Have we just finished DELAY_CHANS channels? */
         {
            uint16_t vBefore = (uint16_t)( ADC_Get_SC_Voltage() * 1000);
            OS_TASK_Sleep( (uint32_t)( DELAY_SEC * 1000.0 ) ); /* Every DELAY_CHANS channels, give the super-cap time to charge up (boost disabled) */
            uint16_t vAfter  = (uint16_t)( ADC_Get_SC_Voltage() * 1000);
            DBG_printf(" Capacitor charged up from %u mV to %u mV (%u mV)", vBefore, vAfter, vAfter-vBefore);
         }
      }
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
      CAPTURE_TIME(6); /* Measure duration of the tabulation */
      OS_TASK_Sleep( PAUSE_MSEC ); // Be nice to other tasks
      CAPTURE_TIME(7); /* Measure duration of the PAUSE_MSEC */
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
      if ( ( j == (end/2) )&& ( 4 == testModeHMC ) )
      {
         ID_LoadLPTables_Helper();
      }
   #if ( TM_INSTRUMENT_NOISEBAND_TIMING == 1 )
      for ( uint32_t k = 0; k < CAPTURE_TIMES-1; k++ )
      {
         deltas[k] = OS_TICK_Get_Diff_InMicroseconds( &times[k], &times[k+1] );
         totals[k] += deltas[k];
      }
   #endif // ( TM_INSTRUMENT_NOISEBAND_TIMING == 1 )
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
   }
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )

   /* Put the debug port back into its default state after test has completed */
   R_BSP_PinCfg (BSP_IO_PORT_03_PIN_00, ((uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_DEBUG));
   R_BSP_PinCfg (BSP_IO_PORT_01_PIN_08, ((uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_DEBUG));
   R_BSP_PinCfg (BSP_IO_PORT_01_PIN_09, ((uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_DEBUG));
   R_BSP_PinCfg (BSP_IO_PORT_01_PIN_10, ((uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_DEBUG));
   R_BSP_PinCfg (BSP_IO_PORT_02_PIN_01, ((uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_DEBUG));
   OS_TICK_Get_CurrentElapsedTicks(&time2);
   TimeDiff = (uint32_t)OS_TICK_Get_Diff_InMicroseconds ( &time1, &time2 );
   DBG_printf( "Noiseband took %d seconds, Lowest super-cap voltage = %u mV, Final super-cap voltage  = %u mV",
                TimeDiff/1000000, (uint32_t)(lowestCapVoltage*1000.0), (uint32_t)(ADC_Get_SC_Voltage()*1000.0) );
   if ( updateDisplayDuringNoiseband )
   {
      HMC_APP_SetScanDelay( HMC_APP_TASK_DELAY_MS ); /* Restore normal timing of HMC_app Applet scanning process  */
      HMC_DISP_ClearContinuousUpdate();              /* Stop updating the LCD display of the meter continuously   */
      (void)OS_TASK_Set_Priority( pTskName_Idle,  oldTaskPriorityIDL ); /* Put the task priorities back to normal */
      (void)OS_TASK_Set_Priority( pTskName_Phy,   oldTaskPriorityPHY );
      (void)OS_TASK_Set_Priority( pTskName_Hmc,   oldTaskPriorityHMC );
      (void)OS_TASK_Set_Priority( pTskName_Print, oldTaskPriorityPRN );
      (void)OS_TASK_Set_Priority( pTskName_Dbg,   oldTaskPriorityDBG );
   }
   #if ( TM_INSTRUMENT_NOISEBAND_TIMING == 1 )
   DBG_printf( "Noiseband TOTALS:    Lock %10llu RSSI %10llu Unlock %10llu Qsort %10llu Stats %10llu Tabulate %10llu PAUSE %10llu",
                   totals[0], totals[1], totals[2], totals[3], totals[4], totals[5], totals[6] );
   for ( uint32_t k = 0; k < CAPTURE_TIMES-1; k++ ) { totals[k] = ( totals[k] + nbChannels/2 ) / nbChannels; }
   DBG_printf( "Noiseband AVERAGES:  Lock %10llu RSSI %10llu Unlock %10llu Qsort %10llu Stats %10llu Tabulate %10llu PAUSE %10llu",
                   totals[0], totals[1], totals[2], totals[3], totals[4], totals[5], totals[6] );
   #endif // ( TM_INSTRUMENT_NOISEBAND_TIMING == 1 )
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )

   // Get the RX channels
   GetConf = PHY_GetRequest( ePhyAttr_RxChannels );
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      // Restart the radio if needed
      if (GetConf.val.RxChannels[radioNum] != PHY_INVALID_CHANNEL) {
         (void) Radio.StartRx(radioNum, GetConf.val.RxChannels[radioNum]);
      }
   }

   DBG_logPrintf( 'R', "noiseband end" );

   return ( 0 );
}//lint !e429 Custodial pointer has not been freed or returned

#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
/******************************************************************************

   Function Name: DBG_CommandLine_NoiseBandClkOn

   Purpose: This function displays/compute the noise for a range of channels with 1MHz clock on

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_NoiseBandClkOn ( uint32_t argc, char *argv[] )
{
   if ( argc == 1 )
   {
      DBG_printf( "NoiseBandClkOn GPIO0 GPIO1 GPIO2 GPIO3 drv_strength:\r\n"
                  "               GPIO0-GPIO3: 0=No change, 1=Tri-State, 2=Drive0, 3=Drive1, 4=Input, 7=Divided Clock\r\n"
                  "                            To enable the 1M pull-up resistor, add 64 to any value above\r\n"
                  "               drv_strength: 0=HIGH, 1=MED_HIGH, 2=MED_LOW, 3=LOW (applies to all GPIOs)" );
   }
   else
   {
      ArgStream_s sTemp
             = {SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK,            /* GPIO0 */
                SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING,          /* GPIO1 */
                SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING,          /* GPIO2 */
                SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING,          /* GPIO3 */
                SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5}; /* Low Drive Strength */
      if ( argc > 1 ) { sTemp.GPIO0 = (uint8_t) atoi( argv[1] ); }
      if ( argc > 2 ) { sTemp.GPIO1 = (uint8_t) atoi( argv[2] ); }
      if ( argc > 3 ) { sTemp.GPIO2 = (uint8_t) atoi( argv[3] ); }
      if ( argc > 4 ) { sTemp.GPIO3 = (uint8_t) atoi( argv[4] ); }
      if ( argc > 5 ) { sTemp.gen_config = (uint8_t) (atoi( argv[5] ) << 5); }
      if ( argc > 6 )
      {
         DBG_printf( "Too many parameters");
      }
      else
      {
         if ( ( ( sTemp.GPIO0 & 0x3F ) <= SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_INPUT   ) ||
              ( ( sTemp.GPIO0 & 0x3F ) == SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK )    )
         {
            if ( ( ( ( sTemp.GPIO1 & 0x3F ) <= SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_INPUT   ) ||
                   ( ( sTemp.GPIO1 & 0x3F ) == SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK ) ) &&
                 ( ( ( sTemp.GPIO2 & 0x3F ) <= SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_INPUT   ) ||
                   ( ( sTemp.GPIO2 & 0x3F ) == SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK ) ) &&
                 ( ( ( sTemp.GPIO3 & 0x3F ) <= SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_INPUT   ) ||
                   ( ( sTemp.GPIO3 & 0x3F ) == SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK ) )    )
            {
               if ( ( sTemp.gen_config >> 5 ) <= SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW )
               {
                  memcpy( (void *)&sArgStream, (void *)&sTemp, sizeof(sArgStream) );
                  DBG_printf( "Radio pin config: GPIO0=%02x GPIO1=%02x GPIO2=%02x GPIO3=%02x DRV=%02x\r\n"
                              "Use the original noiseband command to run the test",
                              sArgStream.GPIO0, sArgStream.GPIO1, sArgStream.GPIO2, sArgStream.GPIO3, sArgStream.gen_config );
               } else {
                  DBG_printf( "Drive strength must be 0 - %u", SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW);
               }
            } else {
               DBG_printf( "GPIO1-3 must be 0 - %u", SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_INPUT);
            }
         } else {
            DBG_printf( "GPIO0 must be either %u or %u", SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK,
                                                         SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK + 0x40 );
         }
      }
   }

   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_NoiseBandClkOff

   Purpose: This function displays/compute the noise for a range of channels with 1MHz clock off

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_NoiseBandClkOff ( uint32_t argc, char *argv[] )
{
   sArgStream.GPIO0 = GPIO_PIN_TRISTATE;
   sArgStream.GPIO1 = GPIO_PIN_TRISTATE;
   sArgStream.GPIO2 = GPIO_PIN_TRISTATE;
   sArgStream.GPIO3 = GPIO_PIN_TRISTATE;
   sArgStream.gen_config = SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5;
   DBG_printf( "Radio parameters: GPIO0=%02x GPIO1=%02x GPIO2=%02x GPIO3=%02x DRV=%02x;\r\n"
               "Use the original noiseband command to run the test",
                              sArgStream.GPIO0,
                              sArgStream.GPIO1,
                              sArgStream.GPIO2,
                              sArgStream.GPIO3,
                              sArgStream.gen_config );

   return ( 0 );
}
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )

#if ( TEST_SYNC_ERROR == 1 )
/******************************************************************************

   Function Name: DBG_CommandLine_SyncError

   Purpose: This function set the number of allowable SYNC bit error

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_SyncError ( uint32_t argc, char *argv[] )
{
   uint8_t err;

   // One parameter
   if ( argc > 2 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   } else {
      if ( argc == 1 )
      {
         PHY_GetConf_t GetConf;
         GetConf = PHY_GetRequest( ePhyAttr_SyncError );
         if (GetConf.eStatus == ePHY_GET_SUCCESS) {
            DBG_printf( "SYNC error is %u", ( uint8_t )GetConf.val.SyncError );
         }
      }
      else
      {
         err = ( uint8_t )atoi( argv[1] );

         // Check for valid range
         if ( err > 7 )
         {
            DBG_logPrintf( 'R', "ERROR - Invalid range. Must be between 0-7" );
            return ( 0 );
         }
         PHY_SetConf_t SetConf;
         SetConf = PHY_SetRequest( ePhyAttr_SyncError, &err);
         if (SetConf.eStatus == ePHY_SET_SUCCESS) {
            DBG_printf( "SYNC error is set to %u", err );
         } else {
            DBG_printf( "Failed to set SYNC error" );
         }
      }
   }

   return ( 0 );
}
#endif

#if ( NOISE_HIST_ENABLED == 1 )
static void NoiseHistHelp ( uint8_t command )
{
   if ( ( command == NOISE_BURST_CMD ) || ( command == BURST_HIST_CMD ) ) {
      DBG_printf( "NoiseBurst/BurstHist computes/displays a record of noise bursts:" );
      DBG_printf( "usage: noiseburst radio waittime sampling samples start end step fraction noiseGap hysteresis" );
      DBG_printf( "       NoiseBurst lists each burst above threshold; BurstHist computes a histogram by burst length" );
   } else {
      DBG_printf( "NoiseHist computes/displays a histogram of noise RSSI values:" );
      DBG_printf( "usage: noisehist radio waittime sampling samples start end step fraction thresh_dBm" );
   }
   DBG_printf( "       radio is the radio to use for RX (0-8)" );
   DBG_printf( "       waittime is the time in seconds that the test will be postponed" );
   DBG_printf( "       sampling is the delay time in microseconds between RSSI reads" );
   DBG_printf( "       samples is the number of individual RSSI measurements to take" );
   DBG_printf( "       start is the first (or only) channel number 0-3200 | DL | UL" );
   DBG_printf( "       end is the last channel number 0-3200; defaults to start" );
   DBG_printf( "       step is the increment between channels; defaults to 1" );
   if ( ( command == NOISE_BURST_CMD ) || ( command == BURST_HIST_CMD ) ) {
      DBG_printf( "       fraction is the cumulative probability (e.g., 0.9995) above which it is a burst" );
      DBG_printf( "          if fraction is negative, it is the threshold in dBm above which it is a burst" );
      DBG_printf( "       noiseGap is how many samples can be below thresh_dBm to continue a burst");
      DBG_printf( "       hysteresis is how many dB (0-5) below thresh_dBm RSSI must fall to end a noise burst");
   } else {
      DBG_printf( "       fraction is the cumulative probability (e.g., 0.9995) to calculate the RSSI in dBm" );
      DBG_printf( "       filter_dBm filters output to only channels that have counts above this" );
   }
}
static float getFraction ( char *pFloatString )
{
   char *pString = pFloatString;
   float fraction = 0.9999999f;
   if ( ( *pString == '0' ) && ( *(pString + 1 ) =='.' ) ) pString += 2;
   if (   *pString == '.' )                               pString += 1;
   uint32_t temp = ( uint32_t )atoi( pString );
   if ( temp < 10 )           { fraction = (float)temp / 10.0f;      }
   else if ( temp < 100 )     { fraction = (float)temp / 100.0f;     }
   else if ( temp < 1000 )    { fraction = (float)temp / 1000.0f;    }
   else if ( temp < 10000 )   { fraction = (float)temp / 10000.0f;   }
   else if ( temp < 100000 )  { fraction = (float)temp / 100000.0f;  }
   else if ( temp < 1000000 ) { fraction = (float)temp / 1000000.0f; }

   return ( fraction );
}

static void DBG_NoiseMeasurements ( uint8_t command, uint32_t argc, char *argv[] )
{
   _task_id previousDebugFilter;
   NH_parameters nh;
   nh.radioNum = 0;  nh.waitTime = 30; nh.samplingDelay = 1000; nh.samples = 50000;
   nh.chanFirst = 0; nh.chanLast = 3200; nh.chanStep = 2; nh.fraction = 0.99f;
   nh.filter_dBm = -134; nh.filter_bin = 0; nh.threshRaw = -120; nh.noiseGap = 0;
   nh.hysteresis = 0; nh.command = 0;
   // No parameters
   if ( ( ( argc == 1 ) && ( !NOISEHIST_IsDataAvailable() ) ) || ( ( argc == 2 ) && ( strcasecmp( "?", argv[1] ) == 0 ) ) )
   {
      NoiseHistHelp ( command );
   }
   else if ( argc == 1 )
   {  // Print data if available
      NOISEHIST_PrintResults ( command );
   }
   else if ( ( ( argc > 10 ) &&   ( command == NOISE_HIST_CMD  ) ) ||        \
             ( ( argc > 11 ) && ( ( command == NOISE_BURST_CMD ) || ( command == BURST_HIST_CMD ) ) ) )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }
   else
   {
      uint32_t paramNdx = 1;
      if ( argc > paramNdx ) nh.radioNum = ( uint8_t  )atoi( argv[paramNdx++] );
      if ( argc > paramNdx ) nh.waitTime = ( uint16_t )atoi( argv[paramNdx++] );
      if ( argc > paramNdx ) nh.samplingDelay = ( uint16_t )atoi( argv[paramNdx++] );
      if ( argc > paramNdx ) nh.samples = ( uint32_t )atoi( argv[paramNdx++] );
      if ( argc > paramNdx )
      {
         if (        strcasecmp( "dl", argv[paramNdx] ) == 0 ) {
            nh.chanFirst = NH_ALL_DOWNLINK; nh.chanLast = 0; nh.chanStep = 1; paramNdx++;
         } else if ( strcasecmp( "ul", argv[paramNdx] ) == 0 ) {
            nh.chanFirst = NH_ALL_UPLINK;   nh.chanLast = 0; nh.chanStep = 1; paramNdx++;
            if ( argc > paramNdx ) nh.chanStep = ( int16_t )atoi( argv[paramNdx++] );
         } else {
            nh.chanFirst = ( int16_t )atoi( argv[paramNdx++] );
            nh.chanLast = nh.chanFirst;
            if ( argc > paramNdx ) nh.chanLast = ( int16_t )atoi( argv[paramNdx++] );
            if ( argc > paramNdx ) nh.chanStep = ( int16_t )atoi( argv[paramNdx++] );
         }
         if ( argc > paramNdx ) nh.fraction = getFraction ( argv[paramNdx++] );
         if ( command == NOISE_HIST_CMD ) {
            if ( argc > paramNdx ) nh.filter_dBm = (  int16_t )atoi( argv[paramNdx++] );
         } else {
            if ( argc > paramNdx ) nh.noiseGap   = ( uint16_t )atoi( argv[paramNdx++] );
         }
         if ( argc > paramNdx ) nh.hysteresis = ( uint16_t )atoi( argv[paramNdx++] ) * 2; // convert raw steps to dBm steps
      }
      if ( command == NOISE_HIST_CMD )
      {
         if ( nh.filter_dBm < -134 ) nh.filter_dBm = -134;
         if ( nh.filter_dBm >   -7 ) nh.filter_dBm =   -7;
         nh.filter_bin = (uint32_t) ( RSSI_DBM_TO_RAW( nh.filter_dBm ) / 2 );
         nh.threshRaw = -2;     // this value tells RADIO to not perform noise burst recording
      }
      else
      {
         nh.filter_bin = (uint32_t) ( RSSI_DBM_TO_RAW( (int16_t)( -134 ) ) );
         if ( nh.fraction < 0.0f )
         {
            nh.threshRaw = (uint8_t ) ( RSSI_DBM_TO_RAW( nh.fraction ) );
            nh.fraction = 0.0f;
         } else {
            nh.threshRaw = -1;  // this value tells RADIO to calcuate threshold based on fraction
         }
      }
      if ( nh.hysteresis > 10   ) nh.hysteresis =    0; // more than 5dB of hystersis doen't make sense
      if ( nh.noiseGap   > 10   ) nh.noiseGap   =    0; // more than 10 samples doesn't make sense
      if ( nh.fraction >  100.0f ) nh.fraction =  100.0f;
      if ( nh.chanStep == 0 ) { nh.chanStep = 1; }
      if ( nh.radioNum >= (uint8_t)MAX_RADIO )       { DBG_logPrintf( 'R', "ERROR - invalid radio" );            return; }
      if ( nh.waitTime > 600 )                       { DBG_logPrintf( 'R', "ERROR - waittime too long" );        return; }
      if ( nh.chanFirst >= PHY_INVALID_CHANNEL )     { DBG_logPrintf( 'R', "ERROR - start channel is invalid" ); return; }
      if ( nh.chanLast >= PHY_INVALID_CHANNEL )      { DBG_logPrintf( 'R', "ERROR - end channel is invalid" );   return; }
      if ( ( nh.chanLast < nh.chanFirst ) && ( nh.chanStep > 0 ) ) { DBG_logPrintf( 'R', "ERROR - start > end channel" );      return; }
      if ( ( nh.chanLast > nh.chanFirst ) && ( nh.chanStep < 0 ) ) { DBG_logPrintf( 'R', "ERROR - start < end and step < 0" ); return; }
      nh.command = command;

#define postDelay ( (uint16_t)( 5 ) )
      NOISEHIST_PrintDuration ( nh, postDelay );

      OS_TASK_Sleep( nh.waitTime * ONE_SEC );
      previousDebugFilter = DBG_GetTaskFilter();
      (void)DBG_SetTaskFilter ( OS_TASK_GetID_fromName( "DBG" ) );

      NOISEHIST_CollectData ( nh );

      OS_TASK_Sleep( postDelay * ONE_SEC );  // wait 5 seconds so the last "CPU..." message is already discarded
      (void)DBG_SetTaskFilter( previousDebugFilter );
   }
}
#endif
/******************************************************************************

   Function Names: DBG_CommandLine_NoiseHistogram
                   DBG_CommandLine_NoiseBurst
                   DBG_CommandLine_BurstHist

   Purpose: This function computes and displays a histogram of noise RSSI values

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
#if ( NOISE_HIST_ENABLED == 1 )
uint32_t DBG_CommandLine_NoiseHistogram ( uint32_t argc, char *argv[] )
{
   DBG_NoiseMeasurements ( NOISE_HIST_CMD, argc, argv );
   return ( 0 );
}
uint32_t DBG_CommandLine_NoiseBurst ( uint32_t argc, char *argv[] )
{
   DBG_NoiseMeasurements ( NOISE_BURST_CMD, argc, argv );
   return ( 0 );
}
uint32_t DBG_CommandLine_BurstHistogram ( uint32_t argc, char *argv[] )
{
   DBG_NoiseMeasurements ( BURST_HIST_CMD, argc, argv );
   return ( 0 );
}
#endif

#if ( MCU_SELECTED == NXP_K24 )
#if 0 // RA6E1 Bob: This command was removed from original K24 code
/***********************************************************************************************************************
   Function Name: DBG_CommandLine_FlashSecurity

   Purpose: Lock/Unlock Flash and JTAG Security.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
uint32_t DBG_CommandLine_FlashSecurity( uint32_t argc, char *argv[] )
{
   PartitionData_t const *pImagePTbl;     // Image partition for PGM memory
   returnStatus_t retVal = eFAILURE;
   dSize destAddr = 0x408u;               // Destination address of the data to move
   uint8_t bLock = false;
   uint8_t bValid = false;
   //                        FPROT3 FPROT2 FPROT1 FPROT0 FSEC
   uint8_t uLockMask[5] =   {0x00u, 0x80u, 0xFFu, 0xFFu, 0xFFu};
   uint8_t uUnlockMask[5] = {0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFEu};

   if ( 1 == argc )
   {
      DBG_printf( "%s", argv[0] );
#if (DCU == 1 )
      DBG_logPrintf( 'I', "FPROT3: %02x", FTFE_FPROT3 );
      DBG_logPrintf( 'I', "FPROT2: %02x", FTFE_FPROT2 );
      DBG_logPrintf( 'I', "FPROT1: %02x", FTFE_FPROT1 );
      DBG_logPrintf( 'I', "FPROT0: %02x", FTFE_FPROT0 );
      DBG_logPrintf( 'I', "  FSEC: %02x", FTFE_FSEC );
#elif ( MQX_CPU == PSP_CPU_MK22FN512 )

      DBG_logPrintf( 'I', "FPROT3: %02x", FTFA_FPROT3 );
      DBG_logPrintf( 'I', "FPROT2: %02x", FTFA_FPROT2 );
      DBG_logPrintf( 'I', "FPROT1: %02x", FTFA_FPROT1 );
      DBG_logPrintf( 'I', "FPROT0: %02x", FTFA_FPROT0 );
      DBG_logPrintf( 'I', "  FSEC: %02x", FTFA_FSEC );
#else
      DBG_logPrintf( 'I', "FPROT3: %02x", FTFE_FPROT3 );
      DBG_logPrintf( 'I', "FPROT2: %02x", FTFE_FPROT2 );
      DBG_logPrintf( 'I', "FPROT1: %02x", FTFE_FPROT1 );
      DBG_logPrintf( 'I', "FPROT0: %02x", FTFE_FPROT0 );
      DBG_logPrintf( 'I', "  FSEC: %02x", FTFE_FSEC );
#endif
   }
   else if ( 2 == argc )
   {
      if ( ( 0 == strcasecmp( argv[1], "true" ) ) || ( 0 == strcmp( argv[1], "1" ) ) )
      {
         bLock = true;
         bValid = true;
      }
      else if ( ( 0 == strcasecmp( argv[1], "false" ) ) || ( 0 == strcmp( argv[1], "0" ) ) )
      {
         bValid = true;
      }
      else
      {
         DBG_logPrintf( 'R', "Invalid argument" );
      }

      if ( bValid )
      {
         DBG_printf( "%s %s", argv[0], argv[1] );

         // Copy the running program image to the alternate partition.
         retVal = DFWA_CopyPgmToPgmImagePartition( true, eFWT_APP );

         if ( eSUCCESS == retVal )
         {
            // Open the alternate program partition for writing.
            if ( eSUCCESS == PAR_partitionFptr.parOpen( &pImagePTbl, ePART_DFW_PGM_IMAGE, 0L ) )
            {
               DBG_logPrintf( 'I', "Flash Partition Opened Successfully" );

               // Write the appropriate pattern to lock or unlock the flash and JTAG.
               if ( true == bLock )
               {
                  retVal = PAR_partitionFptr.parWrite( destAddr, uLockMask, ( lCnt ) sizeof( uLockMask ), pImagePTbl );
               }
               else
               {
                  retVal = PAR_partitionFptr.parWrite( destAddr, uUnlockMask, ( lCnt ) sizeof( uUnlockMask ), pImagePTbl );
               }

               if ( eSUCCESS == retVal )
               {
                  // Swap the program flash partitions and reset.
                  (void)DFWA_WaitForSysIdle( 3000 );   // No need to unlock when done since resetting
                  DBG_logPrintf( 'I', "Swapping Flash" );
                  ( void )DFWA_SetSwapState( eFS_COMPLETE );
                  DBG_logPrintf( 'I', "Resetting Processor" );

                  // Keep all other tasks from running
                  // Increase the priority of the power and idle tasks
                  ( void )OS_TASK_Set_Priority( pTskName_Pwr, 10 );
                  ( void )OS_TASK_Set_Priority( pTskName_MfgUartRecv, 11 );
                  ( void )OS_TASK_Set_Priority( pTskName_Idle, 12 );
                  // Does not return from here
                  PWR_SafeReset();
               }
               else
               {
                  DBG_logPrintf( 'E', "Flash Partition Write FAILED" );
               }
            }
            else
            {
               DBG_logPrintf( 'E', "Flash Partition Open FAILED" );
            }

         }
         else
         {
            DBG_logPrintf( 'E', "Flash Partition Copy FAILED" );
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "Invalid number of parameters" );
   }

   return( 0 );
}
#endif // RA6E1 Bob: This command was removed from original K24 code
#elif ( MCU_SELECTED == RA6E1 )
/***********************************************************************************************************************
   Function Name: DBG_CommandLine_FlashSecurity

   Purpose: For RA6E1, the firmware is not able to alter the Device Lifecycle state.  This must be done by an external device
            such as the E2 Lite programmer.  Therefore, this function only interrogates the MCU's current Device Lifecycle
            state and displays it.

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: void
***********************************************************************************************************************/
uint32_t DBG_CommandLine_FlashSecurity( uint32_t argc, char *argv[] )
{
   /* Names of the Device Lifecycle states.  Our current understanding is that we will only occupy SSD (Secure Software
      Development) and DPL (Deployed).  In SSD, an IDE can Can program/erase/read all code/data flash.  In DPL, an IDE
      has no access to code/data flash.  The MCU must pass through the NSECSD state on the way from SSD to DPL but the
      firmware should probably never see the device in the NSECSD state.  TODO: RA6E1 Bob: Update this as more is known. */
   const char * dlmText[] = { "RSVD", "CM", "SSD", "NSECSD", "DPL", "LCK_DBG", "LCK_BOOT", "RMA_REQ", "RMA_ACK" };
   void * pText;
   uint32_t dlmmon = R_PSCU->DLMMON; /* Retrieve the Device Lifecycle Monitor register value */

   if ( argc > 1 )
   {
      DBG_printf( "For the RA6E1, flash security cannot be set by commands, it can only be examined!" );
   }
   else
   {
      if ( ( dlmmon & R_PSCU_DLMMON_DLMMON_Msk ) < ARRAY_IDX_CNT(dlmText) )
      {
         pText = (void *)&dlmText[ ( dlmmon & R_PSCU_DLMMON_DLMMON_Msk ) ][ 0 ];
      }
      else
      {
         pText = (void *)&dlmText[ 0 ][ 0 ];
      }
      DBG_printf( "Current Device Lifecycle Management State Monitor Register = %08x (%s)", dlmmon, pText );
   }
   return ( 0 );
}
#endif // MCU_SELECTED
#if ( FAKE_TRAFFIC == 1 )
/***********************************************************************************************************************
   Function Name: DBG_CommandLine_ipBhaulGenCount

   Purpose: Get/Set the backhaul generation count

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
uint32_t DBG_CommandLine_ipBhaulGenCount( uint32_t argc, char *argv[] )
{
   NWK_GetConf_t     GetConf;
   char              *endptr;
   uint8_t           val;

   if ( argc == 2 )           /* User wants to set the RF dutycycle */
   {
      val = ( uint8_t )strtoul( argv[1], &endptr, 10 );
      (void)NWK_SetRequest( eNwkAttr_ipBhaulGenCount, &val);
   }

   /* Always print the actual value   */
   GetConf = NWK_GetRequest( eNwkAttr_ipBhaulGenCount );
   DBG_printf("%s %u\n", argv[0], GetConf.val.ipBhaulGenCount );

   return( 0 );
}

/***********************************************************************************************************************
   Function Name: DBG_CommandLine_ipRfDutyCycle

   Purpose: Get/Set the IP RF dutycycle

   Arguments:
      argc - Number of Arguments passed to this function
      argv - pointer to the list of arguments passed to this function

   Returns: none
***********************************************************************************************************************/
uint32_t DBG_CommandLine_ipRfDutyCycle( uint32_t argc, char *argv[] )
{
   NWK_GetConf_t     GetConf;
   char              *endptr;
   uint8_t           val;

   if ( argc == 2 )           /* User wants to set the RF dutycycle */
   {
      val = ( uint8_t )strtoul( argv[1], &endptr, 10 );
      (void)NWK_SetRequest( eNwkAttr_ipRfDutyCycle, &val);
   }

   /* Always print the actual value   */
   GetConf = NWK_GetRequest( eNwkAttr_ipRfDutyCycle );
   DBG_printf("%s %u\n", argv[0], GetConf.val.ipRfDutyCycle );

   return( 0 );
}
#endif // ( FAKE_TRAFFIC == 1 )

/******************************************************************************

   Function Name: DBG_CommandLine_NetworkId

   Purpose: This function sets/gets the network ID

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_NetworkId ( uint32_t argc, char *argv[] )
{
   uint8_t networkId;

   if ( argc > 3 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   else if ( argc == 2 )
   {
      networkId = ( uint8_t )atoi( argv[1] );
      MAC_NetworkId_Set( networkId );
   }

   else if ( argc == 1 )
   {
      DBG_printf( "Network ID is %d", MAC_NetworkId_Get() );
   }
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_RxTimeout

   Purpose: This function sets/gets the Reassembly timeout

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_RxTimeout ( uint32_t argc, char *argv[] )
{
   uint8_t reassemblyTimeout;


   if ( argc > 3 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   else if ( argc == 2 )
   {
      MAC_SetConf_t SetConf;
      reassemblyTimeout = ( uint8_t )atoi( argv[1] );
      SetConf = MAC_SetRequest( eMacAttr_ReassemblyTimeout, &reassemblyTimeout);
      if (SetConf.eStatus != eMAC_SET_SUCCESS) {
         DBG_printf( "MAC set API returned an error" );
      }
   }

   else if ( argc == 1 )
   {
      MAC_GetConf_t GetConf;
      GetConf = MAC_GetRequest( eMacAttr_ReassemblyTimeout );
      if (GetConf.eStatus == eMAC_GET_SUCCESS) {
         DBG_printf( "Reassembly timeout is %d", GetConf.val.ReassemblyTimeout );
      }
   }
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_RelHighCnt

   Purpose: This function sets/gets the ReliabilityHighCount MAC attribute

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_RelHighCnt ( uint32_t argc, char *argv[] )
{
   uint8_t reliabilityHighCount;
   uint32_t tempInputCleaning;

   if ( argc > 3 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   else if ( argc == 2 )
   {
      tempInputCleaning = ( uint32_t )atoi( argv[1] );
      if (tempInputCleaning > 255)
      {
         DBG_printf( "Value too large\n" );
      }
      else
      {
         MAC_SetConf_t SetConf;
         reliabilityHighCount = ( uint8_t )tempInputCleaning;
         SetConf = MAC_SetRequest( eMacAttr_ReliabilityHighCount, &reliabilityHighCount);
         if (SetConf.eStatus != eMAC_SET_SUCCESS) {
            DBG_printf( "MAC set API returned an error" );
         }
      }
   }

   else if ( argc == 1 )
   {
      MAC_GetConf_t GetConf;
      GetConf = MAC_GetRequest( eMacAttr_ReliabilityHighCount );
      if (GetConf.eStatus == eMAC_GET_SUCCESS) {
         DBG_printf( "High Reliability retry count is %d", GetConf.val.ReliabilityHighCount );
      }
   }
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_RelMedCnt

   Purpose: This function sets/gets the ReliabilityMedCount MAC attribute

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_RelMedCnt ( uint32_t argc, char *argv[] )
{
   uint8_t reliabilityMediumCount;
   uint32_t tempInputCleaning;

   if ( argc > 3 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   else if ( argc == 2 )
   {
      tempInputCleaning = ( uint32_t )atoi( argv[1] );
      if (tempInputCleaning > 255)
      {
         DBG_printf( "Value too large\n" );
      }
      else
      {
         MAC_SetConf_t SetConf;
         reliabilityMediumCount = ( uint8_t )tempInputCleaning;
         SetConf = MAC_SetRequest( eMacAttr_ReliabilityMediumCount, &reliabilityMediumCount);
         if (SetConf.eStatus != eMAC_SET_SUCCESS) {
            DBG_printf( "MAC set API returned an error" );
         }
      }
   }

   else if ( argc == 1 )
   {
      MAC_GetConf_t GetConf;
      GetConf = MAC_GetRequest( eMacAttr_ReliabilityMediumCount );
      if (GetConf.eStatus == eMAC_GET_SUCCESS) {
         DBG_printf( "Medium Reliability retry count is %d", GetConf.val.ReliabilityMediumCount );
      }
   }
   return ( 0 );
}

/******************************************************************************

   Function Name: DBG_CommandLine_RelLowCnt

   Purpose: This function sets/gets the ReliabilityLowCount MAC attribute

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_RelLowCnt ( uint32_t argc, char *argv[] )
{
   uint8_t reliabilityLowCount;
   uint32_t tempInputCleaning;

   if ( argc > 3 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   else if ( argc == 2 )
   {
      tempInputCleaning = ( uint32_t )atoi( argv[1] );
      if (tempInputCleaning > 255)
      {
         DBG_printf( "Value too large\n" );
      }
      else
      {
         MAC_SetConf_t SetConf;
         reliabilityLowCount = ( uint8_t )tempInputCleaning;
         SetConf = MAC_SetRequest( eMacAttr_ReliabilityLowCount, &reliabilityLowCount);
         if (SetConf.eStatus != eMAC_SET_SUCCESS) {
            DBG_printf( "MAC set API returned an error" );
         }
      }
   }

   else if ( argc == 1 )
   {
      MAC_GetConf_t GetConf;
      GetConf = MAC_GetRequest( eMacAttr_ReliabilityLowCount );
      if (GetConf.eStatus == eMAC_GET_SUCCESS) {
         DBG_printf( "Low Reliability retry count is %d", GetConf.val.ReliabilityLowCount );
      }
   }
   return ( 0 );
}

#if ( DCU == 1 )
/******************************************************************************

   Function Name: DBG_CommandLine_TxPacketDelay

   Purpose: This function sets/gets tx packet delay

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/

uint32_t DBG_CommandLine_TxPacketDelay ( uint32_t argc, char *argv[] )
{
   uint16_t packetDelay;

   uint32_t tempInputCleaning;

   if ( argc > 3 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   else if ( argc == 2 )
   {
      tempInputCleaning = (uint32_t)atoi( argv[1] );

      if (tempInputCleaning > 10000)
      {
         DBG_printf( "Value too large\n" );
      }
      else
      {
         MAC_SetConf_t SetConf;
         packetDelay = ( uint16_t )tempInputCleaning;
         SetConf = MAC_SetRequest( eMacAttr_TxPacketDelay, &packetDelay);
         if (SetConf.eStatus != eMAC_SET_SUCCESS) {
            DBG_printf( "MAC set API returned an error" );
         }
      }
   }

   else if ( argc == 1 )
   {
      MAC_GetConf_t GetConf;
      GetConf = MAC_GetRequest( eMacAttr_TxPacketDelay );
      if (GetConf.eStatus == eMAC_GET_SUCCESS) {
         DBG_printf( "PacketDelay is %d", GetConf.val.TxPacketDelay );
      }
   }
   return ( 0 );
}
#endif
#if 0
/******************************************************************************

   Function Name: DBG_CommandLine_PacketTimeout

   Purpose: This function sets/gets packet ID duplicate detection timeout

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_PacketTimeout ( uint32_t argc, char *argv[] )
{
   uint16_t packetTimeout;

   if ( argc > 3 )
   {
      DBG_logPrintf( 'R', "ERROR - Too many arguments" );
   }

   else if ( argc == 2 )
   {
      MAC_SetConf_t SetConf;
      packetTimeout = ( uint16_t )( atoi( argv[1] ) );
      SetConf = MAC_SetRequest( eMacAttr_PacketTimeout, &packetTimeout);
      if (SetConf.eStatus != eMAC_SET_SUCCESS) {
         DBG_printf( "MAC set API returned an error" );
      }
   }

   else if ( argc == 1 )
   {
      MAC_GetConf_t GetConf;
      GetConf = MAC_GetRequest( eMacAttr_PacketTimeout );
      if (GetConf.eStatus == eMAC_GET_SUCCESS) {
         DBG_printf( "Reassembly timeout is %d", GetConf.val.PacketTimeout );
      }
   }
   return ( 0 );
}
#endif
/*******************************************************************************

   Function name: DBG_CommandLine_EVLADD

   Purpose: This function will add an event log

   Arguments:  argc - Number of Arguments passed to this function
               argv[1] - priority
               argv[2] - Event Id
               argv[3] - Pairs Count
               argv[4] - Value size

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_EVLADD( uint32_t argc, char *argv[] )
{
   EventData_s event;
   EventKeyValuePair_s ePairs[15];
   EventAction_e rVal;
   int16_t i, j;
   uint8_t e_priority;

   ( void )memset( ePairs, 0, sizeof( ePairs ) );
   ( void )memset( ( void * )&event, 0, sizeof( event ) );

   if ( argc == 5 )
   {
      e_priority = ( uint8_t )strtoul( argv[1], NULL, 0 );
      event.markSent = (bool)false;
      event.eventId = ( uint16_t )strtoul( argv[2], NULL, 0 );
      event.eventKeyValuePairsCount = ( uint8_t )strtoul( argv[3], NULL, 0 );
      event.eventKeyValueSize = ( uint8_t )strtoul( argv[4], NULL, 0 );
      if ( event.eventKeyValuePairsCount > 15 )
      {
         event.eventKeyValuePairsCount = 15;
      }
      if ( event.eventKeyValueSize > 15 )
      {
         event.eventKeyValueSize = 15;
      }
      for ( i = 0; i < event.eventKeyValuePairsCount; i++ )
      {
         ePairs[i].Key[0] = ( uint8_t )( i + 1 );
         ePairs[i].Key[1] = ( uint8_t )( i + 1 );
         for ( j = 0; j < event.eventKeyValueSize; j++ )
         {
            ePairs[i].Value[j] = ( uint8_t )( i + 'A' );
         }
      }
      rVal = EVL_LogEvent( e_priority, &event, ePairs, TIMESTAMP_NOT_PROVIDED, NULL );
      INFO_printf( "EVL_LogEvent returned: " );
      switch ( rVal )
      {
         case ( QUEUED_e ) :
            INFO_printf( "QUEUED" );
            break;
         case ( DROPPED_e ) :
            INFO_printf( "DROPPED" );
            break;
         case ( SENT_e ) :
            INFO_printf( "SENT" );
            break;
         case ( ERROR_e ) :
            INFO_printf( "ERROR" );
            break;
         default:
            INFO_printf( "ILLEGAL VALUE" );
            break;
      }
   }
   else
   {
      INFO_printf( "USAGE: evladd <priority> <event id><num of key pairs> <key value size>" );
   }
   return ( 0 );
} /* end DBG_CommandLine_EVLADD () */

/*******************************************************************************

   Function name: DBG_CommandLine_EVLCMD

   Purpose: This function is called to execute a command as follows:
   (1) - Dump Both Buffers
   (2) - Add events to the buffers <num events>
   (3) - Erase the flash <buffer>
   (4) - Delete all events; maintain current thresholds.
   (5) - Set thresholds realTime Opportunistic
   (6) - Get thresholds

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_EVLCMD( uint32_t argc, char *argv[] )
{
   uint16_t command;
   command = ( uint16_t )atoi( argv[1] );
   EventData_s event;
   EventKeyValuePair_s ePairs[15];
   uint16_t i, j, k;
   uint16_t loop;
   uint16_t rVal = 0;
   uint16_t whichBuffer;
   uint8_t rt;
   uint8_t o;

   ( void )memset( ( void * )&event, 0, sizeof( event ) );
   ( void )memset( ( void * )ePairs, 0, sizeof( ePairs ) );

   if ( argc > 1 )
   {
      switch ( command )
      {
         case ( 1 ) :
         {
            EventLogDumpBuffers();
            break;
         }
         case ( 2 ) :
         {
            loop = ( uint16_t )atoi( argv[2] );
            event.markSent = (bool)false;
            for ( k = 0; k < loop; k++ )
            {
               event.eventId++;
               event.eventKeyValuePairsCount = k % 5;
               event.eventKeyValueSize = ( k + 1 ) % 5;
               for ( i = 0; i < event.eventKeyValuePairsCount; i++ )
               {
                  ePairs[i].Key[0] = ( uint8_t )( i + 1 );
                  ePairs[i].Key[1] = ( uint8_t )( i + 1 );
                  for ( j = 0; j < event.eventKeyValueSize; j++ )
                  {
                     ePairs[i].Value[j] = ( uint8_t )( i + 0x41 );
                  }
               }
               rVal = ( uint16_t )EVL_LogEvent( ( uint8_t )atoi( argv[3] ), &event, ePairs, TIMESTAMP_NOT_PROVIDED, NULL );
               if ( rVal == ( uint16_t )ERROR_e )
               {
                  INFO_printf( "Log Event Failure 0x%x", event.eventId );
               }
               else if ( rVal == ( uint16_t )DROPPED_e )
               {
                  INFO_printf( "Dropped the packet" );
               }
            }
            if ( rVal == 0 )
            {
               INFO_printf( "%d events added to the queue", loop );
            }
            break;
         }
         case ( 3 ) :
         {
            whichBuffer = ( uint16_t )atoi( argv[2] );
            EventLogEraseFlash( ( uint8_t )whichBuffer );
            INFO_printf( "NVRAM ring buffer erased" );
            break;
         }
         case ( 4 ) :
         {
            EventLogDeleteMetaData();
            break;
         }
         case ( 5 ):
         {
            ( void )EVL_SetThresholds( ( uint8_t )atoi( argv[2] ), ( uint8_t )atoi( argv[3] ) );
            break;
         }
         case ( 6 ):
         {
            EVL_GetThresholds( &rt, &o );
            INFO_printf( "Real Time Threshold: %d Opportunistic Threshold: %d", rt, o );
            break;
         }
         default:
         {
            INFO_printf( "Command %d not implemented", command );
         }
      }
   }
   else
   {
      INFO_printf( "USAGE: evlcmd cmd" );
      INFO_printf( "cmd: 1 - dump all, 2 - Add events (num events), 3 - Erase flash (buf num)" );
      INFO_printf( "cmd: 4 - Delete meta data, 5 - set thresholds, 6 - get thresholds" );
   }

   return ( 0 );

} /* end DBG_CommandLine_EVLDUMP () */

/*******************************************************************************

   Function name: DBG_CommandLine_EVLQ

   Purpose: This function will query the logs by date or event id.

   Arguments:  argc - Number of Arguments passed to this function
               argv[1] - query type 0|ALL 1|BY DATE 2|BY EVENT
               argv[2] - start range
               argv[3] - end range
               argv[4] - buffer size
               argv[5] - alarm severity  2|Opportunistic 3|RT (only w/ alarmID query)

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:Alarm Severity must be speficied when querying based on Alarm_id

*******************************************************************************/
uint32_t DBG_CommandLine_EVLQ( uint32_t argc, char *argv[] )
{
   buffer_t       *pBuffer;
   uint8_t        *results = NULL;
   uint32_t       total    = 0;
   uint32_t       size     = 0;
   uint32_t       retVal   = 0;
   EventQuery_s   query;
   uint8_t        alarmId;
   bool           validQry = true;

   if ( argc == 5 || argc == 6 )
   {
      query.qType = ( EventQueryRequest_e )atoi( argv[1] );
      if ( (query.qType == QUERY_BY_DATE_e) && (5 == argc))
      {
         query.start_u.timestamp = strtoul( argv[2], NULL, 0 );
         query.end_u.timestamp = strtoul( argv[3], NULL, 0 );
         size = strtoul( argv[4], NULL, 0 );
      }
      else if ( (query.qType == QUERY_BY_INDEX_e) && (6 == argc))
      {
         query.start_u.eventId = ( uint16_t )strtoul( argv[2], NULL, 0 );
         query.end_u.eventId = ( uint16_t )strtoul( argv[3], NULL, 0 );
         size = strtoul( argv[4], NULL, 0 );
         pBuffer = BM_alloc( ( uint16_t )size );
         query.alarmSeverity = (uint8_t) strtoul(argv[5], NULL, 0);
         if( (MEDIUM_PRIORITY_SEVERITY != query.alarmSeverity) &&
                HIGH_PRIORITY_SEVERITY != query.alarmSeverity )
         {
            INFO_printf( "ERR: invalid alarm priority" );
            validQry = false;
         }
      }

      if( true == validQry )
      {
         pBuffer = BM_alloc( ( uint16_t )size );
         if ( NULL == pBuffer )
         {
           INFO_printf( "ERR: cannot allocate buffer" );
           retVal = ( uint32_t ) - 1;
         }
         else
         {
            results = ( uint8_t* ) pBuffer->data;
            DBG_printf( "\nQUERY BY EVENT" );
            ( void )EVL_QueryBy( query, results, size, &total, &alarmId );
            DBG_printf( "ALARM ID: 0x%x", alarmId );

            INFO_printf( "Total bytes returned: %d", total );
            if ( total > 0 )
            {
               DumpHeadEndData( results, total );
            }
            BM_free( pBuffer );
         }
      }
   }
   else
   {
      INFO_printf( "USAGE: evlq <query type> <start range> <end range> <results buffer size> if(event id): <alrm severity>" );
      INFO_printf( "       query types: 0 - NONE, 1 - date, 2 - event id" );
      INFO_printf( "       severity types (required with event ID) : 2 - opportunistic, 3 - real time" );
   }
   return( retVal );
} /* end DBG_CommandLine_EVLQ () */

/*******************************************************************************

   Function name: DBG_CommandLine_EVLSEND

   Purpose: This function simulates events were successfully sent to the
   head-end.

   Arguments:  argc - Number of Arguments passed to this function
               argv[1] - number of event ids
               argv[n] - event id list (hex)

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_EVLSEND( uint32_t argc, char *argv[] )
{
   uint16_t i;
   uint16_t index;
   EventMarkSent_s markSent;

   if ( argc > 3 )
   {
      markSent.nEvents = ( uint16_t )atoi( argv[1] );
      if ( markSent.nEvents == 0 )
      {
         INFO_printf( "number of events needs to be > 0" );
         return 0;
      }

      index = 2;
      markSent.nEvents = ( uint16_t )min( markSent.nEvents, EVL_MAX_NUMBER_EVENTS_SENT );
      for ( i = 0; i < markSent.nEvents; i++ )
      {
         markSent.eventIds[i].priority = (uint8_t) strtoul( argv[index++], NULL, 0 );
         markSent.eventIds[i].eventId =  (uint16_t)strtoul( argv[index++], NULL, 0 );
         markSent.eventIds[i].offset =             strtoul( argv[index++], NULL, 0 );
         markSent.eventIds[i].size =               strtoul( argv[index++], NULL, 0 );
      }
      EVL_MarkSent( &markSent );
      EventLogDumpBuffers();
   }
   else
   {
      INFO_printf(   "USAGE: evlsend <num events> <pri> <event ids> <event offset> <size> ... "
                     "<pri[n]> <event ids[n]> <event offset[n]. <size[n]>" );
   }
   return ( 0 );
} /* end DBG_CommandLine_EVLSEND () */


/*******************************************************************************

   Function name: DBG_CommandLine_EVLGETLOG

   Purpose: This function will fill a buffer with as many events that fit.

   Arguments:  argc - Number of Arguments passed to this function
               argv[1] - priority 0|NORMAL 1|PRIORITY
               argv[2] - Size of buffer and size of data to get

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DBG_CommandLine_EVLGETLOG( uint32_t argc, char *argv[] )
{
   EventMarkSent_s markSent;
   buffer_t* pBuffer;
   uint32_t uRetVal = 0;
   uint16_t size;
   uint8_t *buffer;
   int32_t  filled;
   uint16_t i = 0;
   uint8_t alarmId;

   markSent.nEvents = 0;

   if ( argc == 2 )
   {
      size = ( uint16_t )strtoul( argv[1], NULL, 0 );

      if ( size == 0 )
      {
         INFO_printf( "Size must be greater than 0" );
         return 0;
      }

      pBuffer = BM_alloc( size );
      if ( pBuffer == NULL )
      {
         ERR_printf( "Failed to allocate memory" );
         uRetVal = ( uint32_t ) - 1;
      }

      else
      {
         buffer = ( uint8_t* ) pBuffer->data;
         filled = EVL_GetLog( size, buffer, &alarmId, &markSent );
         if ( filled >= 0 ) {
            INFO_printf( "ALARM ID: 0x%x", alarmId );
            INFO_printf( "FILLED: %d", filled );
            DumpHeadEndData( buffer, (uint32_t)filled );
            BM_free( pBuffer );

            if ( markSent.nEvents )
            {
               markSent.nEvents = min( markSent.nEvents, ARRAY_IDX_CNT( markSent.eventIds ) );
               INFO_printf( "PASSBACK DATA: " );
               for ( i = 0; i < markSent.nEvents; i++ )
               {
                  INFO_printf( "Priority: %d ev: 0x%x off: %d",
                               markSent.eventIds[i].priority,
                               markSent.eventIds[i].eventId,
                               markSent.eventIds[i].offset );
               }
            }
         } else {
            ERR_printf("EVL_GetLog returned an error");
         }
      }
   }
   else
   {
      INFO_printf( "USAGE: evlgetlog <size>" );
   }

   return( uRetVal );
} /* end DBG_CommandLine_EVLGETLOG () */

#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
/******************************************************************************

   Function Name: DBG_CommandLine_MAC_LinkParameters ( uint32_t argc, char *argv[] )

   Purpose: This function is used to set/get the MAC Link Parameters

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
uint32_t DBG_CommandLine_MAC_LinkParameters ( uint32_t argc, char *argv[] )
{

#if 0 // DG: 05/17/21: Removing repeated code
   struct
   {
      char*            name;
   } field[] =
     {
      {"offset" },
      {"period" },
      {"start" },
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
      {"max_td" },
#endif
   };
#endif // DG: 05/17/21: Removing repeated code
   if ( argc > 1 )
   {
      if ( strcasecmp( "get", argv[1] ) == 0 )
      {
         MAC_LinkParameters_Config_Print();
      }
#if 0 // DG: 05/17/21: Removing repeated code
      else if ( strcasecmp( "set", argv[1] ) == 0 )
      {
         if( argc >= 4 )
         {
            int index = 0;
            do
            {
               if ( strcasecmp( field[index].name, argv[2] ) == 0 )
               {
                  uint32_t value;
                  value = ( uint32_t )atoi( argv[3] );
                  if ( index == 0 )
                  {
                     //eMacAttr_LinkParamMaxOffset
                     ( void )MAC_SetRequest( eMacAttr_LinkParamMaxOffset, &value);
                  }
                  else if ( index == 1 )
                  {
                     //eMacAttr_LinkParamPeriod
                     ( void )MAC_SetRequest( eMacAttr_LinkParamPeriod, &value);
                  }
                  else if ( index == 2 )
                  {
                     //eMacAttr_LinkParamStart
                     ( void )MAC_SetRequest( eMacAttr_LinkParamStart, &value);
                  }
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
                  else if( index == 3 )
                  {
                     //eMacAttr_CmdRespMaxTimeDiversity
                     ( void )MAC_SetRequest( eMacAttr_CmdRespMaxTimeDiversity, &value );
                  }
#endif
                  MAC_LinkParameters_Config_Print();
                  break;
               }
            }while ( index++ < 5 );
            if( index > 5 )
            {
               INFO_printf( "Error: Unavailable" );
            }
         }
         else
         {
            INFO_printf( "Error: Too many Arguments" );
         }
      }
#endif // DG: 05/17/21: Removing repeated code
      else if( strcasecmp( "send", argv[1] ) == 0 )
      {
         ( void )MAC_LinkParametersPush_Request( BROADCAST_MODE, NULL, NULL );
      }
   }
   else
   {
      DBG_logPrintf( 'R', "mac_lp get <cr>" );
#if 0 // DG: 05/17/21: Removing repeated feature
      DBG_logPrintf( 'R', "mac_lp set %s|%s|%s|%s val<cr>",
                     field[0].name, field[1].name, field[2].name, field[3].name);
#endif // DG: 05/17/21: Removing repeated feature
      DBG_logPrintf( 'R', "mac_lp send <cr>" );
   }
   return ( 0 );
}
#endif // end of ( MAC_LINK_PARAMETERS == 1 ) section

#if (USE_DTLS == 1)

uint32_t DBG_CommandLine_Dtls( uint32_t argc, char *argv[] )
{
   bool printUsage = true;
   if ( argc == 2 )
   {
      if ( 0 == strcmp( argv[1], "get" ) )
      {
         DTLS_PrintSessionStateInformation();

         printUsage = false;
      }
      else if ( 0 == strcmp( argv[1], "reset" ) )
      {
         DTLS_ReconnectRf();
         printUsage = false;
      }
      else if ( 0 == strcmp( argv[1], "pubkey" ) )
      {
         uint16_t keyLen;
         uint8_t  key[  VERIFY_256_KEY_SIZE ];  /* 64 byte key used to verify signatures.  */

         keyLen = DTLS_GetPublicKey( key );
         DBG_logPrintHex ( 'D', "public key ", key, keyLen );
         printUsage = false;
      }
   }

   if ( printUsage == true )
   {
      INFO_printf( "USAGE: dtls get OR dtls reset" );
   }

   return 0;
}
#endif
#if 0  // RA6E1 Bob: This command was removed from original K24 code
static uint32_t DBG_CommandLine_hardfault( uint32_t argc, char *argv[] )
{
   *(uint32_t *)0 = 0;
   return 0;
}
#endif // RA6E1 Bob: This command was removed from original K24 code
/*lint +esym(715,argc, argv, Arg0)  */

/******************************************************************************

   Function Name: DBG_CommandLine_EVL_UNIT_TESTING ( uint32_t argc, char *argv[] )

   Purpose: This function is used to initiate an event log unit test from the debug port
   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

******************************************************************************/
#if ( EVL_UNIT_TESTING == 1 )
uint32_t DBG_CommandLine_EVL_UNIT_TESTING( uint32_t argc, char *argv[] )
{
   uint16_t command;
   command = ( uint16_t )atoi( argv[1] );

   if ( argc > 1 )
   {
      switch ( command )
      {
         case ( 1 ) :
         {
            EVL_generateMakeRoomRealtimeFailure();
            break;
         }
         case ( 2 ) :
         {
            EVL_generateMakeRoomOpportunisticFailure();
            break;
         }
         case ( 3 ) :
         {
            if( argc == 4 )
            {
               uint8_t priorityThreshold;
               uint8_t alarmIndex;
               priorityThreshold = ( uint8_t )atoi( argv[2] );
               alarmIndex = ( uint8_t )atoi( argv[3] );
               EVL_loadLogBufferCorruption( priorityThreshold, alarmIndex);
            }
            else
            {
               INFO_printf( "COMMAND 3 USAGE: evlUnitTest 3 <buffer priority> <alarm index to search>" );
            }
            break;
         }
         default:
         {
            INFO_printf( "Command %d not implemented", command );
         }
      }
   }
   else
   {
      INFO_printf( "USAGE: evlUnitTesting cmd" );
      INFO_printf( "cmd: 1 - Generate a make room failure condition when logging an event in the real time alarm buffer" );
      INFO_printf( "cmd: 2 - Generate a make room failure condition when logging an event in the opportunistic alarm buffer" );
      INFO_printf( "cmd: 3 - Generate a corruption failure during call to EventLogLoadBuffer - USAGE: evlUnitTest 3 <buffer priority> <alarm index to search>" );
   }

   return ( 0 );
}
#endif
#if ( MCU_SELECTED == RA6E1 )
static uint32_t DBG_CommandLine_CoreClocks( uint32_t argc, char *argv[] )
{
  INFO_printf( "CPU Clocks (BSP): HOCO = %dMHz, MOCO = %dMHz, LOCO = %dHz, MAIN_OSC = %dMHz, Subclock = %dHz, PLL = %dMHz, PLL2 = %dMHz",
              R_BSP_SourceClockHzGet(FSP_PRIV_CLOCK_HOCO)     / 1000000,
              R_BSP_SourceClockHzGet(FSP_PRIV_CLOCK_MOCO)     / 1000000,
              R_BSP_SourceClockHzGet(FSP_PRIV_CLOCK_LOCO),
              R_BSP_SourceClockHzGet(FSP_PRIV_CLOCK_MAIN_OSC) / 1000000,
              R_BSP_SourceClockHzGet(FSP_PRIV_CLOCK_SUBCLOCK),
              R_BSP_SourceClockHzGet(FSP_PRIV_CLOCK_PLL)      / 1000000,
              R_BSP_SourceClockHzGet(FSP_PRIV_CLOCK_PLL2)     / 10000000 );
    INFO_printf( "CPU Clocks (FSP): PCLKD = %d, PCLKC = %d, PCLKB = %d, PCLKA = %d, BCLK = %d, ICLK = %d, FCLK = %d (all in MHz)",
              R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKD)    / 1000000,
              R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKC)    / 1000000,
              R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKB)    / 1000000,
              R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKA)    / 1000000,
              R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_BCLK)     / 1000000,
              R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_ICLK)     / 1000000,
              R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_FCLK)     / 1000000 );
  return 0;
}
#endif
#if ( TM_BSP_SW_DELAY == 1 )
static uint32_t DBG_CommandLine_TestSWDelay( uint32_t argc, char *argv[] )
{
   bsp_delay_units_t units;
   uint32_t startCycle, endCycle, diff, oldPriority;
   float mult;
   char *unitStr, *delayError = "Delay is too long for IWDT";
   if ( argc == 3 )
   {
      uint32_t delayPeriod = ( uint32_t )atol( argv[1] );
      if ( 0 == strcasecmp( "S", argv[2] ) )
      {
         units = BSP_DELAY_UNITS_SECONDS;
         if ( delayPeriod > 34 )
         {
            INFO_printf("%s", delayError);
            return ( 0 );
         }
      } else if ( 0 == strcasecmp( "M", argv[2] ) )
      {
         units = BSP_DELAY_UNITS_MILLISECONDS;
         if ( delayPeriod > 34000 )
         {
            INFO_printf("%s", delayError);
            return ( 0 );
         }
      } else if ( 0 == strcasecmp( "U", argv[2] ) )
      {
         units = BSP_DELAY_UNITS_MICROSECONDS;
         if ( delayPeriod > 34000000 )
         {
            INFO_printf("%s", delayError);
            return ( 0 );
         }
      }
      else
      {
         INFO_printf( "bad units parameter: %s, use s|m|u", argv[2] );
         return ( 0 );
      }
      INFO_printf( "Will shortly begin delay of %d %s", delayPeriod, argv[2] );
      OS_TASK_Sleep( 1000 ); //Wait for debug to print
      oldPriority = OS_TASK_Get_Priority( pTskName_Dbg );
      OS_TASK_Set_Priority( pTskName_Dbg, OS_MAX_TASK_PRIORITY );
      INFO_printf( "Delay is starting" );

      startCycle = DWT->CYCCNT;
      R_BSP_SoftwareDelay( delayPeriod, units);
      endCycle   = DWT->CYCCNT;

      OS_TASK_Set_Priority( pTskName_Dbg, oldPriority );
      diff = endCycle - startCycle;
      double dDiff = (double)diff / (double)getCoreClock();
      if ( dDiff >= 1.0 ) { unitStr = "sec"; mult = 1.0; }
      else if ( dDiff > 0.001 ) { unitStr = "msec"; mult = 1000.0; }
      else { unitStr = "usec"; mult = 1000000.0; }
      dDiff *= mult;
      INFO_printf( "Delay is complete: startCycle = %u, endCycle = %u, diff = %u, %u.%06u %s", startCycle, endCycle, diff,
                   (uint32_t)dDiff, (uint32_t)( ( dDiff - (uint32_t)dDiff ) * 1000000 ), unitStr );
   }
   else
   {
      INFO_printf("%s requires two parameters: delay time and units; units = { s | m | u }", argv[0] );
   }
   return ( 0 );
}
#endif

#if ( TM_MEASURE_SLEEP_TIMES == 1 )
/******************************************************************************

   Function Name: DBG_CommandLine_TestOsTaskSleep ( uint32_t argc, char *argv[] )

   Purpose: This function tests the OS_TASK_Sleep function to ensure that it waits
            at least as long as requested and no more than 1 tick + 2 msec longer
   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: always 0 (success)

   Notes:

******************************************************************************/
static uint32_t DBG_CommandLine_TestOsTaskSleep( uint32_t argc, char *argv[] )
{
   if ( argc < 4 )
   {
      DBG_printf( "Usage: %s numPasses randomSleepMaxMsed randomDelayMaxUsec", argv[0] );
   }
   else
   {
      uint16_t numPasses          = atoi( argv[1] );
      uint16_t randomSleepMaxMsec = atoi( argv[2] );
      uint16_t randomDelayMaxUsec = atoi( argv[3] );
      uint16_t pass = 0, fail = 0;
      uint16_t maxExtraDelay = (uint32_t)1000 / (uint32_t)configTICK_RATE_HZ + 2; /* Maximum permissable extra delay (1 tick + 2 msec) */
      R_BSP_SoftwareDelay( (uint32_t)aclara_randf( 0.0, (float)randomDelayMaxUsec ), BSP_DELAY_UNITS_MICROSECONDS );
      for ( uint16_t i = 0; i < numPasses; i++ )
      {
         uint32_t sleepMSec = (uint32_t)aclara_randf( 0.0, (float)randomSleepMaxMsec );
         uint32_t startTime = DWT->CYCCNT;
         OS_TASK_Sleep( sleepMSec ); // Be nice to other tasks
         uint32_t actualDelayMSec = (uint32_t)( ( 1000LL * (uint64_t)( DWT->CYCCNT - startTime ) ) / (uint32_t)getCoreClock() );
         if ( actualDelayMSec < sleepMSec )
         {
            fail++;
            DBG_printf( "Pass # %u, Delay too short: %u < %u", i+1, actualDelayMSec, sleepMSec );
         }
         else if ( actualDelayMSec > sleepMSec + maxExtraDelay )
         {
            fail++;
            DBG_printf( "Pass # %u, Delay too long:  %u > %u", i+1, actualDelayMSec, sleepMSec + maxExtraDelay );
         }
         else
         {
            pass++;
         }
      }
      DBG_printf("%u passes completed: %u good, %u bad", numPasses, pass, fail );
   }
   return ( 0 );
}
#endif // ( TM_MEASURE_SLEEP_TIMES == 1 )
///*lint +esym(818, argc, argv) argc, argv could be const */