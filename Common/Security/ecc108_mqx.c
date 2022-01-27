/***********************************************************************************************************************
 *
 * Filename:   ecc108_mqx.c
 *
 * Contents: Interface code between ECC108 and mqx drivers.
 * low level I2C funtions
 * wrapper layer for Freescale I2C example funtions to Atmel function names
 *
 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2015-2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
  
   $Log$ mvirkus Created April 20, 2015
  
 ***********************************************************************************************************************
   Revision History:
   20150420 - mkv: Initial Release
  
 ***********************************************************************************************************************
 **********************************************************************************************************************/
/* INCLUDE FILES */

/*lint --esym(766,assert.h)   assert.h not used */
#include <assert.h>
#include <mqx.h>
#include <fio.h>
#include <i2c.h>
#include <io_gpio.h>

#include "project.h"
#include "buffer.h"
#include "MAC.h"
#include "CRC.h"
#include "dvr_intFlash_cfg.h"
#include "ecc108_comm_marshaling.h"    /* definitions and declarations for the Command Marshaling module */
#include "ecc108_lib_return_codes.h"
#include "ecc108_mqx.h"
#include "ecc108_apps.h"
#include "ecc108_physical.h"
#include "ecc108_comm.h"
#include "pwr_task.h"
#if ( EP == 1 )
#include "pwr_last_gasp.h"
#endif

#if BSPCFG_ENABLE_II2C0 == 0        /*  Polled version   */
const char i2cName[] = "i2c0:";
#else
const char i2cName[] = "ii2c0:";     /*  Interrupt version   */
#endif

static PartitionData_t const *   pSecurePart;   /* Partion handle for security information         */
static OS_MUTEX_Obj              I2Cmutex_;     /* i2c mutext for thread-safe operation            */
static MQX_FILE_PTR              i2cfd;         /* mqx file handle, created when device is opened. */

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

uint8_t ecc108_open()
{
   uint8_t wakeup_response[ECC108_RSP_SIZE_MIN];
   uint32_t baud = 937500;      /* ECC108 spec = 1MHz max - set to be a bit less  */
   uint32_t i2cAddr = ECC108_I2C_DEFAULT_ADDRESS >> 1;
   uint8_t  ret_code = ECC108_GEN_FAIL;

   OS_MUTEX_Lock( &I2Cmutex_ ); /* Function will not return if it fails */

   i2cfd = fopen( i2cName, NULL);
   if ( i2cfd != NULL )
   {
      ret_code = ECC108_SUCCESS;
      (void)ioctl(i2cfd, IO_IOCTL_I2C_SET_MASTER_MODE, NULL);
      (void)ioctl(i2cfd, IO_IOCTL_I2C_SET_BAUD, &baud );
      (void)ioctl(i2cfd, IO_IOCTL_I2C_SET_DESTINATION_ADDRESS, &i2cAddr );
      (void)ecc108c_wakeup( wakeup_response );
   }
   else
   {
      ret_code = ECC108_GEN_FAIL;
      i2cfd = NULL;
   }
   return ret_code;  /*lint !e454 !e456 Mutex locked while device is "open"   */
}  /*lint !e454 !e456 Mutex locked while device is "open"   */

/***********************************************************************************************************************
   Function Name: ecc108_close

   Purpose: Close the i2c port

   Arguments: none

   Returns: none
***********************************************************************************************************************/
void ecc108_close()
{
   (void)ecc108p_sleep();
   assert ( fclose( i2cfd ) == MQX_OK );  /*lint !e522 !e506 lacks side effects; constant boolean */
   i2cfd = NULL;
   OS_MUTEX_Unlock( &I2Cmutex_ ); /*lint !e455 mutex release when device is closed   */ /* Function will not return if it fails  */
}

/***********************************************************************************************************************
   Function Name: ecc108_isOpen

   Purpose: Returns state ecc108 "file"

   Arguments: none

   Returns: bool - true -> device is open
***********************************************************************************************************************/
bool ecc108_isOpen()
{
   return ( i2cfd != NULL );
}

/* This function is only here to satisfy API requirements. Original functionality handled by bsp init */
void i2c_enable(void)
{
   /* This is handled by the mqx bsp initialization   */
}

