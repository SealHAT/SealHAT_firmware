/*
 * seal_GPS.h
 *
 * Created: 5/11/2018 2:35:55 PM
 *  Author: Anthony Koutroulis
 */ 
 #include "seal_RTOS.h"
 #include "seal_CTRL.h"
 #include "sam-m8q/gps.h"

#ifndef SEAL_GPS_H_
#define SEAL_GPS_H_

#define GPS_TXRDY       (0x01)
#define GPS_STACK_SIZE  (1)     // TODO what should this be?
#define GPS_TASK_PRI    (tskIDLE_PRIORITY + 3)
#define GPS_FIFO_SIZE   (1024)  // TODO better pick

typedef enum GPS_NOTIFY_VALS {
    GPS_NOTIFY_NONE     = 0x0000,
    GPS_NOTIFY_TXRDY    = 0x0001,
    GPS_NOTIFY_ALL      = 0xFFFF
} GPS_NOTIFY_VALS;

typedef struct __attribute__((__packed__)) {
    DATA_HEADER_t header;
    gps_log_t     log[GPS_LOGSIZE]; 
} GPS_MSG_t;

extern TaskHandle_t xGPS_th;

int32_t GPS_task_init(void *profile);   // TODO restrict to enumerated type or struct
void    GPS_task(void *pvParameters);
void    GPS_isr_dataready(void);

#endif /* SEAL_GPS_H_ */