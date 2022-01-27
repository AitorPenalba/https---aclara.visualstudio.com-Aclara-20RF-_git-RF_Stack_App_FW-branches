/***********************************************************************************************************************
 *
 * Filename: smtd_config.c
 *
 * Global Designator: SMTDCFG_
 *
 * Contents: Configuration parameter I/O for stack manager time diversity.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2019 Aclara.  All Rights Reserved.
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

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"

#define SMTDCFG_GLOBALS
#include "smtd_config.h"
#undef SMTDCFG_GLOBALS

#include "file_io.h"
#include "DBG_SerialDebug.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */


/* ****************************************************************************************************************** */
/* CONSTANTS */

#define SMTDCFG_FILE_UPDATE_RATE_CFG_SEC ((uint32_t) 30 * 24 * 3600)    /* File updates once per month. */

#define SMTDCFG_FILE_VERSION 2

#if ( EP == 1 )
/*lint -esym(750,SMTDCFG_APPLY_CONFIRM_MINIMUM,SMTDCFG_APPLY_CONFIRM_MAXIMUM) not referenced */
#define SMTDCFG_APPLY_CONFIRM_DEFAULT 120
#define SMTDCFG_APPLY_CONFIRM_MINIMUM 0
#define SMTDCFG_APPLY_CONFIRM_MAXIMUM 255
#endif //#if ( EP == 1 )

#define ENG_BU_TRAFFIC_CLASS_DEFAULT  (uint8_t)1
#define ENG_BU_TRAFFIC_CLASS_MAX      (uint8_t)63

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uint16_t fileVersion;
#if ( EP == 1 )
   uint8_t  uSmLogTimeDiversity;
#endif   //#if ( EP == 1 )
   bool     engBuEnabled;                 /* Enable/disable Bubbleup Engineering Stats */
   uint8_t  engBuTrafficClass;            /* Engineering Stats Traffic Class */
} smTdConfigFileData_t;


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static FileHandle_t smTdConfigFileHandle = {0};
static OS_MUTEX_Obj smTdConfigMutex;

static smTdConfigFileData_t smTdConfigFileData = {0};


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */


/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

static returnStatus_t SMTDCFG_write(void);


/***********************************************************************************************************************
 *
 * Function name: SMTDCFG_init
 *
 * Purpose: Create and initialize or open and read SM Time Diversity configuration.
 *
 * Arguments: None
 *
 * Returns: void
 *
 * Re-entrant Code:
 *
 * Notes:
 *
 **********************************************************************************************************************/

returnStatus_t SMTDCFG_init(void)
{
   FileStatus_t   fileStatus;
   returnStatus_t retVal = eFAILURE;

   if (OS_MUTEX_Create(&smTdConfigMutex))
   {
      if (eSUCCESS == FIO_fopen(&smTdConfigFileHandle, ePART_SEARCH_BY_TIMING, (uint16_t)eFN_SMTDCFG, (lCnt)sizeof(smTdConfigFileData),
                                FILE_IS_NOT_CHECKSUMED, SMTDCFG_FILE_UPDATE_RATE_CFG_SEC, &fileStatus) )
      {
         if (fileStatus.bFileCreated)
         {
            smTdConfigFileData.fileVersion = SMTDCFG_FILE_VERSION;
#if ( EP == 1 )
            smTdConfigFileData.uSmLogTimeDiversity = SMTDCFG_APPLY_CONFIRM_DEFAULT;
#endif   //#if ( EP == 1 )
            smTdConfigFileData.engBuEnabled        = true;
            smTdConfigFileData.engBuTrafficClass   = ENG_BU_TRAFFIC_CLASS_DEFAULT;

            retVal = FIO_fwrite(&smTdConfigFileHandle, 0, (uint8_t*) &smTdConfigFileData, (lCnt)sizeof(smTdConfigFileData));
         }
         else
         {
            retVal = FIO_fread(&smTdConfigFileHandle, (uint8_t*) &smTdConfigFileData, 0, (lCnt)sizeof(smTdConfigFileData));
         }
      }
   }

   return(retVal);
}


#if ( EP == 1 )
/***********************************************************************************************************************
 *
 * Function name: SMTDCFG_get_log
 *
 * Purpose: Get smLogTimeDiversity from SM Time Diversity configuration.
 *
 * Arguments: None
 *
 * Returns: uint8_t uSmLogTimeDiversity
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/

uint8_t SMTDCFG_get_log(void)
{
   uint8_t uSmLogTimeDiversity;

   OS_MUTEX_Lock(&smTdConfigMutex); // Function will not return if it fails
   uSmLogTimeDiversity = smTdConfigFileData.uSmLogTimeDiversity;
   OS_MUTEX_Unlock(&smTdConfigMutex); // Function will not return if it fails

   return(uSmLogTimeDiversity);
}


/***********************************************************************************************************************
 *
 * Function name: SMTDCFG_set_log
 *
 * Purpose: Set smLogTimeDiversity in SM Time Diversity configuration.
 *
 * Arguments: uint16_t uSmLogTimeDiversity
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/

returnStatus_t SMTDCFG_set_log(uint8_t uSmLogTimeDiversity)
{
   returnStatus_t retVal;

   OS_MUTEX_Lock(&smTdConfigMutex); // Function will not return if it fails
   smTdConfigFileData.uSmLogTimeDiversity = uSmLogTimeDiversity;
   retVal = SMTDCFG_write();
   OS_MUTEX_Unlock(&smTdConfigMutex); // Function will not return if it fails

   return(retVal);
}
#endif   //#if ( EP == 1 )

/***********************************************************************************************************************
 *
 * Function name: SMTDCFG_getEngBuEnabled
 *
 * Purpose: Get engBuEnabled from SM Time Diversity configuration.
 *
 * Arguments: None
 *
 * Returns: bool engBuEnabled
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
bool SMTDCFG_getEngBuEnabled(void)
{
   bool bEngBuEnabled;
   /* Since this value is "atomic", no MUTEX is needed here. If changed to larger value or processor with
      bus smaller than variable size, re-enable mutex.  */
   OS_MUTEX_Lock(&smTdConfigMutex); // Function will not return if it fails
   bEngBuEnabled = smTdConfigFileData.engBuEnabled;
   OS_MUTEX_Unlock(&smTdConfigMutex); // Function will not return if it fails
   return ( bEngBuEnabled );
}


