#include "ibeacon_demo.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>
#include <string.h>
#include <esp_random.h>


#define BLE_DATA_LEN 31
uint8_t ble_data[] = {
    0x1e, 0xff, 0x4c, 0x00, 0x02, 0x15, 0x43, 0xbc,
    0x97, 0x36, 0xd4, 0x06, 0x47, 0xdf, 0xb1, 0x30,
    0xdb, 0x5e, 0x07, 0x74, 0xc1, 0xe4, 0x00, 0x01,
    0x00, 0x01, 0x44, 0x44, 0x44, 0x44, 0xc6
};
int8_t scantries = 0;

bool updatestatus() {
    
    return scantries > 0;
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    esp_err_t err;

    switch (event) {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
    #if (BEACON_MODE == BEACON_MODE_RECEIVER)
            // Inicia el escaneo
            uint32_t duration = 0; // 0 significa escaneo permanente
            esp_ble_gap_start_scanning(duration);
    #endif
            break;
        }

        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            // Evento que indica si el escaneo inició correctamente
            if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(DEMO_TAG, "Scan start failed: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(DEMO_TAG, "Scan started successfully");
            }
            break;

        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;

            if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
                int8_t rssi = scan_result->scan_rst.rssi;
                if (rssi >= RSSI_THRESHOLD && memcmp(scan_result->scan_rst.ble_adv, ble_data, BLE_DATA_LEN) == 0) {
                    scantries = 127;
                    ESP_LOGI(DEMO_TAG, "----------Beacon Detected----------");/*
                    // Imprime la dirección MAC del dispositivo
                    esp_log_buffer_hex("Device Address:", scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
                    // Imprime el RSSI
                    ESP_LOGI(DEMO_TAG, "RSSI: %d dBm", rssi);
                    // Imprime los datos de publicidad en formato hexadecimal
                    ESP_LOGI(DEMO_TAG, "Advertising Data:");
                    esp_log_buffer_hex("", scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len);
                    ESP_LOGI(DEMO_TAG, "-----------------------------------");
                    ESP_LOGI(DEMO_TAG, "Status: %d", scantries);*/
                }
                else 
                {
                    scantries = (scantries <= 0) ? 0 : (scantries - 1);
                }
            }
            break;
        }

        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(DEMO_TAG, "Scan stop failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(DEMO_TAG, "Scan stopped successfully");
            }
            break;

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if ((err = param->adv_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(DEMO_TAG, "Adv stop failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(DEMO_TAG, "Adv stopped successfully");
            }
            break;

        default:
            break;
    }
}

void ble_appRegister(void) {
    esp_err_t status;

    ESP_LOGI(DEMO_TAG, "Registering GAP callback");

    // Registra la función de callback de GAP
    if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
        ESP_LOGE(DEMO_TAG, "GAP register error: %s", esp_err_to_name(status));
        return;
    }
}

void ble_init(void) {
    esp_err_t ret;
        // Libera memoria del Bluetooth clásico si no se usa
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // Configura el controlador Bluetooth
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(DEMO_TAG, "Bluetooth controller initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(DEMO_TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(DEMO_TAG, "Bluedroid initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(DEMO_TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return;
    }

    ble_appRegister();

    
    #if (BEACON_MODE == BEACON_MODE_RECEIVER)
        // Establece los parámetros de escaneo
        ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (ret) {
            ESP_LOGE(DEMO_TAG, "Set scan params failed: %s", esp_err_to_name(ret));
            return;
        }
    #endif   
}
void fill_random_bytes(uint8_t *start, size_t length) {
    for (size_t i = 0; i < length; i++) {
        start[i] = esp_random() & 0xFF; // Genera un byte aleatorio
    }
}