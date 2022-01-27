/******************************************************************************
 *
 * Filename: hmc_start.h
 *
 * Contents: #defs, typedefs, and function prototypes for starting the meter
 *    communication session, log-in, password, etc...
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ******************************************************************************
 *
 * $Log$ kad Created 080707
 *
 *****************************************************************************/

#ifndef hmc_start_H
#define hmc_start_H

/* INCLUDE FILES */

#include "psem.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */


/*incuding macros for packet size and number negoiations for future use that may negtaiate their size*/


/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */


/* FUNCTION PROTOTYPES */

returnStatus_t HMC_STRT_init( void );
#if ( ANSI_SECURITY == 1 )
returnStatus_t HMC_STRT_UpdatePasswords( void );
#endif
uint8_t        HMC_STRT_LogOn(uint8_t, void far *);
void           HMC_STRT_ISC(void);
void           HMC_STRT_ISR( uint8_t cmd, void *pdata );
void           HMC_STRT_RestartDelay(void);
uint8_t        HMC_STRT_logInStatus( void );
void           HMC_STRT_loggedOff( void );
#if ( NEGOTIATE_HMC_COMM == 1)
returnStatus_t   HMC_STRT_GetAltBaud(uint32_t *alt_baud);
returnStatus_t   HMC_STRT_SetDefaultBaud(void);
#endif
returnStatus_t HMC_STRT_SetPassword( uint8_t *pwd, uint16_t numberOfBytes );
returnStatus_t HMC_STRT_GetPassword( uint8_t *pwd );
returnStatus_t HMC_STRT_SetMasterPassword( uint8_t *pwd, uint16_t numberOfBytes );
returnStatus_t HMC_STRT_GetMasterPassword( uint8_t *pwd );
#endif
