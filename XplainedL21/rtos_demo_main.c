#include "atmel_start.h"
#include "atmel_start_pins.h"

#include "hal_io.h"
#include "hal_rtos.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "max44009\max44009.h"
#include "si705x\si705x.h"

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

#define TASK_STACK_SIZE                 (500 / sizeof(portSTACK_TYPE))
#define MSG_TASK_PRI                    (tskIDLE_PRIORITY + 1)
#define ENV_TASK_PRI                    (tskIDLE_PRIORITY + 2)

static SemaphoreHandle_t disp_mutex;
static TaskHandle_t      xCreatedMonitorTask;
static TaskHandle_t      xEnvTaskHandle;
static TaskHandle_t      xMsgTaskHandle;
static QueueHandle_t     msgQ;

/**
 * \brief Write string to console
 */
static void str_write(const char *s)
{
	io_write(&EDBG_COM.io, (const uint8_t *)s, strlen(s));
}

static bool disp_mutex_take(void)
{
    return xSemaphoreTake(disp_mutex, ~0);
}

static void disp_mutex_give(void)
{
    xSemaphoreGive(disp_mutex);
}

static void environmentTask(void* pvParameters)
{
    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    int32_t     err = 0;        // for catching API errors
    ENV_MSG_t   message;        // reads the data
    (void*)pvParameters;

    // initialize the temperature sensor
    si705x_init(&I2C_ENV);

    // initialize the light sensor
    max44009_init(&I2C_ENV, LIGHT_ADD_GND);

    // initialize the message header and stop byte
    message.start = 0x03;
    message.stop  = 0xFC;

    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for(;;) {
        // read the temp
        gpio_set_pin_level(DEBUG_0, true);
        err = si705x_measure_asyncStart();

        //  read the light level in floating point lux
        message.light = max44009_read_float();

        uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

        // wait for the temperature sensor to finish
        os_sleep(portTICK_PERIOD_MS * 5);

        // get temp
        message.temp  = si705x_celsius(si705x_measure_asyncGet(&err));
        if(err < 0) { message.temp = 0.00; }

        // send the data to the Queue
        xQueueSend(msgQ, (void*)&message, 0);

//         if (disp_mutex_take()) {
//  		    io_write(&EDBG_COM.io, (uint8_t*)&message, sizeof(ENV_MSG_t));
//  			disp_mutex_give();
//  		}

        gpio_set_pin_level(DEBUG_0, false);
        os_sleep(portTICK_PERIOD_MS * 975);
    }
}

static void msgTask(void* pvParameters)
{
    ENV_MSG_t msg;                              // hold the received messages
    (void*)pvParameters;

    for(;;) {

        // try to get a message. Will sleep for 1000 ticks and print
        // error if nothing received in that time.
        if( !xQueueReceive(msgQ, &msg, 1000) ) {
            if (disp_mutex_take()) {
                io_write(&EDBG_COM.io, (uint8_t *)"MSG RECEIVE FAIL (1000 ticks)\n", 30);
                disp_mutex_give();
            }
        }
        else {
            if (disp_mutex_take()) {
                io_write(&EDBG_COM.io, (uint8_t*)&msg, sizeof(ENV_MSG_t));
                disp_mutex_give();
            }
        }
    }
}

/**
 * \brief Dump statistics to console
 */
static void _os_show_statistics(void)
{
	static portCHAR szList[128];
	sprintf(szList, "%c%c%c%c", 0x1B, '[', '2', 'J');
	str_write(szList);
	sprintf(szList, "--- Number of tasks: %u\r\n", (unsigned int)uxTaskGetNumberOfTasks());
	str_write(szList);
	str_write("> Tasks\tState\tPri\tStack\tNum\r\n");
	str_write("***********************************\r\n");
	vTaskList(szList);
	str_write(szList);
}

/**
 * OS task that monitor system status and show statistics every 5 seconds
 */
static void task_monitor(void *p)
{
	(void)p;
	for (;;) {
		if (disp_mutex_take()) {
			_os_show_statistics();
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
    (void*)xTask;

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
	atmel_start_init();

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

    gpio_set_pin_level(LED0, true);

     disp_mutex = xSemaphoreCreateMutex();
     if (disp_mutex == NULL) { while (1){} }

    msgQ = xQueueCreate(2, sizeof(ENV_MSG_t));
    if(msgQ == NULL) { while(1){} }
    vQueueAddToRegistry(msgQ, "ENV_MSG");

    if(xTaskCreate(environmentTask, "ENV", TASK_STACK_SIZE, NULL, ENV_TASK_PRI, xEnvTaskHandle) != pdPASS) {
        while(1) { ; }
    }

    if(xTaskCreate(msgTask, "MESSAGE", TASK_STACK_SIZE, NULL, MSG_TASK_PRI, xMsgTaskHandle) != pdPASS) {
        while(1) { ; }
    }

	/* Create task to monitor processor activity */
// 	if (xTaskCreate(task_monitor, "Monitor", TASK_MONITOR_STACK_SIZE, NULL, TASK_MONITOR_STACK_PRIORITY, &xCreatedMonitorTask) != pdPASS) {
//         while(1) { ; }
//     }

	vTaskStartScheduler();

	return 0;
}

#define _SPRINTF_OVERRIDE 1
#if _SPRINTF_OVERRIDE
/* Override sprintf implement to optimize */

static const unsigned m_val[] = {1000000000u, 100000000u, 10000000u, 1000000u, 100000u, 10000u, 1000u, 100u, 10u, 1u};
int sprintu(char *s, unsigned u)
{
	char tmp_buf[12];
	int  i, n = 0;
	int  m;

	if (u == 0) {
		*s = '0';
		return 1;
	}

	for (i = 0; i < 10; i++) {
		for (m = 0; m < 10; m++) {
			if (u >= m_val[i]) {
				u -= m_val[i];
			} else {
				break;
			}
		}
		tmp_buf[i] = m + '0';
	}
	for (i = 0; i < 10; i++) {
		if (tmp_buf[i] != '0') {
			break;
		}
	}
	for (; i < 10; i++) {
		*s++ = tmp_buf[i];
		n++;
	}
	return n;
}

int sprintf(char *s, const char *fmt, ...)
{
	int     n = 0;
	va_list ap;
	va_start(ap, fmt);
	while (*fmt) {
		if (*fmt != '%') {
			*s = *fmt;
			s++;
			fmt++;
			n++;
		} else {
			fmt++;
			switch (*fmt) {
			case 'c': {
				char valch = va_arg(ap, int);
				*s         = valch;
				s++;
				fmt++;
				n++;
				break;
			}
			case 'd': {
				int vali = va_arg(ap, int);
				int nc;

				if (vali < 0) {
					*s++ = '-';
					n++;
					nc = sprintu(s, -vali);
				} else {
					nc = sprintu(s, vali);
				}

				s += nc;
				n += nc;
				fmt++;
				break;
			}
			case 'u': {
				unsigned valu = va_arg(ap, unsigned);
				int      nc   = sprintu(s, valu);
				n += nc;
				s += nc;
				fmt++;
				break;
			}
			case 's': {
				char *vals = va_arg(ap, char *);
				while (*vals) {
					*s = *vals;
					s++;
					vals++;
					n++;
				}
				fmt++;
				break;
			}
			default:
				*s = *fmt;
				s++;
				fmt++;
				n++;
			}
		}
	}
	va_end(ap);
	*s = 0;
	return n;
}
#endif /* _SPRINTF_OVERRIDE */
