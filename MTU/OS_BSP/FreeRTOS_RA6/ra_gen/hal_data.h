/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_agt.h"
#include "r_timer_api.h"
#include "r_gpt.h"
#include "r_timer_api.h"
#include "r_lpm.h"
#include "r_lpm_api.h"
#include "r_iwdt.h"
            #include "r_wdt_api.h"
#include "r_dac.h"
#include "r_dac_api.h"
#include "r_sci_uart.h"
            #include "r_uart_api.h"
#include "r_crc.h"
#include "r_crc_api.h"
#include "r_flash_hp.h"
#include "r_flash_api.h"
#include "r_qspi.h"
            #include "r_spi_flash_api.h"
#include "r_dtc.h"
#include "r_transfer_api.h"
#include "r_spi.h"
#include "r_rtc.h"
#include "r_rtc_api.h"
#include "r_adc.h"
#include "r_adc_api.h"
#include "r_iic_master.h"
#include "r_i2c_master_api.h"
FSP_HEADER
/** AGT Timer Instance */
extern const timer_instance_t AGT5_RunTimeStats_1;

/** Access the AGT instance using these structures when calling API functions directly (::p_api is not used). */
extern agt_instance_ctrl_t AGT5_RunTimeStats_1_ctrl;
extern const timer_cfg_t AGT5_RunTimeStats_1_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** AGT Timer Instance */
extern const timer_instance_t AGT4_RunTimeStats_0;

/** Access the AGT instance using these structures when calling API functions directly (::p_api is not used). */
extern agt_instance_ctrl_t AGT4_RunTimeStats_0_ctrl;
extern const timer_cfg_t AGT4_RunTimeStats_0_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** AGT Timer Instance */
extern const timer_instance_t agt2_Freq_Sync;

/** Access the AGT instance using these structures when calling API functions directly (::p_api is not used). */
extern agt_instance_ctrl_t agt2_Freq_Sync_ctrl;
extern const timer_cfg_t agt2_Freq_Sync_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t GPT2_ZCD_Meter;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t GPT2_ZCD_Meter_ctrl;
extern const timer_cfg_t GPT2_ZCD_Meter_cfg;

#ifndef ZCD_hwIsr
void ZCD_hwIsr(timer_callback_args_t * p_args);
#endif
/** lpm Instance */
extern const lpm_instance_t g_lpm_DeepSWStandby_AGT;

/** Access the LPM instance using these structures when calling API functions directly (::p_api is not used). */
extern lpm_instance_ctrl_t g_lpm_DeepSWStandby_AGT_ctrl;
extern const lpm_cfg_t g_lpm_DeepSWStandby_AGT_cfg;
/** WDT on IWDT Instance. */
extern const wdt_instance_t g_wdt0;


/** Access the IWDT instance using these structures when calling API functions directly (::p_api is not used). */
extern iwdt_instance_ctrl_t g_wdt0_ctrl;
extern const wdt_cfg_t g_wdt0_cfg;

#ifndef NULL
void NULL(wdt_callback_args_t * p_args);
#endif
/** DAC on DAC Instance. */
extern const dac_instance_t g_dac0_ULPC;

/** Access the DAC instance using these structures when calling API functions directly (::p_api is not used). */
extern dac_instance_ctrl_t g_dac0_ULPC_ctrl;
extern const dac_cfg_t g_dac0_ULPC_cfg;
/** UART on SCI Instance. */
extern const uart_instance_t      g_uart_lpm_dbg;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_uart_instance_ctrl_t     g_uart_lpm_dbg_ctrl;
extern const uart_cfg_t g_uart_lpm_dbg_cfg;
extern const sci_uart_extended_cfg_t g_uart_lpm_dbg_cfg_extend;

#ifndef lpm_dbg_uart_callback
void lpm_dbg_uart_callback( uart_callback_args_t* p_args );
#endif
/** lpm Instance */
extern const lpm_instance_t g_lpm_DeepSWStandby;

/** Access the LPM instance using these structures when calling API functions directly (::p_api is not used). */
extern lpm_instance_ctrl_t g_lpm_DeepSWStandby_ctrl;
extern const lpm_cfg_t g_lpm_DeepSWStandby_cfg;
/** lpm Instance */
extern const lpm_instance_t g_lpm_SW_Standby;

/** Access the LPM instance using these structures when calling API functions directly (::p_api is not used). */
extern lpm_instance_ctrl_t g_lpm_SW_Standby_ctrl;
extern const lpm_cfg_t g_lpm_SW_Standby_cfg;
/** AGT Timer Instance */
extern const timer_instance_t AGT1_LPM_Wakeup;

/** Access the AGT instance using these structures when calling API functions directly (::p_api is not used). */
extern agt_instance_ctrl_t AGT1_LPM_Wakeup_ctrl;
extern const timer_cfg_t AGT1_LPM_Wakeup_cfg;

#ifndef agt1_timer_callback
void agt1_timer_callback(timer_callback_args_t * p_args);
#endif
extern const crc_instance_t g_crc1;
extern crc_instance_ctrl_t g_crc1_ctrl;
extern const crc_cfg_t g_crc1_cfg;
/** AGT Timer Instance */
extern const timer_instance_t AGT0_ExtFlashBusy;

/** Access the AGT instance using these structures when calling API functions directly (::p_api is not used). */
extern agt_instance_ctrl_t AGT0_ExtFlashBusy_ctrl;
extern const timer_cfg_t AGT0_ExtFlashBusy_cfg;

