/******************************************************************************
 *
 * Filename: hmc_msg.h
 *
 * Contents: #defs, typedefs, and function prototypes for Meter message layer
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
 * $Log$ kad Created 072007
 *
 *****************************************************************************/

#ifndef hmc_msg_H
#define hmc_msg_H

/* INCLUDE FILES */

#include "psem.h"
#include "hmc_prot.h"

#ifdef HMC_MSG_GLOBAL
   #define GLOBAL
#else
   #define GLOBAL extern
#endif

#define  HMC_CRC_HW                      //If defined then use the PIC24 hardware CRC functions
#define  CRC_CHUNK_SIZE_METER ((uint16_t)1) // One byte at a time
/* CONSTANT DEFINITIONS */

typedef union
{
   struct
   {
         unsigned	bBusy          :  1; /* Message layer processing a message */
         unsigned	bInitialized   :  1; /* Message Layer is initialized */
         unsigned	bCPUTime       :  1; /* CRC16 was performed, so CPU time was taken */
         unsigned bTxWait        :  1; /* Transmit delay in effect, don't start a new message */
         unsigned	bDataOverFlow  :  1; /* Meter sent more data than the buffer can handle. */
         unsigned	bInvalidCmd    :  1; /* Invalid Command Sent to the MSG Layer */
         unsigned	b6             :  1;
         unsigned b7             :  1;

         unsigned	bTimeOut       :  1; /* Meter didn't reply as in the time given */
         unsigned	bComError      :  1; /* General communication error */
         unsigned	bComplete      :  1; /* Message complete, no errors detected on a packet level */
         unsigned	bNAK           :  1; /* Host meter sent a NAK, protocol must be re-started */
         unsigned	bToggleBitError:  1; /* Host meter answered the wrong request */
         unsigned	b13            :  1;
         unsigned	b14            :  1;
         unsigned b15            :  1;
   }Bits;
   struct
   {
      uint8_t ucStatus;               /* General status bytes, transaction may be complete or not */
      uint8_t ucTransactionComplete;  /* Transaction complete, could be good or an error */
   }sBytes;
   uint16_t uiStatus;
}HMC_MSG_STATUS;

enum
{
   HMC_MSG_CMD_INIT = 0,      /* Initialize the message module */
   HMC_MSG_CMD_CLR_FLAGS,     /* Clear the flags, done before a new message is sent */
   HMC_MSG_CMD_NEW_SESSION,   /* Clear the flags, done before a new message is sent */
   HMC_MSG_CMD_SET_DEFAULTS,  /* Sets the default communication parameters */
   HMC_MSG_CMD_RD_COM,        /* Reads the communication parameters */
   HMC_MSG_CMD_WR_COM,        /* Writes the communication parameters */
   HMC_MSG_CMD_SEND_DATA,     /* Starts the process of sending a message */
   HMC_MSG_CMD_PROCESS,       /* Processes the message being sent/received */
   HMC_MSG_CMD_COMPORT,
   HMC_MSG_CMD_RD_COMPORT,    /* Reads comport Paramters */
   HMC_MSG_CMD_WR_COMPORT,    /* Set Comport Parameters */
};
/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

typedef struct
{
   uint8_t  ucRetries;            /* Number of retries for each message */
   uint16_t uiResponseTimeOut_mS; /* Number of mS before timing out */
   uint8_t  ucErrorTimeOut_mS;    /* Number of mS to wait on an Error */
   uint8_t  ucTurnAroundDelay;    /* Number of mS to wait before next transmission after the last byte from the meter is received */
   uint16_t uiMeterBusyDelay;     /* Number of mS to wait before next transmission after meter said it is busy */
   uint16_t uiInterCharDelay;     /* Number of mS to wait before between each byte */
   uint16_t uiTrafficTimeOut_mS;      /* Number of mS to wait before the communication is considered broke and we must abort */
//   uint16_t uiBaudRate;           /* Baud Rate of the serial port */
//   uint8_t  ucComPort;            /* Com Port for serial communications */
}HMC_MSG_Configuration;

/* GLOBAL VARIABLES */
//static uint8_t   ucResult = 0x00;           /* Used to store the received byte or result of the UART */
/* FUNCTION PROTOTYPES */

GLOBAL uint16_t HMC_MSG_Processor(uint8_t, HMC_COM_INFO *);

#undef GLOBAL

#endif
