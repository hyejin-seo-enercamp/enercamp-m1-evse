#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_sntp.h"
#include "esp_log.h"

#include "freertos/task.h"

#include "inc/control_pilot.h"
#include "inc/proximity_pilot.h"
#include "inc/gpio.h"

static const char* TAG = "MAIN";

#define MODE_EVSE_SIMULATOR (0) // 1 or 0

// run_mode: MODE_RECEIVER or MODE_EVSE_SIMULATOR
static int32_t run_mode = MODE_EVSE_SIMULATOR;

#if MODE_EVSE_SIMULTOR == 1
#include "inc/signal.h"
#endif


void app_main(void)
{

    #if MODE_EVSE_SIMULTOR == 0
        ESP_LOGI(TAG, "================= Run mode: RECEIVER ===================");
        
        init_control_pilot();
        ESP_LOGI(TAG, "Init CP");

        init_proximity_pilot();
        ESP_LOGI(TAG, "Init PP");

        init_gpio();
        ESP_LOGI(TAG, "Init GPIO");


        init_signal_generator();

    //     for(;;)
    //     {
    //         vTaskDelay(1000/portTICK_PERIOD_MS);
    //         ESP_LOGI(TAG, "Alive");
    //     }
    // #else

        ESP_LOGI(TAG, "================ Run mode: EVSE SIMULATOR =============");

        init_voltage_generator();
        ESP_LOGI(TAG, "Init voltage generator");

        for(;;)
        {
            set_voltage_generator_cp_state(CP_DISCONNECTED);
            set_voltage_generator_pp_state(PP_DISCONNECTED);
            ESP_LOGW(TAG, "Set CP State: DISCONNECTED | PP State: DISCONNECTED");

            vTaskDelay(5000/portTICK_PERIOD_MS);

            set_voltage_generator_cp_state(CP_CONNECTING);
            ESP_LOGW(TAG, "Set CP State: CONNECTING");

            vTaskDelay(5000/portTICK_PERIOD_MS);

            set_voltage_generator_cp_state(CP_CONNECTED);
            set_voltage_generator_pp_state(PP_CONNECTED);
            ESP_LOGW(TAG, "Set CP State: CONNECTED | PP State: CONNECTED");

            vTaskDelay(10 * 1000/portTICK_PERIOD_MS);

            set_voltage_generator_cp_state(CP_CHARGING);
            ESP_LOGW(TAG, "Set CP State: CHARGING");

            vTaskDelay(10 * 1000/portTICK_PERIOD_MS);

            set_voltage_generator_pp_state(PP_PRESSED);
            ESP_LOGW(TAG, "Set PP State: PRESSED");

            vTaskDelay(5000/portTICK_PERIOD_MS);

            set_voltage_generator_cp_state(CP_CHRAGING_DONE);
            ESP_LOGW(TAG, "Set CP State: CHARGING_DONE");

            vTaskDelay(5000/portTICK_PERIOD_MS);
        }
    #endif
}
