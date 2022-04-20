/***********************************************************************************************************************
 *
 * Filename:   Stack_Manager.c
 *
 * Global Designator: SM_
 *
 * Contents:
 *
 ***********************************************************************************************************************
 * A product of Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2018 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
/* INCLUDE FILES */
#include "project.h"
#include <stdbool.h>
#if ( DCU == 1 )
#include "partitions.h"
#endif
#include "buffer.h"
#include "indication.h"
#include "PHY_Protocol.h"
#include "PHY.h"
#include "MAC.h"
#include "SM_Protocol.h"
#include "SM.h"
#include "DBG_SerialDebug.h"
#include "SM.h"
#include "time_util.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define IDLE        false
#define OPERATIONAL true

#define NW_EVENT_ACTIVITY 0x00000001
#define NW_EVENT_NEW_TIME 0x00000002
#define NW_EVENT_REFRESH  0x00000004
#define NW_EVENT_MASK (NW_EVENT_ACTIVITY | NW_EVENT_NEW_TIME | NW_EVENT_REFRESH)

#define MAX_ACTIVE_TO SECONDS_PER_DAY
#define MAX_PASS_TO SECONDS_PER_MINUTE
#if( RTOS_SELECTION == FREE_RTOS )
#define INT_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#define EXT_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define INT_NUM_MSGQ_ITEMS 0 //NRJ: TODO Figure out sizing
#define EXT_NUM_MSGQ_ITEMS 0 //NRJ: TODO Figure out sizing
#endif


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* SM Configurable attributes */
// Data to keep in cache memory
// WARNING: If a variable is added or removed, Process_ResetReq might need to be updated
typedef struct
{  /* SM Communication counters */
   TIMESTAMP_t   LastResetTime;                                /*!< Last reset timestamp. */
}SmCachedAttr_t;

typedef struct
{
   uint8_t     LinkCount                ;    /*!< */
   uint8_t     ModeMaxTransitionAttempts;    /*!< */
   TIMESTAMP_t StatsCaptureTime         ;    /*!< */
   uint8_t     StatsConfig              ;    /*!< */
   bool        StatsEnable              ;    /*!< */
   uint8_t     QueueLevel               ;    /*!< */
   bool        NetworkState;
   uint8_t     NetworkActivityTimeoutActive;
   uint32_t    NetworkActivityTimeoutPassive;
   TIMESTAMP_t NetworkStateTransitionTime;
}SmConfigAttr_t;


/* ****************************************************************************************************************** */
/* CONSTANTS */
/* These are used for printing messages */
static const char sm_attr_names[][31] =  // Don't forget to update the size using the length of the longest element + 1.
{
   [eSmAttr_LinkCount                    ] = "LinkCount",
   [eSmAttr_Mode                         ] = "Mode",
   [eSmAttr_ModeMaxTransitionAttempts    ] = "ModeMaxTransitionAttempts",
   [eSmAttr_StatsCaptureTime             ] = "StatsCaptureTime",
   [eSmAttr_StatsConfig                  ] = "StatsConfig",
   [eSmAttr_StatsEnable                  ] = "StatsEnable",
   [eSmAttr_QueueLevel                   ] = "QueueLevel",
   [eSmAttr_NetworkActivityTimeoutActive ] = "NetworkActivityTimeoutActive",
   [eSmAttr_NetworkActivityTimeoutPassive] = "NetworkActivityTimeoutPassive",
   [eSmAttr_NetworkStateTransitionTime   ] = "NetworkStateTransitionTime",
   [eSmAttr_NetworkState                 ] = "NetworkState",
};

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
static OS_MSGQ_Obj  SM_externalMsgQueue;       // message queue for incoming service requests
static OS_MSGQ_Obj  SM_internalMsgQueue;       // message queue used to layer communication during state transition

static OS_EVNT_Obj nwActivityEvent;

static IndicationObject_t smEventIndication;

static buffer_t *pRequestBuf = NULL; // Handle to unconfirmed request buffers

static SM_MODE_e smMode_;    /*!< Current mode of the stack */

static OS_MUTEX_Obj SM_AttributeMutex_;
static OS_SEM_Obj   SM_AttributeSem_;

static SM_Confirm_t ConfirmMsg;           /*!< Static Buffer used for confirmations */

//static FileHandle_t      PHYcachedFileHndl_ = {0};    /*!< Contains the file handle information. */
// static FileHandle_t      configFileHndl_ = {0}; /*!< Contains the file handle information. */

#define SM_LINK_COUNT_DEFAULT                    0      // Derived from NWK
#define SM_MODE_MAX_TRANSITION_ATTEMPTS          3      // Default = 3
#define SM_STATS_CAPTURE_TIME_DEFAULT            0      // 22:59:45
#define SM_STATS_CONFIG_DEFAULT                  0      // Default = empty
#define SM_STATS_ENABLE_DEFAULT                  false  // Default = false
#define SM_QUEUE_LEVEL_DEFAULT                   0      // Default = 0
#define SM_ACTIVITY_TIMEOUT_ACTIVE_DEFAULT       10     // seconds
#define SM_ACTIVITY_TIMEOUT_PASSIVE_DEFAULT      5400   // seconds
#define SM_NETWORK_STATE_DEFAULT                 false  // not connected
#define SM_NETWORK_STATE_TRANSITION_TIME_DEFAULT {0, 0} // Jan 1 1970

static
SmConfigAttr_t ConfigAttr =
{
  .LinkCount                     = SM_LINK_COUNT_DEFAULT          ,
  .ModeMaxTransitionAttempts     = SM_MODE_MAX_TRANSITION_ATTEMPTS,
  .StatsCaptureTime              = SM_STATS_CAPTURE_TIME_DEFAULT  ,
  .StatsConfig                   = SM_STATS_CONFIG_DEFAULT        ,
  .StatsEnable                   = SM_STATS_ENABLE_DEFAULT        ,
  .QueueLevel                    = SM_QUEUE_LEVEL_DEFAULT         ,
  .NetworkActivityTimeoutActive  = SM_ACTIVITY_TIMEOUT_ACTIVE_DEFAULT,
  .NetworkActivityTimeoutPassive = SM_ACTIVITY_TIMEOUT_PASSIVE_DEFAULT,
  .NetworkState                  = SM_NETWORK_STATE_DEFAULT,
  .NetworkStateTransitionTime    = SM_NETWORK_STATE_TRANSITION_TIME_DEFAULT, /*lint !e651 */
};

static SmCachedAttr_t CachedAttr;                 /*!< SM data saved in cached memory */

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION PROTOTYPES */
static void phy_confirm_cb(PHY_Confirm_t const *confirm, buffer_t const *pReqBuf);
static void mac_confirm_cb(MAC_Confirm_t const *confirm);
static void nwk_confirm_cb(NWK_Confirm_t const *confirm);

static bool Process_GetReq(   SM_Request_t const *pReq);
static bool Process_SetReq(   SM_Request_t const *pReq);
static bool Process_StartReq( SM_Request_t const *pReq);
static bool Process_StopReq(  SM_Request_t const *pReq);

static char* SM_GetStatusMsg(  SM_GET_STATUS_e   status);
static char* SM_SetStatusMsg(  SM_SET_STATUS_e   status);
static char* SM_StartStatusMsg(SM_START_STATUS_e status);
static char* SM_StopStatusMsg( SM_STOP_STATUS_e  status);
static char* SM_RefreshStatusMsg(SM_REFRESH_STATUS_e status);

static void  SM_SendConfirm(SM_Confirm_t const *pConf, buffer_t const *pReqBuf);

static SM_SET_STATUS_e SM_Attribute_Set( SM_SetReq_t const *pSetReq); // Prototype should be only used by SM.c.
                                                                      // We don't want outsiders to call this function directly.
                                                                      // SM_SetRequest is used for that.
static SM_GET_STATUS_e SM_Attribute_Get(SM_GetReq_t const *pGetReq, SM_ATTRIBUTES_u *val);  // Prototype should be only used by SM.c.
                                                                                             // We don't want outsiders to call this function directly.
                                                                                             // SM_GetRequest is used for that.

