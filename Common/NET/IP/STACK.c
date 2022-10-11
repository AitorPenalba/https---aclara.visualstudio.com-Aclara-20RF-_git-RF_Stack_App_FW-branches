/******************************************************************************
 *
 * Filename: STACK.c
 *
 * Global Designator: NWK_
 *
 * Contents: This module contains functionality to receive and parse payloads containing IP/UDP/DTLS headers, and to
 *             encode messages adding these headers.
 *
 ******************************************************************************
 * Copyright (c) 2014 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include <stdlib.h>       // rand()
#include <stdbool.h>
#include <math.h>
#include <limits.h> /* only for CHAR_BIT */
#include <string.h>
#include "pack.h"
#include "time_util.h"
#include "timer_util.h"
#include "DBG_SerialDebug.h"
#include "MAC_Protocol.h"
#include "file_io.h"
#include "pwr_task.h"

#include "eng_res.h"

//#if ( EP == 1 )
#include "smtd_config.h"
//#endif

#define TEST_MODE 0

#if ( DCU == 1 )
#include "version.h"
#include "MAINBD_Handler.h"
#endif

#if ( FAKE_TRAFFIC == 1 )
#include "PHY.h"
#endif
#include "PHY_Protocol.h"

#include "MAC_Protocol.h"
#include "MAC.h"

#include "STACK_Protocol.h"
#include "STACK.h"

// Attributes

#define HE_CONTEXT_DEFAULT                          0
#if ( FAKE_TRAFFIC == 1 )
#define TEST_RF_DUTYCYCLE_DEFAULT                   1
#define TEST_BH_GEN_COUNT_DEFAULT                  15
#endif
#if ( EP == 1 )
#define ROUTING_MAX_PACKET_COUNT_DEFAULT           20L
#define ROUTING_ENTRY_EXPIRATION_THRESHOLD_DEFAULT 24
#endif

#if ( HE == 1 )
#define PACKET_HISTORY_DEFAULT                    168
#define PACKET_ROUTING_HISTORY_DEFAULT             24
#define ROUTING_MAX_DCU_COUNT_DEFAULT               3
#define ROUTING_FREQUENCY_DEFAULT                  15
#define MULTIPATH_FORWARDING_ENABLE_DEFAULT     false
#endif


/* #DEFINE DEFINITIONS */
#define NWK_INVALID_HANDLE_ID 0

#define BASE_UDP_PORT 61616

/* MACRO DEFINITIONS */
/*lint -esym(750,IP_VERSION_SIZE,SRC_ADDR_COMPRESSION_SIZE,DST_ADDR_COMPRESSION_SIZE,QOS_SIZE)   */
/*lint -esym(750,NXT_HDR_PRESENT_SIZE,IPV6_SIZE,EXTENSION_ID_SIZE,CONTEXT_SIZE,DST_MULTI_SIZE,NEXT_HEADER_SIZE)   */
/*lint -esym(750,UDP_PORT_SIZE,SUBTYPE_SIZE,NEXT_HOP_RESERVED_SIZE,PERSIST_SIZE,HOP_COUNT_SIZE,ADDRESS_TYPE_SIZE)   */
/*lint -esym(750,MIN_IP_MSG_SIZE)   */
#define IP_VERSION_SIZE               4    /*!< Size of the Version field */
#define SRC_ADDR_COMPRESSION_SIZE     2    /*!< Size of the source address compression field */
#define DST_ADDR_COMPRESSION_SIZE     3    /*!< Size of the destination address compression field */
#define QOS_SIZE                      6    /*!< Size of the quality of service field */
#define NXT_HDR_PRESENT_SIZE          1    /*!< Size of the next header present field */
#define IPV6_SIZE                     128  /*!< Size of the IPv6 address field */
#define EXTENSION_ID_SIZE             40   /*!< Size of the source extension identifier */
#define CONTEXT_SIZE                  8    /*!< Size of the source context field */
#define DST_MULTI_SIZE                48   /*!< Size of the destination multicast field */
#define NEXT_HEADER_SIZE              8    /*!< Size of the next header field */
#define UDP_PORT_SIZE                 4    /*!< Size of a UDP Port field */
#define SUBTYPE_SIZE                  8    /*!< Size of the subtype field */
#define NEXT_HOP_RESERVED_SIZE        3    /*!< Size of the next hop reserved field */
#define PERSIST_SIZE                  1    /*!< Size of the persist field */
#define HOP_COUNT_SIZE                2    /*!< Size of the hop count field */
#define ADDRESS_TYPE_SIZE             1    /*!< Size of the address type field */

#define MIN_IP_MSG_SIZE (IP_VERSION_SIZE + SRC_ADDR_COMPRESSION_SIZE + DST_ADDR_COMPRESSION_SIZE + QOS_SIZE + NXT_HDR_PRESENT_SIZE)
#define MAX_IP_MSG_SIZE ((IP_VERSION_SIZE + SRC_ADDR_COMPRESSION_SIZE + DST_ADDR_COMPRESSION_SIZE + QOS_SIZE + NXT_HDR_PRESENT_SIZE + IPV6_SIZE + IPV6_SIZE + NEXT_HEADER_SIZE + UDP_PORT_SIZE + UDP_PORT_SIZE )/8)

/* time of day to send stats:  22:59:45 */
#if ( FAKE_TRAFFIC == 1 )
#define FAKE_SRFN_MESSAGE_SIZE 15
#endif

#define TICK_VAL_TO_SEND_STATS ((TIME_TICKS_PER_HR*22)+(TIME_TICKS_PER_MIN*59)+(TIME_TICKS_PER_SEC*45))

// List of supported next header subtype
#define NEXT_HEADER_NEXT_HOP_LIST_SUBTYPE 0

#if( RTOS_SELECTION == FREE_RTOS )
#define NWK_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define NWK_NUM_MSGQ_ITEMS 0
#endif


/* TYPE DEFINITIONS */


/* CONSTANTS */

/* for now pick a private address (FC00::MAC address).  Note:  real mac address modifies the last 5 bytes
of this array */
static uint8_t thisMtuIPAddress[] = {0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x77, 0x77, 0x77, 0x77, 0x77};

/* These are used for printing messages */
static const char * const nwk_attr_names[] = /* Don't forget to update the size using the length of the longest
                                                element + 1.  */
{
#if ( EP == 1 )
   [eNwkAttr_invalidAddrModeCount              ] = "invalidAddressModeCount",
#endif
   [eNwkAttr_ipIfInDiscards                    ] = "ipIfInDiscards",
   [eNwkAttr_ipIfInErrors                      ] = "ipIfInErrors",
   [eNwkAttr_ipIfInMulticastPkts               ] = "ipIfInMulticastPkts",
   [eNwkAttr_ipIfInOctets                      ] = "ipIfInOctets",
   [eNwkAttr_ipIfInPackets                     ] = "ipIfInPackets",
   [eNwkAttr_ipIfInUcastPkts                   ] = "ipIfInUcastPkts",
   [eNwkAttr_ipIfInUnknownProtos               ] = "ipIfInUnknownProtos",
   [eNwkAttr_ipIfLastResetTime                 ] = "ipIfLastResetTime",
   [eNwkAttr_ipIfOutDiscards                   ] = "ipIfOutDiscards",
   [eNwkAttr_ipIfOutErrors                     ] = "ipIfOutErrors",
   [eNwkAttr_ipIfOutMulticastPkts              ] = "ipIfOutMulticastPkts",
   [eNwkAttr_ipIfOutOctets                     ] = "ipIfOutOctets",
   [eNwkAttr_ipIfOutPackets                    ] = "ipIfOutPackets",
   [eNwkAttr_ipIfOutUcastPkts                  ] = "ipIfOutUcastPkts",
   [eNwkAttr_ipLowerLinkCount                  ] = "ipLowerLinkCount",
   [eNwkAttr_ipUpperLayerCount                 ] = "ipUpperLayerCount",
#if ( EP == 1 )
   [eNwkAttr_ipRoutingMaxPacketCount           ] = "ipRoutingMaxPacketCount",
   [eNwkAttr_ipRoutingEntryExpirationThreshold ] = "ipRoutingEntryExpirationThreshold",
#endif
   [eNwkAttr_ipHEContext                       ] = "ipHEContext",
   [eNwkAttr_ipState                           ] = "ipState",
#if ( FAKE_TRAFFIC == 1 )
   [eNwkAttr_ipRfDutyCycle                     ] = "ipRfDutyCycle",
   [eNwkAttr_ipBhaulGenCount                   ] = "ipBhaulGenCount",
#endif
#if ( HE == 1 )
   [eNwkAttr_ipPacketHistory                   ] = "ipPacketHistory",
   [eNwkAttr_ipRoutingHistory                  ] = "ipRoutingHistory",
   [eNwkAttr_ipRoutingMaxDcuCount              ] = "ipRoutingMaxDcuCount",
   [eNwkAttr_ipRoutingFrequency                ] = "ipRoutingFrequency",
   [eNwkAttr_ipMultipathForwardingEnabled      ] = "ipMultipathForwardingEnabled",
#endif
};


static NWK_CachedAttr_t CachedAttr;

typedef struct
{
   uint8_t  ipHEContext;                              /*!< Specifies the IP Context for the Head End.*/
#if ( DCU == 1 )
   uint8_t  ipPacketHistory;                          /*!< Specifies the amount of packet history in hours stored at the
                                                        head end. */
   uint8_t  ipRoutingHistory;                         /*!<  Specifies the amount of packet history in hours included in
                                                        HE Best DCU routing calculations (Section 3.3.2).  Only packets
                                                        from the packet history received by a DCU within
                                                        ipRoutingHistory hours are used in the RSSI calculation. */
   uint8_t  ipRoutingMaxDcuCount;                     /*!< Specifies the number of DCUs per EP with the highest computed
                                                        average RSSI values that are saved in the routing table */
   uint16_t ipRoutingFrequency;                       /*!< Specifies the frequency that the HE Best DCU routing runs in
                                                        minutes (Section 3.3.2). */
   bool     ipMultipathForwardingEnabled;             /*!< Specifies if multipath forwarding is enabled system wide. The
                                                        number of paths is determined based on the traffic class of the
                                                        packet.*/
#endif
#if ( EP == 1 )
   uint8_t  ipRoutingEntryExpirationThreshold;        /*!< The amount of time that a routing table entry can stay exist
                                                        before it is considered expired. */
   uint8_t  ipRoutingMaxPacketCount;                  /*!< The number of previous packets received that the EP routing
                                                        will keep track of. */
#endif
#if ( FAKE_TRAFFIC == 1 )
   uint8_t  ipRfDutyCycle;                            /*!< Temporarily changed to hold RF duty cycle (1-10%) */
   uint8_t  ipBhaulGenCount;                          /*!< The number of fake data records to generate  per second*/
#else
   uint8_t  reserved;                                 /*!< Was Application security/authorization mode; moved to another file*/
#endif
}ConfigAttr_t;

static ConfigAttr_t ConfigAttr;

typedef struct
{
   FileHandle_t          handle;
   char           const *FileName;         /*!< File Name           */
   ePartitionName const  ePartition;       /*!<                     */
   filenames_t    const  FileId;           /*!< File Id             */
   void*          const  Data;             /*!< Pointer to the data */
   lCnt           const  Size;             /*!< Size of the data    */
   FileAttr_t     const  Attr;             /*!< File Attributes     */
   uint32_t       const  UpdateFreq;       /*!< Update Frequency    */
} nwk_file_t;


// NWK Configuration Data File
static nwk_file_t ConfigFile =
{
   .ePartition      = ePART_SEARCH_BY_TIMING,
   .FileName        = "NWK_CONFIG",
   .FileId          = eFN_NWK_CONFIG,               // Configuration Data
   .Attr            = FILE_IS_NOT_CHECKSUMED,
   .Data            = &ConfigAttr,
   .Size            = sizeof(ConfigAttr),
   .UpdateFreq      = 0xFFFFFFFF                    // Updated Seldom
}; /*lint !e785 too few initializers   */

// NWK Cached Data File
static nwk_file_t CachedFile =
{
   .ePartition      = ePART_SEARCH_BY_TIMING,
   .FileName        = "NWK_CACHED",
   .FileId          = eFN_NWK_CACHED,               // Cached Data
   .Attr            = FILE_IS_NOT_CHECKSUMED,
   .Data            = &CachedAttr,
   .Size            = sizeof(CachedAttr),
   .UpdateFreq      = 0                            // Updated Often
}; /*lint !e785 too few initializers   */

static nwk_file_t * const Files[2] =
{
   &ConfigFile,
   &CachedFile
};


/* FILE VARIABLE DEFINITIONS */
static OS_MSGQ_Obj  NWK_msgQueue;
static OS_MUTEX_Obj NWK_AttributeMutex_;
static OS_SEM_Obj   NWK_AttributeSem_;

static NWK_Confirm_t ConfirmMsg;
static NWK_Confirm_t * const pNwkConf = &ConfirmMsg;
#if ( FAKE_TRAFFIC != 1 )
static HEEP_APPHDR_t loglTxInfo;                /* Application header/QOS info */
#endif
static buffer_t* pLogMessage = NULL;
static uint16_t uLogTimerId = 0;                /* Timer ID used by time diversity. */

/* FUNCTION PROTOTYPES */
static void NWK_Reset(NWK_RESET_e type);

#if ( DCU == 1 )
static void NWK_ProcessHeadEndRequest ( NWK_DataReq_t const *stack_buffer );
#endif

/* There is a handler for each port. Currently, only ports 0 and 1 are supported for non-dtls and dtls.  */
static NWK_IndicationHandler pIndicationHandler_callback[ 16 ] =
{
   NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL
};

static void NWK_ProcessDataIndication( buffer_t const *indication);
#if ( FAKE_TRAFFIC != 1 )
static void STK_MGR_SendStats( int type, uint32_t uTimeDiversity);
#endif

static void STK_MGR_LogTimerExpired(uint8_t cmd, void *pData);
static void STK_MGR_SetLogTimer(uint32_t timeMilliseconds);

#if ( TEST_MODE == 1 ) || ( FAKE_TRAFFIC == 1 )
static void NWK_SendTestMsg( void );
#endif

static void ConfigAttr_Init(bool bWrite);
static void CachedAttr_Init(bool bWrite);

// Network Layer Management
static bool Process_StartRequest(NWK_StartReq_t const *pReq);
static bool Process_ResetRequest(NWK_ResetReq_t const *pReq);
static bool Process_StopRequest( NWK_StopReq_t  const *pReq);
static bool Process_GetRequest(  NWK_GetReq_t   const *pReq);
static bool Process_SetRequest(  NWK_SetReq_t   const *pReq);
static bool Process_DataRequest( NWK_DataReq_t  const *pReq);

static bool Process_SetConfirm(  MAC_SetConf_t   const *pConfirm  );
static bool Process_GetConfirm(  MAC_GetConf_t   const *pConfirm  );
static bool Process_ResetConfirm(MAC_ResetConf_t const *pResetConf);
#if ( EP == 1 )
static bool Process_TimeQueryConfirm( MAC_TimeQueryConf_t  const *pTimeConf );
#endif
static bool Process_DataConfirm( MAC_DataConf_t  const *pDataConf );
static bool Process_PurgeConfirm(MAC_PurgeConf_t const *pPurgeConf);
static bool Process_FlushConfirm(MAC_FlushConf_t const *pFlushConf);
static bool Process_PingConfirm( MAC_PingConf_t  const *pPingConf );

static char* NWK_GetStatusMsg(NWK_GET_STATUS_e status);
static char* NWK_SetStatusMsg(NWK_SET_STATUS_e status);
static char* NWK_ResetStatusMsg(NWK_RESET_STATUS_e status);
static char* NWK_StartStatusMsg(NWK_START_STATUS_e status);
static char* NWK_StopStatusMsg(NWK_STOP_STATUS_e status);
static char* NWK_DataStatusMsg(NWK_DATA_STATUS_e status);

#if ( FAKE_TRAFFIC == 1)
static void STACK_RxFakeRecords( void );
#endif

static char* NWK_StateMsg(NWK_STATE_e state);

static NWK_SET_STATUS_e NWK_Attribute_Set(NWK_SetReq_t const *pSetReq); /* Prototype should be only used by stack.c.  We
                                                                           don't want outsiders to call this function
                                                                           directly.  NWK_SetRequest is used for that.
                                                                           */
static NWK_GET_STATUS_e NWK_Attribute_Get(NWK_GetReq_t const *pGetReq, NWK_ATTRIBUTES_u *val);  /* Prototype should be
                                                                                                   only used by stack.c.
                                                                                                   We don't want
                                                                                                   outsiders to call
                                                                                                   this function
                                                                                                   directly.
                                                                                                   NWK_GetRequest is
                                                                                                   used for that. */

static uint32_t NextRequestHandle(void);

static NWK_STATE_e _state = eNWK_STATE_IDLE;
/***********************************************************************************************************************
Function Name: NWK_Request

Purpose: Called by upper layer to send a request to NWK, also registered with MAC layer as handler for indications

Arguments: request - buffer that contains a stack request or indication at request->data

Returns: None
***********************************************************************************************************************/
void NWK_Request(buffer_t *request)
{
   OS_MSGQ_Post(&NWK_msgQueue, (void *)request); // Function will not return if it fails
}

/*!
 */
static void NWK_Confirm(MAC_Confirm_t const *confirm)
{
   // Allocate the buffer
   buffer_t *pBuf = BM_allocStack(sizeof(MAC_Confirm_t));   // Allocate the buffer
   if(pBuf != NULL )
   {
      // Set the type
      pBuf->eSysFmt = eSYSFMT_MAC_CONFIRM;

      MAC_Confirm_t *pConf = (MAC_Confirm_t *)pBuf->data; /*lint !e740 !e826 */
      (void) memcpy(pConf, confirm, sizeof(MAC_Confirm_t) );
      OS_MSGQ_Post(&NWK_msgQueue, (void *)pBuf); // Function will not return if it fails
   }
}

