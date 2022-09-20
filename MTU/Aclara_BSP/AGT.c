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

static volatile uint8_t agt_event_     = AGT_EVENT_INVALID;
static bool             agt_lpm_open_  = (bool)false;

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
   if( !agt_lpm_open_ )
   {
      err = R_AGT_Open(&AGT1_LPM_Wakeup_ctrl, &AGT1_LPM_Wakeup_cfg);
   }

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
   fsp_err_t err;

   agt_event_ = AGT_EVENT_INVALID; // Set it to invalid value
   /* Start AGT1 timer */
   err = R_AGT_Start(&AGT1_LPM_Wakeup_ctrl);
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
	fsp_err_t err;
   timer_status_t agt_status = {0};

	agt_status.state = TIMER_STATE_STOPPED; //Reset status
	err =  R_AGT_StatusGet(&AGT1_LPM_Wakeup_ctrl, &agt_status);
	if(FSP_SUCCESS == err)
	{
		if(agt_status.state)
		{
			/* Stop Timer */
			err = R_AGT_Stop(&AGT1_LPM_Wakeup_ctrl);
         agt_event_ = AGT_EVENT_INVALID; // Set it to invalid value
			if(FSP_SUCCESS == err)
			{
				/* Reset counter */
				err = R_AGT_Reset(&AGT1_LPM_Wakeup_ctrl);
			}
		}
	}

	return err;
}

/*******************************************************************************************************************//**
 * @brief       This function configures AGT1 timer
 * @param[IN]   uint32_t period in milliseconds
 * @retval      FSP_SUCCESS
 * @retval      Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t AGT_LPM_Timer_Configure( uint32_t period )
{
   fsp_err_t      err;
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
   (void) R_AGT_InfoGet(&AGT1_LPM_Wakeup_ctrl, &info);

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
   err = R_AGT_PeriodSet(&AGT1_LPM_Wakeup_ctrl, period_counts);

   assert(FSP_SUCCESS == err);
   agt_event_ = AGT_EVENT_INVALID; // Set it to invalid value

#if 0  /* DBG Code */
   (void) R_AGT_InfoGet(&AGT1_LPM_Wakeup_ctrl, &info);
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

 /*******************************************************************************************************************//**
 * @brief       This function opens AGT2 module
 * @param[IN]   None
 * @retval      eSUCCESS                  Upon successful open of AGT modules
 * @retval      eFAILURE                  Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
returnStatus_t AGT_FreqSyncTimerInit( void )
{
   fsp_err_t      err;
   returnStatus_t rtnVal = eFAILURE;

   /* Open AGT2 Timer in Periodic mode */
	err = R_AGT_Open(&agt2_Freq_Sync_ctrl, &agt2_Freq_Sync_cfg);
	if(FSP_SUCCESS == err)
	{
      rtnVal = eSUCCESS;
	}

	return rtnVal;
}

/*******************************************************************************************************************//**
 * @brief       This function starts AGT2 module
 * @param[IN]   None
 * @retval      eSUCCESS                  Upon successful start of AGT modules
 * @retval      eFAILURE                  Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
returnStatus_t AGT_FreqSyncTimerStart(void)
{
   fsp_err_t      err;
   returnStatus_t rtnVal = eFAILURE;

	/* Start AGT2 timer */
	err = R_AGT_Start(&agt2_Freq_Sync_ctrl);
	if(FSP_SUCCESS == err)
	{
      rtnVal = eSUCCESS;
	}

	return rtnVal;
}

/*******************************************************************************************************************//**
 * @brief       This function stops AGT2 Timer
 * @param[IN]   None
 * @retval      eSUCCESS                  Upon successful open/start of AGT modules
 * @retval      eFAILURE                  Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
returnStatus_t AGT_FreqSyncTimerStop(void)
{
	fsp_err_t      err;
   returnStatus_t rtnVal     = eFAILURE;
   timer_status_t agt_status = {0};

	/* Stop AGT timer if running */
	err = R_AGT_StatusGet(&agt2_Freq_Sync_ctrl, &agt_status);
	if(FSP_SUCCESS == err)
	{
		if(agt_status.state)
		{
			/* Stop Timer */
			err = R_AGT_Stop(&agt2_Freq_Sync_ctrl);
			if(FSP_SUCCESS == err)
			{
				/* Reset counter */
				err = R_AGT_Reset(&agt2_Freq_Sync_ctrl);
			}
		}
	}
   (void)R_AGT_Close(&agt2_Freq_Sync_ctrl);
	if(FSP_SUCCESS == err)
	{
      rtnVal = eSUCCESS;
	}

	return rtnVal;
}

