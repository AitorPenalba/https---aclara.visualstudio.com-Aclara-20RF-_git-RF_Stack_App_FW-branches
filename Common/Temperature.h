/**********************************************************************************************************************
 *
 * Filename:   Temperature.h
 *
 **********************************************************************************************************************
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
 *********************************************************************************************************************
 *
 * @author Dhruv Gaonkar
 *
 ****************************************************************************************************************** */

/* INCLUDE FILES */
#include "HEEP_util.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
#if( EP == 1 )
typedef enum
{
   eNORMAL_TEMP = (uint8_t) 0,    /* Normal Working Radio Temperature */
   eHIGH_TEMP_THRESHOLD,          /* High Temperature threshold at which Alarm is sent */
   //eCLEAR_HIGH_TEMP_EVENT,        /* Clear the High Temperature Threshold Reached Event */
   eLOW_TEMP_THRESHOLD,           /* Low Temperature threshold at which Alarm is sent */
   //eCLEAR_LOW_TEMP_EVENT,         /* Clear the Low Temperature Threshold Reached Event */

}TemperatureStatus_t;
#endif
/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
#if( EP == 1 )
returnStatus_t       TEMPERATURE_init( void );
TemperatureStatus_t  TEMPERATURE_getEpTemperatureStatus( void );
uint8_t              TEMPERATURE_getEpMaxTemperature( void );
returnStatus_t       TEMPERATURE_setEpMaxTemperature( uint8_t maxTemp );
int8_t               TEMPERATURE_getEpMinTemperature( void );
returnStatus_t       TEMPERATURE_setEpMinTemperature( int8_t minTemp );
uint8_t              TEMPERATURE_getEpTempHysteresis( void );
returnStatus_t       TEMPERATURE_setEpTempHysteresis( uint8_t hysteresis );
uint8_t              TEMPERATURE_getHighTempThreshold( void );
returnStatus_t       TEMPERATURE_setHighTempThreshold( uint8_t hightempThreshold );
int8_t               TEMPERATURE_getEpTempMinThreshold( void );
returnStatus_t       TEMPERATURE_setEpTempMinThreshold( int8_t lowTempThreshold );
#endif /* end of if( EP == 1 )*/
returnStatus_t       TEMPERATURE_OR_PM_Handler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr);

/* ****************************************************************************************************************** */
