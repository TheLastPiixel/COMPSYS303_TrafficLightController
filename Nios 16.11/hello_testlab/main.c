/*
 * main.c
 *
 *  Created on: 7/08/2019
 *      Author: kfon596
 */

#include <system.h>
#include <altera_avalon_pio_regs.h>
#include "sys/alt_alarm.h"
#include <alt_types.h> // alt_u32 is a kind of alt_types
#include <sys/alt_irq.h> // to register interrupts
#include <stdio.h>

#define ESC 27
#define CLEAR_LCD_STRING "[2J"

int ButtonPressed;
int Button1 = 0;
int Button0 = 0;

FILE *lcd;
int mode, previousMode, timeCountMain, buttonValue = 0;

// Enum for the car traffic lights.
enum traffic{
	RR, RG, RY, GR, YR
};

enum NSEW_traffic_next{
	NS, WE
};

// Enum for the pedestrian lights. Two for NS and EW are made respectively.
enum pedestrian{
	idle, pressed
};

/*
 * Declaration of variables
 */
alt_alarm timer;
void* timerContext;
void* context_going_to_be_passed = (void*) &buttonValue;
enum pedestrian pedNS, pedWE;
enum traffic traffic;
enum NSEW_traffic_next next_traffic;

/*
 * Initialisers
 */
//Initializes the interrupts for the NS and EW pedestrian buttons

void init_buttons_pio(void* context, alt_u32 id){
	int* temp = (int*) context; // need to cast the context first before using it
	(*temp) = IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE);

	//Allows button 1 and 0 to be click simultaneously
	if (Button0 == 0)
	{
		Button0 = *temp & 0b00001;
	}
	if (Button1 == 0)
	{
		Button1 = *temp & 0b00010;
	}
	
	// Clear the edge capture register
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0);

	// Set button state.
	ButtonPressed = (int) (*temp);
}


/*
 * Interrupts
 */
//handler for the traffic light timer interrupt
alt_u32 tlc_timer_isr(void* context){
	int *timeCount = (int*) context;
	(*timeCount)++;
	printf("time:%d\n", *timeCount);
	return 100;
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
void lcd_set_mode(){
	if(lcd != NULL){
		fprintf(lcd, "%c%s", ESC, CLEAR_LCD_STRING);
		fprintf(lcd, "MODE: %d\n", mode);
	}
}

//parses the configuration string and updates the timeouts
void timeout_data_handler(){

}

/*
 * Implementers
 */
//implements the simple traffic light controller
void simple_tlc(){
	switch(traffic){
		case RR:
			// Set green leds
			IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00100100);

			if(timeCountMain >= 5){
				// next traffic decides whether to go RG or GR
				if(next_traffic == NS){
					traffic = RG;
				}else{
					traffic = GR;
				}
				alt_alarm_stop(&timer);
				timeCountMain = 0;
				// start the timer, with timeout of 0.5 seconds
				alt_alarm_start(&timer, 100, tlc_timer_isr, timerContext);
			}

			break;

		case RG:
				// Set green leds
				IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00100001);

				// Set next traffic not to re-do RG
				if(next_traffic == NS){
					next_traffic = WE;
				}

				if(timeCountMain >= 60){
					traffic = RY;

					alt_alarm_stop(&timer);
					timeCountMain = 0;
					// start the timer, with timeout of 6 seconds
					alt_alarm_start(&timer, 100, tlc_timer_isr, timerContext);
				}
				break;
		case RY:
			// Set green leds
			IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00100010);

			if(timeCountMain >= 20){
				traffic = RR;

				alt_alarm_stop(&timer);
				timeCountMain = 0;
				// start the timer, with timeout of 6 seconds
				alt_alarm_start(&timer, 100, tlc_timer_isr, timerContext);
			}
			break;
		case GR:
			// Set green leds
			IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00001100);

			// Set next traffic not to re-do GR
			if(next_traffic == WE){
				next_traffic = NS;
			}
			if(timeCountMain >= 60){
				traffic = YR;

				alt_alarm_stop(&timer);
				timeCountMain = 0;
				// start the timer, with timeout of 6 seconds
				alt_alarm_start(&timer, 100, tlc_timer_isr, timerContext);
			}
			break;
		case YR:
			// Set green leds
			IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00010100);

			if(timeCountMain >= 20){
				traffic = RR;

				alt_alarm_stop(&timer);
				timeCountMain = 0;
				// start the timer, with timeout of 6 seconds
				alt_alarm_start(&timer, 100, tlc_timer_isr, timerContext);
			}
			break;

	}
}

//implements the configurable traffic light controller
void configurable_tlc(){

}

//implements the traffic light controller with integrated camera
void camera_tlc(){

}

//implements the pedestrian traffic light controller
void pedestrian_tlc(){
	if 
}

//simulates the entry and exit of vehicles at the intersection
void handle_vehicle_button(){

}

int main() {
	// Initialise
	pedNS = idle;
	pedWE = idle;
	traffic = RR;
	next_traffic = NS;

	/*
	 * Make sure only the 2 switches on the right will change the mode of the simulation.
	 * +1 will make sure mode is {1,2,3,4}.
	 */
	mode = (IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE) % 4) + 1;

	lcd = fopen(LCD_NAME, "w");

	/*
	 * Buttons Interrupt Setup
	 */
	context_going_to_be_passed = (void*) &buttonValue;
	// clears the edge capture register
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0);
	// enable interrupts for all buttons
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(KEYS_BASE, 0x7);
	// register the ISR
	alt_irq_register(KEYS_IRQ,context_going_to_be_passed,init_buttons_pio);

	/*
	 * Timer interrupt Setup
	 */


	// start the timer, with timeout of 100 milli-seconds
	timeCountMain = 0;
	timerContext = (void*) &timeCountMain;
	alt_alarm_start(&timer, 100, tlc_timer_isr, timerContext);

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

		/*
		 * Different modes
		 */
		if(mode == 1){
			simple_tlc();
		}
		if(mode == 2){
			pedestrian_tlc();
		}
		if(mode == 3){
			configurable_tlc();
		}
		if(mode == 4){
			camera_tlc();
		}

	}

	fclose(lcd);
	return 0;
}


