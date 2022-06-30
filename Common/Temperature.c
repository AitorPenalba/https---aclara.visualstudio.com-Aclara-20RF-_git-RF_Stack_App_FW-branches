/**********************************************************************************************************************
 *
 * Filename:   Temperature.c
 *
 * Global Designator: TEMPERATURE_
 *
 * Contents: Routines to handle the Temperature of the Radio
 *
 **********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2020-2022 Aclara.  All Rights Reserved.
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

#include "project.h"
#include "Temperature.h"
#include "ALRM_Handler.h"
#include "radio.h"
#include "radio_hal.h"
#if ( EP == 1 )
#include "file_io.h"
#endif
#if ( VSWR_MEASUREMENT == 1 )
#include "math.h"
#endif
#include "PHY.h"

/* #DEFINE DEFINITIONS */
#if ( VSWR_MEASUREMENT == 1 )
#define VSWR_POWER_TEN ((uint8_t)2) // power of ten adjustment for macCmsaPValue parameter
#endif

#if ( EP == 1 )
#define DEFAULT_EP_MAX_TEMPERATURE     ( (uint8_t)25 )
#define DEFAULT_EP_MIN_TEMPERATURE     ( (int8_t)25 )
#define DEFAULT_EP_TEMP_HYSTERESIS     ( (uint8_t)3 )
#define DEFAULT_HIGH_TEMP_THRESHOLD    ( (int8_t)78 )
#define DEFAULT_EP_TEMP_MIN_THRESHOLD  ( (int8_t)-39 )
#define DEFAULT_OLD_STATUS             ( (TemperatureStatus_t)eNORMAL_TEMP )

