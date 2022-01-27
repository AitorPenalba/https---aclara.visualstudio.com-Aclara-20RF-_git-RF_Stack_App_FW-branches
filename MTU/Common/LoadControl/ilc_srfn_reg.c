/***********************************************************************************************************************
 *
 * Filename:   ILC_SRFN_REG.c
 *
 * Global Designator: ILC_SRFN_REG_
 *
 * Contents: Contains the routines to support the SRFN Registration
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2017 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ KAD Created 080207
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 *
 **********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdio.h>
#include "ilc_srfn_reg.h"


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define ILC_SRFN_REG_READ_REGISTER_TIME_OUT_mS     ( (uint32_t) 1000 )         /* Time out in ms when waiting for DRU */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ****************************************************************************************************************** */
/* GLOBAL VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
static ILC_DRU_DRIVER_druInfo_t DruSrfnRegistrationInfo;


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static enum_CIM_QualityCode getDruSerialNumber( uint32_t *pMfgSerialNumber_u32 );
static enum_CIM_QualityCode getDruFwVersion( uint8_t *pFwMajor_u8, uint8_t *pFwMinor_u8, uint16_t *pFwEngBuild_u16 );
static enum_CIM_QualityCode getDruHwVersion( uint8_t *pHwGeneration_u8, uint8_t *pHwRevision_u8 );
static enum_CIM_QualityCode getDruModel( uint8_t *pEdType_u8, uint8_t *pEdModel_u8 );
static enum_CIM_QualityCode getDruInfo ( ILC_DRU_DRIVER_druInfo_t *pDest );


/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
/***********************************************************************************************************************
   Function name: getDruSerialNumber

   Purpose: Requests the DRU Driver to read the DRU Serial Number

   Arguments: uint32_t *pMfgSerialNumber_u32 - Location where to store the DRU Serial Number

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 **********************************************************************************************************************/
static enum_CIM_QualityCode getDruSerialNumber( uint32_t *pMfgSerialNumber_u32 )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   ILC_DRU_DRIVER_druRegister_u Register;

   if ( CIM_QUALCODE_SUCCESS == ILC_DRU_DRIVER_readDruRegister( &Register, ILC_DRU_DRIVER_REG_SERIAL_NUMBER,
                                                               ILC_SRFN_REG_READ_REGISTER_TIME_OUT_mS ) )
   {
      *pMfgSerialNumber_u32  = Register.SerialNumber_u32;
      retVal = CIM_QUALCODE_SUCCESS;
   }
   else
   {
      INFO_printf( "Unsuccessful DRU Serial Number reading" );
   }

   return retVal;
}


/***********************************************************************************************************************
   Function name: getDruFwVersion

   Purpose: Requests the DRU Driver to read the DRU FW Version

   Arguments: uint8_t *pFwMajor_u8      - Location where to store the DRU Fw Major version number
              uint8_t *pFwMinor_u8      - Location where to store the DRU Fw Minor version number
              uint8_t *pFwEngBuild_u16  - Location where to store the DRU Fw Eng Build version number

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 **********************************************************************************************************************/
static enum_CIM_QualityCode getDruFwVersion( uint8_t *pFwMajor_u8, uint8_t *pFwMinor_u8, uint16_t *pFwEngBuild_u16 )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   ILC_DRU_DRIVER_druRegister_u Register;

   if ( CIM_QUALCODE_SUCCESS == ILC_DRU_DRIVER_readDruRegister( &Register, ILC_DRU_DRIVER_REG_FW_VERSION, ILC_SRFN_REG_READ_REGISTER_TIME_OUT_mS ) )
   {
      *pFwMajor_u8     = Register.FwVer.FwMajor_u8;
      *pFwMinor_u8     = Register.FwVer.FwMinor_u8;
      *pFwEngBuild_u16 = Register.FwVer.FwEngBuild_u16;
      retVal = CIM_QUALCODE_SUCCESS;
   }
   else
   {
      INFO_printf( "Unsuccessful DRU FW Version Number reading" );
   }

   return retVal;
}


