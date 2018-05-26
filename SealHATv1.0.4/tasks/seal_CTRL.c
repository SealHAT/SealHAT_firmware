/*
 * seal_MSG.c
 *
 * Created: 30-Apr-18 22:53:41
 *  Author: Ethan
 */

#include "seal_CTRL.h"
#include "seal_USB.h"
#include "sealPrint.h"
#include "storage\flash_io.h"

TaskHandle_t xCTRL_th;                                      // Message accumulator for USB/MEM
StaticTask_t xCTRL_taskbuf;                                 // task buffer for the CTRL task
StackType_t  xCTRL_stack[CTRL_STACK_SIZE];                  // static stack allocation for CTRL task

EventGroupHandle_t xSYSEVENTS_handle;                       // IMU event group
StaticEventGroup_t xSYSEVENTS_eventgroup;                   // static memory for the event group

SemaphoreHandle_t DATA_mutex;                               // mutex to control access to USB terminal
StaticSemaphore_t xDATA_mutexBuff;                          // static memory for the mutex

StreamBufferHandle_t xDATA_sb;                              // stream buffer for getting data into FLASH or USB
static uint8_t       dataQueueStorage[DATA_QUEUE_LENGTH];   // static memory for the data queue
StaticStreamBuffer_t xDataQueueStruct;                      // static memory for data queue data structure

EEPROM_STORAGE_t eeprom_data;                               //struct containing sensor and SealHAT configurations
FLASH_DESCRIPTOR seal_flash_descriptor;                     /* Declare flash descriptor. */

