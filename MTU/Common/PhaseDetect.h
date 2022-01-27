/***********************************************************************************************************************
 *
 * Filename:    PhaseDetect.h
 *
 * Global Designator: PD_
 *
 * Contents: Phase Detection
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2020 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 *****************************************************************************/

#ifndef PHASE_DETECT_H
#define PHASE_DETECT_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "MAC.h"

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* Size of the LCD Message */
#define PD_LCD_MSG_SIZE 6

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

returnStatus_t PD_init ( void );
uint32_t       PD_DebugCommandLine ( uint32_t argc, char* const argv[] );
returnStatus_t PD_DebugCommand(TIMESTAMP_t t, const uint8_t src_addr[5]);
void           PD_SyncDetected(TIMESTAMP_t syncTime, uint32_t syncTimeCYCCNT);

/* Get/Set Functions */
uint8_t  PD_SurveyCapable_Get(void);
uint8_t  PD_SurveySelfAssessment_Get(void);
uint32_t PD_SurveyStartTime_Get(void)  ;
uint32_t PD_SurveyPeriod_Get(void)  ;
uint8_t  PD_SurveyPeriodQty_Get(void);
uint8_t  PD_SurveyQuantity_Get(void);
uint8_t  PD_SurveyMode_Get(void);
int32_t  PD_ZCProductNullingOffset_Get(void);
uint16_t PD_LcdMsg_Get(char* result);
uint16_t PD_BuMaxTimeDiversity_Get(void);
uint8_t  PD_BuDataRedundancy_Get(void);

returnStatus_t PD_SurveyStartTime_Set(uint32_t);
returnStatus_t PD_SurveyPeriod_Set(uint32_t);
returnStatus_t PD_SurveyPeriodQty_Set(uint8_t);
returnStatus_t PD_SurveyQuantity_Set(uint8_t);
returnStatus_t PD_SurveyMode_Set(uint8_t);
returnStatus_t PD_ZCProductNullingOffset_Set(int32_t);
returnStatus_t PD_LcdMsg_Set (uint8_t const * string);
returnStatus_t PD_BuMaxTimeDiversity_Set(uint16_t value);
returnStatus_t PD_BuDataRedundancy_Set(uint8_t value);

void PD_AddSysTimer(void);
bool PD_HandleSysTimer_Event( const tTimeSysMsg *alarmMsg );
void PD_HandleTimeDiversity_Event(void);

returnStatus_t PD_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *pValue, OR_PM_Attr_t *attr );

void PD_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length);
void PD_HistoricalRecoveryHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length);

void PD_SyncTime_Set(TIMESTAMP_t sync_time);

#endif