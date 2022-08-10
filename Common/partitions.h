/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: partitions.h
 *
 * Contents: PAR_ - Contains partition information for performing memory I/O.  Provides a table access to all of the
 *   memory commands.
 *
 ***********************************************************************************************************************
 * Copyright (c) 2013 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
 * or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * v1.0, 20110415 kdavlin
 *
 **********************************************************************************************************************/
#ifndef PARTITIONS_H_
#define PARTITIONS_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* partition_EXTERN DEFINTION */

#ifdef partitions_GLOBAL
   #define partitions_EXTERN
#else
   #define partitions_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define PAR_CACHE_ENABLE            true  /* true enables the Cache, used in the partition attributes */
#define PAR_MIRROR_ENABLE           true  /* true enables the mirroring, used in the partition attributes */
#define PAR_AUTO_ERASE_BANK         true  /* true enables the auto erasing, used in the partition attributes */
#define PAR_NUM_OF_BANKS(x)         ((uint8_t)((uint8_t)x - 1)) /* Macro to set the number of banks where 0 = 1 bank */

#define PAR_UPDATE_RATE_UNLIMITED   ((unsigned)0) /* Used in the partition table to indicate unlimited endurance. */

#define PAR_CACHED                  true
#define PAR_BANKED                  true
#define DFW_MANIP                   true
#define FILE_SYS                    true

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
  ePWR_NORMAL = 0,      /* The flash driver will attempt to write/erase as fast as possible, polling if needed. */
  ePWR_LOW              /* The flash driver will put the processor to sleep while waiting for the BUSY signal. */
}ePowerMode;            /* This instructs the driver how to handle 'wait for write/erase completion'.  */

typedef struct
{
   dSize BankOffset;             /* Current bank for banked data.  This is an offset in the partition. */
   uint8_t CurrentBankSequence;  /* Used as a counter that is placed in the meta data when writing to a new bank.
                                    Zero indicates an erased state. */
   bool  cacheRestored;          /* RAM cache restored/valid */
}PartitionMetaData_t;

typedef struct
{
   uint8_t  const   *pBus;                /* Points to a const string that describes the bus being used */
   uint8_t  const   *pDevice;             /* Points to a const string that describes the chip being accessed */
   uint8_t  const   *pPartType;           /* Points to a const string that describes the partition */
   bool              banked         : 1;  /* True/False */
   bool              cached         : 1;  /* True/False */
   bool              AutoEraseBank  : 1;  /* false = Do not erase, true = erase previous bank? */
   bool              dfwManip       : 1;  /* True->DFW may manipulate this partition.*/
   bool              fileSys        : 1;  /* Does the partition utilize the file system structure */
}PartitionType_t;                         /* Used to help validate the partition. */

typedef struct
{
  PartitionMetaData_t   *pMetaData;             /* Location of the meta data */
  uint32_t              EraseBlockSize;         /* The smallest erase block size supported by the device */
  uint8_t               *pCachedData;           /* Location of the array to store the cached data */
  uint32_t              MaxUpdateRateSec;       /* Max Write Rate in Seconds */
  uint8_t               NumOfBanks;             /* 0 = Not Banked, 1 = Two Banks, 2 = three banks… */
}PartitionAttributes_t;

struct tagPartitionData /* Forward reference mechanism for structure below */
{
   ePartitionName             ePartition;          /* Partition ID, defined in file_io_cfg.h */
   phAddr                     PhyStartingAddress;  /* Physical starting address */
   lCnt                       lSize;               /* Logical Partition Size */
   lCnt                       lDataSize;           /* Usable data size = (total size - meta data) */
   struct tagDeviceDriverMem  const * const *pDriverTbl;  /* Pointer to the driver table */
   void                       *pDriverCfg;         /* Configuration of the device (Port, Address, etc...) */
   PartitionType_t            PartitionType;       /* Contains info about the driver chain, banked, cached, etc...*/
   PartitionAttributes_t      sAttributes;         /* Driver Attributes */
};

typedef struct tagPartitionData   PartitionData_t;  /* Contains partition information */

/* ----------------------------------------------------------------------------------------------------------------- */
/* Memory Device function typedefs and tables */

/* Defines all of the functions that each driver needs to support. */
typedef returnStatus_t (*dev_init_fptr)     (PartitionData_t const *, struct tagDeviceDriverMem const * const *);
typedef returnStatus_t (*dev_open_fptr)     (PartitionData_t const *, struct tagDeviceDriverMem const * const *);
typedef returnStatus_t (*dev_close_fptr)    (PartitionData_t const *, struct tagDeviceDriverMem const * const *);
typedef returnStatus_t (*dev_pwrMode_fptr)  (const ePowerMode, PartitionData_t const *, struct tagDeviceDriverMem const * const *);
typedef returnStatus_t (*dev_read_fptr)     (uint8_t *, const dSize, const lCnt, PartitionData_t const *, struct tagDeviceDriverMem const * const *);
typedef returnStatus_t (*dev_write_fptr)    (const dSize, uint8_t const *, const lCnt, PartitionData_t const *, struct tagDeviceDriverMem const * const *);
typedef returnStatus_t (*dev_erase_fptr)    (phAddr, lCnt, PartitionData_t const *, struct tagDeviceDriverMem const * const *);
#if ( MCU_SELECTED == RA6E1 )
typedef returnStatus_t (*dev_blank_fptr)    (phAddr, lCnt, PartitionData_t const *, struct tagDeviceDriverMem const * const *);
#endif
typedef returnStatus_t (*dev_flush_fptr)    (PartitionData_t const *, struct tagDeviceDriverMem const * const *);
typedef returnStatus_t (*dev_ioctl_fptr)    (const void *, void *, PartitionData_t const *, struct tagDeviceDriverMem const * const *);
typedef returnStatus_t (*dev_restore_fptr)  (lAddr lDest, lCnt Cnt, PartitionData_t const *, struct tagDeviceDriverMem const * const *);
typedef bool           (*dev_timeSlice_ftpr) (PartitionData_t const *, struct tagDeviceDriverMem const * const *);

