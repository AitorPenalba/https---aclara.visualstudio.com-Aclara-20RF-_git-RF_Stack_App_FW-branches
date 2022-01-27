/***********************************************************************************************************************
 *
 * Filename: ed_config.c
 *
 * Global Designator: EDCFG_
 *
 * Contents: Configuration parameter I/O for end device information.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015-2020 Aclara.  All Rights Reserved.
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
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#if ( ACLARA_DA == 0 )
#include <stdio.h>
#endif
#include <string.h>
#if ( HMC_I210_PLUS_C == 1 ) || ( HMC_KV == 1 )
#include <ctype.h>
#endif
#include "compiler_types.h"
#include "file_io.h"
#include "intf_cim_cmd.h"

#define EDCFG_GLOBALS
#include "ed_config.h"
#undef EDCFG_GLOBALS

#if ( ANSI_STANDARD_TABLES == 1 )
#include "intf_cim_cmd.h"
#endif

#if (ACLARA_DA == 1)
#include "da_srfn_reg.h"
#endif

#include "DBG_SerialDebug.h"

#if ( ACLARA_LC == 1 )
#include "ilc_srfn_reg.h"
#endif

#include "time_util.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */


/* ****************************************************************************************************************** */
/* CONSTANTS */

#define EDCFG_FILE_UPDATE_RATE_CFG_SEC ((uint32_t) 30 * 24 * 3600)    /* File updates once per month. */

#define EDCFG_FILE_VERSION 1
#define EDCFG_UTIL_FILE_VERSION 1
#if( SAMPLE_METER_TEMPERATURE == 1 )
#define DEFAULT_ED_TEMPERATURE_HYSTERISIS (uint8_t)2 /* The hysteresis (in degrees C) to be subtracted from a maximum temperature */
#define DEFAULT_ED_TEMPERATURE_SAMPLE_RATE (uint16_t)300 /* time in seconds to sample the meters thermometer */
#endif

#if ( HMC_I210_PLUS_C == 1 ) || ( HMC_KV == 1 )
#if ( HMC_I210_PLUS_C == 1 )
#define STD_TABLE_2                       2
#endif
#define ADD_ASCII_COMMA                   ","
#define ADD_ASCII_COMMA_CL                ",CL"

#if ( HMC_I210_PLUS_C == 1 )
#define E_BASE_TYPE_MASK                  0x3C0
#define E_BASE_TYPE_SHIFT                 6
#endif

#if ( HMC_I210_PLUS_C == 1 )
#define MAX_BUFFER_SIZE_WORST_CASE        50
#else
#define MAX_BUFFER_SIZE_WORST_CASE        31
#endif

// These line up with R&P guide Std Tbl 2 E_BASE_TYPE field
#define BASE_TYPE_NONE                    "none"
#define BASE_TYPE_S                       "S"
#define BASE_TYPE_A                       "A"
#define BASE_TYPE_K                       "K"
#define BASE_TYPE_IEC_BOTTOM_CONNECTED    "IEC bottom-connected"
#define BASE_TYPE_SWITCHBOARD             "switchboard"
#define BASE_TYPE_RACKMOUNT               "rackmount"
#define BASE_TYPE_B                       "B"
#define BASE_TYPE_P                       "P"
#define BASE_TYPE_UNDEFINED               9      // SRFNI-210+C uses values from 0 through 8

// These line up with R&P guide Std Tbl 2 E_ELEMENT_VOLTS field
#define E_ELEMENT_VOLTS_CASE_0            BASE_TYPE_NONE
#define E_ELEMENT_VOLTS_CASE_1            "69.3V"
#define E_ELEMENT_VOLTS_CASE_2            "72V"
#define E_ELEMENT_VOLTS_CASE_3            "120V"
#define E_ELEMENT_VOLTS_CASE_4            "208V"
#define E_ELEMENT_VOLTS_CASE_5            "240V"
#define E_ELEMENT_VOLTS_CASE_6            "277V"
#define E_ELEMENT_VOLTS_CASE_7            "480V"
#define E_ELEMENT_VOLTS_CASE_8            "120-277V"
#define E_ELEMENT_VOLTS_CASE_9            "120-480V"
#define E_ELEMENT_VOLTS_CASE_10           "57-120V"
#define E_ELEMENT_VOLTS_CASE_11           "600V"
#define E_ELEMENT_VOLTS_CASE_12           "100V"
#define E_ELEMENT_VOLTS_CASE_13           "200V"
#define E_ELEMENT_VOLTS_CASE_14           "255V"
#define E_ELEMENT_VOLTS_UNDEFINED         15      // SRFNI-210+C uses values from 0 through 14


static const char* const   baseTypeArray[] =    {  BASE_TYPE_NONE,
                                                   BASE_TYPE_S,
                                                   BASE_TYPE_A,
                                                   BASE_TYPE_K,
                                                   BASE_TYPE_IEC_BOTTOM_CONNECTED,
                                                   BASE_TYPE_SWITCHBOARD,
                                                   BASE_TYPE_RACKMOUNT,
                                                   BASE_TYPE_B,
                                                   BASE_TYPE_P };
static const char* const   elementVoltsArray[] = { E_ELEMENT_VOLTS_CASE_0,
                                                   E_ELEMENT_VOLTS_CASE_1,
                                                   E_ELEMENT_VOLTS_CASE_2,
                                                   E_ELEMENT_VOLTS_CASE_3,
                                                   E_ELEMENT_VOLTS_CASE_4,
                                                   E_ELEMENT_VOLTS_CASE_5,
                                                   E_ELEMENT_VOLTS_CASE_6,
                                                   E_ELEMENT_VOLTS_CASE_7,
                                                   E_ELEMENT_VOLTS_CASE_8,
                                                   E_ELEMENT_VOLTS_CASE_9,
                                                   E_ELEMENT_VOLTS_CASE_10,
                                                   E_ELEMENT_VOLTS_CASE_11,
                                                   E_ELEMENT_VOLTS_CASE_12,
                                                   E_ELEMENT_VOLTS_CASE_13,
                                                   E_ELEMENT_VOLTS_CASE_14 };
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uint16_t fileVersion;
#if( SAMPLE_METER_TEMPERATURE == 1 )
   uint16_t edTempSampleRate;
