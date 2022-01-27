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
#include "PHY_Protocol.h"

#ifndef HAL_TARGET_HARDWARE
#error "Invalid HAL_TARGET_HARDWARE setting"
#endif

#if( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
#ifndef TEST_900MHZ
#error "TEST_900MHz not defined"
#endif

#if ( TEST_900MHZ == 1 )
#include "radio_config_normal_TCXO_29_491_900MHz.h"
#else
#include "radio_config_normal_TCXO_29_491.h"
#endif
#else
#include "radio_config_normal.h"
#endif

/*****************************************************************************
 *  Local Macros & Definitions
 *****************************************************************************/

/*****************************************************************************
 *  Global Variables
 *****************************************************************************/
static const uint8_t Radio_Configuration_Data_Array[] = RADIO_CONFIGURATION_DATA_ARRAY;

const tRadioConfiguration RadioConfiguration_normal = RADIO_CONFIGURATION_DATA;

