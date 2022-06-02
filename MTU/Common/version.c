/***********************************************************************************************************************
 *
 * Filename:   version.c
 *
 * Global Designator: VER_
 *
 * Contents: Contains the firmware version definitions
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 * @author Karl Davlin
 **********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "project.h"
#include <string.h>
#include "version.h"
#ifndef __BOOTLOADER
#include "file_io.h"

#endif   /* __BOOTLOADER   */
/* ****************************************************************************************************************** */
/* Include ALL project wide warnings here.
   This file is rebuilt every time so these warning messages will be displayed. */

#if ( PRODUCTION_BUILD != 1 )
   #warning "This is a NON Production Build - DO NOT RELEASE TO PRODUCTION"
#endif

#if DFW_TEST_KEY
   #warning "Do not release FW with the DFW Test Key enabled!"
#endif

#if ( PORTABLE_DCU )
   #warning "MOBILE DCU UNIT TEST CODE, NOT FOR PRODUCTION"
#endif

#if ( ( MULTIPLE_MAC != 0 ) && ( NUM_MACS != 1 ) )
   #warning "Compiling for MULTIPLE MAC addresses!"
#endif

#if ( ENABLE_PRNT_ALRM_INFO == 1 ) /* Name of enum is included for printing. */
   #warning "DO NOT RELEASE with ENABLE_PRNT_ALRM_INFO set."   /*lint !e10 !e16  */
#endif

#if ( TEST_MODE_ENABLE != 0 )
   #warning "TEST CODE, NOT FOR PRODUCTION"
#endif

#if ( TEST_QUIET_MODE !=0 )
   #warning "QUIET MODE TEST CODE, NOT FOR PRODUCTION"
#endif

#if TRACE_MODE != 0
   #warning "TRACE_MODE set in CompileSwitch.h"
#endif

#if ( SIMULATE_POWER_DOWN !=0 )
   #warning "POWER DOWN SIMULATION TEST CODE, NOT FOR PRODUCTION"
#endif

#if ( LG_WORST_CASE_TEST != 0 )
   #warning "Don't release with LG_WORST_CASE_TEST set"   /*lint !e10 !e16  */
#endif

#if ( ENABLE_TRACE_PINS_LAST_GASP != 0 )
   #warning "Don't release with ENABLE_TRACE_PINS_LAST_GASP set"
#endif

#if ( TEST_REGULATOR_CONTROL_DISABLE !=0 )
   #warning "TEST CODE, NOT FOR PRODUCTION"
#endif

#if ( ( MAC_LINK_PARAMETERS != 0 ) || ( MAC_CMD_RESP_TIME_DIVERSITY != 0 ) )
   #warning "NOT AN EP FEATURE"
#endif

/* ****************************************************************************************************************** */




/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#ifdef __BOOTLOADER
   #define BOOTLOADER_VER    ((uint8_t)1)    /* current bootloader version */
   #define BOOTLOADER_REV    ((uint8_t)2)    /* current bootloader revision */
   #define BOOTLOADER_BUILD  ((uint16_t)2)   /* current bootloader build */
#else //Application version
   #define FIRMWARE_VER    ((uint8_t)3)      /* current firmware version */
   #define FIRMWARE_REV    ((uint8_t)0)      /* current firmware revision */
   #define FIRMWARE_BUILD  ((uint16_t)65)    /* current firmware build */ // Don't put a '0' in front of the rev number. It's going to be interpreted as an octal number and might not build.
#endif

#ifndef __BOOTLOADER
static char const HWVersionDefault[] =
{
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A )
  #if ( HMC_I210_PLUS == 1)
    "A.1.84001"     /* Y84001-1   (K22 I210+) default hardware revision */
  #elif ( HMC_KV == 1 )
    "A.301.84001"   /* Y84001-301 (K22 KV2c)  default hardware revision */
  #else
    ""              /* NULL default hardware revision */
  #endif
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84020_1_REV_A )
  "B.401.84024"      /* Y84020-1   (K24 I210+/I210+c) default hardware revision */
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84114_1_REV_A )
  "A.1.84114" // SRFN DA NIC default
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y99852_1_REV_A )
  "A.1.99852"      /* Y99852-1   (ILC) default hardware revision */
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84030_1_REV_A )
  "B.301-XA.84024"      /* Y84030-1   (K24 KV2c)  default hardware revision */
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_RENESAS_REV_A )
  "A.x.84580"           /* Y84580-x where 1 = Maxim boost, 2 = TI boost */
#else
  ""               /* NULL default hardware revision */
#endif
};
#endif      /* __BOOTLOADER   */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
// Used to allow visability for DFW file offset extraction post processing
#ifndef __BOOTLOADER
typedef struct
{
   uint8_t  String[VER_HW_STR_LEN];      /* Version string including the two '.' and a NULL */
} HWVerString_t;
#endif   /* __BOOTLOADER   */

