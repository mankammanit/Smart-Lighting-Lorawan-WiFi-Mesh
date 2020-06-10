
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "murata_parser.h"
#include "st_profile.h"
#include "nvs_storage.h"

/**
 * @brief murata Parser runtime buffer size
 *
 */
#define murata_PARSER_RUNTIME_BUFFER_SIZE (CONFIG_murata_PARSER_RING_BUFFER_SIZE / 2)
#define murata_MAX_STATEMENT_ITEM_LENGTH (16)
#define murata_EVENT_LOOP_QUEUE_SIZE (16)

/**
 * @brief Define of murata Parser Event base
 *
 */
ESP_EVENT_DEFINE_BASE(ESP_MURATA_EVENT);

static const char *CM_TAG = "LoraWAN";

/**
 * @brief GPS parser library runtime structure
 *
 */
typedef struct {
        uint8_t item_pos;                          /*!< Current position in item */
        uint8_t item_num;                          /*!< Current item number */
        uint8_t asterisk;                          /*!< Asterisk detected flag */
        uint8_t crc;                               /*!< Calculated CRC value */
        uint8_t parsed_statement;                  /*!< OR'd of statements that have been parsed */
        uint8_t sat_num;                           /*!< Satellite number */
        uint8_t sat_count;                         /*!< Satellite count */
        uint8_t cur_statement;                     /*!< Current statement ID */
        uint32_t all_statements;                   /*!< All statements mask */
        char item_str[murata_MAX_STATEMENT_ITEM_LENGTH]; /*!< Current item */
        //cm_t parent;                                  /*!< Parent class */
        uart_port_t uart_port;                     /*!< Uart port number */
        uint8_t *buffer;                           /*!< Runtime buffer */
        esp_event_loop_handle_t event_loop_hdl;    /*!< Event loop handle */
        TaskHandle_t tsk_hdl;                      /*!< murata Parser task handle */
        QueueHandle_t event_queue;                 /*!< UART event queue handle */
} esp_cmlorawan_t;

bool recv_status = false;



ATEerror_t AT_Decode( const uint8_t *data){
        ATEerror_t ret;
        for( ret = AT_OK; ret < AT_UNKNOW; ret++) {
                if(strcmp((char*)data, ATError_description[ret]) == 0) {
                        return ret;
                }
        }
        return AT_UNKNOW;
}

static ATEerror_t Murata_decode(esp_cmlorawan_t *lorawan, size_t len)
{
        // const uint8_t *d = lorawan->buffer;

        ATEerror_t ret = AT_Decode(lorawan->buffer);

        // printf("ret: [%d] %s\n", ret, ATError_description[ret]);

        switch (ret) {
        case AT_OK: printf("ret: %s\n", ATError_description[ret]);

                // esp_event_post_to(lorawan->event_loop_hdl, ESP_MURATA_EVENT, GPS_UNKNOWN,
                //                   lorawan->buffer, read_len, 100 / portTICK_PERIOD_MS);
                return AT_OK;
                break;

        case AT_ERROR: printf("ret: %s\n", ATError_description[ret]);
                return AT_ERROR;
                break;

        case AT_PARAM_ERROR: printf("ret: %s\n", ATError_description[ret]);
                return AT_PARAM_ERROR;
                break;

        case AT_BUSY_ERROR: printf("ret: %s\n", ATError_description[ret]);
                return AT_BUSY_ERROR;
                break;

        case AT_TEST_PARAM_OVERFLOW: printf("ret: %s\n", ATError_description[ret]);
                return AT_TEST_PARAM_OVERFLOW;
                break;

        case AT_NO_NET_JOINED: printf("ret: %s\n", ATError_description[ret]);
                return AT_NO_NET_JOINED;
                break;

        case AT_RX_ERROR: printf("ret: %s\n", ATError_description[ret]);
                return AT_RX_ERROR;
                break;

        case AT_NO_CLASS_B_ENABLE: printf("ret: %s\n", ATError_description[ret]);
                return AT_NO_CLASS_B_ENABLE;
                break;

        case AT_MAX: printf("ret: %s\n", ATError_description[ret]);
                return AT_MAX;
                break;

        case AT_UNKNOW: return AT_UNKNOW;
                break;

        default: return AT_MAX;
                break;
        }


}