/*******************************************************************************************************************//**
 * @brief       This Reads the current AGT2 Timer count
 * @param[IN]   None
 * @retval      eSUCCESS                  Upon successful open/start of AGT modules
 * @retval      eFAILURE                  Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
returnStatus_t AGT_FreqSyncTimerCount(uint16_t *count)
{
	fsp_err_t      err;
   returnStatus_t rtnVal     = eFAILURE;
   timer_status_t agt_status = {0};

	/* Stop AGT timer if running */
	err = R_AGT_StatusGet(&agt2_Freq_Sync_ctrl, &agt_status);
   *count = (uint16_t)agt_status.counter;
	if(FSP_SUCCESS == err)
	{
      rtnVal = eSUCCESS;
	}

	return rtnVal;
}

#if ( GENERATE_RUN_TIME_STATS == 1 )

 /*******************************************************************************************************************//**
 * @brief       This function opens AGT4 and AGT5 modules in cascade
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
static void AGT_RunTimeStatsInit( void )
{
   fsp_err_t      err;

   /* Open AGT3 Timer in Periodic mode */
	err = R_AGT_Open(&AGT4_RunTimeStats_0_ctrl, &AGT4_RunTimeStats_0_cfg);
   if( FSP_SUCCESS == err )
   {
      err = R_AGT_Open(&AGT5_RunTimeStats_1_ctrl, &AGT5_RunTimeStats_1_cfg);
   }
   assert(FSP_SUCCESS == err);
}

/*******************************************************************************************************************//**
 * @brief       This function starts AGT4 and AGT5 modules
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
void AGT_RunTimeStatsStart(void)
{
   fsp_err_t      err;

   AGT_RunTimeStatsInit();
	/* Start AGT4 timer */
	err = R_AGT_Start(&AGT4_RunTimeStats_0_ctrl);
   if( FSP_SUCCESS == err )
   {
      /* Start AGT5 timer */
      err = R_AGT_Start(&AGT5_RunTimeStats_1_ctrl);
   }
   assert(FSP_SUCCESS == err);
}

/*******************************************************************************************************************//**
 * @brief       This function stops AGT4 and AGT5 Timers
 * @param[IN]   None
 * @retval      returnStatus_t
 **********************************************************************************************************************/
returnStatus_t AGT_RunTimeStatsStop(void)
{
	fsp_err_t      err;
   timer_status_t agt_status = {0};

	/* Stop AGT timer if running */
	err = R_AGT_StatusGet(&AGT4_RunTimeStats_0_ctrl, &agt_status);
	if(FSP_SUCCESS == err)
	{
		if(agt_status.state)
		{
			/* Stop Timer */
			err = R_AGT_Stop(&AGT4_RunTimeStats_0_ctrl);
			if(FSP_SUCCESS == err)
			{
				/* Reset counter */
				err = R_AGT_Reset(&AGT4_RunTimeStats_0_ctrl);
			}

         /* Stop Timer */
			err = R_AGT_Stop(&AGT5_RunTimeStats_1_ctrl);
			if(FSP_SUCCESS == err)
			{
				/* Reset counter */
				err = R_AGT_Reset(&AGT5_RunTimeStats_1_ctrl);
			}
		}
	}
   (void)R_AGT_Close(&AGT4_RunTimeStats_0_ctrl);
   (void)R_AGT_Close(&AGT5_RunTimeStats_1_ctrl);
   assert(FSP_SUCCESS == err);
   return eSUCCESS;
}

/*******************************************************************************************************************//**
 * @brief       This Reads the current AGT4 and AGT5 Timer counter value
 * @param[IN]   None
 * @retval      uint32_t counter Value
 **********************************************************************************************************************/
uint32_t AGT_RunTimeStatsCount( void )
{
   timer_status_t    agt_status5 = {0};
   timer_status_t    agt_status4 = {0};
   static uint32_t   counterVal = 0;

	/* Stop AGT timer if running */
   (void)R_AGT_Stop(&AGT4_RunTimeStats_0_ctrl);
   (void)R_AGT_Stop(&AGT5_RunTimeStats_1_ctrl);

   ( void )R_AGT_StatusGet(&AGT5_RunTimeStats_1_ctrl, &agt_status5);
   ( void )R_AGT_StatusGet(&AGT4_RunTimeStats_0_ctrl, &agt_status4);
   counterVal = agt_status5.counter;

   counterVal = (65536 - counterVal) << 16;
   counterVal |= (uint16_t)(65536 - (uint16_t)agt_status4.counter);

   R_AGT_Start(&AGT4_RunTimeStats_0_ctrl);
   R_AGT_Start(&AGT5_RunTimeStats_1_ctrl);

	return ( counterVal );
}

#endif // (GENERATE_RUN_TIME_STATS == 1)

#endif // #if ( MCU_SELECTED == RA6E1 )