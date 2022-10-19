/* generated HAL source file - do not edit */
#include "hal_data.h"
#include "cfg_hal_defs.h" /* Aclara modification to know which board is being used */
#include "hal.h"          /* Aclara modification to know which board is being used */

/* Macros to tie dynamic ELC links to ADC_TRIGGER_SYNC_ELC option in adc_trigger_t. */
#define ADC_TRIGGER_ADC0        ADC_TRIGGER_SYNC_ELC
#define ADC_TRIGGER_ADC0_B      ADC_TRIGGER_SYNC_ELC
#define ADC_TRIGGER_ADC1        ADC_TRIGGER_SYNC_ELC
#define ADC_TRIGGER_ADC1_B      ADC_TRIGGER_SYNC_ELC
agt_instance_ctrl_t AGT5_RunTimeStats_1_ctrl;
const agt_extended_cfg_t AGT5_RunTimeStats_1_extend =
{
    .count_source            = AGT_CLOCK_AGT_UNDERFLOW,
    .agto                    = AGT_PIN_CFG_DISABLED,
    .agtoab_settings_b.agtoa = AGT_PIN_CFG_DISABLED,
    .agtoab_settings_b.agtob = AGT_PIN_CFG_DISABLED,
    .measurement_mode        = AGT_MEASURE_DISABLED,
    .agtio_filter            = AGT_AGTIO_FILTER_NONE,
    .enable_pin              = AGT_ENABLE_PIN_NOT_USED,
    .trigger_edge            = AGT_TRIGGER_EDGE_RISING,
};
const timer_cfg_t AGT5_RunTimeStats_1_cfg =
{
    .mode                = TIMER_MODE_PERIODIC,
    /* Actual period: 0.0021845 seconds. Actual duty: 49.999237048905165%. */ .period_counts = (uint32_t) 0xffff, .duty_cycle_counts = 0x7fff, .source_div = (timer_source_div_t)0,
    .channel             = 5,
    .p_callback          = NULL,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = &AGT5_RunTimeStats_1_extend,
    .cycle_end_ipl       = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_AGT5_INT)
    .cycle_end_irq       = VECTOR_NUMBER_AGT5_INT,
#else
    .cycle_end_irq       = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const timer_instance_t AGT5_RunTimeStats_1 =
{
    .p_ctrl        = &AGT5_RunTimeStats_1_ctrl,
    .p_cfg         = &AGT5_RunTimeStats_1_cfg,
    .p_api         = &g_timer_on_agt
};
agt_instance_ctrl_t AGT4_RunTimeStats_0_ctrl;
const agt_extended_cfg_t AGT4_RunTimeStats_0_extend =
{
    .count_source            = AGT_CLOCK_SUBCLOCK,
    .agto                    = AGT_PIN_CFG_DISABLED,
    .agtoab_settings_b.agtoa = AGT_PIN_CFG_DISABLED,
    .agtoab_settings_b.agtob = AGT_PIN_CFG_DISABLED,
    .measurement_mode        = AGT_MEASURE_DISABLED,
    .agtio_filter            = AGT_AGTIO_FILTER_NONE,
    .enable_pin              = AGT_ENABLE_PIN_NOT_USED,
    .trigger_edge            = AGT_TRIGGER_EDGE_RISING,
};
const timer_cfg_t AGT4_RunTimeStats_0_cfg =
{
    .mode                = TIMER_MODE_PERIODIC,
    /* Actual period: 2 seconds. Actual duty: 50%. */ .period_counts = (uint32_t) 0x10000, .duty_cycle_counts = 0x8000, .source_div = (timer_source_div_t)0,
    .channel             = 4,
    .p_callback          = NULL,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = &AGT4_RunTimeStats_0_extend,
    .cycle_end_ipl       = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_AGT4_INT)
    .cycle_end_irq       = VECTOR_NUMBER_AGT4_INT,
#else
    .cycle_end_irq       = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const timer_instance_t AGT4_RunTimeStats_0 =
{
    .p_ctrl        = &AGT4_RunTimeStats_0_ctrl,
    .p_cfg         = &AGT4_RunTimeStats_0_cfg,
    .p_api         = &g_timer_on_agt
};
agt_instance_ctrl_t agt2_Freq_Sync_ctrl;
const agt_extended_cfg_t agt2_Freq_Sync_extend =
{
    .count_source            = AGT_CLOCK_SUBCLOCK,
    .agto                    = AGT_PIN_CFG_DISABLED,
    .agtoab_settings_b.agtoa = AGT_PIN_CFG_DISABLED,
    .agtoab_settings_b.agtob = AGT_PIN_CFG_DISABLED,
    .measurement_mode        = AGT_MEASURE_DISABLED,
    .agtio_filter            = AGT_AGTIO_FILTER_NONE,
    .enable_pin              = AGT_ENABLE_PIN_NOT_USED,
    .trigger_edge            = AGT_TRIGGER_EDGE_RISING,
};
const timer_cfg_t agt2_Freq_Sync_cfg =
{
    .mode                = TIMER_MODE_PERIODIC,
    /* Actual period: 1 seconds. Actual duty: 50%. */ .period_counts = (uint32_t) 0x8000, .duty_cycle_counts = 0x4000, .source_div = (timer_source_div_t)0,
    .channel             = 2,
    .p_callback          = NULL,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = &agt2_Freq_Sync_extend,
    .cycle_end_ipl       = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_AGT2_INT)
    .cycle_end_irq       = VECTOR_NUMBER_AGT2_INT,
#else
    .cycle_end_irq       = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const timer_instance_t agt2_Freq_Sync =
{
    .p_ctrl        = &agt2_Freq_Sync_ctrl,
    .p_cfg         = &agt2_Freq_Sync_cfg,
    .p_api         = &g_timer_on_agt
};
gpt_instance_ctrl_t GPT2_ZCD_Meter_ctrl;
#if 0
const gpt_extended_pwm_cfg_t GPT2_ZCD_Meter_pwm_extend =
{
    .trough_ipl          = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT2_COUNTER_UNDERFLOW)
    .trough_irq          = VECTOR_NUMBER_GPT2_COUNTER_UNDERFLOW,
#else
    .trough_irq          = FSP_INVALID_VECTOR,
#endif
    .poeg_link           = GPT_POEG_LINK_POEG0,
    .output_disable      = (gpt_output_disable_t) ( GPT_OUTPUT_DISABLE_NONE),
    .adc_trigger         = (gpt_adc_trigger_t) ( GPT_ADC_TRIGGER_NONE),
    .dead_time_count_up  = 0,
    .dead_time_count_down = 0,
    .adc_a_compare_match = 0,
    .adc_b_compare_match = 0,
    .interrupt_skip_source = GPT_INTERRUPT_SKIP_SOURCE_NONE,
    .interrupt_skip_count  = GPT_INTERRUPT_SKIP_COUNT_0,
    .interrupt_skip_adc    = GPT_INTERRUPT_SKIP_ADC_NONE,
    .gtioca_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
    .gtiocb_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
};
#endif
const gpt_extended_cfg_t GPT2_ZCD_Meter_extend =
{
    .gtioca = { .output_enabled = false,
                .stop_level     = GPT_PIN_LEVEL_LOW
              },
    .gtiocb = { .output_enabled = false,
                .stop_level     = GPT_PIN_LEVEL_LOW
              },
    .start_source        = (gpt_source_t) ( GPT_SOURCE_NONE),
    .stop_source         = (gpt_source_t) ( GPT_SOURCE_NONE),
    .clear_source        = (gpt_source_t) ( GPT_SOURCE_NONE),
    .count_up_source     = (gpt_source_t) ( GPT_SOURCE_NONE),
    .count_down_source   = (gpt_source_t) ( GPT_SOURCE_NONE),
    .capture_a_source    = (gpt_source_t) ( GPT_SOURCE_NONE),
    .capture_b_source    = (gpt_source_t) (GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_LOW |  GPT_SOURCE_NONE),
    .capture_a_ipl       = (BSP_IRQ_DISABLED),
    .capture_b_ipl       = (10),
#if defined(VECTOR_NUMBER_GPT2_CAPTURE_COMPARE_A)
    .capture_a_irq       = VECTOR_NUMBER_GPT2_CAPTURE_COMPARE_A,
#else
    .capture_a_irq       = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT2_CAPTURE_COMPARE_B)
    .capture_b_irq       = VECTOR_NUMBER_GPT2_CAPTURE_COMPARE_B,
