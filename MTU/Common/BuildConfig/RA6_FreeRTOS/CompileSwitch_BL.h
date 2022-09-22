/***********************************************************************************************************************
 *
 * Filename: CompileSwitch.h
 *
 * Global Designator:
 *
 * Contents: Contains the configuration for the VLF project.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 * Revision History:
 *
 ******************************************************************************************************************** */
#ifndef CompileSwitch_H
#define CompileSwitch_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

// This is the standard build
#if ( ACLARA_LC != 1 ) && (ACLARA_DA != 1) /* meter specific code */
   #define ENABLE_ALRM_TASKS              1  /* Used to 0=disable, 1=enable the Alarm feature */
   #define ENABLE_ID_TASKS                1  /* Used to 0=disable, 1=enable the Interval Data feature */
   #define ENABLE_MFG_TASKS               1  /* Used to 0=disable, 1=enable the Mfg Port feature */
   #define ENABLE_TMR_TASKS               1  /* Used to 0=disable, 1=enable the Timer feature */
   #define ENABLE_TIME_SYS_TASKS          1  /* Used to 0=disable, 1=enable the System Time feature */
   #define ENABLE_HMC_TASKS               1  /* Used to 0=disable, 1=enable the Host Meter Comm feature */
   #define ENABLE_PAR_TASKS               1  /* Used to 0=disable, 1=enable the Partition feature */
   #define ENABLE_PWR_TASKS               1  /* Used to 0=disable, 1=enable the Power feature */
   #define ENABLE_DFW_TASKS               1  /* Used to 0=disable, 1=enable the DFW feature */
   #define ENABLE_DEMAND_TASKS            1  /* Used to 0=disable, 1=enable the Demand feature */
   #define ENABLE_FIO_TASKS               0  /* Used to 0=disable, 1=enable the FileIO CRC feature */
   #define ENABLE_LAST_GASP_TASK          1  /* Used to 0=disable, 1=enable the Last Gasp feature */
   #define ENABLE_SRFN_ILC_TASKS          0  /* Used to 0=disable, 1=enable the SRFN ILC feature */
   #define ENABLE_SRFN_DA_TASKS           0  /* Used to 0=disable, 1=enable the SRFN DA specific tasks */
   #define USE_IPTUNNEL                   0  /* Used to 0=disable, 1=enable the IP Tunneling port handling */
   #define SIGNAL_NW_STATUS               0  /* Used to network status monitor task */
#elif ACLARA_LC == 1
   #define ENABLE_ALRM_TASKS              0  /* Used to 0=disable, 1=enable the Alarm feature */
   #define ENABLE_ID_TASKS                0  /* Used to 0=disable, 1=enable the Interval Data feature */
   #define ENABLE_MFG_TASKS               1  /* Used to 0=disable, 1=enable the Mfg Port feature */
   #define ENABLE_TMR_TASKS               1  /* Used to 0=disable, 1=enable the Timer feature */
   #define ENABLE_TIME_SYS_TASKS          1  /* Used to 0=disable, 1=enable the System Time feature */
   #define ENABLE_HMC_TASKS               0  /* Used to 0=disable, 1=enable the Host Meter Comm feature */
   #define ENABLE_PAR_TASKS               1  /* Used to 0=disable, 1=enable the Partition feature */
   #define ENABLE_PWR_TASKS               1  /* Used to 0=disable, 1=enable the Power feature */
   #define ENABLE_DFW_TASKS               1  /* Used to 0=disable, 1=enable the DFW feature */
   #define ENABLE_DEMAND_TASKS            0  /* Used to 0=disable, 1=enable the Demand feature */
   #define ENABLE_FIO_TASKS               0  /* Used to 0=disable, 1=enable the FileIO CRC feature */
   #define ENABLE_LAST_GASP_TASK          1  /* Used to 0=disable, 1=enable the Last Gasp feature */
   #define ENABLE_SRFN_ILC_TASKS          1  /* Used to 0=disable, 1=enable the SRFN ILC feature */
   #define ENABLE_SRFN_DA_TASKS           0  /* Used to 0=disable, 1=enable the SRFN DA specific tasks */
   #define USE_IPTUNNEL                   0  /* Used to 0=disable, 1=enable the IP Tunneling port handling */
   #define SIGNAL_NW_STATUS               0  /* Used to network status monitor task */
