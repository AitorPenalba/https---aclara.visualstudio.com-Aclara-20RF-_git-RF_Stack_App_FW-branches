/******************************************************************************
 *
 * Filename: UART.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
   A product of Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include "meter.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <bsp.h>
#elif (RTOS_SELECTION == FREE_RTOS)
#include "BSP_aclara.h"
#include "hal_data.h"
#endif
#include "pwr_last_gasp.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include "psp_cpudef.h"
#endif

/* #DEFINE DEFINITIONS */
#if ( MCU_SELECTED == RA6E1 )  //To process the input byte in RA6E1
#define MAX_DBG_COMMAND_CHARS     1602

#define ESCAPE_CHAR           0x1B
#define LINE_FEED_CHAR        0x0A
#define CARRIAGE_RETURN_CHAR  0x0D
#define BACKSPACE_CHAR        0x08
#define TILDA_CHAR            0x7E
#define WHITE_SPACE_CHAR      0x20
#define CTRL_C_CHAR           0x03
#define DELETE_CHAR           0x7f
#endif
/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
typedef struct
{
   uint32_t UART_Baud;
   uint32_t UART_Options;
   char UART_String[8];
} struct_UART_Setup;

/* CONSTANTS */
#if ( RTOS_SELECTION == MQX_RTOS )
static const struct_UART_Setup UartSetup[MAX_UART_ID] =
{
   /* This data structure must match the listing in BSP_aclara.h enum_UART_ID */
   {  38400,   IO_SERIAL_ECHO|IO_SERIAL_TRANSLATION,  MFG_PORT_IO_CHANNEL     }, /* Manufacturing port */
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
   {   9600,   0,                                     OPTICAL_PORT_IO_CHANNEL }, /* Host Meter Optical port */
#endif
   { 115200,   IO_SERIAL_ECHO|IO_SERIAL_TRANSLATION,  DBG_PORT_IO_CHANNEL     }, /* Debug port */
#if (ACLARA_DA == 1)
   { 115200,   0,                                     HOST_PORT_IO_CHANNEL    }  /* Host connection */
#elif ( ACLARA_LC != 1 )                                                         /* meter specific code */
   {   9600,   IO_SERIAL_NON_BLOCKING,                HOST_PORT_IO_CHANNEL    }  /* UART_HOST_METER_COMM */
#else
   {   2400,   IO_SERIAL_NON_BLOCKING,                HOST_PORT_IO_CHANNEL    }  /* DRU Port */
#endif
};
#elif (RTOS_SELECTION == FREE_RTOS)
/* Baud rate and other configurations are done in RASC configurator */
const sci_uart_instance_ctrl_t *UartCtrl[MAX_UART_ID] =
{
   &g_uart3_ctrl, /* MFG Port */
#if 0 // TODO: RA6 [name_Balaji]: Add Optical Port support
   &g_uart9_ctrl, /* Optical Port */
#endif
   &g_uart4_ctrl, /* DBG Port */
   &g_uart2_ctrl  /* Meter Port */
};

const uart_cfg_t *UartCfg[MAX_UART_ID] =
{
   &g_uart3_cfg,
#if 0 // TODO: RA6 [name_Balaji]: Add Optical Port support
   &g_uart9_cfg,
#endif
   &g_uart4_cfg,
   &g_uart2_cfg
};
#if ( DEBUG_PORT_BAUD_RATE == 1 )
static const char *uart_name[MAX_UART_ID] = {"MFG", /* Order must match UartCtrl and UartCfg above */
#if 0 //TODO: RA6 Bob: Add Optical port support
                                             "Optical",
#endif
                                             "DBG", "Meter" };
#endif
#endif // (RTOS_SELECTION)
/* FILE VARIABLE DEFINITIONS */
#if ( RTOS_SELECTION == MQX_RTOS )
static MQX_FILE_PTR UartHandle[MAX_UART_ID];
#endif

