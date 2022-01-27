/** ****************************************************************************
@file hdlc_frame.c
 
Implements HDLC framing and decoding.
 
A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.

Portions Copyright (c) 2006 Pieter Conradie
*******************************************************************************/

/* =============================================================================

    Copyright (c) 2006 Pieter Conradie <http://piconomic.co.za>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.

    Title:          hdlc.h : HDLC encapsulation layer
    Author(s):      Pieter Conradie
    Creation Date:  2007-03-31

============================================================================= */

/*******************************************************************************
#INCLUDES
*******************************************************************************/

#include "DBG_SerialDebug.h"
#include "OS_aclara.h"

#include "hdlc_frame.h"

/*******************************************************************************
Local #defines (Object-like macros)
*******************************************************************************/

#define CRC_POLYNOMIAL 0x8408 /*!< The generator polynomial CRC16-CCITT: x^16 + x^12 + x^5 + x^0 */
#define CRC_INIT_VAL 0xFFFF /*!< Initial CRC value */
#define CRC_INVERT_VAL 0xFFFF
#define CRC_MAGIC_VAL 0xF0B8 /*!< Magic CRC value */

/// Specifies the start/stop of a HDLC frame
#define HDLC_FLAG_SEQUENCE 0x7E

/// Asynchronous Control Escape
#define HDLC_CONTROL_ESCAPE 0x7D

/// Asynchronous transparency modifier
#define HDLC_ESCAPE_BIT 0x20

#define HDLC_SEQ_MASK 0x07
#define HDLC_UCMD_MASK 0x3B
#define HDLC_SCMD_MASK 0x03

#define HDLC_SEND_SEQ_OFFSET 1
#define HDLC_REC_SEQ_OFFSET 5
#define HDLC_PF_BIT_OFFSET 4
#define HDLC_SCMD_OFFSET 2
#define HDLC_UCMD_OFFSET 2

#define I_FRAME_MASK 0x01
#define SU_FRAME_MASK 0x03

#define HDLC_ADDR_SZ sizeof(hdlc_addr_t)
#define HDLC_CTRL_SZ sizeof(hdlc_ctrl_t)
#define HDLC_FCS_SZ sizeof(hdlc_fcs_t)
#define HDLC_INFO_OFFSET (HDLC_ADDR_SZ + HDLC_CTRL_SZ)

/*******************************************************************************
Local #defines (Function-like macros)
*******************************************************************************/

/*******************************************************************************
Local Struct, Typedef, and Enum Definitions (Private to this file)
*******************************************************************************/

typedef uint16_t hdlc_fcs_t;

/*******************************************************************************
Static Function Prototypes
*******************************************************************************/

static void HdlcTxByte(uint8_t data);
static void HdlcTxByteWithEsc(uint8_t data);
static hdlc_fcs_t HdlcUpdateCrc(hdlc_fcs_t crc, uint8_t data);
static void HdlcResetRxDecoding(void);

/*******************************************************************************
Global Variable Definitions
*******************************************************************************/

/*******************************************************************************
Static Variable Definitions
*******************************************************************************/

static uint8_t rxFrame[HDLC_MRU];
static uint16_t rxFrameIndex;
static hdlc_fcs_t rxFrameFcs;
static bool rxFrameEscFlag;
static bool rxOverflow;

/// Pointer to the function that will be called to send a character
static hdlc_tx_fn_t hdlcTxFn;

/// Pointer to the function that will be called to process a received HDLC frame
static hdlc_on_rx_frame_fn_t hdlcOnRxFrameFn;

/// Makes sure multiple transmit requests do not step on each other
static OS_MUTEX_Obj txFrameMutex;

/*******************************************************************************
Function Definitions
*******************************************************************************/

/**
Initialize HDLC encapsulation layer.

@param txFn Pointer to a function that will be called to send a byte.
@param onRxFrameFn Pointer to function that is called when a valid frame is received.
*/
void HdlcFrameInit(hdlc_tx_fn_t txFn, hdlc_on_rx_frame_fn_t onRxFrameFn)
{
   HdlcResetRxDecoding();
   hdlcTxFn = txFn;
   hdlcOnRxFrameFn = onRxFrameFn;

   if (!OS_MUTEX_Create(&txFrameMutex))
   {
      ERR_printf("Error creating HDLC TX mutex");
   }
}