#else
    .capture_b_irq       = FSP_INVALID_VECTOR,
#endif
    .capture_filter_gtioca       = GPT_CAPTURE_FILTER_NONE,
    .capture_filter_gtiocb       = GPT_CAPTURE_FILTER_NONE,
#if 0
    .p_pwm_cfg                   = &GPT2_ZCD_Meter_pwm_extend,
#else
    .p_pwm_cfg                   = NULL,
#endif
#if 0
    .gtior_setting.gtior_b.gtioa  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.oadflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.oahld  = 0U,
    .gtior_setting.gtior_b.oae    = (uint32_t) false,
    .gtior_setting.gtior_b.oadf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfaen  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsa  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
    .gtior_setting.gtior_b.gtiob  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.obdflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.obhld  = 0U,
    .gtior_setting.gtior_b.obe    = (uint32_t) false,
    .gtior_setting.gtior_b.obdf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfben  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsb  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
#else
    .gtior_setting.gtior = 0U,
#endif
};
const timer_cfg_t GPT2_ZCD_Meter_cfg =
{
    .mode                = TIMER_MODE_PERIODIC,
    /* Actual period: 71.58278826666667 seconds. Actual duty: 50%. */ .period_counts = (uint32_t) 0x100000000, .duty_cycle_counts = 0x80000000, .source_div = (timer_source_div_t)0,
    .channel             = 2,
    .p_callback          = ZCD_hwIsr,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = &GPT2_ZCD_Meter_extend,
    .cycle_end_ipl       = (10),
#if defined(VECTOR_NUMBER_GPT2_COUNTER_OVERFLOW)
    .cycle_end_irq       = VECTOR_NUMBER_GPT2_COUNTER_OVERFLOW,
#else
    .cycle_end_irq       = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const timer_instance_t GPT2_ZCD_Meter =
{
    .p_ctrl        = &GPT2_ZCD_Meter_ctrl,
    .p_cfg         = &GPT2_ZCD_Meter_cfg,
    .p_api         = &g_timer_on_gpt
};
lpm_instance_ctrl_t g_lpm_DeepSWStandby_AGT_ctrl;

const lpm_cfg_t g_lpm_DeepSWStandby_AGT_cfg =
{
    .low_power_mode     = LPM_MODE_DEEP,
    .snooze_cancel_sources      = LPM_SNOOZE_CANCEL_SOURCE_NONE,
    .standby_wake_sources       = LPM_STANDBY_WAKE_SOURCE_IRQ11 | LPM_STANDBY_WAKE_SOURCE_AGT1UD |  (lpm_standby_wake_source_t) 0,
    .snooze_request_source      = LPM_SNOOZE_REQUEST_RXD0_FALLING,
    .snooze_end_sources         =  (lpm_snooze_end_t) 0,
    .dtc_state_in_snooze        = LPM_SNOOZE_DTC_DISABLE,
#if BSP_FEATURE_LPM_HAS_SBYCR_OPE
    .output_port_enable         = LPM_OUTPUT_PORT_ENABLE_HIGH_IMPEDANCE,
#endif
#if BSP_FEATURE_LPM_HAS_DEEP_STANDBY
    .io_port_state              = LPM_IO_PORT_NO_CHANGE,
    .power_supply_state         = LPM_POWER_SUPPLY_DEEPCUT0,
    .deep_standby_cancel_source = LPM_DEEP_STANDBY_CANCEL_SOURCE_IRQ11 | LPM_DEEP_STANDBY_CANCEL_SOURCE_AGT1 |  (lpm_deep_standby_cancel_source_t) 0,
    .deep_standby_cancel_edge   = LPM_DEEP_STANDBY_CANCEL_SOURCE_IRQ11_RISING |  (lpm_deep_standby_cancel_edge_t) 0,
#endif
    .p_extend           = NULL,
};

const lpm_instance_t g_lpm_DeepSWStandby_AGT =
{
    .p_api = &g_lpm_on_lpm,
    .p_ctrl = &g_lpm_DeepSWStandby_AGT_ctrl,
    .p_cfg = &g_lpm_DeepSWStandby_AGT_cfg
};
iwdt_instance_ctrl_t g_wdt0_ctrl;

const wdt_cfg_t g_wdt0_cfg =
{
    .p_callback = NULL,
};

/* Instance structure to use this module. */
const wdt_instance_t g_wdt0 =
{
    .p_ctrl        = &g_wdt0_ctrl,
    .p_cfg         = &g_wdt0_cfg,
    .p_api         = &g_wdt_on_iwdt
};
dac_instance_ctrl_t g_dac0_ULPC_ctrl;
const dac_extended_cfg_t g_dac0_ULPC_ext_cfg =
{
    .enable_charge_pump   = 0,
    .data_format         = DAC_DATA_FORMAT_FLUSH_RIGHT,
    .output_amplifier_enabled = 1,
    .internal_output_enabled = false,
};
const dac_cfg_t g_dac0_ULPC_cfg =
{
    .channel             = 0,
    .ad_da_synchronized  = false,
    .p_extend            = &g_dac0_ULPC_ext_cfg
};
/* Instance structure to use this module. */
const dac_instance_t g_dac0_ULPC =
{
    .p_ctrl    = &g_dac0_ULPC_ctrl,
    .p_cfg     = &g_dac0_ULPC_cfg,
    .p_api     = &g_dac_on_dac
};
sci_uart_instance_ctrl_t     g_uart_lpm_dbg_ctrl;

#define DEBUG_BAUD_RATE 115200 /* Aclara added: allow static (boot-up) configuration of the debug serial port baud rate */

            baud_setting_t               g_uart_lpm_dbg_baud_setting =
            {
/* Aclara Modified: configure UART based on BAUD RATE selection */
#if ( DEBUG_BAUD_RATE == 115200 )
                /* Baud rate 115200 calculated with 1.725% error. */ .semr_baudrate_bits_b.abcse = 0, .semr_baudrate_bits_b.abcs = 0, .semr_baudrate_bits_b.bgdm = 1, .cks = 0, .brr = 31, .mddr = (uint8_t) 256, .semr_baudrate_bits_b.brme = false
#elif ( DEBUG_BAUD_RATE == 38400 )
                /* Baud rate  38400 calculated with 0.677% error. */ .semr_baudrate_bits_b.abcse = 0, .semr_baudrate_bits_b.abcs = 0, .semr_baudrate_bits_b.bgdm = 1, .cks = 0, .brr = 96, .mddr = (uint8_t) 256, .semr_baudrate_bits_b.brme = false
#else
                #error "Unsupported baud rate configured for the debug port!"
#endif
/* Aclara Modified - End */
            };

            /** UART extended configuration for UARTonSCI HAL driver */
            const sci_uart_extended_cfg_t g_uart_lpm_dbg_cfg_extend =
            {
                .clock                = SCI_UART_CLOCK_INT,
                .rx_edge_start          = SCI_UART_START_BIT_FALLING_EDGE,
                .noise_cancel         = SCI_UART_NOISE_CANCELLATION_DISABLE,
                .rx_fifo_trigger        = SCI_UART_RX_FIFO_TRIGGER_MAX,
                .p_baud_setting         = &g_uart_lpm_dbg_baud_setting,
                .flow_control           = SCI_UART_FLOW_CONTROL_RTS,
                #if 0xFF != 0xFF
                .flow_control_pin       = BSP_IO_PORT_FF_PIN_0xFF,
                #else
                .flow_control_pin       = (bsp_io_port_pin_t) UINT16_MAX,
                #endif
                .rs485_setting = {
                    .enable = SCI_UART_RS485_DISABLE,
                    .polarity = SCI_UART_RS485_DE_POLARITY_HIGH,
                #if 0xFF != 0xFF
                    .de_control_pin = BSP_IO_PORT_FF_PIN_0xFF,
                #else
                    .de_control_pin       = (bsp_io_port_pin_t) UINT16_MAX,
                #endif
                },
            };

            /** UART interface configuration */
            const uart_cfg_t g_uart_lpm_dbg_cfg =
            {
                .channel             = 4,
                .data_bits           = UART_DATA_BITS_8,
                .parity              = UART_PARITY_OFF,
                .stop_bits           = UART_STOP_BITS_1,
                .p_callback          = lpm_dbg_uart_callback,
                .p_context           = NULL,
                .p_extend            = &g_uart_lpm_dbg_cfg_extend,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_tx       = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_rx       = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
                .rxi_ipl             = (12),
                .txi_ipl             = (12),
                .tei_ipl             = (12),
                .eri_ipl             = (12),
#if defined(VECTOR_NUMBER_SCI4_RXI)
                .rxi_irq             = VECTOR_NUMBER_SCI4_RXI,
#else
                .rxi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI4_TXI)
                .txi_irq             = VECTOR_NUMBER_SCI4_TXI,
#else
                .txi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI4_TEI)
                .tei_irq             = VECTOR_NUMBER_SCI4_TEI,
