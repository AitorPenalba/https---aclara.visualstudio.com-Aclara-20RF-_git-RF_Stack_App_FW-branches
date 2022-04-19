/**********************************************************************************************************************

  Filename: AGT.c

  Global Designator: AGT_

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
#include "agt.h"


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


 /*******************************************************************************************************************//**
 * @brief       This function opens AGT0 & AGT1 modules
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  Upon successful open of AGT modules
 * @retval      Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t AGT_LPM_Timer_Init(void)
{
   fsp_err_t err = FSP_SUCCESS;
	/* Open AGT0 Timer in Periodic mode */
	err = R_AGT_Open(&agt0_timer_lpm_cascade_trigger_ctrl, &agt0_timer_lpm_cascade_trigger_cfg);
	if(FSP_SUCCESS == err)
	{
		/* Open AGT1 Timer in Periodic mode */
		err = R_AGT_Open(&agt1_timer_cascade_lpm_trigger_ctrl, &agt1_timer_cascade_lpm_trigger_cfg);
	} // TODO: RA6: DG: Do we need to close the agt0 if the agt1 open fails?

   return err;
}

/*******************************************************************************************************************//**
 * @brief       This function starts AGT0 & AGT1 Timer modules
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  Upon successful start of AGT modules
 * @retval      Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t AGT_LPM_Timer_Start(void)
{
   fsp_err_t err = FSP_SUCCESS;

	/* Start AGT0 timer */
	err = R_AGT_Start(&agt0_timer_lpm_cascade_trigger_ctrl);
	if(FSP_SUCCESS == err)
	{
		/* Start AGT1 timer */
		err = R_AGT_Start(&agt1_timer_cascade_lpm_trigger_ctrl);
	} // TODO: RA6: DG: Stop AGT1 if the AGT0 start fails

   return err;
}

/*******************************************************************************************************************//**
 * @brief       This function stops AGT0 & AGT1 Timer
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  Upon successful open/start of AGT modules
 * @retval      Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t AGT_LPM_Timer_Stop(void)
{
	fsp_err_t err = FSP_SUCCESS;
   timer_status_t agt_status = {0};
	/* Stop AGT timers if they are counting */
	err = R_AGT_StatusGet(&agt0_timer_lpm_cascade_trigger_ctrl, &agt_status);
	if(FSP_SUCCESS == err)
	{
		if(agt_status.state)
		{
			/* Stop Timer */
			err = R_AGT_Stop(&agt0_timer_lpm_cascade_trigger_ctrl);
			if(FSP_SUCCESS == err)
			{
				/* Reset counter */
				err = R_AGT_Reset(&agt0_timer_lpm_cascade_trigger_ctrl);
			}
		}
	}
	agt_status.state = 0; //Reset status
	err =  R_AGT_StatusGet(&agt1_timer_cascade_lpm_trigger_ctrl, &agt_status);
	if(FSP_SUCCESS == err)
	{
		if(agt_status.state)
		{
			/* Stop Timer */
			err = R_AGT_Stop(&agt1_timer_cascade_lpm_trigger_ctrl);
			if(FSP_SUCCESS == err)
			{
				/* Reset counter */
				err = R_AGT_Reset(&agt1_timer_cascade_lpm_trigger_ctrl);
			}
		}
	}

	return err;
}

/*******************************************************************************************************************//**
 * @brief       This function configures AGT0 AGT1 timer
 * @param[IN]   None
 * @retval      FSP_SUCCESS
 * @retval      Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t AGT_LPM_Timer_Configure( uint32_t const period )
{
   AGT_LPM_Timer_Init();
   AGT_LPM_Timer_Stop();

   R_AGT_PeriodSet( &agt0_timer_lpm_cascade_trigger_ctrl, period );

   AGT_LPM_Timer_Start();

   return FSP_SUCCESS;
}
