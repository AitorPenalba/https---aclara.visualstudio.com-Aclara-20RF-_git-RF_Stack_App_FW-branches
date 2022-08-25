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
#include "stdarg.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <bsp.h>
#endif
#include "pwr_last_gasp.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include "psp_cpudef.h"
#endif
#if ( TM_UART_EVENT_COUNTERS == 1 )
#include "UART.h"
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
   &g_uart_MFG_ctrl, /* MFG Port */
#if 0 // TODO: RA6 [name_Balaji]: Add Optical Port support
   &g_uart_optical_ctrl, /* Optical Port */
#endif
   &g_uart_DBG_ctrl, /* DBG Port */
   &g_uart_HMC_ctrl  /* Meter Port */
};

const uart_cfg_t *UartCfg[MAX_UART_ID] =
{
   &g_uart_MFG_cfg,
#if 0 // TODO: RA6 [name_Balaji]: Add Optical Port support
   &g_uart_optical_cfg,
#endif
   &g_uart_DBG_cfg,
   &g_uart_HMC_cfg
};

#if (RTOS_SELECTION == FREE_RTOS)
typedef struct
{
   uint8_t * pBuffer; /* Pointer to the ring buffer for this UART */
   uint16_t  size;    /* Size of the ring buffer, does not need to be 2^k size */
   uint16_t  head;
   uint16_t  tail;
} UART_ringBuffer_t;

typedef struct
{ // TODO: check whether sem needed for echo separately
   OS_SEM_Obj receiveUART_sem;
   OS_SEM_Obj transmitUART_sem;
   OS_SEM_Obj echoUART_sem;
} UART_Sem;
#endif

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
/* The following are the input ring buffers used to handle UART input data */
static uint8_t           ringBufferMfgPort[1024];
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
static uint8_t           ringBufferOptPort[1024];
#endif
static uint8_t           ringBufferDbgPort[256];
static uint8_t           ringBufferHMCPort[128];
/* The following structure contains the ring buffer control structures for each UART */
static UART_ringBuffer_t uartRingBuf[MAX_UART_ID] =
{
   {
      .pBuffer = &ringBufferMfgPort[0], .size = sizeof(ringBufferMfgPort), .head = 0, .tail = 0
   },
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
   {
      .pBuffer = &ringBufferOptPort[0], .size = sizeof(ringBufferOptPort), .head = 0, .tail = 0
   },
#endif
   {
      .pBuffer = &ringBufferDbgPort[0], .size = sizeof(ringBufferDbgPort), .head = 0, .tail = 0
   },
   {
      .pBuffer = &ringBufferHMCPort[0], .size = sizeof(ringBufferHMCPort), .head = 0, .tail = 0
   }
};
static UART_Sem          UART_semHandle [MAX_UART_ID]    = { NULL };
static OS_MUTEX_Obj      UART_writeMutex[MAX_UART_ID]    = { NULL };
static bool              ringBufoverflow[MAX_UART_ID]    = { (bool)false };
static bool              uartOverflow   [MAX_UART_ID]    = { (bool)false };
static bool              transmitUARTEnable[MAX_UART_ID] = { (bool)false };
static const char        CRLF[] = { '\r', '\n' };        /* For RA6E1, Used to Process Carriage return */
static bool              dbgUartEchoEnable_ = (bool)true;
static bool              UART_initReady_    = (bool)false;

#endif // ( MCU_SELECTED == RA6E1 )

#if ( TM_UART_EVENT_COUNTERS == 1 )
/* This structure creates counters on a per-UART basis for various events that can occur in this module */
static Uart_Events_t uart_events[MAX_UART_ID] = { 0 };
#endif // TM_UART_EVENT_COUNTERS

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

