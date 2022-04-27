/***********************************************************************************************************************
 *
 * Filename: mode_config.h
 *
 * Global Designator:  MODECFG_
 *
 * Contents: Configuration parameter I/O for MTU mode.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015 - 2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author David McGraw
 *
 * $Log$
 *
 ***********************************************************************************************************************
 * Revision History:
 *
 * @version
 * #since
 **********************************************************************************************************************/

#ifndef mode_config_H
#define mode_config_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "HEEP_util.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */

#ifdef MODECFG_GLOBALS
#define MODECFG_EXTERN
#else
#define MODECFG_EXTERN extern
#endif


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
typedef enum  {
              e_MODECFG_INIT,                   //DO NOT USE
              e_MODECFG_NORMAL_MODE = 0,
              e_MODECFG_DECOMMISSION_MODE = 11, //define decommsion mode levels as #11-20
              e_MODECFG_SHIP_MODE = 21,         //define shipmode levels as #21-30
              e_MODECFG_QUIET_MODE = 100,       //enums > 100 are not customer facing
              e_MODECFG_RF_TEST_MODE = 111

}modeConfig_e;


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

MODECFG_EXTERN returnStatus_t MODECFG_init(void);

MODECFG_EXTERN uint8_t                  MODECFG_get_ship_mode(void);
MODECFG_EXTERN returnStatus_t           MODECFG_set_ship_mode(uint8_t uShipMode);
MODECFG_EXTERN uint8_t                  MODECFG_get_quiet_mode(void);
MODECFG_EXTERN returnStatus_t           MODECFG_set_quiet_mode(uint8_t uQuietMode);
MODECFG_EXTERN uint8_t                  MODECFG_get_rfTest_mode(void);
MODECFG_EXTERN returnStatus_t           MODECFG_set_rfTest_mode(uint8_t urfTestMode);
MODECFG_EXTERN modeConfig_e             MODECFG_get_initialCFGMode( void );
#if ( EP == 1 )
MODECFG_EXTERN uint8_t                  MODECFG_get_decommission_mode(void);
MODECFG_EXTERN returnStatus_t           MODECFG_set_decommission_mode(uint8_t uDecommisionMode);
MODECFG_EXTERN returnStatus_t           MODECFG_decommissionModeHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
#endif
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#endif // #ifndef mode_config_H
