/***********************************************************************************************************************
 *
 * Filename: Stack_Manager.h
 *
 * Contents:
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2010-2016 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
#ifndef SM_H
#define SM_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
#ifdef ERROR_CODES_H_
returnStatus_t SM_init( void );
#endif

void SM_Task ( uint32_t Arg0 );
void SM_NwState_Task(uint32_t arg0);
void SM_WaitForStackInitialized(void);

#ifdef SM_Protocol_H
uint32_t SM_StartRequest(SM_START_e state, SM_ConfirmHandler cb_confirm);
uint32_t SM_StopRequest (SM_ConfirmHandler cb_confirm);
SM_GetConf_t SM_GetRequest(SM_ATTRIBUTES_e eAttribute);
SM_SetConf_t SM_SetRequest( SM_ATTRIBUTES_e eAttribute, void const *val);
uint32_t SM_RefreshRequest(void);

void SM_Stats(bool option);
void SM_ConfigPrint(void);

typedef void (*SM_IndicationHandler)(buffer_t *indication);
void SM_RegisterEventIndicationHandler(SM_IndicationHandler pCallback);

#endif

#endif /* SM_H */




