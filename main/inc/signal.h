#ifndef __SIGNAL_H__
#define __SIGNAL_H__


void init_signal_generator(void);

void init_voltage_generator(void);
void deinit_voltage_generator(void);

void set_voltage_generator_cp_state(int32_t cp_state);
void set_voltage_generator_pp_state(int32_t pp_state);

#endif