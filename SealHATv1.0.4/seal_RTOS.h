/*
 * seal_RTOS.h
 *
 * Created: 30-Apr-18 23:06:44
 *  Author: Ethan
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "limits.h"
#include "event_groups.h"
#include "stream_buffer.h"

#include "hal_io.h"
#include "hal_rtos.h"

#include "seal_Types.h"
#include "seal_UTIL.h"

#ifndef SEAL_RTOS_H_
#define SEAL_RTOS_H_

void vApplicationIdleHook(void);

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);

#endif /* SEAL_RTOS_H_ */