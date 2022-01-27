/******************************************************************************
 *
 * Filename: hmc_start.h
 *
 * Contents: #defs, typedefs, and function prototypes for starting the meter
 *    communication session, log-in, password, etc...
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ******************************************************************************
 *
 * $Log$ kad Created 080707
 *
 *****************************************************************************/

#ifndef hmc_config_H
#define hmc_config_H

/* INCLUDE FILES */

#include "psem.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

#define DEFAULT_HMC_PACKET_SIZE  (uint8_t)64
#define DEFAULT_HMC_PACKET_NUM   (uint8_t)1
#define DEFAULT_HMC_BAUD_RATE    (uint32_t)9600
#define DEFAULT_HMC_BAUD_CODE    (uint32_t)0x06


#if( (NEGOTIATE_HMC_BAUD == 1) || ( NEGOTIATE_HMC_PACKET_NUMBER == 1) || (NEGOTIATE_HMC_PACKET_SIZE == 1)) //default negoatiion?
#define DEFAULT_NEGOTIATION 0
#else
#define DEFAULT_NEGOTIATION 1
#endif


#if( DEFAULT_NEGOTIATION == 1 )
#define ALT_HMC_BAUD_RATE      DEFAULT_HMC_BAUD_RATE
#define ALT_HMC_BAUD_CODE      DEFAULT_HMC_BAUD_CODE
#define ALT_HMC_PACKET_SIZE    DEFAULT_HMC_PACKET_SIZE
#define ALT_HMC_PAKCET_NUMBER  DEFAULT_HMC_PACKET_NUM
#else
#if( HMC_KV == 1  ) /*comm parameters must be meter specific*/
#define ALT_HMC_BAUD_RATE     (uint32_t)57600
#define ALT_HMC_BAUD_CODE     (uint8_t) 0x0A
#define ALT_HMC_PACKET_SIZE    DEFAULT_HMC_PACKET_SIZE
#define ALT_HMC_PAKCET_NUMBER  DEFAULT_HMC_PACKET_NUM
#endif
#endif

/*incuding macros for packet size and number negoiations for future use that may negtaiate their size*/


/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */


/* FUNCTION PROTOTYPES */


#endif