#elif ACLARA_DA == 1
   #define ENABLE_ALRM_TASKS              0  /* Used to 0=disable, 1=enable the Alarm feature */
   #define ENABLE_ID_TASKS                0  /* Used to 0=disable, 1=enable the Interval Data feature */
   #define ENABLE_MFG_TASKS               1  /* Used to 0=disable, 1=enable the Mfg Port feature */
   #define ENABLE_TMR_TASKS               1  /* Used to 0=disable, 1=enable the Timer feature */
   #define ENABLE_TIME_SYS_TASKS          1  /* Used to 0=disable, 1=enable the System Time feature */
   #define ENABLE_HMC_TASKS               0  /* Used to 0=disable, 1=enable the Host Meter Comm feature */
   #define ENABLE_PAR_TASKS               1  /* Used to 0=disable, 1=enable the Partition feature */
   #define ENABLE_PWR_TASKS               1  /* Used to 0=disable, 1=enable the Power feature */
   #define ENABLE_DFW_TASKS               1  /* Used to 0=disable, 1=enable the DFW feature */
   #define ENABLE_DEMAND_TASKS            0  /* Used to 0=disable, 1=enable the Demand feature */
   #define ENABLE_FIO_TASKS               0  /* Used to 0=disable, 1=enable the FileIO CRC feature */
   #define ENABLE_LAST_GASP_TASK          1  /* Used to 0=disable, 1=enable the Last Gasp feature */
   #define ENABLE_SRFN_ILC_TASKS          0  /* Used to 0=disable, 1=enable the SRFN ILC feature */
   #define ENABLE_SRFN_DA_TASKS           1  /* Used to 0=disable, 1=enable the SRFN DA specific tasks */
   #define USE_IPTUNNEL                   1  /* Used to 0=disable, 1=enable the IP Tunneling port handling */
   #define SIGNAL_NW_STATUS               1  /* Used to network status monitor task */
#endif

#define PORTABLE_DCU                   0  /* 0=Normal Functionality, 1=Enable Portable DCU functionality */
#define MFG_MODE_DCU                   0  /* 0=Normal Functionality, 1=Enable MFG Mode DCU functionality */
/******************************************************************************************************************************************************
      Be careful when enabling NOISE_HIST_VERSION!
   It is intended for engineering use only. It uses a LARGE amount of RAM (~64K). With this enabled, the project may build, but not run
   because there is not enough RAM for MQX to allocate for stacks, Task descriptors, etc.
******************************************************************************************************************************************************/
#define NOISE_HIST_VERSION             0  /* 0=Normal Functionality, 1=Enable field testing end-point features */
#define SUPPORT_OLD_PHY      ( ( PORTABLE_DCU == 1 ) || ( NOISE_HIST_VERSION == 1 ) )  /* Support for the old PHY mode (Freq Dev, BT, shorter RS code) */
#define BOB_PADDED_PING      ( NOISE_HIST_VERSION )  /* 0=Ping has no extended payload, 1=Ping has pseudo-random extended payload */
#define BOB_IS_LAZY          ( NOISE_HIST_VERSION )  /* This enables a few shortened debug and MIMT port commands */
#define BOB_THIN_DEBUG       ( NOISE_HIST_VERSION )  /* This is used to thin out the debug */
#define NOISE_TEST_ENABLED   ( NOISE_HIST_VERSION )  /* This is used to enable the function to read the noise after each rx frame */
#define OTA_CHANNELS_ENABLED ( NOISE_HIST_VERSION )  /* This is used to enable the function to write the channels over the air */
#define NOISE_HIST_ENABLED   ( NOISE_HIST_VERSION )  /* This is used to enable the "noisehist" command and its radio API */