#if ( MCU_SELECTED == RA6E1 )
static const char       CRLF[] = { '\r', '\n' };        /* For RA6E1, Used to Process Carriage return */
extern OS_SEM_Obj       dbgReceiveSem_;           /* For RA6E1, UART_read process is Transfered from polling to interrupt method */
static char             DbgPrintBuffer[MAX_DBG_COMMAND_CHARS + 1];
// TODO: RA6 [name_Balaji]: Make the transferSem work without extern
extern OS_SEM_Obj       transferSem[MAX_UART_ID];     /* For RA6E1, UART_write process is used in Semaphore method */
#endif
/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/*******************************************************************************

  Function name: UART_init

  Purpose: This function will initialize the variables and data structures necessary
           for the UART's

  Arguments:

  Returns: returnStatus_t - eSUCCESS if everything was successful, eFAILURE if something failed

  Notes:

*******************************************************************************/
returnStatus_t UART_init ( void )
{
   returnStatus_t retVal = eSUCCESS; /* Start with pass status, and latch on any failure */
   uint8_t i;
#if ( RTOS_SELECTION == MQX_RTOS )
   uint32_t Flags = 0;

   for ( i=0; i<(uint8_t)MAX_UART_ID; i++ )
   {
      if( ( PWRLG_LastGasp() == 0 ) || ( UART_DEBUG_PORT == (enum_UART_ID)i ) )  // Only open DEBUG port in last gasp mode */
      {
         UartHandle[i] = fopen ( &(UartSetup[i].UART_String[0]), NULL );
         if ( UartHandle[i] != NULL )
         {
            /* Set the Baud Rate for this particular UART */
            if ( MQX_OK != ioctl(UartHandle[i], IO_IOCTL_SERIAL_SET_BAUD, (void *)&(UartSetup[i].UART_Baud)) ) /*lint !e835 !e845 !e64 */
            {
               (void)printf( "ioctl (set baud) of uart %d failed\n", i );
               retVal = eFAILURE;
            }
            /* Read the current set of option flags for this UART */
            if ( MQX_OK != ioctl(UartHandle[i], IO_IOCTL_SERIAL_GET_FLAGS, (void *)&Flags) ) /*lint !e835 !e845 !e64 */
            {
               (void)printf( "ioctl (get flags) of uart %d failed\n", i );
               retVal = eFAILURE;
            }
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
            if ( i == (uint8_t)UART_OPTICAL_PORT ) /* Special handling of the optical port - transmitter must be disabled. */
            {
               if ( MQX_OK != ioctl(UartHandle[i], IO_IOCTL_SERIAL_DISABLE_TX, (void *)&Flags) ) /*lint !e835 !e845 !e64 */
               {
                  (void)printf( "ioctl (set flags) of uart %d failed\n", i );
                  retVal = eFAILURE;
               }
            }
            else
#endif
            {
               /* Set options as in the table.  */
               Flags = UartSetup[i].UART_Options;
               if ( MQX_OK != ioctl(UartHandle[i], IO_IOCTL_SERIAL_SET_FLAGS, (void *)&Flags) ) /*lint !e835 !e845 !e64 */
               {
                  (void)printf( "ioctl (set flags) of uart %d failed\n", i );
                  retVal = eFAILURE;
               }
            }
         }
         else
         {
            (void)printf( "fopen of uart %d failed\n", i );
            retVal = eFAILURE;
         }
      }
   }
#elif (RTOS_SELECTION == FREE_RTOS)
   for( i = 0; i < (uint8_t)MAX_UART_ID; i++ )
   {
      if( ( PWRLG_LastGasp() == 0 ) || ( UART_DEBUG_PORT == (enum_UART_ID)i ) )  // Only open DEBUG port in last gasp mode */
      {
         ( void )R_SCI_UART_Open( (void *)UartCtrl[ i ], (void *)UartCfg[ i ] );
      }
   }
#endif

   return ( retVal );
}