#if ( MCU_SELECTED == RA6E1 )
/*******************************************************************************

  Function name: UART_CallbackHandler

  Purpose: Common Interrupt Callback Handler for all UART ports

  Returns: None

*******************************************************************************/
static void UART_CallbackHandler( enum_UART_ID UartId, bool echoRequired, uart_callback_args_t *p_args )
{
   if ( UART_initReady_ )
   {
      /* Handle the UART event */
      switch (p_args->event)
      {
         /* Receive complete */
         case UART_EVENT_RX_COMPLETE:
         {
            TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].eventRxComplete );
            break;
         }
         /* Transmit complete */
         case UART_EVENT_TX_COMPLETE:
         {
            TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].eventTxComplete );
            if ( ( echoRequired ) && ( UART_semHandle[ (uint32_t)UartId ].echoUART_sem != NULL ) )
            {
               OS_SEM_Post_fromISR( &UART_semHandle[ (uint32_t)UartId ].echoUART_sem );
            }
            if ( transmitUARTEnable[ (uint32_t)UartId ] )
            {
               OS_SEM_Post_fromISR( &UART_semHandle[ (uint32_t)UartId ].transmitUART_sem );
               transmitUARTEnable[ (uint32_t)UartId ] = false;
            }
            break;
         }
         /* Received single byte */
         case UART_EVENT_RX_CHAR:
         {
            TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].eventRxChar );
            if( ! ( ringBufoverflow[ (uint32_t)UartId ] ) )
            {
               uint16_t nextHead = ( uartRingBuf[ (uint32_t)UartId ].head + 1 ) % uartRingBuf[ (uint32_t)UartId ].size;
               if ( nextHead == uartRingBuf[ (uint32_t)UartId ].tail )
               {
                  TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].isrRingBufferOverflow );
                  ringBufoverflow[ (uint32_t)UartId ] = true;
               }
               uartRingBuf[ (uint32_t)UartId ].pBuffer[ uartRingBuf[ (uint32_t)UartId ].head ] = ( uint8_t )p_args->data;
               uartRingBuf[ (uint32_t)UartId ].head = nextHead;
               OS_SEM_Post_fromISR( &UART_semHandle[ (uint32_t)UartId ].receiveUART_sem );
            }
            else
            {
               TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].missingPacketsCozRingBuf );
               OS_SEM_Post_fromISR( &UART_semHandle[ (uint32_t)UartId ].receiveUART_sem );
            }
            break;
         }
         case UART_EVENT_ERR_PARITY:
         {
            TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].eventErrParity );
            break;
         }
         case UART_EVENT_ERR_FRAMING:
         {
            TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].eventErrFraming );
            break;
         }
         case UART_EVENT_ERR_OVERFLOW:
         {
            TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].eventErrOverflow );
            uartOverflow[ (uint32_t)UartId ] = (bool)true;
            OS_SEM_Post_fromISR( &UART_semHandle[ (uint32_t)UartId ].receiveUART_sem );
            break;
         }
         case UART_EVENT_BREAK_DETECT:
         {
            TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].eventBreakDetect );
            break;
         }
         case UART_EVENT_TX_DATA_EMPTY:
         {
            TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].eventDataEmpty );
            break;
         }
         default:
         {
            TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].eventUnknownType );
            break;
         }
      }/* end switch () */
   }/* end UART_initReady */
}/* end UART_CallbackHandler () */

/*******************************************************************************

  Function name: mfg_uart_callback

  Purpose: Interrupt Handler for MFG UART Module,Postpones the semaphore wait
            once one byte of data is read in SCI Channel 3 (MFG port)

  Returns: None

*******************************************************************************/
void mfg_uart_callback( uart_callback_args_t *p_args )
{
   UART_CallbackHandler( UART_MANUF_TEST, (bool)true, p_args );
}/* end mfg_uart_callback () */

#if 0 // TODO: Optical port for RA6E1
/* Configured in RA6 for future use of Optical port integration */
/*******************************************************************************

  Function name: optical_uart_callback

  Purpose: Interrupt Handler for UART Module

  Returns: None

*******************************************************************************/
void optical_uart_callback( uart_callback_args_t *p_args )
{
   UART_CallbackHandler( UART_OPTICAL_PORT, (bool)false, p_args );
}/* end optical_uart_callback () */
#endif

