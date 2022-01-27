/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: hmc_ds.h
 *
 * Contents: Meter Disconnect Switch Globals
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012-2015 Aclara.  All Rights Reserved.
 ***********************************************************************************************************************
 *
 * $Log$ Created March 27, 2012
 *
 * Updates:
 * 01/19/2015 - Port to RF electric
 *
 **********************************************************************************************************************/
#ifndef HMC_DS_H_
#define HMC_DS_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "APP_MSG_Handler.h"
#include "hmc_ds_cfg.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef HMC_DS_GLOBALS
   #define HMC_DS_EXTERN
#else
   #define HMC_DS_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* MT117 status bit definitions  */
#define AC_LINE_SIDE                   ((uint8_t)1)
#define AC_LOAD_SIDE                   ((uint8_t)2)
#define CAP_V_OK                       ((uint8_t)0)
#define CAP_V_LOW                      ((uint8_t)1)
#define CAP_V_HIGH                     ((uint8_t)2)

#define NO_VOLTAGE_PRESENT             ((uint8_t)0) // no line or load side voltage detected and the disconnect switch board
#define ONLY_LINE_SIDE_VOLTAGE_PRESENT ((uint8_t)1) // only line side voltage detected and the disconnect switch board
#define ONLY_LOAD_SIDE_VOLTAGE_PRESENT ((uint8_t)2) // only load side voltage detected and the disconnect switch board
#define LINE_LOAD_SIDE_VOLTAGE_PRESENT ((uint8_t)3) // line and load side voltage detected and the disconnect switch board

/* load side voltage status definitions */
#define LOAD_SIDE_VOLTAGE_DEAD                            ((uint8_t)0)
#define LOAD_SIDE_VOLTAGE_COMPARE_NOT_AVAILABLE           ((uint8_t)1)
#define LOAD_SIDE_VOLTAGE_NOT_SYNCHRONIZED_WITH_LINE_SIDE ((uint8_t)2)
#define LOAD_SIDE_VOLTAGE_SYNCHRONIZED_WITH_LINE_SIDE     ((uint8_t)3)

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

