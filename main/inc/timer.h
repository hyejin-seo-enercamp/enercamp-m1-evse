#ifndef __TIMER_H__
#define __TIMER_H__

void start_timer(esp_timer_create_args_t timer_args, esp_timer_handle_t timer, uint64_t period);
void stop_timer(esp_timer_handle_t timer);

#endif