#endif
#if ( HMC_KV == 1 ) || ( HMC_I210_PLUS == 1 ) || ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 )
   char strEdInfo[EDCFG_INFO_LENGTH+1];
#endif
#if ( ANSI_STANDARD_TABLES == 0 )
   char strEdMfgSerialNumber[EDCFG_MFG_SERIAL_NUMBER_LENGTH+1];
#endif
#if( SAMPLE_METER_TEMPERATURE == 1 )
   uint8_t edTemperatureHystersis;
#endif
} edConfigFileData_t;

typedef struct
{
   uint16_t fileVersion;
   char strEdUtilSerialNumber[EDCFG_UTIL_SERIAL_NUMBER_LENGTH+1];
} edConfigUtilFileData_t;

#if ( HMC_I210_PLUS_C == 1 )
// Standard Table 2
PACK_BEGIN
typedef struct
{
      uint8_t    E_Kh[6];
      uint8_t    E_Kt[6];
      uint8_t    E_InputScalar;
      uint8_t    E_EdConfig[5];
      uint16_t   E_elementsByte;
      struct
      {
        uint8_t E_ElementVolts  :4;
        uint8_t E_EdSupplyVolts :4;
      }E_VoltsBitFld_t;
      uint8_t    E_ClassMaxAmps[6];
      uint8_t    E_Ta[6];
}stdTbl2_values_t;
PACK_END
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static FileHandle_t edConfigFileHandle = {0};
static edConfigFileData_t edConfigFileData = {0};
static OS_MUTEX_Obj edConfigMutex;

static FileHandle_t edConfigUtilFileHandle = {0};
static edConfigUtilFileData_t edConfigUtilFileData = {0};
static OS_MUTEX_Obj edConfigUtilMutex;

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */


/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#if ( HMC_KV == 1 ) || ( HMC_I210_PLUS == 1 ) || ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 )
static returnStatus_t EDCFG_write(void);
#endif
static returnStatus_t EDCFG_util_write(void);


