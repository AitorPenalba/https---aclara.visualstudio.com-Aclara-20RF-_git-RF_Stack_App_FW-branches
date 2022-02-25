/**
 * \file      cert-builder.c
 * \brief     X.509 certificate rebuilding functions.
 * \author    Atmel Crypto Group, Colorado Springs, CO
 * \copyright 2015 Atmel Corporation
 * \date      2015-05-01
 *
 * Implmentations of the X.509 certificate rebuilding functions:
 *     buildSignerCert()
 *     buildDeviceCert()
 */

#include <stdint.h>
#include "cert-builder.h"
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#if 0 // TODO: RA6E1 - WolfSSL integration
#include "wolfssl/wolfcrypt/sha.h"
#include "wolfssl/wolfcrypt/sha256.h"
#endif
#endif

/**
 * \brief Reconstructs a signer certificate from the raw data read off the ATECC508A device.
 *
 * This assumes a certain layout of certificate data in the ATECC508A's slots:
 * Slot 8:  [Signer Partial Cert, 72 bytes][Device Partial Cert, 72 bytes][Device EUI-64 as ascii hex, 16 bytes]
 * Slot 10: [Padding, 4 bytes][Signer Public Key X, 32 bytes][Padding, 4 bytes][Signer Public Key Y, 32 bytes]
 *
 * \param[in]  slot8              Raw data read from slot 8 of the ATECC508A device, for signer partial cert.
 *                                Technically only the first 160 bytes are required.
 * \param[in]  slot10             Raw data read from slot 10 for the ATECC108A device, for the signer public key.
 *                                Needs to be all 72 bytes of the slot.
 * \param[in]  issuer_public_key  This is the signer's authority public key (public key of the CA that signed
 *                                the signer's certificate. 64 byte value: [X, 32 bytes][Y, 32 bytes].
 *                                No point compression byte at the beginning.
 * \param[out] signer_cert        Pointer to certificate definition that will have the reconstructed certificate.
 *
 * \return  0 on success.
 */
int16_t buildSignerCert(   const uint8_t* slot8,
                           const uint8_t* slot10,
                           const uint8_t* issuer_public_key,
                           CertDef* signer_cert)
{
   PartialCert partial_cert;
   uint8_t signer_public_key[64];

   // Reformat public key to remove padding
   (void)memcpy(&signer_public_key[0], &slot10[4], 32);
   (void)memcpy(&signer_public_key[32], &slot10[40], 32);

   // Get the base certificate template
   getSignerCert(signer_cert);

   // Parse the partial certificate into its components
   (void)parsePartialCert(&slot8[0], &partial_cert);

   // Merge dynamic elements back into the certificate template
   return buildCert(signer_cert, &partial_cert, issuer_public_key, signer_public_key, NULL);//lint !e418 null pointer OK
}

/**
 * \brief Reconstructs a device certificate from the raw data read off the ATECC508A device.
 *
 * This assumes a certain layout of certificate data in the ATECC508A's slots:
 * Slot 8:  [Signer Partial Cert, 72 bytes][Device Partial Cert, 72 bytes][Device EUI-64 as ascii hex, 16 bytes]
 * Slot 9:  [Padding, 4 bytes][Device Public Key X, 32 bytes][Padding, 4 bytes][Device Public Key Y, 32 bytes]
 * Slot 10: [Padding, 4 bytes][Signer Public Key X, 32 bytes][Padding, 4 bytes][Signer Public Key Y, 32 bytes]
 *
 * \param[in]  slot8        Raw data read from slot 8 of the ATECC508A device, for signer partial cert.
 *                          Technically only the first 160 bytes are required.
 * \param[in]  slot9        Raw data read from slot 9 for the ATECC108A device, for the device public key.
 *                          Needs to be all 72 bytes of the slot.
 * \param[in]  slot10       Raw data read from slot 10 for the ATECC108A device, for the signer public key.
 *                          Needs to be all 72 bytes of the slot.
 * \param[out] device_cert  Pointer to certificate definition that will have the reconstructed certificate.
 *
 * \return  0 on success.
 */
int16_t buildDeviceCert(
                           const uint8_t* slot8,
                           const uint8_t* slot9,
                           const uint8_t* slot10,
                           CertDef*             device_cert)
{
   PartialCert partial_cert;
   uint8_t device_public_key[64];
   uint8_t signer_public_key[64];

   // Reformat public keys to remove padding
   (void)memcpy(&device_public_key[0], &slot9[4], 32);
   (void)memcpy(&device_public_key[32], &slot9[40], 32);
   (void)memcpy(&signer_public_key[0], &slot10[4], 32);
   (void)memcpy(&signer_public_key[32], &slot10[40], 32);

   // Get the base certificate template
   getDeviceCert(device_cert);

   // Parse the partial certificate into its components
   (void)parsePartialCert(&slot8[72], &partial_cert);

   // Merge dynamic elements back into the certificate template
   return buildCert(device_cert, &partial_cert, signer_public_key, device_public_key, &slot8[144]);
}