/**
Takes received bytes and parses out any valid frames.

@param data The byte received
*/
void HdlcFrameOnRxByte(uint8_t data)
{
   // 1st check for frame delimiter
   if (data == HDLC_FLAG_SEQUENCE)
   {
      if (rxFrameEscFlag)
      {
         // if currently in escape sequence then escape byte is missing and this frame is invalid
      }
      else if (rxFrameIndex >= (HDLC_ADDR_SZ + HDLC_CTRL_SZ + HDLC_FCS_SZ)) // need address, control, and FCS to be valid
      {
         // The CRC should match the magic value
         if (rxFrameFcs == CRC_MAGIC_VAL)
         {
            // Good frame so send data for processing
            hdlc_addr_t address = 0;
            hdlc_ctrl_t control = 0;
            uint16_t offset = 0;

            for (uint8_t i = 0; i < HDLC_ADDR_SZ; i++)
            {
               address |= rxFrame[offset] << (8 * i); /*lint !e701 !e734 */
               offset++;
            }

            for (uint8_t i = 0; i < HDLC_CTRL_SZ; i++)
            {
               control |= rxFrame[offset] << (8 * i); /*lint !e701 !e734 */
               offset++;
            }

            INFO_printf("HDLC Got frame addr %d ctrl 0x%X data len %d", address, control, rxFrameIndex - (HDLC_ADDR_SZ + HDLC_CTRL_SZ + HDLC_FCS_SZ));

            (*hdlcOnRxFrameFn)(address, control, &rxFrame[HDLC_INFO_OFFSET], rxFrameIndex - (HDLC_ADDR_SZ + HDLC_CTRL_SZ + HDLC_FCS_SZ), rxOverflow);
         }
         else
         {
            INFO_printHex("HDLC frame CRC Error ", rxFrame, rxFrameIndex);
         }
      }

      // Reset for next frame
      HdlcResetRxDecoding();

      return;
   }

   // Escape sequence processing
   if (rxFrameEscFlag)
   {
      rxFrameEscFlag = false;

      // Toggle escape bit to restore data to correct state
      data ^= HDLC_ESCAPE_BIT;
   }
   else if (data == HDLC_CONTROL_ESCAPE)
   {
      // Set flag to indicate that the next byte's escape bit must be toggled
      rxFrameEscFlag = true;

      // Discard control escape byte (do not buffer it)
      return;
   }

   rxFrameFcs = HdlcUpdateCrc(rxFrameFcs, data);

   // Check for buffer overflow
   if (rxFrameIndex >= HDLC_MRU)
   {
      /**
      Note keep tracking frame to see if the FCS is valid as this could require
      a FRMR response due to not being able to store all the frame data.
      */
      rxOverflow = true;

      return;
   }

   // store the data byte
   rxFrame[rxFrameIndex] = data;
   rxFrameIndex++;
}

/**
Encapsulate and send a HDLC frame.

@param frameInfo Structure with all of the HDLC frame data.
*/
void HdlcFrameTx(const HdlcFrameData_t *frameInfo)
{
   uint8_t byteToSend, i;
   hdlc_fcs_t fcs = CRC_INIT_VAL;
   hdlc_addr_t address = frameInfo->addr;
   hdlc_ctrl_t control = frameInfo->control;
   const uint8_t *data = frameInfo->packet;
   uint16_t nr_of_bytes = frameInfo->packetLen;

   // make sure entire frame is sent before another can start
   OS_MUTEX_Lock(&txFrameMutex);

   INFO_printf("HDLC send frame addr %d ctrl 0x%X data len %d", address, control, nr_of_bytes);

   // Send start marker
   HdlcTxByte(HDLC_FLAG_SEQUENCE);

   // Send address
   for (i = 0; i < HDLC_ADDR_SZ; i++)
   {
      byteToSend = (uint8_t)(address >> (i * 8));
      fcs = HdlcUpdateCrc(fcs, byteToSend);
      HdlcTxByteWithEsc(byteToSend);
   }

   // Send control
   for (i = 0; i < HDLC_CTRL_SZ; i++)
   {
      byteToSend = (uint8_t)(control >> (i * 8));
      fcs = HdlcUpdateCrc(fcs, byteToSend);
      HdlcTxByteWithEsc(byteToSend);
   }

   // Send data
   while (nr_of_bytes)
   {
      byteToSend = *data++;

      fcs = HdlcUpdateCrc(fcs, byteToSend);
      HdlcTxByteWithEsc(byteToSend);

      nr_of_bytes--;
   }

   // Invert CRC
   fcs ^= CRC_INVERT_VAL;

   for (i = 0; i < HDLC_FCS_SZ; i++)
   {
      byteToSend = (uint8_t)(fcs >> (i * 8));
      HdlcTxByteWithEsc(byteToSend);
   }

   // Send end marker
   HdlcTxByte(HDLC_FLAG_SEQUENCE);

   OS_MUTEX_Unlock(&txFrameMutex);
}