/***********************************************************************************************************************
 *
 * Function name: EDCFG_init
 *
 * Purpose: Create and initialize or open and read end device configuration.
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

returnStatus_t EDCFG_init(void)
{
   FileStatus_t   fileStatus;
   returnStatus_t retVal = eFAILURE;

   if ((OS_MUTEX_Create(&edConfigMutex)) && (OS_MUTEX_Create(&edConfigUtilMutex)))
   {
      if (eSUCCESS == FIO_fopen(&edConfigFileHandle, ePART_SEARCH_BY_TIMING, (uint16_t)eFN_EDCFG, (lCnt)sizeof(edConfigFileData),
                                FILE_IS_NOT_CHECKSUMED, EDCFG_FILE_UPDATE_RATE_CFG_SEC, &fileStatus))
      {
         if (fileStatus.bFileCreated)
         {
            (void)memset((void*) &edConfigFileData, 0, sizeof(edConfigFileData));
            edConfigFileData.fileVersion = EDCFG_FILE_VERSION;
#if( SAMPLE_METER_TEMPERATURE == 1 )
            edConfigFileData.edTempSampleRate = DEFAULT_ED_TEMPERATURE_SAMPLE_RATE;
            edConfigFileData.edTemperatureHystersis = DEFAULT_ED_TEMPERATURE_HYSTERISIS;
#endif
            retVal = FIO_fwrite(&edConfigFileHandle, 0, (uint8_t*) &edConfigFileData, (lCnt)sizeof(edConfigFileData));
         }
         else
         {
            retVal = FIO_fread(&edConfigFileHandle, (uint8_t*) &edConfigFileData, 0, (lCnt)sizeof(edConfigFileData));
         }
      }


      if (eSUCCESS == retVal)
      {
         if (eSUCCESS == FIO_fopen(&edConfigUtilFileHandle, ePART_SEARCH_BY_TIMING, (uint16_t)eFN_EDCFGU, (lCnt)sizeof(edConfigUtilFileData),
                                   FILE_IS_NOT_CHECKSUMED, EDCFG_FILE_UPDATE_RATE_CFG_SEC, &fileStatus))
         {
            if (fileStatus.bFileCreated)
            {
               (void)memset((void*) &edConfigUtilFileData, 0, sizeof(edConfigUtilFileData));
               edConfigUtilFileData.fileVersion = EDCFG_UTIL_FILE_VERSION;
               retVal = FIO_fwrite(&edConfigUtilFileHandle, 0, (uint8_t*) &edConfigUtilFileData, (lCnt)sizeof(edConfigUtilFileData));
            }
            else
            {
               retVal = FIO_fread(&edConfigUtilFileHandle, (uint8_t*) &edConfigUtilFileData, 0, (lCnt)sizeof(edConfigUtilFileData));
            }
         }
      }
   }

   return(retVal);
}


/***********************************************************************************************************************
 *
 * Function name: EDCFG_get_info
 *
 * Purpose: Get edInfo from End Device configuration.
 *
 * Arguments: char* pStringBuffer -- Destination
 *            uint8_t uBufferSize
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t  EDCFG_get_info(char* pStringBuffer, uint8_t uBufferSize)
{
#if ( HMC_I210_PLUS == 1 ) || ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 )
   returnStatus_t retVal;
   uint8_t uCopyLength = EDCFG_INFO_LENGTH+1;

   (void)memset((void*) pStringBuffer, 0, uBufferSize);

   if (uBufferSize < uCopyLength)
   {
      uCopyLength = uBufferSize;
   }

   OS_MUTEX_Lock(&edConfigMutex); // Function will not return if it fails
   (void)memcpy((void*) pStringBuffer, (void*) edConfigFileData.strEdInfo, uCopyLength);
   OS_MUTEX_Unlock(&edConfigMutex); // Function will not return if it fails

   retVal = eSUCCESS;              // Always success for SRFNI-210+ and SRFN-KV2C
#elif ( HMC_KV == 1 )
   returnStatus_t retVal = eFAILURE;

   mfgTbl87_t mfgTable87Read;
   char ansiFormRead[ANSI_FORM_SIZE];

   if (uBufferSize >= MAX_BUFFER_SIZE_WORST_CASE)
   {
      if(CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( ansiFormRead, MFG_TBL_ELECTRICAL_SERVICE_CONFIG,
                                                        (uint32_t)offsetof( mfgTbl86_t, ansiForm[0] ), sizeof(ansiFormRead)))
      {
         if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &mfgTable87Read, MFG_TBL_ELECTRICAL_SERVICE_STATUS, 0, sizeof(mfgTable87Read)) )
         {
            //Get the form Number [Might be alphabet or number]
            for (uint8_t i = 0, j = 0; i < sizeof(ansiFormRead); i++)
            {
               if( isalnum( ansiFormRead[i] ))
               {
                  pStringBuffer[j++] = ansiFormRead[i];
               }
               pStringBuffer[j] = '\0';
            }

            // E_BASE_TYPE is defined
            if (mfgTable87Read.meterBase < BASE_TYPE_UNDEFINED)
            {
               (void)strcat(pStringBuffer, baseTypeArray[mfgTable87Read.meterBase]); // Add the base type to the buffer

               (void)strcat(pStringBuffer, ADD_ASCII_COMMA_CL);                //Add ",CL" before next field

               char classAmp[6];
               (void)memset((uint8_t *)classAmp,0,sizeof(classAmp));
               (void)snprintf ( (char *)classAmp, sizeof(classAmp),"%u", mfgTable87Read.maxClassAmps );
               (void)strcat(pStringBuffer, classAmp);
               (void)strcat(pStringBuffer, ADD_ASCII_COMMA); //Add ",CL" before next field

               // If E_ElementVolts is defined
               if(mfgTable87Read.elementVoltageCode < E_ELEMENT_VOLTS_UNDEFINED )
               {
                  (void)strcat(pStringBuffer, elementVoltsArray[mfgTable87Read.elementVoltageCode]); // Add Element Volts to the buffer
                  retVal = eSUCCESS;
               }
               else
               {
                  DBG_logPrintf( 'R', "Parsing the E_ElementVolts failed" );
                  (void)strcpy(pStringBuffer,"");    /* Return nothing if the reading is failed */
               }
            }
            else
            {
               DBG_logPrintf( 'R', "Parsing the Base Type failed" );
               (void)strcpy(pStringBuffer,"");       // Return nothing if the reading is failed
            }
         }
         else
         {
            DBG_logPrintf( 'R', "Failed to read Manuf Table 87" );
            (void)strcpy(pStringBuffer,"");       // Return nothing if the reading is failed
         }
      }
      else
      {
         DBG_logPrintf( 'R', "Failed to read Manuf Table 86" );
         (void)strcpy(pStringBuffer,"");       // Return nothing if the reading is failed
      }
   }
   else
   {
      DBG_logPrintf( 'R', "BUFFER OVERFLOW ERROR" );
      (void)strcpy(pStringBuffer,"");       // Return nothing if the reading is failed
   }
#else   // for SRFNI-210+c
   returnStatus_t retVal = eFAILURE;

   if (uBufferSize >= MAX_BUFFER_SIZE_WORST_CASE)
   {
      uint8_t                   tblOffset = (uint8_t)offsetof(stdTbl2_values_t, E_EdConfig[0] );                    // Find the offset of E_EdConfig[0] in Std table 2
      uint8_t                   totalBytes = (uint8_t)(offsetof(stdTbl2_values_t, E_ClassMaxAmps[5] ) - tblOffset);   /* Running count of bytes read, decreasing  and Total bytes to be read */
      uint8_t                   bytesToRead;              /* Bytes requested each pass. */
      uint8_t                   i;
      uint8_t                   baseType = 0;
      enum_CIM_QualityCode      qualityCode = CIM_QUALCODE_ERROR_CODE;
      stdTbl2_values_t          stdTable2 = {0};

      /* Loop until requested number of bytes have been read, or timed out. */
      while ( totalBytes != 0 )
      {

         bytesToRead = min( totalBytes, HMC_CMD_MSG_MAX );
         qualityCode = INTF_CIM_CMD_ansiRead( &stdTable2.E_EdConfig[0], STD_TABLE_2, tblOffset, bytesToRead);
         if ( CIM_QUALCODE_SUCCESS != qualityCode )
         {
            DBG_logPrintf( 'R', "ANSI read failed" );
            (void)strcpy(pStringBuffer,"");    /* Return nothing if the reading is failed */
            break;                       /* Exit loop on failure.  */
         }

         totalBytes -= bytesToRead;
         tblOffset += bytesToRead;
      }
      if( CIM_QUALCODE_SUCCESS == qualityCode )
      {
         uint8_t j =0;
         //Get the form Number [Might be alphabet or number]
         for (i = 0; i < sizeof(stdTable2.E_EdConfig); i++)
         {
            if( isalnum( stdTable2.E_EdConfig[i] ))
            {
               pStringBuffer[j++] = stdTable2.E_EdConfig[i];
            }
            pStringBuffer[j] = '\0';
         }

         baseType = (stdTable2.E_elementsByte & E_BASE_TYPE_MASK) >> E_BASE_TYPE_SHIFT;

         // E_BASE_TYPE is defined
         if (baseType < BASE_TYPE_UNDEFINED)
         {
            (void)strcat(pStringBuffer, baseTypeArray[baseType]);           // Add the base type to the buffer
            (void)strcat(pStringBuffer, ADD_ASCII_COMMA_CL);                //Add ",CL" before next field
            (void)strcat(pStringBuffer, (char *)stdTable2.E_ClassMaxAmps);  // Add the ClassMaxAmps to the buffer
            (void)strcat(pStringBuffer, ADD_ASCII_COMMA);

            // If E_ElementVolts is defined
            if(stdTable2.E_VoltsBitFld_t.E_ElementVolts < E_ELEMENT_VOLTS_UNDEFINED )
            {
               (void)strcat(pStringBuffer, elementVoltsArray[stdTable2.E_VoltsBitFld_t.E_ElementVolts]);     // Add Element Volts to the buffer
               retVal = eSUCCESS;
            }
            else
            {
               DBG_logPrintf( 'R', "Parsing the E_ElementVolts failed" );
               (void)strcpy(pStringBuffer,"");    /* Return nothing if the reading is failed */
            }
         }
         else
         {
            DBG_logPrintf( 'R', "Parsing the Base Type failed" );
            (void)strcpy(pStringBuffer,"");       // Return nothing if the reading is failed
         }
      }
   }
   else
   {
      DBG_logPrintf( 'R', "BUFFER OVERFLOW ERROR" );
      (void)strcpy(pStringBuffer,"");       // Return nothing if the reading is failed
   }

