#include "si_7006.h"
#include "math.h"


void init_si7006()
{
    si7021_config_t __si7021_config = {.sensors_config = {
                                           .mode = I2C_MODE_MASTER,
                                           .sda_io_num = GPIO_NUM_21,
                                           .scl_io_num = GPIO_NUM_22,
                                           .sda_pullup_en =
                                               GPIO_PULLUP_ENABLE,
                                           .scl_pullup_en = GPIO_PULLUP_ENABLE,
                                           .master = {.clk_speed = 100000},
                                       },
                                       .si7021_port = I2C_NUM_0};

    i2c_param_config(__si7021_config.si7021_port, &__si7021_config.sensors_config);
    i2c_driver_install(__si7021_config.si7021_port, I2C_MODE_MASTER, 0, 0, 0);
    si7021_set_resolution(SI7021_8_12_RES);
}

si7021_err_t si7021_check_availability()
{
    esp_err_t err;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SI7021_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(__si7021_config.si7021_port, cmd,
                               1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        return SI7021_ERR_NOTFOUND;
    }
    return SI7021_ERR_OK;
}

float si7021_read_temperature()
{
    uint16_t raw_temp = __si7021_read(SI7021_MEASTEMP_NOHOLD_CMD);
    if (raw_temp == 0)
    {
        return NAN;
    }
    return (raw_temp * 175.72 / 65536.0) - 46.85;
}

float si7021_read_humidity()
{
    uint16_t raw_humidity = __si7021_read(SI7021_MEASRH_NOHOLD_CMD);
    if (raw_humidity == 0)
    {
        return NAN;
    }
    return (125.0 * raw_humidity / 65536.0) - 6.0;
}

uint16_t __si7021_read(uint8_t command)
{

    esp_err_t err;
    uint8_t msb, lsb, crc;
    uint16_t raw_value;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SI7021_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, command, true);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(__si7021_config.si7021_port, cmd,
                               1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        return 0;
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SI7021_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &msb, 0x00);
    i2c_master_read_byte(cmd, &lsb, 0x00);
    i2c_master_read_byte(cmd, &crc, 0x01);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(__si7021_config.si7021_port, cmd,
                               1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        return 0;
    }

    raw_value = ((uint16_t)msb << 8) | (uint16_t)lsb;
    if (!__is_crc_valid(raw_value, crc))
        printf("CRC invalid\r\n");
    return raw_value & 0xFFFC;
}

bool __is_crc_valid(uint16_t value, uint8_t crc)
{

    // line the bits representing the input in a row (first data, then crc)
    uint32_t row = (uint32_t)value << 8;
    row |= crc;

    // polynomial = x^8 + x^5 + x^4 + 1
    // padded with zeroes corresponding to the bit length of the CRC
    uint32_t divisor = (uint32_t)0x988000;

    for (int i = 0; i < 16; i++)
    {

        // if the input bit above the leftmost divisor bit is 1,
        // the divisor is XORed into the input
        if (row & (uint32_t)1 << (23 - i))
            row ^= divisor;

        // the divisor is then shifted one bit to the right
        divisor >>= 1;
    }

    // the remainder should equal zero if there are no detectable errors
    return (row == 0);
}

uint8_t __si7021_read_user_register()
{
    esp_err_t err;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SI7021_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, SI7021_READRHT_REG_CMD, true);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(__si7021_config.si7021_port, cmd,
                               1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        return 0;
    }
    uint8_t reg_value;
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SI7021_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &reg_value, 0x01);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(__si7021_config.si7021_port, cmd,
                               1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        return 0;
    }
    return reg_value;
}

SI7021_RESOLUTION si7021_get_resolution()
{
    uint8_t reg_value = __si7021_read_user_register();
    return reg_value & 0x81;
}

si7021_err_t si7021_set_resolution(SI7021_RESOLUTION resolution)
{
    uint8_t current_reg_value = __si7021_read_user_register();
    if (resolution & (1 << 0))
    {
        current_reg_value = (current_reg_value & ~(1 << 0)) | (1 << 0);
    }
    else
    {
        current_reg_value = (current_reg_value & ~(1 << 0)) | (0 << 0);
    }
    if (resolution & (1 << 7))
    {
        current_reg_value = (current_reg_value & ~(1 << 7)) | (1 << 7);
    }
    else
    {
        current_reg_value = (current_reg_value & ~(1 << 7)) | (0 << 7);
    }
    return __si7021_write_user_register(current_reg_value);
}

si7021_err_t __si7021_write_user_register(uint8_t value)
{
    esp_err_t err;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SI7021_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, SI7021_WRITERHT_REG_CMD, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(__si7021_config.si7021_port, cmd,
                               1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    switch (err)
    {

    case ESP_ERR_INVALID_ARG:
        return SI7021_ERR_INVALID_ARG;

    case ESP_FAIL:
        return SI7021_ERR_FAIL;

    case ESP_ERR_INVALID_STATE:
        return SI7021_ERR_INVALID_STATE;

    case ESP_ERR_TIMEOUT:
        return SI7021_ERR_TIMEOUT;
    }
    return SI7021_ERR_OK;
}
