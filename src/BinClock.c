/*
 * BinClock.c
 * Jarrod Olivier
 * Modified for EEE3095S/3096S by Keegan Crankshaw
 * August 2019
 *
 * SLDALE003 and TVRJAC001
 * 20 August 2019
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>   //For printf functions
#include <stdlib.h>  //For system functions
#include <signal.h>  //For cleanup fucntion
#include <softPwm.h> //For PWM on seconds LED


#include "BinClock.h"
#include "CurrentTime.h"

//Global variables
int hours, mins, secs;
int bounce = 200; 					//Minimal interrpt interval (ms)
long lastInterruptTime = 0; //Used for button debounce
int RTC; 										//Holds the RTC instance

int HH,MM,SS;

void initGPIO(void){
	/*
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware

	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC

	//Set up the LEDS
	for(int i; i < sizeof(LEDS)/sizeof(LEDS[0]); i++){
	    pinMode(LEDS[i], OUTPUT);
	}
 //Set up a HOURS array
	for(int i; i < sizeof(HOURS)/sizeof(HOURS[0]); i++){
	    pinMode(HOURS[i], OUTPUT);
	}

	//Set up a MINUTES array
 	for(int i; i < sizeof(MINUTES)/sizeof(MINUTES[0]); i++){
 	    pinMode(MINUTES[i], OUTPUT);
 	}

	//Set Up the Seconds LED for PWM
	pinMode(SECS,OUTPUT);
	softPwmCreate(SECS, 0, 60); //set SECONDS pin to support software PWM. Range of 0-60 (fully on)
	printf("LEDS done\n");

	//Set up the Buttons
	for(int j; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP);
	}

	//Attach interrupts to Buttons
	wiringPiISR(BTNS[0], INT_EDGE_FALLING, hourInc);
	wiringPiISR(BTNS[1], INT_EDGE_FALLING, minInc);

	printf("BTNS done\n");
	printf("Setup done\n");
}

/*
 * The main function
 * This function is called, and calls all relevant functions we've written
 */
int main(void){
  signal(SIGINT, cleanup); //set all GPIO pins low when CTRL-C caught
	initGPIO();
	//set the RTC registers with the current system time
	setCurrentTime();

	// Repeat this until we shut down
for (;;){
		//Fetch the time from the RTC
		HH = wiringPiI2CReadReg8(RTC, HOUR); //read SEC register from RTC
		MM = wiringPiI2CReadReg8(RTC, MIN); //read MIN register from RTC
		SS = (wiringPiI2CReadReg8(RTC, SEC) & 0b01111111); //read the SEC register from RTC

		printf("Read %x hours.\n", HH);

		//Function calls to toggle LEDs
		lightHours(HH);
		lightMins(MM);
		secPWM(SS);

		// Print out the time we have stored on our RTC
		printf("The current time is: %d:%d:%d\n", hexCompensation(HH), hexCompensation(MM), hexCompensation(SS));

		//Using a delay to make our program "less CPU hungry"
		delay(1000); //milliseconds
	}
	return 0;
}

/*
 * Change the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}

/*
 * Turns on corresponding LED's for hours
 */
void lightHours(int units){
	int decimal = hexCompensation(units);

	for (int i = 3; i >= 0; i--) {						// Decrease through 4 hour LED's
			if (decimal % 2) {										// If binary 1
				digitalWrite(HOURS[i], HIGH);
				printf("Hours[%d] (Pin %d) HIGH.\n", i, HOURS[i]);
			}
			else {																// If binary 0
				digitalWrite(HOURS[i], LOW);
				printf("Hours[%d] (Pin %d) LOW.\n", i, HOURS[i]);
			}
			decimal /= 2;
		}

}

/*
 * Turn on the Minute LEDs
 */
void lightMins(int units){
	int decimal = hexCompensation(units); //convert hex to dec

	for (int i = 5; i >= 0; i--) {						// Decrease through 5 minute LED's
			if (decimal % 2) {										// If binary 1
				digitalWrite(MINUTES[i], HIGH);
				printf("Hours[%d] (Pin %d) HIGH.\n", i, MINUTES[i]);
			}
			else {																// If binary 0
				digitalWrite(MINUTES[i], LOW);
				printf("Hours[%d] (Pin %d) LOW.\n", i, MINUTES[i]);
			}
			decimal /= 2;
		}
}

/*
 * PWM on the Seconds LED
 * The LED should have 60 brightness levels
 * The LED should be "off" at 0 seconds, and fully bright at 59 seconds
 */
void secPWM(int units){
	softPwmWrite(SECS, units);
}

/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 */
int hexCompensation(int units){
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic)
	*/
	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	//Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>bounce){
		printf("Interrupt 1 triggered, %x\n", HH);
		//Fetch RTC Time
		//Increase hours by 1, ensuring not to overflow
		//Write hours back to the RTC
		HH = hexCompensation(HH);
		//Prevents overflow
		HH++;
		HH%=12;
		wiringPiI2CWriteReg8(RTC, HOUR, decCompensation(HH));
	}
	lastInterruptTime = interruptTime;
}



/*
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	//Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>bounce){
		printf("Interrupt 2 triggered, %x\n", MM);

		MM = hexCompensation(MM); //Fetch RTC Time
		MM++;
		MM%=60;  //Increase minutes by 1, ensuring not to overflow

		if(MM==0){
			hourInc(); //when minutes reach 60, increment hours
		}
		wiringPiI2CWriteReg8(RTC, MIN, decCompensation(MM)); //Write minutes back to the RTC
	}
	lastInterruptTime = interruptTime;
}

//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>bounce){
		HH = getHours();
		MM = getMins();
		SS = getSecs();

		HH = hFormat(HH);
		HH = decCompensation(HH);
		wiringPiI2CWriteReg8(RTC, HOUR, HH);

		MM = decCompensation(MM);
		wiringPiI2CWriteReg8(RTC, MIN, MM);

		SS = decCompensation(SS);
		wiringPiI2CWriteReg8(RTC, SEC, SS);

	}
	lastInterruptTime = interruptTime;
}

/*
 * Reset GPIO pins
 */
void cleanup(int i){
																		 // Set all outputs to LOW
	for(int i=0 ; i<sizeof(LEDS)/sizeof(LEDS[0]); i++){
		digitalWrite(LEDS[i], LOW);
	}
																		// Set up each LED to Input (High Impedance)
	for (int i=0 ; i<sizeof(LEDS)/sizeof(LEDS[0]); i++) {
		pinMode(LEDS[i], INPUT);
	}

	pinMode(SECS, INPUT);

	printf("Cleaning up\n");
	exit(0);
}

/*
 * Gets current system time writes to RTC
 */
void setCurrentTime(){ //
	    HH = getHours();
			MM = getMins();
			SS = getSecs();

			HH = hFormat(HH);
			HH = decCompensation(HH);
			wiringPiI2CWriteReg8(RTC, HOUR, HH);

			MM = decCompensation(MM);
			wiringPiI2CWriteReg8(RTC, MIN, MM);

			SS = decCompensation(SS);
			secPWM(SS);
			wiringPiI2CWriteReg8(RTC, SEC,  0b10000000 + SS);
}