#if ( DEBUG_PORT_BAUD_RATE == 1 )
/*******************************************************************************

  Function name: UART_getName

  Purpose: Gets the ASCII string name of the corresponding UART port

  Arguments: UartId - Identifier of the particular UART on which to change baud rate

  Returns: pointer to string (null terminated) with name of this UART port

  Notes:

*******************************************************************************/
char * UART_getName ( enum_UART_ID uartId )
{
   return ( (char *)uart_name[ (uint32_t) uartId ] );
}

/*******************************************************************************

  Function name: UART_setBaudRate

  Purpose: Sets the baud rate for a specified UART port

  Arguments: UartId - Identifier of the particular UART on which to change baud rate
             baudSetting - baud_setting_t structure for the desired baud rate

  Returns: returnStatus_t err

  Notes:

*******************************************************************************/
returnStatus_t UART_setBaud ( enum_UART_ID uartId, baud_setting_t *baudSetting )
{
   fsp_err_t err = R_SCI_UART_BaudSet( (void *)UartCtrl[ (uint32_t)uartId ], baudSetting );
   return ( (returnStatus_t)err );
}
#endif

/* No function calls for UART_reset */
#if ( MCU_SELECTED == NXP_K24 )
/*******************************************************************************

  Function name: UART_reset

  Purpose: Close the selected port. Re-open and set up as specified in UartSetup

  Arguments: UartId - Identifier of the particular UART to send data on

  Returns: none

  Notes:

*******************************************************************************/
returnStatus_t UART_reset ( enum_UART_ID i )
{
   uint32_t Flags = 0;
   returnStatus_t retVal = eSUCCESS; /* Start with pass status, and latch on any failure */

   (void)fclose( UartHandle[i] );
   UartHandle[i] = (MQX_FILE_PTR)NULL;
   UartHandle[i] = fopen( &(UartSetup[i].UART_String[0]), NULL );
   if ( UartHandle[i] != NULL )
   {
      /* Set the Baud Rate for this particular UART */
      if ( MQX_OK != ioctl(UartHandle[i], IO_IOCTL_SERIAL_SET_BAUD,
                              (void *)&(UartSetup[i].UART_Baud)) ) /*lint !e835 !e845 !e64 */
      {
         retVal = eFAILURE;
      }
      /* Read the current set of option flags for this UART */
      if ( MQX_OK != ioctl(UartHandle[i], IO_IOCTL_SERIAL_GET_FLAGS, (void *)&Flags) ) /*lint !e835 !e845 !e64 */
      {
         retVal = eFAILURE;
      }
      /* Add any additional options to the option flags, and set the options back into the UART */
      Flags |= UartSetup[i].UART_Options;
      if ( MQX_OK != ioctl(UartHandle[i], IO_IOCTL_SERIAL_SET_FLAGS, (void *)&Flags) ) /*lint !e835 !e845 !e64 */
      {
         retVal = eFAILURE;
      }
   }
   else
   {
      retVal = eFAILURE;
   }
   return retVal;
}
#endif
/*******************************************************************************

  Function name: UART_write

  Purpose: This function will Transmit the passed data buffer out on the appropriate
           UART connection

  Arguments: UartId - Identifier of the particular UART to send data on
             DataBuffer - pointer to the Data that is to be sent
             DataLength - number of bytes that are to be sent (size of data in DataBuffer)

  Returns: DataSent - number of bytes that were correctly sent out the UART

  Notes: if the UART_Option is set to IO_SERIAL_NON_BLOCKING, then this function
         may not block and may return immediately (not sure what the return value will be??
         if the UART_Option is 0, then this function will block until all the bytes are sent
         to the lower MQX UART driver

*******************************************************************************/
uint32_t UART_write ( enum_UART_ID UartId, const uint8_t *DataBuffer, uint32_t DataLength )
{
#if ( MCU_SELECTED == NXP_K24 )
   uint32_t DataSent;
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
   if ( UartId == UART_OPTICAL_PORT )
   {
      ( void )UART_ioctl( UartId , IO_IOCTL_SERIAL_ENABLE_TX, NULL );
   }
#endif
   DataSent = (uint32_t)write ( UartHandle[UartId], (void*)DataBuffer, (int32_t)DataLength ); /*lint !e64 */
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
   if ( UartId == UART_OPTICAL_PORT )
   {
      ( void )UART_flush( UartId );
      ( void )UART_ioctl( UartId, IO_IOCTL_SERIAL_WAIT_FOR_TC, NULL );
      ( void )UART_ioctl( UartId, IO_IOCTL_SERIAL_DISABLE_TX, NULL );
   }
#endif
   return ( DataSent );
#elif ( MCU_SELECTED == RA6E1 )
#if 1 // TODO: RA6E1 Bob: attempt to get CRLF instead of just LF
   if ( ( DataLength > 0 ) && ( DataBuffer[DataLength-1] == '\n' ) )
   {
      if ( DataLength > 1 )
      {
         ( void )R_SCI_UART_Write( (void *)UartCtrl[ UartId ], DataBuffer, DataLength-1 );
         ( void )OS_SEM_Pend( &transferSem[ UartId ], OS_WAIT_FOREVER );
      }
      ( void )R_SCI_UART_Write( (void *)UartCtrl[ UartId ], (uint8_t *)&CRLF, sizeof(CRLF) );
      ( void )OS_SEM_Pend( &transferSem[ UartId ], OS_WAIT_FOREVER );
   }
   else
#endif
   {
      ( void )R_SCI_UART_Write( (void *)UartCtrl[ UartId ], DataBuffer, DataLength );
      ( void )OS_SEM_Pend( &transferSem[ UartId ], OS_WAIT_FOREVER );
   }

   return DataLength;/* R_SCI_UART_Write does not return the no. of valid read bytes, returning DataLength */
#endif
}

