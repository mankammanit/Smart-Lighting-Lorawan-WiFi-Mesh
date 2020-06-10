#ifndef __led_driver_H__
#define __led_driver_H__

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "sdkconfig.h"

void init_led();
void Set_Color(uint16_t value);
void Set_Brightness(uint8_t value);

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE

#define LEDC_COLOR_GPIO      (25)
#define LEDC_COLOR_CHANNEL    LEDC_CHANNEL_0

#define LEDC_BRIGHTNESS_GPIO       (26)
#define LEDC_BRIGHTNESS_CHANNEL    LEDC_CHANNEL_1


#define LEDC_MAX_CH_NUM       (2)

//ตั้งเวลา FADE LED
#define LEDC_FADE_TIME    (1000)


//####################//
// เพิ่ม max วัตต์ต่ำต่ำลง  //
// เพิ่ม low วัตต์สูงต่ำลง  //
// ลด max วัตต์ต่ำสูงขึ้น  //
// ลด low วัตต์สูงสูงขึ้น  //
//####################//
//defualt Setting PWM 90 watt 3 module
// #define max_pwm   185
// #define low_pwm   41
#define max_pwm   status_led.MAX_PWM
#define low_pwm   status_led.LOW_PWM



#endif
