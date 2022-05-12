/******************************************************************************

   Filename: cfg_app.h

   Contents: Application Configuration

 ******************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2018-2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ******************************************************************************

   $Log$ kad Created 102907

   1.0 DC 03/21/2011 Modified TAMPER_RSVD_MASK from 0x8F00 to 0x9B00 to make this
       the correct mask for E34 Tamper register RSVD bits (E34 bug 6324)
 *****************************************************************************/

#ifndef cfg_app_H
#define cfg_app_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "features.h"
#ifndef __BOOTLOADER
#include "os_aclara.h"
#include "portable_freescale.h"
#endif   /* __BOOTLOADER   */

#ifndef __BOOTLOADER
#include "error_codes.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include "user_config.h"
#endif
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define EP                             1

// One of the meter types below must be defined as 1 in the project e.g. GE_I210+.ewp
#ifndef HMC_KV
#define HMC_KV                         0              /* Set if the Host meter is a KV, KV2, KV2C */
#endif
#ifndef HMC_I210_PLUS_C
#define HMC_I210_PLUS_C                0              /* Set if the Host meter is a I210+c   */
#endif
#ifndef HMC_I210_PLUS
#define HMC_I210_PLUS                  0              /* Set if the Host meter is a I210+ */
#endif
#ifndef HMC_FOCUS
#define HMC_FOCUS                      0              /* Set if the host meter is a Focus */
#endif
#ifndef ACLARA_DA
#define ACLARA_DA                      0              /* Set if the host meter is an Aclara DA */
#endif
#ifndef ACLARA_LC
#define ACLARA_LC                      0              /* Set if the host meter is an Aclara LC */
#endif
#if ( (HMC_KV + HMC_I210_PLUS + HMC_I210_PLUS_C + HMC_FOCUS + ACLARA_LC + ACLARA_DA) != 1 )
#error "Bad HMC selection in meter.h file"
#endif

// These defines are used to control the DCU/EndPoint specific functionality
#ifndef HE
#define HE  0     // Used to configure for Head End  Functionality (not supported)
#endif
#ifndef DCU
#define DCU 0     // Used to configure for DCU       Functionality
#endif
#ifndef DCU2_PLUS
#define DCU2_PLUS 0 // Used to configure for DCU2 +  Functionality
#endif
#ifndef EP
#define EP  0     // Used to configure for End Point Functionality
#endif
#if ( ( HE + DCU + EP ) != 1 )
#error Invalid Application device - Select one of HE, DCU or EP
#endif

/* user_config.h defines the BSP_ALARM_FREQUENCY; use this to derive the OS_TICK values   */
#define BSP_ALARM_FREQUENCY            (200)
#define OS_TICK_mS                     ((uint32_t)1000/BSP_ALARM_FREQUENCY)
#define OS_TICK_uS                     ((uint32_t)OS_TICK_mS * 1000)

/* Message types in the system */
#define MSG_TYPE_MERCURY               ((uint8_t)1)
#define MSG_TYPE_TMR                   ((uint8_t)2)
#define MSG_TYPE_TIME_SYS              ((uint8_t)3)

#define CFG_SYS_BUFFER_SIZE            ((uint8_t)14)
#define CFG_SYS_STYLE_POINTER          ((uint8_t)0)
#define CFG_SYS_STYLE_DATA             ((uint8_t)1)

#define BL_VERSION_ADDR                ((uint8_t *)0x80000002)
#define BL_VERSION_LEN                 ((uint8_t)1)

#else
#endif   /* __BOOTLOADER   */
#ifdef _DEV_PROJECT
#define STATIC
#else
#define STATIC static
#endif

#define ASSERT(test) // This should be moved to a file called ASSERT!

#ifndef __BOOTLOADER
#ifndef _DEV_PROJECT
#if TEST_MODE_ENABLE == 1
#warning "*** This build is not to be released.  It is configured for DEBUGGING or Test Mode only. ***" /*lint !e16 */
#endif
#endif
#endif   /* BOOTLOADER  */

