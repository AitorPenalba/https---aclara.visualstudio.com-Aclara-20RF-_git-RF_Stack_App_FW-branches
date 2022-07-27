/** ****************************************************************************
@file b2b_req_handler.c

Implements the SRFN DA specific handling of incoming B2B requests/data.

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.
*******************************************************************************/

/*******************************************************************************
#INCLUDES
*******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "project.h" // required for a lot of random headers so be safe and include 1st
#include "b2bmessage.h"
#include "DBG_SerialDebug.h"
#include "endpt_cim_cmd.h"
#include "hdlc_frame.h"
#include "heep_util.h"
#include "tunnel_util.h"
#include "sm_protocol.h"
#include "sm.h"
#include "app_msg_handler.h"
#include "evl_event_log.h"
#include "mode_config.h"
#include "phy.h"
#include "mac.h"
#include "partitions.h"
#include "pwr_task.h"
#include "dfw_app.h"
#include "pwr_task.h"
#include "fio.h"
#include "ecc108_lib_return_codes.h"
#include "ecc108_apps.h"
#include "ecc108_mqx.h"
#include "time_dst.h"

#include "b2b_req_handler.h"

/*******************************************************************************
Local #defines (Object-like macros)
*******************************************************************************/

/*******************************************************************************
Local #defines (Function-like macros)
*******************************************************************************/

#define B2B_MTR_TYPE_SZ 2

/*******************************************************************************
Local Struct, Typedef, and Enum Definitions (Private to this file)
*******************************************************************************/

typedef returnStatus_t (*paramHandler)(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr);

typedef struct
{
  paramHandler pHandler;
  meterReadingType rType;
} req_handler_t;

/*******************************************************************************
Static Function Prototypes
*******************************************************************************/

static void HandleGetReq(meterReadingType type, uint16_t handle);
static void HandleSetReq(meterReadingType type, uint16_t handle, void *params, uint16_t len);

static returnStatus_t B2B_Handler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr);
static returnStatus_t SetPhyRxDetection(uint16_t *value, uint16_t numVal);
static returnStatus_t SetPhyRxFraming(uint16_t *value, uint16_t numVal);
static returnStatus_t SetPhyRxMode(uint16_t *value, uint16_t numVal);
static returnStatus_t SetPhyAvailableFrequencies(void *value, uint16_t numVal);
static returnStatus_t SetPhyTxFrequencies(void *value, uint16_t numVal);
static returnStatus_t SetPhyRxFrequencies(void *value, uint16_t numVal);
static returnStatus_t SetMacChannelSets(void *value, uint16_t numVal);
static returnStatus_t SetFlashSecurityEnabled(uint8_t mode);
static returnStatus_t GetShipMode(uint8_t *mode, OR_PM_Attr_t *attr);
static returnStatus_t SetShipMode(uint8_t mode);
static returnStatus_t SetDtlsNetworkRootCA(uint8_t *value, uint16_t numVal);
static returnStatus_t SetDtlsNetworkHESubject(uint8_t *value, uint16_t numVal);
static returnStatus_t SetDtlsNetworkMSSubject(uint8_t *value, uint16_t numVal);

/*******************************************************************************
Global Variable Definitions
*******************************************************************************/

/*******************************************************************************
Static Variable Definitions
*******************************************************************************/

static const req_handler_t GetHandlers[] =
{
   { MAC_OR_PM_Handler, macNetworkId },
   { PHY_OR_PM_Handler, phyRxDetection },
   { PHY_OR_PM_Handler, phyRxFraming },
   { PHY_OR_PM_Handler, phyRxMode },
   { PHY_OR_PM_Handler, phyAvailableFrequencies },
   { PHY_OR_PM_Handler, phyTxFrequencies },
   { PHY_OR_PM_Handler, phyRxFrequencies },
   { MAC_OR_PM_Handler, macChannelSets },
   { APP_MSG_SecurityHandler, appSecurityAuthMode },
   { HEEP_util_OR_PM_Handler, flashSecurityEnabled },
   { NWK_OR_PM_Handler, ipHEContext },
   { EVL_OR_PM_Handler, realtimeThreshold },
   { B2B_Handler, shipMode },
   { TIME_SYS_OR_PM_Handler, timeAcceptanceDelay },
   { DST_timeZoneOffsetHandler, timeZoneOffset },
   { DST_OR_PM_timeZoneDstHashHander, timeZoneDstHash },
   { ENDPT_CIM_CMD_OR_PM_Handler, comDeviceFirmwareVersion },
   { ENDPT_CIM_CMD_OR_PM_Handler, comDeviceHardwareVersion },
   { ENDPT_CIM_CMD_OR_PM_Handler, comDeviceMACAddress },
   { ENDPT_CIM_CMD_OR_PM_Handler, comDeviceMicroMPN },
   { NULL , invalidReadingType }
};

