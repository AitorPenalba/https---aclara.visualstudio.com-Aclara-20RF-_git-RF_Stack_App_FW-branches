/*!
 * File:
 *  radio_comm.h
 *
 * Description:
 *  This file contains the RADIO communication layer.
 *
 * Silicon Laboratories Confidential
 * Copyright 2012 Silicon Laboratories, Inc.
 */

               /* ======================================= *
                *              I N C L U D E              *
                * ======================================= */

#include "project.h"
#include <stdbool.h>
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <bsp.h>
#endif
#include "compiler_types.h"
#include "radio.h"
#include "radio_hal.h"
#include "radio_comm.h"
#include "time_util.h"
#if (DCU==1)
#include "version.h"
#endif

               /* ======================================= *
                *          D E F I N I T I O N S          *
                * ======================================= */

               /* ======================================= *
                *     G L O B A L   V A R I A B L E S     *
                * ======================================= */

static bool ctsWentHigh[MAX_RADIO] = {0};


               /* ======================================= *
                *      L O C A L   F U N C T I O N S      *
                * ======================================= */

               /* ======================================= *
                *     P U B L I C   F U N C T I O N S     *
                * ======================================= */

/*!
 * Gets a command response from the radio chip
 *
 * @param byteCount     Number of bytes to get from the radio chip
 * @param pData         Pointer to where to put the data
 *
 * @return CTS value
 */
U8 radio_comm_GetResp(uint8_t radioNum, U8 byteCount, U8* pData)
{
   //  SEGMENT_VARIABLE(ctsVal = 0u, U8, SEG_DATA);
   uint8_t ctsVal = 0u;
   //  SEGMENT_VARIABLE(errCnt = RADIO_CTS_TIMEOUT, U16, SEG_DATA);
   uint16_t errCnt = RADIO_CTS_TIMEOUT;

   uint8_t spifd;

   while (errCnt != 0)      //wait until radio IC is ready with the data
   {
      spifd = radio_hal_ClearNsel(radioNum);
      radio_hal_SpiWriteByte(spifd, 0x44);    //read CMD buffer
      ctsVal = radio_hal_SpiReadByte(spifd);
      if (ctsVal == 0xFF)
      {
         if (byteCount)
         {
            radio_hal_SpiReadData(spifd, byteCount, pData);
         }
         radio_hal_SetNsel(spifd);
         break;
      }
      radio_hal_SetNsel(spifd);
      errCnt--;
   }

   if (errCnt == 0)
   {
      // Signal the CTS Line low Event
#if ( RTOS_SELECTION == MQX_RTOS )
      RADIO_Event_Set(eRADIO_CTS_LINE_LOW, radioNum);
#elif ( RTOS_SELECTION == FREE_RTOS )
      RADIO_Event_Set(eRADIO_CTS_LINE_LOW, radioNum, (bool)false);
#endif
   }

   if (ctsVal == 0xFF)
   {
      ctsWentHigh[radioNum] = 1;
   }

   return ctsVal;
}

/*!
 * Sends a command to the radio chip
 *
 * @param byteCount     Number of bytes in the command to send to the radio device
 * @param pData         Pointer to the command to send.
 */
bool radio_comm_SendCmd( uint8_t radioNum, U8 byteCount, U8 const * pData)
{
   bool error = false;

   uint8_t spifd;            /* SPI port "handle"                         */

   RADIO_Lock_Mutex();
   while (!ctsWentHigh[radioNum])
   {
      if (!radio_comm_PollCTS(radioNum))
         error =  true; // error
   }

   if (!error )
   {
      spifd = radio_hal_ClearNsel(radioNum);
      radio_hal_SpiWriteData(spifd, byteCount, pData);
      radio_hal_SetNsel(spifd);
      ctsWentHigh[radioNum] = 0;
   }
   RADIO_UnLock_Mutex();

   return error;
}

/*!
 * Sends a command to the radio chip
 *
 * @param byteCount     Number of bytes in the command to send to the radio device
 * @param pData         Pointer to the command to send.
 */
