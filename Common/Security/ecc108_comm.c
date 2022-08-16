/*
 * \file
 *  \brief  Communication Layer of ECC108 Library
 *  \author Atmel Crypto Products
 *  \date   October 21, 2013

 * \copyright Copyright (c) 2014 Atmel Corporation. All rights reserved.
 *
 * \atmel_crypto_device_library_license_start
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel integrated circuit.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \atmel_crypto_device_library_license_stop
 */
#include "project.h"
#if ( MCU_SELECTED == RA6E1 )
#include "hal_data.h"
#endif
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>                       // Need mqx types
#include <fio.h>                       // mqx file types
#endif
#include <CRC.h>                       // Aclara interface to Kinetis CRC engine
#include "ecc108_comm.h"               // definitions and declarations for the Communication module
#if ( RTOS_SELECTION == MQX_RTOS )
#include "ecc108_mqx.h"                // definitions for delay functions
#endif
#include "ecc108_lib_return_codes.h"   // declarations of function return codes
#if ( RTOS_SELECTION == FREE_RTOS )
#include "ecc108_freertos.h"
#endif
/* This is the time to wait between response polling (rounded up to 10us increments) */
#if (ECC108_RESPONSE_TIMEOUT > 71)
#   define ECC108_RESPONSE_POLL_DELAY   (uint8_t)(((ECC108_RESPONSE_TIMEOUT - 71) + 5) / 10)
#else
#   error Invalid ECC108_RESPONSE_TIMEOUT
#endif
#if ( RTOS_SELECTION == FREE_RTOS )
#include "ecc108_freertos.h"
#endif
/** \brief This function calculates CRC.
 *
 * \param[in] length number of bytes in buffer
 * \param[in] data pointer to data for which CRC should be calculated
 * \param[out] crc pointer to 16-bit CRC
 */
// TODO: RA6E1 - Set it to 0. For now, using local routine CRC as CRC engine does not implemented

#define VERIFY_CRC 1          /* Used to verify CRC generated by CRC engine matches that of local routine.  */
void ecc108c_calculate_crc( uint16_t length, uint8_t *data, uint8_t *crc )
{
#if VERIFY_CRC
   uint8_t counter;              /* Used to step through the bytes in data */
   uint16_t crc_register = 0;    /* Working CRC register/final CRC value   */
   uint16_t polynom = 0x8005;
   uint8_t shift_register;       /* Used to extract each bit in a byte     */
   uint8_t data_bit;             /* Set equal to the bit at shift register position */
   uint8_t crc_bit;
   uint16_t engine_crc;          /* Results of internal CRC engine computation   */

   for (counter = 0; counter < length; counter++)
   {
      for (shift_register = 1; shift_register != 0; shift_register <<= 1)
      {
         data_bit = (data[counter] & shift_register) ? 1 : 0;
         crc_bit = crc_register >> 15;
         crc_register <<= 1;
         if (data_bit != crc_bit)
         {
            crc_register ^= polynom;
         }
      }
#if 0
      printf( "CRC = 0x%04x\n", crc_register );
#endif
   }
   crc[0] = (uint8_t) (crc_register & 0x00FF);
   crc[1] = (uint8_t) (crc_register >> 8);
   CRC_ecc108_crc( length, data, (uint8_t *)&engine_crc, 0, (bool)false );
   assert ( crc_register == engine_crc );
#else
   /* Use CRC engine */
   CRC_ecc108_crc( (uint8_t)length, data, crc, 0, (bool)false );
#endif
}


/** \brief This function checks the consistency of a response.
 *  \ingroup atecc108_communication
 * \param[in] response pointer to response
 * \return status of the consistency check
 */
uint8_t ecc108c_check_crc(uint8_t *response)
{
   uint8_t crc[ECC108_CRC_SIZE];
   uint8_t count = response[ECC108_BUFFER_POS_COUNT]; /*lint !e578 local named same as enum OK  */

   count -= ECC108_CRC_SIZE;
   ecc108c_calculate_crc(count, response, crc);

   return (crc[0] == response[count] && crc[1] == response[count + 1])
      ? ECC108_SUCCESS : ECC108_BAD_CRC;
}

/** \brief This function wakes up a ECC108 device
 *         and receives a response.
 *  \param[out] response pointer to four-byte response
 *  \return status of the operation
 */
