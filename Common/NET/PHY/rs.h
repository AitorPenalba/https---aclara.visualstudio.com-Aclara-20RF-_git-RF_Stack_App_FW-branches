/******************************************************************************
 *
 * Filename: rs.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2014 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

#ifndef RS_H
#define RS_H

#define STAR_FEC_LENGTH		3u

typedef enum
{
   RS_SRFN_63_59,
   RS_SRFN_255_239,
   RS_STAR_63_59,
   RS_LAST
} RSCode_e;

int16_t RS_GetMM(RSCode_e rscode);
int16_t RS_GetKK(RSCode_e rscode);
uint32_t RS_GetParityLength(RSCode_e rscode);
#ifdef PHY_Protocol_H
RSCode_e RS_GetCode(PHY_MODE_e mode, PHY_MODE_PARAMETERS_e modeParameters);
#endif
void RS_Encode(RSCode_e rscode, uint8_t const * const msg, uint8_t * const parity, uint8_t len);
bool RS_Decode(RSCode_e rscode, uint8_t * const msg, uint8_t * const parity, uint8_t len);
void testrs(void);

#endif