#else
                .tei_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI4_ERI)
                .eri_irq             = VECTOR_NUMBER_SCI4_ERI,
#else
                .eri_irq             = FSP_INVALID_VECTOR,
#endif
            };

/* Instance structure to use this module. */
const uart_instance_t g_uart_lpm_dbg =
{
    .p_ctrl        = &g_uart_lpm_dbg_ctrl,
    .p_cfg         = &g_uart_lpm_dbg_cfg,
    .p_api         = &g_uart_on_sci
};
lpm_instance_ctrl_t g_lpm_DeepSWStandby_ctrl;

const lpm_cfg_t g_lpm_DeepSWStandby_cfg =
{
    .low_power_mode     = LPM_MODE_DEEP,
    .snooze_cancel_sources      = LPM_SNOOZE_CANCEL_SOURCE_NONE,
    .standby_wake_sources       = LPM_STANDBY_WAKE_SOURCE_IRQ11 | LPM_STANDBY_WAKE_SOURCE_RTCALM |  (lpm_standby_wake_source_t) 0,
    .snooze_request_source      = LPM_SNOOZE_REQUEST_RXD0_FALLING,
    .snooze_end_sources         =  (lpm_snooze_end_t) 0,
    .dtc_state_in_snooze        = LPM_SNOOZE_DTC_DISABLE,
#if BSP_FEATURE_LPM_HAS_SBYCR_OPE
    .output_port_enable         = LPM_OUTPUT_PORT_ENABLE_HIGH_IMPEDANCE,
#endif
#if BSP_FEATURE_LPM_HAS_DEEP_STANDBY
    .io_port_state              = LPM_IO_PORT_NO_CHANGE,
    .power_supply_state         = LPM_POWER_SUPPLY_DEEPCUT3,
    .deep_standby_cancel_source = LPM_DEEP_STANDBY_CANCEL_SOURCE_IRQ11 | LPM_DEEP_STANDBY_CANCEL_SOURCE_RTC_ALARM |  (lpm_deep_standby_cancel_source_t) 0,
    .deep_standby_cancel_edge   = LPM_DEEP_STANDBY_CANCEL_SOURCE_IRQ11_RISING |  (lpm_deep_standby_cancel_edge_t) 0,
#endif
    .p_extend           = NULL,
};

const lpm_instance_t g_lpm_DeepSWStandby =
{
    .p_api = &g_lpm_on_lpm,
    .p_ctrl = &g_lpm_DeepSWStandby_ctrl,
    .p_cfg = &g_lpm_DeepSWStandby_cfg
};
lpm_instance_ctrl_t g_lpm_SW_Standby_ctrl;

const lpm_cfg_t g_lpm_SW_Standby_cfg =
{
    .low_power_mode     = LPM_MODE_STANDBY,
    .snooze_cancel_sources      = LPM_SNOOZE_CANCEL_SOURCE_NONE,
    .standby_wake_sources       = LPM_STANDBY_WAKE_SOURCE_IRQ11 | LPM_STANDBY_WAKE_SOURCE_AGT1UD |  (lpm_standby_wake_source_t) 0,
    .snooze_request_source      = LPM_SNOOZE_REQUEST_RXD0_FALLING,
    .snooze_end_sources         =  (lpm_snooze_end_t) 0,
    .dtc_state_in_snooze        = LPM_SNOOZE_DTC_DISABLE,
#if BSP_FEATURE_LPM_HAS_SBYCR_OPE
    .output_port_enable         = LPM_OUTPUT_PORT_ENABLE_RETAIN,
#endif
#if BSP_FEATURE_LPM_HAS_DEEP_STANDBY
    .io_port_state              = LPM_IO_PORT_NO_CHANGE,
    .power_supply_state         = LPM_POWER_SUPPLY_DEEPCUT0,
    .deep_standby_cancel_source = LPM_DEEP_STANDBY_CANCEL_SOURCE_IRQ11 | LPM_DEEP_STANDBY_CANCEL_SOURCE_AGT1 |  (lpm_deep_standby_cancel_source_t) 0,
    .deep_standby_cancel_edge   = LPM_DEEP_STANDBY_CANCEL_SOURCE_IRQ11_RISING |  (lpm_deep_standby_cancel_edge_t) 0,
#endif
    .p_extend           = NULL,
};

