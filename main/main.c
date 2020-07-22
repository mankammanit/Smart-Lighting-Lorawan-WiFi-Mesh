/*
Project : SmartStreetlight LoRaWan + WiFiMesh
Type	  : MASTER
Author  : Kammanit Jitkul
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
#include "cJSON.h"

//cm lib
#include "CMLPP.h"
#include "mdf_common.h"
#include "mwifi.h"
#include "mupgrade.h"
#include "murata_parser.h"
#include "CMLPPDec.h"
#include "str2hex.h"
#include "queue.h"
#include "nvs_storage.h"
#include "led_driver.h"
#include "si_7006.h"
#include "nmea_parser.h"
#include "voltage.h"


#define URL_OTA "http://183.88.218.59:5090/fw_streetlight/LoraWAN-MASTER.bin"

static const char *TAG                       = "LIGHTING";
#define RANDOM(min, max) rand() % (max + 1 - min) + min

static char data_form_node[128]="";

uint8_t counter_sec;
float voltage_sl = 0.0;

uint8_t retry_ = 0;

char recvb_form_ble[32]="";
bool ble_cmd_to_gr = false;


extern void init_bluetooth();


void ota_task()
{
								mdf_err_t ret       = MDF_OK;
								uint8_t *data       = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
								char name[32]       = {0x0};
								size_t total_size   = 0;
								int start_time      = 0;
								mupgrade_result_t upgrade_result = {0};
								mwifi_data_type_t data_type = {.communicate = MWIFI_COMMUNICATE_MULTICAST};

								/**
								 * @note If you need to upgrade all devices, pass MWIFI_ADDR_ANY;
								 *       If you upgrade the incoming address list to the specified device
								 */
								// uint8_t dest_addr[][MWIFI_ADDR_LEN] = {{0x1, 0x1, 0x1, 0x1, 0x1, 0x1}, {0x2, 0x2, 0x2, 0x2, 0x2, 0x2},};
								uint8_t dest_addr[][MWIFI_ADDR_LEN] = {MWIFI_ADDR_ANY};

								/**
								 * @brief In order to allow more nodes to join the mesh network for firmware upgrade,
								 *      in the example we will start the firmware upgrade after 30 seconds.
								 */
								vTaskDelay(10 * 1000 / portTICK_PERIOD_MS);

								esp_http_client_config_t config = {
																.url            = URL_OTA,
																.transport_type = HTTP_TRANSPORT_UNKNOWN,
								};

								/**
								 * @brief 1. Connect to the server
								 */
								esp_http_client_handle_t client = esp_http_client_init(&config);
								MDF_ERROR_GOTO(!client, EXIT, "Initialise HTTP connection");

								start_time = xTaskGetTickCount();

								MDF_LOGI("Open HTTP connection: %s", URL_OTA);

								/**
								 * @brief First, the firmware is obtained from the http server and stored on the root node.
								 */
								do {
																ret = esp_http_client_open(client, 0);

																if (ret != MDF_OK) {
																								if (!esp_mesh_is_root()) {
																																goto EXIT;
																								}

																								vTaskDelay(pdMS_TO_TICKS(1000));
																								MDF_LOGW("<%s> Connection service failed", mdf_err_to_name(ret));
																}
								} while (ret != MDF_OK);

								total_size = esp_http_client_fetch_headers(client);
								sscanf(URL_OTA, "%*[^//]//%*[^/]/%[^.bin]", name);

								if (total_size <= 0) {
																MDF_LOGW("Please check the address of the server");
																ret = esp_http_client_read(client, (char *)data, MWIFI_PAYLOAD_LEN);
																MDF_ERROR_GOTO(ret < 0, EXIT, "<%s> Read data from http stream", mdf_err_to_name(ret));

																MDF_LOGW("Recv data: %.*s", ret, data);
																goto EXIT;
								}

								/**
								 * @brief 2. Initialize the upgrade status and erase the upgrade partition.
								 */
								ret = mupgrade_firmware_init(name, total_size);
								MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> Initialize the upgrade status", mdf_err_to_name(ret));

								/**
								 * @brief 3. Read firmware from the server and write it to the flash of the root node
								 */
								for (ssize_t size = 0, recv_size = 0; recv_size < total_size; recv_size += size) {
																size = esp_http_client_read(client, (char *)data, MWIFI_PAYLOAD_LEN);
																MDF_ERROR_GOTO(size < 0, EXIT, "<%s> Read data from http stream", mdf_err_to_name(ret));

																if (size > 0) {
																								/* @brief  Write firmware to flash */
																								ret = mupgrade_firmware_download(data, size);
																								MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> Write firmware to flash, size: %d, data: %.*s",
																																							mdf_err_to_name(ret), size, size, data);
																} else {
																								MDF_LOGW("<%s> esp_http_client_read", mdf_err_to_name(ret));
																								goto EXIT;
																}
								}

								MDF_LOGI("The service download firmware is complete, Spend time: %ds",
																	(xTaskGetTickCount() - start_time) * portTICK_RATE_MS / 1000);

								start_time = xTaskGetTickCount();

								/**
								 * @brief 4. The firmware will be sent to each node.
								 */
								ret = mupgrade_firmware_send((uint8_t *)dest_addr, sizeof(dest_addr) / MWIFI_ADDR_LEN, &upgrade_result);
								MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> mupgrade_firmware_send", mdf_err_to_name(ret));

								if (upgrade_result.successed_num == 0) {
																MDF_LOGW("Devices upgrade failed, unfinished_num: %d", upgrade_result.unfinished_num);
																goto EXIT;
								}

								MDF_LOGI("Firmware is sent to the device to complete, Spend time: %ds",
																	(xTaskGetTickCount() - start_time) * portTICK_RATE_MS / 1000);
								MDF_LOGI("Devices upgrade completed, successed_num: %d, unfinished_num: %d", upgrade_result.successed_num, upgrade_result.unfinished_num);

								/**
								 * @brief 5. the root notifies nodes to restart
								 */
								const char *restart_str = "restart";
								ret = mwifi_root_write(upgrade_result.successed_addr, upgrade_result.successed_num,
																															&data_type, restart_str, strlen(restart_str), true);
								MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> mwifi_root_recv", mdf_err_to_name(ret));