/***********************************************************************************************************************
   Function name: getDruHwVersion

   Purpose: Requests the DRU Driver to read the DRU HW Version

   Arguments: uint8_t *pHwGeneration_u8 - Location where to store the Hw Generation version number
              uint8_t *pHwRevision_u8   - Location where to store the Hw Revision version number

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 **********************************************************************************************************************/
static enum_CIM_QualityCode getDruHwVersion( uint8_t *pHwGeneration_u8, uint8_t *pHwRevision_u8 )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   ILC_DRU_DRIVER_druRegister_u Register;

   if ( CIM_QUALCODE_SUCCESS == ILC_DRU_DRIVER_readDruRegister( &Register, ILC_DRU_DRIVER_REG_HW_VERSION, ILC_SRFN_REG_READ_REGISTER_TIME_OUT_mS ) )
   {
      *pHwGeneration_u8  = Register.HwVer.HwGeneration_u8;
      *pHwRevision_u8 = Register.HwVer.HwRevision_u8;
      retVal = CIM_QUALCODE_SUCCESS;
   }
   else
   {
      INFO_printf( "Unsuccessful DRU HW Version Number reading" );
   }

   return retVal;
}


/***********************************************************************************************************************
   Function name: getDruModel

   Purpose: Requests the DRU Driver to read the DRU Model

   Arguments: uint8_t *pEdType_u8  - Location where to store the Type result
              uint8_t *pEdModel_u8 - Location where to store the Model result

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 **********************************************************************************************************************/
static enum_CIM_QualityCode getDruModel( uint8_t *pEdType_u8, uint8_t *pEdModel_u8 )
{
   enum_CIM_QualityCode retVal;
   ILC_DRU_DRIVER_druRegister_u Register;

   retVal = ILC_DRU_DRIVER_readDruRegister( &Register, ILC_DRU_DRIVER_REG_MODEL, ILC_SRFN_REG_READ_REGISTER_TIME_OUT_mS );
   if ( retVal == CIM_QUALCODE_SUCCESS )
   {
      *pEdType_u8  = Register.ModNum.RceType_u8;
      *pEdModel_u8 = Register.ModNum.RceModel_u8;
   }
   else
   {
      INFO_printf( "Unsuccessful DRU Model reading" );
   }

   return retVal;
}


/***********************************************************************************************************************
   Function name: getDruInfo

   Purpose: Makes the requests to the DRU Driver to read all the DRU Registers needed for SRFN Registration and it
            stores their values in the DRU Information Structure

   Arguments: ILC_DRU_DRIVER_druInfo_t *pDest - Location where to store all the DRU Information (i.e. DRU Registers'
                                                values for SRFN Registration)

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 **********************************************************************************************************************/
static enum_CIM_QualityCode getDruInfo ( ILC_DRU_DRIVER_druInfo_t *pDest )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   if ( CIM_QUALCODE_SUCCESS == getDruSerialNumber( &pDest->SerialNumber_u32 ) )
   {
      if ( CIM_QUALCODE_SUCCESS == getDruHwVersion( &pDest->HwVer.HwGeneration_u8, &pDest->HwVer.HwRevision_u8 ) )
      {
         if ( CIM_QUALCODE_SUCCESS == getDruFwVersion( &pDest->FwVer.FwMajor_u8, &pDest->FwVer.FwMinor_u8,
                                                         &pDest->FwVer.FwEngBuild_u16 ) )
         {
            if ( CIM_QUALCODE_SUCCESS == getDruModel( &pDest->ModNum.RceType_u8, &pDest->ModNum.RceModel_u8 ) )
            {
               retVal = CIM_QUALCODE_SUCCESS;
            }
         }
      }
   }

   return retVal;
}


/***********************************************************************************************************************
  Function name: ILC_SRFN_REG_getEdMfgSerNum

  Purpose: Gets the DRU Serial Number from the DRU Information Structure

  Arguments: uint32_t *pSerialNumber_u32 - Location where to store the DRU Serial Number

  Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 **********************************************************************************************************************/
enum_CIM_QualityCode ILC_SRFN_REG_getEdMfgSerNum( ValueInfo_t *reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_CODE_INDETERMINATE;
   uint32_t serialNumber_u32 = 0;
   uint8_t numberOfBytesWritten = 0;

   if( CIM_QUALCODE_SUCCESS == getDruSerialNumber( &serialNumber_u32 ) )
   {
      numberOfBytesWritten = (uint8_t)snprintf ( (char *)reading->Value.buffer, sizeof(reading->Value.buffer),
                                                  "%u", serialNumber_u32 );
      retVal = CIM_QUALCODE_SUCCESS;
   }

   reading->typecast = ASCIIStringValue;
   reading->valueSizeInBytes = numberOfBytesWritten;
   reading->readingType = edMfgSerialNumber;

   return retVal;
}


