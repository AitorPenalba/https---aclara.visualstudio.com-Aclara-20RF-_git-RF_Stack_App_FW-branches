/***********************************************************************************************************************
 *
 * Filename: MFG_Port.h
 *
 * Contents: Contains prototypes and definitions for the manufacturing port.
 *
 *  NOTES:  When passing a message to the MFG Port, the message should be using a buffer of type buffer_t.  The dataLen
 *          and eSysFmt parameters must be filled in properly in the buffer_t structure.  Once the message is processed
 *          the buffer_t will be freed.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2014-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ Karl Davlin Created 05-01-2014
 *
 ***********************************************************************************************************************
 * Revision History:
 * 05/01/2014 - Initial Release
 *
 * #since      2014-05-01
 *
 **********************************************************************************************************************/
#ifndef MFG_Port_H
#define MFG_Port_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>
#include <stdbool.h>
#include "OS_aclara.h"
#include "error_codes.h"
#include "portable_freescale.h"
#include "buffer.h"

/* ****************************************************************************************************************** */
   /* GLOBAL DEFINTION */

#ifdef MFGP_GLOBALS
#define MFGP_EXTERN
#else
#define MFGP_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* #DEFINE DEFINITIONS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define MFGP_5_MINUTES_IN_MILISECS  (5*60*1000)          /* Connection timeout */
#define MFGP_EVENT_CHAR_RX          (0x01)               /* A character was received. */
#define MFGP_EVENT_COMMAND          (0x02)               /* Decrypted command was received. */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   MFG_SERIAL_IO_e,                     /* MFG serial port mode */
   DTLS_SERIAL_IO_e,                    /* DTLS serial port mode */
#if ( ENABLE_B2B_COMM == 1 )
   B2B_SERIAL_IO_e                      /* Change to B2B Functionality */
#endif
} mfgPortState_e;

typedef struct
{
   uint16_t    cfgLayer;               /* Configuration layer ID */
   uint16_t    destLayer;              /* Destination layer ID */
   OS_MSGQ_Obj msgQueueHndl;           /* Message queue handle to send the message to */
   uint8_t     upperLayerIgnore;       /* 1 - Ignore data coming from upper layer, 0 = don't ignore */
   uint8_t     upperLayerListen;       /* 1 - Listen to data from upper layer (make copy & send), 0 = don't Listen */
   uint8_t     upperLayerIntercept;    /* 1 - Redirect data from upper layer, 0 = don't redirect */
   uint8_t     lowerLayerIgnore;       /* 1 - Ignore data coming from lower layer, 0 = don't ignore */
   uint8_t     lowerLayerListen;       /* 1 - Listen to data from lower layer (make copy & send), 0 = don't Listen */
   uint8_t     lowerLayerIntercept;    /* 1 - Redirect data from lower layer, 0 = don't redirect */
   uint8_t     loggingOn;              /* 1 - Send data to log */
} cfgLayerMfg_t;                        /* Configuration sent to a layer of code */

PACK_BEGIN
typedef struct  PACK_MID
{
   uint16_t dstLayer;                  /* The destination */
   uint16_t srcLayer;                  /* The origen of the message */
   uint8_t  direction;                 /* 1 - At the destination, send data UP the stack, 0 - send it down the stack */
   uint8_t  log;                       /* 1 - send to log, 0 - don't */
   uint8_t  data[];                    /* Data */
}vlfDataLayer_t;                       /* Data from a layer of code */
PACK_END

typedef struct
{
   uint16_t srcLayer;
   uint8_t  data[];
}vlfDataTerminal_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/**
 * MFGP_rfTestTimerReset -  Reset rf test mode timer
 *
 * @param  None
 * @return None
 */
void MFGP_rfTestTimerReset( void );

/**
 * MFG_PhyStTxCwTest -  Put the transmitter into CW mode, at the requested frequency
 *
 * @param  radio_channel
 * @return returnStatus_t
 */
returnStatus_t MFG_PhyStTxCwTest( uint16_t radio_channel );

/**
 * MFG_PhyStRx4GFSK -  Put the receiver into RX mode at the requested frequency
 *
 * @param  radio_channel
 * @return none
 */
