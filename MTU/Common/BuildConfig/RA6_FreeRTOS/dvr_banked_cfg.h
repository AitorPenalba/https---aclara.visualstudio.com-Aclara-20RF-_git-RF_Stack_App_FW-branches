/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: dvr_banked_cfg.h
 *
 * Contents: defines bank driver dependency list
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
 * $Log$ Created Oct 17, 2012
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - KAD 10/17/2012 - Initial Release
 *
 * @version    0.1
 * #since      2012-10-17
 **********************************************************************************************************************/
#ifndef dvr_banked_cfg_H_
#define dvr_banked_cfg_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

//#include "mtr_app.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef dvr_banked_GLOBAL
   #define dvr_banked_cfg_EXTERN
#else
   #define dvr_banked_cfg_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef bool   (*FunctionPtrDependency)(void);  /* Function Pointer Definition for Dependency calls */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* Defines a list of applications that need to be not busy before clearing NV */
dvr_banked_cfg_EXTERN const FunctionPtrDependency extFlashAppList[]
#ifdef dvr_banked_GLOBAL
=
{
  NULL
  //xxx MTR_APP_status
   /* DO NOT NULL TERMINATE!!!! */
}  /*lint !e64  xxx this needs to be changed!!! */
#endif
;

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
#undef dvr_banked_cfg_EXTERN

#endif
