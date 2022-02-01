#include "STRT.h"
/* StartUp_Thread entry function */
/* pvParameters contains TaskHandle_t */
extern void blinky_thread_create(void);
void STRT_entry(void * pvParameters)
{
   FSP_PARAMETER_NOT_USED(pvParameters);
   blinky_thread_create();
   /* TODO: add your own code here */
   while(1)
   {
      vTaskDelay(1);
   }
}
