/* generated main source file - do not edit */
#include "bsp_api.h"
#include "project.h"
#include "hal_data.h"
#include "BSP_aclara.h"

                extern void blinky_thread_create(void);
                extern TaskHandle_t blinky_thread;
//					 extern void SampleTST_create(void);
//                extern TaskHandle_t SampleTST;
                uint32_t g_fsp_common_thread_count;
                bool g_fsp_common_initialized;
                SemaphoreHandle_t g_fsp_common_initialized_semaphore;
                #if configSUPPORT_STATIC_ALLOCATION
                StaticSemaphore_t g_fsp_common_initialized_semaphore_memory;
                #endif
                void g_hal_init(void);
                #if ( TM_RTC_UNIT_TEST == 1 )
                bool RTC_UnitTest(void);
                #endif
                /** Weak reference for tx_err_callback */
                #if defined(__ICCARM__)
                #define rtos_startup_err_callback_WEAK_ATTRIBUTE
                #pragma weak rtos_startup_err_callback = rtos_startup_err_callback_internal
                #elif defined(__GNUC__)
                #define rtos_startup_err_callback_WEAK_ATTRIBUTE __attribute__ ((weak, alias("rtos_startup_err_callback_internal")))
                #endif
                void rtos_startup_err_callback_internal(void * p_instance, void * p_data);
                void rtos_startup_err_callback(void * p_instance, void * p_data) rtos_startup_err_callback_WEAK_ATTRIBUTE;
                /*********************************************************************************************************************
                * @brief This is a weak example initialization error function. It should be overridden by defining a user function
                * with the prototype below.
                * - void rtos_startup_err_callback(void * p_instance, void * p_data)
                *
                * @param[in] p_instance arguments used to identify which instance caused the error and p_data Callback arguments used to identify what error caused the callback.
                **********************************************************************************************************************/
                void rtos_startup_err_callback_internal(void * p_instance, void * p_data);
                void rtos_startup_err_callback_internal(void * p_instance, void * p_data)
                {
                    /** Suppress compiler warning for not using parameters. */
                    FSP_PARAMETER_NOT_USED(p_instance);
                    FSP_PARAMETER_NOT_USED(p_data);

                    /** An error has occurred. Please check function arguments for more information. */
                    BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);
                }

                void rtos_startup_common_init(void);
                void rtos_startup_common_init(void)
                {
                    /* First thread will take care of common initialization. */
                    BaseType_t err;
                    err = xSemaphoreTake(g_fsp_common_initialized_semaphore, portMAX_DELAY);
                    if (pdPASS != err)
                    {
                        /* Check err, problem occurred. */
                        rtos_startup_err_callback(g_fsp_common_initialized_semaphore, 0);
                    }

                    /* Only perform common initialization if this is the first thread to execute. */
                    if (false == g_fsp_common_initialized)
                    {
                        /* Later threads will not run this code. */
                        g_fsp_common_initialized = true;

                        /* Perform common module initialization. */
                        g_hal_init();

                        /* Now that common initialization is done, let other threads through. */
                        /* First decrement by 1 since 1 thread has already come through. */
                        g_fsp_common_thread_count--;
                        while (g_fsp_common_thread_count > 0)
                        {
                            err = xSemaphoreGive(g_fsp_common_initialized_semaphore);
                            if (pdPASS != err)
                            {
                                /* Check err, problem occurred. */
                                rtos_startup_err_callback(g_fsp_common_initialized_semaphore, 0);
                            }
                            g_fsp_common_thread_count--;
                        }
                    }
                }

                int main(void)
                {
                    g_fsp_common_thread_count = 0;
                    g_fsp_common_initialized = false;
//                    vPortDefineHeapRegions( xHeapRegions ); //TODO: We need this for heap_5
                    /* Create semaphore to make sure common init is done before threads start running. */
                    g_fsp_common_initialized_semaphore =
                    #if configSUPPORT_STATIC_ALLOCATION
                    xSemaphoreCreateCountingStatic(
                    #else
                    xSemaphoreCreateCounting(
                    #endif
                        256,
                        1
                        #if configSUPPORT_STATIC_ALLOCATION
                        , &g_fsp_common_initialized_semaphore_memory
                        #endif
                    );

                    if (NULL == g_fsp_common_initialized_semaphore) {
                        rtos_startup_err_callback(g_fsp_common_initialized_semaphore, 0);
                    }

                    /* Init RTOS tasks. */
                    blinky_thread_create();
//                    SampleTST_create();
//                    OS_TASK_Create_All ( true );
                    OS_TASK_Create_STRT();
                    
                    /* Start the scheduler. */
                    vTaskStartScheduler();
                    return 0;
                }
#if ( TM_RTC_UNIT_TEST == 1 )
/*******************************************************************************

  Function name: RTC_UnitTest

  Purpose: This function will test RTC

  Arguments: None

  Returns: bool - 0 if everything was successful, 1 if something failed

*******************************************************************************/
// TODO: RA6 [name_Balaji]: Move to Unit Test Function
bool RTC_UnitTest(void){
    uint16_t retVal = 0;
    uint32_t Sec = 0;
    uint32_t MicroSec = 0;
    sysTime_dateFormat_t set_time =
    {
    .sec  = 55,
    .min  = 59,
    .hour = 23,       //24-HOUR mode       
    .day = 31,      //RDAYCNT
//    .tm_wday = 5,       //RWKCNT 0-SUN, 6-SAT
    .month  = 11,      //RMONCNT 0-Jan 11 Dec
    .year = 121     //RYRCNT //Year SINCE 1900 (2021 = 2021-1900 = 121)
    };
    sysTime_dateFormat_t get_time;
    rtc_alarm_time_t alarm_set_time =
    {
         .sec_match        =  true,
         .min_match        =  true,
         .hour_match       =  false,
         .mday_match       =  false,
         .mon_match        =  false,
         .year_match       =  false,
         .dayofweek_match  =  false
    };
    bool isTimeSetSuccess;
    bool isAlarmSetSuccess;
    bool isRTCValid;
    isTimeSetSuccess = RTC_SetDateTime (&set_time);
    if(isTimeSetSuccess == 0)
    {
      retVal = 1;
    }
    isRTCValid = RTC_Valid();
    if(isRTCValid == false)
    {
      retVal = 1;
    }
    RTC_GetDateTime (&get_time);
    if(get_time.min != 58)
    {
      retVal = 1;
    }
    RTC_GetTimeInSecMicroSec ( &Sec , &MicroSec);
    if(Sec == 0)
    {
      retVal = 1;
    }
    if(MicroSec == 0)
    {
      retVal = 1;
    }
    alarm_set_time.time.tm_sec = 10;
    alarm_set_time.time.tm_min = 59;
    isAlarmSetSuccess = RTC_SetAlarmTime (&alarm_set_time);
    if(isAlarmSetSuccess == 0){
      retVal = 1;
    }
    while(1){
    RTC_GetDateTime (&get_time);
    if(get_time.min != 58)
    {
      retVal = 1;
    }
    }
    return retVal;
}    
#endif
                #if configSUPPORT_STATIC_ALLOCATION
                void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer,
                uint32_t *pulIdleTaskStackSize) BSP_WEAK_REFERENCE;
                void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer,
                uint32_t *pulTimerTaskStackSize) BSP_WEAK_REFERENCE;

                /* If configSUPPORT_STATIC_ALLOCATION is set to 1, the application must provide an
                * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
                * used by the Idle task. */
                void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                                    StackType_t **  ppxIdleTaskStackBuffer,
                                                    uint32_t * pulIdleTaskStackSize )
                {
                    /* If the buffers to be provided to the Idle task are declared inside this
                    * function then they must be declared static - otherwise they will be allocated on
                    * the stack and so not exists after this function exits. */
                    static StaticTask_t xIdleTaskTCB;
                    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

                    /* Pass out a pointer to the StaticTask_t structure in which the Idle
                    * task's state will be stored. */
                    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

                    /* Pass out the array that will be used as the Idle task's stack. */
                    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

                    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
                    * Note that, as the array is necessarily of type StackType_t,
                    * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
                    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
                }

                /* If configSUPPORT_STATIC_ALLOCATION is set to 1, the application must provide an
                * implementation of vApplicationGetTimerTaskMemory() to provide the memory that is
                * used by the RTOS daemon/time task. */
                void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                                     StackType_t **  ppxTimerTaskStackBuffer,
                                                     uint32_t *      pulTimerTaskStackSize )
                {
                    /* If the buffers to be provided to the Timer task are declared inside this
                    * function then they must be declared static - otherwise they will be allocated on
                    * the stack and so not exists after this function exits. */
                    static StaticTask_t xTimerTaskTCB;
                    static StackType_t uxTimerTaskStack[ configMINIMAL_STACK_SIZE ];

                    /* Pass out a pointer to the StaticTask_t structure in which the Idle
                    * task's state will be stored. */
                    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

                    /* Pass out the array that will be used as the Timer task's stack. */
                    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

                    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
                    * Note that, as the array is necessarily of type StackType_t,
                    * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
                    *pulTimerTaskStackSize = configMINIMAL_STACK_SIZE;
                }
                #endif
