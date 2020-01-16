/*
 * File:   interruptHandler.c
 * Author: Yuqian
 *
 * Created on February 25, 2019, 4:15 PM
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

void __interrupt() interruptHandler(void){
    /*Timer 1 interrupt
     * triggered every 0.1 sec
     * see initTimer for pre-load details
     */
    if(TMR1IF&&TMR1IE){
        runTime++;//runTime counts the number of interrupts, initialized at the beginning
        if(runTime>1300){//130 sec or 2:10 min, this is one of the end conditions
            moving=false;//stop moving. jump out of the while loop
            returning=true;//start returning
        }
        else if(firstStop){
            loopTime++;
            if (loopTime > 100) {//if trapped in the firstStop loop for 10 sec 
                firstStop = false;
            }
        }
        else if(startMotor){
            loopTime++;
            if (loopTime > 100) {//if trapped in the startMotor loop for 10 sec 
                startMotor = false;
            }
        }
        else if(stateCode==4){//waiting for Arduino data
            loopTime++;
            if(loopTime > 100){//if has been wait for 10 sec, we decided to go to next pole
                poleCnt++;
                totStateCode[poleCnt]=stateCode;
                totStartDist[poleCnt] = pulse2;
                totStartDistCm[poleCnt] = (int) (slop * pulse2 + intercept);
                send = true;
                detect = false;
                deploy = false;
            }
        }
        TMR1IF = 0;//clear flag
        TMR1IE = 1;//reset enable bit
        TMR1H = 11; // preset for timer1 MSB register
        TMR1L = 220; // preset for timer1 LSB register
    }
    if(INT0IF&&INT0IE){//external interrupt 0 from unused encoder
        pulse1++;
        INT0IF = 0;
    }
    if(INT2IF&&INT2IE){//external interrupt 1 from encoder
        pulse2++;
        if (pulse2 > endDist) {//distance of 4m
            returning = true;//stop moving and start returning
            moving=false;
        }
        if (poleCnt >= 10) {//PoleCnt reach 10
            returning = true;
            moving=false;
        }
        if(startMotor){
            if((pulse2-newPulse2)>=startMotorDist){
                //can change to pulseTemp if we don't want to do anything with a pole 
                startMotor=false;
                noDeploy=false;
            }
        }
        else if(firstStop){
            if((pulse2-pulseTemp)>=firstStopDist){
                firstStop=false;
            }
        }
        else if(secondStop){
            if((pulse2-pulseTemp) >= alignDist){
                secondStop=false;
            }
        }
        INT2IF = 0;
    }
    /*Keypad inputs
     * To make a page flipping effect, 
     */
    if(INT1IF&&INT1IE){
        keypress=(PORTB&0xF0)>>4;
        switch(keypress){
            case 0://1
                if (machine_state==doneRunning_state){
                    enter_key_pressed = true;
                    machine_state=logging_state;
                    break;
                }
                else if(machine_state==report_state){
                    if(report_page==pg1){
                        sub_key_pressed=true;
                        selectItem=1;
                        report_page=pg11;
                        machine_state=report_sub_state;
                        break;
                    }
                }
                break;
            case 1://2
                if (machine_state==doneRunning_state){
                    enter_key_pressed = true;
                    machine_state=prevLogging_state;
                    break;
                }
                else if(machine_state==report_state){
                    sub_key_pressed = true;
                    report_page = pg12;
                    machine_state = report_sub_state;
                    break;
                }
                break;
            case 2://3
                if(machine_state==report_state){
                    sub_key_pressed = true;
                    report_page = pg13;
                    machine_state = report_sub_state;
                    break;
                }
                break;
            case 3://A, up key
                if(machine_state==report_state){
                    if(report_page==pg2){
                        up_key_pressed=true;
                        report_page=pg1;
                        break;
                    }
                }
                if(machine_state==report_sub_state){
                    up_key_pressed=true;
                    break;
                }
                break;
            case 4://4
                if(machine_state==report_state){
                    sub_key_pressed = true;
                    report_page = pg24;
                    machine_state = report_sub_state;
                    break;
                }
                break;   
                
            case 5://5
                if(machine_state==report_state) {
                    sub_key_pressed = true;
                    report_page = pg25;
                    machine_state = report_sub_state;
                }
                break;              
            case 7://B
                if(machine_state==report_state){
                    if(report_page==pg1){
                        down_key_pressed=true;
                        report_page=pg2;
                    }
                }
                if(machine_state==report_sub_state){
                    down_key_pressed=true;
                    break;   
                }
                break;
            case 12://*
                if (machine_state==doneRunning_state){
                    return_key_pressed = true;
                    machine_state=standby_state;
                    break;
                }
                else if(machine_state==logging_state){
                    return_key_pressed = true;
                    machine_state=doneRunning_state;
                    break;
                }
                else if(machine_state==report_state){
                    return_key_pressed = true;
                    machine_state=doneRunning_state;
                    break;
                }
                else if(machine_state==emergencyStop_state){
                    return_key_pressed = true;
                    machine_state=standby_state;
                    break;
                }
                
                else if(machine_state==report_sub_state){
                    return_key_pressed = true;
                    if(selectItem>3){
                        report_page=pg2;
                    }else{
                        report_page=pg1;
                    }
                    machine_state=report_state;
                    break;
                }
                else if(machine_state==testing_state){
                    moving=false;
                    break;
                }
                else if (machine_state == running_state) {
                    moving = false;
                    break;
                }
                break;
            case 14://#
                if (machine_state==doneRunning_state){
                    enter_key_pressed = true;
                    machine_state=logging_state;
                    break;
                }
                else if (machine_state==logging_state){
                    enter_key_pressed = true;
                    machine_state=report_state;
                    break;
                }
                else if (machine_state==standby_state){
                    start_key_pressed = true;
                    machine_state=running_state;
                    break;
                }
            default:break;
        }
        INT1IF = 0;
    }
}