/**
 * @ingroup mission
 *
 * @file mission.c
 *
 * @{
 */

#include "../firmware.h"
#include "mission.h"
#include <string.h>

#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "inc/hw_memmap.h"

#include "../board/memory_map.h"
#include "../board/radio.h"

//#define UART_INTERFACE UART_GNSS

#define TELEMETRY_SIZE 107 // 104 (tel) + 2 (timestamp) + 1 (id)

// Private prototypes
int8_t save_eps_health_data(int8_t t);
int8_t transmit_next_telemetry(int8_t t);
int8_t save_imu_data(int8_t t);
int8_t save_gps_data(int8_t t);
int8_t check_health_status(int8_t t);
int8_t save_image_data(int8_t t);
int8_t ad_deploy_attempt(int8_t t);
int8_t exit_ad(int8_t);
int8_t save_morse_telemetry(int8_t t);
int8_t transmit_morse_telemetry(int8_t t);
int8_t free_timers(void);
int8_t send_morse_telemetry (int8_t t);
int8_t exit_fbu (int8_t t);
int8_t exit_ad (int8_t t);
int8_t save_morse_telemetry (int8_t);
int8_t transmit_morse_telemetry (int8_t);
int8_t save_attitude (int8_t t);
int8_t get_available_timer();
int8_t sm_reboot(int8_t t);
int8_t process_gs_command(int8_t t);
int8_t poll_transmitter(int8_t t);
int8_t take_picture(int8_t);

//Mode initialisation functions
void fbu_init(void);
void ad_init(void);
void nf_init(void);
void lp_init(void);
void sm_init(void);
void cfu_init(void);
void pt_init(void);
void dl_init(void);
void suspend_all_tasks();
void mode_switch(uint8_t mode);

void set_timer_for_task(uint8_t id, uint8_t task_id, uint32_t period, void (*timer_isr)() );


opmode_t modes[8];

// [ RUN-TIME ]
// Total number of tasks that can be run concurrently (limited by number of avail. timers)
#define MAX_TASKS 6

volatile int8_t task_q[MAX_TASKS];  // Task queue directly accessed by interrupts
Node*           task_pq;            // Task priority queue constructed from previous q indirectly
volatile uint8_t no_of_tasks_queued;

// Properties
task_t* current_tasks;
uint8_t current_mode; // for deciding which enums to use
uint8_t previous_mode; // for returning to previous mode from certain modes such as PT

int8_t timer_to_task[MAX_TASKS]; // timer_id -> task_id (reset look-up table)

int8_t mode_init_trigger = 1;

//FBU
bool FBU_exit_switch = false;

//ADM 
uint32_t ADM_status;
char ADM_char;
uint32_t deploy_attempts;


//Camera
bool image_trigger;

// Error detection
uint8_t flash_errors;
uint8_t ram_errors;

// State variables
uint16_t data_packets_pending;
uint8_t rx_noisefloor;


//  Picture taking wait period, 1 by default, 0 may crate bug with scheduler to best to avoid

uint32_t picture_wait_period =1;

//Transmitter settings

radio_config_t radio_config;

static double freq = 145.5;
static double pwr = 10.0;

void (*timer_isr_map[MAX_TASKS]) (); // map of timer-id to isr functions

void timer0_isr(){
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
  UART_puts(UART_INTERFACE, "**timer0_interrupt!**\r\n");

  // Schedule task
  task_q[0] = 0;
}

void timer1_isr(){
	TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
  UART_puts(UART_INTERFACE, "**timer1_interrupt!**\r\n");

  // Schedule task
  task_q[1] = 1;
}

void timer2_isr(){
	TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
  UART_puts(UART_INTERFACE, "**timer2_interrupt!**\r\n");

  // Schedule task
  task_q[2] = 2;
}

void timer3_isr(){
	TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
  UART_puts(UART_INTERFACE, "**timer3_interrupt!**\r\n");

  // Schedule task
  task_q[3] = 3;
}

void timer4_isr(){
	TimerIntClear(TIMER4_BASE, TIMER_TIMA_TIMEOUT);
  UART_puts(UART_INTERFACE, "**timer4_interrupt!**\r\n");

  // Schedule task
  task_q[4] = 4;
}

void timer5_isr(){
	TimerIntClear(TIMER5_BASE, TIMER_TIMA_TIMEOUT);
  UART_puts(UART_INTERFACE, "**timer5_interrupt!**\r\n");

  // Schedule task
  task_q[5] = 5;
}

uint32_t ALL_TIMER_PERIPHERALS[6] =
  { SYSCTL_PERIPH_TIMER0, SYSCTL_PERIPH_TIMER1, SYSCTL_PERIPH_TIMER2,
    SYSCTL_PERIPH_TIMER3, SYSCTL_PERIPH_TIMER4, SYSCTL_PERIPH_TIMER5 };

uint32_t ALL_TIMER_BASES[6] =
  {TIMER0_BASE, TIMER1_BASE, TIMER2_BASE, TIMER3_BASE, TIMER4_BASE, TIMER5_BASE};

uint32_t ulPeriod;

void set_timer_for_task(uint8_t id, uint8_t task_id, uint32_t period, void (*timer_isr)() ){
  //SHOWING STATUS OF TIMERS - TO REMOVE
  /*char outs[100];
  sprintf(outs, "Timer id: %" PRId8 " set with task %" PRId8 " with period %" PRId32 "\r\n", id, task_id, period);
  UART_puts(UART_INTERFACE, outs);*/


  TimerLoadSet(ALL_TIMER_BASES[id], TIMER_A, period-1); // prime it (-1 is unnecessery but keep it just to avoid sysclock conflict?)

  TimerIntRegister(ALL_TIMER_BASES[id], TIMER_A, timer_isr); // this is for dynamic interrupt compilation

  TimerEnable(ALL_TIMER_BASES[id], TIMER_A); 	 // start timer

  timer_to_task[id] = task_id;
}