static SM_SET_STATUS_e Set_ModeMaxTransitionAttempts( SM_ATTRIBUTES_u const *pVal);
static SM_SET_STATUS_e Set_StatsCaptureTime(          SM_ATTRIBUTES_u const *pVal);
static SM_SET_STATUS_e Set_StatsConfig     (          SM_ATTRIBUTES_u const *pVal);
static SM_SET_STATUS_e Set_StatsEnable     (          SM_ATTRIBUTES_u const *pVal);
static SM_SET_STATUS_e Set_PassiveActivityTimeout(SM_ATTRIBUTES_u const *pVal);
static SM_SET_STATUS_e Set_ActiveActivityTimeout(SM_ATTRIBUTES_u const *pVal);

static bool UpdateMode(SM_MODE_e eMode);

static void CommStatusHandler(buffer_t *indication);

static bool Process_RefreshReq(SM_Request_t const *pReq);

/* ****************************************************************************************************************** */
/* GLOBAL FUNCTION DEFINITIONS */

/***********************************************************************************************************************

 Function Name: SM_init

 Purpose: This function is called before starting the scheduler. Creates all resources needed by this task.

 Arguments: none

 Returns: eSUCCESS/eFAILURE indication

***********************************************************************************************************************/
returnStatus_t SM_init( void )
{
   returnStatus_t retVal = eFAILURE;

   // Set mode to Unknown
   smMode_ = eSM_MODE_UNKNOWN;

   // Create the external and internal message queues and other resources
   //TODO RA6: NRJ: determine if semaphores need to be counting
   if (OS_MSGQ_Create(&SM_externalMsgQueue, EXT_NUM_MSGQ_ITEMS)
      && OS_MSGQ_Create(&SM_internalMsgQueue, INT_NUM_MSGQ_ITEMS)
      && OS_MUTEX_Create(&SM_AttributeMutex_)
      && OS_SEM_Create(&SM_AttributeSem_, 0)
      && IndicationCreate(&smEventIndication)
      && OS_EVNT_Create(&nwActivityEvent)
      )
   {
      retVal = eSUCCESS;
   }

   MAC_RegisterCommStatusIndicationHandler(CommStatusHandler);

   return(retVal);
}


/***********************************************************************************************************************

 Function Name: SM_Task

 Purpose:

 Arguments: Arg0 - Not used, but required here because this is a task

 Returns: none

***********************************************************************************************************************/
void SM_Task( uint32_t Arg0 )
{
   (void)   Arg0;

   INFO_printf("SM_Task starting...");
   for (;;)
   {
      buffer_t *pReqBuf = NULL;
      if (OS_MSGQ_Pend( &SM_externalMsgQueue, (void *)&pReqBuf, OS_WAIT_FOREVER) == true)
      {
         /*
          * todo: 6/9/2016 10:57:50 AM [BAH]
          *   Figure out how to process all confirmations here rather than the start/stop functions
          *   For now the start/stops are hanlded in another location
          */
         if(pReqBuf->eSysFmt == eSYSFMT_SM_REQUEST)
         {  // Request
            if(pRequestBuf == NULL)
            {  // There are no requests that have not been confirmed
               SM_Request_t *pReq;

               bool bConfirmReady = false;
               pReq = (SM_Request_t*) &pReqBuf->data[0];  /*lint !e740 !e826 */

               switch (pReq->MsgType)
               {
                  case eSM_GET_REQ:   bConfirmReady = Process_GetReq(  pReq); break;
                  case eSM_SET_REQ:   bConfirmReady = Process_SetReq(  pReq); break;
                  case eSM_START_REQ: bConfirmReady = Process_StartReq(pReq); break;
                  case eSM_STOP_REQ:  bConfirmReady = Process_StopReq( pReq); break;
                  case eSM_REFRESH_REQ: bConfirmReady = Process_RefreshReq(pReq); break;
                  default:
                     // unsupported request type
                     INFO_printf("UnknownReq");
                     BM_free(pRequestBuf);
                     pRequestBuf = NULL;
                     break;
               }

               // Save the request buffer
               pRequestBuf = pReqBuf;

               if( bConfirmReady && ( pRequestBuf != NULL ) )
               {  // Message handler returned the confirm
                  // So send the confirmation
                  SM_SendConfirm(&ConfirmMsg, pRequestBuf);
                  BM_free(pRequestBuf);
                  pRequestBuf = NULL;
               }
            }
            else
            {  // Got a new request before previous was confirmed!
               INFO_printf("Unexpected request");
               BM_free(pReqBuf);
            }
         }
         else
         {
            INFO_printf("Unsupported eSysFmt");
            BM_free(pReqBuf);
         }
      }
   }
}

/**
Watches for network state changes and sends indications as approproiate.

@param arg0 Necessary for MQX tasks, but unused.
*/
void SM_NwState_Task(uint32_t arg0)
{
   (void)arg0;
   bool refreshState = false;

   while (true) /*lint !e716 */
   {
      bool stateChanged = false;
      uint32_t waitResult;

      if (refreshState)
      {
         waitResult = OS_EVNT_Wait(&nwActivityEvent, NW_EVENT_MASK, false, (ConfigAttr.NetworkActivityTimeoutActive * 1000L)); /*lint !e747 stdbool */
      }
      else
      {
         waitResult = OS_EVNT_Wait(&nwActivityEvent, NW_EVENT_MASK, false, (ConfigAttr.NetworkActivityTimeoutPassive * 1000L)); /*lint !e747 stdbool */
      }

      if (waitResult == 0)
      {
         // Timed out, no activity
         // if state is changing or refresh requested signal the state
         if (ConfigAttr.NetworkState || refreshState)
         {
            ConfigAttr.NetworkState = false;
            (void)TIME_UTIL_GetTimeInSecondsFormat(&ConfigAttr.NetworkStateTransitionTime);
            stateChanged = true;
            refreshState = false;
         }
      }

      if (waitResult & NW_EVENT_ACTIVITY)
      {
         // activity detected
         // if state is changing or refresh requested signal the state
         if (!ConfigAttr.NetworkState || refreshState)
         {
            ConfigAttr.NetworkState = true;
            (void)TIME_UTIL_GetTimeInSecondsFormat(&ConfigAttr.NetworkStateTransitionTime);
            stateChanged = true;
            refreshState = false;
         }
      }

      if (waitResult & NW_EVENT_REFRESH)
      {
         refreshState = true;
      }

      if (waitResult & NW_EVENT_NEW_TIME)
      {
         // New timeout set (nothing to do as new timeout is set on next wait)
      }

      if (stateChanged && IndicationHandlerRegistered(&smEventIndication))
      {
         buffer_t *indication = BM_allocStack(sizeof(SM_Indication_t));

         if (indication != NULL)
         {
            indication->eSysFmt = eSYSFMT_SM_INDICATION;
            SM_Indication_t *smInd = (SM_Indication_t*)indication->data; /*lint !e826 */
            smInd->Type = eSM_EVENT_IND;
            smInd->EventInd.type = (uint8_t)SM_EVENT_NW_STATE;
            smInd->EventInd.priority = 0;
            smInd->EventInd.time = ConfigAttr.NetworkStateTransitionTime;
            smInd->EventInd.nwState.networkConnected = ConfigAttr.NetworkState;
            IndicationSend(&smEventIndication, indication);
         }
      }
   }
}

/**
Registers a new handler for indications from the stack manager.

@param pCallback The handler to call.
*/
void SM_RegisterEventIndicationHandler(SM_IndicationHandler pCallback)
{
   (void)IndicationRegisterHandler(&smEventIndication, pCallback);
}

/*!
 *
 * This functions is called to handle SM Confirmations
 *
 * If a confirmation handler was specified in the request, it will call that handler.
 * else it will print the results of the request.
 *
 */