const lpm_instance_t g_lpm_SW_Standby =
{
    .p_api = &g_lpm_on_lpm,
    .p_ctrl = &g_lpm_SW_Standby_ctrl,
    .p_cfg = &g_lpm_SW_Standby_cfg
};
agt_instance_ctrl_t AGT1_LPM_Wakeup_ctrl;/* Note: If this configuration is changed to be non-const then we need to add additional checking in the AGT module */
const agt_extended_cfg_t AGT1_LPM_Wakeup_extend =
{
    .count_source            = AGT_CLOCK_SUBCLOCK,
    .agto                    = AGT_PIN_CFG_DISABLED,
    .agtoab_settings_b.agtoa = AGT_PIN_CFG_DISABLED,
    .agtoab_settings_b.agtob = AGT_PIN_CFG_DISABLED,
    .measurement_mode        = AGT_MEASURE_DISABLED,
    .agtio_filter            = AGT_AGTIO_FILTER_NONE,
    .enable_pin              = AGT_ENABLE_PIN_NOT_USED,
    .trigger_edge            = AGT_TRIGGER_EDGE_RISING,
};
const timer_cfg_t AGT1_LPM_Wakeup_cfg =
{
    .mode                = TIMER_MODE_ONE_SHOT,
    /* Actual period: 10 seconds. Actual duty: 50%. */ .period_counts = (uint32_t) 0xa000, .duty_cycle_counts = 0x5000, .source_div = (timer_source_div_t)3,
    .channel             = 1,
    .p_callback          = agt1_timer_callback,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = &AGT1_LPM_Wakeup_extend,
    .cycle_end_ipl       = (3),
#if defined(VECTOR_NUMBER_AGT1_INT)
    .cycle_end_irq       = VECTOR_NUMBER_AGT1_INT,
#else
    .cycle_end_irq       = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const timer_instance_t AGT1_LPM_Wakeup =
{
    .p_ctrl        = &AGT1_LPM_Wakeup_ctrl,
    .p_cfg         = &AGT1_LPM_Wakeup_cfg,
    .p_api         = &g_timer_on_agt
};
crc_instance_ctrl_t g_crc1_ctrl;
const crc_cfg_t g_crc1_cfg =
{
    .polynomial      = CRC_POLYNOMIAL_CRC_CCITT,
    .bit_order       = CRC_BIT_ORDER_LMS_MSB,
    .snoop_address   = CRC_SNOOP_ADDRESS_NONE,
    .p_extend        = NULL,
};

/* Instance structure to use this module. */
const crc_instance_t g_crc1 =
{
    .p_ctrl        = &g_crc1_ctrl,
    .p_cfg         = &g_crc1_cfg,
    .p_api         = &g_crc_on_crc
};
agt_instance_ctrl_t AGT0_ExtFlashBusy_ctrl;
const agt_extended_cfg_t AGT0_ExtFlashBusy_extend =
{
    .count_source            = AGT_CLOCK_SUBCLOCK,
    .agto                    = AGT_PIN_CFG_DISABLED,
    .agtoab_settings_b.agtoa = AGT_PIN_CFG_DISABLED,
    .agtoab_settings_b.agtob = AGT_PIN_CFG_DISABLED,
    .measurement_mode        = AGT_MEASURE_DISABLED,
    .agtio_filter            = AGT_AGTIO_FILTER_NONE,
    .enable_pin              = AGT_ENABLE_PIN_NOT_USED,
    .trigger_edge            = AGT_TRIGGER_EDGE_RISING,
};
const timer_cfg_t AGT0_ExtFlashBusy_cfg =
{
    .mode                = TIMER_MODE_ONE_SHOT,
    /* Actual period: 0.0009765625 seconds. Actual duty: 50%. */ .period_counts = (uint32_t) 0x20, .duty_cycle_counts = 0x10, .source_div = (timer_source_div_t)0,
    .channel             = 0,
    .p_callback          = AGT0_ExtFlashBusy_Callback,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = &AGT0_ExtFlashBusy_extend,
    .cycle_end_ipl       = (10),
#if defined(VECTOR_NUMBER_AGT0_INT)
    .cycle_end_irq       = VECTOR_NUMBER_AGT0_INT,
#else
    .cycle_end_irq       = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const timer_instance_t AGT0_ExtFlashBusy =
{
    .p_ctrl        = &AGT0_ExtFlashBusy_ctrl,
    .p_cfg         = &AGT0_ExtFlashBusy_cfg,
    .p_api         = &g_timer_on_agt
};
flash_hp_instance_ctrl_t g_flash0_ctrl;
const flash_cfg_t g_flash0_cfg =
{
    .data_flash_bgo      = true,
    .p_callback          = dataBgoCallback,
    .p_context           = NULL,
#if defined(VECTOR_NUMBER_FCU_FRDYI)
    .irq                 = VECTOR_NUMBER_FCU_FRDYI,
#else
    .irq                 = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_FCU_FIFERR)
    .err_irq             = VECTOR_NUMBER_FCU_FIFERR,
#else
    .err_irq             = FSP_INVALID_VECTOR,
#endif
    .err_ipl             = (3),
    .ipl                 = (3),
};
/* Instance structure to use this module. */
const flash_instance_t g_flash0 =
{
    .p_ctrl        = &g_flash0_ctrl,
    .p_cfg         = &g_flash0_cfg,
    .p_api         = &g_flash_on_flash_hp
};
qspi_instance_ctrl_t g_qspi0_ctrl;

static const spi_flash_erase_command_t g_qspi0_erase_command_list[] =
{
#if 4096 > 0
    {.command = 0x20,     .size = 4096 },
#endif
#if 32768 > 0
    {.command = 0x52, .size = 32768 },
#endif
#if 65536 > 0
    {.command = 0xD8,      .size = 65536 },
#endif
#if 0xC7 > 0
    {.command = 0xC7,       .size  = SPI_FLASH_ERASE_SIZE_CHIP_ERASE         },
#endif
};
static const qspi_extended_cfg_t g_qspi0_extended_cfg =
{
    .min_qssl_deselect_cycles = QSPI_QSSL_MIN_HIGH_LEVEL_4_QSPCLK,
    .qspclk_div          = QSPI_QSPCLK_DIV_2,
};
const spi_flash_cfg_t g_qspi0_cfg =
{
    .spi_protocol        = SPI_FLASH_PROTOCOL_EXTENDED_SPI,
    .read_mode           = SPI_FLASH_READ_MODE_STANDARD,
    .address_bytes       = SPI_FLASH_ADDRESS_BYTES_3,
    .dummy_clocks        = SPI_FLASH_DUMMY_CLOCKS_DEFAULT,
    .page_program_address_lines = SPI_FLASH_DATA_LINES_1,
    .page_size_bytes     = 256,
    .page_program_command = 0x02,
    .write_enable_command = 0x06,
    .status_command = 0x05,
    .write_status_bit    = 0,
    .xip_enter_command   = 0x20,
    .xip_exit_command    = 0xFF,
    .p_erase_command_list = &g_qspi0_erase_command_list[0],
    .erase_command_list_length = sizeof(g_qspi0_erase_command_list) / sizeof(g_qspi0_erase_command_list[0]),
    .p_extend            = &g_qspi0_extended_cfg,
};
/** This structure encompasses everything that is needed to use an instance of this interface. */
const spi_flash_instance_t g_qspi0 =
{
    .p_ctrl = &g_qspi0_ctrl,
    .p_cfg =  &g_qspi0_cfg,
    .p_api =  &g_qspi_on_spi_flash,
};
dtc_instance_ctrl_t g_transfer1_ctrl;

transfer_info_t g_transfer1_info =
{
    .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
    .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_DESTINATION,
    .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
    .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
    .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_FIXED,
    .transfer_settings_word_b.size           = TRANSFER_SIZE_2_BYTE,
    .transfer_settings_word_b.mode           = TRANSFER_MODE_NORMAL,
    .p_dest                                  = (void *) NULL,
    .p_src                                   = (void const *) NULL,
    .num_blocks                              = 0,
    .length                                  = 0,
};

const dtc_extended_cfg_t g_transfer1_cfg_extend =
{
    .activation_source   = VECTOR_NUMBER_SPI1_RXI,
};
const transfer_cfg_t g_transfer1_cfg =
{
    .p_info              = &g_transfer1_info,
    .p_extend            = &g_transfer1_cfg_extend,
};

/* Instance structure to use this module. */
const transfer_instance_t g_transfer1 =
{
    .p_ctrl        = &g_transfer1_ctrl,
    .p_cfg         = &g_transfer1_cfg,
    .p_api         = &g_transfer_on_dtc
};
dtc_instance_ctrl_t g_transfer0_ctrl;

transfer_info_t g_transfer0_info =
{
    .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
    .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_SOURCE,
    .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
    .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
    .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_INCREMENTED,
    .transfer_settings_word_b.size           = TRANSFER_SIZE_2_BYTE,
    .transfer_settings_word_b.mode           = TRANSFER_MODE_NORMAL,
    .p_dest                                  = (void *) NULL,
    .p_src                                   = (void const *) NULL,
    .num_blocks                              = 0,
    .length                                  = 0,
};

const dtc_extended_cfg_t g_transfer0_cfg_extend =
{
    .activation_source   = VECTOR_NUMBER_SPI1_TXI,
};
const transfer_cfg_t g_transfer0_cfg =
{
    .p_info              = &g_transfer0_info,
    .p_extend            = &g_transfer0_cfg_extend,
};

/* Instance structure to use this module. */
const transfer_instance_t g_transfer0 =
{
    .p_ctrl        = &g_transfer0_ctrl,
    .p_cfg         = &g_transfer0_cfg,
    .p_api         = &g_transfer_on_dtc
};
#define RA_NOT_DEFINED (UINT32_MAX)
#if (RA_NOT_DEFINED) != (RA_NOT_DEFINED)

/* If the transfer module is DMAC, define a DMAC transfer callback. */
#include "r_dmac.h"
extern void spi_tx_dmac_callback(spi_instance_ctrl_t const * const p_ctrl);

void g_spi1_tx_transfer_callback (dmac_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    spi_tx_dmac_callback(&g_spi1_ctrl);
}
#endif

#if (RA_NOT_DEFINED) != (RA_NOT_DEFINED)

/* If the transfer module is DMAC, define a DMAC transfer callback. */
#include "r_dmac.h"
extern void spi_rx_dmac_callback(spi_instance_ctrl_t const * const p_ctrl);

void g_spi1_rx_transfer_callback (dmac_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    spi_rx_dmac_callback(&g_spi1_ctrl);
}
#endif
#undef RA_NOT_DEFINED

spi_instance_ctrl_t g_spi1_ctrl;

/** SPI extended configuration for SPI HAL driver */
const spi_extended_cfg_t g_spi1_ext_cfg =
{
    .spi_clksyn         = SPI_SSL_MODE_SPI,
    .spi_comm           = SPI_COMMUNICATION_FULL_DUPLEX,
    .ssl_polarity        = SPI_SSLP_LOW,
    .ssl_select          = SPI_SSL_SELECT_SSL0,
    .mosi_idle           = SPI_MOSI_IDLE_VALUE_FIXING_DISABLE,
    .parity              = SPI_PARITY_MODE_DISABLE,
    .byte_swap           = SPI_BYTE_SWAP_DISABLE,
    .spck_div            = {
        /* Actual calculated bitrate: 15000000. */ .spbr = 1, .brdv = 0
    },
    .spck_delay          = SPI_DELAY_COUNT_1,
    .ssl_negation_delay  = SPI_DELAY_COUNT_1,
    .next_access_delay   = SPI_DELAY_COUNT_1
 };

/** SPI configuration for SPI HAL driver */
const spi_cfg_t g_spi1_cfg =
{
    .channel             = 1,

#if defined(VECTOR_NUMBER_SPI1_RXI)
    .rxi_irq             = VECTOR_NUMBER_SPI1_RXI,
#else
    .rxi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SPI1_TXI)
    .txi_irq             = VECTOR_NUMBER_SPI1_TXI,
#else
    .txi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SPI1_TEI)
    .tei_irq             = VECTOR_NUMBER_SPI1_TEI,
