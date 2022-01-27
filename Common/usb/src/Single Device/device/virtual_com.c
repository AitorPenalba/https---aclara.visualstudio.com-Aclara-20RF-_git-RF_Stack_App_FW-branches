/* HEADER**********************************************************************
   Copyright 2008 Freescale Semiconductor, Inc.
   Copyright 1989-2008 ARC International

   This software is owned or controlled by Freescale Semiconductor.
   Use of this software is governed by the Freescale MQX RTOS License
   distributed with this Material.
   See the MQX_RTOS_LICENSE file distributed for more details.

   Brief License Summary:
   This software is provided in source form for you to use free of charge,
   but it is not open source software. You are allowed to use this software
   but you cannot redistribute it or derivative works of it in source form.
   The software may be used only in connection with a product containing
   a Freescale microprocessor, microcontroller, or digital signal processor.
   See license agreement file for full license terms including other
   restrictions.
*****************************************************************************

   Comments:

   @brief  The file emulates a USB PORT as RS232 PORT.

   END************************************************************************/

/******************************************************************************
   Includes
 *****************************************************************************/
#include "project.h"
#include <mqx.h>
#include "CompileSwitch.h"
#include "virtual_com.h"
#include "OS_aclara.h"
#include "charq.h"
#include "DBG_SerialDebug.h"
#include "BSP_aclara.h"

extern CDC_DEVICE_STRUCT_PTR   cdc_device_array[MAX_CDC_DEVICE];
/*****************************************************************************
   Constant and Macro's
 *****************************************************************************/

/*****************************************************************************
   Global Functions Prototypes
 *****************************************************************************/
void USB_App_Init( void );

/****************************************************************************
   Global Variables
 ****************************************************************************/
extern USB_ENDPOINTS                   usb_desc_ep;
extern DESC_CALLBACK_FUNCTIONS_STRUCT  desc_callback;

/*****************************************************************************
   Local Types - None
 *****************************************************************************/

/*****************************************************************************
   Local Functions Prototypes
 *****************************************************************************/
static void USB_App_Callback( uint8_t event_type, void *val, void *arg );
static void USB_Notif_Callback( uint8_t event_type, void *val, void *arg );

/*****************************************************************************
   Local Variables
 *****************************************************************************/
static CDC_HANDLE    g_app_handle;
static uint8_t       *g_curr_recv_buf;
static void          *recvTaskQueue = NULL;   /* Used to suspend task requesting USB input */
static void          *sendTaskQueue = NULL;   /* Used to suspend task requesting USB output */
static uint32_t      g_recv_size;
static _mqx_uint     ioctlFlags = IO_SERIAL_TRANSLATION | IO_SERIAL_ECHO;
static uint8_t       putsBuf[ 2 ];
static bool          start_app = (bool)false;
static bool          start_transactions = (bool)false;
static OS_EVNT_Obj   *MFG_notify;             /* Event handler to "notify" when USB reconnected.     */

/*****************************************************************************
   Local Functions
 *****************************************************************************/

static void initRecvTaskQueue( void )
{
   recvTaskQueue = _taskq_create( MQX_TASK_QUEUE_FIFO );
}
static void initSendTaskQueue( void )
{
   sendTaskQueue = _taskq_create( MQX_TASK_QUEUE_FIFO );
}

static void USB_resumeRecvQueue( void )
{
   (void)_taskq_resume( recvTaskQueue, (bool)true ); /* Wake any task waiting for input. */
}
static void USB_suspendRecvQueue( void )
{
   (void)_taskq_suspend( recvTaskQueue );
}

void USB_resumeSendQueue( bool flag ) /* Wake task(s) waiting for output. */
{
   (void)_taskq_resume( sendTaskQueue, flag );
}
static void USB_suspendSendQueue( void )
{
   (void)_taskq_suspend( sendTaskQueue );
}

/***********************************************************************************************************************
   Function Name: USB_SetEventNotify

   Purpose: Allows MFG process to set the event handler that should be notified when USB connection restored.

   Arguments:  handle - pointer to OS_EVENT_Obj

   Returns: none
***********************************************************************************************************************/
void USB_SetEventNotify( OS_EVNT_Obj *handle )
{
   MFG_notify = handle;
}


