/***********************************************************************************************************************
 *
 * Filename: SM_Protocol.h
 *
 * Contents: typedefs and #defines for the SM implementation
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2010-2016 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
#ifndef SM_Protocol_H
#define SM_Protocol_H

/* INCLUDE FILES */
#include "buffer.h"

/* MACRO DEFINITIONS */

/*!
 * Request Types to SM
 */
typedef enum
{
   eSM_GET_REQ,         /*!< SM GET_REQ   */
   eSM_SET_REQ,         /*!< SM SET_REQ   */
   eSM_START_REQ,       /*!< SM START_REQ */
   eSM_STOP_REQ,        /*!< SM STOP_REQ  */
   eSM_REFRESH_REQ,
} SM_REQ_TYPE_t;

/*!
 * Confirm Types returned from SM
 */
typedef enum
{
   eSM_GET_CONF,        /*!< SM GET_CONFIRM   */
   eSM_SET_CONF,        /*!< SM SET_CONFIRM   */
   eSM_START_CONF,      /*!< SM START_CONFIRM */
   eSM_STOP_CONF,       /*!< SM STOP_CONFIRM  */
   eSM_REFRESH_CONF,
} SM_CONF_TYPE_t;

/*!
 * SM Get/Set Attributes
 */
typedef enum
{
   eSmAttr_LinkCount,                /*! The number of link layers supported in the stack.
                                         This value is derived from the NWK layer. */
   eSmAttr_Mode,                     /*! Defines the mode in which the stack operates.  See Section 3.2 */
   eSmAttr_ModeMaxTransitionAttempts,/*! The maximum number of times the SM will attempt to transition a
                                         layer from one state to another when changing SM mode of operation.*/
   eSmAttr_StatsCaptureTime,         /*! The time each day when the stack statistics event is created.*/
   eSmAttr_StatsConfig,              /*! The list of statistics that are to be captured during each statistics event. */
   eSmAttr_StatsEnable,              /*! Enables stack statistics event creation.*/
   eSmAttr_QueueLevel,               /*! The number of outgoing transactions that are currently queued within the stack */
   eSmAttr_NetworkActivityTimeoutActive,  /*! The number of seconds to use for the active countdown timer in the network state tracking state machine */
   eSmAttr_NetworkActivityTimeoutPassive, /*! The number of seconds to use for the passive countdown timer in the network state tracking state machine */
   eSmAttr_NetworkStateTransitionTime, /*! Time of the last network state transition. */
   eSmAttr_NetworkState                /*! Network state connected tracking. */
} SM_ATTRIBUTES_e;

/*! SM Operational Modes */
typedef enum
{
   eSM_MODE_STANDARD,        /*!< Section 3.2.1   Normal Mode    */
   eSM_MODE_DEAF,            /*!< Section 3.2.3   Deaf           */ // DEAF and MUTE order are swaped to match maindboard +P command
   eSM_MODE_MUTE,            /*!< Section 3.2.2   Mute Mode      */
   eSM_MODE_LOW_POWER,       /*!< Section 3.2.4   Low Power Mode */
   eSM_MODE_BACKLOGGED,      /*!< Section 3.2.5   Backlogged     */
   eSM_MODE_TEST,            /*!< Section 3.2.6   Test           */
   eSM_MODE_BER,             /*!< Section 3.2.6.1 BER            */
   eSM_MODE_OFF,             /*!< Section 3.2.7   Off            */
   eSM_MODE_UNKNOWN          /*!< Section 3.2.7   Unknown        */
} SM_MODE_e;

/*!
 * SM Get/Set Parameters
 */
typedef union
{
   uint8_t     SmLinkCount;                 /*!< */
   SM_MODE_e   SmMode;                      /*!< */
   uint8_t     SmModeMaxTransitionAttempts; /*!< */
   TIMESTAMP_t SmStatsCaptureTime;          /*!< */
   uint8_t     SmStatsConfig[1];            /*!< ??? */
   bool        SmStatsEnable;               /*!< */
   uint8_t     SmQueueLevel;
   bool        SmNetworkState;
   TIMESTAMP_t SmNetworkStateTransitionTime;
   uint32_t    SmActivityTimeout;
} SM_ATTRIBUTES_u;

/*!
 * Start.Request - parameters
 */
typedef enum
{
    eSM_START_STANDARD,        // Set mode to eSM_MODE_STANDARD
    eSM_START_DEAF,            // Set mode to eSM_MODE_DEAF
    eSM_START_MUTE,            // Set mode to eSM_MODE_MUTE
    eSM_START_LOW_POWER,       // Set mode to eSM_MODE_LOW_POWER
} SM_START_e;

/*!
 * SM Get - Confirmation Status
 */
typedef enum
{
   eSM_GET_SUCCESS = 0,           /*!< SM GET - Success */
   eSM_GET_UNSUPPORTED_ATTRIBUTE, /*!< SM GET - Unsupported attribute */
   eSM_GET_SERVICE_UNAVAILABLE    /*!< SM GET - Service Unavailable */
} SM_GET_STATUS_e;

/*!
 * SM Set - Confirmation Status
 */
typedef enum
{
   eSM_SET_SUCCESS = 0,           /*!< SM SET - Success */
   eSM_SET_READONLY,              /*!< SM SET - Read only */
   eSM_SET_UNSUPPORTED_ATTRIBUTE, /*!< SM SET - Unsupported attribute */
   eSM_SET_INVALID_PARAMETER,     /*!< SM SET - Invalid paramter */
   eSM_SET_SERVICE_UNAVAILABLE    /*!< SM SET - Service Unavailable */
} SM_SET_STATUS_e;

