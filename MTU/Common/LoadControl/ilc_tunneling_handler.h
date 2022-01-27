/***********************************************************************************************************************
 *
 * Filename: ILC_TUNNELING_HANDLER.h
 *
 * Contents:  #defs, typedefs, and function prototypes for the routines to
 *    support ILC_TUNNELING_HANDLER (Serial Protocol Tunneling needs)
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

#ifndef ILC_TUNNELING_HANDLER
#define ILC_TUNNELING_HANDLER

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
void ILC_TNL_MsgHandler( HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length );


#endif

/* End of file */
