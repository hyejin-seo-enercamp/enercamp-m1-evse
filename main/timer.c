#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_sntp.h"
#include "esp_log.h"


#include "inc/timer.h"

static const char* TAG = "TIMER";


void start_timer(esp_timer_create_args_t timer_args, esp_timer_handle_t timer, uint64_t period)
{
    if(timer == NULL)
    {
        esp_timer_create(&timer_args, &timer);
    }

    // Start timer
    esp_timer_start_periodic(timer, period);
}

void stop_timer(esp_timer_handle_t timer)
{
    if(timer != NULL)
    {
        // Stop timer
        esp_timer_stop(timer);

        // Delete resources
        esp_timer_delete(timer);
        timer = NULL;
    }
}