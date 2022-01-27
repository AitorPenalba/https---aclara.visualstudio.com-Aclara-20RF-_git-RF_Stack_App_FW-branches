// <editor-fold defaultstate="collapsed" desc="File Header Information">
/* ****************************************************************************************************************** */
/***********************************************************************************************************************

   Filename:   virgin_device

   Global Designator: VDEV_

   Contents: Check for first time power up

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   @author Manoj Singla

   $Log$ Created Feb 6, 2013

 ***********************************************************************************************************************
   Revision History:
   v0.1 - MS 02/06/2013 - Initial Release

   @version    0.1
   #since      2013-02-06
 **********************************************************************************************************************/
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="PSECTs">
/* PSECTS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Include Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#define VDEV_GLOBALS
#include "virgin_device.h"
#undef  VDEV_GLOBALS

#include "partitions.h"
#include "buffer.h"

#if ( EP == 1)
#include "pwr_last_gasp.h"
#include "vbat_reg.h"
#endif

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Constants">
/* ****************************************************************************************************************** */
/* CONSTANTS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Variables (File Static)">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static PartitionData_t const  *pVDEVPTbl_;             // Virgin device signature partition
static PartitionData_t const  *pWholeNVPTbl_;          // The full NV chip
static bool                   bVirginAtPwrUp_ = (bool)false; // Indicates device was a virgin unit

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

static returnStatus_t virginDeviceSetup( void );
static bool VDEV_isNVEmpty( void );

// </editor-fold>
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="static returnStatus_t virginDeviceSetup(void)">
/***********************************************************************************************************************

   Function Name: virginDeviceSetup

   Purpose: Erase complete NV and set the registers to default values

   Arguments: None

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: Erase complete NV and set the registers to default values

   Reentrant Code: NO

 **********************************************************************************************************************/
static returnStatus_t virginDeviceSetup( void )
{

   returnStatus_t RetVal = eFAILURE  ; //Return status

#if ( EP == 1)
   VBATREG_PWR_QUAL_COUNT = 0;      /* Reset power quality count (anomaly count) */
   VBATREG_SHORT_OUTAGE = 0;        /* No outage in process.   */
   PWRLG_FLAGS().byte = 0;          /* Clear the last gasp bit fields.  */
   PWRLG_SOFTWARE_RESET_SET( 1 );   /* Declare this as a software reset - no outage declared.   */
#endif

   if(!VDEV_isNVEmpty())  /* If NV is not already erased, erase the NV */
   {
      if ( eSUCCESS != PAR_partitionFptr.parErase( 0, EXT_FLASH_SIZE, pWholeNVPTbl_ ) ) // Erase the whole flash
      {
         RESET();       /* EXT NV erase failed - can't proceed! NV in unknown state, PWR_SafeReset() not necessary */
      }
   }

   // Flash is erased, Write the signature in flash and set virgin at power up
   if ( eSUCCESS == PAR_partitionFptr.parWrite( 0, SIGNATURE, sizeof( SIGNATURE ), pVDEVPTbl_ ) )
   {
      // Wrote the signature successfully, reset the registers to default
      RetVal = eSUCCESS;
      bVirginAtPwrUp_ = (bool)true;
   }
   else
   {
      RESET();       /* EXT NV write failed - can't proceed! NV erased, PWR_SafeReset() not necessary */
   }

   return( RetVal );

}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t VDEV_init(void)">
/***********************************************************************************************************************

   Function Name: VDEV_init

   Purpose:    Check if this is a virgin device. If virgin device, erase complete NV and set the registers to default
               values

   Arguments: None

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: If virgin device, erase complete NV and set the registers to default values

   Reentrant Code: NO

 **********************************************************************************************************************/
