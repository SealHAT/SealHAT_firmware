/*
 * seal_config.c
 *
 * Created: 20-May-18 2:23:48 PM
 *  Author: Krystine
 */ 

#include "seal_config.h"

#define CONFIG_BLOCK_BASE_ADDR          (0x3F840)   /* First writable page address of on-chip EEPROM. */

/*************************************************************
 * FUNCTION: save_sensor_configs()
 * -----------------------------------------------------------
 * This function writes the SealHAT device's sensor and 
 * configuration data out to the chip's onboard EEPROM.
 *
 * Parameters:
 *      config_settings : Pointer to struct of config settings.
 *
 * Returns:
 *      The error value of the flash_write operation.
 *************************************************************/
uint32_t save_sensor_configs(SENSOR_CONFIGS *config_settings)
{
    uint32_t retVal;
    uint8_t  numBlocksToErase = sizeof(SENSOR_CONFIGS);
    
    /* Flash must be erased before a new value may be written to it. */
    retVal = flash_erase(&FLASH_NVM, CONFIG_BLOCK_BASE_ADDR, numBlocksToErase);
    
    /* If the erase operation succeeded, write the new data to the EEPROM.
     * Otherwise, return the error value. */
    if(retVal == ERR_NONE)
    {
        retVal = flash_write(&FLASH_NVM, CONFIG_BLOCK_BASE_ADDR, &config_settings, sizeof(SENSOR_CONFIGS));
    }
    
    return (retVal);
}

/*************************************************************
 * FUNCTION: read_sensor_configs()
 * -----------------------------------------------------------
 * This function reads the SealHAT device's sensor and
 * configuration settings from the onboard EEPROM.
 *
 * Parameters:
 *      config_settings : Pointer to struct of config settings.
 *
 * Returns:
 *      The error value of the flash_read operation.
 *************************************************************/
uint32_t read_sensor_configs(SENSOR_CONFIGS *config_settings)
{
    return(flash_read(&FLASH_NVM, CONFIG_BLOCK_BASE_ADDR, &config_settings, sizeof(SENSOR_CONFIGS)));
}