static const req_handler_t SetHandlers[] =
{
   { MAC_OR_PM_Handler, macNetworkId },
   { B2B_Handler, phyRxDetection },
   { B2B_Handler, phyRxFraming },
   { B2B_Handler, phyRxMode },
   { B2B_Handler, phyAvailableFrequencies },
   { B2B_Handler, phyTxFrequencies },
   { B2B_Handler, phyRxFrequencies },
   { B2B_Handler, macChannelSets },
   { APP_MSG_SecurityHandler, appSecurityAuthMode },
   { B2B_Handler, flashSecurityEnabled },
   { NWK_OR_PM_Handler, ipHEContext },
   { EVL_OR_PM_Handler, realtimeThreshold },
   { B2B_Handler, shipMode },
   { TIME_SYS_OR_PM_Handler, timeAcceptanceDelay },
   { DST_timeZoneOffsetHandler, timeZoneOffset },
   { B2B_Handler, dtlsNetworkRootCA },
   { B2B_Handler, dtlsNetworkHESubject },
   { B2B_Handler, dtlsNetworkMSSubject },
   { NULL , invalidReadingType }
};

/*******************************************************************************
Function Definitions
*******************************************************************************/

/**
Handles non-response traffic from the Host board.

@param header Any parsed B2B header information.
@param params Pointer to any B2B data.
*/
void HandleB2BRequest(B2BPacketHeader_t header, const uint8_t *params)
{
   // this is a request or passthrough command so it requires per type handling
   switch (header.messageType)   /*lint !e788 not all enums used within switch */
   {
      case B2B_MT_GET_REQ:
      {
         meterReadingType readType = (meterReadingType)B2BGetUint16(params);
         HandleGetReq(readType, header.handle);
         break;
      }
      case B2B_MT_SET_REQ:
      {
         meterReadingType setType = (meterReadingType)B2BGetUint16(params);
         params += B2B_MTR_TYPE_SZ;
         HandleSetReq(setType, header.handle, (void*)params, header.length - (B2B_PKT_HDR_SZ + B2B_MTR_TYPE_SZ));
         break;
      }
      case B2B_MT_TIME_REQ:
         (void)B2BSendTimeResp(header.handle);
         break;
      case B2B_MT_RESET:
         PWR_SafeReset();
         break;
      case B2B_MT_ECHO_REQ:
         (void)B2BSendEchoResp(header.handle);
         break;
      case B2B_MT_REFRESH_NW_REQ:
         (void)SM_RefreshRequest();
         break;
      default:
         // nothing to do for other cases
         ERR_printf("B2B drop unhandled type %u", header.messageType);
         break;
   }  /*lint !e788 not all enums used within switch */
}

/**
Handles non-B2B traffic from the Host board.

@param addr Address the frame arrived on.
@param data Pointer to frame data.
@param len Number of bytes in frame.
*/
void HandleNonB2BRequest(hdlc_addr_t addr, const uint8_t *data, uint16_t len)
{
   switch (addr)  /*lint !e788 not all enums used within switch */
   {
#if (USE_IPTUNNEL == 1)
      case NW_HDLC_ADDR:
         // send any tunnel data directly over the SRFN network
         if (TUNNEL_MSG_Tx(data, len) != eSUCCESS)
         {
            ERR_printf("B2B IP tunnel fail");
         }
         break;
#endif
      default:
         // not handling other addresses
         break;
   }  /*lint !e788 not all enums used within switch */
}