/* ****************************************************************************************************************** */
/* CONSTANTS */
#ifdef __BOOTLOADER
   const firmwareVersion_u blFwVersion_ =
   {
      BOOTLOADER_VER,  /*lint !e708  initialization of a union */
      BOOTLOADER_REV,
      BOOTLOADER_BUILD
   };
#else //Application version
   /*lint --esym(526,LNKR_BL_FW_VER_ADDR)  Address defined in Linker script */
#if 0 // TODO: RA6E1 Link BL firmware version
   extern const firmwareVersion_u   LNKR_BL_FW_VER_ADDR;
#endif
   static const firmwareVersion_u appFwVersion_ =
   {
      #if ( PRODUCTION_BUILD == 1 )
         FIRMWARE_VER,  /*lint !e708  initialization of a union */
      #else
         ( ( int8_t )FIRMWARE_VER * ( uint8_t )-1 ),  /*lint !e708  initialization of a union */
      #endif
         FIRMWARE_REV,
      #if ( PRODUCTION_BUILD == 1 )
         #define INTERIM_FW_BUILD   FIRMWARE_BUILD
      #else
         #define INTERIM_FW_BUILD   (FIRMWARE_BUILD + (uint16_t)1000)
      #endif

      #if ( PRODUCTION_BUILD == 1 )
         INTERIM_FW_BUILD
      #else
         //Not a production build.  Change Build number if any of the DFW tests are enabled
         #if ( BUILD_DFW_TST_VERSION_CHANGE == 1 )
            INTERIM_FW_BUILD  | 0xFF00
         #elif ( BUILD_DFW_TST_CMD_CHANGE == 1 )
            INTERIM_FW_BUILD | 0xFE00
         #elif ( BUILD_DFW_TST_LARGE_PATCH == 1 )
            INTERIM_FW_BUILD | 0xFD00
         #else
            INTERIM_FW_BUILD
         #endif
      #endif
   };

   static const firmwareVersionDT_s appFwVersionDT_ =
   {
      __DATE__,         /* String for date of build: "Mmm dd yyyy" */
      __TIME__          /* String for time of build: "hh:mm:ss" */
   };

#endif   // end of else to #ifdef __BOOTLOADER

#ifndef __BOOTLOADER
   static const char comDeviceType_[COM_DEVICE_TYPE_LEN+1] =
   #if ( HMC_I210_PLUS == 1)
      "SRFNI-I-210+";
   #elif ( HMC_KV == 1 )
      "SRFNI-KV2c";
   #elif ( HMC_I210_PLUS_C == 1 )
      "SRFNI-I-210+c";
   #elif ( ACLARA_LC == 1 )
      "SRFNI-ILC";
   #elif ( ACLARA_DA == 1 )
      "SRFNI-Edge-Gateway";
   #else
      "NOT DEFINED";
   #endif
#endif      /* __BOOTLOADER   */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
#ifndef __BOOTLOADER
static OS_MUTEX_Obj  verMutex_;                    //Serialize access to this module
static FileHandle_t  verFileHndl_;                 //Contains the file handle information

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************
 * Function Name: VER_init
 *
 * Purpose: Initializes files for Version information
 *
 * Arguments: None
 *
 * Returns: None
 *
 * Side Effects: None
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 **********************************************************************************************************************/
returnStatus_t VER_Init ( void )
{
#if 0 /* TODO: RA6: TODO: Add this code  */
   FileStatus_t     fileStatusCfg;              /* Contains the file status */
   returnStatus_t   retVal = eFAILURE;
   HWVerString_t    hWVer;                      /* HW Version string */

   if ( OS_MUTEX_Create(&verMutex_) )
   {
      retVal = FIO_fopen(  &verFileHndl_,                   /* File Handle */
                           ePART_SEARCH_BY_TIMING,          /* Search for the best paritition according to the timing. */
                           (uint16_t)eFN_VERINFO,           /* File ID (filename) */
                           (lCnt)sizeof(hWVer),             /* Size of the data in the file. */
                           0,                               /* File attributes to use. */
                           0xFFFFFFFF,                      /* The update rate of the data in the file. */
                           &fileStatusCfg);
      if (eSUCCESS == retVal)
      {
         if (fileStatusCfg.bFileCreated)
         {
            (void)memset(&hWVer, 0, sizeof(hWVer)); //clear the file
            //Set default
            (void)memcpy(&hWVer.String[0], &HWVersionDefault[0], sizeof(HWVersionDefault));
#if ( HAL_TARGET_HARDWARE != HAL_TARGET_Y84001_REV_A )
            /* Wait 200msecs for the divider network to stabilize before the HW version
               analog voltage value is read by the ADC */
            OS_TASK_Sleep(200);
            hWVer.String[0] = ADC_GetHWRevLetter();   //Replace default Rev Letter with the HW detected value
#endif
            retVal = FIO_fwrite(&verFileHndl_, 0, &hWVer.String[0], (lCnt)sizeof(hWVer.String));
         }
      }
   }
#else
   returnStatus_t   retVal = eSUCCESS;
#endif
   return(retVal);
}

