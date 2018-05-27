#include "seal_RTOS.h"

#include "tasks/seal_ENV.h"
#include "tasks/seal_IMU.h"
#include "tasks/seal_CTRL.h"
#include "tasks/seal_GPS.h"
#include "tasks/seal_DATA.h"

int main(void)
{
    // clear the I2C busses. I2C devices can lock up the bus if there was a reset during a transaction.
    i2c_unblock_bus(ENV_SDA, ENV_SCL);
    i2c_unblock_bus(GPS_SDA, GPS_SCL);
    i2c_unblock_bus(IMU_SDA, IMU_SCL);

    // initialize the system and set low power mode
    system_init();
    set_lowPower_mode();

    // enable the calendar driver. this function ALWAYS returns ERR_NONE.
    calendar_enable(&RTC_CALENDAR);

    // start the control task.
    if(CTRL_task_init() != ERR_NONE) {
        while(1) {;}
    }

    // start the data aggregation task
    if(DATA_task_init() != ERR_NONE) {
        while(1) {;}
    }

    // start the environmental sensors
    if(ENV_task_init(1) != ERR_NONE) {
        while(1) {;}
    }

    // GPS task init
    if(GPS_task_init(0) != ERR_NONE) {
        while(1) {;}
    }

    // IMU task init.
    if(IMU_task_init(ACC_SCALE_2G, ACC_HR_50_HZ, MAG_LP_50_HZ) != ERR_NONE) {
        while(1) {;}
    }

    // Start the freeRTOS scheduler, this will never return.
    vTaskStartScheduler();
    return 0;
}