#define FAKE_TRAFFIC                   0  /* Should stay 0 on EP. This is used to simulate fake traffic between main board an T-board and thus stress the battery */
#if ( ( EP + PORTABLE_DCU + MFG_MODE_DCU ) > 1 )
#error Invalid Application device - Select ony one of EP, PORTABLE_DCU or MFG_MODE_DCU
#endif

#define DFW_TEST_KEY                   0  /* 1=Use DFW test key, 0=Used default DFW Key */
#define DFW_XZCOMPRESS_BSPATCHING      1  /* Update firmware patching technique - Uses XZ for decompression and minibs for patching */
/* Note:  All of the following DFW tests must be disabled "0" before releasing code! */
/* Select one below for DFW testing */
#define BUILD_DFW_TST_VERSION_CHANGE   0  /* Build for testing DFW, version change */
#define BUILD_DFW_TST_CMD_CHANGE       0  /* Build for testing DFW, Changes dbg cmd from GenDFWkey to GenDfwKey */
#define BUILD_DFW_TST_LARGE_PATCH      0  /* Build for testing DFW, Shifts cmd line by adding three NOP's */
#define BUILD_FVT_SHORT_DTLS_TIMEOUT   0  /* Disabled */

#define ENGINEERING_BUILD              0  /* Use to produce an engineering build */
/* This is a production build only if all of these are cleared */
#if ( ( OTA_CHANNELS_ENABLED + DFW_TEST_KEY + ENGINEERING_BUILD + \
        BUILD_DFW_TST_VERSION_CHANGE + BUILD_DFW_TST_CMD_CHANGE + BUILD_DFW_TST_LARGE_PATCH + BUILD_FVT_SHORT_DTLS_TIMEOUT ) == 0 )
#define PRODUCTION_BUILD               1
#else
#define PRODUCTION_BUILD               0
#endif

/* ------------------------------------------------------------------------------------------------------------------ */
#define ENABLE_DEBUG_PORT              0  /* Use to enable/disable the Debug port */
#if ( RTOS_SELECTION == MQX_RTOS )
#define DEBUG_PORT_BAUD_RATE           0
#define FULL_USER_INTERFACE            0  /* Prints actual error messages on debug commands rather than just ERROR */
#elif ( RTOS_SELECTION == FREE_RTOS )
#define DEBUG_PORT_BAUD_RATE           1  /* Enable command to change the debug port baud rate */
#define FULL_USER_INTERFACE            1  /* Prints actual error messages on debug commands rather than just ERROR */
#endif

/* For using trace signals on debugger - replaces LED signals! */
#define TRACE_MODE                     0

#define MULTIPLE_MAC                   1
#if ( MULTIPLE_MAC != 0 )
#define NUM_MACS                       MULTIPLE_MAC
#else
#define NUM_MACS                       1
#endif

/* KTL - the following can be removed at a later time - just used now for testing */
#define ENABLE_TEST_MESSAGEQ           0
#define ENABLE_AES_CODE                1 /* Use to enable/disable the AES code until it is fully working as we want it */
#define ENABLE_HMC_LPTSK               0

// Enable ADC1 for VSWR if necessary
#if (HAL_TARGET_HARDWARE == HAL_TARGET_Y84114_1_REV_A)
#define ENABLE_ADC1                    1
#else
#define ENABLE_ADC1                    0
#endif

/* ------------------------------------------------------------------------------------------------------------------ */
/* Printing Enable Definitions: */

/* ADC - Info */
#define ENABLE_PRINT_ADC               0

/* HMC - send/receive hmc packets   */
#define ENABLE_PRNT_HMC_MSG_INFO       0
#define ENABLE_PRNT_HMC_MSG_WARN       0
#define ENABLE_PRNT_HMC_MSG_ERROR      0

/* HMC - protocol assemble/decode results */
#define ENABLE_PRNT_HMC_PROT_INFO      0
#define ENABLE_PRNT_HMC_PROT_WARN      0
#define ENABLE_PRNT_HMC_PROT_ERROR     0

/* HMC - Diags */
#define ENABLE_PRNT_HMC_DIAGS_INFO     0
#define ENABLE_PRNT_HMC_DIAGS_WARN     1
#define ENABLE_PRNT_HMC_DIAGS_ERROR    1

