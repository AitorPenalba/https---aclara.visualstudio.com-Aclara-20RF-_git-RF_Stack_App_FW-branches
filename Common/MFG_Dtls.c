/******************************************************************************

   Filename: MFG_Dtls.c

   Global Designator: MFG

   Contents:
      This file contains the MFG DTLS calls. It also contains the
      internal calls to support the APIs.

 ******************************************************************************
   Copyright (c) 2015 ACLARA.  All rights reserved.
   This program may not be reproduced, in whole or in part, in any form or by
   any means whatsoever without the written permission of:
      ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "project.h"
#if (USE_DTLS == 1)
#include "DBG_SerialDebug.h"
#include "buffer.h"
#include "STACK.h"
#include "dtls.h"

#include "PHY_Protocol.h"
#include "time_util.h"
#include "MFG_Port.h"    // Include self
#if ( RTOS_SELECTION == MQX_RTOS )
#include "mqx.h"
#include "bsp.h"
#include "io_prv.h"
#include "charq.h"
#include "serinprv.h"
#include "psp_cpudef.h"
#endif
#if (USE_USB_MFG == 1)
#include "virtual_com.h"
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */
#define END                         (0xC0)  /* SLIP end character */
#define ESC                         (0xDB)  /* SLIP Escpape character */
#define ESCEND                      (0xDC)  /* SLIP Escape End character */
#define ESCESC                      (0xDD)  /* SLIP Escape Escape character */

#define RX_START_STATE              (1)      /* SLIP state START */
#define RX_END_STATE                (2)      /* SLIP state END */
#define RX_ESC_STATE                (3)      /* SLIP State ESC received */
#define RX_DATA_STATE               (4)      /* SLIP State DATA received */

#define SLIP_DEFAULT_SIZE           (128)    /* SLIP default size buffer */
#define SLIP_DEFAULT_INCREMENT      (128)     /* SLIP default increase buffer size */
#ifdef TM_DTLS_UNIT_TEST
/* These values are used for testing an will take 8 minutes total to run the test */
#define MFGP_12_HOURS_IN_SECONDS    (8*60*1000)  /* Total tarpit time */
#define MFGP_15_MINUTES_IN_SECONDS  (1*60*1000)     /* Timeout for Tarpit */
#else
#define MFGP_12_HOURS_IN_SECONDS    (12*60*60*1000)  /* Total tarpit time */
#define MFGP_15_MINUTES_IN_SECONDS  (15*60*1000)     /* Timeout for Tarpit */
#endif
#define MFGP_MAX_FREE_ATTEMPTS      (4)      /* Free connection attempts */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
#if !USE_USB_MFG
static void          *dtlsUART_isr_data;        /* MQX isr data pointer */
#if ( RTOS_SELECTION == MQX_RTOS )
static INT_ISR_FPTR  mqxUART_isr = NULL;        /* MQX UART error handler entry  */
#endif
#endif
static buffer_t      *_SlipBuffer;              /* Contains the SLIP buffer */
static uint8_t       _RxState;                  /* Contains the SLIP state value */
static uint8_t       _AuthenticationAttempts;   /* Contains the number of connection attempts */
static uint32_t      _TimeForAuthentication;    /* Contains the time until another connection is allowed */
static uint32_t      _AuthenticationStart;      /* Contains the time stamp when the Tarpit started */
static TIMESTAMP_t   _DateTime;                 /* Contains converted time to seconds */

