/***********************************************************************************************************************

   Filename: filenames.h

   Contents: Contains a list of all of the filenames used in the system.

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   $Log$ kdavlin Created Oct 19, 2012

 **********************************************************************************************************************/
#ifndef filenames_H_
#define filenames_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   //NOTE: If a file is added, removed, or changed the file offsets function MUST also be updated
   eFN_UNDEFINED,
   eFN_DST,
   eFN_ID_CFG,
   eFN_MTRINFO,
   eFN_MTR_DS_REGISTERS,

   /*  5  */
   eFN_HD_DATA,
   eFN_ID_META,
   eFN_PWR,
   eFN_HMC_ENG,
   eFN_PHY_CACHED,    // PHY Counters, etc

   /*  10 */
   eFN_PHY_CONFIG,    // PHY Configuration File
   eFN_MAC_CACHED,    // MAC Counters, etc
   eFN_MAC_CONFIG,    // MAC Configuration File
   eFN_NWK_CACHED,    // NWK Counters, etc
   eFN_NWK_CONFIG,    // NWK Configuration File

   /*  15 */
   eFN_REG_STATUS,    // Registration Status
   eFN_TIME_SYS,
   eFN_DEMAND,
   eFN_SELF_TEST,     // Self test counters
   eFN_HMC_START,

   /*  20 */
   eFN_RTI,
   eFN_HMC_DIAGS,
   eFN_MACU,
   eFN_PWRCFG,
   eFN_ID_PARAMS,

   /*  25 */
   eFN_MODECFG,
   eFN_SECURITY,
   eFN_RADIO_PATCH,   // Radio patch
   eFN_DFWTDCFG,
   eFN_SMTDCFG,

   /*  30 */
   eFN_DMNDCFG,
   eFN_EVL_DATA,
   eFN_HD_FILE_INFO,
   eFN_OR_MR_INFO,
   eFN_EDCFG,

   /*  35 */
   eFN_DFW_VARS,
   eFN_EDCFGU,
   eFN_DTLS_CONFIG,
   eFN_DTLS_CACHED,
   eFN_DTLS_MAJOR,

   /*  40 */
   eFN_DTLS_MINOR,
   eFN_FIO_CONFIG,
   eFN_DR_LIST,
   eFN_DBG_CONFIG,    // DBG Configuration File
   eFN_EVENTS,        // com device and meter events history

   /*  45 */
   eFN_HD_INDEX,
   eFN_MIMTINFO,
   eFN_VERINFO,
   eFN_TIME_SYNC,
   eFN_MTLS_CONFIG,   // MTLS configurable attributes

   /*  50 */
   eFN_MTLS_ATTRIBS,  // MTLS stats
   eFN_DFW_CACHED,    // counters utilized by the DFW module
   eFN_TEMPERATURE_CONFIG,
   eFN_APP_CACHED,
   eFN_PD_CONFIG,     // PD Phase Detection Configuration

   /*  55 */
   eFN_DTLS_WOLFSSL4X,   //Ming: Added for wolfSSL 3.6 to 4.x upgrade
   eFN_PD_CACHED,     // PD Phase Detection Cached Data
   eFN_LASTFILE       // This is always last
   //NOTE: If a file is added, removed, or changed the file names array in file_io.c must be updated, also.
} filenames_t;

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* CONSTANTS */
#define NUM_FILES  ((int) eFN_LASTFILE )

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#endif