/* ------------------------------------------------------------------------------------------------------------------ */
/* Global Project Macro Definitions */

#ifndef __BOOTLOADER
#define RTOS                              1
#else
#define RTOS                              0
#define BSP_BUS_CLOCK 60000000UL
#endif   /* BOOTLOADER  */
#ifndef __BOOTLOADER
#define MIN_120V                          ((uint16_t)720) //in eighth's -- 90V
#define MIN_240V                          ((uint16_t)1200) //in eighth's -- 150V

/* ------------------------------------------------------------------------------------------------------------------ */
/* Diagnostic, Tamper and Alarm */

/* Masks for RESERVED bits in the diagnostic, alarm, and tamper registers.  These are needed to meet the requirement
   that RSVD bits in these registers are not used for activating the header flag bits.
   NOTE:  These are presently set for a LITTLE ENDIAN C COMPILER.  See HEADER.C to see how these are used... */
#define DIAGNOSTIC_RSVD_MASK           ((uint16_t)0xA37A)
#define ALARM_RSVD_MASK                ((uint16_t)0x000F)
#define OUTAGE_RSVD_MASK               ((uint16_t)0x0100)

/* Header bit location definitions */
#define DIAGNOSTICBITMASK              ((uint8_t)0x7F)
#define ALARMBITMASK                   ((uint8_t)0x7F)
#define TAMPERBITMASK                  ((uint8_t)0xBF)
#define TAMPER_REVERSE_BITMASK         ((uint8_t)0xDF)
#define OUTAGEBITMASK                  ((uint8_t)0xEF)
#define REVERSEMASK                    ((uint16_t)0x8000)   //assumes little endian

//TAMPER register 36.6 (For E34)
#define TAMPER_RSVD_MASK               ((uint16_t)0x9B00)   //this is byte reversed to work with header.c
#define TAMPER_BAD_PASSWORD            ((uint16_t)0x0001)   //Bad meter password * Not byte reversed * Not used on E34
#define TAMPER_LSV                     ((uint16_t)0x0002)   //Unexpected Load side voltage * Not byte reversed
#define TAMPER_MAGNET                  ((uint16_t)0x0010)   //Tampering using magnet * Not byte reversed
#define TAMPER_NO_TC24                 ((uint16_t)0x0008)
#define TAMPER_REV_ROT                 ((uint16_t)0x0080)   //Reverse rotation * Not byte reversed * Not used in E34

/* ------------------------------------------------------------------------------------------------------------------ */
/* Host Meter Energy Configuration */
#define TIME_ADJ_THRESHOLD_SEC         ((uint8_t)2)         /* xxx Need to make this a variable! */

//Set the following to the Max energy that any meter in the TYPE can register used in DEMAND and INTERVAL
#define MAX_ENERGY_HR                  (100000L)      //Max energy can be registered in 1 hour
#define ABNT_6_NINES                   (999999999L)   //ABNT 6 nines KWH in WH
#define ABNT_5_NINES                   (99999999L)    //ABNT 5 nines KWH in WH
#define ABNT_4_NINES                   (9999999L)     //ABNT 4 nines KWH in WH

/* ------------------------------------------------------------------------------------------------------------------ */
/* Demand Definitions */
#define DEMAND_FUNCTIONS_LOCAL
#define DEMAND_SOURCE_REGISTER            (uint16_t)1865    /* Demand expects BIG ENDIAN! */
/* ------------------------------------------------------------------------------------------------------------------ */
/* Interval Data Definitions */

#define ENABLE_DELTA_INTERVALS
#define INTERVAL_DATA_PRESENT

#define  SAMPLE_RATE_15_MIN               ((uint8_t)3)
#define  SAMPLE_RATE_30_MIN               ((uint8_t)4)
#define  SAMPLE_RATE_60_MIN               ((uint8_t)5)
#define  ID_MIN_PERIOD                    SAMPLE_RATE_15_MIN   //PDS defined
#define  ID_MAX_PERIOD                    SAMPLE_RATE_60_MIN   //PDS defined

