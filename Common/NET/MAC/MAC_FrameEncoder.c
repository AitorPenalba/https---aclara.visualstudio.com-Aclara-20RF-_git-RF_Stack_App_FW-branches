/***********************************************************************************************************************
 *
 * Filename:   MAC_FrameEncoder.c (MAC_Codec.c)
 *
 * Global Designator: MAC_Codec
 *
 * Contents: This file contains frame encode and decode functionality for MAC frames
 *
 ***********************************************************************************************************************
 * A product of Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
/* INCLUDE FILES */
#include "project.h"
#include <stdlib.h>       // rand()
#include <stdbool.h>
#include <stdio.h>      // sprintf
#include <string.h>
#include <limits.h> /* only for CHAR_BIT */
#include "buffer.h"
#include "MAC_Protocol.h"
#include "pack.h"
#include "MAC.h"
#include "DBG_SerialDebug.h"
#include "MAC_FrameEncoder.h"
#include "time_util.h"
#include "time_sync.h"

// Frame Control Field Sizes
/*lint -esym(750,MAC_VERSION_SIZE,FRAME_TYPE_SIZE,FRAME_DST_ADDR_MODE_SIZE,ACK_REQUIRED_SIZE,ACK_PRESENT_SIZE)   */
/*lint -esym(750,SEGMENTATION_ENABLED_SIZE,PACKET_ID_SIZE,NETWORK_ID_SIZE,SOURCE_ADDRESS_SIZE,DESTINATION_ADDRESS_SIZE)   */
/*lint -esym(750,SEQ_NUM_SIZE,REQ_NUM_SIZE,SEGMENT_ID_SIZE,SEGMENT_COUNT_SIZE,LENGTH_SIZE,FCS_SIZE,MIN_MAC_MSG_SIZE)   */
#define MAC_VERSION_SIZE              4    /* Size of the Version field */
#define FRAME_TYPE_SIZE               2    /* Size of the Frame Type field */
#define FRAME_DST_ADDR_MODE_SIZE      1    /* Size of the Destination Address Mode field */
#define ACK_REQUIRED_SIZE             1    /* Size of the ACK Required field */
#define ACK_PRESENT_SIZE              1    /* Size of the ACK Present field */
#define SEGMENTATION_ENABLED_SIZE     1    /* Size of the Segmentation Enabled field */
#define PACKET_ID_SIZE                2    /* Size of the packet ID field */
#define NETWORK_ID_SIZE               4    /* Size of the Network Identifier field */
#define SOURCE_ADDRESS_SIZE           40   /* Size of the Source Address field */
#define DESTINATION_ADDRESS_SIZE      40   /* Size of the Destination Address field */
#define SEQ_NUM_SIZE                  4    /* Size of the seguence number field */
#define REQ_NUM_SIZE                  4    /* Size of the request number field */
#define SEGMENT_ID_SIZE               4    /* Size of the fragment number field */
#define SEGMENT_COUNT_SIZE            4    /* Size of the segment count field */
#define LENGTH_SIZE                   16   /* Size of the packet length field */
#define FCS_SIZE                      32   /* Size of the fcs (crc32) field */

#define TIME_SYNC_FUTURE     36000000ULL   /* Have the time sync 36ms in the future. The radio takes about 29 ms for the preamble and SYNC and house keeping plus about 3ms from MAC to when PHY burns time. So about 31ms */
#define TIME_FUTURE          69000000ULL   /* Have time sent 69ms in the future */

/* #DEFINE DEFINITIONS */
#define MIN_MAC_MSG_SIZE (MAC_VERSION_SIZE + FCS_SIZE)

#define MIN_MAC_V0_MSG_SIZE (MAC_VERSION_SIZE + FRAME_TYPE_SIZE + FRAME_DST_ADDR_MODE_SIZE + ACK_REQUIRED_SIZE + \
                             ACK_PRESENT_SIZE + SEGMENTATION_ENABLED_SIZE + PACKET_ID_SIZE + NETWORK_ID_SIZE + \
                             SOURCE_ADDRESS_SIZE + FCS_SIZE)

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