#define HIGH_TEMP_THRESHOLD_MIN        ( (uint8_t)15 )
#define HIGH_TEMP_THRESHOLD_MAX        ( (uint8_t)85 )
#define EP_TEMP_HYSTERESIS_MAX         ( (uint8_t)10 )
#endif
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
#if ( EP == 1 )
typedef struct
{
   uint8_t               EpMaxTemp;              /* The highest temperature ever experienced by the EndPoint (comm module radio) over its lifetime. */
   int8_t                EpMinTemp;              /* The lowest temperature ever witnessed by the EndPoint (comm module radio) over its lifetime. */
   uint8_t               EpTempHysteresis;       /* The hysteresis (in degrees C) to be subtracted from a maximum temperature threshold before issuing a radio high temperature cleared alarm */
   int8_t                EpTempMinThreshold;     /* The minimum temperature threshold that when reached, triggers a low temperature event for the radio. */
   uint8_t               HighTempThreshold;      /* The temperature threshold (in degrees Centigrade) which if exceeded, will trip a high temperature alarm by the EP. */
   TemperatureStatus_t   oldStatus;              /* Previously saved temperature used to make decision of clearing the events */
}TemperatureConfig_t;
#endif
/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
#if (VSWR_MEASUREMENT == 1)
extern float cachedVSWR;
#endif
/* FILE VARIABLE DEFINITIONS */
#if ( EP == 1 )
static TemperatureConfig_t  temperatureConfig_;         /* Local Copy of the Temperature Configuration and values */
static FileHandle_t         temperatureFileHandle_;     /* Contains the file handle information */
#endif
/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#if ( EP == 1 )
/*******************************************************************************

   Function name: TEMPERATURE_init

   Purpose: This function will creates/opens the file if doesn't exist

   Arguments: void

   Returns: returnStatus_t

*******************************************************************************/
returnStatus_t TEMPERATURE_init( void )
{
   returnStatus_t   retVal = eFAILURE;  /* Return status  */
   FileStatus_t     fileStatus;         /* Contains the file status */
   if ( eSUCCESS == FIO_fopen( &temperatureFileHandle_,        /* File Handle to access the file. */
                              ePART_NV_APP_BANKED,                   /* File is in the banked partition. */
                              ( uint16_t )eFN_TEMPERATURE_CONFIG,    /* File ID (filename) */
                              ( lCnt )sizeof( TemperatureConfig_t ), /* Size of the data in the file. */
                              FILE_IS_NOT_CHECKSUMED,                /* File attributes to use. */
                              0,                                     /* The update rate of the data in the file. */
                              &fileStatus ) )
   {
      /* If the file is created and opened */
      if ( fileStatus.bFileCreated )
      {
         /* Set the Default Values */
         temperatureConfig_.EpMaxTemp          = DEFAULT_EP_MAX_TEMPERATURE;
         temperatureConfig_.EpMinTemp          = DEFAULT_EP_MIN_TEMPERATURE;
         temperatureConfig_.EpTempHysteresis   = DEFAULT_EP_TEMP_HYSTERESIS;
         temperatureConfig_.HighTempThreshold  = DEFAULT_HIGH_TEMP_THRESHOLD;
         temperatureConfig_.EpTempMinThreshold = DEFAULT_EP_TEMP_MIN_THRESHOLD;
         temperatureConfig_.oldStatus          = DEFAULT_OLD_STATUS;
         /* Write the Default values */
         retVal = FIO_fwrite(&temperatureFileHandle_, 0, (uint8_t*) &temperatureConfig_, (lCnt)sizeof(temperatureConfig_));
      }
      else
      {
         // Read the Config File Data
         retVal = FIO_fread( &temperatureFileHandle_, (uint8_t *)&temperatureConfig_, 0, (lCnt)sizeof(temperatureConfig_));
      }
   }
   return retVal;
}
/*******************************************************************************

   Function name: TEMPERATURE_getEpTemperatureStatus

   Purpose: This function will get Temperature of the Radio

   Arguments: void

   Returns: Temperature Status

*******************************************************************************/
TemperatureStatus_t TEMPERATURE_getEpTemperatureStatus( void )
{
   TemperatureStatus_t status = eNORMAL_TEMP;  /* Set to Default. Will be updated later */
   int8_t              temperatureLocal;       /* Temporary variable to store Temperature */

   // Use CPU temperature which is common to EPs and TBs.
#if ( MCU_SELECTED == NXP_K24 )
   // Avoid using radio temperature because it takes 1ms and this is disruptive to the soft-modem (when used).
   temperatureLocal = (int8_t)ADC_Get_uP_Temperature( TEMP_IN_DEG_C );
#elif ( MCU_SELECTED == RA6E1 )
   int16_t        Temperature;
   bool tempOK = RADIO_Get_Chip_Temperature( (uint8_t) RADIO_0, &Temperature );
   temperatureLocal = (int8_t)Temperature;
   if( tempOK )
#endif
   {
      (void)FIO_fread( &temperatureFileHandle_, (uint8_t *)&temperatureConfig_, 0, (lCnt)sizeof(temperatureConfig_)); /* Read the file */

      if( temperatureLocal > temperatureConfig_.EpMaxTemp )
      {
         temperatureConfig_.EpMaxTemp = ( uint8_t )temperatureLocal;   /* Update */
         (void)FIO_fwrite(&temperatureFileHandle_, (fileOffset)offsetof(TemperatureConfig_t,EpMaxTemp),
                       (uint8_t*) &temperatureConfig_.EpMaxTemp, (lCnt)sizeof(temperatureConfig_.EpMaxTemp)); /* Update the file*/
      }
      else if( temperatureLocal < temperatureConfig_.EpMinTemp )
      {
         temperatureConfig_.EpMinTemp = temperatureLocal;   /* Update */
         (void)FIO_fwrite(&temperatureFileHandle_, (fileOffset)offsetof(TemperatureConfig_t,EpMinTemp),
                       (uint8_t*) &temperatureConfig_.EpMinTemp, (lCnt)sizeof(temperatureConfig_.EpMinTemp)); /* Update the file*/
      }
   /* Determine the Status */
      if ( temperatureLocal >= temperatureConfig_.HighTempThreshold )
      {
         status = eHIGH_TEMP_THRESHOLD;
         temperatureConfig_.oldStatus = status;
         (void)FIO_fwrite(&temperatureFileHandle_, (fileOffset)offsetof(TemperatureConfig_t,oldStatus),
                       (uint8_t*) &temperatureConfig_.oldStatus, (lCnt)sizeof(temperatureConfig_.oldStatus)); /* Update the file*/
      }
      else if( temperatureLocal <= temperatureConfig_.EpTempMinThreshold )
      {
         status = eLOW_TEMP_THRESHOLD;
         temperatureConfig_.oldStatus = status;
         (void)FIO_fwrite(&temperatureFileHandle_, (fileOffset)offsetof(TemperatureConfig_t,oldStatus),
                       (uint8_t*) &temperatureConfig_.oldStatus, (lCnt)sizeof(temperatureConfig_.oldStatus)); /* Update the file*/
      }
      else if( ( temperatureConfig_.oldStatus == eHIGH_TEMP_THRESHOLD ) && ( temperatureLocal <= ( temperatureConfig_.HighTempThreshold - temperatureConfig_.EpTempHysteresis ) ) )
      {
         status = eNORMAL_TEMP;
         temperatureConfig_.oldStatus = status;
         (void)FIO_fwrite(&temperatureFileHandle_, (fileOffset)offsetof(TemperatureConfig_t,oldStatus),
                       (uint8_t*) &temperatureConfig_.oldStatus, (lCnt)sizeof(temperatureConfig_.oldStatus)); /* Update the file*/
      }
      else if( ( temperatureConfig_.oldStatus == eLOW_TEMP_THRESHOLD ) && ( temperatureLocal >= ( temperatureConfig_.HighTempThreshold + temperatureConfig_.EpTempHysteresis ) ) )
      {
         status = eNORMAL_TEMP;
         temperatureConfig_.oldStatus = status;
         (void)FIO_fwrite(&temperatureFileHandle_, (fileOffset)offsetof(TemperatureConfig_t,oldStatus),
                       (uint8_t*) &temperatureConfig_.oldStatus, (lCnt)sizeof(temperatureConfig_.oldStatus)); /* Update the file*/
      }
   }
   return status;
}