/**
Handles a B2B read request and sends the appropriate response.

@param type The item to read
@param handle B2B request handle
*/
static void HandleGetReq(meterReadingType type, uint16_t handle)
{
   bool handled = false;
   OR_PM_Attr_t attr;
   req_handler_t *handler;
   returnStatus_t result = eFAILURE;
   buffer_t *buffer = BM_alloc(MAX_OR_PM_PAYLOAD_SIZE);

   if (buffer == NULL)
   {
      (void)B2BSendGetResp(B2B_GET_GEN_ERR, handle, type, NULL, 0);
      return;
   }

   for (handler = (req_handler_t*)GetHandlers; handler->pHandler != NULL; handler++)
   {
      if (handler->rType == type)
      {
         handled = true;
         result = handler->pHandler(method_get, type, buffer->data, &attr);
         break;
      }
   }

   if (!handled)
   {
      (void)B2BSendGetResp(B2B_GET_NOT_SUPP, handle, type, NULL, 0);
   }
   else if (result != eSUCCESS)
   {
      (void)B2BSendGetResp(B2B_GET_GEN_ERR, handle, type, NULL, 0);
   }
   else
   {
      /*lint -e{644,645} if we get here the handler set buffer and attr */
      (void)B2BSendGetResp(B2B_GET_SUCCESS, handle, type, buffer->data, attr.rValLen);
   }

   BM_free(buffer);
}

/**
Handles setting a value using the B2B protocol and sends the appropriate response.

@param type The item to write
@param handle B2B request handle
@param params Pointer to the new values
@param len Number of bytes in the data being written
*/
static void HandleSetReq(meterReadingType type, uint16_t handle, void *params, uint16_t len)
{
   bool handled = false;
   OR_PM_Attr_t attr;
   req_handler_t *handler;
   returnStatus_t result = eFAILURE;

   attr.rValLen = len;

   for (handler = (req_handler_t*)SetHandlers; handler->pHandler != NULL; handler++)
   {
      if (handler->rType == type)
      {
         handled = true;
         result = handler->pHandler(method_put, type, params, &attr);
         break;
      }
   }

   if (!handled)
   {
      (void)B2BSendSetResp(B2B_SET_NOT_SUPP, handle, type, NULL, 0);
   }
   else if (result != eSUCCESS)
   {
      (void)B2BSendSetResp(B2B_SET_GEN_ERR, handle, type, NULL, 0);
   }
   else
   {
      (void)B2BSendSetResp(B2B_SET_SUCCESS, handle, type, params, len);
   }
}

/**
Get/Set commands not handled via existing OTA methods

@param action get or put
@param id HEEP enum associated with the value
@param value [in, out] pointer to new value or data storage location
@param attr [in, out] pointer to data attributes, may be NULL on get if do not want the result

@return If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
*/
static returnStatus_t B2B_Handler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr)
{
   returnStatus_t retVal = eAPP_NOT_HANDLED;

   if (action == method_get)
   {
      switch (id) /*lint !e787 not all enums used within switch */
      {
         case shipMode:
            retVal = GetShipMode(value, attr);
            break;
      }/*lint !e787 not all enums used within switch */
   }
   else if (action == method_put)
   {
      switch (id)/*lint !e787 not all enums used within switch */
      {
         case phyRxDetection:
            retVal = SetPhyRxDetection(value, attr->rValLen / sizeof(uint16_t));
            break;
         case phyRxFraming:
            retVal = SetPhyRxFraming(value, attr->rValLen / sizeof(uint16_t));
            break;
         case phyRxMode:
            retVal = SetPhyRxMode(value, attr->rValLen / sizeof(uint16_t));
            break;
         case phyAvailableFrequencies:
            retVal = SetPhyAvailableFrequencies(value, attr->rValLen / sizeof(uint32_t));
            break;
         case phyTxFrequencies:
            retVal = SetPhyTxFrequencies(value, attr->rValLen / sizeof(uint32_t));
            break;
         case phyRxFrequencies:
            retVal = SetPhyRxFrequencies(value, attr->rValLen / sizeof(uint32_t));
            break;
         case macChannelSets:
            retVal = SetMacChannelSets(value, attr->rValLen / sizeof(uint16_t));
            break;
         case flashSecurityEnabled:
            retVal = SetFlashSecurityEnabled(*(uint8_t*)value);
            break;
         case shipMode:
            retVal = SetShipMode(*(uint8_t*)value);
            break;
         case dtlsNetworkRootCA:
            retVal = SetDtlsNetworkRootCA(value, attr->rValLen);
            break;
         case dtlsNetworkHESubject:
            retVal = SetDtlsNetworkHESubject(value, attr->rValLen);
            break;
         case dtlsNetworkMSSubject:
            retVal = SetDtlsNetworkMSSubject(value, attr->rValLen);
            break;
      }/*lint !e787 not all enums used within switch */
   }

   return retVal;
}

