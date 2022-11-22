#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_sntp.h"
#include "esp_log.h"


#include "inc/adc.h"

static const char* TAG = "ADC";



/// @brief Read ADC once.
/// @return ADC raw value
int32_t read_adc(adc_channel_t adc_channel)
{
    return adc1_get_raw(adc_channel);
}

/// @brief Initialize ADC.
esp_err_t init_adc1(adc_bits_width_t bit_width, adc_channel_t channel, adc_atten_t atten)
{
    esp_err_t err = adc1_config_width(bit_width);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "adc_config_width FAILED");
        return err;
    }

    err = adc1_config_channel_atten(channel, atten);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "adc_config_channel_atten FAILED");
        return err;
    }
    return err;
}