/*******************************************************************************

  Function name: UART_read

  Purpose: This function will return the number of bytes requested only when that
           number of bytes have been received into the UART - this function will
           block until all bytes have been received (UNLESS configured with IO_SERIAL_NON_BLOCKING)

  Arguments: UartId - Identifier of the particular UART to receive data in
             DataBuffer - pointer to the Data that is received (populated by this function)
             DataLength - number of bytes that are to be received before this function returns

  Returns: DataReceived - Number of valid bytes that are being returned in the buffer

  Notes: This function will block processing until all the requested bytes have
         been received into the UART

         If it is preferred to have a non-blocking function - then we can use the
         MQX io flag IO_SERIAL_NON_BLOCKING - set this during the fopen() call or
         in an ioctl call IO_IOCTL_SERIAL_SET_FLAGS

*******************************************************************************/
uint32_t UART_read ( enum_UART_ID UartId, uint8_t *DataBuffer, uint32_t DataLength )
{
#if ( MCU_SELECTED == NXP_K24 )
   uint32_t DataReceived;

   DataReceived = (uint32_t)read ( UartHandle[UartId], DataBuffer, (int32_t)DataLength ); /*lint !e64 */

   return ( DataReceived );
#elif ( MCU_SELECTED == RA6E1 )
   ( void )R_SCI_UART_Read( (void *)UartCtrl[ UartId ], DataBuffer, DataLength );
   return DataLength;/* R_SCI_UART_Read does not return the no. of valid read bytes, returning DataLength */
#endif
}