uint8_t ecc108c_wakeup(uint8_t *response)
{
   uint16_t crc;        /* Used to check the crc of the response  */
   uint8_t retry = 5;   /* Allow this many attempts to wake the device  */
   uint8_t ret_code;    /* Return value   */

   do
   {
      (void)ecc108p_wakeup();

      ret_code = ecc108p_receive_response(ECC108_RSP_SIZE_MIN, response);
      if (ret_code == ECC108_SUCCESS)
      {
         // Verify status response.
         if (response[ECC108_BUFFER_POS_COUNT] != ECC108_RSP_SIZE_MIN)
         {
            ret_code = ECC108_INVALID_SIZE;
         }
         /* Check for normal wakeup response (0x11 returned if was sleeping), or OK (0, returned if already awake)   */
         else if ((response[ECC108_BUFFER_POS_STATUS] != ECC108_STATUS_BYTE_WAKEUP) &&
                  (response[ECC108_BUFFER_POS_STATUS] != ECC108_SUCCESS))
         {
            ret_code = ECC108_COMM_FAIL;
         }
         else
         {
            /* Rather than check for specific crc, validate it.   */
            ecc108c_calculate_crc( ECC108_CRC_SIZE, response, (uint8_t *)&crc );
            if ( memcmp( (uint8_t *)&crc, (uint8_t *)&response[ ECC108_RSP_SIZE_MIN - ECC108_CRC_SIZE ],
                        sizeof( crc ) ) != 0 )
            {
               ret_code = ECC108_BAD_CRC;
            }
         }
      }
#if 0
      if (ret_code != ECC108_SUCCESS)
      {
         delay_ms(ECC108_COMMAND_EXEC_MAX);
      }
#endif
   } while ( ( ret_code != ECC108_SUCCESS ) && ( --retry != 0 ) );
   return ret_code;
}


/** \brief This function re-synchronizes communication.
 * \ingroup atecc108_communication
 *
   Be aware that succeeding only after waking up the
   device could mean that it had gone to sleep and lost
   its TempKey in the process.\n
   Re-synchronizing communication is done in a maximum of
   three steps:
   <ol>
      <li>
         Try to re-synchronize without sending a Wake token.
         This step is implemented in the Physical layer.
      </li>
      <li>
         If the first step did not succeed send a Wake token.
      </li>
      <li>
         Try to read the Wake response.
      </li>
   </ol>
 *
 * \param[in] size size of response buffer
 * \param[out] response pointer to Wake-up response buffer
 * \return status of the operation
 */
/*lint -esym( 715, size)   */
uint8_t ecc108c_resync(uint8_t size, uint8_t *response)
{
uint8_t ret_code;
#if 0 /* mkv - this method can't work with mqx. Can't send arbitrary byte without
         first sending device address - normal i2c protocol
      */
   // Try to re-synchronize without sending a Wake token (step 1 of the re-synchronization process).
   ret_code = ecc108p_resync(size, response);
   if (ret_code == ECC108_SUCCESS)
   {
      return ret_code;
   }

   /* We lost communication. Send a Wake pulse and try to receive a response (steps 2 and 3 of the re-synchronization
      process).   */
   (void) ecc108p_sleep();
#endif
   ret_code = ecc108c_wakeup(response);

   /* Translate a return value of success into one that indicates that the device had to be woken up and might have lost
      its TempKey.   */
   return (ret_code == ECC108_SUCCESS ? ECC108_RESYNC_WITH_WAKEUP : ret_code);
}
/*lint +esym( 715, size)   */

/** \brief This function runs a communication sequence.
 *
 * Append CRC to tx buffer, send command, delay, and verify response after receiving it.
 *
 * The first byte in tx buffer must be the byte count of the packet.
 * If CRC or count of the response is incorrect, or a command byte did not get acknowledged
 * this function requests re-sending the response.
 * If the response contains an error status, this function resends the command.
 *
 * \param[in] tx_buffer pointer to command
 * \param[in] rx_size size of response buffer
 * \param[out] rx_buffer pointer to response buffer
 * \param[in] execution_delay Start polling for a response after this many ms.
 * \param[in] execution_timeout polling timeout in ms
 * \return status of the operation
 */
