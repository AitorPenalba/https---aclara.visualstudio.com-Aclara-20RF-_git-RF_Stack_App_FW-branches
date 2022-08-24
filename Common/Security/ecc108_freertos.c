/***********************************************************************************************************************
 *
 * Filename:   ecc108_freertos.c
 *
 * Contents: Interface code between ECC108 and freertos, file calls
 * low level I2C funtions
 * wrapper layer for Freescale I2C example funtions to Atmel function names
 *
 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   $Log$ Suriya Created May 26, 2022

 ***********************************************************************************************************************
   Revision History:

 ***********************************************************************************************************************
 **********************************************************************************************************************/
/* INCLUDE FILES */

/*lint --esym(766,assert.h)   assert.h not used */
#include <assert.h>
#include "project.h"

#if ( RTOS_SELECTION == MQX_RTOS )
#error "This file supports only in FreeRTOS. Check the RTOS_SELECTION define and modify considering the OS usage."
#include <mqx.h>
#include <fio.h>
#include <i2c.h>
#include <io_gpio.h>
#endif

#include "buffer.h"
#include "MAC.h"
#include "CRC.h"
#include "dvr_intFlash_cfg.h"
#include "ecc108_comm_marshaling.h"    /* definitions and declarations for the Command Marshaling module */
#include "ecc108_lib_return_codes.h"
#include "ecc108_freertos.h"
#include "ecc108_apps.h"
#include "ecc108_physical.h"
#include "ecc108_comm.h"
#include "pwr_task.h"
#if ( EP == 1 )
#include "pwr_last_gasp.h"
#endif

#define WRITE_READ_IIC_TIMEOUT            10   // Timeout for read and write complete interrupts - 10 ticks /* TODO: SMG Check against longest execution time of the ECC508 - some are 115ms (+50ms as recommended) */
#define IIC_TRANSMIT_MAX_VALUE            255  // TODO: RA6E1: Verify the working after integrating WolfSSL

#if BSPCFG_ENABLE_II2C0 == 0        /*  Polled version   */
const char i2cName[] = "i2c0:";
#else
const char i2cName[] = "ii2c0:";     /*  Interrupt version   */
#endif

static PartitionData_t const *  pSecurePart;   /* Partion handle for security information         */
static OS_MUTEX_Obj             I2Cmutex_;     /* i2c mutext for thread-safe operation            */
static OS_SEM_Obj               _i2cTransmitRecieveSem;    /* Semaphore to notify the transmit and receive complete */
static bool                     _i2cTransmitRecieveSemCreated = (bool)false;

volatile i2c_master_event_t     i2c_event = I2C_MASTER_EVENT_ABORTED;

static uint8_t                  sendVal[IIC_TRANSMIT_MAX_VALUE];


/***********************************************************************************************************************
   Function Name: iic_eventCallback

   Purpose: Event callback from the interrupt context to let the user knows about the status of the IIC events

   Arguments: i2c_master_callback_args_t * p_args

   Returns: Nothing
***********************************************************************************************************************/
void iic_eventCallback( i2c_master_callback_args_t * p_args )
{
   if (NULL != p_args)
   {
      /* capture callback event for validating the i2c transfer event*/
      i2c_event = p_args->event;
      if ( ( i2c_event == I2C_MASTER_EVENT_TX_COMPLETE ) ||
           ( i2c_event == I2C_MASTER_EVENT_RX_COMPLETE ) )
      {
         OS_SEM_Post_fromISR( &_i2cTransmitRecieveSem );
      }
   }
}