/*lint -esym(551,_FailedAuthenticationCounter) */
static uint32_t      _FailedAuthenticationCounter; /* Contains the number of times the full TARPIT failed */
#if !USE_USB_MFG
static enum_UART_ID  dtlsUart = UART_MANUF_TEST;   /* UART to use for dtls I/O.  */
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static void       mfgpBufferInit( buffer_t *bfr );
static buffer_t   *mfgpCreateBuffer( uint16_t size );
static buffer_t   *mfgpGrowBuffer( buffer_t *bfr, uint16_t newsize );
static void       mfgpSlipAddCharacter( buffer_t **bfr_ptr, char c );
#if !USE_USB_MFG
static void       dtlsUART_isr( void *user_isr_ptr );
#endif

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
/***********************************************************************************************************************

   Function name: mfgpBufferInit

   Purpose: This function is called to initialize a buffer

   Arguments:  buffer_t *bfr      - pointer to the buffer

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void mfgpBufferInit( buffer_t *bfr )
{
   bfr->eSysFmt = eSYSFMT_NWK_INDICATION;
   bfr->x.dataLen = 0;
}
/***********************************************************************************************************************

   Function name: mfgpCreateBuffer

   Purpose: This function is called to create a new buffer

   Arguments: uint16_t size       - size of the buffer to create

   Returns: None

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
static buffer_t *mfgpCreateBuffer( uint16_t size )
{
   buffer_t *bfr = ( buffer_t * )BM_allocStack( size );
   if( bfr != NULL )
   {
      mfgpBufferInit( bfr );
   }
   else
   {
      DBG_printf( "mfgpCreateBuffer BM_allocStack( %hu ) failed.", size );
   }
   return( bfr );
}
/***********************************************************************************************************************

   Function name: mfgpGrowBuffer

   Purpose: This function is called to grow the buffer if more data is to be inserted

   Arguments:  buffer_t *bfr    - buffer to grow
               uint16_t newsize - new size of the buffer

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static buffer_t *mfgpGrowBuffer( buffer_t *bfr, uint16_t newsize )
{
   buffer_t *new_bfr = mfgpCreateBuffer( newsize );
   if( new_bfr != NULL )
   {
      ( void )memcpy( ( void * )new_bfr->data, ( void * )bfr->data, bfr->x.dataLen );
      new_bfr->x.dataLen = bfr->x.dataLen;
   }

   BM_free( bfr );
   return( new_bfr );
}
/***********************************************************************************************************************

   Function name: mfgpSlipAddCharacter

   Purpose: This function is called to add a character to the buffer. If the buffer gets full it will make a call to
            grow the buffer.

   Arguments:  buffer_t **bfr_ptr   - buffer to make bigger
               char c               - the character to add to the buffer

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void mfgpSlipAddCharacter( buffer_t **bfr_ptr, char c )
{
   buffer_t *bfr = *bfr_ptr;
   if( bfr->x.dataLen >= bfr->bufMaxSize )
   {
      *bfr_ptr = mfgpGrowBuffer( bfr, bfr->x.dataLen + SLIP_DEFAULT_INCREMENT );
      if( *bfr_ptr == NULL )
      {
         return;
      }
      bfr = *bfr_ptr;
   }
   bfr->data[ bfr->x.dataLen++ ] = c;

   return;
}
/***********************************************************************************************************************

   Function name: MFGP_UartWrite

   Purpose: This function is called to wrap bytes into a SLIP format and send them out the UART port.

   Arguments:  const char *bfr     - Buffer of data to wrap and ship
               int len             - Length of the data

   Returns: None

   Side Effects: None

   Reentrant Code: None

   Notes:

 ******************************************************************************************************************** */
