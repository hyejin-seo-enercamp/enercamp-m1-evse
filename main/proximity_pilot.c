#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_sntp.h"
#include "esp_log.h"


#include "inc/proximity_pilot.h"
#include "inc/control_pilot.h"
#include "inc/gpio.h"
#include "inc/adc.h"
#include "inc/timer.h"

static const char* TAG = "PP";

// ====================
// ADC: ADC1_6 (IO34)
// ====================

// ADC configuration parameters
#define ADC_UNIT        (ADC_UNIT_1) ///< ESP32 ADC Unit
#define ADC_CHANNEL     (ADC_CHANNEL_6) ///< ESP32 ADC Channel
#define ADC_BIT_WIDTH   (ADC_WIDTH_BIT_10) ///< ADC resolution. 10 bits -> 1024 levels
#define ADC_ATTEN       (ADC_ATTEN_DB_11) ///< ADC attenuation. 11 dB -> Voltage range: 150 mV ~ 2450 mV

// Timer period
#define PP_TIMER_PERIOD_US (200 * 1000) // 500 ms (2 Hz)

// PP status
static int32_t pp_status = PP_DISCONNECTED; ///< PP status

// CP charging start waiting time
#define CHARGING_START_WAITING_TIME_S (5)
static int32_t charging_start_waiting_count = 0;

// Voltage lower threshold
#define PP_4_5V_THRESH  (600)
#define PP_3V_THRESH    (400)
#define PP_1_5V_THRES   (200)

#define PP_4_5V         (4)
#define PP_3V           (3)
#define PP_1_5V         (1)
#define PP_0V           (0)

// ============== Getter =====================
int32_t get_pp_status()
{
    return pp_status;
}

int32_t pp_check_volt(int32_t volt)
{
    if(volt > PP_4_5V_THRESH)
        return PP_4_5V;
    if(volt > PP_3V_THRESH)
        return PP_3V;
    if(volt > PP_1_5V_THRES)
        return PP_1_5V;
    else 
        return PP_0V;
}



// =============== Timer ================
static esp_timer_handle_t pp_timer = NULL;

/// @brief 100 ms마다 호출되어 PP signal 전압 (V)를 측정하고 PP staus를 갱신한다.
static void pp_timer_callback(void* arg)
{
    int32_t volt = pp_check_volt(read_adc(ADC_CHANNEL));
    
    // Check PP status
    if(volt == PP_4_5V)
    {
        pp_status = PP_DISCONNECTED;
    }
    else if(volt == PP_3V)
    {
        pp_status = PP_PRESSED;
    }
    else
    {
        pp_status = PP_CONNECTED;
    }

    int32_t cp_status = get_cp_status();

    // 충전 커넥터가 꽂힌 상태이면서, 충전 준비가 되었거나 충전 중이라면 S2(Relay)를 ON상태로 유지하여 충전을 유지한다.
    if(pp_status == PP_CONNECTED && 
        (cp_status == CP_CONNECTED || cp_status == CP_CHARGING))
    {
        charging_start_waiting_count++;
        if(charging_start_waiting_count >= CHARGING_START_WAITING_TIME_S)
        {
            gpio_s2_on(); // Charging start
            charging_start_waiting_count = 0;
            ESP_LOGI(TAG, "Connection cross check done. S2 on. charging will be started");
        }
    }
    else // 이 외의 상태에서는 S2를 OFF하여 충전을 종료한다.
    {
        charging_start_waiting_count = 0;
        gpio_s2_off();
    }
}

const esp_timer_create_args_t pp_timer_args = 
{
    .callback = pp_timer_callback, 
    .name = "pp_timer",
};


/// @brief Initialize PP (Proximity Pilot).
void init_proximity_pilot()
{
    // Initialize ADC
    init_adc1(ADC_BIT_WIDTH, ADC_CHANNEL, ADC_ATTEN);

    // Initialize and start timer
    start_timer(pp_timer_args, pp_timer, PP_TIMER_PERIOD_US);
    ESP_LOGI(TAG, "Timer STARTED");
}