bool radio_comm_SendCmd_timed( uint8_t radioNum, U8 byteCount, U8 const * pData, uint32_t txtime)
{
#if (DCU != 1)
   (void)txtime;
#endif
   bool error = false;
   uint8_t spifd;            /* SPI port "handle"                         */

   RADIO_Lock_Mutex();
   while (!ctsWentHigh[radioNum])
   {
      if (!radio_comm_PollCTS(radioNum))
         error = true; // error
   }

   if (!error)
   {
      spifd = radio_hal_ClearNsel(radioNum);
      radio_hal_SpiWriteData(spifd, byteCount, pData);
#if (DCU == 1)
      if ((VER_getDCUVersion() != eDCU2) && txtime) {
         // Burn remaining time until we need to start TX
         while ( (int32_t)(txtime - DWT_CYCCNT) > 0 )
            ;
      }
#endif
      radio_hal_SetNsel(spifd);
#if ( EP == 1 ) && ( TEST_TDMA == 1 )
      LED_on(RED_LED);
#endif
      ctsWentHigh[radioNum] = 0;
   }
   RADIO_UnLock_Mutex();

   return error;
}

/*!
 * Gets a command response from the radio chip
 *
 * @param cmd           Command ID
 * @param pollCts       Set to poll CTS
 * @param byteCount     Number of bytes to get from the radio chip.
 * @param pData         Pointer to where to put the data.
 */
void radio_comm_ReadData(uint8_t radioNum, U8 cmd, bool pollCts, U8 byteCount, U8* pData)
{
   uint8_t spifd;

   RADIO_Lock_Mutex(); // 309 Cycles, 0.2%
   if(pollCts) //Passed in 0
   {
      while(!ctsWentHigh[radioNum])
      {
         (void)radio_comm_PollCTS( radioNum );
      }
   }
   spifd = radio_hal_ClearNsel( radioNum ); //44
   radio_hal_SpiWriteByte(spifd, cmd);      //516 - 0.3%
   radio_hal_SpiReadData(spifd, byteCount, pData); //10141 - 6.6%
   radio_hal_SetNsel(spifd);                //52
   ctsWentHigh[radioNum] = 0;               //3
   RADIO_UnLock_Mutex();                    //~276
}

/*!
 * Gets a command response from the radio chip
 *
 * @param cmd           Command ID
 * @param pollCts       Set to poll CTS
 * @param byteCount     Number of bytes to get from the radio chip
 * @param pData         Pointer to where to put the data
 */
void radio_comm_WriteData( uint8_t radioNum, U8 cmd, bool pollCts, U8 byteCount, U8 const * pData)
{
   uint8_t spifd;

   RADIO_Lock_Mutex();
   if(pollCts)
   {
      while(!ctsWentHigh[radioNum])
      {
         (void)radio_comm_PollCTS(radioNum);
      }
   }
   spifd = radio_hal_ClearNsel(radioNum);
   radio_hal_SpiWriteByte(spifd, cmd);
   radio_hal_SpiWriteData(spifd, byteCount, pData);
   radio_hal_SetNsel(spifd);
   ctsWentHigh[radioNum] = 0;
   RADIO_UnLock_Mutex();
}

/*!
 * Waits for CTS to be high
 *
 * @return CTS value
 */
U8 radio_comm_PollCTS( uint8_t radioNum )
{
#ifdef RADIO_USER_CFG_USE_GPIO1_FOR_CTS
   while(!radio_hal_Gpio1Level())
   {
      /* Wait...*/
   }
   ctsWentHigh = 1;
   return 0xFF;
#else
   static const uint32_t defaultDelay = 5000;   /*  Delay 5ms after/between polling CTS      */
   static bool firstPollDone[ MAX_RADIO ] = {(bool)false};
   uint8_t retVal;
#if ( RTOS_SELECTION == MQX_RTOS ) //TODO Melvin: replace it with the RTOS timers
   MQX_TICK_STRUCT   startTime;              /* Used to delay sub-tick time               */
   MQX_TICK_STRUCT   endTime;                /* Used to delay sub-tick time               */
   bool              overflow=(bool)false;   /* required by tick to microsecond function  */
#elif ( RTOS_SELECTION == FREE_RTOS )
   OS_TICK_Struct   startTime;              /* Used to delay sub-tick time               */
   OS_TICK_Struct   endTime;                /* Used to delay sub-tick time               */
#endif

   retVal = radio_comm_GetResp(radioNum, 0, 0);

#if ( RTOS_SELECTION == MQX_RTOS ) //TODO Melvin: replace it with the RTOS timers
   if ( !firstPollDone[ radioNum ] )
   {
      _time_get_ticks(&startTime);
      endTime = startTime;

      while ( (uint32_t)_time_diff_microseconds(&endTime, &startTime, &overflow) < defaultDelay)
      {
         _time_get_ticks(&endTime);
      }
      firstPollDone[ radioNum ] = (bool)true;
   }
#elif ( RTOS_SELECTION == FREE_RTOS )
   if ( !firstPollDone[ radioNum ] )
   {
      OS_TICK_Get_CurrentElapsedTicks(&startTime); // TODO: RA6: Check working of having elapsed ticks instead get ticks
      endTime = startTime;

      while ( (uint32_t)OS_TICK_Get_Diff_InMicroseconds( &startTime, &endTime ) < defaultDelay)
      {
         OS_TICK_Get_CurrentElapsedTicks(&endTime);
      }
      firstPollDone[ radioNum ] = (bool)true;
   }
#endif
   return retVal;
#endif
}

