// Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
  #endif

  #include "esp_types.h"
  #include "esp_event.h"
  #include "esp_err.h"
  #include "driver/uart.h"


#define MAX_SEND_FAIL 10
//network connection LoraWAN
char* devaddr;
char* nwkskey;
char* appskey;
char print_devaddr[32];
char print_nwkskey[64];
char print_appskey[64];

/**
 * @brief Configuration of murata Parser
 *
 */
typedef struct {
        struct {
                uart_port_t uart_port; /*!< UART port number */
                uint32_t tx_pin;    /*!< UART Rx Pin number */
                uint32_t rx_pin;    /*!< UART Rx Pin number */
                uint32_t baud_rate; /*!< UART baud rate */
                uart_word_length_t data_bits; /*!< UART data bits length */
                uart_parity_t parity; /*!< UART parity */
                uart_stop_bits_t stop_bits; /*!< UART stop bits length */
                uint32_t event_queue_size; /*!< UART event queue size */
        } uart;                       /*!< UART specific configuration */
} murata_parser_config_t;

/**
 * @brief murata Parser Handle
 *
 */
typedef void *murata_parser_handle_t;

/**
 * @brief Default configuration for murata Parser
 *
 */
  #define murata_PARSER_CONFIG_DEFAULT()       \
        {                                      \
                .uart = {                          \
                        .uart_port = UART_NUM_1,       \
                        .tx_pin = 17,                  \
                        .rx_pin = 16,                  \
                        .baud_rate = 9600,             \
                        .data_bits = UART_DATA_8_BITS, \
                        .parity = UART_PARITY_DISABLE, \
                        .stop_bits = UART_STOP_BITS_1, \
                        .event_queue_size = 16         \
                }                                  \
        }

/**
 * @brief murata Parser Event ID
 *
 */
typedef enum {
        LORAWAN_UPDATE, /*!< LORAWAN information has been updated */
        LORAWAN_UNKNOWN /*!< Unknown statements detected */
} murata_event_id_t;

/**
 * @brief Init murata Parser
 *
 * @param config Configuration of murata Parser
 * @return murata_parser_handle_t handle of murata parser
 */
murata_parser_handle_t murata_parser_init(const murata_parser_config_t *config);

/**
 * @brief Deinit murata Parser
 *
 * @param murata_hdl handle of murata parser
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
esp_err_t murata_parser_deinit(murata_parser_handle_t murata_hdl);

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
esp_err_t murata_parser_add_handler(murata_parser_handle_t murata_hdl, esp_event_handler_t event_handler, void *handler_args);

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
esp_err_t murata_parser_remove_handler(murata_parser_handle_t murata_hdl, esp_event_handler_t event_handler);

void init_LoraWAN(void);
void lora_init(void *arg);
int sendData(const char* logName, const char* data);
void sendCMD( const char* logName, const char* data, bool chk_ret);

extern void cm_lorawan_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);


typedef enum eATEerror
{
        AT_OK = 0,
        AT_ERROR,
        AT_PARAM_ERROR,
        AT_BUSY_ERROR,
        AT_TEST_PARAM_OVERFLOW,
        AT_NO_NET_JOINED,
        AT_RX_ERROR,
        AT_NO_CLASS_B_ENABLE,
        AT_MAX,
        AT_UNKNOW,

        ESP_QUEUE_TOO_SMALL,
} ATEerror_t;

static const char *const ATError_description[] =
{
        "OK\r\n",                 /* AT_OK */
        "AT_ERROR\r\n",           /* AT_ERROR */
        "AT_PARAM_ERROR\r\n",     /* AT_PARAM_ERROR */
        "AT_BUSY_ERROR\r\n",      /* AT_BUSY_ERROR */
        "AT_TEST_PARAM_OVERFLOW\r\n", /* AT_TEST_PARAM_OVERFLOW */
        "AT_NO_NETWORK_JOINED\r\n", /* AT_NO_NET_JOINED */
        "AT_RX_ERROR\r\n",        /* AT_RX_ERROR */
        "AT_NO_CLASS_B_ENABLE\r\n", /* AT_NO_CLASS_B_ENABLE */
        "error unknown\r\n",      /* AT_MAX */
};

  #define AT_START          "AT\n"
  #define AT_CLASS_C        "AT+CLASS=C\n"
  #define AT_DADDR          "AT+DADDR=%s\n"
  #define AT_NWKSKEY        "AT+NWKSKEY=%s\n"
  #define AT_APPSKEY        "AT+APPSKEY=%s\n"
  #define AT_SENDB          "AT+SENDB=%d:"
  #define AT_CFM            "AT+CFM=0\n"
  #define AT_DCS            "AT+DCS=0\n"
  #define AT_TXP            "AT+TXP=5\n"

/* Data Rate	Configuration	bits/s 	Max payload
DR0	SF12/125kHz	250	59
DR1	SF11/125kHz	440	59
DR2	SF10/125kHz	980	59
DR3	SF9/125kHz	1 760	123
DR4	SF8/125kHz	3 125	230
DR5	SF7/125kHz	5 470	230
DR6	SF7/250kHz	11 000	230
DR7	FSK: 50kpbs	50 000	230
*/

  #define AT_DR             "AT+DR=3\n"
  #define AT_ADR            "AT+ADR=1\n"
  #define AT_JN1DL          "AT+JN1DL=5000\n"
  #define AT_JN2DL          "AT+JN2DL=6000\n"
  #define AT_RX1DL          "AT+RX1DL=1000\n"
  #define AT_RX2DL          "AT+RX2DL=2000\n"
  #define AT_NJM            "AT+NJM=0\n"
  #define AT_NJS            "AT+NJS=?\n"
  #define AT_VER            "AT+VER=?\n"
  #define AT_RST            "ATZ\n"
  #define AT_RECVB          "AT+RECVB=?\n"


  #ifdef __cplusplus
}
#endif