void MFG_PhyStRx4GFSK( uint16_t radio_channel );

/**
 * MFG_StTxBlurtTest_Srfn -  Send a message to test Tx
 *
 * @param  None
 * @return None
 */
void MFG_StTxBlurtTest_Srfn( void );

/**
 * MFGP_init - Initializes the Manufacturing port
 *
 * @param  None
 * @return returnStatus_t - eSUCCESS or eFAILURE
 */
returnStatus_t MFGP_init( void );

/**
 * MFGP_cmdInit - Initializes the Manufacturing command task
 *
 * @param  None
 * @return returnStatus_t - eSUCCESS or eFAILURE
 */
returnStatus_t MFGP_cmdInit( void );

/**
 * MFGP_task - Manufacturing port task
 *
 * @param  uint32_t Arg0 - Not used
 * @return None
 */
void           MFGP_task( uint32_t Arg0 );

/**
 * MFGP_optoTask - Manufacturing port task
 *
 * @param  uint32_t Arg0 - Not used
 * @return None
 */
void           MFGP_optoTask( uint32_t Arg0 );
/**
 * MFGP_getQuietMode - Get system quiet mode
 *
 * @param  none
 * @return uint8_t
 */
uint8_t MFGP_getQuietMode( void );

/**
 * MFGP_uartRecvTask - Manufacturing port UART task
 *
 * @param  taskParameter - Not used
 * @return None
 */
void           MFGP_uartRecvTask ( taskParameter );

/**
 * MFGP_uartCmdTask - Manufacturing port command task
 *
 * @param  taskParameter - Not used
 * @return None
 */
void           MFGP_uartCmdTask ( taskParameter );

/**
 * MFGP_AllowDtlsConnect - Allow UART traffic
 *
 * @param  none
 * @return true -Allow traffic false- dont allow traffic
 */
uint32_t MFGP_AllowDtlsConnect(void);

/**
 * MFGP_DtlsInit - Intialize the MFG Dtls port
 *
 * @param  UART number
 * @return none
 */
void MFGP_DtlsInit( enum_UART_ID uartId );

/**
 * MFGP_UartRead - Read the SLIP data from the port
 *
 * @param  uint8_t rxByte - byte read
 * @return None
 */
void MFGP_UartRead(uint8_t rxByte);

#if (USE_DTLS == 1)
/**
 * MFGP_UartWrite - write the SLIP data from the port
 *
 * @param  char bfr - pointer to the UART buffer
 *         int len  - lengthof the buffer to write
 * @return None
 */
void MFGP_UartWrite(const char *bfr, int len);

/**
 * MFGP_EncryptBuffer - Encrypt a buffer;
 *
 * @param  char *bfr - buffer to encrypt
 *         uint32_t bufSz - size of the buffer to encrypt
 * @return None
 */
void MFGP_EncryptBuffer(const char *bfr, uint16_t bufSz);

/**
 * MFGP_DtlsResultsCallback - Callback from DTLS on results of connect
 *
 * @param  uint32_t results eSUCCESS if connect or eFAILURE if not
 * @return none
 */
void MFGP_DtlsResultsCallback(returnStatus_t results);

/**
 * MFGP_GetEventHandle - Get the event handler
 *
 * @param  None
 * @return Event Handle
 */
OS_EVNT_Obj *MFGP_GetEventHandle(void);

/**
 * MFGP_getSerialTimeout - Get the event handler
 *
 * @param  None
 * @return timeout in milliseconds
 */
uint32_t MFGP_getSerialTimeout(void);

/**
 * MFGP_DtlsConnectionClosed - Called to clean up DTLS session
 *
 * @param  None
 * @return None
 */
void MFGP_DtlsConnectionClosed(void);

/**
 * MFGP_ProcessDecryptedCommand - Process a decrypted command
 *
 * @param  buffer_t *bfr - system buffer with command
 * @return None
 */
void MFGP_ProcessDecryptedCommand(buffer_t *bfr);


#endif

#if ( MCU_SELECTED == RA6E1 )
void MFG_UpdatePortState ( mfgPortState_e state );
#endif
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#undef MFGP_EXTERN

#endif /* this must be the last line of the file */