/**
 * \brief Gets the signer certificate template.
 *
 * The certificate definition returned here can be used as a base template
 * for rebuilding an actual signer's certificate from the partial certificate
 * read off the ATECC508A device.
 *
 * \param[out] cert_def  Pointer to the output certificate definition.
 */
void getSignerCert(CertDef* cert_def)
{
   // Raw template data in ASN.1 DER format
   const uint8_t cert_template[] =
   {
#if 0
      0x30, 0x82, 0x01, 0xA8, 0x30, 0x82, 0x01, 0x4F, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x08, 0x01,
      0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
      0x04, 0x03, 0x02, 0x30, 0x2A, 0x31, 0x0F, 0x30, 0x0D, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06,
      0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x31, 0x17, 0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C,
      0x0E, 0x44, 0x65, 0x76, 0x54, 0x73, 0x74, 0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43, 0x41, 0x30,
      0x20, 0x17, 0x0D, 0x31, 0x35, 0x30, 0x34, 0x32, 0x31, 0x32, 0x33, 0x30, 0x30, 0x30, 0x30, 0x5A,
      0x18, 0x0F, 0x39, 0x39, 0x39, 0x39, 0x31, 0x32, 0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35, 0x39,
      0x5A, 0x30, 0x2A, 0x31, 0x0F, 0x30, 0x0D, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06, 0x41, 0x63,
      0x6C, 0x61, 0x72, 0x61, 0x31, 0x17, 0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0E, 0x44,
      0x65, 0x76, 0x69, 0x63, 0x65, 0x20, 0x43, 0x41, 0x20, 0x30, 0x30, 0x30, 0x30, 0x30, 0x59, 0x30,
      0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE,
      0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x3E, 0x23, 0xAD, 0x62, 0x35, 0x81, 0x70, 0x10,
      0x8F, 0x3E, 0x00, 0x0D, 0xAF, 0xBE, 0xDB, 0x0F, 0x7D, 0x0B, 0x0E, 0x8B, 0x6E, 0x48, 0x59, 0xD6,
      0xE7, 0x45, 0xDD, 0xAF, 0x8F, 0xDB, 0xEA, 0xBB, 0xD9, 0x97, 0x72, 0xCC, 0xDC, 0x59, 0xF3, 0xE6,
      0xD2, 0x5E, 0x97, 0x3B, 0x5A, 0xBD, 0xF4, 0xA1, 0x2A, 0x10, 0x06, 0x37, 0xF8, 0x5C, 0x21, 0x75,
      0x0F, 0x68, 0xD1, 0x10, 0x63, 0x65, 0xA1, 0xA3, 0xA3, 0x5D, 0x30, 0x5B, 0x30, 0x0C, 0x06, 0x03,
      0x55, 0x1D, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xFF, 0x30, 0x0B, 0x06, 0x03, 0x55, 0x1D,
      0x0F, 0x04, 0x04, 0x03, 0x02, 0x01, 0x06, 0x30, 0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16,
      0x04, 0x14, 0x51, 0x8F, 0x43, 0x95, 0xF3, 0x7A, 0xEF, 0x3A, 0x70, 0x16, 0xA0, 0x35, 0x94, 0xD4,
      0xB9, 0xD7, 0x72, 0x6C, 0xBD, 0xB7, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30,
      0x16, 0x80, 0x14, 0x40, 0x27, 0x2D, 0xA5, 0x48, 0xDB, 0xA6, 0xC5, 0x3E, 0x38, 0xD6, 0x21, 0xE2,
      0xA2, 0xBF, 0x06, 0x2D, 0xFB, 0x6B, 0xF1, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
      0x04, 0x03, 0x02, 0x03, 0x49, 0x00, 0x30, 0x46, 0x02, 0x21, 0x00, 0x90, 0x8B, 0x7E, 0x36, 0x91,
      0xDD, 0xD3, 0x44, 0xD5, 0xBF, 0xBA, 0x26, 0xDF, 0x6F, 0x5A, 0xE4, 0x3B, 0x2E, 0x24, 0x97, 0x04,
      0xD6, 0x6A, 0xA6, 0xEB, 0xE2, 0x8B, 0x13, 0x4F, 0x6F, 0x73, 0x32, 0x02, 0x21, 0x00, 0x93, 0x7F,
      0x2F, 0x0D, 0x22, 0xD9, 0x8E, 0x2B, 0x04, 0x8E, 0x13, 0xB4, 0x80, 0x42, 0x98, 0x40, 0xCB, 0x8D,
      0xB6, 0x5E, 0x27, 0xAC, 0x7D, 0xA8, 0x8F, 0x8B, 0xBD, 0xD1, 0x6B, 0x3D
#else

      0x30, 0x82, 0x01, 0xAA, 0x30, 0x82, 0x01, 0x4F,  0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x08, 0x01,
      0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09, 0x30,  0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
      0x04, 0x03, 0x02, 0x30, 0x2A, 0x31, 0x0F, 0x30,  0x0D, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06,
      0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x31, 0x17,  0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C,
      0x0E, 0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x20,  0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43, 0x41, 0x30,
      0x20, 0x17, 0x0D, 0x31, 0x35, 0x30, 0x34, 0x32,  0x31, 0x32, 0x33, 0x30, 0x30, 0x30, 0x30, 0x5A,
      0x18, 0x0F, 0x39, 0x39, 0x39, 0x39, 0x31, 0x32,  0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35, 0x39,
      0x5A, 0x30, 0x2A, 0x31, 0x0F, 0x30, 0x0D, 0x06,  0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06, 0x41, 0x63,
      0x6C, 0x61, 0x72, 0x61, 0x31, 0x17, 0x30, 0x15,  0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0E, 0x44,
      0x65, 0x76, 0x69, 0x63, 0x65, 0x20, 0x43, 0x41,  0x20, 0x30, 0x30, 0x30, 0x30, 0x30, 0x59, 0x30,
      0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D,  0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE,
      0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04,  0x3E, 0x23, 0xAD, 0x62, 0x35, 0x81, 0x70, 0x10,
      0x8F, 0x3E, 0x00, 0x0D, 0xAF, 0xBE, 0xDB, 0x0F,  0x7D, 0x0B, 0x0E, 0x8B, 0x6E, 0x48, 0x59, 0xD6,
      0xE7, 0x45, 0xDD, 0xAF, 0x8F, 0xDB, 0xEA, 0xBB,  0xD9, 0x97, 0x72, 0xCC, 0xDC, 0x59, 0xF3, 0xE6,
      0xD2, 0x5E, 0x97, 0x3B, 0x5A, 0xBD, 0xF4, 0xA1,  0x2A, 0x10, 0x06, 0x37, 0xF8, 0x5C, 0x21, 0x75,
      0x0F, 0x68, 0xD1, 0x10, 0x63, 0x65, 0xA1, 0xA3,  0xA3, 0x5D, 0x30, 0x5B, 0x30, 0x0C, 0x06, 0x03,
      0x55, 0x1D, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01,  0x01, 0xFF, 0x30, 0x0B, 0x06, 0x03, 0x55, 0x1D,
      0x0F, 0x04, 0x04, 0x03, 0x02, 0x01, 0x06, 0x30,  0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16,
      0x04, 0x14, 0x51, 0x8F, 0x43, 0x95, 0xF3, 0x7A,  0xEF, 0x3A, 0x70, 0x16, 0xA0, 0x35, 0x94, 0xD4,
      0xB9, 0xD7, 0x72, 0x6C, 0xBD, 0xB7, 0x30, 0x1F,  0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30,
      0x16, 0x80, 0x14, 0xF3, 0x08, 0x1E, 0xF8, 0x17,  0xD1, 0xEE, 0x0D, 0xC6, 0x51, 0xB4, 0xFB, 0x5A,
      0x4F, 0xB9, 0xCF, 0x01, 0x0F, 0x11, 0xD0, 0x30,  0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
      0x04, 0x03, 0x02, 0x03, 0x49, 0x00, 0x30, 0x46,  0x02, 0x21, 0x00, 0x90, 0x8B, 0x7E, 0x36, 0x91,
      0xDD, 0xD3, 0x44, 0xD5, 0xBF, 0xBA, 0x26, 0xDF,  0x6F, 0x5A, 0xE4, 0x3B, 0x2E, 0x24, 0x97, 0x04,
      0xD6, 0x6A, 0xA6, 0xEB, 0xE2, 0x8B, 0x13, 0x4F,  0x6F, 0x73, 0x32, 0x02, 0x21, 0x00, 0x93, 0x7F,
      0x2F, 0x0D, 0x22, 0xD9, 0x8E, 0x2B, 0x04, 0x8E,  0x13, 0xB4, 0x80, 0x42, 0x98, 0x40, 0xCB, 0x8D,
      0xB6, 0x5E, 0x27, 0xAC, 0x7D, 0xA8, 0x8F, 0x8B,  0xBD, 0xD1, 0x6B, 0x3D, 0x88, 0x25

#endif
   };

   (void)memset(cert_def, 0, sizeof(CertDef));
   (void)memcpy(cert_def->data, cert_template, sizeof(cert_template));
   cert_def->data_size = sizeof(cert_template);

   // Initialize the dynamic certificate element data
   cert_def->elements[CEID_SN].byte_offset = 15;
   cert_def->elements[CEID_SN].byte_count = 8;
   cert_def->elements[CEID_ISSUE_DATE].byte_offset = 83;
   cert_def->elements[CEID_ISSUE_DATE].byte_count = 13;
   cert_def->elements[CEID_SIGNER_ID].byte_offset = 153;
   cert_def->elements[CEID_SIGNER_ID].byte_count = 4;
   cert_def->elements[CEID_PUBLIC_KEY].byte_offset = 184;
   cert_def->elements[CEID_PUBLIC_KEY].byte_count = 64;
   cert_def->elements[CEID_SUBJECT_KEY_ID].byte_offset = 290;
   cert_def->elements[CEID_SUBJECT_KEY_ID].byte_count = 20;
   cert_def->elements[CEID_AUTHORITY_KEY_ID].byte_offset = 323;
   cert_def->elements[CEID_AUTHORITY_KEY_ID].byte_count = 20;
   cert_def->elements[CEID_SIGNATURE].byte_offset = 355;
   cert_def->elements[CEID_SIGNATURE].byte_count = 64;
}