void MFGP_UartWrite( const char *bfr, int len )
{
   char c;                       /* One character out of the buffer */
   buffer_t *sb;                 /* Buffer of data used to convert the data into SLIP packet */
   OS_EVNT_Obj *eventHandle;     /* MFG event handle */

   sb = mfgpCreateBuffer( SLIP_DEFAULT_SIZE );
   if( sb == NULL )
   {
      ERR_printf( "Cannot allocate a new buffer" );
      return;
   }

   /* Convert data into a SLIP packet */
   mfgpSlipAddCharacter( &sb, END );

   while( --len >= 0 )
   {
      switch( c = *bfr++ )
      {
         case END:
         {
            mfgpSlipAddCharacter( &sb, ESC );
            mfgpSlipAddCharacter( &sb, ESCEND );
            break;
         }
         case ESC:
         {
            mfgpSlipAddCharacter( &sb, ESC );
            mfgpSlipAddCharacter( &sb, ESCESC );
            break;
         }
         default:
         {
            mfgpSlipAddCharacter( &sb, c );
         }
      }
   }

   mfgpSlipAddCharacter( &sb, END );

#if !USE_USB_MFG
   ( void )UART_write( dtlsUart, ( uint8_t * )sb->data, sb->x.dataLen );
   ( void )UART_flush ( dtlsUart );
#else
   usb_send( (char const *)sb->data, sb->x.dataLen );
#endif
   BM_free( sb );

   eventHandle = MFGP_GetEventHandle();

   /* Let the command task continue with the next printf */
   OS_EVNT_Set( eventHandle, MFGP_EVENT_COMMAND );

}
/***********************************************************************************************************************

   Function name: MFGP_EncryptBuffer

   Purpose: This function is called to encrypt a buffer.

   Arguments:  const char *bfr      - Buffer to encrypt
               uint16_t bufSz       - Size of the buffer

   Returns: None

   Side Effects: None

   Reentrant Code: None

   Notes:

 ******************************************************************************************************************** */