static void SM_SendConfirm(SM_Confirm_t const *pConfirm, buffer_t const *pReqBuf)
{
   SM_Request_t *pReq = (SM_Request_t*) &pReqBuf->data[0];  /*lint !e740 !e826 */

   if (pReq->pConfirmHandler != NULL )
   {  // Call the handler specified
      (pReq->pConfirmHandler)(pConfirm);
   }else
   {
      char* msg;
      switch(pConfirm->MsgType)
      {
      case eSM_GET_CONF    :  msg =  SM_GetStatusMsg(   pConfirm->GetConf.eStatus);   break;
      case eSM_SET_CONF    :  msg =  SM_SetStatusMsg(   pConfirm->SetConf.eStatus);   break;
      case eSM_START_CONF  :  msg =  SM_StartStatusMsg( pConfirm->StartConf.eStatus);   break;
      case eSM_STOP_CONF   :  msg =  SM_StopStatusMsg(  pConfirm->StopConf.eStatus);   break;
      case eSM_REFRESH_CONF:
         msg =  SM_RefreshStatusMsg(pConfirm->RefreshConf.eStatus);
         break;
      default              :  msg = "Unknown";
      }  /*lint !e788 not all enums are handled by the switch  */
      INFO_printf("CONFIRM:%s, handle:%d", msg, 0);
   }
}


/***********************************************************************************************************************
Function Name: SM_Request

Purpose: This function is called to send a message to SM

Arguments:

Returns: None
***********************************************************************************************************************/
static void SM_Request(buffer_t *request)
{
   // Send the message to the task's queue
   OS_MSGQ_Post(&SM_externalMsgQueue, (void *)request); // Function will not return if it fails
}

/***********************************************************************************************************************
Function Name: phy_confirm_cb

Purpose: Called by PHY layer to return a confirmation to the stack manager

Arguments: buffer_t - pointer to a buffer that contains a confirm

Returns: none
***********************************************************************************************************************/
static void phy_confirm_cb(PHY_Confirm_t const *confirm, buffer_t const *pReqBuf)
{
   (void)pReqBuf;

   // Allocate the buffer
   buffer_t *pBuf = BM_allocStack(sizeof(PHY_Confirm_t));   // Allocate the buffer
   if(pBuf != NULL )
   {
      // Set the type
      pBuf->eSysFmt = eSYSFMT_PHY_CONFIRM;

      PHY_Confirm_t *pConf = (PHY_Confirm_t *)pBuf->data; /*lint !e740 !e826 */
      (void) memcpy(pConf, confirm, sizeof(PHY_Confirm_t) );
      OS_MSGQ_Post(&SM_internalMsgQueue, (void *)pBuf); // Function will not return if it fails
   }
}

/***********************************************************************************************************************
Function Name: mac_confirm_cb

Purpose: Called by MAC layer to return a confirmation to the stack manager

Arguments: buffer_t - pointer to a buffer that contains a confirm

Returns: none
***********************************************************************************************************************/
static void mac_confirm_cb(MAC_Confirm_t const *confirm)
{
   // Allocate the buffer
   buffer_t *pBuf = BM_allocStack(sizeof(MAC_Confirm_t));   // Allocate the buffer
   if(pBuf != NULL )
   {
      // Set the type
      pBuf->eSysFmt = eSYSFMT_MAC_CONFIRM;

      MAC_Confirm_t *pConf = (MAC_Confirm_t *)pBuf->data; /*lint !e740 !e826 */
      (void) memcpy(pConf, confirm, sizeof(MAC_Confirm_t) );
      OS_MSGQ_Post(&SM_internalMsgQueue, (void *)pBuf); // Function will not return if it fails
   }
}

/***********************************************************************************************************************
Function Name: nwk_confirm_cb

Purpose: Called by NWK layer to return a confirmation to the stack manager

Arguments: buffer_t - pointer to a buffer that contains a confirm

Returns: none
***********************************************************************************************************************/
static void nwk_confirm_cb(NWK_Confirm_t const *confirm)
{
   // Allocate the buffer
   buffer_t *pBuf = BM_allocStack(sizeof(NWK_Confirm_t));   // Allocate the buffer
   if(pBuf != NULL )
   {
      // Set the type
      pBuf->eSysFmt = eSYSFMT_NWK_CONFIRM;

      NWK_Confirm_t *pConf = (NWK_Confirm_t *)pBuf->data; /*lint !e740 !e826 */
      (void) memcpy(pConf, confirm, sizeof(NWK_Confirm_t) );
      OS_MSGQ_Post(&SM_internalMsgQueue, (void *)pBuf); // Function will not return if it fails
   }
}

/**
Process a Refresh Request message

@param pReq contains the request parameters

@return true if processed
*/
static bool Process_RefreshReq(SM_Request_t const *pReq)
{
   (void)pReq;

   INFO_printf("RefreshReq");

#if (EP == 1)
   OS_EVNT_Set(&nwActivityEvent, NW_EVENT_REFRESH);

   SM_Confirm_t *pConfirm = &ConfirmMsg;
   pConfirm->MsgType = eSM_REFRESH_CONF;

   // send time request to force network traffic
   if (MAC_TimeQueryReq())
   {
      pConfirm->RefreshConf.eStatus = eSM_REFRESH_SUCCESS;
   }
   else
   {
      pConfirm->RefreshConf.eStatus = eSM_REFRESH_ERROR;
   }

   return true;
#else
   return false;
#endif
}

/*!
 ********************************************************************************************************************
 *
 * \fn      Process_SetReq()
 *
 * \brief   Process a Set Request message
 *
 * \return  true  - success
 *
 * \details
 *
 *    eSM_SET_SUCCESS
 *    eSM_SET_READONLY
 *    eSM_SET_UNSUPPORTED
 *    eSM_SET_INVALID_PARAMETER
 *
 ********************************************************************************************************************
 */
static bool Process_SetReq( SM_Request_t const *pReq )
{
   SM_Confirm_t *pConfirm = &ConfirmMsg;

   pConfirm->MsgType         = eSM_SET_CONF;
   pConfirm->SetConf.eStatus = SM_Attribute_Set(&pReq->SetReq);
   return true;
}

/*!
 ********************************************************************************************************************
 *
 * \fn      Process_GetReq()
 *
 * \brief   Process a Get Request message
 *
 * \return  true  - success
 *
 * \details
 *
 *    eSM_GET_SUCCESS
 *    eSM_GET_UNSUPPORTED
 *
 ********************************************************************************************************************
 */
static bool  Process_GetReq( SM_Request_t const *pReq )
{
   SM_Confirm_t *pConfirm = &ConfirmMsg;

   pConfirm->MsgType            = eSM_GET_CONF;
   pConfirm->GetConf.eStatus    = SM_Attribute_Get( &pReq->GetReq, &pConfirm->GetConf.val);
   pConfirm->GetConf.eAttribute = pReq->GetReq.eAttribute;
   return true;
}


/*!
 ********************************************************************************************************************
 *
 * \fn bool phy_StartStop()
 *
 * \brief   Used to to start/stop the PHY Layer
 *
 * \return  true  - success
 *          false - failue
 *
 * \details
 *
 ********************************************************************************************************************
 */