void vbus_detection_cb(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult;

    if( gpio_get_pin_level(VBUS_DETECT) ) {
        usb_start();
        xResult = xEventGroupSetBitsFromISR(xSYSEVENTS_handle, EVENT_VBUS, &xHigherPriorityTaskWoken);
    }
    else {
        usb_stop();
        xResult = xEventGroupClearBitsFromISR(xSYSEVENTS_handle, EVENT_VBUS);
    }

    if(xResult != pdFAIL) {
        /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch should be requested. */
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

void timestamp_FillHeader(DATA_HEADER_t* header)
{
    // get the time in seconds since (the custom set) epoch
    header->timestamp = _calendar_get_counter(&RTC_CALENDAR.device);

    // force a sync on the counter value and get the sub second value (2048 per second, or about .5 mSec)
    hri_tc_set_CTRLB_CMD_bf(TC4, TC_CTRLBSET_CMD_READSYNC_Val);
    header->msTime = hri_tccount16_get_COUNT_reg(TC4, 0xFFFF);
}

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

int32_t CTRL_task_init(void)
{
    struct calendar_date date;
    struct calendar_time time;
    int32_t err = ERR_NONE;
    uint32_t retVal;

    // enable CRC generator. This function does nothing apparently,
    // but we call it to remain consistent with API.
    crc_sync_enable(&CRC_0);

    // enable the calendar driver
    calendar_enable(&RTC_CALENDAR);

    date.year  = 2018;
    date.month = 5;
    date.day   = 4;

    time.hour = 15;
    time.min  = 33;
    time.sec  = 0;

    err = calendar_set_date(&RTC_CALENDAR, &date);
    if(err != ERR_NONE) { return err; }

    err = calendar_set_time(&RTC_CALENDAR, &time);
    if(err != ERR_NONE) { return err; }

    // create 24-bit system event group for system alerts
    xSYSEVENTS_handle = xEventGroupCreateStatic(&xSYSEVENTS_eventgroup);
    configASSERT(xSYSEVENTS_handle);

    // create the mutex for access to the data queue
    DATA_mutex = xSemaphoreCreateMutexStatic(&xDATA_mutexBuff);
    configASSERT(DATA_mutex);

    // create the data queue
    xDATA_sb = xStreamBufferCreateStatic(DATA_QUEUE_LENGTH, PAGE_SIZE_LESS, dataQueueStorage, &xDataQueueStruct);
    configASSERT(xDATA_sb);

    /* Read stored device settings from EEPROM and make them accessible to all devices. */
    retVal = read_sensor_configs(&eeprom_data.config_settings);
    
    /* Initialize flash device(s). */
    flash_io_init(&seal_flash_descriptor, PAGE_SIZE_LESS);
    
    // initialize (clear all) event group and check current VBUS level
    xEventGroupClearBits(xSYSEVENTS_handle, EVENT_MASK_ALL);
    if(gpio_get_pin_level(VBUS_DETECT)) {
        usb_start();
        xEventGroupSetBits(xSYSEVENTS_handle, EVENT_VBUS);
    }

    // initialize (clear all) event group and check current VBUS level
    xEventGroupClearBits(xSYSEVENTS_handle, EVENT_MASK_ALL);
    if(gpio_get_pin_level(VBUS_DETECT)) {
        usb_start();
        xEventGroupSetBits(xSYSEVENTS_handle, EVENT_VBUS);
    }

    xCTRL_th = xTaskCreateStatic(CTRL_task, "MSG", CTRL_STACK_SIZE, NULL, CTRL_TASK_PRI, xCTRL_stack, &xCTRL_taskbuf);
    configASSERT(xCTRL_th);

    return err;
}

void CTRL_task(void* pvParameters)
{
    static DATA_TRANSMISSION_t usbPacket; // TEST DATA -> = {USB_PACKET_START_SYM, "From the Halls of Montezuma; To the shores of Tripoli;\nWe fight our country's battles\nIn the air, on land, and sea;\nFirst to fight for right and freedom\nAnd to keep our honor clean;\nWe are proud to claim the title\nOf United States Marine.\n\nOur flag's unfurled to every breeze\nFrom dawn to setting sun;\nWe have fought in every clime and place\nWhere we could take a gun;\nIn the snow of far-off Northern lands\nAnd in sunny tropic scenes,\nYou will find us always on the job\nThe United States Marines.\n\nHere's health to you and to our Corps\nWhich we are proud to serve;\nIn many a strife we've fought for life\nAnd never lost our nerve.\nIf the Army and the Navy\nEver look on Heaven's scenes,\nThey will find the streets are guarded\nBy United States Marines.\n\nRealizing it is my choice and my choice alone to be a Reconnaissance Marine, I accept all challenges involved with this profession. Forever shall I strive to maintain the tremendous reputation of those who went before me.\nExceeding beyond the limitations set down by others shall be my goal. Sacrificing personal comforts and dedicating myself to the completion of the reconnaissance mission shall be my life. Physical fitness, mental attitude, and high ethics --\nThe title of Recon Marine is my honor.\nConquering all obstacles, both large and small, I shall never quit. To quit, to surrender, to give up is to fail. To be a Recon Marine is to surpass failure; To overcome, to adapt and to do whatever it takes to complete the mission.\nOn the battlefield, as in all areas of life, I shall stand tall above the competition. Through professional pride, integrity, and teamwork, I shall be the example for all Marines to emulate.\nNever shall I forget the principles I accepted to become a Recon Marine. Honor, Perseverance, Spirit and Heart.\nA Recon Marine can speak without saying a word and achieve what others can only imagine.\nIrregular Warfare is not a new concept to the United States Marine Corps, employing direct action with indigenous forces\nThis is extra text to make it fit in the buff!\n\n", 0xFFFFFFFF};
    int32_t err;
    (void)pvParameters;
    
    volatile uint32_t tempSizeThing = sizeof(eeprom_data.config_settings);

    // register VBUS detection interrupt
    ext_irq_register(VBUS_DETECT, vbus_detection_cb);

    // set the stream buffer trigger level for USB
    xStreamBufferSetTriggerLevel(xDATA_sb, PAGE_SIZE_LESS);

    /* Receive and write data forever. */
    for(;;)
    {
        /* Receive a page worth of data. */
        xStreamBufferReceive(xDATA_sb, usbPacket.data, PAGE_SIZE_LESS, portMAX_DELAY);

        /* Write data to USB if the appropriate flag is set. 
        if((xEventGroupGetBits(xSYSEVENTS_handle) & EVENT_LOGTOUSB) != 0)
        {
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
        }        */
        
        /* Log data to flash if the appropriate flag is set.
        if((xEventGroupGetBits(xSYSEVENTS_handle) & EVENT_LOGTOFLASH) != 0)
        {
            
        } */
        
        /* Write data to external flash device. */
        //flash_io_write(&seal_flash_descriptor, usbPacket.data, PAGE_SIZE_LESS);
    }
}