/**
Sets the preamble and sync mode used for listening on the radio

@param value Pointer to the modes
@param numVal Number of modes in the list

@return eSUCCESS on success else failure code.
*/
static returnStatus_t SetPhyRxDetection(uint16_t *value, uint16_t numVal)
{
   PHY_ATTRIBUTES_u phy_attr;

   if ((numVal == 0) || (numVal > PHY_RCVR_COUNT))
   {
      return eFAILURE;
   }

   // make sure access is aligned
   uint16_t rxDetection[PHY_RCVR_COUNT];
   memcpy(rxDetection, value, numVal * sizeof(uint16_t));

   uint8_t i = 0;

   // validate all inputs
   for ( ; i < numVal; i++)
   {
      if ((PHY_DETECTION_e)rxDetection[i] >= ePHY_DETECTION_LAST)
      {
         return eFAILURE;
      }
   }

   i = 0;

   for ( ; i < numVal; i++)
   {
      phy_attr.RxDetectionConfig[i] = (PHY_DETECTION_e)rxDetection[i];
   }

   for ( ; i < PHY_RCVR_COUNT; i++)
   {
      phy_attr.RxDetectionConfig[i] = ePHY_DETECTION_0;
   }

   /*lint -e{772} set just above */
   PHY_SetConf_t confirm = PHY_SetRequest(ePhyAttr_RxDetectionConfig, (uint8_t *)&phy_attr);

   return (confirm.eStatus == ePHY_SET_SUCCESS) ? eSUCCESS : eFAILURE;
}

/**
Sets the framing mode used for radio transmissions

@param value Pointer to the modes
@param numVal Number of modes in the list

@return eSUCCESS on success else failure code.
*/
static returnStatus_t SetPhyRxFraming(uint16_t *value, uint16_t numVal)
{
   PHY_ATTRIBUTES_u phy_attr;

   if ((numVal == 0) || (numVal > PHY_RCVR_COUNT))
   {
      return eFAILURE;
   }

   // make sure access is aligned
   uint16_t rxFraming[PHY_RCVR_COUNT];
   memcpy(rxFraming, value, numVal * sizeof(uint16_t));

   uint8_t i = 0;

   // validate all inputs
   for ( ; i < numVal; i++)
   {
      if ((PHY_FRAMING_e)rxFraming[i] >= ePHY_FRAMING_LAST)
      {
         return eFAILURE;
      }
   }

   i = 0;

   for ( ; i < numVal; i++)
   {
      phy_attr.RxFramingConfig[i] = (PHY_FRAMING_e)rxFraming[i];
   }

   for ( ; i < PHY_RCVR_COUNT; i++)
   {
      phy_attr.RxFramingConfig[i] = ePHY_FRAMING_0;
   }

   /*lint -e{772} set just above */
   PHY_SetConf_t confirm = PHY_SetRequest(ePhyAttr_RxFramingConfig, (uint8_t *)&phy_attr);

   return (confirm.eStatus == ePHY_SET_SUCCESS) ? eSUCCESS : eFAILURE;
}

