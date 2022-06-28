/**********************************************************************************************************************

   Filename: lpm_app.c

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
 /* INCLUDE FILES */
#include "project.h"
#include "DBG_SerialDebug.h"
#include "lpm_app.h"

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

static void StopModules ( void );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/*********************************************************************************************************************

   Function name: LPM_APP_mode_enter

   Purpose: This function enables and puts the MCU in sleep mode.

   Arguments: app_lpm_states_t

   Returns: FSP_SUCCESS - Upon successful entering sleep mode

   Notes:

**********************************************************************************************************************/
fsp_err_t LPM_APP_mode_enter( app_lpm_states_t lpm_mode )
{
   fsp_err_t            err                           = FSP_SUCCESS;
   lpm_instance_ctrl_t  g_lpm_ctrl_instance_ctrls[]   = { g_lpm_SW_Standby_ctrl,
                                                          g_lpm_DeepSWStandby_ctrl };
   lpm_cfg_t			   g_lpm_ctrl_instance_cfgs[]    = { g_lpm_SW_Standby_cfg,
                                                          g_lpm_DeepSWStandby_cfg };

   err = R_LPM_Open( &g_lpm_ctrl_instance_ctrls[lpm_mode], &g_lpm_ctrl_instance_cfgs[lpm_mode] );

   /* Handle error */
   if (FSP_SUCCESS == err)
   {
      switch( lpm_mode )
      {
         case APP_LPM_SW_STANDBY_STATE:
         {
            /* Enter SW Standby mode */
            err = R_LPM_LowPowerModeEnter(&g_lpm_ctrl_instance_ctrls[lpm_mode]);

            /* Configure system clocks. */
#if ( LAST_GASP_RECONFIGURE_CLK == 1 )
            bsp_clock_init();
#endif
            break;
         }
         case APP_LPM_DEEP_SW_STANDBY_STATE:
         {
            StopModules(); // TODO: RA6E1: DG: Do we need to stop for SW Standby?
            /* Enter Deep SW standby mode */
            err = R_LPM_LowPowerModeEnter(&g_lpm_ctrl_instance_ctrls[lpm_mode]);
            break;
            /*
            * Note that MCU will reset after exited from Deep SW Standby (internal reset state)
            */
         }
         default:
         {        /* return error */
            err = FSP_ERR_INVALID_MODE;
            break;
         }
      }
   }
   return err;
}

/*********************************************************************************************************************

   Function name: StopModules

   Purpose: This function disables/Stops the Modules to help reduce the unnecessary power drain

   Arguments: void

   Returns:

   Notes: This is RA6E1 Specific

**********************************************************************************************************************/

static void StopModules ( void )
{
   /* Module Stop Control Register A */
//   R_BSP_MODULE_STOP( FSP_IP_SRAM, 0 );
//   R_BSP_MODULE_STOP( FSP_IP_SRAM, 0 );
   R_BSP_MODULE_STOP( FSP_IP_DMAC, 0 );

   /* Module Stop Control Register B */
   R_BSP_MODULE_STOP( FSP_IP_CAN, 0 );
   R_BSP_MODULE_STOP( FSP_IP_QSPI, 0 ); // QSPI
   R_BSP_MODULE_STOP( FSP_IP_IIC, 1 );
   R_BSP_MODULE_STOP( FSP_IP_IIC, 0 );
   R_BSP_MODULE_STOP( FSP_IP_USBFS, 0 );

   R_BSP_MODULE_STOP( FSP_IP_SPI, 1 );
   R_BSP_MODULE_STOP( FSP_IP_SPI, 0 );
   R_BSP_MODULE_STOP( FSP_IP_SCI, 9 );  // Used - ??

   R_BSP_MODULE_STOP( FSP_IP_SCI, 4 );  // DBG Serial
   R_BSP_MODULE_STOP( FSP_IP_SCI, 3 );  // Used - ??
   R_BSP_MODULE_STOP( FSP_IP_SCI, 2 );
   R_BSP_MODULE_STOP( FSP_IP_SCI, 1 );
   R_BSP_MODULE_STOP( FSP_IP_SCI, 0 );

   /* Module Stop Control Register C */
   R_BSP_MODULE_STOP( FSP_IP_CAC, 0 );
   R_BSP_MODULE_STOP( FSP_IP_CRC, 0 );
   R_BSP_MODULE_STOP( FSP_IP_SSI, 0 );
//   R_BSP_MODULE_STOP( FSP_IP_SDHI, 0 );
   R_BSP_MODULE_STOP( FSP_IP_DOC, 0 );
   R_BSP_MODULE_STOP( FSP_IP_ELC, 0 );
   R_BSP_MODULE_STOP( FSP_IP_SCE, 9 );

   /* Module Stop Control Register D */
   R_BSP_MODULE_STOP( FSP_IP_AGT, 3 );
   R_BSP_MODULE_STOP( FSP_IP_AGT, 2 );
   R_BSP_MODULE_STOP( FSP_IP_AGT, 1 );  // Need for SW Standby Mode
   R_BSP_MODULE_STOP( FSP_IP_POEG, 0 );  // FSP_IP_POEGD
   R_BSP_MODULE_STOP( FSP_IP_POEG, 0 );  // C
   R_BSP_MODULE_STOP( FSP_IP_POEG, 0 );  // B
   R_BSP_MODULE_STOP( FSP_IP_POEG, 0 );  // A
   R_BSP_MODULE_STOP( FSP_IP_ADC, 0 );
   R_BSP_MODULE_STOP( FSP_IP_DAC, 0 );   // TODO: RA6E1: Review this for Uplink Power Control

   /* Module Stop Control Register E */
   R_BSP_MODULE_STOP( FSP_IP_AGT, 5 );
   R_BSP_MODULE_STOP( FSP_IP_AGT, 4 );
   R_BSP_MODULE_STOP( FSP_IP_GPT, 7 );
   R_BSP_MODULE_STOP( FSP_IP_GPT, 6 );
   R_BSP_MODULE_STOP( FSP_IP_GPT, 5 );
   R_BSP_MODULE_STOP( FSP_IP_GPT, 4 );
   R_BSP_MODULE_STOP( FSP_IP_GPT, 2 );
   R_BSP_MODULE_STOP( FSP_IP_GPT, 1 );

}
