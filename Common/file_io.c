/***********************************************************************************************************************

   Filename:   file_io.c

   Global Designator: FIO_

   Contents: File I/O module

 ***********************************************************************************************************************
   Copyright (c) 2011-2020 Aclara Technologies LLC.  All rights reserved.  This program may not be reproduced, in
   whole or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA TECHNOLOGIES LLC.
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
   Revision History:
   v0.1, 20110415, KAD  - Initial Release

 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include <stdbool.h>
#include <string.h>
#include <mqx.h>
#include <mutex.h>
#include <fio.h>
#include "crc32.h"
#include "filenames.h"
#include "EVL_event_log.h"

#ifdef TM_PARTITION_USAGE
#include "DBG_SerialDebug.h"
#endif

#define file_io_GLOBAL
#include "file_io.h"
#undef  file_io_GLOBAL

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* An invalid file id is based upon 0xFFFF ( erased memory) being inverted to 0 after a read.*/
#define INVALID_FILE_ID ((uint16_t) 0x0000)

#define HDR_BYTES_IN_CHECKSUM ((uint32_t)sizeof(tFileHeader) - (uint32_t)offsetof(tFileHeader, dataSize))
/* For Unit Testing, we need to return a FAILURE in place of the assert.  */
#ifdef TM_FILE_IO_UNIT_TEST
#undef ASSERT
#define ASSERT(__e) if (!(__e)) return(eFAILURE)
#endif

#ifdef TM_PARTITION_USAGE
#define TEST_MAX_PARTITION_ID ((uint8_t)12)
/*lint -esym(765,lDataUsage)  */
/*lint -esym(552,lDataUsage)  */
volatile lCnt lDataUsage[TEST_MAX_PARTITION_ID];  /* Used to check search by timimg partitions usage */
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   tFileHeader   sHeader;                                  /* Contains the file header information */
   uint8_t       u8Data[FIO_MAX_FILE_SIZE_WITH_CHECKSUM];  /* Used to hold the data */
} tCsFile;   /* Used to perform read-modify-write operations for files with a checksum attribute. */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#if RTOS
static OS_MUTEX_Obj  fioMutex_;  /* Serialize access to the file i/o driver for files with a checksum. */
#endif
STATIC tCsFile       _sFile;     /* Buffer to store file contents for read-modify-write ops on checksumed files */
STATIC bool          runNVTest_; /* Flag to indicate, if time to run NV test */

/* Create macro to make a list of filenames whose names are exactly their respective enums.
   example: FILENAME( eFN_DST ) expands to [ eFN_DST ] = "eFN_DST"   */
#define FILENAME( x ) [ x ] = #x

static char * const fileNames[] =
{
#if ( EP == 1 )
   FILENAME( eFN_UNDEFINED ),
   FILENAME( eFN_DST ),
   FILENAME( eFN_ID_CFG ),
   FILENAME( eFN_MTRINFO ),
   FILENAME( eFN_MTR_DS_REGISTERS ),

   /*  5  */
   FILENAME( eFN_HD_DATA ),
   FILENAME( eFN_ID_META ),
   FILENAME( eFN_PWR ),
   FILENAME( eFN_HMC_ENG ),
   FILENAME( eFN_PHY_CACHED ),

   /*  10 */
   FILENAME( eFN_PHY_CONFIG ),
   FILENAME( eFN_MAC_CACHED ),
   FILENAME( eFN_MAC_CONFIG ),
   FILENAME( eFN_NWK_CACHED ),
   FILENAME( eFN_NWK_CONFIG ),

   /*  15 */
   FILENAME( eFN_REG_STATUS ),
   FILENAME( eFN_TIME_SYS ),
   FILENAME( eFN_DEMAND ),
   FILENAME( eFN_SELF_TEST ),
   FILENAME( eFN_HMC_START ),

   /*  20 */
   FILENAME( eFN_RTI ),
   FILENAME( eFN_HMC_DIAGS ),
   FILENAME( eFN_MACU ),
   FILENAME( eFN_PWRCFG ),
   FILENAME( eFN_ID_PARAMS ),

   /*  25 */
   FILENAME( eFN_MODECFG ),
   FILENAME( eFN_SECURITY ),
   FILENAME( eFN_RADIO_PATCH ),
   FILENAME( eFN_DFWTDCFG ),
   FILENAME( eFN_SMTDCFG ),

   /*  30 */
   FILENAME( eFN_DMNDCFG ),
   FILENAME( eFN_EVL_DATA ),
   FILENAME( eFN_HD_FILE_INFO ),
   FILENAME( eFN_OR_MR_INFO ),
   FILENAME( eFN_EDCFG ),

   /*  35 */
   FILENAME( eFN_DFW_VARS ),
   FILENAME( eFN_EDCFGU ),
   FILENAME( eFN_DTLS_CONFIG ),
   FILENAME( eFN_DTLS_CACHED ),
   FILENAME( eFN_DTLS_MAJOR ),

   /*  40 */
   FILENAME( eFN_DTLS_MINOR ),
   FILENAME( eFN_FIO_CONFIG ),
   FILENAME( eFN_DR_LIST ),
   FILENAME( eFN_DBG_CONFIG ),
   FILENAME( eFN_EVENTS ),

   /*  45 */
   FILENAME( eFN_HD_INDEX ),
   FILENAME( eFN_MIMTINFO ),
   FILENAME( eFN_VERINFO ),
   FILENAME( eFN_TIME_SYNC ),
   FILENAME( eFN_MTLS_CONFIG ),

   /*  50  */
   FILENAME( eFN_MTLS_ATTRIBS ),
   FILENAME( eFN_DFW_CACHED ),
   FILENAME( eFN_TEMPERATURE_CONFIG ),
   FILENAME( eFN_APP_CACHED ),
   FILENAME( eFN_PD_CONFIG ),

   /*  55  */
   FILENAME( eFN_DTLS_WOLFSSL4X ),
   FILENAME( eFN_PD_CACHED ),

   [ eFN_LASTFILE ] = "File Not Found" // This is always last

#else
   FILENAME( eFN_UNDEFINED ),
   FILENAME( eFN_DST ),
   FILENAME( eFN_PWR ),
   FILENAME( eFN_PHY_CACHED ),
   FILENAME( eFN_PHY_CONFIG ),

   /*  5  */
   FILENAME( eFN_MAC_CACHED ),
   FILENAME( eFN_MAC_CONFIG ),
   FILENAME( eFN_NWK_CACHED ),
   FILENAME( eFN_NWK_CONFIG ),
   FILENAME( eFN_REG_STATUS ),

   /*  10 */
   FILENAME( eFN_SELF_TEST ),
   FILENAME( eFN_MODECFG ),
   FILENAME( eFN_SECURITY ),
   FILENAME( eFN_RADIO_PATCH ),
   FILENAME( eFN_EVL_DATA ),

   /*  15 */
   FILENAME( eFN_DFW_VARS ),
   FILENAME( eFN_DTLS_MAJOR ),
   FILENAME( eFN_DTLS_MINOR ),
   FILENAME( eFN_DTLS_CONFIG ),
   FILENAME( eFN_DTLS_CACHED ),

   /*  20 */
   FILENAME( eFN_EVENTS ),
   FILENAME( eFN_MACU ),
   FILENAME( eFN_FIO_CONFIG ),
   FILENAME( eFN_DBG_CONFIG ),
   FILENAME( eFN_VERINFO ),

   /*  25 */
   FILENAME( eFN_MIMTINFO ),
   FILENAME( eFN_TIME_SYNC ),
   FILENAME( eFN_DEVICEMODE ),
   FILENAME( eFN_TEMPERATURE_CONFIG ),
   FILENAME( eFN_SMTDCFG ),

   /*  30 */
   FILENAME( eFN_MTLS_CONFIG ),
   FILENAME( eFN_MTLS_ATTRIBS ),
   FILENAME( eFN_APP_CACHED ),
   FILENAME( eFN_DTLS_WOLFSSL4X ),

   [ eFN_LASTFILE ] = "File Not Found" // This is always last
#endif
}; /*lint !e785 too few initializers   */

#undef FILENAME

/*
   Given a file id, return the name of the file
*/
const char* getFileName( uint16_t fileId )
{
   return fileNames[min( fileId, ARRAY_IDX_CNT( fileNames ) - 1 )];
}


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

static uint16_t         FileChecksum( uint8_t const *pSource, lCnt Cnt );
static returnStatus_t   verifyFixFileChecksum(  tFileHeader const *pHeader, PartitionData_t const *pPartitionData,
                                                dSize FileOffset );
