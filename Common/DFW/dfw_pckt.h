/***********************************************************************************************************************
 *
 * Filename: dfw_pckt.h
 *
 * Contents: Saves downloaded packets and updates the Bit Field.  Also provides an API to return missing packets.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2004-2014 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Karl Davlin
 *
 * $Log$ Created July 19, 2004
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - 07/19/2004 - Initial Release
 *
 * @version    2.0 - See the 'C' file for details.
 * #since      2014-02-10
 **********************************************************************************************************************/

#ifndef DFW_pckt_H
#define DFW_pckt_H

#ifdef DFWP_GLOBALS
#define DFWP_EXTERN
#else
#define DFWP_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>
#include <stdbool.h>
#include "error_codes.h"
#include "dfw_app.h"

/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define REPORT_NO_PACKETS  ((uint16_t)0)  // Definition for reporting no packets for DFWP_getMissingPackets()

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   eDL_BIT_CLEAR = ((uint8_t)0), /* Command to clear a bit in the bit field. */
   eDL_BIT_SET                   /* Command to set a bit in the bit field.  */
}eDFWP_BitFieldModify_t;         /* Commands for modifyBit() to set or clear a bit in the bit field. */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

returnStatus_t  DFWP_init(void); /* Must be called at power up. */
uint16_t        DFWP_getMaxPackets( void );
bool            DFWP_ClearBitMap(void);   /* Run as low priority from the main loop. */
returnStatus_t  DFWP_modifyBit( uint16_t packetNumber, eDFWP_BitFieldModify_t action );
returnStatus_t  DFWP_getBitStatus(uint16_t pcktNum, uint8_t *pBit);
void            DFWP_clearBitField(void);
returnStatus_t  DFWP_getMissingPackets(dl_packetcnt_t totPckts, dl_packetcnt_t maxPcktsReported,
                                       dl_packetcnt_t *pNumMissingPckts, dl_packetid_t *pMissingPcktIds);
returnStatus_t  DFWP_getMissingPacketsRange(dl_packetcnt_t totPckts, dl_packetcnt_t maxReportPckts,
                                            dl_packetcnt_t *pNumMissingPckts, dl_packetid_t *pMissingPcktIds);

#undef DFWP_EXTERN

#endif