#endif

   return retVal;
}


/***********************************************************************************************************************
 *
 * Function name: EDCFG_get_mfg_serial_number
 *
 * Purpose: Get edMfgSerialNumber from End Device configuration.
 *
 * Arguments: char* pStringBuffer
 *            uint8_t uBufferSize
 *
 * Returns: char* pStringBuffer
 *
 * Re-entrant Code: No
 *
 * Notes: For the kV2c it is read from the meter.
 *
 **********************************************************************************************************************/

char* EDCFG_get_mfg_serial_number(char* pStringBuffer, uint8_t uBufferSize)
{
#if ( ANSI_STANDARD_TABLES == 0 ) && ( ACLARA_LC == 0)  && ( ACLARA_DA == 0)
   uint8_t uCopyLength = EDCFG_MFG_SERIAL_NUMBER_LENGTH+1;

   (void)memset((void*) pStringBuffer, 0, uBufferSize);

   if (uBufferSize < uCopyLength)
   {
      uCopyLength = uBufferSize;
   }
   OS_MUTEX_Lock(&edConfigMutex); // Function will not return if it fails
   (void)memcpy((void*) pStringBuffer, (void*) edConfigFileData.strEdMfgSerialNumber, uCopyLength);
   OS_MUTEX_Unlock(&edConfigMutex); // Function will not return if it fails
#else
   ValueInfo_t readingInfo;
   if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( edMfgSerialNumber, &readingInfo ) )
   {
      if( readingInfo.valueSizeInBytes < uBufferSize )
      {
         (void)memset(pStringBuffer, 0, uBufferSize );
         (void)memcpy( pStringBuffer, readingInfo.Value.buffer, readingInfo.valueSizeInBytes);
      }
   }
#endif

   return(pStringBuffer);
}


/***********************************************************************************************************************
 *
 * Function name: EDCFG_get_util_serial_number
 *
 * Purpose: Get edUtilitySerialNumber from End Device configuration.
 *
 * Arguments: char* pStringBuffer
 *            uint8_t uBufferSize
 *
 * Returns: char* pStringBuffer
 *
 * Re-entrant Code: No
 *
 * Notes: For the kV2c it is read from the meter.
 *
 **********************************************************************************************************************/

char* EDCFG_get_util_serial_number(char* pStringBuffer, uint8_t uBufferSize)
{
   uint8_t uCopyLength = EDCFG_UTIL_SERIAL_NUMBER_LENGTH+1;

   (void)memset((void*) pStringBuffer, 0, uBufferSize);

   if (uBufferSize < uCopyLength)
   {
      uCopyLength = uBufferSize;
   }

   OS_MUTEX_Lock(&edConfigUtilMutex); // Function will not return if it fails
   (void)memcpy((void*) pStringBuffer, (void*) edConfigUtilFileData.strEdUtilSerialNumber, uCopyLength);
   OS_MUTEX_Unlock(&edConfigUtilMutex); // Function will not return if it fails

   return(pStringBuffer);
}

#if ( HMC_KV == 1 ) || ( HMC_I210_PLUS == 1 ) || ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 )
/***********************************************************************************************************************
 *
 * Function name: EDCFG_set_info
 *
 * Purpose: Set strEdInfo in End Device configuration.
 *
 * Arguments: char* strEdInfo
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
/*lint -esym(818,pString)  could be declared as pointer to const  */

returnStatus_t EDCFG_set_info(char* pString)
{
   returnStatus_t retVal;
   uint8_t uCopyLength = EDCFG_INFO_LENGTH;
   uint8_t uStringLength;

   uStringLength = (uint8_t)strlen(pString);

   if (uCopyLength > uStringLength)
   {
      uCopyLength = uStringLength;
   }

   OS_MUTEX_Lock(&edConfigMutex); // Function will not return if it fails
   (void)memset((void*) edConfigFileData.strEdInfo, 0, sizeof(edConfigFileData.strEdInfo));
   (void)memcpy((void*) edConfigFileData.strEdInfo, pString, uCopyLength);

   retVal = EDCFG_write();
   OS_MUTEX_Unlock(&edConfigMutex); // Function will not return if it fails

   return(retVal);
}
#endif

