/***********************************************************************************************************************
 *
 * Filename: cfg_hal_def.h
 *
 * Contents: Hardware Abstraction Layer Definitions
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2011-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$
 *
 **********************************************************************************************************************/
#ifndef CFG_HAL_DEFS_H
#define CFG_HAL_DEFS_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* GLOBAL DEFINITIONS */

/* Define all supported target hardware here. */
#define HAL_TARGET_Y84001_REV_A        1  /* Using Y84001-1-SCH Rev A Board (I210+, KV2c K22)   */
#define HAL_TARGET_Y84020_1_REV_A     10  /* Using Y84020-1-SCH Rev A Board (I210+ K24)         */
#define HAL_TARGET_Y84030_1_REV_A     15  /* Using Y84030-1-SCH Rev A Board (KV2c K24)          */
#define HAL_TARGET_Y84580_x_REV_A     20  /* Using Y84580-1 or Y84580-2 Assembly (I210+c RA6E1) */

/* It was decided to create a gap between metering end-points and ILC end-points */
#define HAL_TARGET_Y99852_1_REV_A     300 /* Using Y99852-1-SCH Rev A board (ILC) */

// This is the NIC board for SRFN DA.  The hardware acts like a network card to allow devices to talk on the SRFN network.
#define HAL_TARGET_Y84114_1_REV_A 500

// It was decided to create a gap between end-point and DCU
#define HAL_TARGET_Y84050_1_REV_A     1000   /* Using Y84050-1-SCH Rev A Board ( should be 9975_XCVR    */
#define HAL_TARGET_XCVR_9985_REV_A    1010   /* DCU 3 Transceiver Board with the 2MBx16 NOR and 2MBx16 SRAM.*/

/* Define all supported processors */
#define NXP_K24                         1
#define RA6E1                           2

/* Define all supported OSes */
#define BARE_METAL                      0
#define MQX_RTOS                        1
#define FREE_RTOS                       2

#endif
/* End File:            cfg_hal_def.h   */
