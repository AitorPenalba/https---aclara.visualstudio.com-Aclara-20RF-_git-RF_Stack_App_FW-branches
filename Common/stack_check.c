/******************************************************************************

   Filename: stack_check.c

   Global Designator:

   Contents: Does stack checking

 ******************************************************************************
   Copyright (c) 2015 ACLARA.  All rights reserved.
   This program may not be reproduced, in whole or in part, in any form or by
   any means whatsoever without the written permission of:
      ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "project.h"
/* Included project.h to redirect printf statement to Debug port */
// TODO: RA6 [name_Balaji]: Add stack_check.c file to RA6E1 Project if required
#include "mqx.h"
#include <fio.h>
#include <io_prv.h>
#include <charq.h>
#include <serinprv.h>
#include <serial.h>
#include <mqx_prv.h>
#include "fio.h"
#include "buffer.h"
#include "DBG_SerialDebug.h"
#include "stack_check.h"

#include <intrinsics.h>

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define FIRST_TASK_ID   0x10001 // This is always the task ID of the first task
#define NUMBER_OF_TASKS 35      // Maximum number of task supported
#define PRINT_OVERHEAD  76      // When stack_usage needs to print something, it will use xxx bytes of stack

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static struct
{
   uint32_t limit;     // stack limit
   uint32_t watermark; // High watermark
   char     name[8];   // stack name
   MQX_FILE_PTR  out;
} stackInfo[NUMBER_OF_TASKS];

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* GLOBAL FUNCTION DEFINITIONS */

/******************************************************************************

   Function name: utoa

   Purpose: Converts unsigned integer to ASCII

   Arguments: i   - unsigned interger to convert
              str - string location

   Returns: nothing

   Notes:

 *****************************************************************************/
static void utoa( unsigned int i, char str[] )
{
   unsigned int shifter = i;
   do  //Move to where representation ends
   {
      ++str;
      shifter = shifter / 10;
   }
   while( shifter );
   *str = 0;
   do  //Move back, inserting digits as u go
   {
      *--str = ( char )( ( i % 10 ) + '0' );
      i = i / 10;
   }
   while( i );
}

/******************************************************************************

   Function name: stack_check_init

   Purpose: inititalize some data related to a task.

   Arguments: task_id - task ID

   Returns: nothing

   Notes:

 *****************************************************************************/
void stack_check_init( _task_id taskID )
{
   _mem_size size;
   _mem_size used;
   _mem_size limit;
   _mqx_uint status;

   if ( ( (taskID - FIRST_TASK_ID) + 1 ) >= NUMBER_OF_TASKS )
   {
      ( void )printf( "\nERROR:stack_check_init:Increase NUMBER_OF_TASKS to at least %u\n", (taskID - FIRST_TASK_ID) + 1 );
   }
   else
   {
      status = _klog_get_task_stack_usage( taskID, &size, &used, &limit );

      if ( status == MQX_OK )
      {
         taskID -= FIRST_TASK_ID; // Subtract the first task ID
         stackInfo[taskID].limit = limit;
         stackInfo[taskID].watermark = 0xFFFFFFFF;
         ( void )strcpy( stackInfo[taskID].name, _task_get_template_ptr( taskID + FIRST_TASK_ID )->TASK_NAME );
         stackInfo[taskID].out = stdout;
      }
      else if ( status == MQX_INVALID_TASK_ID )
      {
         ( void )printf( "\nERROR:stack_check_init:Invalid task ID\n\n" );
      }
      else
      {
         ( void )printf( "\nERROR:stack_check_init:Compile-time configuration option MQX_MONITOR_STACK is not set\n\n" );
      }
   }
}

static void myStrCat( char *str, char const * s1, char const * name, char const * s2, unsigned int byte,
               char const * s3, char const *func, unsigned int line )
{
   ( void )strcpy( str, "\n\r" );
   ( void )strcat( str, s1 );
   ( void )strcat( str, ":stack_Check:Task " );
   ( void )strcat( str, name ); // Add task name
   ( void )strcat( str, s2 );
   utoa( byte, &str[strlen( str )] ); // Add byte count
   ( void )strcat( str, s3 );
   ( void )strcat( str, " Function " );
   ( void )strcat( str, func );
   ( void )strcat( str, ":" );
   utoa( line, &str[strlen( str )] );
   ( void )strcat( str, "\n\n\r" );
}

/******************************************************************************

   Function name: stack_Check

   Purpose: Check stack.
            Generates a warning when the stack high water mark is whithin stackWarning bytes.
            Generates an error when the stack oveflowed.

   Arguments: stackWarning   - Issue warning if there is less than xxx bytes left on stack
              func           - function name of the calling function
              line           - line number of the calling function

   Returns: nothing

   Notes: Not reentrant.

 *****************************************************************************/
