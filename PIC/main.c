/*
 * File:   UImain.c
 * Author: Yuqian
 *
 * Created on January 20, 2019, 11:13 PM
 */

#include <xc.h>
#include <stdio.h>
#include <stdbool.h>
#include "configBits.h"
#include "I2C.h"
#include "lcd.h"
#include "UI.h"
#include "run.h"
#include "mainheader.h"

void main(void) {
    //LCD register configuration
    LATD = 0x00;
    TRISD = 0x00;
    //A2D configuration; no analog GPIO is used
    ADCON1 = 0b00001111;
    //External Interrupt configuration
    INT0IE = 1;//left encoder
    INT2IE = 1;//keypad
    INT1IE = 1;//right encoder
    ei();
    //LCD and RTC configuration    
    initLCD();    
    I2C_Master_Init(100000);
//    initTimeRTC();
    /*Initialize PWM*/
    initPWM();
    /*Initialize all output pins for motors preventing floating in the begining*/
    //<editor-fold defaultstate="collapsed" desc="initMotor">
    //tube 1 
    TRISAbits.RA4 = 0; //step 
    LATAbits.LATA4 = 0;
    TRISAbits.RA5 = 0; //direction
    LATAbits.LATA5 = 0;
    //tube 2
    TRISEbits.RE0 = 0; //step
    LATEbits.LATE0 = 0;
    TRISCbits.RC5 = 0; //direction
    LATCbits.LATC5 = 0; //low
    //ext
    TRISAbits.RA0 = 0; //step
    LATAbits.LATA0 = 0;
    TRISAbits.RA1 = 0; //direction
    LATAbits.LATA1 = 0; //low
    //mux
    TRISBbits.RB3 = 0; //mux choice
    LATBbits.LATB3 = 1; // 1 going forward
    //PWM duty cycle = 0 in the beginning
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    //</editor-fold>
//    machine_state=testing_state;//uncomment this line for debugging
    // Main loop  
    while(1){
        switch(machine_state){
            case standby_state:
                initUI();
                break;
            case running_state:
                run();
                break;
            case testing_state:
                testRun();
                break;
            case doneRunning_state:
                instructMenu();
                break;
            case logging_state:
                instructMenu();
                break;
            case report_state:
                report();
                break;
            case report_sub_state:
                report();
                break;
            case emergencyStop_state:
                emergency_stop();
                break;
            default:break;
        }
    }
}

void restartRoutine(void){
    lcd_clear();
    lcd_home();
    start_key_pressed = false;
    return_key_pressed = false;
    enter_key_pressed=false;
    up_key_pressed = false;
    down_key_pressed = false;
}

void emergency_stop(void){
    lcd_clear();
    lcd_home();
    printf("Emergency Stop");
    while(1){
        if(return_key_pressed){
            restartRoutine();
            return_key_pressed=false;
            return;
        }
    }
}
