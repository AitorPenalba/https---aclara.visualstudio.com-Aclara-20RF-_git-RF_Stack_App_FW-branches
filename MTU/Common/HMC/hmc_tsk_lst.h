/******************************************************************************
 *
 * Filename: hmc_tsk_lst.h
 *
 * Contents: Contains a list of meter tasks to run
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2020 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ******************************************************************************
 *
 * $Log: mtr_tsk_lst.h,v $
 * Revision 1.15  2010-12-15 15:41:06  kdavlin
 * Moved meter energy above meter maps since meter energy should be operated on first.
 *
 * Revision 1.14  2010-12-09 20:18:50  kdavlin
 * Added Meter ANSI Command Opcode.
 *
 * Revision 1.13  2010-11-18 20:45:00  kdavlin
 * Now uses a variable to define the table size.
 *
 * Revision 1.12  2010-10-18 19:14:45  kdavlin
 * Added Display Commands
 *
 * Revision 1.11  2010-09-21 19:15:05  kdavlin
 * Added HMC diags applet.
 *
 * Revision 1.10  2010-09-16 18:42:56  kdavlin
 * Added new HMC applets.
 *
 * Revision 1.9  2010-08-04 18:15:28  kdavlin
 * Added new applet.
 *
 * Revision 1.8  2010-07-28 18:47:03  kdavlin
 * Removed the status applet from the list.  Moved Energy readings, demand and interval to the top of the list.
 *
 * Revision 1.7  2010-07-15 14:34:13  kdavlin
 * Added historical data.
 *
 * Revision 1.6  2010-06-09 20:21:55  msingla
 * Code cleanup and moved function call to initialize HMC interface to main file
 *
 * Revision 1.5  2010-06-04 19:39:00  james
 * fixed a meter function mis-count
 *
 * Revision 1.4  2010-05-26 15:13:35  kdavlin
 * Added Enegery Module.
 *
 * Revision 1.3  2010-05-22 22:07:32  kdavlin
 * Added new task - MTR_ANSI_CMD_Applet
 *
 * Revision 1.2  2010-05-11 21:20:24  rxu
 * Flop_Hmc(with Manoj code) at 5-11-2010
 * kad Created 080707
 *
 *****************************************************************************/

#ifndef hmc_tsk_lst_H
#define hmc_tsk_lst_H

/* INCLUDE FILES */

#include "psem.h"
#include "hmc_snapshot.h"
#include "hmc_diags.h"
#include "string.h"
#include "hmc_app.h"
#include "hmc_start.h"
#include "hmc_finish.h"
#include "hmc_mtr_info.h"
#include "hmc_request.h"
#include "hmc_time.h"
#include "hmc_sync.h"
#include "hmc_ds.h"
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
#include "hmc_display.h"
#endif
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
#include "hmc_prg_mtr.h"
#endif
#if ( DEMAND_IN_METER == 1 )
#include "hmc_demand.h"
#endif

/* TYPE DEFINITIONS */

typedef uint8_t( *FunctionPtr )( uint8_t, void *);

/* CONSTANT DEFINITIONS */

#ifdef HMC_TSK_GLOBAL
   #define GLOBAL
#else
   #define GLOBAL extern
#endif

GLOBAL const FunctionPtr HMC_APP_MeterFunctions[]
#ifdef HMC_TSK_GLOBAL
=
{
   HMC_FINISH_LogOff,   /* Must be 1st applet - Logs off of the host meter. */
   HMC_STRT_LogOn,      /* Must be 2nd applet - Logs on to the host meter. */
   HMC_MTRINFO_app,     /* Reads all of the general information from the host meter. */
#if ( CLOCK_IN_METER == 1 )
   HMC_TIME_Set_Time,   /* Sets the time in the host meter if it is out of sync with the system time. */
#endif
#if ( DEMAND_IN_METER == 1 )
   HMC_DMD_Reset,       /* Handle demand reset in the meter */
#endif
#if ( CLOCK_IN_METER == 1 )
   HMC_SYNC_applet,     /* If the meter time lags behind sys time, wait up to x time for it to catch up */
#endif
#if ( HMC_SNAPSHOT == 1 )
   HMC_SS_applet,       /* Perform a snapshot - KV2 only. */
#endif
#if ( LOG_IN_METER == 1 )
   HMC_DIAGS_DoDiags,   /* Reads diagnostics from the host meter. */
#endif
#if ( REMOTE_DISCONNECT == 1 )
   HMC_DS_Applet,       /* Disconnect switch applet   */
#endif
   HMC_REQ_applet,      /* Generic read of a table */
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
   HMC_DISP_applet,     /* Write to Display (MT119 Table) applet */
#endif
   (FunctionPtr)NULL
}
#endif
;

/* Used to size the table above. */
GLOBAL const uint16_t HMC_APP_NumOfApplets
#ifdef HMC_TSK_GLOBAL
= (uint16_t)(ARRAY_IDX_CNT(HMC_APP_MeterFunctions))
#endif
;


/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

#endif
