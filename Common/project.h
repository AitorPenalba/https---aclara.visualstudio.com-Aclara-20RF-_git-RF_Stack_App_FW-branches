/***********************************************************************************************************************
 *
 * Filename: project.h
 *
 * Contents: Project Specific Includes
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012-2015 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author KAD
 *
 * $Log$ Created June 11, 2012
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - KAD 06/11/2012 - Initial Release
 *
 * @version    0.1
 * #since      2012-06-11
 * 01/22/15 mkv - Added more HEEP enumerations.
 **********************************************************************************************************************/
#ifndef project_H_
#define project_H_

/* MACRO DEFINITIONS */

/* INCLUDE FILES */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>  // TODO: Do we need this?

#include "portable_freescale.h"
#include "portable_aclara.h"

#include "features.h"   /* Need bootloader defined (or not) before cfg_app.h  */
#include "cfg_hal.h"
#include "cfg_app.h"

//#include "CompileSwitch.h" /* MUST come AFTER cfg_app.h which influences choices in CompileSwitch.h   */
#ifndef __BOOTLOADER
#include "OS_aclara.h"
#endif
#include "error_codes.h"
//#include "heep.h"
#include "BSP_aclara.h"

/* TODO: RA6: DG: Move these to appropriate place */
//#include "SEGGER_RTT/SEGGER_RTT.h"
#define SEGGER_INDEX            (0)
#define LVL_ERR      (1u)       /* error conditions   */
#define APP_PRINT(fn_, ...)      printf(fn_);  //SEGGER_RTT_printf (SEGGER_INDEX,(fn_), ##__VA_ARGS__);
#define APP_ERR_PRINT(fn_, ...)  /*if(LVL_ERR)\
        SEGGER_RTT_printf (SEGGER_INDEX, "[ERR] In Function: %s(), %s",__FUNCTION__,(fn_),##__VA_ARGS__); */

/* GLOBAL DEFINTION */
#ifdef PROJ_GLOBALS
#define PROJ_EXTERN
#else
#define PROJ_EXTERN extern
#endif

/* CONSTANTS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

#undef PROJ_EXTERN

#endif