void stop_timer_for_task(uint8_t id){
  TimerDisable(ALL_TIMER_BASES[id], TIMER_A);
  timer_to_task[id] = -1; //free timer
  //SHOWING STATUS OF TIMERS - TO REMOVE
  /*char out[100];
  sprintf(out, "Timer %" PRId8 " stopped, timer_to_task = %" PRId8 "\r\n", id, timer_to_task[id]);
  UART_puts(UART_INTERFACE, out);*/
}

int8_t free_timers(){

  //Free all timers that have been used within the mode
    for(uint8_t ids = 0; ids <6; ids++){
      stop_timer_for_task(ids);
    }
    return 0;
}

int8_t get_available_timer(){
  int8_t id = -1;
  for (uint8_t i=0; i<6; i++){
    //SHOWING TIMER STATUS - TO REMOVE
  /*  char out_g[100];
    sprintf(out_g, "Timer %" PRId8 " status %" PRId8 "\r\n", i, timer_to_task[i]);
    UART_puts(UART_INTERFACE, out_g);*/
    if (timer_to_task[i] == -1) {
      //UART_puts(UART_INTERFACE, "TIMER FREE\r\n");   TO REMOVE
      id = i;
      break;
    }
  }

  return id;
}

static void cw_tone_on(void)
{
  uint8_t pwr_reg;

  radio_reset_config(SPI_RADIO_TX, preferredSettings_cw, sizeof(preferredSettings_cw)/sizeof(registerSetting_t));
  manualCalibration(SPI_RADIO_TX);

  radio_set_freq_f(SPI_RADIO_TX, &freq);

  radio_set_pwr_f(SPI_RADIO_TX, &pwr, &pwr_reg);

  SPI_cmd(SPI_RADIO_TX, CC112X_STX);

  LED_on(LED_B);
}

static void cw_tone_off(void)
{
  radio_reset_config(SPI_RADIO_TX, preferredSettings_cw, sizeof(preferredSettings_cw)/sizeof(registerSetting_t));

  LED_off(LED_B);
}

void Mission_init(void)
{
  LED_on(LED_B);

	/* Leeeeeeeeeeerooooy Jenkinnnnns! */
  Board_init();
  WDT_kick();
  EEPROM_init();
  I2C_init(0);
  UART_init(UART_INTERFACE, 9600);
  Buffer_init();
  

  //radio_config.frequency 
  
  //Radio_rx_receive(radio_config_t *radio_config, void *received_packet_handler);
  
  //TESTING SIZE OF INT - TO REMOVE
  /*char intsize[100];
  sprintf(intsize, "Size of int is: %lu", (unsigned long) sizeof(int8_t));
  UART_puts(UART_INTERFACE, intsize);*/
  int16_t temp = Temperature_read_tmp100();
  //char output[100];
  //sprintf(output,"%d\r\n", temp);
	//UART_puts(UART_INTERFACE, output);

  UART_puts(UART_INTERFACE, "\r\n\n**BOARD & DRIVER INITIALISED**\r\n");

  // Initialise the task queue as empty
  for (uint8_t i=0; i<MAX_TASKS; i++){
    task_q[i] = -1;
    timer_to_task[i] = -1;
  }
  no_of_tasks_queued = 0;

  // Initialise timers
  for (uint8_t i=0; i<6; i++) SysCtlPeripheralEnable(ALL_TIMER_PERIPHERALS[i]);
  for (uint8_t i=0; i<6; i++) {
    while(!SysCtlPeripheralReady(ALL_TIMER_PERIPHERALS[i]));
  }
  for (uint8_t i=0; i<6; i++){
    TimerConfigure(ALL_TIMER_BASES[i], TIMER_CFG_PERIODIC); // set it to periodically trigger interrupt
    TimerIntEnable(ALL_TIMER_BASES[i], TIMER_TIMA_TIMEOUT); // enable interrupt
  }
  timer_isr_map[0] = &timer0_isr;
  timer_isr_map[1] = &timer1_isr;
  timer_isr_map[2] = &timer2_isr;
  timer_isr_map[3] = &timer3_isr;
  timer_isr_map[4] = &timer4_isr;
  timer_isr_map[5] = &timer5_isr;

  image_trigger = false;

  ulPeriod = SysCtlClockGet(); // 1Hz update rate
  //PRINTS ULPERIOD - TO REMOVE
  char output3[100];
  sprintf(output3,"ULPERIOD: %" PRIu32 "\r\n", ulPeriod);
	UART_puts(UART_INTERFACE, output3);
  //Delay_ms(5000);
  UART_puts(UART_INTERFACE, "Before\r\n");
  EEPROM_read(EEPROM_DEPLOY_STATUS, &ADM_status, 4);
  UART_puts(UART_INTERFACE, "Middle\r\n");
  EEPROM_read(EEPROM_ADM_DEPLOY_ATTEMPTS, &deploy_attempts, 4);
  UART_puts(UART_INTERFACE, "After\r\n");
   // third will miss sometimes if processor busy
  mode_switch(FBU);
}

