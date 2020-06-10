#include "voltage.h"


#define READ_VOLTAGE    ADC2_CHANNEL_7

static const adc_bits_width_t width = ADC_WIDTH_BIT_12;

esp_err_t r;

#define NO_OF_SAMPLES 1000

void init_voltage()
{
								gpio_num_t adc_gpio_num;
								r = adc2_pad_get_io_num( READ_VOLTAGE, &adc_gpio_num );
								assert( r == ESP_OK );

								printf("ADC2 channel %d @ GPIO %d\n", READ_VOLTAGE, adc_gpio_num);

								printf("adc2_init...\n");

								adc2_config_channel_atten( READ_VOLTAGE, ADC_ATTEN_DB_11 );

}

int read_voltage()
{
								int volt_read = 0;

								for (int i = 0; i < NO_OF_SAMPLES; i++) {
																int raw;
																adc2_get_raw((adc2_channel_t)READ_VOLTAGE, width, &raw);
																volt_read += raw;

								}
								volt_read /= NO_OF_SAMPLES;
								printf("Volt --> %d\n",volt_read);
								return volt_read;

}
