/***********************************************************************************************************************
 *
 * Filename: smtd_config.h
 *
 * Global Designator:  SMTDCFG_
 *
 * Contents: Configuration parameter I/O for stack manager time diversity.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2019 Aclara.  All Rights Reserved.
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

#ifndef smtd_config_H
#define smtd_config_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "HEEP_util.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */

#ifdef SMTDCFG_GLOBALS
   #define SMTDCFG_EXTERN
#else
   #define SMTDCFG_EXTERN extern
#endif


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

SMTDCFG_EXTERN returnStatus_t SMTDCFG_init(void);
#if ( EP == 1 )
SMTDCFG_EXTERN uint8_t        SMTDCFG_get_log(void);
SMTDCFG_EXTERN returnStatus_t SMTDCFG_set_log(uint8_t uSmLogTimeDiversity);
#endif
SMTDCFG_EXTERN bool           SMTDCFG_getEngBuEnabled(void);
SMTDCFG_EXTERN returnStatus_t SMTDCFG_setEngBuEnabled(uint8_t uEngBuEnabled);
SMTDCFG_EXTERN uint8_t        SMTDCFG_getEngBuTrafficClass(void);
SMTDCFG_EXTERN returnStatus_t SMTDCFG_setEngBuTrafficClass(uint8_t uEngBuTrafficClass);
SMTDCFG_EXTERN returnStatus_t SMTDCFG_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#endif // #ifndef smtd_config_H
