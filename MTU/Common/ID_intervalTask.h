/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: interval.h
 *
 * Contents: #defs, typedefs, and function prototypes for interval data
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013-2015 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Karl Davlin
 *
 * $Log$ Created September 25, 2013
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - 09/25/2013 - Initial Release
 *
 * @version    0.1
 * #since      2013-09-25
 **********************************************************************************************************************/
#ifndef interval_H_
#define interval_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "time_util.h"
#include "MAC_Protocol.h"
#include "APP_MSG_Handler.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   ID_MODE_ABS_e = ((uint8_t)0),      /* Absolute values.  Copies the source register, scales it and stores it */
   ID_MODE_DELTA_e                    /* Delta Mode.  Takes the delta of the scaled values and stores it */
}mode_e;                              /* Mode of operation */

typedef enum
{
   ID_GOOD_DATA_QC_e = ((uint8_t)0),
   ID_OVERFLOW_QC_e,
   ID_SKIPPED_INTERVAL_QC_e,
   ID_TEST_DATA_QC_e,
   ID_CONFIGURATION_CHANGED_QC_e,
   ID_ESTIMATED_DATA_QC_e,
   ID_LONG_INTERVAL_QC_e,
   ID_PARTIAL_INTERVAL_QC_e
}ID_QC_e;

typedef union
{
   struct
   {
      uint8_t SignificantTimeBias:     1;          /* Time bias threshold exceeded during this interval */
      uint8_t clockChanged:            1;          /* Clock change during this interval */
      uint8_t powerFail:               1;          /* Outage during this interval */
      uint8_t ServiceDisconnectSwitch: 1;          /* Service disconnect switch operated during this interval */
      uint8_t qualCode:                4;          /* Quality code enumeration */
   }flags;
   uint8_t byte;                            /* used to copy/clear flags */
}qualCodes_u;                             /* Quality codes supported by interval data */

typedef struct
{
   uint32_t sampleRate_mS;         /* Sample rate in milli-seconds */
   uint8_t storageLen;             /* Storage length allocated for the data (size of deltas) */
   uint8_t trimDigits;             /* Number of digts to be trimmed. scalar = pow(10, trimDigits) */
   mode_e mode;                    /* Delta or Absolute mode. */
   uint16_t rdgTypeID;             /* Source Reading Type ID */
}idChCfg_t;                        /* Interval Data configuration */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
uint16_t ID_getLpBuSchedule( void );
uint8_t ID_getQos(void);
returnStatus_t ID_setLpBuSchedule(uint16_t);
returnStatus_t ID_setQos(uint8_t);
uint8_t ID_getLpBuDataRedundancy(void);
returnStatus_t ID_setLpBuDataRedundancy(uint8_t lpBuDataRedundancy);
uint8_t ID_getLpBuMaxTimeDiversity(void);
returnStatus_t ID_setLpBuMaxTimeDiversity(uint8_t lpBuMaxTimeDiversity);
void ID_MsgHandler(HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length);
bool ID_isModuleBusy( void );
#if ( LP_IN_METER == 0 )
void ID_clearAllIdChannels( void );
returnStatus_t ID_lpSamplePeriod( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t ID_lpBuChannel1Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t ID_lpBuChannel2Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t ID_lpBuChannel3Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t ID_lpBuChannel4Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t ID_lpBuChannel5Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t ID_lpBuChannel6Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
#if ( ID_MAX_CHANNELS > 6 )
returnStatus_t ID_lpBuChannel7Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t ID_lpBuChannel8Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
#endif
#endif

returnStatus_t ID_LpBuDataRedundancyHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t ID_LpBuMaxTimeDiversityHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t ID_LpBuScheduleHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t ID_LpBuTrafficClassHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
bool ID_getLpBuEnabled(void);
returnStatus_t ID_setLpBuEnabled(uint8_t uLpBuEnabled);
returnStatus_t ID_LpBuEnabledHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
/**
 * ID_init - Initializes the interval data module.  Only needs to be called once after the NV memory is initialized.
 * @see Interval data design document
 *
 * @param  None
 * @return returnStatus_t
 */
returnStatus_t ID_init( void );

/**
 * ID_task - This is the interval data task.
 * @see Interval data design document
 *
 * @param  uint32_t Arg0
 * @return None
 */
#ifdef __mqx_h__
void ID_task( uint32_t Arg0 );
#endif /* __mqx_h__ */

/**
 * ID_cfgIntervalData - API to configure interval data
 * @see Interval data design document
 *
 * @param  uint8_t ch - Channel to configure
 * @param  idChCfg_t *pIdCfg - Location to get the configuration
 * @return returnStatus_t
 */
#if ( LP_IN_METER == 0 )
returnStatus_t ID_cfgIntervalData(uint8_t ch, idChCfg_t const *pIdCfg);
#endif

/**
 * ID_rdIntervalData - API to read one sample of interval data
 * @see Interval data design document
 *
 * @param  uint8_t *pQual - Location to store the quality codes
 * @param  double *pVal - Location to store the resulting sample value.
 * @param  uint8_t ch - Channel of the data
 * @param  sysTime_t *pReadingTime - Time of the reading (for example: send 2:00 to get the 1:00 to 2:00 reading)
 * @return returnStatus_t
 */
#if ( LP_IN_METER == 0 )
returnStatus_t ID_rdIntervalData(uint8_t *pQual, double *pVal, uint8_t ch, sysTime_t const *pReadingTime);
#endif

/**
 * ID_rdIntervalDataCfg - Read interval data configuration
 * @see Interval data design document
 *
 * @param  idChCfg_t *pIdCfg - Location to store the configuration
 * @param  uint8_t ch - Channel of the data
 * @return returnStatus_t
 */
#if ( LP_IN_METER == 0 )
returnStatus_t ID_rdIntervalDataCfg(idChCfg_t *pIdCfg, uint8_t ch);
#endif

#endif