/**
 * Clears the CTS state variable.
 */
void radio_comm_ClearCTS(uint8_t radioNum)
{
   ctsWentHigh[radioNum] = 0;
}

/*!
 * Sends a command to the radio chip and gets a response
 *
 * @param cmdByteCount  Number of bytes in the command to send to the radio device
 * @param pCmdData      Pointer to the command data
 * @param respByteCount Number of bytes in the response to fetch
 * @param pRespData     Pointer to where to put the response data
 *
 * @return CTS value
 */
U8 radio_comm_SendCmdGetResp(uint8_t radioNum, U8 cmdByteCount, U8 const * pCmdData, U8 respByteCount, U8* pRespData)
{
   bool error = false;
   uint8_t spifd;            // SPI port "handle"
   uint8_t ctsVal = 0u;
   uint16_t errCnt = RADIO_CTS_TIMEOUT;

   RADIO_Lock_Mutex(); // Takes ~310 Cycles (*781/sec is 2 ms or 0.2%)
   if (!ctsWentHigh[radioNum]) // Make sure radio is ready
   {
      if (!radio_comm_PollCTS(radioNum))
         error =  ((bool)true); // error
   }
   if (!error) // Write the data
   {
      spifd = radio_hal_ClearNsel(radioNum);
      radio_hal_SpiWriteData(spifd, cmdByteCount, pCmdData);
      radio_hal_SetNsel(spifd);
      ctsWentHigh[radioNum] = 0;
   }

   // If here with "error == false" .. we successfully send the command.
   // Get the response

   if (!error)// Poll for response
   {
//      result = radio_comm_GetResp( radioNum, respByteCount, pRespData);
      while (errCnt != 0)      //wait until radio IC is ready with the data (polling) (change to >0)
      {
         spifd = radio_hal_ClearNsel(radioNum);
         radio_hal_SpiWriteByte(spifd, 0x44);    //read CMD buffer
         ctsVal = radio_hal_SpiReadByte(spifd);
         if (ctsVal == 0xFF) //If status ClearToSend
         {
            if (respByteCount)
            {
               // The max possible speed of this is ((7 bytes * 8bits/byte)/10000000b/sec)*781 = 0.43%
               // All things considered, this is not really a bottleneck. The only time this matters is the last
               //  time it is called. so if the while loop cycles 6 times, using ~10% of cpu time, decreaseing the
               //  the execution time of this function from ~1% to ~0.4% would make the function use 9.4% cpu time.
               // So optimizing this is probably not the speedup we are looking for (it would only increase the polling
               //  rate, not decrease the response time of the radio)
               radio_hal_SpiReadData(spifd, respByteCount, pRespData); // Read the response (1143 Cycles, 0.94%)
            }
            radio_hal_SetNsel(spifd);
            break;
         }
         radio_hal_SetNsel(spifd);
         errCnt--;
      } //Takes ~2562 Cycles, 1.66%


      //RADIO_UnLock_Mutex();
   }

   // If error then fake CTS error because there is no way to differentiate the two.
   if ((errCnt == 0) || error)
   {
      // Signal the CTS Line low Event
#if ( RTOS_SELECTION == MQX_RTOS )
      RADIO_Event_Set(eRADIO_CTS_LINE_LOW, radioNum);
#elif ( RTOS_SELECTION == FREE_RTOS )
      RADIO_Event_Set(eRADIO_CTS_LINE_LOW, radioNum, (bool)false);
#endif
   }

   if (ctsVal == 0xFF)
   {
      ctsWentHigh[radioNum] = 1;
   }

   RADIO_UnLock_Mutex();

   return ctsVal;
}
