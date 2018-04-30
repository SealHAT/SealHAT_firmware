#include "driver_init.h"
#include "atmel_start_pins.h"

#include "hal_io.h"
#include "hal_rtos.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "limits.h"

#include "sealUtil.h"
#include "sealUSB.h"
#include "sealPrint.h"
#include "max44009/max44009.h"
#include "si705x/si705x.h"
#include "lsm303/LSM303AGR.h"

typedef enum {
    DEV_ENV  = 0x10,
    DEV_IMU  = 0x20,
    DEV_GPS  = 0x30,
    DEV_MOD  = 0x40,
    DEV_MEM  = 0x50,
    DEV_CPU  = 0x60,
    DEV_MASK = 0xF0
} DEV_ID_t;

typedef enum {
    ERROR_NONE    = 0x00,
    ERROR_TEMP    = 0x01,
    ERROR_LIGHT   = 0x02,
    ERROR_CRC     = 0x03,
    ERROR_I2C     = 0x04,
    ERROR_TIMEOUT = 0x05
} DEV_ERR_CODES_t;

typedef struct __attribute__((__packed__)){
    uint16_t srtSym;    // symbol to indicate start of packet
    uint16_t  id;	    // Upper four bits is the device ID, lower four are device specific event flags
    uint32_t timestamp; // timestamp. how many bits?
    uint16_t size;		// size of data packet to follow. in bytes or samples? (worst case IMU size in bytes would need a uint16 :( )
} DATA_HEADER_t;

typedef struct __attribute__((__packed__)){
    DATA_HEADER_t header;
    uint16_t      temp;
    uint16_t      light;
} ENV_MSG_t;

typedef struct __attribute__((__packed__)){
    DATA_HEADER_t header;
    AxesRaw_t     accelData[25];
} IMU_MSG_t;

#define TASK_LED_STACK_SIZE             (400 / sizeof(portSTACK_TYPE))
#define TASK_LED_STACK_PRIORITY         (tskIDLE_PRIORITY + 1)

#define TASK_MONITOR_STACK_SIZE         (320 / sizeof(portSTACK_TYPE))
#define TASK_MONITOR_STACK_PRIORITY     (tskIDLE_PRIORITY + 2)

#define MSG_STACK_SIZE                  (500 / sizeof(portSTACK_TYPE))
#define MSG_TASK_PRI                    (tskIDLE_PRIORITY + 1)

#define ENV_STACK_SIZE                  (500 / sizeof(portSTACK_TYPE))
#define ENV_TASK_PRI                    (tskIDLE_PRIORITY + 2)

#define IMU_STACK_SIZE                  (1000 / sizeof(portSTACK_TYPE))
#define IMU_TASK_PRI                    (tskIDLE_PRIORITY + 3)

/*** Define the task handles for each system ***/
static TaskHandle_t      xCreatedMonitorTask;   // debug monitoring task
static TaskHandle_t      xENV_th;               // environmental sensors task (light and temp)
static TaskHandle_t      xIMU_th;               // IMU
static TaskHandle_t      xMSG_th;               // Message accumulator for USB/MEM
static TaskHandle_t      xGPS_th;               // GPS
static TaskHandle_t      xMOD_th;               // Modular port task

static SemaphoreHandle_t disp_mutex;            // mutex to control access to USB terminal
static QueueHandle_t     msgQ;                  // a message Q for

/**
 * \brief Write string to console
 */
static void str_write(const char *s)
{
    usb_write((void*)s, strlen(s));
}

static bool disp_mutex_take(void)
{
    return xSemaphoreTake(disp_mutex, ~0);
}

static void disp_mutex_give(void)
{
    xSemaphoreGive(disp_mutex);
}

static void ENV_task(void* pvParameters)
{
//    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    int32_t     err = 0;        // for catching API errors
    ENV_MSG_t   msg;            // reads the data
    (void)pvParameters;

    // initialize the temperature sensor
    si705x_init(&I2C_ENV);

    // initialize the light sensor
    max44009_init(&I2C_ENV, LIGHT_ADD_GND);

//    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    // set the header data
    msg.header.srtSym    = 0xDEAD;
    msg.header.size      = 4;   // four bytes of data in an env packet
    msg.header.timestamp = 0;   // use timestamp as an index for now.

    for(;;) {
        // reset the message header
        msg.header.id           = DEV_ENV;
        msg.header.timestamp++;

        // start an asynchronous temperature reading
        portENTER_CRITICAL();
        err = si705x_measure_asyncStart();
        portEXIT_CRITICAL();

        //  read the light level
        portENTER_CRITICAL();
        msg.light = max44009_read_uint16();
        portEXIT_CRITICAL();

//        uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

        // wait for the temperature sensor to finish
        os_sleep(pdMS_TO_TICKS(5));

        // get temp
        portENTER_CRITICAL();
        msg.temp  = si705x_measure_asyncGet(&err);
        portEXIT_CRITICAL();
        if(err < 0) {
            msg.header.id |= ERROR_TEMP;
        }

        if (disp_mutex_take()) {
            usb_write(&msg, sizeof(ENV_MSG_t));
            disp_mutex_give();
        }

        // sleep till the next sample
        os_sleep(pdMS_TO_TICKS(975));
    }
}

