/*************************************************************************
 * The main code for our game
 *
 ************************************************************************/
#include "Touch_Sen.h"
#include "utils.h"
#include "3140_concur.h"
#include "realtime.h"
#include "game.h"
#include <time.h>
#include <stdlib.h>

/* an integer between 3^0 and 3^5, representing the 5 tasks the user will have to do */
int tasks = 0;
//the interrupt handlers for the respective buttons set these
int pressed_sw1 = 0;
int pressed_sw3 = 0;
int x_curr = 0;
int x_prev = 0;
int accuracy = 0;
//the timer for each level
int timer = 0;
/*------------------*/
/* Helper functions -- This code is from the process.c included in Lab5 */
/*------------------*/
void shortDelay(){delay();}
void mediumDelay() {delay(); delay();}
/*
 * These globals are taken from the discussion 5 switch demo
 */
PORT_Type* global_PORTC = PORTC;
GPIO_Type* global_PTC = PTC;
const int SWITCH_1_PIN = 3;
const int SWITCH_3_PIN = 12;
SIM_Type* global_SIM = SIM;
/*------------------*/
/* Functions required by game.h */
/*------------------*/
/*
 * Inits the random pin (PTA17) we use as a seed for our random # gen
 */
void pin_17_Initialize(void){

	SIM->SCGC5    |= SIM_SCGC5_PORTA_MASK;  /* Enable Clock to Port A */
	//
	PORTA->PCR[17] = (1 <<  8) ;               /* Pin PTA17 is GPIO */
	PTA->PDDR = (0 << 17 );          /* enable PTA17 as Input */


}
void SW1_Initialize(void){
	// setup switch 1
	SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK; //Enable the clock to port C
	PORTC->PCR[SWITCH_1_PIN] &= ~PORT_PCR_MUX(0b111); // Clear PCR Mux bits for PTC3
	PORTC->PCR[SWITCH_1_PIN] |= PORT_PCR_MUX(0b001); // Set up PTC3 as GPIO
	PTC->PDDR &= ~GPIO_PDDR_PDD(1 << SWITCH_1_PIN); // make it input
	PORTC->PCR[SWITCH_1_PIN] |= PORT_PCR_PE(1); // Turn on the pull enable
	PORTC->PCR[SWITCH_1_PIN] |= PORT_PCR_PS(1); // Enable the pullup resistor
	PORTC->PCR[SWITCH_1_PIN] &= ~PORT_PCR_IRQC(0b1111); // Clear IRQC bits for PTC3
	PORTC->PCR[SWITCH_1_PIN] |= PORT_PCR_IRQC(0b1010); // Set up the IRQC to interrupt when FALLING EDGE
	// configure NVIC so that interrupt is enabled
	NVIC_EnableIRQ(PORTC_PORTD_IRQn);
	//Note: all of this is from discussion 5

	//below: just ensure that sw1 does not register as clicked immediately
	pressed_sw1 = 0;
}
/*
 * The interrupt handler for the switches
 * One interrupt handler for this port -- so we make a switch statement based on which button it was
 */
void PORTC_PORTD_IRQHandler(void) {
	if (PORTC->ISFR == (1<<SWITCH_1_PIN)){
		pressed_sw1 = 1;
		PORTC->PCR[SWITCH_1_PIN] |= PORT_PCR_ISF(1);  // clear the interrupt status flag by writing 1 to it
	}else if (PORTC->ISFR == (1<<SWITCH_3_PIN)){
		pressed_sw3 = 1;
		PORTC->PCR[SWITCH_3_PIN] |= PORT_PCR_ISF(1);  // clear the interrupt status flag by writing 1 to it
	}

}
void SW3_Initialize(void){
	// setup switch 3
	SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK; //Enable the clock to port C
	PORTC->PCR[SWITCH_3_PIN] &= ~PORT_PCR_MUX(0b111); // Clear PCR Mux bits for PTC3
	PORTC->PCR[SWITCH_3_PIN] |= PORT_PCR_MUX(0b001); // Set up PTC3 as GPIO
	PTC->PDDR &= ~GPIO_PDDR_PDD(1 << SWITCH_3_PIN); // make it input
	PORTC->PCR[SWITCH_3_PIN] |= PORT_PCR_PE(1); // Turn on the pull enable
	PORTC->PCR[SWITCH_3_PIN] |= PORT_PCR_PS(1); // Enable the pullup resistor
	PORTC->PCR[SWITCH_3_PIN] &= ~PORT_PCR_IRQC(0b1111); // Clear IRQC bits for PTC3
	PORTC->PCR[SWITCH_3_PIN] |= PORT_PCR_IRQC(0b1010); // Set up the IRQC to interrupt ON FALLING EDGE
	// configure NVIC so that interrupt is enabled
	NVIC_EnableIRQ(PORTC_PORTD_IRQn);
	//Note: all of this is from discussion 5

	//below: just ensure that sw3 does not register as clicked immediately
	pressed_sw3 = 0;
}