static uint16_t data_frame_encode( mac_frame_t const *p, uint8_t       *pDst, uint16_t bitNo );

static void print_header( const char* msg, const mac_frame_t *p );

/* FUNCTION DEFINITIONS */

/******************************************************************************

  Function Name: encode_frame

  Purpose: This function converts MAC_Frame_t to a bit packed MAC Frame

  Arguments: p      - Pointer to the MAC Frame structure
             pDst   - Pointer to the data buffer (to be populated by this function)
             RxTime - Future time that we want the receiver to get this message

  Returns: bitNo - Bit offset of the end of the packed data

  Notes:
******************************************************************************/
uint16_t MAC_Codec_Encode ( const mac_frame_t *p, uint8_t *pDst, uint32_t *RxTime )
{
   uint16_t bitNo = 0;
   uint32_t crc_value;
   uint16_t i;
   uint32_t time;
   uint32_t cpuFreq;

   *RxTime = 0;

   /* Encode the Frame Control Field */
   bitNo = PACK_uint8_2bits(&p->frame_version,        MAC_VERSION_SIZE        , pDst, bitNo);
   bitNo = PACK_uint8_2bits(&p->frame_type,           FRAME_TYPE_SIZE         , pDst, bitNo);
   bitNo = PACK_uint8_2bits(&p->dst_addr_mode,        FRAME_DST_ADDR_MODE_SIZE, pDst, bitNo);
   bitNo = PACK_uint8_2bits(&p->ack_required,         ACK_REQUIRED_SIZE       , pDst, bitNo);
   bitNo = PACK_uint8_2bits(&p->ack_present,          ACK_PRESENT_SIZE        , pDst, bitNo);
   bitNo = PACK_uint8_2bits(&p->segmentation_enabled, SEGMENTATION_ENABLED_SIZE,pDst, bitNo);
   bitNo = PACK_uint8_2bits(&p->packet_id,            PACKET_ID_SIZE          , pDst, bitNo);
   bitNo = PACK_uint8_2bits(&p->network_id,           NETWORK_ID_SIZE         , pDst, bitNo);

   // Source Address
   for(i = 0; i < MAC_ADDRESS_SIZE; i++)
   {
      bitNo = PACK_uint8_2bits(&p->src_addr[i], 8, pDst, bitNo);
   }

   // Destination address
   if ( UNICAST_MODE == p->dst_addr_mode)
   {
      for(i = 0; i < MAC_ADDRESS_SIZE; i++)
      {
         bitNo = PACK_uint8_2bits(&p->dst_addr[i], 8, pDst, bitNo);
      }
   }

   if(p->ack_present == ACK_PRESENT)
   {
      bitNo = PACK_uint8_2bits(&p->seq_num, SEQ_NUM_SIZE, pDst, bitNo);
      bitNo = PACK_uint8_2bits(&p->req_num, REQ_NUM_SIZE, pDst, bitNo);
   }

   if (p->segmentation_enabled == FRAME_NOT_SEGMENTATED)
   {
      bitNo = PACK_uint16_2bits(&p->length, LENGTH_SIZE, pDst, bitNo);
   }
   else
   {
      bitNo = PACK_uint8_2bits(&p->segment_id, SEGMENT_ID_SIZE, pDst, bitNo);
      bitNo = PACK_uint8_2bits(&p->segment_count, SEGMENT_COUNT_SIZE, pDst, bitNo);
      if (p->segment_id == 0)
      {
         /* First segment includes the overall packet length */
         bitNo = PACK_uint16_2bits(&p->length, LENGTH_SIZE, pDst, bitNo);
      }
   }
   switch ( p->frame_type )
   {
      case MAC_DATA_FRAME:
         bitNo = data_frame_encode( p, pDst, bitNo );
         break;

      case MAC_ACK_FRAME:    // ACK Frame
         bitNo = data_frame_encode( p, pDst, bitNo );
         break;

      case MAC_CMD_FRAME:
      {
         // TIME_SET, PING_REQ and PING RSP Need updated time
         // NOTE: MAC_CMD_FRAME message is built twice.
         //       The first time is the "normal" call.
         //       The second time is after CCA just before PHY_DataRequest to refresh the time to make sure time is not stale.
         if((p->data[0] == MAC_PING_REQ_CMD) ||
            (p->data[0] == MAC_PING_RSP_CMD) ||
            (p->data[0] == MAC_TIME_SET_CMD) )
         {
            uint8_t* pData;

            TIMESTAMP_t DateTime;
            uint64_t    fracTime;

            uint32_t primask = __get_PRIMASK();
            __disable_interrupt(); // Disable all interrupts. This section is time critical but fast fortunatly.

            time = DWT_CYCCNT; // Get time in CPU cycles
            fracTime = TIME_UTIL_GetTimeInQSecFracFormat();
            __set_PRIMASK(primask); // Restore interrupts

            if ( fracTime )
            {  // Time is valid
               (void)TIME_SYS_GetRealCpuFreq( &cpuFreq, NULL, NULL );
               if ( p->data[0] == MAC_TIME_SET_CMD ) {
                  // Transmit time sync 36ms from now
                  fracTime += TIME_UTIL_ConvertNsecToQSecFracFormat( TIME_SYNC_FUTURE );
                  *RxTime = time + (uint32_t)(TIME_SYNC_FUTURE * (uint64_t)cpuFreq / 1000000000ULL);
               } else {
               /* ------------------------------------------------------------------------------------
                  Add the processing delay to the value transmitted.
                  ------------------------------------------------------------------------------------ */
               /* 69 msec is the amount of time measured from when the time was read until the
                  tx done event from the radio
                  Need to check with mario if this is when the radio last byte cleared the radio.
                  ------------------------------------------------------------------------------------ */
                  fracTime += TIME_UTIL_ConvertNsecToQSecFracFormat( TIME_FUTURE );
                  *RxTime = time + (uint32_t)(TIME_FUTURE * (uint64_t)cpuFreq / 1000000000ULL);
               }

               DateTime.QSecFrac = fracTime ;
               TIME_UTIL_PrintQSecFracFormat("set_time ", DateTime.QSecFrac);
            }
            else{
               DateTime.QSecFrac = 0;
            }
            if(p->data[0] == MAC_PING_REQ_CMD)
            {
                // Skip handle (uint16_t) 2  2
                pData = (uint8_t*) &p->data[1+2];
            }
            else if(p->data[0] == MAC_PING_RSP_CMD)
            {
               // Skip handle (uint16_t) 2  2
               // Skip Origin_xid        5  7
               // Skip RspTxTime         8 15
               // Skip CounterResetFlag  1 16
               //      ResponseMode
               //      Rsvd
               // Skip ReqRxTime         8 24
               pData = (uint8_t*) &p->data[1+24];
            }
            else  /* Only choice left is MAC_TIME_SET_CMD */
            {  // Just skip the frame type
               pData = (uint8_t*) &p->data[1];
            }

            // Update the time field
            pData[0] = (uint8_t) (DateTime.seconds >> 24);
            pData[1] = (uint8_t) (DateTime.seconds >> 16);
            pData[2] = (uint8_t) (DateTime.seconds >>  8);
            pData[3] = (uint8_t) (DateTime.seconds      );

            pData[4] = (uint8_t) (DateTime.QFrac >> 24);
            pData[5] = (uint8_t) (DateTime.QFrac >> 16);
            pData[6] = (uint8_t) (DateTime.QFrac >>  8);
            pData[7] = (uint8_t) (DateTime.QFrac      );

            if((p->data[0] == MAC_TIME_SET_CMD) )
            {  //Add additional parameters for MAC_TIME_SET_CMD
               TIME_SYNC_AddTimeSyncPayload(&pData[0]);
            }
         }
         bitNo = data_frame_encode( p, pDst, bitNo );
      }
      break;

      default:
         break;
   }

   crc_value = CRC_32_Calculate(pDst, (bitNo/8));
   bitNo = PACK_uint32_2bits(&crc_value, FCS_SIZE, pDst, bitNo);

   print_header("TX", p);

   return ( bitNo );

} /* end encode_frame () */

