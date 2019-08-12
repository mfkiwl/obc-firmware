/* firmware.h contains all relevant headers */
#include "../firmware.h"

#include <stdio.h>
#define testGet 0
#define batTest 0
int main(void){
  Board_init();
  EPS_init();
  RTC_init();
  UART_init(UART_INTERFACE, 9600);
  Delay_ms(1); // delay for initialisation to finish for UART

  UART_puts(UART_INTERFACE, "\r\n** INIT COMPLETE **\r\n");

  if (testGet) {
    EPS_setPowerRail(0b111111);
  }
  char output[100];
  uint16_t batt_v, batt_i, batt_t, data;
  uint8_t railNo = 0;
  uint16_t loopCount = 0;

  //
  // EPS_getBatteryInfo(&batt_i, EPS_REG_BAT_I);
  // sprintf(output,"Current: %+06d\r\n", batt_i);
  // UART_puts(UART_INTERFACE, output);
  
  /* EPS_getBatteryInfo(&batt_t, EP`S_REG_BAT_T);
   sprintf(output,"Batt Temperature: %+06d\r\n", batt_t);
   UART_puts(UART_INTERFACE, output);*/

   while(1){/*
     LED_toggle(LED_B);
	 uint8_t i;
	 for (i = 2; i < 0x35; i++) {
		EPS_getInfo(&eps_field, i);
		sprintf(output, "RegID: %x		Data: %u\r\n", i, eps_field);
		UART_puts(UART_INTERFACE, output);
    Delay_ms(1000);
    }*/
    loopCount++;

    if (batTest)  {
      batt_v = 0;
      batt_i = 0;
      batt_t = 0;
      if(!EPS_getInfo(&batt_v, EPS_REG_BAT_V)){UART_puts(UART_INTERFACE, "\r\nCRC incorrect");}
      UART_puts(UART_INTERFACE, "\r\nSuccessfuly read voltage");
      sprintf(output,"  Voltage: %d\r\n", batt_v);
      UART_puts(UART_INTERFACE, output);
      if(!EPS_getInfo(&batt_i, EPS_REG_SW_ON)){UART_puts(UART_INTERFACE, "\r\nCRC not matched");}
      UART_puts(UART_INTERFACE, "\r\nSuccessfuly read rails before setting");
      sprintf(output,"Current: %x\r\n", batt_i);
      UART_puts(UART_INTERFACE, output);
      if(!EPS_setPowerRail(0x24)){UART_puts(UART_INTERFACE, "\r\nUnsuccessful");}
      if(!EPS_getInfo(&batt_t, EPS_REG_SW_ON)){UART_puts(UART_INTERFACE, "\r\nCRC not matched");}
      UART_puts(UART_INTERFACE, "\r\nSuccessfuly read rails after setting");
      sprintf(output,"Current: %x\r\n", batt_t);
      UART_puts(UART_INTERFACE, output);
    }


    if (testGet) {
      //Sets 1 rail to off at a time, for an interval of 500ms excluding the TOBC rail
      EPS_setPowerRail((0x63&~(1<<railNo))|EPS_PWR_MCU);
      railNo++;
      if (railNo>5) {
        railNo = 0;
      }
      Delay_ms(500);
      if (railNo==0) {
        EPS_wri
      }
      //Checking the UART is still connected and both devices switched on
      if (EPS_selfTest()) {
        UART_puts(UART_INTERFACE, "SelfTest success");
      } else {
        UART_puts(UART_INTERFACE, "SelfTest fail");
      }

      EPS_testWrongCRC();
      EPS_testPartialPacket();
      uint8_t i;
      for (i=2, i<35, i++)  {
        if(!EPS_getInfo(&data, i))  {
          sprintf(output,"Register %x unsuccessful\r\n", i);
          UART_puts(UART_INTERFACE, output);
          }
        sprintf(output,"Register: %x Data: %x\r\n", i, data);
        UART_puts(UART_INTERFACE, output);
      }
      
      EPS_setPowerRail(0x63);
    } 
  Delay_ms(500);

  }
}