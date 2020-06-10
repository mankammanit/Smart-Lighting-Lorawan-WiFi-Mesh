#ifndef __CMSTRUCT_H__
#define __CMSTRUCT_H__

#include "string.h"

/////////////////////////////////////////////
typedef struct led_status
{

        int meshid[6];
        uint8_t meshch;

        uint16_t mycolor;
        uint8_t mybrightness;
        uint64_t ledworking;

        uint8_t MAX_PWM;
        uint8_t LOW_PWM;
        uint16_t MODEL;
        uint8_t GEN;

} led_status;
led_status status_led;
/////////////////////////////////////////////

#endif
