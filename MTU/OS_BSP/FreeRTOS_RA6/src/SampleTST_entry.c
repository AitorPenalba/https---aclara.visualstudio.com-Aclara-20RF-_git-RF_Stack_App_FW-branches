#include "SampleTST.h"
#include "OS_aclara.h"

#if 0
//typedef SemaphoreHandle_t     OS_MUTEX_Obj, OS_SEM_Obj, *OS_MUTEX_Handle, *OS_SEM_Handle;
//typedef QueueHandle_t         *OS_MUTEX_Handle, *OS_SEM_Handle;

typedef void (* TASK_FPTR)(void *);

typedef struct
{
   uint32_t         task_template_index;  /* TODO: This should be an enum*/

   TASK_FPTR        pvTaskCode;

   size_t           usStackDepth; /* The number of words (not Bytes!)*/

   UBaseType_t      uxPriority;

   char             *pcName;

   uint32_t         TASK_ATTRIBUTES;

   uint32_t         *pvParameters;

   TaskHandle_t     *pxCreatedTask;
} Task_Template_t, * pTask_Template_t;

const char pTskName_Tst[]  = "TEST";

TaskHandle_t tst_handle;
void STRT_StartupTask_2 (void * pvParameters);
const Task_Template_t Task_template_list[] =
{
   /* Task Index,     Function,                Stack,     Pri,    Name,                 Attributes,    Param,     Handle */
   {    1,         STRT_StartupTask_2,           128/4,      2,    (char *)pTskName_Tst,   0,            (void *)0,  &tst_handle },
   {    0  }
};

static OS_MUTEX_Obj myMutex_ = NULL;
static OS_SEM_Obj   mySem_ = NULL;


void STRT_StartupTask_2 (void * pvParameters)
{
   FSP_PARAMETER_NOT_USED(pvParameters);

   while(1)
   {
      vTaskSuspend(NULL);
      vTaskDelay(1);
   }

}

#endif
/* SampleTest entry function */
/* pvParameters contains TaskHandle_t */
void SampleTST_entry(void * pvParameters)
{
   FSP_PARAMETER_NOT_USED(pvParameters);
    bool initSuccess = true;
//    OS_TASK_Create_All ( initSuccess );
#if 0
   if(OS_MUTEX_Create(&myMutex_))
   {
      //if( pdTRUE == xSemaphoreTake(myMutex_,10))
      {
         //xSemaphoreGive(myMutex_);
      }
      //else
      {
      }
   }
   OS_MUTEX_LOCK(&myMutex_,0,0);
   if(OS_SEM_Create(&mySem_))
   {

   }

   Task_Template_t const *pTaskList;  /* Pointer to task list which contains all tasks in the system */

   for (pTaskList = &Task_template_list[0]; 0 != pTaskList->task_template_index; pTaskList++)
   {  /* Create the task if the "Auto Start" attribute is NOT set */
      //if (!(pTaskList->TASK_ATTRIBUTES & MQX_AUTO_START_TASK))
      {  /* Create the task */
//         if ( ( (quiet == 0) || ((pTaskList->TASK_ATTRIBUTES & QUIET_MODE_ATTR) != 0) ) &&
//             ( (rfTest == 0) || ((pTaskList->TASK_ATTRIBUTES & RFTEST_MODE_ATTR) != 0) ) &&
//                ( (initSuccess) || ((pTaskList->TASK_ATTRIBUTES & FAIL_INIT_MODE_ATTR) != 0) ) )
         {
            if (pdPASS != xTaskCreate( pTaskList->pvTaskCode, pTaskList->pcName, pTaskList->usStackDepth, pTaskList->pvParameters, pTaskList->uxPriority, pTaskList->pxCreatedTask ))
            {
               /* This condition should only show up in development.  This infinite loop should help someone figure out
               * that there is an issue creating a task.  Look at pTaskList->TASK_TEMPLATE_INDEX to figure out which task
               * was not created properly.  */
               while(true) /*lint !e716  */
               {}  /* Todo:  We may wish to discuss the definition of LEDs.  Maybe we could add code here. */
            }
            /* Set the exception handler of the task if still valid */
//            if (MQX_NULL_TASK_ID != _task_get_id_from_name(pTaskList->TASK_NAME)) {
//               (void)_task_set_exception_handler(taskID, task_exception_handler);
//               stack_check_init(taskID);
//            }
         }
      }
   }
#endif
   vTaskSuspend(NULL);
   /* TODO: add your own code here */
   while(1)
   {
      vTaskDelay(1);
   }

}