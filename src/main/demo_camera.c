/* firmware.h contains all relevant headers */
#include "../firmware.h"

#include <stdio.h>

#define PAGE_LENGTH 5000

uint32_t image_length;
void image_length_add(uint8_t *data, uint32_t data_length, uint32_t endfoundcount);
uint8_t **total_data;
int32_t next_frame = 0;

int main(void)
{

  char output[200];

  Board_init();
  RTC_init();

  UART_init(UART_GNSS, 115200);
  UART_puts(UART_GNSS, "Camera Demo\r\n");

  UART_init(UART_CAMERA, 115200);

  while(1)
  {
    next_frame = 0;
    LED_on(LED_B);
    total_data = (uint8_t *) malloc(sizeof(uint8_t) * 50 * PAGE_LENGTH);
    UART_puts(UART_GNSS, "Capturing..\r\n");
    image_length = 0;

    if(Camera_capture(5000, image_length_add))
    {
        sprintf(output, "SUCCESS (%"PRIu32" Bytes)\r\n", image_length);
        UART_puts(UART_GNSS, output);
    }
    else
    {
        sprintf(output, "FAIL (%"PRIu32" Bytes)\r\n", image_length);
        UART_puts(UART_GNSS, output);
    }
    /*for (uint32_t data_count =0; data_count<PAGE_LENGTH; data_count++){
      char data_output[10];
      sprintf(data_output, "%02" PRIx8 " ", total_data[data_count]);
      UART_puts(UART_GNSS, data_output);
    }*/
    LED_off(LED_B);

    /* On period */
    Delay_ms(1000);
  }
}

void image_length_add(uint8_t *data, uint32_t data_length, uint32_t endfoundcount)
{
    char output[60];
    //char output2[60];
    //test
    //DOES NOT WORK, CAUSES DRIVER TO FAIL
    //char data_out[data_length];
    (void) data;
    //sprintf(output2, "endfoundcount is: %d\r\n", endfoundcount);
    //UART_puts(UART_GNSS, output2);
    image_length += data_length;
    //if (endfoundcount == 0){
    //test
    /*for(uint32_t k=0; k<data_length; k++){
        total_data[next_frame] = data[k];
        //UART_puts(UART_GNSS, "data\r\n");
        //sprintf(data_out,"%02" PRIx8 ", ", data[k] );
        //UART_puts(UART_GNSS, data_out);
        next_frame++;
    //}
    }*/
    total_data[endfoundcount] = *data;
    sprintf(output, "0x%02"PRIx8", 0x%02"PRIx8" ..(%"PRIu32"B).. 0x%02"PRIx8", 0x%02"PRIx8"\r\n",
     data[0], data[1],
     data_length,
     data[data_length-2], data[data_length-1]
    );
    UART_puts(UART_GNSS, output);
}