/*******************************************************************************

  Function name: UART_fgets

  Purpose: This function behaves like fgets

  Arguments: UartId - Identifier of the particular UART to receive data in
             DataBuffer - pointer to the Data that is received (populated by this function)
             DataLength - number of bytes that are to be received before this function returns

  Returns: none

  Notes: This function will block processing until all the requested bytes have
         been received into the UART

         If it is preferred to have a non-blocking function - then we can use the
         MQX io flag IO_SERIAL_NON_BLOCKING - set this during the fopen() call or
         in an ioctl call IO_IOCTL_SERIAL_SET_FLAGS

*******************************************************************************/
void UART_fgets( enum_UART_ID UartId, char *DataBuffer, uint32_t DataLength )
{
#if ( RTOS_SELECTION == MQX_RTOS )
   (void)fgets( DataBuffer, (int32_t)DataLength, UartHandle[UartId] );
#elif ( RTOS_SELECTION == FREE_RTOS )
   // TODO: RA6 [name_Balaji]: To process the copy paste of multiple commands
   uint8_t rxByte = 0;
   uint32_t DBGP_numBytes = 0;
   uint32_t DBGP_numPrintBytes = 0;
   bool lineComplete = false;
   /* Loops Until Newline is detected */
   for ( ;; )
   {
      ( void )UART_read ( UART_DEBUG_PORT, &rxByte, sizeof(rxByte) );
      ( void )OS_SEM_Pend( &dbgReceiveSem_, OS_WAIT_FOREVER );
      if ( ( rxByte == ESCAPE_CHAR ) ||  /* User pressed ESC key */
         ( rxByte == CTRL_C_CHAR ) ||    /* User pressed CRTL-C key */
         ( rxByte == 0xC0 ))             /* Left over SLIP protocol characters in buffer */
      {
         /* user canceled the in progress command */
         memset( DbgPrintBuffer, 0, DBGP_numPrintBytes );
         memset( DataBuffer, 0, DBGP_numBytes );
         DBGP_numBytes = 0;
         DBGP_numPrintBytes = 0;
         rxByte = 0x0;
      }/* end if () */
      else if( ( rxByte != (uint8_t)0x00 ) &&
             ( rxByte != LINE_FEED_CHAR ) &&
             ( rxByte != CARRIAGE_RETURN_CHAR ) &&
             ( rxByte != BACKSPACE_CHAR) &&
             (rxByte != DELETE_CHAR ) )
      {
         DataBuffer[DBGP_numBytes++] = rxByte;
         DbgPrintBuffer[DBGP_numPrintBytes++] = rxByte;
         rxByte = 0x0;
         for (DBGP_numBytes;DBGP_numBytes<DataLength;DBGP_numBytes++)
         {
            /* UART_read used to read characters while doing Copy/Paste */
            ( void )UART_read ( UART_DEBUG_PORT, &rxByte, sizeof(rxByte) );
            /* 10millisecond is the delay timing where a successful UART_read happens */
            ( void )OS_SEM_Pend( &dbgReceiveSem_, ( portTICK_RATE_MS * 2) );

            if ( ( rxByte == ESCAPE_CHAR ) ||  /* User pressed ESC key */
               ( rxByte == CTRL_C_CHAR ) ||  /* User pressed CRTL-C key */
               ( rxByte == 0xC0 ) )           /* Left over SLIP protocol characters in buffer */
            {
               /* user canceled the in progress command */
               memset( DbgPrintBuffer, 0, DBGP_numPrintBytes );
               memset( DataBuffer, 0, DBGP_numBytes );
               DBGP_numBytes = 0;
               DBGP_numPrintBytes = 0;
               rxByte = 0x0;
            }/* end if () */
            else if( ( rxByte != (uint8_t)0x00 ) &&
                   ( rxByte != LINE_FEED_CHAR ) &&
                   ( rxByte != CARRIAGE_RETURN_CHAR ) &&
                   ( rxByte != BACKSPACE_CHAR) &&
                   ( rxByte != DELETE_CHAR ) )
            {
               DbgPrintBuffer[DBGP_numPrintBytes++] = rxByte;
               DataBuffer[DBGP_numBytes] = rxByte;
               rxByte = 0x0;
            }/* end if () */
            else if( ( rxByte == LINE_FEED_CHAR ) ||
                   ( rxByte == CARRIAGE_RETURN_CHAR ) )
            {
               if ( DBGP_numBytes == 0 )
               {
                  ( void )UART_write( UART_DEBUG_PORT, (uint8_t*)CRLF, sizeof( CRLF ) );
               }
               else
               {
                  rxByte = 0x0;
                  ( void )UART_write( UART_DEBUG_PORT, (uint8_t*)DbgPrintBuffer, DBGP_numPrintBytes );
                  ( void )UART_write( UART_DEBUG_PORT, (uint8_t*)CRLF, sizeof( CRLF ) );
                  lineComplete = true;
                  break;
               }
            }/* end else if () */
            else if( rxByte == BACKSPACE_CHAR || rxByte == DELETE_CHAR )
            {
               if( DBGP_numBytes != 0 )
               {
                  /* buffer contains at least one character, remove the last one entered */
                  /* Resets the last entered character */
                  DataBuffer[ --DBGP_numBytes ] = 0x00;
                  rxByte = 0x0;
               }
            }/* end else if () */
            else
            {
               /* Used to ECHO the recieved character when no more valid data is read */
               UART_write( UART_DEBUG_PORT, (uint8_t*)DbgPrintBuffer, DBGP_numPrintBytes );
               memset( DbgPrintBuffer, 0, DBGP_numPrintBytes );
               DBGP_numPrintBytes = 0;
               break;
            }/* end else () */
         }/* end for () */
      }/* end else if () */
      else if ( ( rxByte == LINE_FEED_CHAR ) || ( rxByte == CARRIAGE_RETURN_CHAR ) )
      {
            if( DBGP_numBytes == 0 )
            {
               ( void )UART_write( UART_DEBUG_PORT, (uint8_t*)CRLF, sizeof( CRLF ) );
            }
            else
            {
               ( void )UART_write( UART_DEBUG_PORT, (uint8_t*)CRLF, sizeof( CRLF ) );
               DbgPrintBuffer[DBGP_numPrintBytes] = rxByte;
               rxByte = 0x0;
            }
            lineComplete = true;
      }
      else if( rxByte == BACKSPACE_CHAR || rxByte == DELETE_CHAR )
      {
         if( DBGP_numBytes != 0 )
         {
            /* buffer contains at least one character, remove the last one entered */
            /* Resets the last entered character */
            DataBuffer[ --DBGP_numBytes ] = 0x00;
            rxByte = 0x0;
            /* Gives GUI effect in Terminal for Backspace */
            ( void )UART_write( UART_DEBUG_PORT, (uint8_t*)"\b\x20\b", 3 );
         }
      }/* end else if () */
      else
      {
         DbgPrintBuffer[DBGP_numPrintBytes++] = rxByte;
         rxByte = 0x0;
         ( void )UART_write( UART_DEBUG_PORT, (uint8_t*)DbgPrintBuffer, DBGP_numPrintBytes );
         memset( DbgPrintBuffer, 0, DBGP_numPrintBytes );
         DBGP_numPrintBytes = 0;
      }
      if ( lineComplete == true )
      {
         break;
      }
   }
#endif
}
#if ( MCU_SELECTED == NXP_K24 )
/*******************************************************************************

  Function name: UART_flush

  Purpose: This function flushes the uart TX buffers

  Arguments: UartId - Identifier of the particular UART to receive data in

  Returns: ???

  Notes:

*******************************************************************************/
uint8_t UART_flush( enum_UART_ID UartId )
{
   return ( (uint8_t)fflush(UartHandle[UartId]) );
}