/* Interval data Storage length parameter (1 means 2 bytes of storage) */
#define ID_PDS_LIMIT_STORAGE_LEN          ((uint8_t)1)
#define ID_NV_MEM_SIZE                    ((uint16_t)20160)    /* Bytes */

#define OPCODE_221_INT_BIT_LEN_MODE_1     ((uint8_t)13)
#define OPCODE_221_INT_BIT_LEN_5_MIN      ((uint8_t)8)         /* Bit Len values are from the PDS */
#define OPCODE_221_INT_BIT_LEN_10_MIN     ((uint8_t)8)
#define OPCODE_221_INT_BIT_LEN_15_MIN     ((uint8_t)8)
#define OPCODE_221_INT_BIT_LEN_30_MIN     ((uint8_t)10)
#define OPCODE_221_INT_BIT_LEN_60_MIN     ((uint8_t)10)
#define OPCODE_221_RET_BYTE_LEN           ((uint8_t)10)        /* This value is defined in the PDS */
#define ID_NUM_OF_INTVS_TO_CLR            ((uint8_t)2)

/* ------------------------------------------------------------------------------------------------------------------ */
/* RCE Initialize Command */

#define DO_CAI
#define DO_RTM
#define DO_DA
//#define DO_RR
//#define DO_CHMD
#define DO_CPR
#define DO_SDST
#define DO_WBCD
#define DO_WFGUA

//first byte
#define CAI_MASK                          ((uint8_t)128)
#define RTM_MASK                          ((uint8_t)64)
#define DA_MASK                           ((uint8_t)32)
#define RR_MASK                           ((uint8_t)16)
#define CHMD_MASK                         ((uint8_t)8)
#define CPR_MASK                          ((uint8_t)4)
#define SDST_MASK                         ((uint8_t)2)
#define WBCD_MASK                         ((uint8_t)1)
//Second byte
#define WFGUA_MASK                        ((uint8_t)4)
#define RES1_MASK                         ((uint8_t)2)
#define RES2_MASK                         ((uint8_t)1)

#define DST_SIZE_BITS                     ((uint8_t)16)
#define BCD_SIZE_BITS                     ((uint8_t)16)
#define FGUS_SIZE_BITS                    ((uint8_t)4)
#define FGU_SIZE_BITS                     ((uint8_t)32)
#define TM_SIZE                           ((uint8_t)2)
#define ROLLOVER_SIZE                     ((uint8_t)4)
#define OPCODE_166_RET_SIZE_CNST          ((uint8_t)2)
/* ------------------------------------------------------------------------------------------------------------------ */
/* Self Test */

#define TST_CS_BLOCK_SIZE                 ((uint16_t)32) /* Max size is 0xFFFF */

/* ------------------------------------------------------------------------------------------------------------------ */
/* Temperature Sensor Definitions */

#define MIN_TEMPERATURE                   ((uint8_t)0)
#define MAX_TEMPERATURE                   ((uint8_t)165)


/* End Device Programming Type */
#define ED_PROG_FLASH_NOT_SUPPORTED 0      /* must stay zero */
#define ED_PROG_FLASH_PATCH_ONLY    1
#define ED_PROG_FLASH_BASECODE_ONLY 2
#define ED_PROG_FLASH_SUPPORT_ALL   3



/* Timers - timer_util.c/h module */

#define INVALID_TIMER_ID         ((uint8_t)0)      /* Timer ID 0 is NOT VALID */
#define MAX_TIMERS               ((uint8_t)55)     /* Maximum number of timers supported  */
#define SYS_TIME_TICK_IN_mS      ((uint32_t)(10))  /* milliseconds in each system time unit */
#define portTICK_RATE_MS         ((uint8_t)5)      /* RTOS tick rate TODO: should be defined in a system setting! */
#define WAIT_1_SYS_TIME_UNIT     (SYS_TIME_TICK_IN_mS/portTICK_RATE_MS)

