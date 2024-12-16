#ifndef THINGSBOARD_SEND_H
#define THINGSBOARD_SEND_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include <inttypes.h> // For format macros

// MQTT Broker URL (ThingsBoard demo)
#define CONFIG_BROKER_URL "mqtt://demo.thingsboard.io"

// Function declarations
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_app_start(void);
void send_to_thingsboard(int user);
void thingboard_init(void);

#endif // THINGSBOARD_SEND_H
