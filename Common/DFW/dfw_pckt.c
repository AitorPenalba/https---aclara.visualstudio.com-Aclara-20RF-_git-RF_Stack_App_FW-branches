// <editor-fold defaultstate="collapsed" desc="File Header">
/***********************************************************************************************************************
 *
 * Filename:   dfw_pckt.c
 *
 * Global Designator: DFWP_
 *
 * Contents:  Saves downloaded packet and contains the bit array for keeping track of what packets have been received
 * when downloading code.
 *
 * Notes: Per the PDR, Packet Count is 14-bit.  This means the bit field needs to be 2048 bytes long, reserved in NV.
 *
 * When clearing the bit filed, use 'DFWP_clearBitField()' to set a flag so that the bit field is cleared
 * at the lowest priority from the main loop.  Main loop should call 'DFWP_ClearBitMap()'.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2004-2014 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Karl Davlin
 *
 * $Log$ Created July 19, 2004
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - 07/19/2004 - Initial Release
 * ...
 * v1.2 - (dac) Modified Bit_Field_Clear for EEPROM which requires 5ms per write
 * v2.0 01-29-2014 - KAD - Modified for PIC24 project (I210).  (Continuing with Mario's modifications)
 *                         Renamed functions to be more descriptive and hopefully easier to understand.
 *                         Cleaned up the code, added comments, etc...
 *                         Added field to 'DFWP_getMissingPackets' so this module is more compatible with VLF project.
 *                         Function modifyBit() now takes an enum.  It used to have macro definitions.
 *                         Added a global flag to determine if the NV memory needs to be read to get the status.  This
 *                            will save NV memory access (reduce SPI traffic for external memory).
 *
 * @version    2.0
 * #since      2014-02-10
 **********************************************************************************************************************/
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#define DFWP_GLOBALS
#include "dfw_pckt.h"
#undef DFWP_GLOBALS

#include "partitions.h"
#include "partition_cfg.h"

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Constant Definitions">
/* ****************************************************************************************************************** */
/* CONSTANTS */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* The count size is the bytes reserved in the partition manager for DFW bit map. */
/* WARNING the count size must be kept at 4095 or less to allow Missing Packets routine ranges */
#define DFWP_PCKT_CNT_SIZE_BYTES  ((uint16_t)2048)                                    // Maximum packets in Bytes
#define DFWP_PCKT_CNT_SIZE_BITS   ((uint16_t)(DFWP_PCKT_CNT_SIZE_BYTES * CHAR_BIT))   // Maximum packets in Bits
/* WARNING the count size must be kept at 4095 or less to allow Missing Packets routine ranges */

#ifndef CHAR_BIT
#define CHAR_BIT  ((uint8_t)8)
#endif

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="File Variables">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static PartitionData_t const *pDFWBitfieldPTbl_; // Pointer to the DFW bit field partition

/* Used to reduce the NV traffic in the timeSlice function.  If set the timeSlice function will read the status from NV
 * memory to see if action is required.  */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

// </editor-fold>

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="returnStatus_t DFWP_init(void)">
/***********************************************************************************************************************
 *
 * Function Name: DFWP_init
 *
 * Purpose: Open bit field partition for DFW packets.
 *
 * Arguments: None
 *
 * Returns: returnStatus_t - defined by error_codes.h
 *
 * Side Effects: None
 *
 * Reentrant Code: Yes
 *
 * Notes:  Uses a named partition, ePART_NV_DFW_BITFIELD, so the timing will be '0'.
 *
 **********************************************************************************************************************/