EXIT:
								MDF_FREE(data);
								mupgrade_result_free(&upgrade_result);
								esp_http_client_close(client);
								esp_http_client_cleanup(client);
								vTaskDelete(NULL);
}

//add 96b4e62dde26319718e829611dca870a8c6f0a67014068c6740e9c80000992009301940595406a000000003684
void add_queue_lpp_root()
{
								struct CayenneLPP *lpp;
								unsigned char *buf;
								int size=0;
								int i,j;
								uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
								uint8_t sta_parent[MWIFI_ADDR_LEN] = {0};
								mesh_addr_t parent_bssid        = {0};

								//get node id
								esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
								char comp_str_node[ sizeof( sta_mac )*2 +1 ] = {'\0'};
								size_t sta_length_node = (sizeof(sta_mac)/sizeof(sta_mac[0]));
								for( size_t i=0; i<sta_length_node; i++ )
								{
																sprintf( comp_str_node,"%s%02x", comp_str_node, sta_mac[i] );
																// printf("%s\n",comp_str);
								}
								// printf("NODE ID ---> %s\n",comp_str_node);

								//get parent id
								esp_mesh_get_parent_bssid(&parent_bssid);
								memcpy(sta_parent, parent_bssid.addr, MWIFI_ADDR_LEN);
								char comp_str_parent[ sizeof( sta_parent )*2 +1 ] = {'\0'};
								size_t sta_length_parent = (sizeof(sta_parent)/sizeof(sta_parent[0]));
								for( size_t i=0; i<sta_length_parent; i++ )
								{
																sprintf( comp_str_parent,"%s%02x", comp_str_parent, sta_parent[i] );
																// printf("%s\n",comp_str);
								}
								// printf("PARENT ID ---> %s\n",comp_str_parent);


								lpp = CayenneLPP__create(51);

								CayenneLPP__addNodeID(lpp,comp_str_node);

								CayenneLPP__addParentID(lpp,comp_str_parent);

								CayenneLPP__addColor(lpp,status_led.mycolor);

								CayenneLPP__addLightControl(lpp,status_led.mybrightness);

								CayenneLPP__addTemperature(lpp,si7021_read_temperature());

								CayenneLPP__addRelativeHumidity(lpp,(int)si7021_read_humidity());

								//ค่าแรงดัน
								CayenneLPP__addVoltage(lpp,(voltage_sl*10.00));

								//ค่า WATT
								CayenneLPP__addPower(lpp,status_led.mybrightness * status_led.MODEL / 100);

								//Error Code
								//แรงดันเกิน
								if ((voltage_sl - 3.00) > 41.00) CayenneLPP__addError_code(lpp,1);
								//แรงดันต่ำ
								else if ((voltage_sl - 3.00) < 28.00) CayenneLPP__addError_code(lpp,2);
								//อุณหภูมิสูง
								else if (si7021_read_temperature() > 80.00) CayenneLPP__addError_code(lpp,3);
								//ปกติ
								else CayenneLPP__addError_code(lpp,0);

								//Dim 10 - 100% & Adj CCT 2700-6500 , 2 = Dim 10 - 100% only & not Adj CCT
								CayenneLPP__addGen(lpp,status_led.GEN);

								switch (status_led.MODEL) {
								case 45:
																CayenneLPP__addModel(lpp,1);
																break;
								case 70:
																CayenneLPP__addModel(lpp,2);
																break;
								case 130:
																CayenneLPP__addModel(lpp,3);
																break;
								case 120:
																CayenneLPP__addModel(lpp,4);
																break;
								case 90:
																CayenneLPP__addModel(lpp,5);
																break;
								}

								//ค่าความแรงสัญญาณ wifi-mesh
								CayenneLPP__addPowerINDEX(lpp,abs(mwifi_get_parent_rssi()));

								//เวลาการทำงาน
								CayenneLPP__addActuation(lpp,status_led.ledworking);

								buf=CayenneLPP__getBuffer(lpp);
								size=CayenneLPP__getSize(lpp);

								/*set strH with nulls*/
								unsigned char strH[size*2];
								memset(strH,0,sizeof(strH));

								/*converting str character into Hex and adding into strH*/
								for(i=0,j=0; i<size; i++,j+=2)
								{
																sprintf((char*)strH+j,"%02X",buf[i]);
								}
								strH[j]='\0'; /*adding NULL in the end*/

								// printf("Hexadecimal converted string is: \n");
								// printf("%s\n",strH);
								enqueue((char*)strH);
								ESP_LOGI("MSGQUEUE", "ADD QUEUE DATA STREETLIGHT ROOT");

								free(buf);
								free(lpp);

}

