/* generated common header file - do not edit */
#ifndef COMMON_DATA_H_
#define COMMON_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "r_cgc.h"
#include "r_cgc_api.h"
#include "r_elc.h"
#include "r_elc_api.h"
#include "FreeRTOS.h"
                #include "semphr.h"
#include "FreeRTOS.h"
                #include "message_buffer.h"
#include "FreeRTOS.h"
                #include "semphr.h"
#include "FreeRTOS.h"
                #include "queue.h"
#include "FreeRTOS.h"
                #include "timers.h"
#include "r_ioport.h"
#include "bsp_pin_cfg.h"
FSP_HEADER
/** CGC Instance */
extern const cgc_instance_t g_cgc0;

/** Access the CGC instance using these structures when calling API functions directly (::p_api is not used). */
extern cgc_instance_ctrl_t g_cgc0_ctrl;
extern const cgc_cfg_t g_cgc0_cfg;

#ifndef NULL
void NULL(cgc_callback_args_t * p_args);
#endif
/** ELC Instance */
extern const elc_instance_t g_elc;

/** Access the ELC instance using these structures when calling API functions directly (::p_api is not used). */
extern elc_instance_ctrl_t g_elc_ctrl;
extern const elc_cfg_t g_elc_cfg;
/* IOPORT Instance */
extern const ioport_instance_t g_ioport;

/* IOPORT control structure. */
extern ioport_instance_ctrl_t g_ioport_ctrl;
void g_common_init(void);
FSP_FOOTER
#endif /* COMMON_DATA_H_ */
