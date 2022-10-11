// <editor-fold defaultstate="collapsed" desc="File Header Information">
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename:   HMC_seq_id.c
 *
 * Global Designator: HMC_SI_
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
 // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Include Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#define HMC_seq_id_GLOBALS
#include "hmc_seq_id.h"
#undef  HMC_seq_id_GLOBALS
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Constants">
/* ****************************************************************************************************************** */
/* CONSTANTS */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Variables (File Static)">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static uint8_t seqId_ = 0;

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
// </editor-fold>

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="uint8_t HMC_getSequenceId(void)">
/***********************************************************************************************************************
 *
 * Function Name: HMC_getSequenceId
 *
 * Purpose: Gets the next available sequence Id.  This is used so that each applet gets a unique sequence ID every time
 *          a procedure is executed.
 *
 * Arguments: None
 *
 * Returns: uint8_t - Sequence Id to use for the calling applet to use in the procedure.
 *
 * Side Effects: N/A
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint8_t HMC_getSequenceId(void)
{
   if (0 == ++seqId_)
   {
      seqId_ = 0;
   }
   return(seqId_);
}
// </editor-fold>

/* ****************************************************************************************************************** */
