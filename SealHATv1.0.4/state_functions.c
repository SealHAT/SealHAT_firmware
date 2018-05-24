/*
 * state_functions.c
 *
 * Created: 21-May-18 12:20:32 AM
 *  Author: Krystine
 */ 

#include "state_functions.h"

StreamBufferHandle_t xDATA_sb;              // stream buffer for getting data into FLASH or USB
FLASH_DESCRIPTOR     seal_flash_descriptor; /* Declare flash descriptor. */

char READY_TO_RECEIVE = 'r';    /* Character sent over USB to device to initiate packet transfer */
bool STOP_LISTENING;            /* This should be set to true if the device should no longer listen for incoming commands. */
uint8_t dataAr[PAGE_SIZE_EXTRA];

StreamBufferHandle_t init_stream_buffer()
{
    xDATA_sb = xStreamBufferCreate(DATA_QUEUE_LENGTH, PAGE_SIZE_LESS);

    return (xDATA_sb);
}

void set_buffer_trig_level()
{
    xStreamBufferSetTriggerLevel(xDATA_sb, PAGE_SIZE_LESS);
}


/*************************************************************
 * FUNCTION: listen_for_commands()
 * -----------------------------------------------------------
 * This function listens for a command coming in over USB
 * connection. The listening loop may also be broken by setting 
 * the global variable STOP_LISTENING to true. The received 
 * command is returned to the calling function.
 *
 * Parameters: none
 *
 * Returns:
 *      The received command or zero if the loop was broken
 *      before a command was received.
 *************************************************************/
SYSTEM_COMMANDS listen_for_commands()
{
    bool noCommand = true;
    char command;
    
    STOP_LISTENING = false;
    
    /* Keep listening for a command until one is received or until the kill command (STOP_LISTENING) is received. */
    while((noCommand == true) && (STOP_LISTENING == false))
    {
        command = usb_get();
        
        /* If a command was given, break loop. */
        if((command == CONFIGURE_DEV) || (command == RETRIEVE_DATA) || (command == STREAM_DATA))
        {
            noCommand = false;
        }
    }
    
    /* If the loop was broken before a valid command was received, set the return value
     * to NO COMMAND. */
    if(STOP_LISTENING == true)
    {
        command = NO_COMMAND;
    }
    
    return ((SYSTEM_COMMANDS) command);
}

/*************************************************************
 * FUNCTION: call_state_function()
 * -----------------------------------------------------------
 * This function calls the appropriate state function based
 * on the given command.
 *
 * Parameters:
 *      command : One of three options. Function call is based
                  on this value.
 *
 * Returns: void
 *************************************************************/
CMD_RETURN_TYPES call_state_function(SYSTEM_COMMANDS command)
{
    CMD_RETURN_TYPES retVal;
    
    switch(command)
    {
        case CONFIGURE_DEV: retVal = configure_device_state();
            break;      
        case RETRIEVE_DATA: retVal = retrieve_data_state();
            break;      
        case STREAM_DATA: retVal = stream_data_state();
            break;
        default: retVal = UNDEFINED_CMD;
            break;
    }
    
    return (retVal);
}

/*************************************************************
 * FUNCTION: configure_device_state()
 * -----------------------------------------------------------
 * This function receives configuration data and sets it 
 * within the SealHAT device. A ready signal is first sent to
 * the receiver, then the data packet is awaited. The packet
 * integrity is checked before updating configuration data.
 *
 * Parameters: none
 *
 * Returns:
 *      Status of the operation (pass, fail, error)
 *************************************************************/
