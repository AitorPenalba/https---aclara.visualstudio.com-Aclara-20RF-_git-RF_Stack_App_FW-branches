/***********************************************************************************************************************
 *
 * Filename: ILC_TIME_SYNC_HANDLER.h
 *
 * Contents:  #defs, typedefs, and function prototypes for the routines to
 *    support ILC_TIME_SYNC_HANDLER (DRU time synchronization needs)
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2017 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ kad Created 080207
 *
 **********************************************************************************************************************/

#ifndef ILC_TIME_SYNC_HANDLER_H
#define ILC_TIME_SYNC_HANDLER_H

/* ***************************************************************************************************************** */
/* INCLUDE FILES */
#include "ilc_dru_driver.h"


/* ***************************************************************************************************************** */
/* CONSTANT DEFINITIONS */


/* ***************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ***************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ***************************************************************************************************************** */
/* GLOBAL VARIABLES */


/* ***************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
returnStatus_t ILC_TIME_SYNC_Init( void );
void ILC_TIME_SYNC_Task( uint32_t Arg0 );


#endif

/* End of file */