void MFGP_EncryptBuffer( const char *bfr, uint16_t bufSz )
{
   buffer_t *sb;
   OS_EVNT_Obj *eventHandle = MFGP_GetEventHandle();

   sb = mfgpCreateBuffer( bufSz );
   if ( sb != NULL )
   {
      ( void )memcpy( ( char * )sb->data, bfr, bufSz );

#if ( DTLS_DEBUG == 1 )
      INFO_printf( "Encrypt: %s", bfr );
#endif
      sb->eSysFmt = eSYSFMT_NWK_REQUEST;
      sb->x.dataLen = bufSz;
      DTLS_UartTransportIndication( sb );
      ( void )OS_EVNT_Wait( eventHandle, MFGP_EVENT_COMMAND, ( bool )false, OS_WAIT_FOREVER );
   }
   else
   {
      ERR_printf( "No buffer skipping encryption" );
   }
}
/***********************************************************************************************************************

   Function Name: MFGP_DtlsResultsCallback

   Purpose: This function is called by the DTLS task when it has the status of the connection.

            The first 4 attempts to connect have no time limits. After 4 attempts
            a 15 minute timer is set. After each failed attempt the timer will
            double until 12 hours max is reached. After 12 hours an event is logged
            and the process starts over.


   Arguments: returnStatus_t results  - eSUCCESS if connection was made, eFAILURE if it failed.

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
void MFGP_DtlsResultsCallback( returnStatus_t result )
{

   /*
      Upon success of a DTLS session the MFG port will enter the DTLS serial mode.  Upon a failure, the code will keep
      track of how many attempts are made to connect. The user gets 4 attempts after that the port will enter the tarpit
      mode. The wait time between each attempt starts at 15 minutes and doubles every time a failure is made after the
      alloted time has elapsed.  This continues until 12 hours have elaplsed. If after 12 hours there are still failed
      attempts then the port will log the event and start over at 0 attempts.
   */
   if ( result == eSUCCESS )
   {
      _AuthenticationAttempts = 0;
      _TimeForAuthentication = MFGP_15_MINUTES_IN_SECONDS;
      _AuthenticationStart = 0;
   }
   else
   {
      /* Make sure that the port goes back to the MFG mode */
      MFGP_DtlsConnectionClosed();

      if ( _AuthenticationAttempts == MFGP_MAX_FREE_ATTEMPTS )
      {
         _AuthenticationStart = _DateTime.seconds;
      }

      _AuthenticationAttempts++;
      if ( _AuthenticationAttempts > MFGP_MAX_FREE_ATTEMPTS )
      {
         _TimeForAuthentication *= 2;
         if ( _TimeForAuthentication >= MFGP_12_HOURS_IN_SECONDS )
         {
            _AuthenticationAttempts = 0;
            _FailedAuthenticationCounter++;
         }
      }
      INFO_printf( "SERIAL FAIL attempt:%d Timeout: %d Cntr: %d",
                   _AuthenticationAttempts, _TimeForAuthentication, _FailedAuthenticationCounter );
   }
}
/***********************************************************************************************************************

   Function name: MFGP_UartRead

   Purpose: This function is called to read SLIP packets from the UART port.

   Arguments: uint8_t rxByte      - The byte read from hardware

   Returns: None

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void MFGP_UartRead( uint8_t rxByte )
{

   /* Create the buffer if it doesnt exist */
   if( _SlipBuffer == NULL )
   {
      _SlipBuffer = mfgpCreateBuffer( SLIP_DEFAULT_SIZE );
      if ( !_SlipBuffer )
      {
         ERR_printf( "Failed to allocate buffer for read" );
         return;
      }
      _RxState = RX_START_STATE;
   }

   /*****************************************************************************
      The code below is a SLIP state model and processes the packet as described
      in the SLIP documentation.
   ****************************************************************************/

   switch( _RxState )
   {

      case RX_START_STATE:
      {
         if( rxByte == END )
         {
            _RxState = RX_END_STATE;
         }
         break;
      }

      case RX_END_STATE:
      {
         if( rxByte == END )
         {
            _RxState = RX_DATA_STATE;
         }
         else
         {
            if( rxByte == ESC )
            {
               _RxState = RX_ESC_STATE;
            }
            else
            {
               mfgpSlipAddCharacter( &_SlipBuffer, rxByte );
               _RxState = RX_DATA_STATE;
            }
         }
      }
      break;

      case RX_ESC_STATE:
      {
         if( rxByte == ESCEND )
         {
            mfgpSlipAddCharacter( &_SlipBuffer, END );
         }
         else
         {
            if( rxByte == ESCESC )
            {
               mfgpSlipAddCharacter( &_SlipBuffer, ESC );
            }
            else
            {
               mfgpSlipAddCharacter( &_SlipBuffer, rxByte );
            }
         }
         _RxState = RX_DATA_STATE;
         break;
      }

      case RX_DATA_STATE:
      {

         switch ( rxByte )
         {
            case END:
            {

               if ( _SlipBuffer->x.dataLen )
               {

                  uint32_t cksum = 0;

                  for ( int i = 0; i < _SlipBuffer->x.dataLen; i++ )
                  {
                     cksum += ( uint8_t )( _SlipBuffer->data[i] );
                  }

                  INFO_printf( "UART RX sz = %d cksum = %d", _SlipBuffer->x.dataLen, cksum );
                  DTLS_UartTransportIndication( _SlipBuffer );
               }
               else
               {
                  BM_free( _SlipBuffer );
               }
               /* Reset the buffer back */
               _RxState = RX_START_STATE;
               _SlipBuffer = NULL;
               break;
            }

            case ESC:
            {
               _RxState = RX_ESC_STATE;
               break;
            }
            default:
            {
               mfgpSlipAddCharacter( &_SlipBuffer, ( char )rxByte );
            }
            break;
         }
         break;
      }
      default:
      {
         break;
      }
   }
}
/***********************************************************************************************************************

   Function name: MFGP_AllowDtlsConnect

   Purpose: This function is called to check the TARPIT. If the port is in the
            tarpit mode it will make sure that a connection attempt is allowed.

   Arguments:  const char *bfr      - Buffer to encrypt
               uint16_t bufSz       - Size of the buffer

   Returns: None

   Side Effects: None

   Reentrant Code: None

   Notes:

 ******************************************************************************************************************** */