static uint32_t         calcFileCrc(            tFileHeader     const *pHeader,   PartitionData_t const *pPartitionData,
                                                dSize FileOffset );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function Name: FIO_finit

   Purpose: Creates the mutex for the file i/o module and initializes the partition manager.

   Arguments: None

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: All lower drivers will be initialized

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t FIO_finit( void )
{
   returnStatus_t retVal = eFAILURE;   /* Return value */

   if ( OS_MUTEX_Create( &fioMutex_ ) )
   {
      //Mutex create succeeded
      runNVTest_ = false;
      retVal = PAR_partitionFptr.parInit();
   }
   return retVal;
}
/***********************************************************************************************************************

   Function Name: FIO_fopen

   Purpose: Locates a file in a partition that meets the timing requirements.  If the file is not found, the file is
            created.  A file with a checksum is size limited.  See 'FIO_MAX_FILE_SIZE_WITH_CHECKSUM' in cfg_app.h.

   Arguments:
      *pFileHandle:  Contains the Driver information and the File Header Address.  The File IO module will populate the
                     data pointed to by pFile with all of the information that will be needed to access the file later.
      ePartition: Name of the Partition that will contain the file.
      fId:  ID of the file (filename)
      cnt:  Number of bytes to allocate for Data, the actual bytes for the file will be Data + the file header.
      FileAttr_t:  File Attributes to assign to the file
      u32UpdateFreq:  Frequency that the application will be updating the data
      FileStatus_t *pStatus = pointer to status of the file.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes - Should only be called at power up.

 **********************************************************************************************************************/
returnStatus_t FIO_fopen( FileHandle_t *pFileHandle, ePartitionName ePartition, uint16_t fId, lCnt cnt,
                          FileAttr_t fAttr, const uint32_t u32UpdateFreq, FileStatus_t *pStatus )
{
   returnStatus_t eRetVal;          /* Return value, used throughout the function to test for errors */

   OS_MUTEX_Lock( &fioMutex_ );

   pStatus->bFileCreated = false;
   /* Validate file size for files with checksum enabled */
   if ( ( fAttr & FILE_IS_CHECKSUMED ) && ( cnt > FIO_MAX_FILE_SIZE_WITH_CHECKSUM ) )
   {
      eRetVal = eFILEIO_FILE_CHCKSM_SIZE_ERROR;  /* File size too large to have a checksum! */
   }
   else
   {
      /* Validate Partition Exists in the Partition Layer */
      eRetVal = PAR_partitionFptr.parOpen( &pFileHandle->pTblInfo, ePartition, u32UpdateFreq );
      if ( eSUCCESS == eRetVal )
      {
         /* Search for the file */
         tFileHeader sHeader;          /* Contains the current file's header just read from memory */
         bool  bFileFound = false;     /* Set true if the file is found. */
         dSize FileOffset = 0;         /* Offset to the file in the partition.  Used to search the partition. */

         do
         {
            /* Read the file's header */
            eRetVal = PAR_partitionFptr.parRead( ( uint8_t * ) & sHeader, FileOffset, sizeof( sHeader ), pFileHandle->pTblInfo );

            if ( eSUCCESS == eRetVal )   /* Was the header read properly? */
            {
               if ( sHeader.Id == fId ) /* Was the file passed in found? */
               {
                  /* Found the file! */
#ifdef TM_PARTITION_USAGE
                  if ( ( uint8_t )pFileHandle->pTblInfo->ePartition < TEST_MAX_PARTITION_ID )
                  {
                     /* Log data for this partition */
                     lDataUsage[pFileHandle->pTblInfo->ePartition] = FileOffset + cnt + sizeof( sHeader );
                  }
#endif
                  bFileFound = true;
                  pFileHandle->FileOffset = FileOffset;
                  pFileHandle->Attr = fAttr;
                  pFileHandle->dataSize = ( FileDataSize_t )sHeader.dataSize; //return the size of file in NV, if the file size mismatch
                  if ( sHeader.dataSize != cnt )   /* Make sure the file size matches */
                  {
                     eRetVal = eFILEIO_FILE_SIZE_MISMATCH;
                  }
               }
               else if ( INVALID_FILE_ID == sHeader.Id )  /* Are we past the last file in this partition? */
               {
                  /* Will the new file fit in the partition? */
                  if ( ( FileOffset + cnt + sizeof( sHeader ) ) <= pFileHandle->pTblInfo->lDataSize )
                  {
#ifdef TM_PARTITION_USAGE
                     if ( ( uint8_t )pFileHandle->pTblInfo->ePartition < TEST_MAX_PARTITION_ID )
                     {
                        /* Log data for this partition */
                        lDataUsage[pFileHandle->pTblInfo->ePartition] = FileOffset + cnt + sizeof( sHeader );
                     }
#endif
                     pFileHandle->FileOffset = FileOffset;
                     pFileHandle->Attr = fAttr;
                     pFileHandle->dataSize = ( FileDataSize_t )cnt;

                     /* Setup the file header information, the checksum will be calculated later */
                     _sFile.sHeader.Id = fId;
                     _sFile.sHeader.Cs = 0;
                     _sFile.sHeader.dataSize = ( FileDataSize_t )cnt;
                     _sFile.sHeader.Attr = fAttr;

                     if ( _sFile.sHeader.Attr & FILE_IS_CHECKSUMED )
                     {
                        ( void )memset( &_sFile.u8Data[0], 0, ( uint16_t )cnt ); /*lint !e669 !e419   size checked above. */
                        /* Give the new file a correct checksum */
                        _sFile.sHeader.Cs = FileChecksum( ( uint8_t * ) & _sFile.sHeader.dataSize,
                                                          ( lCnt )( _sFile.sHeader.dataSize + HDR_BYTES_IN_CHECKSUM ) );

                        /* The file size was validated above, so just create the file. */
                        eRetVal = PAR_partitionFptr.parWrite( pFileHandle->FileOffset, ( uint8_t * ) & _sFile.sHeader,
                                                              ( lCnt )sizeof( _sFile.sHeader ) + ( lCnt )cnt, pFileHandle->pTblInfo );
                     }
                     else
                     {
                        eRetVal = PAR_partitionFptr.parWrite( pFileHandle->FileOffset, ( uint8_t * ) & _sFile.sHeader,
                                                              ( lCnt )sizeof( sHeader ), pFileHandle->pTblInfo );
                        if ( eSUCCESS == eRetVal )
                        {
                           eRetVal = PAR_partitionFptr.parErase( pFileHandle->FileOffset + sizeof( _sFile.sHeader ),
                                                                 cnt, pFileHandle->pTblInfo );
                        }
                     }
                     bFileFound = true;  /* It has just been created and so it is found. */
                     pStatus->bFileCreated = true;
                  }
                  else
                  {
#ifdef TM_PARTITION_USAGE
                     // Set to MAX + 1 to indicate out of memory
                     if ( ( uint8_t )pFileHandle->pTblInfo->ePartition < TEST_MAX_PARTITION_ID )
                     {
                        /* Log data for this partition */
                        lDataUsage[pFileHandle->pTblInfo->ePartition] = pFileHandle->pTblInfo->lDataSize + 1;
                     }
#endif
                     eRetVal = eFILEIO_MEM_RANGE_ERROR;  /* Range error! */
                  }
               }
               else
               {
                  /* Set offset to next file.  The read above will fail if the partition boundary is exceeded. */
                  /* If by chance the file is corrupt, we will walk a bunch of bogus memory. */
                  /* Are there any checks to perform? */
                  FileOffset += sHeader.dataSize + sizeof( sHeader );
               }
            }
         }
         while ( ( eSUCCESS == eRetVal ) && ( !bFileFound ) );

         /* Validate File */
         if ( ( eSUCCESS == eRetVal ) && ( sHeader.Attr & FILE_IS_CHECKSUMED ) ) //If there are no errors, validate the file
         {
            uint8_t u8RetryCnt = 1;   /* If the file doesn't validate, restore it and try again. */

            do
            {
               //Validate the file checksum and recompute, if requested
               eRetVal = verifyFixFileChecksum( &sHeader, ( PartitionData_t * )pFileHandle->pTblInfo,
                                                pFileHandle->FileOffset );
               if ( eRetVal != eSUCCESS )
               {
                  /* The checksum doesn't match, attempt to restore the file and then re-check the file. */
                  ( void )PAR_partitionFptr.parRestore( pFileHandle->FileOffset, ( lCnt )sHeader.dataSize + sizeof( sHeader ),
                                                        pFileHandle->pTblInfo );
               }
            }
            while ( ( eSUCCESS != eRetVal ) && ( u8RetryCnt-- ) );
         }  /*lint !e438 last value of u8RetryCnt not used  */
      } /* if (eSUCCESS == eRetVal) */
   }
   OS_MUTEX_Unlock( &fioMutex_ );
   return( eRetVal );
}
/***********************************************************************************************************************

   Function Name: FIO_fclose

   Purpose: Closes all of the partitions - For this implementation, the fclose does nothing.

   Arguments:  None

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t FIO_fclose( void )
{
   return( eSUCCESS );
}
/***********************************************************************************************************************

   Function Name: FIO_ferase

   Purpose:  Erases file contents (the file is not removed).

   Arguments:  *pFileHandle - file to erase the contents of.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t FIO_ferase( FileHandle_t const *pFileHandle )
{
   returnStatus_t eRetVal;  /* Return value */

   OS_MUTEX_Lock( &fioMutex_ );

   /* Read the file header information */
   eRetVal = PAR_partitionFptr.parRead( ( uint8_t * ) & _sFile, pFileHandle->FileOffset, sizeof( _sFile.sHeader ), pFileHandle->pTblInfo );

   if ( eSUCCESS == eRetVal )   /* Was the file header successfully read? */
   {
      /* Does this file have a checksum, if so, clear the content of the file & update the checksum. */
      if ( _sFile.sHeader.Attr & FILE_IS_CHECKSUMED )
      {
         ( void )memset( &_sFile.u8Data[0], 0, _sFile.sHeader.dataSize );

         /* Update the checksum, the checksum contains all of the data plus the size and attributes. */
         _sFile.sHeader.Cs = FileChecksum( ( uint8_t * ) & _sFile.sHeader.dataSize,
                                           _sFile.sHeader.dataSize + HDR_BYTES_IN_CHECKSUM );
         /* Write the erased file with the updated checksum */
         eRetVal = PAR_partitionFptr.parWrite( pFileHandle->FileOffset,
                                               ( uint8_t * ) & _sFile,
                                               sizeof( _sFile.sHeader ) + _sFile.sHeader.dataSize,
                                               pFileHandle->pTblInfo );
      }
      else
      {
         /* Erase the non-checksumed file */
         eRetVal = PAR_partitionFptr.parErase( pFileHandle->FileOffset + sizeof( _sFile.sHeader ),
                                               _sFile.sHeader.dataSize,
                                               pFileHandle->pTblInfo );
      }
   }
   OS_MUTEX_Unlock( &fioMutex_ );
   return( eRetVal );
}
/***********************************************************************************************************************

   Function Name: FIO_fwrite

   Purpose:  Writes to device using the partition manager; the file checksum is updated according to the attributes.

   Arguments:
      FileHandle_t *pFileHandle:  Contains the Driver information and the partition Offset.
      fileOffset fOffset:  Offset into the file to write data
      uint8_t *pSrc:  Location of the source data to write to memory
      lCnt Cnt:  Number of bytes to write to the Memory.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t FIO_fwrite( const FileHandle_t *pFileHandle, const fileOffset fOffset, uint8_t const *pSrc, const lCnt Cnt )
{
   returnStatus_t eRetVal = eFILEIO_MEM_RANGE_ERROR;   /* Default return value */

   if ( ( fOffset + Cnt ) <= pFileHandle->dataSize ) /* Writing within the bounds of the file? */
   {
      if ( pFileHandle->Attr & FILE_IS_CHECKSUMED )
      {
         /* For a checksumed file, read the data to a local buffer. Compute new checksum as
            New checksum = old checksum + checksum of new bytes to write - checksum of the byte overwritten
            The over-write local buffer with pSrc and then write the entire file to the device.
            If file get corrupted, this method will not mask it. The NV error check will catch the NV error */
         OS_MUTEX_Lock( &fioMutex_ );

         /* Read the file into the local buffer including the header. */
         eRetVal = PAR_partitionFptr.parRead( ( uint8_t * ) & _sFile, pFileHandle->FileOffset,
                                              ( lCnt )sizeof( _sFile.sHeader ) + pFileHandle->dataSize,
                                              pFileHandle->pTblInfo );
         if ( eSUCCESS == eRetVal )   /* Was the file header successfully read? */
         {
            //Compute the checksum
            _sFile.sHeader.Cs += FileChecksum( ( uint8_t * )pSrc, ( uint16_t )Cnt ); //Add the checksum of the bytes to write
            //Subtract the checksum of the bytes that will be overwritten
            _sFile.sHeader.Cs -= FileChecksum( &_sFile.u8Data[fOffset], ( uint16_t )Cnt );

            /* Modify the local buffer with the data to be written in pSrc */
            ( void )memcpy( &_sFile.u8Data[fOffset], pSrc, Cnt ); //lint !e419 !e669   Data size is checked by FILE_IS_CHECKSUMED

            /* Write the entire file content to memory */
            eRetVal = PAR_partitionFptr.parWrite( pFileHandle->FileOffset,
                                                  ( uint8_t * ) & _sFile,
                                                  ( lCnt )_sFile.sHeader.dataSize + sizeof( tFileHeader ),
                                                  pFileHandle->pTblInfo );
         }
         OS_MUTEX_Unlock( &fioMutex_ );
      }
      else /* Only write the data passed in, the header does not need to be modified. */
      {
         eRetVal = PAR_partitionFptr.parWrite( pFileHandle->FileOffset + sizeof( tFileHeader ) + fOffset,
                                               pSrc,
                                               Cnt,
                                               pFileHandle->pTblInfo );
      }
   }
   return( eRetVal );
}
/***********************************************************************************************************************

   Function Name: FIO_fread

   Purpose:  Retrieves data from a device in the partition manager.  The file checksum is NOT verified.

   Arguments:
      *pFileHandle:  Contains the Driver information and the File Header Address.
      uint8_t *pDest:  Location to write data
      fileOffset fOffset:  Offset into the file to read data
      lCnt Cnt:  Number of bytes to read from memory.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t FIO_fread( const FileHandle_t *pFileHandle, uint8_t *pDest, const fileOffset fOffset, const lCnt Cnt )
{
   returnStatus_t eRetVal;

   if ( ( fOffset + Cnt ) <= pFileHandle->dataSize ) /* Reading within the bounds of the file? */
   {
      /* Read the data requested. */
      eRetVal = PAR_partitionFptr.parRead( pDest, pFileHandle->FileOffset + fOffset + sizeof( tFileHeader ), Cnt,
                                           pFileHandle->pTblInfo ); /*lint !e644 PTblInfo potentially uninitialized. */
   }
   else
   {
      eRetVal = eFILEIO_MEM_RANGE_ERROR; /* Requested data is outside of the file boundaries.  */
   }
   return( eRetVal );
}
/***********************************************************************************************************************

   Function Name: FIO_fflush

   Purpose:  Calls the partition manager to flush any cached contents.

   Arguments: FileHandle_t *pFileHandle - File handle that contains the partition to flush.

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t FIO_fflush( const FileHandle_t *pFileHandle )
{
   return( PAR_partitionFptr.parFlush( pFileHandle->pTblInfo ) ); /* Tell partition mgr to flush the corresponding partition. */
}
/***********************************************************************************************************************

   Function Name: FIO_ioctl

   Purpose:  Performs special operations to the target device.  None are implemented for this implementation.

   Arguments:
      *pFileHandle:  Contains the Driver information and the File Header Address.
      lAddr lAddress:  The logical address indicates which driver to send pData to.
      void *pData:  Command information to the device

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t FIO_ioctl( const FileHandle_t *pFileHandle, const void *pCmd, void *pData )
{
   return( PAR_partitionFptr.parIoctl( pCmd, pData, pFileHandle->pTblInfo ) );
}
/***********************************************************************************************************************

   Function Name: FileChecksum

   Purpose:  Performs a checksum on an array of Cnt length.

   Arguments:
      uint8_t *pSource
      lCnt Cnt

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
static uint16_t FileChecksum( uint8_t const *pSource, lCnt Cnt )
{
   uint16_t u16Checksum = 0; /* contains the checksum. */

   for ( ; Cnt; Cnt-- )
   {
      u16Checksum += *pSource++;
   }
