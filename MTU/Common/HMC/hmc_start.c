/***********************************************************************************************************************

   Filename:   hmc_start.c

   Global Designator: HMC_STRT_

   Contents: Starts the meter communication session, log-in, password, etc...

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2020-2021 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   $Log$ KAD Created 2008, 9/8

 ***********************************************************************************************************************
   Revision History:
   v0.1 -   Initial Release
   v0.2 -   A table error command is now handled the same was as an error or abort command.
            The proper return for an unknown command is now returned properly.
            The starting value of update_ is now "idle".
   v0.3 -   Returns 'Pause' when waiting to communicate with the host meter.  The pause status will be returned when
            the timer gate, wait_mS_, is not zero and this applet has been activated.
   v0.4 -   Added a few comments
   v0.5 -   On an error, call HMC_STRT_RestartDelay() and set state to not logged in.
 ******************************************************************************************************************** */
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "project.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>                       /* Needed for mqx typedefs, etc. */
#include <fio.h>
#endif
#if 0 // TODO: RA6E1 - Enable wolfssl
#include <wolfssl/internal.h>
#endif
#include "dvr_intFlash_cfg.h"          /* Internal flash variables structure  */
#include "hmc_finish.h"
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
#include "hmc_prg_mtr.h"
#endif
#include "hmc_msg.h"
#include "hmc_start.h"
#if ( ANSI_LOGON == 1 )
#include "hmc_eng.h"
#endif
#include "pwr_restore.h"
#include "timer_util.h"
#if ( CLOCK_IN_METER == 1 )
#include "hmc_sync.h"
#endif
#if !RTOS
#include "timer.h"
#endif
#include "ecc108_comm_marshaling.h" /* ECC108 return values, keys sizes, etc. */
#if ( RTOS_SELECTION == MQX_RTOS )
#include "ecc108_mqx.h"             /* SEC_GetSecPartHandle()  */
#endif
#include "DBG_SerialDebug.h"
#if (HMC_KV == 1)
#include "serial.h"
#endif
#include "hmc_config.h"

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

#if PROCESSOR_LITTLE_ENDIAN == 1 && BIT_ORDER_LOW_TO_HIGH == 1

/* The structure below is defined for little endian machine with bit order from low to high. */
#if ( ANSI_SECURITY == 1 )

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  password[20];           /* HMC password   */
   uint8_t  masterPassword[20];     /* HMC Master password   */
   uint8_t  pad[8];                 /* Encryption area must be multiple of 16 bytes */
   uint8_t  md5[ MD5_DIGEST_SIZE ]; /* Integrity check on encrypted passwords */
} MtrStartFileData_t;
PACK_END
#endif

#if (NEGOTIATE_HMC_COMM == 1)
typedef enum
{
  eStrtNegotiate,
  eStrtLoggingIn,
  eStrtIdleLoggedIn,
  eStrtLoggedOff
} eStrtStates_t;
#endif


#else
#if PROCESSOR_LITTLE_ENDIAN == 0
#error "pqFileHndl_t is not defined for a big endian machine!  The structure above must be defined for big endian."
#endif
#if BIT_ORDER_LOW_TO_HIGH == 0
#error "pqFileHndl_t is not defined for a bit order of high to low. "
#endif
#endif

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#if RTOS && HMC_FOCUS
static bool       bDoIscDelay = ( bool )false;  /* If an RTOS is used, this will cause a delay during the process cmd. */
#endif

#if HMC_FOCUS
static bool       iscPause_ = ( bool )false;
#endif

#ifndef TEST_COM_UPDATE_APPLET
static HMC_MSG_Configuration comSettings_;  // The comm protocol params used when communicationg with the host meter.
#else // Global so we can observe it
HMC_MSG_Configuration comSettings_;         // The comm protocol params used when communicationg with the host meter.
#endif

static bool                timerExpired_ = ( bool )true;

static uint8_t             loggedIn_ = ( uint8_t )HMC_APP_API_RPLY_LOGGED_OFF;
static uint8_t             update_   = ( uint8_t )HMC_APP_API_RPLY_IDLE;
static uint16_t            timerId_  = 0;             /* Stores timer ID so module uses only one timer   */
#if ( ANSI_SECURITY == 1 )
static FileHandle_t        fileHndlStart = { 0 };     /* Contains the meter start file handle         */
static MtrStartFileData_t  mtrFileData;               /* Contains the meter start data from the file  */
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#ifdef _DEBUG_MODE
#define HMC_STRT_POWER_UP_COM_DELAY       ((uint32_t)4000)        /* 2s Power up Delay for Testing */
#else
#define HMC_STRT_POWER_UP_COM_DELAY       ((uint32_t)3000)        /* 3s Power up Delay */
#endif

#define HMC_STRT_ISC_DELAY_mS             ((uint32_t)600000)      /* Delay due to ISC error */
/* Note: The actual delay between two logon attempts will be between 5 to 10 mins */

//lint -esym(750,COMM_FORCE_SAMPLE_DELAY_mS,COMM_FORCE_BYPASS_mS,COMM_FORCE_BUSY_PRINT_mS)
#define COMM_FORCE_SAMPLE_DELAY_mS        ((uint32_t)50)          /* Delay between comm force samples */
#define COMM_FORCE_BYPASS_mS              ((uint32_t)5*60*1000)   /* How long to wait for comm_force busy */
#define COMM_FORCE_BUSY_PRINT_mS          ((uint32_t)15*1000)     /* How often to print busy status */

/* NOTE:  Make sure MAX_TIME_TO_WAIT_FOR_TIMER_mS is less than HMC_APP_APPLET_TIMEOUT_mS in config.h.  */
#define MAX_TIME_TO_WAIT_FOR_TIMER_mS     ((uint32_t)5*60*1000)   /* Wait no more than 5 minutes for the timer */
#define TIMER_WAIT_SLEEP_TIME_mS          ((uint32_t)10)          /* Check for timer every x mSeconds */
#define PORT_SETTLE_DELAY                 ((uint32_t)2*TEN_MSEC)  /* Time delay after comm parameter negotiations*/