/***********************************************************************************************************************
 * Function Name: VER_getFirmwareVersion
 *
 * Purpose: Returns the firmware version
 *
 * Arguments: target          - which version to return
 *
 * Returns: firmwareVersion_u - Firmware version for APP or BL, otherwise zeroes
 *
 * Side Effects: None
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 **********************************************************************************************************************/
firmwareVersion_u VER_getFirmwareVersion ( eFwTarget_t target )
{
   firmwareVersion_u ver={0,0,0};   /*lint !e708  initialization of a union */

   if ( eFWT_APP == target )
   {
      ver = appFwVersion_;
   }
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
#else
   else if ( eFWT_BL == target )
   {
#if 0 // TODO: RA6E1 Link BL address
      ver = LNKR_BL_FW_VER_ADDR;
#endif
   }
#endif
   return (ver);
}

/***********************************************************************************************************************
 * Function Name: VER_getFirmwareVersionDT
 *
 * Purpose: Returns the target firmware build Date and Time
 *
 * Arguments: NONE
 *
 * Returns: firmwareVersionDT_s * - pointer to Firmware build Date and Time
 *
 * Side Effects: None
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 **********************************************************************************************************************/
const firmwareVersionDT_s * VER_getFirmwareVersionDT ( void )
{
   return (&appFwVersionDT_);
}

/***********************************************************************************************************************
 * Function Name: VER_getHardwareVersion
 *
 * Purpose: Returns the hardware version
 *
 * Arguments: string - pointer to memory that hardware version string can be written to
 *            len - maximum size available at *string for the string including termination character
 *
 * Returns: eSUCCESS if string and len are successfully updated
 *          eFAILURE if mutex lock failed or if string did no contain a terminator.  string and len are undefined
 *
 * Side Effects: None
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 **********************************************************************************************************************/
returnStatus_t VER_getHardwareVersion ( uint8_t *string, uint8_t len )
{
   returnStatus_t   retVal = eFAILURE;
#if 0 // TODO: RA6E1 restore this code once the file system is working.  Otherwise, garbage is printed.
   HWVerString_t    hWVer;                      /* HW Version string */

   OS_MUTEX_Lock(&verMutex_); // Function will not return if it fails
   (void)FIO_fread(&verFileHndl_, &hWVer.String[0], 0, (lCnt)VER_HW_STR_LEN);
   if ( (strlen((char *)hWVer.String) + 1) <= len )
   {
      ( void )memcpy(string, hWVer.String, strlen((char *)hWVer.String) + 1);
      retVal = eSUCCESS;
   }
   OS_MUTEX_Unlock(&verMutex_); // Function will not return if it fails
#else
   memcpy(string, (uint8_t *)&HWVersionDefault, len);
   retVal = eSUCCESS;
#endif
   return (retVal);
}

/***********************************************************************************************************************
 * Function Name: VER_setHardwareVersion
 *
 * Purpose: Sets the hardware version
 *
 * Arguments: uint8_t *:  HW Version string
 *
 * Returns: eSUCCESS if the hardware version was successfully set
 *          eFAILURE if there was a problem and hardware version is unchanged
 *
 * Side Effects: None
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 **********************************************************************************************************************/
returnStatus_t VER_setHardwareVersion ( uint8_t const *string )
{
   returnStatus_t   retVal = eFAILURE;
#if 0
   uint8_t          len;

   OS_MUTEX_Lock(&verMutex_); // Function will not return if it fails
   len = ( uint8_t )strlen( (char *)string) + 1;
   if ( len <= VER_HW_STR_LEN )
   {
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A )
      (void)FIO_fwrite(&verFileHndl_, 0, &string[0], (lCnt)len);
#else
      // Do not allow Rev letter to be changed.  This is determined by HW at init time.
      (void)FIO_fwrite(&verFileHndl_, 1, &string[1], (lCnt)len-1);
#endif
      retVal = eSUCCESS;
   }
   OS_MUTEX_Unlock(&verMutex_); // Function will not return if it fails
#else
   retVal = eSUCCESS;
#endif
   return (retVal);
}

/***********************************************************************************************************************
 * Function Name: VER_getComDeviceType
 *
 * Purpose: Returns the com device type
 *
 * Arguments: None
 *
 * Returns: uint8_t *: pointer com device type
 *
 * Side Effects: None
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 **********************************************************************************************************************/
char const * VER_getComDeviceType ( void )
{
   return comDeviceType_;
}
#endif      /* __BOOTLOADER   */
