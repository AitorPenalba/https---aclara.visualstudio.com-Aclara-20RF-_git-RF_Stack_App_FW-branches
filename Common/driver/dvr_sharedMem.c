/***********************************************************************************************************************

   Filename:   dvr_sharedMem.c

   Global Designator: DVR_SHM_

   Contents: Shared memory - Used between the internat and external device driver for read-modify-write opperations.


 ***********************************************************************************************************************
   A product of Aclara Technologies LLC Confidential and Proprietary Copyright 2015 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   @author Mario Deschenes

   $Log$ MKD Created August 17, 2015

 ***********************************************************************************************************************
   Revision History:
   v0.1 - MKD 08/17/2015 - Initial Release

   @version    0.1
   #since      2015-08-17
 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "project.h"
#include "dvr_sharedMem.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

//This is 4096 byte RAM area available to non-Flex Memory Kinetis devices, so let's use it

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* GLOBAL FUNCTION PROTOTYPES - Accessed via a table */

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#ifndef __BOOTLOADER
static OS_MUTEX_Obj dvrShmFlashMutex_;
static bool         dvrShmMutexCreated_ = false;


//  Put it into external RAM.   The internal RAM block is too small
#endif

// Typically the Programming Acceleration RAM shared by the internal and external Flash read/write functions
//NOTE: DCU 3 targets use non-4KB sectors, ensure this is assigned to the appropriate RAM space
__no_init union dvr_shm_type dvr_shm_t;

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
/***********************************************************************************************************************

   Function Name: dvr_shm_init

   Purpose: Init Function

   Arguments: none

   Returns: returnStatus_t

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t dvr_shm_init( void )
{
#ifndef __BOOTLOADER
   returnStatus_t retVal = eFAILURE;

#if ( RTOS_SELECTION == MQX_RTOS ) // TODO: Melvin: revist once the critical section is completed
   OS_INT_disable( ); // critical section
#endif
   if (dvrShmMutexCreated_ == true)
   {
      retVal = eSUCCESS;
   } else {
      /* The mutex has not been created, create it. */
      /* The shared memory can only be used by one device at the time. */
      if ( true == OS_MUTEX_Create(&dvrShmFlashMutex_) )
      {
         dvrShmMutexCreated_ = true;
         retVal = eSUCCESS;
      } /* end if() */
   } /* end if() */

#if ( RTOS_SELECTION == MQX_RTOS ) 
   OS_INT_enable( );
#endif
   return retVal;
#else
   return eSUCCESS;
#endif   /* BOOTLOADER  */
}

/***********************************************************************************************************************

   Function Name: dvr_shm_lock

   Purpose: Lock shared memory

   Arguments: none

   Returns: none

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
void dvr_shm_lock( void )
{
#ifndef __BOOTLOADER
   OS_MUTEX_Lock ( &dvrShmFlashMutex_ );   // Function will not return if it fails
#endif   /* BOOTLOADER  */
}

/***********************************************************************************************************************

   Function Name: dvr_shm_unlock

   Purpose: unlock shared memory

   Arguments: none

   Returns: none

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
void dvr_shm_unlock( void )
{
#ifndef __BOOTLOADER
   OS_MUTEX_Unlock ( &dvrShmFlashMutex_ ); // Function will not return if it fails
#endif   /* BOOTLOADER  */
}


