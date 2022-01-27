/***********************************************************************************************************************
 *
 * Filename: dfwtd_config.h
 *
 * Global Designator:  DFWTDCFG_
 *
 * Contents: Configuration parameter I/O for DFW time diversity.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
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

#ifndef dfwtd_config_H
#define dfwtd_config_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "HEEP_util.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */

#ifdef DFWTDCFG_GLOBALS
   #define DFWTDCFG_EXTERN
#else
   #define DFWTDCFG_EXTERN extern
#endif


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
typedef struct
{
   uint16_t fileVersion;
   uint8_t uDfwApplyConfirmTimeDiversity;       //In minuntes
   uint8_t uDfwDownloadConfirmTimeDiversity;    //In minutes
} dfwTdConfigFileData_t;


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

DFWTDCFG_EXTERN returnStatus_t DFWTDCFG_init(void);
DFWTDCFG_EXTERN uint8_t DFWTDCFG_get_apply_confirm(void);
DFWTDCFG_EXTERN uint8_t DFWTDCFG_get_download_confirm(void);

DFWTDCFG_EXTERN returnStatus_t DFWTDCFG_set_apply_confirm(uint8_t uDfwApplyConfirmTimeDiversity);
DFWTDCFG_EXTERN returnStatus_t DFWTDCFG_set_download_confirm(uint8_t uDfwDownloadConfirmTimeDiversity);
DFWTDCFG_EXTERN returnStatus_t DFWTDCFG_GetTimeDiversity(dfwTdConfigFileData_t *diversitySettings);
DFWTDCFG_EXTERN returnStatus_t DFWTDCFG_SetTimeDiversity(dfwTdConfigFileData_t *diversitySettings);
DFWTDCFG_EXTERN returnStatus_t DFWTDCFG_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#endif // #ifndef dfwtd_config_H