/*******************************************************************************

  Function name: hmc_uart_callback

  Purpose: Interrupt Handler for UART Module

  Returns: None

*******************************************************************************/
void hmc_uart_callback( uart_callback_args_t *p_args )
{
   UART_CallbackHandler( UART_HOST_COMM_PORT, (bool)false, p_args );
}/* end hmc_uart_callback () */

/*******************************************************************************

  Function name: dbg_uart_callback

  Purpose: Interrupt Handler for Debug UART Module, postpones the semaphore wait
            once one byte of data is read in SCI Channel 4 (DBG port)

  Returns: None

*******************************************************************************/
void dbg_uart_callback( uart_callback_args_t *p_args )
{
   UART_CallbackHandler( UART_DEBUG_PORT, (bool)true, p_args );
}/* end dbg_uart_callback () */
#endif

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

   OS_INT_disable();    // Enable critical section as we are creating ring buffers and initializing them

   for( i = 0; i < ( uint8_t ) MAX_UART_ID; i++ )
   {
      // Setting bool values to false at init
      ringBufoverflow[i] = false;
      uartOverflow[i] = false;
      transmitUARTEnable[i] = false;
      uartRingBuf[i].head = 0;
      uartRingBuf[i].tail = 0;

      uint16_t semReceiveCount = 0;
      if ( ( i == UART_MANUF_TEST ) || ( i == UART_DEBUG_PORT ) ) // TODO: check optical port needed counting semaphore
      {
         semReceiveCount = uartRingBuf[i].size;
      }

      if( 0 == ( ( OS_SEM_Create  ( &UART_semHandle[i].receiveUART_sem, semReceiveCount ) ) &&
                 ( OS_SEM_Create  ( &UART_semHandle[i].transmitUART_sem, 0 ) )              &&
                 ( OS_MUTEX_Create( &UART_writeMutex[i] ) ) ) )
      {
         retVal |= eFAILURE;
      }

      if ( i != UART_HOST_COMM_PORT )
      {
         if ( 0 == ( OS_SEM_Create( &UART_semHandle[i].echoUART_sem, semReceiveCount ) ) )
         {
            retVal |= eFAILURE;
         }
      }
   }

   OS_INT_enable();

   for( i = 0; i < (uint8_t)MAX_UART_ID; i++ )
   {
      if( ( PWRLG_LastGasp() == 0 ) || ( UART_DEBUG_PORT == (enum_UART_ID)i ) )  // Only open DEBUG port in last gasp mode */
      {
         ( void )R_SCI_UART_Open( (void *)UartCtrl[ i ], (void *)UartCfg[ i ] );
      }
   }
   UART_initReady_ = (bool)true;
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
#endif // ( DEBUG_PORT_BAUD_RATE == 1 )

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
      ( void )UART_flush( UartId );   // TODO: RA6E1 This UART_flush not needed now for the debug and mfg port. Might be added in the future.
      ( void )UART_ioctl( UartId, IO_IOCTL_SERIAL_WAIT_FOR_TC, NULL );
      ( void )UART_ioctl( UartId, IO_IOCTL_SERIAL_DISABLE_TX, NULL );
   }
#endif
   return ( DataSent );
