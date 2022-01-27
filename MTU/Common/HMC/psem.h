/***********************************************************************************************************************
 *
 * Filename: psem.h
 *
 * Contents: All of the host meter communication definitions or a Little Endian Machine, bit fields are LS bit first.
 *           PSEM : Protocol Specification for Electric Metering
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ kad Created 073107
 *
 ***********************************************************************************************************************
 * Revision History:
 * v1.0 - Initial Release
 * v1.1 - Added structure definition for Standard Table 8, tProcResult.
 *
 **********************************************************************************************************************/

#ifndef psem_H
#define psem_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define PSEM_OFFSET_LEN                (uint8_t)3    /* Length of the offset field */
#define PSEM_OBJECT_INDEX_LEN          (uint8_t)8    /* Length of the Obeject Index field */
#define PSEM_PASSWORD_LEN              (uint8_t)20   /* Length of the Password field */
#define PSEM_USER_NAME_LEN             (uint8_t)10   /* Length of the User Name field */

#define PSEM_ACK	                     (uint8_t)0x06
#define PSEM_NAK	                     (uint8_t)0x15

/* ANSI C12 Command List */
#define CMD_IDENT                      (uint8_t)0x20 /* Nil */
#define CMD_TERMINATE                  (uint8_t)0x21 /* Nil */
#define CMD_TBL_RD_FULL                (uint8_t)0x30 /* Table ID */
#define CMD_DEFAULT                    (uint8_t)0x3E /* Nil */
#define CMD_TBL_RD_PARTIAL             (uint8_t)0x3F /* Table ID, Offset, Count */
#define CMD_TBL_WR_FULL                (uint8_t)0x40 /* Table ID, Data */
#define CMD_TBL_WR_PARTIAL             (uint8_t)0x4F /* Table ID, Offset, Data */
#define CMD_LOG_ON_REQ                 (uint8_t)0x50 /* User ID, User Name */
#define CMD_PASSWORD                   (uint8_t)0x51 /* Password */
#define CMD_LOG_OFF                    (uint8_t)0x52 /* Nil */
#define CMD_NEG_SERVICE_REQ              (uint8_t)0x60 /* Max Packet Size, Max Packets, No Baudrate */
#define CMD_NEG_SERVICE_REQ_ONE_BAUD     (uint8_t)0x61 /* Max Packet Size, Max Packets, 1  Baudrates*/
#define CMD_NEG_SERVICE_REQ_TWO_BAUD     (uint8_t)0x62 /* Max Packet Size, Max Packets, 2  Baudrates*/
#define CMD_NEG_SERVICE_REQ_THREE_BAUD   (uint8_t)0x63 /* Max Packet Size, Max Packets, 3  Baudrates*/
#define CMD_NEG_SERVICE_REQ_FOUR_BAUD    (uint8_t)0x64 /* Max Packet Size, Max Packets, 4  Baudrate*/
#define CMD_NEG_SERVICE_REQ_FIVE_BAUD    (uint8_t)0x65 /* Max Packet Size, Max Packets, 5  Baudrate*/
#define CMD_NEG_SERVICE_REQ_SIX_BAUD     (uint8_t)0x66 /* Max Packet Size, Max Packets, 6  Baudrate*/
#define CMD_NEG_SERVICE_REQ_SEVEN_BAUD   (uint8_t)0x67 /* Max Packet Size, Max Packets, 7  Baudrate*/
#define CMD_NEG_SERVICE_REQ_EIGHT_BAUD   (uint8_t)0x68 /* Max Packet Size, Max Packets, 8  Baudrate*/
#define CMD_NEG_SERVICE_REQ_NINE_BAUD    (uint8_t)0x69 /* Max Packet Size, Max Packets, 9  Baudrate*/
#define CMD_NEG_SERVICE_REQ_TEN_BAUD     (uint8_t)0x6A /* Max Packet Size, Max Packets, 10  Baudrate*/
#define CMD_NEG_SERVICE_REQ_ELEVEN_BAUD  (uint8_t)0x6B /* Max Packet Size, Max Packets, 11  Baudrate*/
#define CMD_WAIT_SERVICE_REQ           (uint8_t)0x70 /* Seconds */