/***********************************************************************************************************************
  Function name: ILC_SRFN_REG_getEdHwVerNum

  Purpose: Gets the DRU HW Version Number from the DRU Information Structure

  Arguments: uint8_t *pHwGenerationNumber_u8 - Location where to store the HW Generation Number
             uint8_t *pHwRevisionNumber_u8   - Location where to store the HW Revision Number

  Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 **********************************************************************************************************************/
enum_CIM_QualityCode ILC_SRFN_REG_getEdHwVerNum( ValueInfo_t *reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_CODE_INDETERMINATE;
   uint8_t hwGenerationNumber_u8 = 0;
   uint8_t hwRevisionNumber_u8 = 0;
   uint8_t numberOfBytesWritten = 0;

   if( CIM_QUALCODE_SUCCESS == getDruHwVersion( &hwGenerationNumber_u8, &hwRevisionNumber_u8 ) )
   {
      numberOfBytesWritten = (uint8_t)snprintf ( (char *)reading->Value.buffer, sizeof(reading->Value.buffer),
                                                  "%hhu.%hhu", hwGenerationNumber_u8, hwRevisionNumber_u8 );
      retVal = CIM_QUALCODE_SUCCESS;
   }

   reading->typecast = ASCIIStringValue;
   reading->valueSizeInBytes = numberOfBytesWritten;
   reading->readingType = edHwVersion;

   return retVal;
}


/***********************************************************************************************************************
  Function name: ILC_SRFN_REG_getEdFwVerNum

  Purpose: Gets the DRU FW Version Number from the DRU Information Structure

  Arguments: uint8_t *pFwMajorNumber_u8   - Location where to store the FW Major Number
             uint8_t *pFwMinorNumber_u8   - Location where to store the FW Minor Number
             uint16_t *pFwBuildNumber_u16 - Location where to store the FW Build Number

  Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 **********************************************************************************************************************/
enum_CIM_QualityCode ILC_SRFN_REG_getEdFwVerNum( ValueInfo_t *reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_CODE_INDETERMINATE;
   uint8_t fwMajorNumber_u8 = 0;
   uint8_t fwMinorNumber_u8 = 0;
   uint16_t fwBuildNumber_u16 = 0;
   uint8_t numberOfBytesWritten = 0;

   if( CIM_QUALCODE_SUCCESS == getDruFwVersion( &fwMajorNumber_u8, &fwMinorNumber_u8, &fwBuildNumber_u16 ) )
   {
      numberOfBytesWritten = (uint8_t)snprintf ( (char *)reading->Value.buffer, sizeof(reading->Value.buffer),
                                                  "%hhu.%hhu.%hu", fwMajorNumber_u8, fwMinorNumber_u8, fwBuildNumber_u16 );
      retVal = CIM_QUALCODE_SUCCESS;
   }

   reading->typecast = ASCIIStringValue;
   reading->valueSizeInBytes = numberOfBytesWritten;
   reading->readingType = edFwVersion;

   return retVal;
}


/***********************************************************************************************************************
  Function name: ILC_SRFN_REG_getEdModel

  Purpose: Gets the DRU Model from the DRU Information Structure

  Arguments: uint8_t *pRceTypeNumber_u8  - Location where to store the RCE Type Number
             uint8_t *pRceModelNumber_u8 - Location where to store the RCE Model Number

  Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 **********************************************************************************************************************/
enum_CIM_QualityCode ILC_SRFN_REG_getEdModel( ValueInfo_t *reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_CODE_INDETERMINATE;
   uint8_t rceTypeNumber_u8 = 0;
   uint8_t rceModelNumber_u8 = 0;
   uint8_t numberOfBytesWritten = 0;

   if( CIM_QUALCODE_SUCCESS == getDruModel( &rceTypeNumber_u8, &rceModelNumber_u8 ) )
   {
      numberOfBytesWritten = (uint8_t)snprintf ( (char *)reading->Value.buffer, sizeof(reading->Value.buffer),
                                                  "%hhu.%hhu", rceTypeNumber_u8, rceModelNumber_u8 );
      retVal = CIM_QUALCODE_SUCCESS;
   }

   reading->typecast = ASCIIStringValue;
   reading->valueSizeInBytes = numberOfBytesWritten;
   reading->readingType = edModel;

   return retVal;
}