/******************************************************************************

  Function Name: MAC_Codec_Decode ( const uint8_t *p, uint16_t num_bytes, mac_frame_t *pf )

  Purpose: This function decodes a bit packed frame from the PHY layer into
           a structure of type mac_frame_t

  Arguments: p - Pointer to the data buffer (to be decoded)
             num_bytes - Number of bytes in the data buffer
             pf - Pointer to the MAC Frame structure that is to be populated by this function

  Returns:

  Notes:  For now, include the IPv6 Header, and UDP header as part of the MAC Header.  If/when
    the stack trasnitions to a full stack

******************************************************************************/
bool MAC_Codec_Decode ( const uint8_t *p, uint16_t num_bytes, mac_frame_t *pf )
{
   /* at least 11 bytes will be in every MAC Header */
   uint8_t macHeaderLen = MIN_MAC_HEADER_LEN;
   uint16_t bitNo = 0;
   uint16_t expected_bit_length = MIN_MAC_MSG_SIZE;
   bool frame_valid = true;
   if ( (num_bytes * CHAR_BIT) >= expected_bit_length ) // Need at least 4 bits to extract the frame version + CRC32
   {
      /* Decode the Frame Control Field */
      pf->frame_version = UNPACK_bits2_uint8(p, &bitNo, MAC_VERSION_SIZE);
      if (pf->frame_version == MAC_FRAME_VERSION_0)
      {
         expected_bit_length = MIN_MAC_V0_MSG_SIZE; // Minimum header size for frame version 0
         if ( (num_bytes * CHAR_BIT) >= expected_bit_length)
         {
            // decode the MAC header
            pf->frame_type            = UNPACK_bits2_uint8(p, &bitNo, FRAME_TYPE_SIZE);
            pf->dst_addr_mode         = UNPACK_bits2_uint8(p, &bitNo, FRAME_DST_ADDR_MODE_SIZE);
            pf->ack_required          = UNPACK_bits2_uint8(p, &bitNo, ACK_REQUIRED_SIZE);
            pf->ack_present           = UNPACK_bits2_uint8(p, &bitNo, ACK_PRESENT_SIZE);
            pf->segmentation_enabled  = UNPACK_bits2_uint8(p, &bitNo, SEGMENTATION_ENABLED_SIZE);
            pf->packet_id             = UNPACK_bits2_uint8(p, &bitNo, PACKET_ID_SIZE);
            pf->network_id            = UNPACK_bits2_uint8(p, &bitNo, NETWORK_ID_SIZE);

            // Source Address
            UNPACK_addr(p, &bitNo, MAC_ADDRESS_SIZE, &pf->src_addr[0]);

            expected_bit_length += ((pf->dst_addr_mode) * (MAC_ADDRESS_SIZE*CHAR_BIT));
            expected_bit_length += ((pf->ack_required || pf->ack_present) * (SEQ_NUM_SIZE + REQ_NUM_SIZE));/*lint !e514 */
            expected_bit_length += ((pf->segmentation_enabled) * SEGMENT_ID_SIZE);
            expected_bit_length += ((pf->segmentation_enabled) * SEGMENT_COUNT_SIZE);

            if ( (num_bytes * CHAR_BIT) >= expected_bit_length)
            {  // it is ok to decode the rest of the frame
               if (pf->frame_type != MAC_RESERVED_FRAME)
               {
                  if (pf->dst_addr_mode == UNICAST_MODE)
                  {
                     macHeaderLen += MAC_ADDRESS_SIZE;
                     UNPACK_addr(p, &bitNo, MAC_ADDRESS_SIZE, &pf->dst_addr[0]);
                  }else
                  {  // Set the field to 0xFF
                     (void) memset(&pf->dst_addr[0], 0xFF, MAC_ADDRESS_SIZE);
                  }

                  // Ack Info
                  if (pf->ack_required == ACK_REQUIRED)
                  {
                     pf->seq_num = UNPACK_bits2_uint8(p, &bitNo, SEQ_NUM_SIZE);
                  }else
                  {  // Set to default
                     pf->seq_num = 0;
                  }

                  if (pf->ack_present == ACK_PRESENT)
                  {
                     pf->req_num = UNPACK_bits2_uint8(p, &bitNo, REQ_NUM_SIZE);
                  }else
                  {  // Set to default
                     pf->req_num = 0;
                  }

                  if ( (pf->ack_required == ACK_REQUIRED) || (pf->ack_present == ACK_PRESENT) )
                  {
                     macHeaderLen++;
                  }

                  if (pf->segmentation_enabled == FRAME_SEGMENTATED)
                  {
                     pf->segment_id = UNPACK_bits2_uint8(p, &bitNo,  SEGMENT_ID_SIZE);
                     pf->segment_count = UNPACK_bits2_uint8(p, &bitNo,  SEGMENT_COUNT_SIZE);
                     macHeaderLen++;
                  }else
                  {  // Set to defaults
                     pf->segment_id    = 0;
                     pf->segment_count = 0;
                  }
                  /* Now verify segment_id and segment count are "sane" */
                  if ( ( pf->segment_id <= pf->segment_count ) && ( pf->segment_count <= MAX_VALID_SEGMENT_COUNT ) )
                  {
                     if ( (pf->segmentation_enabled == FRAME_NOT_SEGMENTATED) ||
                        ( (pf->segmentation_enabled == FRAME_SEGMENTATED) && (pf->segment_id == 0) ) )
                     {
                        macHeaderLen += 2;
                        expected_bit_length += LENGTH_SIZE;
                        if ( (num_bytes * CHAR_BIT) >= expected_bit_length)
                        {
                           pf->length = UNPACK_bits2_uint16(p, &bitNo, LENGTH_SIZE);
                        }else
                        {  // Mark as invalid
                           frame_valid = false;
                        }
                     }
                  }
                  else
                  {  // Mark as invalid
                     frame_valid = false;
                  }
                  if(frame_valid)
                  {
                     // This will be the number of bytes remaining
                     pf->thisFrameLen = num_bytes - macHeaderLen;
                     // Copy the data
                     (void)memcpy(pf->data, &p[ (bitNo/8) ], min(pf->thisFrameLen, sizeof(pf->data)));
                     print_header("RX", pf);
                  }
               }else
               {  // invalid frame_type == MAC_RESERVED_FRAME
                  frame_valid = false;
               }
            }else
            {  // invalid length, < expected_bit_length
               frame_valid = false;
            }
         }else
         {  // invalid length, < expected_bit_length
            frame_valid = false;
         }
      }else
      {  // Invalid frame version
         frame_valid = false;
      }
   }else
   {  // invalid length, == 0
      frame_valid = false;
   }
   return frame_valid;

} /* end decode_frame () */