returnStatus_t DFWP_init( void )
{
   return (PAR_partitionFptr.parOpen(&pDFWBitfieldPTbl_, ePART_NV_DFW_BITFIELD, (uint32_t)0));// Open DFW bit field file
}
/* ****************************************************************************************************************** */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="returnStatus_t DFWP_getBitStatus(uint16_t pcktNum, uint8_t *pBit)">
/***********************************************************************************************************************
 *
 * Function name: DFWP_getMaxPackets
 *
 * Purpose: Reads a bit in the bit field in the appropriate location to find out if a packet was received or not.
 *          Result returned via passed pointer, *pBit.
 *
 * Arguments:  uint16_t pcktNum - Packet number in the sequence 0-16383
 *             uint8_t *pBit - *pBit set to 1 if associated bit in bit field set, 0 otherwise
 *
 * Returns: returnStatus_t - defined by error_codes.h
 *
 * Side effect: N/A
 *
 * Reentrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint16_t DFWP_getMaxPackets( void )
{
   return(DFWP_PCKT_CNT_SIZE_BITS);
}
/* ****************************************************************************************************************** */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="bool DFWP_ClearBitMap(void)">
/***********************************************************************************************************************
 *
 * Function name: DFWP_ClearBitMap
 *
 * Purpose: Reads the clrBitArrayStatus from NV memory.  If set, the bit field is erased.
 *
 * Arguments: None
 *
 * Returns: bool - If NV is written (significant CPU time was taken), true is returned.
 *
 * Side Effects: An erase to NV may occur taking CPU time.
 *
 * Reentrant Code: No
 *
 * Notes:  Since clearing the bit field partition and clearing the status each requires an erase cycle, they need to be
 *         completed in multiple passes.  So, first clear the bit field partition and then the next pass clear the NV
 *         status flag.
 *
 **********************************************************************************************************************/