#elif ( MCU_SELECTED == RA6E1 )
   if ( UART_initReady_ )
   {
      /* RA6E1 Bob: convert \n to \r\n on manufacturing port so that setup commands for Radiated PSR can run */
      if ( ( UartId == UART_MANUF_TEST ) && ( DataLength > 0 ) && ( DataBuffer[DataLength-1] == '\n' ) )
      {
         if ( DataLength > 1 )
         {
            transmitUARTEnable[UartId] = true;
            OS_MUTEX_Lock  ( &UART_writeMutex[ (uint32_t)UartId ] );
            ( void )R_SCI_UART_Write( (void *)UartCtrl[ UartId ], DataBuffer, DataLength-1 );
            OS_MUTEX_Unlock( &UART_writeMutex[ (uint32_t)UartId ] );
            TM_UART_COUNTER_INC( uart_events[UartId].uartWritePendBefore );
            ( void )OS_SEM_Pend( &UART_semHandle[UartId].transmitUART_sem, OS_WAIT_FOREVER );
            TM_UART_COUNTER_INC( uart_events[UartId].uartWritePendAfter  );
   //         OS_TASK_Sleep( (uint32_t) 10 );  // It requires a sleep of 10 ms for this write to process/transmit the data completely
         }

         transmitUARTEnable[UartId] = true;
         TM_UART_COUNTER_INC( uart_events[UartId].uartWritePendBefore );
         OS_MUTEX_Lock  ( &UART_writeMutex[ (uint32_t)UartId ] );
         ( void )R_SCI_UART_Write( (void *)UartCtrl[ UartId ], (uint8_t *)&CRLF, sizeof(CRLF) );
         OS_MUTEX_Unlock( &UART_writeMutex[ (uint32_t)UartId ] );
         ( void )OS_SEM_Pend( &UART_semHandle[UartId].transmitUART_sem, OS_WAIT_FOREVER );
         TM_UART_COUNTER_INC( uart_events[UartId].uartWritePendAfter  );
      }
      else if ( DataLength > 0 ) /* Avoid potential problems with a zero-length write */
      {
         TM_UART_COUNTER_INC( uart_events[UartId].basicUARTWriteVar );
         transmitUARTEnable[UartId] = true;
         OS_MUTEX_Lock  ( &UART_writeMutex[ (uint32_t)UartId ] );
         ( void )R_SCI_UART_Write( (void *)UartCtrl[ UartId ], DataBuffer, DataLength );
         OS_MUTEX_Unlock( &UART_writeMutex[ (uint32_t)UartId ] );
         TM_UART_COUNTER_INC( uart_events[UartId].uartWritePendBefore );
         ( void )OS_SEM_Pend( &UART_semHandle[UartId].transmitUART_sem, OS_WAIT_FOREVER );
         TM_UART_COUNTER_INC( uart_events[UartId].uartWritePendAfter  );

      }
   }
   else if ( UartId == UART_DEBUG_PORT )
   {
      /* If called before UART is initialized, use the light-weight method for printing */
      uint8_t tempBuffer[100];
      if ( DataLength >= sizeof(tempBuffer) ) DataLength = sizeof(tempBuffer) - 1;
      memcpy (tempBuffer, DataBuffer, DataLength );
      tempBuffer[DataLength] = '\0';
      DBG_LW_printf( "%s\n", tempBuffer );
   }

   return DataLength;/* R_SCI_UART_Write does not return the no. of valid written bytes, returning DataLength */
#endif
}

#if ( RTOS_SELECTION == MQX_RTOS )
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
   uint32_t DataReceived;

   DataReceived = (uint32_t)read ( UartHandle[UartId], DataBuffer, (int32_t)DataLength ); /*lint !e64 */

   return ( DataReceived );
}