static bool phy_StartStop(PHY_STATE_e phyState)
{
   uint32_t tryCount = 0;
   buffer_t *pBuf;
   bool execStatus  = false; // Default to failure

   while (OS_MSGQ_Pend( &SM_internalMsgQueue, (void *)&pBuf, 0)) // Consume any pending messages that may have been revceived from provious layers but where never processed.
      ;

   do
   {
      bool status = false;
      // Attempt to start the phy in specified mode
      if (phyState == ePHY_STATE_SHUTDOWN)
      {
         status = PHY_StopRequest(phy_confirm_cb);
      }else
      {
         status = PHY_StartRequest((PHY_START_e) phyState, phy_confirm_cb);
      }
      if( status )
      {  // Successful start/stop request
         if (OS_MSGQ_Pend( &SM_internalMsgQueue, (void *)&pBuf, 60*ONE_SEC)) // Starting PHY can take a very long time when things go wrong. Booting the radios take about 5 seconds.
                                                                             // If the noise estimate fails for every channels, it can easily take a second per channel (up to 32 channels).
         {
            if(pBuf->eSysFmt == eSYSFMT_PHY_CONFIRM)
            {
               PHY_Confirm_t * pPhyConfirm = (PHY_Confirm_t *) pBuf->data;  /*lint !e740 !e826 */

               if (pPhyConfirm->MsgType      ==  ePHY_START_CONF)
               {
                  /* Start must return success, running or partial link running */
                  if(pPhyConfirm->StartConf.eStatus ==  ePHY_START_SUCCESS)
                  {
                     execStatus = true; // Success
                  }
               }
               else if (pPhyConfirm->MsgType ==  ePHY_STOP_CONF)
               {
                  // Accept ePHY_STOP_SERVICE_UNAVAILABLE for stop
                  if((pPhyConfirm->StopConf.eStatus ==  ePHY_STOP_SUCCESS) ||
                     (pPhyConfirm->StopConf.eStatus ==  ePHY_STOP_SERVICE_UNAVAILABLE))
                  {
                     execStatus = true; // Success
                  }
               }
            }else
            {
               INFO_printf("Unexpected type");
            }
            BM_free(pBuf);
         }else
         {
            // Timeout/No Response
         }
      }
   }while ( (++tryCount < ConfigAttr.ModeMaxTransitionAttempts) && (execStatus == false) );

   return execStatus;
}

/*!
 ********************************************************************************************************************
 *
 * \fn bool mac_StartStop()
 *
 * \brief   Used to start/stop the MAC Layer
 *
 * \param   true  - start
 * \param   false - stop
 *
 * \return  true  - success
 *          false - failue
 *
 * \details
 *
 *   If the SM successfully starts the PHY in the READY state, the SM will next attempt to start the MAC layer
 * by issuing a MAC.START.request service primitive.
 *
 *   If the MAC returns a MAC.START.comfirm with a status of anything other than SUCCESS or MAC_RUNNING, the SM
 * will wait an appropriate amount of time and retry the MAC.START.request.  This will repeat until either a
 * MAC.START.confirm returns with a status of SUCCESS or MAC_RUNNING, or the process fails smModeMaxTransitionAttempts
 * times.
 *
 *   If the SM fails to start the MAC, the SM will issue a START.confirm with a status of ERROR to the calling
 * application and abort the rest of this process.
 *
 ********************************************************************************************************************
 */
static bool mac_StartStop( bool state )
{
   uint32_t tryCount = 0;
   buffer_t *pBuf;
   bool execStatus  = false; // Default to failure

   while (OS_MSGQ_Pend( &SM_internalMsgQueue, (void *)&pBuf, 0)) // Consume any pending messages that may have been revceived from provious layers but where never processed.
      ;

   do
   {
      uint32_t handleId;
      if (state )
      { // start
         handleId = MAC_StartRequest(mac_confirm_cb);
      }else
      { // stop
         handleId = MAC_StopRequest(mac_confirm_cb);
      }

      if( handleId > 0 )
      {  // Successful start/stop request
         if (OS_MSGQ_Pend( &SM_internalMsgQueue, (void *)&pBuf, 5*ONE_SEC))
         {
            if(pBuf->eSysFmt == eSYSFMT_MAC_CONFIRM)
            {
               MAC_Confirm_t * pMacConfirm = (MAC_Confirm_t *) pBuf->data; /*lint !e740 !e826 */

               if (pMacConfirm->Type ==  eMAC_START_CONF)
               {
                  /* Start must return success, running or partial link running */
                  if((pMacConfirm->StartConf.eStatus ==  eMAC_START_SUCCESS) ||
                     (pMacConfirm->StartConf.eStatus ==  eMAC_START_RUNNING))
                  {
                     execStatus = true; // Success
                  }
               }
               else if (pMacConfirm->Type ==  eMAC_STOP_CONF)
               {
                  if(pMacConfirm->StopConf.eStatus ==  eMAC_STOP_SUCCESS)
                  {
                     execStatus = true; // Success
                  }
               }
            }else
            {
               INFO_printf("Unexpected type");
            }
            BM_free(pBuf);
         }else
         {
            // Timeout/No Response
         }
      }
   }
   while ( (++tryCount < ConfigAttr.ModeMaxTransitionAttempts) && (execStatus == false) );

   return execStatus;
}


/*!
 ********************************************************************************************************************
 *
 * \fn bool nwk_StartStop()
 *
 * \brief   Used to to start/stop the NWK Layer
 *
 * \param   state true  - start
 *                false - stop
 *
 * \return  true  - success
 *          false - failue
 *
 * \details
 *
 *    If the SM successfully starts the MAC, the SM will next attempt to start the NWK layer by
 * issuing a NWK.START.request service primitive.
 *
 *    If the NWK returns a NWK.START.comfirm with a status of anything other than SUCCESS, NETWORK_RUNNING, or
 * PARTIAL_LINK_RUNNING, the SM will wait an appropriate amount of time and retry the NWK.START.request.
 *
 *    This will repeat until either a NWK.START.confirm returns with a status of SUCCESS, NETWORK_RUNNING, or
 * PARTIAL_LINK_ERROR, or the process fails smModeMaxTransitionAttempts times.
 *
 *    If the SM fails to start the NWK, the SM will issue a START.confirm with a status of ERROR to the calling
 * application and abort the rest of this process.
 *
 ********************************************************************************************************************
 */
static bool nwk_StartStop( bool state )
{
   uint32_t tryCount = 0;
   bool execStatus  = false; // Default to failure
   buffer_t *pBuf;

   while (OS_MSGQ_Pend( &SM_internalMsgQueue, (void *)&pBuf, 0)) // Consume any pending messages that may have been revceived from provious layers but where never processed.
      ;

   do
   {
      uint32_t handleId;

      if (state )
      { // start
         handleId = NWK_StartRequest(nwk_confirm_cb);
      }else
      { // stop
         handleId = NWK_StopRequest(nwk_confirm_cb);
      }

      if(handleId > 0)
      {
         if (OS_MSGQ_Pend( &SM_internalMsgQueue, (void *)&pBuf, 5*ONE_SEC)) // Starting the radio can take some time
         {
            if(pBuf->eSysFmt == eSYSFMT_NWK_CONFIRM)
            {
               NWK_Confirm_t * pNwkConfirm = (NWK_Confirm_t *) pBuf->data; /*lint !e740 !e826 */

               if (pNwkConfirm->Type      ==  eNWK_START_CONF)
               {
                  /* Start must return success, running or partial link running */
                  if((pNwkConfirm->StartConf.eStatus ==  eNWK_START_SUCCESS) ||
                     (pNwkConfirm->StartConf.eStatus ==  eNWK_START_NETWORK_RUNNING) ||
                     (pNwkConfirm->StartConf.eStatus ==  eNWK_START_PARTIAL_LINK_RUNNING))
                  {
                     execStatus = true; // Success
                  }
               }
               else if (pNwkConfirm->Type ==  eNWK_STOP_CONF)
               {
                  if(pNwkConfirm->StopConf.eStatus ==  eNWK_STOP_SUCCESS)
                  {
                     execStatus = true; // Success
                  }
               }
            }else
            {
               INFO_printf("Unexpected type");
            }
            BM_free(pBuf);
         }else{
            // No Response timeout
         }
      }
   }
   while ( (++tryCount < ConfigAttr.ModeMaxTransitionAttempts) && (execStatus == false) );

   return execStatus;
}

/*!
 ********************************************************************************************************************
 *
 * \fn         SM_StartLayers
 *
 * \brief      Used to change the state of the PHY, MAC and NWK Layers
 *
 * \param      phyState  -
 * \param      macState  - TRUE to start, FALSE to stop
 * \param      nwkState  - TRUE to start, FALSE to stop
 *
 * \return     bool - true   Success
 *                  - false  Failure
 *
 * \details    First the PHY State is changed.   If that is successfull, then MAC is changed, and then the NWK.
 *
 ********************************************************************************************************************
 */
