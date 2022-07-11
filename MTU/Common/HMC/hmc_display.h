/******************************************************************************
 *
 * Filename:    hmc_display.h
 *
 * Contents:    #defs, typedefs, and function prototypes for setting Meter Time
 *
 ******************************************************************************
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
 ******************************************************************************
 *
 * $Log$
 *
 *****************************************************************************/

#ifndef hmc_display_H  /* replace filetemp with this file's name and remove this comment */
#define hmc_display_H

/* INCLUDE FILES */
#include "psem.h"
#include "hmc_app.h"

/* CONSTANT DEFINITIONS */
/* ***************************************************************************************************************** */
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
/* MT119 NETWORK_STATUS _INFO Entry */
enum
{
   HMC_DISP_POS_PD_LCD_MSG = 0,     /* pdLcdMsg (phase detect LCD message) */
   HMC_DISP_POS_NETWORK_STATUS,     /* Network Status (blue LED functionality) */
   HMC_DISP_POS_SELF_TEST_STATUS,   /* Self-Test Status (red LED functionality) */
   HMC_DISP_POS_NIC_MODE,           /* NIC mode (green LED functionality) */
};
/* ***************************************************************************************************************** */

/* MACRO DEFINITIONS */

/* ***************************************************************************************************************** */
/* Messages to display on the LCD */
#define HMC_DISP_MSG_NULL           "      "
#define HMC_DISP_MSG_INVALID_TIME   "NET---"
#define HMC_DISP_MSG_VALID_TIME     "NETREC"
#define HMC_DISP_MSG_HOT            "HOT   "
#define HMC_DISP_MSG_MODE_QUIET     "QUIET "
#define HMC_DISP_MSG_MODE_SHIP      "SHIP  "
#define HMC_DISP_MSG_MODE_SHOP      "SHOP  "
#define HMC_DISP_MSG_MODE_SECURE    "SECURE"
#define HMC_DISP_MSG_MODE_RUN       "RUN   "

/* Lengths of messages and buffers */
#define HMC_DISP_MSG_LENGTH         ((uint8_t)6)
#define HMC_DISP_TOTAL_BUFF_LENGTH  ((uint8_t)24)


/* ***************************************************************************************************************** */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */
extern uint8_t HMC_DISP_applet ( uint8_t, void * );
returnStatus_t HMC_DISP_Init ( void );
void           HMC_DISP_UpdateDisplayBuffer ( void const *, uint8_t  );

#endif
#endif
