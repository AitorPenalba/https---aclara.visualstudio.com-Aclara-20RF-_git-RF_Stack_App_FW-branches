/******************************************************************************
 *
 * Filename: byteswap.h
 *
 * Contents: Swaps bytes of a string for little to big endian converstions
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
 * $Log$ kad Created 100604
 *
 *****************************************************************************/

#ifndef byteswap_H
#define byteswap_H

/* INCLUDE FILES */

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

extern void Byte_Swap(uint8_t *, uint8_t);
#ifdef PORTABLE_freescale_H
extern void Byte_Swap_Data_String(far uint8_t *, uint8_t, uint8_t);
#endif /* PORTABLE_freescale_H */

#endif
