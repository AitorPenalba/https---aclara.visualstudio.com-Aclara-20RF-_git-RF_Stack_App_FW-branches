
/*
 * \file
 *  \brief Definitions for Hardware Dependent Part of Crypto Device Physical
 *         Layer Using I2C for Communication
 *  \author Atmel Crypto Products
 *  \date January 14, 2013
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

#ifndef I2C_PHYS_H_
#   define I2C_PHYS_H_

#include <stdint.h>        // data type definitions
#include "project.h"
#include "partitions.h"

// error codes for physical hardware dependent module
// Codes in the range 0x00 to 0xF7 are shared between physical interfaces (SWI, TWI, SPI).
// Codes in the range 0xF8 to 0xFF are special for the particular interface.
#define I2C_FUNCTION_RETCODE_SUCCESS     ((uint8_t) 0x00) //!< Communication with device succeeded.
#define I2C_FUNCTION_RETCODE_COMM_FAIL   ((uint8_t) 0xF0) //!< Communication with device failed.
#define I2C_FUNCTION_RETCODE_TIMEOUT     ((uint8_t) 0xF1) //!< Communication timed out.
#define I2C_FUNCTION_RETCODE_NACK        ((uint8_t) 0xF8) //!< TWI nack

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

extern const char i2cName[];

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES  */

extern returnStatus_t   SEC_init(void);
extern PartitionData_t  const *SEC_GetSecPartHandle( void );
extern uint8_t          ecc108_open( void );
extern bool             ecc108_isOpen( void );
extern void             ecc108_close( void );
extern void             delay_10us( uint32_t delay );
extern void             delay_ms( uint32_t delay );
extern void             i2c_wakeup(void);
extern uint8_t          i2c_receive_response(uint8_t size, uint8_t *response);
extern uint8_t          ecc108p_wakeup();
extern uint8_t          ecc108p_flush( void );
extern uint8_t          i2c_send(uint8_t word_address, uint8_t count, uint8_t *buffer);

#endif
