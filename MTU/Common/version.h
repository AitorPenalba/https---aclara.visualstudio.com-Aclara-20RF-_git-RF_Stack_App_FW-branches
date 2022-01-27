/***********************************************************************************************************************
 *
 * Filename: version.h
 *
 * Global Designator: VER_
 *
 * Contents:  Contains the firmware version prototypes.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013-2017 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
#ifndef version_H
#define version_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define COM_DEVICE_TYPE_LEN ((uint8_t)20)
/* For "A.401-XAxx.84001x". Defined in HEEP as: Rev.Dash.BasePN (and NULL) */
#define VER_HW_STR_LEN     ((uint8_t)18)

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   eFWT_NONE = ((uint8_t)0),     // Default State - undeclared/none
   eFWT_APP,                     // Module Application FW
   eFWT_BL,                      // Module Bootloader FW
   eFWT_METER_FW                 // The Meter FW
}eFwTarget_t;                    // Used to identify the FW target


typedef union
{
   struct
   {
      uint8_t  version;       /* Firmware Version */
      uint8_t  revision;      /* Firmware Revision */
      uint16_t build;         /* Firmware Build */
   } field;                   /* Access to each field */
   uint32_t packedVersion;    /* version in 32-bit format (native endian format!) */
} firmwareVersion_u;          /* Structure to access the firmware version as 32-bits or access each field as defined by
                               * the PDS.  Note:  All fields are in native endian format */

typedef struct
{
   uint8_t  date[12];         /* String for date of build: "Mmm dd yyyy" */
   uint8_t  time[9];          /* String for time of build: "hh:mm:ss" */
} firmwareVersionDT_s;        /* Structure to the firmware version build date and time */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

returnStatus_t    VER_Init ( void );
returnStatus_t    VER_getHardwareVersion ( uint8_t *string, uint8_t len );
returnStatus_t    VER_setHardwareVersion ( uint8_t const *string );
firmwareVersion_u VER_getFirmwareVersion ( eFwTarget_t target );
const firmwareVersionDT_s * VER_getFirmwareVersionDT ( void );
char const *      VER_getComDeviceType ( void );

#endif /* this must be the last line of the file */