/**
Sets the GFSK mode used for listening on the radio

@param value Pointer to the modes
@param numVal Number of modes in the list

@return eSUCCESS on success else failure code.
*/
static returnStatus_t SetPhyRxMode(uint16_t *value, uint16_t numVal)
{
   PHY_ATTRIBUTES_u phy_attr;

   if ((numVal == 0) || (numVal > PHY_RCVR_COUNT))
   {
      return eFAILURE;
   }

   // make sure access is aligned
   uint16_t rxMode[PHY_RCVR_COUNT];
   memcpy(rxMode, value, numVal * sizeof(uint16_t));

   uint8_t i = 0;

   // validate all inputs
   for ( ; i < numVal; i++)
   {
      if ((PHY_MODE_e)rxMode[i] >= ePHY_MODE_LAST)
      {
         return eFAILURE;
      }
   }

   i = 0;

   for ( ; i < numVal; i++)
   {
      phy_attr.RxMode[i] = (PHY_MODE_e)rxMode[i];
   }

   for ( ; i < PHY_RCVR_COUNT; i++)
   {
      phy_attr.RxMode[i] = (PHY_MODE_e)ePHY_FRAMING_0;
   }

   /*lint -e{772} set just above */
   PHY_SetConf_t confirm = PHY_SetRequest(ePhyAttr_RxMode, (uint8_t *)&phy_attr);

   return (confirm.eStatus == ePHY_SET_SUCCESS) ? eSUCCESS : eFAILURE;
}

/**
Sets the allowed RX and TX frequencies for the radio.

@param value Pointer to the frequencies
@param numVal Number of frequencies in the list

@return eSUCCESS on success else failure code.
*/
static returnStatus_t SetPhyAvailableFrequencies(void *value, uint16_t numVal)
{
   if ((numVal == 0) || (numVal > PHY_MAX_CHANNEL))
   {
      return eFAILURE;
   }

   // make sure access is aligned
   uint32_t pFrequency[PHY_MAX_CHANNEL];
   memcpy(pFrequency, value, numVal * sizeof(uint32_t));

   uint8_t i = 0;
   bool successful = true;

   for ( ; i < numVal; i++)
   {
      // must validate frequencies
      if ((pFrequency[i] >= PHY_MIN_FREQ ) && (pFrequency[i] <= PHY_MAX_FREQ))
      {
         successful = successful && PHY_Channel_Set(i, FREQ_TO_CHANNEL(pFrequency[i]));
      }
      else
      {
         // invalidate channels with bad frequencies
         successful = successful && PHY_Channel_Set(i, PHY_INVALID_CHANNEL);
      }
   }

   for ( ; i < PHY_MAX_CHANNEL; i++)
   {
      successful = successful && PHY_Channel_Set(i, PHY_INVALID_CHANNEL);
   }

   return (successful) ? eSUCCESS : eFAILURE;
}

/**
Sets the TX frequencies for the radio.

@param value Pointer to the frequencies
@param numVal Number of frequencies in the list

@return eSUCCESS on success else failure code.
*/
static returnStatus_t SetPhyTxFrequencies(void *value, uint16_t numVal)
{
   PHY_ATTRIBUTES_u phy_attr;

   if ((numVal == 0) || (numVal > PHY_MAX_CHANNEL))
   {
      return eFAILURE;
   }

   // make sure access is aligned
   uint32_t pFrequency[PHY_MAX_CHANNEL];
   memcpy(pFrequency, value, numVal * sizeof(uint32_t));

   uint8_t i = 0;

   for ( ; i < numVal; i++)
   {
      // must validate frequencies
      if ((pFrequency[i] >= PHY_MIN_FREQ ) && (pFrequency[i] <= PHY_MAX_FREQ) && PHY_IsChannelValid(FREQ_TO_CHANNEL(pFrequency[i])))
      {
         phy_attr.TxChannels[i] = FREQ_TO_CHANNEL(pFrequency[i]);
      }
      else
      {
         // invalidate channels with bad frequencies
         phy_attr.TxChannels[i] = PHY_INVALID_CHANNEL;
      }
   }

   for ( ; i < PHY_MAX_CHANNEL; i++)
   {
      phy_attr.TxChannels[i] = PHY_INVALID_CHANNEL;
   }

   /*lint -e{772} set just above */
   PHY_SetConf_t confirm = PHY_SetRequest(ePhyAttr_TxChannels, (uint8_t *)phy_attr.TxChannels);

   return (confirm.eStatus == ePHY_SET_SUCCESS) ? eSUCCESS : eFAILURE;
}

