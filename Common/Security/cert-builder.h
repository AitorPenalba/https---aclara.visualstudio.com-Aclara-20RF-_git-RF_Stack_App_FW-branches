/**
 * \file      cert-builder.h
 * \brief     Declarations for X.509 certificate rebuilding functions.
 * \author    Atmel Crypto Group, Colorado Springs, CO
 * \copyright 2015 Atmel Corporation
 * \date      2015-05-01
 *
 * This are the structure definitions and function declarations for
 * the X.509 certificate rebuilding functions:
 *     buildSignerCert()
 *     buildDeviceCert()
 * These functions take the dynamic elements of the certs read off the
 * ATECC508A device and integrate them into certificate templates.
 */

#ifndef CERTBUILDER_H
#define CERTBUILDER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief List dynamic certificate elements
 */
typedef enum
{
   CEID_SN,               //!< Certificate serial number
   CEID_ISSUE_DATE,       //!< Certificate issue date
   CEID_SIGNER_ID,        //!< Signer ID
   CEID_SUBJECT_SN,       //!< Subject serial number (EUI-64)
   CEID_PUBLIC_KEY,       //!< Subject public key
   CEID_AUTHORITY_KEY_ID, //!< Authority public key ID
   CEID_SUBJECT_KEY_ID,   //!< Subject public key ID
   CEID_SIGNATURE,        //!< Certificate signature
   CEID_NUM_IDS           //!< Used in the code the track the number of elements available
} CertElementID;

/**
 * \brief Used to store the location and size of a dynamic element in a certificate.
 */
typedef struct
{
   int16_t byte_offset; //!< Element starts at this byte offset into the certificate
   int16_t byte_count;  //!< Number of bytes for the dynamic element
} CertElement;

/**
 * \brief Store a certificate and a list of the dynamic elements
 */
typedef struct
{
   uint8_t     data[512];                          //!< Full certificate data
   int16_t     data_size;                          //!< Size of the certificate in bytes
   CertElement elements[(uint32_t)CEID_NUM_IDS];   //!< List of dynamic elements. Elements with a byte_count of 0 don't apply.
} CertDef;

/**
 * \brief Returns a pointer to a dynamic elements location in a certificate
 */
static uint8_t* getElementData(CertDef* cert, CertElementID id)
{
   return &cert->data[cert->elements[id].byte_offset];
}

/**
 * \brief Structure containing the parsed elements of a partial certificate
 */
typedef struct
{
   uint8_t signature[64]; //!< Certificate signature, R and S concatenated
   uint8_t enc_dates[3];  //!< Issue and expire dates in the encoded format
   int16_t issue_year;              //!< Issue date year
   int16_t issue_month;             //!< Issue date month
   int16_t issue_day;               //!< Issue date day
   int16_t issue_hour;              //!< Issue date hour
   int16_t expire_years;            //!< Number of years the certificate is valid for. 0 indicates no expiration.
   uint8_t signer_id[2];  //!< Signer ID, used to identify which signer was used
   uint8_t template_id;   //!< Certificate template ID. Can be used to identify which template to use for the certificate.
   uint8_t chain_id;      //!< ID for the certificate chain this certificate applies to.
   uint8_t sn_source;     //!< Indicates where the certificate serial number comes from
   uint8_t format;        //!< Partial cert format
   uint8_t reserved;      //!< Unused
} PartialCert;

int16_t buildSignerCert(
   const uint8_t* slot8,
   const uint8_t* slot10,
   const uint8_t* issuer_public_key,
   CertDef*             signer_cert);

int16_t buildDeviceCert(
   const uint8_t* slot8,
   const uint8_t* slot9,
   const uint8_t* slot10,
   CertDef*             device_cert);

void getSignerCert(CertDef* cert_def);

void getDeviceCert(CertDef* cert_def);

int16_t buildCert(
   CertDef*             cert,
   const PartialCert*   partial_cert,
   const uint8_t* authority_public_key,
   const uint8_t* subject_public_key,
   const uint8_t* subject_sn);

int16_t parsePartialCert(const uint8_t* raw_cert, PartialCert* cert);

int16_t getCertSerialNumber(
   const uint8_t* subject_public_key,
   const uint8_t* enc_dates,
   uint8_t*       serial_num,
   int16_t                  serial_num_size);

int16_t getKeyID(const uint8_t* public_key, uint8_t* key_id);

int16_t mergeSignature(CertDef* cert, const uint8_t* signature);

int16_t buildASN1Signature(const uint8_t* sig, uint8_t* asn1_sig, int16_t* asn1_sig_size);

#define ATCERT_E_SUCCESS              0
#define ATCERT_E_BUFFER_TOO_SMALL     1
#define ATCERT_E_SN_TOO_LARGE         2
#define ATCERT_E_HASH_NOT_IMPLEMENTED 3

#ifdef __cplusplus
}
#endif

#endif