#elif ( RTOS_SELECTION == FREE_RTOS )
/*******************************************************************************
  Function name: UART_getc

  Purpose: This function will return the number of bytes requested only when that
           number of bytes have been received into the UART
  Arguments: UartId - Identifier of the particular UART to receive data in
             DataBuffer - pointer to the Data that is received (populated by this function)
             DataLength - number of bytes that are to be received before this function returns
             Timeout - Timeout for Counting Semaphore pends

  Returns: DataReceived - Number of valid bytes that are being returned in the buffer (1 or 0)

  Notes: This function will block processing for timeout until 1 byte has been received.
         This function always gets only one character and hence if it succeed, it will return datalength 0f 1

*******************************************************************************/
uint32_t UART_getc ( enum_UART_ID UartId, uint8_t *DataBuffer, uint32_t DataLength, uint32_t TimeoutMs )
{
   DataLength = 0;
   if ( UART_initReady_ )
   {
      TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].uartGetcPendBefore );
      ( void )OS_SEM_Pend( &UART_semHandle[ (uint32_t)UartId ].receiveUART_sem, TimeoutMs );
      TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].uartGetcPendAfter  );

      OS_INT_disable(); // Enter critical section
      {
         uint16_t tail = ( uartRingBuf[ (uint32_t)UartId ].tail ) % uartRingBuf[ (uint32_t)UartId ].size;
         if ( uartRingBuf[ (uint32_t)UartId ].head != tail )
         {
            *DataBuffer = uartRingBuf[UartId].pBuffer[ uartRingBuf[ (uint32_t)UartId ].tail ];
            uartRingBuf[ (uint32_t)UartId ].tail = ( tail + 1 ) % uartRingBuf[ (uint32_t)UartId ].size;
            DataLength = 1;
         }
         else
         {
            *DataBuffer = '?'; /* This is an aid to debugging.  Should never be needed */
         }
      }
      OS_INT_enable(); // Exit critical section

      if ( ringBufoverflow[ (uint32_t)UartId ] )
      {
         TM_UART_COUNTER_INC( uart_events[UartId].ringOverFlowVar );
         ( void ) DBG_LW_printf( "\r\nRing buffer overflow of UART Id - %ld\n", (uint32_t)UartId );
         UART_RX_flush( UartId );
      }
      /* If we get an overflow from the UART hardware, we must have lost interrupt enable on the peripheral.  Reset it. */
      if ( uartOverflow[ (uint32_t)UartId ] )
      {
         TM_UART_COUNTER_INC( uart_events[ (uint32_t)UartId ].uartOverflowFlagged );
         ( void ) DBG_LW_printf( "\r\nUART buffer overflow of UART Id - %ld\n", (uint32_t)UartId );
         UART_RX_flush( UartId );
         uartOverflow[ (uint32_t)UartId ] = (bool)false;
      }
   }
   else
   {
      *DataBuffer = '?'; /* This is an aid to debugging.  Should never be needed */
      ( void ) DBG_LW_printf( "Call made to UART_getc before UART is initialized\n" );
   }
   return DataLength; /* Returning DataLength */
}

/*******************************************************************************

  Function name: UART_echo

  Purpose: This function will Transmit the passed data buffer out on the appropriate
           UART connection

  Arguments: UartId - Identifier of the particular UART to send data on
             DataBuffer - pointer to the Data that is to be sent
             DataLength - number of bytes that are to be sent (size of data in DataBuffer)

  Returns: DataSent - number of bytes that were correctly sent out the UART

*******************************************************************************/
extern uint32_t UART_echo ( enum_UART_ID UartId, const uint8_t *DataBuffer, uint32_t DataLength )
{
   if ( UART_initReady_ )
   {
      if ( DataLength > 0 ) // RA6E1 Bob: prevent the possibility of writing zero length
      {
         if ( UartId != UART_HOST_COMM_PORT ) // Echo not required for HMC - If required enable the semaphore to perform write
         {
            if ( ( ( dbgUartEchoEnable_ ) && ( UartId == UART_DEBUG_PORT ) ) || ( UartId != UART_DEBUG_PORT ) )
            {
               TM_UART_COUNTER_INC( uart_events[UartId].UARTechoVar );
               OS_MUTEX_Lock  ( &UART_writeMutex[ (uint32_t)UartId ] );
               ( void )R_SCI_UART_Write( (void *)UartCtrl[ UartId ], DataBuffer, DataLength );
               OS_MUTEX_Unlock( &UART_writeMutex[ (uint32_t)UartId ] );
               TM_UART_COUNTER_INC( uart_events[UartId].uartEchoPendBefore );
               ( void )OS_SEM_Pend( &UART_semHandle[UartId].echoUART_sem, OS_WAIT_FOREVER );
               TM_UART_COUNTER_INC( uart_events[UartId].uartEchoPendAfter  );
            }
         }
         else
         {
            DBG_LW_printf( "Echo of UART - HMC is not supported." );
         }
      }
      else
      {
         TM_UART_COUNTER_INC( uart_events[UartId].ZeroLengthEcho );
      }
   }
   else
   {
      DBG_LW_printf( "UART_echo called before UART was initialized\n" );
   }
   return DataLength; /* R_SCI_UART_Write does not return the no. of valid written bytes, returning DataLength */
}

#endif // RTOS_SELECTION
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

         For RA6E1: This function will read and process the User inputs for
         Debug port alone

