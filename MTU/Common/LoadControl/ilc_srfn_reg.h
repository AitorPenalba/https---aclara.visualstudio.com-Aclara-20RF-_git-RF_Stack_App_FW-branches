/***********************************************************************************************************************
 *
 * Filename: ILC_SRFN_REG.h
 *
 * Contents:  #defs, typedefs, and function prototypes for the routines to
 *    support ILC_SRFN_REG (SRFN Registration needs)
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2017 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ kad Created 080207
 *
 **********************************************************************************************************************/

#ifndef ILC_SRFN_REG_H
#define ILC_SRFN_REG_H

/* ***************************************************************************************************************** */
/* INCLUDE FILES */
#include "ilc_dru_driver.h"
#include "intf_cim_cmd.h"

/* ***************************************************************************************************************** */
/* CONSTANT DEFINITIONS */


/* ***************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ***************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ***************************************************************************************************************** */
/* GLOBAL VARIABLES */


/* ***************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
enum_CIM_QualityCode ILC_SRFN_REG_getEdMfgSerNum( ValueInfo_t *reading );
enum_CIM_QualityCode ILC_SRFN_REG_getEdFwVerNum( ValueInfo_t *reading );
enum_CIM_QualityCode ILC_SRFN_REG_getEdHwVerNum( ValueInfo_t *reading );
enum_CIM_QualityCode ILC_SRFN_REG_getEdModel( ValueInfo_t *reading );
enum_CIM_QualityCode ILC_SRFN_REG_getReading( meterReadingType RdgType, ValueInfo_t *reading );
returnStatus_t ILC_SRFN_REG_Init ( void );
void ILC_SRFN_REG_Task( uint32_t Arg0 );

#endif

/* End of file */