uint32_t MFGP_AllowDtlsConnect( void )
{
   uint32_t timeDiff;              /* Difference in present time minus timestamp */

   ( void )TIME_UTIL_GetTimeInSecondsFormat( &_DateTime );

   /***************************************************************************
      After 4 free attempts the MFGP will go into a tarpit starting at 15 minutes,
      If the alloted time has not expired and an attempt is made the user will
      get back the "LOCKOUT IN EFFECT" message.
   **************************************************************************/
   if ( _AuthenticationAttempts > MFGP_MAX_FREE_ATTEMPTS )
   {
      timeDiff = _DateTime.seconds - _AuthenticationStart;
      if ( timeDiff < _TimeForAuthentication )
      {
         return false;
      }
   }
   return true;
}
/***********************************************************************************************************************

   Function name: MFGP_DtlsInit

   Purpose: This function is called to initialize the DTLS serial data.

   Arguments: UART number

   Returns: None

   Side Effects: None

   Reentrant Code: None

   Notes:

 ******************************************************************************************************************** */
/*lint -esym(715,uartId) not used in all projects. */
void MFGP_DtlsInit( enum_UART_ID uartId )
{
#if !USE_USB_MFG


   _AuthenticationAttempts = 0;
   _TimeForAuthentication = MFGP_15_MINUTES_IN_SECONDS;
   _FailedAuthenticationCounter = 0;
   /* On Samwise, hook into the UART error handler to optimize the RX function.  */
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
   if ( uartId == UART_MANUF_TEST ) /* First time, or logging off optical port */
#endif
   {
      dtlsUart = UART_MANUF_TEST;
#if ( RTOS_SELECTION == MQX_RTOS )
      uint32_t vector;
      if ( mqxUART_isr == NULL )
      {
         vector = ( uint32_t )INT_UART0_RX_TX;
         dtlsUART_isr_data = _int_get_isr_data( vector );
         mqxUART_isr = _int_get_isr( vector );
         ( void )_int_install_isr( vector, dtlsUART_isr, dtlsUART_isr_data );
      }
#elif ( RTOS_SELECTION == FREE_RTOS )
   // TODO: RA6E1 dtls uart isr
#endif
   }
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
   else  /* Switching to Optical port  */
   {
      dtlsUart = UART_OPTICAL_PORT;
   }
#endif
#endif
}
/*lint +esym(715,uartId) not used in all projects. */
/*******************************************************************************

   Function name: dtlsUART_isr

   Purpose: Optimize receiving of characters to prevent overrun when receiving long dtls packets.

   Arguments: MQX data pointer

   Returns: none

   Notes:

*******************************************************************************/
#if !USE_USB_MFG
static void dtlsUART_isr( void *user_isr_ptr )
{
#if ( RTOS_SELECTION == MQX_RTOS )
   IO_SERIAL_INT_DEVICE_STRUCT_PTR  int_io_dev_ptr = user_isr_ptr;
   KUART_INFO_STRUCT_PTR            sci_info_ptr = int_io_dev_ptr->DEV_INFO_PTR;
   UART_MemMapPtr                   sci_ptr = sci_info_ptr->SCI_PTR;
   int32_t                          c;    /* Data received  */
   uint16_t                         stat = sci_ptr->S1;

   /* Loop on RX flag, receiving characters, as needed. UART Transmission will be held off while this process takes
      place, which is OK.  */
   while ( ( stat & UART_S1_RDRF_MASK ) != 0 )
   {
      c = sci_ptr->D;
      if ( !_io_serial_int_addc( int_io_dev_ptr, ( uint8_t )c ) )
      {
         sci_info_ptr->RX_DROPPED_INPUT++;
      }
      sci_info_ptr->RX_CHARS++;
      stat = sci_ptr->S1;
   }
   mqxUART_isr( user_isr_ptr );        /* Chain to the previous handler */
#elif ( RTOS_SELECTION == FREE_RTOS )
   // TODO: RA6E1 dtls uart isr
#endif
}
#endif
#endif
