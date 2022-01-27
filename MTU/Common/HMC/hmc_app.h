/***********************************************************************************************************************
 *
 * Filename: hmc_app.h
 *
 * Contents:  #defs, typedefs, and function prototypes for the routines to
 *    support applet meter communication needs
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ kad Created 080207
 *
 **********************************************************************************************************************/

#ifndef mtr_app_H
#define mtr_app_H

/* INCLUDE FILES */

#include "psem.h"
#include "hmc_prot.h"

#ifdef HMC_APP_GLOBAL
   #define APP_EXTERN
#else
   #define APP_EXTERN extern
#endif

/* CONSTANT DEFINITIONS */

#define HMC_APP_MAX_QUEUE_SIZE ((unsigned portBASE_TYPE)30)
#ifndef NULL
   #define NULL 0
#endif

/* ***************************************************************************************************************** */
/* Meter Application Commands and Responses */

/* Meter Application Commands */
enum HMC_APP_CMD
{
   HMC_APP_CMD_INIT = 0,            /* initialize the Meter Application and all other layers and applets */
   HMC_APP_CMD_PROCESS,             /* Normal Process command, checks all applets and executes commands */
   HMC_APP_CMD_STATUS,              /* Check if meter communication is logged off or in */
   HMC_APP_CMD_TIME_TICK,           /* Increment the timers for time out */
   HMC_APP_CMD_SET_COM_DEFAULTS,    /* Sets the communication default parameters */
   HMC_APP_CMD_SET_COMPORT,         /* Sets up the communications Port, Baud Rate */
   HMC_APP_CMD_SET_PSEM_ID,         /* Sets the PSEM ID of the device we're talking to */
   HMC_APP_CMD_DISABLE,             /* Disable the Meter Application, will only disable if session is closed */
   HMC_APP_CMD_ENABLE,              /* Enables the Meter Application */
   HMC_APP_CMD_MSG,                 /* RTOS Message */
};

/* Meter Application Reply */
enum HMC_APP_RPLY
{
/* Meter Application response */
   HMC_APP_PROC_LOW_CPU = 0,        /* Returned when a process command is sent to the meter application */
   HMC_APP_PROC_HIGH_CPU,           /* Returned when a process command is sent to the meter application */
   HMC_APP_INV_CMD,                 /* Returned when a unknown command is sent to the meter application */
   HMC_APP_STATUS_LOGGED_OFF = 0,   /* Returned when a status or init command is sent to the meter application */
   HMC_APP_STATUS_LOGGED_ON,        /* Returned when a status or init command is sent to the meter application */
   HMC_APP_DISABLED  = 0,           /* Response to an ENABLE or DISABLE command */
   HMC_APP_ENABLED,                 /* Response to an ENABLE or DISABLE command */
};

/* Applet commands and responses are defined in the cfg_app.h file.  This way items can be added for custom commands */

/* ***************************************************************************************************************** */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

APP_EXTERN OS_QUEUE_Obj HMC_APP_QueueHandle;

/* FUNCTION PROTOTYPES */
uint8_t HMC_APP_AccessMeter(uint8_t id, void *pData);
uint8_t HMC_APP_main(uint8_t, void *);
#ifdef __mqx_h__
void HMC_APP_Task( uint32_t Arg0 );
#endif /* __mqx_h__ */
returnStatus_t HMC_APP_RTOS_Init( void );
bool HMC_APP_status(void);
returnStatus_t HMC_APP_TaskPowerDown(void);
returnStatus_t HMC_APP_ResetAppletTimeout(void);

#undef GLOBAL

#endif
