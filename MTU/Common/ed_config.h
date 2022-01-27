/***********************************************************************************************************************
 *
 * Filename: ed_config.h
 *
 * Global Designator:  EDCFG_
 *
 * Contents: Configuration parameter I/O for end device information.
 *
 ***********************************************************************************************************************
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
 * @author David McGraw
 *
 * $Log$
 *
 ***********************************************************************************************************************
 * Revision History:
 *
 * @version
 * #since
 **********************************************************************************************************************/

#ifndef ed_config_H
#define ed_config_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "HEEP_util.h"
#include "intf_cim_cmd.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */

#ifdef EDCFG_GLOBALS
   #define EDCFG_EXTERN
#else
   #define EDCFG_EXTERN extern
#endif


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#if ( HMC_I210_PLUS == 1 ) || ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 )
#define EDCFG_INFO_LENGTH                 20
#elif ( HMC_KV == 1 )
#define EDCFG_INFO_LENGTH                 31
#else  // For SRFNI-210+C
#define EDCFG_INFO_LENGTH                 50
#endif
#define EDCFG_MFG_SERIAL_NUMBER_LENGTH    16
#define EDCFG_UTIL_SERIAL_NUMBER_LENGTH   24
#define EDCFG_FW_VERSION_LENGTH           23  /* Worst case could be "xxx.xxx.xxx;xxx.xxx.xxx" Note: length is w/o NULL */
#define EDCFG_HW_VERSION_LENGTH            7
#define EDCFG_MANUFACTURER_LENGTH          4
#define EDCFG_MODEL_LENGTH                 8
#define EDCFG_PROGRAM_ID_LENGTH            2
#define EDCFG_PROGRAM_DATETIME_LENGTH      4
#define EDCFG_PROGRAMMER_NAME_LENGTH      10
#define EDCFG_ANSI_TBL_OID_LENGTH          4

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

EDCFG_EXTERN returnStatus_t EDCFG_init(void);

returnStatus_t EDCFG_get_info(char* pStringBuffer, uint8_t uBufferSize);
EDCFG_EXTERN char* EDCFG_get_mfg_serial_number(char* pStringBuffer, uint8_t uBufferSize);
EDCFG_EXTERN char* EDCFG_get_util_serial_number(char* pStringBuffer, uint8_t uBufferSize);

enum_CIM_QualityCode EDCFG_getAvgIndPowerFactor( ValueInfo_t *readingInfo );

#if ( ( HMC_KV == 1 ) || ( HMC_I210_PLUS == 1 ) || ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 ) )
EDCFG_EXTERN returnStatus_t EDCFG_set_info(char* pString);
#endif
EDCFG_EXTERN returnStatus_t EDCFG_OR_PM_Handler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr);
#if( SAMPLE_METER_TEMPERATURE == 1 )
EDCFG_EXTERN returnStatus_t EDCFG_setEdTempSampleRate(uint16_t uAmBuMaxTimeDiversity);
EDCFG_EXTERN uint16_t EDCFG_getEdTempSampleRate(void);
EDCFG_EXTERN returnStatus_t EDCFG_setEdTemperatureHystersis(uint8_t uAmBuMaxTimeDiversity);
EDCFG_EXTERN uint8_t EDCFG_getEdTemperatureHystersis(void);
#endif

#if HMC_I210_PLUS || ACLARA_LC || ACLARA_DA
EDCFG_EXTERN returnStatus_t EDCFG_set_mfg_serial_number(char* pString);
#endif
EDCFG_EXTERN returnStatus_t EDCFG_set_util_serial_number(char const * pString);

enum_CIM_QualityCode EDCFG_getFwVerNum( ValueInfo_t *readingInfo );
enum_CIM_QualityCode EDCFG_getHwVerNum( ValueInfo_t *readingInfo );
enum_CIM_QualityCode EDCFG_getEdProgrammedDateTime( ValueInfo_t *readingInfo );
enum_CIM_QualityCode EDCFG_getMeterDateTime( ValueInfo_t *readingInfo );
enum_CIM_QualityCode EDCFG_getTHDCount( ValueInfo_t *readingInfo );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#endif // #ifndef ed_config_H