/* Time - time_util.c/h module */

#define MAX_ALARMS               ((uint8_t)20)       /* Maximum alarms that can be defined */

/* ------------------------------------------------------------------------------------------------------------------ */
/* HMC */

typedef enum
{
   eDEFAULT_METER = ( uint8_t )0,
   eGPLUS_RD = eDEFAULT_METER,
   eFOCUS_AL,
   eFOCUS_AX,
   eKV
} eMeterType_t;   /* If only one meter is supported, it should be the first in the list. */

#define SUPPORT_METER_TIME_FORMAT      1              /* Time conversion utilities should support meter time   */

/* ------------------------------------------------------------------------------------------------------------------ */
/* Meter Interface Definitions */

#if ( HMC_KV || HMC_I210_PLUS_C )
#if ( MCU_SELECTED == NXP_K24 )
//This is the CBF_BK_KV_METER (Comm Board Force/Busy KV) signal connector pin 27
#define HMC_COMM_FORCE()            (((GPIOD_PDIR & (1<<4))>>4)) /* COMM_FORCE signal from the host meter */
#define HMC_COMM_FORCE_TRIS()       { PORTD_PCR4 = 0x100;   GPIOD_PDDR &= ~(1<<4); } /* Set for GPIO, Make Input */

//This is the CBS_BM_KV_METER (Comm Board Sense/Busy Modem) signal connector pin 28
//TODO Add this to the code
#define HMC_COMM_SENSE_OFF()        GPIOA_PCOR = 1<<5       /* COMM_SENSE signal Low */
#define HMC_COMM_SENSE_ON()         GPIOA_PSOR = 1<<5       /* COMM_SENSE signal High */
/* Set PCR for GPIO, Make Output */
#define HMC_COMM_SENSE_TRIS()       { PORTA_PCR5 = 0x100; GPIOA_PDDR |= 1<<5; HMC_COMM_SENSE_OFF(); }

#elif ( MCU_SELECTED == RA6E1 )

//This is the CBF_BK_KV_METER (Comm Board Force/Busy KV) signal connector pin 27
#define HMC_COMM_FORCE()            1  // TODO: RA6E1 Setting to 1 for now
#define HMC_COMM_FORCE_TRIS()

//This is the CBS_BM_KV_METER (Comm Board Sense/Busy Modem) signal connector pin 28
//TODO Add this to the code
#define HMC_COMM_SENSE_OFF()         R_BSP_PinWrite(BSP_IO_PORT_00_PIN_01, BSP_IO_LEVEL_LOW)
#define HMC_COMM_SENSE_ON()          R_BSP_PinWrite(BSP_IO_PORT_00_PIN_01, BSP_IO_LEVEL_HIGH)

/* Set PCR for GPIO, Make Output */
#define HMC_COMM_SENSE_TRIS()
#endif

#if ( HMC_KV == 1 )
//This is the PR_KV_METER (Password Recovery) signal
//TODO add this to code
#define HMC_PW_RECOVERY()           (((GPIOD_PDIR & (1<<5))>>5)^1) /* COMM_FORCE signal from the host meter */
#define HMC_PW_RECOVERY_TRIS()      { PORTD_PCR5 = 0x100;   GPIOD_PDDR &= ~(1<<5); } /* Set for GPIO, Make Input */
#endif

#if ( HMC_I210_PLUS_C == 1 )
#if ( MCU_SELECTED == NXP_K24 )
/* This is the MC_I-210+C_METER on pin 12 */
#define HMC_MUX_CTRL_TRIS()         { PORTD_PCR5 = 0x100; GPIOD_PDDR |= (1<<15);}   /* Make GPIO, output */
#define HMC_MUX_CTRL_ASSERT()       { GPIOD_PCOR = (1<<5); HMC_MUX_CTRL_TRIS() }    /* Take pin low before HMC */
#define HMC_MUX_CTRL_RELEASE()      { GPIOD_PSOR = (1<<5); HMC_MUX_CTRL_TRIS() }    /* Take pin high after HMC */