CMD_RETURN_TYPES configure_device_state()
{
    SENSOR_CONFIGS   tempConfigStruct;  /* Hold configuration settings read over USB. */
    CMD_RETURN_TYPES errVal;            /* Return value for the function call. */
    bool             packetOK;          /* Checking for incoming packet integrity. */
    uint32_t         retVal;            /* Return value of USB function calls. */
    
    /* Initialize return value. */
    errVal = CMD_ERROR;
    
    /* Reinitialize loop control variable. */
    STOP_LISTENING = false;
    packetOK = false;   

    /* Send the ready to receive signal. */
    do { 
        retVal = usb_put(READY_TO_RECEIVE);
    } while((retVal != USB_OK) || (!usb_dtr()));
    
    /* Wait for configuration packet to arrive. */
    do {
        retVal = usb_read(&tempConfigStruct, sizeof(SENSOR_CONFIGS));
    } while((retVal == 0) || (STOP_LISTENING == true));
    
    // TODO: error check packet
    packetOK = true; //for testing. will actually need to be checked.
    
    if(packetOK)
    {
        /* Temp struct has passed the test and may become the real struct. */
        config_settings = tempConfigStruct;
        
        /* Save new configuration settings. */
        save_sensor_configs(&config_settings);
        
        // TODO: restart sensors with new config data? restart device?
        
        errVal = NO_ERROR;
    }
    
    return (errVal);
}

/*************************************************************
 * FUNCTION: retrieve_data_state()
 * -----------------------------------------------------------
 * This function streams all data from the device's external
 * flash to the PC via USB connection. One page's worth of 
 * data is sent at a time.
 *
 * Parameters: none
 *
 * Returns:
 *      Success or failure code.
 *************************************************************/
CMD_RETURN_TYPES retrieve_data_state()
{
    CMD_RETURN_TYPES errVal;    /* Return value for the function call. */
    uint32_t pageIndex;         /* Loop control for iterating over flash pages. */
    uint32_t numPagesWritten;   /* Total number of pages currently written to flash. */
    uint32_t retVal;            /* USB return value for error checking/handling. */
    
    /* Initializations */
    numPagesWritten = num_pages_written();
    pageIndex = 0;
    
    /* Loop through every page that has data and send it over USB in PAGE_SIZE buffers.
     * TODO: send address or page index here too for crash recovery. */
    while(pageIndex < numPagesWritten)
    {
        /* Read a page of data from external flash. */
        retVal = flash_io_read(&seal_flash_descriptor, seal_flash_descriptor.buf_0, PAGE_SIZE_LESS);
        
        /* Write data to USB. */
        do {
           retVal = usb_write(seal_flash_descriptor.buf_0, PAGE_SIZE_LESS);
        } while((retVal != USB_OK) || (!usb_dtr()));    
        
        pageIndex++;        
    }
    
    return (NO_ERROR);
}

/*************************************************************
 * FUNCTION: stream_data_state()
 * -----------------------------------------------------------
 * This function streams live data from all sensors to the PC
 * over USB. 
 *
 * Parameters: none
 *
 * Returns:
 *      Success or failure code.
 *************************************************************/
void stream_data_state()
{
    int32_t err;

    while()
    {
        xStreamBufferReceive(xDATA_sb, dataAr, PAGE_SIZE_LESS, portMAX_DELAY);

        if(usb_state() == USB_Configured) 
        {
            if(usb_dtr()) 
            {
                err = usb_write(dataAr, PAGE_SIZE_LESS);
                if(err != ERR_NONE && err != ERR_BUSY) 
                {
                    // TODO: log USB errors, however rare they are
                    gpio_set_pin_level(LED_GREEN, false);
                }
            }
            else 
            {
                usb_flushTx();
            }
        } /* END if(usb_state() == USB_Configured) */
    } /* END while() */    
}

/*************************************************************
 * FUNCTION: log_data_state()
 * -----------------------------------------------------------
 * This function
 *
 * Parameters: none
 *
 * Returns: void
 *************************************************************/
CMD_RETURN_TYPES log_data_state()
{
       
}

/*************************************************************
 * FUNCTION: log_and_stream_state()
 * -----------------------------------------------------------
 * This function
 *
 * Parameters: none
 *
 * Returns: void
 *************************************************************/
CMD_RETURN_TYPES log_and_stream_state()
{
       
}