returnStatus_t VDEV_init( void )
{
   returnStatus_t RetVal = eSUCCESS;   // Return status

   /* xxx Todo! If the NV memory is virgin, the chip will be erased.  However, the ext
      flash driver will try to put the processor to sleep which shouldn't be done until the rest of the tasks are
      initialized.  The driver will have to be modified in a way that will allow us to erase the flash memory without
      putting the task to sleep.  Other initialization modules may need to read from NV memory, so this needs to be called
      early in the process. */

   uint8_t vSignature[SIGNATURE_SIZE];   // Holds NV signature

   // Open partition containing virgin device signature
   if ( !PAR_partitionFptr.parOpen( &pVDEVPTbl_, ePART_NV_VIRGIN_SIGNATURE, 0 ) )
   {
      //Partition opened successfully
      bool good = ( bool )false;
      /* Since the consequence of improperly reading the device signature is erasure of the entire device, read the
         signature MULTIPLE times. Any good read exits the loop; only exhaustion of the retry counter results in an
         erasure. For a truly virgin device, the time added for the extra reads is insignificant.
      */
      for (  uint8_t retry = 100; !good && retry; retry-- )
      {
         if ( !PAR_partitionFptr.parRead( &vSignature[0], 0, sizeof( vSignature ), pVDEVPTbl_ ) )
         {
            // Read the partition successfully, check if the device is virgin
            if ( 0 != memcmp( &vSignature[0], SIGNATURE, sizeof( SIGNATURE ) ) )
            {
               // Virgin unit
#if defined __BOOTLOADER
               // In the boot loader code, we can only validate the signature but we can't reprogram it
               // since the device driver can only read NV
               RetVal = eFAILURE;
#else
               /* Failed to verify the signature for MAX_VSIG_RETRY_CNT, erase the NV and set register to default */
               // Open a partition to access the whole flash
               if ( retry == 1 )
               {
                  RetVal = virginDeviceSetup(); //Virgin unit
               }
#endif
            }
            else
            {
               good = ( bool )true;
            }
            //else the device is not virgin
         }
         else
         {
            RESET();       /* EXT NV read failed - can't proceed! PWR_SafeReset() not necessary  */
         }
      }
   }
   else
   {
      RESET();       /* EXT NV open failed - can't proceed! PWR_SafeReset() not necessary */
   }

   return( RetVal );
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="bool VDEV_wasVirgin(void)">
/***********************************************************************************************************************

   Function Name: VDEV_wasVirgin

   Purpose: Returns true if device was virgin and false if device was not a virgin at power up.

   Arguments: None

   Returns: bool

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
bool VDEV_wasVirgin( void )
{
   return( bVirginAtPwrUp_ );
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="bool VDEV_isNVEmpty(void)">
/***********************************************************************************************************************

   Function Name: VDEV_isNVEmpty

   Purpose:    Check if the NV device is already empty.

   Arguments: None

   Returns: bool - true if device is empty, false if device is not empty

   Side Effects: None, but MUST be called from Startup Init since using RESET() instead of PWR_SafeReset()

   Reentrant Code: NO

 **********************************************************************************************************************/
static bool VDEV_isNVEmpty( void )
{

   bool nvEmpty = true;
   buffer_t *testBuffer = BM_alloc(1024);
   buffer_t *dataBuffer = BM_alloc(1024);

   if (testBuffer == NULL || dataBuffer == NULL)
   {
      RESET(); /* PWR_SafeReset() not necessary when function called during startup */
   }

   else
   {
      uint32_t currentAddress;
      uint32_t maxAddress;
      ( void )memset(testBuffer->data, 0, testBuffer->bufMaxSize);
      if ( !PAR_partitionFptr.parOpen( &pWholeNVPTbl_, ePART_WHOLE_NV , 0 ) )
      {
         maxAddress = pWholeNVPTbl_->lSize / 2;
         for ( currentAddress = pWholeNVPTbl_->PhyStartingAddress; currentAddress < maxAddress &&
               nvEmpty == true; currentAddress += dataBuffer->bufMaxSize )
         {
            if (!PAR_partitionFptr.parRead(dataBuffer->data, (dSize)currentAddress, (lCnt)dataBuffer->bufMaxSize, pWholeNVPTbl_))
            {
               if (memcmp(testBuffer->data, dataBuffer->data, dataBuffer->bufMaxSize))
               {
                  nvEmpty = false;
                  break;
               }
            }
            else
            {
               RESET(); /* PWR_SafeReset() not necessary when function called during startup */
            }
         }
      }
      else
      {
         RESET(); /* PWR_SafeReset() not necessary when function called during startup */
      }

      BM_free(testBuffer);
      BM_free(dataBuffer);
   }

   return nvEmpty;
}
// </editor-fold>