/*!
 * SM Start - Confirmation Status
 */
typedef enum
{
   eSM_START_SUCCESS = 0,       /*!< SM START - Success */
   eSM_START_FAILURE,           /*!< SM START - Failure */
   eSM_START_INVALID_PARAMETER, /*!< SM START - Invalid Parameter */
} SM_START_STATUS_e;

/*!
 * SM Stop - Confirmation Status
 */
typedef enum
{
   eSM_STOP_SUCCESS = 0,        /*!< SM STOP - Success */
   eSM_STOP_FAILURE,            /*!< SM STOP - Failure */
   eSM_STOP_SERVICE_UNAVAILABLE /*!< SM STOP - Service Unavailable */
} SM_STOP_STATUS_e;

/*!
 * SM Refresh - Confirmation Status
 */
typedef enum
{
   eSM_REFRESH_SUCCESS = 0,        /*!< SM Refresh - Success */
   eSM_REFRESH_ERROR,            /*!< SM Refresh - Failure */
} SM_REFRESH_STATUS_e;

/*!
 * Get.Request
 */
typedef struct
{
   SM_ATTRIBUTES_e eAttribute;  /*!< Specifies the attribute requested */
} SM_GetReq_t;

/*!
 * Set.Request
 */
typedef struct
{
   SM_ATTRIBUTES_e  eAttribute; /*!< Specifies the attribute to set */
   SM_ATTRIBUTES_u  val;        /*!< Specifies the value to set     */
} SM_SetReq_t;

/*!
 * Start.Request
 */
typedef struct
{
   SM_START_e state;  /*!< READY, STANDBY */
} SM_StartReq_t;

/*!
 * Stop.Request
 */
typedef struct
{
   uint8_t dummy; // There are not paramters for this
} SM_StopReq_t;

/*!
 * REFRESH.Request
 */
typedef struct
{
   uint8_t dummy; // There are not paramters for this
} SM_RefreshReq_t;

/*! Get.Confirm */
typedef struct
{
   SM_ATTRIBUTES_e  eAttribute;  /*!< Specifies the attribute requested */
   SM_GET_STATUS_e  eStatus;     /*!< Execution status SUCCESS, UNSUPPORTED */
   SM_ATTRIBUTES_u  val;         /*!< Value Returned */
} SM_GetConf_t;

/*! Set.Confirm */
typedef struct
{
   SM_ATTRIBUTES_e eAttribute;     /*!< Specifies the attribute */
   SM_SET_STATUS_e eStatus;        /*!< SUCCESS, READONLY, UNSUPPORTED */
} SM_SetConf_t;

/*!
 * Start.Confirm
 */
typedef struct
{
   SM_START_STATUS_e eStatus;  /*!< SUCCESS, FAILURE */
} SM_StartConf_t;

/*!
 * Stop.Confirm
 */
typedef struct
{
   SM_STOP_STATUS_e eStatus;  /*!< SUCCESS, FAILURE */
} SM_StopConf_t;

/*!
 * Refresh.Confirm
 */
typedef struct
{
   SM_REFRESH_STATUS_e eStatus;  /*!< SUCCESS, FAILURE */
} SM_RefreshConf_t;

/*!
 * This specifies the confirmation interface from the SM.
 */
typedef struct
{
   SM_CONF_TYPE_t MsgType;
   union
   {
      // Management Services
      SM_GetConf_t      GetConf;    /*!< Get   Confirmation */
      SM_SetConf_t      SetConf;    /*!< Set   Confirmation */
      SM_StartConf_t    StartConf;  /*!< Start Confirmation */
      SM_StopConf_t     StopConf;   /*!< Stop  Confirmation */
      SM_RefreshConf_t  RefreshConf; /*!< Refresh  Confirmation */
   };
} SM_Confirm_t;

/**
Indications from SM
*/
typedef enum
{
   eSM_EVENT_IND
} SM_IND_TYPE_t;

typedef enum
{
   SM_EVENT_STATISTICS = 0,
   SM_EVENT_NW_STATE = 1,
} SM_Event_Type_t;

typedef struct
{
   bool networkConnected;
} SM_EVT_NwState_t;

/* EVENT.indication */
typedef struct
{
   uint8_t type;
   uint8_t priority;
   TIMESTAMP_t time;
   union
   {
      SM_EVT_NwState_t nwState;
   };
} SM_Event_t;

typedef struct
{
   SM_IND_TYPE_t      Type;
   union
   {
      SM_Event_t     EventInd;
   };
} SM_Indication_t;

/*!
 * This struct specifies the request interface to the SM.
 */
#ifdef OS_aclara_H
typedef void (*SM_ConfirmHandler)(SM_Confirm_t const *pConfirm);
#endif

typedef struct
{
   SM_REQ_TYPE_t     MsgType;
   SM_ConfirmHandler pConfirmHandler;
   union
   {
      // Management Services
      SM_GetReq_t     GetReq;      /*!< Get Request to SM */
      SM_SetReq_t     SetReq;      /*!< Set Request to SM */
      SM_StartReq_t   StartReq;    /*!< Request to Start the SM */
      SM_StopReq_t    StopReq;     /*!< Request to Stop  the SM */
      SM_RefreshReq_t RefreshReq;  /*!< Request to refresh network status */
   };
} SM_Request_t;

/* EXTERN FUNCTION PROTOTYPES */

#endif /* SM_Protocol_H */
