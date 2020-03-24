/*
	Author: Richard A
	Last modified: 2020 02 17
	Status: Indev
*/
#define DEBUG_MODE

#include <stdio.h>
#include <stdlib.h>

#include "../driver/board.h"
#include "../driver/wdt.h"
#include "../driver/watchdog_ext.h"
#include "../driver/fram.h"
#include "../mission/buffer.h"
#include "../utility/debug.h"
#include "../driver/uart.h"

char output[256];

void buffer_status_print(){
	sprintf(output, "demo: buffer status: %u, cba %u, cbp %u, tba %u",
			buffer_status.as_struct.buffer_init,
			buffer_status.as_struct.current_block_address,
			buffer_status.as_struct.current_block_position,
			buffer_status.as_struct.transmit_block_address);
	debug_print(output);
}

int main(){
	// Board_init();
	debug_init();
	debug_clear();
	// enable_watchdog_kick();
	// watchdog_update = 0xFF;

	debug_print("=== DEMO buffer ===");
	debug_print("reads the non-volitile status, write some data, read it back\r\n");

	_buffer_overwrite_table();
	uint8_t data_to_write[] = "Some data to write. ";
	uint32_t data_len = sizeof(data_to_write);

	uint8_t more_data_to_write[] = "Did you ever hear the Tragedy of Darth Plagueis the wise? I thought not. It's not a story the Jedi would tell you. It's a Sith legend. Darth Plagueis was a Dark Lord of the Sith, so powerful and so wise he could use the Force to influence the midichlorians to create life... He had such a knowledge of the dark side that he could even keep the ones he cared about from dying. The dark side of the Force is a pathway to many abilities some consider to be unnatural. He became so powerful... the only thing he was afraid of was losing his power, which eventually, of course, he did. Unfortunately, he taught his apprentice everything he knew, then his apprentice killed him in his sleep. It's ironic he could save others from death, but not himself.";
	uint32_t more_data_len = sizeof(more_data_to_write);

	uint8_t block_buffer[256];
	uint8_t read_length;
	// check the status before any writing has occured
	debug_print("DEMO: retrieving the status stored");
	buffer_retrieve_status();
	buffer_status_print();

	// write to buffer
	debug_print("DEMO: writing to buffer. Buffer should init if the above is not performed");
	buffer_write_next(data_to_write, data_len);
	// status should have been updated
	debug_print("DEMO: printing the status post write");
	buffer_status_print();

	// status should have been written to the fram in block 0

	debug_print("DEMO: retrieving the status from fram post write");
	buffer_retrieve_status();
	buffer_status_print();

	debug_print("DEMO: reading the block back");
	buffer_retrieve_block(buffer_status.as_struct.transmit_block_address, block_buffer, &read_length);

	block_buffer[read_length] = (uint8_t) '\0';
	sprintf(output, "DEMO: read length %u", read_length);
	debug_print(output);
	debug_print("DEMO: contents of buffer:");
	debug_print(&block_buffer[BUFFER_DATA_START_INDEX]);
	

	debug_print("DEMO: Writing more data to the buffer, > 1 block");
	buffer_write_next(more_data_to_write, more_data_len);
	debug_print("DEMO: done. status:");
	buffer_status_print();


	debug_print("DEMO: Reading data from the buffer");
	// // read all the data
	// for(int i = 1; i <= buffer_status.as_struct.current_block_address; i++){
	// 	buffer_retrieve_block(i, block_buffer, &read_length);
	// 	for (int j = BUFFER_DATA_START_INDEX; j< read_length; j++){
	// 		UART_putc(UART_INTERFACE, block_buffer[j]);
	// 	}
	// }
	// debug_print("");
	uint32_t length_to_read = more_data_len;
	length_to_read+= data_len;

	uint8_t start_pos = BUFFER_DATA_START_INDEX;
	uint8_t* read_data = malloc(length_to_read);
	uint32_t actual_read = 0;
	if (read_data != NULL){
		actual_read = buffer_read_length(buffer_status.as_struct.transmit_block_address, start_pos, read_data, length_to_read);
	}
	debug_printf("DEMO: data length: %d. actually read: %d bytes", length_to_read, actual_read);
	debug_printl(read_data, actual_read);

	free(read_data);

	debug_print("=== end demo ===");
	// while(1){
	// 	watchdog_update=0xFF;
	// }
	debug_end();
}