/*****************************************************************************

    @name        USB_App_Init

    @brief       This function is the entry for mouse (or other usuage)

    @param       None

    @return      None

*****************************************************************************/
void USB_App_Init( void )
{
   CDC_CONFIG_STRUCT cdc_config;
   USB_CLASS_CDC_ENDPOINT  *endPoint_ptr;

   g_curr_recv_buf = _mem_alloc_uncached( DATA_BUFF_SIZE );

   endPoint_ptr = USB_mem_alloc_zero( sizeof( USB_CLASS_CDC_ENDPOINT )  *CDC_DESC_ENDPOINT_COUNT );
   cdc_config.comm_feature_data_size = COMM_FEATURE_DATA_SIZE;
   cdc_config.usb_ep_data = &usb_desc_ep;
   cdc_config.desc_endpoint_cnt = CDC_DESC_ENDPOINT_COUNT;
   cdc_config.cdc_class_cb.callback = USB_App_Callback;
   cdc_config.cdc_class_cb.arg = &g_app_handle;
   cdc_config.vendor_req_callback.callback = NULL;
   cdc_config.vendor_req_callback.arg = NULL;
   cdc_config.param_callback.callback = USB_Notif_Callback;
   cdc_config.param_callback.arg = &g_app_handle;
   cdc_config.desc_callback_ptr =  &desc_callback;
   cdc_config.ep = endPoint_ptr;
   cdc_config.cic_send_endpoint = CIC_NOTIF_ENDPOINT;
   cdc_config.dic_recv_endpoint = DIC_BULK_OUT_ENDPOINT;
   cdc_config.max_supported_interfaces = USB_MAX_SUPPORTED_INTERFACES;  /* This was missing from original Freescale code!  */

   if ( MQX_OK != _usb_device_driver_install( USBCFG_DEFAULT_DEVICE_CONTROLLER ) )
   {
      DBG_printf( "Driver could not be installed\n" );
      return;
   }

   /* Initialize the USB interface */
   g_app_handle = USB_Class_CDC_Init( &cdc_config );
   g_recv_size = 0;
#if ( USE_USB_MFG == 0 )
   FILE_PTR fh_ptr;

   if ( NULL != ( fh_ptr = fopen( MFG_PORT_IO_CHANNEL, NULL ) ) )
   {
      ( void )_io_set_handle( IO_STDOUT, fh_ptr );
   }
#endif

   /* Set up task send/receive queues  */
   if( recvTaskQueue == NULL )
   {
      initRecvTaskQueue();
   }
   if ( sendTaskQueue == NULL )
   {
      initSendTaskQueue();
   }
}

/******************************************************************************

      @name        USB_App_Callback

      @brief       This function handles the callback

      @param       event_type : value of the event
      @param       val : NULL
      @param       arg : handle to Identify the controller

      @return      None
      Note: Called from ISR

 *****************************************************************************/
/*lint -esym(818,val,arg)   argument could be pointer to const */
static void USB_App_Callback( uint8_t event_type, void *val, void *arg )
{
   UNUSED_ARGUMENT ( arg )
   UNUSED_ARGUMENT ( val )
   if( event_type == USB_APP_BUS_RESET )
   {
      if( start_transactions )   /* If disconnected after successful startup, release task waiting for output to complete. */
      {
         USB_resumeSendQueue( (bool)true );  /* Wake all tasks waiting for output. */
      }
      start_app = (bool)false;
   }
   else if( event_type == USB_APP_ENUM_COMPLETE )
   {
#if 0
      DBG_LW_printf( "\n%s, usb_addr: %hhu\n", __func__, USB0_ADDR );
#endif
      start_app = (bool)true;
   }
   else if( event_type == USB_APP_ERROR )
   {
      /* add user code for error handling */
      DBG_LW_printf( "\n%s, app_error: %hhu\n", __func__, USB0_ADDR );
      USB_resumeSendQueue( (bool)true );  /* Wake all tasks waiting for output. */
      USB_resumeRecvQueue();              /* Wake all tasks waiting for input. */
   }

   return;
}

/*
   USB_isReady - returns state of start_transactions
*/
bool USB_isReady( void )
{
   return start_transactions;
}
/******************************************************************************

      @name        USB_Notif_Callback

      @brief       This function handles the callback

      @param       event_type : value of the event
      @param       val : various
      @param       arg:  handle to Identify the controller

      @return      None
      Note: Called from ISR

 *****************************************************************************/

