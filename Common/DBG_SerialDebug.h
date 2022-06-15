/***********************************************************************************************************************
 *
 * Filename: DBG_SerialDebug.h
 *
 * Global Designator: DBG_
 *
 * Contents: Contains prototypes for the debugger port
 *
 ***********************************************************************************************************************
 * A product of Aclara Technologies LLC Confidential and Proprietary, Copyright 2012-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
#ifndef DBG_SerialDebug_H
#define DBG_SerialDebug_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#ifndef __BOOTLOADER
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#endif
#include "error_codes.h"
#endif /* __BOOTLOADER  */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define DEBUG_MSG_SIZE              ((uint16_t)584)   /* Maximum size message allowed. Long enough to display a PHY frame at this time */
#define SIZEOF_CRLF                 ((uint8_t)2)      /* Size of a CR and LF command in a string */
#define SIZEOF_TERMINATOR           ((uint8_t)1)      /* Size of the null string terminator */
#define PRINT_FLOAT_SIZE            20

// Print options
#define ADD_LF           (1<<0) // Insert \n
#define PRINT_DATE_TIME  (1<<1) // Print date, time and taskID


// Print category, string, adds \n and time stamp
#define DBG_logPrintf( category, fmt, ... ) DBG_log( category, ADD_LF|PRINT_DATE_TIME, fmt, ##__VA_ARGS__)
// Print string, adds \n
#define DBG_printf(fmt, ... )               DBG_log( 0, ADD_LF, fmt, ##__VA_ARGS__)
// Print string
#define DBG_printfNoCr(fmt, ... )           DBG_log( 0, 0, fmt, ##__VA_ARGS__)
// Print 'E', string, adds \n and time stamp
#define ERR_printf( fmt, ... )              DBG_log( 'E', ADD_LF|PRINT_DATE_TIME, fmt, ##__VA_ARGS__)
// Print 'I', string, adds \n and time stamp
#define INFO_printf( fmt, ... )             DBG_log( 'I', ADD_LF|PRINT_DATE_TIME, fmt, ##__VA_ARGS__)
// Print 'I', string (and HEX payload), adds \n and time stamp
#define INFO_printHex(str, data, len)       DBG_logPrintHex ( 'I', str, data, len)

#if 1 /* TODO: RA6E1: This is temporary routing; Add DBG_LW_printf support in RA6 */
#define DBG_LW_printf( fmt, ... )           DBG_log( 'I', ADD_LF|PRINT_DATE_TIME, fmt, ##__VA_ARGS__)
#endif

#define APP_PRINT(fmt, ...)                 DBG_log( 0, ADD_LF, fmt, ##__VA_ARGS__);
#define APP_ERR_PRINT(fmt, ...)             DBG_log( 'E', ADD_LF|PRINT_DATE_TIME, fmt, ##__VA_ARGS__ )

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

returnStatus_t DBG_init( void );
#if ( MCU_SELECTED == RA6E1 )
void           DBG_printfDirect( const char *fmt, ... );
#endif
void           DBG_lockPrintResource( void );
void           DBG_releasePrintResource( void );
void           DBG_TxTask( taskParameter );
void           DBG_LogTask( taskParameter );
void           DBG_log ( char category, uint8_t options, const char *fmt, ... );
void           DBG_logPrintHex( char category, char const *str, const uint8_t *Data, uint16_t Length );
void           DBG_EnableDebug ( void );
void           DBG_DisableDebug ( void );
bool           DBG_GetDebug ( void );
void           DBG_processMfgPortCmd(uint8_t const *pSrc, uint16_t len);
char          *DBG_printFloat(char *str, float f, uint32_t precision);
//void           DBG_SetTaskFilter( _task_id tid );  /* TODO: RA6E1: Add Support*/
//_task_id       DBG_GetTaskFilter(  void );         /* TODO: RA6E1: Add Support*/
#if 0 /* TODO: RA6E1: Add Support*/
void           DBG_LW_printf( char const *fmt, ... );
#endif
#if ( PORTABLE_DCU == 1)
void           DBG_DfwMonitorMode ( bool enable );
bool           DBG_GetDfwMonitorMode( void );
#endif

bool    DBG_IsPortEnabled ( void );
void    DBG_PortTimer_Set ( uint8_t val );
uint8_t DBG_PortTimer_Get( void );
void    DBG_PortTimer_Manage ( void );
void    DBG_PortEcho_Set ( bool val );
bool    DBG_PortEcho_Get( void );

#endif /* this must be the last line of the file */