static bool SM_StartLayers( PHY_STATE_e phyState, bool macState, bool nwkState )
{
   bool status;

   // Process PHY
   status = phy_StartStop(phyState);

   if (status == true)
   {
      // Process MAC
      status = mac_StartStop(macState);
   }

   if (status == true)
   {
      // Process NWK
      status = nwk_StartStop(nwkState);
   }
   return status;
}


/*!
 ********************************************************************************************************************
 *
 * \fn      Process_StartReq
 *
 * \brief   Process a Start Request message
 *
 * \param   pReq ( SM_Request_t) - Must be a start request!
 *
 * \return  true    (Success)
 *
 * \details
 *
 *     The confirmation will indicate the result of the request.
 *
 *     eSM_START_SUCCESS
 *     eSM_START_FAILURE
 *     eSM_START_INVALID_PARAMETER
 *
 ********************************************************************************************************************
 */
static bool Process_StartReq( SM_Request_t const *pReq )
{
   SM_Confirm_t *pConfirm = &ConfirmMsg;

   pConfirm->MsgType = eSM_START_CONF;

   SM_MODE_e newMode;

   // Determine the mode from the requested state
   switch (pReq->StartReq.state)
   {
      case eSM_START_STANDARD      :  newMode = eSM_MODE_STANDARD;  break;
      case eSM_START_DEAF          :  newMode = eSM_MODE_DEAF;      break;
      case eSM_START_MUTE          :  newMode = eSM_MODE_MUTE;      break;
      case eSM_START_LOW_POWER     :  newMode = eSM_MODE_LOW_POWER; break;
      default                      :  newMode = eSM_MODE_UNKNOWN;   break;
   }

   // If the request state is invalid, return an error
   if (newMode == eSM_MODE_UNKNOWN)
   {
      // error
      pConfirm->StartConf.eStatus = eSM_START_INVALID_PARAMETER;
   }else
   {
      // If the new mode is different from current mode, set the new mode
      if (newMode != smMode_)
      {
         // Change the mode
         if( UpdateMode( newMode ) )
         {
            // Mode changed
            pConfirm->StartConf.eStatus = eSM_START_SUCCESS;
            smMode_ = newMode;
         }else
         {  // Error setting the mode
            pConfirm->StartConf.eStatus = eSM_START_FAILURE;
            smMode_ = eSM_MODE_UNKNOWN;
         }
      }else
      {  // Already in the correct mode
         pConfirm->StartConf.eStatus = eSM_START_SUCCESS;
      }
   }
   return true;
}

/*!
 ********************************************************************************************************************
 *
 * \fn    Process_StopReq
 *
 * \brief Process a Stop Request message
 *
 * \param   pReq ( SM_Request_t) - Must be a stop request!
 *
 * \return  true
 *
 * \details
 *
 *    returns:
 *       eSM_STOP_SUCCESS
 *       eSM_STOP_FAILURE
 *
 *
 ********************************************************************************************************************
 */
static bool Process_StopReq( SM_Request_t const *pReq )
{
   (void) pReq;  // Not used

   SM_Confirm_t *pConfirm = &ConfirmMsg;

   INFO_printf("StopReq");

   pConfirm->MsgType = eSM_STOP_CONF;

   if (smMode_ != eSM_MODE_OFF)
   {  // Change the mode to OFF
      if( UpdateMode(eSM_MODE_OFF) )
      {
         smMode_ = eSM_MODE_OFF;
         pConfirm->StopConf.eStatus = eSM_STOP_SUCCESS;
      }else
      {  // Error setting the mode
         pConfirm->StopConf.eStatus = eSM_STOP_FAILURE;
         smMode_ = eSM_MODE_UNKNOWN;
      }
   }else
   {  // Already OFF
      pConfirm->StopConf.eStatus = eSM_STOP_SUCCESS;
   }
   return true;
}




/*!
 ********************************************************************************************************************
 *
 * \fn    SM_Attribute_Set
 *
 * \brief This is called to handle a Set Request
 *
 * \param SM_SetReq_t - Pointer to the request parameters
 *
 * \return SM_SET_STATUS_e - eSM_SET_SUCCESS
 *                           eSM_SET_READONLY
 *                           eSM_SET_UNSUPPORTED
 *                           eSM_SET_INVALID_PARAMETER
 *                           eSM_SET_SERVICE_UNAVAILABLE
 *
 * \details
 *
 ********************************************************************************************************************
 */
static
SM_SET_STATUS_e  SM_Attribute_Set( SM_SetReq_t const *pSetReq)
{
   SM_SET_STATUS_e eStatus = eSM_SET_SERVICE_UNAVAILABLE;

   switch(pSetReq->eAttribute)
   {
      // Writeable Attributes
      case eSmAttr_LinkCount:
         eStatus = eSM_SET_READONLY;
         break;
      case eSmAttr_Mode:
         eStatus = eSM_SET_READONLY;
         break;
      case eSmAttr_ModeMaxTransitionAttempts:
         eStatus = Set_ModeMaxTransitionAttempts(&pSetReq->val);
         break;
      case eSmAttr_StatsCaptureTime:
         eStatus = Set_StatsCaptureTime(&pSetReq->val);
         break;
      case eSmAttr_StatsConfig:
         eStatus = Set_StatsConfig(&pSetReq->val);
         break;
      case eSmAttr_StatsEnable:
         eStatus = Set_StatsEnable(&pSetReq->val);
         break;
      case eSmAttr_QueueLevel:
         eStatus = eSM_SET_READONLY;
         break;
      case eSmAttr_NetworkActivityTimeoutPassive:
         eStatus = Set_PassiveActivityTimeout(&pSetReq->val);
         break;
      case eSmAttr_NetworkActivityTimeoutActive:
         eStatus = Set_ActiveActivityTimeout(&pSetReq->val);
         break;
      case eSmAttr_NetworkState:
         eStatus = eSM_SET_READONLY;
         break;
      case eSmAttr_NetworkStateTransitionTime:
         eStatus = eSM_SET_READONLY;
         break;
      default:
         eStatus   = eSM_SET_UNSUPPORTED_ATTRIBUTE;
         break;
   }

   if(eStatus != eSM_SET_SUCCESS)
   {
      INFO_printf("SM_Attribute_Set:Attr=%s Status=%s", sm_attr_names[pSetReq->eAttribute], SM_SetStatusMsg(eStatus ));
   }

   return eStatus;
}

/*!
 ********************************************************************************************************************
 *
 * \fn    SM_Attribute_Get
 *
 * \brief This is called to handle a Get Request
 *
 * \param SM_SetGet_t - Pointer to the request parameters
 *
 * \return SM_GET_STATUS_e - eSM_GET_SUCCESS
 *                         - eSM_GET_UNSUPPORTED_ATTRIBUTE
 *                         - eSM_GET_SERVICE_UNAVAILABLE
 *
 * \details
 *
 ********************************************************************************************************************
 */

