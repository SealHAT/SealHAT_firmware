/*
 * seal_config.h
 *
 * Created: 20-May-18 2:23:28 PM
 *  Author: Krystine
 */ 

#ifndef SEAL_CONFIG_H_
#define SEAL_CONFIG_H_

#include "tasks/seal_Types.h"
#include "driver_init.h"

/*************************************************************
 * FUNCTION: save_sensor_configs()
 * -----------------------------------------------------------
 * This function writes the SealHAT device's sensor and 
 * configuration data out to the chip's onboard EEPROM.
 *************************************************************/
uint32_t save_sensor_configs(SENSOR_CONFIGS *config_settings);

/*************************************************************
 * FUNCTION: read_sensor_configs()
 * -----------------------------------------------------------
 * This function reads the SealHAT device's sensor and
 * configuration settings from the onboard EEPROM.
 *************************************************************/
uint32_t read_sensor_configs(SENSOR_CONFIGS *config_settings);



#endif /* SEAL_CONFIG_H_ */