/***********************************************************************************************************************
Function Name: NWK_init

Purpose: Initialize this module before tasks start running in the system

Arguments: none

Returns: returnStatus_t - success or failure
***********************************************************************************************************************/
returnStatus_t NWK_init ( void )
{
   returnStatus_t retVal = eFAILURE;
#if ( FAKE_TRAFFIC == 1 )
   tTimeSysPerAlarm alarmSettings;
#endif
   //TODO RA6: NRJ: determine if semaphores need to be counting
   if (OS_SEM_Create(&NWK_AttributeSem_, 0) && OS_MUTEX_Create(&NWK_AttributeMutex_) && OS_MSGQ_Create(&NWK_msgQueue, NWK_NUM_MSGQ_ITEMS, "NWK"))
   {
      // Reset the configuration attributes to default values
      ConfigAttr_Init((bool) false);

      // Clear the counter values, and set the Last Update Time
      CachedAttr_Init((bool) false);

      {  // Normal Mode
         uint8_t i;
         FileStatus_t fileStatus;
         for( i=0; i < (sizeof(Files)/sizeof(*(Files))); i++ )
         {
            nwk_file_t *pFile = Files[i];

            if ( eSUCCESS == FIO_fopen(&pFile->handle,       /* File Handle so that PHY access the file. */
                                        pFile->ePartition,   /* Search for best partition according to the timing. */
                             (uint16_t) pFile->FileId,       /* File ID (filename) */
                                        pFile->Size,         /* Size of the data in the file. */
                                        pFile->Attr,         /* File attributes to use. */
                                        pFile->UpdateFreq,   /* The update rate of the data in the file. */
                                        &fileStatus) )            /* Contains the file status */
            {
               if ( fileStatus.bFileCreated )
               {  // The file was just created for the first time.
                  // Save the default values to the file
                  retVal = FIO_fwrite( &pFile->handle, 0, pFile->Data, pFile->Size);
               }
               else
               {  // Read the STACK Cached File Data
                  retVal = FIO_fread( &pFile->handle, pFile->Data, 0, pFile->Size);
               }
            }
            if (eFAILURE == retVal)
            {
               return retVal;
            }
         }
      }
      MAC_RegisterIndicationHandler(NWK_Request);
#if ( FAKE_TRAFFIC == 1 )
      //Set up a periodic alarm
      alarmSettings.bOnInvalidTime = false;                 /* Only alarmed on valid time, not invalid */
      alarmSettings.bOnValidTime = true;                    /* Alarmed on valid time */
      alarmSettings.bSkipTimeChange = true;                 /* do NOT notify on time change */
      alarmSettings.bUseLocalTime = false;                  /* do not use local time */
      alarmSettings.pQueueHandle = NULL;                    /* do NOT use queue */
      alarmSettings.pMQueueHandle = &NWK_msgQueue;          /* Uses the message Queue */
      alarmSettings.ucAlarmId = 0;                          /* 0 will create a new ID, anything else will reconfigure. */
      alarmSettings.ulOffset  = 0;                          /* Set the offset to some number of milliseconds */
      alarmSettings.ulPeriod = TIME_TICKS_PER_SEC * 1 ;     /* Wake up every second */
      (void)TIME_SYS_AddPerAlarm(&alarmSettings);
#endif
   }

   return ( retVal );
}

/***********************************************************************************************************************
Function Name: NWK_UdpPortIdToNum

Purpose: Convert the 4 bit network ID to the original UDP port value

Arguments: id - the UDP ID from the network

Returns: UDP port number
***********************************************************************************************************************/
uint16_t NWK_UdpPortIdToNum(uint8_t id)
{
   return BASE_UDP_PORT + id;
}

/***********************************************************************************************************************
Function Name: NWK_UdpPortNumToId

Purpose: Convert the UDP port value to the index used by the networking layer

Arguments: num - the UDP ID from the network

Returns: UDP ID
***********************************************************************************************************************/
uint8_t NWK_UdpPortNumToId(uint16_t num)
{
   return (uint8_t)(num - BASE_UDP_PORT);
}

static bool Process_SetConfirm(MAC_SetConf_t const *pMacConfirm  )
{
   pNwkConf->Type   = eNWK_SET_CONF;
   INFO_printf("SetConfirm:%s", MAC_SetStatusMsg(pMacConfirm->eStatus));
   pNwkConf->SetConf.eStatus = eNWK_SET_SUCCESS;
   return (bool)true;
}

/*!
 * Get Confirm from the MAC
 */
static bool Process_GetConfirm(MAC_GetConf_t const *pMacConfirm  )
{
   pNwkConf->Type   = eNWK_GET_CONF;
   INFO_printf("GetConfirm:%s", MAC_GetStatusMsg(pMacConfirm->eStatus));
   pNwkConf->GetConf.eStatus = eNWK_GET_SUCCESS;
   return (bool)true;
}

/*!
 * Reset Confirm from the MAC
 */
static bool Process_ResetConfirm(MAC_ResetConf_t const *pMacResetConf)
{
   pNwkConf->Type   = eNWK_RESET_CONF;
   INFO_printf("ResetConfirm:%s", MAC_ResetStatusMsg(pMacResetConf->eStatus));
//   eNWK_RESET_SUCCESS = 0,       /*!< NWK RESET - Success */
//   eNWK_RESET_FAILURE            /*!< NWK RESET - Failure */
   pNwkConf->ResetConf.eStatus = eNWK_RESET_SUCCESS;
   return (bool)true;
}

#if ( EP == 1 )
static bool Process_TimeQueryConfirm(MAC_TimeQueryConf_t const *pTimeConf)
{
   INFO_printf("TimeQueryConfirm: %s", MAC_TimeQueryStatusMsg(pTimeConf->eStatus));
   return (bool)false;
}
#endif

/*!
 *  Data Confirm from the MAC
 */
static bool Process_DataConfirm(MAC_DataConf_t const *pDataConf)
{
   pNwkConf->Type = eNWK_DATA_CONF;

   INFO_printf("DataConfirm:%s, handle=%d", MAC_DataStatusMsg(pDataConf->status), pDataConf->handle);
   if(pDataConf->status != eMAC_DATA_SUCCESS)
   {
      pNwkConf->DataConf.status = eNWK_DATA_TRANSACTION_FAILED;
      NWK_CounterInc(eNWK_ipIfOutErrors, 1, RF_LINK);
   }else
   {
      pNwkConf->DataConf.status = eNWK_DATA_SUCCESS;
      NWK_CounterInc(eNWK_ipIfOutOctets, pDataConf->payloadLength, RF_LINK);
      NWK_CounterInc(eNWK_ipIfOutPackets, 1, RF_LINK);
   }
   return (bool)true;
}

/*!
 *  Purge Confirm from the MAC
 */
static bool Process_PurgeConfirm(MAC_PurgeConf_t const *pPurgeConf)
{
   /*TODO: not currently used. Add proper actions when implemented.  */
   INFO_printf("PurgeConfirm:%s", MAC_PurgeStatusMsg(pPurgeConf->eStatus), pPurgeConf->handle);
   return (bool)true;
}

/*!
 *  Flush Confirm from the MAC
 */
static bool Process_FlushConfirm(MAC_FlushConf_t const *pFlushConf)
{
   /*TODO: not currently used. Add proper actions when implemented.  */
   INFO_printf("FlushConfirm:%s", MAC_FlushStatusMsg(pFlushConf->eStatus));
   return (bool)true;
}

/*!
 *  Ping Confirm from the MAC
 */
static bool Process_PingConfirm(MAC_PingConf_t const *pPingConf)
{
   /*TODO: not currently used. Add proper actions when implemented.  */
   INFO_printf("PingConfirm:%s", MAC_PingStatusMsg(pPingConf->eStatus));
   return (bool)true;
}

static void SendConfirm(NWK_Confirm_t const *pConf, buffer_t const *pReqBuf)
{
   NWK_Request_t *pReq = (NWK_Request_t*) &pReqBuf->data[0];  /*lint !e740 !e826 */

   if (pReq->pConfirmHandler != NULL )
   {
      (pReq->pConfirmHandler)(pConf);
   }else
   {
   char* msg;
   switch(pConf->Type)
   {
      case eNWK_GET_CONF    :  msg=  NWK_GetStatusMsg(    pConf->GetConf.eStatus);   break;
      case eNWK_RESET_CONF  :  msg = NWK_ResetStatusMsg(  pConf->ResetConf.eStatus); break;
      case eNWK_SET_CONF    :  msg=  NWK_SetStatusMsg(    pConf->SetConf.eStatus);   break;
      case eNWK_START_CONF  :  msg = NWK_StartStatusMsg(  pConf->StartConf.eStatus); break;
      case eNWK_STOP_CONF   :  msg = NWK_StopStatusMsg(   pConf->StopConf.eStatus);  break;
//    case eNWK_TIME_CONF   :  msg = NWK_TimeStatusMsg(   pConf->TimeConf.eStatus);  break;
      case eNWK_DATA_CONF   :  msg = NWK_DataStatusMsg(   pConf->DataConf.status);    break;
      default               :  msg = "Unknown";
   }
      INFO_printf("CONFIRM:%s, handle:%lu", msg, pConf->handleId);
   }
}

/*lint -esym(715,Arg0) */
/***********************************************************************************************************************
Function Name: NWK_Task

Purpose: Task for the NWK networking layer (combination of IP/UDP/DTLS handling)

Arguments: Arg0 - Not used, but required here because this is a task

Returns: none
***********************************************************************************************************************/
void NWK_Task ( taskParameter )
{
   buffer_t *pBuf;

   buffer_t *pRequestBuf = NULL;        // Handle to unconfirmed NWK request buffers

   tTimeSysPerAlarm alarmSettings;
#if ( FAKE_TRAFFIC == 1 )
   OS_TICK_Struct currentTime;
   uint32_t TimeDiff;
   bool Overflow;
   OS_TICK_Struct lastTxTime;
   NWK_GetConf_t     GetConf;
   uint8_t i;
   _time_get_elapsed_ticks(&currentTime);
#endif

   /* configure a daily alarm to send statistics to head end at 1:00AM */
   //Set up a periodic alarm
   alarmSettings.bOnInvalidTime = false;                 /* Only alarmed on valid time, not invalid */
   alarmSettings.bOnValidTime = true;                    /* Alarmed on valid time */
   alarmSettings.bSkipTimeChange = true;                 /* do NOT notify on time change */
   alarmSettings.bUseLocalTime = false;                  /* do not use local time */
   alarmSettings.pQueueHandle = NULL;                    /* do NOT use queue */
   alarmSettings.pMQueueHandle = &NWK_msgQueue;          /* Uses the message Queue */
   alarmSettings.ucAlarmId = 0;                          /* 0 will create a new ID, anything else will reconfigure. */
   alarmSettings.ulOffset = TICK_VAL_TO_SEND_STATS;      /* at 22:59:45 */
   alarmSettings.ulPeriod = TIME_TICKS_PER_DAY;          /* Once per day, at 22:59:45  */
   (void)TIME_SYS_AddPerAlarm(&alarmSettings);

   for ( ;; )
   {
#if TEST_MODE == 1
      if (OS_MSGQ_Pend( &NWK_msgQueue, (void *)&pBuf, ONE_SEC))
#else
      if (OS_MSGQ_Pend( &NWK_msgQueue, (void *)&pBuf, OS_WAIT_FOREVER))
#endif
      {

         bool bConfirmReady = (bool)false;

         switch (pBuf->eSysFmt)  /*lint !e788 not all enums used within switch */
         {
            case eSYSFMT_MAC_INDICATION:
            {
#if 1
               NWK_ProcessDataIndication(pBuf);
#else
               MAC_Indication_t *pInd =  (MAC_Indication_t *)pBuf->data; /*lint !e740 !e826 */
               switch(pInd->Type)
               {
                  case eMAC_DATA_IND:
                     NWK_ProcessDataIndication(pBuf);
               break;
               }
#endif
               BM_free(pBuf);
               break;
            }

            case eSYSFMT_NWK_REQUEST:
            {
               // Request message from Network Interface
               NWK_Request_t *pReq =  (NWK_Request_t *)pBuf->data; /*lint !e740 !e826 */

               switch(pReq->MsgType)
               {
                  case    eNWK_GET_REQ    :  bConfirmReady = Process_GetRequest(  &pReq->Service.GetReq   ); break;
                  case    eNWK_RESET_REQ  :  bConfirmReady = Process_ResetRequest(&pReq->Service.ResetReq ); break;
                  case    eNWK_SET_REQ    :  bConfirmReady = Process_SetRequest(  &pReq->Service.SetReq   ); break;
                  case    eNWK_START_REQ  :  bConfirmReady = Process_StartRequest(&pReq->Service.StartReq ); break;
                  case    eNWK_STOP_REQ   :  bConfirmReady = Process_StopRequest( &pReq->Service.StopReq  ); break;
                  case    eNWK_DATA_REQ   :  bConfirmReady = Process_DataRequest( &pReq->Service.DataReq  ); break;
               }
               if(pRequestBuf == NULL)
               {  // save the request
                  pRequestBuf = pBuf;
               }else
               {  // This is not supposed to happen, but if it does, then free the buffer
                  INFO_printf("Unexpected");
                  BM_free(pRequestBuf);
                  pRequestBuf = pBuf;
               }
            }
            break;


            case eSYSFMT_TIME:
            {
#if ( FAKE_TRAFFIC == 1 )
               PHY_GetConf_t PhyGetConf;
               PhyGetConf = PHY_GetRequest( ePhyAttr_State );
               if (PhyGetConf.eStatus == ePHY_GET_SUCCESS) {
                  // Fake RX traffic. Send fake records to main board
                  if ( (PhyGetConf.val.State == ePHY_STATE_READY) || (PhyGetConf.val.State == ePHY_STATE_READY_RX) ) {
                     STACK_RxFakeRecords();
                  }

                  // Fake TX traffic
                  if ( (PhyGetConf.val.State == ePHY_STATE_READY) || (PhyGetConf.val.State == ePHY_STATE_READY_TX) ) {
                     _time_get_elapsed_ticks(&currentTime);
                     TimeDiff = (uint32_t)_time_diff_milliseconds ( &currentTime, &lastTxTime, &Overflow );
                     GetConf = NWK_GetRequest( eNwkAttr_ipRfDutyCycle );
                     /* 15 seconds minus a few ms so this runs on the 1 second wakeup */
                     if ((TimeDiff) > 14500)
                     {
                        for (i = 0; i < GetConf.val.ipRfDutyCycle; i++)
                        {
                           /* send once for 1%, twice for 2%, etc */
                           NWK_SendTestMsg();
                        }
                        lastTxTime = currentTime;
                     }
                  }
               }
#else
               uint32_t uTimeDiversity;
#if ( EP == 1 )
               uTimeDiversity = aclara_randu(0, ((uint32_t) SMTDCFG_get_log()) * TIME_TICKS_PER_MIN);
#else
               uTimeDiversity =  1;
#endif
#if TEST_MODE == 2
               uTimeDiversity =  1000; // for test
#endif
               if ( SMTDCFG_getEngBuEnabled() )
               {
                  STK_MGR_SendStats(1, uTimeDiversity);
               }
#endif
               BM_free(pBuf);
            break;
            }

            case eSYSFMT_TIME_DIVERSITY:
               (void)HEEP_MSG_Tx(&loglTxInfo, pLogMessage);    // The called function will free the buffer
               pLogMessage = NULL;
            break;

            case eSYSFMT_MAC_CONFIRM:
            {
               // Confirm message from MAC Layer
               MAC_Confirm_t *pConf = (MAC_Confirm_t *)pBuf->data; /*lint !e740 !e826 */

               switch(pConf->Type)  /*lint !e787 not all enums used within switch */
               {
                  case eMAC_GET_CONF    : bConfirmReady = Process_GetConfirm(  &pConf->GetConf)  ; break;
                  case eMAC_RESET_CONF  : bConfirmReady = Process_ResetConfirm(&pConf->ResetConf); break;
                  case eMAC_SET_CONF    : bConfirmReady = Process_SetConfirm(  &pConf->SetConf)  ; break;
#if ( EP == 1 )
                  case eMAC_TIME_QUERY_CONF   : bConfirmReady = Process_TimeQueryConfirm( &pConf->TimeQueryConf) ; break;
#endif
                  case eMAC_DATA_CONF   : bConfirmReady = Process_DataConfirm( &pConf->DataConf) ; break;

                  case eMAC_PURGE_CONF  : bConfirmReady = Process_PurgeConfirm(&pConf->PurgeConf); break;
                  case eMAC_FLUSH_CONF  : bConfirmReady = Process_FlushConfirm(&pConf->FlushConf); break;
                  case eMAC_PING_CONF   : bConfirmReady = Process_PingConfirm (&pConf->PingConf) ; break;
                  case eMAC_START_CONF  : bConfirmReady = false; break;
                  case eMAC_STOP_CONF   : bConfirmReady = false; break;
               }  /*lint !e787 not all enums used within switch */
               BM_free(pBuf);
               break;
            }

            default:
               BM_free(pBuf);
               break;
         }  /*lint !e788 not all enums used within switch */

         // Determine if there is a NWK Confirmation that was create by either the NWK_Request or MAC Confirm
         // Only one confirmation can be created each time a NWK Request or MAC Confirm is processed
         if(bConfirmReady )
         {  // we got a confirmation for previous request, so send confirm and free it
            if(pRequestBuf != NULL)
            {
               NWK_Request_t *pReq = (NWK_Request_t *) pRequestBuf->data;  /*lint !e740 !e826 */
               ConfirmMsg.handleId = pReq->handleId;
               SendConfirm(&ConfirmMsg, pRequestBuf);
               BM_free(pRequestBuf);
               pRequestBuf = NULL;
            }
         }
      }
#if TEST_MODE == 1
      else
      {
         /* print test message once per second for field testing */
         NWK_SendTestMsg();
      }
#endif
   }
}
/*lint +esym(715,Arg0) */


