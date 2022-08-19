/*
 * file: ecc108_apps.c
 * \file
 *  \brief  Taken from Application Atmel ecc108_examples.c
 *  \author Atmel Crypto Products
 *  \date   January 29, 2014

* \copyright Copyright (c) 2014 Atmel Corporation. All rights reserved.
*
* \atmel_crypto_device_library_license_start
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* 3. The name of Atmel may not be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* 4. This software may only be redistributed and used in connection with an
*    Atmel integrated circuit.
*
* THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
* EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
* \atmel_crypto_device_library_license_stop
 *
 *   Example functions are given that demonstrate the device. Before performing
 *   the example, you need to configure the ECC108 using ACES and the suitable
 *   personalization file. The personalization file and the guide for
 *   configuring the device are distributed with the library package.
 *   The example functions will check the configuration of the device before
 *   performing the command sequence. If the device is configured incorrectly,
 *   the examples return error.

 *   CAUTION WHEN DEBUGGING: Be aware of the timeout feature of the device. The
 *   device will go to sleep between 0.7 and 1.5 seconds after a Wakeup. When
 *   hitting a break point, this timeout will likely to kick in and the device
 *   has gone to sleep before you continue debugging. Therefore, after you have
 *   examined variables you might have to restart your debug session.

 Adapted for use in Aclara Single RF Network devices (Frodo and Samwise).
*/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "project.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>                       /* Needed for mqx typedefs, etc. */
#include <fio.h>                       /* Needed for mqx version of (void)file i/o  */
#endif
#include <string.h>                    /* needed for memset(), (void)memcpy() */
#include <assert.h>
#include "partitions.h"
#include "partition_cfg.h"             /* Needed for secROM offset   */
#if ( RTOS_SELECTION == MQX_RTOS )
#include "ecc108_mqx.h"                /* Internal flash variables structure  */
#elif ( RTOS_SELECTION == FREE_RTOS )
#include "ecc108_freertos.h"
#endif
#include "ecc108_lib_return_codes.h"   /* declarations of function return codes  */
#include "ecc108_comm_marshaling.h"    /* definitions and declarations for the Command Marshaling module */
#include "dvr_intFlash_cfg.h"          /* Internal flash variables structure  */
#include "ecc108_apps.h"               /* definitions and declarations for example functions */
#include "cert-builder.h"
#include "CRC.h"
#include "pwr_task.h"
#include "wolfssl/wolfcrypt/sha256.h"
#include "DBG_SerialDebug.h"

/* MACRO DEFINITIONS */
#define MAX_CERT_LEN sizeof( NetWorkRootCA_t )  /* make sure this is compatible with dtls.c MAX_CERT_SIZE   */

/*lint -esym(750,ECC108_PRNT_INFO,ECC108_PRNT_HEX_INFO)   */
/*lint -esym(750,ECC108_PRNT_WARN,ECC108_PRNT_HEX_WARN)   */
/*lint -esym(750,ECC108_PRNT_ERROR,ECC108_PRNT_HEX_ERROR)   */
#define ENABLE_PRNT_ECC108_INFO 1
#define ENABLE_PRNT_ECC108_WARN 1
#define ENABLE_PRNT_ECC108_ERROR 1

#if ( ENABLE_PRNT_ECC108_INFO == 1 )
#define ECC108_PRNT_INFO( a, fmt,... )      DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define ECC108_PRNT_HEX_INFO( a, fmt,... )  DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define ECC108_PRNT_INFO( a, fmt,... )
#define ECC108_PRNT_HEX_INFO( a, fmt,... )
#endif

#if ( ENABLE_PRNT_ECC108_WARN == 1 )
#define ECC108_PRNT_WARN( a, fmt,... )      DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define ECC108_PRNT_HEX_WARN( a, fmt,... )  DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define ECC108_PRNT_WARN( a, fmt,... )
#define ECC108_PRNT_HEX_WARN( a, fmt,... )
#endif

#if ( ENABLE_PRNT_ECC108_ERROR == 1 )
#define ECC108_PRNT_ERROR( a, fmt,... )     DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define ECC108_PRNT_HEX_ERROR( a, fmt,... ) DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define ECC108_PRNT_ERROR( a, fmt,... )
#define ECC108_PRNT_HEX_ERROR( a, fmt,... )
#endif

#define ECCX08_SLOT_CONFIG ((uint16_t)20)
#define ECCX08_KEY_CONFIG  ((uint16_t)96)
#define ECCX08_SN0_OFFSET  ((uint16_t)0)
#define ECCX08_SN4_OFFSET  ((uint16_t)8)
#define ECCX08_RETRIES     ((uint8_t)5)

#define VERBOSE 0
#if (VERBOSE != 0)
#define LINE_LENGTH 16
#warning "DO NOT RELEASE with VERBOSE set"   /*lint !e10 !e16 preprocessor directive not recognized */
#endif

#define UPDATE_DEFAULT_KEYS 0    /* Set non-zero to force update from default keys when flashsecurityenabled. */
#if (UPDATE_DEFAULT_KEYS != 0 )
#warning "DO NOT RELEASE with UPDATE_DEFAULT_KEYS set non-zero" /*lint !e10 !e16 warning not recognized   */
#endif

#define DO_CHECK_INFO 0
#define DO_CHECK_MAC 0
#if ( DO_CHECK_MAC != 0 )
#warning "DO NOT RELEASE with DO_CHECK_MAC set non-zero"
#endif

#define INCLUDE_INFO_CMD 0

#define ECC108A   1
#define ECC508A   5
#define ECC_DEVICE_TYPE ECC508A
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
/**
 * \brief Store a certificate and a list of the dynamic elements
 */
PACK_BEGIN
typedef struct PACK_MID
{
    uint8_t       signature[64];          /* Signature portion of compressed cert         */
    uint8_t       enc_date[3];            /* Encoded date portion of compressed cert      */
    uint16_t      signerID;               /* Signer ID                                    */
    uint8_t       templateChainID;        /* 4 bits each for template ID and Chain ID     */
    uint8_t       SNsrcFmtVer;            /* 4 bits each for SN source and Format version */
    uint8_t       certRsvd;               /* Unused/reserved                              */
} CompCertDef;                /* Compressed Certificate Definition   */
PACK_END

/*lint -esym(751, eccx08_CertType, eccx08_CertType::* ) -esym(749, eccx08_CertType, eccx08_CertType::* ) */
typedef enum
{
   SignerCert = 0,   /*lint !e751 !e749 enum not used.   */
   DevCert = 1,      /*lint !e751 !e749 enum not used.   */
   MaxCert           /*lint !e751 !e749 enum not used.   */
} eccx08_CertType;

PACK_BEGIN
/* Certficate contains two CertDefs (signer, dev), and the macID  */
typedef struct PACK_MID
{
   CompCertDef certs[ ( uint8_t )MaxCert ];
   uint8_t     macID[ 16 ];
} Certificate;                /* Slot 8 (CERT) contents   */
PACK_END

/*lint -esym(751,slotInfo) not referenced */
typedef struct
{
   uint16_t       slotNumber;
   uint8_t        slotSize;
   const uint8_t  *slotData;
} slotInfo;

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
#if 0       /* These are only used during initial ecc108 routine development/debugging */
static uint8_t    ecc108e_gen_public_key( void );
static uint8_t    ecc108e_read_configuration(void);
#endif
static uint8_t    ecc108e_InitHostAuthKey( void );
static uint8_t    ecc108e_read_key( uint16_t keyID, uint8_t size, uint8_t block, uint8_t *key );
static uint8_t    ecc108e_write_key( uint16_t keyID, uint8_t size, uint8_t *key, uint8_t block, uint8_t *mac  );
static uint8_t    random( bool seed, uint8_t *rnd );
static uint8_t    ecc108e_read_config_zone(uint8_t *config_data);
static uint8_t    ecc108e_SignMessage( uint8_t keyID,
                                       uint16_t length,
                                       uint8_t const *msg,
                                       bool hash,
                                       uint8_t signature[ VERIFY_256_SIGNATURE_SIZE ] );

/* ****************************************************************************************************************** */
/* CONSTANTS */

static const __no_init intFlashNvMap_t secROM;   /*lint !e728 not explicitly initialized  */

/*lint -esym(765,AclaraFWkey) May not be referenced   */
const uint8_t AclaraFWkey[ ECC108_KEY_SIZE ] =  /* Used to derive FW key   */
{
   0x4D, 0xF5, 0xDD, 0xCC, 0x97, 0x81, 0xD6, 0x7F, 0x08, 0x21, 0xC8, 0x75, 0xC3, 0x67, 0x35, 0x6E,
   0xF4, 0x40, 0xA1, 0x46, 0x67, 0xF3, 0xE3, 0x5B, 0xA4, 0x21, 0x97, 0xA0, 0x0B, 0x4A, 0xDB, 0x70
};

const uint8_t AclaraDFW_sigKey[DFW_SIGKEY_SIZE] =  /* Used to verify patch signatures  */
{
#if ( DFW_TEST_KEY == 1 )
   0x04,
   0x4A, 0xDC, 0xAD, 0x25, 0xF1, 0x04, 0xD8, 0x36, 0xEE, 0x2B, 0x78, 0x6B, 0x83, 0x0A, 0xB7, 0x3C,
   0x7A, 0x56, 0xCD, 0xBF, 0xE6, 0xF5, 0xAF, 0x50, 0x5B, 0x5B, 0x2D, 0xFA, 0x0F, 0x24, 0x72, 0x66,
   0x87, 0x1E, 0x4E, 0x95, 0xBF, 0x6E, 0x98, 0xDA, 0x5D, 0x35, 0x47, 0xF2, 0x06, 0x23, 0x2D, 0x7B,
   0xCD, 0x6E, 0x75, 0x7D, 0xFC, 0x68, 0x4F, 0xB8, 0x3C, 0x2C, 0x85, 0x19, 0x2B, 0x5A, 0xFF, 0xE1
#else
   0x04,
   0x1A, 0x79, 0x82, 0x95, 0xD1, 0x2E, 0x7C, 0xC3, 0x7B, 0xFE, 0xFA, 0xAB, 0x2A, 0x22, 0xA6, 0x49,
   0x51, 0x96, 0x76, 0xB7, 0x03, 0xE4, 0x3D, 0xA2, 0x2D, 0xB8, 0x4A, 0xA3, 0x5E, 0xC9, 0xBB, 0x10,
   0xAF, 0x6F, 0x22, 0x29, 0x71, 0x8E, 0x61, 0x1D, 0xD6, 0x05, 0x39, 0xFD, 0x6E, 0xC4, 0xDE, 0x72,
   0xEF, 0x14, 0xC8, 0x09, 0x8E, 0xAB, 0xD7, 0x5B, 0x05, 0xD0, 0x0B, 0x96, 0x7B, 0x30, 0xB9, 0x63
#endif
};

const uint8_t ROM_dtlsMfgSubject1[87] =
{
   0x30, 0x55, 0x30, 0x53, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55,
   0x53, 0x31, 0x11, 0x30, 0x0F, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0C, 0x08, 0x4D, 0x69, 0x73, 0x73,
   0x6F, 0x75, 0x72, 0x69, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0C, 0x09, 0x48,
   0x61, 0x7A, 0x65, 0x6C, 0x77, 0x6F, 0x6F, 0x64, 0x31, 0x0F, 0x30, 0x0D, 0x06, 0x03, 0x55, 0x04,
   0x0A, 0x0C, 0x06, 0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55,
   0x04, 0x03, 0x0C, 0x03, 0x4D, 0x46, 0x47
};

