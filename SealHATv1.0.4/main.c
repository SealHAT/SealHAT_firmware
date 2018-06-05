#include "seal_RTOS.h"
#include "tasks/seal_ENV.h"
#include "tasks/seal_IMU.h"
#include "tasks/seal_CTRL.h"
#include "tasks/seal_GPS.h"
#include "tasks/seal_DATA.h"
#include "tasks/seal_SERIAL.h"
#include "max30003/ecg.h"
#include "max30003/max30003.h"

#define FIFO_SIZE (100)
ECG_SAMPLE FIFO[FIFO_SIZE];
void spi_init();	


int main(void)
{
    // clear the I2C buses. I2C devices can lock up the bus if there was a reset during a transaction.
    i2c_unblock_bus(ENV_SDA, ENV_SCL);
    i2c_unblock_bus(GPS_SDA, GPS_SCL);
    i2c_unblock_bus(IMU_SDA, IMU_SCL);

    // set FLASH CS pins HIGH
    gpio_set_pin_level(MEM_CS0, true);
    gpio_set_pin_level(MEM_CS1, true);
    gpio_set_pin_level(MEM_CS2, true);

    // initialize the system and set low power mode
    system_init();
    set_lowPower_mode();

    // enable the calendar driver. this function ALWAYS returns ERR_NONE.
    calendar_enable(&RTC_CALENDAR);

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

    // SERIAL task init.
    if(SERIAL_task_init() != ERR_NONE) {
        while(1) {;}
    }

    // start the control task.
    if(CTRL_task_init() != ERR_NONE) {
        while(1) {;}
    }
	
	
	spi_init();	
	config_status t = CONFIG_FAILURE;
	uint32_t count = 0;
	/* clear the ECG sample log */
	memset(FIFO, 0, sizeof(FIFO));
	uint16_t step[8]={0};
	uint16_t sum = step[0];
	/*Initialize the ecg and set for configuration we want*/
	t = ecg_init();
	t = ecg_switch(ENECG_ENABLED);
//	gpio_toggle_pin_level(MOD_SCK);
	for(count = 0;count<6;)
	{
		/*start sampling the data for 1000 sample when there was an interrupt*/
		if(gpio_get_pin_level(MOD2)==false){
			step[count+1] = ecg_sampling_process(sum,FIFO,16);
			sum+=step[count+1];
			count++;
		}
		//		memset(FIFO, 0, sizeof(FIFO));//clear the storage array, when in state machine, we should put the data into storage and then clear it;
	}//end of forever loop
	memset(FIFO, 0, sizeof(FIFO));//clear the storage array, when in state machine, we should put the data into storage and then clear it;
    // Start the freeRTOS scheduler, this will never return.
    vTaskStartScheduler();
    return 0;
}

void spi_init()
{
	spi_m_sync_set_mode(&SPI_MOD, SPI_MODE_0);
	spi_m_sync_enable(&SPI_MOD);
	gpio_set_pin_level(MOD_CS,true);

	ecg_spi_msg.size  = ECG_BUF_SZ;
	ecg_spi_msg.rxbuf = ECG_BUF_I;
	ecg_spi_msg.txbuf = ECG_BUF_O;
}