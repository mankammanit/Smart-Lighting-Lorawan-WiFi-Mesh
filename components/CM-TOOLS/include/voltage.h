
#ifndef __VOLTAGE_H__
#define __VOLTAGE_H__

#include	<stdio.h>
#include	<stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp_system.h"
#include "esp_adc_cal.h"

void init_voltage();
int read_voltage();

#endif
