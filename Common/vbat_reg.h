/***********************************************************************************************************************
 *
 * Filename: vbat_reg.h
 *
 * Global Designator: VBATREG_
 *
 * Contents: VBAT Flag Register Initialization and access macros.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author David McGraw
 *
 * $Log$
 *
 ***********************************************************************************************************************
 * Revision History:
 *
 * @version
 * #since
 **********************************************************************************************************************/

#ifndef vbat_reg_H
#define vbat_reg_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "buffer.h"
#include "MAC.h"


/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */

#ifdef VBATREG_GLOBALS
   #define VBATREG_EXTERN
#else
   #define VBATREG_EXTERN extern
#endif

VBATREG_EXTERN returnStatus_t VBATREG_init(void);


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* This section of memory is powered by the RTC super cap. Contents are preserved as long as the cap remains
   sufficiently charged.

   NOTE: This struct CANNOT exceed the size of RFVBAT in the processor */
typedef struct VBATREG_VbatRegisterFile
{
   union
   {
      struct
      {
         uint32_t sig                  : 8;  /* Indicates RTC memory is valid.   */
         uint32_t valid                : 1;  /* Indicates RTC memory is valid.   */
         uint32_t rtc_valid            : 1;  /* Indicates RTC time value is valid.  */
         uint32_t mac_config_valid     : 1;  /* Not used.   */
         uint32_t short_outage         : 1;  /* Indicates a power failure duration less than outage declaration delay.  */
         uint32_t ship_mode            : 1;  /* Copy of system "ship mode" status.  */
         uint32_t RestorationTimeMet   : 1;  /* Power restoration delay met.  */
         uint32_t meter_shop_mode      : 1;  /* Meter shop mode active  */
         uint32_t unused_bit_field1    : 1;
         uint32_t currTxAttempt        : 8;  /* No. transmit attempts this message - not to exceed CsmaMaxAttempts */
         uint32_t unused_bit_field2    : 8;
      };
      uint32_t uBitFields;
   };

   union
   {
      struct
      {
         float fCapAfter;     /* Super cap voltage after outage   */
         float fCapBefore;    /* Super cap voltage before outage   */
      };
      mac_config_t      mac_config;    /* Saved MAC Configuration */
   };
   uint16_t pwrQualCount;              /* Incremented every time power fail signal cleared. Added to NV value upon full power up. */
   uint16_t pwrRestorationTimer;       /* Consecutive seconds since pwrFail signal cleared until restoration. */
   uint32_t pwrOutageTimer;            /* Copy of outage declaration time value . */
   uint8_t  uRtcSr;                    /* RTC status register recorded every time power fail signal clears.  */
   /* Note - next two fields are only used in debugging last gasp.  */
   uint8_t uLLWU_F3;
   uint8_t uLLWU_FILT1;
} VBATREG_VbatRegisterFile, *VBATREG_VbatRegisterFilePtr;


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#if ( MCU_SELECTED == NXP_K24 )
#define VBATREG_RFSYS_BASE_PTR            ( ( VBATREG_VbatRegisterFilePtr ) ( void * )RFVBAT_BASE_PTR )
#elif ( MCU_SELECTED  == RA6E1 )
#define VBATREG_RFSYS_BASE_PTR            ( ( VBATREG_VbatRegisterFilePtr ) ( void * )R_SYSTEM->VBTBKR )
#define VBATREG_FILE_SIZE                 (sizeof(VBATREG_VbatRegisterFile))
#endif
#define VBATREG_BIT_FIELDS                ( VBATREG_RFSYS_BASE_PTR->uBitFields )
#define VBATREG_SIG                       ( VBATREG_RFSYS_BASE_PTR->sig )
#define VBATREG_VALID                     ( VBATREG_RFSYS_BASE_PTR->valid )
#define VBATREG_RTC_VALID                 ( VBATREG_RFSYS_BASE_PTR->rtc_valid )
#define VBATREG_MAC_CONFIG_VALID          ( VBATREG_RFSYS_BASE_PTR->mac_config_valid )
#define VBATREG_SHORT_OUTAGE              ( VBATREG_RFSYS_BASE_PTR->short_outage )
#define VBATREG_SHIP_MODE                 ( VBATREG_RFSYS_BASE_PTR->ship_mode )
#define VBATREG_POWER_RESTORATION_MET     ( VBATREG_RFSYS_BASE_PTR->RestorationTimeMet )
#define VBATREG_METER_SHOP_MODE           ( VBATREG_RFSYS_BASE_PTR->meter_shop_mode )
#define VBATREG_UNUSED_BIT_FIELD1         ( VBATREG_RFSYS_BASE_PTR->unused_bit_field1 )
#define VBATREG_UNUSED_BIT_FIELD2         ( VBATREG_RFSYS_BASE_PTR->unused_bit_field2 )
#define VBATREG_CAP_AFTER                 ( VBATREG_RFSYS_BASE_PTR->fCapAfter )
#define VBATREG_CAP_BEFORE                ( VBATREG_RFSYS_BASE_PTR->fCapBefore )
#define VBATREG_CURR_TX_ATTEMPT           ( VBATREG_RFSYS_BASE_PTR->currTxAttempt )
#define VBATREG_PWR_QUAL_COUNT            ( VBATREG_RFSYS_BASE_PTR->pwrQualCount )
#define VBATREG_MAX_TX_ATTEMPTS           ( VBATREG_RFSYS_BASE_PTR->mac_config.CsmaMaxAttempts )
#define VBATREG_CSMA_PVALUE               ( VBATREG_RFSYS_BASE_PTR->mac_config.CsmaPValue )
#define VBATREG_CSMA_QUICK_ABORT          ( VBATREG_RFSYS_BASE_PTR->mac_config.CsmaQuickAbort )
#define VBATREG_PACKET_ID                 ( VBATREG_RFSYS_BASE_PTR->mac_config.PacketId )
#define VBATREG_POWER_RESTORATION_TIMER   ( VBATREG_RFSYS_BASE_PTR->pwrRestorationTimer )
#define VBATREG_OUTAGE_DECLARATION_DELAY  ( VBATREG_RFSYS_BASE_PTR->pwrOutageTimer )
#define VBATREG_RTC_SR()                  ( VBATREG_RFSYS_BASE_PTR->uRtcSr )

#define VBATREG_VALID_SIG           ( ( uint8_t )0xA5 )


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
void VBATREG_EnableRegisterAccess( void );
void VBATREG_DisableRegisterAccess( void );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#endif // #ifndef vbat_reg_H
