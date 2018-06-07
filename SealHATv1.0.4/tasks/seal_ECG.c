/*
 * seal_ECG.c
 *
 * Created: 6/7/2018 4:21:32 AM
 *  Author: Anthony Koutroulis
 */

 #include "seal_ECG.h"
 #include "seal_DATA.h"
 
 TaskHandle_t           xECG_th;                    // ECG task handle
 static StaticTask_t    xECG_taskbuf;               // task buffer for the ECG task
 static StackType_t     xECG_stack[ECG_STACK_SIZE]; // static stack allocation for ECG task
 
void ECG_isr_dataready(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // will be set to true by notify if we are awakening a higher priority task

    /* Notify the GPS task that the FIFO has enough data to trigger the TxReady interrupt */
    vTaskNotifyGiveFromISR(xECG_th, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
    should be performed to ensure the interrupt returns directly to the highest
    priority task.*/
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
 
 int32_t ECG_task_init(void)
 {
     return ERR_NONE;
 }
 
 void ECG_task(void *pvParameters)
 {
     (void)pvParameters;
 }
 