#if ( ANSI_STANDARD_TABLES == 0 )
/***********************************************************************************************************************
 *
 * Function name: EDCFG_set_mfg_serial_number
 *
 * Purpose: Set edMfgSerialNumber in End Device configuration.
 *
 * Arguments: char* strEdMfgSerialNumber
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes: MFG serial number can only be set for the I210+.  For the kV2c it is read from the meter.
 *
 **********************************************************************************************************************/

returnStatus_t EDCFG_set_mfg_serial_number(char* pString)
{
   returnStatus_t retVal;
   uint8_t uCopyLength = EDCFG_MFG_SERIAL_NUMBER_LENGTH;
   uint8_t uStringLength;

   uStringLength = (uint8_t)strlen(pString);

   if (uCopyLength > uStringLength)
   {
      uCopyLength = uStringLength;
   }

   OS_MUTEX_Lock(&edConfigMutex); // Function will not return if it fails
   (void)memset((void*) edConfigFileData.strEdMfgSerialNumber, 0, sizeof(edConfigFileData.strEdMfgSerialNumber));
   (void)memcpy((void*) edConfigFileData.strEdMfgSerialNumber, pString, uCopyLength);

   retVal = EDCFG_write();
   OS_MUTEX_Unlock(&edConfigMutex); // Function will not return if it fails

   return(retVal);
}
#endif
#if( SAMPLE_METER_TEMPERATURE == 1 )
/***********************************************************************************************************************
 *
 * Function name: EDCFG_setEdTemperatureHystersis
 *
 * Purpose: Set edTemperatureHystersis parameter
 *
 * Arguments: uint8_t uEdTemperatureHystersis
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/

returnStatus_t EDCFG_setEdTemperatureHystersis(uint8_t uEdTemperatureHystersis)
{
   returnStatus_t retVal = eFAILURE;

   OS_MUTEX_Lock(&edConfigMutex); // Function will not return if it fails
   edConfigFileData.edTemperatureHystersis = uEdTemperatureHystersis;
   retVal = EDCFG_write();
   OS_MUTEX_Unlock(&edConfigMutex); // Function will not return if it fails

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: EDCFG_getEdTemperatureHystersis
 *
 * Purpose: Get edTemperatureHystersis parameter
 *
 * Arguments: none
 *
 * Returns: uint8_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint8_t EDCFG_getEdTemperatureHystersis(void)
{
   uint8_t uEdTemperatureHystersis;

   OS_MUTEX_Lock(&edConfigMutex); // Function will not return if it fails
   uEdTemperatureHystersis = edConfigFileData.edTemperatureHystersis;
   OS_MUTEX_Unlock(&edConfigMutex); // Function will not return if it fails

   return( uEdTemperatureHystersis );
}
/***********************************************************************************************************************
 *
 * Function name: EDCFG_setEdTempSampleRate
 *
 * Purpose: Set edTempSampleRate parameter
 *
 * Arguments: uint16_t uEdTemperatureHystersis
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/

returnStatus_t EDCFG_setEdTempSampleRate(uint16_t uEdTempSampleRate)
{
   returnStatus_t retVal = eFAILURE;

   OS_MUTEX_Lock(&edConfigMutex); // Function will not return if it fails
   edConfigFileData.edTempSampleRate = uEdTempSampleRate;
   retVal = EDCFG_write();
   OS_MUTEX_Unlock(&edConfigMutex); // Function will not return if it fails

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: EDCFG_getEdTempSampleRate
 *
 * Purpose: Get edTempSampleRate parameter
 *
 * Arguments: none
 *
 * Returns: uint16_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint16_t EDCFG_getEdTempSampleRate(void)
{
   uint16_t uEdTempSampleRate;

   OS_MUTEX_Lock(&edConfigMutex); // Function will not return if it fails
   uEdTempSampleRate = edConfigFileData.edTempSampleRate;
   OS_MUTEX_Unlock(&edConfigMutex); // Function will not return if it fails

   return( uEdTempSampleRate );
}
#endif
/***********************************************************************************************************************
 *
 * Function name: EDCFG_set_util_serial_number
 *
 * Purpose: Set edUtilitySerialNumber in End Device configuration.
 *
 * Arguments: char* strEdUtilitySerialNumber
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/

returnStatus_t EDCFG_set_util_serial_number(char const * pString)
{
   returnStatus_t retVal;
   uint8_t uCopyLength = EDCFG_UTIL_SERIAL_NUMBER_LENGTH;
   uint8_t uStringLength;

   uStringLength = (uint8_t)strlen(pString);

   if (uCopyLength > uStringLength)
   {
      uCopyLength = uStringLength;
   }

   OS_MUTEX_Lock(&edConfigUtilMutex); // Function will not return if it fails
   (void)memset((void*) edConfigUtilFileData.strEdUtilSerialNumber, 0, sizeof(edConfigUtilFileData.strEdUtilSerialNumber));
   (void)memcpy((char *) edConfigUtilFileData.strEdUtilSerialNumber, pString, uCopyLength);

   retVal = EDCFG_util_write();
   OS_MUTEX_Unlock(&edConfigUtilMutex); // Function will not return if it fails

   return(retVal);
}
/*lint +esym(818,pString)   */

#if ( HMC_KV == 1 ) || ( HMC_I210_PLUS == 1 ) || ( ACLARA_LC == 1 ) || ( ACLARA_DA == 1 )