#if ( REMOTE_DISCONNECT == 1 )
PACK_BEGIN
typedef struct PACK_MID
{
   /* STR   */
   uint8_t  loxli                   :1;   /* Set in update switch status to indicate that the 180 out of phase (criss
                                             cross bypass) been detected. EXAC is also set when this is set */
   uint8_t  lohz                    :1;   /* Load Side AC Frequency Error  */
   uint8_t  lihz                    :1;   /* Line Side AC Frequency Error  */
   uint8_t  ccst                    :2;   /* Charge Capacitor Status: indicates the state of the charge capacitor
                                             voltage. The two bits CCST1 and CCST0 together represent the status as
                                             follows:
                                             0x0 - within operating limits
                                             0x1 - below 30 V DC
                                             0x2 - above 40 V DC*/
   uint8_t  acot                    :1;   /* AC Present on Output: the switch did not close because AC was detected on
                                             the load side.  */
   uint8_t  swft                    :1;   /* Switch Position Fault: previous command was not successful. */
   uint8_t  cflt                    :1;   /* Command Fault: indicates an invalid command. */

   /* SSR   */
   uint8_t  swst                    :1;   /* Detected Switch State: 0 - Open 1 - Closed   */
   uint8_t  cmst                    :1;   /* Commanded Switch State: 0 - Open 1 - Closed   */
   uint8_t  acst                    :2;   /* AC Voltage State. The two bits ACST1 and ACST0 together represent the
                                             status as follows:
                                             0x0 - No AC
                                             0x1 - Line Side AC
                                             0x2 - Load Side AC
                                             0x3 - Line/Load Side AC */
   uint8_t  exac                    :1;   /* External AC Voltage Detected
                                             0 - No Load Side AC
                                             1 - Load Side AC  */
   uint8_t  armf                    :1;   /* Arm Fault: this flag is set whenever an open/close command is received
                                             without setting the pre-requisite arm bit first.  */
   uint8_t  capf                    :1;   /* Set when switch actuation fails because of low capacitor voltage or failed
                                             capacitor voltage detection   */
   uint8_t  filler                  :1;

   /* CVR   */
   uint8_t  cvr;                          /* Capacitor Voltage Register from the daughter board. Convert this into volts
                                             by multiplying by 0.196 and rounding?  */
   /* FW    */
   uint8_t  revision                :4;   /* Revision  */
   uint8_t  version                 :4;   /* Version  */
} RCDCStatus, *pRCDCStatus;               /* GE I210+ MT117 */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t communication_error      :1; /*Bit 0: Indicates a communication error or no response has occurred during
                                          recent communication with switch control board */
   uint8_t switch_controller_error  :1; /*Bit 1: Indicates that current switch state, switch controller status, and
                                          current reading in meter are not a valid combination. */
   uint8_t switch_failed_to_close   :1; /*Bit 2: When commanded to close the switch, the meter will attempt to close
                                          the switch every 5 seconds for up to 3 minutes until the switch closes or
                                          time runs out. If the meter is not able to close the switch, this bit will
                                          be set. */
   uint8_t alternate_source         :1; /*Bit 3: Indicates that the switch controller detected a load side voltage
                                          with a different frequency than the line side voltage while the switch is
                                          open with no current running through the meter. The condition must be
                                          present for 3 consecutive 1-minute status updates before it is set. */
   uint8_t filler                   :1; /*Bit 4: RSVD */
   uint8_t bypassed                 :1; /*Bit 5: Indicates that the meter has detected a load side voltage with the
                                          same frequency as the line side voltage while the switch was open and no
                                          current is running through the meter. The condition must be present for 3
                                          consecutive 1-minute status updates before it is set...*/
   uint8_t switch_failed_to_open    :1; /*Bit 6: When commanded to open the switch, the meter will attempt to open
                                          the switch every 5 seconds for up to 3 minutes until the switch opens or
                                          time runs out. If the meter is not able to open the switch, this bit will
                                          be set. */
   uint8_t ppm_alert                :1; /*Bit7: Indicates that the PPM Remaining Credit is negative.
                                          * 0 - PPM Remaining Credit > PPM Alert
                                          * 1 - PPM Remaining Credit <= PPM Alert */
   uint8_t manual_armed_time_out    :1; /*Bit8: Indicates that the Manual Arm Timer expired without a switch closure.
                                          This is cleared the next time the switch is successfully closed in Manual
                                          Arm mode.
                                          0 - Last Manual arming did not time out
                                          1 - Manual Arm Timer timed */
   uint8_t line_side_freq_error     :1; /*Bit 9: Indicates that switch controller detected line side frequency error.
                                          Sets when line side frequency is not within the range 45Hz - 66Hz*/
   uint8_t load_side_freq_error     :1; /*Bit 10: Indicates that switch controller detected load side frequency
                                          error. Sets when load side frequency is not within the range 45Hz - 66Hz*/
   uint8_t rsvd                     :5; /* Bits 11 - 15 */
} lctStatus;
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t filler1                  :1; /*Bit 0: RSVD */
   uint8_t ecp_enabled              :1; /*Bit 1: Emergency Conservation enabled  */
   uint8_t dlp_enabled              :1; /*Bit 2: Demand limiting enabled         */
   uint8_t ppm_enabled              :1; /*Bit 3: Pre-payment enabled             */
   uint8_t filler2                  :1; /*Bit 4: RSVD */
   uint8_t ecp_disconnect           :1; /*Bit 5: */
   uint8_t dlp_disconnect           :1; /*Bit 6: */
   uint8_t ppm_disconnect           :1; /*Bit7: */
} lc_state;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{  /* MFG Table 115, Load Control Status Table */
   /* Information based on the Reading and Programming guide for the I210_ Electicity Meter( 9/24/2010) */
   lctStatus status;          /* See structure above  */ /*lint !e123*/
   lctStatus history;         /* Latches any errors/status in the Status byte. Same bit structure as Status.   */

   struct /*Remote connect/disconnect state: Switch state information */
   {
      uint8_t actual_switch_state      : 1; /*Bit 0: 0 = Open, 1 = Close */
      uint8_t desired_switch_state     : 1; /*Bit 1: 0 = Open, 1 = Close */
      uint8_t open_hold_for_command    : 1; /*Bit 2: 0 = False, 1 = True */
#if ( HMC_I210_PLUS_C == 1 )
      uint8_t waiting_to_arm           : 1; /*Bit 3: Not supported in the I210+c meter */
      uint8_t armed_waiting_to_close   : 1; /*Bit 4: 0 = False, 1 = True */
      uint8_t outage_open_in_effect    : 1; /*Bit 5: 0 = False, 1 = True */
      uint8_t lockout_in_effect        : 1; /*Bit 6: 0 = False, 1 = True */
      uint8_t rsvd                     : 1; /*Bit 7: Reserved. */
#else
      uint8_t armed_waiting_to_close   : 1; /*Bit 3: 0 = False, 1 = True */
      uint8_t outage_open_in_effect    : 1; /*Bit 4: 0 = False, 1 = True */
      uint8_t lockout_in_effect        : 1; /*Bit 5: 0 = False, 1 = True */
      uint8_t waiting_to_arm           : 1; /*Bit 6: Not supported in the I210+ meter */
      uint8_t rsvd                     : 1; /*Bit 7: Reserved. */
#endif
   } rcdcState;
   lc_state lcState;             /* Typ of Load control in effect */
   uint8_t  lcReconnectAttempts; /* The number of reconnect attempts remaining in the Demand Limit or Energy
                                    conservation period  */
   union
   {
#if ( HMC_I210_PLUS == 1 )
      uint32_t ecpAccum;            /* Acuumulates energy consumed when ECP override flag is set and the demand exeeded
                                       the ECP Demand Threshold. The way the meter accumulates is determined by the LC
                                       Source and the accumulator is set to a value on 0.1 Wh/unit through SP22. SP3
                                       Clear Data resets this accumulator to 0. */
      uint32_t ppmRemCredit;        /* Each momentary interval, the Pre-Payment Accumulator is adjusted by the energy
                                       accumulated in deci Wh during the momentary interval based on the Detent
                                       (Source).  Delivered energy is subtracted if the detent is 0, 1 or 2. Received
                                       energy is subtracted the detent is 1 or 3. Received energy is added if the detent
                                       is 2. SP3 Clear Data resets this accumulator to 0. */
#elif ( HMC_I210_PLUS_C == 1 )
      uint8_t ecpAccum[4];          /* Acuumulates energy consumed when ECP override flag is set and the demand exeeded
                                       the ECP Demand Threshold. The way the meter accumulates is determined by the LC
                                       Source and the accumulator is set to a value on 0.1 Wh/unit through SP22. SP3
                                       Clear Data resets this accumulator to 0. */
      uint8_t ppmRemCredit[4];      /* Each momentary interval, the Pre-Payment Accumulator is adjusted by the energy
                                       accumulated in deci Wh during the momentary interval based on the Detent
                                       (Source).  Delivered energy is subtracted if the detent is 0, 1 or 2. Received
                                       energy is subtracted the detent is 1 or 3. Received energy is added if the detent
                                       is 2. SP3 Clear Data resets this accumulator to 0. */
#endif
   } accumCredit;
   uint32_t    lcDemandCalculated;  /* The demand calculated in ECP/DLP mode at the end of a demand interval is
                                       stored/overwritten in this field in deciwatt hours/ demand interval units
                                       irrespective of whether it exceeded the demand threshold or not. SP3 clear data
                                       resets this demand to Zero.   */
   uint8_t     countDownHours;      /* Hours   remaining in the Energy Conservation Period. */
   uint8_t     countDownMinutes;    /* Minutes remaining in the Energy Conservation Period. */
#if ( HMC_I210_PLUS == 1 )
   uint8_t     lastCommandStatus;   /* 0x00 = Last close command was successful
                                       0x10 = Last Open command was successful
                                       0x01 = Last command (open or close) is not successful
                                       0x11 = Not applicable   */
   uint8_t     filler3[4];          /* RSVD  */
#elif ( HMC_I210_PLUS_C == 1 )
   uint16_t    rcdc_switch_count;   /* Counts the RD switch operations and will never be reset t zero after factory
                                       configuration  */
   uint8_t     filler3[3];          /* Unused   */
#endif
   uint16_t    crc;
} ldCtrlStatusTblGPlus_t; /* G+ meter MFG Table 115, Load Control Status Table */
PACK_END
// </editor-fold>
typedef struct
{
   uint8_t                 OpCodeLockOut     :1;   /* Time to hold off the Opcode after a procedure execution */
   uint8_t                 OpCodeProcessed   :1;   /* TRUE=OpCode rec'd/decoded by the API. To be sent to meter */
   uint8_t                 hwExists          :1;   /* Local memory of h/w present status for pwr down/up */
   uint8_t                 ds_unused         :5;   /* Room for expansion   */
} dsMetaData;

