/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: event_log.h
 *
 * Contents: Contains all the information for event logging.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Charlie Paul
 *
 * $Log$ Created July 22, 2015
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - 07/22/2015 - Initial Release
 *
 * @version    0.1
 * #since      2015-07-22
 **********************************************************************************************************************/
#ifndef EVL_EVENT_LOG_H_
#define EVL_EVENT_LOG_H_

#define TIMESTAMP_PROVIDED       ((bool)true)
#define TIMESTAMP_NOT_PROVIDED   ((bool)false)

/* Global Definitions */
#include "HEEP_UTIL.h"
#if ( LAST_GASP_SIMULATION == 1 )
#include "pwr_config.h"
#endif

/* Set this true when wanting the command line functions to be enabled */
#define EVL_TESTING (1)

/*set this to true to debug EVL*/
#define EVL_DEBUG (1)

/* set this to true to enable event log unit testing features */
#define EVL_UNIT_TESTING (0)

#define EVL_ERROR              (-1)   /* Event log error value                                                */
#define EVL_SUCCESS            (0)    /* Event log successful value                                           */
#define EVL_SIZE_LIMIT_REACHED (1)    /* Unable to add more event to provided buffer due to buffer size limit */

#define HIGH_PRIORITY_SEVERITY 3      /*Real Time Priority*/
#define MEDIUM_PRIORITY_SEVERITY 2    /*opportunistic priority*/


typedef enum
{
   QUEUED_e = 0,         /* Event was queued. */
   DROPPED_e,            /* Event was dropped because events with higher priority have consumed available log space. */
   SENT_e,               /* Event was sent. */
   ERROR_e               /* Error occurred. */
} EventAction_e;

typedef enum
{
   NO_QUERY_e,           /* No QUERY take all entries. */
   QUERY_BY_DATE_e,      /* Query by date. */
   QUERY_BY_INDEX_e      /* Query by alarm index ID. */
} EventQueryRequest_e;

#define EVENT_KEY_MAX_LENGTH            (2)              /* Key Pair key length */
#define EVENT_VALUE_MAX_LENGTH          (16)             /* Key Pair value length */
#define NAME_VALUE_PAIR_MAX             ((uint8_t)15)    /* Max number of NVP pairs for each event */
#define PRIORITY_EVENT_BUFFER_LENGTH    PART_NV_HIGH_PRTY_ALRM_SIZE
#define NORMAL_EVENT_BUFFER_LENGTH      PART_NV_LOW_PRTY_ALRM_SIZE
#define EVL_MAX_NUMBER_EVENTS_SENT      (15)             /* Max number of events sent */
#define DEFAULT_OPPORTUNISTIC_THRESHOLD (85)             /* Opportunistic Threshold */
#define DEFAULT_REALTIME_THRESHOLD      (170)            /* Realtime (high) Threshold */

/**
 * Event key value structure.
 */
typedef struct
{
   uint8_t Key[EVENT_KEY_MAX_LENGTH];      /* Key pair key */
   uint8_t Value[EVENT_VALUE_MAX_LENGTH];  /* Key pair value */
} EventKeyValuePair_s;

/******************************************************************************
 * TimeStamp_s - Time stamp structure
 *****************************************************************************/
typedef struct
{
   uint32_t seconds;      /* Senconds since Jan 1 1970 */
   uint8_t  milliseconds; /* miliseconds */
} TimeStamp_s;

/******************************************************************************
 * Event log callback.
 * @eventReference value supplied to LogEvent.
 * @eventAction action taken for the event.
 *****************************************************************************/
typedef void ( *EventLogCallback )( uint16_t eventReference, EventAction_e eventAction );

/******************************************************************************
 * EventQuery_s - this structure is used to query the event log with a range
 *                of event ids or a range of timestamps.
 *****************************************************************************/
typedef struct
{
   EventQueryRequest_e qType;       /* The query by date or time. */
   union
   {
      uint32_t timestamp;           /* Low timestamp for range. */
      uint16_t eventId;             /* Low event id for range. */
   } start_u;
   union
   {
      uint32_t timestamp;           /* High timestamp for range. */
      uint16_t eventId;             /* High event id for range. */
   } end_u;
   uint8_t alarmSeverity;           /* Severity of alarm query, real time or opportunistic */
} EventQuery_s;