void queue_task(int8_t task_id){
  //PRINTS TIMER STATUS - TO REMOVE
  /*for (uint8_t i=0; i<6; i++){
      char out_g[100];
      sprintf(out_g, "[[QUEUE TASK]] Timer %" PRId8 " status %" PRId8 "\r\n", i, timer_to_task[i]);
      UART_puts(UART_INTERFACE, out_g);
    }*/

  /*char output_1[100];
  //PRINTS TASK ID - TO REMOVE
  sprintf(output_1,"Task ID: : " PRId8 "\r\n", task_id);
  UART_puts(UART_INTERFACE, output_1);*/
  task_q[no_of_tasks_queued] = task_id;
  no_of_tasks_queued++;

  int8_t timer_id = get_available_timer();
  char output[100];

  sprintf(output,"set timer: : %+06d\r\n", timer_id);
  UART_puts(UART_INTERFACE, output);
  set_timer_for_task(timer_id, task_id, ulPeriod * current_tasks[task_id].period, timer_isr_map[task_id]);
}


void Mode_init(int8_t type){

  //TODO: alter all times to correct times for mission and for test

  switch(type){
    case FBU:{
      //NEED TO REINITIALISE IN EACH DIFFERENT MODE?
      Node* task_pq = newNode(0, 0);

      task_t* tasks_FBU = (task_t *) malloc (sizeof(task_t)*2);

      modes[FBU].Mode_startup = &fbu_init;
      modes[FBU].opmode_tasks = tasks_FBU;
      modes[FBU].num_tasks = 2;


      tasks_FBU[SAVE_MORSE_TELEMETRY].period = 20;
      tasks_FBU[SAVE_MORSE_TELEMETRY].TickFct = &save_morse_telemetry;

      //should be 45 minutes when overflows are fixed
      tasks_FBU[EXIT_FBU].period = 40;
      tasks_FBU[EXIT_FBU].TickFct = &exit_fbu;


      current_mode = FBU;
      current_tasks = tasks_FBU;


      queue_task(SAVE_MORSE_TELEMETRY);
      queue_task(EXIT_FBU);
      //trigger fixes a 'quirk' of the scheduler
      mode_init_trigger = 1;
    
    } break;
    case AD:{
      //Node* task_pq = newNode(0, 0);

      task_t* tasks_AD = (task_t *) malloc (sizeof(task_t)*4);

      modes[AD].Mode_startup = &ad_init;
      modes[AD].opmode_tasks = tasks_AD;
      modes[AD].num_tasks = 4;

      tasks_AD[AD_SAVE_MORSE_TELEMETRY].period = 30;
      tasks_AD[AD_SAVE_MORSE_TELEMETRY].TickFct = &save_morse_telemetry;

      tasks_AD[TRANSMIT_MORSE_TELEMETRY].period = 60;
      tasks_AD[TRANSMIT_MORSE_TELEMETRY].TickFct = &transmit_morse_telemetry;

      tasks_AD[ANTENNA_DEPLOY_ATTEMPT].period = 90;
      tasks_AD[ANTENNA_DEPLOY_ATTEMPT].TickFct = &ad_deploy_attempt;

      //remove in final build
      tasks_AD[EXIT_AD].period = 100;
      tasks_AD[EXIT_AD].TickFct = &exit_ad;


      current_mode = AD;
      current_tasks = tasks_AD;


      queue_task(AD_SAVE_MORSE_TELEMETRY);
      queue_task(TRANSMIT_MORSE_TELEMETRY);
      queue_task(ANTENNA_DEPLOY_ATTEMPT);
      queue_task(EXIT_AD);
      mode_init_trigger = 1;

    
    }break;
    case NF:{
      //Difference between data downlink mode and transitting telemetry? Is telemetry just for camera data?
      //Node* task_pq = newNode(0, 0);

      task_t* tasks_NF = (task_t *) malloc (sizeof(task_t)*6);

      modes[NF].Mode_startup = &nf_init;
      modes[NF].opmode_tasks = tasks_NF;
      modes[NF].num_tasks = 3;


      tasks_NF[NF_SAVE_EPS_HEALTH].period =30; 
      tasks_NF[NF_SAVE_EPS_HEALTH].TickFct = &save_eps_health_data;

      tasks_NF[NF_SAVE_GPS_POS].period = spacecraft_configuration.data.gps_acquisition_interval;
      tasks_NF[NF_SAVE_GPS_POS].TickFct = &save_gps_data;

			tasks_NF[NF_SAVE_ATTITUDE].period = spacecraft_configuration.data.imu_acquisition_interval;
			tasks_NF[NF_SAVE_ATTITUDE].TickFct = &save_attitude;

      tasks_NF[NF_TRANSMIT_TELEMETRY].period = spacecraft_configuration.data.tx_interval;
      tasks_NF[NF_TRANSMIT_TELEMETRY].TickFct = &transmit_next_telemetry;

			tasks_NF[NF_CHECK_HEALTH].period = spacecraft_configuration.data.health_acquisition_interval;
			tasks_NF[NF_CHECK_HEALTH].TickFct = &check_health_status;

      tasks_NF[NF_TAKE_PICTURE].period = spacecraft_configuration.data.image_acquisition_time;
			tasks_NF[NF_TAKE_PICTURE].TickFct = &take_picture; 
      

      current_mode = NF;
      current_tasks = tasks_NF;


      queue_task(NF_SAVE_EPS_HEALTH);
      queue_task(NF_SAVE_GPS_POS);
      queue_task(NF_SAVE_ATTITUDE);
      queue_task(NF_TRANSMIT_TELEMETRY);
      queue_task(NF_CHECK_HEALTH);
      if(image_trigger == true){
        queue_task(NF_TAKE_PICTURE);
      } 
      mode_init_trigger = 1;
      
    } break;
    case LP:{
      //CAN'T COMMAND OUT OF THIS MODE O NO EXIT FUNCTION NEEDED, JUST USES CHECK HEALTH
      //Node* task_pq = newNode(0, 0);

      task_t* tasks_LP = (task_t *) malloc (sizeof(task_t)*3);

      modes[LP].Mode_startup = &lp_init;
      modes[LP].opmode_tasks = tasks_LP;
      modes[LP].num_tasks = 4;


      tasks_LP[LP_SAVE_EPS_HEALTH].period =30; //300
      tasks_LP[LP_SAVE_EPS_HEALTH].TickFct = &save_eps_health_data;

      tasks_LP[LP_CHECK_HEALTH].period = spacecraft_configuration.data.health_acquisition_interval; //300
      tasks_LP[LP_CHECK_HEALTH].TickFct = &check_health_status;

			tasks_LP[LP_TRANSMIT_TELEMETRY].period = spacecraft_configuration.data.tx_interval;
			tasks_LP[LP_TRANSMIT_TELEMETRY].TickFct = &transmit_next_telemetry;

      current_mode = LP;
      current_tasks = tasks_LP; //SWITCH TO MODES.OPMODE_TASKS FOR CLEANER SOLUTION


      queue_task(LP_SAVE_EPS_HEALTH);
      queue_task(LP_CHECK_HEALTH);
      queue_task(LP_TRANSMIT_TELEMETRY);
      mode_init_trigger = 1;
    }break;
    case SM:{
      //Node* task_pq = newNode(0, 0);

      task_t* tasks_SM = (task_t *) malloc (sizeof(task_t)*4);

      modes[SM].Mode_startup = &sm_init;
      modes[SM].opmode_tasks = tasks_SM;
      modes[SM].num_tasks = 5;

      tasks_SM[SM_SAVE_EPS_HEALTH].period =30; //300
      tasks_SM[SM_SAVE_EPS_HEALTH].TickFct = &save_eps_health_data;

      tasks_SM[SM_CHECK_HEALTH].period = 30; //300
      tasks_SM[SM_CHECK_HEALTH].TickFct = check_health_status;

			tasks_SM[SM_TRANSMIT_TELEMETRY].period = 60;
			tasks_SM[SM_TRANSMIT_TELEMETRY].TickFct = &transmit_next_telemetry;

      //Give a period of 2 days once timer overflows are fixed
      tasks_SM[REBOOT_CHECK].period = 5;
			tasks_SM[REBOOT_CHECK].TickFct = &sm_reboot;


      current_mode = SM;
      current_tasks = tasks_SM; //SWITCH TO MODES.OPMODE_TASKS FOR CLEANER SOLUTION


      queue_task(SM_SAVE_EPS_HEALTH);
      queue_task(SM_CHECK_HEALTH);
      queue_task(SM_TRANSMIT_TELEMETRY);
      queue_task(REBOOT_CHECK);
      mode_init_trigger = 1;
    }break;
  case CFU:{
      //Node* task_pq = newNode(0, 0);

      task_t* tasks_CFU = (task_t *) malloc (sizeof(task_t));

      modes[CFU].Mode_startup = &cfu_init;
      modes[CFU].opmode_tasks = tasks_CFU;
      modes[CFU].num_tasks = 1;

      //NO ACTUAL RECURRING TASKS, CAN THE TASKS JUST BE LEFT AS EMPTY?
      current_mode = CFU;
      current_tasks = tasks_CFU; //SWITCH TO MODES.OPMODE_TASKS FOR CLEANER SOLUTION

      // for (uint8_t i=0; i<tasks; i++)
      // queue_task(tasks[i]);
      //ccmer_priorities[i] = -1;
      mode_init_trigger = 1;
    }break;
    case PT:{
      //Node* task_pq = newNode(0, 0);

      task_t* tasks_PT = (task_t *) malloc (sizeof(task_t));

      modes[PT].Mode_startup = &pt_init;
      modes[PT].opmode_tasks = tasks_PT;
      modes[PT].num_tasks = 1;


      //NO ACTUAL RECURRING TASKS, CAN THE TASKS JUST BE LEFT AS EMPTY?
      //ALL TIMING OCCURS IN NF MODE
      current_mode = PT;
      current_tasks = tasks_PT; //SWITCH TO MODES.OPMODE_TASKS FOR CLEANER SOLUTION

      mode_init_trigger = 1;
    }break;
    case DL:{
      //Node* task_pq = newNode(0, 0);

      task_t* tasks_DL = (task_t *) malloc (sizeof(task_t)*6);

      modes[DL].Mode_startup = &dl_init;
      modes[DL].opmode_tasks = tasks_DL;
      modes[DL].num_tasks = 6;

      tasks_DL[DL_SAVE_EPS_HEALTH].period =20; //300
      tasks_DL[DL_SAVE_EPS_HEALTH].TickFct = &save_eps_health_data;

      tasks_DL[DL_SAVE_GPS_POSITION].period = spacecraft_configuration.data.gps_acquisition_interval; //300
      tasks_DL[DL_SAVE_GPS_POSITION].TickFct = save_gps_data;

			tasks_DL[DL_SAVE_ATTITUDE].period = spacecraft_configuration.data.imu_acquisition_interval;
			tasks_DL[DL_SAVE_ATTITUDE].TickFct = &save_attitude;

      tasks_DL[DL_TAKE_PICTURE].period = spacecraft_configuration.data.image_acquisition_time;
			tasks_DL[DL_TAKE_PICTURE].TickFct = &take_picture;

      tasks_DL[DL_POLL_TRANSMITTER].period = 1; //Should be 1ms but 1s should be sufficient?
			tasks_DL[DL_POLL_TRANSMITTER].TickFct = &poll_transmitter; //To get 1ms a lot has to be changed in the code

      tasks_DL[DL_CHECK_HEALTH].period = 5;
			tasks_DL[DL_CHECK_HEALTH].TickFct = &check_health_status;

      current_mode = DL;
      current_tasks = tasks_DL; //SWITCH TO MODES.OPMODE_TASKS FOR CLEANER SOLUTION

      queue_task(DL_SAVE_EPS_HEALTH);
      queue_task(DL_SAVE_GPS_POSITION);
      queue_task(DL_SAVE_ATTITUDE);
      queue_task(DL_TAKE_PICTURE);
      queue_task(DL_POLL_TRANSMITTER);
      queue_task(DL_CHECK_HEALTH);
      mode_init_trigger = 1;
    }break;
  }
  //Starts after timers assigned but unless timer has extremey short period (<<1s) this should be fine 
  /*TODO: Put queue_task after Mode_startup() and use a bool trigger to prevent tasks being queued if PT 
  or CFU modes (only init called)*/
  modes[current_mode].Mode_startup();
}