/*******************************************************************************

   Function name: TEMPERATURE_getEpMaxTemperature

   Purpose: This function will get Max Temperature of the Radio

   Arguments: void

   Returns: EP Max Temperature

*******************************************************************************/
uint8_t TEMPERATURE_getEpMaxTemperature( void )
{
   return temperatureConfig_.EpMaxTemp;
}
/*******************************************************************************

   Function name: TEMPERATURE_setEpMaxTemperature

   Purpose: This function will set Max Temperature of the Radio

   Arguments: max Temperature

   Returns: returnStatus_t

*******************************************************************************/
returnStatus_t TEMPERATURE_setEpMaxTemperature( uint8_t maxTemp )
{
   returnStatus_t  retVal;

   temperatureConfig_.EpMaxTemp = maxTemp;
   retVal = FIO_fwrite(&temperatureFileHandle_, (fileOffset)offsetof(TemperatureConfig_t,EpMaxTemp),
                       (uint8_t*) &temperatureConfig_.EpMaxTemp, (lCnt)sizeof(temperatureConfig_.EpMaxTemp)); /* Update the file*/

   return retVal;
}

/*******************************************************************************

   Function name: TEMPERATURE_getEpMinTemperature

   Purpose: This function will get Max Temperature of the Radio

   Arguments: void

   Returns: Min Temperature

*******************************************************************************/
int8_t TEMPERATURE_getEpMinTemperature( void )
{
   return temperatureConfig_.EpMinTemp;
}
/*******************************************************************************

   Function name: TEMPERATURE_setEpMinTemperature

   Purpose: This function will set Max Temperature of the Radio

   Arguments: Min Temperature

   Returns: returnStatus_t

*******************************************************************************/
returnStatus_t TEMPERATURE_setEpMinTemperature( int8_t minTemp )
{
   returnStatus_t  retVal;
   temperatureConfig_.EpMinTemp = minTemp;
   retVal = FIO_fwrite(&temperatureFileHandle_, (fileOffset)offsetof(TemperatureConfig_t,EpMinTemp),
                          (uint8_t*) &temperatureConfig_.EpMinTemp, (lCnt)sizeof(temperatureConfig_.EpMinTemp)); /* Update the file*/
   return retVal;
}