/***********************************************************************************************************************
 *
 * Function name: EDCFG_write
 *
 * Purpose: Write end device configuration to flash.
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

static returnStatus_t EDCFG_write(void)
{
   returnStatus_t retVal;

   retVal = FIO_fwrite(&edConfigFileHandle, 0, (uint8_t*) &edConfigFileData, (lCnt)sizeof(edConfigFileData));

   if (eSUCCESS == retVal)
   {
      DBG_logPrintf('I', "eFN_EDCFG Write Successful");
   }
   else
   {
      DBG_logPrintf('E', "eFN_EDCFG Write Failed");
   }

   return(retVal);
}
#endif
/***********************************************************************************************************************
 *
 * Function name: EDCFG_util_write
 *
 * Purpose: Write end device utility configuration to flash.
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

static returnStatus_t EDCFG_util_write(void)
{
   returnStatus_t retVal;

   retVal = FIO_fwrite(&edConfigUtilFileHandle, 0, (uint8_t*) &edConfigUtilFileData, (lCnt)sizeof(edConfigUtilFileData));

   if (eSUCCESS == retVal)
   {
      DBG_logPrintf('I', "eFN_EDCFGU Write Successful");
   }
   else
   {
      DBG_logPrintf('E', "eFN_EDCFGU Write Failed");
   }

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name:  EDCFG_OR_PM_Handler
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
returnStatus_t EDCFG_OR_PM_Handler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr)
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      ValueInfo_t readingInfo;  // declare variable to retrieve reading from end device

      (void)memset( (uint8_t *)&readingInfo, 0, sizeof(readingInfo) );

      switch ( id ) /*lint !e788 not all enums used within switch */
      {
         case edInfo:
         {
            char strEdInfo[EDCFG_INFO_LENGTH + 1];
            (void)memset(strEdInfo, 0, sizeof(strEdInfo));
            if ( EDCFG_INFO_LENGTH <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               retVal = EDCFG_get_info(strEdInfo, EDCFG_INFO_LENGTH);    /* get the parameter value */
               (void)memcpy((char *)value, strEdInfo, sizeof(strEdInfo));
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)strlen((char *)value);
                  attr->rValTypecast = (uint8_t)ASCIIStringValue;
               }
            }
            break;
          }
         case edUtilitySerialNumber:
         {
            char strEdConfigUtilSerialNumber[EDCFG_UTIL_SERIAL_NUMBER_LENGTH + 1];
            (void)memset(strEdConfigUtilSerialNumber, 0, sizeof(strEdConfigUtilSerialNumber));
            if ( EDCFG_UTIL_SERIAL_NUMBER_LENGTH <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               (void)EDCFG_get_util_serial_number(strEdConfigUtilSerialNumber, (uint8_t)sizeof(strEdConfigUtilSerialNumber));
               (void)memcpy((char *)value, strEdConfigUtilSerialNumber, sizeof(strEdConfigUtilSerialNumber));
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)strlen((char *)value);
                  attr->rValTypecast = (uint8_t)ASCIIStringValue;
               }
            }
            break;
         }
         default:
         {
            if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( id, &readingInfo ) )
            {  // we successfully retrieved the value, process it
               if( readingInfo.valueSizeInBytes <= MAX_OR_PM_PAYLOAD_SIZE )
               {  // value will fit into the provided buffer, copy value into destination buffer
                  (void)memcpy(value, readingInfo.Value.buffer, readingInfo.valueSizeInBytes );
                  if (attr != NULL)
                  {
                     attr->rValLen      = readingInfo.valueSizeInBytes; // set reading size in bytes
                     attr->powerOfTen = readingInfo.power10;
                     retVal = eSUCCESS; // set successful return value
                  }
               }
               else
               {  // value will not fit into provided buffer, send back quality code
                  retVal = eAPP_OVERFLOW_CONDITION; // set overflow return status
                  readingInfo.valueSizeInBytes = 0;
               }

               if (attr != NULL)
               {
                  attr->rValTypecast = readingInfo.typecast; // set the typecast value
               }
            }
            break;
         }
      } /*end switch*//*lint !e788 not all enums used within switch */
   }
   else /* Used to "put" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case edUtilitySerialNumber :
         {
            retVal = EDCFG_set_util_serial_number( (char*)value );

            if (retVal != eSUCCESS)
            {
               retVal = eAPP_INVALID_VALUE;
            }
            break;
         }
#if( SAMPLE_METER_TEMPERATURE == 1 )
         case edTempHysteresis :
         {
            retVal = EDCFG_setEdTemperatureHystersis( *(uint8_t *)value );
            break;
         }
         case edTempSampleRate :
         {
            retVal = EDCFG_setEdTempSampleRate( *(uint16_t *)value );
            break;
         }
#endif
         default :
         {
            retVal = eAPP_NOT_HANDLED; // Success/failure
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: EDCFG_getFwVersion
 *
 * Purpose: Get edFwVerwsion from End Device
 *
 * Arguments: ValueInfo_t* reading
 *
 * Returns: returnStatus_t retVal
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode EDCFG_getFwVerNum( ValueInfo_t *reading )
{
   uint8_t numberOfBytesWritten = 0;

#if ( HMC_KV == 1 )
   enum_CIM_QualityCode retVal;
   romConstDataRCD_t    romConstInfo;    // Partial MT0 data structure for base firmware

   /*lint !e838 previous value not used; OK  */
   retVal = INTF_CIM_CMD_ansiRead( &romConstInfo, MFG_TBL_GE_DEVICE_TABLE, (uint32_t)offsetof(mfgTbl0_t,
                                   romConstDataRcd), (uint16_t)sizeof( romConstInfo ) );

   if( CIM_QUALCODE_SUCCESS == retVal )
   {
      numberOfBytesWritten = (uint8_t)snprintf ( (char *)reading->Value.buffer,
                                         sizeof(reading->Value.buffer),
                                         "%u.%u.%u",
                                         romConstInfo.FwVersion,
                                         romConstInfo.FwRevision,
                                         romConstInfo.FwBuild );
   }