/*!
   This is a temporary function to handle data confirmation from the MAC
   This is called in the context of the MAC, rather than via a message send to stack
 */
#if 0
static void DataConfirm_Handler(MAC_DATA_STATUS_e status, uint16_t handle)
{
   INFO_printf("STACK:DataConfirm:%s, handle=%d", MAC_DataStatusMsg(status), handle);
   if(status != eMAC_DATA_SUCCESS)
   {
      NWK_CounterInc(eNWK_ipIfOutErrors, RF_LOWER_LINK);
   }
}
#endif

/***********************************************************************************************************************
Function Name: Process_DataRequest

Purpose: This function will process a DATA.request

Arguments: req - pointer to a NWK layer DATA.request

Returns: true if this function updated pNwkConf to contain confirmation details, false if the caller should not generate
            a confirm using information written to pNwkConf.
***********************************************************************************************************************/
static bool Process_DataRequest( NWK_DataReq_t  const *req)
{
   NWK_DataReq_t const *stack_request = req;

   pNwkConf->Type = eNWK_DATA_CONF;

   uint16_t bitNo = 0;

   // Is the NWK Ready???
   if(_state == eNWK_STATE_OPERATIONAL)
   {
      if (stack_request->payloadLength <= MAX_NETWORK_PAYLOAD)
      {
         if ( (stack_request->dstAddress.addr_type == eCONTEXT) ||
              (stack_request->dstAddress.addr_type == eEXTENSION_ID) )
         {
            NWK_CounterInc(eNWK_ipIfOutUcastPkts, 1, stack_request->port);
         }
         else if (stack_request->dstAddress.addr_type == eMULTICAST)
         {
            NWK_CounterInc(eNWK_ipIfOutMulticastPkts, 1, stack_request->port);
         }
#if ( DCU == 1 )
         if ( ( (stack_request->dstAddress.addr_type == eCONTEXT) && (stack_request->override.linkOverride == eNWK_LINK_OVERRIDE_NULL) ) || // Send to backhaul the normal way
                (stack_request->override.linkOverride == eNWK_LINK_OVERRIDE_BACKHAUL) )                                                     // Force to backhaul
         {
            NWK_ProcessHeadEndRequest( stack_request );
            return (bool)true;
         }
         else
#endif
         {
            if ( ((stack_request->override.linkOverride == eNWK_LINK_OVERRIDE_NULL) && // No override. Validate destination address
                 ((stack_request->dstAddress.addr_type == eCONTEXT) ||
                  (stack_request->dstAddress.addr_type == eEXTENSION_ID) ||
                  (stack_request->dstAddress.addr_type == eMULTICAST) ||
                  (stack_request->dstAddress.addr_type == eELIDED))) ||
                 ((stack_request->override.linkOverride         == eNWK_LINK_OVERRIDE_TENGWAR_MAC)   &&             // Override. This test will force an EP to discard the request when linkOverride is eNWK_LINK_OVERRIDE_BACKHAUL
                  (stack_request->override.linkSettingsOverride != eNWK_LINK_SETTINGS_OVERRIDE_NULL) &&             // Make sure settings override is valid
                  (stack_request->dstAddress.addr_type  == eEXTENSION_ID) ||                                        // Always accept eEXTENSION_ID because it can always be unicasted or broadcasted
                  ((stack_request->dstAddress.addr_type != eEXTENSION_ID) &&                                        // However, if it is a broadcast type address,
                   ((stack_request->override.linkSettingsOverride == eNWK_LINK_SETTINGS_OVERRIDE_BROADCAST_OB) ||   // it can only be overriten to broadcast.
                    (stack_request->override.linkSettingsOverride == eNWK_LINK_SETTINGS_OVERRIDE_BROADCAST_IB)))) ) // This is because we don't have the MAC address needed to unicast so we can't do that.
            {
               // Allocate a temporary buffer to build the payload
               buffer_t *pBuf = BM_allocStack( stack_request->payloadLength + MAX_IP_MSG_SIZE);
               if(pBuf != NULL)
               {
                  uint8_t *payload = (uint8_t*)pBuf->data;

                  bool                 ackRequired;
                  uint8_t              MsgPriority;
                  bool                 droppable;
                  MAC_Reliability_e    reliability;
                  NWK_Address_t        dstAddress = stack_request->dstAddress;
#if ( DCU == 1 )
                  MAC_CHANNEL_SET_INDEX_e channelSetIndex = eMAC_CHANNEL_SET_INDEX_1;
#else
                  MAC_CHANNEL_SET_INDEX_e channelSetIndex = eMAC_CHANNEL_SET_INDEX_2;
#endif
                  (void) NWK_QosToParams(stack_request->qos,
                                &ackRequired,
                                &MsgPriority,
                                &droppable,
                                &reliability);

                  bitNo = PACK_uint8_2bits( (uint8_t []) { IP_VERSION }             , IP_VERSION_SIZE,           payload, bitNo);
                  bitNo = PACK_uint8_2bits( (uint8_t []) { (uint8_t)eELIDED}        , SRC_ADDR_COMPRESSION_SIZE, payload, bitNo);

                  if ((stack_request->dstAddress.addr_type == eEXTENSION_ID) ||
                      (stack_request->dstAddress.addr_type == eELIDED))
                  {
                     bitNo = PACK_uint8_2bits((uint8_t []) {(uint8_t) eELIDED }     , DST_ADDR_COMPRESSION_SIZE, payload, bitNo);
                     bitNo = PACK_uint8_2bits(&stack_request->qos                   , QOS_SIZE                 , payload, bitNo);
                     bitNo = PACK_uint8_2bits((uint8_t []) {0}                      , NXT_HDR_PRESENT_SIZE     , payload, bitNo);
                  }
                  else if (stack_request->dstAddress.addr_type == eMULTICAST)
                  {
                     bitNo = PACK_uint8_2bits((uint8_t []) {(uint8_t) eMULTICAST}   , DST_ADDR_COMPRESSION_SIZE, payload, bitNo);
                     bitNo = PACK_uint8_2bits(&stack_request->qos                   , QOS_SIZE                 , payload, bitNo);
                     bitNo = PACK_uint8_2bits((uint8_t []) {0}                      , NXT_HDR_PRESENT_SIZE     , payload, bitNo);
                     bitNo = PACK_addr(stack_request->dstAddress.multicastAddr      , MAC_ADDRESS_SIZE         , payload, bitNo);
                     // hack last byte of multicast address in
                     bitNo = PACK_uint8_2bits((uint8_t []) {(uint8_t) stack_request->dstAddress.multicastAddr[5] } , 8, payload, bitNo);
                  }
                  else if (stack_request->dstAddress.addr_type == eCONTEXT)
                  {
                     bitNo = PACK_uint8_2bits((uint8_t []) {(uint8_t) eCONTEXT }    , DST_ADDR_COMPRESSION_SIZE, payload, bitNo);
                     bitNo = PACK_uint8_2bits(&stack_request->qos                   , QOS_SIZE                 , payload, bitNo);
                     bitNo = PACK_uint8_2bits((uint8_t []) {0}                      , NXT_HDR_PRESENT_SIZE     , payload, bitNo);
                     bitNo = PACK_uint8_2bits(&stack_request->dstAddress.context    , CONTEXT_SIZE             , payload, bitNo);
                  }

                  /* udp source port */
                  bitNo = PACK_uint8_2bits( &stack_request->port                    , UDP_PORT_SIZE            , payload, bitNo);

                  /* udp destination port */
                  bitNo = PACK_uint8_2bits( &stack_request->port                    , UDP_PORT_SIZE            , payload, bitNo);

                  /* xxx jmb:  warning, shortcut for speed, assumes payload portion starts byte aligned */
                  // Copy the stack payload to the payload buffer
                  (void)memcpy(&payload[(bitNo/8)], stack_request->data,    stack_request->payloadLength);

                  // Override some settings if needed
                  if ( (stack_request->override.linkOverride         == eNWK_LINK_OVERRIDE_TENGWAR_MAC) &&
                       (stack_request->override.linkSettingsOverride != eNWK_LINK_SETTINGS_OVERRIDE_NULL) ) {
                     switch (stack_request->override.linkSettingsOverride ) {
                        case eNWK_LINK_SETTINGS_OVERRIDE_UNICAST_OB:   dstAddress.addr_type = eEXTENSION_ID;            // Force MAC to unicast
                                                                       channelSetIndex      = eMAC_CHANNEL_SET_INDEX_1;
                                                                       break;

                        case eNWK_LINK_SETTINGS_OVERRIDE_UNICAST_IB:   dstAddress.addr_type = eEXTENSION_ID;            // Force MAC to unicast
                                                                       channelSetIndex      = eMAC_CHANNEL_SET_INDEX_2;
                                                                       break;

                        case eNWK_LINK_SETTINGS_OVERRIDE_BROADCAST_OB: dstAddress.addr_type = eMULTICAST;               // Force MAC to broadcast
                                                                       channelSetIndex      = eMAC_CHANNEL_SET_INDEX_1;
                                                                       break;

                        case eNWK_LINK_SETTINGS_OVERRIDE_BROADCAST_IB: dstAddress.addr_type = eMULTICAST;               // Force MAC to broadcast
                                                                       channelSetIndex      = eMAC_CHANNEL_SET_INDEX_2;
                                                                       break;

                        case eNWK_LINK_SETTINGS_OVERRIDE_NULL: // Fall through
                        default: break;
                     }
                  }

                  // This function returns a handle that can be used to associate the confirmation to this request
                  (void)MAC_DataRequest_Srfn(
                           &dstAddress,
                           payload,
                           stack_request->payloadLength+(bitNo/8),
                           ackRequired,
                           MsgPriority,
                           droppable,
                           reliability,
                           channelSetIndex,
                           stack_request->callback,
                           NWK_Confirm);

                  BM_free( pBuf );

                  // Send to MAC, so confirm will be later!
                  return (bool)true;
               }else
               {  // Buffer allocation failed
                  pNwkConf->DataConf.status = eNWK_DATA_TRANSACTION_FAILED;
                  return (bool)true;
               }
            }else
            {
               NWK_CounterInc(eNWK_ipIfOutDiscards, 1, IGNORE_PARAMETER);  // Does not track by LINK
               ERR_printf("Stack layer dropped request due to unsupported destination address compression mode.");
               pNwkConf->DataConf.status = eNWK_DATA_NO_ROUTE;
               return (bool)true;
            }
         }
      }
      else
      {
         ERR_printf("Stack layer dropped request, payload length too large.");
         pNwkConf->DataConf.status = eNWK_DATA_TRANSACTION_OVERFLOW;
         return (bool)true;
      }
   }else
   {  // _state != eNWK_STATE_OPERATIONAL
      pNwkConf->DataConf.status = eNWK_DATA_MAC_IDLE;
      return (bool)true;
   }
} /* end NWK_ProcessRequest () */

#if ( DCU == 1 )
/***********************************************************************************************************************
Function Name: NWK_ProcessHeadEndRequest

Purpose: This function will add IP/UDP header to the stack data.request payload, convert the QoS/Traffic class to
   MAC expected values, and pass the resulting IP packet to the Main Board communicaiton module which expects a MAC
   indication.  Note that some MAC fields need to be overwritten.  RSSI field will be zero filled, and the MAC segment
   count will be set to the Frodo slot ID as per the 'Tengwar DCU2 Hack Spec'

   Note, this function will only act on requests destined for head end context ID.

Arguments: stack_buffer - Pointer to a buffer containing a data.request at stack_buffer->data

Returns: none

Note:  Assumes stack_buffer was previously checked to be not NULL in NWK_ProcessRequest()
***********************************************************************************************************************/
static void NWK_ProcessHeadEndRequest ( NWK_DataReq_t const *stack_request )
{
   buffer_t         *mac_buffer;
   MAC_DataInd_t    *mac_indication;
   MAC_DATA_STATUS_e txStatus = eMAC_DATA_TRANSACTION_FAILED;
   TIMESTAMP_t       CurrentTime;

   // get time
   (void)TIME_UTIL_GetTimeInSecondsFormat( &CurrentTime );

   uint16_t bitNo = 0;
   uint8_t temp;

   if ( stack_request->dstAddress.addr_type == eCONTEXT )
   {
      // Create an indication
      mac_buffer = BM_allocStack( (sizeof(MAC_DataInd_t)) + stack_request->payloadLength + MAX_IP_MSG_SIZE);
      if (mac_buffer != NULL)
      {
         mac_indication = (MAC_DataInd_t*)mac_buffer->data; /*lint !e826 data has MAC_DataInd_t */

         // todo: 7/30/2015 12:28:13 PM [BAH] - I think this should be tagged as MAC_INDICATION
         mac_buffer->eSysFmt = eSYSFMT_MAC_INDICATION;

         /* channel will be overwritten in mainbd handler code as this is from DCU */
         mac_indication->channel = 0xFF;
         mac_indication->rssi = 0;
         /* if message is generated by Frodo, segmentCount field should contain backplane slot */

         SLOT_ADDR(mac_indication->segmentCount);

         /* slot value is zero indexed and needs to be 1 indexed in packet */
         mac_indication->segmentCount++;
         mac_indication->timeStamp = CurrentTime;
         MAC_MacAddress_Get(&thisMtuIPAddress[OFFSET_TO_EUI40]);
         (void)memcpy(mac_indication->src_addr, &thisMtuIPAddress[OFFSET_TO_EUI40], MAC_ADDRESS_SIZE);

         temp = IP_VERSION;
         bitNo = PACK_uint8_2bits(&temp, IP_VERSION_SIZE, mac_indication->payload, bitNo);
         temp = (uint8_t)eELIDED;
         bitNo = PACK_uint8_2bits(&temp, SRC_ADDR_COMPRESSION_SIZE, mac_indication->payload, bitNo);

         temp = (uint8_t)eCONTEXT;
         bitNo = PACK_uint8_2bits(&temp, DST_ADDR_COMPRESSION_SIZE, mac_indication->payload, bitNo);
         temp = stack_request->qos;
         bitNo = PACK_uint8_2bits(&temp, QOS_SIZE, mac_indication->payload, bitNo);
         temp = 0;
         bitNo = PACK_uint8_2bits(&temp, NXT_HDR_PRESENT_SIZE, mac_indication->payload, bitNo);
         temp = stack_request->dstAddress.context;
         bitNo = PACK_uint8_2bits(&temp, CONTEXT_SIZE, mac_indication->payload, bitNo);

         temp = stack_request->port;
         /* udp source port */
         bitNo = PACK_uint8_2bits(&temp, UDP_PORT_SIZE, mac_indication->payload, bitNo);
         /* udp destination port */
         bitNo = PACK_uint8_2bits(&temp, UDP_PORT_SIZE, mac_indication->payload, bitNo);

         /* xxx jmb:  warning, shortcut for speed, assumes payload portion starts byte aligned */
         (void)memcpy(&mac_indication->payload[(bitNo/8)], stack_request->data, stack_request->payloadLength);
         mac_indication->payloadLength = (stack_request->payloadLength + (bitNo/8)); // 3 bytes of header

         /* final IP Header will transition from elided to extension ID so add MAC_ADDRESS_SIZE bytes */
         NWK_CounterInc(eNWK_ipIfOutOctets, mac_indication->payloadLength + MAC_ADDRESS_SIZE, BACKPLANE_LINK);
         NWK_CounterInc(eNWK_ipIfOutPackets, 1, BACKPLANE_LINK);
         MAINBD_GenerateRecord(mac_buffer, (bool)true);
         BM_free(mac_buffer);
         txStatus = eMAC_DATA_SUCCESS;
      }
      else
      {
         ERR_printf("Failed to get buffer when transmitting frodo to HE message");
      }
   }
   else
   {
      NWK_CounterInc(eNWK_ipIfOutDiscards, 1, IGNORE_PARAMETER);  // Does not track by LINK
      ERR_printf("Unexpected context ID, message dropped");
   }

   /* transmission of this packet is complete, notify the upper layer */
   if ( stack_request->callback != NULL )
   {
      (*stack_request->callback)(txStatus, stack_request->handle);
   }

} /* end NWK_ProcessHeadEndRequest () */
#endif

/***********************************************************************************************************************
Function Name: NWK_FindNextHop

Purpose: Go through a routing table to find if a packet need to go through a hop to reach destination.

Arguments: dstAddr     - Destination address to find in table
           nextHopAddr - Next hop address to reach destination address

Returns: TRUE if a hop was found

NOTE: Routing tables check is not implemented yet

***********************************************************************************************************************/
bool NWK_FindNextHop(NWK_Address_t const *dstAddr, NWK_Address_t *nextHopAddr)
{
   // nextHopAddr = dstAddr for now
   *nextHopAddr = *dstAddr;

   return true;
}