//add 96 b4e62dde2631 88 021412 0f4e00 000793
void add_queue_infomation() {

								struct CayenneLPP *lpp;
								unsigned char *buf;
								int size=0;
								int i,j;
								uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};

								//get node id
								esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
								char comp_str_node[ sizeof( sta_mac )*2 +1 ] = {'\0'};
								size_t sta_length_node = (sizeof(sta_mac)/sizeof(sta_mac[0]));
								for( size_t i=0; i<sta_length_node; i++ )
								{
																sprintf( comp_str_node,"%s%02x", comp_str_node, sta_mac[i] );
																// printf("%s\n",comp_str);
								}
								// printf("NODE ID ---> %s\n",comp_str_node);

								lpp = CayenneLPP__create(51);

								CayenneLPP__addNodeID(lpp,comp_str_node);

								gps_t gps_info = get_gps_info();
								CayenneLPP__addGPS(lpp,gps_info.latitude,gps_info.longitude,0.0);

								CayenneLPP__addMESHID(lpp,array_to_num(status_led.meshid,6));

								CayenneLPP__addMESHCH(lpp,status_led.meshch);

								buf=CayenneLPP__getBuffer(lpp);
								size=CayenneLPP__getSize(lpp);

								/*set strH with nulls*/
								unsigned char strH[size*2];
								memset(strH,0,sizeof(strH));

								/*converting str character into Hex and adding into strH*/
								for(i=0,j=0; i<size; i++,j+=2)
								{
																sprintf((char*)strH+j,"%02X",buf[i]);
								}
								strH[j]='\0'; /*adding NULL in the end*/

								// printf("Hexadecimal converted string is: \n");
								// printf("%s\n",strH);
								enqueue((char*)strH);
								ESP_LOGI("MSGQUEUE", "ADD QUEUE GPS&MESH ROOT");

								free(buf);
								free(lpp);
}


