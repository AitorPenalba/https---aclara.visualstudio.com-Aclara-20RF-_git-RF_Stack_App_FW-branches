/******************************************************************************
 *
 * Filename: bcd.h
 *
 * Contents: BCD conversions
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
 * $Log$ kad Created 062804
 *
 *****************************************************************************/

#ifndef bcd_H
#define bcd_H

/* INCLUDE FILES */

#include "project.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */


/* FUNCTION PROTOTYPES */

extern uint8_t HEX_2_BCD(uint8_t);
extern uint8_t BCD_2_HEX(uint8_t);



#endif
