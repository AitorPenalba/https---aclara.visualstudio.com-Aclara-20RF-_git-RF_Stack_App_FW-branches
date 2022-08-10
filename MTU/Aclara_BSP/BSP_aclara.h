/******************************************************************************
 *
 * Filename: BSP_aclara.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2010 - 2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 *****************************************************************************/
#ifndef BSP_aclara_H
#define BSP_aclara_H

/* INCLUDE FILES */

#ifndef __BOOTLOADER
#if ( MCU_SELECTED == NXP_K24 )
#include "user_config.h"
#include "psp_cpudef.h"
#endif
#endif   /* BOOTLOADER  */


/* #DEFINE DEFINITIONS */
#undef NULL
#define NULL               ((void *) 0)

#define MIN_RTC_MONTH         1
#define MAX_RTC_MONTH        12
#define MIN_RTC_DAY           1
#define MAX_RTC_DAY          31
#define MIN_RTC_YEAR       1970
#define MAX_RTC_YEAR       2099
#define MAX_RTC_HOUR         23
#define MAX_RTC_MINUTE       59
#define MAX_RTC_SECOND       59

/* Reset Status bits - that identify what the reset cause was */
#define RESET_SOURCE_POWER_ON_RESET      0x0001
#define RESET_SOURCE_EXTERNAL_RESET_PIN  0x0002
#define RESET_SOURCE_WATCHDOG            0x0004
#define RESET_SOURCE_LOSS_OF_EXT_CLOCK   0x0008
#define RESET_SOURCE_LOW_VOLTAGE         0x0010
#if ( MCU_SELECTED == NXP_K24 )
#define RESET_SOURCE_LOW_LEAKAGE_WAKEUP  0x0020
#elif ( MCU_SELECTED == RA6E1 )
#define RESET_SOURCE_DEEP_SW_STANDBY_CANCEL 0x0020
#endif
#define RESET_SOURCE_STOP_MODE_ACK_ERROR 0x0040
#define RESET_SOURCE_EZPORT_RESET        0x0080
#define RESET_SOURCE_MDM_AP              0x0100
#define RESET_SOURCE_SOFTWARE_RESET      0x0200
#define RESET_SOURCE_CORE_LOCKUP         0x0400
#define RESET_SOURCE_JTAG                0x0800
#define RESET_ANOMALY_COUNT              0x1000

/* ADC Temperature Read type */
#define TEMP_IN_DEG_F      ((bool)true)
#define TEMP_IN_DEG_C      ((bool)false)

/* Temperature Limits */
#if ( EP == 1 )
#define MAX_TEMP_LIMIT     ((int32_t)80) /* Maximum Operating Temperature of CPU is 105 Deg C and Radio is 85 Deg C. */
#define MIN_TEMP_LIMIT     ((int32_t)-40) /* Minimum Operating Temperature of CPU and Radio is -40 Deg C*/
#define STACK_HYSTERISIS   5 /* Stack enable/disable temperature hysteresis */
#endif

#if ( MCU_SELECTED == RA6E1 )
#define LWGPIO_VALUE_LOW   BSP_IO_LEVEL_LOW
#define DEMCR              DCB->DEMCR
#define DWT_CTRL           DWT->CTRL
#define DWT_CYCCNT         DWT->CYCCNT
#endif

#if ( MCU_SELECTED == RA6E1 )
#define AGT_FREQ_SYNC_TIMER_COUNT_MAX ((uint16_t)32768)
#endif

#define BSP_IS_GLOBAL_INT_ENABLED()  (bool)(0 == __get_PRIMASK() ? TRUE : FALSE)

#if ( MCU_SELECTED == RA6E1 )
#define printf( fmt, ... )         DBG_LW_printf( fmt, ##__VA_ARGS__ )
#endif

#if ( MCU_SELECTED == RA6E1 )
#define MPARTNUMBER_ADDR              0x010080f0
#define MCUVERSION_ADDR               0x010081B0
#endif

#if ( MCU_SELECTED == RA6E1 )
#define MPARTNUMBER_ADDR              0x010080f0
#define MCUVERSION_ADDR               0x010081B0
#endif

