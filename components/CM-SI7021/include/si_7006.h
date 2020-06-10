#ifndef COMPONENTS_SI7021_INCLUDE_SI7021_H_
#define COMPONENTS_SI7021_INCLUDE_SI7021_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include <stdlib.h>
#include <stdio.h>

#define SI7021_ADDR 0x40 /*!< SI7021 default address */

#define SI7021_MEASRH_HOLD_CMD 0xE5		/*!< I2C command for RH measuring, hold master mode */
#define SI7021_MEASRH_NOHOLD_CMD 0xF5   /*!< I2C command for RH measuring, no hold master mode */
#define SI7021_MEASTEMP_HOLD_CMD 0xE3   /*!< I2C command for Temperature measuring, hold master mode */
#define SI7021_MEASTEMP_NOHOLD_CMD 0xF3 /*!< I2C command for Temperature measuring, no hold master mode */
#define SI7021_READPREVTEMP_CMD 0xE0	/*!< I2C command for reading previously measured temperature */
#define SI7021_RESET_CMD 0xFE			/*!< I2C command for reseting sensors */
#define SI7021_WRITERHT_REG_CMD 0xE6	/*!< I2C command for writing RH/T user register 1 */
#define SI7021_READRHT_REG_CMD 0xE7		/*!< I2C command for reading RH/T user register 1 */
#define SI7021_WRITEHEATER_REG_CMD 0x51 /*!< I2C command for writing heater register */
#define SI7021_READHEATER_REG_CMD 0x11  /*!< I2C command for reading heater register */
#define SI7021_ID1_CMD 0xFA0F			/*!< I2C command for reading 1st byte of electronic ID */
#define SI7021_ID2_CMD 0xFCC9			/*!< I2C command for reading 2nd byte of electronic ID */
#define SI7021_FIRMVERS_CMD 0x84B8		/*!< I2C command for reading firmware revision */
#define SI7021_SOFT_RESET_CMD 0xFE		/*!< I2C command for soft reseting sensors */

#define SI7021_ERR_OK 0x00			  /*!< Generic OK return value */
#define SI7021_ERR_CONFIG 0x01		  /*!< Error configure i2c driver for sensor. @see ::__si7021_param_config() */
#define SI7021_ERR_INSTALL 0x02		  /*!< Error install i2c driver for sensor. @see ::__si7021_driver_config() */
#define SI7021_ERR_NOTFOUND 0x03	  /*!< Cannot find sensor. @see ::si7021_check_availability() */
#define SI7021_ERR_INVALID_ARG 0x04   /*!< Invalid argument, correlated to esp_err.h#ESP_ERR_INVALID_ARG */
#define SI7021_ERR_FAIL 0x05		  /*!< Generic FAIL return value */
#define SI7021_ERR_INVALID_STATE 0x06 /*!< Sensor in a invalid state */
#define SI7021_ERR_TIMEOUT 0x07		  /*!< Timed out communicating with sensor */

typedef enum SI7021_RESOLUTION
{
	SI7021_12_14_RES = (uint8_t)0x00, /*!< 12bit RH resolution, 14bit temperature resolution */
	SI7021_8_12_RES = (uint8_t)0x01,  /*!<  8bit RH resolution, 12bit temperature resolution */
	SI7021_10_13_RES = (uint8_t)0x80, /*!< 10bit RH resolution, 13bit temperature resolution */
	SI7021_11_11_RES = (uint8_t)0x81  /*!< 11bit RH resolution, 11bit temperature resolution */
} SI7021_RESOLUTION;

typedef enum SI7021_VDD_STATUS
{
	SI7021_VDD_OK = (uint8_t)0x01, /*!< VDD is OK (>1.9 V) */
	SI7021_VDD_LOW = (uint8_t)0x00 /*!< VDD is LOW (1.9~1.8V) */
} SI7021_VDD_STATUS;

#define SI7021_HEATER_ON 0x01  /*!< Heater is ON */
#define SI7021_HEATER_OFF 0x00 /*!< Heater is OFF */

typedef struct si7021_config_t
{
	i2c_config_t sensors_config; /*!< I2C configuration of driver of SI7021. */
	i2c_port_t si7021_port;		 /*!< I2C port of SI7021 sensors, default to port 0. */

} si7021_config_t;

typedef uint8_t si7021_err_t;

void init_si7006();

si7021_config_t __si7021_config;

uint16_t __si7021_read(uint8_t cmd);

bool __is_crc_valid(uint16_t value, uint8_t crc);

si7021_err_t si7021_check_availability();

float si7021_read_temperature();

float si7021_read_humidity();

uint8_t __si7021_read_user_register();

si7021_err_t __si7021_write_user_register(uint8_t value);

SI7021_RESOLUTION si7021_get_resolution();

si7021_err_t si7021_soft_reset();

si7021_err_t si7021_set_resolution(SI7021_RESOLUTION resolution);

uint64_t get_electronic_id();

#endif /* COMPONENTS_SI7021_INCLUDE_SI7021_H_ */