/*******************************************************************************

   Function name: TEMPERATURE_getEpTempHysteresis

   Purpose: This function will get Temperature Hysteresis of the Radio

   Arguments: void

   Returns: Temperature Hysteresis

*******************************************************************************/
uint8_t TEMPERATURE_getEpTempHysteresis( void )
{
   return temperatureConfig_.EpTempHysteresis;
}

/*******************************************************************************

   Function name: TEMPERATURE_setEpTempHysteresis

   Purpose: This function will set Temperature Hysteresis
   Arguments: hysteresis

   Returns: returnStatus_t

*******************************************************************************/
returnStatus_t TEMPERATURE_setEpTempHysteresis( uint8_t hysteresis )
{
   returnStatus_t  retVal = eAPP_INVALID_VALUE;

   if ( hysteresis <= EP_TEMP_HYSTERESIS_MAX )
   {
      temperatureConfig_.EpTempHysteresis = hysteresis;
      retVal = FIO_fwrite(&temperatureFileHandle_, (fileOffset)offsetof(TemperatureConfig_t,EpTempHysteresis),
                          (uint8_t*) &temperatureConfig_.EpTempHysteresis, (lCnt)sizeof(temperatureConfig_.EpTempHysteresis)); /* Update the file*/
   }
   return retVal;
}

/*******************************************************************************

   Function name: TEMPERATURE_getHighTempThreshold

   Purpose: This function will get High Temperature Threshold for the Radio

   Arguments:  void

   Returns: High Temperature Threshold

*******************************************************************************/
uint8_t TEMPERATURE_getHighTempThreshold( void )
{
   return temperatureConfig_.HighTempThreshold;
}

/*******************************************************************************

   Function name: TEMPERATURE_setEpTempHysteresis

   Purpose: This function will set High Temperature Threshold for the Radio

   Arguments: High Temperature Threshold

   Returns: returnStatus_t

*******************************************************************************/
returnStatus_t TEMPERATURE_setHighTempThreshold( uint8_t hightempThreshold )
{
   returnStatus_t  retVal = eAPP_INVALID_VALUE;

   if( ( hightempThreshold >= HIGH_TEMP_THRESHOLD_MIN ) && ( hightempThreshold <= HIGH_TEMP_THRESHOLD_MAX ) )
   {
      temperatureConfig_.HighTempThreshold = hightempThreshold;
      retVal = FIO_fwrite(&temperatureFileHandle_, (fileOffset)offsetof(TemperatureConfig_t,HighTempThreshold),
                          (uint8_t*) &temperatureConfig_.HighTempThreshold, (lCnt)sizeof(temperatureConfig_.HighTempThreshold)); /* Update the file*/
   }
   return retVal;
}

/*******************************************************************************

   Function name: TEMPERATURE_getEpTempMinThreshold

   Purpose: This function will get Low Temperature Threshold for the Radio

   Arguments: void

   Returns: Low Temperature Threshold

*******************************************************************************/
int8_t TEMPERATURE_getEpTempMinThreshold( void )
{
   return temperatureConfig_.EpTempMinThreshold;
}

/*******************************************************************************

   Function name: TEMPERATURE_setEpTempMinThreshold

   Purpose: This function will set Low Temperature Threshold for the Radio

   Arguments:  Low Temperature Threshold

   Returns: returnStatus_t

*******************************************************************************/
returnStatus_t TEMPERATURE_setEpTempMinThreshold( int8_t lowTempThreshold )
{
   returnStatus_t  retVal;
   temperatureConfig_.EpTempMinThreshold = lowTempThreshold;
   retVal = FIO_fwrite(&temperatureFileHandle_, (fileOffset)offsetof(TemperatureConfig_t,EpTempMinThreshold),
                          (uint8_t*) &temperatureConfig_.EpTempMinThreshold, (lCnt)sizeof(temperatureConfig_.EpTempMinThreshold)); /* Update the file*/
   return retVal;
}
#endif /* end of if( EP == 1 )*/
/***********************************************************************************************************************
 *
 * Function name:  TEMPERATURE_OR_PM_Handler
 *
 * Purpose: Handles over the air parameter reading and writing for End Device Details.
 *
 * Arguments: enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t TEMPERATURE_OR_PM_Handler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr)
{
   PHY_GetConf_t   GetConf;
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case temperature :
         {
            GetConf = PHY_GetRequest( ePhyAttr_Temperature );   /* Read the Radio 0 Temperature*/
            *(int8_t *)value = (int8_t)GetConf.val.Temperature;
            retVal = eSUCCESS;
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(int8_t);
               attr->rValTypecast = (uint8_t)intValue;
            }
            break;
         }
