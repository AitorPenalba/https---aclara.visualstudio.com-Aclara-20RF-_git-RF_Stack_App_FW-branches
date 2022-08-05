/******************************************************************************
 *
 * Filename: UART.h
 *
 * Global Designator: UART_
 *
 * Contents:
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 *****************************************************************************/
#ifndef UART_H
#define UART_H
/* INCLUDE FILES */

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
#if ( TM_UART_EVENT_COUNTERS == 1 ) /* Define a set of counters per UART port for debugging purposes */
typedef struct
{
   uint32_t eventRxComplete;
   uint32_t eventTxComplete;
   uint32_t eventRxChar;
   uint32_t eventErrParity;
   uint32_t eventErrFraming;
   uint32_t eventErrOverflow;
   uint32_t eventBreakDetect;
   uint32_t eventDataEmpty;
   uint32_t eventUnknownType;
   uint32_t basicUARTWriteVar;
   uint32_t uartWritePendBefore;
   uint32_t uartWritePendAfter;
   uint32_t uartGetcPendBefore;
   uint32_t uartGetcPendAfter;
   uint32_t ringOverFlowVar;
   uint32_t uartEchoPendBefore;
   uint32_t uartEchoPendAfter;
   uint32_t receiveRxChar;
   uint32_t missingPacketsCozRingBuf;
   uint32_t isrRingBufferOverflow;
   uint32_t UARTechoVar;
   uint32_t ZeroLengthEcho;
} Uart_Events_t;
#define UART_NUM_FIELDS ( sizeof(Uart_Events_t) / 4 ) /* All data elements must be uint32_t */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
extern bool UART_GetCounters  ( enum_UART_ID UartId, const Uart_Events_t *pReturnStruct );
extern bool UART_ClearCounters( enum_UART_ID UartId );
#endif // ( TM_UART_EVENT_COUNTERS == 1 )

#endif