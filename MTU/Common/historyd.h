/******************************************************************************
 *
 * Filename: historyd.h
 *
 * Contents: Historical data
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ msingla Created Feb 10, 2015
 *
 **********************************************************************************************************************/

#ifndef HISTORYD_H
#define HISTORYD_H

/* INCLUDE FILES */
#include "project.h"
#include "OS_aclara.h"
#include "buffer.h"
#include "MAC_Protocol.h"
#include "APP_MSG_Handler.h"
#include "HEEP_util.h"

#ifdef HD_GLOBALS
#define HD_EXTERN
#else
#define HD_EXTERN extern
#endif

/* CONSTANTS */

/* MACRO DEFINITIONS */
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_A ) || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_B ) || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84020_1_REV_A ) || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A   )       )
#define HD_TOTAL_CHANNELS      32  //Number of channels for daily shift for I-210+ and +c meters
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84030_1_REV_A ) // TODO: KV2C-RA6E1 Bob: Makes sure this works for kV2c
#define HD_TOTAL_CHANNELS      64  //Number of channels for daily shift for kV2c meters
#else
#undef HD_TOTAL_CHANNELS // For the non-metering products, there should not be any daily shift data
#endif

/* TYPE DEFINITIONS */

/* Application layer header fields */

/* FILE VARIABLE DEFINITIONS */

/* GLOBAL FUNCTION PROTOTYPES */

HD_EXTERN returnStatus_t HD_init ( void );
HD_EXTERN void HD_MsgHandler(HEEP_APPHDR_t *heepHdr, void *payloadBuf, uint16_t length);
HD_EXTERN void HD_DailyShiftTask ( taskParameter );
HD_EXTERN uint8_t HD_getDsBuDataRedundancy(void);
HD_EXTERN returnStatus_t HD_setDsBuDataRedundancy(uint8_t dsBuDataRedundancy);
HD_EXTERN uint8_t HD_getDsBuMaxTimeDiversity(void);
HD_EXTERN returnStatus_t HD_setDsBuMaxTimeDiversity(uint8_t dsBuMaxTimeDiversity);
HD_EXTERN uint8_t HD_getDsBuTrafficClass(void);
HD_EXTERN returnStatus_t HD_setDsBuTrafficClass(uint8_t dsBuTrafficClass);
HD_EXTERN uint32_t HD_getDailySelfReadTime(void);
HD_EXTERN returnStatus_t HD_setDailySelfReadTime(uint32_t dailySelfReadTime);
HD_EXTERN void HD_getDsBuReadingTypes(uint16_t* pDsBuReadingType);
HD_EXTERN void HD_setDsBuReadingTypes(uint16_t const * pDsBuReadingType);
HD_EXTERN void HD_clearHistoricalData( void );
HD_EXTERN bool HD_getDsBuEnabled(void);
HD_EXTERN returnStatus_t HD_setDsBuEnabled(uint8_t dsBuMaxTimeDiversity);
HD_EXTERN bool HD_getHistoricalRecovery(void);

HD_EXTERN returnStatus_t HD_dailySelfReadTimeHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
HD_EXTERN returnStatus_t HD_dsBuDataRedundancyHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
HD_EXTERN returnStatus_t HD_dsBuMaxTimeDiversityHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
HD_EXTERN returnStatus_t HD_dsBuTrafficClassHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
HD_EXTERN returnStatus_t HD_dsBuReadingTypesHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
HD_EXTERN returnStatus_t HD_dsBuEnabledHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
HD_EXTERN returnStatus_t HD_historicalRecoveryHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
#undef HD_EXTERN

#endif

