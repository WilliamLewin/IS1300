/**********************************************************************************************************
*
*@file display_functions.h
*@author William Lewin
*@date 2018-02-28
*@version 1.0
*@breif Header function for display_functions.c
*
***********************************************************************************************************
*/  

#ifndef DISPLAY_FUNCTIONS_H
#define DISPLAY_FUNCTIONS_H

void display_send(uint8_t rs, uint8_t rw, uint8_t data);
void display_init(void);
void current_time(void);
void print_time(void);

#endif  