#else
    .tei_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SPI1_ERI)
    .eri_irq             = VECTOR_NUMBER_SPI1_ERI,
#else
    .eri_irq             = FSP_INVALID_VECTOR,
#endif

    .rxi_ipl             = (12),
    .txi_ipl             = (12),
    .tei_ipl             = (12),
    .eri_ipl             = (12),

    .operating_mode      = SPI_MODE_MASTER,

    .clk_phase           = SPI_CLK_PHASE_EDGE_ODD,
    .clk_polarity        = SPI_CLK_POLARITY_LOW,

    .mode_fault          = SPI_MODE_FAULT_ERROR_DISABLE,
    .bit_order           = SPI_BIT_ORDER_MSB_FIRST,
    .p_transfer_tx       = g_spi1_P_TRANSFER_TX,
    .p_transfer_rx       = g_spi1_P_TRANSFER_RX,
    .p_callback          = spi_callback,

    .p_context           = NULL,
    .p_extend            = (void *)&g_spi1_ext_cfg,
};

/* Instance structure to use this module. */
const spi_instance_t g_spi1 =
{
    .p_ctrl        = &g_spi1_ctrl,
    .p_cfg         = &g_spi1_cfg,
    .p_api         = &g_spi_on_spi
};
gpt_instance_ctrl_t GPT1_Radio0_ICapture_ctrl;
#if 0
const gpt_extended_pwm_cfg_t GPT1_Radio0_ICapture_pwm_extend =
{
    .trough_ipl          = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT1_COUNTER_UNDERFLOW)
    .trough_irq          = VECTOR_NUMBER_GPT1_COUNTER_UNDERFLOW,
#else
    .trough_irq          = FSP_INVALID_VECTOR,
#endif
    .poeg_link           = GPT_POEG_LINK_POEG0,
    .output_disable      = (gpt_output_disable_t) ( GPT_OUTPUT_DISABLE_NONE),
    .adc_trigger         = (gpt_adc_trigger_t) ( GPT_ADC_TRIGGER_NONE),
    .dead_time_count_up  = 0,
    .dead_time_count_down = 0,
    .adc_a_compare_match = 0,
    .adc_b_compare_match = 0,
    .interrupt_skip_source = GPT_INTERRUPT_SKIP_SOURCE_NONE,
    .interrupt_skip_count  = GPT_INTERRUPT_SKIP_COUNT_0,
    .interrupt_skip_adc    = GPT_INTERRUPT_SKIP_ADC_NONE,
    .gtioca_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
    .gtiocb_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
};
#endif
const gpt_extended_cfg_t GPT1_Radio0_ICapture_extend =
{
    .gtioca = { .output_enabled = false,
                .stop_level     = GPT_PIN_LEVEL_LOW
              },
    .gtiocb = { .output_enabled = false,
                .stop_level     = GPT_PIN_LEVEL_LOW
              },
    .start_source        = (gpt_source_t) ( GPT_SOURCE_NONE),
    .stop_source         = (gpt_source_t) ( GPT_SOURCE_NONE),
    .clear_source        = (gpt_source_t) ( GPT_SOURCE_NONE),
    .count_up_source     = (gpt_source_t) ( GPT_SOURCE_NONE),
    .count_down_source   = (gpt_source_t) ( GPT_SOURCE_NONE),
    .capture_a_source    = (gpt_source_t) (GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_LOW |  GPT_SOURCE_NONE),
    .capture_b_source    = (gpt_source_t) ( GPT_SOURCE_NONE),
    .capture_a_ipl       = (10),
    .capture_b_ipl       = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT1_CAPTURE_COMPARE_A)
    .capture_a_irq       = VECTOR_NUMBER_GPT1_CAPTURE_COMPARE_A,
#else
    .capture_a_irq       = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT1_CAPTURE_COMPARE_B)
    .capture_b_irq       = VECTOR_NUMBER_GPT1_CAPTURE_COMPARE_B,
#else
    .capture_b_irq       = FSP_INVALID_VECTOR,
#endif
    .capture_filter_gtioca       = GPT_CAPTURE_FILTER_NONE,
    .capture_filter_gtiocb       = GPT_CAPTURE_FILTER_NONE,
#if 0
    .p_pwm_cfg                   = &GPT1_Radio0_ICapture_pwm_extend,
#else
    .p_pwm_cfg                   = NULL,
