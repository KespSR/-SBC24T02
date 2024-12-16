#ifndef OTA_EXAMPLE_H
#define OTA_EXAMPLE_H

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <sys/socket.h>


// OTA related constants
#define HASH_LEN 32
#define OTA_URL_SIZE 256

// Function declarations
esp_err_t _http_event_handler(esp_http_client_event_t *evt);

void ota_task(void *pvParameter);

static void print_sha256(const uint8_t *image_hash, const char *label);

static void get_sha256_of_partitions(void);

void ota_check(void);

#endif // OTA_EXAMPLE_H