/**
 * \brief Gets the device certificate template.
 *
 * The certificate definition returned here can be used as a base template
 * for rebuilding an actual device's certificate from the partial certificate
 * read off the ATECC508A device.
 *
 * \param[out] cert_def  Pointer to the output certificate definition.
 */
void getDeviceCert(CertDef* cert_def)
{
   // Raw template data in ASN.1 DER format
   const uint8_t cert_template[] =
   {
      0x30, 0x82, 0x01, 0x71, 0x30, 0x82, 0x01, 0x17, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x08, 0x01,
      0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0A, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
      0x04, 0x03, 0x02, 0x30, 0x2A, 0x31, 0x0F, 0x30, 0x0D, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06,
      0x41, 0x63, 0x6C, 0x61, 0x72, 0x61, 0x31, 0x17, 0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C,
      0x0E, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x20, 0x43, 0x41, 0x20, 0x30, 0x30, 0x30, 0x30, 0x30,
      0x20, 0x17, 0x0D, 0x31, 0x35, 0x30, 0x34, 0x32, 0x31, 0x32, 0x33, 0x30, 0x30, 0x30, 0x30, 0x5A,
      0x18, 0x0F, 0x39, 0x39, 0x39, 0x39, 0x31, 0x32, 0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35, 0x39,
      0x5A, 0x30, 0x2C, 0x31, 0x0F, 0x30, 0x0D, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x06, 0x41, 0x63,
      0x6C, 0x61, 0x72, 0x61, 0x31, 0x19, 0x30, 0x17, 0x06, 0x03, 0x55, 0x04, 0x05, 0x13, 0x10, 0x41,
      0x43, 0x44, 0x45, 0x34, 0x38, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x41, 0x42, 0x43, 0x44, 0x30,
      0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86,
      0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x8D, 0x4A, 0x82, 0x7D, 0x5C, 0xC5,
      0x4B, 0xBD, 0x32, 0x0A, 0x93, 0xC0, 0x64, 0xC8, 0x27, 0xB3, 0xF8, 0x78, 0xC3, 0xC5, 0x1F, 0x91,
      0xE1, 0xEF, 0x63, 0x65, 0x9E, 0xDD, 0xEE, 0xFD, 0xA3, 0xD0, 0x63, 0xAB, 0x36, 0x97, 0x34, 0x15,
      0x46, 0xC9, 0xB2, 0x93, 0x05, 0xAD, 0x18, 0x21, 0x1A, 0x29, 0xF8, 0x38, 0x52, 0xC2, 0x0D, 0x24,
      0x4A, 0x4B, 0x23, 0xDC, 0x00, 0x46, 0x91, 0x33, 0xAA, 0x8D, 0xA3, 0x23, 0x30, 0x21, 0x30, 0x1F,
      0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x51, 0x8F, 0x43, 0x95, 0xF3,
      0x7A, 0xEF, 0x3A, 0x70, 0x16, 0xA0, 0x35, 0x94, 0xD4, 0xB9, 0xD7, 0x72, 0x6C, 0xBD, 0xB7, 0x30,
      0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x03, 0x48, 0x00, 0x30, 0x45,
      0x02, 0x20, 0x22, 0x2E, 0x76, 0x47, 0x59, 0xFE, 0x1A, 0x92, 0x17, 0x38, 0x96, 0x08, 0x4B, 0x24,
      0xE3, 0x67, 0xD7, 0x3E, 0x17, 0x13, 0xC3, 0xED, 0x5C, 0xE6, 0x21, 0x0C, 0x92, 0xEE, 0xE3, 0x90,
      0x17, 0x78, 0x02, 0x21, 0x00, 0xD7, 0xFF, 0xD0, 0xE9, 0xD4, 0xE6, 0xD8, 0x06, 0x61, 0xAB, 0xD7,
      0x1B, 0x36, 0xEE, 0xA3, 0x8C, 0x2C, 0xC5, 0xCF, 0x1E, 0x23, 0x76, 0xD5, 0x49, 0xB8, 0xB7, 0x2F,
      0x67, 0x05, 0xE9, 0x0B, 0x8C
   };

   (void)memset(cert_def, 0, sizeof(CertDef));
   (void)memcpy(cert_def->data, cert_template, sizeof(cert_template));
   cert_def->data_size = sizeof(cert_template);

   // Initialize the dynamic certificate element data
   cert_def->elements[CEID_SN].byte_offset = 15;
   cert_def->elements[CEID_SN].byte_count = 8;
   cert_def->elements[CEID_SIGNER_ID].byte_offset = 75;
   cert_def->elements[CEID_SIGNER_ID].byte_count = 4;
   cert_def->elements[CEID_ISSUE_DATE].byte_offset = 83;
   cert_def->elements[CEID_ISSUE_DATE].byte_count = 13;
   cert_def->elements[CEID_SUBJECT_SN].byte_offset = 143;
   cert_def->elements[CEID_SUBJECT_SN].byte_count = 16;
   cert_def->elements[CEID_PUBLIC_KEY].byte_offset = 186;
   cert_def->elements[CEID_PUBLIC_KEY].byte_count = 64;
   cert_def->elements[CEID_AUTHORITY_KEY_ID].byte_offset = 267;
   cert_def->elements[CEID_AUTHORITY_KEY_ID].byte_count = 20;
   cert_def->elements[CEID_SIGNATURE].byte_offset = 299;
   cert_def->elements[CEID_SIGNATURE].byte_count = 64;
}

