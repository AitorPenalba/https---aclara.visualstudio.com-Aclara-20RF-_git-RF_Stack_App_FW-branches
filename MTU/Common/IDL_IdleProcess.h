/******************************************************************************
 *
 * Filename: IDL_IdleProcess.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2012 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
#ifndef IDL_IdleProcess_H
#define IDL_IdleProcess_H

/* INCLUDE FILES */

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
//TODO: adhere to new task parameter macro
#if ( RTOS_SELECTION == MQX_RTOS )
void IDL_IdleTask ( uint32_t Arg0 );
#elif ( RTOS_SELECTION == FREE_RTOS )
void IDL_IdleTask ( void* pvParameters );
#endif
uint32_t IDL_Get_IdleCounter ( void );

/* FUNCTION DEFINITIONS */

#endif /* this must be the last line of the file */
