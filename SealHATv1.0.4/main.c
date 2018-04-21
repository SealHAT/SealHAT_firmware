#include "atmel_start.h"
#include "atmel_start_pins.h"

#include "hal_io.h"
#include "hal_rtos.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "max44009/max44009.h"
#include "si705x/si705x.h"
#include "lsm303/LSM303AGR.h"

#define TASK_LED_STACK_SIZE (400 / sizeof(portSTACK_TYPE))
#define TASK_LED_STACK_PRIORITY (tskIDLE_PRIORITY + 1)

#define TASK_MONITOR_STACK_SIZE (320 / sizeof(portSTACK_TYPE))
#define TASK_MONITOR_STACK_PRIORITY (tskIDLE_PRIORITY + 2)

typedef struct __attribute__((__packed__)){
    uint8_t start;
    float   temp;
    float   light;
    uint8_t stop;
} ENV_MSG_t;

typedef struct __attribute__((__packed__)){
    uint8_t start;
    AxesSI_t axis;
    uint8_t stop;
} IMU_MSG_t;

#define TASK_STACK_SIZE                 (500 / sizeof(portSTACK_TYPE))
#define MSG_TASK_PRI                    (tskIDLE_PRIORITY + 1)
#define ENV_TASK_PRI                    (tskIDLE_PRIORITY + 2)

static SemaphoreHandle_t disp_mutex;
static TaskHandle_t      xCreatedMonitorTask;
static TaskHandle_t      xENV_th;
static TaskHandle_t      xIMU_th;
static TaskHandle_t      xMSG_th;
static QueueHandle_t     msgQ;

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

    // initialize the message header and stop byte
    msg.start = 0x03;
    msg.stop  = 0xFC;

//    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for(;;) {
        // read the temp
        gpio_set_pin_level(LED_GREEN, true);
        err = si705x_measure_asyncStart();

        //  read the light level in floating point lux
        msg.light = max44009_read_float();

//        uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

        // wait for the temperature sensor to finish
        os_sleep(portTICK_PERIOD_MS * 5);

        // get temp
        msg.temp  = si705x_celsius(si705x_measure_asyncGet(&err));
        if(err < 0) { msg.temp = (float)err; }

        // send the data to the Queue
        xQueueSend(msgQ, (void*)&msg, 0);

        gpio_set_pin_level(LED_GREEN, false);
        os_sleep(portTICK_PERIOD_MS * 975);
    }
}

static void IMU_task(void* pvParameters)
{
    //    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    int32_t     err = 0;        // for catching API errors
    IMU_MSG_t   msg;            // reads the data
    (void)pvParameters;

    // initialize the temperature sensor
    lsm303_init(&I2C_IMU);
    lsm303_startAcc(ACC_SCALE_2G, ACC_HR_10_HZ);

    // initialize the message header and stop byte
    msg.start = 0x03;
    msg.stop  = 0xFC;

    //    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for(;;) {
        // read the temp
        gpio_set_pin_level(LED_RED, true);

        err = lsm303_acc_dataready();
        if(err > 0 ) {
            // get new reading as floats
            msg.axis = lsm303_getGravity();

            if (disp_mutex_take()) {
                usb_write(&msg, sizeof(IMU_MSG_t));
                disp_mutex_give();
            }
        }

        gpio_set_pin_level(LED_RED, false);
        os_sleep(portTICK_PERIOD_MS * 100);
    }
}

static void MSG_task(void* pvParameters)
{
    ENV_MSG_t msg;                              // hold the received messages
    (void)pvParameters;

    for(;;) {

        // try to get a message. Will sleep for 1000 ticks and print
        // error if nothing received in that time.
        if( !xQueueReceive(msgQ, &msg, 1000) ) {
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
	//atmel_start_init();
    system_init();
    usb_init();

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

    msgQ = xQueueCreate(2, sizeof(ENV_MSG_t));
    if(msgQ == NULL) { while(1){} }
    vQueueAddToRegistry(msgQ, "ENV_MSG");

    if(xTaskCreate(ENV_task, "ENV", TASK_STACK_SIZE, NULL, ENV_TASK_PRI, xENV_th) != pdPASS) {
        while(1) { ; }
    }

    if(xTaskCreate(IMU_task, "IMU", TASK_STACK_SIZE, NULL, ENV_TASK_PRI, xIMU_th) != pdPASS) {
        while(1) { ; }
    }

    if(xTaskCreate(MSG_task, "MESSAGE", TASK_STACK_SIZE, NULL, MSG_TASK_PRI, xMSG_th) != pdPASS) {
        while(1) { ; }
    }

	/* Create task to monitor processor activity */
// 	if (xTaskCreate(task_monitor, "Monitor", TASK_MONITOR_STACK_SIZE, NULL, TASK_MONITOR_STACK_PRIORITY, &xCreatedMonitorTask) != pdPASS) {
//         while(1) { ; }
//     }

	vTaskStartScheduler();

	return 0;
}

