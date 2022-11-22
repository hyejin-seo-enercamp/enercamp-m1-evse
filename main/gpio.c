#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_sntp.h"
#include "esp_log.h"

#include "inc/gpio.h"
#include "inc/timer.h"

static const char* TAG = "GPIO";


// ================================
//
// ================================

#define GPIO_O_S2       (GPIO_NUM_12) ///< S2 control pin number
#define GPIO_O_PIN_SEL  (1ULL << GPIO_O_S2)

#define GPIO_I_CP       (GPIO_NUM_32) ///< CP signal pin (for measure frequency)
#define GPIO_I_PIN_SEL  (1ULL << GPIO_I_CP)

#define ESP_INTR_FLAG_DEFAULT (0)


// ============ GPIO Output ==============
static bool s2_is_on = false;

/// @brief Turn on the S2 (Switch2).
void gpio_s2_on()
{
    gpio_set_level(GPIO_O_S2, 1);
    s2_is_on = true;
}

/// @brief Turn of the S2 (Switch2).
void gpio_s2_off()
{
    gpio_set_level(GPIO_O_S2, 0);
    s2_is_on = false;
}



// ============ GPIO Input ================
static uint32_t gpio_num;
static int gpio_value;

static bool cp_rised = false;
static int32_t rising_edge_count = 0;
static int32_t rising_time_us = 0;
static int64_t rised_time_us = 0;

static int32_t cp_frequency = 0;
static int32_t cp_duty_ratio = 0;


/// @brief 현재 시간을 micro second 단위로 반환한다. 경과시간을 측정하기 위해 사용한다.
int64_t get_time_us()
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    // return (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
    return (int64_t)tv_now.tv_usec;
}

/// @brief CP signal input pin에서 rising and falling edge interrupt 발생 시 호출되어 
/// gpio_event_queue에 interrupt 발생 Pin number를 전달한다.
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    gpio_num = (uint32_t)arg;
    gpio_value = gpio_get_level(gpio_num);

    // switch(gpio_num)
    // {
    //     case GPIO_I_CP:
    //     {
            if(cp_rised)
            {
                if(!gpio_value) // Falling edge
                {
                    cp_rised = false;
                    rising_edge_count++;
                    
                    rising_time_us += get_time_us() - rised_time_us; // Rising으로 유지된 시간 누적
                }
            }
            else
            {
                if(gpio_value) // Rising edge
                {
                    cp_rised = true;
                    rised_time_us = get_time_us();
                }
            }
    //     } break;
    // }
}

/// @brief CP signal의 duty ratio를 반환한다.
/// @return Duty ratio (%) of CP signal
int32_t get_cp_duty_ratio()
{
    return cp_duty_ratio;
}

/// @brief CP signal의 frequency를 반환한다.
/// @return Frequency (Hz) of CP signal
int32_t get_cp_frequency()
{
    return cp_frequency;
}





// static void gpio_task(void* arg)
// {


//     for(;;)
//     {
//         if(xQueueReceiveFromISR(gpio_evt_queue, &gpio_num, portMAX_DELAY))
//         {
//             gpio_value = gpio_get_level(gpio_num);

//             if(cp_rised)
//             {
//                 if(!gpio_value) // Falling edge
//                 {
//                     cp_rised = false;
//                     rising_edge_count++;
                    
//                     rising_time_us += get_time_us() - rised_time_us; // Rising으로 유지된 시간 누적
//                 }
//             }
//             else
//             {
//                 if(gpio_value) // Rising edge
//                 {
//                     cp_rised = true;
//                     rised_time_us = get_time_us();
//                 }
//             }
//         }
//     }
// }




// ================= timer ===================
#define GPIO_TIMER_PERIOD_US (1000000) // 1 sec
void calculate_cp_frequency()
{
    cp_frequency = rising_edge_count / (1000000 / GPIO_TIMER_PERIOD_US); 
    rising_edge_count = 0;
}

void calculate_cp_duty_ratio()
{
    cp_duty_ratio = (rising_time_us * 100) / GPIO_TIMER_PERIOD_US;
    rising_time_us = 0;
}


static esp_timer_handle_t gpio_timer = NULL;

/// @brief 1초에 한 번씩 호출되어 CP signal의 frequency (Hz), duty ratio (%)를 계산한다.
static void gpio_timer_callback(void* arg)
{
    calculate_cp_frequency();
    calculate_cp_duty_ratio();

    cp_rised = false;
    // ESP_LOGW(TAG, "freq: %d (Hz), duty: %d (%%)", cp_frequency, cp_duty_ratio);
}

const esp_timer_create_args_t gpio_timer_args = 
{
    .callback = gpio_timer_callback, 
    .name = "gpio_timer", 
};


/// @brief Initialize GPIO.
void init_gpio()
{
    // Configrue GPIO output.
    gpio_config_t io_conf = 
    {
        .intr_type = GPIO_INTR_DISABLE, 
        .mode = GPIO_MODE_OUTPUT, 
        .pin_bit_mask = GPIO_O_PIN_SEL, 
        .pull_down_en = GPIO_PULLDOWN_ENABLE, 
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    gpio_config(&io_conf);

    gpio_s2_off();

    // Configure GPIO input.
    gpio_config_t io_conf_2 =
    {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT, 
        .pin_bit_mask = GPIO_I_PIN_SEL, 
        .pull_down_en = GPIO_PULLDOWN_ENABLE, 
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    gpio_config(&io_conf_2);

    // Start periodic timer.
    start_timer(gpio_timer_args, gpio_timer, 1000*1000); // period: 1 sec

    // Regist GPIO input interrupt.
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_I_CP, gpio_isr_handler, (void*)GPIO_I_CP);
}