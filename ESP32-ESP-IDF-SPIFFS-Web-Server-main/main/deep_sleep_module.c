#include "deep_sleep_module.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/rtc_io.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_bt_main.h"

static const char* TAG = "DeepSleepModule";

#define DEFAULT_WAKEUP_PIN      CONFIG_EXAMPLE_GPIO_WAKEUP_PIN
#define DEFAULT_WAKEUP_LEVEL    0


/**
 * @brief Configura el GPIO como fuente de despertado (EXT0).
 */
void deep_sleep_module_init(void)
{
    const gpio_config_t config = {
        .pin_bit_mask = BIT(DEFAULT_WAKEUP_PIN),
        .mode = GPIO_MODE_INPUT,
    };

    ESP_ERROR_CHECK(gpio_config(&config));
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(BIT(DEFAULT_WAKEUP_PIN), DEFAULT_WAKEUP_LEVEL));

    ESP_LOGI(TAG, "Configurado wakeup por GPIO %d", DEFAULT_WAKEUP_PIN);
}

/**
 * @brief Entra en modo de deep sleep.
 */
void deep_sleep_module_enter(void)
{
    printf("Entrando en deep sleep\n");
    esp_deep_sleep_start();
}
