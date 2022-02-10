/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_icu.h"
#include "r_external_irq_api.h"
#include "r_iic_master.h"
#include "r_i2c_master_api.h"
#include "r_crc.h"
#include "r_crc_api.h"
#include "r_sci_uart.h"
            #include "r_uart_api.h"
FSP_HEADER
/** External IRQ on ICU Instance. */
extern const external_irq_instance_t pf_meter;

/** Access the ICU instance using these structures when calling API functions directly (::p_api is not used). */
extern icu_instance_ctrl_t pf_meter_ctrl;
extern const external_irq_cfg_t pf_meter_cfg;

#ifndef isr_brownOut
extern void isr_brownOut(external_irq_callback_args_t * p_args);
#endif
/* I2C Master on IIC Instance. */
extern const i2c_master_instance_t g_i2c_master0;

/** Access the I2C Master instance using these structures when calling API functions directly (::p_api is not used). */
extern iic_master_instance_ctrl_t g_i2c_master0_ctrl;
extern const i2c_master_cfg_t g_i2c_master0_cfg;

#ifndef NULL
void NULL(i2c_master_callback_args_t * p_args);
#endif
extern const crc_instance_t g_crc0;
extern crc_instance_ctrl_t g_crc0_ctrl;
extern const crc_cfg_t g_crc0_cfg;
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart0;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_uart_instance_ctrl_t     g_uart0_ctrl;
            extern const uart_cfg_t g_uart0_cfg;
            extern const sci_uart_extended_cfg_t g_uart0_cfg_extend;

            #ifndef user_uart_callback
            void user_uart_callback(uart_callback_args_t * p_args);
            #endif
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