/**
Sets the RX frequencies for the radio.

@param value Pointer to the frequencies
@param numVal Number of frequencies in the list

@return eSUCCESS on success else failure code.
*/
static returnStatus_t SetPhyRxFrequencies(void *value, uint16_t numVal)
{
   PHY_ATTRIBUTES_u phy_attr;

   if ((numVal == 0) || (numVal > PHY_RCVR_COUNT))
   {
      return eFAILURE;
   }

   // make sure access is aligned
   uint32_t pFrequency[PHY_RCVR_COUNT];
   memcpy(pFrequency, value, numVal * sizeof(uint32_t));

   uint8_t i = 0;
   for ( ; i < numVal; i++)
   {
      // must validate frequencies
      if ((pFrequency[i] >= PHY_MIN_FREQ ) && (pFrequency[i] <= PHY_MAX_FREQ) && PHY_IsChannelValid(FREQ_TO_CHANNEL(pFrequency[i])))
      {
         phy_attr.RxChannels[i] = FREQ_TO_CHANNEL(pFrequency[i]);
      }
      else
      {
         // invalidate channels with bad frequencies
         phy_attr.RxChannels[i] = PHY_INVALID_CHANNEL;
      }
   }

   for ( ; i < PHY_RCVR_COUNT; i++)
   {
      phy_attr.RxChannels[i] = PHY_INVALID_CHANNEL;
   }

   /*lint -e{772} set just above */
   PHY_SetConf_t confirm = PHY_SetRequest(ePhyAttr_RxChannels, (uint8_t *)phy_attr.RxChannels);

   return (confirm.eStatus == ePHY_SET_SUCCESS) ? eSUCCESS : eFAILURE;
}

/**
Sets the allowed channel sets on the radio

@param value Pointer to the channels
@param numVal Number of channels in the list

@return eSUCCESS on success else failure code.
*/
static returnStatus_t SetMacChannelSets(void *value, uint16_t numVal)
{
   if ((numVal == 0) || (numVal % 2 != 0) || (numVal > (MAC_MAX_CHANNEL_SETS_SRFN * 2)))
   {
      // invalid number of channels
      return eFAILURE;
   }

   CH_SETS_t sets[MAC_MAX_CHANNEL_SETS_SRFN];

   // make sure access is aligned
   uint16_t pChan[MAC_MAX_CHANNEL_SETS_SRFN * 2];
   memcpy(pChan, value, numVal * sizeof(uint16_t));

   uint8_t i = 0;
   for ( ; i < (numVal / 2); i++)
   {
      sets[i].start = pChan[i * 2];
      sets[i].stop = pChan[(i * 2) + 1];
   }

   for ( ; i < MAC_MAX_CHANNEL_SETS_SRFN; i++)
   {
      sets[i].start = PHY_INVALID_CHANNEL;
      sets[i].stop = PHY_INVALID_CHANNEL;
   }

   /*lint -e{772} set just above */
   MAC_SetConf_t confirm = MAC_SetRequest(eMacAttr_ChannelSetsSRFN, sets);

   return (confirm.eStatus == eMAC_SET_SUCCESS) ? eSUCCESS : eFAILURE;
}