#ifdef TM_FILE_IO_UNIT_TEST
   checksumOfLastWrite = u16Checksum;
#endif
   return( u16Checksum );
}
/***********************************************************************************************************************

   Function Name: FIO_timeToRunFileIntegrityCheck

   Purpose:  Sets a flag to run NV test (file integrity check) when idle

   Arguments: None

   Returns: None

   Side Effects: Sets a flag to run NV test when idle

   Reentrant Code: Yes

 **********************************************************************************************************************/
void FIO_timeToRunFileIntegrityCheck( void )
{
   runNVTest_ = true;
}
/***********************************************************************************************************************

   Function Name: verifyFixFileChecksum

   Purpose:  Returns the file Verifies that all checksumed files are valid. If the file is corrupted, set NV error flag

   Arguments: tFileHeader *pHeader: File header, read by the calling function
              PartitionData_t *pPartitionData: Partition in which this file resides
              dSize FileOffset: Offset of start of the file in the partition

   Returns: returnStatus_t - Status

   Side Effects: Sets the NV error flag, if any checksumed file is corrupted

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t verifyFixFileChecksum( tFileHeader const *pHeader, PartitionData_t const *pPartitionData,
                                             dSize FileOffset )
{
   returnStatus_t eRetVal = eSUCCESS;  // Return value, used throughout the function to test for errors

   if ( pHeader->Attr & FILE_IS_CHECKSUMED )
   {
      /* Read the file into the local buffer and validate the checksum */
      OS_MUTEX_Lock( &fioMutex_ );

      eRetVal = PAR_partitionFptr.parRead( ( uint8_t * ) & _sFile,
                                           FileOffset,
                                           pHeader->dataSize + sizeof( tFileHeader ),
                                           pPartitionData );
      if ( eSUCCESS == eRetVal )   /* Was the file read correctly? */
      {
         /* Validate the checksum, the checksum is over all of the data plus the size and attributes */
         /* Fix the checksum first, if requested (after DFW) */
         if ( _sFile.sHeader.Attr & DFW_RECOMPUTE_CHECKSUM )
         {
            /* Write the new checksum and attributes */
            _sFile.sHeader.Attr &= ~DFW_RECOMPUTE_CHECKSUM;
            _sFile.sHeader.Cs = FileChecksum( ( uint8_t * ) & _sFile.sHeader.dataSize, _sFile.sHeader.dataSize + HDR_BYTES_IN_CHECKSUM );
            //Rewrite the header with updated checksum and cleared DFW_RECOMPUTE_CHECKSUM flag
            ( void )PAR_partitionFptr.parWrite( FileOffset, ( uint8_t * ) & _sFile, sizeof( tFileHeader ), pPartitionData );
         }
         else
         {
            uint16_t csum = FileChecksum( ( uint8_t * ) & _sFile.sHeader.dataSize, _sFile.sHeader.dataSize + HDR_BYTES_IN_CHECKSUM );

            if ( _sFile.sHeader.Cs != csum )
            {
               /* Set the NV error flag and return with checksum error. */
               eRetVal = eFILEIO_FILE_CHECKSUM_ERROR;
               //Todo!
            }
         }
      }
      OS_MUTEX_Unlock( &fioMutex_ );
   }
   return eRetVal;
}
/***********************************************************************************************************************

   Function Name: FIO_fIntegrityCheck

   Purpose:  Verifies that all checksumed files are valid. If the file is corrupted, set NV error flag

   Arguments: None

   Returns: bool - Indicate if this function took CPU time. false - No time taken

   Side Effects: Sets the NV error flag, if any checksumed file is corrupted

   Reentrant Code: Yes

 **********************************************************************************************************************/