#elif ( MCU_SELECTED == RA6E1 )
/* This is the MC_I-210+C_METER on pin 12, signal name MC_I-210+C METER on Y84581-SCH */
#define HMC_MUX_CTRL_TRIS()
#define HMC_MUX_CTRL_ASSERT()       R_BSP_PinWrite(BSP_IO_PORT_00_PIN_04, BSP_IO_LEVEL_LOW)
#define HMC_MUX_CTRL_RELEASE()      R_BSP_PinWrite(BSP_IO_PORT_00_PIN_04, BSP_IO_LEVEL_HIGH)
#endif
#endif

//This is the T_KV_METER (Meter Trouble) signal
//TODO Add this to code
#define HMC_TROUBLE()               (((GPIOC_PDIR & (1<<11))>>11)) /* TROUBLE signal from the host meter */
#define HMC_TROUBLE_TRIS()          { PORTC_PCR11= 0x100;   GPIOC_PDDR &= ~(1<<11); } /* Set for GPIO, Make Input */
#endif

#define MAX_NBR_CONST_ENTRIES       ((uint8_t)25)     /* Used to dimension standard table 15 */
#define MAX_NBR_SOURCES             ((uint8_t)100)    /* Used to dimension standard table 16 */

#define MULTI_HOST_METER_SUPPORT    1                 /* Supports multiple meter configurations */

#define HMC_KV_UART_SETUP_DELAY     ((uint32_t)5)     /* The KV2 should wait at least 1.2mS after driving TX high. */
#define HMC_APP_TASK_DELAY_MS       ((uint32_t)100)   /* Number of mS to sleep when scanning applets */
#define HMC_APP_TASK_DELAY_MS_TST   ((uint32_t)200)   /* Number of mS to sleep when scanning applets - In test mode */

#define HMC_COMPORT                 DVR_UART2
#define U2RX_SIZE                   ((uint8_t)64)     /* Low level driver buffer size for RX - Max 255 */
#define U2TX_SIZE                   ((uint8_t)60)     /* Low Level Driver Buffer Size for TX - Max 255 */
#define HMC_MSG_TIMER_INTERVAL_mS   ((uint8_t)1)      /* Number of mS for each time tick. */
#define HMC_MSG_IGNORE_TOGGLE_BIT   1                 /* Set if the Toggle bit from the host meter is not checked */
#define HMC_MSG_CLEAR_RX_BUFFER     1                 /* Clears RX buffer before receiving data (nice for debugging) */

/* The following parameters need to match the host meter's parameters. */
#define HMC_MSG_MAX_RX_LEN             (64U)          /* This includes the 8 bytes of packet overhead */
#define HMC_MSG_MAX_TX_LEN             (64U)
#define HMC_REQ_MAX_BYTES              (HMC_MSG_MAX_RX_LEN-12U)   /* Max # of byte read/written when performing partial table access. */
#define HMC_MSG_RETRIES                ((uint8_t)3)
#define HMC_MSG_ERROR_TIME_OUT_mS      ((uint8_t)60)
#define HMC_MSG_RESPONSE_TIME_OUT_mS   ((uint16_t)4000)
#define HMC_MSG_TURN_AROUND_TIME_mS    ((uint8_t)1)      /* Ms */
#define HMC_MSG_BUSY_DELAY_mS          ((uint16_t)2000)
#define HMC_MSG_ICHAR_TIME_OUT_mS      ((uint16_t)1000)  /* Inter-character Time Out */
#define HMC_MSG_TRAFFIC_TIME_OUT_mS    ((uint16_t)30000)
#define HMC_CMD_MSG_MAX                ( HMC_REQ_MAX_BYTES )
#define RESTART_DELAY_FUDGE_MS         ((uint8_t)100)