#if ENABLE_PRNT_HMC_START_INFO
#define HMC_START_PRNT_INFO(x,y)    DBG_logPrintf (x,y)
#else
#define HMC_START_PRNT_INFO(x,y)
#endif

#if ENABLE_PRNT_HMC_START_WARN
#define HMC_START_PRNT_WARN(x,y)    DBG_logPrintf (x,y)
#else
#define HMC_START_PRNT_WARN(x,y)
#endif

#if ENABLE_PRNT_HMC_START_ERROR
#define HMC_START_PRNT_ERROR(x,y)    DBG_logPrintf (x,y)
#else
#define HMC_START_PRNT_ERROR(x,y)
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */
#if ( ANSI_SECURITY == 1 )
static const uint8_t HMC_PASSWORD[] =
{
   0x5F, 0x29, 0x6E, 0x00, 0x29, 0xFC, 0x7C, 0xAE, 0x64, 0xDD,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t HMC_MASTER_PASSWORD[] =
{
   0x5F, 0x29, 0x6E, 0x00, 0x29, 0xFC, 0x7C, 0xAE, 0x64, 0xEE, // RCZ changed from 0xDD, to accommodate MeterMate
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif


#if (ANSI_LOGON == 1)
static const uint8_t HMC_MTR_USER_NAME[] =
{
   0, 2, 'A', 'C', 'L', 'A', 'R', 'A', 0x20, 0x20, 0x20, 0x20
};
#endif

#if( NEGOTIATE_HMC_COMM == 1)
static const uint8_t  HMC_NEG_COMM[]       = {
   0x00, ALT_HMC_PACKET_SIZE, ALT_HMC_PAKCET_NUMBER, ALT_HMC_BAUD_CODE //deafults are used if parmeter switches not enabled
};
#endif




#if (ANSI_LOGON == 1)
static const uint8_t HMC_IdentifyStr[]   =   { CMD_IDENT };
static const uint8_t HMC_LogonStr[]      =   { CMD_LOG_ON_REQ };
#endif
#if (ANSI_SECURITY == 1)
static const uint8_t HMC_SecurityStr[]   =   { CMD_PASSWORD };
#endif

#if ( NEGOTIATE_HMC_COMM == 1 )
static const uint8_t HMC_NegCommStr[]   = { CMD_NEG_SERVICE_REQ_ONE_BAUD };
static eStrtStates_t curState = eStrtLoggedOff; //a FSM ust be added in order to negoiate the COMM parameters
#endif

#if (  NEGOTIATE_HMC_BAUD == 1 )
static const uint32_t defaultBaudRate_s = DEFAULT_HMC_BAUD_RATE;
static const uint32_t altBaudRate_s = ALT_HMC_BAUD_RATE;
#endif


#if (ANSI_LOGON == 1)
static const HMC_PROTO_TableCmd HMC_Identify[] =
{
   {
      HMC_PROTO_MEM_PGM,
      sizeof ( HMC_IdentifyStr ),
      ( uint8_t far * )HMC_IdentifyStr
   },
   {
      HMC_PROTO_MEM_NULL
   }
};
static const HMC_PROTO_TableCmd HMC_Logon[] =
{
   {
      HMC_PROTO_MEM_PGM, sizeof ( HMC_LogonStr ), ( uint8_t far * )HMC_LogonStr
   },
   {
      HMC_PROTO_MEM_PGM, sizeof ( HMC_MTR_USER_NAME ), ( uint8_t far * )&HMC_MTR_USER_NAME[0]
   },
   {
      HMC_PROTO_MEM_NULL
   }
};
#endif
#if (ANSI_SECURITY == 1)
static const HMC_PROTO_TableCmd HMC_Security[] =
{
   {
      HMC_PROTO_MEM_PGM, sizeof( HMC_SecurityStr ), ( uint8_t far * )HMC_SecurityStr
   },
   {
      HMC_PROTO_MEM_PGM, sizeof( mtrFileData.password ), mtrFileData.password
   },
   {
      HMC_PROTO_MEM_NULL
   }
};

static const uint8_t iv[] =
{
   0x56, 0x70, 0x8f, 0x5e, 0x11, 0x4b, 0x11, 0x69,
   0x9d, 0x3e, 0x88, 0xaa, 0xb1, 0x3e, 0x92, 0x3c
};

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) || ( END_DEVICE_PROGRAMMING_DISPLAY == 1 ) )
static const HMC_PROTO_TableCmd HMC_Master_Security[] =
{
   {
      HMC_PROTO_MEM_PGM, sizeof( HMC_SecurityStr ), ( uint8_t far * )HMC_SecurityStr
   },
   {
      HMC_PROTO_MEM_PGM, sizeof( mtrFileData.masterPassword ), mtrFileData.masterPassword
   },
   {
      HMC_PROTO_MEM_NULL
   }
};
#endif
#endif

#if (NEGOTIATE_HMC_COMM == 1)
static const HMC_PROTO_TableCmd HMC_NegComm[] =
{
  {
    HMC_PROTO_MEM_PGM, sizeof(HMC_NegCommStr), (uint8_t far *)HMC_NegCommStr
  },
  {
    HMC_PROTO_MEM_PGM, sizeof (HMC_NEG_COMM), (uint8_t far *)&HMC_NEG_COMM[0]
  },
  {
    HMC_PROTO_MEM_NULL
  }
};
#endif


#if (ANSI_LOGON == 1 && NEGOTIATE_HMC_COMM == 1 && ANSI_SECURITY == 1 )
static const HMC_PROTO_Table HMC_PROTO_NegCommTbl[] =
{
   {  HMC_Identify },
   {  HMC_NegComm },
   {  (void *)NULL }
};

