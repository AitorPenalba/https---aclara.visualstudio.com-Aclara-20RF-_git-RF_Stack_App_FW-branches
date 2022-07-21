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
#if ( MCU_SELECTED == RA6E1 )
#include "DBG_SerialDebug.h"

/* ****************************************************************************************************************** */
/* #DEFINE DEFINITIONS */

#define AGT_EVENT_INVALID     255
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ******************************************************************************************************************* */

/* CONSTANTS */


/* ******************************************************************************************************************* */

/* FILE VARIABLE DEFINITIONS */

static volatile uint8_t agt_event_ = AGT_EVENT_INVALID;

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
fsp_err_t AGT_LPM_Timer_Init( void )
{
   fsp_err_t err = FSP_SUCCESS;

   err = R_AGT_Open(&agt1_timer_cascade_lpm_trigger_ctrl, &agt1_timer_cascade_lpm_trigger_cfg);

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

   agt_event_ = AGT_EVENT_INVALID; // Set it to invalid value
   /* Start AGT1 timer */
   err = R_AGT_Start(&agt1_timer_cascade_lpm_trigger_ctrl);
   if(err != FSP_SUCCESS)
   {
      printf("Error:AGT1");
   }

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

	agt_status.state = TIMER_STATE_STOPPED; //Reset status
	err =  R_AGT_StatusGet(&agt1_timer_cascade_lpm_trigger_ctrl, &agt_status);
	if(FSP_SUCCESS == err)
	{
		if(agt_status.state)
		{
			/* Stop Timer */
			err = R_AGT_Stop(&agt1_timer_cascade_lpm_trigger_ctrl);
         agt_event_ = AGT_EVENT_INVALID; // Set it to invalid value
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
 * @param[IN]   uint32_t period in milliseconds
 * @retval      FSP_SUCCESS
 * @retval      Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t AGT_LPM_Timer_Configure( uint32_t period )
{
   fsp_err_t      err = FSP_SUCCESS;
   uint32_t       timer_freq_hz;
   timer_info_t   info;

   err = AGT_LPM_Timer_Init();
   assert(FSP_SUCCESS == err);
   err = AGT_LPM_Timer_Stop();
   assert(FSP_SUCCESS == err);

   /* Get the source clock frequency (in Hz). There are several ways to do this in FSP:
   *  - If LOCO or sub-clock is chosen in agt_extended_cfg_t::clock_source
   *      - The source clock frequency is BSP_LOCO_HZ >> timer_cfg_t::source_div
   *  - If PCLKB is chosen in agt_extended_cfg_t::clock_source and the PCLKB frequency has not changed since reset,
   *      - The source clock frequency is BSP_STARTUP_PCLKB_HZ >> timer_cfg_t::source_div
   *  - Use the R_AGT_InfoGet function (it accounts for the clock source and divider).
   *  - Calculate the current PCLKB frequency using R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKB) and right shift
   *    by timer_cfg_t::source_div.
   *
   * This example uses the last option (R_FSP_SystemClockHzGet).
   */
   (void) R_AGT_InfoGet(&agt1_timer_cascade_lpm_trigger_ctrl, &info);

   timer_freq_hz = info.clock_frequency;
#if 0  /* DBG Code */
   printf("Timer:%ld",timer_freq_hz);
   printf("Period:%ld",info.period_counts);
#endif

   /* Calculate the desired period based on the current clock. Note that this calculation could overflow if the
   * desired period is larger than UINT32_MAX / pclkb_freq_hz. A cast to uint64_t is used to prevent this. */
   uint32_t period_counts = (uint32_t) (((uint64_t) timer_freq_hz * period) / 1000);
   /* Set the calculated period. This will return an error if parameter checking is enabled and the calculated
   * period is larger than UINT16_MAX. */
   err = R_AGT_PeriodSet(&agt1_timer_cascade_lpm_trigger_ctrl, period_counts);

   assert(FSP_SUCCESS == err);
   agt_event_ = AGT_EVENT_INVALID; // Set it to invalid value

#if 0  /* DBG Code */
   (void) R_AGT_InfoGet(&agt1_timer_cascade_lpm_trigger_ctrl, &info);
   printf("Period:%ld",info.period_counts);
#endif
   return err;
}

/*******************************************************************************************************************//**
 * @brief       Callback function called when the AGT1 Timer expires
 * @param[OUT]   timer_callback_args_t * p_args
 * @retval      None
 **********************************************************************************************************************/
void agt1_timer_callback(timer_callback_args_t * p_args)
{
   agt_event_ = p_args->event;
}

/*******************************************************************************************************************//**
 * @brief       A wait function waits till the AGT1 timer expires
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
void AGT_LPM_Timer_Wait( void )
{
   while( TIMER_EVENT_CYCLE_END != (timer_event_t)agt_event_ )
   {   }

   (void)AGT_LPM_Timer_Stop();
}
#endif // #if ( MCU_SELECTED == RA6E1 )