/* HMC - Finish */
#define ENABLE_PRNT_HMC_FINISH_INFO    0
#define ENABLE_PRNT_HMC_FINISH_WARN    1
#define ENABLE_PRNT_HMC_FINISH_ERROR   1

/* HMC - Info */
#define ENABLE_PRNT_HMC_INFO_INFO      0
#define ENABLE_PRNT_HMC_INFO_WARN      1
#define ENABLE_PRNT_HMC_INFO_ERROR     1

/* HMC - Request */
#define ENABLE_PRNT_HMC_REQ_INFO       0
#define ENABLE_PRNT_HMC_REQ_WARN       1
#define ENABLE_PRNT_HMC_REQ_ERROR      1

/* HMC - Snapshot */
#define ENABLE_PRNT_HMC_SS_INFO        0
#define ENABLE_PRNT_HMC_SS_WARN        1
#define ENABLE_PRNT_HMC_SS_ERROR       1

/* HMC - Start */
#define ENABLE_PRNT_HMC_START_INFO     0
#define ENABLE_PRNT_HMC_START_WARN     1
#define ENABLE_PRNT_HMC_START_ERROR    1

/* HMC - Sync */
#define ENABLE_PRNT_HMC_SYNC_INFO      0
#define ENABLE_PRNT_HMC_SYNC_WARN      1
#define ENABLE_PRNT_HMC_SYNC_ERROR     1

/* HMC - Time */
#define ENABLE_PRNT_HMC_TIME_INFO      1
#define ENABLE_PRNT_HMC_TIME_WARN      1
#define ENABLE_PRNT_HMC_TIME_ERROR     1

/* HMC - Program Meter */
#define ENABLE_PRNT_HMC_PRG_MTR_INFO   1
#define ENABLE_PRNT_HMC_PRG_MTR_WARN   1
#define ENABLE_PRNT_HMC_PRG_MTR_ERROR  1

/* HMC - Disp */
#define ENABLE_PRNT_HMC_DISP_INFO      0
#define ENABLE_PRNT_HMC_DISP_WARN      0
#define ENABLE_PRNT_HMC_DISP_ERROR     0

/* DFW Module */
#define ENABLE_PRNT_DFW_INFO           1
#define ENABLE_PRNT_DFW_WARN           1
#define ENABLE_PRNT_DFW_ERROR          1

/* ALRM_Handler Module  */
#define ENABLE_PRNT_ALRM_INFO          0
#define ENABLE_PRNT_ALRM_WARN          1
#define ENABLE_PRNT_ALRM_ERROR         1

/* ------------------------------------------------------------------------------------------------------------------ */
#define USE_DTLS                       1
#if ( ACLARA_LC == 1 ) || (ACLARA_DA == 1)
#define DTLS_FIELD_TRIAL               0     /* Disable feature to allows unsecured comm. until session established   */
#else
#define DTLS_FIELD_TRIAL               1     /* Allows unsecured comm. until session established   */
#endif
#define DTLS_DEBUG                     (0)   /* Turn DTLS Debug On */  // TODO: RA6E1: Remove it before releasing for production
#define DTLS_CHECK_UNENCRYPTED         (1)   /* Check for previous version major file not encrypted   */
/* ------------------------------------------------------------------------------------------------------------------ */
#define BM_DEBUG                       0     /* Buffer allocate/free debug printing */
#define ENABLE_B2B_COMM                0     /* DCU3 XCVR only */
/* ------------------------------------------------------------------------------------------------------------------ */
#define USE_MTLS                       1
#define MTLS_DEBUG                     (1)   /* Turn MTLS Debug On */
/* ------------------------------------------------------------------------------------------------------------------ */

#define USE_USB_MFG                    0     /* This has to be 0 on all devices other than 9985T */
#define USB_DEBUG                      0     /* Enable extra output for USB debugging purposes. */

#if (END_DEVICE_NEGOTIATION == 1 && ANSI_LOGON == 1)
#define NEGOTIATE_HMC_COMM             0     /* Enable/disable comm negotiation */
#endif