static const HMC_PROTO_Table HMC_PROTO_OpenSessionTbl[] =
{
   //ident is part of NEGOTIATE
   {  HMC_Logon },
   {  HMC_Security },
   {  (void *)NULL }
};

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
static const HMC_PROTO_Table HMC_PROTO_MasterSecTbl[] =
{
   //ident is part of NEGOTIATE
   {  HMC_Logon },
   {  HMC_Master_Security },
   {  (void *)NULL }
};
#endif
#elif (ANSI_LOGON == 1 && NEGOTIATE_HMC_COMM == 0 && ANSI_SECURITY == 1)
static const HMC_PROTO_Table HMC_PROTO_OpenSessionTbl[] =
{
   {  HMC_Identify },
   {  HMC_Logon },
   {  HMC_Security },
   {  (void *)NULL }
};

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
static const HMC_PROTO_Table HMC_PROTO_MasterSecTbl[] =
{
   {  HMC_Identify },
   {  HMC_Logon },
   {  HMC_Master_Security },
   {  (void *)NULL }
};
#endif
#elif (ANSI_LOGON == 1 && NEGOTIATE_HMC_COMM == 1 && ANSI_SECURITY == 0)
static const HMC_PROTO_Table HMC_PROTO_NegCommTbl[] =
{
   {  HMC_Identify },
   {  HMC_NegComm },
   {  (void *)NULL }
};
static const HMC_PROTO_Table HMC_PROTO_OpenSessionTbl[] =
{
  {  HMC_Logon },
  {  (void *)NULL }
};

#elif (ANSI_LOGON == 1 && NEGOTIATE_HMC_COMM == 0 && ANSI_SECURITY == 0)
static const HMC_PROTO_Table HMC_PROTO_OpenSessionTbl[] =
{
  {  HMC_Identify },
  {  HMC_Logon },
  {  (void *)NULL }
};
#endif
#if 0

static const FileAttributes_t mtrStartFileAttr_ = /* The attributes for the file. */
{
   0 /* This module doesn't use the checksum. */
};

static const tRegDefintion mtrStartRegDefTbl_[] = /* Define registers that belong to the time module. */
{
   {
      { /* Host Meter Password Register Definitions */
         REG_HMC_HMC_PASSWORD,                              /* Reg Number */
         REG_HMC_HMC_PASSWORD_SIZE,                         /* Reg Length */
         REG_MEM_FILE,                                      /* Memory Type */
         ( uint8_t * )&mtrStartFileHndl_,                   /* Address of Register */
//xxx         offsetof(retMtrFileData_t, password),         /*lint !e718 !e746 !e734 !e413 !e545 !e516 !e628 Offset of the register */
         0, //xxx
         ( uint64_t )0x0000000000000000,                    /* Minimum Value contained in this register */
         ( uint64_t )0xFFFFFFFFFFFFFFFF,                    /* Maximum Value contained in this register */
         ( int8_t * )NULL,                                  /* pointer to decoder. */
         { 0, 0, 0, 0, 0, 0, 0, 0x35, 0x31, 0x30, 0x39, 0x33, 0x26, 0x33, 0x32},   /* Initial Value */
         !REG_PROP_NATIVE_FMT,                              /* Register is stored in native format. */
         REG_PROP_WR_TWACS,                                 /* Writable over TWACS/STAR (Main Network)? */
         REG_PROP_WR_MFG,                                   /* Writable over MFG? */
         REG_PROP_WR_PERIF,                                 /* Writable over Perif */
         !REG_PROP_RD_TWACS,                                /* Readable over TWACS/STAR (Main Network)? */
         !REG_PROP_RD_MFG,                                  /* Readable over MFG? */
         !REG_PROP_RD_PERIF,                                /* Readable over Perif? */
         !REG_PROP_SECURE,                                  /* Secure? */
         REG_PROP_MANIPULATE,                               /* Manipulate? */
         !REG_PROP_CHECKSUM,                                /* Checksum? */
         !REG_PROP_MATH,                                    /* Math - Add? */
         !REG_PROP_MATH_ROLL,                               /* Inc/Dec roll over/under? */
         !REG_PROP_SIGNED,                                  /* Signed value? */
         !REG_PROP_USE_MIN_MAX,                             /* use the min and max values when writing? */
         REG_PROP_USE_DEFAULT_FIRST_POWER_UP,               /* Set default at 1st time ever power up? */
         !REG_PROP_USE_DEFAULT_EVERY_POWER_UP,              /* Set default at every power up? */
         !REG_PROP_USE_DEFAULT_CMD_1,                       /* Set default at cmd 1? */
         !REG_PROP_USE_DEFAULT_CMD_2,                       /* Set default at cmd 2? */
         !REG_PROP_USE_DEFAULT_CMD_3,                       /* Set default at cmd 3? */
      },
      REG_DEF_NULL_PRE_READ_EXE,                            /* Function Ptr: Executes before data is read. */
      REG_DEF_NULL_PRE_WRITE,                               /* Function Ptr: Execute before data is written. */
      REG_DEF_NULL_POST_WRITE,                              /* Function Ptr: Executes after data is written. */
      REG_DEF_NULL_UPDATE,                                  /* Function pointer to the update function */
      REG_DEF_NULL_REG_ACCESS                               /* Function pointer to the Access function */
   }
};

/* Define the table that will be used for the object list entry. */
static const OL_tTableDef mtrStartRegTbl_ =
{
   /*lint -e(740) unusual cast */
   ( tElement  * )&mtrStartRegDefTbl_[0].sRegProp.uiRegNumber, /* Points to the 1st element in the register table. */
   ARRAY_IDX_CNT( mtrStartRegDefTbl_ )                       /* Number of registers defined for the time module. */
};

#endif
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#if !RTOS
static void HMC_STRT_SET_COMPORT( void );
#endif
static void startTimer( uint32_t tmr_mS );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
#if 0 // TODO: RA6E1 - Enable wolfssl
/***********************************************************************************************************************

   Function name: encryptBuffer

   Purpose: Encrypt the buffer with AES-CBC mode, which requires block padding before pass-in.

   Arguments:
            uint8_t  *dest - encrypt destinaion
            uint8_t  *src  - encrypt soure
            int32_t   size - soure size, must be multiple integer size of AES block size.
            uint8_t  *key  - key to be used to encrypt the data

   Returns: None
 ******************************************************************************************************************** */