/*lint -esym(715,arg)   argument not referenced */
static void USB_Notif_Callback( uint8_t event_type, void *val, void *arg )
{
   if( start_app == (bool)true )
   {
      if ( ( event_type == USB_APP_CDC_DTE_ACTIVATED ) || ( event_type == USB_APP_CDC_CARRIER_ACTIVATED ) )
      {
         start_transactions = (bool)true;
         USB_resumeSendQueue( (bool)false ); /* Wake 1st task waiting for output. */
         USB_resumeRecvQueue();              /* Wake any task waiting for input. */
         if ( MFG_notify != NULL )
         {
            OS_EVNT_Set ( MFG_notify, 1U );
         }
#if 0
         DBG_LW_printf( "\n%s, usb_addr: %hhu\n", __func__, USB0_ADDR );
#endif
      }
      else if( event_type == USB_APP_CDC_DTE_DEACTIVATED )
      {
         start_transactions = (bool)false;
      }
      else if( ( event_type == USB_APP_DATA_RECEIVED ) && ( start_transactions == (bool)true ) )
      {
         APP_DATA_STRUCT *dp_rcv = (APP_DATA_STRUCT *)val;

         g_recv_size = dp_rcv->data_size;
         (void)memcpy( g_curr_recv_buf, dp_rcv->data_ptr, g_recv_size );

         USB_resumeRecvQueue(); /* Wake any task waiting for input. */

#if USB_DEBUG == 1
         if ( g_recv_size != 0 )
         {
            USB_printHex( "USB rec data: ", g_curr_recv_buf, g_recv_size );
         }
#endif

      }
      else if( event_type == USB_APP_SEND_COMPLETE )
      {
         /* User: add your own code for send complete event */
         USB_resumeSendQueue( (bool)false ); /* Wake 1st task waiting for output. */
      }
#if 1
      else
      {
         DBG_LW_printf( "%s Received event_type 0x%hhx\n", __func__, event_type );
      }
#endif
   }
   return;
}
/*
   function usb_putc

   Arguments:  char c - character to send to USB bus.

   Returns: nothing
*/
void usb_putc( char c )
{
   uint32_t len = 1;

   putsBuf[ 0 ] = ( uint8_t )c;
   if ( ( ioctlFlags & IO_SERIAL_TRANSLATION ) != 0 )
   {
      if ( c == '\n' )
      {
         putsBuf[ 1 ] = '\r';
         len++;
      }
#if ( USB_DEBUG == 1 )
      INFO_printHex( "", putsBuf, 1U );
#endif
   }

   usb_send( (char const *)putsBuf, len );
}


/*
   usb_puts - send character string to USB0 port.

   Note: sends '\r after trailing '\n'
*/
void usb_puts( const char *str )
{
   usb_send( str, strlen( str ) );

   if ( ( ioctlFlags & IO_SERIAL_TRANSLATION ) != 0 )
   {
      if ( str[ strlen( str ) - 1 ] == '\n' )
      {
         usb_putc( '\r' );
      }
   }
}
/*
   usb_send - send bufferto USB0 port.

   Task of calling routine suspended until output completed.

   Note - may be called from ISR! Take care regarding debug print, other OS calls.
*/
void usb_send( char const *data, uint32_t len )
{
   uint8_t  status;
#if 1
   if ( !start_transactions )    /* If called while USB port not ready, suspend until it is ready  */
   {
      USB_suspendSendQueue();
   }
#endif

#if 0
   _int_disable();
   status = _usb_device_get_transfer_status( cdc_device_array[0]->controller_handle, DIC_BULK_IN_ENDPOINT, USB_SEND );
   _int_enable();
   if ( status != USB_STATUS_IDLE )
   {
      USB_suspendSendQueue();
//    DBG_LW_printf( "%s, status: %hhu\n", __func__, status );
   }

   status = USB_Class_CDC_Send_Data( g_app_handle, DIC_BULK_IN_ENDPOINT, (uint8_t *)data, len );
#else
   status = USB_Class_CDC_Send_Data( g_app_handle, DIC_BULK_IN_ENDPOINT, (uint8_t *)data, len );
   if ( status != USB_STATUS_IDLE )
   {
      DBG_LW_printf( "%s, status: %hhu\n", __func__, status );
   }
#endif
   USB_suspendSendQueue();
}
/*
   usb_getc - get a byte from the USB0 port

   Returns one character received from USB port.
Note: Obeys ioctlFlags for echo, \r \b handling, etc.
*/

