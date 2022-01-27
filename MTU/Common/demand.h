/******************************************************************************
 *
 * Filename: demand.h
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2015 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

#ifndef demand_H
#define demand_H

/* INCLUDE FILES */
#include "project.h"
#include "HEEP_util.h"
#include "intf_cim_cmd.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */
#define DEM_ERROR_CODE_0               ((uint8_t)0)  /* Success */
#define DEM_OP90_NO_ERROR              ((uint8_t)0x20)
#define DEM_OP90_INV_TIME              ((uint8_t)0x00)
#define DEMAND_POWER_UP                ((uint8_t)0x01)
#define DEMAND_TIME_SFT_FWD            ((uint8_t)0x02)
#define DEMAND_TIME_SFT_REV            ((uint8_t)0x04)
#define DEMAND_VIRGIN_POWER_UP         ((uint8_t)0x08)

#define DEMAND_ENERGY_LEN              ((uint8_t)3)       /* Size in bytes of the max demand quantities */
#define DAILY_DEMAND_ENERGY_LEN        ((uint8_t)3)       /* Size in bytes of the max daily demand quantities */

#define REG_DEMAND_BILLING_PEAK        ((uint16_t)181)        /* 181.0 - Demand Peak*/
#define REG_DEMAND_BILLING_PEAK_SIZE   (DEMAND_ENERGY_LEN)

#define NUM_OF_DELTAS                  ((uint8_t)12)   //Number of deltas used to calcualte demand
#define DMD_MAX_READINGS               ((uint8_t)32)
#define MAX_COIN_PER_DMD_RDG           ((uint8_t)2)   /* Max number of coincident readings per demand reading */

/* TYPE DEFINITIONS */
typedef struct
{
   uint32_t energy;          //Peak energy value for demand
   uint32_t dtReset;         //Date and time of the reset/shift, in UTC seconds
   uint64_t dateTime;        //Date and Time of the peak, in milli-seconds since epoch
}peak_t;                     //Peak energy information

typedef enum
{
   DEMAND_PEAK_READING = ((uint8_t)1),
   DEMAND_PEAK_SHIFT_READING,
   DEMAND_DAILY_PEAK_READING,
   DEMAND_DAILY_SHIFT_READING
}eDMD_READING_TYPE_t;


/* FUNCTION PROTOTYPES */
void           DEMAND_getConfig( uint64_t *demandConfig );   //Returns the current demand configuration
returnStatus_t DEMAND_init( void );                         //Called before the register module is initialized
void           DEMAND_task(uint32_t Arg0);                //Demand task
enum_CIM_QualityCode DEMAND_Current_Daily_Peak( peak_t *ppeak );
enum_CIM_QualityCode DEMAND_Shift_Daily_Peak( peak_t *ppeak );  //Called when time to shift daily demand
enum_CIM_QualityCode DEMAND_GetDmdValue( meterReadingType RdgType, ValueInfo_t *readingInfo );
#if ( DEMAND_IN_METER == 1 )
void           DEMAND_ResetInMeter(void);                       //Called when demand handled is in meter
#else // DEMAND_IN_METER = 0
void           DEMAND_ResetInModule(void);                      //Called when demand handled is in module
#endif
void           DEMAND_ResetMsg(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length);
uint8_t        DEMAND_configValid(uint8_t demandConfig);
uint8_t        DEMAND_GetConfig(void);
uint8_t        DEMAND_GetFutureConfig(void);
void           DEMAND_GetResetDay( uint8_t *dateOfMonth);
#if ( DEMAND_IN_METER == 0 )
returnStatus_t DEMAND_SetConfig(uint8_t config);
returnStatus_t DEMAND_SetFutureConfig(uint8_t futureConfig);
#endif
returnStatus_t DEMAND_SetResetDay( uint8_t dateOfMonth );
void           DEMAND_GetSchDmdRst( uint32_t *date, uint32_t *time );
void           DEMAND_DumpFile(void);                  //Prints demand file/variables
void           DEMAND_GetTimeout( uint32_t *timeout ); //Gets the Demand Reset Lockout time
returnStatus_t DEMAND_SetTimeout( uint32_t timeout );  //Sets the Demand Reset Lockout time
bool           DEMAND_isModuleBusy( void );
void           DEMAND_getReadingTypes(uint16_t* pReadingType);
#if ( DEMAND_IN_METER == 0 )
void           DEMAND_clearDemand(void);
#endif
void           DEMAND_setReadingTypes(uint16_t* pReadingType);
returnStatus_t DEMAND_DrReadListHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t DEMAND_demandFutureConfigurationHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t DEMAND_demandPresentConfigurationHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t DEMAND_demandResetLockoutPeriodHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );

#endif	/* this must be the last line of the file */