void Mission_loop(void)
{
  //Keep no_of_tasks_queued in range
  if (mode_init_trigger == 0){
    for (uint8_t i=0; i<MAX_TASKS; i++){
      if (task_q[i] != -1){
        no_of_tasks_queued++;
      }
	}
  }
  mode_init_trigger = 0;
  // Sort list of pending tasks by their priorities
  for (uint8_t i=0; i<MAX_TASKS; i++){
    if (task_q[i] != -1)
      circ_push(&task_pq, task_q[i], current_tasks[task_q[i]].period);

      // Remove task from standard queue
      task_q[i] = -1;
      no_of_tasks_queued--;
  }

  // If there is a task to execute
  if (!circ_isEmpty(&task_pq)){
    // peek
    uint8_t todo_task_index = circ_peek(&task_pq);
    char output[100];
    sprintf(output,"%+06d\r\n", todo_task_index);
    UART_puts(UART_INTERFACE, output);

    // execute
    UART_puts(UART_INTERFACE, "**executing**\r\n");

    current_tasks[todo_task_index].TickFct(NULL);

    UART_puts(UART_INTERFACE, "**executed!**\r\n");
    // reset timer for that tasks


    // pop
    if(!circ_isEmpty(&task_pq)) circ_pop(&task_pq);
  }

  // Sleep
  UART_puts(UART_INTERFACE, "sleeping...\r\n");
  LED_toggle(LED_B);
  Delay_ms(1000);
  WDT_kick();
}