/***********************************************************************************************************************
Function Name: MAC_Codec_SetDefault

Purpose: This function is used to intialize a frame to default values.
         It initializes the network id and the source address

Arguments: pf             - Pointer to the frame structure
           frame_type     - the type of frame

Returns: none
***********************************************************************************************************************/
void MAC_Codec_SetDefault(mac_frame_t *pf, uint8_t frame_type)
{
   uint8_t mac_address[MAC_ADDRESS_SIZE];

   MAC_MacAddress_Get(mac_address);

   pf->frame_version          = MAC_FRAME_VERSION_0;
   pf->frame_type             = frame_type;
   pf->dst_addr_mode          = 0; // No destination address present
   pf->ack_required           = NO_ACK_REQUIRED;
   pf->ack_present            = NO_ACK_REQUIRED;
   pf->segmentation_enabled   = FRAME_NOT_SEGMENTATED;
   pf->packet_id              = 0;
   pf->network_id             = NetworkId_Get();
   (void) memcpy(pf->src_addr, mac_address, sizeof(pf->src_addr));
   (void) memset(pf->dst_addr, 0xFF,        sizeof(pf->dst_addr));
   pf->seq_num  = 0;
   pf->req_num = 0;
   pf->segment_id = 0;
   pf->segment_count = 0;
   pf->length = 0;
}

