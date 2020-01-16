/* 
 * File:   header file for run
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef run_H
#define	run_H
/********************************** Macros ***********************************/
/*INCLUDES*/
#include <xc.h>
#include <stdio.h>
#include <stdbool.h>
#include "configBits.h"
#include "lcd.h"
#include "I2C.h"
#include "UI.h"
#include "mainheader.h"

/*CONSTANTS*/
#define M_PI 3.14159265358979323846
/*PWM config constants*/
#define _PWM_FREQ 20000000 // frequence for PWM cycle
#define startPWM 225 //Duty cycle of going forward; speed
#define stopPWM 200
#define returnPWM 800 //Duty cycle of returning
#define TMR2PRESCALE 4 //pre-scale 1:4 according to textbook page.
//#define pwmFreqStart 10000
unsigned long PWM_freq=10000; //10kHz
/*Driving Condition Constants*/
/*OLD DATA at 1.7*/
//#define endDist 680 //number of pulses after going 4m 400*1.7=680
//#define upperBound 51 //30cm, condition of two/one tires
//#define lowerBound 34 //20cm, least vaild distance
//#define alignDistLow 16 //10cm, forward adjustment to align with tube 1 hook
//#define alignDistHigh 25 //16.5cm, forward adjustment to align with tube 2
//#define alighDistDiff 9 //6.5cm diff b/w 1 and 2 tube
//#define firstStopDist 5 //3 cm since the sensor detected
//#define startMotorDist 13 //7~8cm from the last pole
//float slop=0.67;
//float intercept=-7.86;

/*SF hall way*/
//#define endDist 520 //number of pulses after going 4m 400*1.7=680
//#define upperBound 41 //30cm, condition of two/one tires
//#define lowerBound 29 //20cm, least vaild distance
//#define alignDistLow 16 //10cm, forward adjustment to align with tube 1 hook
//#define alighDistDiff 13 //8cm diff b/w 1 and 2 tube
//#define alignDistHigh 27 //16.5cm, forward adjustment to align with tube 2
//#define firstStopDist 5 //2 cm since the sensor detected
//#define startMotorDist 16 //10cm from the last pole
//#define sensorDist 18 //11.5cm from the front
////linear fit 
//float slop=0.760;
//float intercept=4.981;

/*Machine Shop*/
#define endDist 460 //number of pulses after going 4m 400*1.7=680
#define upperBound 41 //30cm, condition of two/one tires
#define lowerBound 29 //20cm, least vaild distance
#define alignDistLow 17 //10cm, forward adjustment to align with tube 1 hook
#define alighDistDiff 13 //8cm diff b/w 1 and 2 tube
#define alignDistHigh 25 //16.5cm, forward adjustment to align with tube 2
#define firstStopDist 7 //2 cm since the sensor detected
#define startMotorDist 16 //10cm from the last pole
#define sensorDist 18 //11.5cm from the front
//linear fit 
float slop=0.864;
float intercept=-1.466;

/*Extension Condition Constants*/
#define extOffset 45 //+6.5cm at resting position, -4.25cm of pole radius
#define extMax 135 //max extention 19.8 cm from the edge; 13.3cm from the resting position 
#define extMin 20 //min extention 9.9 from the edge; 3.4cm from the resting position
/*Deploy Condition Constants*/
#define tube1Tire 7 //Initial number of tires in tube 1(front) 7+1
#define tube2Tire 6 //Initial number of tires in tube 2(rare) 6+1
/*VARIABLES*/
/*Driving Information*/
volatile int pulse1;
volatile int pulse2;
int newPulse1;
int newPulse2;
int pulseTemp;
float leftWheelDist;
float rightWheelDist;
int startDist;//Distance between the current and the last pole
int cntrDist;//Distance of pole from the edge of the machine in mm
int alignDist;
int poleCnt;
int runTime;
int loopTime;
int stateCode;
/*Data Storage*/
int totOldTire[15];//deployed tires
int totSupplyTire[15];//number of tires that is need to supply
int totNewTire[15];
int totCntrDist[15];
int totStartDist[15];
int totStateCode[15];
int totStartDistCm[15];
/*UI*/
volatile bool send = true;
volatile bool detect = false;
volatile bool deploy = false;

volatile bool moving;//for stopping tests 
volatile bool looping=false;//Checking if the machine is trapped in a infinite loop
volatile bool returning=false;//Checking if the machine need to return back
//volatile bool testing = true;//comment this out when running the machine 
volatile bool longDist = false;//Checking if the machine has been run for a long distance
volatile bool arrived = false;

/*Deploying information*/
int tubeID;//id of tube: 1 or 2
int supplyTire;//Number of tires to be deployed on to the pole
int oldTire;//Number of tires on the pole
int totTire;
int tube1_tire = tube1Tire;//Number of tire in tube 1
int tube2_tire = tube2Tire;//Number of tire in tube 2
int pwmVal1 = startPWM;//Duty cycle of left wheel
int pwmVal2 = startPWM;//Duty cycle of right wheel
volatile bool noDeploy = false;
volatile bool startMotor = false;//indicates if the DC motors pass their accelerating state
volatile bool firstStop = false;//indicates if the machine stops at the 'good spot'  
volatile bool secondStop = false;//indicates if the machine stops at the desired position
volatile bool shortDist = false;
/*UI*/
unsigned char time[7];
unsigned char runClk[7];

volatile bool start_key_pressed = false;
volatile bool return_key_pressed = false;
volatile bool enter_key_pressed = false;
volatile bool up_key_pressed = false;
volatile bool down_key_pressed = false;
volatile bool sub_key_pressed = false;
volatile bool readKey;
/************************ Public Function Prototypes *************************/
/*In run.c */
void run(void);
void initPWM(void);
void set_pwm_duty_cycle(unsigned int duty,int wheel_id);
void tube_stepper_control(int tubeID, int tire_num);
void ext_stepper_control(int ext_dist,bool extend);
void Trigger_Pulse(unsigned int x, int stepper_id);
void initTimer(void);
void slowStop(int currPWM);
int tireCheck(int oldTire);
void deployTireFin(int extDist, int supplyTire);
void returnRun(void);
/*In testRun.c*/
void testRun(void);
void deployTest(void);
char keypadConver(unsigned char key);
void irnDCTest(void);
void intTest(void);
void fucTest(void);
void fullFucTest(void);
void fullFucTestVer2(void);
void deployTire(unsigned int extDist);
void resetStepper(void);
//void wheelTest(void);
//void wheelBack(void);
void slowStop(int currPWM);
void slowStart(void);
void shaftEncoder(void);
void distCheck(void);
void timeReceive(void);
void runTimeTest(void);
void sensorAdjust(void);
#endif	/* run_H */