/***********************************************************************************************************************
Function Name: NWK_ElideAndSend

Purpose: This function will elide the destination address from an IP packet (if possible) and send to MAC.

Arguments: ipPacket - IP packet
           length   - Length of IP packet
           ip_frame - Pointer to a buffer containing a decoded IP frame
           dstAddr  - MAC destination address
           channetSetIndex - Index into a channel sets

Returns: none
***********************************************************************************************************************/
void NWK_ElideAndSend(uint8_t *ipPacket, uint16_t length, IP_Frame_t const *ip_frame, NWK_Address_t dstAddr, MAC_CHANNEL_SET_INDEX_e channelSetIndex)
{
   uint8_t           offset = 0;
   bool              ackRequired;
   uint8_t           MsgPriority;
   bool              droppable;
   MAC_Reliability_e reliability;
   bool              sendPkt = (bool)true;

   // Elide IP destination address if IP destination address matches MAC destination address
   if ( (dstAddr.addr_type == eEXTENSION_ID) && (ip_frame->dstAddress.addr_type == eEXTENSION_ID) &&
        (memcmp(ip_frame->dstAddress.ExtensionAddr, dstAddr.ExtensionAddr, MAC_ADDRESS_SIZE) == 0) ) {
      // Destination address compression field straddles 2 bytes.
      // Clear least significant 2 bits of first byte and most significant bit of 2nd byte, then update those bits with eELIDED
      ipPacket[0] = ((ipPacket[0] & 0xFC) | ((uint8_t)eELIDED >> 1));
      ipPacket[1] = ((ipPacket[1] & 0x7F) | (((uint8_t)eELIDED & 0x1) << 7));

      length -= MAC_ADDRESS_SIZE;
      if (ip_frame->srcAddress.addr_type == eCONTEXT)
      {
         offset = 3;
      }
      else if (ip_frame->srcAddress.addr_type == eEXTENSION_ID)
      {
         offset = 7;
      }
      if ((ip_frame->srcAddress.addr_type == eCONTEXT) ||
         (ip_frame->srcAddress.addr_type == eEXTENSION_ID))
      {
         /* remove the mac address bytes from the IP header */
         (void)memcpy(&ipPacket[offset],
            &ipPacket[(offset + MAC_ADDRESS_SIZE)],
            length);
      }
      else
      {
         DBG_logPrintf('E', "Unexpected source address compression received from Head End");
         NWK_CounterInc(eNWK_ipIfOutErrors, 1, RF_LINK);
         sendPkt = (bool)false;
      }
   }

   if (sendPkt) {
      (void)NWK_QosToParams(ip_frame->qualityOfService,
         &ackRequired,
         &MsgPriority,
         &droppable,
         &reliability);

      (void)MAC_DataRequest_Srfn(
               &dstAddr,      //
               ipPacket,      //
               length,        //
               ackRequired,                //
               MsgPriority,                   //
               droppable,                  //
               reliability,                //
               channelSetIndex,            //
               NULL,                       // no callback
               NULL);                      // Confirm Handler
   }
}

/***********************************************************************************************************************
Function Name: NWK_ProcessNextHeader

Purpose: This function processes the next header if present. Only "next hop" header is supported
         If next hop is not present than the routing table is checked for valid routing.
         If the next hop or table is valid, the MAC destination will be updated.

Arguments: ipPacket - IP packet
           length   - Length of IP packet
           ip_frame - Pointer to a buffer containing a decoded IP frame

Returns:
***********************************************************************************************************************/
void NWK_ProcessNextHeader(uint8_t * ipPacket, uint16_t length, IP_Frame_t *ip_frame)
{
   uint32_t      i; // loop counter
   bool          myAddress = (bool)false;
   NWK_Address_t dstAddr   = ip_frame->dstAddress;
   bool          routable  = (bool)false;

   /* Retrieve our MAC extension */
   MAC_MacAddress_Get(&thisMtuIPAddress[OFFSET_TO_EUI40]);

   // Check if "next hop header" is present
   if (ip_frame->nextHeaderPresent && (ip_frame->nextHeader == NEXT_HEADER_NEXT_HOP_TYPE)){
      // Process hop header
      INFO_printf("Next Hop Header detected. Acting as a router.");

      // Is next hop persistent?
      if ( ip_frame->nextHopHeader.persist ) {
         // Next hop is persistent

         // Any next hop address for me?
         for ( i = 0; i < ip_frame->nextHopHeader.hopCount; i++ ) {
            if ( ip_frame->nextHopHeader.addressType ) {
               if ( 0 == memcmp(&thisMtuIPAddress[OFFSET_TO_EUI40], ip_frame->nextHopHeader.nextHop[i].ExtensionAddr, MAC_ADDRESS_SIZE) ) {
                  myAddress = (bool)true;
                  break;
               }
            } else {
               // todo: 11/22/2106 11:29 AM [MKD] - Need to add support for IPv6
            }
         }

         if (myAddress) {
            // Is myAddress the last hop?
            if ( 0 == memcmp(&thisMtuIPAddress[OFFSET_TO_EUI40], ip_frame->nextHopHeader.nextHop[ip_frame->nextHopHeader.hopCount-1].ExtensionAddr, MAC_ADDRESS_SIZE) ) {
               // Send to NWK destination address
               dstAddr = ip_frame->dstAddress;
            } else {
               // Send to next hop after my address
               dstAddr = ip_frame->nextHopHeader.nextHop[i+1];
            }
            routable = (bool)true;
         }
      } else {
         // Next hop is not persistent

         // Is first hop for me?
         if ( 0 == memcmp(&thisMtuIPAddress[OFFSET_TO_EUI40], ip_frame->nextHopHeader.nextHop[0].ExtensionAddr, MAC_ADDRESS_SIZE) ) {
            // Decrement hop counter
            uint16_t offset = ip_frame->nextHopHeader.nextHopHeaderOffset;
            //If the count is zero the result is 0xFF. OK since there are no more headers the byte will be removed below
            ipPacket[offset+1] = (uint8_t)(((uint8_t)(((ipPacket[offset+1] >> 1) & 0x03) - 1) << 1) | (ipPacket[offset+1] & ~0x06));
            ip_frame->nextHopHeader.hopCount--;  //This is the actual count (i.e. message protocol value plus one)

            // Remove first hop based on address length
            if ( ip_frame->nextHopHeader.addressType ) {
               // Extension ID
               (void)memcpy(&ipPacket[offset+2], &ipPacket[offset+2+MAC_ADDRESS_SIZE], (size_t)(length-(offset+2+MAC_ADDRESS_SIZE)));
               length -= MAC_ADDRESS_SIZE;
            } else {
               // IPv6 address
               (void)memcpy(&ipPacket[offset+2], &ipPacket[offset+2+IPV6_ADDRESS_SIZE], (size_t)(length-(offset+2+IPV6_ADDRESS_SIZE)));
               length -= IPV6_ADDRESS_SIZE;
            }

            // Any next hop addresses left?
            if ( ip_frame->nextHopHeader.hopCount ) {
               // Send packet to next hop
               dstAddr = ip_frame->nextHopHeader.nextHop[1];
            } else {
               // No next hop left
               // Check if next header is present
               if ( ip_frame->nextHopHeader.nextHeaderPresent ) {
                  // Set NWK header's Next Header filed to Next Hop header's Next header field
                  ipPacket[offset-1] = ip_frame->nextHopHeader.nextHeader;
               } else {
                  // Set NWK header Next header present field to 0 and remove next header field
                  ipPacket[1] &= 0xFE; // Mask next header present bit
                  (void)memcpy(&ipPacket[offset-1], &ipPacket[offset], (size_t)length-offset);
                  length -= 1;
               }

               // Remove the next hop header
               (void)memcpy(&ipPacket[offset-1], &ipPacket[offset+1], (size_t)(length-(offset+1)));
               length -= 2;

               // Send packet to NWK destination address
               dstAddr = ip_frame->dstAddress;
            }
            routable = (bool)true;
         }
      }
   } else {
      // Check for valid table routing entry
      if ( NWK_FindNextHop(&ip_frame->dstAddress, &dstAddr) ) {
         // Route frame to next hop
         routable = (bool)true;
      }
   }

   if ( routable ) {
      NWK_ElideAndSend(ipPacket, length, ip_frame, dstAddr, eMAC_CHANNEL_SET_INDEX_1);
   } else {
      // This valid packet made it to the network layer but is not for the application layer and has no valid route
      NWK_CounterInc(eNWK_ipIfInDiscards, 1, IGNORE_PARAMETER);  // Does not track by LINK
      INFO_printf("Unroutable packet dropped");
   }
}

/***********************************************************************************************************************
Function Name: NWK_ProcessDataIndication

Purpose: This function handles the DataIndication message from the MAC

Arguments: mac_buffer - Pointer to a buffer containing a data indication at mac_buffer->data

Returns: none
***********************************************************************************************************************/
static void NWK_ProcessDataIndication( buffer_t const *mac_buffer )
{
#if 1
   MAC_DataInd_t *mac_indication = (MAC_DataInd_t*)mac_buffer->data; /*lint !e826 data has MAC_DataInd_t */
#else
   MAC_Indication_t *pInd =  (MAC_Indication_t *)mac_buffer->data; /*lint !e740 !e826 */
   MAC_DataInd_t *mac_indication = &pInd->DataInd;
#endif

   buffer_t *stack_buffer = NULL;
   IP_Frame_t rx_frame;
   NWK_COUNTER_e failReason;
   bool shouldProcess;
#if ( DCU == 1 )
   bool shouldRoute = (bool)false;
#endif

   // Update the RX Octets Count for RF_LINK
   NWK_CounterInc(eNWK_ipIfInOctets, mac_indication->payloadLength, RF_LINK);

   // Update the RX Packets for RF_LINK
   NWK_CounterInc(eNWK_ipIfInPackets, 1, RF_LINK);

   shouldProcess = NWK_decode_frame ( mac_indication->payload, mac_indication->payloadLength, &rx_frame, &failReason );
   INFO_printf("Received a packet from RF link");

   if ( shouldProcess )
   {
      // Retrieve MAC address
      MAC_MacAddress_Get(&thisMtuIPAddress[OFFSET_TO_EUI40]);

      INFO_printf("DataInd:len=%d [Valid]", mac_indication->payloadLength);
      // Some processing for MAC address

      if ( rx_frame.dstAddress.addr_type == eEXTENSION_ID )
      {
#if (DCU == 1)
         if ( ( VER_isComDeviceStar() ) &&
              ( 0 != memcmp(&thisMtuIPAddress[OFFSET_TO_EUI40], rx_frame.dstAddress.ExtensionAddr, MAC_ADDRESS_SIZE) ) )
         {
            shouldProcess = (bool)false;
         } else if ( VER_isComDeviceSrfn() )
#endif
         {
            // Check if destination address is NOT for me
            if ( 0 != memcmp(&thisMtuIPAddress[OFFSET_TO_EUI40], rx_frame.dstAddress.ExtensionAddr, MAC_ADDRESS_SIZE) )
            {
               // Frame is not for me but maybe I'm a router
               // Decode next header if present and send packet back to RF link
               NWK_ProcessNextHeader(mac_indication->payload, mac_indication->payloadLength, &rx_frame);
               shouldProcess = (bool)false;;
            }
         }
      }
      // Don't process frame if frame is for head end
      else if ( rx_frame.dstAddress.addr_type == eCONTEXT )
      {
#if ( DCU == 1 )
         shouldRoute = (bool)true;
#endif
         shouldProcess = (bool)false;
      }
#if ( EP == 1 )
      // Don't process frame if frame is multicast but not for EPs
      else if (rx_frame.dstAddress.addr_type == eMULTICAST)
      {
         int32_t goodMulticast;

         goodMulticast = memcmp(BROADCAST_MULTICAST_EP_ADDR, rx_frame.dstAddress.multicastAddr,
                                                                                    MULTICAST_ADDRESS_SIZE);
#if ( LOAD_CONTROL_DEVICE == 1 )
         /* Don't process frame for load control devices if not a load control device. */
         if ( goodMulticast != 0 )
         {
            goodMulticast = memcmp(BROADCAST_MULTICAST_LC_ADDR, rx_frame.dstAddress.multicastAddr,
                                                                                       MULTICAST_ADDRESS_SIZE);
         }
#endif
         if ( goodMulticast != 0 )
         {
            /* multicast address for some other group */
            shouldProcess = (bool)false;
         }
      }
#endif

#if ( DCU == 1 )
      // Don't process frame if frame is multicast but not for DCUs
      else if ( (rx_frame.dstAddress.addr_type == eMULTICAST) &&
                (0 != memcmp(BROADCAST_MULTICAST_DCU_ADDR, rx_frame.dstAddress.multicastAddr, MULTICAST_ADDRESS_SIZE) ) )
      {
         /* multicast address for some other group */
         shouldProcess = (bool)false;
      }
#endif
      // Don't process frame if frame is an unsupported address type
      else if ( ( rx_frame.dstAddress.addr_type == eCONTEXT ) ||
                ( rx_frame.dstAddress.addr_type == eFULL_IPV6 ) )
      {
         shouldProcess = (bool)false;
#if( EP == 1 )
         NWK_CounterInc(eNWK_invalidAddrModeCount, 1, 0);
#endif
         INFO_printf("stack layer received unexpected destination address mode");
      }

#if ( DCU == 1 )
      if ( shouldRoute )
      {
         NWK_CounterInc(eNWK_ipIfOutOctets, mac_indication->payloadLength, BACKPLANE_LINK);
         if ( eELIDED == rx_frame.srcAddress.addr_type )
         {
            /* if src address is elided, need to add MAC Extension ID to payload */
            NWK_CounterInc(eNWK_ipIfOutOctets, MAC_ADDRESS_SIZE, BACKPLANE_LINK);
         }
         NWK_CounterInc(eNWK_ipIfOutPackets, 1, BACKPLANE_LINK);
         /* Generate Records to be consumed by the Main board in the DCU */
         MAINBD_GenerateRecord( mac_buffer, (bool)false );
      }
      else
#endif
      {
         if ( shouldProcess == (bool)false )
         {
            /* this valid packet made it to the network layer but is not for the application layer and has no valid route  */
            NWK_CounterInc(eNWK_ipIfInDiscards, 1, IGNORE_PARAMETER);  // Does not track by LINK
            INFO_printf("Unroutable packet dropped");
         }
      }
   }
   else
   {
      NWK_CounterInc(failReason, 1, RF_LINK);
      INFO_printf("Malformed network header");
   }

   if ( ( shouldProcess ) && ( NULL != pIndicationHandler_callback[ rx_frame.dst_port ] ) )
   {
      /* generate a stack indication */
      stack_buffer = BM_allocStack( (sizeof(NWK_DataInd_t) + rx_frame.length) );
      if (stack_buffer != NULL)
      {
         NWK_SendDataIndication(stack_buffer, &rx_frame, mac_indication->timeStamp);
      }
   }
} /* end NWK_ProcessDataIndication */

#if 0
/***********************************************************************************************************************
Function Name: NWK_ProcessDataConfirm

Purpose: This function handles the DataConfirm message from a lower layer

Arguments: mac_buffer - Pointer to a buffer containing a data confirmation at mac_buffer->data

Returns: none
***********************************************************************************************************************/
static void NWK_ProcessDataConfirm ( buffer_t *mac_buffer )
{
   // xxx jmb:  needs to be implemented
   (void)mac_buffer;
   // xxx:  Add back in once we transition away from:  DataConfirm_Handler()
   //NWK_CounterInc(eNWK_ipIfOutErrors, RF_LOWER_LINK);
}
#endif