void Mission_SEU(void)
{
	/* Uncomment when working EPS board available
    Turns off mcu power rail and the eps logic turns it back on automatically 
    as the eps mcu will continually check to see if the tobc mcu is on (not yet implemented)*/
  //EPS_setPowerRail(EPS_PWR_MCU, 0);

}

//Telemetry functions to remove from use ------------------------------------------------------------------

uint8_t telemetry[104]; //832 bits (104 bytes) available for each payload
uint8_t buffer_tracker;

void start_telemetry(){
  buffer_tracker = 0;
}

void add_telemetry(uint8_t* value, uint8_t bytes){
  uint8_t index = 0;
  while (bytes-->0){
    telemetry[buffer_tracker++] = value[index++];

  }
}

int16_t find_next_telemetry(){
	uint8_t fram_availability[609];
	FRAM_read(FRAM_TABLE_OFFSET, fram_availability, 609);
	uint16_t available_slot = 0;

	// Cycle through all bits/slots to find one with unsent telemetry
	for (uint8_t i=0; i<609; i++)
		for (uint8_t bit=0; i<8; i++)
			// If the current bit shows unavailability
			if (0b11111110 & (fram_availability[i] >> bit) == FRAM_USED)
				return FRAM_PAYLOAD_OFFSET + (available_slot * FRAM_PAYLOAD_SIZE);
			else
				available_slot++;
	return -1;
}

uint16_t consume_available_location(){
	uint8_t fram_availability[609];
	FRAM_read(FRAM_TABLE_OFFSET, fram_availability, 609);
	uint16_t available_slot = 0;

	// Cycle through all bits/slots to find an available one
	// TODO: make this cyclic to avoid wear concentrated near the start of the FRAM
	// TODO: if none are found, use the oldest
	for (uint16_t i=0; i<609; i++)
		for (uint8_t bit=0; i<8; i++)
			// If the current bit shows availability
			if (0b11111110 & (fram_availability[i] >> bit) == FRAM_AVAILABLE) {
        UART_puts(UART_INTERFACE, "[TASK #001] AVAILABLE.\r\n");
        // Mark as consumed
				fram_availability[i] |= 1 << bit;
				FRAM_write(FRAM_TABLE_OFFSET + i, fram_availability[i], 1);

				return FRAM_PAYLOAD_OFFSET + (available_slot * FRAM_PAYLOAD_SIZE);
			} else {
				available_slot++;
      }
}

void end_telemetry(){
	// Find available location
  uint16_t location = consume_available_location();

	// TODO: add 128-bit authentication hash
	// TODO: add 16-bit crc

  // Save telemetry to FRAM
	FRAM_write(location, telemetry, FRAM_PAYLOAD_SIZE);
  UART_puts(UART_INTERFACE, "[TASK #001] FRAM write complete.\r\n");
}

//Telemetry functions to remove from use ------------------------------------------------------------------


uint16_t perform_subsystem_check(){
	uint16_t status = 0;

	for (int i=0; i<14; i++)
		status |= 0; // insert system check here

	return status;
}

