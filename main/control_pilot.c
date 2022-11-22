#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_sntp.h"
#include "esp_log.h"


#include "inc/control_pilot.h"
#include "inc/proximity_pilot.h"
#include "inc/gpio.h"
#include "inc/timer.h"
#include "inc/adc.h"

static const char* TAG = "CP";


// =======================
// ADC: ADC1_5 (IO33)
// =======================

/// ADC configuration Parameters
#define ADC_UNIT        (ADC_UNIT_1) ///< ESP32 ADC Unit
#define ADC_CHANNEL     (ADC_CHANNEL_5) ///< ESP32 ADC Channel
#define ADC_BIT_WIDTH   (ADC_WIDTH_BIT_10) ///< ADC resolution. 10 bits -> 1024 levels
#define ADC_ATTEN       (ADC_ATTEN_DB_11) ///< ADC attenuation. 11 dB -> Voltage range: 150 mV ~ 2450 mV

// // for calcultation adc raw to millivolt
// static uint32_t v_ref = 2450; ///< ADC reference voltage (mV)
// static uint32_t v_bias = 150; ///< ADC bias voltage (mV)
// static uint32_t resolution = 1024; ///< ADC resoution

// Timer period
// #define CP_FAST_TIMER_PERIOD_US (200) // 100 us (10 kHz)
#define CP_SLOW_TIMER_PERIOD_US (1000 * 1000) // 1 s (1 Hz)

// CP status
static int32_t cp_status = CP_DISCONNECTED; ///< CP status

// Voltage lower threshold
#define CP_12V_THRESH       (600)   ///< ADC raw value of 12V lower threshold
#define CP_9V_THRESH        (400)   ///< ADC raw value of 9V lower threshold
#define CP_6V_THRESH        (200)   ///< ADC raw value of 6V lower threshold
#define CP_3V_THRESH        (100)   ///< ADC raw value of 3V lower threshold
#define CP_1KHZ_THRESH      (500)   ///< 1 kHz lower threshold (Hz)

#define CP_12V              (12)    ///< CP Input DC 12V
#define CP_9V               (9)     ///< CP Input DC 9V
#define CP_6V               (6)     ///< CP Input DC 6V
#define CP_3V               (3)     ///< CP Input DC 3V
#define CP_0V               (0)     ///< CP Input DC 0V

static int32_t volt = 0;
static int32_t adc_raw = 0;



// // CP signal frequency, duty
// static int32_t cp_frequency = 0; ///< Signal frequency [Hz]]
// static int32_t cp_duty_ratio = 0; ///< Duty ratio [%]

// static bool rised = true;
// static uint32_t rising_edge_count = 0; ///< 1초동안 rising edge 발생 횟수
// static uint32_t rising_count = 0; ///< 해당 주기에 rising 상태로 유지된 횟수





// ============== Getter =====================

// int32_t get_cp_duty_ratio()
// {
//     return cp_duty_ratio;
// }

// int32_t get_cp_frequency()
// {
//     return cp_frequency;
// }

int32_t get_cp_status()
{
    return cp_status;
}



// =============== Fast timer =================
// static esp_timer_handle_t cp_fast_timer = NULL;
// static bool cp_fast_timer_running = false;

int32_t cp_check_volt(int32_t volt)
{
    if(volt > CP_12V_THRESH)
        return CP_12V;
    if(volt > CP_9V_THRESH)
        return CP_9V;
    if(volt > CP_6V_THRESH)
        return CP_6V;
    if(volt > CP_3V_THRESH)
        return CP_3V;
    else 
        return CP_0V;
}

// /// @brief 0.1 ms (10 kHz)마다 호출되어 전압 (V) 측정, rising edge 계수를 수행한다.
// static void cp_fast_timer_callback(void* arg)
// {
//     adc_raw = read_adc(ADC_CHANNEL);
//     int32_t volt_temp = cp_check_volt(adc_raw);
//     if(volt_temp != 0)
//         volt = volt_temp;
    
//     // Measure frequency
//     if(rised) // 현재 rising 상태이며, falling edge 감지할 차례
//     {
//         if(!volt_temp) // volt_temp == 0 -> Falling edge detected
//         {
//             rised = false;
//         }
//         else // rising 상태 유지
//         {
//             rising_count++; // rising edge 유지 횟수 증가
//         }
//     }
//     else // 현재 falling 상태이며, rising edge 감지할 차례
//     {
//         if(volt_temp) // volt_temp > 0 -> Rising edge detected
//         {
//             rised = true;

//             rising_edge_count++; // rising edge 개수 증가
//             rising_count++; // rising edge 유지 횟수 증가
//         }
//     }
// }

