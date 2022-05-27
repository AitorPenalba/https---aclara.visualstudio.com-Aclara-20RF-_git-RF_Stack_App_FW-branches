/** \file
 *  \brief  Application examples that Use the ECC108 Library
 *  \author Atmel Crypto Products
 *  \date   January 29, 2014
 *

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

 *   Three example functions are given that demonstrate the device.
*/
#ifndef ECC108_EXAMPLES_H
#   define ECC108_EXAMPLES_H

#include <stdint.h>                   // data type definitions
#include "ecc108_comm_marshaling.h"
#include "dvr_intFlash_cfg.h"          // Internal flash variables structure
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


// The examples demonstrate client / host device scenarios for I2C and SWI bitbang
// configurations.
#if defined(ECC108_I2C) || defined(ECC108_I2C_BITBANG)
// If you have two devices at your disposal you can run this example as a real-world
// host / client demonstration. You have to change the address of one of the devices
// by writing it to configuration zone address 16. Be aware that bit 3 of
// the I2C address is also used to configure the input level reference
// (see data sheet table 2-1).
#   define ECC108_CLIENT_ADDRESS   (0xC0)  //was 0xC0
// To make the Mac / CheckMac examples work out-of-the-box, only one device is being
// used as example default. See above.
#   define ECC108_HOST_ADDRESS     (0xC0)
#else
// These settings have an effect only when using bit-banging where the SDA of every
// device is connected to its own GPIO pin. When using one UART the SDA of both
// devices is connected to the same GPIO pin. In that case you have to use a Pause
// flag. (Refer to data sheet.)
#   define ECC108_CLIENT_ADDRESS   (0x00)
#   define ECC108_HOST_ADDRESS     (0x00)
#endif

// Uncomment this line to generate a private key using GenKey mode 0x04/
//#define ECC108_EXAMPLE_GENERATE_PRIVATE_KEY

// Uncomment this line to activate lock function.
//#define ECC108_EXAMPLE_CONFIG_WITH_LOCK

// Uncomment this line to activate GPIO in Authorization Mode.
//#define ECC108_EXAMPLE_ACTIVATE_GPIO_AUTH_MODE

#define ECC108_KEY_ID            (0x0000)

// Macros for GPIO mode in SWI device
#define ECC108_GPIO_MODE_DISABLED       (0x00)
#define ECC108_GPIO_MODE_AUTH_OUTPUT    (0x01)
#define ECC108_GPIO_MODE_INPUT          (0x02)
#define ECC108_GPIO_MODE_OUTPUT         (0x03)
#define ECC108_GPIO_STATE_HIGH          (0x04)
#define ECC108_GPIO_STATE_LOW           (0x00)
#define ECC108_SET_HIGH                 (0x01)
#define ECC108_SET_LOW                  (0x00)

/* Abstracted names for each of the keys in the security device  */
#define DEV_PRI_KEY_0             0 /* Device Private Key 0          */
#define LOC_DEV_PRI_KEY_1         1 /* Local Device Private Key 1    */
#define LOC_DEV_PRI_KEY_2         2 /* Local Device Private Key 2    */
#define LOC_DEV_PRI_KEY_3         3 /* Local Device Private Key 3    */
#define HOST_AUTH_KEY             4 /* Host Authentication Key       */
#define ACLARA_PSK                5 /* Aclara PSK                    */
#define KEY_UPDATE_WRITE_KEY      6 /* Key Update Write Key          */
#define NETWORK_AUTH_KEY          7 /* Network Authentication Key    */
#define CERT                      8 /* Compressed Certs and MAC ID   */
#define DEV_PUB_KEY               9 /* Device Public Key             */
#define SIGNER_PUB_KEY           10 /* Signer Public Key             */
#define KEY_UPDATE_AUTH_KEY      11 /* Key Update Auth Key           */
#define NETWORK_PUB_KEY          12 /* Network Public Key            */
#define NETWORK_PUB_KEY_SECRET   13 /* Network Public Key Secret     */
#define PUB_KEY                  14 /* Public Key                    */
#define PUB_KEY_SECRET           15 /* Public Key Secret             */

#define FIRST_STORED_KEY    4
#define FIRST_SIGNATURE_KEY 9
#define LAST_STORED_KEY    15