/*lint -e{818}  parameters could be ptr to const. */
static void encryptBuffer( uint8_t *dest, uint8_t *src, int32_t size, uint8_t key[ECC108_KEY_SIZE] )
{
   Aes aes;

   (void)wc_AesSetKey( &aes, key, (uint32_t)ECC108_KEY_SIZE, iv, AES_ENCRYPTION );
   (void)wc_AesCbcEncrypt( &aes, dest, src, (uint32_t)size );
}

/***********************************************************************************************************************

   Function name: decryptBuffer

   Purpose: Decrypt the buffer with AES-CBC mode, which requires block padding before pass-in.

   Arguments:
            uint8_t  *dest - decrypt destinaion
            uint8_t  *src  - decrypt soure
            int32_t   size  - soure size, must be multiple integer size of AES block size.
            uint8_t  *key  - key to be used to encrypt the data

   Returns: None
 ******************************************************************************************************************** */
/*lint -e{818}  parameters could be ptr to const. */
static void decryptBuffer( uint8_t *dest, uint8_t *src, int32_t size, uint8_t key[ECC108_KEY_SIZE] )
{
   Aes aes;

   (void)wc_AesSetKey( &aes, key, (uint32_t)ECC108_KEY_SIZE, iv, AES_DECRYPTION );
   (void)wc_AesCbcDecrypt( &aes, dest, src, (uint32_t)size );
}

/***********************************************************************************************************************

   Function name: generateMD5

   Purpose: Generate MD5 at the end of buffer.

   Arguments:
            uint8_t  *buf  - buffer
            int32_t   size  - buffer size.

   Returns: None
 ******************************************************************************************************************** */
static void generateMD5( uint8_t *buffer, int32_t size )
{
   int32_t  dataSize = size - MD5_DIGEST_SIZE;
   (void)wc_Md5Hash( buffer, (uint32_t)dataSize, buffer + dataSize );
}

/***********************************************************************************************************************

   Function name: validateMD5

   Purpose: Verify buffer data integration with the MD5 sotred at the end of buffer.

   Arguments: bool

   Returns: None
 ******************************************************************************************************************** */
/*lint -e{818}  parameters could be ptr to const. */
static int32_t validateMD5( uint8_t *buffer, int32_t size )
{
   uint8_t  actual[MD5_DIGEST_SIZE];
   int32_t  dataSize = size - MD5_DIGEST_SIZE;
   int32_t  result;

   (void)wc_Md5Hash( buffer, (uint32_t)dataSize, actual );

   result = XMEMCMP( buffer + dataSize, actual, MD5_DIGEST_SIZE );
   return !result;
}

#endif
#if ( ANSI_SECURITY == 1 )
/***********************************************************************************************************************
   Function name: HMC_STRT_UpdatePasswords

   Purpose: Check host meter passwords for encryption and decrypt if necessary. Will also encrypt passwords and write to file if not already
            encrypted.

   Notes:   Uses the RAM values in mtrFileData.
            If the structure passes a decryption/md5 validity check, then the decrypted values replace the values in the structure.
            Otherwise, the orginal values are considered to be unencrypted. Those values have an md5 hash computed and placed in the
            structure. Then the entire structure is encrypted and written to the file.
            In any case, the unencrypted values end up in the RAM structure.

   Arguments: none
   Returns: Success/failure of update
 ******************************************************************************************************************** */
returnStatus_t HMC_STRT_UpdatePasswords( void )
{
   buffer_t                *keyBuf;
   buffer_t                *mtrFile;
   PartitionData_t const   *pPart;              /* Used to access the security info partition      */
   returnStatus_t          retVal = eFAILURE;
   uint8_t                 *key;

   keyBuf = BM_alloc( sizeof( ECC108_KEY_SIZE ) );
   if( keyBuf != NULL )
   {
      key = keyBuf->data;
      mtrFile = BM_alloc( sizeof( mtrFileData ) );
      if( mtrFile != NULL )
      {
         pPart = SEC_GetSecPartHandle();              /* Obtain handle to access internal NV variables   */
         if ( eSUCCESS == PAR_partitionFptr.parRead( key, (dSize)offsetof( intFlashNvMap_t, hostPasswordKey.key ), (lCnt)ECC108_KEY_SIZE, pPart ) )
         {
            /* Attempt decryption/verification  */
            decryptBuffer( mtrFile->data, (uint8_t *)&mtrFileData, (int32_t)sizeof( mtrFileData ), key );   /* Decrypted passwords in mtrFile->data   */
            if ( !validateMD5( mtrFile->data, (int32_t)sizeof( mtrFileData ) ) )  /* If this fails, assume unencrypted data; encrypt and write file.  */
            {
               /* Save unencrypted data   */
               (void)memset( mtrFileData.pad, 0, sizeof( mtrFileData.pad ) );
               (void)memcpy( mtrFile->data, (uint8_t *)&mtrFileData, sizeof( mtrFileData ) );   /* Copy unencrypted passwords to mtrFile->data  */
               generateMD5( (uint8_t *)&mtrFileData, (int32_t)sizeof( mtrFileData ) ); /* Compute integrity check over the file  */
               encryptBuffer( (uint8_t *)&mtrFileData, (uint8_t *)&mtrFileData, (int32_t)sizeof( mtrFileData ), key );        /* Encrypt in place  */
               retVal = FIO_fwrite( &fileHndlStart, 0, ( uint8_t * )&mtrFileData, ( lCnt )sizeof( mtrFileData ) );   /* Update the file   */
            }
            else
            {
               retVal = eSUCCESS;
            }
            /* Need unencrypted passwords in the normal location (mtrFileData)   */
            (void)memcpy( (uint8_t *)&mtrFileData, mtrFile->data, sizeof( mtrFileData ) );
         }
         BM_free( mtrFile );
      }
      BM_free( keyBuf );
   }
   if ( eSUCCESS != retVal )
   {
      DBG_logPrintf( 'E', "%s failed", __func__ );
   }
   return retVal;
}
#endif

/***********************************************************************************************************************

   Function name: HMC_STRT_init

   Purpose: Adds registers to the Object list and opens files.  This must be called at power before any other function
            in this module is called and before the register module is initialized.

   Arguments: None

   Returns: returnStatus_t - result of file open.

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
returnStatus_t HMC_STRT_init( void )
{
   timer_t        timerCfg;               /* Used to configure the timers and read the status of a timer. */
   returnStatus_t retVal;
