/*! @file radio.c
 * @brief This file contains functions to interface with the radio chip.
 *
 * @b COPYRIGHT
 * @n Silicon Laboratories Confidential
 * @n Copyright 2012 Silicon Laboratories, Inc.
 * @n http://www.silabs.com
 */

#include "xradio.h"
#include "radio_config_normal_dev_800_IF_8_23_TCXO_29_491.h"
#warning "radio_config_normal_dev_800_IF_8_23_TCXO_29_491.h"

/*****************************************************************************
 *  Local Macros & Definitions
 *****************************************************************************/

/*****************************************************************************
 *  Global Variables
 *****************************************************************************/
static const uint8_t Radio_Configuration_Data_Array[] = RADIO_CONFIGURATION_DATA_ARRAY;

const tRadioConfiguration RadioConfiguration_normal_dev_800_IF_8_23_TCXO_29_491 = RADIO_CONFIGURATION_DATA;

