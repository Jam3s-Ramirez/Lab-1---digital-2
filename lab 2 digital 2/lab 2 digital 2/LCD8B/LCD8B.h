/*
 * LCD8B.h
 *
 * Created: 19/07/2024 11:52:08
 *  Author: James Ramirez
 */ 

#ifndef LCD8B_H_
#define LCD8B_H_

#include <avr/io.h>
#include <stdint.h>
#define F_CPU 16000000
#include <util/delay.h>


void Lcd_Init8bits();

void inicio(char a);    //Función de repeticion de comandos

void dato_a_mostrar(char a);

void Lcd_Clear();

void Lcd_Set_Cursor(char a, char b);

void Lcd_Write_Char(char data);

void Lcd_Write_String(char *a);

void Lcd_Shift_Right();

void Lcd_Shift_Left();

#endif /* LCD8B_H_ */