#if ( NEGOTIATE_HMC_COMM == 1)
#define NEGOTIATE_HMC_BAUD             1     /* negotiate an alternative baud rate*/
#define NEGOTIATE_HMC_PACKET_NUMBER    0     /* negotiate an alternative MAX packet number*/
#define NEGOTIATE_HMC_PACKET_SIZE      0     /* negotiate an alternative MAX packet size*/
#endif
/* ------------------------------------------------------------------------------------------------------------------ */
//Enable Processor Random Number Generator if supported
#define ENABLE_PROC_RNG                1
/* ------------------------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------------------------ */
//Enable Sleep modes
#define ENABLE_SLEEP_IDLE              0
#define ENABLE_SLEEP_DEEP              0
/* ------------------------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------------------------ */
//Enable System Clock modes
#define ENABLE_LOW_POWER_SYS_CLK       0
/* ------------------------------------------------------------------------------------------------------------------ */
/**** Various Test Modes available in the Project ****/

/* Set to true to enable unit/integration test code */
#define TEST_MODE_ENABLE                  0     /* Set to 0 before releasing production code! */

// NOTE: 0=Production mode, !0=Enable the respective feature
#define TEST_QUIET_MODE                   0     /* Enable Debug output during quiet mode */
#define TEST_UNENCRYPTED                  0     /* DTLS test */
#define SIMULATE_POWER_DOWN               0     /* Enable the Power Down Simulation test (NOT FOR Production) */
#define TEST_REGULATOR_CONTROL_DISABLE    0     /* Disable the LDO and BOOST Control Pins */
#define LG_WORST_CASE_TEST                0     /* For testing the worst-case last gasp conditions.
                                                   Test procedure is listed in the SRFN Last Gasp Worst-Case Test Procedure document. */
#define ENABLE_TRACE_PINS_LAST_GASP       0     /* Set to non-zero for using enabling the TRACE_D0 & TRACE_D1 pin usage for Last Gasp test code  */
#define DEBUG_PWRLG                       0     /* If set, allows debugging by disconnecting PF signal from meter and manually grounding the
                                                   signal to simulate power fail, and reconnecting to meter to simulate power restoration.
                                                   NOTE: Be sure to clear VBAT memory and RFSYS memory prior to operation! */
#define LG_QUICK_TEST                     0     /* For quick testing - makes all windows the same as the outage declaration delay
                                                   Only available when DEBUG_PWR_LG is 0 */
#define DEBUG_LAST_GASP_TASK              0     /* This is dev test code to debug the LastGasp Task while the Power is still On */
#define TEST_TDMA                         0     /* Basic TDMA test */
#define TEST_DEVIATION                    0     /* Test 600Hz, 700Hz and 800Hz deviation */
#define OVERRIDE_TEMPERATURE              0     /* 0=Do not include temperature override, 1=Do inlcude temperature override */
#define TEST_SYNC_ERROR                   0     /* Allow configuration of the number of errored bit acceptable in SYNC */

