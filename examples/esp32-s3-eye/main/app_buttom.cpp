#include "app_buttom.hpp"

#include <stdio.h>
#include <stdlib.h>

#include "driver/adc_common.h"
#include "esp_log.h"
#include "esp_adc_cal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ADC Channels
#define ADC1_EXAMPLE_CHAN0 ADC1_CHANNEL_0
// ADC Attenuation
#define ADC_EXAMPLE_ATTEN ADC_ATTEN_DB_11
// ADC Calibration
#if CONFIG_IDF_TARGET_ESP32
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_VREF
#elif CONFIG_IDF_TARGET_ESP32S2
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32C3
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32S3
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP_FIT
#endif

#define PRESS_INTERVAL 500000

static const char *TAG = "App/Buttom";

AppButtom::AppButtom() : key_configs({{_MENU, 2800, 3000}, {_PLAY, 2250, 2450}, {_UP, 300, 500}, {_DOWN, 850, 1050}}),
                         pressed(_IDLE)
{
    ESP_ERROR_CHECK(adc1_config_width((adc_bits_width_t)ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_EXAMPLE_CHAN0, ADC_EXAMPLE_ATTEN));
}

static void task(AppButtom *self)
{
    int64_t backup_time = esp_timer_get_time();
    int64_t last_time = esp_timer_get_time();

    uint8_t menu_count = 0;

    while (true)
    {
        uint32_t voltage = adc1_get_raw(ADC1_EXAMPLE_CHAN0);
        backup_time = esp_timer_get_time();
        for (auto key_config : self->key_configs)
        {
            if ((voltage >= key_config.min) && (voltage <= key_config.max))
            {
                if (((backup_time - last_time) > PRESS_INTERVAL))
                {
                    ESP_LOGD(TAG, "Key[%d] is pressed", self->pressed);
                    self->pressed = key_config.key;

                    if (self->pressed == _MENU)
                    {
                        self->menu++;
                        self->menu %= (MENU_MOTION_DETECTION + 1);
                    }

                    last_time = backup_time;
                    self->notify();

                    self->pressed = _IDLE;
                    break;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void AppButtom::run()
{
    xTaskCreatePinnedToCore((TaskFunction_t)task, TAG, 3 * 1024, this, 5, NULL, 0);
}