/* TYPE DEFINITIONS */
typedef enum
{
   GRN_LED,     // LED0
   RED_LED,     // LED1
   BLU_LED,     // LED2
   LED_RUN4,    // ?
   MAX_LED_ID
} enum_LED_ID;

typedef enum
{
   LED_OFF,
   LED_ON,
   LED_TOGGLE
} enum_LED_State;

typedef struct
{
   uint8_t  month;   /* 1 - 12 valid range */
   uint8_t  day;     /* 1 - 31 valid range */
   uint16_t year;    /* 1970 - 2099 valid range (due to MQX) */
   uint8_t  hour;    /* 0 - 23 valid range */
   uint8_t  min;     /* 0 - 59 valid range */  /*lint !e123 */
   uint8_t  sec;     /* 0 - 59 valid range */
   uint16_t msec;    /* 0 - 999 valid range */
} sysTime_dateFormat_t;

typedef struct
{
   uint16_t days_u16;       /* Number of days after January 1, 1900; 0 - 65,535 valid range */
   uint16_t counts_u16;     /* Number of 2.5 second increments after midnight; 0 - 34,599 (34,599 = 23:59:57.5)  valid range */
} sysTime_druFormat_t;

typedef enum
{
   UART_MANUF_TEST,
#if ( ( OPTICAL_PASS_THROUGH != 0 ) && ( MQX_CPU == PSP_CPU_MK24F120M ) )
   UART_OPTICAL_PORT,
#endif
   UART_DEBUG_PORT,
   UART_HOST_COMM_PORT,
   MAX_UART_ID
} enum_UART_ID;

/* Used by the AES module. */
typedef struct
{
   uint8_t *pKey;         /* Pointer to 16-byte key */
   uint8_t *pInitVector;  /* Pointer to 16-byte Init Vector */
}aesKey_T;

typedef enum
{
   NORMALMODE = ((uint8_t)0),    /* state when EUT is normal operation mode */
   QUIETMODE,                    /* indicates EUT has been put into quietmode */
   SHIPMODE,                     /* indicates EUT has been put into shipmode */
   METERSHOPMODE,                /* indicates EUT has been put into metershop/decommission mode */
   MANUAL_GRN                    /* LED is currently in manual control */
}enum_GreenLedStatus_t;

typedef enum
{
   TIME_IDLE = ((uint8_t)0),     /* Timeout has expired on LED operation from power up */
   VALID_TIME,                   /* Endpoint currently has a valid time */
   NO_VALID_TIME,                /* Endpoint currently has an invalid time */
   MANUAL_BLU                   /*  LED is currently in manual control */
}enum_BlueLedStatus_t;

typedef enum
{
   DIAGNOSTIC_IDLE = ((uint8_t)0),  /* Diagnostic timeout from powerup has expired */
   MICRO_RUNNING,                   /* indicates the micro is powered and running until timeout */
   SELFTEST_RUNNING,                /* indicates that the self test is currently running */
   SELFTEST_FAIL,                   /* indicates self test ran and results failed */
   SELFTEST_PASS,                   /* indicates self test ran and results passed */
   RADIO_ENABLED,                   /* indicates when micro temp is over stack disable limit */
   RADIO_DISABLED,                  /* indicates when micro temp is under stack disable limit */
   MANUAL_RED                       /* LED is currently in manual control */
}enum_RedLedStatus_t;

#if (RTOS_SELECTION == FREE_RTOS) /* TODO: Remove this struct as this is mqx structure. For now added for successful compilation */
typedef struct time_struct
{

/*! \brief The number of seconds in the time. */
uint32_t SECONDS;

/*! \brief The number of milliseconds in the time. */
uint32_t MILLISECONDS;

} TIME_STRUCT, * TIME_STRUCT_PTR;
#endif

/* CONSTANTS */

/* KTL - 16 byte key: "ultrapassword123" - This is temporary and will change to an external key later */
extern const uint8_t AES_128_KEY[];

/* KTL - 16 byte initialization vector: "mysecretpassword" - This is temporary and will change to external value later
 * (may also be referred as nonce? */
extern const uint8_t AES_128_INIT_VECTOR[];


