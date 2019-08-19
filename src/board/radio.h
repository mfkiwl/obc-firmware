

#ifndef __RADIO_H__

#define __RADIO_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../cc112x/cc112x.h"
#include "../cc112x/cc112x_spi.h"
//#include "../firmware.h"


typedef struct radio_config_t{
	double frequency; // MHz
	double power; // dBm
}radio_config_t ;

typedef enum {
	RADIO_STATUS_OK,
	RADIO_STATUS_BUSY,
	RADIO_STATUS_WRONGPART
} Radio_Status_t;

Radio_Status_t Radio_tx_msk(radio_config_t *radio_config, uint8_t *data_buffer, uint32_t data_length, void *end_of_tx_handler(void));

Radio_Status_t Radio_tx_fsk(radio_config_t *radio_config, uint8_t *data_buffer, uint32_t data_length, void *end_of_tx_handler(void));

Radio_Status_t Radio_tx_morse(radio_config_t *radio_config, uint8_t *text_buffer, uint32_t text_length, void *end_of_tx_handler(void));

Radio_Status_t Radio_tx_off(radio_config_t *radio_config);

Radio_Status_t Radio_rx_receive(radio_config_t *radio_config, uint8_t *receive_buffer, uint32_t receive_length, void received_packet_handler(void));

Radio_Status_t Radio_rx_off(radio_config_t *radio_config);

void cw_tone_on(radio_config_t *radio_config);

void cw_tone_off(void);




#endif /*  __RADIO_H__ */ 