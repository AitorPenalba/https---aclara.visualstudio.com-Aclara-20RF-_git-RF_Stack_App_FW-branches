/***********************************************************************************************************************
 *
 * Filename: dfwtd_config.c
 *
 * Global Designator: DFWTDCFG_
 *
 * Contents: Configuration parameter I/O for DFW time diversity.
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

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#if ( EP == 1 )   /* Entire file is only used by the EP */

#define DFWTDCFG_GLOBALS
#include "dfwtd_config.h"
#undef DFWTDCFG_GLOBALS

#include "file_io.h"
#include "DBG_SerialDebug.h"
#include "dfw_interface.h"


/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */


/* ****************************************************************************************************************** */
/* CONSTANTS */

#define DFWTDCFG_FILE_UPDATE_RATE_CFG_SEC ((uint32_t) 30 * 24 * 3600)    /* File updates once per month. */

#define DFWTDCFG_FILE_VERSION 1

/*lint -esym(750,DFWTDCFG_APPLY_CONFIRM_DEFAULT,DFWTDCFG_APPLY_CONFIRM_MINIMUM,DFWTDCFG_APPLY_CONFIRM_MAXIMUM)    */
/*lint -esym(750,DFWTDCFG_DOWNLOAD_CONFIRM_DEFAULT,DFWTDCFG_DOWNLOAD_CONFIRM_MINIMUM)                             */
/*lint -esym(750,DFWTDCFG_DOWNLOAD_CONFIRM_MAXIMUM)*/
#define DFWTDCFG_APPLY_CONFIRM_DEFAULT 25
#define DFWTDCFG_APPLY_CONFIRM_MINIMUM 0
#define DFWTDCFG_APPLY_CONFIRM_MAXIMUM 255

#define DFWTDCFG_DOWNLOAD_CONFIRM_DEFAULT 25
#define DFWTDCFG_DOWNLOAD_CONFIRM_MINIMUM 0
#define DFWTDCFG_DOWNLOAD_CONFIRM_MAXIMUM 255


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static FileHandle_t dfwTdConfigFileHandle = {0};
static OS_MUTEX_Obj dfwTdConfigMutex;

static dfwTdConfigFileData_t dfwTdConfigFileData = {0};

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */


/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

static returnStatus_t DFWTDCFG_write(void);