bool DFWP_ClearBitMap( void )
{
#if 0
   static bool    bClearStatus_ = false;  // Indicates that the NV status needs to be cleared.
          bool    bTimeSpent    = false;  // Assume no time has been taken

   if (bReadStatusFromNv_)                   // Is there a need to read the NV memory to see if an update is required?
   {
      DFW_vars_t dfwVars;                    // Contains all of the DFW file variables

      (void)DFWA_getFileVars(&dfwVars);      // Read the DFW NV variables (status, etc...)
      if (bClearStatus_)                     // Need to clear the status flag?
      {  // Yes - clear the status flag
         dfwVars.clrBitArrayStatus = false;  // Clear the flag
         bReadStatusFromNv_ = false;         // No longer need to read the NV status.
         bClearStatus_ = false;              // No longer need to clear the status flag
         bTimeSpent = true;                  // An NV erase/write just occurred, so time was spent
         (void)DFWA_setFileVars(&dfwVars, eDFWF_FLUSH_BACKGROUND); // Write the DFW NV variables (status, etc...)
      }
      else // Need to check if the status is set.  If set, clear the bit array partition.
      {
         if ( dfwVars.clrBitArrayStatus )    // Clear (erase) the bit array partition?
         {  // Erase download control bit field
            CLRWDT();
            if ( eSUCCESS == PAR_partitionFptr.parErase((lAddr)0, PART_NV_DFW_BITFIELD_SIZE, pDFWBitfieldPTbl_) )
            {  // Success!
               bClearStatus_ = true;         // Next time slice, need to clear the status flag.
            }
            bTimeSpent = true;               // Time was spent
         }
         else  // The bit field does not need to be cleared, no need to read the status from NV anymore.
         {
            bReadStatusFromNv_ = false;      // No longer need to read the NV status.
         }
      }
   }
#else
   bool    bTimeSpent = false;  // Assume no time has been taken

   CLRWDT();
   if ( eSUCCESS == PAR_partitionFptr.parErase((lAddr)0, PART_NV_DFW_BITFIELD_SIZE, pDFWBitfieldPTbl_) )
   {  // Success!
      bTimeSpent = true;               // Time was spent
   }
#endif
   return(bTimeSpent);
}
/* ****************************************************************************************************************** */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="returnStatus_t DFWP_getBitStatus(uint16_t pcktNum, uint8_t *pBit)">
/***********************************************************************************************************************
 *
 * Function name: DFWP_getBitStatus
 *
 * Purpose: Reads a bit in the bit field in the appropriate location to find out if a packet was received or not.
 *          Result returned via passed pointer, *pBit.
 *
 * Arguments:  uint16_t pcktNum - Packet number in the sequence 0-16383
 *             uint8_t *pBit - *pBit set to 1 if associated bit in bit field set, 0 otherwise
 *
 * Returns: returnStatus_t - defined by error_codes.h
 *
 * Side effect: N/A
 *
 * Reentrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t DFWP_getBitStatus( uint16_t pcktNum, uint8_t *pBit )
{
   returnStatus_t retVal = eFAILURE;   // Assume error
   uint8_t          rdByte;              // rdByte containing the bit to look at

   if ( pcktNum < DFWP_PCKT_CNT_SIZE_BITS ) // Check for valid size
   {  // Read byte from bit field containing requested packet's bit
      retVal = PAR_partitionFptr.parRead(&rdByte, (dSize)(pcktNum/CHAR_BIT), (lCnt)(sizeof(rdByte)), pDFWBitfieldPTbl_);
      /* If the read above were to fail, the *pBit will be updated with an incorrect value.  It doesn't matter because
       * the returned value will be eFAILURE.  So, to save code, don't bother checking if the data was read properly. */
      *pBit = (uint8_t)((rdByte >> (pcktNum % (uint8_t)CHAR_BIT)) & 1); /* Update the pBit value w/corresponding bit */
   }
   return(retVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>


// <editor-fold defaultstate="collapsed" desc="returnStatus_t DFWP_getMissingPackets(dl_packetcnt_t totPckts, dl_packetcnt_t maxPcktsReported, dl_packetcnt_t *pNumMissingPckts, dl_packetid_t *pMissingPcktIds)">
/***********************************************************************************************************************
 *
 * Function name: DFWP_getMissingPackets
 *
 * Purpose: Finds the total number of missing packets and also returns the 1st 'maxPcktsReported' missing packet IDs.
 *
 * Arguments: dl_packetcnt_t totPckts          - Total Packets in bit field
 *            dl_packetcnt_t maxPcktsReported  - Maximum number of missing packets to report in pNumMissingPckts
 *            dl_packetcnt_t *pNumMissingPckts - Number of Missing Packets found (up to maxPcktsReported)
 *            dl_packetid_t  *pMissingPcktIds  - Array of Missing Pckt IDs.  Uses 'maxPcktsReported' for array cnt size
 *
 * Returns: returnStatus_t - defined by error_codes.h
 *
 * Side effect: None
 *
 * Reentrant Code: Yes
 *
 * Notes:  On a PIC24 @ 16MHz, it takes 53mS to report on 2048 bytes!
 *
 **********************************************************************************************************************/
returnStatus_t DFWP_getMissingPackets(dl_packetcnt_t  totPckts,         dl_packetcnt_t maxPcktsReported,
                                      dl_packetcnt_t *pNumMissingPckts, dl_packetid_t *pMissingPcktIds)
{
   returnStatus_t retVal       = eFAILURE;         // Error code
   uint16_t       missingPckts = 0;                // Local copy of the number of missing packets.

   if ( (0 != totPckts) && (totPckts <= DFWP_PCKT_CNT_SIZE_BITS) )      // Check for valid size
   {  // Scan each bit in bit field to find those that are 0.  Stop scanning when maxPcktsReported are found
      uint32_t      offset;                        // Offset of current byte in bitfield
      dl_packetid_t bitInBitmap;                   // Loop counter that iterates through all bits in bit field
      uint8_t       mapData = 0;                   // Copy of NV memory bit array

      offset       = 0;
      for ( bitInBitmap = 0; bitInBitmap < totPckts; bitInBitmap++ )
      {  //Get a byte of the bit map field when needed
         if ( bitInBitmap % CHAR_BIT == 0)
         {
            /*lint -e(776) Only read the curent byte needed. */
            retVal = PAR_partitionFptr.parRead(&mapData, (dSize)offset, (lCnt)1, pDFWBitfieldPTbl_);
            if (eSUCCESS != retVal)
            {
               break;   //Exit loop and function if unable to read data
            }
            offset++;
         }
         // Check if we have a missing packet at offset bitInBitmap
         if ( 0 == (mapData & (1 << (bitInBitmap % CHAR_BIT))) )
         {  // A missing packet has been found!
            if (missingPckts < maxPcktsReported)   // Have the maximum number of missing packets been reported?
            {  // Number of missing packets reported hasn't exceeded the requested maximum.
               if (NULL != pMissingPcktIds)
               {
                  //lint -e(413)  NULL is checked for above.
                  pMissingPcktIds[missingPckts] = bitInBitmap; // Store missing pckt ID.  Using missingPckts as index
               }
            }
            missingPckts++;  // Increment the number of missing packets.  Done here so it can be used as an index.
         }
      }
   }
   if (NULL != pNumMissingPckts)
   {
      *pNumMissingPckts = missingPckts;
   }
   return(retVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="returnStatus_t DFWP_getMissingPacketsRange(dl_packetcnt_t totPckts, dl_packetcnt_t maxPcktsReported, dl_packetcnt_t *pNumMissingPckts, dl_packetid_t *pMissingPcktIds)">
/***********************************************************************************************************************
 *
 * Function name: DFWP_getMissingPacketsRange
 *
 * Purpose: Finds the total number of missing packets and also returns the 1st 'maxPcktsReported' missing packet IDs.
 *          If two or more PacketId's are consecutive, then the last consecutive PacketId is made negative to indicate
 *          a packet range.  pNumMissingPckts will be set to the number packet IDs in pMissingPcktIds.
 *          e.g. pMissingPcktIds[0]=12, pMissingPcktIds[1]=15, pMissingPcktIds[2]=20, pMissingPcktIds[3]=-26,
 *          pMissingPcktIds[4]=31 etc. for missing packets 12, 15, 20 through 26, 31 etc.  If these were the only
 *          missing packets, then pNumMissingPckts would be set to 5.
 *
 * Arguments: dl_packetcnt_t totPckts          - Total Packets in bit field
 *            dl_packetcnt_t maxPcktsReported  - Maximum number of missing packets to report in pNumMissingPckts
 *            dl_packetcnt_t *pNumMissingPckts - Number of Missing Packets found (up to maxPcktsReported)
 *            dl_packetid_t  *pMissingPcktIds  - Array of Missing Pckt IDs.  Uses 'maxPcktsReported' for array cnt size
 *
 * Returns: returnStatus_t - defined by error_codes.h
 *
 * Side effect: None
 *
 * Reentrant Code: Yes
 *
 * Notes:  On a K60 @ ??MHz, it takes ??mS to report on 2048 bytes!
 *
 **********************************************************************************************************************/
returnStatus_t DFWP_getMissingPacketsRange(dl_packetcnt_t  totPckts,         dl_packetcnt_t maxReportPckts,
                                           dl_packetcnt_t *pNumMissingPckts, dl_packetid_t *pMissingPcktIds)
{
   returnStatus_t retVal   = eFAILURE;    // Error code

   if ( (0 != totPckts)                       &&      // Can't be zero
        (totPckts <= DFWP_PCKT_CNT_SIZE_BITS) &&      // Check for valid size
        (    NULL != pMissingPcktIds)         &&      // Check for valid pointer
        (    NULL != pNumMissingPckts)        )       // Check for valid pointer
   {  // Scan each bit in bit field to find those that are 0.  Stop scanning when maxPcktsReported are found
      dl_packetid_t bitInBitmap;          // Loop counter that iterates through all bits in bit field
      uint8_t       mapData = 0;          // Copy of NV memory bit array
      uint32_t      offset;               // Offset of current byte in bitfield
      uint16_t      pktIndex;             // Index in pMissingPcktIds.
      dl_packetid_t nextID;               // Next PacketID when checking for consecutive ID's
      uint8_t       consecutive;          // Used to track consecutive packet ID's

      offset      = 0;
      pktIndex    = 0;
      consecutive = false;
      nextID      = 0;        // Just to make sure it is initialized
      for ( bitInBitmap = 0; bitInBitmap < totPckts; bitInBitmap++ )
      {  //Get a byte of the bit map field when needed
         if ( bitInBitmap % CHAR_BIT == 0)
         {
            /*lint -e(776) Only read the curent byte needed. */
            retVal = PAR_partitionFptr.parRead(&mapData, (dSize)offset, (lCnt)1, pDFWBitfieldPTbl_);
            if (eSUCCESS != retVal)
            {
               break;   //Exit loop and function if unable to read data
            }
            offset++;
         }
         // Check if we have a missing packet at offset bitInBitmap
         if ( 0 == (mapData & (1 << (bitInBitmap % CHAR_BIT))) )
         {  // A missing packet has been found!
            if (pktIndex < maxReportPckts)   // Have the maximum number of missing packets been reported?
            {  // Number of missing packets reported hasn't exceeded the requested maximum.
               if (0 == pktIndex)  /* Have to save the first one to get started */
               {
                  pMissingPcktIds[pktIndex++] = bitInBitmap;
                  nextID                      = bitInBitmap + 1;
               }
               else
               {
                  /*lint -e(644) Variable is initialized when pktIndex is 0 */
                  if (nextID == bitInBitmap)
                  {
                     nextID      = bitInBitmap + 1;
                     consecutive = true; // indicate that we have found a range of missing packets

                     // Special case if it is the last packet...
                     if ( (totPckts - 1) <= bitInBitmap )
                     {  // Make it negative to indicate it is the last consecutive packet of a range
                        /*lint -e{734} Loss of precision acceptable as long as Max Packet Count < INT16_MAX */
                        pMissingPcktIds[pktIndex++] = -1 * bitInBitmap;
                     }
                  }
                  else // It is not a consecutive ID
                  {
                     // Put the next non-consecutive missing packet id in the buffer
                     pMissingPcktIds[pktIndex++] = bitInBitmap;
                     nextID                      = bitInBitmap + 1;
                     consecutive                 = false;
                  }
               }
            }  //end of else to if (0 == pktIndex)
            else
            {  // No more room to add packet ID's, break out of loop to exit function
               break;
            }
         }  //end of if (pktIndex < maxReportPckts)
         else
         {  // packet is not missing

            // check to see if a consectutive range has ended, if so add range end to packet ID list
            if( true == consecutive )
            {
               pMissingPcktIds[pktIndex++] = (dl_packetid_t)(-1 * (nextID - 1));
               consecutive = false;
            }
         }
      }  //end of for ( bitInBitmap = 0; ...
      *pNumMissingPckts = pktIndex;
   }  //end of if ( (totPckts <= DFWP_PCKT_CNT_SIZE_BITS) && ...
   return(retVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="returnStatus_t modifyBit(uint16_t packetNumber, eDFWP_BitFieldModify_t action)">
/***********************************************************************************************************************
 *
 * Function name: DFWP_modifyBit
 *
 * Purpose: Sets or clears a bit in the NV memory bit array that corresponds to the packetNumber passed in.
 *
 * Arguments:  uint16_t                  packetNumber - Packet number in the sequence 0-16383
 *             eDFWP_BitFieldModify_t  action       - Set or clear bit
 *
 * Returns: returnStatus_t - defined by error_codes.h
 *
 * Side effect: NV memory may take an endurance hit.
 *
 * Reentrant Code: No
 *
 * Notes:  May cause an erase cycle in NV memory (for flash memory and possibly EEPROM)
 *
 **********************************************************************************************************************/
returnStatus_t DFWP_modifyBit( uint16_t packetNumber, eDFWP_BitFieldModify_t action )
{
   returnStatus_t RetVal = eFAILURE;               // Error code

   if ( packetNumber < DFWP_PCKT_CNT_SIZE_BITS )   // Check for valid size, Is the packet number in the bit field?
   {  // Yes - Compute offset and mask
      uint8_t          Byte;                         // Byte that will be modified
      uint8_t          BitMask;                      // Bit mask to set or clear a bit
      uint16_t         ByteOffset;                   // Byte offset in the bitfield

      ByteOffset = packetNumber / CHAR_BIT;        // There are 8 bits/byte
      BitMask    = 1 << (packetNumber % CHAR_BIT); // Get the mask for the requested packet number within the byte.
      // Read byte from bit field containing requested packet's bit
      RetVal = PAR_partitionFptr.parRead(&Byte, (dSize)ByteOffset, (lCnt)sizeof(Byte), pDFWBitfieldPTbl_);
      if ( eSUCCESS == RetVal )
      {  // Set bit or clear bit
         if ( eDL_BIT_SET == action )              // Set the bit?
         {
            Byte |= BitMask;                       // Set bit
         }
         else
         {
            Byte &= ~BitMask;                      // Clear bit
         }
         // Update bit field
         RetVal = PAR_partitionFptr.parWrite((dSize)ByteOffset, &Byte, (lCnt)sizeof(Byte), pDFWBitfieldPTbl_);
      }
   }
   return(RetVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>