#endif
#if 0
    .gtior_setting.gtior_b.gtioa  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.oadflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.oahld  = 0U,
    .gtior_setting.gtior_b.oae    = (uint32_t) false,
    .gtior_setting.gtior_b.oadf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfaen  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsa  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
    .gtior_setting.gtior_b.gtiob  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.obdflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.obhld  = 0U,
    .gtior_setting.gtior_b.obe    = (uint32_t) false,
    .gtior_setting.gtior_b.obdf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfben  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsb  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
#else
    .gtior_setting.gtior = 0U,
#endif
};
const timer_cfg_t GPT1_Radio0_ICapture_cfg =
{
    .mode                = TIMER_MODE_PERIODIC,
    /* Actual period: 71.58278826666667 seconds. Actual duty: 50%. */ .period_counts = (uint32_t) 0x100000000, .duty_cycle_counts = 0x80000000, .source_div = (timer_source_div_t)0,
    .channel             = 1,
    .p_callback          = Radio0_IC_ISR,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = &GPT1_Radio0_ICapture_extend,
    .cycle_end_ipl       = (10),
#if defined(VECTOR_NUMBER_GPT1_COUNTER_OVERFLOW)
    .cycle_end_irq       = VECTOR_NUMBER_GPT1_COUNTER_OVERFLOW,
#else
    .cycle_end_irq       = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const timer_instance_t GPT1_Radio0_ICapture =
{
    .p_ctrl        = &GPT1_Radio0_ICapture_ctrl,
    .p_cfg         = &GPT1_Radio0_ICapture_cfg,
    .p_api         = &g_timer_on_gpt
};
rtc_instance_ctrl_t g_rtc0_ctrl;
const rtc_error_adjustment_cfg_t g_rtc0_err_cfg =
{
    .adjustment_mode         = RTC_ERROR_ADJUSTMENT_MODE_MANUAL,
    .adjustment_period       = RTC_ERROR_ADJUSTMENT_PERIOD_10_SECOND,
    .adjustment_type         = RTC_ERROR_ADJUSTMENT_NONE,
    .adjustment_value        = 0,
};
const rtc_cfg_t g_rtc0_cfg =
{
    .clock_source            = RTC_CLOCK_SOURCE_SUBCLK,
    .freq_compare_value_loco = 255,
    .p_err_cfg               = &g_rtc0_err_cfg,
    .p_callback              = RTC_Callback,
    .p_context               = NULL,
    .alarm_ipl               = (12),
    .periodic_ipl            = (BSP_IRQ_DISABLED),
    .carry_ipl               = (BSP_IRQ_DISABLED),  /*Aclara change from 12 */
#if defined(VECTOR_NUMBER_RTC_ALARM)
    .alarm_irq               = VECTOR_NUMBER_RTC_ALARM,
#else
    .alarm_irq               = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_RTC_PERIOD)
    .periodic_irq            = VECTOR_NUMBER_RTC_PERIOD,
#else
    .periodic_irq            = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_RTC_CARRY)
    .carry_irq               = VECTOR_NUMBER_RTC_CARRY,
#else
    .carry_irq               = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const rtc_instance_t g_rtc0 =
{
    .p_ctrl        = &g_rtc0_ctrl,
    .p_cfg         = &g_rtc0_cfg,
    .p_api         = &g_rtc_on_rtc
};
adc_instance_ctrl_t g_adc0_ctrl;
const adc_extended_cfg_t g_adc0_cfg_extend =
{
    .add_average_count   = ADC_ADD_AVERAGE_FOUR,
    .clearing            = ADC_CLEAR_AFTER_READ_ON,
    .trigger_group_b     = ADC_TRIGGER_SYNC_ELC,
    .double_trigger_mode = ADC_DOUBLE_TRIGGER_DISABLED,
    .adc_vref_control    = ADC_VREF_CONTROL_VREFH,
    .enable_adbuf        = 0,
#if defined(VECTOR_NUMBER_ADC0_WINDOW_A)
    .window_a_irq        = VECTOR_NUMBER_ADC0_WINDOW_A,
#else
    .window_a_irq        = FSP_INVALID_VECTOR,
#endif
    .window_a_ipl        = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_ADC0_WINDOW_B)
    .window_b_irq      = VECTOR_NUMBER_ADC0_WINDOW_B,
#else
    .window_b_irq      = FSP_INVALID_VECTOR,
#endif
    .window_b_ipl      = (BSP_IRQ_DISABLED),
};
const adc_cfg_t g_adc0_cfg =
{
    .unit                = 0,
    .mode                = ADC_MODE_SINGLE_SCAN,
    .resolution          = ADC_RESOLUTION_12_BIT,
    .alignment           = (adc_alignment_t) ADC_ALIGNMENT_RIGHT,
    .trigger             = ADC_TRIGGER_SOFTWARE,
    .p_callback          = NULL,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = &g_adc0_cfg_extend,
#if defined(VECTOR_NUMBER_ADC0_SCAN_END)
    .scan_end_irq        = VECTOR_NUMBER_ADC0_SCAN_END,
#else
    .scan_end_irq        = FSP_INVALID_VECTOR,
#endif
    .scan_end_ipl        = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_ADC0_SCAN_END_B)
    .scan_end_b_irq      = VECTOR_NUMBER_ADC0_SCAN_END_B,
#else
    .scan_end_b_irq      = FSP_INVALID_VECTOR,
#endif
    .scan_end_b_ipl      = (BSP_IRQ_DISABLED),
};
#if ((0) | (0))
const adc_window_cfg_t g_adc0_window_cfg =
{
    .compare_mask        =  0,
    .compare_mode_mask   =  0,
    .compare_cfg         = (0) | (0) | (0) | (ADC_COMPARE_CFG_EVENT_OUTPUT_OR),
    .compare_ref_low     = 0,
    .compare_ref_high    = 0,
    .compare_b_channel   = (ADC_WINDOW_B_CHANNEL_0),
    .compare_b_mode      = (ADC_WINDOW_B_MODE_LESS_THAN_OR_OUTSIDE),
    .compare_b_ref_low   = 0,
    .compare_b_ref_high  = 0,
};
#endif
const adc_channel_cfg_t g_adc0_channel_cfg =
{
    .scan_mask           = ADC_MASK_CHANNEL_5 | ADC_MASK_CHANNEL_7 | ADC_MASK_CHANNEL_8 |  0,
    .scan_mask_group_b   =  0,
    .priority_group_a    = ADC_GROUP_A_PRIORITY_OFF,
    .add_mask            = ADC_MASK_CHANNEL_5 | ADC_MASK_CHANNEL_7 | ADC_MASK_CHANNEL_8 |  0,
    .sample_hold_mask    =  0,
    .sample_hold_states  = 24,
#if ((0) | (0))
    .p_window_cfg        = (adc_window_cfg_t *) &g_adc0_window_cfg,
#else
    .p_window_cfg        = NULL,
#endif
};
/* Instance structure to use this module. */
const adc_instance_t g_adc0 =
{
    .p_ctrl    = &g_adc0_ctrl,
    .p_cfg     = &g_adc0_cfg,
    .p_channel_cfg = &g_adc0_channel_cfg,
    .p_api     = &g_adc_on_adc
};
sci_uart_instance_ctrl_t     g_uart_optical_ctrl;

            baud_setting_t               g_uart_optical_baud_setting =
            {
                /* Baud rate calculated with 0.160% error. */ .semr_baudrate_bits_b.abcse = 0, .semr_baudrate_bits_b.abcs = 0, .semr_baudrate_bits_b.bgdm = 0, .cks = 0, .brr = 194, .mddr = (uint8_t) 256, .semr_baudrate_bits_b.brme = false
            };

            /** UART extended configuration for UARTonSCI HAL driver */
            const sci_uart_extended_cfg_t g_uart_optical_cfg_extend =
            {
                .clock                = SCI_UART_CLOCK_INT,
                .rx_edge_start          = SCI_UART_START_BIT_FALLING_EDGE,
                .noise_cancel         = SCI_UART_NOISE_CANCELLATION_DISABLE,
                .rx_fifo_trigger        = SCI_UART_RX_FIFO_TRIGGER_1,
                .p_baud_setting         = &g_uart_optical_baud_setting,
                .flow_control           = SCI_UART_FLOW_CONTROL_RTS,
                #if 0xFF != 0xFF
                .flow_control_pin       = BSP_IO_PORT_FF_PIN_0xFF,
                #else
                .flow_control_pin       = (bsp_io_port_pin_t) UINT16_MAX,
                #endif
                .rs485_setting = {
                    .enable = SCI_UART_RS485_DISABLE,
                    .polarity = SCI_UART_RS485_DE_POLARITY_HIGH,
                #if 0xFF != 0xFF
                    .de_control_pin = BSP_IO_PORT_FF_PIN_0xFF,
                #else
                    .de_control_pin       = (bsp_io_port_pin_t) UINT16_MAX,
                #endif
                },
            };

            /** UART interface configuration */
            const uart_cfg_t g_uart_optical_cfg =
            {
                .channel             = 9,
                .data_bits           = UART_DATA_BITS_8,
                .parity              = UART_PARITY_OFF,
                .stop_bits           = UART_STOP_BITS_1,
                .p_callback          = optical_uart_callback,
                .p_context           = NULL,
                .p_extend            = &g_uart_optical_cfg_extend,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_tx       = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_rx       = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
                .rxi_ipl             = (12),
                .txi_ipl             = (12),
                .tei_ipl             = (12),
                .eri_ipl             = (12),
#if defined(VECTOR_NUMBER_SCI9_RXI)
                .rxi_irq             = VECTOR_NUMBER_SCI9_RXI,
#else
                .rxi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI9_TXI)
                .txi_irq             = VECTOR_NUMBER_SCI9_TXI,
