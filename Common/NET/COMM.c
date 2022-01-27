/******************************************************************************
 *
 * Filename: COMM.c
 *
 * Global Designator:  COMM_
 *
 * Contents:  This file is temporary and will be removed when full MSG_APP_ file is ready to be checked in.  This
 *          module ctonains implementation of transmit/receive (loopback).
 *
 ******************************************************************************
 * Copyright (c) 2014 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include <stdlib.h>       // rand()
#include <stdbool.h>
#include <string.h>
#include "DBG_SerialDebug.h"

#include "MAC_Protocol.h"
#include "STACK_Protocol.h"
#include "STACK.h"

#include "COMM.h"
#include "App_Msg_Handler.h"
#include "dtls.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/*! ********************************************************************************************************************
 *
 * \fn void COMM_transmit( COMM_Tx_t const *uint8_t )
 *
 * \brief This function transmits data to a destination address.
 *
 * \param  tx_data  - pointer to the data to send
 *
 * \return  none
 *
 *********************************************************************************************************************/
void COMM_transmit ( COMM_Tx_t const *tx_data )
{
   uint8_t  securityMode;

   ( void )APP_MSG_SecurityHandler( method_get, appSecurityAuthMode, &securityMode, NULL );

   (void)NWK_DataRequest(
      ( uint8_t )UDP_NONDTLS_PORT,
      tx_data->qos,
      tx_data->length,
      tx_data->data,
      &tx_data->dst_address,
      &tx_data->override,
      tx_data->callback,
      NULL);
}


#if (USE_IPTUNNEL == 1)
/**
This function transmits IP tunneling data to a destination address.

@param tx_data pointer to the data to send
*/
void IP_transmit(COMM_Tx_t const *tx_data)
{
   uint8_t securityMode;

   (void)APP_MSG_SecurityHandler(method_get, appSecurityAuthMode, &securityMode, NULL);

   (void)NWK_DataRequest(
      (uint8_t)UDP_IP_NONDTLS_PORT,
      tx_data->qos,
      tx_data->length,
      tx_data->data,
      &tx_data->dst_address,
      &tx_data->override,
      tx_data->callback,
      NULL);
}
#endif

/*! ********************************************************************************************************************
 *
 * \fn void MTLS_transmit( COMM_Tx_t const *uint8_t )
 *
 * \brief This function transmits data to a destination address, using the mtls port
 *
 * \param  tx_data  - pointer to the data to send
 *
 * \return  none
 *
 *********************************************************************************************************************/
void MTLS_transmit ( COMM_Tx_t const *tx_data )
{
   (void)NWK_DataRequest(
      ( uint8_t )UDP_MTLS_PORT,
      tx_data->qos,
      tx_data->length,
      tx_data->data,
      &tx_data->dst_address,
      &tx_data->override,
      tx_data->callback,
      NULL);
}