/***********************************************************************************************************************
Function Name: NWK_decode_frame

Purpose: This function decodes a bit packed NWK Frame from a lower layer into a structure of type IP_Frame_t

Arguments: p - Pointer to the data buffer (to be decoded)
           num_bytes - Number of bytes in the bit stuffed data buffer
           pf - Pointer to the IP_Frame_t structure that is to be populated by this function
           fail_reason - if function returns false, this will be the reason the decode failed

Returns: bool - true if successfully decoded.  False if there was a problem:  the IP_Frame_t is undefined/untrusted and
            the fail_reason is populated with the stack counter to increment.

Note:  For now, include the IPv6 Header, and UDP header as part of the MAC Header.  If/when the stack transitions
         to a full stack these can be seperated out.

       The reason this function returns the fail reason rather than just incrementing it is because this function may
         be called multiple times on certain receive chains.
***********************************************************************************************************************/
bool NWK_decode_frame ( uint8_t *p, uint16_t num_bytes, IP_Frame_t *pf, NWK_COUNTER_e *fail_reason )
{
   uint32_t i,j; // loop counter
   uint16_t bitNo = 0;
   uint16_t expected_bit_length = MIN_IP_MSG_SIZE;
   bool frame_valid = (bool)false;
   bool duplicate;

   /* pre-load the fail reason with the most common failure cause */
   *fail_reason = eNWK_ipIfInErrors;

   if ( (num_bytes * CHAR_BIT) >= expected_bit_length)
   {
      pf->version = UNPACK_bits2_uint8(p, &bitNo, IP_VERSION_SIZE );
      if (pf->version == IP_VERSION)
      {
         pf->srcAddress.addr_type = (NWK_AddrType_e)UNPACK_bits2_uint8(p, &bitNo, SRC_ADDR_COMPRESSION_SIZE);
         pf->dstAddress.addr_type = (NWK_AddrType_e)UNPACK_bits2_uint8(p, &bitNo, DST_ADDR_COMPRESSION_SIZE);
         pf->qualityOfService     = UNPACK_bits2_uint8(p, &bitNo, QOS_SIZE);
         pf->nextHeaderPresent    = UNPACK_bits2_uint8(p, &bitNo, NXT_HDR_PRESENT_SIZE);

         if ( NWK_IsQosValueValid(pf->qualityOfService) )
         {
            /* source address */
            if (pf->srcAddress.addr_type == eCONTEXT)
            {
               expected_bit_length += CONTEXT_SIZE;
               if ( (num_bytes * CHAR_BIT) >= expected_bit_length)
               {
                  pf->srcAddress.context = UNPACK_bits2_uint8(p, &bitNo, CONTEXT_SIZE);
                  frame_valid = (bool)true;
               }
            }

            else if ( pf->srcAddress.addr_type == eEXTENSION_ID )
            {
               expected_bit_length += EXTENSION_ID_SIZE;
               if ( (num_bytes * CHAR_BIT) >= expected_bit_length)
               {
                  UNPACK_addr(p, &bitNo, MAC_ADDRESS_SIZE, pf->srcAddress.ExtensionAddr);
                  frame_valid = (bool)true;
               }
            }

            else if ( pf->srcAddress.addr_type == eELIDED )
            {
               frame_valid = (bool)true;
            }
         }

         if (frame_valid )
         {
            frame_valid = (bool)false;
            /* destination address */
            if (pf->dstAddress.addr_type == eCONTEXT)
            {
               expected_bit_length += CONTEXT_SIZE;
               if ( (num_bytes * CHAR_BIT) >= expected_bit_length)
               {
                  pf->dstAddress.context = UNPACK_bits2_uint8(p, &bitNo, CONTEXT_SIZE);
                  frame_valid = (bool)true;
               }
            }
            else if (pf->dstAddress.addr_type == eMULTICAST)
            {
               expected_bit_length += DST_MULTI_SIZE;
               if ( (num_bytes * CHAR_BIT) >= expected_bit_length)
               {
                  UNPACK_addr(p, &bitNo, MULTICAST_ADDRESS_SIZE, pf->dstAddress.multicastAddr);
                  frame_valid = (bool)true;
               }
            }
            else if ( pf->dstAddress.addr_type == eEXTENSION_ID )
            {
               expected_bit_length += EXTENSION_ID_SIZE;
               if ( (num_bytes * CHAR_BIT) >= expected_bit_length)
               {
                  UNPACK_addr(p, &bitNo, MAC_ADDRESS_SIZE, pf->dstAddress.ExtensionAddr);
                  frame_valid = (bool)true;
               }
            }
            else if ( pf->dstAddress.addr_type == eELIDED )
            {
               frame_valid = (bool)true;
            }

            // Retrieve next header field if present
            if ( pf->nextHeaderPresent ) {
               frame_valid = (bool)false;
               expected_bit_length += NEXT_HEADER_SIZE;
               if ( (num_bytes * CHAR_BIT) >= expected_bit_length) {
                  pf->nextHeader = UNPACK_bits2_uint8(p, &bitNo, NEXT_HEADER_SIZE);
                  frame_valid = (bool)true;
               }
            }
         }

         // Process next header if present
         if ( (frame_valid == (bool)true) && pf->nextHeaderPresent )
         {
            frame_valid = (bool)false;

            // Only Next Hop header is supported at this time
            if ( pf->nextHeader == NEXT_HEADER_NEXT_HOP_TYPE ) {
               expected_bit_length +=  SUBTYPE_SIZE + NXT_HDR_PRESENT_SIZE + NEXT_HOP_RESERVED_SIZE + PERSIST_SIZE +
                                       HOP_COUNT_SIZE + ADDRESS_TYPE_SIZE;
               if ( (num_bytes * CHAR_BIT) >= expected_bit_length)
               {  // Extract first part of next hop header
                  pf->nextHopHeader.nextHopHeaderOffset = bitNo/8; /* Save offset in bytes of the next hop header. This
                                                                      will simplify the code when it's time to remove
                                                                      the first hop. */
                  pf->nextHopHeader.subtype             = UNPACK_bits2_uint8(p, &bitNo, SUBTYPE_SIZE);
                  pf->nextHopHeader.nextHeaderPresent   = UNPACK_bits2_uint8(p, &bitNo, NXT_HDR_PRESENT_SIZE);
                  pf->nextHopHeader.reserved            = UNPACK_bits2_uint8(p, &bitNo, NEXT_HOP_RESERVED_SIZE);
                  pf->nextHopHeader.persist             = UNPACK_bits2_uint8(p, &bitNo, PERSIST_SIZE);
                  pf->nextHopHeader.hopCount            = UNPACK_bits2_uint8(p, &bitNo, HOP_COUNT_SIZE) +  1; // Add 1 because this field is one based
                  pf->nextHopHeader.addressType         = UNPACK_bits2_uint8(p, &bitNo, ADDRESS_TYPE_SIZE);

                  // This must be a "Next Hop list subtype"
                  if ( pf->nextHopHeader.subtype == NEXT_HEADER_NEXT_HOP_LIST_SUBTYPE ) {
                     if ( pf->nextHopHeader.addressType ) {
                        // Using extenstion ID
                        expected_bit_length += pf->nextHopHeader.hopCount * EXTENSION_ID_SIZE;
                     } else {
                        // Using IPv6 address
                        expected_bit_length += pf->nextHopHeader.hopCount * IPV6_SIZE;
                     }

                     // Reset all hops. This will simplify the code when it's time to look for duplicate
                     (void)memset(pf->nextHopHeader.nextHop, 0, sizeof(pf->nextHopHeader.nextHop));

                     // Extract each "next hop" field
                     if ( (num_bytes * CHAR_BIT) >= expected_bit_length) {
                        for ( i = 0; i < pf->nextHopHeader.hopCount; i++ ) {
                           if ( pf->nextHopHeader.addressType ) {
                              pf->nextHopHeader.nextHop[i].addr_type = eEXTENSION_ID;
                              UNPACK_addr(p, &bitNo, MAC_ADDRESS_SIZE,  pf->nextHopHeader.nextHop[i].ExtensionAddr);
                           } else {
                              pf->nextHopHeader.nextHop[i].addr_type = eFULL_IPV6;
                              UNPACK_addr(p, &bitNo, IPV6_ADDRESS_SIZE, pf->nextHopHeader.nextHop[i].IPv6_address);
                           }
                        }
                     }

                     // Validate that each hop entry is unique (no duplicate)
                     duplicate = (bool)false;
                     for ( i = 0; i < pf->nextHopHeader.hopCount; i++ ) {
                        for (j = i+1; j < pf->nextHopHeader.hopCount; j++ ) {
                           if ( memcmp(&pf->nextHopHeader.nextHop[i], &pf->nextHopHeader.nextHop[j],
                                       sizeof(NWK_Address_t)) == 0 ) {
                              duplicate = (bool)true;
                           }
                        }
                     }

                     // Next header must be absent since no other header is supported at this time
                     if ( (duplicate == (bool)false) ) {
                        if ( pf->nextHopHeader.nextHeaderPresent == 0 ) {
                           frame_valid = (bool)true;
                        } else {
                           *fail_reason = eNWK_ipIfInUnknownProtos;
                        }
                     }
                  }
               }
            } else {
               *fail_reason = eNWK_ipIfInUnknownProtos;
            }
         }

         // Process UDP
         if (frame_valid == (bool)true)
         {
            frame_valid = (bool)false;
            expected_bit_length += UDP_PORT_SIZE + UDP_PORT_SIZE;

            if ( (num_bytes * CHAR_BIT) >= expected_bit_length)
            {
               pf->src_port = UNPACK_bits2_uint8(p, &bitNo, UDP_PORT_SIZE);
               pf->dst_port = UNPACK_bits2_uint8(p, &bitNo, UDP_PORT_SIZE);
               if ( pf->src_port == pf->dst_port )             /* Make sure src and dst ports match   */
               {
                  pf->length = ( uint16_t)( num_bytes - (bitNo/8));
                  pf->data = &p[ (bitNo/8) ];
                  if (  ( pf->dst_port == (uint8_t)UDP_NONDTLS_PORT ) || /* Unsecured msg -> pass to app layer to determine
                                                                            whether to accept */
                        ( pf->dst_port == (uint8_t)UDP_DTLS_PORT )       /* Secured - pass to dtls handler   */
#if (USE_MTLS == 1)
                        || ( pf->dst_port == (uint8_t)UDP_MTLS_PORT )    /* mtls - pass to mtls handler   */
#endif
#if (USE_IPTUNNEL == 1)
                        || (pf->dst_port == (uint8_t)UDP_IP_NONDTLS_PORT)  /* unsecured - pass to IP handler   */
                        || (pf->dst_port == (uint8_t)UDP_IP_DTLS_PORT) /* Secured - pass to dtls handler   */
#endif
                     )
                  {
                     frame_valid = (bool)true;
                  }
                  else
                  {
                     *fail_reason = eNWK_ipIfInUnknownProtos;  /* Unknown port number. */
                  }
               }
               else
               {
                  *fail_reason = eNWK_ipIfInUnknownProtos;  /* src/dst ports don't match. */
               }
            }
         }
      }
   }
   return frame_valid;
} /* end NWK_decode_frame() */

/***********************************************************************************************************************
Function Name: MAC_RegisterIndicationHandler

Purpose: This function is called by an upper layer module to register a callback function.  That function will accept
   an indication (data.confirmation, data.indication, etc) from this module.  That function is also responsible for
   freeing the indication buffer.

Arguments: pointer to a funciton of type NWK_RegisterIndicationHandler

Returns: none

Note:  Currently only one upper layer can receive indications but the function could be expanded to have an list
   of callbacks if needed.
***********************************************************************************************************************/
void NWK_RegisterIndicationHandler(uint8_t port, NWK_IndicationHandler pCallback)
{
   pIndicationHandler_callback[ port ] = pCallback;
}

/***********************************************************************************************************************
Function Name: NWK_QosToParams

Purpose: Set some parameters based on QOS

Arguments: qos         - quality of service
           ackRequired - TRUE or FALSE based on QOS
           MsgPriority    - 1 to 7 based on QOS
           droppable   - TRUE or FALSE based on QOS
           reliability - HIGH, NORMAL or LOW based on QOS

Returns: TRUE if the paramters are valid

***********************************************************************************************************************/
uint8_t NWK_QosToParams(uint8_t qos, bool *ackRequired, uint8_t *MsgPriority,
                          bool *droppable, MAC_Reliability_e *reliability)
{
   uint8_t returnVal = true;

   *ackRequired = (bool)false;
   *droppable = (bool)false;
   *MsgPriority = 1;

   switch (qos)
   {
      case 0x3F:
         *ackRequired = (bool)true;
         *MsgPriority = 7;
         *reliability = eMAC_RELIABILITY_HIGH;
         break;
      case 0x3E:
         *ackRequired = (bool)true;
         *MsgPriority = 6;
         *reliability = eMAC_RELIABILITY_HIGH;
         break;
      case 0x3C:
         *ackRequired = (bool)true;
         *MsgPriority = 5;
         *reliability = eMAC_RELIABILITY_HIGH;
         break;
      case 0x39:
         *MsgPriority = 4;
         *reliability = eMAC_RELIABILITY_HIGH;
         break;
      case 0x38:
         *MsgPriority = 4;
         *droppable = (bool)true;
         *reliability = eMAC_RELIABILITY_LOW;
         break;
      case 0x32:
         *MsgPriority = 3;
         *reliability = eMAC_RELIABILITY_HIGH;
         break;
      case 0x31:
         *MsgPriority = 3;
         *reliability = eMAC_RELIABILITY_MED;
         break;
      case 0x30:
         *MsgPriority = 3;
         *reliability = eMAC_RELIABILITY_LOW;
         break;
      case 0x22:
         *MsgPriority = 2;
         *reliability = eMAC_RELIABILITY_HIGH;
         break;
      case 0x21:
         *MsgPriority = 2;
         *reliability = eMAC_RELIABILITY_MED;
         break;
      case 0x20:
         *MsgPriority = 2;
         *reliability = eMAC_RELIABILITY_LOW;
         break;
      case 0x2:
         *droppable = (bool)true;
         *reliability = eMAC_RELIABILITY_HIGH;
         break;
      case 0x1:
         *droppable = (bool)true;
         *reliability = eMAC_RELIABILITY_MED;
         break;
      case 0:
         *droppable = (bool)true;
         *reliability = eMAC_RELIABILITY_LOW;
         break;

      default:
            returnVal = false;
            break;
   }
   return returnVal;
}

#if ( EP == 1 )
/*! ********************************************************************************************************************
 *
 * \fn NWK_SET_STATUS_e  ipRoutingMaxPacketCount_Set ( uint8_t value )
 *
 * \brief Set the ipRoutingPacketCount
 *
 * \param  value (1 to 50)
 *
 * \return  NWK_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static NWK_SET_STATUS_e  ipRoutingMaxPacketCount_Set ( uint8_t value )
{
   NWK_SET_STATUS_e eStatus = eNWK_SET_INVALID_PARAMETER;
   if((value > 0 ) && ( value <= 50))
   {
      ConfigAttr.ipRoutingMaxPacketCount = value;
   }

   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn NWK_SET_STATUS_e  ipRoutingEntryExpirationThreshold_Set(uint8_t value )
 *
 * \brief Set the ipRoutingEntryExpirationThreshold
 *
 * \param  value (1 to 255)
 *
 * \return  NWK_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static NWK_SET_STATUS_e  ipRoutingEntryExpirationThreshold_Set(uint8_t value )
{
   NWK_SET_STATUS_e eStatus = eNWK_SET_INVALID_PARAMETER;
   if(value > 0 )
   {
      ConfigAttr.ipRoutingEntryExpirationThreshold = value;
   }
   return eStatus;
}
#endif

/*! ********************************************************************************************************************
 *
 * \fn NWK_SET_STATUS_e ipHEContext_Set( uint8_t value )
 *
 * \brief Set the Head End Context
 *
 * \param  value (0 to 255)
 *
 * \return  NWK_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static NWK_SET_STATUS_e ipHEContext_Set( uint8_t value )
{
   NWK_SET_STATUS_e eStatus = eNWK_SET_SUCCESS;
   ConfigAttr.ipHEContext  = value;
   return eStatus;
}

#if ( FAKE_TRAFFIC == 1 )
/*! ********************************************************************************************************************
 *
 * \fn NWK_SET_STATUS_e ipRfDutyCycle_Set( uint8_t value )
 *
 * \brief Set the RF transmit duty cycle
 *
 * \param  value (0 to 10)
 *
 * \return  NWK_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static NWK_SET_STATUS_e ipRfDutyCycle_Set( uint8_t value )
{
   NWK_SET_STATUS_e eStatus = eNWK_SET_INVALID_PARAMETER;
   if(value <= 10 )
   {
      ConfigAttr.ipRfDutyCycle = value;
      eStatus = eNWK_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn NWK_SET_STATUS_e ipBhaulGenCount_Set( uint8_t value )
 *
 * \brief Set the number of fake records to be generated per second
 *
 * \param  value (0 to 60)
 *
 * \return  NWK_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static NWK_SET_STATUS_e ipBhaulGenCount_Set( uint8_t value )
{
   NWK_SET_STATUS_e eStatus = eNWK_SET_INVALID_PARAMETER;
   if(value <= 60 )
   {
      ConfigAttr.ipBhaulGenCount = value;
      eStatus = eNWK_SET_SUCCESS;
   }
   return eStatus;
}
#endif

/*! ********************************************************************************************************************
 *
 * \fn
 *
 * \brief
 *
 * \param
 *
 * \return
 *
 ********************************************************************************************************************
 */
static NWK_SET_STATUS_e NWK_Attribute_Set( NWK_SetReq_t const *pSetReq)
{
   NWK_SET_STATUS_e eStatus;

   // This function should only be called inside the NWK task
   if ( 0 != strcmp("NWK", OS_TASK_GetTaskName()) ) {
      ERR_printf("WARNING: NWK_Attribute_Set should only be called inside the NWK task. Please use NWK_SetRequest instead.");
   }

   switch (pSetReq->eAttribute)
   {
      case    eNwkAttr_ipIfInDiscards:                    eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfInErrors:                      eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfInMulticastPkts:               eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfInOctets:                      eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfInPackets:                     eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfInUcastPkts:                   eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfInUnknownProtos:               eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfLastResetTime:                 eStatus = eNWK_SET_READONLY; break;         //
      case    eNwkAttr_ipIfOutDiscards:                   eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfOutErrors:                     eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfOutMulticastPkts:              eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfOutOctets:                     eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfOutPackets:                    eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipIfOutUcastPkts:                  eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipLowerLinkCount:                  eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipUpperLayerCount:                 eStatus = eNWK_SET_READONLY; break;
#if ( EP == 1 )
      case    eNwkAttr_ipRoutingMaxPacketCount:           eStatus = ipRoutingMaxPacketCount_Set(          pSetReq->val.ipRoutingMaxPacketCount);           break;
      case    eNwkAttr_ipRoutingEntryExpirationThreshold: eStatus = ipRoutingEntryExpirationThreshold_Set(pSetReq->val.ipRoutingEntryExpirationThreshold); break;
      case    eNwkAttr_invalidAddrModeCount:              eStatus = eNWK_SET_READONLY; break;
#endif
      case    eNwkAttr_ipHEContext:                       eStatus = ipHEContext_Set(                      pSetReq->val.ipHEContext);                       break;
      case    eNwkAttr_ipState:                           eStatus = eNWK_SET_READONLY; break;
#if ( HE == 1 )
      case    eNwkAttr_ipPacketHistory:                   eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipRoutingHistory:                  eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipRoutingMaxDcuCount:              eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipRoutingFrequency:                eStatus = eNWK_SET_READONLY; break;
      case    eNwkAttr_ipMultipathForwardingEnabled:      eStatus = eNWK_SET_READONLY; break;
#endif
#if ( FAKE_TRAFFIC == 1)
      case    eNwkAttr_ipRfDutyCycle:                     eStatus = ipRfDutyCycle_Set(                    pSetReq->val.ipRfDutyCycle);                     break;
      case    eNwkAttr_ipBhaulGenCount:                   eStatus = ipBhaulGenCount_Set(                  pSetReq->val.ipBhaulGenCount);                   break;
#endif
      default:
         eStatus   = eNWK_SET_UNSUPPORTED ;
         break;
   }

   // This is for debug/test
   char *pStatusMsg;
   switch (eStatus)
   {
      case eNWK_SET_SUCCESS:             pStatusMsg = "SUCCESS";             break;
      case eNWK_SET_READONLY:            pStatusMsg = "READONLY";            break;
      case eNWK_SET_UNSUPPORTED:         pStatusMsg = "UNSUPPORTED";         break;
      case eNWK_SET_INVALID_PARAMETER:   pStatusMsg = "INVALID_PARAMETER";   break;
      case eNWK_SET_SERVICE_UNAVAILABLE: pStatusMsg = "SERVICE_UNAVAILABLE"; break;
      default:                           pStatusMsg = "Unknown";             break;
   }

   if (eStatus == eNWK_SET_SUCCESS)
   {
      nwk_file_t *pFile = Files[0];
      (void)FIO_fwrite( &pFile->handle, 0, pFile->Data, pFile->Size);
   }else
   {
      char const *attr_name = "unknown";
      if(eStatus != eNWK_SET_UNSUPPORTED)
      {
         attr_name = nwk_attr_names[pSetReq->eAttribute];
      }
      ERR_printf("NWK_Attribute_Set:Attr=%d[%s] Status=%s", ( int32_t )pSetReq->eAttribute, attr_name, pStatusMsg);
   }
   return eStatus;
}


