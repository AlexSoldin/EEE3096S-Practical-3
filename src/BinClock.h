#ifndef BINCLOCK_H
#define BINCLOCK_H

//Some reading (if you want)
//https://stackoverflow.com/questions/1674032/static-const-vs-define-vs-enum

// Function definitions
int hFormat(int hours);
void lightHours(int units);
void lightMins(int units);
int hexCompensation(int units);
int decCompensation(int units);
void initGPIO(void);
void secPWM(int units);
void hourInc(void);
void minInc(void);
void toggleTime(void);
void hoursButtonPressed(void);
void minutesButtonPressed(void);
void cleanup(int);

// define constants
const char RTCAddr = 0x6f;
const char SEC = 0x00; // see register table in datasheet
const char MIN = 0x01;
const char HOUR = 0x02;
const char TIMEZONE = 2; // +02H00 (RSA)

// define pins
const int LEDS[] = {1,3,4,5,6,21,26,22,25,27}; //H0-H4, M0-M5 BCM
const int HOURS[] = {1,3,4,5}; //wiringPi values
const int MINUTES[] = {6,21,26,22,25,27}; //wiringPi values
const int SECS = 23;
const int BTNS[] = {0,7}; // B0, B1


#endif