/*******************************************************************************

  Function name: UART_RX_flush

  Purpose: This function flushes the uart RX buffers. This is not native to the OS.

  Arguments: UartId - Identifier of the particular UART to receive data in

  Returns: none

  Notes:

*******************************************************************************/
void UART_RX_flush ( enum_UART_ID UartId )
{
   bool     charsAvailable;
   uint8_t  trash;
   /* user canceled the in progress command */
   do
   {
      ( void )UART_ioctl ( UartId, IO_IOCTL_CHAR_AVAIL, &charsAvailable );
      if ( charsAvailable )
      {
         (void)UART_read( UartId, &trash, sizeof(trash) );
      }
   }
   while ( charsAvailable );/* Loop while bytes in queue  */
}

/*******************************************************************************

  Function name: UART_gotChar

  Purpose: Checks for characters in input buffer.

  Arguments: UartId - Identifier of the particular UART to receive data in

  Returns: true if Empty

  Notes:

*******************************************************************************/
bool UART_gotChar ( enum_UART_ID UartId )
{
   bool bDone; /* Return value that is true if TX is completed, False if not completed */

   (void) ioctl( UartHandle[ UartId ], IO_IOCTL_CHAR_AVAIL, &bDone );
   return( bDone );
}

/*******************************************************************************

  Function name: UART_isTxEmpty

  Purpose: Waits for the TX to finish

  Arguments: UartId - Identifier of the particular UART to receive data in

  Returns: true if Empty

  Notes:

*******************************************************************************/
uint8_t UART_isTxEmpty ( enum_UART_ID UartId )
{
   uint8_t bDone; /* Return value; true if Rx char(s) available, False otherwise */

   (void) ioctl( UartHandle[ UartId ], IO_IOCTL_SERIAL_TRANSMIT_DONE, ( void *)&bDone ); /*lint !e835 !e845 */
   return( bDone );
}

