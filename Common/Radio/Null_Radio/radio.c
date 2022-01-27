/*! @file radio.c
 * @brief This file contains functions to interface with the radio chip.
 *
 * @b COPYRIGHT
 * @n Silicon Laboratories Confidential
 * @n Copyright 2012 Silicon Laboratories, Inc.
 * @n http://www.silabs.com
 */


#include <stdint.h>
#include <stdbool.h>
#include "DBG_SerialDebug.h"

// todo - This is in DBG_CommandLine.h
extern returnStatus_t atoh(uint8_t *pHex, char const *pAscii);

#include "..\radio.h"      // lint does not like this!!!

/*****************************************************************************
 *  Local Macros & Definitions
 *****************************************************************************/

/*****************************************************************************
 *  Global Variables
 *****************************************************************************/

/*****************************************************************************
 *  Local Function Declarations
 *****************************************************************************/

/*!
 * Initialize the radio
 */
static bool Init(void)
{
   INFO_printf("Radio.Init()");
   return true;   // SUCCESS
}

/*!
 * Prepare the radio with a packet to be sent.
 */
static bool PrepareData(const void *payload, uint8_t payload_len)
{
   INFO_printf("Radio.PrepareData:");
   (void) payload;
   (void) payload_len;
   return true;   // SUCCESS
}

/*!
 * Send the packet that has previously been prepared.
 */
static bool Transmit(uint8_t transmit_len)
{
   INFO_printf("Radio.Transmit:");
   (void) transmit_len;
   return true;   // Success
}


/*!
 * Transmit data - Prepare & transmit a packet.
 */
static int rx_data = false;

static bool SendData(const void *payload, uint8_t payload_len)
{
   INFO_printf("Radio.SendData:");
   INFO_printHex ( payload, payload_len);

   // set a flag so we will now received 1 frame
   rx_data = true;

   return true;   // Success


}

/*!
 * Read a received packet into a buffer.
 */

char dummy_message[] = "060F7777777777987654321000200C80989898989811FFFFFFFFFFFFFFFF0123456789ABCDEFFFFFFFFFFFFFFFFFE8E77D72";
//char   dummy_message[] = "060F7777777777987654321000200C80989898989811FFFFFFFFFFFFFFFF0123456789ABCDEFFFFFFFFFFFFFFFFF5A558D99 ";




static uint16_t ReadData(void *buf, uint16_t buf_len)
{
   INFO_printf("Radio.ReadData()");
   (void) buf;
   (void) buf_len;

   // convert asci to hex
   int len = strlen(dummy_message);
   uint8_t* pDst = buf;
   int i;
   for (i = 0; i < len;i+=2)
   {
      (void)atoh(pDst, &dummy_message[i]);
      pDst++;
   }
   INFO_printHex(buf, len/2);
   rx_data = false;
   return len/2;
}

/*!
 *  Perform a Clear-Channel Assessment (CCA) to find out if there is
 *  a packet in the air or not.
 */
static bool Is_ChannelClear(uint8_t chan)
{
  (void) chan;
   INFO_printf("Radio.Is_ChannelClear(chan=%d) true", chan);
   return true;
}

/*!
 * Check if the radio driver is currently receiving a packet
 */
static bool Is_Receiving(void)
{
   INFO_printf("Radio.Is_Receiving()");
   return false;
}


/*!
 * Check if the radio driver has just received a packet
 */
static bool Is_DataPending(void)
{
// INFO_printf("Radio.Is_DataPending()");
   return rx_data;
//   return false;
}

/*!
 * Turn Radio On
 */
static bool On(void)
{
   INFO_printf("Radio.On()");
   return true;
}

/*!
 * Turn the radio off.
 */
static bool Off(void)
{
   INFO_printf("Radio.Off()");
   return true;
}

/*!
 * Set radio to standby mode
 */
static bool Standby(void)
{
   INFO_printf("Radio.Standby()");
   return true;
}


/*!
 * Get a character from the RX Buffer
 */
int8_t GetChar(void)
{
   return(-1);
}



/*!
 * Radio Driver
 */
struct radio_driver Radio =
{
  .Init               = Init,
  .PrepareData        = PrepareData,
  .Transmit           = Transmit,
  .SendData           = SendData,
  .ReadData           = ReadData,
  .Is_ChannelClear    = Is_ChannelClear,
  .Is_Receiving       = Is_Receiving,
  .Is_DataPending     = Is_DataPending,
  .On                 = On,
  .Off                = Off,
  .Standby            = Standby,
  .GetChar            = GetChar
};