bool FIO_fIntegrityCheck( void )
{
   PartitionData_t      *pPartitionData;     // open partition
   PAR_getNextMember_t  sMember;             // data structure needed to get all partitions
   returnStatus_t       eRetVal;             // Return value, used throughout the function to test for errors
   bool                 cpuTimeUsed = false; // Default, no time used
   bool                 fileCheckSumError;   // Flag to indicate there is a checksum issue with a file

   if ( runNVTest_ )
   {
      /* Time to run the test */
      runNVTest_ = false;
      cpuTimeUsed = true;
      fileCheckSumError = false;

      /* Initialize and get the first partition */
      pPartitionData = PAR_GetFirstPartition( &sMember );

      while ( ( NULL != pPartitionData ) && ( pPartitionData->ePartition < ePART_START_NAMED_PARTITIONS ) )
      {
         /* Verify files in non-named partitions only */
         tFileHeader sHeader;          /* Contains the current file's header just read from memory */
         dSize    FileOffset = 0;      /* Offset to the file in the partition.  Used to search the partition. */

         // we only want to process partitions that contain a file system
         if( pPartitionData->PartitionType.fileSys )
         {
            do
            {
               /* Read the file's header */
               eRetVal = PAR_partitionFptr.parRead( ( uint8_t * ) & sHeader, FileOffset, sizeof( sHeader ), pPartitionData );
               if ( eSUCCESS == eRetVal )   /* Was the header read properly? */
               {
                  if ( INVALID_FILE_ID != sHeader.Id )
                  {
                     /* Valid file, set NV error/checksum error flag, if file is corrupt */
                     if( eFILEIO_FILE_CHECKSUM_ERROR == verifyFixFileChecksum( &sHeader, pPartitionData, FileOffset ) )
                     {
                        // The file has a checksum problem, set the error flag
                        fileCheckSumError = true;
                     }

                     /* Set offset to next file.  The read above will fail if the partition boundary is exceeded. */
                     FileOffset += sHeader.dataSize + sizeof( sHeader );
                  }
                  else
                  {
                     /* Invalid file, go to next partition */
                     break;
                  }
               }
            }
            while ( eSUCCESS == eRetVal );
         }
         /* Get next partition */
         pPartitionData = PAR_GetNextPartition( &sMember );
      }

      if( fileCheckSumError )
      {
         // One or more files have a checksum problem, log an event indicating device corruption
         EventKeyValuePair_s  keyVal;     /* Event key/value pair info  */
         EventData_s          progEvent;  /* Event info  */
         ( void )memset( ( uint8_t * )&keyVal, 0, sizeof( keyVal ) );
         progEvent.markSent = ( bool )false;
         progEvent.eventId = ( uint16_t )comDeviceConfigurationCorrupted;
         progEvent.eventKeyValuePairsCount = 0;
         progEvent.eventKeyValueSize = 0;
         ( void )EVL_LogEvent( 200, &progEvent, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
      }
   }

   return cpuTimeUsed;
}
#ifdef TM_PARTITION_USAGE
/***********************************************************************************************************************

   Function Name: FIO_printFileInfo

   Purpose:  Print all the files header in frequency based partitions (non-named partitions)

   Arguments: None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
void FIO_printFileInfo( void )
{
   PartitionData_t      *pPartitionData;  // open partition
   PAR_getNextMember_t  sMember;          // data structure needed to get all partitions
   returnStatus_t       eRetVal;          // Return value, used throughout the function to test for errors

   DBG_logPrintf( 'R', "PartID, FID, Offset, Data Offset, Data Size, Checksum, Attr, FileName,               Crc32" );

   /* Initialize and get the first partition */
   pPartitionData = PAR_GetFirstPartition( &sMember );

   while ( ( NULL != pPartitionData ) && ( pPartitionData->ePartition < ePART_LAST_PARTITION ) )
   {

      if( pPartitionData->PartitionType.fileSys )
      {  // the partition contains file system structure, print out the file details for each file in the partition

         tFileHeader sHeader;          /* Contains the current file's header just read from memory */
         dSize       FileOffset = 0;   /* Offset to the file in the partition.  Used to search the partition. */

         do
         {
            /* Read the file's header */
            eRetVal = PAR_partitionFptr.parRead( ( uint8_t * )&sHeader, FileOffset, sizeof( sHeader ), pPartitionData );

            if ( eSUCCESS == eRetVal ) /* Was the header read properly? */
            {
               /* Print the file info */

               if ( ( INVALID_FILE_ID != sHeader.Id ) && ( sHeader.Id < (uint16_t) eFN_LASTFILE ) )
               {
                  /* Valid file */
                  uint32_t crc32;
                  crc32 = calcFileCrc(   &sHeader, pPartitionData, FileOffset );

                  DBG_logPrintf( 'R', "%6u, %3u, %6lu,      %6lu, %9u, %8u,    %x, %-22s, 0x%08lx",
                                 ( uint32_t )pPartitionData->ePartition, sHeader.Id, FileOffset, FileOffset + sizeof( sHeader ),
                                 sHeader.dataSize, sHeader.Cs, sHeader.Attr, getFileName( sHeader.Id ), crc32 );

                  /* Set offset to next file.  The read above will fail if the partition boundary is exceeded. */
                  FileOffset += sHeader.dataSize + sizeof( sHeader );
                  OS_TASK_Sleep( 20 ); //give time to the debug task to print
               }
               else
               {
                  /* Invalid file, go to next partition */
                  break;
               }
            }
         }
         while( eSUCCESS == eRetVal );
      }
      /* Get next partition */
      pPartitionData = PAR_GetNextPartition( &sMember );
   }
}
/***********************************************************************************************************************

   Function Name: FIO_printFile

   Purpose:  Prints information for a file

   Arguments: const filenames_t fileName

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
void FIO_printFile( const filenames_t fileName )
{
   PartitionData_t      *pPartitionData;  // open partition
   PAR_getNextMember_t  sMember;          // data structure needed to get all partitions
   returnStatus_t       eRetVal;          // Return value, used throughout the function to test for errors
   bool                 bFileFound = false;
   dSize                logicalOffset = 0;

   /* Initialize and get the first partition */
   pPartitionData = PAR_GetFirstPartition( &sMember );

   while ( ( NULL != pPartitionData ) && ( pPartitionData->ePartition < ePART_LAST_PARTITION ) && !bFileFound )
   {
      /* Print file headers of non-named partitions only */

      tFileHeader sHeader;          /* Contains the current file's header just read from memory */
      dSize       FileOffset = 0;   /* Offset to the file in the partition.  Used to search the partition. */

      // we only want to process partitions that contain a file system
      if( pPartitionData->PartitionType.fileSys )
      {
         do
         {
            /* Read the file's header */
            eRetVal = PAR_partitionFptr.parRead( ( uint8_t * )&sHeader, FileOffset, sizeof( sHeader ), pPartitionData );

            if ( eSUCCESS == eRetVal ) /* Was the header read properly? */
            {
               /* Print the file info */

               if ( INVALID_FILE_ID != sHeader.Id )
               {
                  /* Valid file */
                  if ( sHeader.Id == ( uint16_t )fileName )
                  {
                     uint16_t dataAddr;
                     uint16_t index;
                     uint8_t  fileData[16];
                     char     buf[( sizeof( fileData ) * 2 ) + 1];
                     uint16_t cnt;
                     FileHandle_t fHandle;
                     FileStatus_t fileStatus;


                     if ( eSUCCESS == FIO_fopen( &fHandle, pPartitionData->ePartition, ( uint16_t )fileName, sHeader.dataSize,
                                                 sHeader.Attr, 0, &fileStatus ) )
                     {
                        bFileFound = true;
                        DBG_logPrintf( 'R', " File %d is %u Bytes & is Located in Partition %u @ Phy Addr %lu",
                                       ( int32_t )fileName, sHeader.dataSize,
                                       ( uint32_t )pPartitionData->ePartition, pPartitionData->PhyStartingAddress );
                        DBG_logPrintf( 'R', " File Partition Offset is @ %lu, Data is @ %lu",
                                       FileOffset, FileOffset + sizeof( sHeader ) );
                        DBG_logPrintf( 'R', " For DFW: File Logical Offset: %lu, Data Offset: %lu",
                                       logicalOffset + FileOffset, logicalOffset + FileOffset + sizeof( sHeader ) );
                        DBG_logPrintf( 'R', " File Contents:" );
                        DBG_logPrintf( 'R', " Par Offset, Data" );
                        cnt = sizeof( fileData );
                        for ( dataAddr = 0, index = 0; index < sHeader.dataSize; index += cnt )
                        {
                           if ( ( sHeader.dataSize - index ) < ( uint16_t )sizeof( fileData ) )
                           {
                              cnt = ( uint16_t )( sHeader.dataSize - index );
                           }
                           if ( eSUCCESS == FIO_fread( &fHandle, &fileData[0], dataAddr + index, cnt ) )
                           {
                              uint8_t i;

                              ( void )memset( &buf[0], 0, sizeof( buf ) );
                              for ( i = 0; i < cnt; i++ )
                              {
                                 ( void )sprintf( &buf[i * 2], "%02X", fileData[i] );
                              }
                              DBG_logPrintf( 'R', "%6lu,%6s%s", FileOffset + sizeof( sHeader ) + index, " ", &buf[0] );
                              OS_TASK_Sleep( 5 ); //Allow a little time for large files to not over run DBG_logPrintf
                           }
                           else
                           {
                              DBG_logPrintf( 'R', "  %6lu,     Read Failed!", sHeader.dataSize );
                           }
                        }
                     }
                     else
                     {
                        DBG_logPrintf( 'R', "Couldn't open file" );
                     }
                  }

                  /* Set offset to next file.  The read above will fail if the partition boundary is exceeded. */
                  FileOffset += sHeader.dataSize + sizeof( sHeader );
               }
               else
               {
                  /* Invalid file, go to next partition */
                  break;
               }
            }
         }while( eSUCCESS == eRetVal );
      }
      /* Get next partition */
      logicalOffset += pPartitionData->lDataSize;
      pPartitionData = PAR_GetNextPartition( &sMember );
   }
   if ( !bFileFound )
   {
      DBG_logPrintf( 'R', "File %d Not found!", ( int32_t )fileName );
   }
}
/***********************************************************************************************************************

   Function Name: FIO_fileDump

   Purpose:  Prints all the files in un-named partitions

   Arguments: None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
void FIO_fileDump( void )
{
   PartitionData_t      *pPartitionData;  // open partition
   PAR_getNextMember_t  sMember;          // data structure needed to get all partitions
   returnStatus_t       eRetVal;          // Return value, used throughout the function to test for errors
   FileHandle_t         fHandle;

   DBG_printf( "FID, size, PID, FID CS  sz  Attr file content\n" );

   pPartitionData = PAR_GetFirstPartition( &sMember ); //Initialize and get the first partition
   while ( ( NULL != pPartitionData ) && ( pPartitionData->ePartition < ePART_START_NAMED_PARTITIONS ) )
   {
      /* Print file headers of non-named partitions only */
      tFileHeader sHeader;          /* Contains the current file's header just read from memory */
      uint8_t     *pHdrBytes;
      dSize       FileOffset = 0;   /* Offset to the file in the partition.  Used to search the partition. */

      // we only want to process partitions that contain a file system
      if( pPartitionData->PartitionType.fileSys )
      {
         do
         {
            /* Read the file's header */
            eRetVal = PAR_partitionFptr.parRead( ( uint8_t * )&sHeader, FileOffset, sizeof( sHeader ), pPartitionData );

            if ( eSUCCESS == eRetVal )
            {
               /* Read file header successfully */
               if ( INVALID_FILE_ID == sHeader.Id )
               {
                  /* End of partition, go to next partition */
                  break;
               }
               uint16_t dataAddr;
               uint16_t index;
               uint8_t  fileData[16];
               char     hexString[(sizeof(fileData) * 2) + 1];
               uint16_t cnt;
               uint8_t  i;
               FileStatus_t fileStatus;

               if ( eSUCCESS == FIO_fopen( &fHandle, pPartitionData->ePartition, sHeader.Id, sHeader.dataSize,
                                           sHeader.Attr, 0, &fileStatus ) )
               {
                  DBG_printfNoCr( "%2u, %-22s, %4u,   %u, ", sHeader.Id, getFileName( sHeader.Id ), sHeader.dataSize, ( uint32_t )pPartitionData->ePartition );
                  pHdrBytes = ( uint8_t * )&sHeader;
                  for ( i = 0; i < sizeof( sHeader ); i++ )
                  {  // convert file header hexs data into hex value string
                     (void)snprintf(&hexString[i * 2], ( sizeof(hexString) - (i * 2) ), "%02X", *pHdrBytes++ );
                  }
                  DBG_printfNoCr( "%s ", hexString );
                  OS_TASK_Sleep( 5 ); // give time to the debug task to print
                  cnt = sizeof( fileData ); // set count to data storage buffer size
                  for ( dataAddr = 0, index = 0; index < sHeader.dataSize; index += cnt )
                  {
                     if ( ( sHeader.dataSize - index ) < ( uint16_t )sizeof( fileData ) )
                     {
                        cnt = ( uint16_t )( sHeader.dataSize - index );
                     }
                     if ( eSUCCESS == FIO_fread( &fHandle, &fileData[0], dataAddr + index, cnt ) )
                     {
                        for ( i = 0; i < cnt; i++ )
                        {  // convert file data contained in byte array into hex value string
                           (void)snprintf(&hexString[i * 2], ( sizeof(hexString) - (i * 2) ), "%02X", fileData[i] );
                        }
                        DBG_printfNoCr( "%s", hexString );
                        OS_TASK_Sleep( 5 ); //give time to the debug task to print
                     }
                  }
                  DBG_printf( "" );
               }
            }

            /* Set offset to next file.  The read above will fail if the partition boundary is exceeded. */
            FileOffset += sHeader.dataSize + sizeof( sHeader );
         }
         while( eSUCCESS == eRetVal );
      }
      /* Get next partition */
      pPartitionData = PAR_GetNextPartition( &sMember );
   }
}
#endif
#ifdef TM_VALIDATE_FILE_CHKSUM
/***********************************************************************************************************************

   Function Name: FIO_validateFileChksum

   Purpose:  Validates file checksum and validating functions

   Arguments: None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
void FIO_validateFileChksum( void )
{
   FileHandle_t snFileHndlTest;
   PartitionData_t const *pParTblWhole;
   tCsFile sFileTest;
   returnStatus_t eRetVal;                // Return value, used throughout the function to test for errors
   char  prntBuf[100];
   uint8_t cnt;
   uint8_t fileSize = 6;
   diagnosticReg_t diagsReg;

   /* Open the partition */
   ( void )PAR_partitionFptr.parOpen( &pParTblWhole, ePART_WHOLE_NV, 0 );

   ( void )FIO_fopen(  &snFileHndlTest,      /* File Handle */
                       ePART_SEARCH_BY_TIMING, /* Search for the best paritition according to the timing. */
                       eFN_SN,                 /* File ID (filename) */
                       ( lCnt )fileSize,       /* Size of the data in the file. */
                       FILE_IS_CHECKSUMED,     /* File attributes to use. */
                       ULONG_MAX );

   PRNT_crlf();
   PRNT_sprintf( &prntBuf[0], "File ID, Offset, Data Size, Checksum,   Attr, Data,        Diag Reg" );
   PRNT_stringCrLf( &prntBuf[0] );

   /* Read the complete file */
   eRetVal = PAR_partitionFptr.parRead( ( uint8_t * )&sFileTest,
                                        snFileHndlTest.FileOffset,
                                        snFileHndlTest.dataSize + sizeof( tFileHeader ),
                                        snFileHndlTest.pTblInfo );

   memset( &sFileTest.u8Data[0], 0, fileSize );

   /* Write the whole file */
   for( cnt = 0; cnt < fileSize; cnt++ )
   {
      CLRWDT();
      sFileTest.u8Data[cnt] += cnt;
      ( void )FIO_fwrite( &snFileHndlTest, 0, ( uint8_t * )&sFileTest.u8Data[0], ( lCnt )fileSize );

      /* Read the complete file */
      eRetVal = PAR_partitionFptr.parRead( ( uint8_t * )&sFileTest,
                                           snFileHndlTest.FileOffset,
                                           snFileHndlTest.dataSize + sizeof( tFileHeader ),
                                           snFileHndlTest.pTblInfo );
      diagsReg.reg = 0;
      REG_Write( REG_DIAG_INDICATORS, sizeof( diagsReg ), ( uint8_t * )&diagsReg );

      runNVTest_ = true;
      FIO_fIntegrityCheck();
      ( void )REG_Read( REG_DIAG_INDICATORS, sizeof( diagsReg ), ( uint8_t * )&diagsReg );
      //print
      PRNT_sprintf( &prntBuf[0], "%7u, %6lu, %9u, %8u,    %3x, %x %x %x %x %x %x, %x",
                    sFileTest.sHeader.Id, snFileHndlTest.FileOffset, sFileTest.sHeader.dataSize,
                    sFileTest.sHeader.Cs, sFileTest.sHeader.Attr, sFileTest.u8Data[0], sFileTest.u8Data[1],
                    sFileTest.u8Data[2], sFileTest.u8Data[3], sFileTest.u8Data[4], sFileTest.u8Data[5],
                    diagsReg.reg );
      PRNT_stringCrLf( &prntBuf[0] );
   }
   PRNT_crlf();

   /* Write 2 bytes */
   for( cnt = 0; cnt < ( fileSize - 1 ); cnt++ )
   {
      CLRWDT();
      sFileTest.u8Data[cnt]++;
      sFileTest.u8Data[cnt + 1]++;
      ( void )FIO_fwrite( &snFileHndlTest, cnt, ( uint8_t * )&sFileTest.u8Data[cnt], 2 );

      /* Read the complete file */
      eRetVal = PAR_partitionFptr.parRead( ( uint8_t * )&sFileTest,
                                           snFileHndlTest.FileOffset,
                                           snFileHndlTest.dataSize + sizeof( tFileHeader ),
                                           snFileHndlTest.pTblInfo );
      diagsReg.reg = 0;
      REG_Write( REG_DIAG_INDICATORS, sizeof( diagsReg ), ( uint8_t * )&diagsReg );

      runNVTest_ = true;
      FIO_fIntegrityCheck();
      ( void )REG_Read( REG_DIAG_INDICATORS, sizeof( diagsReg ), ( uint8_t * )&diagsReg );
      //print
      PRNT_sprintf( &prntBuf[0], "%7u, %6lu, %9u, %8u,    %3x, %x %x %x %x %x %x, %x",
                    sFileTest.sHeader.Id, snFileHndlTest.FileOffset, sFileTest.sHeader.dataSize,
                    sFileTest.sHeader.Cs, sFileTest.sHeader.Attr, sFileTest.u8Data[0], sFileTest.u8Data[1],
                    sFileTest.u8Data[2], sFileTest.u8Data[3], sFileTest.u8Data[4], sFileTest.u8Data[5],
                    diagsReg.reg );
      PRNT_stringCrLf( &prntBuf[0] );
   }
   PRNT_crlf();

   /* Play with the checksum */
   CLRWDT();
   sFileTest.sHeader.Cs += 2;
   PAR_partitionFptr.parWrite( ( dSize )snFileHndlTest.FileOffset + offsetof( tFileHeader, Cs ),
                               &sFileTest.sHeader.Cs, ( lCnt )sizeof( sFileTest.sHeader.Cs ), pParTblWhole );
   diagsReg.reg = 0;
   REG_Write( REG_DIAG_INDICATORS, sizeof( diagsReg ), ( uint8_t * )&diagsReg );
   runNVTest_ = true;
   FIO_fIntegrityCheck();
   ( void )REG_Read( REG_DIAG_INDICATORS, sizeof( diagsReg ), ( uint8_t * )&diagsReg );
   /* Read the complete file */
   eRetVal = PAR_partitionFptr.parRead( ( uint8_t * )&sFileTest,
                                        snFileHndlTest.FileOffset,
                                        snFileHndlTest.dataSize + sizeof( tFileHeader ),
                                        snFileHndlTest.pTblInfo );
   //print
   PRNT_sprintf( &prntBuf[0], "%7u, %6lu, %9u, %8u,    %3x, %x %x %x %x %x %x, %x",
                 sFileTest.sHeader.Id, snFileHndlTest.FileOffset, sFileTest.sHeader.dataSize,
                 sFileTest.sHeader.Cs, sFileTest.sHeader.Attr, sFileTest.u8Data[0], sFileTest.u8Data[1],
                 sFileTest.u8Data[2], sFileTest.u8Data[3], sFileTest.u8Data[4], sFileTest.u8Data[5],
                 diagsReg.reg );
   PRNT_stringCrLf( &prntBuf[0] );


   //Change the attributes
   CLRWDT();
   sFileTest.sHeader.Attr += DFW_RECOMPUTE_CHECKSUM;
   PAR_partitionFptr.parWrite( ( dSize )snFileHndlTest.FileOffset + offsetof( tFileHeader, Attr ),
                               &sFileTest.sHeader.Attr, ( lCnt )sizeof( sFileTest.sHeader.Attr ), pParTblWhole );
   /* Read the complete file */
   eRetVal = PAR_partitionFptr.parRead( ( uint8_t * )&sFileTest,
                                        snFileHndlTest.FileOffset,
                                        snFileHndlTest.dataSize + sizeof( tFileHeader ),
                                        snFileHndlTest.pTblInfo );
   //print
   PRNT_sprintf( &prntBuf[0], "%7u, %6lu, %9u, %8u,    %3x, %x %x %x %x %x %x, %x",
                 sFileTest.sHeader.Id, snFileHndlTest.FileOffset, sFileTest.sHeader.dataSize,
                 sFileTest.sHeader.Cs, sFileTest.sHeader.Attr, sFileTest.u8Data[0], sFileTest.u8Data[1],
                 sFileTest.u8Data[2], sFileTest.u8Data[3], sFileTest.u8Data[4], sFileTest.u8Data[5],
                 diagsReg.reg );
   PRNT_stringCrLf( &prntBuf[0] );


   //Recompute the checksum
   CLRWDT();
   diagsReg.reg = 0;
   REG_Write( REG_DIAG_INDICATORS, sizeof( diagsReg ), ( uint8_t * )&diagsReg );
   runNVTest_ = true;
   FIO_fIntegrityCheck();
   ( void )REG_Read( REG_DIAG_INDICATORS, sizeof( diagsReg ), ( uint8_t * )&diagsReg );
   /* Read the complete file */
   eRetVal = PAR_partitionFptr.parRead( ( uint8_t * )&sFileTest,
                                        snFileHndlTest.FileOffset,
                                        snFileHndlTest.dataSize + sizeof( tFileHeader ),
                                        snFileHndlTest.pTblInfo );
   //print
   PRNT_sprintf( &prntBuf[0], "%7u, %6lu, %9u, %8u,    %3x, %x %x %x %x %x %x, %x",
                 sFileTest.sHeader.Id, snFileHndlTest.FileOffset, sFileTest.sHeader.dataSize,
                 sFileTest.sHeader.Cs, sFileTest.sHeader.Attr, sFileTest.u8Data[0], sFileTest.u8Data[1],
                 sFileTest.u8Data[2], sFileTest.u8Data[3], sFileTest.u8Data[4], sFileTest.u8Data[5],
                 diagsReg.reg );
   PRNT_stringCrLf( &prntBuf[0] );

   PRNT_crlf();
   PRNT_crlf();
}
#endif