#if ( ANSI_SECURITY == 1 )
   FileStatus_t   fileStatus;

   retVal = eSUCCESS;
   if( NULL == fileHndlStart.pTblInfo )
   {
      if ( eSUCCESS == FIO_fopen( &fileHndlStart, ePART_SEARCH_BY_TIMING, ( uint16_t )eFN_HMC_START,
                                  ( lCnt )sizeof( MtrStartFileData_t ), FILE_IS_NOT_CHECKSUMED,
                                  0xFFFFFFFF, &fileStatus ) )
      {
         if ( fileStatus.bFileCreated )
         {
            /* The file was just created for the first time. Save the default values to the file */
            (void)memset( (uint8_t *)&mtrFileData, 0, sizeof( mtrFileData ) );
            (void)memcpy( mtrFileData.password, HMC_PASSWORD, sizeof( mtrFileData.password ) );
            (void)memcpy( mtrFileData.masterPassword, HMC_MASTER_PASSWORD, sizeof( mtrFileData.masterPassword ) );
            retVal = FIO_fwrite( &fileHndlStart, 0, ( uint8_t * )&mtrFileData, ( lCnt )sizeof( mtrFileData ) );
         }
         else
         {
            retVal = FIO_fread( &fileHndlStart, ( uint8_t * )&mtrFileData, 0, ( lCnt )sizeof( mtrFileData ) );
         }
      }
      else
      {
         retVal = eFAILURE;
      }
   }
   if ( eSUCCESS == retVal )
#endif
   {
      timerExpired_ = ( bool )false;
      ( void )memset( &timerCfg, 0, sizeof( timerCfg ) ); /* Clear the timer values */
      timerCfg.bOneShot = true;
      timerCfg.ulDuration_mS = 1;   /* Set to max duration for initialization */
      timerCfg.pFunctCallBack = HMC_STRT_ISR;
      retVal = TMR_AddTimer( &timerCfg );
      timerId_ = timerCfg.usiTimerId;
   }
   return( retVal );
}

/***********************************************************************************************************************

   Function name: HMC_STRT_LogOn()

   Purpose: Logs onto the host meter

   Arguments: uint8_t ucCmd, void far *pPtr

   Returns: uint8_t - STATUS

 **********************************************************************************************************************/
