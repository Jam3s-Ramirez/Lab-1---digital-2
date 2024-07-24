/**************************
Universidad del Valle de Guatemala
Electrónica digital 2
Proyecto: Lab 2
Hardware: ATMEGA328p
Created: 19/07/2024 10:52:46
Author : James Ramírez
***************************/

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "LCD8B/LCD8B.h"
#include "ADC/ADC.h"

// Definición de la velocidad de comunicación UART
#define BAUD_RATE 9600
#define UBRR_VALUE ((F_CPU / 16 / BAUD_RATE) - 1)

// Variables globales
volatile uint8_t sensor1 = 0, sensor2 = 0, switchCase = 0;
char display1[8], display2[8], counterDisplay[4] = {'0', '0', '0', '0'};
volatile uint8_t counter = 0;
volatile uint8_t lcdUpdateFlag = 0;  // Indicador de actualización de LCD
volatile uint8_t adcFlag = 0;        // Indicador de actualización de ADC

// Prototipos de funciones
void initializeSystem(void);
void updateVoltageDisplay(char *buffer, uint8_t value);
void sendUART(char data);
void sendStringUART(char* str);
void updateCounterDisplay(char *buffer, int value);
void refreshLCD(void);

// Inicialización del sistema
void initializeSystem(void) {
	cli();  // Deshabilitar interrupciones globales
	DDRD = 0xFF;  // Configurar puerto D como salida
	DDRB = 0xFF;  // Configurar puerto B como salida
	DDRC = 0;     // Configurar puerto C como entrada

	// Configuración UART
	UBRR0H = (UBRR_VALUE >> 8);  // Configurar la parte alta del baud rate
	UBRR0L = UBRR_VALUE;         // Configurar la parte baja del baud rate
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);  // Habilitar transmisión, recepción e interrupción RX
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // Configurar formato: 8 bits de datos, 1 bit de parada

	Lcd_Init8bits();  // Inicializar la pantalla LCD
	_delay_ms(50);    // Esperar para la inicialización de la LCD
	Lcd_Clear();      // Limpiar la pantalla LCD
	initADC();        // Inicializar el ADC
	ADCSRA |= (1 << ADSC);  // Iniciar la primera conversión del ADC

	sei();  // Habilitar interrupciones globales
}

// Convertir un valor ADC a voltaje y actualizar la cadena de caracteres
void updateVoltageDisplay(char *buffer, uint8_t value) {
	float voltage = (value * 5.0) / 255.0;
	uint16_t intPart = (uint16_t)voltage;
	uint16_t decPart = (uint16_t)((voltage - intPart) * 100);  // Dos decimales

	if (intPart < 10) {
		buffer[0] = '0' + intPart;
		buffer[1] = '.';
		buffer[2] = '0' + (decPart / 10);
		buffer[3] = '0' + (decPart % 10);
		buffer[4] = 'V';
		buffer[5] = '\0';
		} else {
		buffer[0] = '0' + (intPart / 10);
		buffer[1] = '0' + (intPart % 10);
		buffer[2] = '.';
		buffer[3] = '0' + (decPart / 10);
		buffer[4] = '0' + (decPart % 10);
		buffer[5] = 'V';
		buffer[6] = '\0';
	}
}

// Actualizar la cadena de caracteres con el valor del contador
void updateCounterDisplay(char *buffer, int value) {
	buffer[0] = '0' + (value / 100);
	buffer[1] = '0' + ((value / 10) % 10);
	buffer[2] = '0' + (value % 10);
	buffer[3] = '\0';
}

// Enviar un dato por UART
void sendUART(char data) {
	while (!(UCSR0A & (1 << UDRE0)));  // Esperar hasta que el buffer esté vacío
	UDR0 = data;  // Enviar el dato
}

// Enviar una cadena de caracteres por UART
void sendStringUART(char* str) {
	while (*str) {
		sendUART(*str++);
	}
}

// Actualizar la pantalla LCD
void refreshLCD(void) {
	Lcd_Clear();  // Limpiar la pantalla
	Lcd_Set_Cursor(0, 1);
	Lcd_Write_String("S1:");  // Mostrar etiqueta del Sensor 1
	Lcd_Set_Cursor(0, 7);
	Lcd_Write_String("S2:");  // Mostrar etiqueta del Sensor 2
	Lcd_Set_Cursor(0, 13);
	Lcd_Write_String("S3:");  // Mostrar etiqueta del Sensor 3

	// Actualizar las cadenas de caracteres con los valores actuales
	updateVoltageDisplay(display1, sensor1);
	updateVoltageDisplay(display2, sensor2);
	updateCounterDisplay(counterDisplay, counter);

	// Mostrar los valores en la LCD
	Lcd_Set_Cursor(1, 1);
	Lcd_Write_String(display1);
	Lcd_Set_Cursor(1, 7);
	Lcd_Write_String(display2);
	Lcd_Set_Cursor(1, 13);
	Lcd_Write_String(counterDisplay);
}

// Función principal
int main(void) {
	initializeSystem();  // Inicializar el sistema

	// Variables para almacenar los valores anteriores
	uint8_t previousSensor1 = 255, previousSensor2 = 255, previousCounter = 255;

	while (1) {
		// Verificar si hay cambios en los valores del ADC o el contador
		if ((sensor1 != previousSensor1) || (sensor2 != previousSensor2) || (counter != previousCounter) || lcdUpdateFlag) {
			refreshLCD();  // Actualizar la pantalla LCD
			// Guardar los valores actuales como anteriores
			previousSensor1 = sensor1;
			previousSensor2 = sensor2;
			previousCounter = counter;
			adcFlag = 0;
			lcdUpdateFlag = 0;
		}

		// Actualizar las cadenas de caracteres con los valores actuales
		updateVoltageDisplay(display1, sensor1);
		updateVoltageDisplay(display2, sensor2);
		updateCounterDisplay(counterDisplay, counter);

		// Enviar los valores actuales por UART
		sendStringUART("S1: ");
		sendStringUART(display1);
		sendStringUART(" S2: ");
		sendStringUART(display2);
		sendStringUART(" S3: ");
		sendStringUART(counterDisplay);
		sendUART('\n');
		
		_delay_ms(100);  // Esperar 100ms
	}
}

// Interrupción del ADC
ISR(ADC_vect) {
	if (switchCase == 0) {
		ADMUX &= ~((1 << MUX2) | (1 << MUX1) | (1 << MUX0));  // Seleccionar canal ADC0
		sensor1 = ADCH;  // Leer valor alto del ADC
		switchCase = 1;  // Cambiar a caso 1
		} else {
		ADMUX = (ADMUX & ~((1 << MUX2) | (1 << MUX1) | (1 << MUX0))) | (1 << MUX0);  // Seleccionar canal ADC1
		sensor2 = ADCH;  // Leer valor alto del ADC
		switchCase = 0;  // Cambiar a caso 0
	}
	ADCSRA |= (1 << ADSC);  // Iniciar la próxima conversión del ADC
	adcFlag = 1;  // Indicar que se debe actualizar la LCD
}

// Interrupción UART para recibir datos
ISR(USART_RX_vect) {
	char receivedChar = UDR0;  // Leer dato recibido
	if (receivedChar == '+') {
		if (counter < 255) {
			counter++;  // Incrementar el contador si es menor a 255
		}
		} else if (receivedChar == '-') {
		if (counter > 0) {
			counter--;  // Decrementar el contador si es mayor a 0
		}
	}
	lcdUpdateFlag = 1;  // Indicar que se debe actualizar la LCD
}