typedef struct
{
   uint32_t crc32;
} FIO_Attr_t;

#if ENABLE_FIO_TASKS == 1
static FIO_Attr_t FIO_Attr[ NUM_FILES ] = {0};
#endif

/* This table is used to determine what files are included in the file checksum used to sanity check important
   configurations.  Items that are going to be set at manufacturing that we do not expect to change afterward should be
   set to true.  */

/* This contains the flag for the files that are to be checked */
static bool const check_flags[ NUM_FILES ] =
{
#if ( EP == 1 )
   [0                     ] = ( bool )false, // Don't check!
   [eFN_DST               ] = ( bool )true,
   [eFN_ID_CFG            ] = ( bool )true,
   [eFN_MTRINFO           ] = ( bool )true,
   [eFN_MTR_DS_REGISTERS  ] = ( bool )false,

   /*  5  */
   [eFN_HD_DATA           ] = ( bool )false,
   [eFN_ID_META           ] = ( bool )false,
   [eFN_PWR               ] = ( bool )false,
   [eFN_HMC_ENG           ] = ( bool )false,
   [eFN_PHY_CACHED        ] = ( bool )false,

   /*  10 */
   [eFN_PHY_CONFIG        ] = ( bool )true,
   [eFN_MAC_CACHED        ] = ( bool )false,
   [eFN_MAC_CONFIG        ] = ( bool )true,
   [eFN_NWK_CONFIG        ] = ( bool )true,
   [eFN_NWK_CACHED        ] = ( bool )false,

   /*  15 */
   [eFN_REG_STATUS        ] = ( bool )false,
   [eFN_TIME_SYS          ] = ( bool )false, // Need to review this!
   [eFN_DEMAND            ] = ( bool )false,
   [eFN_SELF_TEST         ] = ( bool )false,
   [eFN_HMC_START         ] = ( bool )false,

   /*  20 */
   [eFN_RTI               ] = ( bool )false,
   [eFN_HMC_DIAGS         ] = ( bool )false,
   [eFN_MACU              ] = ( bool )false,
   [eFN_PWRCFG            ] = ( bool )true,
   [eFN_ID_PARAMS         ] = ( bool )true,

   /*  25 */
   [eFN_MODECFG           ] = ( bool )true,
   [eFN_SECURITY          ] = ( bool )true,
   [eFN_RADIO_PATCH       ] = ( bool )true,
   [eFN_DFWTDCFG          ] = ( bool )true,
   [eFN_SMTDCFG           ] = ( bool )true,

   /*  30 */
   [eFN_DMNDCFG           ] = ( bool )true,
   [eFN_EVL_DATA          ] = ( bool )false,
   [eFN_HD_FILE_INFO      ] = ( bool )true,
   [eFN_OR_MR_INFO        ] = ( bool )true,
   [eFN_EDCFG             ] = ( bool )true,

   /*  35 */
   [eFN_DFW_VARS          ] = ( bool )false,
   [eFN_EDCFGU            ] = ( bool )true,
   [eFN_DTLS_CONFIG       ] = ( bool )true,
   [eFN_DTLS_CACHED       ] = ( bool )false,
   [eFN_DTLS_MAJOR        ] = ( bool )false,

   /*  40 */
   [eFN_DTLS_MINOR        ] = ( bool )false,
   [eFN_FIO_CONFIG        ] = ( bool )true,
   [eFN_DR_LIST           ] = ( bool )false,
   [eFN_DBG_CONFIG        ] = ( bool )true,
   [eFN_EVENTS            ] = ( bool )false,

   /*  45 */
   [eFN_HD_INDEX          ] = ( bool )false,
   [eFN_MIMTINFO          ] = ( bool )false,
   [eFN_VERINFO           ] = ( bool )false,
   [eFN_TIME_SYNC         ] = ( bool )false,
   [eFN_MTLS_CONFIG       ] = ( bool )false,

   /*  50 */
   [eFN_MTLS_ATTRIBS      ] = ( bool )false,
   [eFN_DFW_CACHED        ] = ( bool )false,
   [eFN_TEMPERATURE_CONFIG] = ( bool )false,
   [eFN_APP_CACHED        ] = ( bool )false,
   [eFN_PD_CONFIG         ] = ( bool )false,

   /* 55 */
   [eFN_DTLS_WOLFSSL4X    ] = ( bool )false,
   [eFN_PD_CACHED         ] = ( bool )false

#else
   [0                     ] = ( bool )false, // Don't check!
   [eFN_DST               ] = ( bool )true,
   [eFN_PWR               ] = ( bool )false,
   [eFN_PHY_CACHED        ] = ( bool )false,
   [eFN_PHY_CONFIG        ] = ( bool )true,

   /*  5  */
   [eFN_MAC_CACHED        ] = ( bool )false,
   [eFN_MAC_CONFIG        ] = ( bool )true,
   [eFN_NWK_CACHED        ] = ( bool )false,
   [eFN_NWK_CONFIG        ] = ( bool )true,
   [eFN_REG_STATUS        ] = ( bool )true,

   /*  10 */
   [eFN_SELF_TEST         ] = ( bool )false,
   [eFN_MODECFG           ] = ( bool )true,
   [eFN_SECURITY          ] = ( bool )true,
   [eFN_RADIO_PATCH       ] = ( bool )true,
   [eFN_EVL_DATA          ] = ( bool )false,

   /*  15 */
   [eFN_DFW_VARS          ] = ( bool )false,
   [eFN_DTLS_MAJOR        ] = ( bool )false,
   [eFN_DTLS_MINOR        ] = ( bool )false,
   [eFN_DTLS_CONFIG       ] = ( bool )true,
   [eFN_DTLS_CACHED       ] = ( bool )false,

   /*  20 */
   [eFN_EVENTS            ] = ( bool )false,
   [eFN_MACU              ] = ( bool )false,
   [eFN_FIO_CONFIG        ] = ( bool )true,
   [eFN_DBG_CONFIG        ] = ( bool )true,
   [eFN_VERINFO           ] = ( bool )false,

   /*  25 */
   [eFN_MIMTINFO          ] = ( bool )false,
   [eFN_TIME_SYNC         ] = ( bool )false,
   [eFN_DEVICEMODE        ] = ( bool )true,
   [eFN_TEMPERATURE_CONFIG] = ( bool )false,
   [eFN_SMTDCFG           ] = ( bool )true,

   /*  30 */
   [eFN_MTLS_CONFIG       ] = ( bool )false,
   [eFN_MTLS_ATTRIBS      ] = ( bool )false,
   [eFN_APP_CACHED        ] = ( bool )false,
   [eFN_DTLS_WOLFSSL4X    ] = ( bool )false,
#endif
};