/*******************************************************************************

  Function name: UART_ioctl

  Purpose: Set IOCTL fields on UargHandle entries

  Arguments: UartId - Identifier of the particular UART to receive data in

  Returns: result of ioctl operation

  Notes:

*******************************************************************************/
uint8_t UART_ioctl ( enum_UART_ID UartId, int32_t op, void *addr )
{
   return (uint8_t)( ioctl( UartHandle[ UartId ], (uint32_t)op, ( void *)addr ) );
}

/*******************************************************************************

  Function name: UART_SetEcho

  Purpose: Turn echo on/off

  Arguments: UartId - Identifier of the particular UART to change attribute
             val    - 0 for echo off, on otherwise

  Returns: result of the last ioctl operation

  Notes:

*******************************************************************************/
uint8_t UART_SetEcho( enum_UART_ID UartId, bool val )
{
   uint32_t flags;

   (void)UART_ioctl ( UartId, IO_IOCTL_SERIAL_GET_FLAGS, &flags ); // Get current settings
   if ( val ) {
      flags |=  IO_SERIAL_ECHO; // Enable ECHO
   } else {
      flags &= ~IO_SERIAL_ECHO; // Disable ECHO
   }
   return UART_ioctl ( UartId, IO_IOCTL_SERIAL_SET_FLAGS, &flags ); // Update settings
}

#endif // #if ( MCU_SELECTED == NXP_K24 )
/* No function calls for UART_close */
/*******************************************************************************

  Function name: UART_close

  Purpose: Closes specified UART

  Arguments: UartId - Identifier of the particular UART to close

  Returns: result of fclose operation

  Notes:

*******************************************************************************/
uint8_t UART_close ( enum_UART_ID UartId )
{
#if ( MCU_SELECTED == NXP_K24 )
   return (uint8_t)fclose( UartHandle[ UartId  ] );
#elif ( MCU_SELECTED == RA6E1 )
   (void)R_SCI_UART_Close((void *)UartCtrl[UartId]);
   return 1;
#endif
}


/*******************************************************************************

  Function name: UART_getHandle

  Purpose: get handle of specified UART

  Arguments: UartId - Identifier of the particular UART to close

  Returns: handle of specified UART

  Notes:

*******************************************************************************/
void * UART_getHandle ( enum_UART_ID UartId )
{
#if ( MCU_SELECTED == NXP_K24 )
   return ( (void *)UartHandle[ UartId  ] );
#elif ( MCU_SELECTED == RA6E1 )
   return ( (void *)UartCtrl[ UartId ] );
#endif
}