/**************************************************************************************************

   Function: SEC_init
   Purpose:  Security initialization:
             - Validate/Update Host Authorization Key
             - Create i2c mutex

   Arguments: None
   Return - uint8_t Success/failure of operation - See ecc108_lib_return_codes.h for codes

**************************************************************************************************/
returnStatus_t SEC_init( void )
{
   uint8_t     deviceMAC[8];  /* MAC from security device   */
   uint8_t     localMAC[8];   /* MAC from ROM               */
   eui_addr_t  addr;          /* value from external NV */
   returnStatus_t retVal;

   retVal = PAR_partitionFptr.parOpen(&pSecurePart, ePART_ENCRYPT_KEY, 0);
   if (retVal != eSUCCESS)
   {  /* Partition opened failed  */
      return retVal;
   }
   if ( !OS_MUTEX_Create( &I2Cmutex_ ) )
   {  /* Mutex creation failed */
      return ((returnStatus_t)101);
   }
   if ( ECC108_SUCCESS != ecc108_open() )
   {  /* Open failed  */
      return ((returnStatus_t)102);
   }

   retVal = eFAILURE;
   /* Read the device serial number at start up */
   if ( ECC108_SUCCESS == ecc108e_GetSerialNumber() )   /* Update device serial number   */
   {
#if ( EP == 1 )
      if( PWRLG_LastGasp() == 0 )
#endif
      {
         ecc108e_InitKeys();
         /* Now, get the MAC from the security device, if available   */
         if ( ECC108_SUCCESS == ecc108e_read_mac( deviceMAC ) )
         {
            retVal = PAR_partitionFptr.parRead( localMAC,
                                                (dSize)offsetof( intFlashNvMap_t,mac ),
                                                sizeof(localMAC),
                                                pSecurePart );
            if ( eSUCCESS == retVal )
            {
               /* Check full MAC from device against value in ROM address (8 bytes)   */
               if( memcmp( deviceMAC, localMAC, sizeof( deviceMAC) ) != 0 )
               {
#if ( EP == 1 )
                  PWR_lockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails   */
#else
                  PWR_lockMutex(); /* Function will not return if it fails */
#endif
                  /* Update ROM  */
                  retVal = PAR_partitionFptr.parWrite( (dSize)offsetof( intFlashNvMap_t,mac ),
                                                       deviceMAC,
                                                       sizeof(deviceMAC),
                                                       pSecurePart );
                  MAC_EUI_Address_Set( (eui_addr_t *)(void *)&deviceMAC[ 0 ] ); /* Update NV   */
#if ( EP == 1 )
                  PWR_unlockMutex( PWR_MUTEX_ONLY ); /* Function will not return if it fails */
#else
                  PWR_unlockMutex(); /* Function will not return if it fails  */
#endif
               }
            }
            /* Now verify full MAC address with external NV */
            MAC_EUI_Address_Get( (eui_addr_t *)addr );
            if( memcmp( deviceMAC, (uint8_t *)&addr[ 0 ], sizeof( deviceMAC) ) != 0 )
            {
               MAC_EUI_Address_Set( (eui_addr_t *)(void *)&deviceMAC[ 0 ] ); /* Update NV   */
            }
         }
      }
#if ( EP == 1 )
      else
      {
         retVal = eSUCCESS;   /* Last gasp mode; partition open, mutex created, SN read: success */
      }
#endif
   }
   ecc108_close();

   return(retVal);
}
/***********************************************************************************************************************
   Function Name: SEC_GetSecPartHandle

   Purpose: Return the SEC_test file handle

   Arguments: none

   Returns: Security partition handle
***********************************************************************************************************************/
PartitionData_t const *SEC_GetSecPartHandle( void )
{
   return pSecurePart;
}


/***********************************************************************************************************************
   Function Name: ecc108_open

   Purpose: Open the i2c port

   Arguments: none

   Returns: Status of the open
***********************************************************************************************************************/
uint8_t ecc108_open( void )
{
   // TODO: RA6E1 - Check this removal of wakeup call when integrating with WolfSSL
   uint8_t wakeup_response[ECC108_RSP_SIZE_MIN];

   if ( (bool)false == _i2cTransmitRecieveSemCreated )
   {
      //TODO RA6: NRJ: determine if semaphores need to be counting
      if ( OS_SEM_Create( &_i2cTransmitRecieveSem , 0 ) )
      {
         _i2cTransmitRecieveSemCreated = true;
      }
   }

   fsp_err_t err = R_IIC_MASTER_Open( &g_i2c_master0_ctrl, &g_i2c_master0_cfg );
   if ( FSP_SUCCESS == err )
   {
       (void)ecc108c_wakeup( wakeup_response );   // This wakeup call here results in not communicating with the security chip as there will be another wakeup at every APIs
   }

   return ( uint8_t ) err;
}

/***********************************************************************************************************************
   Function Name: ecc108_close

   Purpose: Close the i2c port

   Arguments: none

   Returns: none
***********************************************************************************************************************/
void ecc108_close( void )
{
    ( void ) ecc108p_sleep( );
    ( void ) R_IIC_MASTER_Close( &g_i2c_master0_ctrl );
}

/***********************************************************************************************************************
   Function Name: delay_10us

   Purpose: This routine provides an accurate delay in 10us increments

   Arguments: delay - number of 10us periods to delay

   Returns: none
***********************************************************************************************************************/
void delay_10us( uint32_t delay )
{
   OS_TICK_Struct   startTime;                   /* Used to delay sub-tick time */
   OS_TICK_Struct   endTime;                     /* Used to delay sub-tick time */
   uint32_t         localDelay;                  /* Time converted from 10us to us */

   OS_TICK_Get_CurrentElapsedTicks( &startTime );
   endTime = startTime;
   localDelay = delay * 10;

   while ( ( uint32_t ) OS_TICK_Get_Diff_InMicroseconds(  &startTime , &endTime ) < localDelay )
   {
      OS_TICK_Get_CurrentElapsedTicks( &endTime );
   }
}

