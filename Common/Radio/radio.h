/*!
 * @file radio.h
 * @brief This file is contains the public radio interface functions.
 */

#ifndef RADIO_H_
#define RADIO_H_

#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
 *  Global Macros & Definitions
 *****************************************************************************/

#if ( EP == 1 )
#define POWER_LEVEL_DEFAULT   7 // End-point default power level
#else
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
#define POWER_LEVEL_DEFAULT  60 // DCU radio chip default power level
#else
#define POWER_LEVEL_DEFAULT  20 // DCU radio chip default power level
#endif
#endif

#define CCA_SLEEP_TIME    130 // Used to fix a delay between RSSI reads when doing CCA.
                              // It takes 70 usec to read RSSI.  Adding 130 usec to this dealy brings the read time to 200 usec.
                              // 200 usec is about a symbol time.

#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
#define RADIO_TCXO_NOMINAL         29491200U   //  Nominal radio TCXO frequency
#define RADIO_TCXO_MIN             29491051U   //  -5 ppm from nominal
#define RADIO_TCXO_MAX             29491348U   //  +5 ppm from nominal
#else
#define RADIO_TCXO_NOMINAL         30000000U   //  Nominal radio TCXO frequency
#define RADIO_TCXO_MIN             29999250U   //  -25 ppm from nominal
#define RADIO_TCXO_MAX             30000750U   //  +25 ppm from nominal
#endif

#if ( DCU == 1 )
#define RADIO_TEMPERATURE_MIN -40 // Minimum valid temperature for PA power algorithm.
#define RADIO_TEMPERATURE_MAX  90 // Maximum valid temperature for PA power algorithm.
#define RADIO_PA_LEVEL_MAX     50 // Maximum valid radio PA value.
#endif

#if (VSWR_MEASUREMENT == 1)
#define VSWR_AVERAGE_COUNT      8 /* Number of samples that go into VSWR average */
#endif


#define RADIO_TEMP_SAMPLE 5


/*****************************************************************************
 *  Global Typedefs & Enums
 *****************************************************************************/

/*****************************************************************************
 *  Global Variable Declarations
 *****************************************************************************/
extern bool MFG_Port_Print;                        /*!< Used to print some PHY result to the MFG port */

/*****************************************************************************
 *  Global Function Declarations
 *****************************************************************************/
#if 0
extern float RADIO_Get_Current_Temperature( bool bSoft_demod );
extern bool RADIO_Is_Radio_Ready( void );
#endif
extern bool RADIO_Get_Chip_Temperature( uint8_t radioNum,int16_t *temp );
extern void RADIO_Temperature_Update( void );
/**
 * \addtogroup dev
 * @{
 */

/**
 * \defgroup radio Radio API
 *
 * The radio API module defines a set of functions that a radio device
 * driver must implement.
 *
 * @{
 */

/*! Radio Event Id's used by the callback function*/
typedef enum
{
   eRADIO_TX_DONE = 0,    /*!< Transmit Done */
   eRADIO_RX_DATA,        /*!< RX Framed Received */
   eRADIO_CTS_LINE_LOW,   /*!< CTS line low detected */
   eRADIO_INT,            /*!< Interrupts received */
} RADIO_EVENT_t;

/*!
 * RADIO Mode
 */
// WARNING: this must match pRadioConfiguration array order
typedef enum
{
   eRADIO_MODE_NORMAL = 0,     /*!< RADIO MODE - Normal */
   eRADIO_MODE_PN9,            /*!< RADIO MODE - Transmit PN9 test */
   eRADIO_MODE_CW,             /*!< RADIO MODE - Transmit unmodulated carrier wave */
   eRADIO_MODE_4GFSK_BER,      /*!< RADIO MODE - 4GFSK BER test. NOTE: Deprecated. Default to normal */
   eRADIO_MODE_NORMAL_BER,     /*!< RADIO MODE - Normal, BER test print result to mfg port */
#if ( PORTABLE_DCU == 1)
   eRADIO_MODE_NORMAL_DEV_600, /*!< RADIO MODE - Normal, Old PHY 600Hz dev and force RS(59,63) */
#endif
   eRADIO_LAST_MODE,           /*!< Keep last */
} RADIO_MODE_t;

/*! Radio Event Callback Function */
#if ( RTOS_SELECTION == MQX_RTOS )
typedef void (* RadioEvent_Fxn)(RADIO_EVENT_t event_type, uint8_t radio_num);
#elif ( RTOS_SELECTION == FREE_RTOS )
typedef void (* RadioEvent_Fxn)(RADIO_EVENT_t event_type, uint8_t radio_num, bool fromISR);
#endif

/**
 * The structure of a device driver for a radio in Aclara
 */
#ifdef PHY_Protocol_H
struct radio_driver {
  /** Initialize Radio hardware (i.e. patch, config, oscillator, PA) */
  bool (* Init)(RadioEvent_Fxn pCallback );

  bool (* StartRx)(uint8_t radioNum, uint16_t channel);

  /** Put radio to sleep */
  bool (* SleepRx)(uint8_t radioNum);

  /** Prepare & transmit a packet. */
  PHY_DATA_STATUS_e (* SendData)(uint8_t radioNum, uint16_t channel, PHY_MODE_e mode, PHY_DETECTION_e detection, const void *payload, uint32_t TxTime, PHY_TX_CONSTRAIN_e priority, float32_t power, uint16_t payload_len);

  /** Read a received packet into a buffer. */
  RX_FRAME_t * (* ReadData)(uint8_t radioNum);

  /** Do CCA on a specific channel. */
  PHY_CCA_STATUS_e (* Do_CCA)(uint16_t channel, uint8_t *rssi);

  /** Set radios to standby TX */
  void (* StandbyTx)(void);