/***********************************************************************************************************************

   Function Name: calcFileCrc

   Purpose:  Returns the crc for file data including the header

   Arguments: *pHeader          : File header, read by the calling function
              *pPartitionData   : Partition in which this file resides
              FileOffset        : Offset of start of the file in the partition

   Returns: crc32

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
static uint32_t calcFileCrc(
   tFileHeader     const *pHeader,
   PartitionData_t const *pPartitionData,
   dSize            FileOffset )
{
   uint32_t crc32;

   uint16_t index;
   crc32Cfg_t crcCfg;

   static uint8_t  fileData[32];
   CRC32_init( CRC32_DFW_START_VALUE, CRC32_DFW_POLY, eCRC32_RESULT_INVERT, &crcCfg ); /* Init CRC */

   // Included the header in the crc32
   crc32 = CRC32_calc( pHeader, sizeof( tFileHeader ), &crcCfg );

   uint16_t cnt;
   dSize  dataAddr = FileOffset + sizeof( tFileHeader );
   for ( index = 0; index < pHeader->dataSize; index += cnt )
   {
      cnt = ( uint16_t )min( ( pHeader->dataSize - index ),  ( uint16_t )sizeof( fileData ) );

      if( PAR_partitionFptr.parRead( fileData, dataAddr, cnt, pPartitionData ) == eSUCCESS )
      {
         crc32 = CRC32_calc( fileData, cnt, &crcCfg );
      }
      dataAddr += cnt;
   }
   return crc32;
}

