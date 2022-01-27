/***********************************************************************************************************************
 *
 * Filename: FTM.h
 *
 * Contents: Functions related to FlexTimer module
 *
 ***********************************************************************************************************************
 * Copyright (c) 2010-2020 Aclara Power-Line Systems Inc.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 **********************************************************************************************************************/

#ifndef FTM_H
#define FTM_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
returnStatus_t FTM0_Init( void );
returnStatus_t FTM0_Channel_Enable (uint32_t Channel, uint32_t SC, void (*channel_callback)(void) );
returnStatus_t FTM0_Channel_Disable (uint32_t Channel);

returnStatus_t FTM1_Init( void );
returnStatus_t FTM1_Channel_Enable (uint32_t Channel, uint32_t SC, void (*channel_callback)(void) );
returnStatus_t FTM1_Channel_Disable (uint32_t Channel);

returnStatus_t FTM2_Init( void );
returnStatus_t FTM2_Channel_Enable (uint32_t Channel, uint32_t SC, void (*channel_callback)(void) );
returnStatus_t FTM2_Channel_Disable (uint32_t Channel);

returnStatus_t FTM3_Init( void );
returnStatus_t FTM3_Channel_Enable (uint32_t Channel, uint32_t SC, void (*channel_callback)(void) );
returnStatus_t FTM3_Channel_Disable (uint32_t Channel);

#endif