/**
 * \brief Merges a certificate's dynamic elements back into its template
 *
 * \param[inout] cert                  Pointer to a certificate definition template. This will be updated with merged
 *                                     elements and be reconstructed certificate.
 * \param[in]    partial_cert          Partial cert data to merge into the certificate template.
 * \param[in]    authority_public_key  Certificate authority public key. 64 byte value: [X, 32 bytes][Y, 32 bytes].
 * \param[in]    subject_public_key    Certificate subject public key. 64 byte value: [X, 32 bytes][Y, 32 bytes].
 * \param[in]    subject_sn            Certificate subject serial number (EUI-64) as 16 bytes of ascii hex.
 *                                     This is only required for the device certificate and can be set to NULL
 *                                     for signer certificates.
 *
 * \return  0 on success.
 */
int16_t buildCert(
                           CertDef *      cert,
                     const PartialCert *  partial_cert,
                     const uint8_t *      authority_public_key,
                     const uint8_t *      subject_public_key,
                     const uint8_t *      subject_sn)
{
   int16_t result = ATCERT_E_SUCCESS;
   char issue_date[16];
   char signer_id_hex[8];

   do
   {
      // Format the issue date in UTC format (YYMMDDhhmmssZ)
      (void)snprintf(issue_date, sizeof(issue_date), "%02d%02d%02d%02d0000Z",
      partial_cert->issue_year - 2000,
      partial_cert->issue_month,
      partial_cert->issue_day,
      partial_cert->issue_hour);
      (void)memcpy(getElementData(cert, CEID_ISSUE_DATE), issue_date, 13);

      // Format the signer ID as 4 asscii hex digits
      (void)snprintf(signer_id_hex,
                     sizeof(signer_id_hex), "%02X%02X", partial_cert->signer_id[0], partial_cert->signer_id[1]);
      (void)memcpy(getElementData(cert, CEID_SIGNER_ID), signer_id_hex, 4);

      // Set the subject public key
      (void)memcpy(getElementData(cert, CEID_PUBLIC_KEY), subject_public_key, 64);

      // Generate the certificate serial number
      result = getCertSerialNumber( subject_public_key,
                           partial_cert->enc_dates,
                           getElementData(cert, CEID_SN),
                           cert->elements[CEID_SN].byte_count);

      if (result != ATCERT_E_SUCCESS)
      {
         break;
      }
      // Generate and merge the ASN.1 signature from the raw signature
      result = mergeSignature(cert, partial_cert->signature);
      if (result != ATCERT_E_SUCCESS)
      {
         break;
      }

      if (cert->elements[CEID_AUTHORITY_KEY_ID].byte_count != 0)
      {
         // Generate and merge the authority key ID
         result = getKeyID(authority_public_key, getElementData(cert, CEID_AUTHORITY_KEY_ID));
         if (result != ATCERT_E_SUCCESS)
         {
            break;
         }
      }

      if (cert->elements[CEID_SUBJECT_KEY_ID].byte_count != 0)
      {
         // Generate and merge the aubject key ID
         result = getKeyID(subject_public_key, getElementData(cert, CEID_SUBJECT_KEY_ID));
         if (result != ATCERT_E_SUCCESS)
         {
            break;
         }
      }

      if (cert->elements[CEID_SUBJECT_SN].byte_count != 0)
      {
         // Set the subject serial number (EUI-64)
         (void)memcpy(getElementData(cert, CEID_SUBJECT_SN), subject_sn, 16); //lint !e418 null ptr
      }
   } while (0);   /*lint !e717   */

   return result;
}

