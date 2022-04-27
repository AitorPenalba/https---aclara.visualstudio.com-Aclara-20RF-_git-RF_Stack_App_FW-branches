/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: file_io.h
 *
 * Contents: FIO_ - File I/O module handles all file accesses.  Files may reside in any partition.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2011-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * v1.0, 20110415 kdavlin
 *
 **********************************************************************************************************************/
#ifndef FILE_IO_H_
#define FILE_IO_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "partitions.h"           /* Use the 'normal' partition information when not in test mode. */
#include "filenames.h"

#ifdef TM_FILE_IO_UNIT_TEST
   #include "unit_test_file_io.h"   /* Uses a 'dummy' partition manager to test the file io requests. */
#endif

/* ****************************************************************************************************************** */
/* file_io_EXTERN DEFINTION */

#ifdef file_io_GLOBAL
   #define file_io_EXTERN
#else
   #define file_io_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef uint16_t FileDataSize_t;
typedef uint16_t FileAttr_t;

typedef struct
{
  uint16_t           Id;         /* File Identifier (filename) */
  uint16_t           Cs;         /* File checksum of the data, size and attributes */
  FileDataSize_t     dataSize;   /* Data Size (64K max) */
  FileAttr_t         Attr;       /* File Attributes */
  /* The data will be located here, the amount of the data is defined by Size */
}tFileHeader;

typedef struct
{
   dSize            FileOffset;        /* Offset to the file header in the partition. */
   PartitionData_t  const *pTblInfo;   /* Partition data, contains driver info */
   FileDataSize_t   dataSize;          /* Data Size (64K max) */
   FileAttr_t       Attr;              /* File Attributes */
}FileHandle_t;                         /* File handle, used for applications to access files. */

typedef struct
{
   bool             bFileCreated;      /* The file was created and then opened. */
}FileStatus_t;

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* NOTE:  Files with checksums are size constrained.  The maximum checksummed file is defined in cfg_app.h and the
 * symbol is 'FIO_MAX_FILE_SIZE_WITH_CHECKSUM' */
#define FILE_IS_NOT_CHECKSUMED ((FileAttr_t)0x0000)
#define FILE_IS_CHECKSUMED     ((FileAttr_t)0x0001)
#define DFW_RECOMPUTE_CHECKSUM ((FileAttr_t)0x0100)


/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* file_io_EXTERN VARIABLES */

#ifdef TM_FILE_IO_UNIT_TEST
file_io_EXTERN uint16_t checksumOfLastWrite;
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

file_io_EXTERN returnStatus_t FIO_finit(void);
file_io_EXTERN returnStatus_t FIO_fopen(FileHandle_t *pFileHandle, ePartitionName ePartition, uint16_t fId, lCnt cnt,
                                        FileAttr_t fAttr, const uint32_t u32UpdateFreq,FileStatus_t *pStatus);
file_io_EXTERN returnStatus_t FIO_fclose(void);
file_io_EXTERN returnStatus_t FIO_ferase(FileHandle_t const *pFileHandle);
file_io_EXTERN returnStatus_t FIO_fwrite(const FileHandle_t *pFileHandle, const fileOffset fOffset, uint8_t const *pSrc,
                                         const lCnt Cnt);
file_io_EXTERN returnStatus_t FIO_fread(const FileHandle_t *pFileHandle, uint8_t *pDest, const fileOffset fOffset,
                                        const lCnt Cnt);
file_io_EXTERN returnStatus_t FIO_fflush(const FileHandle_t *pFileHandle);
file_io_EXTERN returnStatus_t FIO_ioctl(const FileHandle_t *pFileHandle, const void *pCmd, void *pData);
file_io_EXTERN bool           FIO_fIntegrityCheck(void);
file_io_EXTERN void           FIO_timeToRunFileIntegrityCheck(void);
file_io_EXTERN void           FIO_timeToRunFileIntegrityCheck(void);
file_io_EXTERN const char*    getFileName(uint16_t fileId);
#ifdef TM_PARTITION_USAGE
file_io_EXTERN void           FIO_printFileInfo(void);
file_io_EXTERN void           FIO_printFile( const filenames_t fileName );
file_io_EXTERN void           FIO_fileDump( void );
#endif

returnStatus_t FIO_init ( void );
void     FIO_Task ( uint32_t Arg0 );
uint32_t FIO_ConfigGet( void );
bool     FIO_ConfigTest( bool bUpdate );

#undef file_io_EXTERN

#endif