static SM_GET_STATUS_e SM_Attribute_Get( SM_GetReq_t const *pGetReq, SM_ATTRIBUTES_u *val)
{
   SM_GET_STATUS_e eStatus = eSM_GET_SUCCESS;

   switch (pGetReq->eAttribute)
   {
      case eSmAttr_LinkCount:
         val->SmLinkCount = ConfigAttr.LinkCount;
         break;
      case eSmAttr_Mode:
         val->SmMode = smMode_;
         break;
      case eSmAttr_ModeMaxTransitionAttempts:
         val->SmModeMaxTransitionAttempts = ConfigAttr.ModeMaxTransitionAttempts;
         break;
      case eSmAttr_StatsCaptureTime:
         val->SmStatsCaptureTime = ConfigAttr.StatsCaptureTime;
         break;
      case eSmAttr_StatsConfig:
         val->SmStatsConfig[0] = 0;
         break;
      case eSmAttr_StatsEnable:
         val->SmStatsEnable = ConfigAttr.StatsEnable;
         break;
      case eSmAttr_QueueLevel:
         val->SmQueueLevel = ConfigAttr.QueueLevel;
         break;
      case eSmAttr_NetworkActivityTimeoutPassive:
         val->SmActivityTimeout = ConfigAttr.NetworkActivityTimeoutPassive;
         break;
      case eSmAttr_NetworkActivityTimeoutActive:
         val->SmActivityTimeout = ConfigAttr.NetworkActivityTimeoutActive;
         break;
      case eSmAttr_NetworkState:
         val->SmNetworkState = ConfigAttr.NetworkState;
         break;
      case eSmAttr_NetworkStateTransitionTime:
         val->SmNetworkStateTransitionTime = ConfigAttr.NetworkStateTransitionTime;
         break;
      default:
        eStatus = eSM_GET_UNSUPPORTED_ATTRIBUTE;
        break;
   }

   if(eStatus != eSM_GET_SUCCESS)
   {
      INFO_printf("SM_Attribute_Get:Attr=%s Status=%s", sm_attr_names[pGetReq->eAttribute], SM_GetStatusMsg(eStatus ));
   }

   return eStatus;
}

/*!
 ********************************************************************************************************************
 *
 * \fn      SM_ConfigPrint(void)
 *
 * \brief   This function is used to print the stack manager configuration to the terminal
 *
 * \param   void
 *
 * \return  void
 *
 ********************************************************************************************************************
 */
void SM_ConfigPrint(void)
{
   INFO_printf( "SM_CONFIG:%s %u", sm_attr_names[eSmAttr_LinkCount                    ], ConfigAttr.LinkCount                );
   INFO_printf( "SM_CONFIG:%s %u", sm_attr_names[eSmAttr_Mode                         ], smMode_                             );
   INFO_printf( "SM_CONFIG:%s %u", sm_attr_names[eSmAttr_ModeMaxTransitionAttempts    ], ConfigAttr.ModeMaxTransitionAttempts);
   INFO_printf( "SM_CONFIG:%s %u.%u", sm_attr_names[eSmAttr_StatsCaptureTime          ], ConfigAttr.StatsCaptureTime.seconds, ConfigAttr.StatsCaptureTime.QFrac );
   INFO_printf( "SM_CONFIG:%s %u", sm_attr_names[eSmAttr_StatsConfig                  ], ConfigAttr.StatsConfig              );
   INFO_printf( "SM_CONFIG:%s %u", sm_attr_names[eSmAttr_StatsEnable                  ], ConfigAttr.StatsEnable              );
   INFO_printf( "SM_CONFIG:%s %u", sm_attr_names[eSmAttr_QueueLevel                   ], ConfigAttr.QueueLevel               );
   INFO_printf( "SM_CONFIG:%s %u", sm_attr_names[eSmAttr_NetworkActivityTimeoutActive ], ConfigAttr.NetworkActivityTimeoutActive);
   INFO_printf( "SM_CONFIG:%s %u", sm_attr_names[eSmAttr_NetworkActivityTimeoutPassive], ConfigAttr.NetworkActivityTimeoutPassive);
   INFO_printf( "SM_CONFIG:%s %u.%u", sm_attr_names[eSmAttr_NetworkStateTransitionTime], ConfigAttr.NetworkStateTransitionTime.seconds, ConfigAttr.NetworkStateTransitionTime.QFrac);
   INFO_printf( "SM_CONFIG:%s %u", sm_attr_names[eSmAttr_NetworkState                 ], ConfigAttr.NetworkState);
}

/*!
 ********************************************************************************************************************
 *
 * \fn      SM_Stats()
 *
 * \brief   This function is used to print the SM stats to the terminal
 *
 * \param   option - true  : reset
 *                   false : reset
 * \return  void
 *
 ********************************************************************************************************************
 */
void SM_Stats(bool option)
{
   if(option == true)
   {
      // Reset the stats
      (void) memset(&CachedAttr, 0, sizeof(CachedAttr));
//    (void)FIO_fwrite( &PHYcachedFileHndl_, 0, (uint8_t*)&CachedAttr, sizeof(CachedAttr));
   }
   INFO_printf( "SM_STATS:?");
#if 0
   // Always print after
   {
      sysTime_t            sysTime;
      sysTime_dateFormat_t sysDateFormat;
      TIME_UTIL_ConvertSecondsToSysFormat(CachedAttr.LastResetTime.time, 0, &sysTime);
      (void) TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &sysDateFormat);
      INFO_printf("PHY_STATS:%s=%02d/%02d/%04d %02d:%02d:%02d",phy_attr_names[ePhyAttr_LastResetTime],
                    sysDateFormat.month, sysDateFormat.day, sysDateFormat.year, sysDateFormat.hour, sysDateFormat.min, sysDateFormat.sec); /*lint !e123 */
   }
   INFO_printf( "PHY_STATS:%s %li", phy_attr_names[ePhyAttr_AfcAdjustment           ],CachedAttr.AfcAdjustment);
#endif
}


/*!
 ********************************************************************************************************************
 *
 * \fn      SM_GetStatusMsg(SM_GET_STATUS_e GETstatus)
 *
 * \brief   Returns the Message for GET Status
 *
 * \param   SM_GET_STATUS_e
 *
 * \return  char pointer to message
 *
 ********************************************************************************************************************
 */
static
char* SM_GetStatusMsg(SM_GET_STATUS_e GETstatus)
{
   char* msg="";
   switch(GETstatus)
   {
   case eSM_GET_SUCCESS:               msg = "SM_GET_SUCCESS";               break;
   case eSM_GET_UNSUPPORTED_ATTRIBUTE: msg = "SM_GET_UNSUPPORTED_ATTRIBUTE"; break;
   case eSM_GET_SERVICE_UNAVAILABLE:   msg = "SM_GET_SERVICE_UNAVAILABLE";   break;
   }
   return msg;
}

/*!
 ********************************************************************************************************************
 *
 * \fn      SM_SetStatusMsg
 *
 * \brief   Returns the Message for SET Status
 *
 * \param   SM_SET_STATUS_e
 *
 * \return  char pointer to message
 *
 ********************************************************************************************************************
 */
static
char* SM_SetStatusMsg(SM_SET_STATUS_e SETstatus)
{
   char* msg="";
   switch(SETstatus)
   {
   case eSM_SET_SUCCESS:               msg = "SM_SET_SUCCESS";              break;
   case eSM_SET_READONLY:              msg = "SM_SET_READONLY";             break;
   case eSM_SET_UNSUPPORTED_ATTRIBUTE: msg = "SM_SET_UNSUPPORTED";          break;
   case eSM_SET_INVALID_PARAMETER:     msg = "SM_SET_INVALID_PARAMETER";    break;
   case eSM_SET_SERVICE_UNAVAILABLE:   msg = "SM_SET_SERVICE_UNAVAILABLE";  break;
   }
   return msg;
}

/*!
 ********************************************************************************************************************
 *
 * \fn      SM_StartStatusMsg
 *
 * \brief   Returns the Message for START Status
 *
 * \param   SM_START_STATUS_e
 *
 * \return  char pointer to message
 *
 ********************************************************************************************************************
 */
static
char* SM_StartStatusMsg(SM_START_STATUS_e STARTstatus)
{
   char* msg="";
   switch(STARTstatus)
   {
   case eSM_START_SUCCESS:            msg = "SM_START_SUCCESS";            break;
   case eSM_START_FAILURE:            msg = "SM_START_FAILURE";            break;
   case eSM_START_INVALID_PARAMETER:  msg = "SM_START_INVALID_PARAMETER";  break;
   }
   return msg;
}


/*!
 ********************************************************************************************************************
 *
 * \fn      SM_StopStatusMsg
 *
 * \brief   Returns the Message for STOP Status
 *
 * \param   SM_STOP_STATUS_e
 *
 * \return  char pointer to message
 *
 ********************************************************************************************************************
 */