/* Units is the frequency of Meter App calls with HMC_APP_CMD_TIME_TICK parameter */
#define HMC_APP_APPLET_TIMEOUT_mS      ((uint32_t)6*60*1000)   /* Timeout in 6 Minutes */
#define HMC_APP_DISABLED_TIMER_mS      ((uint32_t)300000)      /* Disable for 5 minutes */

/* ------------------------------------------------------------------------------------------------------------------ */
/* Tamper Definitions */
#define REV_TC_TAMPER_TRIP_POINT   ((int64_t)-255)     /* Maximum reverse energy before setting the tamper flag. */
#define REV_TC_TAMPER_FLAG_MASK    ((uint16_t)0x8000)  /* Bytes are swapped! Sets bit 7 over TWACS */

#define ENABLE_METER_EVENT_LOGGING 1     /* Set non-zero to support meter event logs in meters that are capable.  */

/* ****************************************************************************************************************** */
#endif
/* TYPE DEFINITIONS */

typedef enum /* Note:  The enum is filled with examples, actual code may differ */
{
   ePART_SEARCH_BY_TIMING = ( uint8_t )0,
   ePART_NV_APP_RAW,
   ePART_NV_APP_BANKED,
   ePART_NV_RSVD0,
   ePART_NV_APP_CACHED,
   /* *** The following will not be searched by timing.  They can only be searched by the Partition Name. *** */
   ePART_START_NAMED_PARTITIONS = ( uint8_t )0x80,    /* When MS bit is set, part mgr will not look for auto timing. */
   ePART_NV_VIRGIN_SIGNATURE = ePART_START_NAMED_PARTITIONS,
   ePART_NV_MAC_PHY,
   /* 130 */
   ePART_NV_DFW_NV_IMAGE,
   ePART_NV_DFW_PATCH,
   ePART_PGM_APP,
   ePART_NV_DFW_BITFIELD,
   ePART_NV_RSVD1,
   /* 135 */
   ePART_ENCRYPT_NV,
   ePART_HD,
   ePART_NV_RSVD2,
   ePART_NV_RSVD3,
   ePART_NV_RSVD4,
   /* 140 */
   ePART_NV_RSVD5,
   ePART_NV_RSVD6,
   ePART_NV_RSVD7,
   ePART_NV_RSVD8,

   ePART_DTLS_MAJOR_DATA,
   /* 145 (if ID_CH_8) */
   ePART_DTLS_MINOR_DATA,
   ePART_NV_HIGH_PRTY_ALRM,
   /* 145 (if not ID_CH_8) */
   ePART_NV_LOW_PRTY_ALRM,
   ePART_WHOLE_NV,
   ePART_ALL_APP,                      /* All partitions that are a part of the "Search by timing" */
   /* 150 (if ID_CH_8) */
   ePART_ALL_PARTITIONS,               /* All partitions. */
   ePART_APP_CODE,                     /* SAME AS K22 ePART_INT_FLASH_CODE? */
   /* 150 (if not ID_CH_8) */
   ePART_ENCRYPT_KEY,
   ePART_DFW_PGM_IMAGE,
   ePART_NV_TEST,
   /* 155 (if ID_CH_8) */
   ePART_MTR_INFO_DATA,
   ePART_BL_CODE,
   ePART_APP_UPPER_CODE,
   ePART_SWAP_STATE,
   ePART_BL_BACKUP,
   ePART_DFW_BL_INFO,
   ePART_LAST_PARTITION                /* This marker MUST be here, it indicates we're past the last partition. */
} ePartitionName;
#ifndef __BOOTLOADER
enum APP_CMD
{
   APP_DISABLE,
   APP_ENABLE,
   APP_PROCESS
};
/* API commands, 0-100 for Meter App, 100-255 - User configurable */
enum HMC_APP_API_CMD
{
   HMC_APP_API_CMD_ACTIVATE = 0,    /* Tell the applet to set it meter communication flags */
   HMC_APP_API_CMD_INIT,            /* Initialize the Applet */
   HMC_APP_API_CMD_STATUS,          /* Ask the applet for the status of host meter com needed */
   HMC_APP_API_CMD_LOGGED_IN,       /* Tell the applet we're logged in */
   HMC_APP_API_CMD_LOG_OFF,         /* Tell the applet we're logged off */
   HMC_APP_API_CMD_PROCESS,         /* Tell the applet to process, fill the point with com information */
   HMC_APP_API_CMD_MSG_COMPLETE,    /* Tell the applet the message is complete, passed with com data */
   HMC_APP_API_CMD_TBL_ERROR,       /* Tell the applet the table read had an error, com data is passed to the applet */
   HMC_APP_API_CMD_ERROR,           /* Tell the applet the requested communication had an error */
   HMC_APP_API_CMD_ABORT,           /* Tell the applet to abort communications, session will be restarted. */
   HMC_APP_API_CMD_INV_REGISTER,    /* Tell the applet the register is invalid, data was not written */
   HMC_APP_API_CMD_INV_RESPONSE,    /* Tell the applet it responded with an unknown command */
   HMC_APP_API_CMD_RINIT_VERIFY,    /* Ask the applet if re-initialize is really valid */
   HMC_APP_API_CMD_COM_PARAM,       /* Com parameters are sent to the applet */
   HMC_APP_API_CMD_COMPORT_PARAM,   /* Comport parameters are sent to the applet */
   HMC_APP_API_CMD_TIME_TICK,       /* Time Tick has occurred */
   HMC_APP_API_CMD_SPEC_REG,        /* Special Register Check */
   HMC_APP_API_CMD_ENABLE_MODULE,   /* Enables the Applet */
   HMC_APP_API_CMD_DISABLE_MODULE,  /* Disables the Applet */
   HMC_APP_API_CMD_VALID_REG,       /* Check if the register is valid */
   HMC_APP_API_CMD_BUSY,            /* Check if the register is valid */
   HMC_APP_API_CMD_MSG,
   HMC_APP_API_CMD_GET_QUANTITY,    /* Gets a quantity of measure from the host meter */
   /* Custom commands are to be added after this point.  Custom commands must start at 100 */