static NWK_GET_STATUS_e NWK_Attribute_Get( NWK_GetReq_t const *pGetReq, NWK_ATTRIBUTES_u *val)
{
   NWK_GET_STATUS_e eStatus = eNWK_GET_SUCCESS;

   // This function should only be called inside the NWK task
   if ( 0 != strcmp("NWK", OS_TASK_GetTaskName()) )
   {
      ERR_printf("WARNING: NWK_Attribute_Get should only be called inside the NWK task. Please use NWK_GetRequest instead.");
   }

   switch (pGetReq->eAttribute)
   {
      case    eNwkAttr_ipIfInDiscards:                    val->ipIfInDiscards                    = CachedAttr.ipIfInDiscards                                        ; break;
      case    eNwkAttr_ipIfInErrors:         (void)memcpy(val->ipIfInErrors,                       CachedAttr.ipIfInErrors,        sizeof(val->ipIfInErrors))       ; break;
      case    eNwkAttr_ipIfInMulticastPkts:  (void)memcpy(val->ipIfInMulticastPkts,                CachedAttr.ipIfInMulticastPkts, sizeof(val->ipIfInMulticastPkts)); break;
      case    eNwkAttr_ipIfInOctets:         (void)memcpy(val->ipIfInOctets,                       CachedAttr.ipIfInOctets,        sizeof(val->ipIfInOctets))       ; break;
      case    eNwkAttr_ipIfInUcastPkts:      (void)memcpy(val->ipIfInUcastPkts,                    CachedAttr.ipIfInUcastPkts,     sizeof(val->ipIfInUcastPkts))    ; break;
      case    eNwkAttr_ipIfInUnknownProtos:  (void)memcpy(val->ipIfInUnknownProtos,                CachedAttr.ipIfInUnknownProtos, sizeof(val->ipIfInUnknownProtos)); break;
      case    eNwkAttr_ipIfLastResetTime:                 val->ipIfLastResetTime                 = CachedAttr.LastResetTime                                         ; break;
      case    eNwkAttr_ipIfInPackets:        (void)memcpy(val->ipIfInPackets,                      CachedAttr.ipIfInPackets,       sizeof(val->ipIfInPackets))      ; break;
      case    eNwkAttr_ipIfOutDiscards:                   val->ipIfOutDiscards                   = CachedAttr.ipIfOutDiscards                                       ; break;
      case    eNwkAttr_ipIfOutErrors:        (void)memcpy(val->ipIfOutErrors,                      CachedAttr.ipIfOutErrors,       sizeof(val->ipIfOutErrors))      ; break;
      case    eNwkAttr_ipIfOutMulticastPkts: (void)memcpy(val->ipIfOutMulticastPkts,               CachedAttr.ipIfOutMulticastPkts,sizeof(val->ipIfOutMulticastPkts));break;
      case    eNwkAttr_ipIfOutOctets:        (void)memcpy(val->ipIfOutOctets,                      CachedAttr.ipIfOutOctets,       sizeof(val->ipIfOutOctets))      ; break;
      case    eNwkAttr_ipIfOutPackets:       (void)memcpy(val->ipIfOutPackets,                     CachedAttr.ipIfOutPackets,      sizeof(val->ipIfOutPackets))     ; break;
      case    eNwkAttr_ipIfOutUcastPkts:     (void)memcpy(val->ipIfOutUcastPkts,                   CachedAttr.ipIfOutUcastPkts,    sizeof(val->ipIfOutUcastPkts))   ; break;
      case    eNwkAttr_ipLowerLinkCount:                  val->ipLowerLinkCount                  = IP_LOWER_LINK_COUNT                                              ; break;
      case    eNwkAttr_ipUpperLayerCount:                 val->ipUpperLayerCount                 = (uint32_t)IP_PORT_COUNT                                          ; break;
#if ( EP == 1 )
      case    eNwkAttr_ipRoutingMaxPacketCount:           val->ipRoutingMaxPacketCount           = ConfigAttr.ipRoutingMaxPacketCount                               ; break;
      case    eNwkAttr_ipRoutingEntryExpirationThreshold: val->ipRoutingEntryExpirationThreshold = ConfigAttr.ipRoutingEntryExpirationThreshold ; break;
      case    eNwkAttr_invalidAddrModeCount:              val->invalidAddrModeCount              = CachedAttr.invalidAddrModeCount                                  ; break;
#endif
      case    eNwkAttr_ipHEContext:                       val->ipHEContext                       = ConfigAttr.ipHEContext                                           ; break;
      case    eNwkAttr_ipState:                           val->ipState                           = _state                                                           ; break;
#if ( HE == 1 )
      case    eNwkAttr_ipPacketHistory:                   val->ipPacketHistory                   = ConfigAttr.ipPacketHistory                                       ; break;
      case    eNwkAttr_ipRoutingHistory:                  val->ipRoutingHistory                  = ConfigAttr.ipRoutingHistory                                      ; break;
      case    eNwkAttr_ipRoutingMaxDcuCount:              val->ipRoutingMaxDcuCount              = ConfigAttr.ipRoutingMaxDcuCount                                  ; break;
      case    eNwkAttr_ipRoutingFrequency:                val->ipRoutingFrequency                = ConfigAttr.ipRoutingFrequency                                    ; break;
      case    eNwkAttr_ipMultipathForwardingEnabled:      val->ipMultipathForwardingEnabled      = ConfigAttr.ipMultipathForwardingEnabled                          ; break;
#endif
#if ( FAKE_TRAFFIC == 1 )
      case    eNwkAttr_ipRfDutyCycle:                     val->ipRfDutyCycle                     = ConfigAttr.ipRfDutyCycle                                         ; break;
      case    eNwkAttr_ipBhaulGenCount:                   val->ipBhaulGenCount                   = ConfigAttr.ipBhaulGenCount                                        ; break;
#endif
      default :   eStatus = eNWK_GET_UNSUPPORTED;
   }

   // This is for debug/test
   char *pStatusMsg;
   switch (eStatus)
   {
      case eNWK_GET_SUCCESS:             pStatusMsg = "SUCCESS";             break;
      case eNWK_GET_UNSUPPORTED:         pStatusMsg = "UNSUPPORTED";         break;
      case eNWK_GET_SERVICE_UNAVAILABLE: pStatusMsg = "SERVICE_UNAVAILABLE"; break;
      default:                           pStatusMsg = "Unknown";             break;
   }

   if (eStatus != eNWK_GET_SUCCESS)
   {
      char const *attr_name = "unknown";
      if(eStatus != eNWK_GET_UNSUPPORTED)
      {
         attr_name = nwk_attr_names[pGetReq->eAttribute];
      }
      ERR_printf("NWK_Attribute_Get:Attr=%d[%s] Status=%s", ( int32_t )pGetReq->eAttribute, attr_name, pStatusMsg);
   }
   return eStatus;
}

/*!
 * This function is used to print the NWK stats to the terminal
 */
void NWK_Stats(void)
{
   uint8_t i;

   sysTime_t            sysTime;
   sysTime_dateFormat_t sysDateFormat;

   TIME_UTIL_ConvertSecondsToSysFormat(CachedAttr.LastResetTime.seconds, 0, &sysTime);
   (void) TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &sysDateFormat);
   DBG_logPrintf('I', "NWK_STATS:ipIfLastResetTime=%02u/%02u/%04u %02u:%02u:%02u",
                 sysDateFormat.month, sysDateFormat.day, sysDateFormat.year,
                 sysDateFormat.hour, sysDateFormat.min, sysDateFormat.sec); /*lint !e123 macro name min OK */

   INFO_printf( "NWK_STATS:%s %lu", nwk_attr_names[eNwkAttr_ipIfInDiscards         ], CachedAttr.ipIfInDiscards);
   INFO_printf( "NWK_STATS:%s %lu", nwk_attr_names[eNwkAttr_ipIfOutDiscards        ], CachedAttr.ipIfOutDiscards);
#if( EP == 1)
   INFO_printf( "NWK_STATS:%s %lu", nwk_attr_names[eNwkAttr_invalidAddrModeCount   ], CachedAttr.invalidAddrModeCount);
#endif
#if ( IP_LOWER_LINK_COUNT > 2 )
#error "Fix this array:nwk_lower_link_names"
#endif
   // Either RF LINK or BACKPLANE LINK
   static const char * const nwk_lower_link_names[] =
   {
      [0] = "RF_LINK",  // RF Link
      [1] = "BP_LINK"   // Backplane Link
   };

   for( i = 0; i < IP_LOWER_LINK_COUNT; i++ )
   {
      INFO_printf( "NWK_STATS[%s]:%s %lu", nwk_lower_link_names[i], nwk_attr_names[eNwkAttr_ipIfInErrors        ], CachedAttr.ipIfInErrors[i]);
      INFO_printf( "NWK_STATS[%s]:%s %lu", nwk_lower_link_names[i], nwk_attr_names[eNwkAttr_ipIfInOctets        ], CachedAttr.ipIfInOctets[i]);
      INFO_printf( "NWK_STATS[%s]:%s %lu", nwk_lower_link_names[i], nwk_attr_names[eNwkAttr_ipIfInPackets       ], CachedAttr.ipIfInPackets[i]);
      INFO_printf( "NWK_STATS[%s]:%s %lu", nwk_lower_link_names[i], nwk_attr_names[eNwkAttr_ipIfInUnknownProtos ], CachedAttr.ipIfInUnknownProtos[i]);
      INFO_printf( "NWK_STATS[%s]:%s %lu", nwk_lower_link_names[i], nwk_attr_names[eNwkAttr_ipIfOutErrors       ], CachedAttr.ipIfOutErrors[i]);
      INFO_printf( "NWK_STATS[%s]:%s %lu", nwk_lower_link_names[i], nwk_attr_names[eNwkAttr_ipIfOutOctets       ], CachedAttr.ipIfOutOctets[i]);
      INFO_printf( "NWK_STATS[%s]:%s %lu", nwk_lower_link_names[i], nwk_attr_names[eNwkAttr_ipIfOutPackets      ], CachedAttr.ipIfOutPackets[i]);
   }

   static const char * const nwk_port_names[IP_PORT_COUNT] =
   {
      [UDP_NONDTLS_PORT]    = "UDP_NONDTLS_PORT   ",
      [UDP_DTLS_PORT]       = "UDP_DTLS_PORT      ",
#if ( USE_MTLS == 1 ) || (DCU == 1 ) // Remove DCU == 1 when MTLS is enabled for TB
      [UDP_MTLS_PORT]       = "UDP_MTLS_PORT      ",
#endif
#if (USE_IPTUNNEL == 1)
      [UDP_IP_NONDTLS_PORT] = "UDP_NONDTLS_IP_PORT",
      [UDP_IP_DTLS_PORT]    = "UDP_DTLS_IP_PORT   "
#endif
   };

   // for each port (
   for( i = 0; i < ( uint8_t )IP_PORT_COUNT; i++ )
   {
      INFO_printf( "NWK_STATS[%s]:%s %lu", nwk_port_names[i],nwk_attr_names[eNwkAttr_ipIfInMulticastPkts ], CachedAttr.ipIfInMulticastPkts [i]);
      INFO_printf( "NWK_STATS[%s]:%s %lu", nwk_port_names[i],nwk_attr_names[eNwkAttr_ipIfInUcastPkts     ], CachedAttr.ipIfInUcastPkts     [i]);
      INFO_printf( "NWK_STATS[%s]:%s %lu", nwk_port_names[i],nwk_attr_names[eNwkAttr_ipIfOutMulticastPkts], CachedAttr.ipIfOutMulticastPkts[i]);
      INFO_printf( "NWK_STATS[%s]:%s %lu", nwk_port_names[i],nwk_attr_names[eNwkAttr_ipIfOutUcastPkts    ], CachedAttr.ipIfOutUcastPkts    [i]);
   }
}


/*
 * This is used to send the second stats message (OB)
 */
#if ( FAKE_TRAFFIC != 1 )
static void STK_MGR_Stats_CB(MAC_DATA_STATUS_e eStatus, uint16_t handle )
{
   (void) eStatus;
   (void) handle;
#if ( EP == 1 )
   // Send the OB Stats Message
   STK_MGR_SendStats( 2, 1 );
#else
   // todo: 8/21/2015 4:14:43 PM [BAH] - Not supported on DCU
#endif
}

/***********************************************************************************************************************
Function Name: STK_MGR_SendStats

Purpose: send a message containing network statistics

Arguments: type - 1 for network statistics, 2 for RSSI statistics
           uTimeDiversity - number of milliseconds to delay before packet is transmitted,

Returns: none
***********************************************************************************************************************/
static void STK_MGR_SendStats( int type, uint32_t uTimeDiversity)
{
   // If there is a previous statistic message pending do to time diversity, discard it.
   if (0 != uLogTimerId)
   {
      (void) TMR_DeleteTimer(uLogTimerId);
      uLogTimerId = 0;
   }

   if (NULL != pLogMessage)
   {
      BM_free(pLogMessage);
      pLogMessage = NULL;
   }

   // Initialize the header
   HEEP_initHeader( &loglTxInfo );

   switch(type)
   {
      case 1:
         pLogMessage = eng_res_BuildStatsPayload();
         // This is to cause the  STK_MGR_Build_OB_LinkPayload() to be sent
         loglTxInfo.callback = STK_MGR_Stats_CB;
         break;
#if ( EP == 1 )
      case 2:
         pLogMessage = eng_res_Build_OB_LinkPayload();
         loglTxInfo.callback = NULL;
         break;
#endif
      default:
         pLogMessage = NULL;
         break;
   }

   if (pLogMessage != NULL)                            // Send the message to message handler.
   {
      //Application header/QOS info
      loglTxInfo.TransactionType = (uint8_t)TT_ASYNC;
      loglTxInfo.Resource        = (uint8_t)bu_en;
      loglTxInfo.Method_Status   = (uint8_t)method_post;
      loglTxInfo.Req_Resp_ID     = (uint8_t)0;
      loglTxInfo.appSecurityAuthMode = 0; // System requirement to send payload always unencrypted

      // For MTU, Increase traffic class on BU stats message for increased reliability
      // QOS is ignored for TB. The payload will be sent to the MB only once.
      loglTxInfo.qos = SMTDCFG_getEngBuTrafficClass();   // HI RE



      // Set a timer so the message will be sent after the time diversity delay.
      INFO_printf("Time Diversity: %u Milliseconds", max( 1, uTimeDiversity ) );
      STK_MGR_SetLogTimer( max( 1, uTimeDiversity ) ); // Enforce a minimum time of 1 msec so that the timer will accept the request.
   }
}
#endif

/***********************************************************************************************************************
 *
 * Function Name: STK_MGR_SetLogTimer(uint32_t timeMilliseconds)
 *
 * Purpose: Create the timer to re-send the historical data after the given number
 *          of milliseconds have elapsed.
 *
 * Arguments: uint32_t timeRemainingMSec - The number of milliseconds until the timer goes off.
 *
 * Returns: returnStatus_t - indicates if the timer was successfully created
 *
 * Side Effects: None
 *
 * Reentrant Code: NO
 *
 **********************************************************************************************************************/