/* A Generic Memory Device Driver function interface */
struct tagDeviceDriverMem
{
   dev_init_fptr        devInit;       /* init  Function Ptr */
   dev_open_fptr        devOpen;       /* Open  Function Ptr */
   dev_close_fptr       devClose;      /* Close Function Ptr */
   dev_pwrMode_fptr     devSetPwrMode; /* Power Mode Function Ptr */
   dev_read_fptr        devRead;       /* Read  Function Ptr */
   dev_write_fptr       devWrite;      /* Write Function Ptr */
   dev_erase_fptr       devErase;      /* Erase Function Ptr */
#if ( MCU_SELECTED == RA6E1 )
   dev_blank_fptr       devBlankCheck; /* Black check function Ptr */
#endif
   dev_flush_fptr       devFlush;      /* Flush Function Ptr */
   dev_ioctl_fptr       devIoctl;      /* Ioctl Function Ptr */
   dev_restore_fptr     devRestore;    /* restore Function Ptr */
   dev_timeSlice_ftpr   devTimeSlice;  /* Time Slice function pointer, used for Maintenance. */
};

/* A Generic Memory Device Driver function interface */
typedef struct tagDeviceDriverMem const DeviceDriverMem_t;

/* ------------------------------------------------------------------------------------------------------------------ */
/* Partition Manager Interface */

typedef returnStatus_t (*par_init_fptr)      (void);
typedef returnStatus_t (*par_open_fptr)      (PartitionData_t const **, const ePartitionName , uint32_t);
typedef returnStatus_t (*par_close_fptr)     (PartitionData_t const *);
typedef returnStatus_t (*par_pwr_fptr)       (const ePowerMode);
typedef returnStatus_t (*par_read_fptr)      (uint8_t *, const dSize, lCnt, PartitionData_t const *);
typedef returnStatus_t (*par_write_fptr)     (const dSize , uint8_t const *pSrc, lCnt, PartitionData_t const *);
typedef returnStatus_t (*par_erase_fptr)     (lAddr, lCnt, PartitionData_t const *);
#if ( MCU_SELECTED == RA6E1 )
typedef returnStatus_t (*par_blank_fptr)     (lAddr, lCnt, PartitionData_t const *);
#endif
typedef returnStatus_t (*par_flush_fptr)     (PartitionData_t const *);
typedef returnStatus_t (*par_restore_fptr)   (lAddr, lCnt, PartitionData_t const *);
typedef returnStatus_t (*par_ioctl_fptr)     (const void *, void *, PartitionData_t const *);
typedef returnStatus_t (*par_size_fptr)      (const ePartitionName, lCnt *);
typedef bool           (*par_timeSlice_ftpr) (void);

/* A Generic partition Memory Device Driver function interface */
typedef struct
{
   par_init_fptr        parInit;       /* Init Function Ptr */
   par_open_fptr        parOpen;       /* Open Function Ptr */
   par_close_fptr       parClose;      /* Close Function Ptr */
   par_pwr_fptr         parMode;       /* Power mode */
   par_read_fptr        parRead;       /* Read Function Ptr */
   par_write_fptr       parWrite;      /* Write Function Ptr */
   par_erase_fptr       parErase;      /* Erase Function Ptr */
#if ( MCU_SELECTED == RA6E1 )
   par_blank_fptr       parBlankCheck; /* Blank check Function Ptr */
#endif
   par_flush_fptr       parFlush;      /* Flush Function Ptr */
   par_restore_fptr     parRestore;    /* Restore Function Ptr */
   par_ioctl_fptr       parIoctl;      /* ioctl commands */
   par_size_fptr        parSize;       /* returns the size of the given partition(s) */
   par_timeSlice_ftpr   parTimeSlice;  /* Time Slice function pointer, used for Maintenance. */
}PartitionTbl_t;

/* This structure is used to traverse the partition table */
typedef struct
{
   uint8_t nextPartition; /* Index of next partition to be returned */
}PAR_getNextMember_t;


/* ****************************************************************************************************************** */
/* CONSTANTS */

extern const   PartitionTbl_t PAR_partitionFptr;

/* ****************************************************************************************************************** */
/* partition_EXTERN VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

bool PAR_timeSlice(void);
#if (RTOS == 1)
void PAR_appTask( taskParameter );
#endif
void PAR_flushPartitions(void);
returnStatus_t PAR_init( void );
returnStatus_t PAR_initRtos( void );
returnStatus_t PAR_ValidatePartitionTable( void );
PartitionData_t *PAR_GetFirstPartition(PAR_getNextMember_t *pMember);
PartitionData_t *PAR_GetNextPartition(PAR_getNextMember_t *pMember);

#if defined TEST_MODE_ENABLE && defined TM_DVR_PARTITION_UNIT_TEST
returnStatus_t ValidatePartitionTable(PartitionData_t const *, uint8_t);
#endif

#undef partitions_EXTERN

#endif