// const esp_timer_create_args_t cp_fast_timer_args = 
// {
//     .callback = cp_fast_timer_callback, 
//     .name = "cp_fast_timer", 
// };

// void start_cp_fast_timer()
// {
//     rising_edge_count = 0;
//     rising_count = 0;
//     rised = true;

//     cp_fast_timer_running = true;

//     start_timer(cp_fast_timer_args, cp_fast_timer, CP_FAST_TIMER_PERIOD_US); // period: 100 us (10 kHz)

//     ESP_LOGI(TAG, "Fast timer STARTED");
// }

// void stop_cp_fast_timer()
// {
//     stop_timer(cp_fast_timer);

//     cp_fast_timer_running = false;

//     ESP_LOGI(TAG, "Fast timer STOPPED");
// }



// ================= Slow timer ===================
static esp_timer_handle_t cp_slow_timer = NULL;

/// @brief 1초에 한 번씩 호출되어 cp_fast_timer_calback()에서 측정한 CP signal의 전압 (V), rising edge 개수를 통해 
/// frequency (Hz), duty ratio (%)를 계산하여 EVSE의 연결 상태, 최대 출력 전류(해당 시)를 감지한다.
static void cp_slow_timer_callback(void* arg)
{
    int32_t cp_frequency = get_cp_frequency();
    int32_t cp_duty_ratio = get_cp_duty_ratio();

    int32_t volt = 0;
    while((volt = cp_check_volt(read_adc(ADC_CHANNEL))) == 0);

    // Check CP status
    switch(cp_status)
    {
        case CP_DISCONNECTED:
        {
            if(volt == CP_9V)
            {
                cp_status = CP_CONNECTING;
                ESP_LOGI(TAG, "Status: DISCONNECTED -> CONNECTING");
            }
        } break;

        case CP_CONNECTING: 
        {
            if(volt == CP_9V && cp_frequency > CP_1KHZ_THRESH)
            {
                cp_status = CP_CONNECTED;
                ESP_LOGI(TAG, "Status: CONNECTING -> CONNECTED");
            }
            else if(volt == CP_12V)
            {
                cp_status = CP_DISCONNECTED;
                ESP_LOGI(TAG, "Status: CONNECTING -> DISCONENCTED");
            }
        } break;

        case CP_CONNECTED: 
        {
            if(volt == CP_9V && cp_frequency > CP_1KHZ_THRESH)
            {
                cp_status = CP_CONNECTED;
                if(get_pp_status() == PP_CONNECTED) // PP에서도 충전기 연결상태 cross check.
                {                

                }
            }
            else if (volt == CP_6V || volt == CP_3V)
            {
                cp_status = CP_CHARGING;
                ESP_LOGI(TAG, "Status: CONNECTING -> CHARGING");
            }
            else if(volt == CP_12V)
            {
                cp_status = CP_DISCONNECTED;
                ESP_LOGI(TAG, "Status: CONNECTED -> DISCONNECTED");
            }
        } break;

        case CP_CHARGING: 
        {
            if(volt == CP_9V)
            {
                cp_status = CP_CHRAGING_DONE;
                ESP_LOGI(TAG, "Status: CHARGING -> CHARGING_DONE");
            }
        } break;

        case CP_CHRAGING_DONE: 
        {
            if(volt == CP_12V)
            {
                cp_status = CP_DISCONNECTED;
                ESP_LOGI(TAG, "Status: CHARGING_DONE -> DISCONNECTED");
            }
        } break;

        default:
        {
            // Do nothing
        } break;
    }
    ESP_LOGI(TAG, "ADC read. volt: %d V (%d), freq: %d Hz, duty: %d %%, CP: %d, PP: %d", volt, adc_raw, cp_frequency, cp_duty_ratio, cp_status, get_pp_status());
}

const esp_timer_create_args_t cp_slow_timer_args = 
{
    .callback = cp_slow_timer_callback, 
    .name = "cp_slow_timer", 
};

void start_cp_slow_timer()
{
    start_timer(cp_slow_timer_args, cp_slow_timer, CP_SLOW_TIMER_PERIOD_US); // period: 1 s (1 Hz)

    ESP_LOGI(TAG, "Slow timer STARTED");
}

void stop_cp_slow_timer()
{
    stop_timer(cp_slow_timer);

    ESP_LOGI(TAG, "Slow timer STOPPED");
}

// ============ Initialize =================
/// @brief Initialize CP (Pilot Control).
void init_control_pilot()
{
    // Initialize ADC
    init_adc1(ADC_BIT_WIDTH, ADC_CHANNEL, ADC_ATTEN);

    // Initialize and start timer
    start_cp_slow_timer();
    // start_cp_fast_timer();
}