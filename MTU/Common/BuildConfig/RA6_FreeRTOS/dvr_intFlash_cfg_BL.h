/***********************************************************************************************************************
 *
 * Filename: dvr_intFlash_cfg.h
 *
 * Contents: defines internal flash memory organization
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2014-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author KAD
 *
 * $Log$ Created May 12, 2014
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - KAD 05/12/2014 - Initial Release
 *
 * @version    0.1
 * #since      2014-05-12
 **********************************************************************************************************************/
#ifndef dvr_intFlash_cfg_H_
#define dvr_intFlash_cfg_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>
#include "portable_freescale.h"

#ifndef __BOOTLOADER
#include "ecc108_comm_marshaling.h"    /* Needed for key sizes */
#include "MAC_Protocol.h"
#endif   /* BOOTLOADER  */

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef dvr_intFlash_GLOBAL
   #define dvr_intFlash_cfg_EXTERN
#else
   #define dvr_intFlash_cfg_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define MAX_COPY_RANGES          2        /* Maximum number of ranges to copy from NV to ROM.   */
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

#ifndef __BOOTLOADER
typedef struct
{
   uint8_t NetWorkRootCA[ 512 ];
} NetWorkRootCA_t;

typedef struct
{
   uint8_t partialCert[ 192 ];
} HESubject_t, MSSubject_t;

typedef struct
{
   uint8_t           key[ ECC108_KEY_SIZE ]; /* Private/secret keys  */
} privateKey;

typedef struct
{
   uint8_t           key[ VERIFY_256_KEY_SIZE ]; /* Private/Data/secret keys/signatures  */
} sigKey;

typedef struct
{
   privateKey        pKeys[5];               /* Private keys. Note, includes slot 8, which is larger, but not actually
                                                kept in code space. */
   sigKey            sigKeys[7];             /* keys used for signatures.                 */
   privateKey        replPrivKeys[5];        /* Replacement private keys                  */
   sigKey            replSigKeys[7];         /* Replacement signature keys                */
   uint16_t          keyCRC[12];             /* Used to verify secROM.pkeys on boot up    */
   uint16_t          replKeyCRC[12];         /* Used to verify secROM.ReplKeys on boot up */
   uint8_t           aesKey[16];
   macAddr_t         mac;
   uint16_t          macChecksum;
   uint8_t           pad[6];                 /* Force alignment of following certs to 16  */
   NetWorkRootCA_t   dtlsNetworkRootCA;      /* Loaded at integration   */
   HESubject_t       dtlsHESubject;          /* Loaded at integration   */
   MSSubject_t       dtlsMSSubject;          /* Loaded at integration   */
   MSSubject_t       dtlsMfgSubject2;        /* Loaded at integration   */
   uint8_t           publicKey[64];          /* Public key received during DTLS handshake; used for verifying MTLS
                                                signatures. */
   uint8_t           subjectKeyId[ 20 ];     /* sha 1 hash of publicKey; used for MTLS messages.   */
   privateKey        hostPasswordKey;        /* Key used to encrypt/decrypt host password */
#if ( MCU_SELECTED == RA6E1 )
   uint16_t          hostPasswordKeyCRC;     /* Used to verify host password key (secROM.hostPasswordKey) on boot up */
#endif
}intFlashNvMap_t;

typedef enum
{
   AclaraPublicRootCert_e,
   SignerCert_e,
   DeviceCert_e,
   dtlsNetworkRootCA_e,
   dtlsHESubject_e,
   dtlsMSSubject_e
} Cert_type;
#endif   /* BOOTLOADER  */

/* Used to inform Bootloader about items to transfer from external NV to Internal Flash */
typedef struct
{
  nvAddr       SrcAddr;    /* Absolute External NV Memory Address */
  flAddr       DstAddr;    /* Absolute Internal Flash Address */
  flSize       Length;     /* Number of bytes to copy */
  uint32_t     CRC;        /* CRC over the copied space */
  uint32_t     FailCount;  /* Number of attempts before CRC validated.  */
} DfwBlInfo_t, * pDfwBlInfo_t;

typedef struct
{
   DfwBlInfo_t             DFWinformation[ MAX_COPY_RANGES ];
#if ( MCU_SELECTED == RA6E1 )
   uint32_t                crcDfwInfo;
#endif
}DfwBlInfoCrc_t;
/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
#undef dvr_intFlash_cfg_EXTERN

#endif