#else
                .txi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI9_TEI)
                .tei_irq             = VECTOR_NUMBER_SCI9_TEI,
#else
                .tei_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI9_ERI)
                .eri_irq             = VECTOR_NUMBER_SCI9_ERI,
#else
                .eri_irq             = FSP_INVALID_VECTOR,
#endif
            };

/* Instance structure to use this module. */
const uart_instance_t g_uart_optical =
{
    .p_ctrl        = &g_uart_optical_ctrl,
    .p_cfg         = &g_uart_optical_cfg,
    .p_api         = &g_uart_on_sci
};
sci_uart_instance_ctrl_t     g_uart_DBG_ctrl;

            baud_setting_t               g_uart_DBG_baud_setting =
            {
/* Aclara Modified: configure UART based on BAUD RATE selection */
#if ( DEBUG_BAUD_RATE == 115200 )
                /* Baud rate 115200 calculated with 1.725% error. */ .semr_baudrate_bits_b.abcse = 0, .semr_baudrate_bits_b.abcs = 0, .semr_baudrate_bits_b.bgdm = 1, .cks = 0, .brr = 31, .mddr = (uint8_t) 256, .semr_baudrate_bits_b.brme = false
#elif ( DEBUG_BAUD_RATE == 38400 )
                /* Baud rate  38400 calculated with 0.677% error. */ .semr_baudrate_bits_b.abcse = 0, .semr_baudrate_bits_b.abcs = 0, .semr_baudrate_bits_b.bgdm = 1, .cks = 0, .brr = 96, .mddr = (uint8_t) 256, .semr_baudrate_bits_b.brme = false
#else
                #error "Unsupported baud rate configured for the debug port!"
#endif
/* Aclara Modified - End */
            };

            /** UART extended configuration for UARTonSCI HAL driver */
            const sci_uart_extended_cfg_t g_uart_DBG_cfg_extend =
            {
                .clock                = SCI_UART_CLOCK_INT,
                .rx_edge_start          = SCI_UART_START_BIT_FALLING_EDGE,
                .noise_cancel         = SCI_UART_NOISE_CANCELLATION_DISABLE,
                .rx_fifo_trigger        = SCI_UART_RX_FIFO_TRIGGER_1,
                .p_baud_setting         = &g_uart_DBG_baud_setting,
                .flow_control           = SCI_UART_FLOW_CONTROL_RTS,
                #if 0xFF != 0xFF
                .flow_control_pin       = BSP_IO_PORT_FF_PIN_0xFF,
                #else
                .flow_control_pin       = (bsp_io_port_pin_t) UINT16_MAX,
                #endif
                .rs485_setting = {
                    .enable = SCI_UART_RS485_DISABLE,
                    .polarity = SCI_UART_RS485_DE_POLARITY_HIGH,
                #if 0xFF != 0xFF
                    .de_control_pin = BSP_IO_PORT_FF_PIN_0xFF,
                #else
                    .de_control_pin       = (bsp_io_port_pin_t) UINT16_MAX,
                #endif
                },
            };

            /** UART interface configuration */
            const uart_cfg_t g_uart_DBG_cfg =
            {
                .channel             = 4,
                .data_bits           = UART_DATA_BITS_8,
                .parity              = UART_PARITY_OFF,
                .stop_bits           = UART_STOP_BITS_1,
                .p_callback          = dbg_uart_callback,
                .p_context           = NULL,
                .p_extend            = &g_uart_DBG_cfg_extend,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_tx       = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_rx       = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
                .rxi_ipl             = (12),
                .txi_ipl             = (12),
                .tei_ipl             = (12),
                .eri_ipl             = (12),
#if defined(VECTOR_NUMBER_SCI4_RXI)
                .rxi_irq             = VECTOR_NUMBER_SCI4_RXI,
#else
                .rxi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI4_TXI)
                .txi_irq             = VECTOR_NUMBER_SCI4_TXI,
#else
                .txi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI4_TEI)
                .tei_irq             = VECTOR_NUMBER_SCI4_TEI,
#else
                .tei_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI4_ERI)
                .eri_irq             = VECTOR_NUMBER_SCI4_ERI,
#else
                .eri_irq             = FSP_INVALID_VECTOR,
#endif
            };

/* Instance structure to use this module. */
const uart_instance_t g_uart_DBG =
{
    .p_ctrl        = &g_uart_DBG_ctrl,
    .p_cfg         = &g_uart_DBG_cfg,
    .p_api         = &g_uart_on_sci
};
sci_uart_instance_ctrl_t     g_uart_MFG_ctrl;

            baud_setting_t               g_uart_MFG_baud_setting =
            {
                /* Baud rate calculated with 0.677% error. */ .semr_baudrate_bits_b.abcse = 0, .semr_baudrate_bits_b.abcs = 0, .semr_baudrate_bits_b.bgdm = 1, .cks = 0, .brr = 96, .mddr = (uint8_t) 256, .semr_baudrate_bits_b.brme = false
            };

            /** UART extended configuration for UARTonSCI HAL driver */
            const sci_uart_extended_cfg_t g_uart_MFG_cfg_extend =
            {
                .clock                = SCI_UART_CLOCK_INT,
                .rx_edge_start          = SCI_UART_START_BIT_FALLING_EDGE,
                .noise_cancel         = SCI_UART_NOISE_CANCELLATION_DISABLE,
                .rx_fifo_trigger        = SCI_UART_RX_FIFO_TRIGGER_1,
                .p_baud_setting         = &g_uart_MFG_baud_setting,
                .flow_control           = SCI_UART_FLOW_CONTROL_RTS,
                #if 0xFF != 0xFF
                .flow_control_pin       = BSP_IO_PORT_FF_PIN_0xFF,
                #else
                .flow_control_pin       = (bsp_io_port_pin_t) UINT16_MAX,
                #endif
                .rs485_setting = {
                    .enable = SCI_UART_RS485_DISABLE,
                    .polarity = SCI_UART_RS485_DE_POLARITY_HIGH,
                #if 0xFF != 0xFF
                    .de_control_pin = BSP_IO_PORT_FF_PIN_0xFF,
                #else
                    .de_control_pin       = (bsp_io_port_pin_t) UINT16_MAX,
                #endif
                },
            };

            /** UART interface configuration */
            const uart_cfg_t g_uart_MFG_cfg =
            {
                .channel             = 3,
                .data_bits           = UART_DATA_BITS_8,
                .parity              = UART_PARITY_OFF,
                .stop_bits           = UART_STOP_BITS_1,
                .p_callback          = mfg_uart_callback,
                .p_context           = NULL,
                .p_extend            = &g_uart_MFG_cfg_extend,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_tx       = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_rx       = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
                .rxi_ipl             = (12),
                .txi_ipl             = (12),
                .tei_ipl             = (12),
                .eri_ipl             = (12),
#if defined(VECTOR_NUMBER_SCI3_RXI)
                .rxi_irq             = VECTOR_NUMBER_SCI3_RXI,
#else
                .rxi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI3_TXI)
                .txi_irq             = VECTOR_NUMBER_SCI3_TXI,