/**
 * \brief Parse the partial certificate data into individual elements.
 *
 * \param[in]  raw_cert  Raw partial certificate bytes as read from the ATECC508A device (72 bytes).
 * \param[out] cert      Parsed elements from the partial certificate.
 *
 * \return  0 on success.
 */
int16_t parsePartialCert(const uint8_t* raw_cert, PartialCert* cert)
{
   (void)memcpy(cert->signature, &raw_cert[0], 64);

   // Issue and expire dates are compressed/encoded as below
   // +---------------+---------------+---------------+
   // | Byte 1        | Byte 2        | Byte 3        |
   // +---------------+---------------+---------------+
   // | | | | | | | | | | | | | | | | | | | | | | | | |
   // | 5 bits  | 4 bits| 5 bits  | 5 bits  | 5 bits  |
   // | Issue   | Issue | Issue   | Issue   | Expire  |
   // | Year    | Month | Day     | Hour    | Years   |
   // +---------+-------+---------+---------+---------+
   // Minutes and seconds are always zero.
   (void)memcpy(cert->enc_dates, &raw_cert[64], 3);
   cert->issue_year   = (cert->enc_dates[0] >> 3) + 2000;
   cert->issue_month  = ((cert->enc_dates[0] & 0x07) << 1) | ((cert->enc_dates[1] & 0x80) >> 7);
   cert->issue_day    = ((cert->enc_dates[1] & 0x7C) >> 2);
   cert->issue_hour   = ((cert->enc_dates[1] & 0x03) << 3) | ((cert->enc_dates[2] & 0xE0) >> 5);
   cert->expire_years = (cert->enc_dates[2] & 0x1F);

   (void)memcpy(cert->signer_id, &raw_cert[67], 2);

   cert->template_id = raw_cert[69] >> 4;
   cert->chain_id    = raw_cert[69] & 0x0F;
   cert->sn_source   = raw_cert[70] >> 4;
   cert->format      = raw_cert[70] & 0x0F;
   cert->reserved    = raw_cert[71];

   return ATCERT_E_SUCCESS;
}

