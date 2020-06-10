#include "led_driver.h"
#include "st_profile.h"

void init_led()
{

        ledc_timer_config_t ledc_timer = {
                .duty_resolution = LEDC_TIMER_8_BIT, // 0-255
                .freq_hz = 5000,            // frequency of PWM signal
                .speed_mode = LEDC_HS_MODE, // timer mode
                .timer_num = LEDC_HS_TIMER  // timer index
        };
        // Set configuration of timer0 for high speed channels
        ledc_timer_config(&ledc_timer);


        ledc_channel_config_t ledc_channel0 =
        {
                .channel    = LEDC_COLOR_CHANNEL,
                .duty       = 0,
                .gpio_num   = LEDC_COLOR_GPIO,
                .speed_mode = LEDC_HS_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_HS_TIMER
        };
        ledc_channel_config(&ledc_channel0);

        ledc_channel_config_t ledc_channel1 =
        {
                .channel    = LEDC_BRIGHTNESS_CHANNEL,
                .duty       = 0,
                .gpio_num   = LEDC_BRIGHTNESS_GPIO,
                .speed_mode = LEDC_HS_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_HS_TIMER
        };
        ledc_channel_config(&ledc_channel1);

        // Initialize fade service.
        ledc_fade_func_install(0);

        Set_Color(status_led.mycolor);
        Set_Brightness(status_led.mybrightness);

}

void Set_Color(uint16_t value) //<--- input 2700 - 6500 K
{

//return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
        if(value >6500 || value <2700) value =6500;
        uint16_t val = (value - 2700) * (255 - 0) / (6500 - 2700) + 0;
        // printf("Set_Color map :%d\n", val );

        ledc_set_fade_with_time(LEDC_HS_MODE, LEDC_COLOR_CHANNEL, val, LEDC_FADE_TIME);
        ledc_fade_start(LEDC_HS_MODE, LEDC_COLOR_CHANNEL, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);
}

void Set_Brightness(uint8_t value) //<--- input 10-100%
{

//return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
        if(value >100 || value <10) value =100;
        //invert
        value = 100 - value;
        uint16_t val = (value - 10) * (max_pwm - low_pwm) / (100 - 10) + low_pwm;
        printf("Set_Brightness map :%d\n", val );

        ledc_set_fade_with_time(LEDC_HS_MODE, LEDC_BRIGHTNESS_CHANNEL, val, LEDC_FADE_TIME);
        ledc_fade_start(LEDC_HS_MODE, LEDC_BRIGHTNESS_CHANNEL, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);

        // for(int i=0; i<256; i++)
        // {
        //         printf("duty cycle [%d]",i);
        //         ledc_set_fade_with_time(LEDC_HS_MODE, LEDC_BRIGHTNESS_CHANNEL, i, LEDC_FADE_TIME);
        //         ledc_fade_start(LEDC_HS_MODE, LEDC_BRIGHTNESS_CHANNEL, LEDC_FADE_NO_WAIT);
        //         vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);
        // }

}