/*
 * This function is used to print the header to the terminal
 */
static void print_header( const char* msg, const mac_frame_t *p )
{
   char buffer[130];
   int32_t offset = 0;
   offset += sprintf(&buffer[offset],   "%s",  msg);
   offset += sprintf(&buffer[offset],   " ver:%d",  p->frame_version);

   char* szFrameType;
   switch ( p->frame_type )
   {
      case MAC_DATA_FRAME:
         szFrameType = "DATA";
         break;
      case MAC_ACK_FRAME:
         szFrameType = "ACK";
         break;
      case MAC_CMD_FRAME:
         szFrameType = "CMD";
         break;
      default:
         szFrameType = "UNK";
         break;
   }
   offset += sprintf(&buffer[offset],   " type:%s", szFrameType);

   char* szAddrMode;
   if(p->dst_addr_mode == UNICAST_MODE)
   {
      szAddrMode = "UNICAST";
   }else
   {
      szAddrMode = "BROADCAST";
   }
   offset += sprintf(&buffer[offset],   " addrmode:%s", szAddrMode);

   offset += sprintf(&buffer[offset],   " arq:%d"                     // ACK Requested
                                        " ack:%d"                     // Ack
                                        " seg:%d"                     // Segmented
                                        " pid:%d"                     // Packet Id
                                        " nid:%d",                    // Network Id
       p->ack_required,
       p->ack_present,
       p->segmentation_enabled,
       p->packet_id,
       p->network_id);

   // Only print if segmented
   if(p->segmentation_enabled)
   {  // Only print if segmented
      offset += sprintf(&buffer[offset]," segid:%d/%d",            // Segment Id / Segment Count
      p->segment_id, p->segment_count);
   }

   offset += sprintf(&buffer[offset],   " src:%02x%02x%02x%02x%02x",   // Source Address
       p->src_addr[0], p->src_addr[1], p->src_addr[2], p->src_addr[3], p->src_addr[4]);

   // Only print destination address if unicast
   if(p->dst_addr_mode == UNICAST_MODE)
   {
      offset += sprintf(&buffer[offset],   " dst:%02x%02x%02x%02x%02x",   // Source Address
          p->dst_addr[0], p->dst_addr[1], p->dst_addr[2], p->dst_addr[3], p->dst_addr[4]);
   }

   offset += sprintf(&buffer[offset],   " Ns:%d Nr:%d"                 // Sequence Number S(n), Request Number R(n)
                                        " len:%d",                     // Frame Length
       p->seq_num, p->req_num,
       p->thisFrameLen);

   INFO_printf("%s", buffer);

}

/*!
 * Data Frame Encoder
 */
static uint16_t data_frame_encode ( mac_frame_t const *p, uint8_t* pDst, uint16_t bitNo )
{
   uint16_t i;
   for(i = 0; i < p->thisFrameLen; i++)
   {
      bitNo = PACK_uint8_2bits(&p->data[i], 8, pDst, bitNo);
   }
   return bitNo;
}