#ifdef WIN32
/**
 * \brief Use the windows crypto functions to calculate a hash
 *
 * \param[in]    alg_id    Hash algorithm ID.
 * \param[in]    msg       Data to compute the hash over.
 * \param[in]    msg_size  Number of bytes in msg.
 * \param[out]   hash      Buffer to store the output hash in.
 * \param[inout] hash_size As input, size of the hash buffer. As output, the size of the resulting hash.
 *
 * \return  0 on success.
 */
int16_t wincryptHash(ALG_ID alg_id, const uint8_t* msg, DWORD msg_size, uint8_t* hash, DWORD* hash_size)
{
   HCRYPTPROV hProv = NULL;
   HCRYPTHASH hHash = NULL;
   int16_t result = ATCERT_E_SUCCESS;

   do
   {
      if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
      {
         result = GetLastError();
         break;
      }

      if (!CryptCreateHash(hProv, alg_id, 0, 0, &hHash))
      {
         result = GetLastError();
         break;
      }

      if (!CryptHashData(hHash, msg, msg_size, 0))
      {
         result = GetLastError();
         break;
      }

      if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, hash_size, 0))
      {
         result = GetLastError();
         break;
      }
   } while (FALSE);

   if (hHash != NULL)
   {
      CryptDestroyHash(hHash);
   }
   if (hProv != NULL)
   {
      CryptReleaseContext(hProv, 0);
   }

   return result;
}
#endif

/**
 * \brief Calculate a SHA1 hash over the input data.
 *
 * \param[in]  msg        Data to compute the hash over.
 * \param[in]  msg_size   Number of bytes in msg.
 * \param[out] hash       Buffer to store the output hash in.
 * \param[in]  hash_size  Size of the hash buffer. Min 20 bytes.
 *
 * \return  0 on success.
 */
