/**
 * @ingroup uos3-proto
 * @ingroup camera
 *
 * @file uos3-proto/camera.c
 * @brief Camera Hardware Driver - uos3-proto board
 *
 * @{
 */

#include "board.h"
#include "../uart.h"
#include "../rtc.h"
#include "../delay.h"
#include "../camera.h"

static char LK_RESET[]     = {0x56, 0x00, 0x26, 0x00};
static char LK_RESET_RE[]    = {0x0d, 0x0a, 0x49, 0x6e, 0x69, 0x74, 0x20, 0x65, 0x6e, 0x64, 0x0d, 0x0a};  //legit!

//commands for the camera baudrate settings -> datasheet
//static char LK_BAUDRATE_9600[] = {0x56, 0x00, 0x24, 0x03, 0x01, 0xAE};
//static char LK_BAUDRATE_19200[]  = {0x56, 0x00, 0x24, 0x03, 0x01, 0x56, 0xe4};
//static char LK_BAUDRATE_38400[]  = {0x56, 0x00, 0x24, 0x03, 0x01, 0x2a, 0xf2};
//static char LK_BAUDRATE_115200[] = {0x56, 0x00, 0x24, 0x03, 0x01, 0x0D, 0xA6};
//static char LK_BAUDRATE_RE[] = {0x76, 0x00, 0x24, 0x00, 0x00};

//commands for the camera resolution settings -> datasheet
//static char LK_RESOLUTION_VGA[] = {0x56, 0x00, 0x54, 0x01, 0x00};
static char LK_RESOLUTION_160[] = {0x56, 0x00, 0x54, 0x01, 0x22};
static char LK_RESOLUTION_800[] = {0x56, 0x00, 0x54, 0x01, 0x1D};
static char LK_RESOLUTION_1280[] = {0x56, 0x00, 0x54, 0x01, 0x1B};
static char LK_RESOLUTION_1600[] = {0x56, 0x00, 0x54, 0x01, 0x21};
static char LK_RESOLUTION_RE[] = {0x76, 0x00, 0x54, 0x00, 0x00}; // v T; response from the camera after setting the resolution

//setting compression rate -> value of compression in last HEX number
static char LK_COMPRESSION[] = {0x56, 0x00, 0x31, 0x05, 0x01, 0x01, 0x12, 0x04, 0x00}; // Value in last byte
static char LK_COMPRESSION_RE[] = {0x76, 0x00, 0x31, 0x00, 0x00}; // value in last byte

static char LK_PICTURE[]   = {0x56, 0x00, 0x36, 0x01, 0x00};
static char LK_PICTURE_RE[]    = {0x76, 0x00, 0x36, 0x00, 0x00};
static char LK_JPEGSIZE[]    = {0x56, 0x00, 0x34, 0x01, 0x00};
static char LK_JPEGSIZE_RE[] = {0x76, 0x00, 0x34, 0x00, 0x04}; //, 0x00, 0x00, 0x00, 0x00}; // then XH XL // assumption is last 4 bytes are size
//static char LK_STOP[]    = {0x56, 0x00, 0x36, 0x01, 0x03};
//static char LK_STOP_RE[]   = {0x76, 0x00, 0x36, 0x00, 0x00};

static char LK_READPICTURE[] = {0x56, 0x00, 0x32, 0x0C, 0x00, 0x0A,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00};
static char LK_PICTURE_TIME_dot1ms[]  = {0x00, 0x0a}; // 0.1 ms
static char LK_READPICTURE_RE[]  = {0x76, 0x00, 0x32, 0x00, 0x00};
static char JPEG_START[]     = {0xFF, 0xD8};
static char JPEG_END[]   = {0xFF, 0xD9};

#define CAMWRITE(a) UART_putb(UART_CAMERA,a,sizeof(a)); // send this message
/*
static bool UART_waitmatch(char *string, uint32_t string_length, uint32_t timeout)  ///use this legit!
{
  char c;
  uint32_t index;
  uint64_t start_timestamp;
  RTC_getTime_ms(&start_timestamp);

  index = 0;
  while(!RTC_timerElapsed_ms(start_timestamp, timeout))
  {
    if(UART_getc_nonblocking(UART_CAMERA, &c))
    {
      if(c == string[index])
      {
        index++;
        if(index == string_length)
        {
          return true;
        }
      }
      else
      {
        index = 0;
      }
    }
  }
  return false;
}
*/
static bool UART_command_match(char *string, uint32_t string_length)
{
  char data_in;
  uint8_t index = 0;
  while(UART_charsAvail(UART_CAMERA))
  {
    if(UART_getc_nonblocking(UART_CAMERA, &data_in)){
      if(data_in == string[index])
      {
        index++;
      }
      else
      {
        index = 0;
      }
    }
  }
  if(index == string_length){ return true;}
  else{ return false;}
}