/******************************************************************************
 * EventStoredData_s - this structure is a footprint of the data that is
 *                     stored in the eventlog.
 *****************************************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t size;                    /* Size of the event block. */
   EventLogCallback callback;        /* The callback function. */
   uint8_t priority;                 /* Priority of the event. */
   uint8_t alarmId;                  /* Increments with each alarm. */
   uint16_t eventId;                 /* Event ID. */
   TimeStamp_s timestamp;            /* Time stamp for the event. */
   struct
   {
      uint16_t nPairs      : 5;        /* Number of Value Pairs. */
      uint16_t valueSize   : 5;        /* Value size. */
      uint16_t sent        : 1;        /* Data is sent. */
   } EventKeyHead_s;
} EventStoredData_s;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
  uint32_t          offset;
  EventStoredData_s StoredData;
  //specify which Buffer?
} EventMsgData_s;
PACK_END

/******************************************************************************
 * EventHeadEndData_s - this structure is a footprint of the data that is
 *                     sent back to the headend.
 *****************************************************************************/
PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t eventId;                   /* Event ID. */
   uint32_t timestamp;                 /* Timestamp in seconds */
   struct
   {
      uint8_t nPairs      : 4;         /* Number of Value Pairs. */
      uint8_t valueSize   : 4;         /* Value size. */
   } EventKeyHead_s;
} EventHeadEndData_s;
PACK_END

/******************************************************************************
 * EventData_s - this structure is passed into the event log manager with the
 *               the parameters for the event.
 *****************************************************************************/
typedef struct
{
   uint32_t timestamp;                 /* Timestamp in seconds */
   uint16_t eventId;                   /* Event ID */
   uint8_t  eventKeyValuePairsCount;   /* Pairs Count */
   uint8_t  eventKeyValueSize;         /* Value size */
   bool     markSent;                  /* prevent event from bubble up if true */
   uint8_t  alarmId;                   /* the event index into the alarm buffer */
} EventData_s;

/*****************************************************************************
 * EventMark_s - this structure represents the information passed back to the
 *              EVL when an event is sent to the headend successfully.
 *****************************************************************************/
typedef struct
{
   uint8_t  priority;      /* priority of the event */
   uint16_t eventId;       /* Event ID */
   uint32_t size;          /* Size of the event in the buffer */
   uint32_t offset;        /* The physical offset into the buffer */
} EventMark_s;

/******************************************************************************
 * EventMarkSent - this stucture represents the complete list of events that
 *                 need to be marked as sent.
 *****************************************************************************/
typedef struct
{
   uint16_t nEvents;                 /* The number of events to clear */
   EventMark_s eventIds[EVL_MAX_NUMBER_EVENTS_SENT]; /* List of event ids to clear */
} EventMarkSent_s;

/******************************************************************************
 * EventMarkSent - this stucture represents information about of an event that
 *                 just logged.  Provides information such as which buffer
 *                 the alarm was logged in along with its index ID.
 *****************************************************************************/
typedef struct
{
   meterReadingType alarmIndex;   // the buffer the event was stored in
   uint8_t          indexValue;   // the index id value of the event
} LoggedEventDetails;

/******************************************************************************
 * Saves an event to the event log and queues it for sending if possible.
 * @priority contains the desired priority queue.
 * @eventData contains event information to be logged.
 * @eventKeyValuePairs contains a pointer to the 'key' data
 * @timeStampProvided  True: Caller provided event timestamp, False: use current time
 *
 * @return The action taken with the event.
 *****************************************************************************/
EventAction_e EVL_LogEvent( uint8_t priority, const EventData_s *eventData,
                            const EventKeyValuePair_s *eventKeyValuePairs,
                            bool timeStampProvided, LoggedEventDetails *loggedData );

/******************************************************************************
 * Handles transmitting high priority alarms with time diversity.
 *****************************************************************************/
void EVL_AlarmHandlerTask ( uint32_t Arg0 );

/******************************************************************************
 * Initializes the Event Log data structers.
 * initialize is used to tell the evl to initialize from scratch or not.
 *
 * @return Success or Failure.
 ******************************************************************************/
returnStatus_t EVL_Initalize( void );