/**
Sets the flash security mode.

@param mode 1 for lock 0 for unlock.

@return eSUCCESS on success else failure code.

@note If the mode is modified from the current setting the device will reboot.
*/
static returnStatus_t SetFlashSecurityEnabled(uint8_t mode)
{
   const uint8_t flashSecEnabled  = 0xFF;
   const uint8_t flashSecDisabled = 0xFE;
   const dSize destAddr = 0x40Cu;
   PartitionData_t const *pImagePTbl;
   uint8_t currFlashProt;

   if (mode > 1)
   {
      // invalid mode specified
      return eFAILURE;
   }

   if (PAR_partitionFptr.parOpen(&pImagePTbl, ePART_BL_CODE, 0L) != eSUCCESS)
   {
      // can't open the partition
      return eFAILURE;
   }

   (void)PAR_partitionFptr.parRead(&currFlashProt, destAddr, (lCnt)sizeof(currFlashProt), pImagePTbl);

   // check if this write changes the current mode
   if ((mode == 0 && currFlashProt != flashSecDisabled) || (mode == 1 && currFlashProt != flashSecEnabled))
   {
      /*
      ALL interrupts MUST be disabled since the sector containing the FSEC also
      contains the Interrupt Vector Table.  The sector will be erased when updating
      the FSEC field.  MUST use the PRIMASK form of disabling the interrupts.
      */
       __disable_interrupt();
      returnStatus_t result = PAR_partitionFptr.parWrite( destAddr, (mode == 1) ? &flashSecEnabled : &flashSecDisabled,
                                    (lCnt)sizeof(currFlashProt), pImagePTbl);
      __enable_interrupt();

      if (result == eSUCCESS)
      {
         (void)DFWA_WaitForSysIdle(3000); // No need to unlock when done since resetting

         // Keep all other tasks from running. Increase the priority of the power and idle tasks
         (void)OS_TASK_Set_Priority(pTskName_Pwr, 10);
         (void)OS_TASK_Set_Priority(pTskName_MfgUartRecv, 11);
         (void)OS_TASK_Set_Priority(pTskName_Idle, 12);

         // Does not return from here
         PWR_SafeReset();
      }
      else
      {
         return eFAILURE;
      }
   }

   return eSUCCESS;
}

/**
Gets the current ship mode setting.

@param mode [out] Pointer to storage location for mode.
@param attr [out] Stores meta data for result if not NULL.

@return eSUCCESS on success else failure code.
*/
static returnStatus_t GetShipMode(uint8_t *mode, OR_PM_Attr_t *attr)
{
   *mode = MODECFG_get_ship_mode();

   if (attr != NULL)
   {
      attr->rValLen = sizeof(uint8_t);
      attr->rValTypecast = (uint8_t)Boolean;
   }

   return eSUCCESS;
}

/**
Writes a new DTLS MS subject to the ECC108.

@param mode 1 to enter ship mode or 0 to exit.

@return eSUCCESS on success else failure code.
*/

static returnStatus_t SetShipMode(uint8_t mode)
{
   if (mode > 1)
   {
      // invalid mode specified
      return eFAILURE;
   }

   if (mode == 0)
   {
      (void)SM_StartRequest(eSM_START_STANDARD, NULL);
   }
   else
   {
      (void)SM_StartRequest(eSM_START_LOW_POWER, NULL);
   }

   return MODECFG_set_ship_mode(mode);
}

/**
Writes a new DTLS root CA to the ECC108.

@param value Pointer to the data.
@param numVal Number of bytes.

@return eSUCCESS on success else failure code.
*/
static returnStatus_t SetDtlsNetworkRootCA(uint8_t *value, uint16_t numVal)
{
   if (numVal > sizeof(NetWorkRootCA_t))
   {
      return eFAILURE;
   }

   PartitionData_t const *pPart = SEC_GetSecPartHandle();

   return PAR_partitionFptr.parWrite((dSize)offsetof(intFlashNvMap_t, dtlsNetworkRootCA), value, (lCnt)numVal, pPart);
}

/**
Writes a new DTLS HE subject to the ECC108.

@param value Pointer to the data.
@param numVal Number of bytes.

@return eSUCCESS on success else failure code.
*/
static returnStatus_t SetDtlsNetworkHESubject(uint8_t *value, uint16_t numVal)
{
   if (numVal > sizeof(HESubject_t))
   {
      return eFAILURE;
   }

   PartitionData_t const *pPart = SEC_GetSecPartHandle();

   return PAR_partitionFptr.parWrite((dSize)offsetof(intFlashNvMap_t, dtlsHESubject), value, (lCnt)numVal, pPart);
}

/**
Writes a new DTLS MS subject to the ECC108.

@param value Pointer to the data.
@param numVal Number of bytes.

@return eSUCCESS on success else failure code.
*/
static returnStatus_t SetDtlsNetworkMSSubject(uint8_t *value, uint16_t numVal)
{
   if (numVal > sizeof(MSSubject_t))
   {
      return eFAILURE;
   }

   PartitionData_t const *pPart = SEC_GetSecPartHandle();

   return PAR_partitionFptr.parWrite((dSize)offsetof(intFlashNvMap_t, dtlsMSSubject), value, (lCnt)numVal, pPart);
}
