/*!
 **********************************************************************************************************************
 *
 * \file   MIMT_info.c
 *
 * \brief  A brief description of what this file is.
 *
 * \details Every file that is part of a documented module has to have a file block, else it will not show
 *          up in the Doxygen "Modules" section.
 *
 **********************************************************************************************************************/

/***********************************************************************************************************************
 *
 * Copyright (c) 2017 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *
**********************************************************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include "MIMT_info.h"
#include "file_io.h"
#include "DBG_SerialDebug.h"
#include <stdlib.h>
#include <stdbool.h>

/* MACRO DEFINITIONS */
#define MIMTINFO_FILE_UPDATE_RATE_CFG_SEC ((uint32_t) 30 * 24 * 3600)    /* File updates once per month. */

/* TYPE DEFINITIONS */
typedef struct
{
   uint16_t fileVersion;
   uint16_t fctModuleTestDate;
   uint16_t fctEnclosureTestDate;
   uint16_t integrationSetupDate;
   uint32_t fctModuleProgramVersion;
   uint32_t fctEnclosureProgramVersion;
   uint32_t integrationProgramVersion;
   uint32_t fctModuleDatabaseVersion;
   uint32_t fctEnclosureDatabaseVersion;
   uint32_t integrationDatabaseVersion;
   uint64_t dataConfigurationDocumentVersion;
   uint64_t manufacturerNumber;
   uint32_t repairInformation;
} mimtInfoFileData_t;

/* CONSTANTS */
#define MIMTINFO_FILE_VERSION 1
#define SIX_BYTE_MIMT_MAX 0xFFFFFFFFFFFF
#define THREE_BYTE_MIMT_MAX 0xFFFFFF

/* VARIABLE DEFINITIONS */
static FileHandle_t  mimtInfoFileHandle = {0};
static OS_MUTEX_Obj mimtInfoMutex;

static mimtInfoFileData_t mimtInfoFileData = {0};

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */
/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_init
 *
 * \brief      Create/Initialize or open the MIMT information stored in the device.
 *
 * \param      none
 *
 * \return     returnStatus_t
 *
 * \details    If no file has been initialized for the MIMT information, create the file, intialize the static
 *             mimtInfoFileData_t struct with default values, and store the data in NV.  If the file already
 *             exists, read the data from the file and store in the static  mimtInfoFileData_t struct.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