#if ( VSWR_MEASUREMENT == 1 )
      case vswr :
        {
             retVal              = eSUCCESS;
             *(int32_t *)value   =  (int32_t)( cachedVSWR * pow(10.0,(double)VSWR_POWER_TEN) );
             if (attr != NULL)
             {
               attr->rValLen      = (uint16_t)sizeof(int32_t);
               attr->rValTypecast = (uint8_t)intValue;
               attr->powerOfTen   = (uint8_t)VSWR_POWER_TEN;
             }
             break;
        }
#endif
#if ( EP == 1 )
         case epMaxTemperature:
         {
            if ( sizeof(temperatureConfig_.EpMaxTemp) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint8_t *)value = TEMPERATURE_getEpMaxTemperature();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(uint8_t);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case epMinTemperature:
         {
            if ( sizeof(temperatureConfig_.EpMinTemp) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(int8_t *)value = TEMPERATURE_getEpMinTemperature();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(temperatureConfig_.EpMinTemp);
                  attr->rValTypecast = (uint8_t)intValue;
               }
            }
            break;
         }
         case epTempHysteresis:
         {
            if ( sizeof(temperatureConfig_.EpTempHysteresis) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint8_t *)value = TEMPERATURE_getEpTempHysteresis();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(temperatureConfig_.EpTempHysteresis);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case epTempMinThreshold:
         {
            if ( sizeof(temperatureConfig_.EpTempMinThreshold) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(int8_t *)value = TEMPERATURE_getEpTempMinThreshold();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(temperatureConfig_.EpTempMinThreshold);
                  attr->rValTypecast = (uint8_t)intValue;
               }
            }
            break;
         }
         case highTempThreshold:
         {
            if ( sizeof(temperatureConfig_.HighTempThreshold) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint8_t *)value = TEMPERATURE_getHighTempThreshold();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(temperatureConfig_.HighTempThreshold);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
#endif /* end of if( EP == 1 )*/
         default :
         {
            retVal = eAPP_NOT_HANDLED; // Success/failure
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
#if ( EP == 1 )
   else /* Used to "put" the variable. */
   {

      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case epMaxTemperature:
         {
            retVal = TEMPERATURE_setEpMaxTemperature( *(uint8_t *)value );

            if (retVal != eSUCCESS)
            {
               retVal = eAPP_INVALID_VALUE;
            }
            break;
         }
         case epMinTemperature:
         {
            retVal = TEMPERATURE_setEpMinTemperature( *(int8_t *)value );

            if (retVal != eSUCCESS)
            {
               retVal = eAPP_INVALID_VALUE;
            }
            break;
         }
         case epTempHysteresis:
         {
            retVal = TEMPERATURE_setEpTempHysteresis( *(uint8_t *)value );

            if (retVal != eSUCCESS)
            {
               retVal = eAPP_INVALID_VALUE;
            }
            break;
         }
         case epTempMinThreshold:
         {
            retVal = TEMPERATURE_setEpTempMinThreshold( *(int8_t *)value );

            if (retVal != eSUCCESS)
            {
               retVal = eAPP_INVALID_VALUE;
            }
            break;
         }
         case highTempThreshold:
         {
            retVal = TEMPERATURE_setHighTempThreshold( *(uint8_t *)value );

            if (retVal != eSUCCESS)
            {
               retVal = eAPP_INVALID_VALUE;
            }
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED; // Success/failure
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
#endif /* end of if( EP == 1 )*/
   return(retVal);
}