int8_t save_eps_health_data(int8_t t){
  //Uncomment code when EPS board being used
  /*uint8_t i; // loop counter
	uint32_t eeprom_read;
  start_telemetry();

  // TODO: Generate dataset id
  add_telemetry((uint16_t)0, 2);

  // TODO: Get timestamp
  add_telemetry((uint32_t)0, 4);

  // Get TOBC values
  add_telemetry(get_temp(TEMP_OBC), 1);
  add_telemetry(get_temp(TEMP_RX), 1);
  add_telemetry(get_temp(TEMP_TX), 1);
  add_telemetry(get_temp(TEMP_PA), 1);

  // read EEPROM for reboot_count
	EEPROM_read(EEPROM_REBOOT_COUNT, &eeprom_read, 1);
	add_telemetry((uint8_t)eeprom_read, 1);

  // [CLARIFY] data_packets_pending
	add_telemetry((uint16_t)data_packets_pending, 2);

  // [CLARIFY] rx_noisefloor
	add_telemetry((uint8_t)rx_noisefloor, 1);

	// Flash and RAM errors in same byte
  add_telemetry( (flash_errors << 4) + ram_errors, 1);

  // Get 45 EPS values (16-bit fields)
  for (i=0; i<45; i++){
    uint16_t eps_field;
    EPS_getBatteryInfo(&eps_field, i);
    add_telemetry(eps_field, 2);
  }
	uint16_t subsystem_status = perform_subsystem_check();
	// read antenna switch state (and ask where this can be attached)
	subsystem_status |= (uint8_t)Antenna_read_deployment_switch() << 14;

	add_telemetry(subsystem_status, 2);

  end_telemetry();*/

  UART_puts(UART_INTERFACE, "[TASK #001] Finished saving EPS health data.\r\n");

  return 0;
}

int8_t save_gps_data(int8_t t){
  //Driver needs to be updated

  UART_puts(UART_INTERFACE, "[TASK #003] Saving GPS payload data...\r\n");

  start_telemetry();

  // TODO: Generate dataset id
	add_telemetry((uint16_t)0, 2);

	// TODO: Get timestamp
	add_telemetry((uint32_t)0, 4);


	// Take multiple samples
	for (uint8_t i=0; i<spacecraft_configuration.data.gps_sample_count; i++){
		int32_t longitude, latitude, altitude;
    uint8_t long_sd, lat_sd, alt_sd;
    uint16_t week_num;
    uint32_t week_seconds;
    uint64_t ex_time;

    GNSS_getData(&longitude, &latitude, &altitude, &long_sd, &lat_sd, &alt_sd, &week_num, &week_seconds, &ex_time);
		
    //Use cast and number of bytes
    add_telemetry((int32_t)&longitude, 4); 
		add_telemetry((int32_t)&latitude, 4); 
    add_telemetry((int32_t)&altitude, 4); 
		add_telemetry((uint8_t)&long_sd, 1); 
    add_telemetry((uint8_t)&lat_sd, 1); 
		add_telemetry((uint8_t)&alt_sd, 1); 
    //add_telemetry((uint32_t)&time, 4); 
	}

	end_telemetry();



  UART_puts(UART_INTERFACE, "[GENERAL][TASK #?] Save GPS data\r\n");
  return 0;
}

int8_t send_morse_telemetry (int8_t t){
  //TODO: test and change string to fully match TMTC specification
  //char morse_string[30];

  //UART_puts(UART_INTERFACE, "[FBU][TASK #001] Sending morse telemetry...\r\n");
  
  //char batt_str[20];
  //uint16_t batt_volt = 5;
  //**UNCOMMENT WHEN YOU HAVE ACCESS TO WORKING EPS BOARD
  //EPS_getBatteryInfo(&batt_volt,  EPS_REG_BAT_V/*EPS_REG_SW_ON*/); //May need to change to 4, given in eps header
  //if(Antenna_read_deployment_switch()){
    //ADM_status = 'Y';
  //}
  //sprintf(batt_str, "%" PRId16, batt_volt);

  //sprintf(morse_string, "U O S 3   %" PRId16 " V   %c   %" PRId16 "  K\0", batt_volt, ADM_status, deploy_attempts);
  //Packet_cw_transmit_buffer(morse_string, strlen(morse_string), cw_tone_on, cw_tone_off); ***UNCOMMENT WHEN TX IS WORKING
  //Delay_ms(2000);  //REMOVE WHEN TX IS WORKING
  UART_puts(UART_INTERFACE, "[FBU][TASK #001] Morse telemetry sent\r\n");
  return 0;

}


int8_t exit_fbu(int8_t t){
  if (FBU_exit_switch == true){
    //Free all timers that have been used within FBU mode
    UART_puts(UART_INTERFACE, "[TASK] Wait Period Finished, Exiting FBU Mode...\r\n");
      if(ADM_status == 1){
        UART_puts(UART_INTERFACE, "[TASK] Entering ADM Mode.\r\n");
        mode_switch(AD);
      }
      else{
        UART_puts(UART_INTERFACE, "[TASK] Entering NF Mode.\r\n");
        mode_switch(NF);
      }
    return 0;
  }
  else{
      //Delay_ms(500);
      FBU_exit_switch = true;
      return 0;
  }
}

