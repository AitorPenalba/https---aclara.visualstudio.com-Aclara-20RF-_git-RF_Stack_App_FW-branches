/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: hal.h
 *
 * Contents: SRFN Target Hardware Definition
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2016-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
#ifndef HAL_H
#define HAL_H


/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* --------------------------------------------------------------------------------------------------------------------
   No Hardware specific abstractions required for this board
   -------------------------------------------------------------------------------------------------------------------*/

/* ****************************************************************************************************************** */
/* GLOBAL DEFINITIONS */

/* TODO: RA6E1 Bob: When everyone is switched to Rev B equivalent hardware, this can be permanently changed to Rev B  */
/* To build the firmware for the Y84081 Rev A board, use HAL_TARGET_Y84580_x_REV_A */
/* To build the firmware for the Y84081 Rev B board, use HAL_TARGET_Y84580_x_REV_B */
#define HAL_TARGET_HARDWARE     HAL_TARGET_Y84580_x_REV_B
#define MCU_SELECTED            RA6E1
#define RTOS_SELECTION          FREE_RTOS

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#endif
