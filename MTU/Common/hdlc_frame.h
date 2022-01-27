/** ****************************************************************************
@file hdlc_frame.h
 
API for the HDLC farme formatting code.
 
A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.

Portions Copyright (c) 2006 Pieter Conradie
*******************************************************************************/

#ifndef HDLC_FRAME_H
#define HDLC_FRAME_H

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

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
Public #defines (Object-like macros)
*******************************************************************************/

#ifndef HDLC_MRU
/// Receive frame size (Maximum Receive Unit)
#define HDLC_MRU 2048
#endif

/*******************************************************************************
Public #defines (Function-like macros)
*******************************************************************************/

/*******************************************************************************
Public Struct, Typedef, and Enum Definitions
*******************************************************************************/

/** Defines the different types of frames supported by HDLC. */
typedef enum
{
   /** Information frame with data. */
   HDLC_FRAME_I = 0x00,

   /** Supervisory frame for error and flow control. */
   HDLC_FRAME_S = 0x01,

   /** Unnumbered frame for miscellaneous data and link control. */
   HDLC_FRAME_U = 0x03
} hdlc_frame_t;

/** Defines the different unnumbered HDLC commands. */
typedef enum
{
   /** Set Normal Response Mode */
   HDLC_UCMD_SNRM = 0x20,

   /** Set Asynchronous Response Mode */
   HDLC_UCMD_SARM = 0x03,

   /** Set Asynchronous Balanced Mode */
   HDLC_UCMD_SABM = 0x0B,

   /** Disconnect */
   HDLC_UCMD_DISC = 0x10,

   /** Set Normal Response Mode Extended */
   HDLC_UCMD_SNRME = 0x33,

   /** Set Asynchronous Response Mode Extended */
   HDLC_UCMD_SARME = 0x13,

   /** Set Asynchronous Balanced Mode Extended */
   HDLC_UCMD_SABME = 0x1B,

   /** Set Initialization Mode */
   HDLC_UCMD_SIM = 0x01,

   /** Unnumbered Poll */
   HDLC_UCMD_UP = 0x08,

   /** Unnumbered Information */
   HDLC_UCMD_UI = 0x00,

   /** Exchange ID */
   HDLC_UCMD_XID = 0x2B,

   /** Reset */
   HDLC_UCMD_RSET = 0x23,

   /** Test */
   HDLC_UCMD_TEST = 0x38,

   /** Set Mode */
   HDLC_UCMD_SM = 0x30,

   /** Unnumbered Information with Header */
   HDLC_UCMD_UIH = 0x3B
} hdlc_ucommand_t;

/** Defines standard responses expected for the various unnumbered commands. */
typedef enum
{
   /** Unnumbered Acknowledgement  */
   HDLC_URESP_UA = 0x18,

   /** Frame Error */
   HDLC_URESP_FRMR = 0x21,

   /** Disconnected Mode */
   HDLC_URESP_DM = 0x03,

   /** Request Disconnect */
   HDLC_URESP_RD = 0x10,

   /** Request Initialization Mode */
   HDLC_URESP_RIM = 0x01,

   /** Unnumbered Information */
   HDLC_URESP_UI = 0x00,

   /** Exchange ID */
   HDLC_URESP_XID = 0x2B,

   /** Test */
   HDLC_URESP_TEST = 0x38,

   /** Unnumbered Information with Header */
   HDLC_URESP_UIH = 0x3B
} hdlc_uresp_t;

/** Defines the different supervisory HDLC commands */
typedef enum
{
   /** Receive Ready  */
   HDLC_SCMD_RR = 0x00,

   /** Reject */
   HDLC_SCMD_REJ = 0x01,

   /** Receive Not Ready */
   HDLC_SCMD_RNR = 0x02,

   /** Selective Reject */
   HDLC_SCMD_SREJ = 0x03
} hdlc_scommand_t;

/** Stores all the possible information to send in a HDLC control value */
typedef struct
{
   /** The type of the frame */
   hdlc_frame_t type;

   /** The unnumbered command if an U-frame */
   hdlc_ucommand_t uCommand;

   /** The supervisory command if a S-frame */
   hdlc_scommand_t sCommand;

   /** The send sequence for an I-frame */
   uint8_t sendSequence;

   /** The receive sequence for a I or S-frame */
   uint8_t recSequence;

   /** The Poll/Final bit status */
   bool pfBit;
} hdlc_control_info_t;

/** Defines the size of the address value for the HDLC implementation */
typedef uint8_t hdlc_addr_t;

/** Defines the size of the control value for the HDLC implementation */
typedef uint8_t hdlc_ctrl_t;

/** Stores frame information for sending over HDLC */
typedef struct
{
   hdlc_addr_t addr;
   hdlc_ctrl_t control;
   uint8_t *packet;
   uint16_t packetLen;
} HdlcFrameData_t;

/**
Definition for a pointer to a function that will be called to send a byte
*/
typedef void (*hdlc_tx_fn_t)(uint8_t data);

/**
Definition for a pointer to a function that will be called once a frame has been received.
*/
typedef void (*hdlc_on_rx_frame_fn_t)(hdlc_addr_t addr, hdlc_ctrl_t ctrl, const uint8_t *data, uint16_t nr_of_bytes, bool overflow);

/*******************************************************************************
Global Variable Extern Statements
*******************************************************************************/

/*******************************************************************************
Public Function Prototypes
*******************************************************************************/

void HdlcFrameInit(hdlc_tx_fn_t txFn, hdlc_on_rx_frame_fn_t onRxFrameFn);
void HdlcFrameOnRxByte(uint8_t data);
void HdlcFrameTx(const HdlcFrameData_t *frameInfo);

hdlc_ctrl_t HdlcFrameCreateControlValue(hdlc_control_info_t info);
hdlc_control_info_t HdlcFrameGetControlInfo(hdlc_ctrl_t value);

#ifdef __cplusplus
}
#endif

#endif // #ifndef __HDLC_FRAME_H__
