/***********************************************************************************************************************

   Filename: DBG_SerialDebug.c

   Global Designator: DBG_

   Contents:  Contains a task to print to the debugging port.

 ***********************************************************************************************************************
   A product of Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   @author Kevin Lilard

   $Log$ Created 2012

 ***********************************************************************************************************************
   Revision History:
   v0.1 - KL 2012 - Initial Release
   v0.2 - KAD 06/02/2014 - Make this module a task
 **********************************************************************************************************************/
/* INCLUDE FILES */

#include "project.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
//#include <mqx.h>
//#include <fio.h>
//#include <io_prv.h>
//#include <charq.h>
//#include <serinprv.h>
//#include <bsp.h>
//#include <file_io.h>
#include "timer_util.h"
//#include "time_sys.h"
#include "DBG_SerialDebug.h"
//#include "ascii.h"
//#include "time_util.h"
#include "buffer.h"
//#ifndef BSP_DEFAULT_IO_CHANNEL_DEFINED
//#error This application requires BSP_DEFAULT_IO_CHANNEL to be not NULL. Please set corresponding BSPCFG_ENABLE_TTYx to non-zero in user_config.h and recompile BSP with this option.
//#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define PRNT_TRAP_ERR " BUFF_TRAP"

/* DBG_log() adds this many bytes to the string it sends.  This needs to be considered when calculating how many
   bytes can be transmitted and still fit in one debug print of size DEBUG_MSG_SIZE */
#define DBG_SIZE_OF_LINE_COUNT_AND_CATEGORY ((uint8_t)9)
#define DBG_PORT_CONFIG_VERSION  0
#define DBG_PORT_TIMEOUT_DEFAULT 0
#define DBG_PORT_ECHO_DEFAULT    1

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

#define DBG_RSVD_SIZE 30
#if( RTOS_SELECTION == FREE_RTOS )
#define SERIAL_DBG_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define SERIAL_DBG_NUM_MSGQ_ITEMS 0
#endif
#if ( MCU_SELECTED == RA6E1 )
#define MAX_DBG_COMMAND_CHARS 1602 // TODO: RA6: Question: Please add comment for the magic number. 1600 seems way more than we need.
#endif

typedef struct
{
   uint8_t  version;              // Used to track the version/format of this file
   uint8_t  PortTimeout_hh;       // Port Timeout in Hours
   bool     echoState;            // Port echo on/off
   uint8_t  rsvd[DBG_RSVD_SIZE];  // Reserved space for future additions to this file
}DBG_ConfigAttr_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static bool             EnableDebugPrint_ = ( bool )true;
//static FileHandle_t     dbgFileHndl_;                       //Contains the file handle information
#if ( PORTABLE_DCU == 1)
/* this flag disables all debug printing but still allows debug command processing */
static bool             EnableDfwMonMode = ( bool )false;
#endif

static OS_MUTEX_Obj     mutex_;
static OS_MUTEX_Obj     logPrintf_mutex_;
static OS_MUTEX_Obj     DBG_logPrintHex_mutex_;
static OS_MSGQ_Obj      mQueueHandle_;                      /* Message Queue Handle */
#if ( MCU_SELECTED == RA6E1 )
OS_SEM_Obj       dbgReceiveSem_;                           /* Used as Semaphore for interrupt method of UART_read in DBG_CommandLine.c */
extern OS_SEM_Obj       transferSem[MAX_UART_ID];          /* For RA6E1, UART_write process is used in Semaphore method */
#endif
//static _task_id         taskPrintFilter_;                   /* If set, only print messages from this task id   */
static uint16_t         line_num_ = 0;                      /* Line number used by DBG_log */
#if ENABLE_TMR_TASKS
static uint16_t         PortTimerID = INVALID_TIMER_ID;     /* Debug port timeout timer ID  */
static timer_t          PortTimerCfg;                       /* Debug port timeout Timer configuration */
#endif
static DBG_ConfigAttr_t ConfigAttr;                         /* Debug Configuration Attributes */

static char logPrintf_buf[DEBUG_MSG_SIZE];
static char DBG_logPrintHex_buf[DEBUG_MSG_SIZE];

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

