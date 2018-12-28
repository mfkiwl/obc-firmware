#ifndef __CC112x_H__
#define __CC112x_H__

#include "../firmware.h"

typedef struct cc112x_reg_t
{
  uint16_t  addr;
  uint8_t   data;
} cc112x_reg_t;

void cc112x_reset_config(uint8_t radio_id, const registerSetting_t *cfg, uint16_t len);

uint8_t cc112x_set_pwr_f(uint8_t radio_id, double *pwr, uint8_t *reg_value);
uint8_t cc112x_set_freq_f(uint8_t radio_id, double *freq);


bool cc112x_pollGPIO(uint8_t gpio_pin);
void cc112x_manualCalibration(uint8_t radio_id);

uint8_t radio_set_fsk_param(uint8_t radio_id, uint32_t *symbol_rate, uint32_t *deviation);
uint8_t radio_set_rxbw_param(uint8_t radio_id, uint32_t *rxbw);
uint8_t radio_set_pwr_reg(uint8_t radio_id, uint8_t reg_value);
double radio_pwr_reg_to_dbm(uint8_t reg_value);

#endif /* __CC112x_H__ */