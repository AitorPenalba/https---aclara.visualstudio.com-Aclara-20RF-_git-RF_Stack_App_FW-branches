/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: hmc_snapshot.h
 *
 * Contents: Sends the snap shot procedure to the host meter.  This is required for KV2 meters.
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
 * @author Karl Davlin
 *
 * $Log$ Created April 4, 2013
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - KAD 04/04/2013 - Initial Release
 *
 * @version    0.1
 * #since      2013-04-04
 **********************************************************************************************************************/
#ifndef hmc_snapshot_H_
#define hmc_snapshot_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef hmc_snapshot_GLOBALS
   #define hmc_snapshot_EXTERN
#else
   #define hmc_snapshot_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   eHMC_SS_DISABLED = (uint8_t)0,           /* Snapshot is disabled */
   eHMC_SS_SNAPPED = eHMC_SS_DISABLED,    /* Snapshot performed for the current session */
   eHMC_SS_NOT_SNAPPED,                   /* Snapshot NOT performed for the current session */
   eHMC_SS_ERROR                          /* There was an error performing a snapshot on the host meter! */
}HMC_SS_Status_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/**
 * HMC_SS_applet - Executes a snap shot procedure.
 * @see KV2RandP.doc Section 6.9.2.24, Procedure 84: Snapshot Data
 *
 * @param  uint8_t cmd
 * @param  void *pData
 * @return uint8_t - See applet definition
 */
uint8_t HMC_SS_applet(uint8_t cmd, void *pData);

/**
 * HMC_SS_isSnapped - Returns that state if the snapshot
 * @see KV2RandP.doc Section 6.9.2.24, Procedure 84: Snapshot Data
 *
 * @param  None
 * @return HMC_SS_Status_t - See definition above
 */
HMC_SS_Status_t HMC_SS_isSnapped(void);

/* ****************************************************************************************************************** */
#undef hmc_snapshot_EXTERN

#endif
