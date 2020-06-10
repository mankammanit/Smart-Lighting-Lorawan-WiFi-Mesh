/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include "time.h"
#include "sys/time.h"

#include "mdf_common.h"
#include "mwifi.h"
#include "mupgrade.h"
#include "led_driver.h"
#include "mwifi.h"
#include "cJSON.h"
#include "st_profile.h"
#include "nvs_storage.h"
#include "str2hex.h"
#include "murata_parser.h"

#define SPP_TAG "SPP_ACCEPTOR_DEMO"
#define SPP_SERVER_NAME "SPP_SERVER"
#define EXCAMPLE_DEVICE_NAME  "cm_master_%s"

extern void ota_task();

enum cmd_bluetooth
{
        WRITE_LORAWAN_PF = 1,
        READ_LORAWAN_PF = 2,
        WRITE_MESH_NETWORK_PF = 3,
        READ_MESH_NETWORK_PF = 4,
        TEST_CONTROL_SL = 5,
        UPDATE_FW_VIA_BLE = 6,
        RESTART_MCU = 7,
        WRITE_STREET_LIGHT_PF =8,
        READ_STREET_LIGHT_PF = 9,

};

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;

static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_NONE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
        char buf[1024];
        char spp_data[256];
        switch (event) {
        case ESP_SPP_INIT_EVT:
                ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
                uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
                esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
                char comp_str_node[ sizeof( sta_mac )*2 +1 ] = {'\0'};
                size_t sta_length_node = (sizeof(sta_mac)/sizeof(sta_mac[0]));
                for( size_t i=0; i<sta_length_node; i++ )
                {
                        sprintf( comp_str_node,"%s%02x", comp_str_node, sta_mac[i] );
                }
                // printf("NODE ID ---> %s\n",comp_str_node);
                char bluetooth_[24];
                sprintf(bluetooth_,EXCAMPLE_DEVICE_NAME,comp_str_node);
                esp_bt_dev_set_device_name(bluetooth_);
                esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
                esp_spp_start_srv(sec_mask,role_slave, 0, SPP_SERVER_NAME);
                break;
        case ESP_SPP_DISCOVERY_COMP_EVT:
                ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
                break;
        case ESP_SPP_OPEN_EVT:
                ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
                break;
        case ESP_SPP_CLOSE_EVT:
                ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT");
                break;
        case ESP_SPP_START_EVT:
                ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
                break;
        case ESP_SPP_CL_INIT_EVT:
                ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
                break;
        case ESP_SPP_DATA_IND_EVT:

                ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len=%d handle=%d",
                         param->data_ind.len, param->data_ind.handle);
                if (param->data_ind.len < 1023) {
                        snprintf(buf, (size_t)param->data_ind.len, (char *)param->data_ind.data);
                        printf("%s\n", buf);

                        cJSON *attributes = cJSON_Parse(buf);
                        if (attributes != NULL)
                        {
                                uint8_t LIST_CMD = cJSON_GetObjectItem(attributes,"CMD")->valueint;

                                switch (LIST_CMD) {
                                case WRITE_LORAWAN_PF:
                                        // {"CMD":1,"Devaddr":"00:00:00:00","Nwkskey":"00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00","Appskey":"00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00"}

                                        printf("case WRITE_LORAWAN_PF\n");

                                        devaddr =  cJSON_GetObjectItem(attributes,"Devaddr")->valuestring;
                                        nwkskey =  cJSON_GetObjectItem(attributes,"Nwkskey")->valuestring;
                                        appskey =  cJSON_GetObjectItem(attributes,"Appskey")->valuestring;

                                        save_string("DEVADDR",devaddr);
                                        save_string("NWKSKEY",nwkskey);
                                        save_string("APPSKEY",appskey);

                                        sprintf(spp_data, "Write: %s,[%d]\n",buf,param->data_ind.len);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        lora_init(NULL);

                                        break;

                                case READ_LORAWAN_PF:
                                        // {"CMD":2}
                                        printf("case READ_LORAWAN_PF\n");


                                        sprintf(spp_data, "Devaddr:%s\n",print_devaddr);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        sprintf(spp_data, "NWKSkey:%s\n",print_nwkskey);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        sprintf(spp_data, "APPSKEY:%s\n",print_appskey);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);


                                        break;

                                case WRITE_MESH_NETWORK_PF:
                                        // {"CMD":3,"MESHID":123456,"MESHCH":13}
                                        printf("case WRITE_MESH_NETWORK_PF\n");

                                        int myvalue = cJSON_GetObjectItem(attributes,"MESHID")->valueint;

                                        const int numDigits = 6;
                                        int i = numDigits - 1;
                                        while (myvalue > 0)
                                        {
                                                int digit = myvalue % 10;
                                                myvalue /= 10;
                                                status_led.meshid[i] = digit;
                                                printf("status_led.meshid[%d] = %d\n", i, status_led.meshid[i]);
                                                i--;
                                        }

                                        status_led.meshch = cJSON_GetObjectItem(attributes,"MESHCH")->valueint;

                                        printf("status_led.meshch:%d\n",status_led.meshch);

                                        save_led(status_led);

                                        sprintf(spp_data, "Write : %s,[%d]\n",buf,param->data_ind.len);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        break;

                                case READ_MESH_NETWORK_PF:
                                        // {"CMD":4}
                                        printf("case READ_MESH_NETWORK_PF\n");

                                        read_led(&status_led);

                                        uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
                                        esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
                                        char comp_str_node[ sizeof( sta_mac )*2 +1 ] = {'\0'};
                                        size_t sta_length_node = (sizeof(sta_mac)/sizeof(sta_mac[0]));
                                        for( size_t i=0; i<sta_length_node; i++ )
                                        {
                                                sprintf( comp_str_node,"%s%02x", comp_str_node, sta_mac[i] );
                                        }

                                        sprintf(spp_data, "MAC ID:%s\n",comp_str_node);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        sprintf(spp_data, "MESH ID:%d\n",array_to_num(status_led.meshid,6));
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        sprintf(spp_data, "MESH CH:%d\n",status_led.meshch);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        break;

                                case TEST_CONTROL_SL:
                                        // {"CMD":5,"BRIGHT":10,"COLOR":3000}
                                        printf("case TEST_CONTROL_SL\n");

                                        int color_ = cJSON_GetObjectItem(attributes,"COLOR")->valueint;
                                        int bright_ = cJSON_GetObjectItem(attributes,"BRIGHT")->valueint;


                                        Set_Color(color_);
                                        Set_Brightness(bright_);
                                        status_led.mycolor = color_;
                                        status_led.mybrightness = bright_;


                                        save_led(status_led);

                                        sprintf(spp_data, "Receined characters: %s,[%d]\n",buf,param->data_ind.len);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);
                                        break;

                                case UPDATE_FW_VIA_BLE:
                                        // {"CMD":6}
                                        printf("case UPDATE_FW_VIA_BLE\n");
                                        xTaskCreate(ota_task, "ota_task", 4 * 1024,
                                                    NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
                                        break;

                                case READ_STREET_LIGHT_PF:
                                        // {"CMD":9}
                                        printf("case READ_STREET_LIGHT_PF\n");

                                        read_led(&status_led);

                                        sprintf(spp_data, "MODEL:%d watt\n",status_led.MODEL);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);
                                        switch (status_led.GEN) {
                                        case 2:
                                                sprintf(spp_data, "GEN:DIM\n");
                                                esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);
                                                break;
                                        case 1:
                                                sprintf(spp_data, "GEN:DIMCOLOR\n");
                                                esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);
                                                break;
                                        }

                                        sprintf(spp_data, "PWM LOW:%d watt\n",status_led.LOW_PWM);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        sprintf(spp_data, "PWM MAX:%d watt\n",status_led.MAX_PWM);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        break;

                                case WRITE_STREET_LIGHT_PF:
                                        // {"CMD":8,"GEN":"DIM","MODEL":45,"LOW_WATT":41,"HIGH_WATT":185}
                                        printf("case WRITE_STREET_LIGHT_PF\n");
                                        status_led.MODEL = cJSON_GetObjectItem(attributes,"MODEL")->valueint;
                                        char *mygen = cJSON_GetObjectItem(attributes,"GEN")->valuestring;
                                        status_led.LOW_PWM = cJSON_GetObjectItem(attributes,"LOW_WATT")->valueint;
                                        status_led.MAX_PWM = cJSON_GetObjectItem(attributes,"HIGH_WATT")->valueint;

                                        if (strcmp(mygen, "DIM") == 0) {         // match!
                                                printf("case READ_STREET_LIGHT_PF [DIM]\n");
                                                status_led.GEN = 2;
                                        }
                                        else if(strcmp(mygen, "DIMCOLOR") == 0) {         // match!
                                                printf("case READ_STREET_LIGHT_PF [DIMCOLOR]\n");
                                                status_led.GEN = 1;
                                        }

                                        save_led(status_led);

                                        sprintf(spp_data, "Receined characters: %s,[%d]\n",buf,param->data_ind.len);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        break;

                                case RESTART_MCU:
                                        // {"CMD":7}
                                        sprintf(spp_data, "Receined characters: %s,[%d]\n",buf,param->data_ind.len);
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        sprintf(spp_data, "MCU RESTART PLEASE WAIT..\n");
                                        esp_spp_write(param->write.handle, strlen(spp_data), (uint8_t *)spp_data);

                                        esp_restart();

                                        break;

                                }


                        }
                        cJSON_Delete(attributes);


                }
                else {
                        esp_log_buffer_hex("",param->data_ind.data,param->data_ind.len);
                }

                break;
        case ESP_SPP_CONG_EVT:
                ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
                break;
        case ESP_SPP_WRITE_EVT:
                ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
                break;
        case ESP_SPP_SRV_OPEN_EVT:
                ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT");
                break;
        default:
                break;
        }
}