/***********************************************************************************************************************
   i2c_send_start() - This function creates a Start condition (SDA low, then SCL low).

   Arguments: fd - Device address - lsb determines read/write, read = 1.
   Returns: success
***********************************************************************************************************************/
uint8_t i2c_send_start(uint8_t sla)
{
   uint8_t dummy;

   if ( (sla & 1) != 0 )      /* Start a read operation  */
   {
      (void)fread( &dummy, 1, 0, i2cfd );
   }
   else
   {
      (void)fwrite( & dummy, 1, 0, i2cfd );
   }

   return I2C_FUNCTION_RETCODE_SUCCESS;  /* could add some error checks!   */
}

/***********************************************************************************************************************
   i2c_send_stop() - This function creates a Stop condition (SCL high, then SDA high).

   Arguments: None
   Returns: success
***********************************************************************************************************************/
uint8_t i2c_send_stop()
{
  (void)ioctl( i2cfd, IO_IOCTL_I2C_STOP, NULL);
  return I2C_FUNCTION_RETCODE_SUCCESS;  /* could add some error checks! */
}

/***********************************************************************************************************************
   i2c_send_bytes() - This function sends bytes to an I2C device.

   Arguments: Count - number of bytes to transmit
              data - pointer to data to be transmitted
   Returns: SUCCESS, if transfer occurs. FAILURE, otherwise
***********************************************************************************************************************/
uint8_t i2c_send_bytes(uint8_t Count, uint8_t *data)
{
   uint8_t retval = I2C_FUNCTION_RETCODE_SUCCESS;

#if BSPCFG_ENABLE_II2C0 == 0   /*  Polled version   */
   if ( Count != fwrite( data, 1, Count, i2cfd ) )
   {
      retval = I2C_FUNCTION_RETCODE_COMM_FAIL;
   }
#else
   uint8_t bytesWritten = 0;
   do
   {
      bytesWritten = (uint8_t)fwrite( data + bytesWritten, 1, Count - bytesWritten, i2cfd );
   } while ( bytesWritten < Count );
#endif
   (void)fflush( i2cfd );
   return retval;  /* could add some error checks! */
}

/***********************************************************************************************************************
   i2c_receive_bytes() - This function receives bytes from an I2C device and sends a Stop.
   Arguments:  Count - number of bytes to receive
               data  - pointer to rx buffer
   Returns: SUCCESS
***********************************************************************************************************************/
uint8_t i2c_receive_bytes(uint8_t Count, uint8_t *data)
{
   uint8_t retval = I2C_FUNCTION_RETCODE_SUCCESS;
   uint8_t readBytes = 0;

   (void)ioctl( i2cfd, IO_IOCTL_I2C_REPEATED_START, NULL);
   (void)ioctl( i2cfd, IO_IOCTL_I2C_SET_RX_REQUEST, &Count );
#if BSPCFG_ENABLE_II2C0 == 0   /*  Polled version   */

   readBytes = fread( data, 1, Count, i2cfd );
#else                         /* Interrupt version */
   do
   {
      readBytes += (uint8_t)fread( data + readBytes, 1, Count - readBytes, i2cfd );
   }while ( readBytes < Count );
#endif

   return retval;  /* could add some error checks! */
}

/***********************************************************************************************************************
   i2c_receive_byte() - This function receives one byte from an I2C device.
   Arguments: data - pointer to received byte
   Returns: Success/failure of operation
***********************************************************************************************************************/
uint8_t i2c_receive_byte(uint8_t *data)
{
   uint8_t retval = I2C_FUNCTION_RETCODE_SUCCESS;
   uint32_t Count = 1;

   (void)ioctl( i2cfd, IO_IOCTL_I2C_REPEATED_START, NULL );
   (void)ioctl( i2cfd, IO_IOCTL_I2C_SET_RX_REQUEST, &Count );
#if BSPCFG_ENABLE_II2C0 == 0   /*  Polled version   */
   if ( Count != fread( data, 1, Count, i2cfd ) )
   {
      retval = I2C_FUNCTION_RETCODE_COMM_FAIL;
   }
#else
   Count = 0;
   do
   {
      Count += (uint32_t)fread( data, 1, 1, i2cfd );
   } while ( Count < 1 );
#endif
	return retval;
}