/***********************************************************************************************************************
   delay_ms()   This routine provides an accurate delay in 1ms increments
   Arguments: delay - number of 1ms periods to delay
   Returns: void
***********************************************************************************************************************/
void delay_ms( uint32_t delay )
{
   delay_10us( delay * 100 );
}

/***********************************************************************************************************************
   Function Name: i2c_wakeUp

   Purpose: Wakeup the security chip. To wakeup, make the SDA pin low for 70us and high for 1ms. This will result in response
            from security chip

   Arguments: none

   Returns: none
***********************************************************************************************************************/
void i2c_wakeUp(void)
{
   R_BSP_PinCfg( BSP_IO_PORT_04_PIN_01, ((uint32_t) IOPORT_CFG_DRIVE_HIGH
           | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW) );
   R_BSP_PinAccessEnable();

   R_BSP_PinWrite ( BSP_IO_PORT_04_PIN_01, BSP_IO_LEVEL_LOW);
   R_BSP_SoftwareDelay(70U, BSP_DELAY_UNITS_MICROSECONDS);
   R_BSP_PinWrite ( BSP_IO_PORT_04_PIN_01, BSP_IO_LEVEL_HIGH);
   R_BSP_SoftwareDelay(1000U, BSP_DELAY_UNITS_MICROSECONDS);

   R_BSP_PinCfg( BSP_IO_PORT_04_PIN_01, ((uint32_t) IOPORT_CFG_DRIVE_MID | (uint32_t) IOPORT_CFG_PERIPHERAL_PIN
           | (uint32_t) IOPORT_CFG_PULLUP_ENABLE | (uint32_t) IOPORT_PERIPHERAL_IIC) );
}

/***********************************************************************************************************************
   Function Name: i2c_receive_response

   Purpose: Receive the response message from the security chip

   Arguments: uint8_t size, uint8_t *response

   Returns: Receive status
***********************************************************************************************************************/
uint8_t i2c_receive_response( uint8_t size, uint8_t *response )
{
   fsp_err_t err = FSP_SUCCESS;
   err = R_IIC_MASTER_Read(&g_i2c_master0_ctrl, response, size, false);
   if (err == FSP_SUCCESS)
   {
      /* Pend for the receive complete interrupt. Wait for only certain time as there
       * won't be replies for every tranmit command from the security chip */
      ( void ) OS_SEM_Pend( &_i2cTransmitRecieveSem, WRITE_READ_IIC_TIMEOUT );
      if( I2C_MASTER_EVENT_RX_COMPLETE != i2c_event )
      {
         err = (uint8_t) ECC108_RX_NO_RESPONSE;
      }
   }

   return (uint8_t) err;
}

/***********************************************************************************************************************
   Function Name: ecc108p_wakeup

   Purpose: Wakeup the security chip

   Arguments: none

   Returns: Status of the wakeup
***********************************************************************************************************************/
uint8_t ecc108p_wakeup( void )
{
   (void) i2c_wakeUp();
   return ECC108_SUCCESS;
}


/***********************************************************************************************************************
   Function Name: ecc108p_flush

   Purpose: Flush the file descriptor

   Arguments: none

   Returns: Status of the flush

   Note: Not implemented for FreeRTOS
***********************************************************************************************************************/
uint8_t ecc108p_flush( void )
{
   return ECC108_SUCCESS;
}

/***********************************************************************************************************************
   Function Name: i2c_send

   Purpose: Send the message to the security chip

   Arguments: uint8_t word_address, uint8_t count, uint8_t *buffer

   Returns: Send status
***********************************************************************************************************************/
uint8_t i2c_send( uint8_t word_address, uint8_t count, uint8_t *buffer )
{
   fsp_err_t err = FSP_SUCCESS;
   memset(sendVal, 0, sizeof(sendVal));
   uint8_t sendCount = count + 1;
   sendVal[0] = word_address;
   for(uint8_t i = 1; i < sendCount; i++ )
   {
      sendVal[i] = buffer[i-1];
   }

   err = R_IIC_MASTER_Write( &g_i2c_master0_ctrl, sendVal, sendCount, false );
   /* Pend for the transmit complete. Wait for only certain time to avoid hung up
    * as there wont be any interrupts for the reset and sleep commands from the security chip */
   ( void ) OS_SEM_Pend( &_i2cTransmitRecieveSem, WRITE_READ_IIC_TIMEOUT );
   if( I2C_MASTER_EVENT_TX_COMPLETE != i2c_event )
   {
      err = (uint8_t) ECC108_GEN_FAIL;
   }

   return (uint8_t) err;
}