/*lint -esym(715, hash_size)  */
static int16_t sha1(const uint8_t* msg, int16_t msg_size, uint8_t* hash, int16_t hash_size)
{
#ifdef WIN32
   DWORD dw_hash_size = hash_size;
   return wincryptHash(CALG_SHA1, msg, msg_size, hash, &dw_hash_size);
#else
   int16_t retval;
#if 0 // TODO: RA6E1 - WolfSSL integration
   Sha sha;

   retval = (int16_t)wc_InitSha( &sha );
   if ( retval == 0 )
   {
      retval = (int16_t)wc_ShaUpdate( &sha, msg, (uint16_t)msg_size );
      if ( retval == 0 )
      {
         retval = (int16_t)wc_ShaFinal( &sha, hash );
      }
   }
#endif
   return retval;
#endif
}
/*lint +esym(715, hash_size)  */

/**
 * \brief Calculate a SHA256 hash over the input data.
 *
 * \param[in]  msg        Data to compute the hash over.
 * \param[in]  msg_size   Number of bytes in msg.
 * \param[out] hash       Buffer to store the output hash in.
 * \param[in]  hash_size  Size of the hash buffer. Min 32 bytes.
 *
 * \return  0 on success.
 */
/*lint -esym(715, hash_size)  */
static int16_t sha256(const uint8_t* msg, int16_t msg_size, uint8_t* hash, int16_t hash_size)
{
#ifdef WIN32
   DWORD dw_hash_size = hash_size;
   return wincryptHash(CALG_SHA_256, msg, msg_size, hash, &dw_hash_size);
#else
   int16_t retval;
#if 0 // TODO: RA6E1 - WolfSSL integration
   Sha256 sha;

   retval = (int16_t)wc_InitSha256( &sha );
   if ( retval == 0 )
   {
      retval = (int16_t)wc_Sha256Update( &sha, msg, (uint16_t)msg_size );
      if ( retval == 0 )
      {
         retval = (int16_t)wc_Sha256Final( &sha, hash );
      }
   }
#endif
   return retval;
#endif
}
/*lint +esym(715, hash_size)  */

/**
 * \brief Generate a certificate's serial number.
 *
 * This assumes the serial number is using the 0xC scheme (see serial number source in the
 * partial cert definition).
 * SN = SHA256(certificate subject public key + certificate encoded dates)
 * MSB of the SN is forced to 0 to ensure a positive number per RFC 5280
 *
 * \param[in]  subject_public_key  Certificate subject public key. 64 byte value: [X, 32 bytes][Y, 32 bytes].
 * \param[in]  enc_dates           3-byte encoded date from the certificate's partial certificate.
 * \param[out] serial_num          Output buffer to receive the certificate serial number.
 * \param[in]  serial_num_size     Size of the serial number to generate. Must be <= 32 bytes.
 *
 * \return  0 on success.
 */
int16_t getCertSerialNumber(
                              const uint8_t* subject_public_key,
                              const uint8_t* enc_dates,
                              uint8_t*       serial_num,
                              int16_t        serial_num_size)
{
   uint8_t msg[64 + 3];
   uint8_t hash[32];
   int16_t result;

   // Build input to hash
   (void)memcpy(&msg[0], subject_public_key, 64);
   (void)memcpy(&msg[64], enc_dates, 3);

   // Perform hash
   result = sha256(msg, sizeof(msg), hash, sizeof(hash));
   if (result == 0)
   {
      if (serial_num_size <= (int16_t)sizeof(hash))
      {
         (void)memcpy(serial_num, hash, (uint16_t)serial_num_size);
         // Force MSB to 0 to ensure a positive SN, per RFC 5280
         serial_num[0] &= 0x7F;
      }
      else
      {
         result = ATCERT_E_SN_TOO_LARGE;
      }
   }

   return result;
}

/**
 * \brief Generate a X.509 key ID from a public key.
 *
 * Key ID = SHA1([point compression byte = 0x04][public key x][public key y])
 *
 * \param[in]  public_key  Public key to compute the key ID for. 64 byte value: [X, 32 bytes][Y, 32 bytes].
 * \param[out] key_id      Key ID output, 20 bytes.
 *
 * \return  0 on success.
 */
int16_t getKeyID(const uint8_t* public_key, uint8_t* key_id)
{
   uint8_t msg[1 + 64];

   msg[0] = 0x04; // Point compression byte. 0x04 = Uncompressed
   (void)memcpy(&msg[1], public_key, 64);

   return sha1(msg, sizeof(msg), key_id, 20);
}

/**
 * \byte Merge a signature into the X.509 certificate.
 *
 * While most of the certificate's dynamic elements are simple "copy/pastes" into the template.
 * The signature presents more of a challenge due to it's format in a X.509 certificate.
 * X.509 has the signature's R and S components stored as two ASN.1 integers. Despite the
 * raw R and S integers being a fixed size for P256 (32 bytes), ASN.1 integers are signed
 * values and may require an extra padding byte.
 *
 * ASN.1 integers are in two's-complement form which essentially uses the MSB to indicate
 * negative numbers. Because R and S are unsigned integers, if their MSB is one, we need to
 * add an extra padding byte of 0x00 to prevent the ASN.1 integer from being intepreted as
 * negative.
 *
 * This variability in field length affects the certificate's ASN.1 length headers and needs
 * to be accounted for when merging the signature.
 *
 * \param[inout] cert       Certificate definition to merge the signature into.
 * \param[in]    signature  Signature to merge into the certificate. 64 bytes: [R, 32 bytes][S, 32 bytes].
 *
 * \return  0 on success.
 */