uint8_t ecc108c_send_and_receive(uint8_t *tx_buffer, uint8_t rx_size, uint8_t *rx_buffer,
         uint8_t execution_delay, uint8_t execution_timeout)
{
   uint8_t ret_code = ECC108_FUNC_FAIL;
   uint8_t ret_code_resync;
   uint8_t n_retries_send;
   uint8_t n_retries_receive;
   uint8_t i;
   uint8_t status_byte;
   uint8_t count = tx_buffer[ECC108_BUFFER_POS_COUNT];   /*lint !e578 local named same as enum OK  */
   uint8_t count_minus_crc = ( uint8_t )( count - ECC108_CRC_SIZE );
   uint32_t execution_timeout_us = ((uint32_t) execution_timeout * 1000) + ECC108_RESPONSE_TIMEOUT;
   volatile uint32_t timeout_countdown;

   ecc108c_calculate_crc(count_minus_crc, tx_buffer, tx_buffer + count_minus_crc);  // Append CRC.
   n_retries_send = ECC108_RETRY_COUNT + 1;  // Retry loop for sending a command and receiving a response.

   while ((n_retries_send-- > 0) && (ret_code != ECC108_SUCCESS))
   {
      ret_code = ecc108p_send_command(count, tx_buffer); // Send command.
      if (ret_code != ECC108_SUCCESS)
      {
         if (ecc108c_resync(rx_size, rx_buffer) == ECC108_RX_NO_RESPONSE)
         {
            return ret_code;  // The device seems to be dead in the water.
         }
         else
         {
            continue;
         }
      }

      // TODO: RA6E1 - Handle with delay_10us API once it is in place
#if ( ( MCU_SELECTED == NXP_K24 ) && ( RTOS_SELECTION == MQX_RTOS ) )
      delay_10us(execution_delay * 100);  // Wait minimum command execution time and then start polling for a response.
#elif ( ( MCU_SELECTED == RA6E1 ) && ( RTOS_SELECTION == FREE_RTOS ) )
      delay_10us(execution_delay * 100);  // Wait minimum command execution time and then start polling for a response.
#endif
      // Retry loop for receiving a response.
      n_retries_receive = ECC108_RETRY_COUNT + 1;
      while (n_retries_receive-- > 0)
      {
         // Reset response buffer.
         for (i = 0; i < rx_size; i++)
         {
            rx_buffer[i] = 0;
         }

         // Poll for response.
         timeout_countdown = execution_timeout_us;
         do
         {
            ret_code = ecc108p_receive_response(rx_size, rx_buffer);
            // TODO: RA6E1 - Handle with delay_10us API once it is in place
#if ( ( MCU_SELECTED == NXP_K24 ) && ( RTOS_SELECTION == MQX_RTOS ) )
            delay_10us(ECC108_RESPONSE_POLL_DELAY);  // Wait between polling for a response.
#elif ( ( MCU_SELECTED == RA6E1 ) && ( RTOS_SELECTION == FREE_RTOS ) )
            delay_10us(ECC108_RESPONSE_POLL_DELAY);  // Wait between polling for a response.
#endif
            timeout_countdown -= ECC108_RESPONSE_TIMEOUT;
         } while ((timeout_countdown > ECC108_RESPONSE_TIMEOUT) && (ret_code == ECC108_RX_NO_RESPONSE));

         if (ret_code == ECC108_RX_NO_RESPONSE)
         {
            // We did not receive a response. Re-synchronize and send command again.
            if (ecc108c_resync(rx_size, rx_buffer) == ECC108_RX_NO_RESPONSE)
            {
               return ret_code;  // The device seems to be dead in the water.
            }
            else
            {
               break;
            }
         }

         // Check whether we received a valid response.
         if (ret_code == ECC108_INVALID_SIZE)
         {
            // We see 0xFF for the count when communication got out of sync.
           ret_code_resync = ecc108c_resync(rx_size, rx_buffer);
           if (ret_code_resync == ECC108_SUCCESS)
            {
               continue;   // We did not have to wake up the device. Try receiving response again.
            }
            if (ret_code_resync == ECC108_RESYNC_WITH_WAKEUP)
            {
               break;   // We could re-synchronize, but only after waking up the device. Re-send command.
            }
            else
            {
               return ret_code;  // We failed to re-synchronize.
            }
         }

         // We received a response of valid size.  Check the consistency of the response.
         ret_code = ecc108c_check_crc(rx_buffer);
         if (ret_code == ECC108_SUCCESS)
         {
            // Received valid response.
            if (rx_buffer[ECC108_BUFFER_POS_COUNT] > ECC108_RSP_SIZE_MIN)
            {
               return ret_code;  // Received non-status response. We are done.
            }

            // Received status response.
            status_byte = rx_buffer[ECC108_BUFFER_POS_STATUS];

            // Translate the four possible device status error codes into library return codes.
            if (status_byte == ECC108_STATUS_BYTE_PARSE)
            {
               return ECC108_PARSE_ERROR;
            }
            if (status_byte == ECC108_STATUS_BYTE_EXEC)
            {
               return ECC108_CMD_FAIL;
            }
            if (status_byte == ECC108_STATUS_BYTE_COMM)
            {
               /* In case of the device status byte indicating a communication error this function exits the retry
                  loop for receiving a response and enters the overall retry loop (send command / receive response). */
                  ret_code = ECC108_STATUS_CRC;
               break;
            }
            if (status_byte == ECC108_STATUS_BYTE_ECC)
            {
               /* In case of the device status byte indicating an ECC fault this function exits the retry loop for
                  receiving a response and enters the overall retry loop (send command / receive response).  */
               ret_code = ECC108_STATUS_ECC;
               break;
            }
            /* Received status response from CheckMAC, DeriveKey, GenDig, Lock, Nonce, Pause, UpdateExtra, Verify, or
               Write command. */
            return ret_code;
         }
         else
         {
            // Received response with incorrect CRC.
            ret_code_resync = ecc108c_resync(rx_size, rx_buffer);
            if (ret_code_resync == ECC108_SUCCESS)
            {
               continue;   // We did not have to wake up the device. Try receiving response again.
            }
            if (ret_code_resync == ECC108_RESYNC_WITH_WAKEUP)
            {
               break;   // We could re-synchronize, but only after waking up the device. Re-send command.
            }
            else
            {
               return ret_code;  // We failed to re-synchronize.
            }
         } // block end of check response consistency
      } // block end of receive retry loop
   } // block end of send and receive retry loop

   return ret_code;
}
