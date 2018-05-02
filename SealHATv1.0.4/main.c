#include "driver_init.h"
#include "atmel_start_pins.h"

#include "seal_UTIL.h"
#include "seal_RTOS.h"
#include "seal_ENV.h"
#include "seal_IMU.h"
#include "seal_MSG.h"

#define TASK_MONITOR_STACK_SIZE         (320 / sizeof(portSTACK_TYPE))
#define TASK_MONITOR_STACK_PRIORITY     (tskIDLE_PRIORITY + 2)

/*** Define the task handles for each system ***/
//static TaskHandle_t      xCreatedMonitorTask;   // debug monitoring task
//static TaskHandle_t      xGPS_th;               // GPS
//static TaskHandle_t      xMOD_th;               // Modular port task

/**
 * \brief Write string to console
 */
// static void str_write(const char *s)
// {
//     usb_write((void*)s, strlen(s));
// }
// 
// static bool disp_mutex_take(void)
// {
//     return xSemaphoreTake(disp_mutex, ~0);
// }
// 
// static void disp_mutex_give(void)
// {
//     xSemaphoreGive(disp_mutex);
// }

/**
 * OS task that monitor system status and show statistics every 5 seconds
 */
// static void task_monitor(void* pvParameters)
// {
//     static portCHAR szList[128];
// 	(void)pvParameters;
// 
//     for (;;) {
// 		if (disp_mutex_take()) {
// 
//             // print RTOS stats to console
// 			sprintf(szList, "%c%c%c%c", 0x1B, '[', '2', 'J');
// 			str_write(szList);
// 			sprintf(szList, "--- Number of tasks: %u\r\n", (unsigned int)uxTaskGetNumberOfTasks());
// 			str_write(szList);
// 			str_write("> Tasks\tState\tPri\tStack\tNum\r\n");
// 			str_write("***********************************\r\n");
// 			vTaskList(szList);
// 			str_write(szList);
// 
// 			disp_mutex_give();
// 		}
// 
// 		os_sleep(5000);
// 	}
// }

int main(void)
{
    i2c_unblock_bus(IMU_SDA, IMU_SCL);
    i2c_unblock_bus(GPS_SDA, GPS_SCL);
    i2c_unblock_bus(ENV_SDA, ENV_SCL);
	system_init();
    set_lowPower_mode();
    usb_start();

    if(MSG_task_init(2000) != ERR_NONE) {
        while(1) {;}
    }

    if(ENV_task_init(1) != ERR_NONE) {
        while(1) {;}
    }

    if(IMU_task_init(0xCAFED00D) != ERR_NONE) {
        while(1) {;}
    }

	/* Create task to monitor processor activity */
// 	if (xTaskCreate(task_monitor, "Monitor", TASK_MONITOR_STACK_SIZE, NULL, TASK_MONITOR_STACK_PRIORITY, &xCreatedMonitorTask) != pdPASS) {
//         while(1) { ; }
//     }

	vTaskStartScheduler();

	return 0;
}