static uint16_t addLogPrefixToString ( char category, char *pDst );
static void PortTimer_CallBack( uint8_t cmd, const void *pData );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#if ( MCU_SELECTED == RA6E1 )
/***********************************************************************************************************************

   Function name: DBG_printfDirect()

   Purpose: Directs printf to debug port and prints without using any buffer, task and Queues

   Arguments: const char *fmt, ...  - The print string

   Returns: None

**********************************************************************************************************************/
void DBG_printfDirect( const char *fmt, ... )
{
   static uint16_t         dbgCmdLen;
   static char             dbgCommandBuffer[MAX_DBG_COMMAND_CHARS + 1];
   static const enum_UART_ID dbgUart = UART_DEBUG_PORT;     /* UART used for DBG port prints   */
   /* Assigns the Command Length to zero */
   dbgCmdLen = 0;
   /* Declaring Variable Argument list of type va_list */
   va_list  args;
   /* Initializing arguments to store all values after fmt */
   va_start( args, fmt );
   /* Assigns data to the buffer from the Variable argument list and updates the dbgCmdLen */
   dbgCmdLen = ( uint16_t )vsnprintf( &dbgCommandBuffer[ dbgCmdLen ], ( int32_t )( sizeof( dbgCommandBuffer ) - dbgCmdLen ), fmt, args );
   /* Cleans up the Variable Argument list */
   va_end( args );
   /* Adds Carriage Return and New Line to the each line of prints */
   dbgCmdLen += ( uint16_t )snprintf( &dbgCommandBuffer[ dbgCmdLen ], ( int32_t )( sizeof( dbgCommandBuffer ) - dbgCmdLen ), "\r\n" ); 
   /* Writes the print to Debug terminal */
   UART_write( dbgUart, (uint8_t *)dbgCommandBuffer, dbgCmdLen ); 
} /* end of DBG_printfDirect() */
#endif

/***********************************************************************************************************************

   Function name: DBG_init()

   Purpose: Initializes the debug print variables

   Arguments: None

   Returns: returnStatus_t

**********************************************************************************************************************/
returnStatus_t DBG_init( void )
{
   returnStatus_t retVal = eFAILURE;
#if (FILE_IO == 1)
   FileStatus_t fileStatus;
#endif
   if (  OS_MSGQ_Create( &mQueueHandle_, SERIAL_DBG_NUM_MSGQ_ITEMS ) &&
         OS_MUTEX_Create( &mutex_ ) &&
#if ( MCU_SELECTED == RA6E1 )
         OS_SEM_Create( &dbgReceiveSem_ ) &&
         OS_SEM_Create( &transferSem[UART_DEBUG_PORT] ) &&
#endif
         OS_MUTEX_Create( &logPrintf_mutex_ ) &&
         OS_MUTEX_Create( &DBG_logPrintHex_mutex_ ) )
   {
#if (FILE_IO == 1 )
      if ( eSUCCESS == FIO_fopen(&dbgFileHndl_,                 /* File Handle so that PHY access the file. */
                                 ePART_SEARCH_BY_TIMING,        /* Search for the best partition according to the timing. */
                                 (uint16_t) eFN_DBG_CONFIG,     /* File ID (filename) */
                                 (lCnt)sizeof(DBG_ConfigAttr_t),/* Size of the data in the file. */
                                 FILE_IS_NOT_CHECKSUMED,        /* File attributes to use. */
                                 0xFFFFFFFF,                    /* The update rate of the data in the file. */
                                 &fileStatus) )                 /* Contains the file status */
      {
         if ( fileStatus.bFileCreated )
         {  // The file was just created for the first time.
            (void)memset(&ConfigAttr, 0, sizeof(ConfigAttr));
            ConfigAttr.version        = DBG_PORT_CONFIG_VERSION;   // File Format Version Default
            ConfigAttr.PortTimeout_hh = DBG_PORT_TIMEOUT_DEFAULT;  // Port Disabled
            ConfigAttr.echoState      = DBG_PORT_ECHO_DEFAULT;     // Echo ON
            retVal = FIO_fwrite( &dbgFileHndl_, 0, (uint8_t *)&ConfigAttr, (lCnt)sizeof(DBG_ConfigAttr_t));
         }
         else
         {  // Read the Config File Data
            retVal = FIO_fread( &dbgFileHndl_, (uint8_t *)&ConfigAttr, 0, (lCnt)sizeof(DBG_ConfigAttr_t));
         }
      }
#else
      retVal = eSUCCESS;
#endif /* FILE_IO */
   }
#if (TM_SEMAPHORE == 1)
OS_SEM_TestCreate();
#endif
#if (TM_MSGQ == 1)
OS_MSGQ_TestCreate();
#endif
#if( TM_EVENTS == 1 )
OS_EVENT_TestCreate();
#endif

//   DBG_PortTimer_Manage ( );
//   DBG_PortEcho_Set( DBG_PortEcho_Get() ); // Get the echo setting and update the current UART setting
   return( retVal );
}
/***********************************************************************************************************************

   Function name: DBG_TxTask()

   Purpose: prints to the terminal

   Arguments: uint32_t Arg0

   Returns: None

 **********************************************************************************************************************/