void add_queue_lpp_node(char* node_info)
{
								static char node_id_compare[12];
								//test parser value lpp node id
								char *tmp = NULL;
								if(strcmp(node_info, "") != 0)
								{
																// tmp = (char *)node_info;
																// printf("****** %s ********\n", tmp);
																tmp = test((char *)node_info);
																cJSON *Payload = NULL;
																ParseCMLPP ((uint8_t*)tmp, strlen(tmp), &Payload);
																char* text = cJSON_Print(Payload);
																printf("ENQUEUE NODE %s\n", text);
																free(text);

																if (Payload != NULL)

																{
																								cJSON *parent_mac = cJSON_GetObjectItemCaseSensitive(Payload, "nodeid");
																								// printf("parent_mac: %s\n",  parent_mac->valuestring);
																								// printf("node id compare : %s : %s\n",  parent_mac->valuestring,node_id_compare);

																								// check node id ว่าซ้ำหรือไม่ ถ้าไม่ซ้ำ Add queue
																								if(parent_mac!=NULL)
																								{
																																if (strcmp(parent_mac->valuestring, node_id_compare) == 0) { // match!
																																								retry_++;
																																								ESP_LOGE("MSGQUEUE", "NOT QUEUE NODE COUNT %d",retry_);


																																}
																																else if(retry_ > 10) //try count =10
																																{
																																								retry_ = 0;
																																								strcpy(node_id_compare, parent_mac->valuestring);
																																								ESP_LOGI("MSGQUEUE", "ADD QUEUE NODE COUNT > 10");
																																								enqueue(node_info);
																																}
																																else { // not matched
																																								retry_ = 0;
																																								strcpy(node_id_compare, parent_mac->valuestring);
																																								ESP_LOGI("MSGQUEUE", "ADD QUEUE NODE");
																																								enqueue(node_info);
																																}
																								}

																}
																cJSON_Delete(Payload);
								}
								free(tmp);

}


static void root_write_msg(char* value)
{

								mdf_err_t ret = MDF_OK;
								char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
								size_t size   = MWIFI_PAYLOAD_LEN;
								mwifi_data_type_t data_type = {
																.compression = true,
																.communicate = MWIFI_COMMUNICATE_MULTICAST,
								};


								while (!mwifi_is_started()) {
																vTaskDelay(500 / portTICK_RATE_MS);
								}

								uint8_t nodes_num = esp_mesh_get_routing_table_size();
								MDF_LOGI("nodes_num : %d", nodes_num);
								int size_node = 0;
								mesh_addr_t *mac = (mesh_addr_t*)malloc(6*nodes_num);
								ret = esp_mesh_get_routing_table(mac, nodes_num * 6, &size_node);
								// for(uint8_t n = 0; n < size_node; n++)
								// 								MDF_LOGI(MACSTR,  MAC2STR(mac[n].addr));

								size = sprintf(data, value);

								ret = mwifi_root_write((uint8_t*)mac, size_node, &data_type, data, size, false);

								if (ret != MDF_OK)
								{
																MDF_LOGE("<%s> mwifi_root_write", mdf_err_to_name(ret));
								} else
								{
																MDF_LOGI("<%s> mwifi_root_write", mdf_err_to_name(ret));
								}


								MDF_FREE(data);
								free(mac);

}

static void root_read_task(void *arg)
{

								mdf_err_t ret = MDF_OK;
								char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
								size_t size   = MWIFI_PAYLOAD_LEN;
								mwifi_data_type_t data_type      = {0x0};
								uint8_t src_addr[MWIFI_ADDR_LEN] = {0};

								MDF_LOGI("Root Read is running");

								while (mwifi_is_connected()) {
																size = MWIFI_PAYLOAD_LEN;
																memset(data, 0, MWIFI_PAYLOAD_LEN);
																ret = mwifi_root_read(src_addr, &data_type, data, &size, portMAX_DELAY);
																MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_root_recv", mdf_err_to_name(ret));

																if (data_type.upgrade) { // This mesh package contains upgrade data.
																								ret = mupgrade_root_handle(src_addr, data, size);
																								MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mupgrade_root_handle", mdf_err_to_name(ret));
																} else {
																								MDF_LOGI("Receive [NODE] addr: " MACSTR ", size: %d, data: %s",
																																	MAC2STR(src_addr), size, data);

																								strcpy(data_form_node, data);
																								// printf("recv form node %s\n",data_form_node);

																								//add queue node
																								add_queue_lpp_node(data_form_node);

																								// clear buffer string node
																								for(uint8_t i=0; i<strlen(data_form_node); i++)
																								{
																																data_form_node[i] = 0;
																								}
																}
								}

								MDF_LOGW("Root is exit");
								MDF_FREE(data);
								vTaskDelete(NULL);
}