static
char* SM_StopStatusMsg(SM_STOP_STATUS_e STOPstatus)
{
   char* msg="";
   switch(STOPstatus)
   {
   case eSM_STOP_SUCCESS:             msg = "SM_STOP_SUCCESS";             break;
   case eSM_STOP_FAILURE:             msg = "SM_STOP_FAILURE";             break;
   case eSM_STOP_SERVICE_UNAVAILABLE: msg = "SM_STOP_SERVICE_UNAVAILABLE"; break;
   }
   return msg;
}

/**
Returns the Message for Refresh Status

@param status The result of the refresh attempt

@return char pointer to message
*/
static char* SM_RefreshStatusMsg(SM_REFRESH_STATUS_e status)
{
   char* msg = "";

   switch (status)
   {
      case eSM_REFRESH_SUCCESS:
         msg = "SM_REFRESH_SUCCESS";
         break;
      case eSM_REFRESH_ERROR:
         msg = "SM_REFRESH_ERROR";
         break;
   }

   return msg;
}


/*!
 ********************************************************************************************************************
 *
 * \fn      RequestBuffer_Get
 *
 * \brief   Get a SM Request Buffer
 *
 * \param   size - Size of the buffer
 *
 * \return  pBuf - NULL : failure
 *
 * \details This will allocate a buffer.
 *
 ********************************************************************************************************************
 */
static buffer_t *RequestBuffer_Get(uint16_t size)
{
   buffer_t *pBuf = BM_allocStack(size);   // Allocate the buffer
   if(pBuf)
   {
      pBuf->eSysFmt = eSYSFMT_SM_REQUEST;
   }
   return pBuf;
}

/**
Send a request to refresh the current network status.

@retutn Pointer to request buffer or NULL on error.
*/
uint32_t SM_RefreshRequest(void)
{
   // Allocate the buffer
   buffer_t *pBuf = RequestBuffer_Get(sizeof(SM_Request_t));
   if (pBuf != NULL)
   {
      SM_Request_t *pMsg = (SM_Request_t *)pBuf->data; /*lint !e740 !e826 */
      pMsg->MsgType = eSM_REFRESH_REQ;
      pMsg->pConfirmHandler = NULL;
      SM_Request(pBuf);
   }

   return (uint32_t)pBuf;
}

/*!
 ********************************************************************************************************************
 *
 * \fn       SM_StartRequest
 *
 * \brief   Send a Start Request to the SM
 *
 * \param   SM_START_e        - Desired state
 * \param   SM_ConfirmHandler - Callback
 *
 * \return  >0 - Success
 *           0 - Failure
 *
 * \details
 *
 ********************************************************************************************************************
 */

uint32_t SM_StartRequest(SM_START_e state, SM_ConfirmHandler cb_confirm)
{
   // Allocate the buffer
   buffer_t *pBuf = RequestBuffer_Get(sizeof(SM_Request_t));
   if (pBuf != NULL)
   {
      SM_Request_t *pMsg = (SM_Request_t *)pBuf->data; /*lint !e740 !e826 */
      pMsg->MsgType         = eSM_START_REQ;
      pMsg->pConfirmHandler = cb_confirm;
      pMsg->StartReq.state  = state;
      SM_Request(pBuf);
   }
   return (uint32_t)pBuf;
}

/*!
 ********************************************************************************************************************
 *
 * \fn       SM_StopRequest
 *
 * \brief   Send a Stop Request to the SM
 *
 * \param   SM_ConfirmHandler - Callback
 *
 * \return  >0 - Success
 *           0 - Failure
 *
 * \details
 *
 ********************************************************************************************************************
 */
uint32_t SM_StopRequest(SM_ConfirmHandler cb_confirm)
{
   // Allocate the buffer
   buffer_t *pBuf = RequestBuffer_Get(sizeof(SM_Request_t));
   if (pBuf != NULL)
   {
      SM_Request_t *pMsg = (SM_Request_t *)pBuf->data; /*lint !e740 !e826 */
      pMsg->MsgType         = eSM_STOP_REQ;
      pMsg->pConfirmHandler = cb_confirm;
      SM_Request(pBuf);
   }
   return (uint32_t)pBuf;
}

/*!
 *  Confirmation handler for Get/Set Request
 */
static void SM_GetSetRequest_CB(SM_Confirm_t const *confirm)
{
   // Notify calling function that SM has got/set attribute
   OS_SEM_Post ( &SM_AttributeSem_ );
} /*lint !e715 */

/*!
 ********************************************************************************************************************
 *
 * \fn      SM_GetRequest
 *
 * \brief   Send a Get Request to the SM
 *
 * \param   SM_ATTRIBUTES_e   - Attribute
 *
 * \return  confirmation structure
 *
 * \details
 *
 ********************************************************************************************************************
 */
SM_GetConf_t SM_GetRequest(SM_ATTRIBUTES_e eAttribute)
{
   SM_GetConf_t confirm;

   (void)memset(&confirm, 0, sizeof(confirm));

   confirm.eStatus = eSM_GET_SERVICE_UNAVAILABLE;

   OS_MUTEX_Lock( &SM_AttributeMutex_ ); // Function will not return if it fails
   buffer_t *pBuf = RequestBuffer_Get(sizeof(SM_Request_t));
   if (pBuf != NULL)
   {
      SM_Request_t *pReq = (SM_Request_t*)pBuf->data; /*lint !e740 !e826 payload holds the SM_DataReq_t information */
      pReq->MsgType           = eSM_GET_REQ;
      pReq->pConfirmHandler   = SM_GetSetRequest_CB;
      pReq->GetReq.eAttribute = eAttribute;
      SM_Request(pBuf);
      // Wait for SM to retrieve attribute
      if (OS_SEM_Pend ( &SM_AttributeSem_, 10*ONE_SEC )) {
         confirm = ConfirmMsg.GetConf; // Save data
      }
   }
   OS_MUTEX_Unlock( &SM_AttributeMutex_ ); // Function will not return if it fails

   if (confirm.eStatus != eSM_GET_SUCCESS) { /*lint !e456 Two execution paths are being combined with different mutex lock states */
      INFO_printf("ERROR: SM_GetRequest: status %s, attribute %s", SM_GetStatusMsg(confirm.eStatus), sm_attr_names[eAttribute]);
   }

   return confirm; /*lint !e454 A thread mutex has been locked but not unlocked */
}

/*!
 ********************************************************************************************************************
 *
 * \fn      SM_SetRequest
 *
 * \brief   Send a Set Request to the SM
 *
 * \param      SM_ATTRIBUTES_e   - Attribute
 * \param      SM_ATTRIBUTES_u   - Value
 *
 *
 * \return  confirmation structure
 *
 * \details
 *
 ********************************************************************************************************************
 */
SM_SetConf_t SM_SetRequest( SM_ATTRIBUTES_e eAttribute, void const *val)
{
   SM_SetConf_t confirm;

   (void)memset(&confirm, 0, sizeof(confirm));

   confirm.eStatus = eSM_SET_SERVICE_UNAVAILABLE;

   OS_MUTEX_Lock( &SM_AttributeMutex_ ); // Function will not return if it fails
   buffer_t *pBuf = RequestBuffer_Get(sizeof(SM_Request_t));
   if (pBuf != NULL)
   {
      SM_Request_t *pReq = (SM_Request_t*)pBuf->data; /*lint !e740 !e826 payload holds the SM_DataReq_t information */
      pReq->MsgType           = eSM_SET_REQ;
      pReq->pConfirmHandler   = SM_GetSetRequest_CB;
      pReq->SetReq.eAttribute = eAttribute;
      (void)memcpy(&pReq->SetReq.val, val, sizeof(SM_ATTRIBUTES_u));
      SM_Request(pBuf);
      // Wait for SM to retrieve attribute
      if (OS_SEM_Pend ( &SM_AttributeSem_, 10*ONE_SEC )) {
         confirm = ConfirmMsg.SetConf; // Save data
      }
   }
   OS_MUTEX_Unlock( &SM_AttributeMutex_ ); // Function will not return if it fails

   if (confirm.eStatus != eSM_SET_SUCCESS) { /*lint !e456 Two execution paths are being combined with different mutex lock states */
      INFO_printf("ERROR: SM_SetRequest: status %s, attribute %s", SM_SetStatusMsg(confirm.eStatus), sm_attr_names[eAttribute]);
   }

   return confirm; /*lint !e454 A thread mutex has been locked but not unlocked */
}