// <editor-fold defaultstate="collapsed" desc="mtrDsFileData_t: File structure for the disconnect switch module.">
typedef struct
{
   uint32_t          lastHEEPtime;     /* Time stamp of last HE cmd that resulted in switch movement  */
   regStateStatus_u  dsStatusReg;      /* Service Disconnect Status' */
   uint16_t          sdSwActCntr;      /* #Service Disconnect Open Counter */
   EDEventType       OpCodeCmd;        /* Command paramter: 145 = Connect  146 = disconnect   */
   dsMetaData        dsInfo;
}mtrDsFileData_t;                               /* File structure for the disconnect switch. */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="eLastOpSrc_t:  Switch state/status Field Definition">
//lint -esym(751, eLastOpSrc_t)  //  Not referenced
typedef enum
{
   eCmdAmr = (uint8_t)0,
   eAutonomous             /* Soft fuse or demand limiting or Optical command */
}eLastOpSrc_t;             /* Valid Values for "Last Operation Source" field. */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="eSdCmdStatus_t:  Valid Values for Switch status fields">
//lint -esym(749,eCmdAccepted,eCmdRejectedLsv,eCmdArmRejected,eCmdCodeNotSupported) Included for completeness
typedef enum
{
   eCmdAccepted =        (uint8_t)0,
   eCmdRejectedLsv,
   eCmdNoSd =            (uint8_t)5,      /*Command rejected because meter is not capable of service disconnect*/
   eCmdInProcess =       (uint8_t)8,      /* Last command execution still in progress  */
   eCmdCodeNotSupported,
   eSwFailedToClose =    (uint8_t)11,
   eSwFailedToOpen
}eSdCmdStatus_t;        /* Valid Values , "SD Command Status" field. */

