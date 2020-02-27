/*
		Debug tools, feel free to expand.

		Author: Richard A
		Last modified: 2020 02 13
		Status: Functional
*/

#include <stdio.h>
#include <stdarg.h>

#include "../driver/board.h"

#include "../driver/uart.h"
#include "../driver/watchdog_ext.h"
#include "../component/led.h"
#include "../driver/delay.h"

#include "debug.h"


void debug_init(){
	// might aswell include board watchdog init
	Board_init();
	enable_watchdog_kick();
	watchdog_update = 0xFF;
	UART_init(UART_INTERFACE, 9600);
	LED_on(LED_B);
}

void debug_print(char* debug_message) {
	#ifdef DEBUG_MODE
		UART_puts(UART_INTERFACE, debug_message); 
		UART_puts(UART_INTERFACE, "\r\n");
	#endif
}

void debug_clear(){
	for (int i = 0; i < 100; i++){
		#ifdef DEBUG_MODE
			UART_puts(UART_INTERFACE, "\r\n");
		#endif
	}
}

void debug_end(){
	LED_off(LED_B);
	while(1){
		watchdog_update = 0xFF;
	}
}

void debug_hex(uint8_t data[], uint32_t data_len){
	char output[6];
	for (int i = 0; i< data_len; i++){
		sprintf(output, "0x%02X ", data[i]);
		UART_puts(UART_INTERFACE, output);
	}
	UART_puts(UART_INTERFACE, "\r\n");
}

void _vdebug_printf(const char f_string[], va_list vars)
{
	char tmp_output[256];
	vsprintf(tmp_output, f_string, vars);
	debug_print(tmp_output);
}

void debug_printf(const char f_string[], ...){
	va_list argp;
	va_start(argp, f_string);
	_vdebug_printf(f_string, argp);
	va_end(argp);
}


void debug_flash(uint8_t n_times){
	for (int i = 0; i< n_times; i++){
		LED_off(LED_B);
		Delay_ms(200);
		LED_on(LED_B);
		Delay_ms(200);
	}
}