#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
// #include "esp_ibeacon_api.h" // Ya no es necesario para beacons genéricos
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "ibeacon_demo.h"
#include "webserver.h"
#include "ota.h"
#include "deep_sleep_module.h"
#define MOTOR_GPIO2 19
#define MOTOR_GPIO 18
#define OTA_ACTIVAR CONFIG_OTA_ACTIVATE


#define TAG "---------------------------------DOOR CONTROL"

#define TOTAL_OPERATION_TIME_MS 3000
#define TIEMPO_INACTIVIDAD 60000
typedef enum {
    PUERTA_CERRADA,
    PUERTA_ABRIENDO,
    PUERTA_ABIERTA,
    PUERTA_CERRANDO
} puerta_estado_t;

static puerta_estado_t estado_puerta = PUERTA_CERRADA;
static TickType_t tiempo_final = 0;
static TickType_t tiempo_restante = 0;
static TickType_t tiempo_inactivo = 0;
static void configure_motor(void)
{
    ESP_LOGI(TAG, "Example configured to control GPIO LED!");
    gpio_reset_pin(MOTOR_GPIO);
    gpio_reset_pin(MOTOR_GPIO2);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(MOTOR_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_GPIO2, GPIO_MODE_OUTPUT);

}

static void activar_motor(bool abrir)
{
    if (abrir) {
        // Lógica para activar el motor en sentido de apertura
        ESP_LOGI(TAG, "Motor abriendo la puerta");
        gpio_set_level(MOTOR_GPIO, 1); // Asumiendo que 1 activa el motor para abrir
		gpio_set_level(MOTOR_GPIO2, 0); // Asumiendo que 1 activa el motor para abrir
    } else {
        // Lógica para activar el motor en sentido de cierre
        ESP_LOGI(TAG, "Motor cerrando la puerta");
        gpio_set_level(MOTOR_GPIO, 0); // Asumiendo que 0 activa el motor para cerrar
		gpio_set_level(MOTOR_GPIO2, 1); // Asumiendo que 1 activa el motor para abrir
    }
}

static void detener_motor(void)
{
    // Lógica para detener el motor
    ESP_LOGI(TAG, "Motor detenido");
    // Dependiendo del hardware, podrías desactivar el GPIO o ponerlo en estado de alta impedancia
    gpio_set_level(MOTOR_GPIO, 0); // Asumiendo que 0 detiene el motor
    gpio_set_level(MOTOR_GPIO2, 0);
	//set_door_state(0);
}

void main_loop(void)
{
     while (1) {
        bool status = updatestatus() || get_door_state(); // Supongo que esta función actualiza el estado del beacon
        TickType_t tiempo_actual = xTaskGetTickCount();

        switch (estado_puerta) {
            case PUERTA_CERRADA:
                if (status) {
                    // Iniciar apertura
                    activar_motor(true);
                    estado_puerta = PUERTA_ABRIENDO;
					set_door_status("PUERTA_ABRIENDO");
                    tiempo_final = tiempo_actual + pdMS_TO_TICKS(TOTAL_OPERATION_TIME_MS);
                    tiempo_inactivo = 0;
                } 
                else if(tiempo_inactivo == 0)
                {
                    tiempo_inactivo = tiempo_actual + pdMS_TO_TICKS(TIEMPO_INACTIVIDAD);
                } else if (tiempo_inactivo > 0){
                    if(tiempo_actual > tiempo_inactivo){
                        ESP_LOGI(TAG, "El esp32 ha estado inactivo por %d milisegundos, yéndose a dormir", TIEMPO_INACTIVIDAD);
                        deep_sleep_module_enter();
                    }
                }

                break;
            case PUERTA_ABRIENDO:
                if (!status) {
                    // Detener apertura y posiblemente iniciar cierre
                    detener_motor();
                    estado_puerta = PUERTA_CERRANDO;
					set_door_status("PUERTA_CERRANDO");
                    // Calcular tiempo restante para cerrar
                    tiempo_restante = tiempo_final - tiempo_actual;
                    tiempo_final = tiempo_actual + pdMS_TO_TICKS(TOTAL_OPERATION_TIME_MS) - tiempo_restante;
                } else if (tiempo_actual >= tiempo_final) {
                    // Apertura completa
                    detener_motor();
                    estado_puerta = PUERTA_ABIERTA;
					set_door_status("PUERTA_ABIERTA");
                }
                break;

            case PUERTA_ABIERTA:
                if (!status) {
                    // Iniciar cierre
                    activar_motor(false);
                    estado_puerta = PUERTA_CERRANDO;
					set_door_status("PUERTA_CERRANDO");
                    tiempo_final = tiempo_actual + pdMS_TO_TICKS(TOTAL_OPERATION_TIME_MS);
                }
                break;

            case PUERTA_CERRANDO:
                if (status) {
                    // Detener cierre y posiblemente iniciar apertura
                    detener_motor();
                    estado_puerta = PUERTA_ABRIENDO;
					set_door_status("PUERTA_ABRIENDO");
                    // Calcular tiempo restante para abrir
                    tiempo_restante = tiempo_final - tiempo_actual;
                    tiempo_final = tiempo_actual + pdMS_TO_TICKS(TOTAL_OPERATION_TIME_MS) - tiempo_restante;
                } else if (tiempo_actual >= tiempo_final) {
                    // Cierre completo
                    detener_motor();
                    estado_puerta = PUERTA_CERRADA;
					set_door_status("PUERTA_CERRADA");
                }
                break;
        }

        // Espera el periodo definido
        vTaskDelay(pdMS_TO_TICKS(100)); // Comprobar cada 100 ms
    }
}

void app_main(void) {
    // Inicializa NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializa Bluedroid y registra callbacks
    ble_init();
    configure_motor();
    webserver_init();
    #if OTA_ACTIVAR
        ota_check();
    #endif
    deep_sleep_module_init(); 
    main_loop();
}