#ifndef AGT0_ExtFlashBusy_Callback
void AGT0_ExtFlashBusy_Callback(timer_callback_args_t * p_args);
#endif
/* Flash on Flash HP Instance */
extern const flash_instance_t g_flash0;

/** Access the Flash HP instance using these structures when calling API functions directly (::p_api is not used). */
extern flash_hp_instance_ctrl_t g_flash0_ctrl;
extern const flash_cfg_t g_flash0_cfg;

#ifndef dataBgoCallback
void dataBgoCallback(flash_callback_args_t * p_args);
#endif
extern const spi_flash_instance_t g_qspi0;
            extern qspi_instance_ctrl_t g_qspi0_ctrl;
            extern const spi_flash_cfg_t g_qspi0_cfg;
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer1;

/** Access the DTC instance using these structures when calling API functions directly (::p_api is not used). */
extern dtc_instance_ctrl_t g_transfer1_ctrl;
extern const transfer_cfg_t g_transfer1_cfg;
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer0;

/** Access the DTC instance using these structures when calling API functions directly (::p_api is not used). */
extern dtc_instance_ctrl_t g_transfer0_ctrl;
extern const transfer_cfg_t g_transfer0_cfg;
/** SPI on SPI Instance. */
extern const spi_instance_t g_spi1;

/** Access the SPI instance using these structures when calling API functions directly (::p_api is not used). */
extern spi_instance_ctrl_t g_spi1_ctrl;
extern const spi_cfg_t g_spi1_cfg;

/** Callback used by SPI Instance. */
#ifndef spi_callback
void spi_callback(spi_callback_args_t * p_args);
#endif


#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == g_transfer0)
    #define g_spi1_P_TRANSFER_TX (NULL)
#else
    #define g_spi1_P_TRANSFER_TX (&g_transfer0)
#endif
#if (RA_NOT_DEFINED == g_transfer1)
    #define g_spi1_P_TRANSFER_RX (NULL)
#else
    #define g_spi1_P_TRANSFER_RX (&g_transfer1)
#endif
#undef RA_NOT_DEFINED
/** Timer on GPT Instance. */
extern const timer_instance_t GPT1_Radio0_ICapture;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t GPT1_Radio0_ICapture_ctrl;
extern const timer_cfg_t GPT1_Radio0_ICapture_cfg;

#ifndef Radio0_IC_ISR
void Radio0_IC_ISR(timer_callback_args_t * p_args);
#endif
/* RTC Instance. */
extern const rtc_instance_t g_rtc0;

/** Access the RTC instance using these structures when calling API functions directly (::p_api is not used). */
extern rtc_instance_ctrl_t g_rtc0_ctrl;
extern const rtc_cfg_t g_rtc0_cfg;

#ifndef RTC_Callback
void RTC_Callback(rtc_callback_args_t * p_args);
#endif
/** ADC on ADC Instance. */
extern const adc_instance_t g_adc0;

/** Access the ADC instance using these structures when calling API functions directly (::p_api is not used). */
extern adc_instance_ctrl_t g_adc0_ctrl;
extern const adc_cfg_t g_adc0_cfg;
extern const adc_channel_cfg_t g_adc0_channel_cfg;

#ifndef NULL
void NULL(adc_callback_args_t * p_args);
#endif

#ifndef NULL
#define ADC_DMAC_CHANNELS_PER_BLOCK_NULL  3
#endif
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart_optical;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_uart_instance_ctrl_t     g_uart_optical_ctrl;
            extern const uart_cfg_t g_uart_optical_cfg;
            extern const sci_uart_extended_cfg_t g_uart_optical_cfg_extend;

            #ifndef optical_uart_callback
            void optical_uart_callback(uart_callback_args_t * p_args);
            #endif
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart_DBG;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_uart_instance_ctrl_t     g_uart_DBG_ctrl;
            extern const uart_cfg_t g_uart_DBG_cfg;
            extern const sci_uart_extended_cfg_t g_uart_DBG_cfg_extend;

            #ifndef dbg_uart_callback
            void dbg_uart_callback(uart_callback_args_t * p_args);
            #endif
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart_MFG;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_uart_instance_ctrl_t     g_uart_MFG_ctrl;
            extern const uart_cfg_t g_uart_MFG_cfg;
            extern const sci_uart_extended_cfg_t g_uart_MFG_cfg_extend;

            #ifndef mfg_uart_callback
            void mfg_uart_callback(uart_callback_args_t * p_args);
            #endif
/* I2C Master on IIC Instance. */
extern const i2c_master_instance_t g_i2c_master0;

/** Access the I2C Master instance using these structures when calling API functions directly (::p_api is not used). */
extern iic_master_instance_ctrl_t g_i2c_master0_ctrl;
extern const i2c_master_cfg_t g_i2c_master0_cfg;

#ifndef iic_eventCallback
void iic_eventCallback(i2c_master_callback_args_t * p_args);
#endif
extern const crc_instance_t g_crc0;
extern crc_instance_ctrl_t g_crc0_ctrl;
extern const crc_cfg_t g_crc0_cfg;
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart_HMC;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_uart_instance_ctrl_t     g_uart_HMC_ctrl;
            extern const uart_cfg_t g_uart_HMC_cfg;
            extern const sci_uart_extended_cfg_t g_uart_HMC_cfg_extend;

            #ifndef hmc_uart_callback
            void hmc_uart_callback(uart_callback_args_t * p_args);
            #endif
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