  /** Set radios to standby RX */
  void (* StandbyRx)(void);

  /** Set radios to standby */
  void (* Standby)(void);

  /** Set radios to ready TX */
  void (* ReadyTx)(void);

  /** Set radios to ready RX */
  void (* ReadyRx)(void);

  /** Set radios to ready */
  void (* Ready)(void);

  /** Shutdown radios */
  void (* Shutdown)(void);
};
#endif

extern struct radio_driver Radio;
#ifdef PHY_Protocol_H
bool     RADIO_RxDetectionConfig(uint8_t radioNum, PHY_DETECTION_e detection, bool forceSet);
bool     RADIO_RxFramingConfig(uint8_t radioNum, PHY_FRAMING_e framing, bool forceSet);
bool     RADIO_RxModeConfig(uint8_t radioNum, PHY_MODE_e mode, bool forceSet);
uint16_t RADIO_RxChannelGet(uint8_t radioNum);
#endif
#if ( RTOS_SELECTION == MQX_RTOS )
void     RADIO_Event_Set(RADIO_EVENT_t event_type, uint8_t radioNum);
#elif ( RTOS_SELECTION == FREE_RTOS )
void     RADIO_Event_Set(RADIO_EVENT_t event_type, uint8_t radioNum, bool fromISR);
#endif
void     RADIO_Lock_Mutex(void);
void     RADIO_UnLock_Mutex(void);
void     RADIO_PreambleDetected(uint8_t radioNum);
void     RADIO_SyncDetected(uint8_t radioNum, float32_t offset);
void     RADIO_CaptureRSSI(uint8_t radioNum);
void     RADIO_UnBufferRSSI(uint8_t radioNum);
void     RADIO_TX_Watchdog(void);
void     RADIO_RX_WatchdogService(uint8_t radioNum);
void     RADIO_RX_Watchdog(void);
void     RADIO_Update_Freq_Watchdog(void);
RADIO_MODE_t RADIO_TxMode_Get(void);
void     RADIO_Mode_Set(RADIO_MODE_t TXmode);
bool     RADIO_Is_TX(void);
uint32_t RADIO_Status_Get(uint8_t radioNum, uint8_t *state, uint8_t *int_pend,   uint8_t *int_status,
                                                            uint8_t *ph_pend,    uint8_t *ph_status,
                                                            uint8_t *modem_pend, uint8_t *modem_status,
                                                            uint8_t *chip_pend,  uint8_t *chip_status);
#ifdef PHY_Protocol_H
float32_t RADIO_Filter_CCA(uint8_t radioNum, CCA_RSSI_TYPEe *processingType, uint16_t samplingRate);
void      RADIO_Set_SyncTime(uint8_t radioNum, TIMESTAMP_t syncTime, uint32_t syncTimeCYCCNT);
#endif
#if ( NOISEBAND_LOWEST_CAP_VOLTAGE == 0 )
void      RADIO_Get_RSSI(uint8_t radioNum, uint16_t chan, uint8_t *buf, uint16_t nSamples, uint16_t rate, uint8_t boost);
#else
float     RADIO_Get_RSSI(uint8_t radioNum, uint16_t chan, uint8_t *buf, uint16_t nSamples, uint16_t rate, uint8_t boost);
#endif
void      RADIO_Set_SyncError(uint8_t err);
uint8_t   RADIO_Get_CurrentRSSI(uint8_t radioNum);
bool      RADIO_Get_RxStartOccured(uint8_t radioNum);
uint32_t  RADIO_Get_RxStartCYCCNTTimeStamp(uint8_t radioNum);
uint32_t  RADIO_Get_IntFIFOCYCCNTTimeStamp(uint8_t radioNum);
void      RADIO_Clear_RxStartOccured(uint8_t radioNum);
void      RADIO_Set_RxStart(uint8_t radioNum, uint32_t cyccnt);
void      RADIO_Power_Level_Set(uint8_t powerLevel);
uint8_t   RADIO_Power_Level_Get(void);
bool      RADIO_Temperature_Get(uint8_t radioNum, int16_t *temp);
#ifdef PHY_Protocol_H
TX_FRAME_t *RADIO_Transmit_Buffer_Get(void);
#endif
bool     RADIO_Update_Noise_Floor(void);
bool     RADIO_Build_Noise_Floor(uint16_t const *Channels_list, uint8_t radioNum, bool useSpecifiedRadio, uint16_t samplingRate, bool useDefaultInError, int16_t *noiseEstimate, int16_t *noiseEstimateBoostOn);
#if ( DCU == 1 )
void     RADIO_SetPower( uint32_t frequency, float powerOutput );
#endif
void     RADIO_Update_Freq( void );
#ifdef TIME_SYS_H
bool     RADIO_TCXO_Set ( uint8_t radioNum, uint32_t TCXOfreq, TIME_SYS_SOURCE_e source, bool reset );
bool     RADIO_TCXO_Get ( uint8_t radioNum, uint32_t *TCXOfreq, TIME_SYS_SOURCE_e *source, uint32_t *seconds );
#endif
void     printHex ( char const *rawStr, const uint8_t *str, uint16_t num_bytes );
uint16_t RnCrc_Gen (uint16_t accum, uint8_t ch);
uint8_t  FEClength(uint8_t const *buffer);

#if ( NOISE_HIST_ENABLED == 1 )
void RADIO_SetupNoiseReadings ( uint8_t radioNum, uint16_t samplingDelay, uint16_t channel );
uint8_t RADIO_GetNoiseValue( uint8_t radioNum, uint16_t samplingDelay);
void RADIO_TerminateNoiseReadings(uint8_t radioNum);
#endif
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
float getVSWRvalue( meterReadingType value );
bool *get_VSWRholdOff( void );
#endif
/** @} */

#endif /* RADIO_H_ */
