/**********************************************************************************************************
*
*@file temp_functions.c
*@author William Lewin
*@date 2018-02-26
*@version 1.0
*@breif All the functions regarding the temperature and bitstream handling from a 433Mhz Telldus sensor.
*
***********************************************************************************************************
*/  

#include "main.h"
#include "stm32f3xx_hal.h"
#include "crc.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "display_functions.h"
#include "temp_functions.h"

static uint8_t preamble =0;
static uint32_t data = 0;
static uint8_t crc = 0;
static uint8_t counter = 0;
static uint8_t humidity = 0;
static uint16_t temperature = 0;
static uint8_t crccheck = 0;
static uint8_t crccounter = 0;
static uint8_t preamblecounter =0;
static uint8_t pastValueTemp = 0;
static uint8_t pastValueHumid = 0;




/**
*@brief Resets the variables and counters that belongs in the other functions
*@param none
*@return none
*@author William Lewin
*@version 1.0
*@date 2018-02-22
*/

void reset(void){
  preamble = 0;
  data = 0;
  crc = 0;
  counter = 0;
  crccounter = 0; 
}

/**
*@brief Adds a one or a zero to either the preamble, data or crc 
* depending on wether the preamble is full, shifts useful info,
* and performs crc check
*@param bit The bit that shall be added to the bitstream, 1/0
*@return 32-bit unsigned int with the databits from the bitstream
*@author William Lewin
*@version 1.0
*@date 2018-02-22
*/

uint32_t addBit(uint8_t bit){
  //Mata in 1 i Preamble
  if(preamble != 0xff){
    preamble = preamble << 1;
    preamble = preamble | bit;
    preamblecounter++;
  }
  //Mata in bit i CRC
  else if(counter == 32){
    crc = crc << 1;
    crc = crc | bit;
    crccounter++;
    crccheck = HAL_CRC_Calculate(&hcrc,  &data, 1);
    
    if (crccounter == 8){
      //CRC inkorrekt
      if(crccheck != crc){ 
        reset();
      }
      
      humidity = data & 0x7F;
      temperature = (data >> 8) & 0x2FF;
      //Printa till kommandotolk
     printf("temp: %d c \n", temperature);
      printf("humid: %d% \n" , humidity);
      return data;
    }
    
  }
  //Mata in 1 i data
  else if(bit == 1){
    data = data << 1;
    data = data | 0x1;
    counter++;
  }
  //Mata in 0 i data
  else if(bit == 0){
    data = data << 1;
    data = data & 0xFFFFFFFE;
    counter++;
  }
  return(data);
}

/**
*@brief Prints the active temperature to the display with display_send
*@param void
*@return void
*@author William Lewin
*@version 1.0
*@date 2018-02-22
*/

void print_temp(void){
if(pastValueTemp != temperature){
  //Plats för temp
  char temp_array[6];
  //Extra plats för bluetooth-kompabilitet
  char bluetooth_temp_array[14];
  
  double temperaturedouble = ((double)(temperature)/10);
  //Gör om till char-array
  sprintf(temp_array, "%.1f C", temperaturedouble); 
  sprintf(bluetooth_temp_array,"\n\r%.1fC\n\r", temperaturedouble);
  int i;
  //Rad 2
  display_send(0,0,0xA0);
  for(i=0; i<6;i++){
    display_send(1,0,temp_array[i]);
  }
  //Send to bluetooth
  if(HAL_UART_Transmit(&huart2, (uint8_t *) bluetooth_temp_array,14,100)!= HAL_OK){
    Error_Handler();
    }
  
  pastValueTemp = temperature;
  
}
}

/**
*@brief Prints the active humidity to the display with display_send
*@param void
*@return void
*@author William Lewin
*@version 1.0
*@date 2018-02-22
*/

void print_humidity(void){
  if(pastValueHumid != humidity){
  //Plats för humid
  char humidity_array[4];
  char bluetooth_humidity_array[7];
  sprintf(humidity_array, "%d %", humidity); 
  //For extra space with bluetooth compability
  sprintf(bluetooth_humidity_array,"\n\r%d%%", humidity);
  
  int i;
  //Rad 3
  display_send(0,0,0xC0);
  for(i=0; i<4;i++){
    display_send(1,0,humidity_array[i]);
  }
  //Send to bluetooth
   if(HAL_UART_Transmit(&huart2, (uint8_t *) bluetooth_humidity_array,7,100)!= HAL_OK){
      Error_Handler();
    }
  pastValueHumid = humidity;
  }
}

