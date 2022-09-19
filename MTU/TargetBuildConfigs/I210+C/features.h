/******************************************************************************
 *
 * Filename: features.h
 *
 * Contents: Defines the options/features for the I-210+C
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2022 Aclara. All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 *****************************************************************************************************************/

#ifndef features_H
#define features_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#ifdef __BOOTLOADER
#undef __BOOTLOADER
#endif

#define HMC_I210_PLUS_C                   1  /* Target host meter is GE i210+c */
#define MAX_COIN_PER_DEMAND               2  /* Maximum number of coincident values per demand */
#define TOU_TIER_E_SUPPORT                0  /* Only Tiers A-D supported   */
#define DAILY_TOU_TIER_C_D_SUPPORT        0  /* Only Tiers A-B supported   */
#define ED_PROGRAMMER_NAME_LENTGH         10 /* Aclara Meters Manufacturing table field */

#define COMMERCIAL_METER                  2  /* Not a commercial meter, but can have more than one phase voltage */
#define CLOCK_IN_METER                    1
#define LP_IN_METER                       1
#define DEMAND_IN_METER                   1
#define COINCIDENT_SUPPORT                0

#if ( LP_IN_METER == 0 )
#define ID_MAX_CHANNELS                   (6)
#else
#define ID_MAX_CHANNELS                   (8)
#endif

#define REMOTE_DISCONNECT                 1
#define HMC_SNAPSHOT                      1
#define ANSI_STANDARD_TABLES              1
#define ANSI_LOGON                        1
#define ANSI_SECURITY                     1
#define METER_TROUBLE_SIGNAL              1
#define END_DEVICE_NEGOTIATION            0
#define LOG_IN_METER                      1
#define OPTICAL_PASS_THROUGH              0
#define SAMPLE_METER_TEMPERATURE          0  /* Is sampling the meter's thermometer required, some meter's do this on their own */
#define LOAD_CONTROL_DEVICE               1  /* 1 -> supports load control and signed multicast messages.   */
#define END_DEVICE_PROGRAMMING_CONFIG     1  /* Supports configuration of the meter by programming/writing tables */
#define END_DEVICE_PROGRAMMING_FLASH      ED_PROG_FLASH_PATCH_ONLY  /* Supports programming of the meter Flash */
#define END_DEVICE_PROGRAMMING_DISPLAY    1  /* Supports the ability to write to the meter display */
#define VSWR_MEASUREMENT                  0  /* Supports VSWR measurements of antenna */
#define PHASE_DETECTION                   1  /* Supports Phase Detection */
#define LAST_GASP_SIMULATION              1  /* Supports Last Gasp Simulation Feature */ // TODO: RA6E1: Enable and verify this feature
#define MAC_LINK_PARAMETERS               0  /* Supports Decoding MAC Link Parameters Messages */
#define MAC_CMD_RESP_TIME_DIVERSITY       0  /* Not an EP feature */
#define TX_THROTTLE_CONTROL               0  /* 0=No Tx Throttling, 1=Tx Throttling */
#define BM_USE_KERNEL_AWARE_DEBUGGING     0  /* This helps to see the name of the Buffer pool while debugging */
#define GET_TEMPERATURE_FROM_RADIO        1  /* When GET_TEMPERATURE_FROM_RADIO set as 1 the radio temperature calculated using the RA6E1.

                                               While using the RA6E1 there are two methods for calculate the radio temperature.
                                               1 - RADIO_Temperature_Update :   Only called from SoftDemodulator.c to update
                                                                                RADIO_Temperature using the average calculation.
                                               2 - RADIO_Get_Chip_Temperature : Either get a radio temperature if Soft Demod is not active
                                                                                or to return the current average temperature in static variable RADIO_Temperature
                                                                                if Soft Demod is active.

                                               When GET_TEMPERATURE_FROM_RADIO set as 0 the radio temperature calculated using the K24.
                                               While using the K24 one method for calculate the radio temperature.
                                               1 - RADIO_Temperature_Get : This function will only be used in the K24 implementation.
                                                                           It will therefore call ADC_Get_uP_Temperature if the soft demodulator is active*/

#define DAC_CODE_CONFIG                   1  /* Supports DAC0 for TX Power Control */
#define LAST_GASP_RECONFIGURE_CLK         0  /* If enabled, the System Clock will be re-configured to use MOCO instead of Main Clock */
#define LAST_GASP_USE_2_DEEP_SLEEP        1  /* If enabled, will use the different configurations of the Deep SW Standby Modes in Last Gasp to achieve the random sleep delay */
#if( RTOS_SELECTION == FREE_RTOS )
#define GENERATE_RUN_TIME_STATS           1  /* Enable the FreeRTOS feature to generate the Run-Time Stats. For this to work, among other things
                                                   configGENERATE_RUN_TIME_STATS should be enabled in FreeRTOSConfig.h. Please refer FreeRTOS document for more info. */
#else
#define GENERATE_RUN_TIME_STATS           0
#endif
#endif
#define LG_UPDATE_RADIO_MODE              1 /* Change the Radio mode to Standby during CSMA back-off time in LastGasp. This is in development not verified yet */

