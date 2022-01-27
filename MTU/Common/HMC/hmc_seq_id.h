/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: HMC_seq_id.h
 *
 * Contents: Gets the next available sequence Id.  This is used so that each applet gets a unique sequence ID every time
 *           a procedure is executed.
 *
 ***********************************************************************************************************************
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
 ***********************************************************************************************************************
 *
 * @author Karl Davlin
 *
 * $Log$ Created April 4, 2013
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - 04/04/2012 - Initial Release
 *
 * @version    0.1
 * #since      2013-04-04
 **********************************************************************************************************************/
#ifndef HMC_seq_id_H_
#define HMC_seq_id_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef HMC_seq_id_GLOBALS
   #define HMC_seq_id_EXTERN
#else
   #define HMC_seq_id_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/**
 * HMC_getSequenceId - Gets the next available sequence Id.  This is used so that each applet gets a unique sequence ID
 * every time a procedure is executed.
 *
 * @see PSEM definition
 *
 * @param  None
 * @return uint8_t - Sequence Id to use for the calling applet to use in the procedure.
 */
uint8_t HMC_getSequenceId(void);

/* ****************************************************************************************************************** */
#undef HMC_seq_id_EXTERN

#endif
