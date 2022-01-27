/******************************************************************************

   Filename:    HMC_REQ_.h

   Contents:  Receives a message from a mailbox, performs HMC and then responds via a mailbox send or posts a semaphore.

 ******************************************************************************
   Copyright (c) 2013 Aclara Power-Line Systems Inc.  All rights reserved.
   This program may not be reproduced, in whole or in part, in any form or by
   any means whatsoever without the written permission of:
                  ACLARA POWER-LINE SYSTEMS INC.
                  ST. LOUIS, MISSOURI USA
 ******************************************************************************

   $Log$

 *****************************************************************************/

#ifndef hmc_req_H
#define hmc_req_H

/* INCLUDE FILES */

#include "psem.h"
#include "hmc_app.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

typedef enum
{
   eHMC_WRITE = 0,   /* Partial table write  */
   eHMC_READ,        /* Partial table read   */
   eHMC_WRITE_FULL,  /* Full table write     */
   eHMC_READ_FULL,   /* Full table read      */
   eHMC_PROCEDURE    /* Initiate procedure   */
} hmcOperation_t;

typedef enum
{
   eHMC_SUCCESS = 0,
   eHMC_ABORT,
   eHMC_ERROR,
   eHMC_TBL_ERROR,
   eHMC_SIZE_ERROR,
   eHMC_NOT_PROCESSED
} hmcStatus_t;

/* TYPE DEFINITIONS */

PACK_BEGIN
typedef struct
{
   uint16_t id;
   uint32_t offset;
   uint16_t cnt;
} HMC_REQ_tblInfo_t;
PACK_END

PACK_BEGIN
typedef struct
{
   OS_QUEUE_Element  QueueElement;  /* MUST BE 1ST ELEMENT; QueueElement as defined by MQX */
   hmcOperation_t    bOperation;    /* See hmcOperation_t definition */
   HMC_REQ_tblInfo_t tblInfo;       /* Table, Offset & Length.  If offset & length are 0, then do full table access */
   void              *pData;        /* Data to write or location to store data */
   uint8_t           maxDataLen;    /* Length of pData in bytes.  Used to make sure the data doesn't overflow for full
                                       table reads. */
   uint8_t           rdLen;         /* Number of bytes placed in to the pData field */
   hmcStatus_t       hmcStatus;     /* Successful or not? */
   uint8_t           tblResp;       /* Response from the table access */
   OS_SEM_Handle     pSem;          /* Semaphore to post when complete */
} HMC_REQ_queue_t;
PACK_END

/* GLOBAL VARIABLES */

extern OS_QUEUE_Obj HMC_REQ_queueHandle;
extern uint8_t HMC_REQ_queueCreated;

/* FUNCTION PROTOTYPES */

uint8_t HMC_REQ_applet( uint8_t ucCmd, void *pData );

#endif
