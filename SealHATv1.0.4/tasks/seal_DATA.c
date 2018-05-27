/*
 * seal_DATA.c
 *
 * Created: 26-May-18 12:23:37
 *  Author: Ethan
 */
#include "seal_DATA.h"

TaskHandle_t        xDATA_th;                       // Message accumulator for USB/MEM
static StaticTask_t xDATA_taskbuf;                  // task buffer for the CTRL task
static StackType_t  xDATA_stack[DATA_STACK_SIZE];   // static stack allocation for CTRL task

static SemaphoreHandle_t DATA_mutex;                // mutex to control access to USB terminal
static StaticSemaphore_t xDATA_mutexBuff;           // static memory for the mutex

static StreamBufferHandle_t xDATA_sb;                            // stream buffer for getting data into FLASH or USB
static uint8_t              dataQueueStorage[DATA_QUEUE_LENGTH]; // static memory for the data queue
static StaticStreamBuffer_t xDataQueueStruct;                    // static memory for data queue data structure

int32_t ctrlLog_write(uint8_t* buff, const uint32_t LEN)
{
    uint32_t err;

    if(xSemaphoreTake(DATA_mutex, ~0)) {

        // bail early if there isn't enough space
        portENTER_CRITICAL();
        if(xStreamBufferSpacesAvailable(xDATA_sb) >= LEN) {
            err = xStreamBufferSend(xDATA_sb, buff, LEN, 0);
        }
        else {
            err = ERR_NO_RESOURCE;
            gpio_set_pin_level(LED_RED, false);
        }
        portEXIT_CRITICAL();

        xSemaphoreGive(DATA_mutex);
    }
    else {
        err = ERR_FAILURE;
        gpio_set_pin_level(LED_RED, false);
    }

    return err;
}

int32_t ctrlLog_writeISR(uint8_t* buff, const uint32_t LEN)
{
    uint32_t err;

    if(xSemaphoreTake(DATA_mutex, ~0)) {

        // bail early if there isn't enough space
        portENTER_CRITICAL();
        if(xStreamBufferSpacesAvailable(xDATA_sb) >= LEN) {
            err = xStreamBufferSendFromISR(xDATA_sb, buff, LEN, 0);
        }
        else {
            err = ERR_NO_RESOURCE;
            gpio_set_pin_level(LED_RED, false);
        }
        portEXIT_CRITICAL();

        xSemaphoreGive(DATA_mutex);
    }
    else {
        err = ERR_FAILURE;
        gpio_set_pin_level(LED_RED, false);
    }

    return err;
}

int32_t DATA_task_init(void)
{
    struct calendar_date date;
    struct calendar_time time;

    date.year  = 2018;
    date.month = 5;
    date.day   = 4;

    time.hour = 15;
    time.min  = 33;
    time.sec  = 0;

    // return values not checked since they  ALWAYS returns ERR_NONE.
    calendar_set_baseyear(&RTC_CALENDAR, SEALHAT_BASE_YEAR);
    calendar_set_date(&RTC_CALENDAR, &date);
    calendar_set_time(&RTC_CALENDAR, &time);

    // enable CRC generator. This function does nothing apparently,
    // but we call it to remain consistent with API. it ALWAYS returns ERR_NONE.
    crc_sync_enable(&CRC_0);

    // create the mutex for access to the data queue
    DATA_mutex = xSemaphoreCreateMutexStatic(&xDATA_mutexBuff);
    configASSERT(DATA_mutex);

    // create the data queue
    xDATA_sb = xStreamBufferCreateStatic(DATA_QUEUE_LENGTH, PAGE_SIZE_LESS, dataQueueStorage, &xDataQueueStruct);
    configASSERT(xDATA_sb);

    /* Initialize flash device(s). */
    //flash_io_init(&seal_flash_descriptor, PAGE_SIZE_LESS);

    xDATA_th = xTaskCreateStatic(DATA_task, "DATA", DATA_STACK_SIZE, NULL, DATA_TASK_PRI, xDATA_stack, &xDATA_taskbuf);
    configASSERT(xDATA_th);

    return ERR_NONE;
}

void DATA_task(void* pvParameters)
{
    static DATA_TRANSMISSION_t usbPacket; // TEST DATA -> = {USB_PACKET_START_SYM, "From the Halls of Montezuma; To the shores of Tripoli;\nWe fight our country's battles\nIn the air, on land, and sea;\nFirst to fight for right and freedom\nAnd to keep our honor clean;\nWe are proud to claim the title\nOf United States Marine.\n\nOur flag's unfurled to every breeze\nFrom dawn to setting sun;\nWe have fought in every clime and place\nWhere we could take a gun;\nIn the snow of far-off Northern lands\nAnd in sunny tropic scenes,\nYou will find us always on the job\nThe United States Marines.\n\nHere's health to you and to our Corps\nWhich we are proud to serve;\nIn many a strife we've fought for life\nAnd never lost our nerve.\nIf the Army and the Navy\nEver look on Heaven's scenes,\nThey will find the streets are guarded\nBy United States Marines.\n\nRealizing it is my choice and my choice alone to be a Reconnaissance Marine, I accept all challenges involved with this profession. Forever shall I strive to maintain the tremendous reputation of those who went before me.\nExceeding beyond the limitations set down by others shall be my goal. Sacrificing personal comforts and dedicating myself to the completion of the reconnaissance mission shall be my life. Physical fitness, mental attitude, and high ethics --\nThe title of Recon Marine is my honor.\nConquering all obstacles, both large and small, I shall never quit. To quit, to surrender, to give up is to fail. To be a Recon Marine is to surpass failure; To overcome, to adapt and to do whatever it takes to complete the mission.\nOn the battlefield, as in all areas of life, I shall stand tall above the competition. Through professional pride, integrity, and teamwork, I shall be the example for all Marines to emulate.\nNever shall I forget the principles I accepted to become a Recon Marine. Honor, Perseverance, Spirit and Heart.\nA Recon Marine can speak without saying a word and achieve what others can only imagine.\nIrregular Warfare is not a new concept to the United States Marine Corps, employing direct action with indigenous forces\nThis is extra text to make it fit in the buff!\n\n", 0xFFFFFFFF};
    int32_t err;
    (void)pvParameters;

    /* Receive and write data forever. */
    for(;;)
    {
        /* Receive a page worth of data. */
        xStreamBufferReceive(xDATA_sb, usbPacket.data, PAGE_SIZE_LESS, portMAX_DELAY);

        // setup the packet header and CRC start value, then perform CRC32
        usbPacket.startSymbol = USB_PACKET_START_SYM;
        usbPacket.crc = 0xFFFFFFFF;
        crc_sync_crc32(&CRC_0, (uint32_t*)usbPacket.data, PAGE_SIZE_LESS/sizeof(uint32_t), &usbPacket.crc);

        // complement CRC to match standard CRC32 implementations
        usbPacket.crc ^= 0xFFFFFFFF;

        if(usb_state() == USB_Configured) {
            if(usb_dtr()) {
                err = usb_write(&usbPacket, sizeof(DATA_TRANSMISSION_t));
                if(err != ERR_NONE && err != ERR_BUSY) {
                    // TODO: log usb errors, however rare they are
                    gpio_set_pin_level(LED_GREEN, false);
                }
            }
            else {
                usb_flushTx();
            }
        }
        else {
            /* Write data to external flash device. */
            //            flash_io_write(&seal_flash_descriptor, usbPacket.data, PAGE_SIZE_LESS);
        }
    }
}
