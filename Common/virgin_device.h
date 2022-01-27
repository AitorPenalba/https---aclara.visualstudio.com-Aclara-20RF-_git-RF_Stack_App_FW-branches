/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: virgin_device.h
 *
 * Contents: Check for first time power up
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Manoj Singla
 *
 * $Log$ Created Feb 6, 2013
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - MS 02/06/2013 - Initial Release
 *
 * @version    0.1
 * #since      2013-02-06
 **********************************************************************************************************************/
#ifndef VIRGIN_DEVICE_H_
#define VIRGIN_DEVICE_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef VDEV_GLOBALS
   #define VDEV_EXTERN
#else
   #define VDEV_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define SIGNATURE_SIZE ((uint8_t)7)     //Size of virgin signature

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

VDEV_EXTERN const uint8_t SIGNATURE[]
#ifdef VDEV_GLOBALS
 = "Aclara"
#endif
;

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

VDEV_EXTERN returnStatus_t VDEV_init(void);
bool VDEV_wasVirgin(void);

/* ****************************************************************************************************************** */
#undef VDEF_EXTERN

#endif
