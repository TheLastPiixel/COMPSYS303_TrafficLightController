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

/*
 * Traffic Light Signals
 * P - Pedestrian
 * T - traffic
 * NS - North-South
 * WE - East-West
 */
#define PWE_G 128
#define PNS_G 64
#define TWE_R 32
#define TWE_Y 16
#define TWE_G 8
#define TNS_R 4
#define TNS_Y 2
#define TNS_G 1

/*
 * Enum for the car traffic lights.
 */
enum traffic{
	RR, RG, RY, GR, YR
};

enum NSEW_traffic_next{
	NS, WE
};

/*
 * Enum for the pedestrian lights. Two for NS and EW are made respectively.
 */
enum pedestrian{
	idle, pressed, running
};

enum car_state{
	entered, exited
};

enum picture_taken{
	standby, monitoring, taken
};

/*
 * Declaration of variables
 */
alt_alarm timer, timer1;
void* timerContext;
void* timerCameraContext;
void* context_going_to_be_passed;
int LEDs = 0;
FILE *lcd;
int mode, previousMode, timeCountMain, timeCountCamera, buttonValue = 0;
enum pedestrian pedNS, pedWE;
enum traffic traffic;
enum NSEW_traffic_next next_traffic;
enum car_state car_action, previous_car_action;
enum picture_taken cam_0;


/*
 * Initialisers
 */
//Initializes the interrupts for the NS, EW pedestrian and car drive through buttons

void init_buttons_pio(void* context, alt_u32 id){
	int* temp = (int*) context; // need to cast the context first before using it
	(*temp) = IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE);

	// Clear the edge capture register
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0);

	if ((*temp) == 1 && traffic != RG){
		pedNS = pressed;
	}
	if ((*temp) == 2 && traffic != GR){
		pedWE = pressed;
	}
	if ((*temp) == 4 && mode == 4){
		if(car_action == exited){
			car_action = entered;
		}else{
			car_action = exited;
		}
	}

	(*temp) = 0;
}

/*
 * Interrupts
 */
//handler for the traffic light timer interrupt
alt_u32 tlc_timer_isr(void* context){
	int *timeCount = (int*) context;
	(*timeCount)++;
	return 100;
}

//handler for the red light camera timer interrupt
alt_u32 camera_timer_isr(void* context){
	int *timeCountCamera = (int*) context;
	(*timeCountCamera)++;
	return 100;
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
			LEDs = TWE_R + TNS_R;

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
			LEDs = TWE_R + TNS_G;

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
			LEDs = TWE_R + TNS_Y;

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
			LEDs = TWE_G + TNS_R;

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
			LEDs = TWE_Y + TNS_R;

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

//implements the pedestrian traffic light controller
void pedestrian_tlc(){
	 // Any state (pedNS and/or pedWE) that is in pressed state.
	 // Set respective running pedestrian signal back to idle on yellow light.
	switch(traffic){
		case RG:
			if(pedNS == pressed){
				pedNS = running;
			}
			if(pedNS == running){
				LEDs = LEDs + PNS_G;
			}
			break;
		case RY:
			if(pedNS == running){
				pedNS = idle;
			}
			break;
		case GR:
			if(pedWE == pressed){
				pedWE = running;
			}
			if(pedWE == running){
				LEDs = LEDs + PWE_G;
			}
			break;
		case YR:
			if(pedWE == running){
				pedWE = idle;
			}
			break;
		default:
			break;
	}
}

//implements the configurable traffic light controller
void configurable_tlc(){

}

//implements the traffic light controller with integrated camera
void camera_tlc(){
	if(timeCountCamera >= 20 && cam_0 == monitoring){
		printf("Snapshot Taken!\n");
		cam_0 = taken;
	}
	switch(traffic){
		case YR:

			// Check that car has entered/exited

			//First if statement checks for recent change in car action.
			if(car_action == entered && previous_car_action != car_action){
				printf("Camera activated\n");
				alt_alarm_start(&timer1, 100, camera_timer_isr, timerCameraContext);
				cam_0 = monitoring;

			//Check that the car has exited. Report time taken
			}else if(cam_0 != standby && car_action == exited){
				printf("Vehicle left\n");
				printf("Vehicle time in intersection taken: %d.%d seconds\n\n",
						timeCountCamera / 10, timeCountCamera % 10);
				alt_alarm_stop(&timer1);
				timeCountCamera = 0;
				cam_0 = standby;
			}
			break;

		case RY:
			// Check that car has entered/exited

			//First if statement checks for recent change in car action.
			if(car_action == entered && previous_car_action != car_action){
				printf("Camera activated\n");
				alt_alarm_start(&timer1, 100, camera_timer_isr, timerCameraContext);
				cam_0 = monitoring;

			//Check that the car has exited. Report time taken
			}else if(cam_0 != standby && car_action == exited){
				printf("Vehicle left\n");
				printf("Vehicle time in intersection taken: %d.%d seconds\n\n",
						timeCountCamera / 10, timeCountCamera % 10);
				alt_alarm_stop(&timer1);
				timeCountCamera = 0;
				cam_0 = standby;
			}
			break;

		default:
			/*
			 * Other states such as RG, GR and RR.
			 * Take picture immediately when car passes on button click.
			 */

			// Check for change in car action.
			if(previous_car_action != car_action){

				//Check that the snapshot is not taken and changed to entered state.
				if(car_action == entered && cam_0 == standby){
					printf("Camera activated\n");
					alt_alarm_start(&timer1, 100, camera_timer_isr, timerCameraContext);
					printf("Snapshot Taken!\n");
					cam_0 = taken;

				//Anytime the vehicle exits
				}else if(car_action == exited){
					printf("Vehicle left\n");
					printf("Vehicle time in intersection taken: %d.%d seconds\n\n",
							timeCountCamera / 10, timeCountCamera % 10);
					alt_alarm_stop(&timer1);
					timeCountCamera = 0;
					cam_0 = standby;
				}
			}

			break;
	}
	previous_car_action = car_action;
}




int main() {
	// Initialise
	car_action = exited;
	previous_car_action = exited;
	pedNS = idle;
	pedWE = idle;
	traffic = RR;
	next_traffic = NS;
	cam_0 = standby;

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
	// start the tlc timer, with timeout of 100 milli-seconds
	timeCountMain = 0;
	timerContext = (void*) &timeCountMain;
	alt_alarm_start(&timer, 100, tlc_timer_isr, timerContext);

	timeCountCamera = 0;
	timerCameraContext = (void*) &timeCountCamera;

	/*
	// setup camera timer
	timeCountCamera = 0;
	timerCameraContext = (void*) &timeCountCamera;
	alt_alarm_start(&timer1, 100, NSEW_ped_isr, timerCameraContext);
*/
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
				car_action = exited;
			}
			previousMode = mode;
		}

		/*
		 * Different modes
		 */
		// If mode is not 3 because modes 1, 2 and 4 uses simple_tlc
		if(mode != 3){
			simple_tlc();

			//Only mode one does not utilise pedestrian crossing
			if(mode == 2){
				pedestrian_tlc();
			}
			if(mode == 4){
				camera_tlc();
			}
		}else{
			configurable_tlc();
		}


		//Turns on LEDs based on TLC
		IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, LEDs);

	}

	fclose(lcd);
	return 0;
}


