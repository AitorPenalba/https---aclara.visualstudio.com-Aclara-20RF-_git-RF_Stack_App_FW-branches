/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: APP_MSG_HandlerTable.h
 *
 * Contents: Functions pointers for each resource
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2014 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ msingla Created Nov 26, 2014
 *
 **********************************************************************************************************************/
#ifndef APP_MSG_HandlerTable_H_
#define APP_MSG_HandlerTable_H_

/* INCLUDE FILES */
#include "project.h"
#include "meter.h"
#include "OR_MR_Handler.h"
#include "RG_CF_Handler.h"
#include "RG_MD_Handler.h"
#include "ALRM_Handler.h"
#include "HEEP_util.h"
#include "hmc_ds.h"
#include "demand.h"
#include "historyd.h"
#include "ID_intervalTask.h"
#include "dfw_interface.h"
#include "eng_res.h"

/* GLOBAL DEFINTION */

#ifdef APP_MSG_GLOBALS
#define APP_MSG_EXTERN
#else
#define APP_MSG_EXTERN extern
#endif
#if (ACLARA_LC == 1)
#include "ilc_tunneling_handler.h"
#endif

#if (PHASE_DETECTION == 1)
#include "PhaseDetect.h"
#endif

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

/* CONSTANTS */


//void PD_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length);

#ifdef APP_MSG_GLOBALS
static const APP_MSG_HandlerInfo_t APP_MSG_HandlerTable[] =
{
   /*   Resource Type       Function Handler        Valid Ports       */
   /*                                               Ser, Ami, Rsvd    */
#if ( ACLARA_LC != 1 ) && (ACLARA_DA != 1)                         /* meter specific code */
   {   (uint8_t)or_mr,      OR_MR_MsgHandler,       {1, 1, 0} },   /* On-request readings */
   {   (uint8_t)or_ds,      HD_MsgHandler,          {1, 1, 0} },   /* On-request daily-shift readings */
   {   (uint8_t)or_lp,      ID_MsgHandler,          {1, 1, 0} },   /* On-request load-profile readings */
#elif ( ENABLE_SRFN_ILC_TASKS == 1 )
   {   (uint8_t)tn_tw_mp,   ILC_TNL_MsgHandler,     {1, 1, 0} },   /* Tunnel/TWACS/manufacturing port */
#endif
   {   (uint8_t)rg_cf,      RG_CF_MsgHandler,       {1, 1, 0} },   /* New Endpoint Registration Confirmation */
   {   (uint8_t)rg_md,      RG_MD_MsgHandler,       {1, 1, 0} },   /* New Endpoint Registration Metadata */
#if ( REMOTE_DISCONNECT != 0 )                                     /* meter specific code */
   {   (uint8_t)rc,         HMC_DS_ExecuteSD,       {1, 1, 0} },   /* Remote Disconnect */
#endif
   {   (uint8_t)bu_en,      eng_res_MsgHandler,     {1, 1, 0} },   /* On-request engineering data */
   {   (uint8_t)or_pm,      OR_PM_MsgHandler,       {1, 1, 0} },   /* On-request parameter reading/writing */
#if ENABLE_DFW_TASKS
   {   (uint8_t)df_ap,      DFWI_Apply_MsgHandler,  {1, 1, 0} },   /* DFW Apply */
   {   (uint8_t)df_dp,      DFWI_Packet_MsgHandler, {1, 1, 0} },   /* DFW Download Packet */
   {   (uint8_t)df_in,      DFWI_Init_MsgHandler,   {1, 1, 0} },   /* DFW Init */
   {   (uint8_t)df_vr,      DFWI_Verify_MsgHandler, {1, 1, 0} },   /* DFW Verify */
#endif
#if ENABLE_DEMAND_TASKS
   {   (uint8_t)dr,         DEMAND_ResetMsg,        {1, 1, 0} },   /* Demand Reset */
#endif
   {   (uint8_t)or_am,      OR_AM_MsgHandler,       {1, 1, 0} },   /* alarm query by date range */
   {   (uint8_t)or_ha,      OR_HA_MsgHandler,       {1, 1, 0} },   /* alarm query by alarm index range */

#if ( PHASE_DETECTION == 1 )
   {   (uint8_t)pd,         PD_MsgHandler,                 {1, 1, 0} },   /* Phase detect beacon message */
   {   (uint8_t)bu_pd,      PD_HistoricalRecoveryHandler,  {1, 1, 0} },   /* Phase detect historical recovery message */
#endif
   {   0,                   0,                      {0, 0, 0} }
}
#endif
;

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#undef APP_MSG_EXTERN

#endif
