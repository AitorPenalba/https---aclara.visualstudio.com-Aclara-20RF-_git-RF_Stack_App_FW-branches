/**********************************************************************************************************************

   Filename: lpm_app.h

   Global Designator: LPM_APP_

   Contents:

 **********************************************************************************************************************
  A product of
  Aclara Technologies LLC
  Confidential and Proprietary
  Copyright 2022 Aclara. All Rights Reserved.

  PROPRIETARY NOTICE
  The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
  (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
  authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
  be used or copied only in accordance with the terms of such license.
 *********************************************************************************************************************

   @author Dhruv Gaonkar

 ****************************************************************************************************************** */

#ifndef LPM_APP_H_
#define LPM_APP_H_

/* INCLUDE FILES */

/* ****************************************************************************************************************** */

/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */

/* TYPE DEFINITIONS */

/*
 * Low Power Mode Definitions for LPM ep
 * Since there are no Normal mode definition in LPM driver, use this enum to keep LPM app state including:
 *  Deep SW Standby,SW Standby, Normal.
 */
typedef enum e_app_lpm_state
{
   APP_LPM_SW_STANDBY_STATE = 0,       // SW Standby mode
   APP_LPM_DEEP_SW_STANDBY_STATE,      // Deep SW Standby mode
   APP_LPM_DEEP_SW_STANDBY_STATE_AGT,  // Deep SW Standby mode with AGT enabled
   APP_LPM_NORMAL_STATE                // Normal mode
} app_lpm_states_t;


/* ****************************************************************************************************************** */

/* CONSTANTS */

/* ****************************************************************************************************************** */

/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */


/* FUNCTION PROTOTYPES */

fsp_err_t LPM_APP_mode_enter	   ( app_lpm_states_t lpm_mode );

#endif /* LPM_APP_H_ */
