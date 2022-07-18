/***********************************************************************************************************************
 *
 * Filename: xzminidec.h
 *
 * Contents: Contains prototypes and definitions for xz and minibs combined init, close
 *
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/

#ifndef _XZMINIDEC_H_
#define _XZMINIDEC_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "dfw_xzmini_interface.h"
#include "xz.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

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

xz_returnStatus_t  XZMINIDEC_init( compressedData_position_t *patchOffset );
void               XZMINIDEC_end( void );
u_char             XZMINIDEC_getchar( compressedData_position_t *patchOffset );

#endif
