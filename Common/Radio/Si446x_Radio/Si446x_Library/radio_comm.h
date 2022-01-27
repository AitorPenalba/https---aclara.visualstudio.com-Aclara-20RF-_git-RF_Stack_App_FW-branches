
/*!
 * File:
 *  radio_comm.h
 *
 * Description:
 *  This file contains the RADIO communication layer.
 *
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 */
#ifndef _RADIO_COMM_H_
#define _RADIO_COMM_H_


                /* ======================================= *
                 *              I N C L U D E              *
                 * ======================================= */


                /* ======================================= *
                 *          D E F I N I T I O N S          *
                 * ======================================= */

#define RADIO_CTS_TIMEOUT 20000  // Max value is 65535

                /* ======================================= *
                 *     G L O B A L   V A R I A B L E S     *
                 * ======================================= */

//extern SEGMENT_VARIABLE(radioCmd[16u], U8, SEG_XDATA);
extern uint8_t radioCmd[16u];


                /* ======================================= *
                 *  F U N C T I O N   P R O T O T Y P E S  *
                 * ======================================= */

U8 radio_comm_GetResp(uint8_t radioNum, U8 byteCount, U8* pData);
bool radio_comm_SendCmd(uint8_t radioNum, U8 byteCount, U8 const * pData);
bool radio_comm_SendCmd_timed(uint8_t radioNum, U8 byteCount, U8 const * pData, uint32_t txtime);
void radio_comm_ReadData(uint8_t radioNum, U8 cmd, bool pollCts, U8 byteCount, U8* pData);
void radio_comm_WriteData(uint8_t radioNum, U8 cmd, bool pollCts, U8 byteCount, U8 const * pData);
U8 radio_comm_PollCTS(uint8_t radioNum);
U8 radio_comm_SendCmdGetResp(uint8_t radioNum, U8 cmdByteCount, U8 const * pCmdData, \
                             U8 respByteCount, U8* pRespData);
void radio_comm_ClearCTS(uint8_t radioNum);

#endif //_RADIO_COMM_H_
