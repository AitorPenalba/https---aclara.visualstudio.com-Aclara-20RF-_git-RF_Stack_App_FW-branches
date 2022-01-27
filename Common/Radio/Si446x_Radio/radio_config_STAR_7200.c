/*! @file radio.c
 * @brief This file contains functions to interface with the radio chip.
 *
 * @b COPYRIGHT
 * @n Silicon Laboratories Confidential
 * @n Copyright 2012 Silicon Laboratories, Inc.
 * @n http://www.silabs.com
 */

#include "project.h"
#include "xradio.h"

#ifndef HAL_TARGET_HARDWARE
#error "Invalid HAL_TARGET_HARDWARE setting"
#endif

#if( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
#include "radio_config_STAR_7200_TCXO_29_491.h"
#else
#include "radio_config_STAR_7200.h"
#endif

/*****************************************************************************
 *  Local Macros & Definitions
 *****************************************************************************/

/*****************************************************************************
 *  Global Variables
 *****************************************************************************/
static const uint8_t Radio_Configuration_Data_Array[] = RADIO_CONFIGURATION_DATA_ARRAY;

const tRadioConfiguration RadioConfiguration_STAR_7200 = RADIO_CONFIGURATION_DATA;