/* *********************************************************************************************************************
  Function name: ILC_SRFN_REG_Init

  Purpose: Initialize the DRU's Information Structure

  Arguments: None

  Returns: eSUCCESS/eFAILURE indication
 **********************************************************************************************************************/
returnStatus_t ILC_SRFN_REG_Init ( void )
{
   returnStatus_t retVal = eSUCCESS;   /* retVal is returned to keep consistency with other Init functions */

   DruSrfnRegistrationInfo.SerialNumber_u32 = 0;
   DruSrfnRegistrationInfo.HwVer.HwGeneration_u8 = 0;
   DruSrfnRegistrationInfo.HwVer.HwRevision_u8 = 0;
   DruSrfnRegistrationInfo.FwVer.FwMajor_u8 = 0;
   DruSrfnRegistrationInfo.FwVer.FwMinor_u8 = 0;
   DruSrfnRegistrationInfo.FwVer.FwEngBuild_u16 = 0;
   DruSrfnRegistrationInfo.ModNum.RceType_u8 = 0;
   DruSrfnRegistrationInfo.ModNum.RceModel_u8 = 0;
   DruSrfnRegistrationInfo.ValidData_b = (bool)false;

   return retVal;
}


/***********************************************************************************************************************
  Function name: ILC_SRFN_REG_Task

  Purpose: Provides the SRFN Registration Process with the DRU Registers it needs

  Arguments: uint32_t Arg0 - Not used, but required here by MQX because this is a task

  Returns: None
 **********************************************************************************************************************/
void ILC_SRFN_REG_Task( uint32_t Arg0 )
{
   OS_TASK_Sleep( ONE_SEC * 5);  /* Wait for the DRU to initialize in case SRFN EP and DRU are powered-up at the same
                                    time */

   while ( !DruSrfnRegistrationInfo.ValidData_b )
   {
      if ( CIM_QUALCODE_SUCCESS == getDruInfo( &DruSrfnRegistrationInfo ) )
      {
         DruSrfnRegistrationInfo.ValidData_b = (bool)true;
         INFO_printf( "Registers successfully read from DRU" );
      }
      else
      {
         INFO_printf( "Registers unsuccessfully read from DRU, retrying in 10 seconds" );
         OS_TASK_Sleep( ONE_SEC * 10 );
      }
   }

   /* Once the DRU Registers' values needed for SRFN Registration are read and stored in the DRU Information Structure
      (DruSrfnRegistrationInfo) kill ILC_SRFN_REG_Task, since any function that needs to read those DRU Registers'
      values will do it from the DRU Information Structure, i.e. not from the DRU */
   OS_TASK_Exit();
}

/***********************************************************************************************************************
  Function name: ILC_SRFN_REG_getReading

  Purpose: Gets readings specific to the ILC

  Arguments: meterReadingType RdgType  - Id value to indicate which reading is being requested
             ValueInfo_t *reading - poiner to a readomg data structure to store information about the reading

  Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 **********************************************************************************************************************/
enum_CIM_QualityCode ILC_SRFN_REG_getReading( meterReadingType RdgType, ValueInfo_t *reading )
{

   enum_CIM_QualityCode retVal;

   switch ( RdgType )
   {
      case edFwVersion:
      {  // retrieve edFwVersion reading type from ILC
         retVal = ILC_SRFN_REG_getEdFwVerNum( reading );
         break;
      }
      case edHwVersion:
      {  // retrieve edHwVersion reading type from ILC
         retVal = ILC_SRFN_REG_getEdHwVerNum( reading );
         break;
      }
      case edMfgSerialNumber:
      {  // retrieve edMfgSerialNumber reading type from ILC
         retVal = ILC_SRFN_REG_getEdMfgSerNum( reading );
         break;
      }
      case edModel:
      {  // retrieve edModel reading type from ILC
         retVal = ILC_SRFN_REG_getEdModel( reading );
         break;
      }
      default:
      {  // reading type requested not supported
         retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
         break;
      }
   } /*lint !e788 not all enumbs used in switch   */

   return retVal;
}

/* End of file */