#define DFW_SIGKEY_SIZE (65)
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t readKey     : 4;  /* Use this keyID to encrypt data being read from this slot using the Read command. See
                                 more information in the description for bit 6 in this table, the Section 9.16, Read
                                 Command, and Table 2-6 for more details.
                                 0 = Then this slot can be the source for the CheckMac copy operation. See Section
                                     3.2.7, Password Checking.
                                 -> Do not use zero as a default. Do not set this field to zero unless the CheckMac copy
                                    operation is explicitly desired, regardless of any other read/write restrictions.

                                 Slots containing private keys can never be read and this field has a different meaning:
                                 Bit 0: External signatures of arbitrary messages are enabled.
                                 Bit 1: Internal signatures of messages generated by CheckMac or GenKey are enabled.
                                 Bit 2: ECDH operation is permitted for this key.
                                 Bit 3: If clear, then ECDH master secret will be output in the clear.
                                        If set, then master secret will be written into slot N|1. Ignored if Bit 2 is
                                        zero.

                                 For slots containing public keys that can be validated (PubInfo is one, see Section
                                 2.2.5, KeyConfig), this field stored the ID of the key that should be used to perform
                                 the validation. */
   uint16_t noMac       : 1;  /* 1 = The key stored in the slot is intended for verification usage and cannot be used by
                                     the MAC or HMAC commands. When this key is used to generate or modify TempKey, then
                                     that value may not be used by the MAC and HMAC commands.
                                 0 = The key stored in the slot can be used by all commands */
   uint16_t limitedUse  : 1;  /* 1 = The key stored in the slot is “Limited Use”. See Sections 3.2.5, High Endurance
                                     Monotonic Counters and 3.2.6, Limited Use Key (Slot 15 only).
                                 0 = There are no usage limitations. */
   uint16_t encryptRead : 1;  /* 1 = Reads from this slot will be encrypted using the procedure specified in the Read
                                     command (Section 9.1.4, Address Encoding) using ReadKey (bits 0 – 3 in this table)
                                     to generate the encryption key. No input MAC is required. If this bit is set, then
                                     IsSecret must also be set (in addition, see the following Table 2-6).
                                 0 = Clear text reads may be permitted. */
   uint16_t isSecret    : 1;  /* 1 = The contents of this slot are secret – Clear text reads are prohibited and both
                                     4-byte reads and writes are prohibited. This bit must be set if EncryptRead is a
                                     one or if WriteConfig has any value other than Always to ensure proper operation of
                                     the device.
                                 0 = The contents of this slot should contain neither confidential data nor keys. The
                                     GenKey and Sign commands will fail if IsSecret is set to zero for any ECC private
                                     key.  See Table 2-6 for additional information. */
   uint16_t writeKey    : 4;  /* Use this key to validate and encrypt data written to this slot. See Section 9.21, Write
                                 Command. */
   uint16_t writeConfig : 4;  /* Controls the ability to modify the data in this slot.  See Table 2-7, Table 2-8, Table
                                 2-10, and 9.21. */
} slotConfig;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t privateKey        : 1;  /* 1 = The key slot contains an ECC private key and can be accessed only with the
                                           Sign, GenKey, and PrivWrite commands. 0 = The key slot does not contain an
                                           ECC private key and cannot be accessed with the Sign, GenKey, and PrivWrite
                                           commands.  It may contain an ECC public key, a SHA key, or data.  */
   uint16_t pubInfo           : 1;  /* If Private indicates this slot contains an ECC private key:
                                       0 = The public version of this key can never be generated. Use this mode for the
                                           highest security.
                                       1 = The public version of this key can always be generated.

                                       If Private indicates that this slot does not contain an ECC private key, then
                                       this bit may be used to control validity of public keys. If so configured, the
                                       Verify command will only use a stored public key to verify a signature if it has
                                       been validated. The Sign and Info commands are used to report the validity
                                       state. The public key validity feature is ignored by all other commands and
                                       applies only to Slots 8 – 15.
                                       0 = The public key in this slot can be used by the Verify command without being
                                           validated.
                                       1 = The public key in this slot can be used by the Verify command only if the
                                           public key in the slot has been validated. When this slot is written for any
                                           reason, the most significant four bits of byte 0 of block 0 will be set to
                                           0xA to invalidate the slot. The Verify command can be used to write those
                                           bits to 0x05 to validate the slot.*/
   uint16_t KeyType           : 3;  /* If the slot contains an ECC public or private key, then the key type field below
                                       must be set to four. If the slot contains any other kind of data, key, or secret,
                                       then this field must be set to seven for proper operation.
                                       0 = RFU (reserved for future use)
                                       1 = RFU (reserved for future use)
                                       2 = RFU (reserved for future use)
                                       3 = RFU (reserved for future use)
                                       4 = P256 NIST ECC key
                                       5 = RFU (reserved for future use)
                                       6 = RFU (reserved for future use)
                                       7 = Not an ECC key   */
   uint16_t lockable          : 1;  /* 1 = Then this slot can be individually locked using the Lock command. See the
                                           SlotLocked field in the Configuration zone to determine whether a slot is
                                           currently locked or not.
                                       0 = Then the remaining keyConfig and slotConfig bits control modification
                                           permission. Applies to all slots, regardless of whether or not they contain
                                           keys. See Section 2.4, EEPROM Locking.  */
   uint16_t reqRandom         : 1;  /* This field controls the requirements for random nonces used by the following
                                           commands: GenKey, MAC, HMAC, CheckMac, Verify, DeriveKey, and GenDig.
                                       1 = A random nonce is required.
                                       0 = A random nonce is not required. */
   uint16_t reqAuth           : 1;  /* 1 = Before this key must be used, a prior authorization using the key pointed to
                                           by AuthKey must be completed successfully prior to cryptographic use of the
                                           key.  Applies to all key types, both public, secret, and private. See Section
                                           3.2.9, Authorized Keys.
                                       0 = No prior authorization is required */
   uint16_t authKey           : 4;  /* If ReqAuth is one, this field points to the key that must be used for
                                       authorization before the key associated with this slot may be used.
                                       Must be zero if ReqAuth is zero. */
   uint16_t IntrusionDisable  : 1;  /* If ReqAuth is one, this field points to the key that must be used for
                                       authorization before the key associated with this slot may be used.
                                       Must be zero if ReqAuth is zero.  */
   uint16_t rfu               : 1;  /* Must be zero.  */
   uint16_t X509id            : 2;  /* The index into the X509format array within the Configuration zone (addresses
                                       92-95) which corresponds to this slot.  If the corresponding format byte is zero,
                                       then the public key can be validated by any format signature by the parent.  If
                                       the corresponding format byte is non-zero, then the validating certificate must
                                       be of a certain length; the stored public key must be located at a certain place
                                       within the message and the SHA() commands must be used to generate the digest of
                                       the message.  Must be zero if the slot does not contain a public key.  */
} keyConfig;
PACK_END

