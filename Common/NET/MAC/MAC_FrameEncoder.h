/***********************************************************************************************************************
 *
 * Filename:   MAC_FrameEncoder.h (MAC_Codec.h)
 *
 * Global Designator: MAC_Codec
 *
 * Contents: MAC Encode and Decode functions
 *
 ***********************************************************************************************************************
 * Copyright (c) 2015 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 **********************************************************************************************************************/
#ifndef MAC_Codec_H
#define MAC_Codec_H

/* INCLUDE FILES */

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
extern bool     MAC_Codec_Decode ( const uint8_t *p, uint16_t num_bytes, mac_frame_t *pf );
extern uint16_t MAC_Codec_Encode ( mac_frame_t const *p, uint8_t *pDst, uint32_t *RxTime );
extern void     MAC_Codec_SetDefault(mac_frame_t *pf, uint8_t frame_type);

/* FUNCTION DEFINITIONS */

#endif /* this must be the last line of the file */