//lint -efunc(818,HMC_STRT_LogOn)
uint8_t HMC_STRT_LogOn( uint8_t ucCmd, void far *pPtr )
{
   static uint8_t bFirstLogon = true;
   uint8_t retVal;


   retVal = ( uint8_t )HMC_APP_API_RPLY_UNSUPPORTED_CMD;

   switch ( ucCmd )
   {
      case HMC_APP_API_CMD_ABORT:
      case HMC_APP_API_CMD_ERROR:
      case HMC_APP_API_CMD_TBL_ERROR:
      {
         HMC_START_PRNT_ERROR( 'E', "Start - Logon" );
         //LED_HMC_OFF();
#if ( ANSI_LOGON == 1 )
         ( void )HMC_FINISH_LogOff( ( uint8_t )HMC_APP_API_CMD_ACTIVATE, NULL );
         loggedIn_ = ( uint8_t )HMC_APP_API_RPLY_LOGGED_OFF;
         HMC_STRT_RestartDelay();  /* About 39.9 seconds */
         update_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
         retVal = update_;
#else
         update_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
         retVal = update_;
#endif
#if ( NEGOTIATE_HMC_COMM == 1)
         curState = eStrtLoggedOff;
#endif
         break;
      }
      case HMC_APP_API_CMD_ACTIVATE:
      {
         HMC_START_PRNT_INFO( 'I', "Start - Log on Requested" );
#if ( ANSI_LOGON == 1 )
         update_ = ( uint8_t )HMC_APP_API_RPLY_READY_COM;
         loggedIn_ = ( uint8_t )HMC_APP_API_RPLY_LOGGING_IN;
#if( NEGOTIATE_HMC_COMM == 1)
         curState = eStrtNegotiate; //have to negoiate comm parameters
#endif //END NEGOTIATE_HMC_COMM
         retVal = update_;
#else //ANSI_LOGON == 0
         update_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
         loggedIn_ = ( uint8_t )HMC_APP_API_RPLY_LOGGED_IN;
         retVal = update_;
#endif //END ANSI_LOGON
         break;
      }
      case HMC_APP_API_CMD_PROCESS:
      {
         HMC_START_PRNT_INFO( 'I', "Start - Logging in" );
         uint32_t maxTimerWait = MAX_TIME_TO_WAIT_FOR_TIMER_mS;
         while( !timerExpired_ && ( 0 != maxTimerWait ) )
         {
            OS_TASK_Sleep( TIMER_WAIT_SLEEP_TIME_mS );
            maxTimerWait -= TIMER_WAIT_SLEEP_TIME_mS;
         }
#if ( ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
         /* todo: Need to check for power down signal from the KV before communications can start!  */

         if ( HMC_COMM_FORCE() ) /*lint !e506 !e681 This may be a constant value depending on the build */
         {
            /* Check for busy signal from the KV before communications can start!  */
            uint16_t printCnt          = COMM_FORCE_BUSY_PRINT_mS / COMM_FORCE_SAMPLE_DELAY_mS;
            uint16_t commForceBypass   = COMM_FORCE_BYPASS_mS / COMM_FORCE_SAMPLE_DELAY_mS;
            while ( HMC_COMM_FORCE() ) /*lint !e506 !e681 This may be a constant value depending on the build */
            {
               OS_TASK_Sleep( COMM_FORCE_SAMPLE_DELAY_mS );
               if ( 0 == --commForceBypass )
               {
                  HMC_START_PRNT_WARN( 'W',  "Start - Bypassing COMM_FORCE signal!" );
                  break;
               }
               if ( 0 == --printCnt )
               {
                  HMC_START_PRNT_ERROR( 'E',  "Start - Meter is Busy - Can't Log In!" );
                  printCnt = COMM_FORCE_BUSY_PRINT_mS / COMM_FORCE_SAMPLE_DELAY_mS;
               }
            }
            HMC_ENG_Execute( ( uint8_t )HMC_ENG_HDWR_BSY, 0 );
            OS_TASK_Sleep( 50 );
            /* Delay here */
            startTimer( HMC_KV_UART_SETUP_DELAY );
         }
#endif
#if ( HMC_I210_PLUS_C == 1 )
         HMC_MUX_CTRL_ASSERT();
#endif
#if ( ANSI_LOGON == 1 )
#if ( NEGOTIATE_HMC_COMM == 1 )
         switch ( curState )
         {
         case eStrtNegotiate:
           ((HMC_COM_INFO *)pPtr)->pCommandTable = (uint8_t far *)&HMC_PROTO_NegCommTbl[0]; //ident, change baud
           break;
         case eStrtLoggingIn:
#endif //END NEGOTIATE_HMC_COM
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
         if ( !HMC_PRG_MTR_IsSynced( ) )
#endif
         {
            ( ( HMC_COM_INFO * )pPtr )->pCommandTable = ( uint8_t far * )&HMC_PROTO_OpenSessionTbl[0]; // Ident, Logon, Security
         }
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
         else
         {
            ( ( HMC_COM_INFO * )pPtr )->pCommandTable = ( uint8_t far * )&HMC_PROTO_MasterSecTbl[0]; // Ident, Logon, Master Security
         }
#endif
#if ( NEGOTIATE_HMC_COMM == 1 )
           break;
         }
#endif //END NEGOTIATE_HMC_COM
         loggedIn_ = (uint8_t)HMC_APP_API_RPLY_LOGGING_IN;
         retVal = update_;
#else  //ANSI_LOGON == 0
         retVal = (uint8_t)HMC_APP_API_RPLY_IDLE;
#endif //END ANSI_LOGON
         break;
      }
      case HMC_APP_API_CMD_INIT:
      {
#if ( ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
         HMC_COMM_FORCE_TRIS();
         HMC_COMM_SENSE_TRIS();
#endif
         //LED_HMC_INIT();
         startTimer( HMC_STRT_POWER_UP_COM_DELAY );
         loggedIn_ = ( uint8_t )HMC_APP_API_RPLY_LOGGED_OFF;
         update_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
         retVal = update_;
         //xxx HMC_STRT_SET_COMPORT();
         break;
      }
      case HMC_APP_API_CMD_MSG_COMPLETE:
      {

#if ( ANSI_LOGON == 1 )
#if (NEGOTIATE_HMC_COMM == 1)
         if( eStrtNegotiate == curState )
         {
           curState = eStrtLoggingIn;
           update_ = (uint8_t)HMC_APP_API_RPLY_READY_COM;
           if( (((HMC_COM_INFO *)pPtr)->RxPacket.ucResponseCode == RESP_OK ) &&
               (((HMC_COM_INFO *)pPtr)->RxPacket.uRxData.sNegotiate0.ucBaudCode == ALT_HMC_BAUD_CODE))

           {
             HMC_START_PRNT_INFO('I', "Finsihed COMM negoaitions");
             OS_TASK_Sleep( PORT_SETTLE_DELAY ); //wait for serial port to settle
#if( NEGOTIATE_HMC_BAUD == 1)

             if( MQX_OK != UART_ioctl(UART_HOST_COMM_PORT, IO_IOCTL_SERIAL_SET_BAUD, (void *)&(altBaudRate_s)) )
             {
               update_ = HMC_APP_API_RPLY_ABORT_SESSION;
               HMC_START_PRNT_ERROR('E',  "Unable to set ALT_BAUD baud rate during negotiate. Session Failed");
             }
             else
             {
               HMC_START_PRNT_INFO('I', "Set UART to alternative baud rate");
             }
#endif

           }
           else //failed to negoiate the paramters correctly
           {
             update_ = HMC_APP_API_RPLY_ABORT_SESSION;
           }
         }
         else if( eStrtLoggingIn == curState )
#endif //end NEGOTIATE_HMC_COMM
         {
           loggedIn_ = (uint8_t)HMC_APP_API_RPLY_LOGGED_IN;
           update_ = (uint8_t)HMC_APP_API_RPLY_IDLE;
#if (NEGOTIATE_HMC_COMM == 1)
           curState = eStrtIdleLoggedIn;
#endif
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
         if ( !HMC_PRG_MTR_IsSynced( ) )
#endif
         {
            HMC_START_PRNT_INFO( 'I', "Start - Logged in" );
         }
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
         else
         {
            HMC_START_PRNT_INFO( 'I', "Start - Master Logged in" );
         }
#endif
         }
         retVal = update_;
#else //ANSI_LOGON == 0
       retVal = (uint8_t)HMC_APP_API_RPLY_IDLE;
#endif //ANSI_LOGON
#if ( CLOCK_IN_METER == 1 )
         ( void )HMC_SYNC_applet( ( uint8_t )HMC_APP_API_CMD_ACTIVATE, NULL );
#endif
         break;
      }
      case HMC_APP_API_CMD_LOGGED_IN:
      {
         retVal = loggedIn_;
         break;
      }
      case HMC_APP_API_CMD_LOG_OFF:
      {
         loggedIn_ = ( uint8_t )HMC_APP_API_RPLY_LOGGED_OFF;
         update_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
         retVal = update_;
         break;
      }
      case HMC_APP_API_CMD_STATUS:
      {
#if ( ANSI_LOGON == 1 )
         retVal = update_;
#if RTOS
#if HMC_FOCUS
         if ( bDoIscDelay )
         {
            startTimer( HMC_STRT_ISC_DELAY_mS );
         }
         bDoIscDelay = false;
#endif
#else
         if ( !timerExpired_ ) /* Are we waiting for a meter imposed delay (e.g. pwr up or abort)? */
#endif
         {
            /* If the meter start applet is ready to log in but has to wait for a gate to log in, return pause. */
            if ( ( uint8_t )HMC_APP_API_RPLY_LOGGING_IN == loggedIn_ )
            {
               retVal = update_;
#if !RTOS
               retVal = ( uint8_t )HMC_APP_API_RPLY_PAUSE;
               /* In pause state, reset sys_chk so that meter com doesn't reset the transponder. */
               if ( iscPause_ && !timerExpired_ )
               {

               }
               else
               {
                  iscPause_ = ( bool )false;
               }
#endif
            }
            else /* Not waiting, just idle.  */
            {
               retVal = ( uint8_t )HMC_APP_API_RPLY_IDLE;
            }
         }
#endif
#if ( ANSI_LOGON == 0 )
         retVal = ( uint8_t )HMC_APP_API_RPLY_IDLE;
#endif
         break;
      }
      case HMC_APP_API_CMD_CUSTOM_UPDATE_COM_PARAMS:
      {
         // Copy the new settings into our file-local cache.
         ( void )memcpy( &comSettings_, ( HMC_MSG_Configuration* ) pPtr, sizeof ( comSettings_ ) );
         retVal = ( uint8_t )HMC_APP_API_RPLY_IDLE;
      }
      //lint -fallthrough
      default:
      {
         break;
      }
   }

   if ( ( true == bFirstLogon ) && ( ( uint8_t )HMC_APP_API_RPLY_LOGGED_IN == retVal ) )
   {
      // Signal PWROR_Task that it can talk to the meter.
      PWROR_HMC_signal();
      bFirstLogon = false;
   }

   return ( retVal );
}