/******************************************************************************
 * Returns as many events that can fit into the provided buffer..
 * @size of the provided buffer.
 * @logData is where the data is returned.
 * @alarmId holds the alarmID returned.
 * @passback is a pointer to the passback data which must be sent to EVL_MarkSent
 *           funtion.
 *
 * @return The number of bytes put into the buffer.
 ******************************************************************************/
int32_t EVL_GetLog( uint32_t size, uint8_t *logData, uint8_t *alarmId,
                   EventMarkSent_s *passback );

/******************************************************************************
 * Marks the list of events as sent and calls the associated callback function.
 * @event contains the list of events that must be marked as sent.
 *
 * @return Nothing.
 *****************************************************************************/
void EVL_MarkSent( const EventMarkSent_s *events );

/******************************************************************************
 * Called to query by a range of dates or event ids.
 * @query what kind of events using the event ids or timestamp.
 * @results is a pointer to where the buffer will be written.
 * @sizeRequest contains the size of the results buffer.
 * @sizeReturned contains the actual number of valid data in the results buffer.
 * @alarmId contains the alarmID of the first event returned.
 *
 * @return Success or Failure.
 *****************************************************************************/
int32_t EVL_QueryBy( EventQuery_s query, uint8_t *results, uint32_t sizeRequest,
                    uint32_t *sizeReturned, uint8_t *alarmId );

/******************************************************************************
 * Set the event thresholds.
 * @rtThreshold contains the real time threshold.
 * @oThreshold contains the opportunistic threshold.
 *
 * @return Nothing.
 *****************************************************************************/
returnStatus_t EVL_SetThresholds( uint8_t rtThreshold, uint8_t oThreshold );

/******************************************************************************
 * Gets the event thresholds.
 * @rtThreshold contains the value of the stored real time threshold.
 * @oThreshold contains the value of the stored opportunistic threshold.
 *
 * @return Nothing.
 *****************************************************************************/
void EVL_GetThresholds( uint8_t *rtThreshold, uint8_t *oThreshold );
/******************************************************************************
 * Gets the high priority event current index.
 * @rtMost recently logged normal priority event index
 *
 * @return uint8_t index.
 *****************************************************************************/
uint8_t EVL_GetRealTimeIndex( void );
/******************************************************************************
 * Gets the normal event current index.
 * @rtMost recently logged normal priority event index
 *
 * @return uint8_t index.
 *****************************************************************************/
uint8_t        EVL_GetNormalIndex( void );
uint8_t        EVL_getAmBuMaxTimeDiversity( void );
returnStatus_t EVL_setAmBuMaxTimeDiversity( uint8_t uAmBuMaxTimeDiversity );
uint8_t        EVL_getRealTimeAlarm( void );
void           EVL_clearEventLog( void );
returnStatus_t EVL_postRealTimeAlarms(void);
returnStatus_t EVL_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
void           EVL_FirmwareError( char *function, char *file, int line );
void           OR_AM_MsgHandler( HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length );
void           OR_HA_MsgHandler( HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length );

#ifdef EVL_TESTING
void EventLogDumpBuffers( void );
void EventLogCallbackTest( uint16_t eventReference, EventAction_e eventAction );
void DumpEventData( uint8_t const * const buffer, uint32_t numBytes );
void DumpHeadEndData( uint8_t const * const buffer, uint32_t numBytes );
void EventLogEraseFlash( uint8_t whichBuffer );
void EventLogDeleteMetaData( void );
#endif
#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
returnStatus_t EVL_LGSimStart( uint32_t StartTime );
returnStatus_t EVL_LGSimActive( void );
returnStatus_t EVL_LGSimAllowMsg( HEEP_APPHDR_t const *heepMsgInfo, buffer_t const *payloadBuf );
void           EVL_LGSimTxDoneSignal( bool success );
void           EVL_LGSimCancel( void );
eSimLGState_t  EVL_GetLGSimState( void );
#endif

#if ( EVL_UNIT_TESTING == 1 )
void EVL_generateMakeRoomOpportunisticFailure( void );
void EVL_generateMakeRoomRealtimeFailure( void );
void EVL_loadLogBufferCorruption( uint8_t priorityThreshold, uint8_t alarmIndex );
#endif

#endif /* EVL_EVENT_LOG_H_ */