/***********************************************************************************************************************
 *
 * Function name: DFWTDCFG_init
 *
 * Purpose: Create and initialize or open and read DFW Time Diversity configuration.
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

returnStatus_t DFWTDCFG_init(void)
{
   FileStatus_t   fileStatus;
   returnStatus_t retVal = eFAILURE;

   if (OS_MUTEX_Create(&dfwTdConfigMutex))
   {
      if (eSUCCESS == FIO_fopen(&dfwTdConfigFileHandle, ePART_SEARCH_BY_TIMING, (uint16_t)eFN_DFWTDCFG, (lCnt)sizeof(dfwTdConfigFileData),
                                FILE_IS_NOT_CHECKSUMED, DFWTDCFG_FILE_UPDATE_RATE_CFG_SEC, &fileStatus) )
      {
         if (fileStatus.bFileCreated)
         {
            dfwTdConfigFileData.fileVersion                      = DFWTDCFG_FILE_VERSION;
            dfwTdConfigFileData.uDfwApplyConfirmTimeDiversity    = DFWTDCFG_APPLY_CONFIRM_DEFAULT;
            dfwTdConfigFileData.uDfwDownloadConfirmTimeDiversity = DFWTDCFG_DOWNLOAD_CONFIRM_DEFAULT;
            retVal = FIO_fwrite(&dfwTdConfigFileHandle, 0, (uint8_t*) &dfwTdConfigFileData, (lCnt)sizeof(dfwTdConfigFileData));
         }
         else
         {
            retVal = FIO_fread(&dfwTdConfigFileHandle, (uint8_t*) &dfwTdConfigFileData, 0, (lCnt)sizeof(dfwTdConfigFileData));
         }
      }
   }

   return(retVal);
}


/***********************************************************************************************************************
 *
 * Function name: DFWTDCFG_get_apply_confirm
 *
 * Purpose: Get dfwApplyConfirmTimeDiversity from DFW Time Diversity configuration.
 *
 * Arguments: None
 *
 * Returns: uint8_t uDfwApplyConfirmTimeDiversity
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/

uint8_t DFWTDCFG_get_apply_confirm(void)
{
   uint8_t uDfwApplyConfirmTimeDiversity;

   OS_MUTEX_Lock(&dfwTdConfigMutex); // Function will not return if it fails
   uDfwApplyConfirmTimeDiversity = dfwTdConfigFileData.uDfwApplyConfirmTimeDiversity;
   OS_MUTEX_Unlock(&dfwTdConfigMutex); // Function will not return if it fails

   return(uDfwApplyConfirmTimeDiversity);
}


/***********************************************************************************************************************
 *
 * Function name: DFWTDCFG_get_download_confirm
 *
 * Purpose: Get dfwDownloadConfirmTimeDiversity from DFW Time Diversity configuration.
 *
 * Arguments: None
 *
 * Returns: uint8_t uDfwDownloadConfirmTimeDiversity
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/

uint8_t DFWTDCFG_get_download_confirm(void)
{
   uint8_t uDfwDownloadConfirmTimeDiversity;

   OS_MUTEX_Lock(&dfwTdConfigMutex); // Function will not return if it fails
   uDfwDownloadConfirmTimeDiversity = dfwTdConfigFileData.uDfwDownloadConfirmTimeDiversity;
   OS_MUTEX_Unlock(&dfwTdConfigMutex); // Function will not return if it fails

   return(uDfwDownloadConfirmTimeDiversity);
}


/***********************************************************************************************************************
 *
 * Function name: DFWTDCFG_set_apply_confirm
 *
 * Purpose: Set dfwApplyConfirmTimeDiversity in DFW Time Diversity configuration.
 *
 * Arguments: uint16_t uDfwApplyConfirmTimeDiversity
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/

returnStatus_t DFWTDCFG_set_apply_confirm(uint8_t uDfwApplyConfirmTimeDiversity)
{
   returnStatus_t retVal;

   OS_MUTEX_Lock(&dfwTdConfigMutex); // Function will not return if it fails
   dfwTdConfigFileData.uDfwApplyConfirmTimeDiversity = uDfwApplyConfirmTimeDiversity;
   retVal = DFWTDCFG_write();
   OS_MUTEX_Unlock(&dfwTdConfigMutex); // Function will not return if it fails

   return(retVal);
}


/***********************************************************************************************************************
 *
 * Function name: DFWTDCFG_set_download_confirm
 *
 * Purpose: Set dfwDownloadConfirmTimeDiversity in DFW Time Diversity configuration.
 *
 * Arguments: uuint8_t uDfwDownloadConfirmTimeDiversity
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/

returnStatus_t DFWTDCFG_set_download_confirm(uint8_t uDfwDownloadConfirmTimeDiversity)
{
   returnStatus_t retVal;

   OS_MUTEX_Lock(&dfwTdConfigMutex); // Function will not return if it fails
   dfwTdConfigFileData.uDfwDownloadConfirmTimeDiversity = uDfwDownloadConfirmTimeDiversity;
   retVal = DFWTDCFG_write();
   OS_MUTEX_Unlock(&dfwTdConfigMutex); // Function will not return if it fails

   return(retVal);
}


/***********************************************************************************************************************
 *
 * Function name: DFWTDCFG_write
 *
 * Purpose: Write DFW Time Diversity configuration to flash.
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

static returnStatus_t DFWTDCFG_write(void)
{
   returnStatus_t retVal;

   retVal = FIO_fwrite(&dfwTdConfigFileHandle, 0, (uint8_t*) &dfwTdConfigFileData, (lCnt)sizeof(dfwTdConfigFileData));

   if (eSUCCESS == retVal)
   {
      DBG_logPrintf('I', "eFN_DFWTDCFG Write Successful");
   }
   else
   {
      DBG_logPrintf('E', "eFN_DFWTDCFG Write Failed");
   }

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: DFWTDCFG_GetTimeDiversity
 *
 * Purpose: Get DFW Time Diversity configuration data.
 *
 * Arguments: dfwTdConfigFileData_t - structure for the configuration settings
 *
 * Returns: returnStatus_t - always eSUCCESS
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t DFWTDCFG_GetTimeDiversity(dfwTdConfigFileData_t *diversitySettings)
{
   OS_MUTEX_Lock(&dfwTdConfigMutex); // Function will not return if it fails
   *diversitySettings = dfwTdConfigFileData;
   OS_MUTEX_Unlock(&dfwTdConfigMutex); // Function will not return if it fails

   return(eSUCCESS);
}

/***********************************************************************************************************************
 *
 * Function name: DFWTDCFG_SetTimeDiversity
 *
 * Purpose: Set DFW Time Diversity configuration data.
 *
 * Arguments: dfwTdConfigFileData_t - structure for the configuration settings
 *
 * Returns: returnStatus_t - eSUCCESS if written, else eFAILURE
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t DFWTDCFG_SetTimeDiversity(dfwTdConfigFileData_t *diversitySettings)  /*lint !e818 could be pointer to const */
{
   returnStatus_t retVal;

   OS_MUTEX_Lock(&dfwTdConfigMutex); // Function will not return if it fails
   dfwTdConfigFileData.uDfwDownloadConfirmTimeDiversity = diversitySettings->uDfwDownloadConfirmTimeDiversity;
   dfwTdConfigFileData.uDfwApplyConfirmTimeDiversity    = diversitySettings->uDfwApplyConfirmTimeDiversity;
   retVal = DFWTDCFG_write();
   OS_MUTEX_Unlock(&dfwTdConfigMutex); // Function will not return if it fails

   return(retVal);
}  /*lint !e818 diversitySettings could be pointer to const */
#else
   qweuopqiwue
#endif   //#if ( EP == 1 )


/***********************************************************************************************************************

   Function Name: DFWTDCFG_OR_PM_Handler

   Purpose: Get/Set parameters related to DFWTDCFG

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t DFWTDCFG_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case dfwApplyConfirmTimeDiversity :
         {
            if ( sizeof(dfwTdConfigFileData.uDfwApplyConfirmTimeDiversity) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       *(uint8_t *)value = DFWTDCFG_get_apply_confirm();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(dfwTdConfigFileData.uDfwApplyConfirmTimeDiversity);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case dfwDowloadConfirmTimeDiversity :
         {
            if ( sizeof(dfwTdConfigFileData.uDfwDownloadConfirmTimeDiversity) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       *(uint8_t *)value = DFWTDCFG_get_download_confirm();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(dfwTdConfigFileData.uDfwDownloadConfirmTimeDiversity);
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
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case dfwApplyConfirmTimeDiversity :
         {
            retVal = DFWTDCFG_set_apply_confirm(*(uint8_t *)value);
            break;
         }
         case dfwDowloadConfirmTimeDiversity :
         {
            retVal = DFWTDCFG_set_download_confirm(*(uint8_t *)value);
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
