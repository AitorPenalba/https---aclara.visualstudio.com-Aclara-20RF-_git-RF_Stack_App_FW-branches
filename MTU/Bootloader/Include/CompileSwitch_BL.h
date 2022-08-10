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
 * Copyright 2013-2015 Aclara.  All Rights Reserved.
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
#define ENABLE_ALRM_TASKS              1  // Used to 0=disable, 1=enable the Alarm feature
#define ENABLE_ID_TASKS                1  // Used to 0=disable, 1=enable the Interval Data feature
#define ENABLE_MFG_TASKS               1  // Used to 0=disable, 1=enable the Mfg Port feature
#define ENABLE_TMR_TASKS               1  // Used to 0=disable, 1=enable the Timer feature
#define ENABLE_TIME_SYS_TASKS          1  // Used to 0=disable, 1=enable the System Time feature
#define ENABLE_HMC_TASKS               1  // Used to 0=disable, 1=enable the Host Meter Comm feature
#define ENABLE_PAR_TASKS               1  // Used to 0=disable, 1=enable the Partition feature
#define ENABLE_PWR_TASKS               1  // Used to 0=disable, 1=enable the Power feature
#define ENABLE_DFW_TASKS               1  // Used to 0=disable, 1=enable the DFW feature
#define ENABLE_DEMAND_TASKS            1  // Used to 0=disable, 1=enable the Demand feature
#define ENABLE_FIO_TASKS               0  // Used to 0=disable, 1=enable the FileIO CRC feature
#define ENABLE_LAST_GASP_TASK          1  // Used to 0=disable, 1=enable the Last Gasp feature

#define PORTABLE_DCU                   0  /* 0=Normal Functionality, 1=Enable Portable DCU functionality */
#define MFG_MODE_DCU                   0  /* 0=Normal Functionality, 1=Enable MFG Mode DCU functionality */
#define OTA_CHANNELS_ENABLED           0  // This is used to enable the function to write the channels over the air
#define NOISE_TEST_ENABLED             0  // This is used to enable the function to read the noise after each rx frame
#if ( ( EP + PORTABLE_DCU + MFG_MODE_DCU ) > 1 )
#error Invalid Application device - Select ony one of EP, PORTABLE_DCU or MFG_MODE_DCU
#endif

#define HAL_IGNORE_BROWN_OUT_SIGNAL    0 /* 1 = Ignore brown-out signal, 0 = Use Brown Out Signal */

#define DFW_TEST_KEY                   0  /* 1=Use DFW test key, 0=Used default DFW Key */
/* Note:  All of the folowing DFW tests must be disabled "0" before releasing code! */
/* Select one below for DFW testing */
#define BUILD_DFW_TST_VERSION_CHANGE   0  /* Build for testing DFW, version change */
#define BUILD_DFW_TST_CMD_CHANGE       0  /* Build for testing DFW, Changes dbg cmd from GenDFWkey to GenDfwKey */
#define BUILD_DFW_TST_LARGE_PATCH      0  /* Build for testing DFW, Shifts cmd line by adding three NOP's */

/* This is a production build only if all of these are cleared */
#if ( ( OTA_CHANNELS_ENABLED + DFW_TEST_KEY + \
        BUILD_DFW_TST_VERSION_CHANGE + BUILD_DFW_TST_CMD_CHANGE + BUILD_DFW_TST_LARGE_PATCH ) == 0 )
#define PRODUCTION_BUILD               1
#else
#define PRODUCTION_BUILD               0
#endif

#define __BOOTLOADER                   1

/* ------------------------------------------------------------------------------------------------------------------ */
#define ENABLE_DEBUG_PORT              0  // Use to enable/disable the Debug port

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
#define ENABLE_AES_CODE                1 // Use to enable/disable the AES code until it is fully working as we want it
#define ENABLE_ADC1                    0
#define ENABLE_HMC_LPTSK               0

/* ------------------------------------------------------------------------------------------------------------------ */
/* Printing Enable Definitions: */

/* ADC - Info */
#define ENABLE_PRINT_ADC               0

/* HMC - send/receive hmc packets   */
#define ENABLE_PRNT_HMC_MSG_INFO       0
#define ENABLE_PRNT_HMC_MSG_WARN       0
#define ENABLE_PRNT_HMC_MSG_ERROR      0

/* HMC - protcol assemble/decode results */
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
#define DTLS_FIELD_TRIAL               1     /* Allows unsecured comm. until session established   */
#define DTLS_DEBUG                     (0)   /* Turn DTLS Debug On */
/* ------------------------------------------------------------------------------------------------------------------ */
#define BM_DEBUG                       0     /* Buffer allocate/free debug printing */
#define ENABLE_B2B_COMM                0     /* DCU3 XCVR only */


/* ------------------------------------------------------------------------------------------------------------------ */
//Enable Processor Random Number Generator if supported
#if (HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A)  //K22 is the only one that does not support the RNG
#define ENABLE_PROC_RNG                0
#else
#define ENABLE_PROC_RNG                1
#endif
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

/* Set to true to enable unit/integration test code */
#define TEST_MODE_ENABLE               0     /* Set to 0 before releasing production code! */

/* All unit/integration defines MUST code inside the #if below! */
#if (TEST_MODE_ENABLE == 1)
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
#define TM_DVR_EXT_FL_UNIT_TEST   /* Enabled - Run unit testing on external flash driver. */
#define TM_PARTITION_TBL          /* Will validate the partition tables. */
#define TM_PARTITION_USAGE        /* Enables printing of partition useage */

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
