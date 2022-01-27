/***********************************************************************************************************************
 *
 * Filename: time_sync.h
 *
 * Contents: Functions related to time sync between EP's and DCU
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2010-2017 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 *****************************************************************************/

#ifndef TIME_SYNC_H
#define TIME_SYNC_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "HEEP_util.h"

/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */
/* TODO: these should only be defined in one place!   */
#ifndef TIME_SYNC_SOURCE
#define TIME_SYNC_SOURCE           ((uint8_t)0) //Time Sync source only
#endif
#ifndef TIME_SYNC_CONSUMER
#define TIME_SYNC_CONSUMER         ((uint8_t)1) //Time Sync consumer only
#endif
#ifndef TIME_SYNC_BOTH
#define TIME_SYNC_BOTH             ((uint8_t)2) //Time Sync source and consumer
#endif

#define TIME_PRECISION_MIN            ((int8_t)-20)
#define TIME_PRECISION_MAX            ((int8_t)5)

#define LEAP_INDICATOR_NO_WARNING    ((uint8_t)0)
#define LEAP_INDICATOR_INSERT_LEAP   ((uint8_t)1)
#define LEAP_INDICATOR_DELETE_LEAP   ((uint8_t)2)
#define LEAP_INDICATOR_NO_SYNC       ((uint8_t)3)
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
returnStatus_t TIME_SYNC_Init(void);
returnStatus_t TIME_SYNC_TimeDefaultAccuracy_Set(int8_t val);
returnStatus_t TIME_SYNC_TimeLastUpdatedPrecision_Set(int8_t val);
returnStatus_t TIME_SYNC_TimeQueryResponseMode_Set(uint8_t val);
returnStatus_t TIME_SYNC_TimeSetMaxOffset_Set(uint16_t val);
returnStatus_t TIME_SYNC_TimeSetPeriod_Set(uint32_t val);
returnStatus_t TIME_SYNC_TimeSetStart_Set(uint32_t val);
returnStatus_t TIME_SYNC_TimeSource_Set(uint8_t val);
int8_t         TIME_SYNC_TimeLastUpdatedPrecision_Get(void);
int8_t         TIME_SYNC_TimePrecision_Get(void);
uint32_t       TIME_SYNC_TimePrecisionMicroSeconds_Get(void);
int8_t         TIME_SYNC_TimeDefaultAccuracy_Get(void);
uint8_t        TIME_SYNC_TimeQueryResponseMode_Get(void);
uint16_t       TIME_SYNC_TimeSetMaxOffset_Get(void);
uint32_t       TIME_SYNC_TimeSetPeriod_Get(void);
uint32_t       TIME_SYNC_TimeSetStart_Get(void);
uint8_t        TIME_SYNC_TimeSource_Get(void);
void           TIME_SYNC_Parameters_Print(void);
returnStatus_t TIME_SYNC_OR_PM_Handler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr);
void           TIME_SYNC_AddTimeSyncPayload(uint8_t *payload);
uint32_t       TIME_SYNC_LogPrecisionToPrecisionInMicroSec(int8_t precision);
uint8_t        TIME_SYNC_PeriodicAlaramID_Get ( void );

#endif