static void node_read_task(void *arg)
{
								mdf_err_t ret = MDF_OK;
								char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
								size_t size   = MWIFI_PAYLOAD_LEN;
								mwifi_data_type_t data_type      = {0x0};
								uint8_t src_addr[MWIFI_ADDR_LEN] = {0};

								MDF_LOGI("Node read task is running");

								while (mwifi_is_connected()) {
																size = MWIFI_PAYLOAD_LEN;
																memset(data, 0, MWIFI_PAYLOAD_LEN);
																ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
																MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_root_recv", mdf_err_to_name(ret));

																if (data_type.upgrade) { // This mesh package contains upgrade data.
																								ret = mupgrade_handle(src_addr, data, size);
																								MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mupgrade_handle", mdf_err_to_name(ret));
																} else {
																								MDF_LOGI("Receive [ROOT] addr: " MACSTR ", size: %d, data: %s",
																																	MAC2STR(src_addr), size, data);

																								/**
																								 * @brief Finally, the node receives a restart notification. Restart it yourself..
																								 */
																								if (!strcmp(data, "restart")) {
																																MDF_LOGI("Restart the version of the switching device");
																																MDF_LOGW("The device will restart after 3 seconds");
																																vTaskDelay(pdMS_TO_TICKS(3000));
																																esp_restart();
																								}
																}
								}

								MDF_LOGW("Node read task is exit");

								MDF_FREE(data);
								vTaskDelete(NULL);
}


/**
 * @brief Timed printing system information
 */
static void print_system_info_timercb(void *timer)
{
								uint8_t primary                 = 0;
								wifi_second_chan_t second       = 0;
								mesh_addr_t parent_bssid        = {0};
								uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
								mesh_assoc_t mesh_assoc         = {0x0};
								wifi_sta_list_t wifi_sta_list   = {0x0};

								esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
								esp_wifi_ap_get_sta_list(&wifi_sta_list);
								esp_wifi_get_channel(&primary, &second);
								esp_wifi_vnd_mesh_get(&mesh_assoc);
								esp_mesh_get_parent_bssid(&parent_bssid);

								MDF_LOGI("System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
																	", parent rssi: %d, node num: %d, free heap: %u", primary,
																	esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
																	mesh_assoc.rssi, esp_mesh_get_total_node_num(), esp_get_free_heap_size());

								// for (int i = 0; i < wifi_sta_list.num; i++) {
								// 								MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
								// }

								uint8_t nodes_num = esp_mesh_get_routing_table_size();
								MDF_LOGI("nodes_num : %d", nodes_num);
								int size_node = 0;
								mesh_addr_t *mac = (mesh_addr_t*)malloc(6*nodes_num);
								esp_mesh_get_routing_table(mac, nodes_num * 6, &size_node);
								for(uint8_t n = 0; n < size_node; n++)
																MDF_LOGI("Child mac: " MACSTR,  MAC2STR(mac[n].addr));

								if(esp_get_free_heap_size()<8000) esp_restart();

								free(mac);

#ifdef MEMORY_DEBUG

								if (!heap_caps_check_integrity_all(true)) {
																MDF_LOGE("At least one heap is corrupt");
								}

								mdf_mem_print_heap();
								mdf_mem_print_record();
								mdf_mem_print_task();
#endif /**< MEMORY_DEBUG */
}

static mdf_err_t wifi_init()
{
								mdf_err_t ret          = nvs_flash_init();
								wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

								if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
																MDF_ERROR_ASSERT(nvs_flash_erase());
																ret = nvs_flash_init();
								}

								MDF_ERROR_ASSERT(ret);

								tcpip_adapter_init();
								MDF_ERROR_ASSERT(esp_event_loop_init(NULL, NULL));
								MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
								MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
								MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
								MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
								MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
								MDF_ERROR_ASSERT(esp_wifi_start());

								return MDF_OK;
}

/**
 * @brief All module events will be sent to this task in esp-mdf
 *
 * @Note:
 *     1. Do not block or lengthy operations in the callback function.
 *     2. Do not consume a lot of memory in the callback function.
 *        The task memory of the callback function is only 4KB.
 */