void DBG_TxTask( taskParameter )
{
//   vTaskDelay(pdMS_TO_TICKS(1000));
#if (TM_SEMAPHORE == 1)
   OS_SEM_TestPost();
#endif
#if (TM_MSGQ == 1)
   OS_MSGQ_TestPost();
#endif
#if( TM_EVENTS == 1 )
   OS_EVENT_TestSet();
#endif
//   vTaskSuspend(NULL);
   for ( ; ; )
   {

      buffer_t *pBuf;
      (void)OS_MSGQ_Pend( &mQueueHandle_, ( void * )&pBuf, OS_WAIT_FOREVER );  /* Check for message in the queue */
      OS_MUTEX_Lock( &mutex_ ); // Function will not return if it fails
#if ( MCU_SELECTED == NXP_K24 )
      ( void ) puts ( (char*)&pBuf->data[0] );
#elif ( MCU_SELECTED == RA6E1 )
      ( void )UART_write( UART_DEBUG_PORT, &pBuf->data[0], pBuf->x.dataLen );
#endif
      OS_MUTEX_Unlock( &mutex_ ); // Function will not return if it fails
      BM_free( pBuf );

   }
}  /*lint !e715 !e818  pvParameters is not used */

/***********************************************************************************************************************

   Function name: DBG_log

   Purpose: This function is to be used by the general application code to print debug information to the Debug port.
            Added functionality to allow filtering by task id.

   Arguments:  char category   - Typically 'I','E','R' or 'W'
               uint8_t options - ADD_LF          Insert \n
                                PRINT_DATE_TIME  Print date, time and taskID
               char *fmt, ...  - The print string

   Returns: None

   Notes:

 **********************************************************************************************************************/
