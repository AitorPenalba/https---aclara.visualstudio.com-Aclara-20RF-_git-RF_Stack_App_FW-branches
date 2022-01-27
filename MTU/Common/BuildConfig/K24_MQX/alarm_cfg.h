/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: alarm_cfg.h
 *
 * Contents: Contains the alarm IDs
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013 Aclara.  All Rights Reserved.
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
 * $Log$ Created August 19, 2013
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - 08/09/2012 - Initial Release
 *
 * @version    0.1
 * #since      2013-08-09
 **********************************************************************************************************************/
#ifndef alarm_cfg_H_
#define alarm_cfg_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   eALRMID_RSVD = 0,
   eALRMID_MTRDIAG_METERING,
   eALRMID_MTRDIAG_TST_MODE,
   eALRMID_MTRDIAG_MTR_SHOP_MODE,
   eALRMID_MTRDIAG_UNPROGRAMMED,
   eALRMID_MTRDIAG_CFG_ERR,
   eALRMID_MTRDIAG_SLF_CHK_ERR,
   eALRMID_MTRDIAG_RAM_FAILURE,
   eALRMID_MTRDIAG_ROM_FAILURE,
   eALRMID_MTRDIAG_NV_FAILURE,
   eALRMID_MTRDIAG_CLK_ERR,
   eALRMID_MTRDIAG_MEAS_ERR,
   eALRMID_MTRDIAG_LOW_BAT,
   eALRMID_MTRDIAG_LOW_LOSS_POT,
   eALRMID_MTRDIAG_DMD_OVERLOAD,
   eALRMID_MTRDIAG_PWR_FAILURE,
   eALRMID_MTRDIAG_DSP_ERR,
   eALRMID_MTRDIAG_SYS_ERR,
   eALRMID_MTRDIAG_RCVD_KWH,
   eALRMID_MTRDIAG_LEADING_KVARH,
   eALRMID_MTRDIAG_LOSS_OF_PGM,
   eALRMID_MTRDIAG_FLSH_CODE_ERR,
   eALRMID_MTRDIAG_FLSH_DATA_ERR
}alarmId_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */


#endif
