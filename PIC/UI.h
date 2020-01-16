/* 
 * File:   
 * Author: Yuqian Zhou
 * Comments: To simplify UI function
 * Revision history: 
 * v1.1 Feb.25 Stand along header files for UI.c
 */

#ifndef UI_H
#define	UI_H
/********************************* Includes **********************************/
#include <xc.h> 
#include <stdio.h>
#include <stdbool.h>
#include "configBits.h"
#include "I2C.h"
#include "lcd.h"
#include "mainheader.h"
/********************************** Macros ***********************************/

//<editor-fold defaultstate="collapsed" desc=" MACHINE STATES ">
enum state {
    /*Machine States*/
    standby_state,//before "start" is pressed; RTC is shown.
    running_state,//"start" is pressed; machine goes forward; I2C for comm.  w/ Arduino.
    returning_state,//end cond'n is reached; machine goes backwards; 
    doneRunning_state,//machine stopped behind start line;write to eeprom; wait for user input 
    prevLogging_state,//retrieve data from eeprom
    logging_state,//retrieve data from program memory
    report_state,//show entries of the report 
    report_sub_state,//show data of each entry
    emergencyStop_state,//emergency stop
    testing_state,//for testing and debugging
    /*Reporting States*/
    pg0,//instruction page
    pg1,//report menu page 1 
    pg2,//report menu page 2
    pg11,//report data
    pg12,
    pg13,
    pg24,
    pg25
};
//</editor-fold>
enum state machine_state;
enum state report_page;

const char resetTime[7] = {
    0x00, // Seconds 
    0x36, // Minutes
    0x13, // 24 hour mode
    0x02, // Week
    0x09, // Date
    0x04, // Month
    0x19  // Year
};
unsigned char upArrow = 0b00010100;
unsigned char downArrow = 0b00010101;
int pageCnt = 0;
int itemCnt = 0;
unsigned char data; // Holds the data to be sent/received between PIC and Arduino
unsigned char keypress;// Holds the input from keypad
unsigned char selectItem;
const char keys[] = "123A456B789C*0#D";
/************************ Public Function Prototypes *************************/
void initUI(void);
void getRTC(void);
void initTimeRTC(void);
void instructMenu(void);
void instruct_page_1(void);
void report(void);
void report_page_1(void);
void report_page_2(void);
void report_page_11(void);
void report_page_12(void);
void report_page_13(void);
void report_page_24(void);
void report_page_25(void);
#endif	/* UI_H */

