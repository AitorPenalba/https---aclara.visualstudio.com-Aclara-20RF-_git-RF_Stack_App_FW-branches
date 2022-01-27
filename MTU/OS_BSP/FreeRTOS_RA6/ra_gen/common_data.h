/* generated common header file - do not edit */
#ifndef COMMON_DATA_H_
#define COMMON_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
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
/* IOPORT Instance */
extern const ioport_instance_t g_ioport;

/* IOPORT control structure. */
extern ioport_instance_ctrl_t g_ioport_ctrl;
extern SemaphoreHandle_t g_new_counting_semaphore0;
extern MessageBufferHandle_t g_new_message_buffer0;
extern SemaphoreHandle_t g_new_mutex0;
extern QueueHandle_t g_new_queue0;
extern TimerHandle_t g_new_timer0;
                void g_new_timer0_callback( TimerHandle_t xTimer );
void g_common_init(void);
FSP_FOOTER
#endif /* COMMON_DATA_H_ */
