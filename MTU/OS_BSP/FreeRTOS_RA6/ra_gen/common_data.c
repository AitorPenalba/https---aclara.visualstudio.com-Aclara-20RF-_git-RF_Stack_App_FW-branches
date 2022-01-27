/* generated common source file - do not edit */
#include "common_data.h"
ioport_instance_ctrl_t g_ioport_ctrl;
const ioport_instance_t g_ioport =
        {
            .p_api = &g_ioport_on_ioport,
            .p_ctrl = &g_ioport_ctrl,
            .p_cfg = &g_bsp_pin_cfg,
        };
SemaphoreHandle_t g_new_counting_semaphore0;
                #if 1
                StaticSemaphore_t g_new_counting_semaphore0_memory;
                #endif
                void rtos_startup_err_callback(void * p_instance, void * p_data);
MessageBufferHandle_t g_new_message_buffer0;
                #if 1
                StaticMessageBuffer_t g_new_message_buffer0_memory;
                uint8_t g_new_message_buffer0_buffer_memory[512 + 1];
                #endif
                void rtos_startup_err_callback(void * p_instance, void * p_data);
SemaphoreHandle_t g_new_mutex0;
                #if 1
                StaticSemaphore_t g_new_mutex0_memory;
                #endif
                void rtos_startup_err_callback(void * p_instance, void * p_data);
QueueHandle_t g_new_queue0;
                #if 1
                StaticQueue_t g_new_queue0_memory;
                uint8_t g_new_queue0_queue_memory[4 * 20];
                #endif
                void rtos_startup_err_callback(void * p_instance, void * p_data);
TimerHandle_t g_new_timer0;
                #if 1
                StaticTimer_t  g_new_timer0_memory;
                #endif
                void rtos_startup_err_callback(void * p_instance, void * p_data);
void g_common_init(void) {
g_new_counting_semaphore0 =
                #if 1
                xSemaphoreCreateCountingStatic(
                #else
                xSemaphoreCreateCounting(
                #endif
                256,
                0
                #if 1
                , &g_new_counting_semaphore0_memory
                #endif
                );
                if (NULL == g_new_counting_semaphore0) {
                rtos_startup_err_callback(g_new_counting_semaphore0, 0);
                }
g_new_message_buffer0 =
                #if 1
                xMessageBufferCreateStatic(
                #else
                xMessageBufferCreate(
                #endif
                512
                #if 1
                , &g_new_message_buffer0_buffer_memory[0],
                &g_new_message_buffer0_memory
                #endif
                );
                if (NULL == g_new_message_buffer0) {
                rtos_startup_err_callback(g_new_message_buffer0, 0);
                }
g_new_mutex0 =
                #if 0
                #if 1
                xSemaphoreCreateRecursiveMutexStatic(&g_new_mutex0_memory);
                #else
                xSemaphoreCreateRecursiveMutex();
                #endif
                #else
                #if 1
                xSemaphoreCreateMutexStatic(&g_new_mutex0_memory);
                #else
                xSemaphoreCreateMutex();
                #endif
                #endif
                if (NULL == g_new_mutex0) {
                rtos_startup_err_callback(g_new_mutex0, 0);
                }
g_new_queue0 =
                #if 1
                xQueueCreateStatic(
                #else
                xQueueCreate(
                #endif
                20,
                4
                #if 1
                , &g_new_queue0_queue_memory[0],
                &g_new_queue0_memory
                #endif
                );
                if (NULL == g_new_queue0) {
                rtos_startup_err_callback(g_new_queue0, 0);
                }
g_new_timer0 =
                #if 1
                xTimerCreateStatic(
                #else
                xTimerCreate(
                #endif
                "New Timer 0",
                100,
                pdFALSE,
                NULL,
                g_new_timer0_callback
                #if 1
                , &g_new_timer0_memory
                #endif
                );
                if (NULL == g_new_timer0) {
                rtos_startup_err_callback(g_new_timer0, 0);
                }
}