static void STK_MGR_SetLogTimer(uint32_t timeMilliseconds)
{
   timer_t tmrSettings;             // Timer for sending pending ID bubble up message
   returnStatus_t retStatus;

   (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
   tmrSettings.bOneShot = true;
   tmrSettings.bOneShotDelete = true;
   tmrSettings.bExpired = false;
   tmrSettings.bStopped = false;
   tmrSettings.ulDuration_mS = timeMilliseconds;
   tmrSettings.pFunctCallBack = STK_MGR_LogTimerExpired;
   retStatus = TMR_AddTimer(&tmrSettings);

   if (eSUCCESS == retStatus)
   {
      uLogTimerId = tmrSettings.usiTimerId;
   }
   else
   {
      uLogTimerId = 0;
      ERR_printf("Unable to add Log timer");
   }
}

/***********************************************************************************************************************
 *
 * Function Name: STK_MGR_LogTimerExpired(uint8_t cmd, void *pData)
 *
 * Purpose: Called by the timer to send the historical data bubble up message.  Time diveristy for historical
 *          data is implemented in this manner so that in the case of a time jump, it might be necessary to
 *          either send the message before the time diversity delay has expired or it might be necessary to
 *          discard the pending message in the case of a backward time jump.
 *          Since this code is called from within the timer task, we can't muck around with the
 *          timer, so we need to notify the app message class to do the actual work.
 *
 * Arguments: uint8_t cmd - from the ucCmd member when the timer was created.
 *            void *pData - from the pData member when the timer was created
 *
 * Returns: None
 *
 * Side Effects: None
 *
 * Reentrant Code: NO
 *
 **********************************************************************************************************************/
/*lint -esym(715, cmd, pData ) not used; required by API  */
static void STK_MGR_LogTimerExpired(uint8_t cmd, void *pData)
{
   static buffer_t buf;

 /***
 * Use a static buffer instead of allocating one because of the allocation fails,
 * there wouldn't be much we could do - we couldn't reset the timer to try again
 * because we're already in the timer callback.
 ***/
   BM_AllocStatic(&buf, eSYSFMT_TIME_DIVERSITY);
   OS_MSGQ_Post(&NWK_msgQueue, (void *)&buf);

   uLogTimerId = 0;
}  /*lint !e818 pData could be pointer to const */
/*lint +esym(715, cmd, pData ) not used; required by API  */

/***********************************************************************************************************************
Function Name: NWK_SendDataIndication

Purpose: Send a data indication to the registered upper layer handler

Arguments: pBuf      - a pointer to a buffer_t containing a NWK_DataInd_t
           rx_frame  - Pointer to the IP_Frame_t structure
           timestamp -

Returns: none

Note:
***********************************************************************************************************************/
void NWK_SendDataIndication(buffer_t *pBuf, IP_Frame_t const *rx_frame, TIMESTAMP_t timeStamp)
{
   NWK_DataInd_t *stack_indication;

   if (pBuf != NULL)
   {
      if ( pIndicationHandler_callback[ rx_frame->dst_port ] != NULL )
      {
         pBuf->eSysFmt = eSYSFMT_NWK_INDICATION;

         stack_indication                = (NWK_DataInd_t*)pBuf->data; /*lint !e826 data has NWK_DataInd_t */
         stack_indication->timePresent   = true;
         stack_indication->timeStamp     = timeStamp;
         stack_indication->qos           = rx_frame->qualityOfService;
         stack_indication->payloadLength = rx_frame->length;
         stack_indication->payload       = (uint8_t*)&pBuf->data[(sizeof(NWK_DataInd_t))];

         (void)memcpy(&stack_indication->dstAddr, &rx_frame->dstAddress, sizeof(stack_indication->dstAddr));
         stack_indication->dstUdpPort = NWK_UdpPortIdToNum(rx_frame->dst_port);
         (void)memcpy(&stack_indication->srcAddr, &rx_frame->srcAddress, sizeof(stack_indication->srcAddr));
         stack_indication->srcUdpPort = NWK_UdpPortIdToNum(rx_frame->src_port);
         (void)memcpy(stack_indication->payload, rx_frame->data, rx_frame->length);

         if (rx_frame->dstAddress.addr_type == eELIDED)
         {
            stack_indication->dstAddr.addr_type = eEXTENSION_ID;

            // Retrieve MAC address
            MAC_MacAddress_Get(&thisMtuIPAddress[OFFSET_TO_EUI40]);

            (void)memcpy(stack_indication->dstAddr.ExtensionAddr,
               &thisMtuIPAddress[OFFSET_TO_EUI40],
               MAC_ADDRESS_SIZE);
         }
         else if (rx_frame->dstAddress.addr_type == eMULTICAST)
         {
            NWK_CounterInc(eNWK_ipIfInMulticastPkts, 1, rx_frame->dst_port);
         }
         else
         {
            NWK_CounterInc(eNWK_ipIfInUcastPkts, 1, rx_frame->dst_port);
         }

         (*pIndicationHandler_callback[ rx_frame->dst_port ])(pBuf);
      }
      else
      {  // no message handler is registered for this port, so free the buffer
         BM_free(pBuf);
         INFO_printf("NWK - no message handler registered");
      }
   }
}

/***********************************************************************************************************************
Function Name: STK_MGR_ApplyFrequencies

Purpose: Currently a stub - this function does nothing.  Eventually this function will handle
   gracefully applying new frequencies at the PHY layer, restarting PHY/MAC/NWK layers as required.

Arguments: none

Returns: NWK_Status_e - eNWK_SUCCESS always until full function implemented

Note:
***********************************************************************************************************************/
STACK_Status_e STK_MGR_ApplyFrequencies( void )
{
   return eNWK_SUCCESS;
}


static uint32_t NextRequestHandle(void)
{
   static uint32_t handle = 1000;
   return(++handle);
}


#if TEST_MODE== 1
static uint8_t testMsg[] = {0x00, 0x83, 0x02, 0x00, 0x55, 0x81, 0x92, 0x78, 0x04, 0x40, 0x00, 0x3F, 0x82,
0x21, 0x00, 0x4A, 0x00, 0x42, 0x82, 0x21, 0x0F, 0x42, 0x42, 0x40, 0x00, 0x21, 0x82, 0x06, 0x00, 0x40,
0x00, 0x20, 0x82, 0x06, 0x00, 0x8A, 0x3D, 0xDE, 0x78, 0xA3, 0x2D};


static void NWK_SendTestMsg( void )
{
   NWK_Address_t dst;
   NWK_GetConf_t GetConf;

   // Retrieve Head End context ID
   GetConf = NWK_GetRequest( eNwkAttr_ipHEContext );

   if (GetConf.eStatus != eNWK_GET_SUCCESS) {
      GetConf.val.ipHEContext = DEFAULT_HEAD_END_CONTEXT; // Use default in case of error
   }

   dst.addr_type = eCONTEXT;
   dst.context   = GetConf.val.ipHEContext;

   (void) NWK_DataRequest(
      0,                 // qos
      sizeof(testMsg),   // size
      testMsg,           // data
      &dst,              // dst
      NULL);
}

#endif

#if ( FAKE_TRAFFIC == 1 )
static const uint8_t testMsg[] = {
0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
0xA0, 0xA1, 0xA2, 0xA3
};

static void NWK_SendTestMsg( void )
   {
   NWK_Address_t dst;

   dst.addr_type = eEXTENSION_ID;
   dst.ExtensionAddr[0] = 0x11;
   dst.ExtensionAddr[0] = 0x22;
   dst.ExtensionAddr[0] = 0x33;
   dst.ExtensionAddr[0] = 0x44;
   dst.ExtensionAddr[0] = 0x55;

   (void) NWK_DataRequest(
      0,                 // port
      0,                 // qos
      sizeof(testMsg),   // size
      testMsg,           // data
      &dst,              // dst
      NULL,
      NULL);
}

#endif
/*
 * This function will reset the attributes to default values
 */
static void ConfigAttr_Init(bool bWrite)
{
   ConfigAttr.ipHEContext                              = HE_CONTEXT_DEFAULT;
#if ( HE == 1 )
   // head end only
   // Set to defaults, these will then be reloaded from NV
   ConfigAttr.ipPacketHistory                          = PACKET_HISTORY_DEFAULT;
   ConfigAttr.ipRoutingHistory                         = PACKET_ROUTING_HISTORY_DEFAULT;
   ConfigAttr.ipRoutingMaxDcuCount                     = ROUTING_MAX_DCU_COUNT_DEFAULT;
   ConfigAttr.ipRoutingFrequency                       = ROUTING_FREQUENCY_DEFAULT;
   ConfigAttr.ipMultipathForwardingEnabled             = MULTIPATH_FORWARDING_ENABLE_DEFAULT;
#endif
#if ( EP == 1 )
   ConfigAttr.ipRoutingMaxPacketCount                  = ROUTING_MAX_PACKET_COUNT_DEFAULT;
   ConfigAttr.ipRoutingEntryExpirationThreshold        = ROUTING_ENTRY_EXPIRATION_THRESHOLD_DEFAULT;
#endif
#if ( FAKE_TRAFFIC == 1 )
   ConfigAttr.ipRfDutyCycle                            = TEST_RF_DUTYCYCLE_DEFAULT;
   ConfigAttr.ipBhaulGenCount                          = TEST_BH_GEN_COUNT_DEFAULT;
#endif
   if(bWrite)
   {
      (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
   }
}

/*
 * This function will clear all of the counters
 */
static void CachedAttr_Init(bool bWrite)
{
   (void) memset(&CachedAttr, 0, sizeof(CachedAttr));
   (void) TIME_UTIL_GetTimeInSecondsFormat( &CachedAttr.LastResetTime );
   if(bWrite)
   {
      (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
#if ( DCU == 1 )
      /* This is a good time to flush all statistics on the T-board. */
      /* This is because if we lose power, the statistics are not flushed and since they are not flushed, the counters could appeared to have rooled backward next time they are read. */
      /* Flush all partitions (only cached actually flushed) and ensures all pending writes are completed */
      ( void )PAR_partitionFptr.parFlush( NULL );
#endif
   }
}


/*!
 * Reset.Request - parameters
 */
static void NWK_Reset(NWK_RESET_e type)
{
   if(type == eNWK_RESET_ALL)
   {
      ConfigAttr_Init((bool) true);
   }
   // This will update the LastReset Time
   CachedAttr_Init((bool) true);
}

bool NWK_IsQosValueValid( uint8_t QoS )
{
   bool returnVal = (bool)true;

   if ( ( QoS != 0x3F ) &&
        ( QoS != 0x3E ) &&
        ( QoS != 0x3C ) &&
        ( QoS != 0x39 ) &&
        ( QoS != 0x38 ) &&
        ( QoS != 0x32 ) &&
        ( QoS != 0x31 ) &&
        ( QoS != 0x30 ) &&
        ( QoS != 0x22 ) &&
        ( QoS != 0x21 ) &&
        ( QoS != 0x20 ) &&
        ( QoS != 0x02 ) &&
        ( QoS != 0x01 ) &&
        ( QoS != 0x00 ) )
   {
      returnVal = (bool)false;
   }

   return returnVal;
}


/*! ********************************************************************************************************************
 *
 * \fn void u32_counter_inc(uint32_t* pCounter, uint32_t count, bool bRollover)
 *
 * \brief  This function is used to increment a u32 counter
 *
 * \param  pCounter  - counter to increment
 *         IncCount     - value to add to counter
 *         bRollover - rollover allowed
 *
 * \return  TRUE if counter was updated
 *
 ********************************************************************************************************************
 */
static bool u32_counter_inc(uint32_t* pCounter, uint32_t IncCount, bool bRollover)
{
   bool bStatus = (bool)false;
   if ( (bRollover) || ( ((uint64_t)*pCounter+(uint64_t)IncCount) < UINT32_MAX ) )
   {
      (*pCounter) += IncCount;
      bStatus = (bool)true;
   }
   return bStatus;
}

#if( EP == 1 )
/*! ********************************************************************************************************************
 *
 * \fn bool u16_counter_inc(uint16_t* pCounter, uint32_t count, bool bRollover)
 *
 * \brief  This function is used to increment a u16 counter
 *
 * \param  pCounter  - counter to increment
 *         IncCount     - value to add to counter
 *         bRollover - rollover allowed
 *
 * \return  TRUE if counter was updated
 *
 ********************************************************************************************************************
 */
static bool u16_counter_inc(uint16_t* pCounter, uint16_t IncCount, bool bRollover)
{
   bool bStatus = (bool)false;
   if ( (bRollover) || ( ((uint32_t)*pCounter + (uint32_t)IncCount) < UINT16_MAX ) )
   {
      (*pCounter) += IncCount;
      bStatus = (bool)true;
   }
   return bStatus;
}
#endif

/*! ********************************************************************************************************************
 *
 * \fn void NWK_CounterInc(NWK_COUNTER_e eCounter, uint32_t count, uint8_t index)
 *
 * \brief  This function is used to increment the NWK Counters
 *
 * \param  eStackCounter - Enumerated valued for the counter
 *         IncCount         - value to add to counter
 *         index         - Offset into array
 *
 * \return  none
 *
 * \details This functions is intended to handle the rollover if required.
 *          If the counter was modified the CacheFile will be updated.
 *
 ********************************************************************************************************************
 */
void NWK_CounterInc(NWK_COUNTER_e eStackCounter, uint32_t IncCount, uint8_t index)
{
   bool bStatus = (bool)false;
   switch(eStackCounter)   /*lint !e787 !e788 not all enums used within switch */
   {  //                                                                      Counter                                                           rollover
      /*lint --e{778} constant evaluates to 0   */
      case eNWK_ipIfInDiscards       :  bStatus = u32_counter_inc(&CachedAttr.ipIfInDiscards                                          ,      IncCount, (bool) true); break;
      case eNWK_ipIfInErrors         :  bStatus = u32_counter_inc(&CachedAttr.ipIfInErrors        [min(index, IP_LOWER_LINK_COUNT -1)],      IncCount, (bool) true); break;
      case eNWK_ipIfInMulticastPkts  :  bStatus = u32_counter_inc(&CachedAttr.ipIfInMulticastPkts [min(index, (uint8_t)IP_PORT_COUNT -1)],   IncCount, (bool) true); break;
      case eNWK_ipIfInOctets         :  bStatus = u32_counter_inc(&CachedAttr.ipIfInOctets        [min(index, IP_LOWER_LINK_COUNT -1)],      IncCount, (bool) true); break;
      case eNWK_ipIfInUcastPkts      :  bStatus = u32_counter_inc(&CachedAttr.ipIfInUcastPkts     [min(index, (uint8_t)IP_PORT_COUNT -1)],   IncCount, (bool) true); break;
      case eNWK_ipIfInUnknownProtos  :  bStatus = u32_counter_inc(&CachedAttr.ipIfInUnknownProtos [min(index, IP_LOWER_LINK_COUNT -1)],      IncCount, (bool) true); break;
      case eNWK_ipIfInPackets        :  bStatus = u32_counter_inc(&CachedAttr.ipIfInPackets       [min(index, IP_LOWER_LINK_COUNT -1)],      IncCount, (bool) true); break;
      case eNWK_ipIfOutDiscards      :  bStatus = u32_counter_inc(&CachedAttr.ipIfOutDiscards                                         ,      IncCount, (bool) true); break;
      case eNWK_ipIfOutErrors        :  bStatus = u32_counter_inc(&CachedAttr.ipIfOutErrors       [min(index, IP_LOWER_LINK_COUNT -1)],      IncCount, (bool) true); break;
      case eNWK_ipIfOutMulticastPkts :  bStatus = u32_counter_inc(&CachedAttr.ipIfOutMulticastPkts[min(index, (uint8_t)IP_PORT_COUNT -1)],   IncCount, (bool) true); break;
      case eNWK_ipIfOutOctets        :  bStatus = u32_counter_inc(&CachedAttr.ipIfOutOctets       [min(index, IP_LOWER_LINK_COUNT -1)],      IncCount, (bool) true); break;
      case eNWK_ipIfOutPackets       :  bStatus = u32_counter_inc(&CachedAttr.ipIfOutPackets      [min(index, IP_LOWER_LINK_COUNT -1)],      IncCount, (bool) true); break;
      case eNWK_ipIfOutUcastPkts     :  bStatus = u32_counter_inc(&CachedAttr.ipIfOutUcastPkts    [min(index, (uint8_t)IP_PORT_COUNT -1)],   IncCount, (bool) true); break;
#if ( EP == 1 )
      case eNWK_invalidAddrModeCount :  bStatus = u16_counter_inc(&CachedAttr.invalidAddrModeCount                                     , (uint16_t)IncCount, (bool) true); break;
#endif
   }  /*lint !e787 !e788 not all enums used within switch */
   if(bStatus)
   {   // The counter was modified
      (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
   }
}

/*
 * Get.Request
 */
static bool Process_GetRequest(   NWK_GetReq_t const *pGetReq)
{
   pNwkConf->Type            = eNWK_GET_CONF;
   pNwkConf->GetConf.eStatus = NWK_Attribute_Get( pGetReq, &pNwkConf->GetConf.val);
   return (bool)true;
}

/*
 * Reset.Request
 */
static bool Process_ResetRequest( NWK_ResetReq_t const *pResetReq)
{
   INFO_printf("Process_ResetReq");
   pNwkConf->Type = eNWK_RESET_CONF;

   if((pResetReq->eType == eNWK_RESET_ALL) ||
      (pResetReq->eType == eNWK_RESET_STATISTICS))
   {
      NWK_Reset(pResetReq->eType);
      pNwkConf->ResetConf.eStatus = eNWK_RESET_SUCCESS;
   }
   else{
      pNwkConf->ResetConf.eStatus = eNWK_RESET_FAILURE;
   }
   return (bool)true;
}

/*
 * Set.Request
 */
static bool Process_SetRequest(NWK_SetReq_t const *pSetReq)
{
   pNwkConf->Type               = eNWK_SET_CONF;
   pNwkConf->SetConf.eStatus    = NWK_Attribute_Set( pSetReq);
   pNwkConf->SetConf.eAttribute = pSetReq->eAttribute;
   return (bool)true;
}

/*
   Upon receipt of a START.request primitive, the NWK will attempt to start.
   If successful will transition to the operational state and reply with a START.confirm status of SUCCESS.
   Any attempt to start the NWK while it is already in the operational state will fail with the NWK issuing a
   START.confirm with a status of NWKWORK_RUNNING.
*/
static bool Process_StartRequest(NWK_StartReq_t const *pStartReq)
{
   (void) pStartReq;
#if ( PWRLG_PRINT_ENABLE == 0 ) // Thin out the debug if we are debugging last gasp
   INFO_printf("StartReq");
#endif
   pNwkConf->Type = eNWK_START_CONF;

   // Attempt to start
   if(_state == eNWK_STATE_OPERATIONAL)
   {  // Already operational
      pNwkConf->StartConf.eStatus = eNWK_START_NETWORK_RUNNING;
   }
   else
   {  // Start NWK
      _state = eNWK_STATE_OPERATIONAL;
      pNwkConf->StartConf.eStatus = eNWK_START_SUCCESS;
   }
#if ( PWRLG_PRINT_ENABLE == 0 ) // Thin out the debug if we are debugging last gasp
   INFO_printf("%s", NWK_StateMsg(_state));
#endif
   return (bool)true;
}

/*
 * Process a Stop Request
 */
static bool Process_StopRequest(NWK_StopReq_t const *pStopReq)
{
   (void) pStopReq;

   INFO_printf("StopReq");
   pNwkConf->Type = eNWK_STOP_CONF;

   // Attempt to stop if not already stopped
   if(_state == eNWK_STATE_OPERATIONAL)
   {  // Already operational
      _state            = eNWK_STATE_IDLE;
      pNwkConf->StopConf.eStatus = eNWK_STOP_SUCCESS;
   }
   else{
      pNwkConf->StopConf.eStatus = eNWK_STOP_ERROR;
   }
#if ( PWRLG_PRINT_ENABLE == 0 ) // Thin out the debug if we are debugging last gasp
   INFO_printf("%s", NWK_StateMsg(_state));
#endif
   return (bool)true;
}

// NLME-Stop
// Upon receipt, the NWK will attempt to stop the MAC and then will transition into the idle state.
// There are no STOP.request parameters.
// static void Process_StopRequest(NWK_StopReq_t* pReg)

/*
   The upper layers can stop the NWK layer by issuing a STOP.request service primitive.
   The STOP service will cause the NWK to cease transmission, reception, routing, and any other
   functionality before finally transitioning to the idle state.  Stopping the NWK will also force
   a stop of a Tengwar MAC layer if present.  This operation is not necessarily a "graceful" stopping
   of the NWK layer.
*/



/*!
 * Returns the Message for NWK_STATE_e
 */
static char* NWK_StateMsg(NWK_STATE_e state)
{
   char* msg="";
   switch(state)
   {
      case eNWK_STATE_IDLE              : msg = "eNWK_STATE_IDLE"       ; break;
      case eNWK_STATE_OPERATIONAL       : msg = "eNWK_STATE_OPERATIONAL"; break;
   }
   return msg;
}

#if 0
/*!
 * Used to send a .Confirmation message to the NWK
 * The request specifes the handler to call
 */
// static void Confirm_Send(buffer_t *pBuf, NWK_ConfirmHandler pConfirmHandler)
static void Confirm_Send(buffer_t *pBuf)
{
   if (pBuf != NULL)
   {
      // Set the type
      pBuf->eSysFmt = eSYSFMT_NWK_CONFIRM;

      //if (pConfirmHandler != NULL )
      {
        // (pConfirmHandler)(pBuf);
      }
   }
}
#endif

/*!
 * Allocate a MAC Request Buffer
 */
static buffer_t *ReqBuffer_Alloc(NWK_REQ_TYPE_t type, uint16_t data_size)
{
   uint16_t size;

   // Account for the NWK_Request Header and pad
   size = sizeof(NWK_Request_t) - sizeof( NWK_Services_u );
   switch(type)
   {
      case eNWK_GET_REQ    :    size += sizeof( NWK_GetReq_t   ); break;
      case eNWK_RESET_REQ  :    size += sizeof( NWK_ResetReq_t ); break;
      case eNWK_SET_REQ    :    size += sizeof( NWK_SetReq_t   ); break;
      case eNWK_START_REQ  :    size += sizeof( NWK_StartReq_t ); break;
      case eNWK_STOP_REQ   :    size += sizeof( NWK_StopReq_t  ); break;
      case eNWK_DATA_REQ   :    size += sizeof( NWK_DataReq_t  ); break;
   }
   // Account for data space
   size += data_size;

   buffer_t *pBuf = BM_allocStack( size );
// INFO_printf("ReqBuffer_Alloc:%d", size);

   if (pBuf != NULL)
   {
      pBuf->eSysFmt  = eSYSFMT_NWK_REQUEST;
      NWK_Request_t *pMsg = (NWK_Request_t *)pBuf->data; /*lint !e740 !e826 */
      pMsg->MsgType   = type;
   }
   return(pBuf);
}


/*!
 * NWK_StartRequest()
 */
uint32_t NWK_StartRequest(NWK_ConfirmHandler confirm_cb)
{
   // Allocate the buffer
   buffer_t *pBuf = ReqBuffer_Alloc(eNWK_START_REQ, 0);
   if (pBuf != NULL)
   {
      NWK_Request_t *pReq = (NWK_Request_t *)pBuf->data; /*lint !e740 !e826 */
      pReq->handleId = NextRequestHandle();
      pReq->pConfirmHandler = confirm_cb;
//    NWK_Request(pBuf);
      OS_MSGQ_Post(&NWK_msgQueue, (void *)pBuf); // Function will not return if it fails
      return pReq->handleId;
   }
   return NWK_INVALID_HANDLE_ID;
}

/*!
 * NWK_StopRequest()
 */
uint32_t NWK_StopRequest(NWK_ConfirmHandler confirm_cb)
{
   // Allocate the buffer
   buffer_t *pBuf = ReqBuffer_Alloc(eNWK_STOP_REQ, 0);
   if (pBuf != NULL)
   {
      NWK_Request_t *pReq = (NWK_Request_t *)pBuf->data; /*lint !e740 !e826 */
      pReq->handleId = NextRequestHandle();
      pReq->pConfirmHandler = confirm_cb;

//    NWK_Request(pBuf);
      OS_MSGQ_Post(&NWK_msgQueue, (void *)pBuf); // Function will not return if it fails
      return pReq->handleId;
   }
   return NWK_INVALID_HANDLE_ID;
}

/*!
 *  Confirmation handler for Get/Set Request
 */
static void NWK_GetSetRequest_CB(NWK_Confirm_t const *confirm)
{
   // Notify calling function that NWK has got/set attribute
   OS_SEM_Post ( &NWK_AttributeSem_ );
} /*lint !e715 */

/*!
 *  Create a Get Request and send to the NWK
 */
NWK_GetConf_t NWK_GetRequest(NWK_ATTRIBUTES_e eAttribute)
{
   NWK_GetConf_t confirm;

   (void)memset(&confirm, 0, sizeof(confirm));

   confirm.eStatus = eNWK_GET_SERVICE_UNAVAILABLE;

   OS_MUTEX_Lock( &NWK_AttributeMutex_ ); // Function will not return if it fails

   /* 2017/05/09 [SMG] Cannot post a message to ourselves and expect a response.  So if current task is the NWK task,
      then get the attribute directly, otherwise post a message to the NWK task.
      This was added since engineering data bubbleup is being performed by the NWK task but should now be the SM task.
      This intended to be a temporary workaround.
      TODO in V1.50.xx: Move callback and all associated functions for engineering data bubble up timer from NWK task
                        to SM task.
   */
   if ( OS_TASK_IsCurrentTask ( (char *)pTskName_Nwk ) )
   {
      // Request message from Network Interface
      NWK_GetReq_t   getReq;
      getReq.eAttribute          = eAttribute;
      ConfirmMsg.Type            = eNWK_GET_CONF;  //This isn't really needed here, just trying to be consistent
      ConfirmMsg.GetConf.eStatus = NWK_Attribute_Get( &getReq, &ConfirmMsg.GetConf.val);
      confirm                    = ConfirmMsg.GetConf; // Save data
   }
   else
   {
      // Allocate the buffer
      buffer_t *pBuf = ReqBuffer_Alloc(eNWK_GET_REQ, 0);
      if (pBuf != NULL)
      {
         NWK_Request_t *pReq     = (NWK_Request_t *)pBuf->data; /*lint !e740 !e826 */
         pReq->pConfirmHandler   = NWK_GetSetRequest_CB;
         pReq->handleId          = NextRequestHandle();
         pReq->Service.GetReq.eAttribute = eAttribute;
         NWK_Request(pBuf); // Function will not return if it fails
         // Wait for NWK to retrieve attribute
         if (OS_SEM_Pend ( &NWK_AttributeSem_, 10*ONE_SEC ))
         {
            confirm = ConfirmMsg.GetConf; // Save data
         }
      }
   }
   OS_MUTEX_Unlock( &NWK_AttributeMutex_ ); // Function will not return if it fails

   if (confirm.eStatus != eNWK_GET_SUCCESS) { /*lint !e456 Two execution paths are being combined with different mutex
                                                lock states */
      INFO_printf("ERROR: NWK_GetRequest: status %s, attribute %s",
                     NWK_GetStatusMsg(confirm.eStatus), nwk_attr_names[eAttribute]);
   }

   return confirm; /*lint !e454 A thread mutex has been locked but not unlocked */
}

/*!
 *  Create a Set Request and send to the NWK
 */
NWK_SetConf_t NWK_SetRequest(NWK_ATTRIBUTES_e eAttribute, void const *val)
{
   NWK_SetConf_t confirm;

   (void)memset(&confirm, 0, sizeof(confirm));

   confirm.eStatus = eNWK_SET_SERVICE_UNAVAILABLE;

   OS_MUTEX_Lock( &NWK_AttributeMutex_ ); // Function will not return if it fails
   // Allocate the buffer
   buffer_t *pBuf = ReqBuffer_Alloc(eNWK_SET_REQ, 0);
   if (pBuf != NULL)
   {
      NWK_Request_t *pReq = (NWK_Request_t *)pBuf->data; /*lint !e740 !e826 */
      pReq->pConfirmHandler   = NWK_GetSetRequest_CB;
      pReq->handleId          = NextRequestHandle();
      pReq->Service.SetReq.eAttribute = eAttribute;
      (void)memcpy(&pReq->Service.SetReq.val, val, sizeof(NWK_ATTRIBUTES_u)); /*lint !e420 Apparent access beyond array for
                                                                        function 'memcpy' */
      NWK_Request(pBuf); // Function will not return if it fails
      // Wait for NWK to retrieve attribute
      if (OS_SEM_Pend ( &NWK_AttributeSem_, 10*ONE_SEC )) {
         confirm = ConfirmMsg.SetConf; // Save data
      }
   }
   OS_MUTEX_Unlock( &NWK_AttributeMutex_ ); // Function will not return if it fails

   if (confirm.eStatus != eNWK_SET_SUCCESS) { /*lint !e456 Two execution paths are being combined with different mutex
                                                lock states */
      INFO_printf("ERROR: NWK_SetRequest: status %s, attribute %s",
                     NWK_SetStatusMsg(confirm.eStatus), nwk_attr_names[eAttribute]);
   }

   return confirm; /*lint !e454 A thread mutex has been locked but not unlocked */
}

/*!
 *  Create a Reset Request and send to the MAC
 */
uint32_t NWK_ResetRequest(NWK_RESET_e eType, NWK_ConfirmHandler confirm_cb)
{
   // Allocate the buffer
   buffer_t *pBuf = ReqBuffer_Alloc(eNWK_RESET_REQ, 0);
   if (pBuf != NULL)
   {
      NWK_Request_t *pReq = (NWK_Request_t *)pBuf->data; /*lint !e740 !e826 */
      pReq->handleId = NextRequestHandle();
      pReq->pConfirmHandler = confirm_cb;

      pReq->MsgType   = eNWK_RESET_REQ;
      pReq->Service.ResetReq.eType = eType;
//    NWK_Request(pBuf);
      OS_MSGQ_Post(&NWK_msgQueue, (void *)pBuf); // Function will not return if it fails
      return pReq->handleId;
   }
   return NWK_INVALID_HANDLE_ID;
}


/*!
 *  Create a Data Request and send to the NWK
 */
uint32_t NWK_DataRequest(
   uint8_t port,  /* UDP port number   */
   uint8_t qos,
   uint16_t size,
   uint8_t  const data[],
   NWK_Address_t const *dst_addr,
   NWK_Override_t const *override,
   MAC_dataConfCallback callback,
   NWK_ConfirmHandler confirm_cb)
{
   // Allocate the buffer
   buffer_t *pBuf = ReqBuffer_Alloc(eNWK_DATA_REQ, size);
   if (pBuf != NULL)
   {
      NWK_Request_t *pReq = (NWK_Request_t *)pBuf->data; /*lint !e740 !e826 */
      pReq->handleId = NextRequestHandle();
      pReq->pConfirmHandler = confirm_cb;

      pReq->Service.DataReq.qos           = qos;
      pReq->Service.DataReq.payloadLength = size;
      pReq->Service.DataReq.callback      = callback;
      pReq->Service.DataReq.port          = port;
      pReq->Service.DataReq.dstAddress    = *dst_addr;
      pReq->Service.DataReq.override      = *override;
      (void) memcpy(pReq->Service.DataReq.data, data, size);

      OS_MSGQ_Post(&NWK_msgQueue, (void *)pBuf); // Function will not return if it fails
      return pReq->handleId;
   }
   return NWK_INVALID_HANDLE_ID;
}

#if 0
bool NWK_DataIndication(uint8_t qos, uint8_t *data, uint16_t length, NWK_Address_t *dst_addr,
                        NWK_Address_t *src_addr, NWK_ConfirmHandler confirm_cb)
{
   buffer_t *pBuf = ReqBuffer_Alloc(eNWK_DATA_REQ);
   if (pBuf != NULL)
   {
      NWK_Indication_t *pMsg = (NWK_Indication_t *)pBuf->data; /*lint !e740 !e826 */

      STACK_DataInd_t* stack_indication = (STACK_DataInd_t*)pBuf->data; /*lint !e826 data holds the STACK_DataInd_t */

      pBuf->eSysFmt = eSYSFMT_NWK_INDICATION;

//    stack_indication->timeStamp = recordEntry->timeStamp;
      stack_indication->qos = qos;
      stack_indication->payloadLength = length;
      (void)memcpy(&stack_indication->dstAddr, dst_addr, sizeof(stack_indication->dstAddr));
      (void)memcpy(&stack_indication->srcAddr, src_addr, sizeof(stack_indication->srcAddr));
      stack_indication->payload = &pBuf->data[(sizeof(STACK_DataInd_t))];
      (void)memcpy(stack_indication->payload, data, length);
//    STACK_SendDataIndication(stack_buffer);
   }
}
#endif


/*!
 * Returns the Message for GET Status
 */
static char* NWK_GetStatusMsg(NWK_GET_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eNWK_GET_SUCCESS:              msg = "NWK_GET_SUCCESS";              break;
      case eNWK_GET_UNSUPPORTED:          msg = "NWK_GET_UNSUPPORTED";          break;
      case eNWK_GET_SERVICE_UNAVAILABLE:  msg = "NWK_GET_SERVICE_UNAVAILABLE";  break;
   }
   return msg;
}

/*!
 * Returns the Message for SET Status
 */
static char* NWK_SetStatusMsg(NWK_SET_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eNWK_SET_SUCCESS:              msg = "NWK_SET_SUCCESS";              break;
      case eNWK_SET_READONLY:             msg = "NWK_SET_READONLY";             break;
      case eNWK_SET_UNSUPPORTED:          msg = "NWK_SET_UNSUPPORTED";          break;
      case eNWK_SET_INVALID_PARAMETER:    msg = "NWK_SET_INVALID_PARAMETER";    break;
      case eNWK_SET_SERVICE_UNAVAILABLE:  msg = "NWK_SET_SERVICE_UNAVAILABLE";  break;
   }
   return msg;
}

static char* NWK_ResetStatusMsg(NWK_RESET_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eNWK_RESET_SUCCESS:              msg = "NWK_RESET_SUCCESS";              break;
      case eNWK_RESET_FAILURE:              msg = "NWK_RESET_FAILURE";              break;
   }
   return msg;
}

static char* NWK_StartStatusMsg(NWK_START_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eNWK_START_SUCCESS:               msg = "NWK_START_SUCCESS";                break;
      case eNWK_START_NETWORK_RUNNING:       msg = "NWK_START_RUNNING";                break;
      case eNWK_START_PARTIAL_LINK_RUNNING:  msg = "NWK_START_PARTIAL_LINK_RUNNING";   break;
      case eNWK_START_NO_MAC_START:          msg = "NWK_START_NO_MAC_START";           break;
   }
   return msg;
}

