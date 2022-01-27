/******************************************************************************
 *
 * Filename: crc16_m.h
 *
 * Contents:  #defs, typedefs, and function prototypes for meter specific
 *    CRC calculation.
 *
 ******************************************************************************
 * Copyright (c) 2001-2011 Aclara Power-Line Systems Inc.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ******************************************************************************
 *
 * $Log$
 * Created  072007 KAD
 *****************************************************************************/

#ifndef crc16_H
#define crc16_H

/* INCLUDE FILES */

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

extern void   CRC16_CalcMtr( uint8_t );
extern void   CRC16_InitMtr( uint8_t const * );
extern uint16_t CRC16_ResultMtr( void );

#endif	/* this must be the last line of the file */