#if ( NEGOTIATE_HMC_BAUD == 1)
returnStatus_t   HMC_STRT_GetAltBaud(uint32_t *alt_baud)
{
  returnStatus_t retVal = eSUCCESS;
  if( alt_baud != NULL )
  {
    *alt_baud = altBaudRate_s;
  }
  else
  {
    retVal = eFAILURE;
  }
  return retVal;
}

returnStatus_t  HMC_STRT_SetDefaultBaud(void)
{
   returnStatus_t retVal = eSUCCESS;
   if( MQX_OK != UART_ioctl(UART_HOST_COMM_PORT, IO_IOCTL_SERIAL_SET_BAUD, (void *)&(defaultBaudRate_s)) )
    {

      HMC_START_PRNT_ERROR('E',  "Unable to set DEFAULT baud rate");
      retVal = eFAILURE;
    }
    else
    {
      HMC_START_PRNT_INFO('I', "Set DEFAULT baud rate");
    }
  return retVal;
}
#endif

/***********************************************************************************************************************

   Function name: HMC_STRT_SetPassword

   Purpose: Update meter password with string passed in

   Arguments:  uint8_t *pwd - source location for the password that will be written
               uint16_t numberOfBytes - number of bytes to write from source

   Returns: Success/failure of the update.

 **********************************************************************************************************************/
/*lint -e{715} -e{818}  parameters are not used and/or could be ptr to const. */
returnStatus_t HMC_STRT_SetPassword( uint8_t *pwd, uint16_t numberOfBytes )
{
#if ( ANSI_SECURITY == 1 )
   returnStatus_t retVal = eFAILURE;
   if( sizeof( mtrFileData.password ) < numberOfBytes )
   {
      retVal = eAPP_OVERFLOW_CONDITION;
   }
   else
   {
      if ( eSUCCESS == FIO_fread( &fileHndlStart, ( uint8_t * )&mtrFileData, 0, ( lCnt )sizeof( mtrFileData ) )               &&
         ( eSUCCESS == HMC_STRT_UpdatePasswords() ) )                            /* Decrypt the values from the file.   */
      {
         (void)memset( mtrFileData.password, 0, sizeof( mtrFileData.password ) );
         (void)memcpy( mtrFileData.password, pwd, numberOfBytes );
         retVal = HMC_STRT_UpdatePasswords();                                    /* Encrypt the values and write the file.   */
         if ( eSUCCESS == retVal )
         {
            startTimer( 100 );   /* Password changing; if stuck in loop waiting for ISC delay to complete, cause the delay loop to exit quickly.  */
         }
      }
   }
#else
   ( void )*pwd;
   ( void )numberOfBytes;
   returnStatus_t retVal = eSUCCESS;
#endif
   return retVal;
}

/***********************************************************************************************************************

   Function name: HMC_STRT_SetMasterPassword

   Purpose: Update meter master password with string passed in

   Arguments:  uint8_t *pwd - source location for the password that will be written
               uint16_t numberOfBytes - number of bytes to write from source

   Returns: Success/failure of the update.

 **********************************************************************************************************************/
/*lint -e{715} -e{818}  parameters are not used and/or could be ptr to const. */
returnStatus_t HMC_STRT_SetMasterPassword( uint8_t *pwd, uint16_t numberOfBytes )
{
#if ( ANSI_SECURITY == 1 )
   returnStatus_t retVal = eFAILURE;
   if( sizeof( mtrFileData.masterPassword ) < numberOfBytes )
   {
      retVal = eAPP_OVERFLOW_CONDITION;
   }
   else
   {
      if ( eSUCCESS == FIO_fread( &fileHndlStart, ( uint8_t * )&mtrFileData, 0, ( lCnt )sizeof( mtrFileData ) )               &&
         ( eSUCCESS == HMC_STRT_UpdatePasswords() ) )                            /* Decrypt the values from the file.   */
      {
         ( void )memset( mtrFileData.masterPassword, 0, sizeof( mtrFileData.masterPassword ) );
         ( void )memcpy( mtrFileData.masterPassword, pwd, numberOfBytes );
         retVal = HMC_STRT_UpdatePasswords();                                    /* Encrypt the values and write the file.   */
      }
   }
#else
   ( void )*pwd;
   ( void )numberOfBytes;
   returnStatus_t retVal = eSUCCESS;
#endif
   return retVal;
}

/***********************************************************************************************************************

   Function name: HMC_STRT_GetPassword

   Purpose: Read meter password into string passed in

   Arguments: None

   Returns: Success/failure of the update.

 **********************************************************************************************************************/
