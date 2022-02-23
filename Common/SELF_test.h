/***********************************************************************************************************************
 *
 * Filename: SELF_test.h
 *
 * Contents: Contains prototypes and definitions for self/diagnostic test.
 *
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ Karl Davlin Created 6/17/15
 *
 ***********************************************************************************************************************
 * Revision History:
 *
 **********************************************************************************************************************/
#ifndef SELF_test_H
#define SELF_test_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#if (ENABLE_FIO_TASKS == 1)
#include "file_io.h"
#endif
//#include "HEEP_util.h"

/* ****************************************************************************************************************** */
   /* GLOBAL DEFINTION */

/* ****************************************************************************************************************** */
/* #DEFINE DEFINITIONS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* SELF Test data file layout  */
typedef struct
{
   union
   {
      struct
      {
         uint16_t    nvFail          :   1;
         uint16_t    RTCFail         :   1;
         uint16_t    securityBFCFail :   1;
         uint16_t    securityFail    :   1;
         uint16_t    sdRAMFail       :   1;
         uint16_t    RSVD            :   11;
      }Bits;
      uint16_t Bytes;
   }uAllResults;
}LatestSelfTestResults_t;

typedef struct
{
   uint16_t selfTestCount;    /* Number of times self test has run            */
   uint16_t nvFail;           /* Number of times NV memory has failed         */
   uint16_t RTCFail;          /* Number of times Real Time Clock has failed   */
   uint16_t securityFail;     /* Number of times security device has failed   */
#if ( ( DCU == 1 ) || ( MQX_CPU == PSP_CPU_MK22FN512 ) )  /* DCU will always support externam RAM */
   uint16_t SDRAMFail;        /* Number of times SD RAM has failed   */
#endif
   LatestSelfTestResults_t lastResults;  /* Last test results - used to determine when Failed and Cleared events occur */
} SELF_TestData_t;

typedef enum
{
   e_nvFail = 0,
   e_RTCFail,
   e_securityBFCfail,
   e_securityFail,
#if ( DCU == 1 )  /* DCU will always support externam RAM */
   e_sdRAMFail
#endif
} SELFT_enum;

#if ( ACLARA_DVR_ABSTRACTION != 0)
typedef struct
{
   FileHandle_t            handle;
   char              const *FileName;     /*!< File Name           */
   ePartitionName    const ePartition;    /*!<                     */
   filenames_t       const FileId;        /*!< File Id             */
   SELF_TestData_t*  const Data;          /*!< Pointer to the data */
   lCnt              const Size;          /*!< Size of the data    */
   FileAttr_t        const Attr;          /*!< File Attributes     */
   uint32_t          const UpdateFreq;    /*!< Update Frequency    */
} SELF_file_t;
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

extern returnStatus_t   SELF_init( void );
#if ( FILE_IO !=0 )
extern SELF_file_t      *SELF_GetTestFileHandle( void );
#endif
#if (RTOS_SELECTION == MQX_RTOS)
extern OS_EVNT_Obj      *SELF_getEventHandle( void );
extern void SELF_setEventNotify( OS_EVNT_Obj *handle );
#endif
extern returnStatus_t   SELF_UpdateTestResults( void );
extern void             SELF_testTask( taskParameter );
extern returnStatus_t   SELF_testRTC( void );
//extern returnStatus_t   SELF_testSecurity( void );
extern returnStatus_t   SELF_testNV( void );
#if (SUPPORT_HEEP != 0)
extern returnStatus_t   SELF_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
#endif
#if ( DCU == 1 )  /* DCU will always support externam RAM */
extern returnStatus_t   SELF_testSDRAM( uint32_t LoopCount );
#endif

#endif /* this must be the last line of the file */
