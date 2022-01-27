/***********************************************************************************************************************

   Filename: MP_SPP.h

   Contents:

************************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2017 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
************************************************************************************************************************
   $Log$

***********************************************************************************************************************/

#ifndef MP_SPP_H
#define MP_SPP_H


/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>


/* ***************************************************************************************************************** */
/* CONSTANT DEFINITIONS */


/* ***************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define ILC_DRU_DRIVER_REG_SERIAL_NUMBER           24
#define ILC_DRU_DRIVER_REG_SERIAL_NUMBER_SIZE       4

#define ILC_DRU_DRIVER_REG_HW_VERSION              50
#define ILC_DRU_DRIVER_REG_HW_VERSION_SIZE          2

#define ILC_DRU_DRIVER_REG_FW_VERSION              48
#define ILC_DRU_DRIVER_REG_FW_VERSION_SIZE          2

#define ILC_DRU_DRIVER_REG_MODEL                   51
#define ILC_DRU_DRIVER_REG_MODEL_SIZE               2

#define SET_TIME_DAYS_SIZE_IN_BITS                 ( (uint8_t)  16 )
#define SET_TIME_COUNTS_SIZE_IN_BITS               ( (uint8_t)  16 )
#define SET_TIME_PAD_SIZE_IN_BITS                  ( (uint8_t)   2 )
#define SET_TIME_DATA_AND_PAD_SIZE_IN_BYTES        ( (uint8_t)   5 )
#define SET_TIME_DATA_START_POSITION_ASCII         ( (uint8_t)  15 )
#define SET_TIME_DATA_START_POSITION_HEX           ( (uint8_t)   7 )
#define SET_TIME_CHECKSUM_START_POSITION_ASCII     ( (uint8_t)  25 )
#define DRU_REQUEST_MAX_SIZE_IN_BYTES              ( (uint8_t)  78 )    /* Maximum length in bytes of a Request Packet from and including the SOH Field to
                                                                           and including the Pad Field according to the Serial Protocol Specification.
                                                                           It was calculated as follows:
                                                                           SOH Field: 1 raw data byte +
                                                                           Opcode Field (8 bits) + Ignore1 Field (8 bits) + Ignore2 Field (46 bits) + Data Field (227 bits) +
                                                                           Pad Field (7 bits) + Checksum Field (8 bits) = 304 bits = 76 raw data bytes when encoded to ascii hex +
                                                                           EOT Field: 1 raw data byte
                                                                           TOTAL = 78 raw data bytes */
#define DRU_RESPONSE_MAX_SIZE_IN_BYTES             ( (uint8_t)  204 )   /* Maximum length in bytes of a Response Packet from and including the SOH Field to
                                                                           and including the Pad Field according to the Serial Protocol Specification.
                                                                           It was calculated as follows:
                                                                           SOH Field:              (8 bits)1 raw data byte +
                                                                           Opcode Field            (16 bits) +
                                                                           Ignore1 Field           (16 bits) +
                                                                           Response Length Field   (16 bits) +
                                                                           Ignore2 Field           (16 bits) +
                                                                           Data Field              (1536 bits )(96 * 2 * 8 bits) +
                                                                           Checksum Field          (16 bits) +
                                                                           EOT Field:              (8 bits )1 raw data byte= 1632 bits = 204 raw data bytes when encoded to ASCII
                                                                           TOTAL = 204 raw data bytes */
#define ACK_SIZE                                   ( (uint8_t)   1 )
#define DRU_SERIAL_NUMBER_REQUEST_SIZE             ( (uint32_t) ( sizeof ( ReadDruSerialNumberRequestPacket_au8 ) ) )
#define DRU_FW_VERSION_REQUEST_SIZE                ( (uint32_t) ( sizeof ( ReadDruFwVersionRequestPacket_au8 ) ) )
#define DRU_HW_VERSION_REQUEST_SIZE                ( (uint32_t) ( sizeof ( ReadDruHwVersionRequestPacket_au8 ) ) )
#define DRU_MODEL_REQUEST_SIZE                     ( (uint32_t) ( sizeof ( ReadDruRceProductRequestPacket_au8 ) ) )
#define DRU_SET_DATE_TIME_REQUEST_SIZE             ( (uint32_t) ( sizeof ( SetDruDateTimeFrameHexAscii_au8 ) ) )
#define DRU_SERIAL_NUMBER_RESPONSE_SIZE            ( (uint8_t)  20 )
#define DRU_FW_VERSION_RESPONSE_SIZE               ( (uint8_t)  20 )
#define DRU_HW_VERSION_RESPONSE_SIZE               ( (uint8_t)  16 )
#define DRU_MODEL_RESPONSE_SIZE                    ( (uint8_t)  16 )
#define DRU_SET_DATE_TIME_RESPONSE_SIZE            ( (uint8_t)  28 )

#define SOH_VALUE                                  ( (uint8_t)  0x01 )
#define EOT_VALUE                                  ( (uint8_t)  0x04 )
#define ACK_VALUE                                  ( (uint8_t)  0x06 )
#define NACK_VALUE                                 ( (uint8_t)  0x15 )

#define MPSPP_RESPONSE_SOH_OFFSET                  ( (uint8_t)   0 )
#define MPSPP_RESPONSE_OPCODE_OFFSET               ( (uint8_t)   1 )
#define MPSPP_RESPONSE_IGNORE1_OFFSET              ( (uint8_t)   3 )
#define MPSPP_RESPONSE_RESPONSE_LENGHT_OFFSET      ( (uint8_t)   5 )
#define MPSPP_RESPONSE_IGNORE2_OFFSET              ( (uint8_t)   6 )
#define MPSPP_RESPONSE_DATA_OFFSET                 ( (uint8_t)   9 )

/* ***************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ***************************************************************************************************************** */
/* GLOBAL VARIABLES */


/* ***************************************************************************************************************** */
/* FUNCTION PROTOTYPES */


#endif /* MP_SPP_H */

/* End of file */