#if ENABLE_FIO_TASKS == 1
/***********************************************************************************************************************

   Function Name: FIO_CheckFileCrc

   Purpose: This function is called to check the CRC32 for each file in the named partitions

   Arguments: pFIO_Attr  -
   Arguments: update     - Indicates is the crc is to be updated

   Returns:  bool

 ***********************************************************************************************************************/
static bool FIO_CheckFileCrc( FIO_Attr_t *pFIO_Attr, bool update )
{
   PartitionData_t      *pPartitionData;  // open partition
   PAR_getNextMember_t  sMember;          // data structure needed to get all partitions
   returnStatus_t       eRetVal;          // Return value, used throughout the function to test for errors

   bool status = true;

   /* Initialize and get the first partition */
   pPartitionData = PAR_GetFirstPartition( &sMember ); //Initialize and get the first partition

   while ( ( NULL != pPartitionData ) && ( pPartitionData->ePartition < ePART_START_NAMED_PARTITIONS ) )
   {
      /* Print file headers of non-named partitions only */
      tFileHeader sHeader;          /* Contains the current file's header just read from memory */
      dSize       FileOffset = 0;   /* Offset to the file in the partition.  Used to search the partition. */

      // we only want to process partitions that contain a file system
      if( pPartitionData->PartitionType.fileSys )
      {
         do
         {
            /* Read the file's header */
            eRetVal = PAR_partitionFptr.parRead( ( uint8_t * )&sHeader, FileOffset, sizeof( sHeader ), pPartitionData );

            if ( eSUCCESS == eRetVal )   /* Was the header read properly? */
            {
               if ( INVALID_FILE_ID != sHeader.Id )
               {
                  uint32_t crc32;
                  crc32 = calcFileCrc( &sHeader, pPartitionData, FileOffset );

                  if ( update )
                  {
                     if( check_flags[sHeader.Id] )
                     {
                        if ( pFIO_Attr[sHeader.Id].crc32 != crc32 )
                        {
                           INFO_printf( "%2u, %-20s, %4u,   %u CRC :0x%08lx ERROR crc=0x%08lx)",
                                        sHeader.Id, getFileName( sHeader.Id ), sHeader.dataSize, pPartitionData->ePartition,
                                        pFIO_Attr[sHeader.Id].crc32, crc32 );

                           pFIO_Attr[sHeader.Id].crc32 = crc32;
                           status = false;
                        }
                        else
                        {
                           INFO_printf( "%2u, %-20s, %4u,   %u CRC :0x%08lx OK",
                                        sHeader.Id,
                                        getFileName( sHeader.Id ),
                                        sHeader.dataSize,
                                        pPartitionData->ePartition,
                                        pFIO_Attr[sHeader.Id].crc32 );
                        }
                     }

                  }
                  else
                  {
                     // Just report errors
                     if ( pFIO_Attr[sHeader.Id].crc32 != crc32 )
                     {
                        if( check_flags[sHeader.Id] )
                        {
                           // Checking this file
                           status = false;
                           INFO_printf( "%2u, %-20s, %4u,   %u CRC :0x%08lx ERROR crc=0x%08lx)",
                                        sHeader.Id, getFileName( sHeader.Id ), sHeader.dataSize, pPartitionData->ePartition,
                                        pFIO_Attr[sHeader.Id].crc32, crc32 );
                        }
                     }
                  }
               }
               else
               {
                  /* Invalid file, go to next partition */
                  break;
               }
            }
            else
            {
               /* Invalid file, go to next partition */
               break;
            }

            /* Set offset to next file.  The read above will fail if the partition boundary is exceeded. */
            FileOffset += sHeader.dataSize + sizeof( sHeader );

         }while( eSUCCESS == eRetVal );
      }

      /* Get next partition */
      pPartitionData = PAR_GetNextPartition( &sMember );
   }
   return status;
}
#endif