/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
#if ( OVERRIDE_TEMPERATURE == 1 )
extern void        ADC_Set_Man_Temperature   ( float newTemperature );
extern float       ADC_Get_Man_Temperature   ( void );
#endif
extern float       ADC_Get_uP_Temperature    ( bool bFahrenheit );
#if (0)  //K22 is the only one that supports the Board temp. sensor
extern float       ADC_Get_Board_Temperature ( bool bFahrenheit );
#endif
#ifndef __BOOTLOADER
float              ADC_Get_VSWR              ( void );
extern float       ADC_Get_SC_Voltage        ( void );
extern float       ADC_Get_4V0_Voltage       ( void );
extern float       ADC_GetHWRevVoltage       ( void );
extern uint8_t     ADC_GetHWRevLetter        ( void );
#if ( MCU_SELECTED == NXP_K24 )
extern char const *BSP_Get_BspRevision       ( void );
extern char const *BSP_Get_IoRevision        ( void );
extern char const *BSP_Get_PspRevision       ( void );
#elif ( MCU_SELECTED == RA6E1 )
extern char const *BSP_Get_BSPVersion        ( void );
#endif
extern uint16_t    BSP_Get_ResetStatus       ( void );
extern void        BSP_Setup_VREF            ( void );

extern uint16_t    CRC_16_Calculate          ( uint8_t *Data, uint32_t Length );
#if ( DCU == 1)
extern uint16_t    CRC_16_DcuHack            ( uint8_t *Data, uint32_t Length );
#endif
extern uint16_t    CRC_16_PhyHeader          ( uint8_t *Data, uint32_t Length );
extern uint32_t    CRC_32_Calculate          ( uint8_t *Data, uint32_t Length );

extern void        LED_on                    ( enum_LED_ID LedId );
extern void        LED_off                   ( enum_LED_ID LedId );
extern void        LED_toggle                ( enum_LED_ID LedId );
extern uint16_t    LED_getLedTimerId         ( void );
extern void        LED_setGreenLedStatus     ( enum_GreenLedStatus_t status);
extern void        LED_setBlueLedStatus      ( enum_BlueLedStatus_t status);
extern void        LED_setRedLedStatus       ( enum_RedLedStatus_t status);
extern void        LED_enableManualControl   ( void );
extern void        LED_checkModeStatus       ( void );
extern uint32_t    RNG_GetRandom_32bit       ( void );
extern void        RNG_GetRandom_Array       ( uint8_t *DataPtr, uint16_t DataLen );

extern void        RTC_GetDateTime           ( sysTime_dateFormat_t *RT_Clock );
extern bool        RTC_SetDateTime           ( const sysTime_dateFormat_t *RT_Clock );
extern bool        RTC_Valid                 ( void );
extern void        RTC_GetTimeInSecMicroSec  ( uint32_t *sec, uint32_t *microSec );
extern void        RTC_GetTimeAtRes          ( TIME_STRUCT *ptime, uint16_t fractRes );
#if ( MCU_SELECTED == RA6E1 )
extern returnStatus_t RTC_init( void );
extern void           RTC_ConfigureAlarm( uint32_t seconds );
extern void           RTC_DisableAlarm  ( void );
extern void           RTC_Start         ( void );
extern void           RTC_Callback      (rtc_callback_args_t *p_args);
extern bool           RTC_isRunning     ( void );
#if ( TM_RTC_UNIT_TEST == 1 )
extern bool           RTC_UnitTest      ( void );
#endif
#endif

