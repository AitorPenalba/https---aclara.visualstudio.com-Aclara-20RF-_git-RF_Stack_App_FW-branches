/**********************************************************************************************************************

  Filename: GPT.c

  Global Designator: GPT_

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
 /* INCLUDE FILES */

#include "project.h"
#if ( MCU_SELECTED == RA6E1 )

/* ****************************************************************************************************************** */
/* #DEFINE DEFINITIONS */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ******************************************************************************************************************* */

/* CONSTANTS */


/* ******************************************************************************************************************* */

/* FILE VARIABLE DEFINITIONS */


/* ******************************************************************************************************************* */
/* FUNCTION PROTOTYPES */


/* ****************************************************************************************************************** */

/* FUNCTION DEFINITIONS */

/* ADD */
returnStatus_t GPT_PD_Init( void )
{
   fsp_err_t err = FSP_SUCCESS;

   /* Initializes the module. */
   err = R_GPT_Open(&GPT2_ZCD_Meter_ctrl, &GPT2_ZCD_Meter_cfg);

   /* Handle any errors. This function should be defined by the user. */
   assert(FSP_SUCCESS == err);

   return (returnStatus_t)err;
}

/* ADD */

void GPT_PD_Enable( void )
{
   /* Enable captures. Captured values arrive in the interrupt. */
   (void) R_GPT_Enable(&GPT2_ZCD_Meter_ctrl);
   (void) R_GPT_Start(&GPT2_ZCD_Meter_ctrl);
}

/* ADD */
void GPT_PD_Disable( void )
{
   /* Disables captures. */
   (void) R_GPT_Disable(&GPT2_ZCD_Meter_ctrl);
}


/* ADD */
returnStatus_t GPT_Radio0_Init( void )
{
   fsp_err_t err = FSP_SUCCESS;
//   R_BSP_MODULE_START(FSP_IP_POEG, 0U);
   /* Initializes the module. */
   err = R_GPT_Open(&GPT1_Radio0_ICapture_ctrl, &GPT1_Radio0_ICapture_cfg);

   /* Handle any errors. This function should be defined by the user. */
   assert(FSP_SUCCESS == err);

   return (returnStatus_t)err;
}

/* ADD */

void GPT_Radio0_Enable( void )
{
   /* Enable captures. Captured values arrive in the interrupt. */
   (void) R_GPT_Enable(&GPT1_Radio0_ICapture_ctrl);
   (void) R_GPT_Start(&GPT1_Radio0_ICapture_ctrl);
}

/* ADD */
void GPT_Radio0_Disable( void )
{
   /* Disables captures. */
   (void) R_GPT_Disable(&GPT1_Radio0_ICapture_ctrl);
}

#endif