typedef enum                     /* Table read retry status flags */
{
   eTableRetryNotWaiting = 0,
   eTableRetryWaiting,
   eTableRetryTmrExpired
}eTableRetryStatus_t;

/* Last Operation Attempted Field Definition */
typedef enum
{
   eCloseSd = (uint8_t) 0,
   eOpenSd,
   ePowerUpState
}LastOperationAtempt_t;


/*  This register shall be updated at the following times:
 *  1.	At power-up
 *  2.	Every 24 hours at time 0x0000 if the RCE has a valid date and time.
 *  3.	For OpCode #151, within 1 minute after execution.
 *  4.	For OpCode #153, within 1 minute after executing the scheduled switch operation. */
typedef struct
{
   uint16_t rsvd;          /* Unused */
   uint16_t sdOpenCnt;     /* Service Disconnect Open Counter */
}serviceDsActivityCntr_t;  /* Service Disconnect Switch Activity Counter, Register #515 */
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */

// <editor-fold defaultstate="collapsed" desc="mtrDsRegDefTbl: Register Definition Table">
#if ( TWACS_REGISTERS_SUPPORTED == 1 )
static const tRegDefintion const mtrDsRegDefTbl_[]
#ifdef HMC_DS_GLOBALS
= /* Define registers that belong to the DS module. */
{
   // <editor-fold defaultstate="collapsed" desc="REG_DS_STATUS">
   {
      { /* Disconnect Switch Status */
         REG_DS_STATUS,                                     /* Reg Number */
         REG_DS_STATUS_SIZE,                                /* Reg Length */
         REG_MEM_FILE,                                      /* Memory Type */
         (uint8_t *) & mtrDsFileHndl_,                      /* Address of Register */
         //lint -e{40, 78, 446, 350, 530, 732}
         offsetof(mtrDsFileData_t, dsStatusReg),            /* Offset of the register */
         (int8 const *)NULL,                                /* pointer to decoder. */
         &REG_InitVal[0],                                   /* pointer to initial value */
         REG_PROP_NATIVE_FMT,                               /* Register is stored in native format. */
         !REG_PROP_WR_TWACS,                                /* Writable over TWACS/STAR (Main Network)? */
         !REG_PROP_WR_MFG,                                  /* Writable over MFG? */
         !REG_PROP_WR_PERIF,                                /* Writable over Perif */
         REG_PROP_RD_TWACS,                                 /* Readable over TWACS/STAR (Main Network)? */
         REG_PROP_RD_MFG,                                   /* Readable over MFG? */
         REG_PROP_RD_PERIF,                                 /* Readable over Perif? */
         !REG_PROP_SECURE,                                  /* Secure? */
         !REG_PROP_MANIPULATE,                              /* Manipulate? */
         !REG_PROP_CHECKSUM,                                /* Checksum? */
         !REG_PROP_MATH,                                    /* Math - Add? */
         !REG_PROP_MATH_ROLL,                               /* Inc/Dec roll over/under? */
         !REG_PROP_SIGNED,                                  /* Signed value? */
         REG_PROP_USE_DEFAULT_FIRST_POWER_UP,               /* Set default at 1st time ever power up? */
         !REG_PROP_USE_DEFAULT_EVERY_POWER_UP,              /* Set default at every power up? */
         REG_PROP_PRE_WRITE_FUNCTION                        /* True=Pre-Write function, False=Post-Write function - if
                                                               fxn is NULL this item doesn't matter */
      },
      NULL,                                                 /* Read Function Ptr: Pre-Read */
      NULL                                                  /* Write Function Ptr: Pre-Write, or Post-Write, dependent
                                                               on setting above */
   }, // </editor-fold>

   // <editor-fold defaultstate="collapsed" desc="Service Disconnect Switch Activity Counter">
   {
      { /* Service Disconnect Switch Activity Counter */
         REG_DS_CONTACT_CNT,                             /* Reg Number */
         REG_DS_CONTACT_CNT_SIZE,                        /* Reg Length */
         REG_MEM_FILE,                                   /* Memory Type */
         ( uint8_t * )&mtrDsFileHndl_,                   /* Address of Register */
         //lint -e{40, 78, 446, 350, 530, 732}
         (uint16_t)offsetof(mtrDsFileData_t, sdSwActCntr), /* Offset of the register */
         (int8 const *)NULL,                             /* pointer to decoder. */
         &REG_InitVal[0],                                /* pointer to initial value */
         REG_PROP_NATIVE_FMT,                            /* Register is stored in native format. */
         !REG_PROP_WR_TWACS,                             /* Writable over TWACS/STAR (Main Network)? */
         !REG_PROP_WR_MFG,                               /* Writable over MFG? */
         !REG_PROP_WR_PERIF,                             /* Writable over Perif */
         REG_PROP_RD_TWACS,                              /* Readable over TWACS/STAR (Main Network)? */
         REG_PROP_RD_MFG,                                /* Readable over MFG? */
         REG_PROP_RD_PERIF,                              /* Readable over Perif? */
         !REG_PROP_SECURE,                               /* Secure? */
         !REG_PROP_MANIPULATE,                           /* Manipulate? */
         !REG_PROP_CHECKSUM,                             /* Checksum? */
         REG_PROP_MATH,                                  /* Math - Add? */
         REG_PROP_MATH_ROLL,                             /* Inc/Dec roll over/under? */
         !REG_PROP_SIGNED,                               /* Signed value? */
         REG_PROP_USE_DEFAULT_FIRST_POWER_UP,            /* Set default at 1st time ever power up? */
         !REG_PROP_USE_DEFAULT_EVERY_POWER_UP,           /* Set default at every power up? */
         REG_PROP_PRE_WRITE_FUNCTION                     /* True=Pre-Write function, False=Post-Write function - if fxn
                                                            is NULL this item doesn't matter */
      },
      NULL,                                                 /* Read Function Ptr: Pre-Read */
      NULL                                                  /* Write Function Ptr: Pre-Write, or Post-Write, dependent
                                                               on setting above */
   } // </editor-fold>
}
#endif
;
#endif
// </editor-fold>

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

uint8_t        HMC_DS_Applet( uint8_t ucCmd, void far *pData);
void           HMC_DS_ExecuteSD(HEEP_APPHDR_t *heepHdr, void *payloadBuf, uint16_t length);
uint16_t       HMC_DS_getSwitchPositionCount( void );
#ifdef TM_SD_UNIT_TEST
void           HMC_DS_UnitTest(void);
void           UnitTest_ResponseCodes(void);
#endif

#undef HMC_DS_EXTERN

#endif