#define ACC_DATA_READY      (0x01)
#define MAG_DATA_READY      (0x02)
#define MOTION_DETECT       (0x04)

static void AccelerometerDataReadyISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // will be set to true by notify if we are awakening a higher priority task

    gpio_set_pin_level(MOD2, true);
    /* Notify the IMU task that the ACCEL FIFO is ready to read */
    xTaskNotifyFromISR(xIMU_th, ACC_DATA_READY, eSetBits, &xHigherPriorityTaskWoken);

    gpio_set_pin_level(MOD2, false);
    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
    should be performed to ensure the interrupt returns directly to the highest
    priority task. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void MagnetometerDataReadyISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // will be set to true by notify if we are awakening a higher priority task

    gpio_set_pin_level(MOD9, true);
    /* Notify the IMU task that the ACCEL FIFO is ready to read */
    xTaskNotifyFromISR(xIMU_th, MAG_DATA_READY, eSetBits, &xHigherPriorityTaskWoken);

    gpio_set_pin_level(MOD9, false);
    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
    should be performed to ensure the interrupt returns directly to the highest
    priority task.*/
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void AccelerometerMotionISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // will be set to true by notify if we are awakening a higher priority task

    /* Notify the IMU task that the ACCEL FIFO is ready to read */
    xTaskNotifyFromISR(xIMU_th, MOTION_DETECT, eSetBits, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
    should be performed to ensure the interrupt returns directly to the highest
    priority task.*/
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void IMU_task(void* pvParameters)
{
//    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 3000 );     // max block time, set to slightly more than accelerometer ISR period
    BaseType_t  xResult;                // holds return value of blocking function
    int32_t     err = 0;                // for catching API errors
    uint32_t    ulNotifyValue;          // notification value from ISRs
    IMU_MSG_t   msg;                    // reads the data
    AxesRaw_t   magData;                // hold magnetometer data
    (void)pvParameters;

    // initialize the temperature sensor
    lsm303_init(&I2C_IMU);
    lsm303_acc_startFIFO(ACC_SCALE_2G, ACC_HR_50_HZ);
    lsm303_mag_start(MAG_LP_50_HZ);

    // initialize the message header
    msg.header.srtSym    = 0xDEAD;
    msg.header.id        = DEV_IMU;
    msg.header.size      = sizeof(AxesRaw_t)*25;
    msg.header.timestamp = 0;

    // enable the data ready interrupts
    ext_irq_register(IMU_INT1_XL, AccelerometerDataReadyISR);
    ext_irq_register(IMU_INT2_XL, AccelerometerMotionISR);
    ext_irq_register(IMU_INT_MAG, MagnetometerDataReadyISR);

//    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for(;;) {

        xResult = xTaskNotifyWait( pdFALSE,          /* Don't clear bits on entry. */
                                   ULONG_MAX,        /* Clear all bits on exit. */
                                   &ulNotifyValue,   /* Stores the notified value. */
                                   xMaxBlockTime );

        if( pdPASS == xResult ) {

            if( ACC_DATA_READY & ulNotifyValue ) {
                portENTER_CRITICAL();
                gpio_set_pin_level(MOD2, true);
                err = lsm303_acc_FIFOread(&msg.accelData[0], 25, NULL);
                gpio_set_pin_level(MOD2, false);
                portEXIT_CRITICAL();
                msg.header.timestamp++;

                if (disp_mutex_take() && err >= 0) {
                    usb_write(&msg, sizeof(IMU_MSG_t));
                    disp_mutex_give();
                }
            }

            if( MAG_DATA_READY & ulNotifyValue ) {
                portENTER_CRITICAL();
                gpio_set_pin_level(MOD9, true);
                err = lsm303_mag_rawRead(&magData);
                gpio_set_pin_level(MOD9, false);
                portEXIT_CRITICAL();
            }

            if( MOTION_DETECT & ulNotifyValue ) {
                // TODO - read type of motion detected and trigger callback
            }
        }
        else {
            DATA_HEADER_t tmOut;
            tmOut.id        = DEV_IMU | ERROR_TIMEOUT;
            tmOut.size      = 0;
            tmOut.timestamp = 65;

            if (disp_mutex_take()) {
                usb_write(&tmOut, sizeof(DATA_HEADER_t));
                disp_mutex_give();
            }
        }
    } // END FOREVER LOOP
}