void accel_Initialize(void){

}
/*
 * Credit to Ben Roloff : https://forum.digikey.com/t/using-the-capacitive-touch-sensor-on-the-frdm-kl46z/13246
 */
void ts_Initialize(void){
	// Enable clock for TSI PortB 16 and 17
		SIM->SCGC5 |= SIM_SCGC5_TSI_MASK;
		TSI0->GENCS = TSI_GENCS_OUTRGF_MASK |  // Out of range flag, set to 1 to clear
		//TSI_GENCS_ESOR_MASK |  // This is disabled to give an interrupt when out of range.  Enable to give an interrupt when end of scan
		TSI_GENCS_MODE(0u) |  // Set at 0 for capacitive sensing.  Other settings are 4 and 8 for threshold detection, and 12 for noise detection
		TSI_GENCS_REFCHRG(0u) | // 0-7 for Reference charge
		TSI_GENCS_DVOLT(0u) | // 0-3 sets the Voltage range
		TSI_GENCS_EXTCHRG(0u) | //0-7 for External charge
		TSI_GENCS_PS(0u) | // 0-7 for electrode prescaler
		TSI_GENCS_NSCN(31u) | // 0-31 + 1 for number of scans per electrode
		TSI_GENCS_TSIEN_MASK | // TSI enable bit
		//TSI_GENCS_TSIIEN_MASK | //TSI interrupt is disabled
		TSI_GENCS_STPE_MASK | // Enables TSI in low power mode
		//TSI_GENCS_STM_MASK | // 0 for software trigger, 1 for hardware trigger
		//TSI_GENCS_SCNIP_MASK | // scan in progress flag
		TSI_GENCS_EOSF_MASK ; // End of scan flag, set to 1 to clear
		//TSI_GENCS_CURSW_MASK; // Do not swap current sources
		// The TSI threshold isn't used is in this application
		//	TSI0->TSHD = 	TSI_TSHD_THRESH(0x00) | TSI_TSHD_THRESL(0x00);
}
void setup(void){
	LED_Initialize();
	pin_17_Initialize();
	SW1_Initialize();
	SW3_Initialize();
	ts_Initialize();
	tasks = choose_tasks();
}
void start_countdown(void){
	int i = 3;
	while (i > 0){
		LEDGreen_Toggle();
		LEDRed_Toggle();
		mediumDelay();
		LEDGreen_Toggle();
		LEDRed_Toggle();
		mediumDelay();
		i-=1;
	}
}
int choose_tasks(void){
	srand(PTA->PDIR);   // Initialization, should only be called once.
	//3^5 = 243
	return (rand() % 243);
}
int play_game(void){
	int num_levels = 5;
	while (num_levels > 0){
		x_curr = Touch_Scan_LH();
		x_prev = x_curr;
		timer = 100000 * num_levels;
		accuracy = 0;
		if (tasks % 3 == 0){
			//if tasks is even then LSB is 0 -- do hit SW1
			must_hit_SW1();
			//needs to poll here for changes in pressed_sw1 or pressed_sw2
			while (pressed_sw1 == 0 && pressed_sw3 == 0 && accuracy < 0.2 && timer > 0){
				//keep polling the touch sensor to update x_curr
				x_prev = x_curr;
				x_curr = Touch_Scan_LH();
				accuracy = abs((x_curr - x_prev)) / x_prev;
				//decrement timer so if the user times out they lose
				timer -= 1;
			}
			if (!pressed_sw1 || pressed_sw3 || timer == 0){
				//user mis-pressed, lose
				game_over();
				lose_game(); //comment: seems like you can lose by mishitting before the LED for the round goes off
				return 5-num_levels;
			}
		}else if (tasks % 3 == 1){
			//else tasks is odd and LSB is 1
			must_hit_SW3();
			//needs to poll here for changes in pressed_sw1 or pressed_sw2
			while (pressed_sw1 == 0 && pressed_sw3 == 0 && accuracy < 0.2 && timer > 0){
				//keep polling the touch sensor to update x_curr
				x_prev = x_curr;
				x_curr = Touch_Scan_LH();
				accuracy = abs((x_curr - x_prev)) / x_prev;
				timer--;
			}
			if (pressed_sw1 || !pressed_sw3 || timer == 0){
				game_over();
				lose_game();
				return 5-num_levels;
			}
		}else{
			//touch bar LR
			must_touch_bar_lr();
			//have a variable for TS
			while (pressed_sw1 == 0 && pressed_sw3 == 0 && accuracy < 0.2 && timer > 0){
				//keep polling the touch sensor to update x_curr
				x_prev = x_curr;
				x_curr = Touch_Scan_LH();
				accuracy = abs((x_curr - x_prev)) / x_prev;
				timer--;
			}
			if (pressed_sw1 || pressed_sw3 || timer == 0 || accuracy < 0.2){
				game_over();
				lose_game();
				return 5-num_levels;
			}
		}
		//reset these values so for next round, no button has been pressed
		pressed_sw1 = 0;
		pressed_sw3 = 0;
		//don't need to reset the x_cur and x_prev and accuracy, they're reset at the top of the loop
		//
		tasks = tasks / 2; //divide by 2 to get to next digits
		num_levels = num_levels - 1;
		delay(); //the light was flashing too quick
	}
	game_over();
	win_game();
	return 5-num_levels;
}
void game_over(void){
	int i = 3;
	while (i > 0){
		LEDGreen_Toggle();
		shortDelay();
		LEDGreen_Toggle();
		LEDRed_Toggle();
		shortDelay();
		LEDRed_Toggle();
		i-=1;
	}
}
void lose_game(void){
	LED_Off();
	shortDelay();
	LEDRed_On();
}
void win_game(void){
	LED_Off();
	shortDelay();
	LEDGreen_On();
}
void must_hit_SW1(void){
	LEDRed_Toggle();
	shortDelay();
	LEDRed_Toggle();
}
void must_hit_SW3(void){
	LEDGreen_Toggle();
	shortDelay();
	LEDGreen_Toggle();
}
void must_shake_board(void){
	LEDGreen_Toggle();
	LEDRed_Toggle();
	shortDelay();
	LEDGreen_Toggle();
	LEDRed_Toggle();
}
/*
 * Flashes red and green LED once at the same time
 */
void must_touch_bar_lr(void){
	must_shake_board();
}

//these functions are from Ben Roloff : https://forum.digikey.com/t/using-the-capacitive-touch-sensor-on-the-frdm-kl46z/13246
// Function to read touch sensor from low to high capacitance for left to right
int Touch_Scan_LH(void)
{
	int scan;
	TSI0->DATA = 	TSI_DATA_TSICH(10u); // Using channel 10 of The TSI
	TSI0->DATA |= TSI_DATA_SWTS_MASK; // Software trigger for scan
	scan = SCAN_DATA;
	TSI0->GENCS |= TSI_GENCS_EOSF_MASK ; // Reset end of scan flag

	return scan - SCAN_OFFSET;
}

// Function to read touch sensor from high to low capacitance for left to right
int Touch_Scan_HL(void)
{
	int scan;
	TSI0->DATA = 	TSI_DATA_TSICH(9u); // Using channel 9 of the TSI
	TSI0->DATA |= TSI_DATA_SWTS_MASK; // Software trigger for scan
	scan = SCAN_DATA;
	TSI0->GENCS |= TSI_GENCS_EOSF_MASK ; // Reset end of scan flag

	return scan - SCAN_OFFSET;
}
