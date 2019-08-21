/*
 * main.c
 *
 *  Created on: 7/08/2019
 *      Author: kfon596
 */

#include <system.h>
#include <altera_avalon_pio_regs.h>
#include <stdio.h>
#include "sccharts.h"

#define ESC 27
#define CLEAR_LCD_STRING "[2J"

unsigned int buttonValue = 0, buttonValuePrevious = 0;
char pA, pB, pR, pO;
FILE *lcd;

int main() {
	// Initialise
	reset();

	lcd = fopen(LCD_NAME, "w");

	fprintf(lcd, "%c%s", ESC, CLEAR_LCD_STRING);
	fprintf(lcd, "A: %d, B: %d, R: %d\n", A, B, R);
	fprintf(lcd, "O: %d\n", O);

	while(1)  {
		// Fetch button inputs
		// A is Key 2, B is Key 1, R is Key 0
		// Remember that keys are active low
		buttonValue = IORD_ALTERA_AVALON_PIO_DATA(KEYS_BASE);
		if(buttonValue == 0b011){
			R = 0;
			A = 1;
		}
		else if(buttonValue == 0b101){
			R = 0;
			B = 1;
		}
		else if(buttonValue == 0b110){
			R = 1;
			A = 0;
			B = 0;
		}

		if(!((pA == A) && (pB == B) && (pR == R) && (pO == O))){
			fprintf(lcd, "%c%s", ESC, CLEAR_LCD_STRING);
			fprintf(lcd, "A: %d, B: %d, R: %d\n", A, B, R);
			fprintf(lcd, "O: %d\n", O);
		}

		pA = A;
		pB = B;
		pR = R;
		pO = O;

		// Output O to Red LED
		if(O == 1){
			IOWR_ALTERA_AVALON_PIO_DATA(LEDS_RED_BASE, 0b111111111111111111);
		}else{
			IOWR_ALTERA_AVALON_PIO_DATA(LEDS_RED_BASE, 0);
		}

		// Do a tick!
		tick();

		buttonValuePrevious = buttonValue;
	}

	fclose(lcd);
	return 0;
}
