/*****************************************************************************
 *
 * Filename: hmc_prg_mtr.h
 *
 * Contents:  For the support of meter programming.
 *
******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2018 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies
 * LLC an Ohio limited liability company (Aclara).  This information may not be
 * published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara. Any software or firmware described in this document
 * is furnished under a license and may be used or copied only in accordance
 * with the terms of such license.
******************************************************************************
 *
 * $Log$ rcz Created 032918
 *
*****************************************************************************/

#ifndef HMC_PRG_MTR
#define HMC_PRG_MTR
#include "meter.h"

/****************** FOR PROGRAMMING METER DISPLAY ****************************
In production, the MeterMate program MUST include ID's 998 and 999
(NETWORK_STATUS_INFO[1] and NETWORK_STATUS_INFO[2]) in the ALT display list
(ST32:DISPLAY_SOURCES[]) and "point" to them in display sources
(ST33:PRI_DISP_SOURCES[]). The firmware will write the RSSI label and value
for these entries in MT119.

If the meter program does not include the proper setup, the entries provided
by the FW will not be displayed.

Note: MT119 is stored in RAM and is therefore volatile with power cycles.

*******************************************************************************/
// Enable this define when testing without MeterMate.
// Comment it out for production version, since MeterMate will then be used
// to replace this code's functions (sets tables ST32 and ST33).
#define IN_LIEU_OF_METER_MATE       0




/****************** FOR PROGRAMMING METER FLASH *******************

When the define END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED, the code
for downloading to the meter Flash is included.
**************************************************************/

/****************************************************************************/
/* INCLUDE FILES */
#include "error_codes.h"
#include "stdbool.h"
#include "meter.h"

/****************************************************************************/
/* CONSTANT DEFINITIONS */


/****************************************************************************/
/* MACRO DEFINITIONS */


/****************************************************************************/
/* TYPE DEFINITIONS */

#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
// The various sections in the meter configuration file.
typedef enum
{
    SECTION_COMPATIBILITY,   // Assures correct meter for this program file.
    SECTION_PROGRAM,    // This section programs/configures the meter.
    SECTION_AUDIT,      // Some verification that programming was successful.
    SECTION_MODULE      // Start of Module Configuration (optional)
}eFileSection_t;
#endif

/****************************************************************************/
/* GLOBAL VARIABLES */


/****************************************************************************/
/* GLOBAL FUNCTION PROTOTYPES */

#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
 bool            HMC_PRG_MTR_IsNotTestable ( void );
 void            HMC_PRG_MTR_ClearNotTestable ( void );
 void            HMC_PRG_MTR_SetNotTestable ( void );
 returnStatus_t  HMC_PRG_MTR_RunProgramMeter ( eFileSection_t SectionToRun );
#endif

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
 returnStatus_t  HMC_PRG_MTR_init ( void );
 void            HMC_PRG_MTR_SwitchTableAccessFunctions ( void );
 void            HMC_PRG_MTR_ReturnTableAccessFunctions ( void );
 bool            HMC_PRG_MTR_IsStarted ( void );
 bool            HMC_PRG_MTR_IsSynced ( void );
 void            HMC_PRG_MTR_ClearSync ( void );
 void            HMC_PRG_MTR_SetSync ( void );
 bool            HMC_PRG_MTR_IsHoldOff ( void );
#endif

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
 returnStatus_t  HMC_PRG_MTR_PrepareToProgMeter( void );
 uint8_t         HMC_PRG_MTR_GetProcedureTimeout ( void );
#endif


#endif /* end of "#ifndef HMC_PRG_MTR" */

