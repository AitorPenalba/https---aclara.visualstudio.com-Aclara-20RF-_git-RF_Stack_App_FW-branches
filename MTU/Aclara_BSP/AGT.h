/**********************************************************************************************************************

  Filename: AGT.h

  Global Designator: AGT_

  Contents:

 **********************************************************************************************************************
  A product of
  Aclara Technologies LLC
  Confidential and Proprietary
  Copyright 2022 Aclara. All Rights Reserved.

  PROPRIETARY NOTICE
  The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
  (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
  authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
  be used or copied only in accordance with the terms of such license.
 *********************************************************************************************************************

   @author Dhruv Gaonkar

 ****************************************************************************************************************** */

#ifndef AGT_H
#define AGT_H

/* INCLUDE FILES */
#include "hal_data.h"
#include "bsp_pin_cfg.h"
#include "r_ioport.h"


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

fsp_err_t AGT_LPM_Timer_Init            ( void );
fsp_err_t AGT_LPM_Timer_Start           ( void );
fsp_err_t AGT_LPM_Timer_Stop            ( void );
fsp_err_t AGT_LPM_Timer_Configure   ( uint32_t const period );


#endif /* AGT_H */