void DBG_log ( char category, uint8_t options, const char *fmt, ... )
{
   sysTime_dateFormat_t RT_Clock;
   buffer_t *pBuf;
   uint16_t len;

   /* Print out the Debug port? */
   if ( EnableDebugPrint_
#if ( PORTABLE_DCU == 1)
         && (EnableDfwMonMode == (bool)false)
#endif
      )
   {
#if (MQX_RTOS == 1)
      if (( taskPrintFilter_ == 0 )            ||                /* Filter active? */
            taskPrintFilter_ == _task_get_id() ||                /* Filter match?  */
            _task_get_id() == _task_get_id_from_name( "DBG" ) )  /* DBG task */
#endif
      {
         OS_MUTEX_Lock( &logPrintf_mutex_ ); // Function will not return if it fails

         // Reset length
         len = 0;
         logPrintf_buf[0] = 0;

         // Check if we need to print the category
         if ( category )
         {
            /* DEVELOPER NOTE:  Update DBG_SIZE_OF_LINE_COUNT_AND_CATEGORY #def if you modify the below line */
            len += (uint16_t)snprintf( logPrintf_buf, (int32_t)sizeof( logPrintf_buf ), "[%05d %c]", ++line_num_, category );
         }
#if 0
         // Build time/data header
         if ( options & PRINT_DATE_TIME )
         {
            (void)TIME_UTIL_GetTimeInDateFormat( &RT_Clock );
            len += ( uint16_t )snprintf( &logPrintf_buf[len], (int32_t)sizeof( logPrintf_buf ) - len,
                                         "%04u/%02u/%02u %02u:%02u:%02u.%03u %s_TSK ",
                                         RT_Clock.year,
                                         RT_Clock.month,
                                         RT_Clock.day  ,
                                         RT_Clock.hour ,
                                         RT_Clock.min  , //lint !e123
                                         RT_Clock.sec,
                                         RT_Clock.msec,
                                         _task_get_template_ptr( _task_get_id() )->TASK_NAME );
         }
#endif
         // Format rest of the string
         va_list  ap;
         va_start( ap, fmt );
         len += ( uint16_t )vsnprintf( &logPrintf_buf[ len ], ( int32_t )( sizeof( logPrintf_buf ) - len ), fmt, ap );
         va_end( ap );

         // Check for excessive data for logPrintf_buf
         if ( sizeof( logPrintf_buf ) <= len )
         {
            ( void )memcpy( &logPrintf_buf[( sizeof( logPrintf_buf ) - sizeof( PRNT_TRAP_ERR ) ) - 1], PRNT_TRAP_ERR,
                            sizeof( PRNT_TRAP_ERR ) );
            len = sizeof( logPrintf_buf ) - 2;  //Account for \n and NULL
         }
         // Add \n if needed
         if ( options & ADD_LF )
         {
#if ( MCU_SELECTED == NXP_K24 )
           len += ( uint16_t )snprintf( &logPrintf_buf[ len ], ( int32_t )( sizeof( logPrintf_buf ) - len ), "\n" );
#elif ( MCU_SELECTED == RA6E1 )
           /* Added carriage return to follow printing standard */
           len += ( uint16_t )snprintf( &logPrintf_buf[ len ], ( int32_t )( sizeof( logPrintf_buf ) - len ), "\r\n" );
#endif
         }

         if ( len < sizeof( logPrintf_buf ) )
         {
            len++;      // Account for the NULL terminator
         }
         pBuf = BM_allocDebug( len );

         if ( NULL != pBuf )
         {
            ( void )memcpy( pBuf->data, logPrintf_buf, len );
            OS_MSGQ_Post ( &mQueueHandle_, pBuf );
         }
         OS_MUTEX_Unlock( &logPrintf_mutex_ ); // Function will not return if it fails
      }
   }
}  /*lint !e818 fmt could be ptr to const   */
/***********************************************************************************************************************

   Function name: DBG_printHex

   Purpose: Prints an array as hex ascii to debug port with the log information.

   Arguments:  char category
               const uint8_t *pSrc
               uint16_t num_bytes

   Returns: None

   Notes:

 **********************************************************************************************************************/
void DBG_logPrintHex ( char category, char const *str, const uint8_t *pSrc, uint16_t num_bytes )
{
   uint16_t bytesLeft;
   uint16_t lastLoc;
   bool     firstCall = ( bool )true;

   OS_MUTEX_Lock( &DBG_logPrintHex_mutex_ ); // Function will not return if it fails
   bytesLeft = num_bytes;

   while ( bytesLeft )
   {
      lastLoc = addLogPrefixToString( category, &DBG_logPrintHex_buf[ 0 ] ); // Add time stamp

      // Add str to segment
      if ( firstCall )
      {
         if ( strlen( str ) )
         {
            lastLoc += ( uint16_t )snprintf( &DBG_logPrintHex_buf[lastLoc], (int32_t)sizeof( DBG_logPrintHex_buf ) - lastLoc, str );
         }
      }
      else
      {
         // Add "Cont" to any continous segment
         lastLoc += ( uint16_t )snprintf( &DBG_logPrintHex_buf[lastLoc], (int32_t)sizeof( DBG_logPrintHex_buf ) - lastLoc, "Cont " );
      }

      firstCall = ( bool )false;

      /* Limit the output to buffer size. Save room for \n and NULL terminator; Could be an odd count in buffer, so
         reserve 3 bytes in output and include the number of bytes added inside DBG_log
         (DBG_SIZE_OF_LINE_COUNT_AND_CATEGORY) */
      num_bytes = min( bytesLeft,((( sizeof( DBG_logPrintHex_buf ) - lastLoc ) - 3) - DBG_SIZE_OF_LINE_COUNT_AND_CATEGORY) / 2);
      bytesLeft -= num_bytes;

      while( num_bytes != 0 )
      {
         lastLoc += ( uint16_t )snprintf( &DBG_logPrintHex_buf[ lastLoc ], (int32_t)sizeof( DBG_logPrintHex_buf ) - lastLoc,
                                          "%02X", *pSrc++ );
         num_bytes--;
      }
      DBG_log( category, ADD_LF, DBG_logPrintHex_buf );
   }
   OS_MUTEX_Unlock( &DBG_logPrintHex_mutex_ ); // Function will not return if it fails
}
/***********************************************************************************************************************

   Function name: addLogPrefixToString()

   Purpose: Adds the category, time and task to the string

   Arguments: char category, char *pDst

   Returns: index to the last byte (the null character)

   Notes:

 **********************************************************************************************************************/
