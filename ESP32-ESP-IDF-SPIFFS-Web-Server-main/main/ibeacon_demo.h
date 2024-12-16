#ifndef IBEACON_DEMO_H
#define IBEACON_DEMO_H

#include <stdint.h>
#include <stdbool.h>

// Definiciones de etiquetas de log
#define DEMO_TAG "IBEACON_DEMO"

// Definiciones de modo de beacon
#define BEACON_MODE_RECEIVER 1
#define BEACON_MODE_TRANSMITTER 2 // Si es necesario

#define BEACON_MODE BEACON_MODE_RECEIVER

// Umbral RSSI
#define RSSI_THRESHOLD -80 // Ajusta según necesidad

// Parámetros de escaneo BLE
#include "esp_gap_ble_api.h"

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

// Variables globales
extern int8_t scantries;

// Funciones
bool updatestatus(void);
void ble_appRegister(void);
void ble_init(void);

#endif // IBEACON_DEMO_H