#elif ( HMC_I210_PLUS_C == 1 )
   enum_CIM_QualityCode retVal;
   romConstDataRCD_t      romConstInfo;    // Partial MT0 data structure for base firmware
   updateConstants_t     meterPatchInfo;    // Partial MTO data structure for patch firmware

   retVal = INTF_CIM_CMD_ansiRead(&romConstInfo, MFG_TBL_GE_DEVICE_TABLE, (uint32_t)offsetof(mfgTbl0_t, romConstDataRcd),
                                                                               (uint16_t)sizeof( romConstDataRCD_t ) );  //lint !e838
   if( CIM_QUALCODE_SUCCESS == retVal )
   {
      retVal = INTF_CIM_CMD_ansiRead(&meterPatchInfo, MFG_TBL_GE_DEVICE_TABLE, (uint32_t)offsetof(mfgTbl0_t, updateConstants),
                                                                                  (uint16_t)sizeof( updateConstants_t ) );  //lint !e838
      if( CIM_QUALCODE_SUCCESS == retVal )
      {
         numberOfBytesWritten = (uint8_t)snprintf ( (char *)reading->Value.buffer,
                                            sizeof(reading->Value.buffer),
                                            "%u.%u.%u;%u.%u.%u",
                                            romConstInfo.FwVersion,
                                             romConstInfo.FwRevision,
                                            romConstInfo.FwBuild,
                                            meterPatchInfo.updateVer,
                                            meterPatchInfo.updateRev,
                                            meterPatchInfo.updateBuild );
      }
   }
#else
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
#endif

   reading->typecast = ASCIIStringValue;
   reading->valueSizeInBytes = numberOfBytesWritten;
   reading->readingType = edFwVersion;

   return retVal;
}

/***********************************************************************************************************************
 *
 * Function name: EDCFG_getHwVersion
 *
 * Purpose: Get edHwVerwsion from End Device
 *
 * Arguments: ValueInfo_t* reading
 *
 * Returns: returnStatus_t retVal
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode EDCFG_getHwVerNum( ValueInfo_t *reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
#if (HMC_KV == 1 ) || (HMC_I210_PLUS_C == 1)
   pValueInfo_t readingPtr = (pValueInfo_t)reading;
   uint8_t numberOfBytesWritten = 0;
   stdTbl1_t            *pSt1;                           /* Points to ST1 table data */
   if ( eSUCCESS == HMC_MTRINFO_getStdTable1( &pSt1 ) )  /* Successfully retrieved ST1 data? */
   {
      numberOfBytesWritten = (uint8_t)snprintf( (char *)readingPtr->Value.buffer,
                                        sizeof(readingPtr->Value.buffer),
                                        "%d.%d", pSt1->hwVersionNumber, pSt1->hwRevisionNumber );
      retVal = CIM_QUALCODE_SUCCESS;
   }
   readingPtr->valueSizeInBytes = numberOfBytesWritten;  // set number of bytes
   readingPtr->typecast = ASCIIStringValue;              // set the typecast
   readingPtr->readingType = edHwVersion;
#endif
   return retVal;
}

/***********************************************************************************************************************

   Function name: EDCFG_getEdProgrammedDateTime

   Purpose: Gets the DT_Last_PROGRAMMED

   Arguments: uint32_t dateTimeVal to store the value acquired from meter

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
enum_CIM_QualityCode EDCFG_getEdProgrammedDateTime( ValueInfo_t *readingInfo )
{
#if (HMC_KV == 1 ) || (HMC_I210_PLUS_C == 1)
   enum_CIM_QualityCode retVal; //default
   MtrTimeFormatHighPrecision_t  mtrTime;    /* Meter time in YY MM DD HH MM SS format */
   sysTime_t                     mtrSysTime; /* Meter time in system time format       */
   uint32_t                      seconds;
   uint32_t                      fractionalSec;

   (void)memset( (uint8_t *)&mtrTime, 0, sizeof(mtrTime) );
   (void)memcpy( (uint8_t *)&mtrTime, readingInfo->Value.buffer, sizeof(mtrTime));
   (void)memset( readingInfo->Value.buffer, 0, sizeof(readingInfo->Value.buffer) );

   retVal = INTF_CIM_CMD_ansiRead( &mtrTime, MFG_TBL_SECURITY_LOG, (uint32_t)offsetof(mfgTbl78_t, mtrProgInfo.dtLastProgrammed), 5 );

   if( CIM_QUALCODE_SUCCESS == retVal )
   {
      if ( CIM_QUALCODE_SUCCESS == ( enum_CIM_QualityCode )TIME_UTIL_ConvertMtrHighFormatToSysFormat( &mtrTime, &mtrSysTime ) )
      {
         TIME_UTIL_ConvertSysFormatToSeconds( &mtrSysTime , &seconds, &fractionalSec );
         readingInfo->Value.uintValue = seconds;  // set the value
         readingInfo->valueSizeInBytes = sizeof(seconds);  // set the number of bytes
         retVal = CIM_QUALCODE_SUCCESS;
      }
   }
#else
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
#endif
   readingInfo->typecast = dateTimeValue;  // set the typecast
   readingInfo->readingType = edProgrammedDateTime;

   return retVal;
}

