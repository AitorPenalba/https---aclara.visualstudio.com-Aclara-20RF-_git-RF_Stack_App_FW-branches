/***********************************************************************************************************************
 *
 * Filename: COMM.h
 *
 * Contents: This file will be deleted when the APP_MSG_ module is ready to be checked in.  This module ctonains
 *          typedefs and #defines for the applicaiton implementation of transmit/receive (loopback).
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2010-2014 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
#ifndef COMM_H
#define COMM_H

/* INCLUDE FILES */

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* Data.Request */
typedef struct
{
   NWK_Address_t dst_address; /* destination address (5 format options) */
   NWK_Override_t override;   /* override paramters */
   uint16_t length; /* packet length */
   uint8_t *data; /* packet payload */
   uint8_t qos; /* quality of service */
   MAC_dataConfCallback callback; /* optional function pointer to be called with tx result (QoS dictates 'success') */
} COMM_Tx_t;

extern void COMM_transmit ( COMM_Tx_t const *tx_data );
extern void MTLS_transmit ( COMM_Tx_t const *tx_data );
void IP_transmit(COMM_Tx_t const *tx_data);

#endif /* COMM_H */
