/***********************************************************************************************************************

   Filename: pwr_config.h

   Global Designator:  PWRCFG_

   Contents: Configuration parameter I/O for power outage and restoration.

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2015 - 2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   @author David McGraw

   $Log$

 ***********************************************************************************************************************
   Revision History:

   @version
   #since
 **********************************************************************************************************************/

#ifndef pwr_config_H
#define pwr_config_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"


/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */

#ifdef PWRCFG_GLOBALS
#define PWRCFG_EXTERN
#else
#define PWRCFG_EXTERN extern
#endif

#include "HEEP_UTIL.h"

/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
typedef struct
{
   uint8_t         CCA_Attempts;        /* Number of CCA attempts */
   uint8_t         pPersistAttempts;    /* Number of p-persist attempts */
   bool            MessageSent;         /* Was message sent? */
} SimStats_t;

typedef enum
{
   eSimLGDisabled = (uint8_t)0,   /* LG Simulation is disabled */
   eSimLGArmed,                   /* LG Simulation is scheduled, waiting for start time */
   eSimLGOutageDecl,              /* LG Simulation is in Outage Declaration window */
   eSimLGTx,                      /* LG Simulation is in Tx windows */
   eSimLGTxDone,                  /* LG Simulation is waiting for the Power Restoration Phase */
   eSimLGPowerOn                  /* LG Simulation is in the Power Restoration Phase */
} eSimLGState_t;

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define PWRCFG_SIM_LG_TX_ONLY                (true)
#define PWRCFG_SIM_LG_TX_ALL                 (false)

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

PWRCFG_EXTERN returnStatus_t PWRCFG_init( void );

PWRCFG_EXTERN uint16_t       PWRCFG_get_outage_delay_seconds( void );
PWRCFG_EXTERN returnStatus_t PWRCFG_set_outage_delay_seconds( uint16_t uOutageDeclarationDelaySeconds );
PWRCFG_EXTERN returnStatus_t PWRCFG_OutageDeclarationDelayHandler( enum_MessageMethod action, meterReadingType id,
                                                                     void *value, OR_PM_Attr_t *attr );
PWRCFG_EXTERN uint16_t       PWRCFG_get_restoration_delay_seconds( void );
PWRCFG_EXTERN returnStatus_t PWRCFG_set_restoration_delay_seconds( uint16_t uRestorationDelaySeconds );
PWRCFG_EXTERN returnStatus_t PWRCFG_RestorationDeclarationDelayHandler( enum_MessageMethod action, meterReadingType id,
                                                                        void *value, OR_PM_Attr_t *attr );
PWRCFG_EXTERN returnStatus_t PWRCFG_set_PowerQualityEventDuration( uint16_t uPowerQualityEventDuration );
PWRCFG_EXTERN uint16_t       PWRCFG_get_PowerQualityEventDuration( void );
PWRCFG_EXTERN returnStatus_t PWRCFG_PowerQualityEventDurationHandler( enum_MessageMethod action, meterReadingType id,
                                                                      void *value, OR_PM_Attr_t *attr );
PWRCFG_EXTERN uint8_t        PWRCFG_get_LastGaspMaxNumAttempts( void );
PWRCFG_EXTERN returnStatus_t PWRCFG_set_LastGaspMaxNumAttempts( uint8_t uLastGaspMaxNumAttempts );
PWRCFG_EXTERN returnStatus_t PWRCFG_LastGaspMaxNumAttemptsHandler( enum_MessageMethod action, meterReadingType id,
                                                                   void *value, OR_PM_Attr_t *attr );

PWRCFG_EXTERN uint32_t       PWRCFG_get_SimulateLastGaspStart( void );
PWRCFG_EXTERN returnStatus_t PWRCFG_set_SimulateLastGaspStart( uint32_t uSimulateLastGaspStart );

PWRCFG_EXTERN uint16_t       PWRCFG_get_SimulateLastGaspDuration( void );
PWRCFG_EXTERN returnStatus_t PWRCFG_set_SimulateLastGaspDuration( uint16_t uSimulateLastGaspDuration );

PWRCFG_EXTERN uint8_t        PWRCFG_get_SimulateLastGaspTraffic( void );
PWRCFG_EXTERN returnStatus_t PWRCFG_set_SimulateLastGaspTraffic( uint8_t uSimulateLastGaspTraffic );

PWRCFG_EXTERN uint8_t        PWRCFG_get_SimulateLastGaspMaxNumAttempts( void );
PWRCFG_EXTERN returnStatus_t PWRCFG_set_SimulateLastGaspMaxNumAttempts( uint8_t uSimulateLastGaspMaxNumAttempts );

PWRCFG_EXTERN void*          PWRCFG_get_SimulateLastGaspStats( void );

PWRCFG_EXTERN returnStatus_t PWRCFG_SIMLG_OR_PM_Handler( enum_MessageMethod action, meterReadingType id,
                                                         void *value, OR_PM_Attr_t *attr );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#endif // #ifndef pwr_config_H