static char* NWK_StopStatusMsg(NWK_STOP_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eNWK_STOP_SUCCESS: msg = "NWK_STOP_SUCCESS";              break;
      case eNWK_STOP_ERROR:   msg = "NWK_STOP_ERROR";                break;
   }
   return msg;
}

static char* NWK_DataStatusMsg(NWK_DATA_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eNWK_DATA_SUCCESS:                msg = "NWK_DATA_SUCCESS";              break;
      case eNWK_DATA_NO_ROUTE:               msg = "NWK_DATA_NO_ROUTE";             break;
      case eNWK_DATA_MAC_IDLE:               msg = "NWK_DATA_MAC_IDLE";             break;
      case eNWK_DATA_TRANSACTION_OVERFLOW:   msg = "NWK_DATA_TRANSACTION_OVERFLOW"; break;
      case eNWK_DATA_TRANSACTION_FAILED:     msg = "NWK_DATA_TRANSACTION_FAILED";   break;
      case eNWK_DATA_INVALID_PARAMETER:      msg = "NWK_DATA_INVALID_PARAMETER";    break;
      case eNWK_DATA_INVALID_HANDLE:         msg = "NWK_DATA_INVALID_HANDLE";       break;
   }
   return msg;
}

#if ( FAKE_TRAFFIC == 1 )
/***********************************************************************************************************************
Function Name: STACK_RxFakeRecords

Purpose: generate fake records based on the number currently configured in ConfigAttr.ipBhaulGenCount

Arguments: none

Returns: returnStatus_t - success or failure
***********************************************************************************************************************/
static void STACK_RxFakeRecords( void )
{
   uint8_t i;
   NWK_GetConf_t GetConf;
   HEEP_APPHDR_t fakeTxHeader;
   buffer_t* pBuf;
   static uint16_t RecCount = 0; // Record count

   GetConf = NWK_GetRequest( eNwkAttr_ipBhaulGenCount );
   for (i=1; i < (GetConf.val.ipBhaulGenCount + 1); i++)
   {
      // generate a records worth of fake RX data

      // Initialize the header
      HEEP_initHeader( &fakeTxHeader );
      fakeTxHeader.appSecurityAuthMode = 0;
      fakeTxHeader.callback = NULL;
      fakeTxHeader.TransactionType = (uint8_t)TT_ASYNC;
      fakeTxHeader.Resource = (uint8_t)bu_en;
      fakeTxHeader.Method_Status = (uint8_t)method_post;
      fakeTxHeader.Req_Resp_ID = (uint8_t)0;
      fakeTxHeader.qos = 0;   // HI RE

      pBuf = BM_alloc( FAKE_SRFN_MESSAGE_SIZE );  //OK to use an App buffer since simulating a App task sending a msg
      if (pBuf != NULL)
      {
         (void)memset(pBuf->data, i, FAKE_SRFN_MESSAGE_SIZE);
         pBuf->data[0] = RecCount >> 8;
         pBuf->data[1] = RecCount & 0xFF;
         pBuf->x.dataLen = FAKE_SRFN_MESSAGE_SIZE;
         (void)HEEP_MSG_Tx(&fakeTxHeader, pBuf);     // The called function will free the buffer
         RecCount++;
      }


   }

}
#endif
