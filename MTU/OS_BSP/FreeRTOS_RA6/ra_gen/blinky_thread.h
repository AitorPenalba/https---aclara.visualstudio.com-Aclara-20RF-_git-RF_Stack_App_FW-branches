/* generated thread header file - do not edit */
#ifndef BLINKY_THREAD_H_
#define BLINKY_THREAD_H_
#include "bsp_api.h"
                #include "FreeRTOS.h"
                #include "task.h"
                #include "semphr.h"
                #include "hal_data.h"
                #ifdef __cplusplus
                extern "C" void blinky_thread_entry(void * pvParameters);
                #else
                extern void blinky_thread_entry(void * pvParameters);
                #endif
#include "r_icu.h"
#include "r_external_irq_api.h"
FSP_HEADER

/** External IRQ on ICU Instance. */
extern const external_irq_instance_t g_sw1_irq1;

/** Access the ICU instance using these structures when calling API functions directly (::p_api is not used). */
extern icu_instance_ctrl_t g_sw1_irq1_ctrl;
extern const external_irq_cfg_t g_sw1_irq1_cfg;

#ifndef sw1_irq_Handler
void sw1_irq_Handler(external_irq_callback_args_t * p_args);
#endif
FSP_FOOTER
#endif /* BLINKY_THREAD_H_ */
