/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: exe_list.h
 *
 * Contents: Prototypes for the execute list function(s)
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
 * @author KAD
 *
 * $Log$ KAD Created June 4, 2012
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - KAD 06/04/2012 - Initial Release
 *
 * @version    0.1
 * #since      2012-06-04
 **********************************************************************************************************************/
#ifndef exe_list_H_
#define exe_list_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef EL_GLOBALS
   #define EL_EXTERN
#else
   #define EL_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef returnStatus_t( *eFv_PTR )(void);

// <editor-fold defaultstate="collapsed" desc="typedef exeList_t">
/* Note:  The source pointer constant should be the native type to the processor.  For example, the PIC18 should be a
 * uint8_t * volatile and the PIC24 should be uint16_t * volatile.  */
typedef struct
{
   volatile uint16_t *pDest;   /* Points the Dest location (SFR or RAM) to initialize */
   volatile void   *pSrc;    /* Points to the Source value. */
   eFv_PTR const Fct;        /* Funct ptr, functions return SUCCESS/FAILURE & passed none. */
} exeList_t; /* Contains the SFR and initial value to set the SFR.  NOTE:  NULL TERMINATED!!! */
// </editor-fold>

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/**
 * bool EL_doList(void) - Operates on a list type exeList_t.  The command (initCmd_t eCmd) determines if the list
 *                             is a run-time initialize or copy of the source to destination.
 *
 * @param  eCmd            - Command to execute
 * @param  exeList_t *     - Points to the table that will be executed
 * @return returnStatus_t  - eSUCCESS or eFAILURE
 */
returnStatus_t EL_doList(initCmd_t eCmd, exeList_t const *pInitReg);

/* ****************************************************************************************************************** */

#undef EL_EXTERN

#endif