static void MSG_task(void* pvParameters)
{
    ENV_MSG_t msg;                              // hold the received messages
    (void)pvParameters;

    for(;;) {

        // try to get a message. Will sleep for 1000 ticks and print
        // error if nothing received in that time.
        if( !xQueueReceive(msgQ, &msg, 1500) ) {
            if (disp_mutex_take()) {
                str_write("MSG RECEIVE FAIL (1000 ticks)\n");
                disp_mutex_give();
            }
        }
        else {
            if (disp_mutex_take()) {
                usb_write(&msg, sizeof(ENV_MSG_t));
                disp_mutex_give();
            }
        }
    }
}

/**
 * OS task that monitor system status and show statistics every 5 seconds
 */
static void task_monitor(void* pvParameters)
{
    static portCHAR szList[128];
	(void)pvParameters;

    for (;;) {
		if (disp_mutex_take()) {

            // print RTOS stats to console
			sprintf(szList, "%c%c%c%c", 0x1B, '[', '2', 'J');
			str_write(szList);
			sprintf(szList, "--- Number of tasks: %u\r\n", (unsigned int)uxTaskGetNumberOfTasks());
			str_write(szList);
			str_write("> Tasks\tState\tPri\tStack\tNum\r\n");
			str_write("***********************************\r\n");
			vTaskList(szList);
			str_write(szList);

			disp_mutex_give();
		}

		os_sleep(5000);
	}
}

void vApplicationIdleHook( void )
{
    sleep(PM_SLEEPCFG_SLEEPMODE_IDLE_Val);
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
    TaskHandle_t xHandle;
    TaskStatus_t xTaskDetails;
    (void)xTask;

    /* Obtain the handle of a task from its name. */
    xHandle = xTaskGetHandle((char*)pcTaskName);

    /* Check the handle is not NULL. */
    configASSERT(xHandle);

    /* Use the handle to obtain further information about the task. */
    vTaskGetInfo( /* The handle of the task being queried. */
                  xHandle,
                  /* The TaskStatus_t structure to complete with information
                  on xTask. */
                  &xTaskDetails,
                  /* Include the stack high water mark value in the
                  TaskStatus_t structure. */
                  pdTRUE,
                  /* Include the task state in the TaskStatus_t structure. */
                  eInvalid );

    while(1) {;}
}

int main(void)
{
    // TODO put all system init in a function in utils
    i2c_unblock_bus(IMU_SDA, IMU_SCL);
    i2c_unblock_bus(GPS_SDA, GPS_SCL);
    i2c_unblock_bus(ENV_SDA, ENV_SCL);
	system_init();
    usb_start();

    // disable Brown Out Detector
    SUPC->BOD33.reg &= ~SUPC_BOD33_ENABLE;

    /* Select BUCK converter as the main voltage regulator in active mode */
    SUPC->VREG.bit.SEL = SUPC_VREG_SEL_BUCK_Val;
    /* Wait for the regulator switch to be completed */
    while(!(SUPC->STATUS.reg & SUPC_STATUS_VREGRDY));

    /* Set Voltage Regulator Low power Mode Efficiency */
    SUPC->VREG.bit.LPEFF = 0x1;

    /* Apply SAM L21 Erratum 15264 - CPU will freeze on exit from standby if BUCK is disabled */
    SUPC->VREG.bit.RUNSTDBY = 0x1;
    SUPC->VREG.bit.STDBYPL0 = 0x1;

    /* Set Performance Level to PL0 as we run @12MHz */
    _set_performance_level(PM_PLCFG_PLSEL_PL0_Val);

     disp_mutex = xSemaphoreCreateMutex();
     if (disp_mutex == NULL) { while (1){} }

//     msgQ = xQueueCreate(2, sizeof(ENV_MSG_t));
//     if(msgQ == NULL) { while(1){ ; } }
//     vQueueAddToRegistry(msgQ, "ENV_MSG");

//     if(xTaskCreate(MSG_task, "MESSAGE", MSG_STACK_SIZE, NULL, MSG_TASK_PRI, &xMSG_th) != pdPASS) {
//         while(1) { ; }
//     }

    if(xTaskCreate(ENV_task, "ENV", ENV_STACK_SIZE, NULL, ENV_TASK_PRI, &xENV_th) != pdPASS) {
        while(1) { ; }
    }

    if(xTaskCreate(IMU_task, "IMU", IMU_STACK_SIZE, NULL, IMU_TASK_PRI, &xIMU_th) != pdPASS) {
        while(1) { ; }
    }

	/* Create task to monitor processor activity */
// 	if (xTaskCreate(task_monitor, "Monitor", TASK_MONITOR_STACK_SIZE, NULL, TASK_MONITOR_STACK_PRIORITY, &xCreatedMonitorTask) != pdPASS) {
//         while(1) { ; }
//     }

	vTaskStartScheduler();

	return 0;
}

