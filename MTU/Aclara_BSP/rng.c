/******************************************************************************
 *
 * Filename: rng.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2013 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mqx.h>
#include "project.h"
#include "OS_aclara.h"
#include "BSP_aclara.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */
static OS_MUTEX_Obj RNG_Mutex;

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/******************************************************************************

  Function Name: RNG_init

  Purpose: Initialize the Random Number Generator module before any tasks start

  Arguments:

  Returns: RetVal - indicates success or the reason for error

  Notes:

******************************************************************************/
returnStatus_t RNG_init ( void )
{
   returnStatus_t RetVal = eSUCCESS;

   if ( OS_MUTEX_Create(&RNG_Mutex) == false )
   {
      RetVal = eFAILURE;
   } /* end if() */

   return ( RetVal );
} /* end RNG_init () */

/******************************************************************************

  Function Name: RNG_GetRandom_32bit

  Purpose: This function will return a single 32bit random generated number

  Arguments:

  Returns: RandNum - 32 bit random number

  Notes: This call can block as it is protected with a Mutex

******************************************************************************/
uint32_t RNG_GetRandom_32bit ( void )
{
#if ( ENABLE_PROC_RNG == 1 )
   uint32_t RandNum;
   OS_MUTEX_Lock(&RNG_Mutex); // Function will not return if it fails

   /* Open the hardware Random Number Generator Module */
   SIM_SCGC3 |= SIM_SCGC3_RNGA_MASK; // Turn clock on for the RNG
   RNG_CR     = RNG_CR_GO_MASK; // Start the random number generator

   /* Use the hardware random number generator as a seed */
   srand(RNG_OR);
   /* Get the actual Random Number */
   RandNum = (uint32_t)rand();

   /* Close the hardware Random Number Generator Module */
   RNG_CR &= ~RNG_CR_GO_MASK; // Stop the random number generator
   SIM_SCGC3 &= ~SIM_SCGC3_RNGA_MASK; // Turn clock off for the RNG
   OS_MUTEX_Unlock(&RNG_Mutex); // Function will not return if it fails
#else
   uint32_t RandNum;
   // todo - Need to seed the random number generator
// srand ( time(NULL) );
   /* Get the actual Random Number */
   RandNum = (uint32_t)rand();
#endif
   return ( RandNum );
}

/******************************************************************************

  Function Name: RNG_GetRandom_Array

  Purpose: This function will populate an array with random numbers

  Arguments: DataPtr - Pointer to the array to populate (modified by this function)
             DataLen - Length of the array to populate with random data

  Returns:

  Notes: This call can block as it is protected with a Mutex

******************************************************************************/
/*lint -esym(715,DataPtr,DataLen)  not used */
/*lint -esym(818,DataPtr)  could be const */
void RNG_GetRandom_Array ( uint8_t *DataPtr, uint16_t DataLen )
{
#if ( ENABLE_PROC_RNG == 1 )
   // MKD 2015-02-12 RNG not supported on K22
   uint16_t i;

   OS_MUTEX_Lock(&RNG_Mutex); // Function will not return if it fails

   /* Open the hardware Random Number Generator Module */
   SIM_SCGC3 |= SIM_SCGC3_RNGA_MASK; // Turn clock on for the RNG
   RNG_CR = RNG_CR_GO_MASK; // Start the random number generator

   /* Use the hardware random number generator as a seed */
   srand(RNG_OR);

   /* Populate the Array */
   for ( i=0; i<DataLen; i++ )
   {
      DataPtr[i] = (uint8_t)rand();
   } /* end for() */

   /* Close the hardware Random Number Generator Module */
   RNG_CR &= ~RNG_CR_GO_MASK; // Stop the random number generator
   SIM_SCGC3 &= ~SIM_SCGC3_RNGA_MASK; // Turn clock off for the RNG
   OS_MUTEX_Unlock(&RNG_Mutex); // Function will not return if it fails
#endif
}
/*lint +esym(715,DataPtr,DataLen)  not used */
/*lint +esym(818,DataPtr)  could be const */
/* end RNG_GetRandom_Array () */