   HMC_APP_API_CMD_CUSTOM_UPDATE_COM_PARAMS = 100 // For the meter start applet, sets its local copy of the com params.
};
/* Applet Responses */
enum HMC_APP_API_RPLY
{
   HMC_APP_API_RPLY_IDLE = 0,             /* Idle, nothing to do */
   HMC_APP_API_RPLY_READY_COM,            /* Data ready to send message */
   HMC_APP_API_RPLY_RDY_COM_PERSIST,      /* High Priority Task, if session is restarted, start with this task
                                             again. */
   HMC_APP_API_RPLY_ABORT_SESSION,        /* Abort the session, log off */
   HMC_APP_API_RPLY_RE_INITIALIZE,        /* Re-initialize all applications */
   HMC_APP_API_RPLY_PAUSE,                /* return without performing any actions */
   HMC_APP_API_RPLY_LOGGING_IN,           /* Appet is logging into the host meter (only start com uses this) */
   HMC_APP_API_RPLY_LOGGED_IN,            /* Appet is logged into the host meter (only start com uses this) */
   HMC_APP_API_RPLY_LOGGED_OFF,           /* Appet is logged off the host meter (only finish com uses this) */
   HMC_APP_API_RPLY_RE_INIT_VERIFIED,     /* Re-initialize all applications, response to HMC_APP_API_CMD_RINIT_VERIFY
                                             cmd */
   HMC_APP_API_RPLY_GET_COM_PARAM,        /* Request to update the communication parameters, only to be returned after a
                                             successful com (HMC_APP_API_CMD_MSG_COMPLETE) OR a status command,
                                             (HMC_APP_API_CMD_STATUS) */
   HMC_APP_API_RPLY_WRITE_COM_PARAM,
   HMC_APP_API_RPLY_GET_COMPORT_PARAM,    /* Request to update the communication parameters, only to be returned after a
                                             successful com (HMC_APP_API_CMD_MSG_COMPLETE) OR a status command,
                                             (HMC_APP_API_CMD_STATUS) */
   HMC_APP_API_RPLY_WRITE_COMPORT_PARAM,
   HMC_APP_API_RPLY_UNSUPPORTED_CMD       /* Applet doesn't understand the command */

