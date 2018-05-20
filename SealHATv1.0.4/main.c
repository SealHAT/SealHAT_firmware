#include "driver_init.h"
#include "atmel_start_pins.h"

#include "seal_UTIL.h"
#include "seal_RTOS.h"
#include "seal_USB.h"
#include "tasks/seal_ENV.h"
#include "tasks/seal_IMU.h"
#include "tasks/seal_CTRL.h"

int main(void)
{
    // clear the I2C busses. I2C devices can lock up the bus if there was a reset during a transaction.
    i2c_unblock_bus(IMU_SDA, IMU_SCL);
    i2c_unblock_bus(GPS_SDA, GPS_SCL);
    i2c_unblock_bus(ENV_SDA, ENV_SCL);

    // initialize the system and set low power mode
	system_init();
    set_lowPower_mode();

    // start the control task.
    if(CTRL_task_init() != ERR_NONE) {
        while(1) {;}
    }

    // start the environmental sensors
//     if(ENV_task_init() != ERR_NONE) {
//         while(1) {;}
//     }

    // IMU task init.
    if(IMU_task_init() != ERR_NONE) {
        while(1) {;}
    }

    // Start the freeRTOS scheduler, this will never return.
	vTaskStartScheduler();

	return 0;
}