/* C12 Table Response Types - The enumeration is for the protocol layer, not per ANSI C12*/
#define RESP_TYPE_NILL                 (uint8_t)0
#define RESP_TYPE_SERVICE_CODE         (uint8_t)1
#define RESP_TYPE_TABLE_DATA           (uint8_t)2
#define RESP_TYPE_NEGOTIATE0           (uint8_t)3

/* C12 Table Response Error Codes - Located in the 1st byte of data in the response */
#define RESP_OK                        (uint8_t)0x00  /* No Whammies */
#define RESP_ERR                       (uint8_t)0x01  /* Rejection of the received service request */
#define RESP_SNS                       (uint8_t)0x02  /* Service Not Supported */
#define RESP_ISC                       (uint8_t)0x03  /* Insufficient Security Clearance */
#define RESP_ONP                       (uint8_t)0x04  /* Operation Not Possible, MSG Not Processed */
#define RESP_IAR                       (uint8_t)0x05  /* Inappropriate Action */
#define RESP_BSY                       (uint8_t)0x06  /* Busy, no action taken */
#define RESP_DNR                       (uint8_t)0x07  /* Data Not Ready */
#define RESP_DLK                       (uint8_t)0x08  /* Data Locked */
#define RESP_RNO                       (uint8_t)0x09  /* Renegotiate Request */
#define RESP_ISSS                      (uint8_t)0x0A  /* Invalid Service Sequence State */
#define RESP_COM_ERROR                 (uint8_t)100   /* Aclara Communication Error, not from the response! */

#define PROC_STD                       ((uint8_t)0)     /* Execute a standard procedure */
#define PROC_MFG                       ((uint16_t)1<<11) /* Execute a manufacturing procedure */
#define PROC_WR_SELECTOR               ((uint8_t)0)     /* This valud is always sent as 0 */

#define PROC_RESP_PROC_COMPLETED       ((uint8_t)0) /* Procedure executed normally */
#define PROC_RESP_INVALID_PARAM        ((uint8_t)2) /* Using invalid parameter in the PARM_RCD field */
#define PROC_RESP_UNRECONGNIZED_PROC   ((uint8_t)6) /* Attempted execution of undefined procedure, operation ignored */

/* Misc Format Characters and locations */
#define PSEM_CTRL_TOGGLE               (uint8_t)0x20  /* Control Byte TOGGLE, bit gets toggled for every new packet */
#define PSEM_CTRL_FIRST                (uint8_t)0x40  /* Control Byte, true = First packet in a multipacket command */
#define PSEM_CTRL_MULTI                (uint8_t)0x80  /* Control Byte, true = MULTI part packet */
#define PSEM_CTRL_MASK                 (uint8_t)0xE0  /* Control Byte MASK, The lower 5-bits must be 0s */

#define PSEM_RESP_DATA_START           (uint8_t)0x03  /* Location of 1st Data Byte */
#define PSEM_RESP_DATA_STATUS          (uint8_t)0x00  /* Location of the status byte in the data */

#define PSEM_STP                       (uint8_t)0xEE  /* Start Of Packet Character */
#define PSEM_UNIVERSAL_ID              (uint8_t)0x00  /* Universal ID Byte */

#define PSEM_PROCEDURE_TBL_ID          BIG_ENDIAN_16BIT((uint16_t)0x0007) /* Procedure Initiate Table */
#define PSEM_PROCEDURE_TBL_RES         BIG_ENDIAN_16BIT((uint16_t)0x0008) /* Procedure Initiate Table */

/* Procedure Results Definition:  Note:  Use sizeof 'tProcResult' for the length.  */
#define TBL_PROC_RES_OFFSET            (uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00  /* No Offset */