/**
 * @brief Handle when a pattern has been detected by uart
 *
 * @param lorawan esp_cmlorawan_t type object
 */
static ATEerror_t esp_handle_uart_pattern(esp_cmlorawan_t *lorawan)
{
        ATEerror_t ret = AT_UNKNOW;

        int pos = uart_pattern_pop_pos(lorawan->uart_port);
        if (pos != -1) {

                // ESP_LOGW(CM_TAG, "Pattern Queue Size");
                /* read one line(include '\n') */
                int read_len = uart_read_bytes(lorawan->uart_port, lorawan->buffer, pos + 1, 100 / portTICK_PERIOD_MS);
                /* make sure the line is a standard string */
                lorawan->buffer[read_len] = '\0';
                // ESP_LOGW(CM_TAG, "%s", lorawan->buffer);

                if(strchr((char *)lorawan->buffer, ':') != NULL) {
                        char *tmp = calloc(51, sizeof(char));
                        int port= 0;

                        sscanf((char *)lorawan->buffer, "%d:%s", &port, tmp);

                        if(strlen(tmp) != 0) {
                                printf("%d : %s\n", port, tmp);
                                esp_event_post_to(lorawan->event_loop_hdl, ESP_MURATA_EVENT, LORAWAN_UPDATE,
                                                  tmp, read_len, 100 / portTICK_PERIOD_MS);
                        }

                        free(tmp);
                }


                // for(uint8_t i = 0; i < read_len; i++){
                //   printf("%02x ", lorawan->buffer[i]);
                // }
                // printf("\n");

                /* Send new line to handle */
                ret = Murata_decode(lorawan, read_len + 1);

                if(ret == AT_UNKNOW) {

                }

                return ret;


        } else {
                ESP_LOGW(CM_TAG, "Pattern Queue Size too small");
                uart_flush_input(lorawan->uart_port);
        }

        return ESP_QUEUE_TOO_SMALL;
}

/**
 * @brief murata Parser Task Entry
 *
 * @param arg argument
 */
 #define RD_BUF_SIZE (1024)
static void murata_parser_task_entry(void *arg)
{
        esp_cmlorawan_t *lorawan = (esp_cmlorawan_t *)arg;
        ATEerror_t ret;
        uart_event_t event;
        uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
        while (1) {
                if (xQueueReceive(lorawan->event_queue, &event, pdMS_TO_TICKS(200))) {
                        bzero(dtmp, RD_BUF_SIZE);
                        switch (event.type) {
                        case UART_DATA:
                                ESP_LOGI(CM_TAG, "[UART DATA]: %d", event.size);
                                uart_read_bytes(lorawan->uart_port, dtmp, event.size, portMAX_DELAY);
                                printf("%s\n", (char*)dtmp);
                                break;
                        case UART_FIFO_OVF:
                                ESP_LOGW(CM_TAG, "HW FIFO Overflow");
                                uart_flush(lorawan->uart_port);
                                xQueueReset(lorawan->event_queue);
                                break;
                        case UART_BUFFER_FULL:
                                ESP_LOGW(CM_TAG, "Ring Buffer Full");
                                uart_flush(lorawan->uart_port);
                                xQueueReset(lorawan->event_queue);
                                break;
                        case UART_BREAK:
                                ESP_LOGW(CM_TAG, "Rx Break");
                                break;
                        case UART_PARITY_ERR:
                                ESP_LOGE(CM_TAG, "Parity Error");
                                break;
                        case UART_FRAME_ERR:
                                ESP_LOGE(CM_TAG, "Frame Error");
                                break;
                        case UART_PATTERN_DET:
                                // ESP_LOGW(CM_TAG, "event type: %d", event.type);
                                ret = esp_handle_uart_pattern(lorawan);
                                if(ret == AT_OK) {
                                        recv_status = true;
                                }

                                break;
                        default:
                                ESP_LOGW(CM_TAG, "unknown uart event type: %d", event.type);
                                break;
                        }
                }
                /* Drive the event loop */
                esp_event_loop_run(lorawan->event_loop_hdl, pdMS_TO_TICKS(50));
        }
        vTaskDelete(NULL);

        // return false;
}


