#ifndef __nvs_storage_H__
#define __nvs_storage_H__

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "time.h"
#include "sdkconfig.h"
#include "st_profile.h"
#include "string.h"


////////////////////////////////////
void save_led(led_status ptr);
bool read_led(led_status *ptr);
void save_string(const char *key, char *str);
bool get_string(const char* key, char* out_value, size_t* length);
////////////////////////////////////

#endif
