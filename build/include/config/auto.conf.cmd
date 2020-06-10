deps_config := \
	/Users/macbook/esp/esp-mdf/esp-idf/components/app_trace/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/aws_iot/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/bt/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/driver/Kconfig \
	/Users/macbook/esp/esp-mdf/components/third_party/esp-aliyun/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/esp32/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/esp_adc_cal/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/esp_event/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/esp_http_client/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/esp_http_server/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/ethernet/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/fatfs/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/freemodbus/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/freertos/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/heap/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/libsodium/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/log/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/lwip/Kconfig \
	/Users/macbook/esp/esp-mdf/components/maliyun_linkkit/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/mbedtls/Kconfig \
	/Users/macbook/esp/esp-mdf/components/mcommon/Kconfig \
	/Users/macbook/esp/esp-mdf/components/mconfig/Kconfig \
	/Users/macbook/esp/esp-mdf/components/mdebug/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/mdns/Kconfig \
	/Users/macbook/esp/esp-mdf/components/mespnow/Kconfig \
	/Users/macbook/esp/esp-mdf/components/third_party/miniz/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/mqtt/Kconfig \
	/Users/macbook/esp/esp-mdf/components/mupgrade/Kconfig \
	/Users/macbook/esp/esp-mdf/components/mwifi/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/nvs_flash/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/openssl/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/pthread/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/spi_flash/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/spiffs/Kconfig \
	/Users/macbook/esp/esp-mdf/components/third_party/esp-aliyun/components/ssl/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/tcpip_adapter/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/vfs/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/wear_levelling/Kconfig \
	/Users/macbook/esp/esp-mdf/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/macbook/esp/esp-mdf/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/macbook/esp/LoraWAN-MASTER/main/Kconfig.projbuild \
	/Users/macbook/esp/esp-mdf/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/macbook/esp/esp-mdf/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
