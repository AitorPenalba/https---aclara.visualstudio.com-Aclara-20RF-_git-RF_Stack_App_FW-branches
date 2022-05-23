/***********************************************************************************************************************
 *
 * Filename: dvr_DAC0.h
 *
 * Contents: DAC0 Driver
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2020-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Melvin Francis
 *
  ***********************************************************************************************************************/
#ifndef dvr_DAC0_H
#define dvr_DAC0_H
/* INCLUDE FILES */

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

#if ( DAC_CODE_CONFIG == 1 )
returnStatus_t DVR_DAC0_init ( void );
void           DVR_DAC0_setVoltageStep ( uint16_t stepValue );
void           DVR_DAC0_disable( void );
returnStatus_t DVR_DAC0_setDacCode ( uint16_t channelNyumber, float32_t dBM );
int16_t        DVR_DAC0_setOffset ( int16_t dacOffset );
#endif
#endif