/**
Creates the control value based on the passed parameters.

@param info Structure with parameters to use for the control byte generation.

@return Control value to use for the HDLC frame.
*/
hdlc_ctrl_t HdlcFrameCreateControlValue(hdlc_control_info_t info)
{
   hdlc_ctrl_t control = (uint8_t)info.type;

   switch (info.type)
   {
      case HDLC_FRAME_I:
         control |= (info.sendSequence & HDLC_SEQ_MASK) << HDLC_SEND_SEQ_OFFSET;

         if (info.pfBit)
         {
            control |= (1 << HDLC_PF_BIT_OFFSET);
         }

         control |= (info.recSequence & HDLC_SEQ_MASK) << HDLC_REC_SEQ_OFFSET;
         break;
      case HDLC_FRAME_S:
         control |= (uint8_t)info.sCommand << HDLC_SCMD_OFFSET; /*lint !e734 */

         if (info.pfBit)
         {
            control |= (1 << HDLC_PF_BIT_OFFSET);
         }

         control |= (info.recSequence & HDLC_SEQ_MASK) << HDLC_REC_SEQ_OFFSET;
         break;
      case HDLC_FRAME_U:
         control |= (uint8_t)info.uCommand << HDLC_UCMD_OFFSET; /*lint !e734 */

         if (info.pfBit)
         {
            control |= (1 << HDLC_PF_BIT_OFFSET);
         }

         break;
   }

   return control;
}

/**
Parses out the control information from the raw control values.

@param value The raw control data from the HDLC frame.

@return Structure with parsed control information from the HDLC frame.
*/
hdlc_control_info_t HdlcFrameGetControlInfo(hdlc_ctrl_t value)
{
   hdlc_control_info_t info;

   (void)memset(&info, 0, sizeof(info));

   if ((value & I_FRAME_MASK) != 0)
   {
      info.type = (hdlc_frame_t)(value & SU_FRAME_MASK);
   }

   switch (info.type)
   {
      case HDLC_FRAME_I:
         info.sendSequence = (value >> HDLC_SEND_SEQ_OFFSET) & HDLC_SEQ_MASK;
         info.pfBit = (value & (1 << HDLC_PF_BIT_OFFSET)) != 0;
         info.recSequence = (value >> HDLC_REC_SEQ_OFFSET) & HDLC_SEQ_MASK;
         break;
      case HDLC_FRAME_S:
         info.sCommand = (hdlc_scommand_t)((value >> HDLC_SCMD_OFFSET) & HDLC_SCMD_MASK);
         info.pfBit = (value & (1 << HDLC_PF_BIT_OFFSET)) != 0;
         info.recSequence = (value >> HDLC_REC_SEQ_OFFSET) & HDLC_SEQ_MASK;
         break;
      case HDLC_FRAME_U:
         info.uCommand = (hdlc_ucommand_t)((value >> HDLC_UCMD_OFFSET) & HDLC_UCMD_MASK);
         info.pfBit = (value & (1 << HDLC_PF_BIT_OFFSET)) != 0;
         break;
   }

   return info;
}

/**
Transmits a byte of data.

@param data Byte to transmit.
*/
static void HdlcTxByte(uint8_t data)
{
   // Call provided function
   (*hdlcTxFn)(data);
}

/**
Transmits a byte of data and adds escape sequence if required.

@param data Byte to transmit.
*/
static void HdlcTxByteWithEsc(uint8_t data)
{
   if ((data == HDLC_CONTROL_ESCAPE) || (data == HDLC_FLAG_SEQUENCE))
   {
      // Send control escape byte
      HdlcTxByte(HDLC_CONTROL_ESCAPE);

      // Toggle escape bit
      data ^= HDLC_ESCAPE_BIT;
   }

   // Send data
   HdlcTxByte(data);
}

/**
Resets frame decoding tracking variables.
*/
static void HdlcResetRxDecoding(void)
{
   rxFrameIndex = 0;
   rxFrameFcs = CRC_INIT_VAL;
   rxFrameEscFlag = false;
   rxOverflow = false;
}

/**
Computes a new CRC value given a starting value and data byte.

@param crc The current CRC value.
@param data New data byte.

@return the new CRC value
*/
static hdlc_fcs_t HdlcUpdateCrc(hdlc_fcs_t crc, uint8_t data)
{
   crc ^= data;

   for (uint8_t i = 8; i != 0; i--)
   {
      if (crc & 1)
      {
         crc = (crc >> 1) ^ CRC_POLYNOMIAL;
      }
      else
      {
         crc = (crc >> 1);
      }
   }

   return crc;
}
