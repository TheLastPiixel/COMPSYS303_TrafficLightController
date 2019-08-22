/*
 * main.c
 *
 *  Created on: 7/08/2019
 *      Author: kfon596
 */

#include <system.h>
#include <altera_avalon_pio_regs.h>
#include <stdio.h>

#define ESC 27
#define CLEAR_LCD_STRING "[2J"

FILE *lcd;
int mode, previousMode;

//enum for the car traffic lights.
enum traffic{
	RR, RG, RY, GR, YG
};

//enum for the pedestrian lights. Two for NS and EW are made respectively.
enum pedestrian{
	idle, pressed
};

int main() {
	// Initialise
	enum pedestrian pedNS = idle;
	enum pedestrian pedWE = idle;
	enum traffic traffic = RR;

	/*
	 * Make sure only the 2 switches on the right will change the mode of the simulation.
	 * +1 will make sure mode is {1,2,3,4}.
	 */
	mode = (IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE) % 4) + 1;

	lcd = fopen(LCD_NAME, "w");

	while(1)  {

		/*
		 * This function gets the switches setup and prints the value of mode on LCD.
		 * LCD re-print only happens when the mode has changed.
		 * The mode can only change in the condition that traffic state is on RR.
		 */
		if(traffic == RR){
			mode = (IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE) % 4) + 1;
			if(mode != previousMode){
				lcd_set_mode(mode);
			}
			previousMode = mode;
		}

	}

	fclose(lcd);
	return 0;
}

/*
 * Initialisers
 */
//Initializes the interrupts for the NS and EW pedestrian buttons
void init_buttons_pio(){

}

/*
 * Interrupts
 */
//handler for the traffic light timer interrupt
void tlc_timer_isr(){

}

//handler for the red light camera timer interrupt
void camera_timer_isr(){

}

//handles the NS and EW pedestrian button interrupts
void NSEW_ped_isr(){

}


/*
 * Setters
 */
//write the current mode to the LCD
void lcd_set_mode(int mode){
	fprintf(lcd, "%c%s", ESC, CLEAR_LCD_STRING);
	fprintf(lcd, "MODE: %d\n", mode);
}

//parses the configuration string and updates the timeouts
void timeout_data_handler(){

}

/*
 * Implementers
 */
//implements the simple traffic light controller
void simple_tlc(){

}

//implements the configurable traffic light controller
void configurable_tlc(){

}

//implements the traffic light controller with integrated camera
void camera_tlc(){

}

//implements the pedestrian traffic light controller
void pedestrian_tlc(){

}

//simulates the entry and exit of vehicles at the intersection
void handle_vehicle_button(){

}