/*lint -esym(715,category) not referenced */
static uint16_t addLogPrefixToString ( char category, char *pDst )
{
   sysTime_dateFormat_t RT_Clock;
   uint16_t len;
   (void)TIME_UTIL_GetTimeInDateFormat( &RT_Clock );
   // TODO: RA6 [name_Balaji]: Integrate once _task API's are done
#if 0
   len = ( uint16_t )sprintf(   pDst, "%04u/%02u/%02u %02u:%02u:%02u.%03u %s_TSK ",
                                RT_Clock.year,
                                RT_Clock.month,
                                RT_Clock.day  ,
                                RT_Clock.hour ,
                                RT_Clock.min  ,
                                RT_Clock.sec,
                                RT_Clock.msec,
                                _task_get_template_ptr( _task_get_id() )->TASK_NAME );
#endif
   return( len );
}  /*lint +esym(715,category) not referenced */
/***********************************************************************************************************************

   Function name: DBG_printFloat()

   Purpose: Convert a float value into a string

   Arguments:  str - string that will hold the results
               f   - floating point value to convert into string
               precision - how many decimal after the point

   Returns: index to the last byte (the null character)

   Notes:   This is fairly limited function
            The input range of the float value must be betwen -2^31 and 2^31-1
            The precision must be 0 to 6.

 **********************************************************************************************************************/
char * DBG_printFloat( char *str, float f, uint32_t precision )
{
   int32_t  integer;
   uint32_t fraction = 0;
   char     formatStr[5];

   // Make sure we can built the float number into a string
   // the integer part has to fit
   if ( ( f > ( float32_t )0x7FFFFFFF ) || ( f < ( float )( int32_t )0x80000000 ) )
   {
      ( void )strcpy( str, "Float out of range" );
      return str;
   }
   if ( precision > 6 )
   {
      ( void )strcpy( str, "Precision too large" );
      return str;
   }
   // Decompose float in integer and fractionnal part
   integer = ( int32_t )f;
   if ( precision <= 6 )
   {
      fraction = ( uint32_t )( ( fabs( ( double )f ) - abs( integer ) ) * pow( 10.0, ( double )precision ) );
   }

   // Convert integer part to a string
   ( void )sprintf( str, "%d", integer );

   // Add a decimal point if needed
   if ( precision > 0 )
   {
      ( void )strcat( str, "." );
   }
   else
   {
      return str;
   }

   // Add fractional part
   ( void )sprintf( formatStr, "%%0%uu", precision );
   ( void )sprintf( &str[strlen( str )], formatStr, fraction );

   return str;
}

#if (MQX_RTOS == 1)
/***********************************************************************************************************************

   Function name: DBG_SetTaskFilter()

   Purpose: Set the task id filter to allow prints only from this task.

   Arguments: tid - mqx task id

   Returns: none

 **********************************************************************************************************************/