char usb_getc( void )
{
   static   uint8_t  idx = 0;
            uint32_t len = 0;
            uint8_t  rxByte = 0;
            uint8_t  busy;

   do /* Loop until at least one byte received. */
   {
      if ( g_recv_size != 0 )
      {
         len = g_recv_size;
         rxByte = g_curr_recv_buf[ idx++ ];
         if ( ( ioctlFlags & IO_SERIAL_TRANSLATION ) != 0 ) /* Special character handling enabled? */
         {
            if ( rxByte == '\r' )
            {
               if ( ( ioctlFlags & IO_SERIAL_ECHO ) != 0 ) /* Port set to echo input? */
               {
                  usb_putc ( rxByte );  /* Send CRLF   */
               }
               rxByte = '\n';
            }
            else if ( ( rxByte == '\b' ) && ( ( ioctlFlags & IO_SERIAL_ECHO ) != 0 ) )
            {
               usb_putc( '\b' ); /* Send backspace */
               usb_putc( ' ' );  /* Send space, original character ('\b') sent below */
            }
         }
         if ( ( ioctlFlags & IO_SERIAL_ECHO ) != 0 )
         {
            usb_putc ( rxByte ); /* Echo back   */
         }

         g_recv_size--;
         if ( g_recv_size == 0 ) /* All the input consumed? */
         {
            idx = 0;
            do
            {
               /* Schedule buffer for next receive event, only when existing input consumed. */
               busy = USB_Class_CDC_Recv_Data( g_app_handle, DIC_BULK_OUT_ENDPOINT, g_curr_recv_buf, DIC_BULK_OUT_ENDP_PACKET_SIZE );
               if ( busy != 0 )
               {
                  _sched_yield();
                  DBG_logPrintf( 'E', "USB_Class_CDC_Recv_Data returned: %hhu", busy );
               }
            } while ( busy  != 0 );
         }
      }
      else
      {
         USB_suspendRecvQueue(); /* Suspend task until input available  */
      }
   } while ( len == 0 );

   return rxByte;
}
void usb_flush( void )
{
#if 0
   if ( ( g_recv_size != 0 ) && USB_isReady() )
   {
      g_recv_size = 0;
      USB_Class_CDC_Recv_Data( g_app_handle, DIC_BULK_OUT_ENDPOINT, g_curr_recv_buf, DIC_BULK_OUT_ENDP_PACKET_SIZE );
   }
#endif
}

/*
   usb_ioctl - perform ioctl operations on USB0 peripheral
   Note: file_ptr is not used in this case
*/
/*lint -esym(715,file_ptr)   argument not referenced */
void usb_ioctl( const MQX_FILE_PTR file_ptr, _mqx_uint cmd, const void *param_ptr )
{
   if ( cmd == IO_IOCTL_SERIAL_SET_FLAGS )
   {
      ioctlFlags = *(_mqx_uint *)param_ptr;
   }
}

#if 1
/***********************************************************************************************************************

   Function name: USB_printHex

   Purpose: Prints an array as hex ascii to debug port with the log information.

   Arguments: char *str
              const uint8_t *pSrc
              uint16_t num_bytes

   Returns: None

   Notes:

 **********************************************************************************************************************/
static char USB_printBuf[ 100 ];
void USB_printHex ( char const *str, const uint8_t *pSrc, uint16_t num_bytes )
{
   uint32_t len;
   int32_t  idx = 0;

   len = strlen( str ) + ( num_bytes * 2 ) + 2; /* Add room for \n'0' */
   while ( num_bytes )
   {
      if ( strlen( str ) ) /* Add str to segment   */
      {
         idx += snprintf( USB_printBuf, (int32_t)sizeof( USB_printBuf ), "%s ", str );
      }

      while( num_bytes != 0 )
      {
         idx += snprintf( &USB_printBuf[ idx ], (int32_t)sizeof( USB_printBuf ) - idx, "%02X", *pSrc++ );
         num_bytes--;
      }
   }
   (void)snprintf( &USB_printBuf[ idx ], (int32_t)sizeof( USB_printBuf ) - idx, "\n" );

#if !USE_USB_MFG
   (void)UART_write( UART_MANUF_TEST, (uint8_t *)USB_printBuf, len );
#else
   usb_send( (char const *)USB_printBuf, len );
#endif
}
#endif
/* EOF */