/* ****************************************************************************************************************** */
/* GLOBAL DEFINITION */
extern const uint8_t          ROM_dtlsMfgSubject1[87];/* Mfg subject 1 - Aclara specific - not writable  */
extern const uint8_t          dtlsMfgRootCA[];        /* Mfg root CA   - Aclara specific - not writable  */
extern const uint8_t          AclaraDFW_sigKey[DFW_SIGKEY_SIZE]; /* Used to verify patch signatures                 */
extern const intFlashNvMap_t  intROM;                 /* Internal flash intial values for secROM         */
extern const uint16_t         dtlsMfgRootCASz;        /* Size of the Mfg Root CA - Aclara Specific       */

extern void             ecc108e_sleep(void);
#if 0
extern uint8_t    ecc108e_gen_public_key( void );
#endif
extern uint8_t    ecc108e_verify_config( void );
extern uint8_t    ecc108e_read_mac( uint8_t *mac_id );
extern uint8_t    ecc108e_bfc( void );
extern uint8_t    ecc108e_Sign(  uint16_t len, uint8_t const *msg, uint8_t signature[ VERIFY_256_SIGNATURE_SIZE ] );
extern uint8_t    ecc108e_KeyAuthorization( uint8_t keyID, uint8_t const *key );
extern uint8_t    ecc108e_SelfTest( void );
extern uint8_t    *ecc108e_SeedRandom( void );
extern uint8_t    *ecc108e_SecureRandom( void );
extern uint8_t    ecc108e_Random( bool seed, uint8_t len, uint8_t *result );
extern uint8_t    ecc108e_EncryptedKeyRead( uint16_t keyID, uint8_t keyData[ ECC108_KEY_SIZE ]);
extern uint8_t    ecc108e_GetSerialNumber(void);
extern uint8_t    ecc108e_ReadKey( uint16_t keyID, uint8_t keyLen, uint8_t *keyData );
extern uint8_t    ecc108e_WriteKey( uint16_t keyID, uint8_t keyLen, const uint8_t *keyData );
extern uint8_t    ecc108e_EncryptedKeyWrite( uint16_t keyID, uint8_t *key, uint8_t keyLen );
extern uint8_t    ecc108e_DeriveFWDecryptionKey( uint8_t fwen[ ECC108_KEY_SIZE ], uint8_t fwdk[ ECC108_KEY_SIZE ] );
extern uint8_t    ecc108e_Verify( uint8_t keyID, uint16_t msglen, uint8_t const *msg, uint8_t signature[ 64 ] );
extern uint8_t    ecc108e_GetDeviceCert( Cert_type cert, uint8_t *dest, uint32_t *length);
extern void       ecc108e_UpdateKeys( const intFlashNvMap_t *intKeys );
extern void       ecc108e_InitKeys( void );
// TODO: RA6E1 [name_Suriya] - Resolve with ecc108_mqx.c/.h "replacement"
#if ( ( MCU_SELECTED == RA6E1 ) && ( RTOS_SELECTION == FREE_RTOS ) )
#include "partitions.h"

extern PartitionData_t const *SEC_GetSecPartHandle( void );
extern returnStatus_t SEC_init( void );
#endif

#endif
