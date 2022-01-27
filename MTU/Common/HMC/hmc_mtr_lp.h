/***********************************************************************************************************************
 * Filename:    hmc_mtr_lp.h
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
 *  Created 04/18/13     KAD  Created
 *
 ***********************************************************************************************************************
 ***********************************************************************************************************************
 * Revision History:
 * v1.0 - Initial Release
 **********************************************************************************************************************/

#ifndef hmc_mtr_lp_h  /* replace filetemp with this file's name and remove this comment */
#define hmc_mtr_lp_h

#ifdef HMC_LP_GLOBAL
#define HMC_LP_EXTERN
#else
#define HMC_LP_EXTERN extern
#endif

/* INCLUDE FILES */

#include "project.h"
#include "ansi_tables.h"
#include "hmc_mtr_info.h"
#include "hmc_mtr_lp.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

typedef struct
{
   lpSelSet_t  lpSelSet;
   uint8_t       intFmtCde;
   uint16_t      scalarsSet;
   uint16_t      divisorsSet;
}loadProfileCtrl_t;

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

uint8_t          HMC_LP_initApp(uint8_t, void *);
returnStatus_t HMC_LD_getLPTbl(stdTbl61_t **pStdTbl61);
returnStatus_t HMC_LD_readSetxChx(uint8_t set, uint8_t ch, loadProfileCtrl_t *pLpCtrl);
returnStatus_t HMC_LP_getQuantityProp(uint8_t set, quantity_t qs, uint8_t *pCh, quantDesc_t *pProperties);
returnStatus_t HMC_LP_getFmtSize(uint8_t *pSize);

#undef HMC_LP_EXTERN

#endif