int16_t mergeSignature(CertDef* cert, const uint8_t* signature)
{
   // Get the location of the ASN.1 signature strucuture in the certificate.
   uint8_t* asn1_sig = getElementData(cert, CEID_SIGNATURE);
   // Get the length of the current ASN.1 signature structure in the certificate
   int16_t cur_asn1_sig_size = asn1_sig[1] + 2; // The 2 accounts for the field ID and length bytes
   // Calculate how much space there is the certificate buffer for the new ASN.1 signature structure
   int16_t new_asn1_sig_size = (int16_t)((sizeof(cert->data) - (uint16_t)cert->data_size) +
                               (uint16_t)cur_asn1_sig_size);
   int16_t cert_size = 0;
   int16_t result;

   // Build up the new ASN.1 signature structure from the raw signature
   result = buildASN1Signature(signature, asn1_sig, &new_asn1_sig_size);
   if (result == ATCERT_E_SUCCESS)
   {
      if (new_asn1_sig_size != cur_asn1_sig_size)
      {
         // The ASN.1 signature structure has changed size, adjust the certificates length header
         cert_size = (int16_t)(cert->data[2] << 8) | cert->data[3];   // Length header is a two-byte value in MSB order
         cert_size += (int16_t)(new_asn1_sig_size - cur_asn1_sig_size);
         cert->data[2] = (uint16_t)cert_size >> 8;
         cert->data[3] = cert_size & 0xFF;

         // Adjust the total certificate's size as well.
         cert->data_size += (new_asn1_sig_size - cur_asn1_sig_size);
      }
   }

   return result;
}

/**
 * \brief Convert a raw ECC signature into X.509 ASN.1 format.
 *
 * \param[in]    sig            Raw ECC signature. 64 bytes: [R, 32 bytes][S, 32 bytes].
 * \param[out]   asn1_sig       Output buffer for the ASN.1 signature.
 * \param[inout] asn1_sig_size  As input, the total size of the asn1_sig buffer.
 *                              As output, the actual size of the ASN.1 signature in asn1_buf.
 *
 * \return  0 on success.
 */
int16_t buildASN1Signature(const uint8_t* sig, uint8_t* asn1_sig, int16_t* asn1_sig_size)
{
   int16_t index = 0;
   int16_t result;

   // Calculate total ASN.1 signature size
   int16_t tag_size = 7 + 32 + 32; // Min ASN.1 encoded size
   // Tack on the size of the padding bytes if required
   if (sig[0] & 0x80)
   {
      tag_size++;
   }
   if (sig[32] & 0x80)
   {
      tag_size++;
   }

   // Check output buffer size
   if (*asn1_sig_size < tag_size + 2)
   {
      *asn1_sig_size = tag_size + 2;
      result = ATCERT_E_BUFFER_TOO_SMALL;
   }
   else
   {
      result = ATCERT_E_SUCCESS;
      *asn1_sig_size = tag_size + 2;

      // Build ASN.1 signature
      asn1_sig[index++] = 0x03;                    // Bit string ID
      asn1_sig[index++] = (uint8_t)tag_size;       // Bit string size
      asn1_sig[index++] = 0x00;
      asn1_sig[index++] = 0x30;                    // Sequence ID
      asn1_sig[index++] = (uint8_t)(tag_size - 3); // Sequence size

      // Add signature R integer
      asn1_sig[index++] = 0x02;           // Integer ID
      if (sig[0] & 0x80)
      {
         /* Integer has 1 in MSB, add extra 0 to ASN.1 encoding to prevent it from
            being interepeted as a negative number.   */
         asn1_sig[index++] = 33;          // Integer size
         asn1_sig[index++] = 0x00;
      }
      else
      {
         asn1_sig[index++] = 32;          // Integer size
      }
      (void)memcpy(&asn1_sig[index], &sig[0], 32);
      index += 32;

      // Add signature S integer
      asn1_sig[index++] = 0x02;           // Integer ID
      if (sig[32] & 0x80)
      {
         // Integer has 1 in MSB, add extra 0 to ASN.1 encoding to prevent it from
         // being interepeted as a negative number.
         asn1_sig[index++] = 33;          // Integer size
         asn1_sig[index++] = 0x00;
      }
      else
      {
         asn1_sig[index++] = 32;          // Integer size
      }
      (void)memcpy(&asn1_sig[index], &sig[32], 32);
   }

   return result;
}
