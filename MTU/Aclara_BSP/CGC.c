/**********************************************************************************************************************

  Filename: CGC.c

  Global Designator: CGC_

  Contents: Clock Configuration abstraction layer for RA6E1

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

/*********************************************************************************************************************

   Function name: CGC_Stop_Unused_Clocks

   Purpose: This function disables unused clocks that are automatically enabled by default.

   Arguments: void

   Returns: FSP_SUCCESS - Upon successful set up the clocks

   Notes:

**********************************************************************************************************************/
fsp_err_t CGC_Stop_Unused_Clocks( void )
{
   fsp_err_t                err;
   cgc_clock_t       		 sys_clock_source = CGC_CLOCK_PLL;
   cgc_divider_cfg_t 		 sys_divider_cf   = {0};
	agt_extended_cfg_t const *p_agt1_extend	= AGT1_LPM_Wakeup_cfg.p_extend;

   /* Open CGC module */
   err = R_CGC_Open(&g_cgc0_ctrl, &g_cgc0_cfg);
   /* Handle error */
   if(FSP_SUCCESS == err)
   {
      /*
      * Start Sub clock, which is used as clock source for RTC
      */
      err = R_CGC_ClockStart(&g_cgc0_ctrl, CGC_CLOCK_SUBCLOCK, NULL);
      if(FSP_SUCCESS == err)
      {
         /* Get system clock source */
         err = R_CGC_SystemClockGet (&g_cgc0_ctrl, &sys_clock_source, &sys_divider_cf);
         /* Handle error */
         if(FSP_SUCCESS == err)
         {
            /* Disable unused clock */
            switch(sys_clock_source)
            {
               case CGC_CLOCK_PLL:
               case CGC_CLOCK_MAIN_OSC:
               {
                  /*
                  * LOCO should be handled properly if using them as count sources for RTC, AGT timers
                  * e.g. avoid stop if it's used as RTC counter source
                  */
                  /* Stop MOCO clock if it's unused */
                  err = R_CGC_ClockStop(&g_cgc0_ctrl, CGC_CLOCK_MOCO);
                  if(FSP_SUCCESS == err)
                  {
                     /* Stop LOCO clock if LOCO is unused (not use as AGT1 count source)*/
                     if(AGT_CLOCK_LOCO != p_agt1_extend->count_source)
                     {
                        err = R_CGC_ClockStop(&g_cgc0_ctrl, CGC_CLOCK_LOCO);
                     }
                     err = R_CGC_ClockStop(&g_cgc0_ctrl, CGC_CLOCK_HOCO);
                  }
                  break;
               }
               case CGC_CLOCK_HOCO:
               {
                  /* Stop MOCO clock if it's unused */
                  err = R_CGC_ClockStop(&g_cgc0_ctrl, CGC_CLOCK_MOCO);
                  if(FSP_SUCCESS == err)
                  {
                     /* Stop LOCO clock if LOCO is unused (not use as AGT1 count source)*/
                     if(AGT_CLOCK_LOCO != p_agt1_extend->count_source)
                     {
                        err = R_CGC_ClockStop(&g_cgc0_ctrl, CGC_CLOCK_LOCO);
                     }
                  }
                  break;
               }
               case CGC_CLOCK_MOCO:
               {
                  /* Stop LOCO clock if it's unused (not use as AGT1 count source)*/
                  if(AGT_CLOCK_LOCO != p_agt1_extend->count_source)
                  {
                     err = R_CGC_ClockStop(&g_cgc0_ctrl, CGC_CLOCK_LOCO);
                  }
                  break;
               }
               case CGC_CLOCK_LOCO:
               {
                  /* Stop MOCO clock if it's unused */
                  err = R_CGC_ClockStop(&g_cgc0_ctrl, CGC_CLOCK_MOCO);
                  break;
               }
               case CGC_CLOCK_SUBCLOCK:
               {
                  /* Stop MOCO clock if it's unused */
                  err = R_CGC_ClockStop(&g_cgc0_ctrl, CGC_CLOCK_MOCO);
                  if(FSP_SUCCESS == err)
                  {
                     /* Stop LOCO clock if LOCO is unused (not use as AGT1 count source)*/
                     if(AGT_CLOCK_LOCO != p_agt1_extend->count_source)
                     {
                        err = R_CGC_ClockStop(&g_cgc0_ctrl, CGC_CLOCK_LOCO);
                     }
                  }
                  break;
               }
               default:
               {
                  /* return error */
                  err = FSP_ERR_CLOCK_INACTIVE;
                  break;
               }
            }//switch sys_clock_source
         }//if FSP_SUCCESS == err (R_CGC_SystemClockGet)
      }//if FSP_SUCCESS == err (Sub clock)
   }//if FSP_SUCCESS == err

   return err;
}

