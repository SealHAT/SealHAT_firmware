#include "driver_init.h"
#include "atmel_start_pins.h"

#include "seal_UTIL.h"
#include "seal_RTOS.h"
#include "seal_ENV.h"
#include "seal_IMU.h"
#include "seal_CTRL.h"

/*** Define the task handles for each system ***/
//static TaskHandle_t      xGPS_th;               // GPS
//static TaskHandle_t      xMOD_th;               // Modular port task

int main(void)
{
    i2c_unblock_bus(IMU_SDA, IMU_SCL);
    i2c_unblock_bus(GPS_SDA, GPS_SCL);
    i2c_unblock_bus(ENV_SDA, ENV_SCL);
	system_init();
    set_lowPower_mode();

    if(MSG_task_init(2000) != ERR_NONE) {
        while(1) {;}
    }

    if(ENV_task_init(1) != ERR_NONE) {
        while(1) {;}
    }

//     if(IMU_task_init(0xCAFED00D) != ERR_NONE) {
//         while(1) {;}
//     }

	vTaskStartScheduler();

	return 0;
}