/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* Common typedefs that is used by HMC */
// <editor-fold defaultstate="collapsed" desc="tTbl8ProcResponse">
#if ( PROCESSOR_LITTLE_ENDIAN == 1 && BIT_ORDER_LOW_TO_HIGH == 1 )
PACK_BEGIN
typedef struct PACK_MID
{

   uint16_t    tblProcNbr  : 11;
   uint16_t    mfgFlag     : 1;
   uint16_t    selector    : 4;
   uint8_t     seqNbr;
   uint8_t     resultCode;
   uint8_t     respDataRcd;
} tTbl8ProcResponse;
PACK_END
#endif
// </editor-fold>

typedef struct
{
   uint16_t tableID;
   uint8_t  offset[PSEM_OFFSET_LEN];
   uint16_t cnt;
   uint8_t  lsb;        // least significant bit of bit field
   uint8_t  width;      // width of bit field
} tableOffsetCnt_t;

/* Structures of Commands sent to Host Meter */

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucServiceCode;
   uint16_t uiTbleID;
}tWriteFull;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucServiceCode;
   uint16_t uiTbleID;
   uint8_t  ucOffset[PSEM_OFFSET_LEN];
   uint16_t uiCount;
}tWritePartial;
PACK_END

/* ------------------------------------------------------------------------------------------------------------------ */
/* NOTE:  This following structures are ONLY used by the protocol layer.  Do NOT use these for applets!!! */
PACK_BEGIN
typedef struct PACK_MID
{
   tWriteFull sWriteFull PACK_MID;
   uint16_BA uiCount;
   uint8_t sTxBuffer[HMC_MSG_MAX_TX_LEN];
}tWriteFullWithBuffer;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   tWritePartial sWritePartial PACK_MID;
   uint8_t sTxBuffer[HMC_MSG_MAX_TX_LEN];
}tWritePartialWithBuffer;
PACK_END
/* NOTE:  The preceeding structures are ONLY used by the protocol layer.  Do NOT use these for applets!!! */
/* ------------------------------------------------------------------------------------------------------------------ */

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucServiceCode;
   uint16_t uiTbleID;
   uint16_t ucObjectIndex[PSEM_OBJECT_INDEX_LEN];     /* This may have to adjusted someday. */
   uint16_t uiCount;
}tWritePartialIndex;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucServiceCode;
   uint16_t uiTbleID;
}tReadFull;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucServiceCode;
   uint16_t uiTbleID;
   uint8_t  ucOffset[PSEM_OFFSET_LEN];
   uint16_t uiCount;
}tReadPartial;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucServiceCode;
   uint16_t uiTbleID;
   uint16_t ucObjectIndex[PSEM_OBJECT_INDEX_LEN];     /* This may have to adjusted someday. */
   uint16_t uiCount;
}tReadPartialIndex;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucServiceCode;
   uint16_t uiUserID;
   uint8_t  ucUserName[PSEM_USER_NAME_LEN];
}tLogOn;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucServiceCode;
   uint8_t  ucPassword[PSEM_PASSWORD_LEN];
}tPassword;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucServiceCode;
   uint16_t uiMaxPacketSize;
   uint8_t  ucMaxPackets;
   uint8_t ucBaudRate[HMC_MSG_MAX_TX_LEN];
}tNegotiateTx0;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucServiceCode;
   uint8_t ucSeconds;
}tWait;
PACK_END

/* ------------------------------------------------------------------------------------------------------------------ */
/* structures of Responses received from the Host Meter */

PACK_BEGIN
typedef struct PACK_MID
{
/* Yeah, the #define below will only work for PIC 8-bit micros! I don't see any other good #define that does the job. */
#if	defined(__PICC18__)        /* For an 8-bit machine, unsigned values can't be larger than 8-bits. */
      unsigned tblProcNbr_L: 8;  /* Procedure number executed by end device - Lower Byte */
      unsigned tblProcNbr_H: 3;  /* Procedure number executed by end device - Upper bits */
      unsigned stdVsMfgFlag: 1;  /* 0 = STD Procedure, 1 = MFG Procedure */
      unsigned selector:     4;  /* Selector */
#else                            /* For a 16-bit or 32-bit machines */
      unsigned tblProcNbr:  11;  /* Procedure number executed by end device */
      unsigned stdVsMfgFlag: 1;  /* 0 = STD Procedure, 1 = MFG Procedure */
      unsigned selector:     4;  /* Selector */
#endif
   uint8_t    seqNbr;        /* Supplied by initiator of the proc.  Returned with response as a means of coordination. */
   uint8_t    resultCode;    /* Code that identifies the status of the procedure execution. */
   uint8_t    respDataRcd;   /* Procedure response record as defined for individual procedures in PROC_INITIATE_TBL. */
}tProcResult;                /* STD Table 8 ? Procedure Response, data inside the data part of the RX buffer */
PACK_END