void stack_Check( uint32_t stackWarning, char const *func, unsigned int line )
{
   IO_SERIAL_INT_DEVICE_STRUCT_PTR int_io_dev_ptr;
   uint64_t *stack_ptr;
   uint64_t *stack_top;
   static char str[140];
   char *ptr = str;

   // Print error only if debug port is enabled
   if ( DBG_GetDebug() )
   {
      _task_id taskID = _task_get_id() - FIRST_TASK_ID; // Convert taskID from 0x10001 to zero based

      // Make sure limit was initialized for this task
      if ( stackInfo[taskID].limit )
      {
         /* Check stack integrity
            We offet the current stack pointer by PRINT_OVERHEAD because printing will consume PRINT_OVERHEAD bytes of
            the stack */
         if ( ( __get_PSP() - PRINT_OVERHEAD ) < stackInfo[taskID].limit )
         {
            // Stack Overflow
            myStrCat( str, "ERROR", stackInfo[taskID].name, " overflowed stack by ",
                      stackInfo[taskID].limit - ( __get_PSP() - PRINT_OVERHEAD ),
                      " bytes. Task is suspended.", func, line );

            // Print message
            int_io_dev_ptr = ( IO_SERIAL_INT_DEVICE_STRUCT_PTR )( ( void * )( ( FILE_DEVICE_STRUCT_PTR )
                             stackInfo[taskID].out )->DEV_PTR->DRIVER_INIT_PTR );
            while( *ptr )
            {
               while( !_io_serial_int_putc_internal( int_io_dev_ptr, *ptr, IO_SERIAL_NON_BLOCKING ) )
               {}
               ptr++;
            }
            _task_block();
         }
         else
         {
            // Compute nd check watermark location
            /* This part of the code use up to PRINT_OVERHEAD bytes so I'm marking the high water mark PRINT_OVERHEAD
               byte from the stack pointer. This makes the water mark computation easier and more reliable.   */
            ( ( char* )__get_PSP() )[-PRINT_OVERHEAD] = 0; // The negative offset is because the stack grows backward

            // Find watermark location
            stack_ptr = ( uint64_t* )stackInfo[taskID].limit;       // Start at the bottom of stack
            stack_top = stack_ptr + ( stackWarning / sizeof( uint64_t ) ); // End at top of stack
            while ( stack_ptr < stack_top )
            {
               if ( *stack_ptr != ( ( ( ( uint64_t )MQX_STACK_MONITOR_VALUE ) << 32 ) | MQX_STACK_MONITOR_VALUE ) )
               {
                  break;
               } /* Endif */
               ++stack_ptr;
            } /* Endwhile */

            // Issue warning when less than stackWarning bytes are left on stack
            if ( stack_ptr < stack_top )
            {
               // Display warning only if this is new watermark
               if ( ( uint32_t )stack_ptr < stackInfo[taskID].watermark )
               {
                  stackInfo[taskID].watermark = ( uint32_t )stack_ptr; // Save new watermark
                  // It is likely that we have a stack overflow if watermark is at the stack limit
                  if ( ( uint32_t )stack_ptr == stackInfo[taskID].limit )
                  {
                     // Stack Overflow
                     myStrCat( str, "ERROR", stackInfo[taskID].name, " likely overflowed stack by at least ",
                               1, " byte. Task is suspended.", func, line );

                     // Print message
                     int_io_dev_ptr = ( IO_SERIAL_INT_DEVICE_STRUCT_PTR )( ( void * )( ( FILE_DEVICE_STRUCT_PTR )
                                      stackInfo[taskID].out )->DEV_PTR->DRIVER_INIT_PTR );
                     while( *ptr )
                     {
                        while( !_io_serial_int_putc_internal( int_io_dev_ptr, *ptr, IO_SERIAL_NON_BLOCKING ) )
                        {}
                        ptr++;
                     }
                     _task_block();

                  }
                  else
                  {
                     myStrCat(   str, "WARNING", stackInfo[taskID].name, " has ",
                                 ( uint32_t )stack_ptr - stackInfo[taskID].limit,
                                 " bytes left on stack.", func, line );

                     // Print message
                     int_io_dev_ptr = ( IO_SERIAL_INT_DEVICE_STRUCT_PTR )( ( void * )( ( FILE_DEVICE_STRUCT_PTR )
                                      stackInfo[taskID].out )->DEV_PTR->DRIVER_INIT_PTR );
                     while( *ptr )
                     {
                        while( !_io_serial_int_putc_internal( int_io_dev_ptr, *ptr, IO_SERIAL_NON_BLOCKING ) )
                        {}
                        ptr++;
                     }
                     /* Wait for buffer to be empty. We do that in case the rest of code crashes. This way, we got to
                        print the warning before the crash.*/
                     while( !_CHARQ_EMPTY( int_io_dev_ptr->OUT_QUEUE ) )
                     {}
                  }
               }
            }
         }
      }
   }
}