int8_t ad_deploy_attempt(int8_t t){
  //ATTEMPT TO DEPLOY ANTENNA
  uint16_t batt_volt;
  UART_puts(UART_INTERFACE, "[TASK] Attempting To Deploy Antenna...\r\n");
  EPS_getBatteryInfo(&batt_volt,  EPS_REG_BAT_V/*EPS_REG_SW_ON*/); //May need to change to 4, given in eps header)
  if(batt_volt >= 7.5){
    Antenna_deploy();
    ADM_status = 1;
    EEPROM_write(EEPROM_DEPLOY_STATUS, &ADM_status, 4);
    deploy_attempts++;
    EEPROM_write(EEPROM_ADM_DEPLOY_ATTEMPTS, &deploy_attempts, 4);
    mode_switch(NF);
    UART_puts(UART_INTERFACE, "[TASK] Antenna Deployed, Switched to NF Mode.\r\n");
  }
  else{
    UART_puts(UART_INTERFACE, "[TASK] Insufficient Battery Voltage, Waiting.\r\n");
    deploy_attempts++;
    EEPROM_write(EEPROM_ADM_DEPLOY_ATTEMPTS, &deploy_attempts, 4);

  }
  return 0;
}

int8_t save_morse_telemetry(int8_t t){
  //SAVE MORSE TELEMETRY
  UART_puts(UART_INTERFACE, "[TASK] Saving Morse Telemetry...\r\n");
  char *morse_data;
  char morse_string[30];
  
  //char batt_str[20];
  uint16_t batt_volt = 5;
  //**UNCOMMENT WHEN YOU HAVE ACCESS TO WORKING EPS BOARD
  //EPS_getBatteryInfo(&batt_volt,  EPS_REG_BAT_V/*EPS_REG_SW_ON*/); //May need to change to 4, given in eps header
  if(ADM_status == 1){
    ADM_char = 'Y';
  }
  else{
    ADM_char = 'N';
  }
  //sprintf(batt_str, "%" PRId16, batt_volt);

  sprintf(morse_string, "U O S 3   %" PRId16 " V   %c   %" PRId32 "  K\0", batt_volt, ADM_char, deploy_attempts);
  morse_data = &morse_string;
  Buffer_store_new_data(&morse_data);
  UART_puts(UART_INTERFACE, "[TASK] Morse Telemetry Saved.\r\n");
  return 0;
}

int8_t transmit_morse_telemetry(int8_t t){
  //TRANSMIT MORSE TELEMETRY
  UART_puts(UART_INTERFACE, "[TASK #002] Transmit morse telemetry\r\n");
  return 0;
}


int8_t exit_ad(int8_t t){
  //EXIT AD MODE
  //UART_puts(UART_INTERFACE, "[AD][TASK #002] Exit AD mode\r\n");
  
  UART_puts(UART_INTERFACE, "[AD][TASK #002] Exit AD mode\r\n");
  
  mode_switch(NF);
  return 0;
}

void mode_switch(uint8_t mode){
  free_timers();
  previous_mode = current_mode;
  Mode_init(mode);
}

int8_t sm_reboot(int8_t t){
  UART_puts(UART_INTERFACE, "[SM][TASK?] No contact from ground, rebooting...\r\n");
  return 0;
}

int8_t save_attitude(int8_t t){
  UART_puts(UART_INTERFACE, "[TASK #003] Saving IMU payload data...\r\n");
  /*
  start_telemetry();

  // TODO: Generate dataset id
	add_telemetry((uint16_t)0, 2);

	// TODO: Get timestamp
	add_telemetry((uint32_t)0, 4);

	// TODO: Get IMU temperature based on Ed's new driver

	// Take multiple samples
	for (uint8_t i=0; i<spacecraft_configuration.data.imu_sample_count; i++){
		int16_t accel_data[3], gyro_data[3];
		//IMU_read_accel(accel_data[0], accel_data[1], accel_data[2]);
		//IMU_read_gyro(gyro_data[0], gyro_data[1], gyro_data[2]);

		add_telemetry((int16_t)&accel_data, 6); // 2bytes * 3
		add_telemetry((int16_t)&gyro_data, 6); // 2bytes * 3

		Delay_ms(500); // TODO: update time once it has been determined
	}

	end_telemetry();*/

  UART_puts(UART_INTERFACE, "[TASK #003] Finished saving IMU payload data.\r\n");
}


int8_t save_image_data(int8_t t){
  UART_puts(UART_INTERFACE, "[TASK #004] Saving Image data...\r\n");
  

  
  
  
  
  /*
  // TODO: Generate dataset id
  add_telemetry((uint16_t)0, 2);

  // TODO: Get timestamp
  add_telemetry((uint32_t)0, 4);

  // TODO: Get IMU temperature based on Ed's new driver

  // Take multiple samples
  for (uint8_t i=0; i<6; i++){
    int16_t accel_data[3], gyro_data[3];
    IMU_read_accel(accel_data[0], accel_data[1], accel_data[2]);
    IMU_read_gyro(gyro_data[0], gyro_data[1], gyro_data[2]);

    add_telemetry((int16_t)&accel_data, 6); // 2bytes * 3
    add_telemetry((int16_t)&gyro_data, 6); // 2bytes * 3

    Delay_ms(500); // TODO: update time once it has been determined
    
  }*/
  return 0;
}

//MODE INITIALISATION FUNCTIONS-------------------------------------------

void fbu_init(){
  //Not (yet?) needed
  //Exit fbu immediately if antenna is not already deployed
  UART_puts(UART_INTERFACE, "[FBU] FBU INITIALISED\r\n");
}

void ad_init(){
  //Not (yet?) needed
  UART_puts(UART_INTERFACE, "[AD] AD INITIALISED\r\n");
}

void nf_init(){
  //Not (yet?) needed
  UART_puts(UART_INTERFACE, "[NF] NF INITIALISED\r\n");
}