/***********************************************************************************************************************

   Function name: EDCFG_getMeterDateTime

   Purpose: Gets the reading information for DT_Last_PROGRAMMED

   Arguments: ValueInfo_t *readingInfo - pointer to a reading data structure

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
enum_CIM_QualityCode EDCFG_getMeterDateTime( ValueInfo_t *readingInfo )
{
#if (HMC_KV == 1 ) || (HMC_I210_PLUS_C == 1)
   enum_CIM_QualityCode retVal;

   MtrTimeFormatHighPrecision_t  mtrTime;    /* Meter time in YY MM DD HH MM SS format */
   sysTime_t                     mtrSysTime; /* Meter time in system time format       */
   uint32_t                      seconds;
   uint32_t                      fractionalSec;

   (void)memset( (uint8_t *)&mtrTime, 0, sizeof(mtrTime) );
   (void)memcpy( (uint8_t *)&mtrTime, readingInfo->Value.buffer, sizeof(mtrTime));
   (void)memset( readingInfo->Value.buffer, 0, sizeof(readingInfo->Value.buffer) );

   retVal = INTF_CIM_CMD_ansiRead( &mtrTime, STD_TBL_CLOCK, (uint32_t)offsetof(stdTbl52_t, clockCalendar), 6 );

   if( CIM_QUALCODE_SUCCESS == retVal )
   {
      if ( CIM_QUALCODE_SUCCESS == ( enum_CIM_QualityCode )TIME_UTIL_ConvertMtrHighFormatToSysFormat( &mtrTime, &mtrSysTime ) )
      {
         TIME_UTIL_ConvertSysFormatToSeconds( &mtrSysTime , &seconds, &fractionalSec );
         readingInfo->Value.uintValue = seconds;
         readingInfo->valueSizeInBytes = sizeof(seconds);
         retVal = CIM_QUALCODE_SUCCESS;
      }
   }
#else
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ; //default
#endif
   readingInfo->typecast = dateTimeValue;  // set the typecast
   readingInfo->readingType = meterDateTime;

   return retVal;
}

/***********************************************************************************************************************

   Function name: EDCFG_getAvgIndPowerFactor

   Purpose:

   Arguments:

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
enum_CIM_QualityCode EDCFG_getAvgIndPowerFactor( ValueInfo_t *readingInfo )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_CODE_INDETERMINATE; //default
#if ( HMC_KV == 1 )
   avgPfData_t currentAvgPfData;  // variable to store data read from the emeter

   // initialize the data structure used to retrieve data from the meter
   (void)memset( (uint8_t *)&currentAvgPfData, 0, sizeof(avgPfData_t) );

   // retrieve the numerator and denominator accumulators from the meter
   if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &currentAvgPfData, MFG_TBL_AVERAGE_PF_DATA,
                                                      (uint32_t)offsetof(mfgTbl81_t, currentAvgPfs),
                                                      sizeof(avgPfData_t) ) )
   {
      uint64_t num = 0;  // variable to hold numerator value
      uint64_t den = 0;  // variable to hold denominator value

      // copy data read into numerator and denominator variables
      (void)memcpy( (uint8_t *)&num, currentAvgPfData.numerator, sizeof(currentAvgPfData.numerator) );
      (void)memcpy( (uint8_t *)&den, currentAvgPfData.denominator, sizeof(currentAvgPfData.denominator) );

      // convert floating point number into integer value, set decimal precision, and number of bytes needed for value
      readingInfo->Value.intValue = (int64_t)( ( ((float)num / den) + 0.005f) * pow(10.0,(double)2) );
      readingInfo->power10 = 2;
      readingInfo->valueSizeInBytes = HEEP_getMinByteNeeded( readingInfo->Value.intValue, intValue, 0);
      retVal = CIM_QUALCODE_SUCCESS;
   }
#endif
   readingInfo->typecast = intValue;  // typecast is dec, so set to intValue
   readingInfo->readingType = avgIndPowerFactor; // initialize the reading type

   return retVal;
}

/***********************************************************************************************************************

   Function name: EDCFG_getTHDCount

   Purpose: Retrieve the number of Diagnostic 5 occurences for Total Harmonic Distortion Current or Voltage

   Arguments: ValueInfo_t *readingInfo

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
enum_CIM_QualityCode EDCFG_getTHDCount( ValueInfo_t *readingInfo )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ; //default

#if (HMC_KV == 1 )
   uint32_t diagnostic5ConfigValue = 0; // variable to read the diagnostic 5 configuration
   if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiReadBitField( &diagnostic5ConfigValue, MFG_TBL_POWER_QUALITY_CONFIG,
                                                             (uint32_t)offsetof(mfgTbl71_t, diagnostic5Config), 0, 3 ) )
   {  // successfully read the diagnostic 5 configuration, we need to validate the configuration settings
      if( MT71_DIAG5_TOTAL_HARMONIC_CURRENT_DISTORTION == diagnostic5ConfigValue ||
          MT71_DIAG5_TOTAL_HARMONIC_VOLTAGE_DISTORTION == diagnostic5ConfigValue  )
      {  // meter is configured for THDV or THDC
         uint8_t counterValue = 0;
         if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( readingInfo->Value.buffer, MFG_TBL_LINE_SIDE_DIAG_PWR_QUAL_DATA,
                                                            (uint32_t)offsetof(mfgTbl72_t, diagCounters.diag5Total),
                                                            sizeof(counterValue) ) )
         {  // we read the value from the meter, update the reading structure
           readingInfo->Value.uintValue = counterValue;
            readingInfo->valueSizeInBytes = HEEP_getMinByteNeeded( (int64_t)counterValue, uintValue, 0);
            retVal = CIM_QUALCODE_SUCCESS;
         }
      }
   }
   #endif
   readingInfo->typecast = uintValue;  // this is a counter so typecast is an unsigned integer
   readingInfo->readingType = highTHDcount; // initialize the reading type

   return retVal;
}
