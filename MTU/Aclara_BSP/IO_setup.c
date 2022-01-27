/******************************************************************************
 *
 * Filename: IO_setup.c
 *
 * Global Designator:
 *
 * Contents:
 *
 * Note: Some pins go to expansion header. Not currently used on Functional Version B board
 *
 ******************************************************************************
 * Copyright (c) 2020 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include <mqx.h>
#include <bsp.h>
#include "project.h"

#if ( HMC_I210_PLUS_C == 1 )  // TODO: Verify the changes GIPO Pin configs for other EPs,
                              // and then make necessary changes and include this file in other projects

/* #DEFINE DEFINITIONS */
#define PIN_MUX_SLOT_DISABLE 0
#define PIN_MUX_SLOT_GPIO    1

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/*******************************************************************************

  Function name: IO_init

  Purpose: This function initializes the data structures and drivers needed to
           configure the GPIO's of the board

  Arguments:

  Returns: InitSuccessful - true if everything was successful, false if something failed

  Notes: By setting the port MUX register to 0 which disables the GPIO

*******************************************************************************/
returnStatus_t IO_init ( void )
{
   returnStatus_t retVal = eSUCCESS;

   /* Configuring the unused GPIO pin's Pin Muxing mode to disabled or analog will disable the
    * pin’s input buffer and result in the lowest power consumption */

   PORTB_PCR9 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port B Pin 9 to disabled mode */
   PORTB_PCR20 = PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port B Pin 20 to disabled mode */
   PORTB_PCR21 = PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port B Pin 21 to disabled mode */
   PORTB_PCR22 = PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port B Pin 22 to disabled mode */
   PORTB_PCR23 = PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port B Pin 23 to disabled mode */

   PORTC_PCR1 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port C Pin 1 to analog mode */
   PORTC_PCR2 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port C Pin 2 to analog mode */
   PORTC_PCR3 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port C Pin 3 to analog mode */
   PORTC_PCR4 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port C Pin 4 to disabled mode */
   PORTC_PCR9 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port C Pin 9 to analog mode */
   PORTC_PCR12 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );      /* Configure Port C Pin 12 disabled mode */
   PORTC_PCR13 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );      /* Configure Port C Pin 13 disabled mode */
   PORTC_PCR14 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );      /* Configure Port C Pin 14 disabled mode */
   PORTC_PCR15 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );      /* Configure Port C Pin 15 disabled mode */
   PORTC_PCR18 = PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port C Pin 18 disabled mode */

   PORTE_PCR5 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port E Pin 5 to disabled mode */
   PORTE_PCR6 =  PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port E Pin 6 to disabled mode */
   PORTE_PCR24 = PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port E Pin 24 to analog mode */
   PORTE_PCR25 = PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port E Pin 25 to analog mode */
   PORTE_PCR26 = PORT_PCR_MUX( PIN_MUX_SLOT_DISABLE );       /* Configure Port E Pin 26 to disabled mode */

   return ( retVal );
} /* end IO_init () */

#endif