returnStatus_t MIMTINFO_init(void)
{
   FileStatus_t   fileStatus;
   returnStatus_t retVal = eFAILURE;

   if ( OS_MUTEX_Create( &mimtInfoMutex ) )
   {
      if ( eSUCCESS == FIO_fopen(  &mimtInfoFileHandle, ePART_SEARCH_BY_TIMING,
                                   ( uint16_t )eFN_MIMTINFO,
                                   ( lCnt )sizeof( mimtInfoFileData ),
                                   FILE_IS_NOT_CHECKSUMED, MIMTINFO_FILE_UPDATE_RATE_CFG_SEC, &fileStatus ) )
      {
         if ( fileStatus.bFileCreated )
         {
            mimtInfoFileData.fileVersion = MIMTINFO_FILE_VERSION;
            retVal = FIO_fwrite( &mimtInfoFileHandle, 0, ( uint8_t* ) &mimtInfoFileData,
                                 ( lCnt )sizeof( mimtInfoFileData ) );
         }
         else
         {
            retVal = FIO_fread( &mimtInfoFileHandle, ( uint8_t* ) &mimtInfoFileData, 0,
                                ( lCnt )sizeof( mimtInfoFileData ) );
         }
      }
   }

   return( retVal );
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getFCTModuleTestDate
 *
 * \brief      Return the fctModuleTestDate parameter.
 *
 * \param      void
 *
 * \return     uint16_t
 *
 * \details    Return the fctModuleTestDate parameter.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
uint16_t MIMTINFO_getFctModuleTestDate(void)
{
   uint16_t uFctModuleTestDate_Local = 0; /*lint -esym(838,uFctModuleTestDate_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uFctModuleTestDate_Local = mimtInfoFileData.fctModuleTestDate;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return( uFctModuleTestDate_Local );
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setFctModuleTestDate
 *
 * \brief      Update fctModuleTestDate to a new value.
 *
 * \param      uint16_t
 *
 * \return     none
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setFctModuleTestDate(uint16_t uFctModuleTestDate)
{
   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   mimtInfoFileData.fctModuleTestDate = uFctModuleTestDate;
   (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                    (lCnt)sizeof(mimtInfoFileData_t)); //save file
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getFctEnclosureTestDate
 *
 * \brief      Return the fctEnclosureTestDate paramater.
 *
 * \param      void
 *
 * \return     uint16_t
 *
 * \details    Return the fctEnclosureTestDate parameter.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
uint16_t MIMTINFO_getFctEnclosureTestDate(void)
{
   uint16_t uFctEnclosureTestDate_Local = 0; /*lint -esym(838,uFctEnclosureTestDate_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uFctEnclosureTestDate_Local = mimtInfoFileData.fctEnclosureTestDate;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return( uFctEnclosureTestDate_Local );
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setFctEnclosureTestDate
 *
 * \brief
 *
 * \param      uint16_t
 *
 * \return     none
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setFctEnclosureTestDate(uint16_t uFctEnclosureTestDate)
{
   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   mimtInfoFileData.fctEnclosureTestDate = uFctEnclosureTestDate;
   (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                    (lCnt)sizeof(mimtInfoFileData_t)); //save file
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getIntegrationSetupDate
 *
 * \brief      Return the integrationSetupDate parameter.
 *
 * \param      void
 *
 * \return     uint16_t
 *
 * \details    Return the integrationSetupDate parameter.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
uint16_t MIMTINFO_getIntegrationSetupDate(void)
{
   uint16_t uIntegrationSetupDate_Local = 0; /*lint -esym(838,uIntegrationSetupDate_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uIntegrationSetupDate_Local = mimtInfoFileData.integrationSetupDate;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return( uIntegrationSetupDate_Local );
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setIntegrationSetupDate
 *
 * \brief
 *
 * \param      uint16_t
 *
 * \return     none
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setIntegrationSetupDate(uint16_t uIntegrationSetupDate)
{
   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   mimtInfoFileData.integrationSetupDate = uIntegrationSetupDate;
   (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                    (lCnt)sizeof(mimtInfoFileData_t)); //save file
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getFctModuleProgramVersion
 *
 * \brief      Return the fctModuleProgramVersion parameter.
 *
 * \param      void
 *
 * \return     uint32_t
 *
 * \details    Return the fctModuleProgramVersion parameter.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
uint32_t MIMTINFO_getFctModuleProgramVersion(void)
{
   uint32_t uFctModuleProgramVersion_Local = 0; /*lint -esym(838,uFctModuleProgramVersion_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uFctModuleProgramVersion_Local = mimtInfoFileData.fctModuleProgramVersion;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return(  uFctModuleProgramVersion_Local );
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setFctModuleProgramVersion
 *
 * \brief
 *
 * \param      uint32_t
 *
 * \return     none
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setFctModuleProgramVersion(uint32_t uFctModuleProgramVersion)
{
   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   mimtInfoFileData.fctModuleProgramVersion = uFctModuleProgramVersion;
   (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                    (lCnt)sizeof(mimtInfoFileData_t)); //save file
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getFctEnclosureProgramVersion
 *
 * \brief      Return the fctEnclosureProgramVersion parameter.
 *
 * \param      void
 *
 * \return     uint32_t
 *
 * \details    Return the fctEnclosureProgramVersion parameter.
 *
 * \sideeffect None
 *
 * \reentrant  Yes  MIMTINFO_setFctModuleTestDate
 *
 ***********************************************************************************************************************/
uint32_t MIMTINFO_getFctEnclosureProgramVersion(void)
{
   uint32_t uFctEnclosureProgramVersion_Local = 0; /*lint -esym(838,uFctEnclosureProgramVersion_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uFctEnclosureProgramVersion_Local = mimtInfoFileData.fctEnclosureProgramVersion;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return(  uFctEnclosureProgramVersion_Local );
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setFctEnclosureProgramVersion
 *
 * \brief
 *
 * \param      uint32_t
 *
 * \return     none
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setFctEnclosureProgramVersion(uint32_t uFctEnclosureProgramVersion)
{
   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   mimtInfoFileData.fctEnclosureProgramVersion = uFctEnclosureProgramVersion;
   (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                    (lCnt)sizeof(mimtInfoFileData_t)); //save file
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getIntegrationProgramVersion
 *
 * \brief      Return the integrationProgramVersion paramater.
 *
 * \param      void
 *
 * \return     uint32_t
 *
 * \details    Return the integrationProgramVersion paramater.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
uint32_t MIMTINFO_getIntegrationProgramVersion(void)
{
   uint32_t uIntegrationProgramVersion_Local = 0; /*lint -esym(838,uIntegrationProgramVersion_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uIntegrationProgramVersion_Local = mimtInfoFileData.integrationProgramVersion;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return(  uIntegrationProgramVersion_Local );
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setIntegrationProgramVersion
 *
 * \brief
 *
 * \param      uint32_t
 *
 * \return     none
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setIntegrationProgramVersion(uint32_t uIntegrationProgramVersion)
{
   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   mimtInfoFileData.integrationProgramVersion = uIntegrationProgramVersion;
   (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                    (lCnt)sizeof(mimtInfoFileData_t)); //save file
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getFctModuleDatabaseVersion
 *
 * \brief      Return the fctModuleDatabaseVersion parameter.
 *
 * \param      void
 *
 * \return     uint32_t
 *
 * \details    Return the fctModuleDatabaseVersion parameter.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
uint32_t MIMTINFO_getFctModuleDatabaseVersion(void)
{
   uint32_t uFctModuleDatabaseVersion_Local = 0; /*lint -esym(838,uFctModuleDatabaseVersion_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uFctModuleDatabaseVersion_Local = mimtInfoFileData.fctModuleDatabaseVersion;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return(  uFctModuleDatabaseVersion_Local );
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setFctModuleDatabaseVersion
 *
 * \brief
 *
 * \param      uint32_t
 *
 * \return     none
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setFctModuleDatabaseVersion(uint32_t uFctModuleDatabaseVersion)
{
   if( uFctModuleDatabaseVersion <= THREE_BYTE_MIMT_MAX )
   {
      OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
      mimtInfoFileData.fctModuleDatabaseVersion = uFctModuleDatabaseVersion;
      (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                       (lCnt)sizeof(mimtInfoFileData_t)); //save file
      OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
   }
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getFctEnclosureDatabaseVersion
 *
 * \brief      Return the fctEnclosureDatabaseVersion parameter.
 *
 * \param      void
 *
 * \return     uint32_t
 *
 * \details    Return the fctEnclosureDatabaseVersion parameter.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
uint32_t MIMTINFO_getFctEnclosureDatabaseVersion(void)
{
   uint32_t uFctEnclosureDatabaseVersion_Local = 0; /*lint -esym(838,uFctEnclosureDatabaseVersion_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uFctEnclosureDatabaseVersion_Local = mimtInfoFileData.fctEnclosureDatabaseVersion;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return(  uFctEnclosureDatabaseVersion_Local );
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setFctEnclosureDatabaseVersion
 *
 * \brief
 *
 * \param      uint32_t
 *
 * \return     none
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setFctEnclosureDatabaseVersion(uint32_t uFctEnclosureDatabaseVersion)
{
   if( uFctEnclosureDatabaseVersion <= THREE_BYTE_MIMT_MAX )
   {
      OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
      mimtInfoFileData.fctEnclosureDatabaseVersion = uFctEnclosureDatabaseVersion;
      (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                       (lCnt)sizeof(mimtInfoFileData_t)); //save file
      OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
   }
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getIntegrationDatabaseVersion
 *
 * \brief      Return the integrationDatabaseVersion parameter.
 *
 * \param      void
 *
 * \return     uint32_t
 *
 * \details    Return the integrationDatabaseVersion parameter.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
uint32_t MIMTINFO_getIntegrationDatabaseVersion(void)
{
   uint32_t uIntegrationDatabaseVersion_Local = 0; /*lint -esym(838,uIntegrationDatabaseVersion_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uIntegrationDatabaseVersion_Local = mimtInfoFileData.integrationDatabaseVersion;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return(  uIntegrationDatabaseVersion_Local );
}




/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setIntegrationDatabaseVersion
 *
 * \brief
 *
 * \param      uint32_t
 *
 * \return     none
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setIntegrationDatabaseVersion(uint32_t uIntegrationDatabaseVersion)
{
   if( uIntegrationDatabaseVersion <= THREE_BYTE_MIMT_MAX )
   {
      OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
      mimtInfoFileData.integrationDatabaseVersion = uIntegrationDatabaseVersion;
      (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                       (lCnt)sizeof(mimtInfoFileData_t)); //save file
      OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
   }
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getDataConfigurationDocumentVersion
 *
 * \brief      Return the eepromMapdocumentVersion parameter.
 *
 * \param      void
 *
 * \return     uint64_t
 *
 * \details    Return the eepromMapdocumentVersion parameter.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
uint64_t MIMTINFO_getDataConfigurationDocumentVersion(void)
{
   uint64_t uDataConfigurationDocumentVersion_Local = 0; /*lint -esym(838,uDataConfigurationDocumentVersion_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uDataConfigurationDocumentVersion_Local = mimtInfoFileData.dataConfigurationDocumentVersion;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return(  uDataConfigurationDocumentVersion_Local );
}



/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setDataConfigurationDocumentVersion
 *
 * \brief
 *
 * \param      uint64_t
 *
 * \return     void
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setDataConfigurationDocumentVersion(uint64_t uDataConfigurationDocumentVersion)
{
   if( uDataConfigurationDocumentVersion <= SIX_BYTE_MIMT_MAX )
   {
      OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
      mimtInfoFileData.dataConfigurationDocumentVersion = uDataConfigurationDocumentVersion;
      (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                       (lCnt)sizeof(mimtInfoFileData_t)); //save file
      OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
   }
}



/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getManufacturerNumber
 *
 * \brief      Return the manufacturerNumber parameter.
 *
 * \param      void
 *
 * \return     uint64_t
 *
 * \details    Return the manufacturerNumber parameter.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
uint64_t MIMTINFO_getManufacturerNumber(void)
{
   uint64_t uManufacturerNumber_Local = 0; /*lint -esym(838,uManufacturerNumber_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uManufacturerNumber_Local = mimtInfoFileData.manufacturerNumber;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return(  uManufacturerNumber_Local );
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setManufacturerNumber
 *
 * \brief
 *
 * \param      uint64_t
 *
 * \return     void
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setManufacturerNumber(uint64_t uManufacturerNumber)
{
   if( uManufacturerNumber <= SIX_BYTE_MIMT_MAX )
   {
      OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
      mimtInfoFileData.manufacturerNumber = uManufacturerNumber;
      (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                       (lCnt)sizeof(mimtInfoFileData_t)); //save file
      OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
   }
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_getRepairInformation
 *
 * \brief      Return the repairInformation parameter.
 *
 * \param      void
 *
 * \return     uint32_t
 *
 * \details    Return the repairInformation parameter.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
uint32_t MIMTINFO_getRepairInformation(void)
{
   uint32_t uRepairInformation_Local = 0; /*lint -esym(838,uRepairInformation_Local) previous value not used */

   OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
   uRepairInformation_Local = mimtInfoFileData.repairInformation;
   OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails

   return(  uRepairInformation_Local );
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_setRepairInformation
 *
 * \brief      Set the repair information parameter in NV.
 *
 * \param      uint32_t
 *
 * \return     void
 *
 * \details    This function will update the static mimtInfoFileData_t data structure and store
 *             the updated data structure in NV.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
void MIMTINFO_setRepairInformation(uint32_t uRepairInformation)
{
   if( uRepairInformation <= (uint32_t)THREE_BYTE_MIMT_MAX )
   {
      OS_MUTEX_Lock( &mimtInfoMutex ); // Function will not return if it fails
      mimtInfoFileData.repairInformation = uRepairInformation;
      (void)FIO_fwrite(&mimtInfoFileHandle, 0, (uint8_t const *)&mimtInfoFileData,
                       (lCnt)sizeof(mimtInfoFileData_t)); //save file
      OS_MUTEX_Unlock( &mimtInfoMutex ); // Function will not return if it fails
   }
}


/*!
 **********************************************************************************************************************
 *
 * \fn         MIMTINFO_OR_PM_Handler
 *
 * \brief      Handle over the air parameter read requests.
 *
 * \param      enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr
 *
 * \return     returnStatus_t
 *
 * \details    This function will process over the air read requests for items contained within the
 *             mimtInfoFileData data structure.  Based upon the meterReadingType passed into the function,
 *             different getter functions are utilized to return the requested values.  Based upon HEEP
 *             documentation, the parameters contained in the mimtInfoFileData data structure are only
 *             allowed to be read over the air.  No writing is allowed over the air.
 *
 * \sideeffect None
 *
 * \reentrant  Yes
 *
 ***********************************************************************************************************************/
returnStatus_t MIMTINFO_OR_PM_Handler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr)
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case FctModuleTestDate :
         {
            if ( sizeof(mimtInfoFileData.fctModuleTestDate) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint16_t *)value = MIMTINFO_getFctModuleTestDate(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.fctModuleTestDate, uintValue, 0);
                  attr->rValTypecast = (uint8_t)dateValue;
               }
            }
            break;
         }
         case FctEnclosureTestDate :
         {
            if ( sizeof(mimtInfoFileData.fctEnclosureTestDate) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint16_t *)value = MIMTINFO_getFctEnclosureTestDate(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.fctEnclosureTestDate, uintValue, 0);
                  attr->rValTypecast = (uint8_t)dateValue;
               }
            }
            break;
         }
         case IntegrationSetupDate :
         {
            if ( sizeof(mimtInfoFileData.integrationSetupDate) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint16_t *)value = MIMTINFO_getIntegrationSetupDate(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.integrationSetupDate, uintValue, 0);
                  attr->rValTypecast = (uint8_t)dateValue;
               }
            }
            break;
         }
         case FctModuleProgramVersion :
         {
            if ( sizeof(mimtInfoFileData.fctModuleProgramVersion) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint32_t *)value = MIMTINFO_getFctModuleProgramVersion(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.fctModuleProgramVersion, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case FctEnclosureProgramVersion :
         {
            if ( sizeof(mimtInfoFileData.fctEnclosureProgramVersion) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint32_t *)value = MIMTINFO_getFctEnclosureProgramVersion(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.fctEnclosureProgramVersion, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case IntegrationProgramVersion :
         {
            if ( sizeof(mimtInfoFileData.integrationProgramVersion) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint32_t *)value = MIMTINFO_getIntegrationProgramVersion(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.integrationProgramVersion, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case FctModuleDatabaseVersion :
         {
            if ( sizeof(mimtInfoFileData.fctModuleDatabaseVersion) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint32_t *)value = MIMTINFO_getFctModuleDatabaseVersion(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.fctModuleDatabaseVersion, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case FctEnclosureDatabaseVersion :
         {
            if ( sizeof(mimtInfoFileData.fctEnclosureDatabaseVersion) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint32_t *)value = MIMTINFO_getFctEnclosureDatabaseVersion(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.fctEnclosureDatabaseVersion, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case IntegrationDatabaseVersion :
         {
            if ( sizeof(mimtInfoFileData.integrationDatabaseVersion) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint32_t *)value = MIMTINFO_getIntegrationDatabaseVersion(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.integrationDatabaseVersion, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case dataConfigurationDocumentVersion :
         {
            if ( sizeof(mimtInfoFileData.dataConfigurationDocumentVersion) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint64_t *)value = MIMTINFO_getDataConfigurationDocumentVersion(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.dataConfigurationDocumentVersion, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case manufacturerNumber :
         {
            if ( sizeof(mimtInfoFileData.manufacturerNumber) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint64_t *)value = MIMTINFO_getManufacturerNumber(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.manufacturerNumber, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case repairInformation :
         {
            if ( sizeof(mimtInfoFileData.repairInformation) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint32_t *)value = MIMTINFO_getRepairInformation(); /* get the parameter value */
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mimtInfoFileData.repairInformation, uintValue, 0);
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

   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched
