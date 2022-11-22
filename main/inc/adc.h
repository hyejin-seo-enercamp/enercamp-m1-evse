#ifndef __ADC_H__
#define __ADC_H__

int32_t read_adc(adc_channel_t adc_channel);
esp_err_t init_adc1(adc_bits_width_t bit_width, adc_channel_t channel, adc_atten_t atten);

#endif