static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
								MDF_LOGI("event_loop_cb, event: %d", event);

								switch (event) {
								case MDF_EVENT_MWIFI_STARTED:
																MDF_LOGI("MESH is started");
																break;

								case MDF_EVENT_MWIFI_PARENT_CONNECTED:
																MDF_LOGI("Parent is connected on station interface");
																//Priority-->6
																xTaskCreate(root_read_task, "root_read_task", 4 * 1024,
																												NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
																//Priority-->6
																xTaskCreate(node_read_task, "node_read_task", 4 * 1024,
																												NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
																break;

								case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
																MDF_LOGI("Parent is disconnected on station interface");
																break;

								case MDF_EVENT_MWIFI_ROOT_GOT_IP:
																MDF_LOGI("MDF_EVENT_MWIFI_ROOT_GOT_IP");
																break;

								case MDF_EVENT_MUPGRADE_STARTED: {
																mupgrade_status_t status = {0x0};
																mupgrade_get_status(&status);

																MDF_LOGI("MDF_EVENT_MUPGRADE_STARTED, name: %s, size: %d",
																									status.name, status.total_size);
																break;
								}

								case MDF_EVENT_MUPGRADE_STATUS:
																MDF_LOGI("Upgrade progress: %d%%", (int)ctx);
																break;

								default:
																break;
								}

								return MDF_OK;
}

/****** 96c44f33232ee9870a8c6f32 ********
   96, c4, 4f, 33, 23, 2e, e9, 87, 0a, 8c, 6f, 32, After {
   "nodeid":       "c44f33232ee9",
   "color":        2700,
   "brightness":   50,
   "type_control":  1
   }
 */
void cm_lorawan_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
								char *tmp = NULL;
								switch (event_id) {
								case LORAWAN_UPDATE:

																// tmp = (char *)event_data;
																// printf("****** %s ********\n", tmp);
																tmp = test((char *)event_data);
																// for(int i =0; i < strlen(tmp); i++)
																// 								printf("%02x, ", *(tmp + i));
																cJSON *Payload = NULL;
																ParseCMLPP ((uint8_t*)tmp, strlen(tmp), &Payload);
																char* text = cJSON_Print(Payload);
																printf("RECVB LORAWAN %s\n", text);
																free(text);

																uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
																esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
																char comp_str_node[ sizeof( sta_mac )*2 +1 ] = {'\0'};
																size_t sta_length_node = (sizeof(sta_mac)/sizeof(sta_mac[0]));

																if (Payload != NULL)
																{
																								cJSON *color = cJSON_GetObjectItemCaseSensitive(Payload, "color");
																								cJSON *brightness = cJSON_GetObjectItemCaseSensitive(Payload, "brightness");
																								cJSON *nodeid = cJSON_GetObjectItemCaseSensitive(Payload, "nodeid");
																								cJSON *read_gps = cJSON_GetObjectItemCaseSensitive(Payload, "read_gps");
																								cJSON *type_control = cJSON_GetObjectItemCaseSensitive(Payload, "type_control");

																								if(color != NULL && brightness != NULL && nodeid != NULL && type_control != NULL ) //use mesh + lorawan
																								{

																																for( size_t i=0; i<sta_length_node; i++ )
																																{
																																								sprintf( comp_str_node,"%s%02x", comp_str_node, sta_mac[i] );
																																}
																																// printf("NODE ID ---> %s\n",comp_str_node);
																																switch (type_control->valueint) {
																																// 96c44f33232ee9 870a8c6f32 9b01
																																case 1:
																																								printf("Type Single Control\n");
																																								if (strcmp(nodeid->valuestring, comp_str_node) == 0) {

																																																printf("mesh [%s] brightness %d , color %d\n",
																																																							nodeid->valuestring,
																																																							brightness->valueint,
																																																							color->valueint);

																																																Set_Color(color->valueint);
																																																Set_Brightness(brightness->valueint);
																																																status_led.mycolor = color->valueint;
																																																status_led.mybrightness = brightness->valueint;

																																																save_led(status_led);

																																								}

																																								else
																																								{
																																																root_write_msg((char *)event_data);
																																								}
																																								break;
																																// 96c44f33232ee9 870a8c6f32 9b02
																																case 2:
																																								printf("Type Group Control\n");
																																								if (strcmp(nodeid->valuestring, comp_str_node) == 0) {

																																																printf("mesh [%s] brightness %d , color %d\n",
																																																							nodeid->valuestring,
																																																							brightness->valueint,
																																																							color->valueint);

																																																Set_Color(color->valueint);
																																																Set_Brightness(brightness->valueint);
																																																status_led.mycolor = color->valueint;
																																																status_led.mybrightness = brightness->valueint;

																																																save_led(status_led);
																																																root_write_msg((char *)event_data);

																																								}
																																								break;
																																}
																								}
																								// 96c44f33232ee9 870a8c6f32
																								else if(color != NULL && brightness != NULL && nodeid !=NULL )
																								{
																																printf("Type Auto Control\n");
																																for( size_t i=0; i<sta_length_node; i++ )
																																{
																																								sprintf( comp_str_node,"%s%02x", comp_str_node, sta_mac[i] );
																																}
																																if (strcmp(nodeid->valuestring, comp_str_node) == 0) {

																																								printf("mesh [%s] brightness %d , color %d\n",
																																															nodeid->valuestring,
																																															brightness->valueint,
																																															color->valueint);

																																								Set_Color(color->valueint);
																																								Set_Brightness(brightness->valueint);
																																								status_led.mycolor = color->valueint;
																																								status_led.mybrightness = brightness->valueint;

																																								save_led(status_led);
																																								root_write_msg((char *)event_data);

																																}
																								}

																								else if(color != NULL && brightness != NULL) //use lorawan only
																								{
																																printf("lorawan brightness %d , color %d\n",
																																							brightness->valueint,
																																							color->valueint);

																																Set_Color(color->valueint);
																																Set_Brightness(brightness->valueint);
																																status_led.mycolor = color->valueint;
																																status_led.mybrightness = brightness->valueint;
																																save_led(status_led);
																								}

																								else if(read_gps != NULL) //use read gps
																								{
																																uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
																																esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
																																char comp_str_node[ sizeof( sta_mac )*2 +1 ] = {'\0'};
																																size_t sta_length_node = (sizeof(sta_mac)/sizeof(sta_mac[0]));
																																for( size_t i=0; i<sta_length_node; i++ )
																																{
																																								sprintf( comp_str_node,"%s%02x", comp_str_node, sta_mac[i] );
																																}
																																// printf("NODE ID ---> %s\n",comp_str_node);
																																if (strcmp(nodeid->valuestring, comp_str_node) == 0) { // match!
																																								if(read_gps->valueint == 0xA5) {

																																																printf("READ ROOT GPS!!");
																																																add_queue_infomation();

																																								}
																																}
																																else
																																{
																																								printf("READ NODE GPS !!");
																																								root_write_msg((char *)event_data);
																																}
																								}
																}
																cJSON_Delete(Payload);
																break;

								case LORAWAN_UNKNOWN:
																/* print unknown statements */
																// ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
																break;
								default:
																break;
								}

								free(tmp);
}