extern uint32_t    UART_write                ( enum_UART_ID UartId, const uint8_t *DataBuffer, uint32_t DataLength );
#if ( MCU_SELECTED == NXP_K24 )
extern uint32_t    UART_read                 ( enum_UART_ID UartId,       uint8_t *DataBuffer, uint32_t DataLength );
#endif
#if ( MCU_SELECTED == RA6E1 )
extern uint32_t    UART_getc                 ( enum_UART_ID UartId,       uint8_t *DataBuffer, uint32_t DataLength, uint32_t TimeoutMs );
extern uint32_t    UART_echo                 ( enum_UART_ID UartId, const uint8_t *DataBuffer, uint32_t DataLength );
#endif
extern void        UART_fgets                ( enum_UART_ID UartId,       char    *DataBuffer, uint32_t DataLength );
extern uint8_t     UART_flush                ( enum_UART_ID UartId );
extern void        UART_RX_flush             ( enum_UART_ID UartId );
extern uint8_t     UART_isTxEmpty            ( enum_UART_ID UartId );
extern uint8_t     UART_ioctl                ( enum_UART_ID UartId, int op, void *addr );
extern uint8_t     UART_SetEcho              ( enum_UART_ID UartId, bool val );
extern uint8_t     UART_close                ( enum_UART_ID UartId );
extern uint8_t     UART_open                 ( enum_UART_ID UartId );
extern bool        UART_gotChar              ( enum_UART_ID UartId );
extern void *      UART_getHandle            ( enum_UART_ID UartId );
extern uint32_t    UART_polled_printf        ( char *fmt, ... );
#endif   /* BOOTLOADER  */
extern void        WDOG_Kick                 ( void );
extern void        WDOG_Disable              ( void );

#ifdef ERROR_CODES_H_
extern returnStatus_t ADC_init       ( void );
extern returnStatus_t ADC_ShutDown   ( void );
extern returnStatus_t CRC_initialize ( void );
extern returnStatus_t LED_init       ( void );
extern returnStatus_t RNG_init       ( void );
extern returnStatus_t UART_init      ( void );
#if ( DEBUG_PORT_BAUD_RATE == 1 )
extern char *         UART_getName   ( enum_UART_ID uartId );
extern returnStatus_t UART_setBaud   ( enum_UART_ID uartId, baud_setting_t *baudSetting );
#endif
extern returnStatus_t UART_reset     ( enum_UART_ID UartId );
extern returnStatus_t WDOG_Init      ( void );
extern returnStatus_t IO_init        ( void );
extern returnStatus_t CRC_Shutdown   ( void );
#if ( MCU_SELECTED == RA6E1 )
extern fsp_err_t      AGT_LPM_Timer_Init      ( void );
extern fsp_err_t      AGT_LPM_Timer_Start     ( void );
extern fsp_err_t      AGT_LPM_Timer_Stop      ( void );
extern fsp_err_t      AGT_LPM_Timer_Configure ( uint32_t const period );
extern void           AGT_LPM_Timer_Wait      ( void );
extern returnStatus_t AGT_PD_Init             ( void );
extern void           AGT_PD_Enable           ( void );
extern returnStatus_t AGT_FreqSyncTimerInit   (void);
extern returnStatus_t AGT_FreqSyncTimerStart  (void);
extern returnStatus_t AGT_FreqSyncTimerStop   (void);
extern returnStatus_t AGT_FreqSyncTimerCount  (uint16_t *count);
#if ( GENERATE_RUN_TIME_STATS == 1 )
extern void           AGT_RunTimeStatsStart   ( void );
extern uint32_t       AGT_RunTimeStatsCount   ( void );
#endif
extern returnStatus_t GPT_PD_Init             ( void );
extern void           GPT_PD_Enable           ( void );
extern void           GPT_PD_Disable          ( void );
extern returnStatus_t GPT_Radio0_Init         ( void );
extern void           GPT_Radio0_Enable       ( void );
extern void           GPT_Radio0_Disable      ( void );
#endif

#endif /* ERROR_CODES_H_ */


#if ( ENABLE_AES_CODE == 1 )
#  ifdef ERROR_CODES_H_
extern returnStatus_t AES_128_Init ( void );
#  endif /* ERROR_CODES_H_ */
extern void AES_128_EncryptData ( uint8_t *pDest, const uint8_t *pSrc, uint32_t cnt, aesKey_T const *pAesKey);
extern void AES_128_DecryptData ( uint8_t *pDest, const uint8_t *pSrc, uint32_t cnt, aesKey_T const *pAesKey );
extern void AES_128_UnitTest( void );
#endif /* ENABLE_AES_CODE */

#if ( MCU_SELECTED == RA6E1 )
extern fsp_err_t CGC_Stop_Unused_Clocks         ( void );
extern void      CGC_Switch_SystemClock_to_MOCO ( void );
/* TODO: RA6: Move AGT function prototypes here */
#endif

/* FUNCTION DEFINITIONS */

#endif /* this must be the last line of the file */