static uint32_t UART_getw4(uint8_t serial)
{
  char c1=UART_getc(serial),c2=UART_getc(serial),c3=UART_getc(serial),c4=UART_getc(serial);
  return (uint32_t)((c1 << 24) | (c2 << 16) | (c3 << 8) | (c4 << 0)); //shifting values of read char/hex numbers to create one 32bit number
}
/*
static bool Camera_command(char *command, uint32_t command_length, char *response, uint32_t response_length)
{
  uint32_t attempts = 1;

  UART_putb(UART_CAMERA,command,command_length);
  while(!UART_waitmatch(response, response_length, 3000))
  {
    if(--attempts == 0)     //why --attempts; as attempts = 1; so ut will make if true, and funtion will return false
    {                       //how it works - UART_waitmatch is looking for returning string matching the return command;
      return false;         //if it is correct then this function return true adn command is successfully send; if return false (not matched for specified waiting time), this function is returning false, as we have only one try
    }
    Delay_ms(1000);
    UART_putb(UART_CAMERA,command,command_length);
  } 
  return true;
}*/

static bool Camera_command(char *command, char *response)
{
  uint32_t attempts = 1;
  CAMWRITE(command);
  Delay_ms(25);
  if(!UART_command_match(response, sizeof(response)))
  {
    if(--attempts == 0)     //why --attempts; as attempts = 1; so ut will make if true, and funtion will return false
    {                       //how it works - UART_waitmatch is looking for returning string matching the return command;
      return false;         //if it is correct then this function return true adn command is successfully send; if return false (not matched for specified waiting time), this function is returning false, as we have only one try
    }
    Delay_ms(1000);
    CAMWRITE(command);
  }else
  { 
  return true;
  }
}

bool Camera_capture()
{
  uint16_t rest, page_size = 104;   //BUFFER_SLOT_SIZE / 8 =106; but it need to be multiple integer of 8
  uint32_t jpegsize, number_of_pages;
  uint8_t count=0, page_buffer[page_size];
  bool endflag = false;
  char bin;
  // initialise camera
  if(!Camera_command(LK_RESET, LK_RESET_RE))
  {
    UART_puts(UART_GNSS, "1");
    return false;
  }
  Delay_ms(3000); // 2-3 sec gap required by data sheet before camera ready
  // set resolution
  if(!Camera_command(LK_RESOLUTION_1600, LK_RESOLUTION_RE))
  {
    UART_puts(UART_GNSS, "3");
    return false; 
  }
  // set compression
  set_compression(0x10);
  if(!Camera_command(LK_COMPRESSION, LK_COMPRESSION_RE))
  {
    UART_puts(UART_GNSS, "4");
    return false;
  }
  // take picture
  if(!Camera_command(LK_PICTURE, LK_PICTURE_RE))
  {
    UART_puts(UART_GNSS, "5");
    return false;
  }
  // read size
  if(!Camera_command(LK_JPEGSIZE, LK_JPEGSIZE_RE))
  {
    UART_puts(UART_GNSS, "6");
    return false;
  }
  jpegsize = UART_getw4(UART_CAMERA);
  number_of_pages = jpegsize/page_size;
 //unnecessary for the operation, just for the debugging, takes time to print----------------------------
  char jp_output[100];
  sprintf(jp_output, "jpeg size is: %d\r\n", jpegsize); //output of the sprintf is the char buffer specified in first argument
  UART_puts(UART_GNSS, jp_output); //print this message about size of jpeg
 //------------------------------------------------------------------------------------------------------
  Delay_ms(100);
  // offset in file start (0 here) - place where in the command is starting address
  LK_READPICTURE[6] = 0x00;
  LK_READPICTURE[7] = 0x00;
  LK_READPICTURE[8] = 0x00;
  LK_READPICTURE[9] = 0x00;
  // write length to obtain +8 bytes?
  LK_READPICTURE[10] = (char)((page_size >> 24) & 0xFF);
  LK_READPICTURE[11] = (char)((page_size >> 16) & 0xFF);
  LK_READPICTURE[12] = (char)((page_size >> 8) & 0xFF);
  LK_READPICTURE[13] = (char)(page_size & 0xFF);
  // 0.1ms
  LK_READPICTURE[14] = LK_PICTURE_TIME_dot1ms[0];
  LK_READPICTURE[15] = LK_PICTURE_TIME_dot1ms[1];

  //UART_puts(UART_GNSS, "In Camera driver\r\n");
  //for(i=0; i<jpegsize && endfoundcount<2;i++)
  while(!endflag)
  {
    if(!Camera_command(LK_READPICTURE, LK_READPICTURE_RE))
    {
      return false;
    }
    Delay_ms(10);
    //DELAYS THE READING OF CHARACTER  FROM CAMERA SO GETS STUCK IN LOOP
    //UART_puts(UART_EPS, "In Camera driver loop\r\n");
    for(int count = 0; count<page_size; count++){
        page_buffer[count] = UART_getc(UART_CAMERA);  //characters in line
        if((page_buffer[count-1]==0xFF) && (page_buffer[count]==0xD9)) endflag = true;
    }
    Delay_ms(10);
    while(UART_charsAvail(UART_CAMERA)){
      UART_getc_nonblocking(UART_CAMERA, &bin);
    }
  //Buffer_store_new_data(page_buffer);
  UART_putb(UART_GNSS, page_buffer, sizeof(page_buffer));
  }
  return true;
}
void set_compression(char compression){
    LK_COMPRESSION[8] = compression;
    LK_COMPRESSION_RE[5] = compression;
}