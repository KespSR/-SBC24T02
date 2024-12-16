set(srcs "main.c" "connect_wifi.c" "webserver.c" "ibeacon_demo.c" "thingsboard_send.c" "deep_sleep_module.c" "ota.c")

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
		    REQUIRES esp_http_client esp_http_server esp-tls mbedtls json nvs_flash driver spiffs esp_netif esp_wifi bt mqtt app_update esp_https_ota)
set(ESP_PARTITION_TABLE_CUSTOM "partitions.csv")
spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)