/*lint -e{651} { 0 } initializers intended   */
const intFlashNvMap_t intROM =
{
   /* Keys 4 - 8   */
   {
      {  /* Private key HOST_AUTH_KEY (4)   Tim Dierking 6/25/15  */
         0x33, 0x4D, 0xE4, 0x75, 0x9A, 0xB2, 0x69, 0xE1, 0x87, 0xFE, 0x60, 0xB2, 0x07, 0xAC, 0xED, 0xD7,
         0x82, 0xD8, 0x81, 0x83, 0xF1, 0xE7, 0xAB, 0xFB, 0x40, 0x13, 0x98, 0xA8, 0x46, 0xE1, 0xB1, 0x08
      },
      {  /* Private key ACLARA_PSK (5)  */
         0xD0, 0x2A, 0x5D, 0x73, 0x15, 0x29, 0x83, 0x61, 0x99, 0x10, 0x89, 0x01, 0xFD, 0xBC, 0x98, 0x21,
         0x82, 0x66, 0xEE, 0x06, 0x22, 0xC9, 0x22, 0x63, 0x34, 0x78, 0x31, 0x37, 0x87, 0x6E, 0x5F, 0x30
      },
      {  /* Private key KEY_UPDATE_WRITE_KEY (6)  */
         0x70, 0xB0, 0xDE, 0xBD, 0x1A, 0x41, 0xC2, 0x5C, 0xD0, 0x9B, 0x9A, 0xE7, 0xA0, 0x73, 0x14, 0xE6,
         0x44, 0xC7, 0x56, 0x7B, 0x5F, 0xA9, 0x86, 0x87, 0x79, 0x29, 0x4D, 0x0A, 0xD0, 0x92, 0x9D, 0x45
      },
      {  /* Private key NETWORK_AUTH_KEY (7)  */
         0xE1, 0xA1, 0x72, 0x1F, 0x0E, 0xBE, 0x47, 0xBD, 0xEC, 0xFA, 0x04, 0x7B, 0xB3, 0xD3, 0xDC, 0x29,
         0xCA, 0x1B, 0xCB, 0xE2, 0x2A, 0xD6, 0xAE, 0x2F, 0x8F, 0x6C, 0x9F, 0x2F, 0x06, 0xC4, 0x44, 0xC2
      },
      {  /* Slot 8 */
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
   },
   {     /* signature keys */
      {  /* Slot 9 */
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {  /* Slot 10 */
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {  /* Slot 11 */
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {  /* Slot 12 */
         0xC6, 0x3D, 0x57, 0x9D, 0xD4, 0xBF, 0xC2, 0xC0, 0x33, 0xDF, 0x0C, 0x98, 0x61, 0x86, 0xE3, 0x13,
         0xD4, 0x6E, 0xBD, 0x57, 0x69, 0x3D, 0x86, 0xC9, 0xE2, 0x2C, 0x6F, 0x09, 0x4B, 0x3B, 0x61, 0x84,
         0x6D, 0x88, 0x83, 0x38, 0xA2, 0xB0, 0x49, 0x60, 0x44, 0xA2, 0x68, 0x1E, 0x2C, 0x5F, 0x8F, 0xB3,
         0x0D, 0x93, 0xC1, 0x23, 0xFF, 0xE6, 0xC7, 0x65, 0x4B, 0x54, 0x42, 0xD7, 0xC2, 0x21, 0xB8, 0x85
#if 0
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
#endif
      },
      {  /* NETWORK_PUB_KEY_SECRET (13) Tim Dierking 6/25/15 */
         0x81, 0xF3, 0xA7, 0xE0, 0x68, 0x3B, 0x91, 0xC8, 0x1D, 0xB1, 0xE0, 0xF8, 0x88, 0x5C, 0xFF, 0x35,
         0x11, 0x3F, 0x91, 0x60, 0xDB, 0xB8, 0x72, 0x10, 0x82, 0x3F, 0x99, 0x69, 0x32, 0xBF, 0x25, 0xA1,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {  /* Slot 14 */
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {  /* PUB_KEY_SECRET (15) Tim Dierking 6/25/15 */
         0x50, 0x0B, 0x15, 0x53, 0x4F, 0x4D, 0x73, 0x3D, 0xE2, 0x03, 0xF4, 0xA6, 0x3C, 0xB7, 0xC1, 0x86,
         0xDD, 0x4E, 0x1E, 0x68, 0x8A, 0x1E, 0x18, 0xFD, 0xE7, 0x17, 0x62, 0xBC, 0x58, 0x89, 0xCA, 0x7A,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      }
   },
   {  /* replKeys */
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
   },
   {
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      },
      {
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      }
   },
   {  /* keyCRC[12]      */
      0xffff, 0xffff, 0xffff, 0xffff,
      0xffff, 0xffff, 0xffff, 0xffff,
      0xffff, 0xffff, 0xffff, 0xffff
   },
   {  /* replKeyCRC[12]  */
      0xffff, 0xffff, 0xffff, 0xffff,
      0xffff, 0xffff, 0xffff, 0xffff,
      0xffff, 0xffff, 0xffff, 0xffff
   },
   {  /* aesKey[16]  */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
   },
   {  /* mac   *//*lint !e708 union initializer OK here */
      0xff, 0xff, 0xff, 0xff,    /*lint !e708 union initializer OK here */
      0xff, 0xff, 0xff, 0xff     /*lint !e708 union initializer OK here */
   },
   0xffff,  /* macChecksum          */
   { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff   }, /* Pad   */
   { 0 },   /* dtlsNetworkRootCA    */
   { 0 },   /* dtlsHESubject        */
   { 0 },   /* dtlsMSSubject        */
   { 0 },   /* dtlsMfgSubject2      */
   { 0 },   /* public key for mtls  */
   { 0 }    /* subject key id for mtls */
};

static const uint8_t slotNumbers[] = { HOST_AUTH_KEY, SIGNER_PUB_KEY };
#if 0    /* These are original prototype slot contents. Only used during initial dev/debugging  */
/* Information used to store the compressed certificate into slot 8 (CERT)  */
static const uint8_t CompCert[] =       /* Signer Public Value, combined with DeviceCert; into slot 8 (CERT)   */
{
   0x4A, 0xB7, 0x7F, 0x30, 0xC0, 0x90, 0x07, 0xBB, 0xF4, 0x65, 0xE6, 0x71, 0xCE, 0x5A, 0xEF, 0x17,
   0x07, 0x73, 0x96, 0x3A, 0xF2, 0x40, 0x82, 0x83, 0x0A, 0x04, 0x37, 0x4E, 0xAB, 0x53, 0xA0, 0x2C,
   0x0E, 0x22, 0x01, 0xC9, 0x91, 0x6E, 0xE5, 0x1E, 0x98, 0xB6, 0xC9, 0x15, 0x65, 0x25, 0x36, 0x76,
   0x32, 0x9E, 0x69, 0xC3, 0xB5, 0xCF, 0x78, 0xB5, 0xD1, 0x57, 0xA1, 0xC6, 0x1E, 0x11, 0xC6, 0xCB,

   0x7a, 0xb6, 0x20,    /* Date  */
   0, 0,                /* SignerID */
   0x10, 0xC0, 0x00,    /* Signer template chain ID, SN src, Format Version, reserved  */

/* Generated 6/4/2015  */
   0x63, 0x18, 0x67, 0x97, 0x60, 0x43, 0xFF, 0xFF, 0xE3, 0x07, 0x85, 0x66, 0xFF, 0xBB, 0x6A, 0x56,
   0xCD, 0x4E, 0x69, 0xE9, 0xD6, 0x15, 0xA6, 0x5D, 0xD0, 0x19, 0x2A, 0xD0, 0x47, 0x98, 0x91, 0x91,
   0xEB, 0x64, 0x4F, 0xB7, 0x46, 0x13, 0x82, 0xB5, 0x94, 0xC6, 0xCB, 0x34, 0xEE, 0xE0, 0x5E, 0x99,
   0xC4, 0x66, 0x40, 0x5A, 0x2C, 0x42, 0x53, 0x1D, 0x7F, 0x60, 0x92, 0x15, 0x2F, 0x6E, 0x3D, 0x02,
   0x7a, 0xd6, 0x00,    /* Date  */
   0, 0,                /* Device ID */
   0x00, 0xC0, 0x00,    /* Device template chain ID, SN src, Format Version, reserved  */
   /* EUI64[] =   */
   /* 0     0     1     D     2     4     0     0     0     0     0     0     0     0    0      3  */
   0x30, 0x30, 0x31, 0x44, 0x32, 0x34, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x33
};

/* keys 9 - 15   */
static const uint8_t publicKeys[7][64] =
{
   {  /* DEV_PUB_KEY (9)  */
      0x45, 0x28, 0x81, 0x56, 0xA3, 0x9F, 0xCA, 0x6B, 0x51, 0x8E, 0x0F, 0x0A, 0x47, 0xFF, 0xA9, 0xF0,
      0x0E, 0x76, 0x47, 0xA2, 0xA5, 0x25, 0xB2, 0x54, 0x93, 0x3D, 0x6D, 0x5F, 0xC6, 0x8D, 0x70, 0xD4,
      0xB6, 0xE5, 0x64, 0x86, 0x08, 0x71, 0x1E, 0x3F, 0xE8, 0x57, 0xF5, 0x51, 0xA1, 0x95, 0xFB, 0xE2,
      0x34, 0x1A, 0x5C, 0xA4, 0xAF, 0xD4, 0xCA, 0xF9, 0xB3, 0x40, 0xDB, 0x69, 0x95, 0xE2, 0xF3, 0x4A
   },
   {  /* SIGNER_PUB_KEY (10)  */
      0x64, 0xFE, 0x2A, 0xF1, 0xE1, 0x48, 0x5B, 0xF4, 0xE6, 0xFD, 0x4A, 0xC4, 0xD7, 0xAB, 0x0F, 0x1C,
      0xC8, 0x98, 0xAF, 0x0C, 0x75, 0xE2, 0x79, 0x42, 0x55, 0x5E, 0x12, 0x03, 0x47, 0xFE, 0x75, 0x5C,
      0x4B, 0x43, 0x4C, 0xAA, 0x34, 0x01, 0x97, 0x0E, 0x94, 0xC2, 0x40, 0xE8, 0xA1, 0xA7, 0x7A, 0x80,
      0x84, 0x26, 0x0B, 0x36, 0x9C, 0xE9, 0xBD, 0xC4, 0x16, 0x60, 0x3A, 0x11, 0x5E, 0xF2, 0x0B, 0xCC
   },
   {  /* KEY_UPDATE_AUTH_KEY (11)  */
      0x06, 0x14, 0xB8, 0x2A, 0x4B, 0x94, 0x89, 0x6A, 0x5C, 0x7F, 0x5A, 0x1A, 0xAB, 0x5C, 0x28, 0xBD,
      0x79, 0x80, 0x69, 0x4D, 0xFD, 0xAF, 0x09, 0x69, 0x92, 0x7D, 0x80, 0xC6, 0x5C, 0xDB, 0x32, 0xBE,
      0x70, 0x94, 0x3C, 0x92, 0x2C, 0xBF, 0x85, 0x52, 0x7C, 0x86, 0xB6, 0x26, 0x06, 0xC8, 0x5D, 0x91,
      0x73, 0x25, 0xA3, 0x66, 0x99, 0x4C, 0x97, 0xD3, 0xF8, 0xAB, 0xC8, 0xD9, 0xA2, 0x25, 0xFC, 0xF2
   },
   {  /* NETWORK_PUB_KEY (12)  */
      0x5F, 0xA0, 0xFF, 0x1E, 0x82, 0xB4, 0x99, 0xC1, 0xA1, 0x55, 0xC1, 0x78, 0xEB, 0x66, 0x19, 0x67,
      0x37, 0x2F, 0x30, 0x15, 0x0A, 0x69, 0x46, 0x61, 0xCC, 0xA9, 0xE2, 0x12, 0xBF, 0x8D, 0x48, 0x87,
      0x9D, 0xEA, 0xA3, 0xAE, 0xF5, 0x31, 0xFA, 0x96, 0x59, 0xEF, 0x6C, 0x70, 0x3E, 0x68, 0x89, 0x0D,
      0xC5, 0x3C, 0xB1, 0x57, 0x32, 0x71, 0x75, 0x24, 0xEE, 0xDC, 0xA6, 0x50, 0x14, 0xF4, 0xDB, 0x62
   },
   {  /* NETWORK_PUB_KEY_SECRET (13) Tim Dierking 6/25/15 */
      0x81, 0xF3, 0xA7, 0xE0, 0x68, 0x3B, 0x91, 0xC8, 0x1D, 0xB1, 0xE0, 0xF8, 0x88, 0x5C, 0xFF, 0x35,
      0x11, 0x3F, 0x91, 0x60, 0xDB, 0xB8, 0x72, 0x10, 0x82, 0x3F, 0x99, 0x69, 0x32, 0xBF, 0x25, 0xA1,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   },
   {  /* PUB_KEY (14)  */
      0x26, 0x3E, 0xDD, 0x98, 0x14, 0xFC, 0x7F, 0xA9, 0xCE, 0x00, 0x41, 0x5F, 0xCB, 0x14, 0xF7, 0xB3,
      0xC7, 0x3B, 0xF7, 0x3B, 0x90, 0x0A, 0x64, 0x94, 0x67, 0x7A, 0xA4, 0xE1, 0x22, 0x19, 0x60, 0xF8,
      0x78, 0x8C, 0x37, 0x64, 0xBF, 0x98, 0xD1, 0xCC, 0xA6, 0x51, 0x0E, 0xC3, 0x44, 0x88, 0x6F, 0xE2,
      0xF9, 0x5E, 0x4B, 0xE7, 0x1F, 0x4F, 0x73, 0x31, 0xB4, 0x2C, 0x3F, 0x03, 0xE5, 0xD6, 0x2F, 0xB2
   },
   {  /* PUB_KEY_SECRET (15) Tim Dierking 6/25/15 */
      0x50, 0x0B, 0x15, 0x53, 0x4F, 0x4D, 0x73, 0x3D, 0xE2, 0x03, 0xF4, 0xA6, 0x3C, 0xB7, 0xC1, 0x86,
      0xDD, 0x4E, 0x1E, 0x68, 0x8A, 0x1E, 0x18, 0xFD, 0xE7, 0x17, 0x62, 0xBC, 0x58, 0x89, 0xCA, 0x7A,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   }
};
#endif

#ifdef TM_DTLS_UNIT_TEST
#warning "Do not release with TM_DTLS_UNIT_TEST set!" /*lint !e10 !e16 unrecognized name, expect EOL  */
/* This is MFG_CA_0000.cer */
const uint8_t dtlsMfgRootCA[] =
{
  0x30, 0x82, 0x01, 0xA4, 0x30, 0x82, 0x01, 0x4A, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x00,
  0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x2B, 0x31, 0x0F,
  0x30, 0x0D, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06, 0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x31,
  0x18, 0x30, 0x16, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0F, 0x55, 0x74, 0x69, 0x6C, 0x69, 0x74,
  0x79, 0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43, 0x41, 0x30, 0x20, 0x17, 0x0D, 0x31, 0x35, 0x30,
  0x39, 0x31, 0x35, 0x31, 0x34, 0x30, 0x30, 0x30, 0x30, 0x5A, 0x18, 0x0F, 0x39, 0x39, 0x39, 0x39,
  0x31, 0x32, 0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35, 0x39, 0x5A, 0x30, 0x2B, 0x31, 0x0F, 0x30,
  0x0D, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06, 0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x31, 0x18,
  0x30, 0x16, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0F, 0x55, 0x74, 0x69, 0x6C, 0x69, 0x74, 0x79,
  0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43, 0x41, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86,
  0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03,
  0x42, 0x00, 0x04, 0xFB, 0xB0, 0xD9, 0xA4, 0x54, 0x14, 0xCE, 0x83, 0x5E, 0xC0, 0xEB, 0x00, 0xE5,
  0x6C, 0x23, 0xF0, 0x62, 0xDA, 0x71, 0xEA, 0xB6, 0xD5, 0x8E, 0xC0, 0xF1, 0xBA, 0xC0, 0xDD, 0x23,
  0xB7, 0xBA, 0xFF, 0x74, 0xDC, 0x4F, 0xB1, 0xDF, 0x8B, 0x83, 0x3D, 0xEC, 0x1D, 0xCB, 0x72, 0x89,
  0xFA, 0x8F, 0xB5, 0xFE, 0xAD, 0x3F, 0x52, 0xC3, 0x84, 0xE1, 0x96, 0xF6, 0x1C, 0xA3, 0xBF, 0xC0,
  0x99, 0x84, 0xBC, 0xA3, 0x5D, 0x30, 0x5B, 0x30, 0x0C, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x05,
  0x30, 0x03, 0x01, 0x01, 0xFF, 0x30, 0x0B, 0x06, 0x03, 0x55, 0x1D, 0x0F, 0x04, 0x04, 0x03, 0x02,
  0x01, 0x06, 0x30, 0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x8E, 0xC3, 0x27,
  0x59, 0x65, 0xD7, 0xF7, 0x3C, 0x77, 0x21, 0x58, 0x94, 0x9F, 0x23, 0x92, 0x18, 0xCA, 0x90, 0xCA,
  0xD9, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x8E, 0xC3,
  0x27, 0x59, 0x65, 0xD7, 0xF7, 0x3C, 0x77, 0x21, 0x58, 0x94, 0x9F, 0x23, 0x92, 0x18, 0xCA, 0x90,
  0xCA, 0xD9, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x03, 0x48,
  0x00, 0x30, 0x45, 0x02, 0x20, 0x3F, 0x4D, 0x91, 0x06, 0x35, 0xE2, 0x58, 0x10, 0x93, 0x3D, 0xBE,
  0xC1, 0xBD, 0xC0, 0xE2, 0x41, 0x92, 0x29, 0x62, 0x60, 0x2A, 0x7C, 0xBF, 0xE9, 0xBA, 0x1A, 0x83,
  0xFF, 0xD3, 0x23, 0xB1, 0x55, 0x02, 0x21, 0x00, 0xBD, 0x2E, 0x67, 0xEC, 0x97, 0x35, 0xC0, 0xC3,
  0x0C, 0x2E, 0x1C, 0x2C, 0x07, 0xF1, 0x87, 0x57, 0x9A, 0x83, 0x33, 0x75, 0xF1, 0xD2, 0x6B, 0x1A,
  0x3D, 0xEB, 0x2E, 0x13, 0x9F, 0x1E, 0x10, 0xB9
};
#else
const uint8_t dtlsMfgRootCA[] =
{
   0x30, 0x82, 0x01, 0xA4, 0x30, 0x82, 0x01, 0x4A, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x00,
   0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x2B, 0x31, 0x0F,
   0x30, 0x0D, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06, 0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x31,
   0x18, 0x30, 0x16, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0F, 0x55, 0x74, 0x69, 0x6C, 0x69, 0x74,
   0x79, 0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43, 0x41, 0x30, 0x20, 0x17, 0x0D, 0x31, 0x35, 0x30,
   0x36, 0x31, 0x30, 0x31, 0x34, 0x30, 0x30, 0x30, 0x30, 0x5A, 0x18, 0x0F, 0x39, 0x39, 0x39, 0x39,
   0x31, 0x32, 0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35, 0x39, 0x5A, 0x30, 0x2B, 0x31, 0x0F, 0x30,
   0x0D, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06, 0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x31, 0x18,
   0x30, 0x16, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0F, 0x55, 0x74, 0x69, 0x6C, 0x69, 0x74, 0x79,
   0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43, 0x41, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86,
   0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03,
   0x42, 0x00, 0x04, 0x6D, 0x6A, 0xB6, 0x34, 0xB5, 0xE4, 0x3A, 0x87, 0x7D, 0xAF, 0xD4, 0xA3, 0x66,
   0xA3, 0x9D, 0x06, 0x49, 0xC2, 0xE1, 0x23, 0x43, 0xC7, 0x8A, 0xF3, 0xE9, 0xB8, 0xE7, 0x09, 0xD8,
   0xD9, 0x64, 0xF9, 0x0F, 0xA9, 0x5D, 0xC6, 0x70, 0x41, 0xDB, 0xF1, 0xCC, 0x76, 0xD4, 0xE3, 0x3A,
   0x18, 0xC2, 0x97, 0x8D, 0xEE, 0xAD, 0x8A, 0xCF, 0x65, 0xA3, 0x31, 0xF3, 0xFA, 0xDD, 0x3D, 0x4A,
   0x86, 0xF5, 0x55, 0xA3, 0x5D, 0x30, 0x5B, 0x30, 0x0C, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x05,
   0x30, 0x03, 0x01, 0x01, 0xFF, 0x30, 0x0B, 0x06, 0x03, 0x55, 0x1D, 0x0F, 0x04, 0x04, 0x03, 0x02,
   0x01, 0x06, 0x30, 0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x1B, 0xC0, 0x46,
   0x4E, 0xFC, 0xFB, 0xE6, 0x86, 0xCF, 0x55, 0x2D, 0xBF, 0x57, 0xC8, 0x74, 0xD2, 0x2A, 0xB7, 0x1C,
   0xE9, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x1B, 0xC0,
   0x46, 0x4E, 0xFC, 0xFB, 0xE6, 0x86, 0xCF, 0x55, 0x2D, 0xBF, 0x57, 0xC8, 0x74, 0xD2, 0x2A, 0xB7,
   0x1C, 0xE9, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x03, 0x48,
   0x00, 0x30, 0x45, 0x02, 0x20, 0x4D, 0xA3, 0x70, 0x41, 0x72, 0x81, 0x03, 0x3D, 0xB1, 0x13, 0x9A,
   0xAF, 0x2D, 0x78, 0x59, 0xF9, 0xB2, 0x91, 0xE4, 0xDE, 0x2C, 0x2D, 0x8F, 0x7D, 0x95, 0x5D, 0xC7,
   0xB4, 0x1E, 0xA8, 0x89, 0x80, 0x02, 0x21, 0x00, 0xD8, 0xC6, 0x22, 0xBD, 0x34, 0x54, 0x84, 0x41,
   0xD4, 0xCC, 0x94, 0x72, 0x22, 0x6B, 0x2C, 0xC5, 0x41, 0xC3, 0xA4, 0x6D, 0x4E, 0xB6, 0xA5, 0x03,
   0x5D, 0xFA, 0x4E, 0x9A, 0x65, 0x29, 0xFD, 0x8C
};
#endif
const uint16_t dtlsMfgRootCASz = sizeof(dtlsMfgRootCA);

static const char AclaraOUI[] = "001D24";

static const uint8_t DeviceRootCAPublicKey[] =
{  /* 6/2915   */
   0x79, 0x01, 0xE3, 0x44, 0x88, 0x4A, 0x5C, 0x74, 0xEB, 0x2E, 0xF7, 0x82, 0x97, 0xA8, 0x75, 0x48,
   0x78, 0x7F, 0x92, 0x21, 0x73, 0x0A, 0x58, 0x99, 0x18, 0xF7, 0xF2, 0x42, 0x85, 0x5A, 0xDE, 0x4F,
   0x57, 0x7B, 0xB1, 0x86, 0x35, 0xDD, 0x4A, 0x8D, 0x66, 0x2F, 0xDE, 0x90, 0x1D, 0xBB, 0xA4, 0xCA,
   0xB7, 0xA9, 0x14, 0xA3, 0x6C, 0xF0, 0x59, 0x59, 0x7C, 0x27, 0x59, 0x67, 0xDB, 0x76, 0x1B, 0x57
};

/* Aclara Root CA - from .der */
static const uint8_t AclaraPublicRootCert[] =
{
   0x30, 0x82, 0x01, 0xA2, 0x30, 0x82, 0x01, 0x48, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x00,
   0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x2A, 0x31, 0x0F,
   0x30, 0x0D, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06, 0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x31,
   0x17, 0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0E, 0x41, 0x63, 0x6C, 0x61, 0x72, 0x61,
   0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43, 0x41, 0x30, 0x20, 0x17, 0x0D, 0x31, 0x35, 0x30, 0x36,
   0x31, 0x30, 0x31, 0x34, 0x30, 0x30, 0x30, 0x30, 0x5A, 0x18, 0x0F, 0x39, 0x39, 0x39, 0x39, 0x31,
   0x32, 0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35, 0x39, 0x5A, 0x30, 0x2A, 0x31, 0x0F, 0x30, 0x0D,
   0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06, 0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x31, 0x17, 0x30,
   0x15, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0E, 0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x20, 0x52,
   0x6F, 0x6F, 0x74, 0x20, 0x43, 0x41, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE,
   0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00,
   0x04, 0x79, 0x01, 0xE3, 0x44, 0x88, 0x4A, 0x5C, 0x74, 0xEB, 0x2E, 0xF7, 0x82, 0x97, 0xA8, 0x75,
   0x48, 0x78, 0x7F, 0x92, 0x21, 0x73, 0x0A, 0x58, 0x99, 0x18, 0xF7, 0xF2, 0x42, 0x85, 0x5A, 0xDE,
   0x4F, 0x57, 0x7B, 0xB1, 0x86, 0x35, 0xDD, 0x4A, 0x8D, 0x66, 0x2F, 0xDE, 0x90, 0x1D, 0xBB, 0xA4,
   0xCA, 0xB7, 0xA9, 0x14, 0xA3, 0x6C, 0xF0, 0x59, 0x59, 0x7C, 0x27, 0x59, 0x67, 0xDB, 0x76, 0x1B,
   0x57, 0xA3, 0x5D, 0x30, 0x5B, 0x30, 0x0C, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x05, 0x30, 0x03,
   0x01, 0x01, 0xFF, 0x30, 0x0B, 0x06, 0x03, 0x55, 0x1D, 0x0F, 0x04, 0x04, 0x03, 0x02, 0x01, 0x06,
   0x30, 0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x40, 0x27, 0x2D, 0xA5, 0x48,
   0xDB, 0xA6, 0xC5, 0x3E, 0x38, 0xD6, 0x21, 0xE2, 0xA2, 0xBF, 0x06, 0x2D, 0xFB, 0x6B, 0xF1, 0x30,
   0x1F, 0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x40, 0x27, 0x2D, 0xA5,
   0x48, 0xDB, 0xA6, 0xC5, 0x3E, 0x38, 0xD6, 0x21, 0xE2, 0xA2, 0xBF, 0x06, 0x2D, 0xFB, 0x6B, 0xF1,
   0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x03, 0x48, 0x00, 0x30,
   0x45, 0x02, 0x21, 0x00, 0xE2, 0xEC, 0xE9, 0x14, 0x6D, 0x27, 0x46, 0x59, 0x25, 0xC5, 0x34, 0x1B,
   0x46, 0x2A, 0x47, 0xE8, 0xB7, 0xC6, 0x10, 0x4D, 0x60, 0x60, 0x80, 0x52, 0x24, 0xE9, 0x41, 0xA6,
   0xC6, 0xE4, 0xF9, 0xB3, 0x02, 0x20, 0x63, 0x05, 0x50, 0x0D, 0x2B, 0xFB, 0x83, 0x4A, 0x1B, 0x66,
   0x52, 0xCC, 0xD9, 0x41, 0xAF, 0x88, 0x88, 0xBD, 0xD0, 0x3E, 0x0F, 0xC1, 0xED, 0xA5, 0x86, 0x3A,
   0xB2, 0x8B, 0xB6, 0x6C, 0x71, 0x82
};
#if 0    /* Only used during initial dev/debugging */
/* Sorted slot data, for calculating device CRC */
static const slotInfo allSlots[] =
{
   {  4, sizeof(intROM.pKeys[0]),   intROM.pKeys[0]  },
   {  5, sizeof(intROM.pKeys[0]),   intROM.pKeys[1]  },
   {  6, sizeof(intROM.pKeys[0]),   intROM.pKeys[2]  },
   {  7, sizeof(intROM.pKeys[0]),   intROM.pKeys[3]  },
   {  8, sizeof(CompCert),          CompCert  },
   {  9, sizeof(publicKeys[0]),     (uint8_t *)publicKeys[  9 - FIRST_STORED_KEY ] },  /*lint !e778 expr evaluates to 0 */
   { 10, sizeof(publicKeys[0]),     (uint8_t *)publicKeys[ 10 - FIRST_STORED_KEY ] },
   { 11, sizeof(publicKeys[0]),     (uint8_t *)publicKeys[ 11 - FIRST_STORED_KEY ] },
   { 12, sizeof(publicKeys[0]),     (uint8_t *)publicKeys[ 12 - FIRST_STORED_KEY ] },
   { 13, sizeof(publicKeys[0]),     (uint8_t *)publicKeys[ 13 - FIRST_STORED_KEY ] },
   { 14, sizeof(publicKeys[0]),     (uint8_t *)publicKeys[ 14 - FIRST_STORED_KEY ] },
   { 15, sizeof(publicKeys[0]),     (uint8_t *)publicKeys[ 15 - FIRST_STORED_KEY ] },
   { 16, 0, NULL }
};
#endif

#if 0
static uint16_t const slotcfg[16] =
{
   0x809F, 0x80D4, 0x2093, 0x6493, 0x4684, 0x8085, 0xB5C4, 0x8087,
   0x8008, 0x8009, 0x800A, 0x800B, 0x440C, 0x448D, 0x440E, 0x448F
};

static const keyConfig keycfg[ 16 ] =
{
   { .privateKey = 1, .pubInfo = 1, .KeyType = 4, .lockable = 1, .reqRandom = 1, .reqAuth = 1, .authKey =  4, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 7, .lockable = 0, .reqRandom = 1, .reqAuth = 1, .authKey =  4, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 1, .pubInfo = 1, .KeyType = 4, .lockable = 0, .reqRandom = 1, .reqAuth = 1, .authKey =  4, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 1, .pubInfo = 1, .KeyType = 4, .lockable = 0, .reqRandom = 1, .reqAuth = 1, .authKey =  4, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 7, .lockable = 0, .reqRandom = 1, .reqAuth = 0, .authKey =  0, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 7, .lockable = 0, .reqRandom = 0, .reqAuth = 1, .authKey = 11, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 7, .lockable = 0, .reqRandom = 1, .reqAuth = 1, .authKey =  4, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 7, .lockable = 0, .reqRandom = 0, .reqAuth = 1, .authKey =  4, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 7, .lockable = 1, .reqRandom = 0, .reqAuth = 0, .authKey =  0, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 4, .lockable = 1, .reqRandom = 0, .reqAuth = 1, .authKey = 10, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 4, .lockable = 1, .reqRandom = 0, .reqAuth = 0, .authKey =  0, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 4, .lockable = 1, .reqRandom = 1, .reqAuth = 0, .authKey =  0, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 4, .lockable = 0, .reqRandom = 0, .reqAuth = 0, .authKey =  0, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 7, .lockable = 0, .reqRandom = 0, .reqAuth = 1, .authKey = 12, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 4, .lockable = 0, .reqRandom = 0, .reqAuth = 0, .authKey =  0, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, },
   { .privateKey = 0, .pubInfo = 0, .KeyType = 7, .lockable = 0, .reqRandom = 0, .reqAuth = 1, .authKey = 14, .IntrusionDisable = 0, .rfu = 0, .X509id = 0, }
};

static uint16_t const keycfg[16] =
{  /* 8/12/15  */
    0x04F3, 0x04DC, 0x04D3, 0x04D3, 0x005C, 0x0B9C, 0x04DC, 0x049C,
    0x003C, 0x0AB0, 0x0030, 0x0070, 0x0010, 0x0C9C, 0x0010, 0x0E9C
};
#endif

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
static uint8_t    LastRandom[ ECC108_KEY_SIZE ];   /* This value is updated whenever random number function is invoked. */
static uint8_t    SerialNumber[ 9 ];               /* Device serial number; used in several apps - keep local copy      */
static uint8_t    RNGseed[ ECC108_KEY_SIZE ];      /* Value to be used as Secure RNG seed */
static keyConfig  keycfg[ 16 ];                    /* key configuration bits read from device   */
static slotConfig slotcfg[ 16 ];                   /* slot configuration bits read from device   */


/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/**************************************************************************************************

   Function: printBuf
   Purpose:  Convenient buffer "dump" utility

   Arguments: char *title - printed before buffer contents
              uint16_t *number - optional (can be NULL) pointer to number printed to describe buffer contents
              uint8_t *data - buffer
              uint16_t len - number of bytes in buffer to print
   Return - none

**************************************************************************************************/

#if (VERBOSE != 0)
static void printBuf( const char *title, const uint16_t *number, const uint8_t *data, const uint16_t len )
{
   char     pBuf[ 89 ];
   uint16_t lcount = 0;
   if ( number != NULL )
   {
      DBG_logPrintf( 'I', "%s: %d", title, *number );
   }
   else
   {
      DBG_logPrintf( 'I', "%s", title );
   }
   /* Print the contents of data.   */
   for ( uint16_t i = 0; i < len; i++ )
   {
      if ( ( i % LINE_LENGTH ) == 0 )
      {
         lcount = (uint16_t)snprintf( pBuf, (int32_t)sizeof( pBuf ), "0x%04X:", i );
      }
      lcount += (uint16_t)snprintf( pBuf + lcount, (int32_t)sizeof( pBuf) - lcount, " 0x%02x", *data++ );
      if ( ( ( ( i + 1 ) % LINE_LENGTH ) == 0 ) || ( i == ( len - 1 ) ) )
      {
         pBuf[ lcount ] = '\0';
         DBG_logPrintf( 'I', pBuf );
      }
   }
   DBG_printfNoCr( "\n" );
}
#endif

/**************************************************************************************************
   Function:   ecc108e_sleep
   Purpose:    Put eccx08 device to sleep

   Arguments: none

   Return - none
**************************************************************************************************/
void ecc108e_sleep(void)
{
   (void)ecc108p_sleep();
}

#if 0
/**************************************************************************************************
   Function: ecc108e_gen_public_key
   Purpose:  Generate a public key. Private key is put in slot 0.

   Arguments: none

   Note: eccx08 device must already be opened.
   Return - uint8_t Success/failure of operation - See ecc108_lib_return_codes.h for codes
**************************************************************************************************/
static uint8_t ecc108e_gen_public_key( void )
{
   uint16_t len;
   uint8_t ret_code;    /* Function return value   */
   uint8_t num_in[ NONCE_NUMIN_SIZE ];       /* 20 byte input number to NONCE command; currently all zeroes */
   uint8_t rsp_key[GENKEY_RSP_SIZE_LONG];    /* Response buffer large enough for generated key  */
   uint8_t response[NONCE_RSP_SIZE_LONG];    /* Response buffer large enough for other responses in this function */
   uint8_t command[WRITE_COUNT_LONG];        /* Command buffer large enough for worst case use in this function   */
   uint8_t keycmd[GENKEY_OTHER_DATA_SIZE];   /* Command buffer for genkey command - 3 bytes of zero for "other" data */

   (void)memset( num_in, 0, sizeof( num_in ) );
   (void)ecc108c_wakeup(response);
   ret_code = ecc108m_execute(ECC108_NONCE, NONCE_MODE_SEED_UPDATE, 0,
                              NONCE_NUMIN_SIZE, num_in,        /* data1 size, data1 */
                              0, NULL, 0, NULL,                /* No data2 or data3 */
                              NONCE_NUMIN_SIZE, command,       /* buffer to hold data1, data2, and data3 */
                              NONCE_RSP_SIZE_LONG, response);
   if ( ret_code == ECC108_SUCCESS )
   {
      (void)memset( keycmd, 0, GENKEY_OTHER_DATA_SIZE );   /* Send 3 bytes of zeroes with this command  */
      do
      {
         ret_code = ecc108m_execute(ECC108_GENKEY, GENKEY_MODE_PRIVATE|GENKEY_MODE_ADD_DIGEST,
                  DEV_PRIV_KEY_0,                  /* Slot 0   */
                  GENKEY_OTHER_DATA_SIZE, keycmd,  /* 3 bytes of zero required   */
                  0, NULL, 0, NULL,                /* No data2 or data3          */
                  GENKEY_COUNT_DATA, command,      /* Normal write + 3 bytes of zero required   */
                  GENKEY_RSP_SIZE_LONG, rsp_key);
      }
      /* Make sure key is at least VERIFY_256_KEY_SIZE (64) bytes long, else retry  */
      while ( ( ret_code != ECC108_SUCCESS ) || ( rsp_key[ 0 ] < VERIFY_256_KEY_SIZE ) );

      /* Now, store the key in slot 9 (DEV_PUB_KEY), bank 0, offset 1  */
      ret_code = ecc108e_write_key( DEV_PUB_KEY, 72, &rsp_key[1], 0, NULL );

      /* Print out the generated key   */
      len = rsp_key[ 0 ] - 3;    /* Length of response in 1st byte; includes length and CRC (3 bytes total).   */
#if (VERBOSE > 1)
      printBuf( "ECC key =", NULL, &rsp_key[ 1 ], len );
#endif
   }
   return ret_code;
}
#endif

/**************************************************************************************************

   Function: ecc108e_read_configuration
   Purpose: Read key and slot configurations

   Arguments: None
   Note: eccx08 device must already be opened.
   Returns: Success/failure of verification or chip access

**************************************************************************************************/
#define PRINT_CONFIG_ZONE 1
static uint8_t ecc108e_read_configuration(void)
{
   volatile uint8_t ret_code;             /* declared as "volatile" for easier debugging  */
   uint16_t config_address;
   uint8_t  command[READ_COUNT];          /* Make the command buffer the size of the Read command. */
   uint8_t  response[READ_32_RSP_SIZE];   /* Make the response buffer the size of the maximum Read response.   */
   uint8_t  i;                            /* Loop counter   */
   uint8_t  block;                        /* Device block address */

   /* Slot configuration data reside in locations 20-51. Since not on a 32 byte boundary, use 4 byte accesses. */
   config_address = 20;
   for( i = 0; i < 32/4; i++ )   /* Read the config zone in 32 byte chunks */
   {
      block = (uint8_t)(config_address / 32);
      ret_code = ecc108m_execute(ECC108_READ, ECC108_ZONE_CONFIG, ((config_address % 32) >> 2) + (block << 3 ),
                                 0, NULL, 0, NULL, 0, NULL,    /* No data 1, data 2 or data 3   */
                                 sizeof(command), command,
                                 sizeof(response), response);
      if ( ret_code == ECC108_SUCCESS )
      {
         (void)memcpy( (uint8_t *)&slotcfg[ i*2 ], &response[ECC108_BUFFER_POS_DATA], 4 );
      }
      config_address += 4;
   }
   /* key configuration data is at location 96, so it can be read in one 32 byte chunk.   */
   ret_code = ecc108m_execute(ECC108_READ, ECC108_ZONE_CONFIG | ECC108_ZONE_COUNT_FLAG, (96/32)<<3,
                              0, NULL, 0, NULL, 0, NULL,    /* No data 1, data 2 or data 3   */
                              sizeof(command), command,
                              sizeof(response), response);
   if ( ret_code == ECC108_SUCCESS )
   {
      (void)memcpy( (uint8_t *)keycfg, &response[ECC108_BUFFER_POS_DATA], ECC108_ZONE_ACCESS_32);
   }
#if PRINT_CONFIG_ZONE
   if ( ret_code == ECC108_SUCCESS )
   {
#if (VERBOSE > 1)
      printBuf( "Slot config data", NULL, (uint8_t *)slotcfg, sizeof( slotcfg ) );
      printBuf( "Key  config data", NULL, (uint8_t *)keycfg, sizeof( keycfg ) );
#endif
   }
#endif
   return ret_code;
}

/**************************************************************************************************

   Function: getKeyData
   Purpose: Return pointer to key data in code space (secROM) for keyID, if a valid key ID. Also returns the key size
            in bytes. Can be used to compute offset into intROM, also.

   Arguments:  uint16_t  keyID - data slot to use
               uint8_t  *keySize - returned size of key
   Note: if keySize is not needed, pass a NULL.

   Return - pointer to secROM, or NULL

**************************************************************************************************/
static uint8_t *getKeyData( uint16_t keyID, uint8_t *keySize )
{
   uint8_t *keyData = NULL;

   if ( ( keyID >= FIRST_STORED_KEY ) && ( keyID <= LAST_STORED_KEY  )  )
   {
      if ( keyID < FIRST_SIGNATURE_KEY )
      {
         keyData = ( uint8_t * )secROM.pKeys[ keyID - FIRST_STORED_KEY ].key; /*lint !e728 secROM is programmatically initialized.  */
         if ( keySize != NULL )
         {
            *keySize = ECC108_KEY_SIZE;
         }
      }
      else
      {
         keyData = ( uint8_t * )secROM.sigKeys[ keyID - FIRST_SIGNATURE_KEY ].key;
         if ( keySize != NULL )
         {
            if ( keycfg[ keyID ].KeyType == 4 )
            {
               *keySize = VERIFY_256_KEY_SIZE;
            }
            else
            {
               *keySize = ECC108_KEY_SIZE;
            }
         }
      }
   }
   return keyData;
}

/**************************************************************************************************

   Function: ecc108e_verify_config
   Purpose: Verify Aclara default configuration is loaded

   Arguments: None
   Note: eccx08 device must already be opened.
   Returns: Success/failure of verification or chip access

**************************************************************************************************/
uint8_t ecc108e_verify_config( void )
{
   uint8_t ret_code;                         /* Function results                 */
   uint8_t config_data[ECC108_CONFIG_SIZE];  /* Contains entire config zone data */

   ret_code = ecc108e_read_config_zone( config_data);
   if ( ret_code == ECC108_SUCCESS )
   {
      /* Verify the slot configuration data  */
      if( memcmp( &config_data[ ECCX08_SLOT_CONFIG ], (uint8_t *)slotcfg, sizeof( slotcfg ) ) != 0 )
      {
#if (VERBOSE > 2)
         DBG_logPrintf( 'E', "Slot configuration verification fails.\n" );
#endif
         ret_code = ECC108_GEN_FAIL;
      }
      /* Verify the key configuration data  */
      else if( memcmp( &config_data[ ECCX08_KEY_CONFIG ], (uint8_t *)keycfg, sizeof( keycfg ) ) != 0 )
      {
#if (VERBOSE > 2)
         DBG_logPrintf( 'E', "Key configuration verification fails.\n" );
#endif
         ret_code = ECC108_GEN_FAIL;
      }
   }

   return ret_code;
}

/**************************************************************************************************

   Function: keyIsBlank
   Purpose:  - Validate a key in secROM ( verify CRC ).
   Examples:

   Arguments: uint16_t keyID
   Return - true if key is invalid, false otherwise

**************************************************************************************************/
static bool keyIsBlank( uint16_t keyID )
{
   buffer_t                *key;          /* Holder for key data     */
   uint8_t         const   *defaultKey;   /* Pointer to intROM key   */
   PartitionData_t const   *pPart;        /* Used to access the security info partition      */
   dSize                   keyOffset;     /* Partition offset        */
   uint16_t                calcCRC;       /* Calculated CRC of key   */
   uint16_t                romCRC;        /* CRC of key in ROM       */
   uint8_t                 keySize;       /* Length of key in bytes  */
   bool                    retVal = (bool)false;

   pPart = SEC_GetSecPartHandle();                    /* Obtain handle to access internal NV variables   */
   defaultKey = getKeyData( keyID, &keySize );        /* Get key in secROM */
   if ( defaultKey != NULL )
   {
      key = BM_alloc( keySize );
      keyOffset = (dSize)( defaultKey - secROM.pKeys[ 0 ].key );

      if ( key != NULL )   /* Got buffer; proceed. */
      {
         if ( ( eSUCCESS == PAR_partitionFptr.parRead( key->data, keyOffset, keySize, pPart ) ) )
         {
            /* Compute CRC on key (secROM)  */
            CRC_ecc108_crc(   keySize,                /* Number of bytes   */
                              key->data,              /* Source data       */
                              (uint8_t *)&calcCRC,    /* Resulting crc     */
                              0,                      /* Seed value        */
                              (bool)false );          /* Don't hold the CRC engine  */
            /* Now read the stored CRC to compare to the calculated CRC.   */
            /* If CRC doesn't match, key in secROM is "blank"   */
            keyOffset = (dSize)( offsetof( intFlashNvMap_t, keyCRC ) + (uint16_t)( keyID - FIRST_STORED_KEY ) * sizeof( romCRC ) );/*lint !e737 loss of sign   */
            if ( ( eSUCCESS == PAR_partitionFptr.parRead( ( uint8_t *)&romCRC, keyOffset, sizeof( romCRC ), pPart ) ) )
            {
               retVal = ( romCRC != calcCRC );
            }
         }
         BM_free( key );
      }
   }
   return retVal;
}

/**************************************************************************************************

   Function: ecc108e_InitKeys
   Purpose:  - For each key in secROM - check for blank. If blank, copy intROM.pKeys to secROM.pKeys
               Also, read key config and slot config info.
               Also, checks/creates key used for host password(s)

   Arguments: none
   Return - none

**************************************************************************************************/
void ecc108e_InitKeys( void )
{
#if ( EP == 1 )
   uint8_t                 hpk[ECC108_KEY_SIZE];   /* Host password key */
#endif
   PartitionData_t const   *pPart;                 /* Used to access the security info partition      */
   uint8_t                 *key;                   /* Pointer to key data in secROM.   */
   dSize                   keyOffset;              /* Offset of key into secROM. */
   uint16_t                keyID;                  /* Used to step through the keys in secROM   */
   uint16_t                keyCRC;                 /* crc of each key                           */
   uint8_t                 keySize;
   returnStatus_t          retVal;
#if ( EP == 1 )
#if ( MCU_SELECTED == RA6E1)
   uint16_t                hostPswdKeyCRC;
   uint16_t                romHostPswdKeyCRC;
#endif
   bool                    hpkValid = (bool)false; /* hostPasswordKey is valid   */
#endif

   (void)ecc108e_read_configuration();                /* Read slot and key configurations */
   pPart = SEC_GetSecPartHandle();              /* Obtain handle to access internal NV variables   */
   /* Step through each of the keys in intROM   */
   for ( keyID = FIRST_STORED_KEY; keyID <= LAST_STORED_KEY; keyID++ )
   {
      if ( keyIsBlank( keyID ) )
      {
         key = getKeyData( keyID, &keySize );
         if ( key != NULL )
         {
            keyOffset = (dSize)( key - secROM.pKeys[ 0 ].key );

            /* Compute CRC on factory default key (intROM)  */
            CRC_ecc108_crc(   keySize,                                        /* Number of bytes   */
                              (uint8_t *)(intROM.pKeys[ 0 ].key + keyOffset), /* Source data       */
                              (uint8_t *)&keyCRC,                             /* Resulting crc     */
                              0,                                              /* Seed value        */
                              (bool)false );                                  /* Don't hold the CRC engine  */
            do
            {
#if ( EP == 1 )
               PWR_lockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails */
#else
               PWR_lockMutex(); /* Function will not return if it fails */
#endif

               /* Attempt to write the new key */
               retVal =  PAR_partitionFptr.parWrite(
                                    keyOffset,
                                    intROM.pKeys[ 0 ].key + keyOffset, (lCnt)keySize, pPart );
               if ( retVal == eSUCCESS )
               {
                  /* Key updated. Now, set the key's crc.  */
                  retVal = PAR_partitionFptr.parWrite( (dSize)offsetof( intFlashNvMap_t, keyCRC ) +
                                                      ((uint16_t)(keyID - FIRST_STORED_KEY) * sizeof( keyCRC ) ),
                                                      (uint8_t *)&keyCRC, (lCnt)sizeof( keyCRC ), pPart );
               }
#if ( EP == 1 )
               PWR_unlockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails  */
#else
               PWR_unlockMutex(); /* Function will not return if it fails  */
#endif
            } while ( retVal != eSUCCESS );
         }
      }
   }
#if ( EP == 1 )
   /* Now check the host password key and generate, if necessary. */
   key = (uint8_t *)secROM.hostPasswordKey.key;
#if ( MCU_SELECTED == NXP_K24 )
   for ( keyID = 0; keyID < sizeof( intROM.hostPasswordKey.key ); keyID++ )
   {
      if ( *( key + keyID ) != 0xff )  /* Found non-blank byte in key; key is considered valid. */
      {
         hpkValid = (bool)true;
      }
   }
#elif ( MCU_SELECTED == RA6E1 )
   keySize = sizeof( secROM.hostPasswordKey.key );
   buffer_t *hostPasswordKey = BM_alloc( keySize );
   if ( ( eSUCCESS == PAR_partitionFptr.parRead( hostPasswordKey->data, (dSize)( offsetof( intFlashNvMap_t, hostPasswordKey.key ) ),
                                                 keySize, pPart ) ) )
   {
      /* Compute CRC on host password key (secROM) */
      CRC_ecc108_crc( keySize,                    /* Number of bytes   */
                      hostPasswordKey->data,      /* Source data       */
                      (uint8_t *)&hostPswdKeyCRC, /* Resulting crc     */
                      0,                          /* Seed value        */
                      (bool)false );              /* Don't hold the CRC engine  */
      /* Now read the stored CRC to compare to the calculated CRC.   */
      /* If CRC doesn't match, host password key in secROM has to be updated */
      if ( ( eSUCCESS == PAR_partitionFptr.parRead( ( uint8_t *)&romHostPswdKeyCRC, (dSize)( offsetof( intFlashNvMap_t, hostPasswordKeyCRC ) ),
                                                    sizeof( romHostPswdKeyCRC ), pPart ) ) )
      {
        if( romHostPswdKeyCRC == hostPswdKeyCRC )
        {
           hpkValid = ( bool ) true;
        }
      }
   }

   BM_free( hostPasswordKey );
#endif

   if ( !hpkValid )
   {  /* The hostPasswordKey is invalid; generate a new one.   */
      (void)random( (bool)false, hpk );
      PWR_lockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails */
      do
      {
         retVal =  PAR_partitionFptr.parWrite( (dSize)offsetof( intFlashNvMap_t, hostPasswordKey.key ), hpk, (lCnt)sizeof( hpk ), pPart );
      } while ( retVal != eSUCCESS );
#if ( MCU_SELECTED == RA6E1 )
      CRC_ecc108_crc( sizeof( hpk ),              /* Number of bytes   */
                      hpk,                        /* Source data       */
                      (uint8_t *)&hostPswdKeyCRC, /* Resulting crc     */
                      0,                          /* Seed value        */
                      (bool)false );              /* Don't hold the CRC engine  */
      do
      {
         retVal =  PAR_partitionFptr.parWrite( (dSize)offsetof( intFlashNvMap_t, hostPasswordKeyCRC ), ( uint8_t *)&hostPswdKeyCRC, (lCnt)sizeof( hostPswdKeyCRC ), pPart );
      } while ( retVal != eSUCCESS );
#endif
      DBG_logPrintf( 'I', "New host password key and its CRC has been written into offset of %u and %u respectively\n",
                     (dSize)offsetof( intFlashNvMap_t, hostPasswordKey.key ), (dSize)offsetof( intFlashNvMap_t, hostPasswordKeyCRC ) );
      PWR_unlockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails  */
   }
#endif
}

/**************************************************************************************************

   Function: ecc108e_UpdateKeys
   Purpose:  - Validate/Update Keys
            Check Host Authorization replacement key crc (replPrivKeys, replKeyCRC). If not valid, no action; otherwise:
               1) Copy Host Authorization replacement key (replPrivKeys) to Host Authorization key (pKeys)
               2) Set HOST_AUTH_KEY crc (keyCRC)
               3) Invalidate the Check Host Authorization replacement key crc.(replKeyCRC)
            For other keys (5-15)
               1) If CRC of secROM.replKeys[ keyID ] correct
                  a)   copy replacement key to secROM.pKeys, invalidate replKeys[ keyID ]
                  Else
                  b)   If secROM.keyCRC[ keyID ] incorrect copy intROM.pKeys[ keyID ] to secROM.pKeys[ keyID ]

   Arguments: pointer to internal (factory default keys)
   Return - none - MUST succeed or security device is inaccessible from now on!

**************************************************************************************************/
void ecc108e_UpdateKeys( const intFlashNvMap_t *intKeys )
{
   PartitionData_t const   *pPart;    /* Used to access the security info partition      */
   returnStatus_t          retVal = eSUCCESS;
   uint8_t                 key[ VERIFY_256_KEY_SIZE ];
   uint16_t                replCRC;
   uint16_t                keyID;
   uint16_t                crc;
   uint8_t                 keySize;

   pPart = SEC_GetSecPartHandle();                          /* Obtain handle to access internal NV variables.  */
   /* Update NV with newHostAuthKey */
   /* Compute crc on key and update NV to validate "new key" */
   do
   {
      if ( ( eSUCCESS == PAR_partitionFptr.parRead( key, (dSize)offsetof( intFlashNvMap_t, replPrivKeys ),
                                                   (lCnt)ECC108_KEY_SIZE, pPart ) )
         &&
         ( eSUCCESS == PAR_partitionFptr.parRead( (uint8_t *)&replCRC, (dSize)offsetof( intFlashNvMap_t, replKeyCRC ),
                                                   (lCnt)sizeof( replCRC ), pPart ) ))
      {
         /* Compute CRC on proposed replacement key   */
         CRC_ecc108_crc( ECC108_KEY_SIZE,       /* Number of bytes   */
                           key,                 /* Source data       */
                           (uint8_t *)&crc,     /* Resulting crc     */
                           0,                   /* Seed value        */
                           (bool)false );       /* Don't hold the CRC engine  */
         if ( replCRC == crc )   /* Need to update Host Authorization Key.  */
         {
#if ( EP == 1 )
            PWR_lockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails */
#else
            PWR_lockMutex(); /* Function will not return if it fails  */
#endif
            /* Attempt to write the HostAuthKey */
            /*lint -e{778} expr evals to 0 */
            retVal =  PAR_partitionFptr.parWrite(
                                 (dSize)offsetof( intFlashNvMap_t, pKeys[ HOST_AUTH_KEY - FIRST_STORED_KEY ] ),
                                 key, ECC108_KEY_SIZE, pPart );
            if ( retVal == eSUCCESS )
            {
               /* Update HOST_AUTH_KEY crc   */
               retVal = PAR_partitionFptr.parWrite( (dSize)offsetof( intFlashNvMap_t, keyCRC ),
                                                      (uint8_t *)&crc, (lCnt)sizeof( crc ), pPart );

               if ( retVal == eSUCCESS )
               {
                  /* Host Authorization key updated. Now, invalidate the Replacement key CRC.  */
                  crc ^= 0xffff;
                  retVal = PAR_partitionFptr.parWrite( (dSize)offsetof( intFlashNvMap_t, replKeyCRC ),
                                                      (uint8_t *)&crc, (lCnt)sizeof( crc ), pPart );
                  crc ^= 0xffff; /* Restore crc in case of write failure   */
               }
            }
#if ( EP == 1 )
            PWR_unlockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails  */
#else
            PWR_unlockMutex(); /* Function will not return if it fails  */
#endif
         }
      }
   } while ( retVal != eSUCCESS );
   /* Now deal with the other keys ( 5 - 15 )  */
   for ( keyID = ACLARA_PSK; keyID <= LAST_STORED_KEY; keyID++ )
   {
      dSize    keyOffset;
      uint8_t  *rkey;

      rkey = getKeyData( keyID, &keySize );  /* Pointer into secROM.pKeys  *//*lint !e613 rkey is not NULL in this loop. */

      /* Find offset from pKeys to replPrivKeys.   */
      keyOffset = (dSize)( offsetof( intFlashNvMap_t, replPrivKeys ) - offsetof( intFlashNvMap_t, pKeys ) );
      keyOffset += (dSize)(rkey - secROM.pKeys[ 0 ].key);  /* Offset into secROM.replPrivKeys[ keyID ]  *//*lint !e613 rkey is not NULL in this loop. */

      /* Check the "replacement" key   */
      (void)PAR_partitionFptr.parRead( key, (dSize)keyOffset, (lCnt)keySize, pPart );
      (void)PAR_partitionFptr.parRead( (uint8_t *)&replCRC,
                                       (dSize)offsetof( intFlashNvMap_t, replKeyCRC ) +
                                       ((uint16_t)(keyID - FIRST_STORED_KEY) * sizeof( replCRC ) ),
                                       (lCnt)sizeof( replCRC ),
                                       pPart );
      CRC_ecc108_crc(   keySize,          /* Number of bytes   */
                        key,              /* Source data       */
                        (uint8_t *)&crc,  /* Resulting crc     */
                        0,                /* Seed value        */
                        (bool)false );    /* Don't hold the CRC engine  */
      if( crc == replCRC ) /* secROM.replKeys[ keyID ] valid, copy replacement key   */
      {
         do
         {
#if ( EP == 1 )
            PWR_lockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails */
#else
            PWR_lockMutex(); /* Function will not return if it fails  */
#endif
            /* Attempt to write the Key. rkey has absolute address. Compute offset from start of secROM. */
            retVal =  PAR_partitionFptr.parWrite( (dSize)( rkey - secROM.pKeys[ 0 ].key ), key, (lCnt)keySize, pPart );/*lint !e613 rkey is not NULL in this loop. */
            if ( retVal == eSUCCESS )
            {
               /* Key updated. Now, the key's crc.  */
               retVal = PAR_partitionFptr.parWrite( (dSize)offsetof( intFlashNvMap_t, keyCRC ) + ( (uint16_t)( keyID - FIRST_STORED_KEY ) * sizeof( crc ) ),
                                                   (uint8_t *)&crc, (lCnt)sizeof( crc ), pPart );
               if ( retVal == eSUCCESS )
               {  /* Now, invalidate the replacement key crc   */
                  replCRC ^= 0xffff;
                  retVal = PAR_partitionFptr.parWrite( (dSize)offsetof( intFlashNvMap_t, replKeyCRC ) + ( (uint16_t)( keyID - FIRST_STORED_KEY ) * sizeof( crc ) ),
                                                      (uint8_t *)&replCRC, (lCnt)sizeof( replCRC ), pPart );
                  replCRC ^= 0xffff;   /* Restore to valid value in case write fails and loop repeats.   */
               }
            }
#if ( EP == 1 )
            PWR_unlockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails  */
#else
            PWR_unlockMutex(); /* Function will not return if it fails  */
#endif
         } while ( retVal != eSUCCESS );
      }
      else  /* replKey[ keyID ] not valid, check the existing pKeys[ keyID ]  */
      {
         /* Need to read from secROM.pKeys   */
         keyOffset = (dSize)( rkey - (uint8_t*)&secROM ); /* keyOffset is offset of secROM.pKeys[ keyID ] *//*lint !e613 rkey is not NULL in this loop. */
         (void)PAR_partitionFptr.parRead( key, (dSize)keyOffset, (lCnt)keySize, pPart );
         (void)PAR_partitionFptr.parRead( (uint8_t *)&replCRC,
                                          (dSize)offsetof( intFlashNvMap_t, keyCRC ) +
                                          ((uint16_t)(keyID - FIRST_STORED_KEY) * sizeof( replCRC ) ),
                                          (lCnt)sizeof( replCRC ),
                                          pPart );
         CRC_ecc108_crc(   keySize,          /* Number of bytes   */
                           key,              /* Source data       */
                           (uint8_t *)&crc,  /* Resulting crc     */
                           0,                /* Seed value        */
                           (bool)false );    /* Don't hold the CRC engine  */
         if( crc != replCRC ) /* secROM.pKeys[ keyID ] not valid, copy intROM.pKeys[ keyID ] to secROM.pKeys[ keyID ] */
         {
            /* Compute CRC on factory default key  */
            CRC_ecc108_crc(   keySize,                         /* Number of bytes   */
                              (uint8_t *)intKeys + keyOffset,  /* Source data       */
                              (uint8_t *)&crc,                 /* Resulting crc     */
                              0,                               /* Seed value        */
                              (bool)false );                   /* Don't hold the CRC engine  */
            do
            {
#if ( EP == 1 )
               PWR_lockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails */
#else
               PWR_lockMutex(); /* Function will not return if it fails  */
#endif
               /* Attempt to write the new key */
               retVal =  PAR_partitionFptr.parWrite( (dSize)keyOffset,                 /* Partition offset  */
                                                      (uint8_t *)intKeys + keyOffset,  /* Source data       */
                                                      (lCnt)keySize,                   /* Number of bytes   */
                                                      pPart );                         /* Partition handle  */
               if ( retVal == eSUCCESS )
               {
                  /* Key updated. Now, set the key's crc.  */
                  retVal = PAR_partitionFptr.parWrite( (dSize)offsetof( intFlashNvMap_t, keyCRC ) +
                                                      ((uint16_t)(keyID - FIRST_STORED_KEY) * sizeof( crc ) ),
                                                      (uint8_t *)&crc, (lCnt)sizeof( crc ), pPart );
               }
#if ( EP == 1 )
               PWR_unlockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails  */
#else
               PWR_unlockMutex(); /* Function will not return if it fails  */
#endif
            } while ( retVal != eSUCCESS );
         }
      }
   }
}

/**************************************************************************************************
   Function: ecc108e_write_key
   Purpose:  Write a key.

   Arguments: keyID - eccx08 slot number
              size - number of bytes to write
              key - pointer to key data
              block - offset into slot
              mac - optional pointer to mac data

   Note: keys are 72 bytes - 4 bytes of pad, X, 4 bytes of pad, Y.
         blocks are 32 bytes, so write 32 bytes, 32 bytes, 32 bytes because the extra bytes are ignored, per the data
         sheet.

   Note: eccx08 device must already be opened.
   Return - uint8_t Success/failure of operation - See ecc108_lib_return_codes.h for codes
**************************************************************************************************/
static uint8_t ecc108e_write_key( uint16_t keyID, uint8_t size, uint8_t *key, uint8_t block, uint8_t *mac )
{
   uint16_t ecc_offset;                      /* Used to build eccx08 offset from slot  */
   uint8_t command[WRITE_COUNT_LONG_MAC];    /* Command buffer    */
   uint8_t response[WRITE_RSP_SIZE];         /* Response buffer   */
   uint8_t ret_code;                         /* Function return value   */

   ecc_offset = (uint16_t)(keyID << 3);      /* Slot number indicated in 3-6  */
   ecc_offset += block << 8;
   assert ( ( key != NULL ) &&
          ( (keyID <  8 ) && ( size <=  36 ) ) ||
          ( (keyID >  8 ) && ( size <=  72 ) ) ||
          ( (keyID == 8 ) && ( size <= 160 ) ) ) ;

   do
   {
      ret_code = ecc108m_execute(ECC108_WRITE, ECC108_ZONE_DATA|ECC108_ZONE_COUNT_FLAG, ecc_offset,
                                       ECC108_KEY_SIZE, key,                     /* data1 len, data 1 */
                                       (( mac == NULL ) ? 0 : WRITE_MAC_SIZE ),  /* Data 2 len        */
                                       mac,                                      /* Data 2            */
                                       0, NULL,                                  /* No data 3         */
                                       (( mac == NULL ) ? WRITE_COUNT_LONG : WRITE_COUNT_LONG_MAC ),
                                       command,     /* tx len, tx buffer    */
                                       WRITE_RSP_SIZE, response);

      size -= min( size, ECC108_KEY_SIZE );
      ecc_offset += 1 << 8;
   } while ( ( ret_code == ECC108_SUCCESS ) && ( size != 0 ) );
   if ( ret_code == ECC108_SUCCESS )   /* Operation worked, check value returned */
   {
      if( response[ 1 ] != 0 )
      {
         ret_code = ECC108_GEN_FAIL;
      }
   }

   return ret_code;
}

/**************************************************************************************************
   Function: ecc108e_WriteKey
   Purpose:  External API to Write a key ( encrypted ).

   Arguments: keyID - eccx08 slot number
              keyLen - number of bytes to write
              keydata - pointer to key data

   Note: for keys of length greater than 32 bytes, the data must be sent to the device as:
      32 bytes of X
      4  bytes of pad
      32 bytes of Y
      4  bytes of pad.

      Since 32 bytes must be written per write command, the data must be split into:
      1st 32 bytes -> 4 bytes of pad plus 28 MS bytes of X
      2nd 32 bytes -> 4 LS bytes of X plus 4 bytes of pad plus 24 MS bytes of Y
      3rd 32 bytes -> 8 LS bytes of Y plus 24 bytes of "don't care (0)"

   Note: eccx08 device must NOT be opened.
   Return - uint8_t Success/failure of operation - See ecc108_lib_return_codes.h for codes
**************************************************************************************************/
uint8_t ecc108e_WriteKey( uint16_t keyID, uint8_t keyLen, const uint8_t *keyData )
{
   buffer_t *keyBuf;             /* Memory for entire key.  */
   uint8_t  *paddedKey;          /* Pointer to data portion of keyBuf   */
   uint8_t  ret_code;            /* Return value.  */

   ret_code = ECC108_GEN_FAIL;   /* Assume failure.   */

   keyBuf = BM_alloc( 96 );      /*  Assume longest possible key (72, but smallest multiple of 32).   */
   if ( keyBuf != NULL )
   {
      paddedKey = keyBuf->data;
      ( void )memset( paddedKey, 0, 96 );                      /* Set all pad bytes to 0. */

      /* Separate keys into pad plus key ( X and Y, as necessary ). */
      if ( keyLen <= 32 )
      {
         ( void )memcpy( paddedKey, keyData, 32 );  /* Copy 32 bytes of key to paddedKey.  */
      }
      else
      {
         ( void )memcpy( paddedKey +  0 + 4, keyData +  0, 32 );  /* Copy 32 bytes of X to offset  4. */
         ( void )memcpy( paddedKey + 32 + 8, keyData + 32, 32 );  /* Copy 32 bytes of Y to offset 40. */
         keyLen = VERIFY_283_KEY_SIZE;                            /* Force writing of all 72 bytes in slot. */
      }

      /* Attempt this a few times as the device may time out.  */
      for ( uint8_t i = 0; i < ECCX08_RETRIES; i++ )
      {
         ret_code = ecc108e_EncryptedKeyWrite( keyID, paddedKey, keyLen );
         if ( ECC108_SUCCESS == ret_code )
         {
            break;
         }
      }
      BM_free( keyBuf );
   }

   return ret_code;
}

/**************************************************************************************************
   Function: ecc108e_GetSerialNumber
   Purpose:  Read the device serial number for use by many apps

   Arguments: None
   Note: eccx08 device must already be opened.
   Return - success/failure of operation

   Side effect: Updates local (static) buffer - SerialNumber
**************************************************************************************************/
uint8_t ecc108e_GetSerialNumber(void)
{
   uint8_t command[READ_COUNT];        /* command buffer */
   uint8_t response[READ_32_RSP_SIZE]; /* response buffer   */
   uint8_t ret_code;

   (void)ecc108c_wakeup( response );
   (void)memset(response, 0, sizeof(response));
   /* Read 1st 32 bytes of config zone. 1st 4 bytes have SN[0-3]; bytes 4-7 are rev; 8-12 are SN[4-8] */
   ret_code = ecc108m_execute(ECC108_READ, ECC108_ZONE_CONFIG|ECC108_ZONE_COUNT_FLAG, ECCX08_SN0_OFFSET,
                              0, NULL, 0, NULL, 0, NULL,
                              sizeof(command), command,
                              sizeof(response), response);
   if (ret_code == ECC108_SUCCESS)
   {
      (void)memcpy( &SerialNumber[0], &response[1], 4 );                   /* First 4 bytes of serial number   */
      (void)memcpy( &SerialNumber[4], &response[1+ECCX08_SN4_OFFSET], 5 ); /* Last  5 bytes of serial number   */
   }
   return ret_code;
}

/**************************************************************************************************
   Function: random
   Purpose:  Request a random number from the eccx08 device

   Arguments: bool   seed     - true -> update seed
              uint8_t *rnd - destination of random number generated

   Return - uint8_t Success/failure of operation - See ecc108_lib_return_codes.h for codes
   Note: eccx08 device must already be opened.

   Side effects: Updates static LastRandom[]
**************************************************************************************************/
static uint8_t random( bool seed, uint8_t rnd[ ECC108_KEY_SIZE ] )
{
   uint8_t ret_code;
   uint8_t command[RANDOM_COUNT]; /* Make the command buffer the size of the Verify command. */
   uint8_t response_random[RANDOM_RSP_SIZE];   /* Random response buffer   */
   uint8_t response[READ_4_RSP_SIZE];

   (void)memset( response, 0, sizeof( response ) );
   ret_code = ecc108c_wakeup(response);
   if ( ret_code == ECC108_SUCCESS )
   {
      ret_code = ecc108m_execute(ECC108_RANDOM, (seed ? NONCE_MODE_SEED_UPDATE : NONCE_MODE_NO_SEED_UPDATE), 0,
                                 0, NULL, 0, NULL, 0, NULL,
                                 sizeof(command), command,
                                 sizeof(response_random), response_random);

      if ( ret_code == ECC108_SUCCESS )
      {
         if ( rnd != NULL )
         {
            (void)memcpy( LastRandom, &response_random[1], sizeof( LastRandom ) );
            (void)memcpy( rnd, &response_random[1], ECC108_KEY_SIZE );
         }
#if (VERBOSE > 2)
         printBuf( "Random number =", NULL, LastRandom, sizeof( LastRandom ) );
#endif
      }
   }
   return ret_code;
}

/**************************************************************************************************
   Function: ecc108e_read_mac
   Purpose:  Read ECCx08 chip slot 8 (CERT) to obtain 16 byte (ASCII) MAC address, including Alara OUI.
             Converts from ASCII to hex. Verifies 1st 3 bytes match Aclara OUI.

   Arguments: *mac_id - location to store MAC address.
   Note: eccx08 device must already be opened.

   Return - uint8_t Success/failure of operation - See ecc108_lib_return_codes.h for codes
**************************************************************************************************/
uint8_t ecc108e_read_mac( uint8_t *mac_id )
{
   uint8_t command[READ_COUNT];           /* ECCx08 command buffer      */
   uint8_t response[READ_32_RSP_SIZE];    /* ECCx08 response buffer     */
   uint16_t offset;                       /* ECCx08 address calculation */
   uint8_t i;                             /* Loop counter               */
   uint8_t ret_code;                      /* Function results           */

   /* MAC address follows two CompCertDefs in slot 8 (CERT), so offset is 144  */
   /* Blocks are 32 bytes long and are indicated in bits 8-11, slot 8 ONLY */
   offset = (( offsetof( Certificate, macID ) / 32 ) & 0xf ) << 8;
   offset |= CERT << 3;                /* Slot number 8 (CERT); indicated in 3-6  */
   offset &= ~7;                       /* Clear 3 lsbs   */
   /*lint -e{778} Info 778: Constant expression evaluates to 0 in operation '&' */
   offset |= ( offsetof( Certificate, macID ) % 32 ) & 7;   /* Word offset    */
   /* Read this 32 byte string to slot 8 (CERT), starting at offset 144 */
   ret_code = ecc108m_execute(ECC108_READ, ECC108_ZONE_DATA|ECC108_ZONE_COUNT_FLAG, offset,
                                 0, NULL, 0, NULL, 0, NULL,                   /* No data 1, data 2 or data 3   */
                                 sizeof(command), command,                    /* tx len, tx buffer    */
                                 sizeof(response), response);
   if ( ret_code != ECC108_SUCCESS )
   {
#if (VERBOSE > 1)
      DBG_logPrintf( 'E', "Reading macaddr failed at 0x%04x.\n", offset );
#endif
   }
   if ( ret_code == ECC108_SUCCESS )
   {
      offset = (uint16_t)offsetof( Certificate, macID ) % 32 ;
      response[ response[ 0 ] - sizeof( uint16_t )] = 0;   /* NULL terminate response buffer, starting at CRC   */
#if (VERBOSE > 2)
      DBG_logPrintf( 'I', "MAC addr -> %s\n", &response[1 + offset] );
#endif
      if( memcmp( &(response[1 + offset]), AclaraOUI, sizeof(AclaraOUI)-1 ) != 0 )  /*lint !e670 144%32 = 16 so no out of bounds access.  */
      {
#if (VERBOSE > 1)
         DBG_logPrintf( 'E', "Aclara OUI mismatch: " );
         for( i = 0; i < 6; i++ )
         {
            (void)DBG_printfNoCr( "%c", response[i+1 + offset] );
         }
         DBG_logPrintf( 'I', "" );
#endif
         ret_code = ECC108_GEN_FAIL;
      }
      /* Extract the remaining digits, regardless of OUI match, into the requested buffer */
      for( i = 0; i < 8; i++ )
      {
         (void)sscanf( (char *)&response[ 1 +  offset + (i*2) ], "%02hhx", &mac_id[ i ] );
      }
   }

   return ret_code;
}

/**************************************************************************************************
   Function: ecc108e_read_key
   Purpose:  Read a key.

   Arguments: keyID  - eccx08 slot number
              size   - number of bytes to read
              block  - 32 byte block number
              key    - pointer to key data

   Note: keys are 72 bytes - 4 bytes of pad, X, 4 bytes of pad, Y.
         blocks are 32 bytes

   Note: eccx08 device must already be opened.
   Return - uint8_t Success/failure of operation - See ecc108_lib_return_codes.h for codes
**************************************************************************************************/
#define PRINT_KEY 0
static uint8_t ecc108e_read_key( uint16_t keyID, uint8_t size, uint8_t block, uint8_t *key )
{
   uint16_t ecc_offset;                      /* Used to build eccx08 offset from slot  */
   int16_t bytes_to_read;                    /* Track bytes read        */
   uint8_t command[READ_COUNT];              /* Command buffer          */
   uint8_t response[ECC108_RSP_SIZE_32];     /* local response buffer   */
#if PRINT_KEY
   uint8_t *dst = key;
#endif
   uint8_t ret_code = ECC108_SUCCESS;        /* Function return value   */

   ecc_offset = (uint16_t)(keyID << 3);      /* Slot number indicated in bits 3-6  */
   ecc_offset += block << 8;                 /* Requested block  */
#if 0
   assert ( ( key != NULL ) &&
          ( (keyID <  8 ) && ( size ==  36 ) ) ||
          ( (keyID >  8 ) && ( size ==  72 ) ) ||
          ( (keyID == 8 ) && ( size == 160 ) ) ) ;
#endif

   /* Even though the number of bytes to read is not a multiple of 32, ALWAYS read 32 bytes; the device will not
      accept 4 byte reads in public keys.  */
   for( bytes_to_read = (int16_t)size; ( bytes_to_read > 0 ) && ( ret_code == ECC108_SUCCESS ); )
   {
      ret_code = ecc108m_execute(ECC108_READ, ECC108_ZONE_DATA | ECC108_ZONE_COUNT_FLAG, ecc_offset,
                                    0, NULL, 0, NULL, 0, NULL,    /* No data 1, data 2 or data 3  */
                                    sizeof(command), command,     /* tx len, tx buffer    */
                                    sizeof(response), response );

      /*lint --e{613} key cannot be NULL based on assert statement above */
      (void)memcpy( key, &response[1], min( (uint16_t)ECC108_KEY_SIZE, (uint16_t)bytes_to_read ) );
      bytes_to_read -= ECC108_KEY_SIZE;      /* Update bytes to write by amount just written */
      ecc_offset += 1 << 8;                  /* Next block  */
      key += ECC108_KEY_SIZE;                /* Update offset into source data   */
   }
#if INCLUDE_INFO_CMD
   /* Diagnostic - check key Authorization state   */
   infoCommand( &readKeyID );
#endif
   if ( ret_code != ECC108_SUCCESS )
   {
      asm("nop");
   }
#if PRINT_KEY
   printBuf( "Key", &keyID, dst, size );
#endif
   return ret_code;
}

/**************************************************************************************************
   Function: ecc108e_ReadKey
   Purpose:  External API to read a key ( unencrypted ).

   Arguments: keyID - eccx08 slot number
              keyLen - number of bytes to write
              keydata - pointer to key data

   Note: eccx08 device must NOT be opened.
   Return - uint8_t Success/failure of operation - See ecc108_lib_return_codes.h for codes
**************************************************************************************************/
uint8_t ecc108e_ReadKey( uint16_t keyID, uint8_t keyLen, uint8_t *keyData )
{
   buffer_t *keyBuf;             /* Malloc'd buffer for key data  */
   uint8_t  *paddedKey;          /* Pointer to data portion of keyBuf   */
   uint8_t  ret_code;            /* Return value.  */

   keyBuf = BM_alloc( 96 );      /*  Assume longest possible key (72, but smallest multiple of 32). */
   if ( keyBuf != NULL )
   {
      paddedKey = keyBuf->data;

      /* Attempt this a few times as the device may time out.  */
      for ( uint8_t i = 0; i < ECCX08_RETRIES; i++ )
      {
         ret_code = ecc108_open();
         if ( ret_code == ECC108_SUCCESS )   /* Operation worked, check value returned */
         {
            ret_code = ecc108e_read_key( keyID, keyLen <= 32 ? keyLen : 96, 0, paddedKey );

            /* Remove pad from read value. */
            if ( keyLen > 32 )
            {
               ( void )memcpy( keyData,      paddedKey + 4,       32 ); /* Copy 32 bytes of x from offset 4.   */
               ( void )memcpy( keyData + 32, paddedKey + 36 + 4,  32 ); /* Copy 32 bytes of y from offset 36.  */
            }
            else
            {
               ( void )memcpy( keyData,      paddedKey + 0,       32 ); /* Copy 32 bytes of x from offset 0.   */
            }
            ecc108_close();
         }
         if ( ECC108_SUCCESS == ret_code )
         {
            break;
         }
      }
      BM_free( keyBuf );
   }
   else
   {
      ret_code = ECC108_GEN_FAIL;   /* Memory allocation failed.  */
   }
   return ret_code;
}

/**************************************************************************************************

   Function: ecc108e_read_config_zone
   Purpose:  Read eccx08 configuration zone

   Arguments: uint8_t *config_data - destination of data read from device
   Note: eccx08 device must already be opened.

   Return - Success/failure of operation

**************************************************************************************************/
static uint8_t ecc108e_read_config_zone(uint8_t *config_data)
{
   volatile uint8_t ret_code;          /* declared as "volatile" for easier debugging  */
   uint16_t config_address;
   uint8_t command[READ_COUNT];        /* Make the command buffer the size of the Read command. */
   uint8_t response[READ_32_RSP_SIZE]; /* Make the response buffer the size of the maximum Read response.   */
   uint8_t i;                          /* Loop counter   */

   config_address = 0;

   for( i = 0; i < ECC108_CONFIG_SIZE; i += ECC108_ZONE_ACCESS_32 )   /* Read the config zone in 32 byte chunks */
   {
      /* Read 32 bytes. Put a breakpoint after the read and inspect "response" to obtain the data. */
      (void)memset(response, 0, sizeof(response));
      ret_code = ecc108m_execute(ECC108_READ, ECC108_ZONE_CONFIG | ECC108_ZONE_COUNT_FLAG, config_address >> 2,
                                 0, NULL, 0, NULL, 0, NULL,    /* No data 1, data 2 or data 3   */
                                 sizeof(command), command,
                                 sizeof(response), response);
      if (ret_code != ECC108_SUCCESS)
      {
         break;
      }
      if (config_data)
      {
         (void)memcpy(config_data, &response[ECC108_BUFFER_POS_DATA], ECC108_ZONE_ACCESS_32);
         config_data += ECC108_ZONE_ACCESS_32;
      }
      config_address += ECC108_ZONE_ACCESS_32;
   }
   return ret_code;
}

/**************************************************************************************************

   Function: NonceTempKey
   Purpose:  Execute NONCE to update TempKey and compute resulting TempKey

   Arguments: uint8_t mode - Nonce mode
               uint8_t *TempKey - destination of computed TempKey
   Note: eccx08 device must already be opened.
   Return - Success/Failure of NONCE operation

**************************************************************************************************/
static uint8_t NonceTempKey( uint8_t mode, uint8_t TempKey[ECC108_KEY_SIZE] )
{
   buffer_t *responseBuf;
   buffer_t *commandBuf    = NULL;
   buffer_t *numinBuf      = NULL;
   buffer_t *shaBuf        = NULL;
   Sha256   *sha           = NULL;
   uint8_t  *response;
   uint8_t  *command;
   uint8_t  *numin;
   uint8_t  ret_code;

   /* To reduce stack usage, allocate buffers for the large variables needed by this routine.   */
   ret_code = ECC108_GEN_FAIL;
   responseBuf = BM_alloc( NONCE_RSP_SIZE_LONG );  /* buffer large enough for all responses in this function */
   if ( responseBuf != NULL )
   {
      response = responseBuf->data;
      commandBuf = BM_alloc( NONCE_COUNT_SHORT );  /* buffer large enough for all commands in this function */
      if ( commandBuf  != NULL )
      {
         command = commandBuf->data;
         numinBuf = BM_alloc( NONCE_NUMIN_SIZE );  /* Additional data passed to NONCE command   */
         if ( numinBuf  != NULL )
         {
            numin = numinBuf->data;
            shaBuf = BM_alloc( sizeof( Sha256 ) ); /* sha256 working area  */
            if ( shaBuf != NULL )
            {
               sha = ( Sha256 *)( void *)shaBuf->data;
               ret_code = ECC108_SUCCESS;
            }
         }
      }
   }
   if ( ret_code == ECC108_SUCCESS )
   {
      /*lint --e{613} --e{644} pointer are set and not NULL if ret_code == ECC108_SUCCESS.  */
      (void)wc_InitSha256( sha );     /* Initialize the sha256 machine */
      (void)memset( numin, 0, NONCE_NUMIN_SIZE );                 /*lint !e644 numin is set if ret_code == ECC108_SUCCESS  */
      ret_code = ecc108m_execute(ECC108_NONCE, mode, 0,
                                    NONCE_NUMIN_SIZE, numin,         /* data1 size, data1 *//*lint !e644 numin is set if ret_code == ECC108_SUCCESS. */
                                    0, NULL, 0, NULL,                /* No data2 or data3 */
                                    NONCE_COUNT_SHORT, command,      /* Tx buffer   *//*lint !e644 command is set if ret_code == ECC108_SUCCESS  */
                                    NONCE_RSP_SIZE_LONG, response);  /*lint !e644 response is set if ret_code == ECC108_SUCCESS  */

      if ( ret_code == ECC108_SUCCESS )
      {
         /* Recreate TempKey in device from Nonce command:
            Compute sha256 on RNGseed(RandOut), numin, NONCE opcode, Nonce mode  */
         (void)wc_Sha256Update( sha, &response[ 1 ], ECC108_KEY_SIZE ); /* sha256 RandOut   *//*lint !e644 response set if ret_code == ECC108_SUCCESS.   */
         (void)wc_Sha256Update( sha, numin, NONCE_NUMIN_SIZE );         /* sha256 Numin (20B 0)   *//*lint !e644 numin set if ret_code == ECC108_SUCCESS.   */
         numin[ 0 ] = ECC108_NONCE; /*lint !e644 numin set if ret_code == ECC108_SUCCESS.   */
         numin[ 1 ] = mode;         /*lint !e644 numin set if ret_code == ECC108_SUCCESS.   */
         /* numin[2] already 0, which is needed for the next sha256 call */
         (void)wc_Sha256Update( sha, numin, 3 );   /* sha256 Opcode, Mode, 0   *//*lint !e644 numin set if ret_code == ECC108_SUCCESS.   */
         (void)wc_Sha256Final( sha, TempKey );     /* shaSum = TempKey (inside device) */
      }
   }

   /* Rather than complicated if/else above, just check each buffer_t for NULL and free as necessary.   */
   if ( responseBuf != NULL )
   {
      BM_free( responseBuf );
   }
   if ( commandBuf != NULL )
   {
      BM_free( commandBuf );
   }
   if ( numinBuf != NULL )
   {
      BM_free( numinBuf );
   }
   if ( shaBuf != NULL )
   {
      BM_free( shaBuf );
   }

   return ret_code;
}
/**************************************************************************************************

   Function: GenDigTempKey
   Purpose:  Compute TempKey resulting from GenDig command

   Arguments:  uint16_t keyID - data slot to use
               uint8_t *keyData - Key data to use in the sha256 computation
               uint8_t *TempKeyIn - TempKey from previous operation
               uint8_t *TempKeyOut - destination of computed TempKey
   Note: eccx08 device must already be opened.
   Return - none

**************************************************************************************************/
#define GENDIG_NUMINSIZE 25
static uint8_t GenDigTempKey( uint16_t keyID,
                              uint8_t const keyData[ECC108_KEY_SIZE],
                              uint8_t const TempKeyIn[ECC108_KEY_SIZE],
                              uint8_t TempKeyOut[ECC108_KEY_SIZE] )
{
   buffer_t *responseBuf;        /* Response from security device */
   buffer_t *commandBuf = NULL;  /* commands to security device */
   buffer_t *numinBuf   = NULL;  /* data to security device */
   buffer_t *shaBuf     = NULL;  /* sha256 calculations buffer */
   Sha256   *sha;                /* sha256 working area  */
   uint8_t  *numin;              /* Used in TempKey calculation - 25B 0 */
   uint8_t  *command;
   uint8_t  *response;
   uint8_t  ret_code;

   /* To reduce stack usage, allocate buffers for the large variables needed by this routine.   */
   ret_code = ECC108_GEN_FAIL;
   responseBuf = BM_alloc( GENDIG_RSP_SIZE );
   if ( responseBuf != NULL )
   {
      response = responseBuf->data;
      commandBuf = BM_alloc( GENKEY_COUNT );
      if ( commandBuf  != NULL )
      {
         command = commandBuf->data;
         numinBuf = BM_alloc( GENDIG_NUMINSIZE );
         if ( numinBuf  != NULL )
         {
            numin = numinBuf->data;
            shaBuf = BM_alloc( sizeof( Sha256 ) );
            if ( shaBuf != NULL )
            {
               sha = ( Sha256 *)( void *)shaBuf->data;
               ret_code = ECC108_SUCCESS;
            }
         }
      }
   }
   if ( ret_code == ECC108_SUCCESS )
   {
      /*lint --e{613} --e{644} pointer are set and not NULL if ret_code == ECC108_SUCCESS.  */
      ret_code = ecc108m_execute( ECC108_GENDIG, ECC108_ZONE_DATA, keyID,
                                  0, NULL, 0, NULL, 0, NULL,      /* No data1, data2 or data3 */
                                  GENKEY_COUNT, command,          /* Tx buffer   *//*lint !e644 command is set if ret_code == ECC108_SUCCESS  */
                                  GENDIG_RSP_SIZE, response);     /*lint !e644 response is set if ret_code == ECC108_SUCCESS  */

      if ( ECC108_SUCCESS == ret_code )
      {
         if ( response[ 1 ] == ECC108_SUCCESS ) /*lint !e644 response is set if ret_code == ECC108_SUCCESS  */
         {
            (void)wc_InitSha256( sha );     /* Initialize the sha256 machine *//*lint !e644 sha is set if ret_code == ECC108_SUCCESS  */
            (void)memset( numin, 0, GENDIG_NUMINSIZE );  /*lint !e644 numin is set if ret_code == ECC108_SUCCESS  */

            /* Recreate TempKey in device resulting from GenDig command:
               sha256 on key ID data, GenDig opcode, GenDig mode, Key ID, SN8, SN[0:1], 25 Zeros, previous TempKey */
            (void)wc_Sha256Update( sha, keyData, ECC108_KEY_SIZE );
            numin[ 0 ] = ECC108_GENDIG;      /*lint !e644 numin is set if ret_code == ECC108_SUCCESS  */
            numin[ 1 ] = ECC108_ZONE_DATA;   /*lint !e644 numin is set if ret_code == ECC108_SUCCESS  */
            *(uint16_t *)&numin[ 2 ] = (uint16_t)keyID;  /*lint !e826 !e2445 stuffing uint16_t into two uint8_t locations intentionally *//*lint !e644 numin is set if ret_code ==
                                                                                                                                            ECC108_SUCCESS  */
            numin[ 4 ] = SerialNumber[ 8 ];  /*lint !e644 numin is set if ret_code == ECC108_SUCCESS  */
            numin[ 5 ] = SerialNumber[ 0 ];  /*lint !e644 numin is set if ret_code == ECC108_SUCCESS  */
            numin[ 6 ] = SerialNumber[ 1 ];  /*lint !e644 numin is set if ret_code == ECC108_SUCCESS  */
            (void)wc_Sha256Update( sha, numin, 7 );   /* sha256 GenDig Opcode, Mode, keyID, SN[8], SN[0:1]   *//*lint !e644 numin is set if ret_code == ECC108_SUCCESS  */

            (void)memset( numin, 0, GENDIG_NUMINSIZE );                 /*lint !e644 numin is set if ret_code == ECC108_SUCCESS  */
            (void)wc_Sha256Update( sha, numin, GENDIG_NUMINSIZE );      /* sha256 25 bytes of 0  *//*lint !e644 numin is set if ret_code == ECC108_SUCCESS  */
            (void)wc_Sha256Update( sha, TempKeyIn, ECC108_KEY_SIZE );   /* sha256  previous TempKey    */
            (void)wc_Sha256Final( sha, TempKeyOut );                    /* Return TempKey resulting from GenDig command*/
         }
         else
         {
            ret_code = ECC108_CHECKMAC_FAILED;
         }
      }
   }
   /* Rather than complicated if/else above, just check each buffer_t for NULL and free as necessary.   */
   if ( responseBuf != NULL )
   {
      BM_free( responseBuf );
   }
   if ( commandBuf != NULL )
   {
      BM_free( commandBuf );
   }
   if ( numinBuf != NULL )
   {
      BM_free( numinBuf );
   }
   if ( shaBuf != NULL )
   {
      BM_free( shaBuf );
   }

   return ret_code;
}

/**************************************************************************************************

   Function: MACResponse
   Purpose:  Compute Response resulting from MAC command

   Arguments: uint8_t   mode - MAC mode used
              uint16_t  keyID - data slot to use
              uint16_t  keylen - number of bytes in key
              uint8_t   TempKey - TempKey from previous sequence
              uint8_t   challenge - additional data input to MAC command
              uint8_t   *result - destination of computed Response
   Note: eccx08 device must already be opened.
   Return - success/failure of operation

**************************************************************************************************/
static uint8_t MACResponse( uint8_t mode, uint16_t keyID, uint16_t keylen,
                         uint8_t const TempKey[ECC108_KEY_SIZE],
                         uint8_t const challenge[ECC108_KEY_SIZE],
                         uint8_t result[ECC108_KEY_SIZE] )
{
#define MISC_DATA_SIZE  24
         buffer_t *responseBuf;
         buffer_t *shaBuf        = NULL;
         buffer_t *commandBuf    = NULL;
         buffer_t *miscDataBuf   = NULL;
         Sha256   *sha;       /* sha256 working area              */
   const uint8_t  *keyData;   /* Pointer to appropriate key data  */
   const uint8_t  *block1;    /* Pointer to block 1 data          */
         uint8_t  *block2;    /* Pointer to block 2 data          */
         uint16_t p1size;     /* Size of parameter 1 to device    */
         uint16_t p2size;     /* Size of parameter 2 to device    */
         uint8_t  *miscData;  /* Used in result calculations      */
         uint8_t  *command;   /* buffer long enough for all commands  in this routine   */
         uint8_t  *response;  /* buffer long enough for all responses in this routine   */
         uint8_t  ret_code;   /* Return value                     */

   ret_code = ECC108_GEN_FAIL;
   responseBuf = BM_alloc( MAC_RSP_SIZE );
   if ( responseBuf != NULL )
   {
      response = responseBuf->data;
      commandBuf = BM_alloc( MAC_COUNT_LONG );
      if ( commandBuf  != NULL )
      {
         command = commandBuf->data;
         miscDataBuf = BM_alloc( MISC_DATA_SIZE   ); /* Holds:
                                          miscData[ 0 ] = ECC108_MAC
                                          miscData[ 1 ] = mode
                                          miscData[ 2,3 ] = keyID;
                                          miscData[ 4-14 ] = 11 bytes of zeroes
                                          miscData[ 15 ] = SerialNumber[ 8 ];
                                          miscData[ 16-23 ] = various portions of SN   */
         if ( miscDataBuf != NULL )
         {
            miscData = miscDataBuf->data;
            shaBuf = BM_alloc( sizeof( Sha256 ) );
            if ( shaBuf != NULL )
            {
               sha = ( Sha256 *)( void *)shaBuf->data;
               ret_code = ECC108_SUCCESS;
            }
         }
      }
   }
   if ( ret_code == ECC108_SUCCESS )   /*lint -e{644} pointers are set if ret_code == ECC108_SUCCESS   */
   {
      if ( ( keyData = getKeyData( keyID, NULL ) ) != NULL )
      {
         /* Is block 1 TempKey or keyData?   */
         if ( ( mode & MAC_MODE_BLOCK1_TEMPKEY ) != 0 )
         {
            block1 = (uint8_t *)TempKey;
            p1size = ECC108_KEY_SIZE;
         }
         else
         {
            block1 = keyData;
            p1size = keylen;
         }
         /* Is block 2 TempKey data or challenge?  */
         if ( ( mode & MAC_MODE_BLOCK2_TEMPKEY ) != 0 )
         {
            p2size = 0;                   /* No parameter 2 sent to device          */
            block2 = (uint8_t *)TempKey;  /* But parameter 2 is included in sha256  */
         }
         else
         {
            p2size = ECC108_KEY_SIZE;
            block2 = (uint8_t *)challenge;
         }

         /* Execute MAC( mode, key ID  */
         ret_code = ecc108m_execute(   ECC108_MAC, mode, keyID,
                                       (uint8_t)p2size, block2,   /* block 2 size and ptr(might not be used - OK) */
                                       0, NULL, 0, NULL,          /* No data2 or data3 */
                                       MAC_COUNT_LONG, command,   /* Tx buffer   */
                                       MAC_RSP_SIZE, response);   /*lint !e644 response is set if ret_code == ECC108_SUCCESS  */

         if ( ECC108_SUCCESS == ret_code )
         {
            /* Compute Response =
               sha256(block1 data, block2 data, MAC opcode, MAC mode, keyID ID, 11B 0, SN[8], SN[4:7], Sn[0:3];
               where block1/2 can be TempKey/Key Data or TempKey/challenge, depending on mode. */
            (void)wc_InitSha256( sha );     /* Initialize the sha256 machine *//*lint !e644 sha is set if ret_code == ECC108_SUCCESS  */
            (void)wc_Sha256Update( sha, block1, p1size );               /* sha256 block1  */
            (void)wc_Sha256Update( sha, block2, ECC108_KEY_SIZE );      /* sha256 block2  */
            (void)memset( miscData, 0, MISC_DATA_SIZE );                /*lint !e644 miscData is set if ret_code == ECC108_SUCCESS  */
            miscData[ 0 ] = ECC108_MAC;                                 /*lint !e644 miscData  is set if ret_code == ECC108_SUCCESS  */
            miscData[ 1 ] = mode;                                       /*lint !e644 miscData is set if ret_code == ECC108_SUCCESS  */
            *(uint16_t *)&miscData[ 2 ] = keyID;                        /*lint !e826 stuffing uint16_t into two uint8_t locations intentionally */
            miscData[ 4 + 11 ] = SerialNumber[ 8 ];   /* Skip 11 bytes of zeros  */
            if ( ( mode & MAC_MODE_INCLUDE_SN ) != 0 )
            {
               (void)memcpy( &miscData[ 4+11+1 ], &SerialNumber[ 4 ], 4 );
               (void)memcpy( &miscData[ 4+11+1+4 ], &SerialNumber[ 0 ], 4 );
            }
            else
            {
               (void)memcpy( &miscData[ 4+11+1+4 ], &SerialNumber[ 0 ], 2 );  /* SN[0:1] always included */
            }
            (void)wc_Sha256Update( sha,
                     miscData, MISC_DATA_SIZE ); /* sha256 Opcode,Mode,keyID,11B 0,SN8,SN[4:7],SN[0:3]*/
            (void)wc_Sha256Final( sha, result );                        /* Save computed response  */

            /* Verify results */
#if (VERBOSE > 1)
            printBuf( "MAC result", NULL, &response[ 1 ], ECC108_KEY_SIZE );
            printBuf( "Computed MAC", NULL, result, ECC108_KEY_SIZE );
#endif
            if( memcmp( &response[ 1 ], result, SHA256_DIGEST_SIZE ) != 0 )
            {
               ret_code = ECC108_CHECKMAC_FAILED;
            }
         }
         else
         {
            ECC108_PRNT_ERROR( 'E', "%s ECC108_MAC failed, ret_code: 0x%02x", __func__, ret_code );
         }
      }
      else
      {
         ret_code = ECC108_BAD_PARAM;
      }
   }
   /* Rather than complicated if/else above, just check each buffer_t for NULL and free as necessary.   */
   if ( responseBuf != NULL )
   {
      BM_free( responseBuf );
   }
   if ( commandBuf != NULL )
   {
      BM_free( commandBuf );
   }
   if ( miscDataBuf != NULL )
   {
      BM_free( miscDataBuf );
   }
   if ( shaBuf != NULL )
   {
      BM_free( shaBuf );
   }
   return ret_code;
}

#if INCLUDE_INFO_CMD || DO_CHECK_INFO
/**************************************************************************************************
   Function:   infoCommand
   Purpose:    Helper diagnostic function to check the TempKey status bits

   Arguments:  uint16_t *keyID - key ID to print ( if not NULL )

   Note: eccx08 device must already be opened.
   Return - success/failure of operation
**************************************************************************************************/
static void infoCommand( uint16_t *keyID )
{
   uint8_t  command[INFO_COUNT];
   uint8_t  response[INFO_RSP_SIZE];   /* buffer large enough for info command response */

   /* Diagnostic - check key Authorization state   */
   ( void ) ecc108m_execute( ECC108_INFO, INFO_MODE_STATE, 2,
                             0, NULL, 0, NULL, 0, NULL, /* No data     */
                             INFO_COUNT, command,       /* Tx buffer   */
                             INFO_RSP_SIZE, response ); /* Rx buffer   */
   printBuf( "TempKey ", keyID, &response[1], 4 );
}
#endif

/**************************************************************************************************
   Function:   ecc108e_EncryptedKeyRead
   Purpose:    Read key keyID, using slotcfg[ keyID ].readKey to authorize access

   Arguments:  uint8_t keyID - key to read
               uint8_t *pDest Destination of key data

   Steps:   NonceTempKey()
            GenDigTempKey()
            Read slot keyID
            XOR slot data with TempKey
   Note: eccx08 device must NOT be opened.
   Return - success/failure of operation
**************************************************************************************************/
uint8_t ecc108e_EncryptedKeyRead( uint16_t keyID, uint8_t *keyData )
{
   const uint8_t                 *pKeyData;                    /* Pointer to key data in secROM                */
         uint8_t                 TempKey[SHA256_DIGEST_SIZE];  /* Storage for sha-256                          */
         uint8_t                 secretKey[ECC108_KEY_SIZE];   /* Data from KEY_UPDATE_WRITE_KEY key           */
         PartitionData_t const   *pPart;                       /* Used to access the security info partition   */
         dSize                   keyOff;                       /* Used to compute offset of key in secROM      */
         uint16_t                crc;                          /* Offset of key into partition                 */
         uint8_t                 keySize;                      /* Length of key in bytes                       */
         uint8_t                 bytesToRead;                  /* Number of bytes to read in do loop           */
         uint8_t                 ret_code = ECC108_GEN_FAIL;   /* Function return value                        */

   if ( keyID < ARRAY_IDX_CNT( slotcfg ) && ( keyData != NULL ) ) /*lint !e644 keyData is initialized by caller.  */
   {
      pPart = SEC_GetSecPartHandle();  /* Obtain handle to access internal NV variables   */

      if ( ( pKeyData = getKeyData( slotcfg[ keyID ].readKey, &keySize ) ) != NULL )   /* Need readKey data; assume keyID length is same as readKey length.  */
      {
         ret_code = ecc108_open();
         if( ret_code == ECC108_SUCCESS )
         {
            /* For keys longer than ECC108_KEY_SIZE, loop through the portion of the flow from Nonce to key read for each
               block of 32 bytes needed.
            */
            uint8_t block = 0;
            bytesToRead = keySize;
            do
            {
               ret_code = NonceTempKey( NONCE_MODE_SEED_UPDATE, TempKey );
#if INCLUDE_INFO_CMD
                  /* Diagnostic - check key Authorization state   */
               infoCommand( NULL ); /* No key ID to specify here. */
#endif
               if (ret_code == ECC108_SUCCESS)
               {
                  /* Execute GENDIG and recreate TempKey in device resulting from GenDig command: */
                  ret_code  = GenDigTempKey( slotcfg[ keyID ].readKey, pKeyData + ( block * ECC108_KEY_SIZE ), TempKey, TempKey );

                  if (ret_code == ECC108_SUCCESS)
                  {
                     ret_code = ecc108e_read_key( keyID, ECC108_KEY_SIZE, block, secretKey );
                     if (ret_code == ECC108_SUCCESS)
                     {
                        for( uint32_t i = 0; i < ECC108_KEY_SIZE; i++ )
                        {
                           secretKey[ i ] ^= TempKey[ i ];
                        }
#if (VERBOSE > 1)
                        printBuf( "Key", &keyID, secretKey, ECC108_KEY_SIZE );
#endif
                        (void)memcpy( keyData + ( block * ECC108_KEY_SIZE ), secretKey, sizeof( secretKey ) );   /* Pass result to calling function  */
                     }
                  }
               }
               block++;
               bytesToRead -= min( bytesToRead, ECC108_KEY_SIZE );
            } while ( ( bytesToRead != 0 ) && ( ECC108_SUCCESS == ret_code ) );
            ecc108_close();
            /* Now check that the copy in secROM matches, if this is one of the keys kept there. */
            if ( ret_code == ECC108_SUCCESS )
            {
               pKeyData = getKeyData( keyID, &keySize );          /* Point to original key data in secROM.  */
               if ( ( pKeyData != NULL ) && ( memcmp( secretKey, pKeyData, keySize ) != 0 ) ) /*lint !e645 if ret_code success, secretKey is init'd */
               {  /* Comparison failed - update secROM   */
                  /* Compute new key's CRC   */
                  CRC_ecc108_crc( keySize,         /* Number of bytes   */
                                  keyData,         /* Source data       */
                                  (uint8_t *)&crc, /* Resulting crc     */
                                  0,               /* Seed value        */
                                  (bool)false );   /* Don't hold the CRC engine  */

                  /* pKeyData has address of storage location for new key in secROM. Need offset from partition start.  */
                  keyOff = (dSize)( pKeyData - secROM.pKeys[ 0 ].key ); /* Offset into array       */
#if ( EP == 1 )
                  PWR_lockMutex( PWR_MUTEX_ONLY );                                   /* Function will not return if it fails   */
#else
                  PWR_lockMutex();                                   /* Function will not return if it fails   */
#endif
                  (void)PAR_partitionFptr.parWrite( (dSize)keyOff, secretKey, (lCnt)keySize, pPart ); /* Save new key   */

                  keyOff  = (dSize)offsetof( intFlashNvMap_t, keyCRC );                                           /* Offset to array of crc values */
                  keyOff += (dSize)( (uint16_t)( keyID - FIRST_STORED_KEY ) * sizeof( crc ) );                    /* Offset into array       */
                  (void)PAR_partitionFptr.parWrite( (dSize)keyOff, (uint8_t *)&crc, (lCnt)sizeof( crc ), pPart ); /* Save new key crc  */

#if ( EP == 1 )
                  PWR_unlockMutex(PWR_MUTEX_ONLY);    /* Function will not return if it fails   */
#else
                  PWR_unlockMutex();                  /* Function will not return if it fails   */
#endif
               }
            }
         }
         else
         {
            ret_code = ECC108_BAD_PARAM;
         }
      }
   }
   return ret_code;
}

/**************************************************************************************************
   Function:   ecc108e_KeyAuthorization
   Purpose:    Authorize a specific key

   Arguments: uint8_t keyID (slot) number to authorize
               uint8_t *key

Steps:   Nonce(mode 1, zero = Rnd, 20B numin (zeros))
         sha256(Nonce response, Numin, opcode, mode 0)
         sha256( key data, sha256 result, OtherData[0:3],
            OTP[0:7], OtherData[4:6], SN[8], OtherData[7:10], SN[0:1], OtherData[11:12]
         CheckMac( mode 0x21, keyID, 32B Zeros, Client Response, OtherData )

   Note: eccx08 device must already be opened.
   Return - none
**************************************************************************************************/
uint8_t ecc108e_KeyAuthorization( uint8_t keyID, uint8_t const *key )
{
#if ECC_DEVICE_TYPE == ECC108A
   uint8_t  OTP[8];                             /* Holds OTP[0:7]  */
#endif
   uint8_t  numin[NONCE_NUMIN_SIZE];            /* 20 bytes of zero input to Nonce command   */
   uint8_t  command[CHECKMAC_COUNT];            /* buffer large enough for all commands in this function  */
   uint8_t  response[ECC108_RSP_SIZE_32];       /* buffer large enough for all responses in this function */
   uint8_t  TempKey[SHA256_DIGEST_SIZE];        /* Storage for sha-256  */
   Sha256   sha;                                /* sha256 working area  */
   uint8_t  ret_code;
   uint8_t  macMode;                            /* CheckMac mode; dependant on device type   */

   (void)wc_InitSha256( &sha );  /* Only needed once - Sha256Final calls this as its last step  */

   /* Read the OTP   */
#if ECC_DEVICE_TYPE == ECC108A
   ret_code = ecc108m_execute(ECC108_READ, ECC108_ZONE_OTP|ECC108_ZONE_COUNT_FLAG, 0,
                              0, NULL, 0, NULL, 0, NULL,          /* No data 1, data 2 or data 3   */
                              READ_COUNT, command,
                              READ_32_RSP_SIZE, response);
#endif

#if ECC_DEVICE_TYPE == ECC108A
   if (ret_code == ECC108_SUCCESS)
#endif
   {
#if ECC_DEVICE_TYPE == ECC108A
         (void)memcpy( OTP, &response[1], sizeof( OTP ) );  /* Save the OTP data   */
#elif ECC_DEVICE_TYPE == ECC508A
#else
#warning "Undefined security chip"
#endif

      /* sha256 of response from Nonce, Numin, Opcode, Mode, 0 */
      ret_code = NonceTempKey( NONCE_MODE_NO_SEED_UPDATE, TempKey );

      if (ret_code == ECC108_SUCCESS)
      {

         /* sha256 of key data, TempKey, OtherData, etc...  */
         (void)wc_Sha256Update( &sha, key, ECC108_KEY_SIZE );     /* sha256 on key data */
         (void)wc_Sha256Update( &sha, TempKey, sizeof( TempKey ) );  /* sha256 on TempKey   */
         (void)memset( numin, 0, sizeof(numin) );
#if ECC_DEVICE_TYPE == ECC108A
         (void)wc_Sha256Update( &sha, numin, 4 );                 /* sha256 on OtherData[0:3]   */
         (void)wc_Sha256Update( &sha, OTP, sizeof( OTP ) );       /* sha256 on OTP data   */
#elif ECC_DEVICE_TYPE == ECC508A
         (void)wc_Sha256Update( &sha, numin, 4 + 8 );             /* sha256 on OtherData[0:3] + OTP[8] (all 0's)  */
#endif
         (void)wc_Sha256Update( &sha, numin, 3 );                 /* sha256 on OtherData[4:6]   */
         (void)wc_Sha256Update( &sha, &SerialNumber[8], 1 );      /* sha256 on SN[8]   */
         (void)wc_Sha256Update( &sha, numin, 4 );                 /* sha256 on OtherData[7:10]   */
         (void)wc_Sha256Update( &sha, &SerialNumber[0], 2 );      /* sha256 on SN[0:1]   */
         (void)wc_Sha256Update( &sha, numin, 2 );                 /* sha256 on OtherData[11:12]   */
         (void)wc_Sha256Final( &sha, TempKey );                /* Retrieve the sha256 results   */

         (void)memset( response, 0, sizeof( response ) );
#if ECC_DEVICE_TYPE == ECC108A
         macMode = MAC_MODE_INCLUDE_OTP_64|MAC_MODE_BLOCK2_TEMPKEY;
#elif ECC_DEVICE_TYPE == ECC508A
         macMode = MAC_MODE_BLOCK2_TEMPKEY;
#else
#warning "Undefined security chip"
#endif
         /* CheckMac( Mode 0x21, keyID, 32B zeros, Clent Response, OtherData  */
         ret_code = ecc108m_execute(ECC108_CHECKMAC, macMode, keyID,
                                    CHECKMAC_CLIENT_CHALLENGE_SIZE, response, /* 32 bytes of 0           */
                                    sizeof( TempKey ), TempKey,               /* ClientResp from sha256  */
                                    CHECKMAC_OTHER_DATA_SIZE, response,       /* 13 bytes of 0           */
                                    CHECKMAC_COUNT, command,
                                    READ_4_RSP_SIZE, response);
         if( ret_code == ECC108_SUCCESS )
         {
            /* Operation succeed, return the result of the check; 0->pass, 1->fail  */
            ret_code = response[1];
         }
      }
   }
#if (VERBOSE > 0)
   if( ret_code == ECC108_SUCCESS )
   {
      DBG_logPrintf( 'I', "Key %d is authorized", keyID );
   }
#endif

   return ret_code;
}

/**************************************************************************************************
   Function:   ecc108e_SignMessage
   Purpose:    Sign a message with a specific key

   Arguments:  key (slot) to sign
               pointer to message to be signed
               message length
               hash flag
               pointer to Signature generated (storage)

   Steps:   if hash set, sha256( message )
            nonce( mode=3(Passthrough), zero = 0, message digest )
            random( mode = 0, (Update seed) zero = 0 )
            Repeat: sign( mode = 0x80(External), key, ) while length of sign operation < VERIFY_256_SIGNATURE_SIZE (64)

   NOTE: if hash is false, the message passed in is expected to be a hash. If not, the operation will probably not
         produce the desired results!
   Note: eccx08 device must already be opened.
   Return - none
**************************************************************************************************/
static uint8_t ecc108e_SignMessage( uint8_t keyID,
                                    uint16_t length,
                                    uint8_t const *msg,
                                    bool  hash,
                                    uint8_t signature[ VERIFY_256_SIGNATURE_SIZE ] )
{
   uint8_t  ret_code;
   uint8_t  command[NONCE_COUNT_LONG];
   uint8_t  response[SIGN_RSP_SIZE];      /* buffer large enough for all responses in this function */
   uint8_t  Digest[SHA256_DIGEST_SIZE];   /* Storage for sha-256  */
   Sha256   sha;                          /* sha256 working area  */

   if ( hash )
   {
      /* sha256 the message */
      (void)wc_InitSha256( &sha );
      (void)wc_Sha256Update( &sha, msg, length );  /* sha256 on message */
      (void)wc_Sha256Final( &sha, Digest );        /* Retrieve the sha256 results   */
   }
   else
   {  /*  The length should be EXACTLY 32 bytes, but protect the buffer in case of error  */
      (void)memcpy( Digest, msg, min( length, sizeof( Digest ) ) );
   }

   /*lint -e{778} index computes to 0  */
   ret_code = ecc108e_KeyAuthorization( HOST_AUTH_KEY, secROM.pKeys[ HOST_AUTH_KEY - FIRST_STORED_KEY ].key );

   if (ret_code == ECC108_SUCCESS)
   {
      /* Generate a random number. Only done to update the random seed; otherwise the sign operation will fail   */
      ret_code = ecc108m_execute(ECC108_RANDOM, RANDOM_SEED_UPDATE, 0,
                                    0, NULL, 0, NULL, 0, NULL,
                                    sizeof(command), command,
                                    RANDOM_RSP_SIZE, response);

      if (ret_code == ECC108_SUCCESS)
      {
         /* Nonce( mode 3, zero = 0, 32B digest */
         ret_code = ecc108m_execute(ECC108_NONCE, NONCE_MODE_PASSTHROUGH, 0,
                                       sizeof( Digest ), Digest,        /* data1 size, data1 */
                                       0, NULL, 0, NULL,                /* No data2 or data3 */
                                       NONCE_COUNT_LONG, command,       /* Tx buffer */
                                       NONCE_RSP_SIZE_SHORT, response);

         if (ret_code == ECC108_SUCCESS)
         {
            do
            {
               if (ret_code == ECC108_SUCCESS)
               {
                  /* Sign( mode = 0x80, keyID   */
                  ret_code = ecc108m_execute(ECC108_SIGN, SIGN_MODE_EXTERNAL, keyID,
                                             0, NULL, 0, NULL, 0, NULL,
                                             SIGN_COUNT, command,
                                             SIGN_RSP_SIZE, response);

                  /* Return the signature, response[0] has returned signature length */
                  (void)memcpy( signature, &response[1], (size_t)min(response[0]-3, VERIFY_256_SIGNATURE_SIZE) ); /*lint !e644 signature IS initialized.  */
               }
            } while ( ( ret_code == ECC108_SUCCESS ) && ( response[0] < (VERIFY_256_SIGNATURE_SIZE + 3) ) );
         }
      }
   }
   return ret_code;
}

/**************************************************************************************************

   Function: ecc108e_InitHostAuthKey
   Purpose:  - Make sure key in secROM is valid:
             - Check to see if the default mfg keys are in place;
               If so, replace keys 4, 13, 15 with Encrypted Writes.
                  If flashsecurity is enabled - generate new keys.
                  Else rewrite default keys.

   Arguments: none
   Return - success/failure
   NOTE: MUST succeed or security device is inaccessible from now on!
         Also, eccx08 device must NOT be opened.

**************************************************************************************************/
static uint8_t ecc108e_InitHostAuthKey( void )
{
#if (UPDATE_DEFAULT_KEYS != 0)   /* Leave off until problem with corrupting keys in secROM resolved   */
   int32_t  diff1;
   int32_t  diff2;
   int32_t  diff3;
   bool     doUpdate = (bool)false;
#endif
   uint8_t  ret_code
#if (UPDATE_DEFAULT_KEYS != 0)
      = ECC108_SUCCESS
#endif
;

#if (UPDATE_DEFAULT_KEYS != 0)   /* Leave off until problem with corrupting keys in secROM resolved   */
   /* Compare each of the 3 keys in secROM with the factory defaults *//*lint --e{778} constant expression evaluates to 0. */
   diff1 = memcmp( intROM.pKeys  [ HOST_AUTH_KEY - FIRST_STORED_KEY ].key,             secROM.pKeys[ HOST_AUTH_KEY - FIRST_STORED_KEY ].key,               ECC108_KEY_SIZE );
   diff2 = memcmp( intROM.sigKeys[ NETWORK_PUB_KEY_SECRET - FIRST_SIGNATURE_KEY ].key, secROM.sigKeys[ NETWORK_PUB_KEY_SECRET - FIRST_SIGNATURE_KEY ].key, ECC108_KEY_SIZE );
   diff3 = memcmp( intROM.sigKeys[ PUB_KEY_SECRET - FIRST_SIGNATURE_KEY ].key,         secROM.sigKeys[ PUB_KEY_SECRET - FIRST_SIGNATURE_KEY ].key,         ECC108_KEY_SIZE );

   if ( NV_FSEC_SEC( NV_FSEC ) != 2 )     /* Value of 2 means unsecured; all others secured. */
   {
      doUpdate = ( ( diff1 == 0 ) || ( diff2 == 0 ) || ( diff3 == 0 ) );  /* If any of the keys match the default, update. */
   }
   else
   {
      /* Flash security is off - restore default keys */
      doUpdate = ( ( diff1 != 0 ) || ( diff2 != 0 ) || ( diff3 != 0 ) );  /* If any of the keys don't match the default, update. */
   }
   if ( doUpdate )
   {
      /* If any of the 3 need updates, they probably all do; update them with new values. */
      ret_code |= ecc108e_EncryptedKeyWrite( HOST_AUTH_KEY,          NULL, ECC108_KEY_SIZE );
      ret_code |= ecc108e_EncryptedKeyWrite( NETWORK_PUB_KEY_SECRET, NULL, ECC108_KEY_SIZE );
      ret_code |= ecc108e_EncryptedKeyWrite( PUB_KEY_SECRET,         NULL, ECC108_KEY_SIZE );
   }
#endif

#if (UPDATE_DEFAULT_KEYS != 0)
   if ( ECC108_SUCCESS == ret_code )
#endif
   {
      ret_code = ecc108_open();
      if ( ECC108_SUCCESS == ret_code )
      {
         /*lint -e{778} constant equates to 0; OK  */
         ret_code = ecc108e_KeyAuthorization( HOST_AUTH_KEY, secROM.pKeys[ HOST_AUTH_KEY - FIRST_STORED_KEY ].key );
         ecc108_close();
      }
   }
   if ( ECC108_SUCCESS != ret_code )
   {
      DBG_logPrintf( 'E', "Failed to authorize key: %d \n", HOST_AUTH_KEY );
      ECC108_PRNT_ERROR( 'E', "ecc108e_KeyAuthorization failed: 0x%02x", ret_code );
   }
   return ret_code;
}

/**************************************************************************************************

   Function: ecc108e_verifyDevicePKI
   Purpose:  Verify Device Private Key Infrastructure:
             - Verify OUI matches "001D24"
             - Reconstruct signer X.509 certificate
             - Perform ECDSA verification of signer CA using root CA
             - Perform ECDSA verification of device CA using signer CA
             - Sign a random challenge; Verify that signature returned matches public key

   Arguments: None
   Note: eccx08 device must NOT be opened.
   Return - uint8_t Success/failure of operation - See ecc108_lib_return_codes.h for codes

**************************************************************************************************/
static uint8_t ecc108e_verifyDevicePKI( void )
{
   buffer_t    *certBuf;            /* for builtCert           */
   buffer_t    *keyBuf     = NULL;  /* for key (Certificate)   */
   buffer_t    *shaBuf     = NULL;  /* for Sha256              */
   buffer_t    *sumBuf     = NULL;  /* for shaSum              */
   buffer_t    *cmdBuf     = NULL;  /* for command             */
   buffer_t    *rspBuf     = NULL;  /* for response            */
   buffer_t    *key9Buf    = NULL;  /* for key9                */
   buffer_t    *key10Buf   = NULL;  /* for key10               */
   buffer_t    *sigBuf     = NULL;  /* for signature           */
   buffer_t    *randBuf    = NULL;  /* for randomMsg           */
   Certificate *key;                /* Holds slot 8 - two 72 byte signatures + 16 byte macID   */
   CertDef     *builtCert;          /* Space for re-built certificates  */
   Sha256      *sha;                /* Storage for sha-256  */
   uint8_t     *tbs;                /* Pointer to cert section To Be Signed   */
   uint8_t     *shaSum;             /* sha256 result  */
   uint8_t     *command;            /* buffer large enough for all commands in this function */
   uint8_t     *response;           /* buffer large enough for all responses in this function */
   uint8_t     *key9;               /* Data from key DEV_PUB_KEY (9)  */
   uint8_t     *key10;              /* Data from key SIGNER_PUB_KEY (10) */
   uint8_t     *signature;
   uint8_t     *randomMsg;
   uint16_t    taglen;              /* Length of field(s) in template   */
   int16_t     result = 0;
   uint8_t     certIndex;           /* Which key from slot 8 is to be used */
   uint8_t     mode;                /* Mode passed to verify command */
   uint8_t     verifyResults = 0;   /* Diagnostic: bit set for each verification step  */
   uint8_t     ret_code = ECC108_GEN_FAIL;

   /* To reduce stack usage, allocate buffers for the large variables needed by this routine.   */

   certBuf = BM_alloc( sizeof( CertDef ) );
   if ( certBuf != NULL )
   {
      builtCert = (CertDef *)(void *)certBuf->data;
      shaBuf = BM_alloc( sizeof( Sha256 ) ); /* Sha256 buffer  */
      if ( shaBuf != NULL )
      {
         sha = (Sha256 *)(void *)shaBuf->data;
         keyBuf = BM_alloc( sizeof( Certificate ) );  /* key buffer  */
         if ( keyBuf != NULL )
         {
            key = (Certificate *)(void *)keyBuf->data;
            sumBuf = BM_alloc( SHA256_DIGEST_SIZE );  /* sha sum buffer */
            if ( sumBuf != NULL )
            {
               shaSum = sumBuf->data;
               cmdBuf = BM_alloc( VERIFY_256_EXTERNAL_COUNT ); /* command buffer */
               if ( cmdBuf != NULL )
               {
                  command = cmdBuf->data;
                  rspBuf = BM_alloc( SIGN_RSP_SIZE );
                  if ( rspBuf != NULL )
                  {
                     response = rspBuf->data;
                     key9Buf = BM_alloc( VERIFY_283_KEY_SIZE );
                     if ( key9Buf != NULL )
                     {
                        key9 = key9Buf->data;
                        key10Buf = BM_alloc( VERIFY_283_KEY_SIZE );
                        if ( key10Buf != NULL )
                        {
                           key10 = key10Buf->data;
                           sigBuf = BM_alloc( VERIFY_256_SIGNATURE_SIZE );
                           if ( sigBuf != NULL )
                           {
                              signature = sigBuf->data;
                              randBuf = BM_alloc( ECC108_KEY_SIZE );
                              if ( randBuf != NULL )
                              {
                                 randomMsg = randBuf->data;
                                 ret_code = ECC108_SUCCESS; /* All buffers allocated!  */
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
   if( ECC108_SUCCESS == ret_code )
   {
      /*lint --e{613} --e{644} pointer are set and not NULL if ret_code == ECC108_SUCCESS.  */
      /* List of public keys, their size and destinations needed for this routine   */
      struct
      {
         uint16_t keyID;
         uint16_t size;
         uint8_t  *dest;
      } pubKeys[] =
      {
         {  CERT,             sizeof( Certificate ), (uint8_t *)&key->certs[ SignerCert ]},
         {  DEV_PUB_KEY,      VERIFY_283_KEY_SIZE,    key9 },
         {  SIGNER_PUB_KEY,   VERIFY_283_KEY_SIZE,   key10 }
      };

      ret_code = ecc108e_InitHostAuthKey();
      if( ECC108_SUCCESS == ret_code )
      {
         (void)wc_InitSha256( sha );   /* Only needed once - Sha256Final calls this as its last step  */

         ret_code = ecc108_open();
         if( ECC108_SUCCESS == ret_code )
         {
            /* Generate a random number as a message to be signed */
            ret_code = random( (bool)false, randomMsg );

            if( ECC108_SUCCESS == ret_code )
            {
#if (VERBOSE > 2)
               printBuf( "Message", NULL, randomMsg, ECC108_KEY_SIZE );
#endif
               ret_code = ecc108e_SignMessage( DEV_PRI_KEY_0, ECC108_KEY_SIZE, randomMsg, (bool)true, signature );
               if( ECC108_SUCCESS == ret_code )
               {
#if (VERBOSE > 2)
                  printBuf( "Signature: ", NULL, signature, VERIFY_256_SIGNATURE_SIZE );
#endif

                  /* Retrieve the public keys needed for this process   */
                  for( uint8_t i = 0; (i < ARRAY_IDX_CNT(pubKeys)) && (ret_code == ECC108_SUCCESS); i++ )
                  {
                     ret_code = ecc108e_read_key( pubKeys[i].keyID, (uint8_t)pubKeys[i].size, 0, pubKeys[i].dest );
#if (VERBOSE > 2)
                     printBuf( "Key ", &pubKeys[i].keyID, pubKeys[i].dest, pubKeys[i].size );
#endif
                  }
                  if( ret_code == ECC108_SUCCESS )
                  {
                     /* Verify OUI  */
                     if ( strncmp( AclaraOUI, (char const *)key->macID, sizeof( AclaraOUI ) - 1 ) == 0 )
                     {
                        /* Loop through two certificates: Signer and Device   */
                        for( certIndex = 0; certIndex < 2; certIndex++ )
                        {
                           /* Reconstruct X.509 signer certificate */
                           (void)memset( (uint8_t *)builtCert, 0, sizeof( CertDef ) ); /*lint !e668 buildCert is set if ret_code == ECC108_SUCCESS  */
                           if( certIndex == 0 )
                           {
                              mode = VERIFY_MODE_EXTERNAL;
                              /* Build the Signer certificate in builtCert  */
                              /*lint -e{613} *//* pointers not NULL if ret_code == ECC108_SUCCESS  */
                              result = buildSignerCert( (uint8_t *)key, key10, DeviceRootCAPublicKey, builtCert );
                           }
                           else
                           {
                              mode = VERIFY_MODE_STORED;
                              /* Build the Device certificate in builtCert  */
                              /*lint -e{613} *//* pointers not NULL if ret_code == ECC108_SUCCESS  */
                              result = buildDeviceCert( (uint8_t *)key, key9, key10, builtCert);
                           }
                           assert( result == 0 );

                           /* Find the tbs portion of the cert - second tag   */
                           tbs = (uint8_t *)builtCert;
                           if( (*(tbs+1) & 0x80 ) != 0 )
                           {
                              tbs += 2 + (*(tbs+1) & ~0x80);   /* Skip tag, length byte, size of length  */
                           }
                           else
                           {
                              tbs += 3;                  /* Skip tag, length, length of field(1)   */
                           }
                           /* Find length portion of builtCert to be signed */
                           taglen = 2 + *(tbs+1);        /* Assume 1 byte length */
                           if( (taglen & 0x80) != 0 )    /* Verify 1 byte length */
                           {
                              taglen = 2 + ( *(tbs+1) & ~0x80 ) + (*(tbs+2) << 8 ) + *(tbs+3);
                           }

#if (VERBOSE > 2)
                           printBuf( "Hashing", NULL, tbs, taglen );
#endif
                           /* run sha256 on certificate  */
                           (void)wc_Sha256Update( sha, tbs, taglen );   /* Run the sha256 algorithm   */
                           (void)wc_Sha256Final( sha, shaSum );         /* Retrieve the sha256 results   */

                           /* ECDSA verification   */
                           /* Start with Nonce to put the sha256 digest in Tempkey  */
                           ret_code = ecc108m_execute(ECC108_NONCE, NONCE_MODE_PASSTHROUGH, 0,
                                    SHA256_DIGEST_SIZE, shaSum,         /* data1 size, data1 */
                                    0, NULL, 0, NULL,                   /* No data2 or data3 */
                                    NONCE_COUNT_LONG, command,          /* Buffer to hold data1, data2, and data3 */
                                    NONCE_RSP_SIZE_SHORT, response);

                           if( ret_code == ECC108_SUCCESS )
                           {
                              /* Verify command with key in slot certIndex with public key from slot 8 (CERT) */
                              ret_code = ecc108m_execute(ECC108_VERIFY, mode, slotNumbers[ certIndex ],
                                                   VERIFY_256_SIGNATURE_SIZE, key->certs[ certIndex ].signature,
                                                   (mode == VERIFY_MODE_EXTERNAL) ? VERIFY_256_SIGNATURE_SIZE : 0,
                                                   (mode == VERIFY_MODE_EXTERNAL) ? (uint8_t *)DeviceRootCAPublicKey : NULL,
                                                   0, NULL,
                                                   (mode == VERIFY_MODE_EXTERNAL) ?
                                                      VERIFY_256_EXTERNAL_COUNT : VERIFY_256_STORED_COUNT,
                                                   command,
                                                   ECC108_RSP_SIZE_MIN, response);
                              if( ret_code == ECC108_SUCCESS )
                              {
                                 /* Requested operation suceeded - check for verification */
                                 if ( response[ECC108_BUFFER_POS_DATA] != ECC108_SUCCESS )
                                 {
                                    /* Invalid certificate  */
                                    verifyResults |= (uint8_t)( ((uint8_t)1) << certIndex ); /*lint !e701 1 is unsigned */
                                    ECC108_PRNT_ERROR( 'E', "%s verify result not 0. certIndex: %d, mode: %hhu", __func__, certIndex, mode );
                                 }
                              }
                              else  /* Requested operation failed */
                              {
                                 ECC108_PRNT_ERROR( 'E', "%s verify failed. ret_code: 0x%02x, certIndex: %d, mode: %hhu", __func__, ret_code, certIndex, mode );
                                 verifyResults |= (uint8_t)( ((uint8_t)8) << certIndex ); /*lint !e701 8 is unsigned */
                              }
                           }
                           else
                           {
                              ECC108_PRNT_ERROR( 'E', "%s nonce failed: 0x%02x", __func__, ret_code );
                           }
                        }  /* end for loop   */

                        /* Signer and device certs verfied; now ECDSA verify a signature  */
                        if( ret_code == ECC108_SUCCESS )
                        {
#if (VERBOSE > 2)
                           printBuf( "Message: ", NULL, randomMsg, ECC108_KEY_SIZE );
#endif
                           /* Perform sha256 on message generated above   */
                           (void)wc_Sha256Update( sha, randomMsg, ECC108_KEY_SIZE );   /* Run sha256 algorithm   */
                           (void)wc_Sha256Final( sha, shaSum );                        /* Retrieve the sha256 results   */
#if (VERBOSE > 2)
                           printBuf( "Hashed message: ", NULL, shaSum, SHA256_DIGEST_SIZE );
#endif

                           /* Perform a nonce to put the digest into Tempkey */
                           ret_code = ecc108m_execute(ECC108_NONCE, NONCE_MODE_PASSTHROUGH, 0,
                                    SHA256_DIGEST_SIZE, shaSum,      /* data1 size, data1 */
                                    0, NULL, 0, NULL,                /* No data2 or data3 */
                                    NONCE_COUNT_LONG, command,       /* buffer for data1, data2, and data3 */
                                    NONCE_RSP_SIZE_SHORT, response);
                           if( ret_code == ECC108_SUCCESS )
                           {
                              /* Verify the signature from the previous step, in the response buffer.  */
                              ret_code = ecc108m_execute(ECC108_VERIFY, VERIFY_MODE_STORED, DEV_PUB_KEY,
                                                            VERIFY_256_SIGNATURE_SIZE, signature,
                                                            0, NULL,
                                                            0, NULL,
                                                            VERIFY_256_STORED_COUNT, command,
                                                            ECC108_RSP_SIZE_MIN, response);
                              if( ret_code == ECC108_SUCCESS )
                              {
                                 if( response[ 1 ] != 0 )
                                 {
#if (VERBOSE > 1)
                                    DBG_logPrintf( 'E', "Signature verification failed\n" );
#endif
                                    ret_code = ECC108_GEN_FAIL;
                                 }
                              }
                              else
                              {
                                 ECC108_PRNT_ERROR( 'E', "%s Signature verification failed: 0x%02x", __func__, ret_code );
                              }
                           }
                           else
                           {
                              ECC108_PRNT_ERROR( 'E', "%s nonce failed: 0x%02x", __func__, ret_code );
                           }
                        }
                     }
                  }
               }
               else
               {
                 ECC108_PRNT_ERROR( 'E', "%s sign msg failed: 0x%02x", __func__, ret_code );
               }
            }
            ecc108_close();
         }
      }
      else
      {
         ECC108_PRNT_ERROR( 'E', "%s: Init host auth key failed: 0x%02x", __func__, ret_code );
      }
   }
   else
   {
      ECC108_PRNT_ERROR( 'E', "%s: BM_Alloc failed: 0x%02x", __func__, ret_code );
   }
   /* Rather than complicated if/else above, just check each buffer_t for NULL and free as necessary.   */
   if ( certBuf != NULL )
   {
      BM_free( certBuf );
   }
   if ( keyBuf != NULL )
   {
      BM_free( keyBuf );
   }
   if ( shaBuf != NULL )
   {
      BM_free( shaBuf );
   }
   if ( sumBuf != NULL )
   {
      BM_free( sumBuf );
   }
   if ( cmdBuf != NULL )
   {
      BM_free( cmdBuf );
   }
   if ( rspBuf != NULL )
   {
      BM_free( rspBuf );
   }
   if ( key9Buf != NULL )
   {
      BM_free( key9Buf );
   }
   if ( key10Buf != NULL )
   {
      BM_free( key10Buf );
   }
   if ( sigBuf != NULL )
   {
      BM_free( sigBuf );
   }
   if ( randBuf != NULL )
   {
      BM_free( randBuf );
   }
   /* Note: Upper nibble has bits set for operation failure.
            Lower nibble has bits set for invalid certificates.  */
#if (VERBOSE > 2)
   DBG_logPrintf( 'I', "Verify results = 0x%02x\n", verifyResults );
#endif
   if ( verifyResults != 0 )
   {
      ret_code = ECC108_GEN_FAIL ;
   }
   return ret_code;
}

/**************************************************************************************************

   Function: ecc108e_bfc
   Purpose:  Basic functionality check
             - Wake up
             - Generate random number

   Arguments: None
   Note: eccx08 device must NOT be opened.
   Return - uint8_t Success/failure of operation - See ecc108_lib_return_codes.h for codes

**************************************************************************************************/
uint8_t ecc108e_bfc( void )
{
   uint8_t config_data[ECC108_CONFIG_SIZE];
   uint8_t ret_code;

   /* Attempt this a few times as the device may time out.  */
   for ( uint8_t i = 0; i < ECCX08_RETRIES; i++ )
   {
      ret_code = ecc108_open();
      if ( ECC108_SUCCESS == ret_code )      /* Wakeup successful.   */
      {
         ret_code = random( (bool)false, config_data );
         ecc108_close();
      }
      if ( ECC108_SUCCESS == ret_code )
      {
         break;
      }
   }
   return ret_code;
}

/**************************************************************************************************

   Function: ecc108e_SelfTest
   Purpose:  Perform self test:
             - Check config lock
               if NOT locked
                  - write config
                  - lock config
             - Verify config
             - Check data lock
               if locked
                  verifyDevicePKI

   Arguments: None
   Note: eccx08 device must NOT be opened.
   Return - uint8_t Success/failure of operation - See ecc108_lib_return_codes.h for codes

**************************************************************************************************/
uint8_t ecc108e_SelfTest( void )
{
   uint8_t ret_code;

   /* Attempt this a few times as the device may time out.  */
   for ( uint8_t i = 0; i < ECCX08_RETRIES; i++ )
   {
      ret_code = ecc108e_verifyDevicePKI();     /* Does it pass the device PKI verification?     */
      if ( ECC108_SUCCESS == ret_code )
      {
         break;
      }
   }
   return ret_code;
}

/**************************************************************************************************

   Function: ecc108e_SeedRandom
   Purpose:  Seed the random number generator

            If LastRandom has all-zero data, update it by calling the random function.

   Arguments: None
   Return - pointer to uint8_t[ 32 ] byte random number.

**************************************************************************************************/
uint8_t *ecc108e_SeedRandom( void )
{
   uint8_t  *rnd = LastRandom;
   bool     needRand = (bool)true;

   for ( uint8_t i = 0; i < sizeof( LastRandom ); i++ )
   {
      if( *rnd != 0 )
      {
         needRand = (bool)false;
      }
   }
   if ( needRand )
   {
   }
   return LastRandom;
}

/**************************************************************************************************

   Function: ecc108e_SecureRandom
   Purpose:  Securely pass RNG Seed

   Arguments: None
   Note: eccx08 device must NOT be opened.
   Return - pointer to uint8_t[ ECC108_KEY_SIZE ] byte random number.

**************************************************************************************************/
uint8_t *ecc108e_SecureRandom( void )
{
   uint8_t  TempKeyNonce[ECC108_KEY_SIZE];   /* Results of Nonce command calculations                 */
   uint8_t  TempKeyGenDig[ECC108_KEY_SIZE];  /* Results of GenDig command calculations                */
   uint8_t  mac_resp[ECC108_KEY_SIZE];       /* Results of MAC command calculations                   */
   uint8_t  ret_code;

   /* Attempt this a few times as the device may time out.  */
   for ( uint8_t i = 0; i < ECCX08_RETRIES; i++ )
   {
      ret_code = ecc108_open();
      if ( ECC108_SUCCESS == ret_code )
      {
         /* Recreate TempKey in device resulting from Nonce command: */
         ret_code = NonceTempKey( NONCE_MODE_SEED_UPDATE, TempKeyNonce );

         if ( ECC108_SUCCESS == ret_code )
         {
            /* Execute GENDIG and recreate TempKey in device resulting from GenDig command: */
            /*lint -e{778} Info 778: Constant expression evaluates to 0 in operation '&' */
            ret_code = GenDigTempKey( HOST_AUTH_KEY, secROM.pKeys[ HOST_AUTH_KEY - FIRST_STORED_KEY ].key, TempKeyNonce, TempKeyGenDig );

            if ( ECC108_SUCCESS == ret_code )
            {
               /* Execute MAC( mode = 0x41, key ID = HOST_AUTH_KEY */
               ret_code = MACResponse( MAC_MODE_INCLUDE_SN | MAC_MODE_BLOCK2_TEMPKEY, HOST_AUTH_KEY, ECC108_KEY_SIZE,
                           TempKeyGenDig, NULL, mac_resp );   /* Compute Response *//*lint !e473 NULL ptr OK here.   */

            }
#if ( VERBOSE != 0 )
            else
            {
               DBG_logPrintf( 'E', "ecc108e_SecureRandom failed GenDigTempKey operation: 0x%02x\n", ret_code );
            }
#endif
         }
#if ( VERBOSE != 0 )
         else
         {
           DBG_logPrintf( 'E', "ecc108e_SecureRandom failed NonceTempKey operation: 0x%02x\n", ret_code );
         }
#endif
         /* Set or invalidate RNGseed based upon results of operations  */
         if ( ECC108_SUCCESS == ret_code )
         {
            (void)memcpy( RNGseed, TempKeyGenDig, sizeof( RNGseed ) );  /*lint !e645 TemKeyGenDig been set if success */
         }
         else
         {
            (void)memset( RNGseed, 0, sizeof( RNGseed ) );
#if ( VERBOSE != 0 )
            DBG_logPrintf( 'E', "ecc108e_SecureRandom failed MACResponse operation: 0x%02x\n", ret_code );
#endif
         }
         ecc108_close();
         if ( ECC108_SUCCESS == ret_code )
         {
            break;
         }
      }
   }
#if ( VERBOSE > 1 )
      DBG_logPrintf( 'E', "ecc108e_SecureRandom returns: 0x%08x\n", ret_code == ECC108_SUCCESS ? RNGseed : NULL  );
#endif
   return ( ret_code == ECC108_SUCCESS ? RNGseed : NULL );
}

/**************************************************************************************************
   Function:   ecc108e_Random
   Purpose:    External interface to request random number generated by security device

   Arguments:  bool     seed     - True -> update seed; false -> don't update seed
               uint8_t  len      - Number of bytes desired
               uint8_t *result   - Destination of new random number

   Return - success/failure populates result
   Side effect:   Updates LastRandom
**************************************************************************************************/
uint8_t ecc108e_Random( bool seed, uint8_t len, uint8_t *result )
{
   uint8_t  randomMsg[ ECC108_KEY_SIZE ];
   uint8_t ret_code;

   ret_code = random( seed, randomMsg );
   if( ECC108_SUCCESS == ret_code )
   {
      (void)memcpy( result, randomMsg, min( len, ECC108_KEY_SIZE ) );
   }
   return ret_code;
}

/**************************************************************************************************
   Function:   ecc108e_Sign
   Purpose:    External interface to request sign a message

   Arguments:  uint16_t  len     - Number of bytes desired
               uint8_t  *msg     - message to sign
               uint8_t  *result  - Destination of signature

   Return - success/failure populates result
   Note: eccx08 device must NOT be opened.
   Side effect:
**************************************************************************************************/
uint8_t ecc108e_Sign( uint16_t len, uint8_t const *msg, uint8_t signature[ VERIFY_256_SIGNATURE_SIZE ] )
{
   uint8_t        ret_code = ECC108_GEN_FAIL;

   /* Attempt this a few times as the device may time out.  */
   for ( uint8_t i = 0; i < ECCX08_RETRIES; i++ )
   {
      if ( ECC108_SUCCESS == ecc108_open() )
      {
         ret_code = ecc108e_SignMessage( DEV_PRI_KEY_0, len, msg, (bool)false, signature );
         ecc108_close();
      }
      if ( ECC108_SUCCESS == ret_code )
      {
         break;
      }
   }
   return ret_code;
}

#if 0
/**************************************************************************************************

   Function: compressKeyData
   Purpose: buffer_t pointer to key data, compressed by excluding pad bytes.

   Arguments:  uint8_t  *keySize - returned size of key

   Note: Caller MUST free the returned buffer!!!!

   Return - buffer_t *

**************************************************************************************************/
static buffer_t *compressKeyData( const uint8_t *key )
{
   buffer_t *keyBuf;
   uint8_t  *keyData;

   keyBuf = BM_alloc( VERIFY_256_KEY_SIZE );
   if ( ( keyBuf != NULL ) && ( key != NULL ) )
   {
      keyData = keyBuf->data;
      ( void )memset( keyData, 0, VERIFY_256_KEY_SIZE );
      ( void )memcpy( keyData +  0, key +  4, ECC108_KEY_SIZE );
      ( void )memcpy( keyData + 32, key + 40, ECC108_KEY_SIZE );
   }

   return keyBuf;
}
#endif

/**************************************************************************************************

   Function: expandKeyData
   Purpose: buffer_t pointer to key data, expanded to include pad bytes, if necessary.

   Arguments:  uint16_t  keyID - data slot to use
               uint8_t  *keySize - returned size of key
   Note: if keySize is not needed, pass a NULL.
   Note: Caller MUST free the returned buffer!!!!

   Note: this function is only used for debugging and the project shouldn't be released with this enabled.

   Return - buffer_t *

**************************************************************************************************/
#if DO_CHECK_MAC
static buffer_t *expandKeyData( uint16_t keyID, uint8_t *keySize )
{
   buffer_t *keyBuf;
   uint8_t  *keyData;

   if ( ( keyID <= LAST_STORED_KEY  )  )  /* Validate keyID as being in range.   */
   {
      if ( keyID < FIRST_SIGNATURE_KEY )  /* Lower slots are 32 byte keys.       */
      {
         keyBuf = BM_alloc( ECC108_KEY_SIZE );
         if ( keyBuf != NULL )
         {
            keyData = keyBuf->data;
            ( void )memcpy( keyData, secROM.pKeys[ keyID - FIRST_STORED_KEY ].key, ECC108_KEY_SIZE );
            if ( keySize != NULL )
            {
               *keySize = ECC108_KEY_SIZE;
            }
         }
      }
      else  /* Upper slots are 64 byte keys and must be expanded to 72 bytes. */
      {
         keyBuf = BM_alloc( VERIFY_283_KEY_SIZE );
         if ( keyBuf != NULL )
         {
            keyData = keyBuf->data;
            ( void )memset( keyData, 0, VERIFY_283_KEY_SIZE );
            ( void )memcpy( keyData +  4, secROM.sigKeys[ keyID - FIRST_SIGNATURE_KEY ].key, ECC108_KEY_SIZE );
            ( void )memcpy( keyData + 40, secROM.sigKeys[ keyID - FIRST_SIGNATURE_KEY ].key + 32, ECC108_KEY_SIZE );

            if ( keySize != NULL )
            {
               *keySize = VERIFY_256_KEY_SIZE;
            }
         }
      }
   }
   return keyBuf;
}
#endif

/**************************************************************************************************
   Function:   ecc108e_EncryptedKeyWrite
   Purpose:    Write new key to requested slot

   Arguments:  uint16_t keyID    - Key to be updated
               uint8_t  *key     - pointer to key data. If NULL, key generated internally
               uint8_t  keyLen   - if key data provided, length of key

   Steps:   NonceTempKey()
            GenDigTempKey()
            Value = TempKey XOR key (new key value)
            GenDig( mode 2, keyID = KEY_UPDATE_WRITE_KEY (6) )
            Compute MAC value (sha256 operations)
            Write( zone = 0x82, keyID HOST_AUTH_KEY (4), MAC )
            Store results (non-volatile memory)
   Note: eccx08 device must NOT be opened.
   Return - success/failure of operation
   Side effect: Changes key in slot keyID
**************************************************************************************************/
uint8_t ecc108e_EncryptedKeyWrite( uint16_t keyID, uint8_t *key, uint8_t keyLen )
{
#if DO_CHECK_MAC == 1
   uint8_t                 OTP[8];                             /* Holds OTP[0:7]  */
   uint8_t                 ClientResp[SHA256_DIGEST_SIZE];     /* Computed client response for CHECKMAC command   */
   buffer_t                *expandedKey;
#endif
#if ( ( DO_CHECK_INFO == 1 ) || ( DO_CHECK_MAC == 1 ) )
   uint8_t                 command[CHECKMAC_COUNT];
   uint8_t                 response[ECC108_RSP_SIZE_32];       /* buffer large enough for all responses in this function */
#endif
   uint8_t                 numin[25];     /* Used in TempKey calculation - 25B 0       */
   uint8_t                 extra[7];      /* Additional data used in MAC computation   */
   buffer_t                *tk;           /* buffer for TempKey   */
   buffer_t                *val = NULL;   /* buffer for Value     */
   buffer_t                *sh = NULL;    /* buffer for sha     */
   buffer_t                *m = NULL;     /* buffer for mac     */
   buffer_t                *k = NULL;     /* buffer for kuwk     */
   buffer_t                *compressKey = NULL;
   uint8_t                 *TempKey;      /* Storage for sha-256                       */
   uint8_t                 *Value;        /* Encrypted new secret key                  */
   uint8_t                 *kuwk;         /* Key Update Write Key - read from device   */
   uint8_t                 *mac;          /* Computed MAC digest                       */
   Sha256                  *sha;          /* sha256 working area  */
   uint8_t                 *newKey;
   uint8_t                 *locKey;
   PartitionData_t const   *pPart;        /* Used to access the security info partition  */
   uint16_t                crc;
   uint8_t                 keySize;
   uint8_t                 bytesToWrite;
   uint8_t                 ret_code = ECC108_GEN_FAIL;

   /* To reduce stack usage, allocate buffers for the large variables needed by this routine.   */
   tk = BM_alloc( SHA256_DIGEST_SIZE );
   if ( tk != NULL )
   {
      TempKey = tk->data;
      val = BM_alloc( ECC108_KEY_SIZE );
      if ( val != NULL )
      {
         Value = val->data;
         sh = BM_alloc( sizeof( Sha256 ) );
         if ( sh != NULL )
         {
            sha = ( Sha256 * )( void * )sh->data;
            m = BM_alloc( ECC108_KEY_SIZE );
            if ( m != NULL )
            {
               mac = m->data;
               k = BM_alloc( ECC108_KEY_SIZE );
               if ( k != NULL )
               {
                  kuwk = k->data;
                  ret_code = ECC108_SUCCESS;
               }
            }
         }
      }
   }
   if ( ret_code == ECC108_SUCCESS )
   {
      /*lint --e{644,613} all buffers/pointers initialized and NOT NULL  */
      ret_code = ECC108_GEN_FAIL;
      pPart = SEC_GetSecPartHandle();              /* Obtain handle to access internal NV variables   */
      (void)wc_InitSha256( sha );     /* Only needed once - Sha256Final calls this as its last step  */
      (void)memset( numin, 0, sizeof( numin ) );

      if ( key != NULL )   /* If key data passed in, use it.   */
      {
         newKey = key;
         keySize = min( keyLen, VERIFY_283_KEY_SIZE );
         ret_code = ECC108_SUCCESS;
      }
      else     /* Key is NULL; generate new one (either random, or regenerate default based on flash security mode. */
      {
#if 0 // TODO: RA6E1 Flash security to be enabled
         if ( NV_FSEC_SEC( NV_FSEC ) != 2 )     /* Value of 2 means unsecured; all others secured. */
#else
         if ( 0 )
#endif
         {
            /* ecc108e_SecureRandom does an open/close operation. Move before the open in this routine   */
            newKey = ecc108e_SecureRandom();       /* Generate new secret key */
            if ( newKey != NULL )
            {
               ret_code = ECC108_SUCCESS;
               keySize = ECC108_KEY_SIZE;
            }
         }
         else  /* Flash security off - use default keys. */
         {
            newKey = getKeyData( keyID, NULL ); /* To force rewriting factory default values */
            if ( newKey != NULL )
            {
               /* Get pointer to intROM corresponding to keyID */
               newKey = ( uint8_t * )intROM.pKeys[ 0 ].key + ( newKey - secROM.pKeys[ 0 ].key );
               keySize = ECC108_KEY_SIZE;    /* Restored default keys are all 32 bytes long. */
               ret_code = ECC108_SUCCESS;
            }
         }
      }
      if ( ECC108_SUCCESS == ret_code )   /* newKey is assigned and NOT NULL at this point.  */
      {
         /*lint --e{644,613} all buffers/pointers initialized and NOT NULL  */
         uint16_t writeKey;
         locKey = newKey;

         /* Read key needed to allow updating keyID  */
         writeKey = slotcfg[ keyID ].writeKey;
         DBG_logPrintf( 'I', "ecc108e_EncryptedKeyWrite: slotcfg[ %d ].encryptRead = %d", writeKey, slotcfg[ writeKey ].encryptRead );
         if ( ( writeKey != 0 ) && ( writeKey < ARRAY_IDX_CNT( slotcfg ) ) && ( slotcfg[ writeKey ].encryptRead ) )
         {
            ret_code = ecc108e_EncryptedKeyRead( writeKey, kuwk );
         }
         else
         {
            uint8_t *romKey;
            romKey = getKeyData( writeKey, NULL );
            if ( romKey != NULL )
            {
               (void)memcpy( kuwk, romKey, ECC108_KEY_SIZE );
            }
            else
            {
               ret_code = ECC108_GEN_FAIL;
            }
         }
         if (ret_code == ECC108_SUCCESS)
         {
#if (VERBOSE > 0)
            printBuf( "New Key", &keyID, locKey, ECC108_KEY_SIZE );
#endif
            /* locKey is the candidate for the new key. Write it to secROM.replKeys array, but with an invalid crc. After the key is
               successfully written to the device, update the replCRC array. This will cause the replacement key to be copied to the
               appropriate key in secROM (pKeys or sigKeys).   */

            dSize    keyOffset;
            uint8_t  *replacementKey;

            crc = 0;
            replacementKey = getKeyData( keyID, NULL );  /* Find offset into secROM for keyID.  */

            /* Find offset from corresponding pKeys to replPrivKeys */
            keyOffset = (dSize)( offsetof( intFlashNvMap_t, replPrivKeys ) + ( replacementKey - secROM.pKeys[ 0 ].key ) );/*lint !e613 replacementKey is not NULL if key is NOT NULL. */

            do
            {
#if ( EP == 1 )
               PWR_lockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails */
#else
               PWR_lockMutex(); /* Function will not return if it fails */
#endif
               /* Invalidate replacement key crc.  */
               ret_code = (uint8_t)PAR_partitionFptr.parWrite( (dSize)offsetof( intFlashNvMap_t, replKeyCRC ) + \
                                                               (lCnt)( keyID - FIRST_STORED_KEY ) * sizeof( crc ),
                                                               (uint8_t *)&crc, (lCnt)sizeof( crc ), pPart );
               if ( ret_code == ECC108_SUCCESS )
               {
                  /* Update replacement key with potential replacement key.   */
                  if ( ( key != NULL ) && ( keyLen == VERIFY_283_KEY_SIZE ) )
                  {
                     if ( compressKey == NULL ) /* Make sure we don't leak buffers if multiple passes through this do loop required.   */
                     {
                        compressKey = BM_alloc( VERIFY_256_KEY_SIZE );
                     }

                     if ( compressKey != NULL )
                     {
                        uint8_t *keyData;
                        keyData = compressKey->data;
                        ( void )memset( keyData, 0, VERIFY_256_KEY_SIZE );
                        ( void )memcpy( keyData +  0, key +  4, ECC108_KEY_SIZE );
                        ( void )memcpy( keyData + 32, key + 40, ECC108_KEY_SIZE );

                        ret_code = (uint8_t)PAR_partitionFptr.parWrite( keyOffset, keyData, VERIFY_256_KEY_SIZE, pPart );
                     }
                  }
                  else
                  {
                     ret_code = (uint8_t)PAR_partitionFptr.parWrite( keyOffset, locKey, keySize, pPart );      /*lint !e644 keySize set if ret_code == ECC108_SUCCESS   */
                  }
               }
#if ( EP == 1 )
               PWR_unlockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails  */
#else
               PWR_unlockMutex(); /* Function will not return if it fails  */
#endif
            } while ( ret_code != ECC108_SUCCESS );

            /* Prepare CRC for validation after successful update to device.  */
            if ( ( key != NULL ) && ( keyLen > ECC108_KEY_SIZE ) && ( compressKey != NULL ))
            {
               CRC_ecc108_crc( VERIFY_256_KEY_SIZE,/* Number of bytes   */
                               compressKey->data,  /* Source data       */
                               (uint8_t *)&crc,    /* Resulting crc     */
                               0,                  /* Seed value        */
                               (bool)false );      /* Don't hold the CRC engine  */
            }
            else
            {
               CRC_ecc108_crc( keySize,         /* Number of bytes   */
                               locKey,          /* Source data       */
                               (uint8_t *)&crc, /* Resulting crc     */
                               0,               /* Seed value        */
                               (bool)false );   /* Don't hold the CRC engine  */

            }
            /* For keys longer than ECC108_KEY_SIZE, loop through the portion of the flow from key authorization to
               key authorization.
            */
            uint8_t block = 0;

            bytesToWrite = keySize;

            /*******************************************************************************************
               This is a critical section; once we start updating a key in the chip, we must finish!
            *******************************************************************************************/
#if ( EP == 1 )
            PWR_lockMutex( PWR_MUTEX_ONLY );
#else
            PWR_lockMutex();
#endif

            DBG_logPrintf( 'I', "ecc108e_EncryptedKeyWrite: keycfg[ %d ].reqAuth = %d, .authKey = %d", writeKey, keycfg[ writeKey ].reqAuth, keycfg[ writeKey ].authKey );
            ret_code = ecc108_open();
            if ( ret_code == ECC108_SUCCESS )
            {
               /*lint --e{613} --e{644} pointer are set and not NULL if ret_code == ECC108_SUCCESS.  */
               do
               {
                  if( ( writeKey != 0 ) && ( writeKey < ARRAY_IDX_CNT( slotcfg ) ) && ( keycfg[ writeKey ].reqAuth ) )
                  {
                     ret_code = ecc108e_KeyAuthorization( (uint8_t)keyID, secROM.pKeys[ ( keycfg[ writeKey ].authKey ) - FIRST_STORED_KEY ].key );
                  }
                  if ( ret_code == ECC108_SUCCESS )
                  {
                     /* Execute NONCE and recreate resulting TempKey */
                     ret_code = NonceTempKey( NONCE_MODE_SEED_UPDATE, TempKey ); /* Calc TempKey from Nonce cmd   */

                     if ( ret_code == ECC108_SUCCESS )
                     {
                        /* Execute GENDIG with the writeKey and recreate TempKey in device resulting from GenDig command: */
                        ret_code = GenDigTempKey( writeKey, kuwk, TempKey, TempKey );  /*lint !e644 !e645 kuwk is set if ret_code == ECC108_SUCCESS  */

                        if ( ret_code == ECC108_SUCCESS )
                        {
                           /* Encrypt value before writing to device   */
                           for( uint32_t i = 0; i < ECC108_KEY_SIZE; i++ )
                           {
                              Value[ i ] = (uint8_t)(TempKey[ i ] ^ locKey[ i ]);   /*lint !e613 locKey CANNOT be NULL here */
                           }

                           /* Compute MAC */
                           (void)wc_Sha256Update( sha, TempKey, ECC108_KEY_SIZE );
                           extra[ 0 ] = ECC108_WRITE;
                           extra[ 1 ] = ECC108_ZONE_COUNT_FLAG | ECC108_ZONE_DATA;
                           *(uint16_t *)&extra[ 2 ] = (uint16_t)(keyID << 3) | ( (uint16_t)block << 8 );/*lint !e2445 !e826 stuffing uint16_t into 2 uint8_t locations ok. */
                           extra[ 4 ] = SerialNumber[ 8 ];
                           extra[ 5 ] = SerialNumber[ 0 ];
                           extra[ 6 ] = SerialNumber[ 1 ];
                           (void)wc_Sha256Update( sha, extra, sizeof( extra ) );   /* Opcode, zone, key address, SN8, SN0,
                                                                                       SN1*/
                           (void)wc_Sha256Update( sha, numin, sizeof( numin ) );   /* 25B zeros   */
                           (void)wc_Sha256Update( sha, locKey, ECC108_KEY_SIZE );  /* New secret key */
                           (void)wc_Sha256Final( sha, mac );    /* shaSum = mac needed for write command   */
#if DO_CHECK_INFO == 1
                           /* Diagnostic - check key Authorization state   */
                           infoCommand( &writeKey );

#endif
#if DO_CHECK_MAC == 1
                           (void)memset( OTP, 0, sizeof( OTP ) );
                           (void)memset( numin, 0, sizeof( numin ) );
                           expandedKey = expandKeyData( writeKey, NULL );
                           (void)wc_Sha256Update( sha, expandedKey->data + ( block * ECC108_KEY_SIZE ),
                                                   ECC108_KEY_SIZE );                  /* sha256 on key data */
                           BM_free( expandedKey );
                           (void)wc_Sha256Update( sha, TempKey, SHA256_DIGEST_SIZE );  /* sha256 on TempKey   */
                           (void)wc_Sha256Update( sha, numin, 4 );                     /* sha256 on OtherData[0:3] (0s) */
                           (void)wc_Sha256Update( sha, OTP, sizeof( OTP ) );           /* sha256 on 8 bytes of 0 */
                           (void)wc_Sha256Update( sha, numin, 3 );                     /* sha256 on OtherData[4:6] (0s) */
                           (void)wc_Sha256Update( sha, &SerialNumber[8], 1 );          /* sha256 on SN[8]   */
                           (void)wc_Sha256Update( sha, numin, 4 );                     /* sha256 on OtherData[7:10] (0s)   */
                           (void)wc_Sha256Update( sha, &SerialNumber[0], 2 );          /* sha256 on SN[0:1]   */
                           (void)wc_Sha256Update( sha, numin, 2 );                     /* sha256 on OtherData[11:12] (0s)  */
                           (void)wc_Sha256Final(  sha, ClientResp );                   /* Retrieve the sha256 results   */

                           (void)memset( numin, 0, CHECKMAC_OTHER_DATA_SIZE );
                           printBuf( "CHECKMAC: ", NULL, ClientResp, sizeof( ClientResp ) );
                           ret_code = ecc108m_execute(ECC108_CHECKMAC, MAC_MODE_BLOCK2_TEMPKEY, writeKey,
                                             CHECKMAC_CLIENT_CHALLENGE_SIZE, response, /* 32 bytes ignored        */
                                             sizeof( ClientResp ), ClientResp,         /* ClientResp from sha256  */
                                             CHECKMAC_OTHER_DATA_SIZE, numin,          /* 13 bytes of 0           */
                                             CHECKMAC_COUNT, command,
                                             READ_4_RSP_SIZE, response);
                           printBuf( "checkmac", &writeKey, response, READ_4_RSP_SIZE );
#endif
                           ret_code = ecc108e_write_key( keyID, ECC108_KEY_SIZE, Value, block, mac );
                           if (ret_code == ECC108_SUCCESS)
                           {
                              if ( keyID == HOST_AUTH_KEY )    /* Per End Point Security Write Key flow from T. Dierking.   */
                              {
                                 ret_code = ecc108e_KeyAuthorization( (uint8_t)keyID, locKey );
                              }
                              block++;
                              bytesToWrite -= min( bytesToWrite, ECC108_KEY_SIZE );
                              locKey += ECC108_KEY_SIZE; /*lint !e613 locKey cannot be NULL here.  */
                           }
                        }
                     }
                  }
               } while ( ( bytesToWrite != 0 ) && ( ECC108_SUCCESS == ret_code ) );
               ecc108_close();
            }

            if (ret_code == ECC108_SUCCESS)
            {
               /* Write the replKeyCRC */
               do
               {
                  ret_code = (uint8_t)PAR_partitionFptr.parWrite( (dSize)offsetof( intFlashNvMap_t, replKeyCRC ) + \
                                                                  (lCnt)( keyID - FIRST_STORED_KEY ) * sizeof( crc ),
                                                                  (uint8_t *)&crc, (lCnt)sizeof( crc ), pPart );
               } while ( ret_code != ECC108_SUCCESS );
            }
#if ( EP == 1 )
            PWR_unlockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails  */
#else
            PWR_unlockMutex(); /* Function will not return if it fails  */
#endif

            if (ret_code == ECC108_SUCCESS)
            {
               ecc108e_UpdateKeys( &intROM ); /* Copy valid secROM.replacement keys to secROM.pKeys  */
            }
         }
      }
   }

   /* Free all of the buffers allocated above.  */
   if ( compressKey != NULL )
   {
      BM_free( compressKey );
   }
   if ( tk != NULL )
   {
      BM_free( tk );
   }
   if ( val != NULL )
   {
      BM_free( val );
   }
   if ( sh != NULL )
   {
      BM_free( sh );
   }
   if ( m != NULL )
   {
      BM_free( m );
   }
   if ( k != NULL )
   {
      BM_free( k );
   }
   return ret_code;
}

/**************************************************************************************************
   Function:   ecc108e_DeriveFWDecryptionKey
   Purpose:    Derive key needed to decrypt firwware patches

   Arguments: uint8_t fwen[ 32 ] - FW Encryption Number
              uint8_t fwdk[ 32 ] - FW Decryption key

   Steps:   Authorize HOST_AUTH_KEY
            Run MAC on fwen
            Sha256 on AclaraFWkey and response from MAC command - result to fwdk

   Note: eccx08 device must NOT be opened.
   Return - populates fwdk
   Side effect:
**************************************************************************************************/
uint8_t ecc108e_DeriveFWDecryptionKey( uint8_t fwen[ ECC108_KEY_SIZE ], uint8_t fwdk[ ECC108_KEY_SIZE ] )
{
   uint8_t  response[ MAC_RSP_SIZE ];  /* buffer large enough for all responses in this function   */
   uint8_t  command[ MAC_COUNT_LONG ]; /* buffer large enough for all commands  in this function   */
   Sha256   sha;                       /* sha256 working area  */
   uint8_t  ret_code;

   /* Attempt this a few times as the device may time out.  */
   for ( uint8_t i = 0; i < ECCX08_RETRIES; i++ )
   {
      ret_code = ecc108_open();
      if ( ret_code  == ECC108_SUCCESS )
      {
         /*lint -e{778} Info 778: Constant expression evaluates to 0 in operation '-' */
         ret_code = ecc108e_KeyAuthorization( HOST_AUTH_KEY, secROM.pKeys[ HOST_AUTH_KEY - FIRST_STORED_KEY ].key );
         if (ret_code == ECC108_SUCCESS)
         {
            ret_code = ecc108m_execute(   ECC108_MAC, MAC_MODE_CHALLENGE, NETWORK_AUTH_KEY,
                                          MAC_CHALLENGE_SIZE, fwen,        /* data1 size, data1 */
                                          0, NULL, 0, NULL,                /* No data2 or data3 */
                                          sizeof command, command,         /* Tx buffer   */
                                          sizeof( response ), response);
            if (ret_code == ECC108_SUCCESS)
            {
               /* Compute decryption key */
               (void)wc_InitSha256( &sha );
               (void)wc_Sha256Update( &sha, (uint8_t *)AclaraFWkey, sizeof( AclaraFWkey ) );
               (void)wc_Sha256Update( &sha, &response[ 1 ], ECC108_KEY_SIZE );
               (void)wc_Sha256Final( &sha, fwdk );    /* shaSum = decryption key */
            }
         }
         ecc108_close();
         if ( ECC108_SUCCESS == ret_code )
         {
            break;
         }
      }
   }
   return ret_code;
}

/**************************************************************************************************
   Function:   ecc108e_Verify
   Purpose:    Verify signature digest

   Arguments:  uint8_t keyID        - key (pair) to use in the process
               uint16_t msglen      - number of bytes in the message to be verified
               uint8_t *msg         - message to be verified
               uint8_t signature[ VERIFY_256_SIGNATURE_SIZE ] - signature to verify

   Steps:   Nonce(mode 3, zero = 0, 32B digest
            Verify command (mode 0, keyID, Signature )
            MAC command( mode = 0x45, keyID +1 )
            Sha256 keyID(data), message(digest),ECC108_VERIFY, 0x45, keyID, 11B 0, SN8, SN[4:7], SN[0:3])
            if result of Sha256 matches response from device, pass; otherwise fail.

   Return - populates fwdk
   Note: eccx08 device must not be opened.
   Side effect:
**************************************************************************************************/
uint8_t ecc108e_Verify( uint8_t keyID, uint16_t msglen, uint8_t const *msg,
                                 uint8_t signature[ VERIFY_256_SIGNATURE_SIZE ] )
{
   buffer_t *tk;           /* buffer for TempKey   */
   buffer_t *cmd = NULL;   /* buffer for command   */
   buffer_t *sh = NULL;    /* buffer for sha   */
   buffer_t *m = NULL;     /* buffer for mac   */
   buffer_t *rsp = NULL;   /* buffer for response   */
   uint8_t  *TempKey;      /* TempKey computed from NONCE command                      */
   uint8_t  *command;      /* buffer large enough for all commands  in this function   */
   Sha256   *sha;          /* sha256 working area                                      */
   uint8_t  *mac;          /* MAC computed by MAC command                              */
   uint8_t  *response;     /* buffer large enough for all responses in this function   */
   uint8_t  ret_code = ECC108_GEN_FAIL;

   /* To reduce stack usage, allocate buffers for the large variables needed by this routine.   */
   tk = BM_alloc( SHA256_DIGEST_SIZE );
   if ( tk != NULL )
   {
      TempKey = tk->data;
      cmd = BM_alloc( VERIFY_256_STORED_COUNT );
      if ( cmd != NULL )
      {
         command = cmd->data;
         sh = BM_alloc( sizeof( Sha256 ) );
         if ( sh != NULL )
         {
            sha = ( Sha256 * )( void * )sh->data;
            m = BM_alloc( ECC108_KEY_SIZE );
            if ( m != NULL )
            {
               mac = m->data;
               rsp = BM_alloc( NONCE_RSP_SIZE_LONG );
               if ( rsp != NULL )
               {
                  response = rsp->data;
                  ret_code = ECC108_SUCCESS;
               }
            }
         }
      }
   }

   if ( ret_code == ECC108_SUCCESS )      /* All buffer allocations succeeded.   */
   {
      /*lint --e{644,613} all buffers/pointers initialized and NOT NULL  */
      (void)wc_InitSha256( sha );
      (void)wc_Sha256Update( sha, msg, msglen );
      (void)wc_Sha256Final( sha, TempKey );    /* data to send with NONCE command   */

#if (VERBOSE > 1)
         printBuf( "TempKey", NULL, TempKey, ECC108_KEY_SIZE );
#endif
      /* Attempt this a few times as the device may time out.  */
      for ( uint8_t i = 0; i < ECCX08_RETRIES; i++ )
      {
         ret_code = ecc108_open();
         if ( ret_code  == ECC108_SUCCESS )
         {
            ret_code = ecc108m_execute(ECC108_NONCE, NONCE_MODE_PASSTHROUGH, 0,
                                          SHA256_DIGEST_SIZE, TempKey,     /* data1 size, data1 */
                                          0, NULL, 0, NULL,                /* No data2 or data3 */
                                          NONCE_COUNT_LONG, command,       /* Tx buffer   */
                                          NONCE_RSP_SIZE_SHORT, response); /* Rx buffer   */
            if (ret_code == ECC108_SUCCESS)
            {
               ret_code = ecc108m_execute(ECC108_VERIFY, VERIFY_MODE_STORED, keyID,
                                             VERIFY_256_SIGNATURE_SIZE, signature,  /* data1 size, data1 */
                                             0, NULL, 0, NULL,                      /* No data2 or data3 */
                                             VERIFY_256_STORED_COUNT, command,      /* Tx buffer   */
                                             ECC108_RSP_SIZE_MIN, response);        /* Rx buffer   */

               if (ret_code == ECC108_SUCCESS)
               {
                  if ( response[ 1 ] == 0 )
                  {
#if 1
                     ret_code = ecc108m_execute(ECC108_NONCE, NONCE_MODE_PASSTHROUGH, 0,
                                                   SHA256_DIGEST_SIZE, TempKey,     /* data1 size, data1 */
                                                   0, NULL, 0, NULL,                /* No data2 or data3 */
                                                   NONCE_COUNT_LONG, command,       /* Tx buffer   */
                                                   NONCE_RSP_SIZE_SHORT, response); /* Rx buffer   */
#endif
                     if (ret_code == ECC108_SUCCESS)
                     {
                        /* Compute Response */
#if 0
                        ret_code = MACResponse( MAC_MODE_INCLUDE_SN | MAC_MODE_SOURCE_FLAG_MATCH | MAC_MODE_BLOCK2_TEMPKEY,
                                          keyID + 1, ECC108_KEY_SIZE, TempKey, NULL, mac );
#else
                        /* Use a challenge with TemKey, instead of a key with TempKey  */
                        ( void )memset( sha->buffer, 0, ECC108_KEY_SIZE );
                        ret_code = MACResponse( MAC_MODE_INCLUDE_SN | MAC_MODE_SOURCE_FLAG_MATCH | MAC_MODE_BLOCK1_TEMPKEY,
                                          keyID + 1, ECC108_KEY_SIZE, TempKey, (uint8_t *)sha->buffer, mac );
#endif
                        if (ret_code != ECC108_SUCCESS)
                        {
                           ECC108_PRNT_ERROR( 'E', "%s MacResponse failed, ret_code: 0x%02x", __func__, ret_code );
                        }
                     }
                     else
                     {
                        ECC108_PRNT_ERROR( 'E', "%s 2nd Nonce failed, ret_code: 0x%02x", __func__, ret_code );
                     }
                  }
                  else
                  {
                     ret_code = ECC108_GEN_FAIL;
                  }
               }
               else
               {
                  ECC108_PRNT_ERROR( 'E', "%s verify failed, ret_code: 0x%02x", __func__, ret_code );
               }

            }
            else
            {
               ECC108_PRNT_ERROR( 'E', "%s 1st Nonce failed, ret_code: 0x%02x", __func__, ret_code );
            }
            ecc108_close();
         }
         if ( ECC108_SUCCESS == ret_code )
         {
            break;
         }
      }
   }
   if ( tk != NULL )
   {
      BM_free( tk );
   }
   if ( cmd != NULL )
   {
      BM_free( cmd );
   }
   if ( sh != NULL )
   {
      BM_free( sh );
   }
   if ( m != NULL )
   {
      BM_free( m );
   }
   if ( rsp != NULL )
   {
      BM_free( rsp );
   }

   return ret_code;
}

/**************************************************************************************************

   Function: ecc108e_GetDeviceCert
   Purpose:  Return requested Device Certificate to caller

   Arguments: enum Cert_e - which cert is desired
              uint8_t *cert - Destination of requested cert

   Note: eccx08 device must NOT be opened.
   Return - success/failure of operation

**************************************************************************************************/
uint8_t ecc108e_GetDeviceCert( Cert_type cert, uint8_t *dest, uint32_t *length )
{
         buffer_t    *bc;                       /* buffer for builtCert                      */
         buffer_t    *k8 = NULL;                /* buffer for key8                           */
         buffer_t    *k9 = NULL;                /* buffer for key9                           */
         buffer_t    *k10 = NULL;               /* buffer for key10                          */
         CertDef     *builtCert;                /* Space for re-built certificates           */
         Certificate *key8;                     /* Data from slot 8  of the device           */
         uint8_t     *key9;                /* Data from slot 9  of the device           */
         uint8_t     *key10;               /* Data from slot 10 of the device           */
   const uint8_t     *pCert         = NULL;     /* Pointer to next byte in cert              */
   const uint8_t     *certBase;                 /* Pointer to beginning of cert              */
         uint32_t    certLen;                   /* Computed length of cert for copying       */
         uint32_t    maxCertLen = 0;            /* Maximum allowed cert length, based on request type */
         uint8_t     numBytes;                  /* Use to compute length of cert for copying */
         uint8_t     ret_code;                  /* Device interface command results          */

   ret_code = ECC108_GEN_FAIL;
   /* To reduce stack usage, allocate buffers for the large variables needed by this routine.   */
   bc = BM_alloc( sizeof( CertDef ) );
   if ( bc != NULL )
   {
      builtCert = ( CertDef * )( void * )bc->data;
      k8 = BM_alloc( sizeof( Certificate ) );
      if ( k8 != NULL )
      {
         key8 = ( Certificate * )( void * )k8->data;
         k9 = BM_alloc( VERIFY_283_KEY_SIZE );
         if ( k9 != NULL )
         {
            key9 = k9->data;
            k10 = BM_alloc( VERIFY_283_KEY_SIZE );
            if ( k10 != NULL )
            {
               key10 = k10->data;
               ret_code = ECC108_SUCCESS;
            }
         }
      }
   }
   if ( ret_code == ECC108_SUCCESS )
   {
      /*lint --e{644,613} all buffers/pointers initialized and NOT NULL  */
      /* Attempt this a few times as the device may time out.  */
      for ( uint8_t i = 0; i < ECCX08_RETRIES; i++ )
      {
         ret_code = ecc108_open();
         if ( ECC108_SUCCESS == ret_code )
         {
            switch ( cert )
            {
               case AclaraPublicRootCert_e:
               {
                  pCert = (uint8_t *)AclaraPublicRootCert;
                  maxCertLen = sizeof( AclaraPublicRootCert );
                  break;
               }
               case SignerCert_e:
               {
                  (void)memset( (uint8_t *)builtCert, 0, sizeof( *builtCert ) ); /*lint !e644 builCert initialized if ret_code == ECC108_SUCCESS   */
                  ret_code = ecc108e_read_key( CERT, sizeof( *key8 ), 0, (uint8_t *)key8 );  /*lint !e644 key8 initialized if ret_code == ECC108_SUCCESS   */
                  if ( ECC108_SUCCESS == ret_code  )
                  {
                     ret_code = ecc108e_read_key( SIGNER_PUB_KEY, VERIFY_283_KEY_SIZE, 0, key9 );
                     if ( ECC108_SUCCESS == ret_code  )
                     {
                        /*lint -e{603} key10 init'd by buildSignerCert  */
                        (void)buildSignerCert( (uint8_t *)key8, key10, DeviceRootCAPublicKey, builtCert );
                        pCert = (uint8_t *)builtCert;
                        maxCertLen = MAX_CERT_LEN;
                     }
                  }
                  break;
               }
               case DeviceCert_e:
               {
                  (void)memset( (uint8_t *)builtCert, 0, sizeof( *builtCert ) );
                  ret_code = ecc108e_read_key( CERT, sizeof( *key8 ), 0, (uint8_t *)key8 );
                  if ( ECC108_SUCCESS == ret_code  )
                  {
                     ret_code = ecc108e_read_key( DEV_PUB_KEY, VERIFY_283_KEY_SIZE, 0, key9 );
                     if ( ECC108_SUCCESS == ret_code  )
                     {
                        ret_code = ecc108e_read_key( SIGNER_PUB_KEY, VERIFY_283_KEY_SIZE, 0, key10 );
                        if ( ECC108_SUCCESS == ret_code  )
                        {
                           int16_t certSuccess;
#if (VERBOSE > 2)
                           printBuf( "key 8",   NULL,  (uint8_t *)key8,    sizeof( *key8) );
                           printBuf( "key 9",   NULL,             key9,    VERIFY_283_KEY_SIZE );
                           printBuf( "key 10",  NULL,             key10,   VERIFY_283_KEY_SIZE );
#endif
                           /*lint -e{613} *//* pointers not NULL if ret_code == ECC108_SUCCESS  */
                           certSuccess = buildDeviceCert( (uint8_t *)key8, key9, key10, builtCert);
                           if ( certSuccess != 0 )
                           {
                              DBG_logPrintf( 'E', "buildDeviceCert failed: %d\n", certSuccess );
                              ret_code = ECC108_GEN_FAIL;
                           }
                           else
                           {
                              pCert = (uint8_t *)builtCert;
                              maxCertLen = MAX_CERT_LEN;
                           }
                        }
                     }
                  }
                  break;
               }
               case dtlsNetworkRootCA_e:
               {
                  pCert = secROM.dtlsNetworkRootCA.NetWorkRootCA;
                  maxCertLen = MAX_CERT_LEN;
                  break;
               }
               case dtlsHESubject_e:
               {
                  pCert = secROM.dtlsHESubject.partialCert;
                  maxCertLen  = sizeof ( HESubject_t );
                  break;
               }
               case dtlsMSSubject_e:
               {
                  pCert = secROM.dtlsMSSubject.partialCert;
                  maxCertLen  = sizeof ( MSSubject_t );
                  break;
               }
               default:
               {
                  break;
               }
            }
            ecc108_close();
         }
         if ( ECC108_SUCCESS == ret_code )
         {
            break;
         }
      }
      if ( pCert != NULL )       /* valid cert request   */
      {
         if ( (uint16_t)0xFFFF != *(uint16_t *)pCert )   /* check for erased Cert */
         {
            certBase = pCert;
            if ( ( *(certBase + 1 ) & 0x80 ) != 0 )   /* Multibyte length field  */
            {
               numBytes = *(certBase + 1 ) & ~0x80;   /* Pick up number of bytes in length   */
               pCert = certBase + 2 ;                 /* Point to start of length            */
            }
            else                                      /* Single byte length field   */
            {
               numBytes = 1;
               pCert = certBase + 1;                  /* Point to start of length            */
            }
            certLen = 0;
            while ( ( numBytes-- != 0 ) && ( certLen <= maxCertLen ) )    /* Loop through the length field of the cert */
            {
               certLen = ( certLen << 8 ) + *pCert++;
            }
            certLen += (uint32_t)( pCert - certBase ); /* Account for tag/tag-length fields */
            if ( certLen <= maxCertLen )
            {
               (void)memcpy( dest, certBase, certLen );
               if (NULL != length)
               {
                 *length = certLen;
               }
            }
            else
            {
               ret_code = ECC108_GEN_FAIL;
            }
         }
         else
         {
            ret_code = ECC108_GEN_FAIL;
         }
      }
   }
#if (VERBOSE > 2)
   printBuf( "Cert", (uint16_t *)&cert, dest, certLen );
#endif
   if ( bc != NULL )
   {
      BM_free( bc );
   }
   if ( k8 != NULL )
   {
      BM_free( k8 );
   }
   if ( k9 != NULL )
   {
      BM_free( k9 );
   }
   if ( k10 != NULL )
   {
      BM_free( k10 );
   }
   return ret_code;
}