/***********************************************************************************************************************

   Function Name: FIO_CalcFileCrc

   Purpose: This function is called to calculate the CRC32 for each file in the named partitions

   Arguments: pFIO_Attr  -
   Arguments: update     - Indicates is the crc is to be updated

   Returns:  bool

 ***********************************************************************************************************************/
static bool FIO_CalcFileCrc( FIO_Attr_t *pFIO_Attr )
{
   PartitionData_t      *pPartitionData;  // open partition
   PAR_getNextMember_t  sMember;          // data structure needed to get all partitions
   returnStatus_t       eRetVal;          // Return value, used throughout the function to test for errors

   bool status = true;

   /* Initialize and get the first partition */
   pPartitionData = PAR_GetFirstPartition( &sMember ); //Initialize and get the first partition

   while ( ( pPartitionData != NULL ) && ( pPartitionData->ePartition < ePART_START_NAMED_PARTITIONS ) )
   {
      /* Print file headers of non-named partitions only */
      tFileHeader sHeader;          /* Contains the current file's header just read from memory */
      dSize       FileOffset = 0;   /* Offset to the file in the partition.  Used to search the partition. */

      // we only want to process partitions that contain a file system
      if( pPartitionData->PartitionType.fileSys )
      {
         do
         {
            /* Read the file's header */
            eRetVal = PAR_partitionFptr.parRead( ( uint8_t * )&sHeader, FileOffset, sizeof( sHeader ), pPartitionData );

            if ( eRetVal == eSUCCESS )   /* Was the header read properly? */
            {
               if ( sHeader.Id != INVALID_FILE_ID )
               {
                  if( ( sHeader.Id < NUM_FILES ) && check_flags[sHeader.Id] )
                  {
                     uint32_t crc32;
                     crc32 = calcFileCrc(   &sHeader, pPartitionData, FileOffset );
                     pFIO_Attr[sHeader.Id].crc32 = crc32;
                  }
               }
               else
               {
                  /* Invalid file, go to next partition */
                  break;
               }
            }
            else
            {
               /* Invalid file, go to next partition */
               break;
            }

            /* Set offset to next file.  The read above will fail if the partition boundary is exceeded. */
            FileOffset += sHeader.dataSize + sizeof( sHeader );

         }while( eSUCCESS == eRetVal );
      }

      /* Get next partition */
      pPartitionData = PAR_GetNextPartition( &sMember );
   }
   return status;
}

#if ENABLE_FIO_TASKS == 1
#define FIO_CHECK_PERIOD 15000 /*!< Period for checking the files */
static FileHandle_t fileHandle_;
#endif

/***********************************************************************************************************************

   Function Name: FIO_ConfigGet()

   Purpose: This function is called to calculate the signature ( crc32) for the configuration data.

    The configuration data is typically saved in the RAW partition.
    A CRC32 is calculated over the buffer containing the CRC's that were computed for each of the
    files.   Files that are not monitored will CRC values of 0.

   Returns:  crc32

 ***********************************************************************************************************************/
uint32_t FIO_ConfigGet( void )
{
   crc32Cfg_t crcCfg;
   uint32_t   crc32;

   FIO_Attr_t tmp[ NUM_FILES ];

   ( void )memset( tmp, 0, sizeof( tmp ) );

   ( void )FIO_CalcFileCrc( tmp );

   CRC32_init( CRC32_DFW_START_VALUE, CRC32_DFW_POLY, eCRC32_RESULT_INVERT, &crcCfg ); /* Init CRC */
   crc32 = CRC32_calc( tmp, sizeof( tmp ), &crcCfg );

   INFO_printf( "crc32 = 0x%08lx", crc32 );

   return crc32;
}


#if ENABLE_FIO_TASKS == 1
/***********************************************************************************************************************

   Function Name: FIO_ConfigTest

   Purpose: This function is called from the command line to test crc's for the each of the files that are monitored.
            If any file that is monitored fails, the status will be returned as false

   bUpdate : Indicates if the reference files save in NV will be updated

   Returns:  true  - all         files have correct crc
             false - one or more files have correct crc

 ***********************************************************************************************************************/
bool FIO_ConfigTest( bool bUpdate )
{
   bool status;
   crc32Cfg_t crcCfg;


   status = FIO_CheckFileCrc( FIO_Attr, ( bool ) true );

   if ( ( !status ) && ( bUpdate ) )
   {
      INFO_printf( "FileUpdated" );
      ( void )FIO_fwrite( &fileHandle_, 0, ( uint8_t* )FIO_Attr, sizeof( FIO_Attr ) );
      ( void )FIO_fflush( &fileHandle_ ); // Write back in flash
   }

   // Compute the signature
   CRC32_init( CRC32_DFW_START_VALUE, CRC32_DFW_POLY, eCRC32_RESULT_INVERT, &crcCfg ); /* Init CRC */
   uint32_t crc32 = CRC32_calc( FIO_Attr, sizeof( FIO_Attr ), &crcCfg );

   INFO_printf( "crc32 = 0x%08lx", crc32 );

   return status;
}
#endif


#if ENABLE_FIO_TASKS == 1
/***********************************************************************************************************************

   Function Name: FIO_Task

   Purpose: This task will periodically check the configurtion saved in NV.  Currently set to 15 seconds.

   Arguments: Arg0

   Returns:  none

 ***********************************************************************************************************************/
void FIO_Task ( uint32_t Arg0 )
{
   OS_TASK_Sleep( 500 );
   for ( ;; )
   {
      // Check the files, but do not update the check values
      if( !FIO_CheckFileCrc( FIO_Attr, ( bool ) false ) )
      {
         ERR_printf( "FIO_CheckFile:failed" );
         LED_on( RED_LED );
      }
      else
      {
         LED_off( RED_LED );
      }
      OS_TASK_Sleep( FIO_CHECK_PERIOD );
   }
}
#endif

/***********************************************************************************************************************

   Function Name: FIO_init

   Purpose:

   Arguments: none

   Returns:  returnStatus_t

***********************************************************************************************************************/
returnStatus_t FIO_init ( void )
{
#if ENABLE_FIO_TASKS == 1

   returnStatus_t retVal = eFAILURE;
   FileStatus_t fileStatus;

   if ( eSUCCESS == FIO_fopen( &fileHandle_,          /* File Handle so that PHY access the file. */
                               ePART_SEARCH_BY_TIMING,/* Search for the best partition according to the timing. */
                               ( uint16_t ) eFN_FIO_CONFIG,      /* File ID (filename) */
                               sizeof( FIO_Attr ),    /* Size of the data in the file. */
                               FILE_IS_NOT_CHECKSUMED,/* File attributes to use. */
                               0xFFFFFFFF,            /* The update rate of the data in the file. */
                               &fileStatus ) )        /* Contains the file status */
   {
      if ( fileStatus.bFileCreated )
      {
         // The file was just created for the first time.
         // Save the default values to the file
         retVal = FIO_fwrite( &fileHandle_, 0, ( uint8_t* )FIO_Attr, sizeof( FIO_Attr ) );
      }
      else
      {
         // Read the FIO Cached File Data
         retVal = FIO_fread( &fileHandle_, ( uint8_t* ) FIO_Attr, 0, sizeof( FIO_Attr ) );
      }
   }
   return ( retVal );
#else
   return ( eSUCCESS );
#endif
}