void Brideg_LORAWAN_TX(){

								while(1) {

																gps_t gps_info = get_gps_info();
																printf("lat[%f]\nlong[%f]\n", gps_info.latitude, gps_info.longitude);

																if(!isEmpty()) {
																								print_queue();
																								char *BUFF_PAYLOAD = MDF_MALLOC( QUEUE_MAX_MSG_SIZE + 11);
																								sprintf(BUFF_PAYLOAD, AT_SENDB "%s\n", RANDOM(1,10), dequeue());

																								// printf("\n%s\n",  BUFF_PAYLOAD);
																								sendCMD("Brideg_LORAWAN_TX", BUFF_PAYLOAD, true);
																								MDF_FREE(BUFF_PAYLOAD);

																}


																vTaskDelay(10000 / portTICK_RATE_MS);
								}

}

static void streetlight_working(void* arg)
{

								if (counter_sec  == 59) {
																status_led.ledworking++;
																save_led(status_led);
																counter_sec = 0;
																//add queue root any 1 min
																add_queue_lpp_root();
								} else {
																counter_sec++;
								}

								if(ble_cmd_to_gr) {
									root_write_msg((char *)recvb_form_ble);
									ble_cmd_to_gr = false;
								}

								ESP_LOGI("periodic", "counter seconds : %d sec", counter_sec);
}