/*********************************************************************************************************************

   Function name: CGC_Switch_SystemClock_to_MOCO

   Purpose: This function configures the MOCO, switches the System Clock from Main OSC to MOCO and Stops the Main OSC.
            This is used to reduce the current draw in Low Power Modes.

   Arguments: void

   Returns: void

   Notes:

**********************************************************************************************************************/
void CGC_Switch_SystemClock_to_MOCO( void )
{
   fsp_err_t         err;
   cgc_clocks_cfg_t  clocks_cfg;

   clocks_cfg.system_clock          = CGC_CLOCK_MOCO;
   clocks_cfg.divider_cfg.iclk_div  = CGC_SYS_CLOCK_DIV_2;
   clocks_cfg.divider_cfg.pclka_div = CGC_SYS_CLOCK_DIV_4;
   clocks_cfg.divider_cfg.pclkb_div = CGC_SYS_CLOCK_DIV_4;
   clocks_cfg.divider_cfg.pclkc_div = CGC_SYS_CLOCK_DIV_4;
   clocks_cfg.divider_cfg.pclkd_div = CGC_SYS_CLOCK_DIV_4;
   clocks_cfg.divider_cfg.bclk_div  = CGC_SYS_CLOCK_DIV_4;
   clocks_cfg.divider_cfg.fclk_div  = CGC_SYS_CLOCK_DIV_2;
   clocks_cfg.mainosc_state         = CGC_CLOCK_CHANGE_STOP;
   clocks_cfg.hoco_state            = CGC_CLOCK_CHANGE_STOP;
   clocks_cfg.moco_state            = CGC_CLOCK_CHANGE_START;
   clocks_cfg.loco_state            = CGC_CLOCK_CHANGE_STOP;
   clocks_cfg.pll_state             = CGC_CLOCK_CHANGE_STOP;
   clocks_cfg.pll_cfg.source_clock  = CGC_CLOCK_MOCO;      // unused
   clocks_cfg.pll_cfg.multiplier    = CGC_PLL_MUL_10_0;    // unused
   clocks_cfg.pll_cfg.divider       = CGC_PLL_DIV_2;       // unused
   clocks_cfg.pll2_state            = CGC_CLOCK_CHANGE_STOP;

   R_BSP_SoftwareDelay( BSP_FEATURE_CGC_MOCO_STABILIZATION_MAX_US, BSP_DELAY_UNITS_MICROSECONDS );
   /* Change the Clock Source */
   /* Assuming the system clock is Main CLock, switch to MOCO. */
   err = R_CGC_ClocksCfg(&g_cgc0_ctrl, &clocks_cfg);
   assert(FSP_SUCCESS == err);

   /* NOTE: Not sure why this magic delay is needed. But this reduces the current to ~2.3mA without this delay its ~5mA*/
   R_BSP_SoftwareDelay( 200, BSP_DELAY_UNITS_MILLISECONDS );
}


#endif // #if ( MCU_SELECTED == RA6E1 )