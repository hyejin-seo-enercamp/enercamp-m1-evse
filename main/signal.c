#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "driver/ledc.h"
#include "esp_adc_cal.h"
#include "esp_sntp.h"
#include "esp_log.h"

#include "freertos/task.h"

#include "inc/signal.h"
#include "inc/timer.h"
#include "inc/control_pilot.h"
#include "inc/proximity_pilot.h"

static const char* TAG = "SIGNAL";


// =======================
// 1KHZ SIGNAL OUTPUT: IO27
// VOLTAGE GENERATOR OUTPUT: IO25
// =======================

#define GPIO_O_1KHZ (GPIO_NUM_27)

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          GPIO_O_1KHZ // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (150)//(127) // Set duty to 50%. ((2 ** 8) - 1) * 50% = 127.5
#define LEDC_FREQUENCY          (1000) // Frequency in Hertz. Set frequency at n Hz


// ================ 1 kHz square signal generator ======================
static void example_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

/// @brief Initailize 1 kHz square signal generator (IO32).
void init_signal_generator(void)
{
    // Set the LEDC peripheral configuration
    example_ledc_init();
    // Set duty to 50%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

void deinit_signal_generator()
{
        // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

// ================ DAC voltage generator =======================
#define DAC_CHANNEL_CP (DAC_CHANNEL_1) // IO25
#define DAC_CHANNEL_PP (DAC_CHANNEL_2) // IO26

static int32_t dac_cp_value = 0;
static int32_t dac_pp_value = 0;
static int32_t dac_rised = false;




/// @brief Initialize voltage generator (IO25).
void init_voltage_generator(void)
{
    dac_output_enable(DAC_CHANNEL_1);
    dac_output_enable(DAC_CHANNEL_2);

    set_voltage_generator_cp_state(CP_DISCONNECTED);
    set_voltage_generator_pp_state(PP_DISCONNECTED);
}

/// @brief Deinitialize voltage generator (IO25).
void deinit_voltage_generator(void)
{
    dac_output_disable(DAC_CHANNEL_1);
    dac_output_disable(DAC_CHANNEL_2);
}

/// @brief Change output voltage of CP voltage generator.
void set_voltage_generator_cp_voltage(int32_t millivolt)
{
    dac_cp_value = millivolt * 255 / 3300;
    dac_output_voltage(DAC_CHANNEL_1, dac_cp_value);
}

/// @brief Change output voltage of PP voltage generator.
void set_voltage_generator_pp_voltage(int32_t millivolt)
{
    dac_pp_value = millivolt * 255 / 3300;
    dac_output_voltage(DAC_CHANNEL_2, dac_pp_value);
}


static esp_timer_handle_t voltage_generator_timer = NULL;

static void voltage_generator_timer_callback(void* arg)
{
    if(dac_rised)
    {
        dac_output_voltage(DAC_CHANNEL_1, 0);
        dac_rised = false;
    }
    else
    {
        dac_output_voltage(DAC_CHANNEL_1, dac_cp_value);
        dac_rised = true;
    }
}

const esp_timer_create_args_t voltage_generator_timer_args = 
{
    .callback = voltage_generator_timer_callback, 
    .name = "voltage_generator_timer",
};

/// @brief Set voltage generator output signal frequency to 1 kHz.
void voltage_generator_set_frequency_1khz(void)
{
    init_signal_generator();

    // start_timer(voltage_generator_timer_args, voltage_generator_timer, 500);
    // dac_output_voltage(DAC_CHANNEL_1, 0);
    // dac_rised = false;
}

/// @brief Set voltage generator output signal frequency to 0 Hz .
void voltage_generator_set_frequency_0hz(void)
{
    deinit_signal_generator();
    
    // stop_timer(voltage_generator_timer);
    // dac_output_voltage(DAC_CHANNEL_1, dac_cp_value);
}

/// @brief Set CP state for simulation.
/// @param [in] cp_state
void set_voltage_generator_cp_state(int32_t cp_state)
{
    switch(cp_state)
    {
        case CP_DISCONNECTED: 
        {
            voltage_generator_set_frequency_0hz();
            set_voltage_generator_cp_voltage(2100); // 2.1V
        } break;
        case CP_CONNECTING:
        {
            voltage_generator_set_frequency_0hz();
            set_voltage_generator_cp_voltage(1570); // 1.57V
        } break;
        case CP_CONNECTED:
        case CP_CHRAGING_DONE:
        {
            voltage_generator_set_frequency_1khz();
            set_voltage_generator_cp_voltage(1570); // 1.57V
        } break;
        case CP_CHARGING:
        {
            voltage_generator_set_frequency_1khz();
            set_voltage_generator_cp_voltage(1050); // 1.05V
        } break;
    }
}


/// @breif Set PP state for simulation.
/// @param [in] pp_state
void set_voltage_generator_pp_state(int32_t pp_state)
{
    switch(pp_state)
    {
        case PP_DISCONNECTED:
        {
            set_voltage_generator_pp_voltage(2100); //(2940); // 2.94V
        } break;
        case PP_PRESSED:
        {
            set_voltage_generator_pp_voltage(1570); //(1780); // 1.78V
        } break;
        case PP_CONNECTED:
        {
            set_voltage_generator_pp_voltage(1050); // (990); // 0.99V
        } break;
    }
}