void app_main()
{
								ESP_LOGI("ESP32", "[APP] Startup..");
								ESP_LOGI("ESP32", "[APP] Free memory: %d bytes", esp_get_free_heap_size());
								ESP_LOGI("ESP32", "[APP] IDF version: %s", esp_get_idf_version());


								//Initialize NVS
								esp_err_t ret = nvs_flash_init();
								if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
								{
																ESP_ERROR_CHECK(nvs_flash_erase());
																ret = nvs_flash_init();
								}
								ESP_ERROR_CHECK(ret);

								init_voltage();
								voltage_sl = read_voltage()/100.00;
								ESP_LOGI("VOLTAGE", "VOLTAGE : %.2f v", voltage_sl);


								if (read_led(&status_led))
																;
								else
								{

																status_led.meshid[0] = 1;
																status_led.meshid[1] = 2;
																status_led.meshid[2] = 3;
																status_led.meshid[3] = 4;
																status_led.meshid[4] = 5;
																status_led.meshid[5] = 6;

																status_led.meshch = 10;

																status_led.mycolor = 2700;
																status_led.mybrightness = 100;
																status_led.ledworking = 0;

																status_led.MODEL=90;
																//Dim 10 - 100% & Adj CCT 2700-6500 , 2 = Dim 10 - 100% only & not Adj CCT
																status_led.GEN=1;
																status_led.MAX_PWM = 185;
																status_led.LOW_PWM = 41;

																save_led(status_led);
								}

								char load_devaddr[strlen("00:00:00:00") + 1];
								char load_nwkskey[strlen("00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00") + 1];
								char load_appskey[strlen("00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00") + 1];
								size_t buf_len_devaddr = sizeof(load_devaddr);
								size_t buf_len_nwkskey = sizeof(load_nwkskey);
								size_t buf_len_appskey = sizeof(load_appskey);

								if(get_string("DEVADDR",load_devaddr,&buf_len_devaddr))
								{
																devaddr = load_devaddr;
								}
								else{
																devaddr = "76:6D:A0:20";
																// devaddr = "6F:00:00:01";
																save_string("DEVADDR",devaddr);
								}
								if(get_string("NWKSKEY",load_nwkskey,&buf_len_nwkskey))
								{
																nwkskey = load_nwkskey;
								}
								else{
																nwkskey = "28:AE:D2:2B:7E:15:16:A6:09:CF:AB:F7:15:88:4F:3C";
																// nwkskey = "00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF";
																save_string("NWKSKEY",nwkskey);
								}
								if(get_string("APPSKEY",load_appskey,&buf_len_appskey))
								{
																appskey = load_appskey;
								}
								else{
																appskey = "16:28:AE:2B:7E:15:D2:A6:AB:F7:CF:4F:3C:15:88:09";
																// appskey = "00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF";
																save_string("APPSKEY",appskey);
								}


								mwifi_init_config_t cfg = MWIFI_INIT_CONFIG_DEFAULT();
								mwifi_config_t config   = {
																.router_ssid     = CONFIG_ROUTER_SSID,
																.router_password = CONFIG_ROUTER_PASS,
																.channel   = status_led.meshch,
																// .mesh_id   = CONFIG_MESH_ID,
																.mesh_id = {status_led.meshid[0],
																												status_led.meshid[1],
																												status_led.meshid[2],
																												status_led.meshid[3],
																												status_led.meshid[4],
																												status_led.meshid[5]},
																.mesh_type = MESH_ROOT,
								};

/**
 * @brief Set the log level for serial port printing.
 */
								esp_log_level_set("*", ESP_LOG_INFO);
								esp_log_level_set(TAG, ESP_LOG_DEBUG);
								esp_log_level_set("mupgrade_root", ESP_LOG_DEBUG);

/**
 * @brief Initialize wifi mesh.
 */
								MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
								MDF_ERROR_ASSERT(wifi_init());
								MDF_ERROR_ASSERT(mwifi_init(&cfg));
								MDF_ERROR_ASSERT(mwifi_set_config(&config));
								MDF_ERROR_ASSERT(mwifi_start());

/**
 * @brief Data transfer between wifi mesh devices
 */
								init_bluetooth();

								init_si7006();

								// /* NMEA parser configuration */
								nmea_parser_config_t config_gps = NMEA_PARSER_CONFIG_DEFAULT();
								// /* init NMEA parser library */
								nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config_gps);

								init_led(); //led fedc kammanit

								queue_init(); //queu msg

								init_LoraWAN(); //Priority-->3 parser , //Priority-->4 READ CMD

								//Priority-->5
								xTaskCreate(Brideg_LORAWAN_TX, "Brideg_LORAWAN_TX", 4 * 1024,
																				NULL, 5, NULL);


								TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS,
																																											true, NULL, print_system_info_timercb);
								xTimerStart(timer, 0);


								const esp_timer_create_args_t periodic_timer_args = {
																.callback = &streetlight_working,
																.name = "periodic"
								};
								esp_timer_handle_t periodic_timer;
								ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
								/* Start the timers */
								ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000000));

}