/***********************************************************************************************************************
   delay_10us()   This routine provides an accurate delay in 10us increments
   Arguments: delay - number of 10us periods to delay
   Returns: void
***********************************************************************************************************************/
void delay_10us( uint32_t delay )
{
   MQX_TICK_STRUCT   startTime;              /* Used to delay sub-tick time               */
   MQX_TICK_STRUCT   endTime;                /* Used to delay sub-tick time               */
   uint32_t          localDelay;             /* Time converted from 10us to us   */
   bool              overflow=(bool)false;   /* required by tick to microsecond function  */

   _time_get_ticks(&startTime);
   endTime = startTime;
   localDelay = delay * 10;

   while ( (uint32_t)_time_diff_microseconds(&endTime, &startTime, &overflow) < localDelay)
   {
      _time_get_ticks(&endTime);
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
   i2c_wakeup() wake up the crypto device on the I2C port
   Arguments: none
   Returns: void
***********************************************************************************************************************/
void i2c_wakeup(void)
{
  /* set the data pin as an I/O pin and drive low for (min)60uS  */
  PORTB_PCR3 = (PORT_PCR_ODE_MASK | PORT_PCR_MUX(1)); /* set i/o pin, open drain enabled.    */
  GPIOB_PDDR |= GPIO_PDDR_PDD(1<<3 );                 /* set as output  */

  GPIOB_PCOR = (1<<3);        /* Set low to wake device up  */
  delay_10us(7);              /* Data sheet calls for 60us min, add some margin  */

  /* Set data pin high and wait for 1000us */
  GPIOB_PSOR = (1<<3);
  delay_10us(100);            /* Wait 1ms (mfr recommends at least 9xxus)   */

  PORTB_PCR3 = PORT_PCR_ODE_MASK | PORT_PCR_MUX(2);   /* set data pin back to I2C mode, with open drain enabled   */
}
/***********************************************************************************************************************
   i2c_receive_response() - Receive ECC108's response to previously sent command
   Arguments:  size - number of bytest expected
               response - pointer to buffer to receive data
   Returns: Success/failure of operation
***********************************************************************************************************************/
uint8_t i2c_receive_response(uint8_t size, uint8_t *response)
{
	uint32_t Count = size;
	uint32_t readBytes = 0;
   uint32_t i2cStatus;
   uint8_t retVal = I2C_FUNCTION_RETCODE_SUCCESS;

   (void)ioctl( i2cfd, IO_IOCTL_I2C_SET_RX_REQUEST, &Count );

#if BSPCFG_ENABLE_II2C0 == 0   /*  Polled version   */

   readBytes = fread( response, 1, Count, i2cfd );
#else                         /* Interrupt version */
#if ( MQX_VERSION == 420 )
   (void)ioctl( i2cfd, IO_IOCTL_I2C_GET_STATE, &i2cStatus );
#endif
   do
   {
#if ( MQX_VERSION == 410 )
      (void)ioctl( i2cfd, IO_IOCTL_I2C_GET_STATE, &i2cStatus );
#endif
      if (i2cStatus == I2C_STATE_LOST_ARBITRATION )
      {
         retVal = I2C_FUNCTION_RETCODE_TIMEOUT;
         break;
      }
      readBytes += (uint32_t)fread( response + readBytes, 1, (int32_t)(Count - readBytes), i2cfd );
#if ( MQX_VERSION == 420 )
      (void)ioctl( i2cfd, IO_IOCTL_I2C_GET_STATE, &i2cStatus );
#endif
      /* Check for all bytes received, or NAK (state is finished) */
   }while (( readBytes < Count ) && ( i2cStatus != I2C_STATE_FINISHED) );
#endif
   (void) i2c_send_stop();    /* Required to get I2CSTATE back to ready!   */

   if ( readBytes < Count )   /* Device NAK'd - not present */
   {
      retVal = ECC108_RX_NO_RESPONSE;
   }
   else  /* Responded, validate data   */
   {
      Count = response[ECC108_BUFFER_POS_COUNT];
      if ((Count < ECC108_RSP_SIZE_MIN) || (Count > size))
      {
         retVal = ECC108_INVALID_SIZE;
      }
   }
   return retVal;
}

/***********************************************************************************************************************
   ecc108p_wakeup() - Wrapper or i2c_wakeup()
   Arguments: None
   Returns: SUCCESS
***********************************************************************************************************************/
uint8_t ecc108p_wakeup()
{
   i2c_wakeup();
   return ECC108_SUCCESS;
}

/***********************************************************************************************************************
   ecc108p_flush() - Wrapper for mqx flush routine
   Arguments: None
   Returns: result of flush operation
***********************************************************************************************************************/
uint8_t ecc108p_flush( void )
{
   return (uint8_t)fflush( i2cfd );
}

/***********************************************************************************************************************
   i2c_send() - Utility function that concatenates the ECC108 word address with the rest of a command
   Arguments:  word_address - ECC108 specific value
               buffer - Destination for response
   Returns: SUCCESS/FAILURE of operation
***********************************************************************************************************************/
uint8_t i2c_send(uint8_t word_address, uint8_t Count, uint8_t *buffer)
{
	uint8_t i2cStatus = ECC108_GEN_FAIL;
	uint32_t writtenBytes = 0;

   if ( i2cfd != NULL )
   {
#if BSPCFG_ENABLE_II2C0 == 0   /*  Polled version   */
      /* Send the "word address" byte  */
      writtenBytes = fwrite( &word_address, 1, 1, i2cfd );
      /* Make sure device acknowleges! */
      (void)ioctl( i2cfd, IO_IOCTL_I2C_GET_STATE, &i2cStatus );
      if ( i2cstatus == I2C_STATE_TRANSMIT )
      {
         /* Send the rest of the command  */
         writtenBytes = fwrite( localBuf, 1, Count, i2cfd );
      }

#else                         /* Interrupt version */
      /* Send the "word address" byte  */
#if ( MQX_VERSION == 420 )
      do
      {
         writtenBytes += (uint32_t)fwrite( &word_address, 1, 1, i2cfd );
         if ( writtenBytes == 0 )      /* Did the device NAK (sleeping)?   */
         {
            (void)ioctl( i2cfd, IO_IOCTL_I2C_GET_STATE, &i2cStatus );
         }
      }while ( ( i2cStatus < I2C_STATE_LOST_ARBITRATION ) && ( writtenBytes < 1 ) );
#else /* MQX 410  */
      do
      {
         writtenBytes += (uint32_t)fwrite( &word_address, 1, 1, i2cfd );
      }while (  writtenBytes < 1 );
#endif

      /* Make sure device acknowleges! */
      (void)ioctl( i2cfd, IO_IOCTL_I2C_GET_STATE, &i2cStatus );

#if ( MQX_VERSION == 420 )
      if ( Count != 0 )
#endif
      {
         /* Send the rest of the command  */
         if ( i2cStatus == I2C_STATE_TRANSMIT )
         {
            writtenBytes = 0;
            do
            {
               /*lint -e{413} if buffer is NULL, then Count is zero; OK to call fwrite */
               writtenBytes += (uint32_t)fwrite( buffer + writtenBytes, 1, (int32_t)(Count - writtenBytes), i2cfd );
            }while ( writtenBytes < Count );
         }
      }
#if ( MQX_VERSION == 420 )
      else  /* Count = 0 -> putting device in sleep, reset, or idle. May already be asleep.   */
      {
         i2cStatus = ECC108_SUCCESS;
      }
#endif
#endif
#if ( MQX_VERSION == 410 )
      if ( i2cStatus == I2C_STATE_TRANSMIT )
#else /* MQX 4.2  */
      if ( ( Count != 0 ) && ( i2cStatus == I2C_STATE_TRANSMIT ) )
#endif
      {
         (void)ecc108p_flush();        /* Wait for completion  */
         i2cStatus = i2c_send_stop();  /* End the transaction  */
      }
      else  /* Device NAK'd - it is sleeping; just exit  */
      {
         (void)i2c_send_stop();  /* Reset mqx i2c state machine   */
      }
   }
   return i2cStatus;
}
