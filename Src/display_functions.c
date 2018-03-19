/**********************************************************************************************************
*
*@file display_functions.c
*@author William Lewin
*@date 2018-02-28
*@version 1.0
*@breif All the functions that regard communication with 
*the display.
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


//Time definitions
static RTC_TimeTypeDef sTime;
static RTC_DateTypeDef sDate;
static uint32_t h = 0;
static uint32_t m = 0;
static uint32_t s = 0;
static uint16_t previoussecond = 0;

//Bugfix for display init
static uint8_t first = 0;
static uint8_t second = 0;
static uint8_t third = 0;
ITStatus UartReady = RESET;
static uint8_t dataByte [3] = 0;


/**
*@brief Prepares a bitstream to be sent to the display
*@param rs RS-bit
*@param rw RW-bit
*@param data The byte which is to be sent to the display
*@return void
*@author William Lewin
*@version 1.0
*@date 2018-02-28
*/

void display_send(uint8_t rs, uint8_t rw, uint8_t data){
  
  uint8_t firstbyte = 0x1F; //0 RS(0) RW(0) 11111
  uint8_t secondbyte = 0x00; // 1111 0000
  uint8_t thirdbyte = 0x00;  // 1111 0000
  rw = rw << 5; // 0 0 RW 00000
  firstbyte = firstbyte | rw; 
  rs = rs << 6; // 0 RS 0 00000
  firstbyte = firstbyte | rs; // 0 RS RW 11111
  first = firstbyte;
  
  
  secondbyte = secondbyte | (data & 0x0F);   //0000 D0D1D2D3
  second = secondbyte; 
  
  data = data & 0xF0; // Mask first 4
  data = data >> 4; // Shift masked to last 4
  thirdbyte = thirdbyte | data; // 0000 D4D5D6D7
  third = thirdbyte;
  
  dataByte[0] = first;
  dataByte[1] = second;
  dataByte[2] = third;
  //Enables information transmission
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi2, dataByte,3,1000);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}


/**
*@brief Sends a stream of data to the display for initialisation.
*@param void
*@return void
*@author William Lewin
*@version 1.0
*@date 2018-02-28
*/

void display_init(void){
  display_send(0,0,0x3A);
  display_send(0,0,0x09);
  display_send(0,0,0x06);
  display_send(0,0,0x1E);
  display_send(0,0,0x39);
  display_send(0,0,0x1B);
  display_send(0,0,0x6E);
  display_send(0,0,0x56);
  display_send(0,0,0x7A);
  display_send(0,0,0x38);
  display_send(0,0,0x0F);
  display_send(0,0,0x01);
}

/** 
@brief  Tx Transfer completed callback
@param  UartHandle: UART handle.
@note   This example shows a simple way to report end of IT Tx transfer, and  
you can add your own implementation.
@retval None 
*/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle){
  /* Set transmission flag: trasfer complete*/
  UartReady = SET;
}




/**
*@brief Get current time structures from RTC and user info
*@param void
*@return void
*@author William Lewin
*@version 1.0
*@date 2018-03-01
*/


void current_time(void){
  uint8_t Buffer[] = "\n\rMata in HH:MM:SS\n\r";
  if(HAL_UART_Transmit_IT(&huart3, (uint8_t *)Buffer, sizeof(Buffer))!= HAL_OK){
    Error_Handler();   
  }
  /* Wait for the end of the transfer*/    
  while (UartReady != SET);
  /* Reset transmission flag*/
  UartReady = RESET;
  
  
  
  // Väntar på 2 chars i varje del i 10 sekunder var
  while(HAL_UART_Receive(&huart3, (uint8_t *)&h, 2, 10000) != HAL_OK);
  while(HAL_UART_Receive(&huart3, (uint8_t *)&m, 2, 10000) != HAL_OK);
  while(HAL_UART_Receive(&huart3, (uint8_t *)&s, 2, 10000) != HAL_OK);
  
  HAL_RTC_GetDate(&hrtc,&sDate,RTC_FORMAT_BIN);
  HAL_RTC_GetTime(&hrtc,&sTime,RTC_FORMAT_BIN);
  //Get the time from user
  sscanf((uint8_t *)&h,"%hhu", &sTime.Hours);
  sscanf((uint8_t *)&m,"%hhu", &sTime.Minutes);
  sscanf((uint8_t *)&s,"%hhu", &sTime.Seconds);
  //Set the time from user
  HAL_RTC_SetDate(&hrtc,&sDate,RTC_FORMAT_BIN);
  HAL_RTC_SetTime(&hrtc,&sTime,RTC_FORMAT_BIN);
  
}

/**
*@brief 
*@param void
*@return void
*@author William Lewin
*@version 1.0
*@date 2018-03-01
*/

void print_time(void){
  //Plats för time
  char time_array[8];
  //Extra plats för bluetooth-kompabilitet
  char bluetooth_time_array[12];
  
  HAL_RTC_GetDate(&hrtc,&sDate,RTC_FORMAT_BIN);
  HAL_RTC_GetTime(&hrtc,&sTime,RTC_FORMAT_BIN);
  sprintf(time_array, "%2d:%2d:%2d",sTime.Hours,sTime.Minutes,sTime.Seconds);
  sprintf(bluetooth_time_array,"%2d:%2d:%2d\n\r",sTime.Hours,sTime.Minutes);
  if(previoussecond != sTime.Seconds){
    int i;
    //Rad 1
    display_send(0,0,0x80);
    for(i=0; i<8;i++){
      display_send(1,0,time_array[i]);
    }
      //Send to bluetooth
    if(HAL_UART_Transmit(&huart2, (uint8_t *) bluetooth_time_array,12,100)!= HAL_OK){
     Error_Handler();
    }
  } 
  previoussecond = sTime.Seconds;
}

