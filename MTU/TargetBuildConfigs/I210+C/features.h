/******************************************************************************
 *
 * Filename: features.h
 *
 * Contents: Defines the options/features for the XCVR
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2016 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ******************************************************************************
 *
 * $Log$ kad Created 20160324
 *
 *****************************************************************************/

#ifndef feature_H
#define feature_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "meter.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define BARE_METAL                        0
#ifdef __mqx_h__
   #define MQX_RTOS                       1
#endif
#define FREE_RTOS                         2
/* */
#define ACLARA_DVR_ABSTRACTION            0
#if ( ACLARA_DVR_ABSTRACTION != 0 )
   #define FILE_IO                        0
   #define PARTITION_MANAGER              0 /* 0 = No PARTITION_MANAGER, 1 = Parition Manager */
   #define TIMER_UTIL                     0
   #define RTC                            0
#endif
#define SUPPORT_HEEP                      0
#define INCLUDE_ECC                       0
#define RTOS                              FREE_RTOS /* 0 = Bare Metal, 1 = MQX , 2 = FreeRTOS */
#define DBG_TESTS                         0
#define SEGGER_RTT                        0
#define TM_MUTEX                          0
#define TM_SEMAPHORE                      0
#endif