/*!
 ********************************************************************************************************************
 *
 * \fn      UpdateMode(SM_MODE_e mode)
 *
 * \brief   This function is used to update the operating mode of the Stack Manager (SM)
 *
 * \param   eMode (SM_MODE_e)
 *
 * \return  true    (Success)
 * \return  false   (Failure)
 *
 * \details
 *
 ********************************************************************************************************************
 */
static bool UpdateMode(SM_MODE_e eMode)
{
   bool status = false;
   switch (eMode)
   {
   case eSM_MODE_STANDARD   : status = SM_StartLayers( ePHY_STATE_READY,    (bool)OPERATIONAL, (bool)OPERATIONAL ); break;
   case eSM_MODE_DEAF       : status = SM_StartLayers( ePHY_STATE_READY_TX, (bool)OPERATIONAL, (bool)OPERATIONAL ); break;
   case eSM_MODE_MUTE       : status = SM_StartLayers( ePHY_STATE_READY_RX, (bool)OPERATIONAL, (bool)OPERATIONAL ); break;
   case eSM_MODE_LOW_POWER  : status = SM_StartLayers( ePHY_STATE_STANDBY,  (bool)OPERATIONAL, (bool)OPERATIONAL );
#if ( DCU == 1 )
      /* This is a good time to flush all statistics on the T-board. */
      /* This is because we are close to losing power and there is no signal to tell us when we will unlike the EPs who know when power is lost. */
      /* Flush all partitions (only cached actually flushed) and ensures all pending writes are completed */
      ( void )PAR_partitionFptr.parFlush( NULL );
#endif
      break;
   case eSM_MODE_BACKLOGGED :                                                                                       break;
   case eSM_MODE_TEST       :                                                                                       break;
   case eSM_MODE_BER        :                                                                                       break;
   case eSM_MODE_OFF        : status = SM_StartLayers( ePHY_STATE_SHUTDOWN, (bool)IDLE,        (bool)IDLE        ); break;
   case eSM_MODE_UNKNOWN    : // fall thru
   default                  :
      break;
   }
   return status;
}

/*!
 ********************************************************************************************************************
 *
 * \fn      Set_StartWaitTime
 *
 * \brief   This function use used by SM_Attribute_Set to changed the specified attribute
 *
 * \param   SM_ATTRIBUTES_u pVal
 *
 * \return  SM_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static SM_SET_STATUS_e Set_ModeMaxTransitionAttempts( SM_ATTRIBUTES_u const *pVal)
{
   SM_SET_STATUS_e eStatus = eSM_SET_INVALID_PARAMETER;

   if (pVal->SmModeMaxTransitionAttempts > 0 )
   {
      ConfigAttr.ModeMaxTransitionAttempts = pVal->SmModeMaxTransitionAttempts;
      eStatus = eSM_SET_SUCCESS;
   }

   return eStatus;
}

/*!
 ********************************************************************************************************************
 *
 * \fn      Set_StatsCaptureTime
 *
 * \brief   This function use used by SM_Attribute_Set to changed the specified attribute
 *
 * \param   SM_ATTRIBUTES_u pVal
 *
 * \return  SM_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static SM_SET_STATUS_e Set_StatsCaptureTime( SM_ATTRIBUTES_u const *pVal )
{

   SM_SET_STATUS_e eStatus;

// todo: 6/8/2016 5:30:52 PM [BAH]
// if (pVal->SmStatsCaptureTime < ???
   {
      ConfigAttr.StatsCaptureTime = pVal->SmStatsCaptureTime;
      eStatus = eSM_SET_SUCCESS;
   }

   return eStatus;
}

/*!
 ********************************************************************************************************************
 *
 * \fn       Set_StatsConfig
 *
 * \brief   This function use used by SM_Attribute_Set to changed the specified attribute
 *
 * \param   SM_ATTRIBUTES_u pVal
 *
 * \return  SM_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static SM_SET_STATUS_e Set_StatsConfig( SM_ATTRIBUTES_u const *pVal )
{
   (void) pVal;

// todo: 6/8/2016 5:30:52 PM [BAH]
   SM_SET_STATUS_e eStatus = eSM_SET_INVALID_PARAMETER;

   return eStatus;
}

/*!
 ********************************************************************************************************************
 *
 * \fn      Set_StatsEnable
 *
 * \brief   This function use used by SM_Attribute_Set to changed the specified attribute
 *
 * \param   SM_ATTRIBUTES_u pVal
 *
 * \return  SM_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static SM_SET_STATUS_e Set_StatsEnable( SM_ATTRIBUTES_u const *pVal )
{
   SM_SET_STATUS_e eStatus = eSM_SET_SUCCESS;

   ConfigAttr.StatsEnable = pVal->SmStatsEnable;

   return eStatus;
}

/**
Sets the passive network activity timeout

@param pVal The new value

@return eSM_SET_SUCCESS
*/
static SM_SET_STATUS_e Set_PassiveActivityTimeout(SM_ATTRIBUTES_u const *pVal)
{
   if ((pVal->SmActivityTimeout < 1) || (pVal->SmActivityTimeout > MAX_ACTIVE_TO))
   {
      return eSM_SET_INVALID_PARAMETER;
   }

   ConfigAttr.NetworkActivityTimeoutPassive = pVal->SmActivityTimeout;

   // reset timeout tracking
   OS_EVNT_Set(&nwActivityEvent, NW_EVENT_NEW_TIME);

   return eSM_SET_SUCCESS;
}

/**
Sets the active network activity timeout

@param pVal The new value

@return eSM_SET_SUCCESS
*/
static SM_SET_STATUS_e Set_ActiveActivityTimeout(SM_ATTRIBUTES_u const *pVal)
{
   if ((pVal->SmActivityTimeout < 1) || (pVal->SmActivityTimeout > MAX_PASS_TO))
   {
      return eSM_SET_INVALID_PARAMETER;
   }

   ConfigAttr.NetworkActivityTimeoutActive = (uint8_t)pVal->SmActivityTimeout;

   // reset timeout tracking
   OS_EVNT_Set(&nwActivityEvent, NW_EVENT_NEW_TIME);

   return eSM_SET_SUCCESS;
}

/**
Handles COMM-STATUS indications from the MAC layer.
*/
static void CommStatusHandler(buffer_t *indication)
{
   if (indication->eSysFmt == eSYSFMT_MAC_INDICATION)
   {
      MAC_Indication_t *msg = (MAC_Indication_t*)indication->data; /*lint !e826 */

      if ((msg->Type == eMAC_COMM_STATUS_IND) && (msg->CommStatus.network_id == NetworkId_Get()))  // This is actually called by the MAC Task since this function is registered at init
      {
         OS_EVNT_Set(&nwActivityEvent, NW_EVENT_ACTIVITY);
      }
   }

   // clean up the indication memory now that it was serviced
   BM_free(indication);
}

/*!
 ********************************************************************************************************************
 *
 * \fn      SM_WaitForStackInitialized
 *
 * \brief   This function wait for the SM mode to be initialized
 *
 * \param
 *
 * \return
 *
 ********************************************************************************************************************
 */
void SM_WaitForStackInitialized(void)
{
   SM_GetConf_t GetSMConf;

   do
   {
      OS_TASK_Sleep ( 1 );
      GetSMConf = SM_GetRequest( eSmAttr_Mode );
   } while( !( (GetSMConf.eStatus == eSM_GET_SUCCESS) && (GetSMConf.val.SmMode != eSM_MODE_UNKNOWN) ) );
}