void lp_init(){

  //Suspend all activities in queue from previous mode
  suspend_all_tasks();

  //UNCOMMENT WHEN WORKING EPS BOARD AVAILABLE
  /*EPS_setPowerRail(EPS_PWR_CAM, 0);
  EPS_setPowerRail(EPS_PWR_GPS, 0);
  EPS_setPowerRail(EPS_PWR_GPSLNA, 0);*/

  UART_puts(UART_INTERFACE, "[LP] LP INITIALISED\r\n");
}

void sm_init(){

  //Suspend all activities in queue from previous mode
  suspend_all_tasks();

  //UNCOMMENT WHEN WORKING EPS BOARD AVAILABLE
  /*EPS_setPowerRail(EPS_PWR_CAM, 0);
  EPS_setPowerRail(EPS_PWR_GPS, 0);
  EPS_setPowerRail(EPS_PWR_GPSLNA, 0);*/

  UART_puts(UART_INTERFACE, "[SM] SM INITIALISED\r\n");
}

void cfu_init(){
  //suspend data gathering, picture taking and downlinking
  suspend_all_tasks();
  //Open recieved file and verify values (assuming the data has already been stored
  //in the spacecraft_configuration struct from the data uplink)

  //Configuration_save_to_eeprom();
  
  //Overwrite config file

  //Return becon with configuration state

  //Return to previous mode
  mode_switch(NF);

}

void pt_init(){

  //Take pciture and write to FRAM buffer

  save_image_data(NULL);

  //Return to previous mode
  if (previous_mode == DL){
    mode_switch(DL);
  }
  else{
    mode_switch(NF);
  }

}

void dl_init(){
  //Transfer packets to cc1125 buffer
  
}

void suspend_all_tasks(){
  while(!circ_isEmpty(&task_pq)){
    circ_pop(&task_pq);
  }
}

//---------------------------------------------------------------------------


void update_radio_parameters(){
	// TODO: read these from the config
	// radio_set_freq_f
	// radio_set_pwr_f

	// TODO: investigate whether any of the following are neccessary
	// -set sym_dev
	// -set packets length
	// -get packet wait
	// -manual calibration
}
//SO THIS FUNCTION TRANSMITS THE CURRENT IMU AND GPS DATA IN A BEACON, DL MODE DOWNLINKS ALL DATA STORED IN THE BUFFER

int8_t transmit_next_telemetry(int8_t t){
  /*uint8_t data_read[TELEMETRY_SIZE];

  // Read lookup table to find where to look in FRAM
	int16_t telemetry_available = find_next_telemetry();

	if (telemetry_available != -1){

		// Load packet from FRAM
		uint16_t telemetry_payload_location = (uint16_t)telemetry_available;
		uint8_t telemetry[FRAM_PAYLOAD_SIZE];
		FRAM_read(telemetry_payload_location, telemetry, FRAM_PAYLOAD_SIZE);

		// TODO: add Matt's weird error detection & check code

		// Set-up radio
		EPS_setPowerRail(EPS_PWR_TX, 1);
		radio_reset_config(SPI_RADIO_TX, preferredSettings_cw, sizeof(preferredSettings_cw)/sizeof(registerSetting_t));
		update_radio_parameters();

		// Transfer buffer to TX transceiver buffer
		cc112xSpiWriteTxFifo(SPI_RADIO_TX, telemetry, FRAM_PAYLOAD_SIZE);

		// Transmit
		SPI_cmd(SPI_RADIO_TX, CC112X_STX);

		// wait for the packet to send
		while( cc1125_pollGPIO(GPIO0_RADIO_TX)) {} ;

		EPS_setPowerRail(EPS_PWR_TX, 0);
    
	  UART_puts(UART_INTERFACE, "[TASK #002] Finished transmit next telemetry\r\n");
		return 0;
	}*/

  UART_puts(UART_INTERFACE, "[TASK #002] Finished transmit next telemetry\r\n");
	return 1;
}

int8_t check_health_status(int8_t t){

	// load allowed voltage of battery

	// Get current voltage of battery from EPS board

	// Calculate checksums for all payload

  UART_puts(UART_INTERFACE, "[GENERAL] Finished checking health\r\n");
  return 0;
}

int8_t poll_transmitter(int8_t t){
  UART_puts(UART_INTERFACE, "[DL] Finished polling transmitter\r\n");
  return 0;
}

int8_t take_picture(int8_t t){
  mode_switch(PT);
  image_trigger = false;
  UART_puts(UART_INTERFACE, "[DL] Taking picture\r\n");
  return 0;
}


int8_t process_gs_command(int8_t t){
  //Reciever interupt has not been finsished so not completely sure what the parameters of this function will look like
	switch(t){
		case IMAGE_REQUEST:
    image_trigger = true;
    if (current_mode == NF){
      queue_task(NF_TAKE_PICTURE);
    }
    UART_puts(UART_INTERFACE, "[TASK] Image Scheduled.\r\n");
		break;
		case ACK_REC_PACKETS:
		// update entries for packets in meta-table stored in FRAM
		break;
		case DL_REQUEST:
    //Dowlinking timing handled inside DL mode, basically an augmented version of NF mode
      mode_switch(DL);
		break;
		case UPDATE_CONFIG:
      mode_switch(CFU);
      UART_puts(UART_INTERFACE, "[TASK] Switched to config update mode.\r\n");
		//generate config bytes and save to EEPROM
		// in two separate locations
		break;
		case ENTER_SAFE_MODE:
      mode_switch(SM);
      UART_puts(UART_INTERFACE, "[TASK] Switched to safe mode.\r\n");
		break;
		case MANUAL_OVERRIDE:
		//reboot or switch-off subsystem
		break;
	}
  UART_puts(UART_INTERFACE, "[GENERAL][TASK #?] Finished processing gs command.\r\n");
}


// Utility

/**
 * @}
 */

