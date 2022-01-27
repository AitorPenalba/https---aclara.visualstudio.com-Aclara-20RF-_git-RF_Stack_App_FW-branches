/***********************************************************************************************************************
 * Filename:    hmc_mtr_lp_tsk.h
 *
 * Contents:    #defs, typedefs, and function prototypes for reading LP data from the host meter.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$
 *
 *  Created 06/24/13     KAD  Created
 *
 ***********************************************************************************************************************
 ***********************************************************************************************************************
 * Revision History:
 * v1.0 - Initial Release
 **********************************************************************************************************************/

#ifndef hmc_mtr_lptsk_h  /* replace filetemp with this file's name and remove this comment */
#define hmc_mtr_lptsk_h

#ifdef HMC_LPTSK_GLOBAL
#define HMC_LPTSK_EXTERN
#else
#define HMC_LPTSK_EXTERN extern
#endif

/* INCLUDE FILES */

#include "project.h"
#include "ansi_tables.h"
#include "hmc_mtr_info.h"
#include "time_util.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

typedef struct
{
   OS_QUEUE_Element  QueueElement;   /* QueueElement as defined by MQX */
   quantity_t        eReadingType;   /* Quantity is being requested */
   sysTime_t          reqTime;        /* Date and time of the data being requested */
   uint8_t             intTime;        /* Interval Time in minutes.  Correspondes to LP_DATA_SETx.MAX_INT_TIME_SETx */
   calcSiData_t      *pSiData;       /* Location to store the result */
   uint8_t             dataSize;       /* Size of pData */
   returnStatus_t    retVal;         /* Result of request */
   OS_SEM_Handle     pSem;          /* Semaphore to post when operation is complete */
}HMC_LPTSK_queue_t;                  /* Data format of the queue for LP task data request. */


/* GLOBAL VARIABLES */

HMC_LPTSK_EXTERN OS_QUEUE_Obj  HMC_LPTSK_queueHandle; /* Queue handle for requesting data from LP */

/* FUNCTION PROTOTYPES */

uint8_t HMC_LP_initApp(uint8_t, void *);

#undef HMC_LPTSK_EXTERN

#endif
