/* generated common header file - do not edit */
#ifndef COMMON_DATA_H_
#define COMMON_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "r_elc.h"
#include "r_elc_api.h"
#include "r_icu.h"
#include "r_external_irq_api.h"
#include "r_cgc.h"
#include "r_cgc_api.h"
/* Aclara added: include FreeRTOS and associated includes */
#include "FreeRTOS.h"
#include "semphr.h"
#include "message_buffer.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"
/* Aclara added - End */
#include "r_ioport.h"
#include "bsp_pin_cfg.h"
FSP_HEADER
/** ELC Instance */
extern const elc_instance_t g_elc;

/** Access the ELC instance using these structures when calling API functions directly (::p_api is not used). */
extern elc_instance_ctrl_t g_elc_ctrl;
extern const elc_cfg_t g_elc_cfg;
/** External IRQ on ICU Instance. */
extern const external_irq_instance_t miso_busy;

/** Access the ICU instance using these structures when calling API functions directly (::p_api is not used). */
extern icu_instance_ctrl_t miso_busy_ctrl;
extern const external_irq_cfg_t miso_busy_cfg;

#ifndef isr_busy
void isr_busy(external_irq_callback_args_t * p_args);
#endif
/** External IRQ on ICU Instance. */
extern const external_irq_instance_t hmc_trouble_busy;

/** Access the ICU instance using these structures when calling API functions directly (::p_api is not used). */
extern icu_instance_ctrl_t hmc_trouble_busy_ctrl;
extern const external_irq_cfg_t hmc_trouble_busy_cfg;

#ifndef meter_trouble_isr_busy
void meter_trouble_isr_busy(external_irq_callback_args_t * p_args);
#endif
/** CGC Instance */
extern const cgc_instance_t g_cgc0;

/** Access the CGC instance using these structures when calling API functions directly (::p_api is not used). */
extern cgc_instance_ctrl_t g_cgc0_ctrl;
extern const cgc_cfg_t g_cgc0_cfg;

#ifndef NULL
void NULL(cgc_callback_args_t * p_args);
#endif
/** External IRQ on ICU Instance. */
extern const external_irq_instance_t g_external_irq0;

/** Access the ICU instance using these structures when calling API functions directly (::p_api is not used). */
extern icu_instance_ctrl_t g_external_irq0_ctrl;
extern const external_irq_cfg_t g_external_irq0_cfg;

#ifndef Radio0_IRQ_ISR
void Radio0_IRQ_ISR(external_irq_callback_args_t * p_args);
#endif
/** External IRQ on ICU Instance. */
extern const external_irq_instance_t pf_meter;

/** Access the ICU instance using these structures when calling API functions directly (::p_api is not used). */
extern icu_instance_ctrl_t pf_meter_ctrl;
extern const external_irq_cfg_t pf_meter_cfg;

#ifndef isr_brownOut
void isr_brownOut(external_irq_callback_args_t * p_args);
#endif
/* IOPORT Instance */
extern const ioport_instance_t g_ioport;

/* IOPORT control structure. */
extern ioport_instance_ctrl_t g_ioport_ctrl;
void g_common_init(void);
FSP_FOOTER
#endif /* COMMON_DATA_H_ */