*******************************************************************************/
void UART_fgets( enum_UART_ID UartId, char *DataBuffer, uint32_t DataLength )
{
#if ( RTOS_SELECTION == MQX_RTOS )
   (void)fgets( DataBuffer, (int32_t)DataLength, UartHandle[UartId] );
#elif ( RTOS_SELECTION == FREE_RTOS )
   /* This function currently used for DBG port alone for RA6E1*/
   uint8_t rxByte = 0;
   uint32_t DBGP_numBytes = 0;
   uint32_t DBGP_printIndex = 0;
   uint32_t DBGP_numPrintBytes = 0;
   bool lineComplete = false;
   /* Loops Until Newline is detected */
   for ( ;; )
   {
      if ( 0 != UART_getc( UART_DEBUG_PORT, &rxByte, sizeof( rxByte ), OS_WAIT_FOREVER ) )
      {
         if ( ( rxByte == ESCAPE_CHAR ) ||  /* User pressed ESC key */
              ( rxByte == CTRL_C_CHAR ) ||  /* User pressed CRTL-C key */
              ( rxByte == 0xC0 ))           /* Left over SLIP protocol characters in buffer */
         {
            /* user canceled the in progress command */
            memset( DataBuffer, 0, DBGP_numBytes );
            DBGP_numBytes = 0;
            DBGP_numPrintBytes = 0;
            DBGP_printIndex = 0;
            rxByte = 0x0;
         }/* end if () */
         else if ( ( rxByte != (uint8_t)0x00 ) &&
                   ( rxByte != LINE_FEED_CHAR ) &&
                   ( rxByte != CARRIAGE_RETURN_CHAR ) &&
                   ( rxByte != BACKSPACE_CHAR) &&
                   ( rxByte != DELETE_CHAR ) )
         {
            DataBuffer[DBGP_numBytes++] = rxByte;
            DataBuffer[DBGP_numBytes  ] = '\0';   /* Make sure string is always terminated */
            DBGP_numPrintBytes++;
            ( void )UART_echo( UartId, (const uint8_t *)&rxByte, sizeof(rxByte) );
            rxByte = 0x0;
            while ( DBGP_numBytes < DataLength )
            {
               /* UART_read used to read characters while doing Copy/Paste - 10millisecond is the delay timing where a successful UART_read happens */
               if ( 0 != UART_getc ( UART_DEBUG_PORT, &rxByte, sizeof(rxByte), OS_WAIT_FOREVER ) )
               {
                  if ( ( rxByte == ESCAPE_CHAR ) ||  /* User pressed ESC key */
                       ( rxByte == CTRL_C_CHAR ) ||  /* User pressed CRTL-C key */
                       ( rxByte == 0xC0 ) )          /* Left over SLIP protocol characters in buffer */
                  {
                     /* user canceled the in progress command */
                     memset( DataBuffer, 0, DBGP_numBytes );
                     DBGP_numBytes = 0;
                     DBGP_numPrintBytes = 0;
                     DBGP_printIndex = 0;
                     rxByte = 0x0;
                  }/* end if () */
                  else if( ( rxByte != (uint8_t)0x00 ) &&
                           ( rxByte != LINE_FEED_CHAR ) &&
                           ( rxByte != CARRIAGE_RETURN_CHAR ) &&
                           ( rxByte != BACKSPACE_CHAR) &&
                           ( rxByte != DELETE_CHAR ) )
                  {
                     DBGP_numPrintBytes++;
                     DataBuffer[DBGP_numBytes++] = rxByte;
                     DataBuffer[DBGP_numBytes  ] = '\0';   /* Make sure string is always terminated */
                     ( void )UART_echo( UartId, (const uint8_t *)&rxByte, sizeof(rxByte) );
                     rxByte = 0x0;
                  }/* end if () */
                  else if ( ( rxByte == LINE_FEED_CHAR ) ||
                            ( rxByte == CARRIAGE_RETURN_CHAR ) )
                  {
                     if ( DBGP_numBytes == 0 )
                     {
                        ( void )UART_echo( UART_DEBUG_PORT, (const uint8_t *)CRLF, sizeof( CRLF ) );
                     }
                     else
                     {
                        rxByte = 0x0;
                        ( void )UART_echo( UART_DEBUG_PORT, (const uint8_t *)CRLF, sizeof( CRLF ) );
                        DBGP_numPrintBytes = 0;
                        DBGP_printIndex = 0;
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
                        DBGP_numPrintBytes--;
                        rxByte = 0x0;
                        ( void )UART_echo( UART_DEBUG_PORT, (const uint8_t *)"\b\x20\b", 3 );
                     }
                  }/* end else if () */
               }
            }/* end for () */
         }/* end else if () */
         else if ( rxByte == CARRIAGE_RETURN_CHAR )
         {
            if( DBGP_numBytes == 0 )
            {
               ( void )UART_echo( UART_DEBUG_PORT, (const uint8_t *)CRLF, sizeof( CRLF ) );
            }
            else
            {
               ( void )UART_echo( UART_DEBUG_PORT, (const uint8_t *)CRLF, sizeof( CRLF ) );
               DBGP_numPrintBytes = 0;
               DBGP_printIndex = 0;
               rxByte = 0x0;
            }

            lineComplete = true;
         }
         /* Fix for issue from Regression Test Tool Version 1.1.0 */
         else if ( rxByte == LINE_FEED_CHAR )
         {
            rxByte = 0x0;
         }
         else if( rxByte == BACKSPACE_CHAR || rxByte == DELETE_CHAR )
         {
            if( DBGP_numBytes != 0 )
            {
               /* buffer contains at least one character, remove the last one entered */
               /* Resets the last entered character */
               DataBuffer[ --DBGP_numBytes ] = 0x00;
               DBGP_printIndex--;
               rxByte = 0x0;
               /* Gives GUI effect in Terminal for Backspace */
               ( void )UART_echo( UART_DEBUG_PORT, (uint8_t*)"\b\x20\b", 3 );
            }
         }/* end else if () */
         else
         {
            ( void )UART_echo( UART_DEBUG_PORT, (uint8_t*)&DataBuffer[DBGP_printIndex], DBGP_numPrintBytes );
            DBGP_printIndex += DBGP_numPrintBytes;
            DBGP_numPrintBytes = 0;
         }
      }
      if ( ( lineComplete == true ) || ( DBGP_numBytes >= DataLength ) )
      {
         break;
      }
   }
#endif // RTOS_SELECTION
}