void DBG_SetTaskFilter( _task_id tid )
{
   taskPrintFilter_ = tid;
}
/***********************************************************************************************************************

   Function name: DBG_GetTaskFilter()

   Purpose: Get the task id filter.

   Arguments: none

   Returns: tid - mqx task id

 **********************************************************************************************************************/
_task_id DBG_GetTaskFilter(  void )
{
   return ( taskPrintFilter_ );
}
#endif // #if (MQX_RTOS == 1)
/*******************************************************************************

   Function name: DBG_PortEcho_Get

   Purpose:  Used to get the port echo state

   Arguments: none

   Returns: Echo state 1:On, 0:Off

*******************************************************************************/
bool DBG_PortEcho_Get( void )
{
   return ConfigAttr.echoState;
}
/*******************************************************************************

   Function name: DBG_PortEcho_Set

   Purpose:  Used to set the port echo state

   Arguments: val - Echo state 1:On, 0:Off

   Returns: none

*******************************************************************************/
void DBG_PortEcho_Set ( bool val )
{
   ConfigAttr.echoState = val;
#if (FILE_IO == 1)
   (void)FIO_fwrite( &dbgFileHndl_, 0, (uint8_t const *)&ConfigAttr, (lCnt)sizeof(DBG_ConfigAttr_t));
#endif
//   (void)UART_SetEcho( UART_DEBUG_PORT, val );
}
/*******************************************************************************

   Function name: DBG_PortTimer_Get

   Purpose:  Used to get the port timeout

   Arguments: none

   Returns: uint8_t - Timeout in hours

*******************************************************************************/
uint8_t DBG_PortTimer_Get( void )
{
   return ConfigAttr.PortTimeout_hh;
}
/*******************************************************************************

   Function name: DBG_PortTimer_Set

   Purpose:  Used to set the port timeout

   Arguments: timeout in hours

   Returns: bool

*******************************************************************************/
void DBG_PortTimer_Set ( uint8_t val )
{
   ConfigAttr.PortTimeout_hh = val;
#if (FILE_IO == 1)
   (void)FIO_fwrite( &dbgFileHndl_, 0, (uint8_t const *)&ConfigAttr, (lCnt)sizeof(DBG_ConfigAttr_t));
#endif
   DBG_PortTimer_Manage ( );
}
/*******************************************************************************

   Function name: PortTimer_CallBack

   Purpose: This function is called when the debug port timer expires.
            If the timer expires, the port will be disabled.

   Arguments:  uint8_t cmd - Not used, but required due to generic API
               void *pData - Not used, but required due to generic API

   Returns: none

   Notes:

*******************************************************************************/
static void PortTimer_CallBack( uint8_t cmd, const void *pData )
{
   (void) cmd;
   (void) pData;
#if ( ENABLE_DEBUG_PORT == 0 )
#if ( ( EP == 1 ) || ( PORTABLE_DCU == 0 ) )
   EnableDebugPrint_ = ( bool )false;

   // Disable DBG port timeout
   ConfigAttr.PortTimeout_hh = 0;
#if (FILE_IO == 1)
   (void)FIO_fwrite( &dbgFileHndl_, 0, (uint8_t const *)&ConfigAttr, (lCnt)sizeof(DBG_ConfigAttr_t));
#endif
#endif
#endif
}  /*lint !e818 pData could be pointer to const */
/*******************************************************************************

   Function name: DBG_PortTimer_Manage

   Purpose: Used to manage the DBG Port Timer

   Arguments: none

   Returns: none

*******************************************************************************/
void DBG_PortTimer_Manage ( void )
{
   /* Initialize/update timer used to automatically disable port if no activity is detected before the timer expires  */
   if(ConfigAttr.PortTimeout_hh > 0)
   {
#if (TIMER_UTIL == 1)
      // Create timer if not already created
      if ( PortTimerID == INVALID_TIMER_ID )
      {
         ( void )memset( &PortTimerCfg, 0, sizeof( PortTimerCfg ) ); /* Clear the timer values */
         PortTimerCfg.bOneShot = true;
         PortTimerCfg.ulDuration_mS = TIME_TICKS_PER_HR * ConfigAttr.PortTimeout_hh;
         PortTimerCfg.pFunctCallBack = ( vFPtr_u8_pv )PortTimer_CallBack;
         ( void )TMR_AddTimer( &PortTimerCfg );           /* Create the status update timer   */
         PortTimerID = PortTimerCfg.usiTimerId;
      }
      else
      {
         ( void )TMR_ResetTimer( PortTimerID, TIME_TICKS_PER_HR * ConfigAttr.PortTimeout_hh );
      }
#endif
      EnableDebugPrint_ = ( bool )true;
   }
#if ( ENABLE_DEBUG_PORT == 0 )
#if ( ( EP == 1 ) || ( PORTABLE_DCU == 0 ) )
   else
   {
      EnableDebugPrint_ = ( bool )false;
   }
#endif
#endif
}
/*******************************************************************************

   Function name: DBG_IsPortEnabled

   Purpose:  Returns the state of the enable state

   Arguments: none

   Returns: bool

*******************************************************************************/
bool DBG_IsPortEnabled ( void )
{
   return EnableDebugPrint_ ;
}
/*******************************************************************************

   Function name: DBG_LW_printf

   Purpose:  Low stack usage print function.  This function will only support strings that will result in DEBUG_MSG_SIZE bytes or less.

   Arguments: fmt - format string

   Returns: bool

   Note: This function should be used only to print error messages that need to bypass the normal DBG_log function or printing from inside an
         interrupt

   WARNING: This call will disrupt the real-time processing because the interrupts are disabled.
            This function should be called only as part of a last gasp when we can no longer trust that MQX is running properly.
            Can be also called when we are not sure that the logger task is running or will run (e.g. at boot up)

*******************************************************************************/
void DBG_LW_printf( char const *fmt, ... )
{
#if 0
   static char    LW_printf_str[DEBUG_MSG_SIZE];
   static int32_t len;
   static int32_t i;
   static IO_SERIAL_INT_DEVICE_STRUCT_PTR ioptr = NULL;  /* Needed to get proper UART interface */

   OS_INT_disable( );

   if ( ioptr == NULL )
   {
      ioptr = (IO_SERIAL_INT_DEVICE_STRUCT_PTR)((void *)((FILE_DEVICE_STRUCT_PTR)UART_getHandle( UART_DEBUG_PORT ))->DEV_PTR->DRIVER_INIT_PTR);
   }

   // Format rest of the string
   va_list  ap;
   va_start( ap, fmt );
   len = vsnprintf( LW_printf_str, sizeof( LW_printf_str ), fmt, ap );
   va_end( ap );

   // Print string
   for ( i = 0; i < len; i++ )
   {
      // Add \r before \n
      if ( LW_printf_str[ i ] == '\n' )
      {
         (*ioptr->DEV_PUTC)(ioptr, '\r');
      }
      (*ioptr->DEV_PUTC)(ioptr, LW_printf_str[ i ]);
   }

   OS_INT_enable( );
#endif
}

#if ( PORTABLE_DCU == 1)
/*******************************************************************************

   Function name: DBG_DfwMonitorMode

   Purpose:  Enable to disable DFW monitor mode.

   Arguments: enable - set to true to enable, and false to disable this mode

   Returns: none

   Note:   This mode will disable all debug printing except for respones to the following messages:
         DFW Verify Response
         DFW Init Response
         DFW Download Confirmation
         DFW Apply Acknowledgement
         DFW Apply Confirmation
      The mode is not persistant through power cycles

*******************************************************************************/
void DBG_DfwMonitorMode( bool enable )
{
   if (enable == (bool)true)
   {
      EnableDfwMonMode = (bool)true;
   }
   else
   {
      EnableDfwMonMode = (bool)false;
   }
}
/*******************************************************************************

   Function name: DBG_GetDfwMonitorMode

   Purpose:  get current estate of dfw monitor enable mode

   Arguments: none

   Returns: true if DFW monitor mode is enabled, false otherwise

   Note:

*******************************************************************************/
bool DBG_GetDfwMonitorMode( void )
{
   return EnableDfwMonMode;
}
#endif /* end PORTABLE_DCU section */