PACK_BEGIN
typedef struct  PACK_MID
{
   uint16_t uiCount;         /* Returned in little endian to applet */
   uint8_t sRxBuffer[HMC_MSG_MAX_RX_LEN];
   uint8_t ucChecksum;
}tTblData;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t ucStandard;
   uint8_t ucVersion;
   uint8_t ucRevision;
   uint8_t ucReserved;
}tServiceCode;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t uiMaxPacketSize;    /* Returned in little endian to applet */
   uint8_t  ucMaxPackets;
   uint8_t  ucBaudCode;
}tNegotiate0;
PACK_END

/* ------------------------------------------------------------------------------------------------------------------ */
/* Receive Structure */

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucSTP;
   uint8_t  ucId;
   uint8_t  ucControl;
   uint8_t  ucSequenceNum;
   uint16_BA uiPacketLength;     /* In little endian format when reported to the applet */
   uint8_t ucResponseCode;
   union /* Struct for each response type, none for the NILL because it is empty */
   {
      tTblData       sTblData;
      tServiceCode   sServiceCode;
      tNegotiate0    sNegotiate0;
      uint8_t        sRxData[sizeof(tTblData)];
   }uRxData;
   uint16_BA uiPacketCRC16;
   uint8_t ucRespType;
}sMtrRxPacket;
PACK_END

/* ------------------------------------------------------------------------------------------------------------------ */
/* Transmit Structure */

/* All data is in big endian format for the write structure */
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucSTP;
   uint8_t  ucId;
   uint8_t  ucControl;
   uint8_t  ucSequenceNum;
   uint16_BA uiPacketLength;
   union tagRxData /* A struct for every ServiceCode sent to the Meter */
   {
      tReadFull               sReadFull;
      tReadPartial            sReadPartial;
      tReadPartialIndex       sReadPartialIndex;
      tWriteFullWithBuffer    sWriteFullWithBuffer;
      tWritePartialWithBuffer sWritePartialWithBuffer;
      tWritePartialIndex      sWritePartialIndex;
      tLogOn                  sLogOn;
      tPassword               sPassword;
      tNegotiateTx0           sNegotiate0;
      tWait                   sWait;
      uint8_t                   sTxData[sizeof(tWritePartial)];
   }uTxData;
   uint16_BA uiPacketCRC16;
}sMtrTxPacket;
PACK_END

/* ------------------------------------------------------------------------------------------------------------------ */
/* Full Rx/Tx Structure with command table pointer */

PACK_BEGIN
typedef struct PACK_MID    /* Used for all applets and communication layers of code.  */
{
   sMtrTxPacket TxPacket;
   sMtrRxPacket RxPacket;
   void far *pCommandTable;   /* This is always a MTR_PROTO_Table *, but that is defined in mtr_prot.h
                                 cross-including the two files, psem.h and mtr_prot.h, doesn't work   */
}HMC_COM_INFO;
PACK_END

/* ANSI C12.18 Packet */

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  ucSTP;
   uint8_t  ucId;
   uint8_t  ucControl;
   uint8_t  ucSequenceNum;
   uint16_BA uiPacketLength;     /* In little endian format when reported to the applet */
   uint8_t  cmd;
   uint8_t  sRxData[sizeof(tTblData)];
   uint16_BA uiPacketCRC16;
} sC1218Packet;
PACK_END

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#endif