/*******************************************************************************

  Function name: UART_flush

  Purpose: This function flushes the uart TX buffers

  Arguments: UartId - Identifier of the particular UART to receive data in

  Returns: uint8_t - UART_flush status

  Notes:

*******************************************************************************/
uint8_t UART_flush( enum_UART_ID UartId )
{
#if ( RTOS_SELECTION == MQX_RTOS )
   return ( (uint8_t)fflush(UartHandle[UartId]) );
#elif ( RTOS_SELECTION == FREE_RTOS )

   OS_INT_disable();    // Enable critical section as we are creating ring buffers and initializing them
   TM_UART_COUNTER_INC( uart_events[UartId].uartFlushCalls );
   // Setting bool values to false at init
   ringBufoverflow   [ (uint32_t)UartId ] = false;
   uartOverflow      [ (uint32_t)UartId ] = false;
   uartRingBuf       [ (uint32_t)UartId ].head = 0;
   uartRingBuf       [ (uint32_t)UartId ].tail = 0;

   if (( UartId == UART_MANUF_TEST ) ||  ( UartId == UART_DEBUG_PORT ) )
   {
      OS_SEM_Reset ( &UART_semHandle[ (uint32_t)UartId ].transmitUART_sem );
      OS_SEM_Reset ( &UART_semHandle[ (uint32_t)UartId ].receiveUART_sem );
      OS_SEM_Reset ( &UART_semHandle[ (uint32_t)UartId ].echoUART_sem );
   }
   else if( UartId == UART_HOST_COMM_PORT )
   {
      /* HMC does not have the echoUART_sem, So that we need not to reset the echoUART_sem */
      OS_SEM_Reset ( &UART_semHandle[ (uint32_t)UartId ].transmitUART_sem );
      OS_SEM_Reset ( &UART_semHandle[ (uint32_t)UartId ].receiveUART_sem );
   }
   else
   {
      //do nothing
   }

   OS_INT_enable();
   return eSUCCESS;
#endif
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
#if ( RTOS_SELECTION == MQX_RTOS )
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
#elif ( RTOS_SELECTION == FREE_RTOS )

   OS_INT_disable();    // Enable critical section as we are creating ring buffers and initializing them
   TM_UART_COUNTER_INC( uart_events[UartId].uartFlushCalls );
   // Setting bool values to false at init
   ringBufoverflow   [ (uint32_t)UartId ] = false;
   uartOverflow      [ (uint32_t)UartId ] = false;
   uartRingBuf       [ (uint32_t)UartId ].head = 0;
   uartRingBuf       [ (uint32_t)UartId ].tail = 0;

   if (( UartId == UART_MANUF_TEST ) ||  ( UartId == UART_DEBUG_PORT ) || ( UartId == UART_HOST_COMM_PORT ) )
   {
      OS_SEM_Reset ( &UART_semHandle[ (uint32_t)UartId ].receiveUART_sem );
   }

   OS_INT_enable();
#endif // RTOS_SELECTION
}
#if ( MCU_SELECTED == NXP_K24 )
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
#endif // #if ( MCU_SELECTED == NXP_K24 )
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
 #if ( MCU_SELECTED == NXP_K24 )
   uint32_t flags;

   (void)UART_ioctl ( UartId, IO_IOCTL_SERIAL_GET_FLAGS, &flags ); // Get current settings
   if ( val ) {
      flags |=  IO_SERIAL_ECHO; // Enable ECHO
   } else {
      flags &= ~IO_SERIAL_ECHO; // Disable ECHO
   }
   return UART_ioctl ( UartId, IO_IOCTL_SERIAL_SET_FLAGS, &flags ); // Update settings