/*lint -e{715} -e{818}  parameters are not used and/or could be ptr to const. */
returnStatus_t HMC_STRT_GetPassword( uint8_t *pwd )
{
   returnStatus_t retVal = eFAILURE;
#if ( ANSI_SECURITY == 1 )
   if ( eSUCCESS == FIO_fread( &fileHndlStart, ( uint8_t * )&mtrFileData, 0, ( lCnt )sizeof( mtrFileData ) )               &&
      ( eSUCCESS == HMC_STRT_UpdatePasswords() ) )                            /* Decrypt the values from the file.   */
   {
      (void)memcpy( pwd, mtrFileData.password, sizeof( mtrFileData.password ) );
      retVal = eSUCCESS;
   }
#else
   retVal = eSUCCESS;
#endif
   return retVal;
}

/***********************************************************************************************************************

   Function name: HMC_STRT_GetMasterPassword

   Purpose: Read the meter master password into string passed in

   Arguments: None

   Returns: Success/failure of the update.

 **********************************************************************************************************************/
/*lint -e{715} -e{818}  parameters are not used and/or could be ptr to const. */
returnStatus_t HMC_STRT_GetMasterPassword( uint8_t *pwd )
{
   returnStatus_t retVal = eFAILURE;
#if ( ANSI_SECURITY == 1 )
   if ( eSUCCESS == FIO_fread( &fileHndlStart, ( uint8_t * )&mtrFileData, 0, ( lCnt )sizeof( mtrFileData ) )               &&
      ( eSUCCESS == HMC_STRT_UpdatePasswords() ) )                            /* Decrypt the values from the file.   */
   {
      (void)memcpy( pwd, mtrFileData.masterPassword, sizeof( mtrFileData.masterPassword ) );
      retVal = eSUCCESS;
   }
#else
   retVal = eSUCCESS;
#endif
   return retVal;
}

/***********************************************************************************************************************

   Function name: HMC_STRT_ISC()

   Purpose: Sets time-out for certain meters (Focus), logs off from the meter and
            sets and engineering flag/counter.

   Arguments: None

   Returns: None

 **********************************************************************************************************************/
void HMC_STRT_ISC( void )
{
#if HMC_FOCUS
   iscPause_ = ( bool )true;
#endif
#if RTOS && HMC_FOCUS
   bDoIscDelay = ( bool )true;   /* Set a flag to allow for the log-off command to execute. */
#else
   startTimer( HMC_STRT_ISC_DELAY_mS );
#endif
   HMC_START_PRNT_WARN( 'W',  "Start - ISC Delay. \n\n\nCheck meter password\n\n" );
   ( void )HMC_FINISH_LogOff( ( uint8_t )HMC_APP_API_CMD_ACTIVATE, ( uint8_t far * )0 );
}

/***********************************************************************************************************************

   Function name: HMC_STRT_logInStatus()

   Purpose: Returns the log-on status

   Arguments: None

   Returns: uint8_t - loggedIn_ (log-on status)

 **********************************************************************************************************************/
uint8_t HMC_STRT_logInStatus( void )
{
   return( loggedIn_ );
}

/***********************************************************************************************************************

   Function name: HMC_STRT_loggedOff()

   Purpose: Tells this module that the status is logged-off.

   Arguments: None

   Returns: None

 **********************************************************************************************************************/
void HMC_STRT_loggedOff( void )
{
   loggedIn_ = ( uint8_t )HMC_APP_API_RPLY_LOGGED_OFF;
   update_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
#if ( NEGOTIATE_HMC_COMM == 1)
   curState = eStrtLoggedOff;
#if ( NEGOTIATE_HMC_BAUD == 1)
   HMC_STRT_SetDefaultBaud();
#endif
#endif
}

/***********************************************************************************************************************

   Function name: HMC_STRT_RestartDelay()

   Purpose: Sets time-out for certain meters (Focus), logs off from the meter and
            sets and engineering flag/counter.

   Arguments: None

   Returns: None

 **********************************************************************************************************************/
void HMC_STRT_RestartDelay( void )
{
   startTimer( ( uint32_t )( comSettings_.uiTrafficTimeOut_mS +
                             ( ( uint32_t )comSettings_.uiResponseTimeOut_mS * comSettings_.ucRetries ) + RESTART_DELAY_FUDGE_MS ) );
   HMC_STRT_loggedOff();
   HMC_START_PRNT_INFO('I',"Restart Requested");
}

#if !RTOS
/***********************************************************************************************************************

   Function name: HMC_STRT_SET_COMPORT)

   Purpose: Sets up the comport.

   Arguments: None

   Returns: None

 **********************************************************************************************************************/
static void HMC_STRT_SET_COMPORT( void )
{
   tComPortSettings sComport;

   memset( &sComport, 0, sizeof ( sComport ) );
   sComport.bitRate = 9600;
   sComport.isrTxPri = 1;
   sComport.isrRxPri = 2;
   sComport.sFlags.bRxEnbl = 1;
   sComport.sFlags.bTxEnbl = 1;
   sComport.sFlags.bRxIe = 1;
   ( void ) DVR_UART_App( UART_HMC_PORT, ( uint8_t ) PORT_OPEN, ( void * ) &sComport );
}
#endif

/***********************************************************************************************************************

   Function name: HMC_STRT_ISR()

   Purpose: Decrements the wait_mS_ every 20mS based on a timer.

   Arguments: None

   Returns: None

   Side Effects:  All Timers

 **********************************************************************************************************************/
/*lint -e{715} -e{818}  parameters are not used. */
void HMC_STRT_ISR( uint8_t cmd, void *pData )
{
   (void)pData;
   timerExpired_ = ( bool )true;
}

/***********************************************************************************************************************

   Function name: startTimer()

   Purpose: start a timer based on the value passed in.  If an RTOS is used, the delay will put the task to sleep.

   Arguments: uint32_t tmr_mS

   Returns: None

   Side Effects:  All Timers

 **********************************************************************************************************************/
static void startTimer( uint32_t tmr_mS )
{
   timerExpired_ = ( bool )false;
   ( void )TMR_ResetTimer( timerId_, tmr_mS );
}