   /* Custom responses are to be added after this point. */
};
/* Data format to pass to the Any Application
   This message format shall be used, if any queue is expected to receive multiple message types
   The exception is specialized modules like buffer management */
typedef struct
{
#if 0 // TODO: RA6E1 - Queue handling
   OS_QUEUE_Element  QueueElement;  /* QueueElement as defined by MQX */
#endif
   uint8_t           msgType;       // Type of message
   void              *pData;
} sysQueueEntry_t;
typedef enum
{
   eCFG_SYS_MSG_TYPE_TMR = ( uint8_t ) 1,
   eCFG_SYS_MSG_TYPE_TIME_SYS,
   eCFG_SYS_MSG_TYPE_LCD_DRIVER,
   eCFG_SYS_MSG_TYPE_INTF_ANSI_CMD,
   eCFG_SYS_MSG_TYPE_INTF_OPCODE_CMD,
   eCFG_SYS_MSG_TYPE_BILLING_SHIFT,
   eCFG_SYS_MSG_TYPE_INTF_INTERVAL_DATA,
   eCFG_SYS_MSG_TYPE_INTERVAL_DATA,
   eCFG_SYS_MSG_TYPE_DAILY_SHIFT,
   eCFG_SYS_MSG_TYPE_HMC_ANSI_CMD,
   eCFG_SYS_MSG_TYPE_HMC_MTR_SNAP,
   eCFG_SYS_MSG_TYPE_HMC_DIAGS,
   eCFG_SYS_MSG_TYPE_HMC_DIAGS_REG,
   eCFG_SYS_MSG_TYPE_HMC_MTR_DISP,
   eCFG_SYS_MSG_TYPE_SEQUENCE,
   eCFG_SYS_MSG_TYPE_TURN_ON_RECEIVER,
   eCFG_SYS_MSG_TYPE_LED_TOGLE,
   eCFG_SYS_MSG_TYPE_DE_TIMEOUT,
   eCFG_SYS_MSG_TYPE_DFW_TIMEOUT,
   eCFG_SYS_MSG_TYPE_AFC_TIMEOUT,
   eCFG_SYS_MSG_POWER_RESTORATION_ID,
   eCFG_SYS_MSG_TYPE_DIAG_MESSAGE,
   eCFG_SYS_MSG_TYPE_MAIN,
   eCFG_SYS_MSG_TYPE_REG,
   eCFG_SYS_MSG_TYPE_CFG_SYS,
   eCFG_SYS_MSG_TYPE_TIME_THRES_EXCEEDED_FWD,
   eCFG_SYS_MSG_TYPE_TIME_THRES_EXCEEDED_REV,
   eCFG_SYS_MSG_TYPE_RSSI_AGEING_TIMEOUT,
} cfgSysMsgTypes_t;
/* Data format to pass to the Any Application - This is the 'Standard' Queue Format */
typedef struct
{
#if 0 // TODO: RA6E1 - Queue handling
   OS_QUEUE_Element  QueueElement;   /* QueueElement as defined by MQX */
#endif
   uint8_t ucPacketFormat; /* Pointer style or Structure, 0 = Pointers, 1 = Structure */
   uint8_t ucMsgType; /* Type of message */

   union /* Combine both pointer and buffer formats, use less memory */
   {
      struct
      {
         void *pDataToApplet;    // for Opcode #222, this points to tOpcode222HMC
         void *pDataFromApplet;  // for Opcode #222, this is NULL
      } sPointerData;
      uint8_t ucBuffer[CFG_SYS_BUFFER_SIZE];
   } uData;
} tSysQueueFormat;
/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#endif   /* __BOOTLOADER   */
#endif