#elif ( MCU_SELECTED == RA6E1 )

   dbgUartEchoEnable_ = val;

   return val;
#endif
}

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
         retVal = (uint8_t)ioctl(UartHandle[UartId], IO_IOCTL_SERIAL_SET_FLAGS, (void *)&Flags); /*lint !e835 !e845 !e64 */
      }
   }
   return retVal;
#elif ( MCU_SELECTED == RA6E1 )
   /*Uart Open for RA6E1 is done in Uart Init*/
   return ( uint8_t )eSUCCESS;
#endif
}

#if ( TM_UART_EVENT_COUNTERS == 1 )
/*******************************************************************************

  Function name: UART_GetCounters

  Purpose: Retrieve the UART diagnostic counters for a specified UART port

  Arguments: UartId - Identifier of the particular UART for which data is to be retrieved

  Returns: Contents of Uart_Events_t structure for that port
  Returns: true if the port number is valid, otherwise false

  Notes:

*******************************************************************************/
extern bool UART_GetCounters  ( enum_UART_ID UartId, const Uart_Events_t *pReturnStruct )
{
   if ( UartId < MAX_UART_ID )
   {
      OS_INT_disable();
      memcpy ( (void *)pReturnStruct, (void *)&uart_events[ (uint32_t)UartId ], sizeof(Uart_Events_t) );
      OS_INT_enable ();
      return (bool)true;
   }
   return (bool)false;
}

/*******************************************************************************

  Function name: UART_ClearCounters

  Purpose: Clears the UART diagnostic counters for a specified UART port

  Arguments: UartId - Identifier of the particular UART for which data is to be retrieved

  Returns: true if the port number is valid, otherwise false

  Notes:

*******************************************************************************/
extern bool UART_ClearCounters( enum_UART_ID UartId )
{
   if ( UartId < MAX_UART_ID )
   {
      OS_INT_disable();
      memset ( (void *)&uart_events[ (uint32_t)UartId ], 0L, sizeof(Uart_Events_t) );
      OS_INT_enable ();
      return (bool)true;
   }
   return (bool)false;
}
#endif // ( TM_UART_EVENT_COUNTERS == 1 )