/* All unit/integration defines MUST code inside the #if below! */
#if (TEST_MODE_ENABLE == 1)
#define TM_MUTEX                          0
#define TM_SEMAPHORE                      0
#define TM_RTC_UNIT_TEST                  0
#define TM_ADC_UNIT_TEST                  0
#define TM_QUEUE                          0
#define TM_MSGQ                           0
#define TM_EVENTS                         0
#define TM_LINKED_LIST                    0
#define TM_CRC_UNIT_TEST                  0
#define TM_TIME_COMPOUND_TEST             0
#define TM_OS_EVENT_TEST                  0 /* Test the time compound functions */
#define TM_INTERNAL_FLASH_TEST            0
#define TM_BSP_SW_DELAY                   0 /* Tests the Renesas R_BSP_SoftwareDelay function */
#define TM_ENHANCE_NOISEBAND_FOR_RA6E1    0 /* Enhancements to Noiseband: 1MHz clock test, list frequencies, control GPIO pins, extra HMC traffic */
#define TM_NOISEBAND_LOWEST_CAP_VOLTAGE   0 /* Capture lowest super-cap voltage during a noiseband run (requires TM_ENHANCE_NOISEBAND_FOR_RA6E1 = 1) */
#define TM_DELAY_FOR_TACKED_ON_LED        0 /* Adds some 2 second delays so that tacked-on LED is more human-visible */
#define TM_MEASURE_SLEEP_TIMES            0 /* Adds a debug command to measure the actual sleep times based on the CYCCNT */
#define TM_UART_ECHO_COMMAND              0 /* Adds an echo command to the debug port for testing UART echoing */
#define TM_INSTRUMENT_NOISEBAND_TIMING    0 /* Adds instrumentation of noiseband timing to determine if there are bugs */
#define TM_TEST_SECURITY_CHIP             0 /* More extensive test code for security chip that was disabled in the K24 starting point DOES NOT COMPILE! */
#define TM_UART_EVENT_COUNTERS            0 /* Various counters in UART.c and DBG_SerialDebug.c to figure out UART lockup issue */
#define TM_ROUTE_UNKNOWN_MFG_CMDS_TO_DBG  0 /* Route any unrecognized commands on the MFG port to the DBG_CommandLine_Process function */
#define TM_CREATE_TWO_BLABBER_TASKS       0 /* Create two tasks that output random numbers of messages on DBG port with random timing */
#if ( TM_UART_EVENT_COUNTERS == 1 )
#define TM_UART_COUNTER_INC(x) (x)++
#else
#define TM_UART_COUNTER_INC(x)
#endif
#define TM_RANDOM_NUMBER_GEN              0 /* Enable commands to test aclara random number generator */
#define TM_EXT_FLASH_BUSY_TIMING          0 /* Measure time for busyCheck in dvr_extflash to receive a complete interrupt */
#define TM_HARDFAULT                      0 /* Enable hardfault command for testing */
#define TM_BL_TEST_COMMANDS               0 /* Enable the Bootloader Test Commands */
#define TM_VERIFY_TICK_TIME               0 /* Enable delta time printout from loop in STRT_Startup.c every 10 seconds; 0 disables this test */
#define TM_TICKHOOK_SEMAPHORE_POST_ERRORS 1 /* Count number of semaphore post errors from the tickHooks of FreeRTOS (N/A for MQX) */
//#define TEST_COM_UPDATE_APPLET    /* If defined, causes the com params to be set to unusual values. */
//#define TM_HMC_APP                /* Enabled - Makes the application static variables global for watch window. */
//#define TM_UART_BUF_CLR           /* When defined the UART buffers will clear when the port is opened. */
//#define TM_POWER_UP_TIMING        /* Enables the power up timing code.  This affects the boot loader and app. */
//#define TM_ADC_DRIVER_UNIT_TEST   /* Enables the Unit Test for the ADC driver. */
//#define TM_BANKED_DRIVER_VALIDATE /* Enabled - Verifies the banked driver configuration. */
//#define TM_OL_HEAP_SIZE           /* Creates a global variable that can be used to see how much heap is used. */
//#define TM_SPI_DRIVER_UNIT_TEST   /* Enabled - compiles SPI driver unit test code - Needs print util. */
//#define TM_BANKED_UNIT_TEST       /* Enable the Banked Driver Unit Test Code */
//#define TM_CACHE_UNIT_TEST        /* Enable the Cached Driver Unit Test Code */
//#define TM_ENCRYPT_UNIT_TEST      /* Enable the Encryption Driver Unit Test Code */
//#define TM_AES_UNIT_TEST          /* Enable the AES Unit Test Code */
//#define TM_DTLS_UNIT_TEST         /* Enable the DTLS Unit Test Code */
#endif
/* These are now part of normal build   */
#ifndef __BOOTLOADER
//TODO: undefine external flash driver testing for boot
#define TM_DVR_EXT_FL_UNIT_TEST     /* Enabled - Run unit testing on external flash driver. */
#define TM_PARTITION_TBL            /* Will validate the partition tables. */
#define TM_PARTITION_USAGE          /* Enables printing of partition usage */
#endif  /* NOT BOOTLOADER */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#endif /* this must be the last line of the file */
