/*HEADER**********************************************************************
*
* Copyright 2008 Freescale Semiconductor, Inc.
* Copyright 1989-2008 ARC International
*
* This software is owned or controlled by Freescale Semiconductor.
* Use of this software is governed by the Freescale MQX RTOS License
* distributed with this Material.
* See the MQX_RTOS_LICENSE file distributed for more details.
*
* Brief License Summary:
* This software is provided in source form for you to use free of charge,
* but it is not open source software. You are allowed to use this software
* but you cannot redistribute it or derivative works of it in source form.
* The software may be used only in connection with a product containing
* a Freescale microprocessor, microcontroller, or digital signal processor.
* See license agreement file for full license terms including other
* restrictions.
*****************************************************************************
*
* Comments:
*
* @brief The file contains Macro's and functions needed by the virtual com
*        application
*
*END************************************************************************/

#ifndef _VIRTUAL_COM_H
#define _VIRTUAL_COM_H  1

#include "usb_descriptor.h"

/******************************************************************************
 * Constants - None
 *****************************************************************************/

/******************************************************************************
 * Macro's
 *****************************************************************************/
#define  DATA_BUFF_SIZE    (DIC_BULK_OUT_ENDP_PACKET_SIZE)

/*****************************************************************************
 * Global variables
 *****************************************************************************/

/*****************************************************************************
 * Global Functions
 *****************************************************************************/
extern void USB_App_Init(void);
extern bool USB_isReady(void);
extern void usb_putc( char c );
extern void usb_puts( const char *str );
extern char usb_getc( void );
extern void usb_send( char const *data, uint32_t len );
extern void USB_printHex ( char const *str, const uint8_t *pSrc, uint16_t num_bytes );
extern void usb_flush( void );
extern void usb_ioctl( const MQX_FILE_PTR file_ptr, _mqx_uint cmd, const void *param_ptr);
extern void USB_resumeSendQueue( bool flag );   /* Wake task(s) waiting for USB output completion. */
extern void USB_SetEventNotify( OS_EVNT_Obj *handle );

#endif


/* EOF */