/*******************************************************************************

  Function name: UART_open

  Purpose: open specified UART

  Arguments: UartId - Identifier of the particular UART to open

  Returns: uint8_t - results of ioctl operation

  Notes:

*******************************************************************************/
uint8_t UART_open ( enum_UART_ID UartId )
{
#if ( MCU_SELECTED == NXP_K24 )
   uint32_t Flags;
   uint8_t  retVal = (uint8_t)eFAILURE;

   UartHandle[UartId] = fopen( &(UartSetup[UartId].UART_String[0]), NULL );
   if ( UartHandle[UartId] != NULL )
   {
      /* Set the Baud Rate for this particular UART */
      retVal = (uint8_t)ioctl(UartHandle[UartId], IO_IOCTL_SERIAL_SET_BAUD,
                                       (void *)&(UartSetup[UartId].UART_Baud)); /*lint !e835 !e845 !e64 */
      if ( MQX_OK != (_mqx_int) retVal )
      {
         (void)printf( "ioctl (set baud) of uart %d failed( %d )\n", UartId, retVal );
      }
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
      if ( UartId == UART_OPTICAL_PORT ) /* Special handling of the optical port - transmitter must be disabled. */
      {
         retVal = (uint8_t)ioctl(UartHandle[UartId], IO_IOCTL_SERIAL_DISABLE_TX, NULL); /*lint !e835 !e845 !e64 */
      }
      else
#endif
      {
         /* Set options as in the table.  */
         Flags = UartSetup[UartId].UART_Options;
         retVal = (uint8_t)ioctl(UartHandle[UartId], IO_IOCTL_SERIAL_SET_FLAGS,
                                    (void *)&Flags); /*lint !e835 !e845 !e64 */
      }
   }
   return retVal;
#elif ( MCU_SELECTED == RA6E1 )
   /*Uart Open for RA6E1 is done in Uart Init*/
   return ( uint8_t )eSUCCESS;
#endif
}

#if ( MCU_SELECTED == RA6E1 )
/* Configured in RASC for future use of Optical port and Meter port integration */
/*******************************************************************************

  Function name: user_uart_callback

  Purpose: Interrupt Handler for UART Module

  Returns: None

*******************************************************************************/
void user_uart_callback( uart_callback_args_t *p_args )
{

   /* Handle the UART event */
   switch (p_args->event)
   {
      /* Receive complete */
      case UART_EVENT_RX_COMPLETE:
      {
         break;
      }
      /* Transmit complete */
      case UART_EVENT_TX_COMPLETE:
      {
         // TODO: RA6 [name_Balaji]: Discuss requirement for feature - print is completed or not
         break;
      }
      default:
      {
      }
   }
}/* end user_uart_callback () */

#endif

#if ( MCU_SELECTED == RA6E1 )
/*******************************************************************************

  Function name: dbg_uart_callback

  Purpose: Interrupt Handler for Debug UART Module, postpones the semaphore wait
            once one byte of data is read in SCI Channel 4 (DBG port)

  Returns: None

*******************************************************************************/
void dbg_uart_callback( uart_callback_args_t *p_args )
{
   /* Handle the UART event */
   switch (p_args->event)
   {
      /* Receive complete */
      case UART_EVENT_RX_COMPLETE:
      {
         OS_SEM_Post_fromISR( &dbgReceiveSem_ );
         break;
      }
      /* Transmit complete */
      case UART_EVENT_TX_COMPLETE:
      {
         OS_SEM_Post_fromISR( &transferSem[ UART_DEBUG_PORT ] );
         break;
      }
      default:
      {
         break;
      }
   }/* end switch () */
}/* end user_uart_callback () */
#endif