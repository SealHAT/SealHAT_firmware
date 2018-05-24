/*
 * state_functions.h
 *
 * Created: 21-May-18 12:20:21 AM
 *  Author: Krystine
 */ 

#ifndef STATE_FUNCTIONS_H_
#define STATE_FUNCTIONS_H_

#include "utilities/seal_USB.h"
#include "seal_RTOS.h"
#include "storage/flash_io.h"
#include "seal_config.h"

#define DATA_QUEUE_LENGTH (3000)

typedef enum {
    NO_COMMAND      = 0,
    CONFIGURE_DEV   = 'c',
    RETRIEVE_DATA   = 'r',
    STREAM_DATA     = 's',
} SYSTEM_COMMANDS;

typedef enum {
    UNDEFINED_CMD   = -2,
    CMD_ERROR       = -1,
    NO_ERROR        = 0,
} CMD_RETURN_TYPES;

extern bool STOP_LISTENING;     /* This should be set to true if the device should no longer listen for incoming commands. */
extern char READY_TO_RECEIVE;
extern FLASH_DESCRIPTOR seal_flash_descriptor; /* Declare flash descriptor. */

extern StreamBufferHandle_t xDATA_sb;           // stream buffer for getting data into FLASH or USB


/*************************************************************
 * FUNCTION: listen_for_commands()
 * -----------------------------------------------------------
 * This function listens for a command coming in over USB
 * connection. The listening loop may also be broken by setting 
 * the global variable STOP_LISTENING to true.
 *************************************************************/
SYSTEM_COMMANDS listen_for_commands();

/*************************************************************
 * FUNCTION: configure_device_state()
 * -----------------------------------------------------------
 * This function receives configuration data and sets it 
 * within the SealHAT device.
 *************************************************************/
CMD_RETURN_TYPES configure_device_state();

/*************************************************************
 * FUNCTION: retrieve_data_state()
 * -----------------------------------------------------------
 * This function streams all data from the device's external
 * flash to the PC via USB connection. One page's worth of 
 * data is sent at a time.
 *************************************************************/
CMD_RETURN_TYPES retrieve_data_state();

void stream_data_state();

#endif /* STATE_FUNCTIONS_H_ */