void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
        switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT: {
                if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                        ESP_LOGI(SPP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
                        esp_log_buffer_hex(SPP_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
                } else {
                        ESP_LOGE(SPP_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
                }
                break;
        }
        case ESP_BT_GAP_PIN_REQ_EVT: {
                ESP_LOGI(SPP_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
                if (param->pin_req.min_16_digit) {
                        ESP_LOGI(SPP_TAG, "Input pin code: 0000 0000 0000 0000");
                        esp_bt_pin_code_t pin_code = {0};
                        esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
                } else {
                        ESP_LOGI(SPP_TAG, "Input pin code:  4321");
                        esp_bt_pin_code_t pin_code;
                        pin_code[0] = '4';
                        pin_code[1] = '3';
                        pin_code[2] = '2';
                        pin_code[3] = '1';
                        esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
                }
                break;
        }

        case ESP_BT_GAP_CFM_REQ_EVT:
                ESP_LOGI(SPP_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
                esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
                break;
        case ESP_BT_GAP_KEY_NOTIF_EVT:
                ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
                break;
        case ESP_BT_GAP_KEY_REQ_EVT:
                ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
                break;

        default: {
                ESP_LOGI(SPP_TAG, "event: %d", event);
                break;
        }
        }
        return;
}



void init_bluetooth()
{

        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
                ESP_LOGE(SPP_TAG, "%s initialize controller failed\n", __func__);
                return;
        }

        if (esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK) {
                ESP_LOGE(SPP_TAG, "%s enable controller failed\n", __func__);
                return;
        }

        if (esp_bluedroid_init() != ESP_OK) {
                ESP_LOGE(SPP_TAG, "%s initialize bluedroid failed\n", __func__);
                return;
        }

        if (esp_bluedroid_enable() != ESP_OK) {
                ESP_LOGE(SPP_TAG, "%s enable bluedroid failed\n", __func__);
                return;
        }

        //add password bluetooth
        if (esp_bt_gap_register_callback(esp_bt_gap_cb) != ESP_OK) {
                ESP_LOGE(SPP_TAG, "%s gap register failed\n", __func__);
                return;
        }

        if (esp_spp_register_callback(esp_spp_cb) != ESP_OK) {
                ESP_LOGE(SPP_TAG, "%s spp register failed\n", __func__);
                return;
        }

        if (esp_spp_init(esp_spp_mode) != ESP_OK) {
                ESP_LOGE(SPP_TAG, "%s spp init failed\n", __func__);
                return;
        }

        //add password bluetooth
        esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
        esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
        esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
        /*
         * Set default parameters for Legacy Pairing
         * Use variable pin, input pin code when pairing
         */
        esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
        esp_bt_pin_code_t pin_code;
        esp_bt_gap_set_pin(pin_type, 0, pin_code);


}