/***********************************************************************************************************************
 *
 * Function name: SMTDCFG_setEngBuEnabled
 *
 * Purpose: Set engBuEnabled in SM Time Diversity configuration.
 *
 * Arguments: uint8_t bEngBuEnabled
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t SMTDCFG_setEngBuEnabled(uint8_t uEngBuEnabled)
{
   returnStatus_t retVal;

   if( uEngBuEnabled <= 1 )
   {
      OS_MUTEX_Lock(&smTdConfigMutex); // Function will not return if it fails
      smTdConfigFileData.engBuEnabled = uEngBuEnabled;
      retVal = SMTDCFG_write();
      OS_MUTEX_Unlock(&smTdConfigMutex); // Function will not return if it fails
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }
   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: SMTDCFG_getEngBuTrafficClass
 *
 * Purpose: Get engBuTrafficClass from SM Time Diversity configuration.
 *
 * Arguments: None
 *
 * Returns: uint8_t engBuTrafficClass
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint8_t SMTDCFG_getEngBuTrafficClass(void)
{
   uint8_t uEngBuTrafficClass;

   OS_MUTEX_Lock(&smTdConfigMutex); // Function will not return if it fails
   uEngBuTrafficClass = smTdConfigFileData.engBuTrafficClass;
   OS_MUTEX_Unlock(&smTdConfigMutex); // Function will not return if it fails
   return ( uEngBuTrafficClass );
}


/***********************************************************************************************************************
 *
 * Function name: SMTDCFG_setEngBuTrafficClass
 *
 * Purpose: Set engBuTrafficClass in SM Time Diversity configuration.
 *
 * Arguments: uint8_t engBuTrafficClass
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t SMTDCFG_setEngBuTrafficClass(uint8_t uEngBuTrafficClass)
{
   returnStatus_t retVal = eFAILURE;

   if ( uEngBuTrafficClass <= ENG_BU_TRAFFIC_CLASS_MAX )
   {
      OS_MUTEX_Lock(&smTdConfigMutex); // Function will not return if it fails
      smTdConfigFileData.engBuTrafficClass = uEngBuTrafficClass;
      retVal = SMTDCFG_write();
      OS_MUTEX_Unlock(&smTdConfigMutex); // Function will not return if it fails
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }

   return(retVal);
}
/***********************************************************************************************************************
 *
 * Function name: SMTDCFG_write
 *
 * Purpose: Write SM Time Diversity configuration to flash.
 *
 * Arguments: None
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/

static returnStatus_t SMTDCFG_write(void)
{
   returnStatus_t retVal;

   retVal = FIO_fwrite(&smTdConfigFileHandle, 0, (uint8_t*) &smTdConfigFileData, (lCnt)sizeof(smTdConfigFileData));

   if (eSUCCESS == retVal)
   {
      DBG_logPrintf('I', "eFN_SMTDCFG Write Successful");
   }
   else
   {
      DBG_logPrintf('E', "eFN_SMTDCFG Write Failed");
   }

   return(retVal);
}


/***********************************************************************************************************************

   Function Name: SMTDCFG_OR_PM_Handler

   Purpose: Get/Set parameters related to TIME_SYS

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t SMTDCFG_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
#if ( EP == 1 )
         case smLogTimeDiversity :
         {
            if ( sizeof(smTdConfigFileData.uSmLogTimeDiversity) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       *(uint8_t *)value = SMTDCFG_get_log();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(smTdConfigFileData.uSmLogTimeDiversity);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
#endif   //#if ( EP == 1 )
         case engBuEnabled :
         {
            if ( sizeof(smTdConfigFileData.engBuEnabled) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       *(uint8_t *)value = SMTDCFG_getEngBuEnabled();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(smTdConfigFileData.engBuEnabled);
                  attr->rValTypecast = (uint8_t)Boolean;
               }
            }
            break;
         }
         case engBuTrafficClass:
         {
            if ( sizeof(smTdConfigFileData.engBuTrafficClass) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       *(uint8_t *)value = SMTDCFG_getEngBuTrafficClass();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(smTdConfigFileData.engBuTrafficClass);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = eAPP_INVALID_TYPECAST;
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
#if ( EP == 1 )
         case smLogTimeDiversity : // set smLogTimeDiversity parameter
         {
            if ( (uint8_t)uintValue == attr->rValTypecast )
            {
               retVal = SMTDCFG_set_log(*(uint8_t *)value);
            }
            break;
         }
#endif   //#if ( EP == 1 )
         case engBuEnabled :
         {
            if ( (uint8_t)Boolean == attr->rValTypecast )
            {
               retVal = SMTDCFG_setEngBuEnabled(*(uint8_t *)value);
            }
            break;
         }
         case engBuTrafficClass :
         {
            if ( (uint8_t)uintValue == attr->rValTypecast )
            {
               retVal = SMTDCFG_setEngBuTrafficClass(*(uint8_t *)value);
            }
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   return ( retVal );
}