/**
 * @brief Init murata Parser
 *
 * @param config Configuration of murata Parser
 * @return murata_parser_handle_t handle of murata_parser
 */
murata_parser_handle_t murata_parser_init(const murata_parser_config_t *config)
{
        esp_cmlorawan_t *lorawan = calloc(1, sizeof(esp_cmlorawan_t));
        if (!lorawan) {
                ESP_LOGE(CM_TAG, "calloc memory for esp_fps failed");
                goto err_cm;
        }
        lorawan->buffer = calloc(1, murata_PARSER_RUNTIME_BUFFER_SIZE);
        if (!lorawan->buffer) {
                ESP_LOGE(CM_TAG, "calloc memory for runtime buffer failed");
                goto err_buffer;
        }

        /* Set attributes */
        lorawan->uart_port = config->uart.uart_port;
        lorawan->all_statements &= 0xFE;
        /* Install UART friver */
        uart_config_t uart_config = {
                .baud_rate = config->uart.baud_rate,
                .data_bits = config->uart.data_bits,
                .parity = config->uart.parity,
                .stop_bits = config->uart.stop_bits,
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
        };
        if (uart_param_config(lorawan->uart_port, &uart_config) != ESP_OK) {
                ESP_LOGE(CM_TAG, "config uart parameter failed");
                goto err_uart_config;
        }
        if (uart_set_pin(lorawan->uart_port, config->uart.tx_pin, config->uart.rx_pin,
                         UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
                ESP_LOGE(CM_TAG, "config uart gpio failed");
                goto err_uart_config;
        }
        if (uart_driver_install(lorawan->uart_port, CONFIG_murata_PARSER_RING_BUFFER_SIZE, 0,
                                config->uart.event_queue_size, &lorawan->event_queue, 0) != ESP_OK) {
                ESP_LOGE(CM_TAG, "install uart driver failed");
                goto err_uart_install;
        }
        /* Set pattern interrupt, used to detect the end of a line */
        uart_enable_pattern_det_intr(lorawan->uart_port, '\n', 1, 10000, 10, 10);
        /* Set pattern queue size */
        uart_pattern_queue_reset(lorawan->uart_port, config->uart.event_queue_size);
        uart_flush(lorawan->uart_port);
        /* Create Event loop */
        esp_event_loop_args_t loop_args = {
                .queue_size = murata_EVENT_LOOP_QUEUE_SIZE,
                .task_name = NULL
        };
        if (esp_event_loop_create(&loop_args, &lorawan->event_loop_hdl) != ESP_OK) {
                ESP_LOGE(CM_TAG, "create event loop faild");
                goto err_eloop;
        }
        /* Create murata Parser task */
        BaseType_t err = xTaskCreate(
                murata_parser_task_entry,
                "murata_parser",
                CONFIG_murata_PARSER_TASK_STACK_SIZE,
                lorawan,
                CONFIG_murata_PARSER_TASK_PRIORITY,
                &lorawan->tsk_hdl);
        if (err != pdTRUE) {
                ESP_LOGE(CM_TAG, "create murata Parser task failed");
                goto err_task_create;
        }
        ESP_LOGI(CM_TAG, "murata Parser init OK");
        return lorawan;
        /*Error Handling*/
err_task_create:
        esp_event_loop_delete(lorawan->event_loop_hdl);
err_eloop:
err_uart_install:
        uart_driver_delete(lorawan->uart_port);
err_uart_config:
err_buffer:
        free(lorawan->buffer);
err_cm:
        free(lorawan);
        return NULL;
}


static void lora_rst(){
        gpio_set_level(5, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(5, 1);

        sendCMD("lora_rst", AT_START, true);
        sendCMD("lora_rst", AT_RST, false);
}

void lora_init(void *arg){
        // murata_parser_handle_t murata_hdl = (murata_parser_handle_t *)arg;
        static const char *TX_TASK_TAG = "TX_TASK";
        esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);

        lora_rst();

        char buf_cmd[64];
        char load_devaddr[strlen("00:00:00:00") + 1];
        char load_nwkskey[strlen("00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00") + 1];
        char load_appskey[strlen("00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00") + 1];
        size_t buf_len_devaddr = sizeof(load_devaddr);
        size_t buf_len_nwkskey = sizeof(load_nwkskey);
        size_t buf_len_appskey = sizeof(load_appskey);
        get_string("DEVADDR",load_devaddr,&buf_len_devaddr);
        get_string("NWKSKEY",load_nwkskey,&buf_len_nwkskey);
        get_string("APPSKEY",load_appskey,&buf_len_appskey);

        sendCMD(TX_TASK_TAG, AT_START, true);
        sendCMD(TX_TASK_TAG, AT_CLASS_C, true);

        sprintf(buf_cmd, AT_DADDR, load_devaddr);
        sprintf(print_devaddr,"%s",load_devaddr);
        sendCMD(TX_TASK_TAG, buf_cmd, true);

        sprintf(buf_cmd, AT_NWKSKEY, load_nwkskey);
        sprintf(print_nwkskey,"%s",load_nwkskey);
        sendCMD(TX_TASK_TAG, buf_cmd, true);

        sprintf(buf_cmd, AT_APPSKEY, load_appskey);
        sprintf(print_appskey,"%s",load_appskey);
        sendCMD(TX_TASK_TAG, buf_cmd, true);

        sendCMD(TX_TASK_TAG, AT_CFM, true);
        sendCMD(TX_TASK_TAG, AT_DCS, true);
        sendCMD(TX_TASK_TAG, AT_TXP, true);
        sendCMD(TX_TASK_TAG, AT_DR, true);
        sendCMD(TX_TASK_TAG, AT_ADR, true);
        sendCMD(TX_TASK_TAG, AT_JN1DL, true);
        sendCMD(TX_TASK_TAG, AT_JN2DL, true);
        sendCMD(TX_TASK_TAG, AT_RX1DL, true);
        sendCMD(TX_TASK_TAG, AT_RX2DL, true);

        sendCMD(TX_TASK_TAG, AT_NJM, true);
        sendCMD(TX_TASK_TAG, AT_NJS, true);
}

int sendData(const char* logName, const char* data)
{
        const int len = strlen(data);
        const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
        ESP_LOGI(logName, "Wrote %d bytes", txBytes);
        return txBytes;
}

static EventGroupHandle_t s_wifi_event_group;
static const int RECV_BIT = BIT0;
static const int SEND_BIT = BIT1;
void sendCMD( const char* logName, const char* data, bool chk_ret)
{
        const int len = strlen(data);
        recv_status = false;
        uint8_t send_fail = 0;

        xEventGroupWaitBits(s_wifi_event_group, RECV_BIT, false, false, portMAX_DELAY);


        xEventGroupClearBits(s_wifi_event_group, SEND_BIT);
        do {
                const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
                ESP_LOGI(logName, "[%d] %s Wrote %d bytes", send_fail, data, txBytes );
                // for(uint8_t i =0; i<len; i++){
                //   printf("%x ", data[i]);
                // }
                // ret = murata_parser_task_entry(lorawan);
                vTaskDelay(500 / portTICK_PERIOD_MS);
                send_fail = send_fail + 1;
        } while(!recv_status && chk_ret && send_fail < MAX_SEND_FAIL);

        if(send_fail >= MAX_SEND_FAIL ) {
                lora_init(NULL);
                sendCMD(logName, data, chk_ret);
        }

        xEventGroupSetBits(s_wifi_event_group, SEND_BIT);
}

void readCMD(){

        uint8_t send_fail =0;
        while(1) {
                xEventGroupWaitBits(s_wifi_event_group, SEND_BIT, false, false, portMAX_DELAY);
                xEventGroupClearBits(s_wifi_event_group, RECV_BIT);
                send_fail = 0;


                do {
                        const int txBytes = uart_write_bytes(UART_NUM_1, AT_RECVB, strlen(AT_RECVB));
                        ESP_LOGI("RECV", "[%d] %s Wrote %d bytes", send_fail, AT_RECVB, strlen(AT_RECVB) );
                        vTaskDelay(500 / portTICK_PERIOD_MS);
                        send_fail = send_fail + 1;
                } while(!recv_status && send_fail < MAX_SEND_FAIL);

                if(send_fail >= MAX_SEND_FAIL ) {
                        lora_init(NULL);
                }


                xEventGroupSetBits(s_wifi_event_group, RECV_BIT);

                vTaskDelay(1000 / portTICK_RATE_MS);
        }

}




void init_LoraWAN(void)
{
        /* murata parser configuration */
        murata_parser_config_t config = murata_PARSER_CONFIG_DEFAULT();
        /* init murata parser library */
        murata_parser_handle_t murata_hdl = murata_parser_init(&config);
        s_wifi_event_group = xEventGroupCreate();
        xEventGroupSetBits(s_wifi_event_group, RECV_BIT);
        /* register event handler for murata parser library */
        murata_parser_add_handler(murata_hdl, cm_lorawan_event_handler, NULL);

        lora_init(NULL);

        xTaskCreate(readCMD, "readCMD", 1024*2, readCMD, 4, NULL);

}

/**
 * @brief Deinit murata Parser
 *
 * @param murata_hdl handle of murata parser
 * @return esp_err_t ESP_OK on success,ESP_FAIL on error
 */
esp_err_t murata_parser_deinit(murata_parser_handle_t murata_hdl)
{
        esp_cmlorawan_t *lorawan = (esp_cmlorawan_t *)murata_hdl;
        vTaskDelete(lorawan->tsk_hdl);
        esp_event_loop_delete(lorawan->event_loop_hdl);
        esp_err_t err = uart_driver_delete(lorawan->uart_port);
        free(lorawan->buffer);
        free(lorawan);
        return err;
}

/**
 * @brief Add user defined handler for murata parser
 *
 * @param murata_hdl handle of murata parser
 * @param event_handler user defined event handler
 * @param handler_args handler specific arguments
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_ERR_NO_MEM: Cannot allocate memory for the handler
 *  - ESP_ERR_INVALIG_ARG: Invalid combination of event base and event id
 *  - Others: Fail
 */
esp_err_t murata_parser_add_handler(murata_parser_handle_t murata_hdl, esp_event_handler_t event_handler, void *handler_args)
{
        esp_cmlorawan_t *lorawan = (esp_cmlorawan_t *)murata_hdl;
        return esp_event_handler_register_with(lorawan->event_loop_hdl, ESP_MURATA_EVENT, ESP_EVENT_ANY_ID,
                                               event_handler, handler_args);
}

/**
 * @brief Remove user defined handler for murata parser
 *
 * @param murata_hdl handle of murata parser
 * @param event_handler user defined event handler
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_ERR_INVALIG_ARG: Invalid combination of event base and event id
 *  - Others: Fail
 */
esp_err_t murata_parser_remove_handler(murata_parser_handle_t murata_hdl, esp_event_handler_t event_handler)
{
        esp_cmlorawan_t *lorawan = (esp_cmlorawan_t *)murata_hdl;
        return esp_event_handler_unregister_with(lorawan->event_loop_hdl, ESP_MURATA_EVENT, ESP_EVENT_ANY_ID, event_handler);
}