#else
                .txi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI3_TEI)
                .tei_irq             = VECTOR_NUMBER_SCI3_TEI,
#else
                .tei_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI3_ERI)
                .eri_irq             = VECTOR_NUMBER_SCI3_ERI,
#else
                .eri_irq             = FSP_INVALID_VECTOR,
#endif
            };

/* Instance structure to use this module. */
const uart_instance_t g_uart_MFG =
{
    .p_ctrl        = &g_uart_MFG_ctrl,
    .p_cfg         = &g_uart_MFG_cfg,
    .p_api         = &g_uart_on_sci
};
iic_master_instance_ctrl_t g_i2c_master0_ctrl;
const iic_master_extended_cfg_t g_i2c_master0_extend =
{
    .timeout_mode             = IIC_MASTER_TIMEOUT_MODE_SHORT,
    .timeout_scl_low          = IIC_MASTER_TIMEOUT_SCL_LOW_DISABLED,
    /* Actual calculated bitrate: 993377. Actual calculated duty cycle: 52%. */ .clock_settings.brl_value = 7, .clock_settings.brh_value = 8, .clock_settings.cks_value = 0,
};
const i2c_master_cfg_t g_i2c_master0_cfg =
{
    .channel             = 0,
    .rate                = I2C_MASTER_RATE_FASTPLUS,
    .slave               = 0x60,
    .addr_mode           = I2C_MASTER_ADDR_MODE_7BIT,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_tx       = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_rx       = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
    .p_callback          = iic_eventCallback,
    .p_context           = NULL,
#if defined(VECTOR_NUMBER_IIC0_RXI)
    .rxi_irq             = VECTOR_NUMBER_IIC0_RXI,
#else
    .rxi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC0_TXI)
    .txi_irq             = VECTOR_NUMBER_IIC0_TXI,
#else
    .txi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC0_TEI)
    .tei_irq             = VECTOR_NUMBER_IIC0_TEI,
#else
    .tei_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC0_ERI)
    .eri_irq             = VECTOR_NUMBER_IIC0_ERI,
#else
    .eri_irq             = FSP_INVALID_VECTOR,
#endif
    .ipl                 = (12),
    .p_extend            = &g_i2c_master0_extend,
};
/* Instance structure to use this module. */
const i2c_master_instance_t g_i2c_master0 =
{
    .p_ctrl        = &g_i2c_master0_ctrl,
    .p_cfg         = &g_i2c_master0_cfg,
    .p_api         = &g_i2c_master_on_iic
};
crc_instance_ctrl_t g_crc0_ctrl;
const crc_cfg_t g_crc0_cfg =
{
    .polynomial      = CRC_POLYNOMIAL_CRC_32C,
    .bit_order       = CRC_BIT_ORDER_LMS_MSB,
    .snoop_address   = CRC_SNOOP_ADDRESS_NONE,
    .p_extend        = NULL,
};

/* Instance structure to use this module. */
const crc_instance_t g_crc0 =
{
    .p_ctrl        = &g_crc0_ctrl,
    .p_cfg         = &g_crc0_cfg,
    .p_api         = &g_crc_on_crc
};
sci_uart_instance_ctrl_t     g_uart_HMC_ctrl;

            baud_setting_t               g_uart_HMC_baud_setting =
            {
                /* Baud rate calculated with 0.160% error. */ .semr_baudrate_bits_b.abcse = 0, .semr_baudrate_bits_b.abcs = 0, .semr_baudrate_bits_b.bgdm = 0, .cks = 0, .brr = 194, .mddr = (uint8_t) 256, .semr_baudrate_bits_b.brme = false
            };

            /** UART extended configuration for UARTonSCI HAL driver */
            const sci_uart_extended_cfg_t g_uart_HMC_cfg_extend =
            {
                .clock                = SCI_UART_CLOCK_INT,
                .rx_edge_start          = SCI_UART_START_BIT_FALLING_EDGE,
                .noise_cancel         = SCI_UART_NOISE_CANCELLATION_DISABLE,
                .rx_fifo_trigger        = SCI_UART_RX_FIFO_TRIGGER_1,
                .p_baud_setting         = &g_uart_HMC_baud_setting,
                .flow_control           = SCI_UART_FLOW_CONTROL_RTS,
                #if 0xFF != 0xFF
                .flow_control_pin       = BSP_IO_PORT_FF_PIN_0xFF,
                #else
                .flow_control_pin       = (bsp_io_port_pin_t) UINT16_MAX,
                #endif
                .rs485_setting = {
                    .enable = SCI_UART_RS485_DISABLE,
                    .polarity = SCI_UART_RS485_DE_POLARITY_HIGH,
                #if 0xFF != 0xFF
                    .de_control_pin = BSP_IO_PORT_FF_PIN_0xFF,
                #else
                    .de_control_pin       = (bsp_io_port_pin_t) UINT16_MAX,
                #endif
                },
            };

            /** UART interface configuration */
            const uart_cfg_t g_uart_HMC_cfg =
            {
                .channel             = 2,
                .data_bits           = UART_DATA_BITS_8,
                .parity              = UART_PARITY_OFF,
                .stop_bits           = UART_STOP_BITS_1,
                .p_callback          = hmc_uart_callback,
                .p_context           = NULL,
                .p_extend            = &g_uart_HMC_cfg_extend,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_tx       = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
                .p_transfer_rx       = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
                .rxi_ipl             = (12),
                .txi_ipl             = (12),
                .tei_ipl             = (12),
                .eri_ipl             = (12),
#if defined(VECTOR_NUMBER_SCI2_RXI)
                .rxi_irq             = VECTOR_NUMBER_SCI2_RXI,
#else
                .rxi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI2_TXI)
                .txi_irq             = VECTOR_NUMBER_SCI2_TXI,
#else
                .txi_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI2_TEI)
                .tei_irq             = VECTOR_NUMBER_SCI2_TEI,
#else
                .tei_irq             = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI2_ERI)
                .eri_irq             = VECTOR_NUMBER_SCI2_ERI,
#else
                .eri_irq             = FSP_INVALID_VECTOR,
#endif
            };

/* Instance structure to use this module. */
const uart_instance_t g_uart_HMC =
{
    .p_ctrl        = &g_uart_HMC_ctrl,
    .p_cfg         = &g_uart_HMC_cfg,
    .p_api         = &g_uart_on_sci
};
void g_hal_init(void) {
g_common_init();
}
