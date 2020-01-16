/*
 * File:   testRun.c
 * Author: Yuqian
 *
 * Created on March 7, 2019, 11:00 PM
 */

/* Terms:
 * 1.temp.:temporary; will be discarded after a function ends
 * 2.short-term: is initialized since reset
 * 3.long-term: is written into eeprom
 * 
 * 4.Tubes: Tube 1 is at the front and tube 2 represents the rare motor. 
 * Each has two pins step and direction. 
 * Direction should be kept 0/1 => forward/backward
 * Step should alter b/w 0 and 1 to drive the motor
 * see Trigger_Pulse() for details
 * 
 * 5.Wheel ID: 1 is left and 2 is right
 * Select high/1 represents forward; low/0 represents backwards
 */

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "configBits.h"
#include "lcd.h"
#include "I2C.h"
#include "UI.h"
#include "mainheader.h"
#include "run.h"

void testRun(void) {
    /*Initialize PWM*/
    initPWM();
    /*Initialize I2C with address 7*/
    I2C_Master_Init(100000);
    I2C_Master_Start();
    I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
    I2C_Master_Stop();
    /*Initialize all output for motors preventing floating in the begining*/
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
    LATBbits.LATB3 = 1;// 1 going forward
    //PWM duty cycle = 0 in the beginning
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    while(machine_state==testing_state) {
        /*User can test for different modules by pressing the correlated key*/
        lcd_clear();
        lcd_home();
        printf("Please Select");
        lcd_set_ddram_addr(LCD_LINE2_ADDR);
        printf("Testing State");
        /*Polling for user input
         * I wrote all user input events using polling instead of interrupts cuz 
         * 1. its easier 
         * 2. there's rumor that the pic board can't handle too many interrupts
         */
        while (PORTBbits.RB1 == 0) {
            continue;
        }
        keypress = (PORTB & 0xF0) >> 4;
        while (PORTBbits.RB1 == 1) {
            continue;
        }
        moving=true;
        /* Basic rules of UI in this project:
         * 0~9 A~D are what they are; sometimes A/B are page up/down
         * # for enter/confirm; * for cancel
         * Maybe not intuitive, but I'm too lazy to change zzz(-_-
         */
        switch(keypress){
            case 0://1
            /*Stepper motor test*/
                deployTest();
                break;
            case 1://2
            /*Sensors and DC test*/
                irnDCTest();
                break;
            case 2://3
            /*Reset stepper motors or should I say life saver?*/
                resetStepper();
                break;
            case 3://A
            /*First test for the entire system after integration*/
                intTest();
                break;
            case 4://4
            /* Second Test for entire system. 
             * The algorithm makes sure that machine stops entirely 
             * before sensors starting detection*/
                fucTest();
                //wheelTest();//For DC motor debugging. To see if they can turn
                break;
            case 5://5
            /*First encoder test used frequently in <Week 10 ~ 11>*/
                shaftEncoder();
                break;
            case 6://6
            /*Second encoder. Develop a relation b/w dist and encoder reading*/
                distCheck();
                break;
            case 7://B
            /*Full functionality test. Will replace the run.c*/
                fullFucTest();
                break;
            case 8://7
                fullFucTestVer2();
                break;
            case 9://8
                runTimeTest();
                break;
            case 10://9
                sensorAdjust();
            case 11://C
//                sensorAdjust();
                run();
                break;
            case 12://*
                machine_state=doneRunning_state;
                return;
                break;
            default: break;
        }
    }
    return;
}
void deployTest(void){
    /* Stepper motor test
     * a. Type in the distance you'd like to extent the stepper motor in mm
     * b. Stepper motor (RA4 step; RA5 dir) will go forward and backward for this distance
     * Stepper motors (RA0\RE0 step; RA1\RC5 dir) will go forward 2*27 mm (two tires)*/
    unsigned int extDist=0;
    unsigned char counter = 0;
    unsigned char temp=0;
    lcd_clear();
    lcd_home();
    printf("Deployment Test");
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    printf("Ext Dist(0~170):");
    lcd_set_ddram_addr(LCD_LINE3_ADDR);
    readKey=true;
    while (readKey) {
        while (PORTBbits.RB1 == 0) {
            continue;
        }
        keypress = (PORTB & 0xF0) >> 4;
        while (PORTBbits.RB1 == 1) {
            continue;
        }
        Nop(); // Apply breakpoint here to prevent compiler optimizations
        data = keys[keypress];
        if(keypress==14){// #: end of input
            readKey=false;
            break;
        }
        else{
            putch(data); // Put the character to be displayed on the LCD
            temp=keypadConver(keypress);
            if(temp>=0 && temp<10) {
                /*Input is saved in extDist*/
                extDist = temp +(extDist*10);
                counter++;
                if (counter > 3) {
                    counter = 0;
                    extDist = 0;
                }
            }
            else{
                extDist=0;
                counter=0;
            }
        }
    }
    if(extDist<300){
        lcd_clear();
        lcd_home();
        printf("Ext Dist=%d", extDist);
        deployTire(extDist);
    }
    else{
        lcd_clear();
        lcd_home();
        printf("Invaild Dist:%d!",extDist);
        __delay_ms(1000);
        return;
    }
}

char keypadConver(unsigned char key){
    /*Make 0~9 A~D on the keypad to be what they are*/
    if(key>=0 && key<=2){
        return key+1;
    }
    else if(key>=4 && key<=6){
        return key;
    }
    else if(key>=8 && key<=10){
        return key-1;
    }
    else if(key==13){
        return 0;
    }
    else if (key==3){
        return 10;//A
    }
    else if (key==7){
        return 11; //B
    }
    else if (key==11){
        return 12;//C
    }
    else if (key==15){
        return 13;//D
    }
    else{return 15;}
}

void irnDCTest(void){
    /*Start running DC motors. Stop if sensor detects a "pole". 
     * Pair with testRun.ino on Arduino
     * We never use this, this was integrated with steppers and becomes intTest()
     */
    bool send = true;
    bool detect = false;
    unsigned char in_cnt = 0;
    unsigned char dist[3];
    while(moving){
        if(send){
            data='A';
            I2C_Master_Start(); // Start condition
            I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
            I2C_Master_Write(data); // Write key press data
            I2C_Master_Stop();
            send = false;
            set_pwm_duty_cycle(startPWM, 1);
            set_pwm_duty_cycle(startPWM, 2);
            lcd_clear();
            lcd_home();
            printf("Wheels Start");
        }else{
            I2C_Master_Start();
            I2C_Master_Write(0b00001111); // 7-bit Arduino slave address + Read
            data = I2C_Master_Read(NACK); // Read one char only
            I2C_Master_Stop();
            if ((data)&&(data < 255)&&(data != 10)) {
                if (data == 'B') {
                    lcd_clear();
                    lcd_home();
                    printf("Pole Detected:");
                    set_pwm_duty_cycle(0, 1);
                    set_pwm_duty_cycle(0, 2);
                    detect = true; //a pole is detected
                    in_cnt = 0;
                    continue;
                }
                if (detect) {
                    if (in_cnt == 0) {
                        dist[in_cnt] = data;
                    }
                    else if(in_cnt == 1){
                        dist[in_cnt] = data-1;
                        cntrDist = (dist[0] | (dist[1] << 8));
                    }
                    else if (in_cnt == 2) {
                        dist[in_cnt] = data - 1; //for easy communication
                        cntrDist = (dist[0] | (dist[1] << 8));
                        oldTire = dist[2];
                        lcd_clear();
                        lcd_home();
                        printf("Pole Dist:%d", cntrDist);
                        lcd_set_ddram_addr(LCD_LINE2_ADDR);
                        printf("Tire Num:%d", oldTire);
                        in_cnt = 0;
                        detect = false;
                        send = true;
                        return;
                    }
                    in_cnt++;
                }
            }
        }
    }
    lcd_clear();
    lcd_home();
    printf("Wheels Stopped");
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    return;
}

void intTest(void) {
    /* This is the first integrated test for the major modules. <Week 10>
     * In this test sensor,dc and steppers are integrated. 
     * Encoders are not included. 
     * Start dist is not counted or calculated
     * Pair with Arduino testRun.ino
     * 
     * Process: 
     * 1.Start DC motors; PIC->Arduino: send 'A' to start sensor detecting
     * 2.A pole is detected; Arduino->PIC: sends back 'B' to stop dc motors [First Stop]
     * 3.Sensor starts reading the number of tires and dist to pole;
     * Arduino->PIC: send 3 bytes data for cntr dist (2 bytes) and tire number (1 byte)
     * 4.Move the machine forward for 10.5cm/16.5cm to align with the selected pole
     * 5.Stop DC, extend and deploy tires.[Second Stop]
     * Note: there is no start dist calculation to calculate the number of supply tire
     * 6.Terminate the run 
     */
    
    bool send = true;
    bool detect = false;
    bool deploy = true;
    unsigned char in_cnt = 0;
    unsigned char dist[3];
    while(moving){
        if(send){
            //PIC->Arduino: send 'A' to start sensor detecting
            data='A';
            I2C_Master_Start(); // Start condition
            I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
            I2C_Master_Write(data); // Write key press data
            I2C_Master_Stop();
            send = false;
            //Start DC motors
            set_pwm_duty_cycle(startPWM, 1);
            set_pwm_duty_cycle(startPWM, 2);
            lcd_clear();
            lcd_home();
            printf("Wheels Start");
        }else{
            //Continuously receiving data from Arduino
            I2C_Master_Start();
            I2C_Master_Write(0b00001111); // 7-bit Arduino slave address + Read
            data = I2C_Master_Read(NACK); // Read one char only
            I2C_Master_Stop();
            if ((data)&&(data < 255)&&(data != 10)) {
                //Arduino->PIC: sends back 'B' to stop dc motors
                if (data == 'B') {
                    lcd_clear();
                    lcd_home();
                    printf("Pole Detected:");
                    __delay_ms(100);
                    set_pwm_duty_cycle(0, 1);
                    set_pwm_duty_cycle(0, 2);
                    detect = true; //a pole is detected
                    in_cnt = 0;
                    continue;
                }
                if (detect) {
                //Arduino->PIC: send 3 bytes data for cntr dist (2 bytes) and tire number (1 byte)
                    if (in_cnt == 0) {
                        dist[in_cnt] = data;
                    }
                    else if(in_cnt == 1){
                        dist[in_cnt] = data-1;
                        cntrDist = (dist[0] | (dist[1] << 8));
                    }
                    else if (in_cnt == 2) {
                        dist[in_cnt] = data - 1; //for easy communication, 0 is excluded
                        cntrDist = (dist[0] | (dist[1] << 8));
                        oldTire = dist[2];
                        lcd_clear();
                        lcd_home();
                        printf("Pole Dist:%d", cntrDist);
                        lcd_set_ddram_addr(LCD_LINE2_ADDR);
                        printf("Tire Num:%d", oldTire);
                        //Move the machine forward for 10.5cm/16.5cm.
                        set_pwm_duty_cycle(startPWM, 1);
                        set_pwm_duty_cycle(startPWM, 2);
                        pulse1=0;
                        pulse2=0;
                        while(pulse1<35 && moving){//This is changed to 'alignDistLow'
                            continue;
                        }
//                        __delay_ms(800);//Time period for counting the distance[Removed]
                        set_pwm_duty_cycle(0, 1);
                        set_pwm_duty_cycle(0, 2);
                        //Display the pulses for easy debugging
                        printf("L:%d;R:%d",pulse2,pulse1);
                        deployTire(cntrDist);//Extend and deploy tires.
                        in_cnt = 0;
                        detect = false;
//                        send = true;
//                        deploy = true;
                        return;//Termination
                    }
                    in_cnt++;
                }
            }
        }
    }
    //This part is never called.
    lcd_clear();
    lcd_home();
    printf("Wheels Stopped");
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    return;
}
void fucTest(void) {
    /* This is the second test for the all modules. Or functionality test. <Week 11>
     * Differences from intTest() or ver. 1:
     * 1. In this test, IR sensors, encoders, DCs and steppers are integrated. 
     * 2. Start distance is also calculated.
     * 3. A back-and-forth communication is added to the First Stop ('S'&'D'), 
     *  making sure that the machine is fully stopped before distance detecting and tire counting.
     * 4. supplyTire and deployTireFin are added to calculate desired number of tires.
     * 5. End conditions are added but never called in the process
     * This test is designed only to detect one pole and deploy desired number of tires onto it.
     * Pair with Arduino testRunver2.ino
     * 
     * Process: 
     * 1.Start DC motors; PIC->Arduino: send 'A' to start sensor detecting
     * 2.A pole is detected; Arduino->PIC: sends back 'B' to show a pole is detected
     * 3.After DC motors are stopped, PIC->Arduino: send 's' to indicate the stop
     *  Note: for better sensor reading, the machine is designed to stop 3cm later
     *  after a pole is detected.
     * 4.Sensor starts reading the number of tires and dist to pole;
     * 5. Arduino->PIC: send 1+3+1 bytes data for 
     *  a.'D' to indicate the start of data receiving/sending
     *  b.cntr dist (2 bytes) and tire number (1 byte)
     *  c. 'E' to indicate the end of data receiving/sending
     * 6.Calculate the designed number of tires and select a tube to deploy
     * 7.Move the machine forward for 10.5cm/16.5cm to align with the selected pole
     * 8.Extend and deploy tires.
     * 9.Terminate the run 
     */
    /*Private variables*/
    bool send = true;//indicates the sending/receiving relation b/w Arduino;
    bool detect = false;//indicates if a pole is detected (true) or not (false)
    bool deploy = false;//indicates if the tires are counted and ready to deploy
    unsigned char in_cnt = 0;//counter for data receiving
    unsigned char dist[3];//temp. storage of I2C data
    int pulseCnt=0;//temp. storing the pulses when calculating the difference b/w pulses
    /*Public variables*/
    returning = false;//indicates the machine is returning back to start line
    poleCnt = 0;//No pole has counted in the beginning
    pulse1 = 0;//Clear pulses before the wheels start
    pulse2 = 0;
    newPulse2 = 0;//Clear the short-term storage of pulse2 too
    tube1_tire = tube1Tire;//Initial number of tires stored
    tube2_tire = tube2Tire;
    /*The main loop is a while loop. Making sure there is no delay in the receiving process*/
    while(moving){//moving can be set to false by pressing *. Emergency stop from the software end.
        if(send){
            if(!detect){
                //PIC->Arduino: send 'A' to start sensor detecting
                data='A';
                I2C_Master_Start(); // Start communication
                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
                I2C_Master_Write(data); // Write data
                I2C_Master_Stop();// Stop communication
                send = false;//Start receiving after sending the signal byte
                
                //Start DC motors;
                set_pwm_duty_cycle(startPWM, 1);
                set_pwm_duty_cycle(startPWM, 2);
                lcd_clear();
                lcd_home();
                printf("Wheels Start");
                continue;
            }
            else{
                //PIC->Arduino: send 'S' to indicate the stop
                data = 'S';
                I2C_Master_Start(); // Start communication
                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
                I2C_Master_Write(data); // Write key press data
                I2C_Master_Stop();//Stop communication
                send = false;
                lcd_clear();
                lcd_home();
                printf("Pole Detected");
                continue;
            }
        }else{
            I2C_Master_Start();
            I2C_Master_Write(0b00001111); // 7-bit Arduino slave address + Read
            data = I2C_Master_Read(NACK); // Read one char only
            I2C_Master_Stop();
            //End conditions: 4 meters and 10 poles
            if (pulse1 > endDist) {//distance of 4 meters
                returning=true;
                break;
            }
            if (poleCnt>=10){
                returning=true;
                break;
            }
            if ((data)&&(data < 255)&&(data != 10)) {
                //Arduino->PIC: sends back 'B' to show a pole is detected
                if (data == 'B') {
                    pulseCnt=pulse2;
                    while((pulse2-pulseCnt)<6){
                        continue;
                    }
                    set_pwm_duty_cycle(0, 1);
                    set_pwm_duty_cycle(0, 2);
                    __delay_ms(500);//wait for 
                    detect = true; //a pole is detected
                    send=true;//
                    poleCnt++;
                    continue;
                }
                if(data == 'D') {
                    /*Arduino->PIC: send 1+3+1 bytes data 
                     * 'D' to indicate the start of data receiving/sending
                     */
                    lcd_clear();
                    lcd_home();
                    printf("Receiving Data");
                    detect = false;
                    deploy = true;
                    in_cnt = 0;
                    continue;
                }
                if (deploy) {
                    //cntr dist (2 bytes) and tire number (1 byte)
                    if (in_cnt == 0) {
                        dist[in_cnt] = data;
                    }
                    else if(in_cnt < 3){
                        dist[in_cnt] = data-1;
                    }
                    else if (data=='E' && in_cnt>=3) {
                        //'E' to indicate the end of data receiving/sending
                        cntrDist = (dist[0] | (dist[1] << 8));//Retrieve from temp.array
                        totCntrDist[poleCnt]=cntrDist;//Save data into short-term array
                        oldTire = dist[2];
                        totOldTire[poleCnt]=oldTire;
                        lcd_clear();
                        lcd_home();
                        printf("Pole Dist:%d", cntrDist);
                        lcd_set_ddram_addr(LCD_LINE2_ADDR);
                        printf("Tire Num:%d", oldTire);
                        
                        supplyTire=tireCheck(oldTire);//fuc calculate # of tires to supply
                        
                        lcd_set_ddram_addr(LCD_LINE3_ADDR);
                        printf("1::%d,2:%d;Supply:%d",pulse1,pulse2,supplyTire);//change to pulse 2 later
                        totSupplyTire[poleCnt]=supplyTire;
                        
                        deployTireFin(cntrDist-extOffset,supplyTire);//fuc deploy # of tires from a selected pole
                        
                        in_cnt = 0;
                        deploy=false;
                        send=true;
                        while(moving){
                            continue;
                        }
                        return;
                    }
                    in_cnt++;
                }
            }
        }
    }
    if(returning){
       LATBbits.LATB3 = 0;//turning back
       pulse1=0;
       pulse2=0;
       set_pwm_duty_cycle(returnPWM, 1);
       set_pwm_duty_cycle(returnPWM, 2);
       while(pulse1<=endDist && moving){
           continue;
       }
    }
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    return;
}

void fullFucTest(void) {
    
    bool send = true;
    bool detect = false;
    bool deploy = false;
    unsigned char in_cnt = 0;
    unsigned char dist[3];
    int pulseCnt=0;
    int displayCnt=0;
    poleCnt=0;
    pulse1 = 0;
    pulse2 = 0;
    newPulse2 = 0;
    tube1_tire = tube1Tire;
    tube2_tire = tube2Tire;
    returning = false;
//    testing=false;
    while(moving){
        if(send){
            if(!detect) {
                set_pwm_duty_cycle(startPWM, 1);
                set_pwm_duty_cycle(startPWM, 2);
                while((pulse2-newPulse2)<13 && moving && noDeploy){
                    continue;
                }
                noDeploy=false;
                data='A';
                I2C_Master_Start(); // Start condition
                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
                I2C_Master_Write(data); // Write key press data
                I2C_Master_Stop();
               
//                pulse2=pulse2+(displayCnt)*10;//TESTING
//                displayCnt++;
                send = false;
                lcd_clear();
                lcd_home();
                printf("Wheels Start");
                continue;
            }
            else{
                data = 'S';
                I2C_Master_Start(); // Start condition
                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
                I2C_Master_Write(data); // Write key press data
                I2C_Master_Stop();
                send = false;
                lcd_clear();
                lcd_home();
                printf("Signal Sent");
                continue;
            }
        }else{
            I2C_Master_Start();
            I2C_Master_Write(0b00001111); // 7-bit Arduino slave address + Read
            data = I2C_Master_Read(NACK); // Read one char only
            I2C_Master_Stop();
            if (pulse1 > endDist) {//distance of 4m
                returning=true;
                break;
            }
            if (poleCnt>=10){
                returning=true;
                break;
            }
            if ((data)&&(data < 255)&&(data != 10)) {
                if (data == 'B' && !detect) {
                    lcd_clear();
                    lcd_home();
                    printf("Pole Detected");
                    pulseCnt=pulse2;
                    detect = true; //a pole is detected
                    send=true;
                    while((pulse2-pulseCnt)<5){
                        continue;
                    }
//                    testing = true;
                    set_pwm_duty_cycle(0, 1);
                    set_pwm_duty_cycle(0, 2);
                    lcd_clear();
                    lcd_home();
                    printf("Machine Stop%d",poleCnt);
//                    __delay_ms(1000);//TESTING
                    continue;
                }
                if(data == 'D' && detect) {
                    lcd_clear();
                    lcd_home();
                    printf("Receiving Data");
                    send=false;
                    detect = false;
                    deploy = true;
                    in_cnt = 0;
                    continue;
                }
                if (deploy) {
                    if (in_cnt == 0) {
                        dist[in_cnt] = data;
                    }
                    else if(in_cnt < 3){
                        dist[in_cnt] = data-1;
                    }
                    else if (data=='E' && in_cnt>=3) {
                        cntrDist = (dist[0] | (dist[1] << 8));
                        totCntrDist[poleCnt]=cntrDist;
                        oldTire = dist[2];
                        totOldTire[poleCnt]=oldTire;
                        lcd_clear();
                        lcd_home();
                        printf("Pole Dist:%d", cntrDist);
                        lcd_set_ddram_addr(LCD_LINE2_ADDR);
                        printf("Tire Num:%d", oldTire);
                        
                        supplyTire=tireCheck(oldTire);//fuc calculate # of tires to supply
                        
                        lcd_set_ddram_addr(LCD_LINE3_ADDR);
                        printf("Dist:%d;Supply:%d",pulse1,supplyTire);//change to pulse 2 later
                        totSupplyTire[poleCnt]=supplyTire;
                        
                        deployTireFin(cntrDist-extOffset,supplyTire);//fuc deploy # of tires from a selected pole
                        poleCnt++;
                        in_cnt = 0;
                        detect=false;
                        deploy=false;
                        send=true;
                        continue;
                    }
                    in_cnt++;
                }
            }
        }
    }
    if(returning){
        lcd_clear();
        lcd_home();
        printf("Run Terminated"); 
        LATBbits.LATB3 = 0;//turning back
        pulse1=0;
        pulse2=0;
        set_pwm_duty_cycle(returnPWM, 1);
        set_pwm_duty_cycle(returnPWM, 2);
        while(pulse2<=endDist+100 && moving){
            continue;
        }
    }
    printf("Run End");
//    data = 'T';
//    I2C_Master_Start(); // Start condition
//    I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
//    I2C_Master_Write(data); // Write key press data
//    I2C_Master_Stop();
//    
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    moving=true;
    displayCnt=0;
    while(moving){
        lcd_clear();
        lcd_home();
        printf("Pole%d",displayCnt);
        lcd_set_ddram_addr(LCD_LINE2_ADDR);
        printf("Start Dist%d",totStartDist[displayCnt]);
        lcd_set_ddram_addr(LCD_LINE3_ADDR);
        printf("Cntr Dist%d",totCntrDist[displayCnt]);
        lcd_set_ddram_addr(LCD_LINE4_ADDR);
        printf("Tire:%d,Supply:%d", totOldTire[displayCnt],totSupplyTire[displayCnt]);
        displayCnt++;
        if(displayCnt==poleCnt){
            displayCnt=0;
        }
        __delay_ms(3000);
    }
    return;
}

void fullFucTestVer2(void){
    /*Replace all polling with interrupts
     * Use timer delays instead of external interrupts from encoders
     * Should I change to Timer interrupt?
     */
    bool send = true;
    bool detect = false;
    bool deploy = false;
    unsigned char in_cnt = 0;
    unsigned char dist[3];
    int displayCnt=0;
    poleCnt=0;
    pulse1 = 0;
    pulse2 = 0;
    newPulse1 = 0;
    newPulse2 = 0;
    pulseTemp = 0;
    tube1_tire = tube1Tire;
    tube2_tire = tube2Tire;
    returning = false;
    while(moving){
        if(send){
            if(!detect) {
                set_pwm_duty_cycle(startPWM, 1);
                set_pwm_duty_cycle(startPWM, 2);
                startMotor=true;
                pulseTemp=pulse2;
                while(startMotor && moving){
                    continue;
                }
                
                data='A';
                I2C_Master_Start(); // Start condition
                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
                I2C_Master_Write(data); // Write key press data
                I2C_Master_Stop();
                send = false;
                lcd_clear();
                lcd_home();
                printf("Wheels Start");
                continue;
            }
            else{
                data = 'S';
                I2C_Master_Start(); // Start condition
                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
                I2C_Master_Write(data); // Write key press data
                I2C_Master_Stop();
                send = false;
                lcd_clear();
                lcd_home();
                printf("Signal Sent");
                continue;
            }
        }else{
            I2C_Master_Start();
            I2C_Master_Write(0b00001111); // 7-bit Arduino slave address + Read
            data = I2C_Master_Read(NACK); // Read one char only
            I2C_Master_Stop();
            if (pulse1 > endDist) {//distance of 4m
                returning=true;
                break;
            }
            if (poleCnt>=10){
                returning=true;
                break;
            }
            if ((data)&&(data < 255)&&(data != 10)) {
                if (data == 'B' && !detect) {
                    lcd_clear();
                    lcd_home();
                    printf("Pole Detected");
                    detect = true; //a pole is detected
                    send=true;
                    pulseTemp=pulse2;
                    firstStop=true;
                    while(firstStop && moving){
                        //stop the machine ~3cm after the first detection
                        continue;
                    }
                    LATBbits.LATB3 = 0;
                    __delay_ms(50);
                    set_pwm_duty_cycle(0, 1);
                    set_pwm_duty_cycle(0, 2);
                    LATBbits.LATB3 = 1;
                    lcd_clear();
                    lcd_home();
                    printf("Machine Stop%d",poleCnt);
                    continue;
                }
                if(data == 'D' && detect) {
                    lcd_clear();
                    lcd_home();
                    printf("Receiving Data");
                    send=false;
                    detect = false;
                    deploy = true;
                    in_cnt = 0;
                    continue;
                }
                if (deploy) {
                    if (in_cnt == 0) {
                        dist[in_cnt] = data;
                    }
                    else if(in_cnt < 3){
                        dist[in_cnt] = data-1;
                    }
                    else if (data=='E' && in_cnt>=3) {
                        cntrDist = (dist[0] | (dist[1] << 8));
                        totCntrDist[poleCnt]=cntrDist;
                        oldTire = dist[2];
                        totOldTire[poleCnt]=oldTire;
                        lcd_clear();
                        lcd_home();
                        printf("Pole Dist:%d", cntrDist);
                        lcd_set_ddram_addr(LCD_LINE2_ADDR);
                        printf("Tire Num:%d", oldTire);
                        
                        supplyTire=tireCheck(oldTire);//fuc calculate # of tires to supply
                        
                        lcd_set_ddram_addr(LCD_LINE3_ADDR);
                        printf("Dist:%d;Supply:%d",pulse1,supplyTire);//change to pulse 2 later
                        totSupplyTire[poleCnt]=supplyTire;
                        
                        deployTireFin(cntrDist-extOffset,supplyTire);//fuc deploy # of tires from a selected pole
                        poleCnt++;
                        in_cnt = 0;
                        detect=false;
                        deploy=false;
                        send=true;
                        continue;
                    }
                    in_cnt++;
                }
            }
            if(longDist){
                set_pwm_duty_cycle(startPWM, 1);
                set_pwm_duty_cycle(startPWM, 2);
                longDist=false;
                newPulse1=pulse1;
            }
        }
    }
    if(returning){
        lcd_clear();
        lcd_home();
        printf("Run Terminated"); 
        LATBbits.LATB3 = 0;//turning back
        pulse1=0;
        pulse2=0;
        set_pwm_duty_cycle(returnPWM, 1);
        set_pwm_duty_cycle(returnPWM, 2);
        while(pulse2<=endDist+100 && moving){
            continue;
        }
    }
    printf("Run End");
//    data = 'T';
//    I2C_Master_Start(); // Start condition
//    I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
//    I2C_Master_Write(data); // Write key press data
//    I2C_Master_Stop();
//    
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    moving=true;
    displayCnt=0;
    while(moving){
        lcd_clear();
        lcd_home();
        printf("Pole%d",displayCnt);
        lcd_set_ddram_addr(LCD_LINE2_ADDR);
        printf("Start Dist%d",totStartDist[displayCnt]);
        lcd_set_ddram_addr(LCD_LINE3_ADDR);
        printf("Cntr Dist%d",totCntrDist[displayCnt]);
        lcd_set_ddram_addr(LCD_LINE4_ADDR);
        printf("Tire:%d,Supply:%d", totOldTire[displayCnt],totSupplyTire[displayCnt]);
        displayCnt++;
        if(displayCnt==poleCnt){
            displayCnt=0;
        }
        __delay_ms(3000);
    }
    return;
}


void deployTire(unsigned int extDist){
    ext_stepper_control(extDist, 0);
//    tube_stepper_control(1,2);
    tube_stepper_control(2,2);
    ext_stepper_control(extDist, 1);
    lcd_clear();
    lcd_home();
    printf("Deploy End");
    __delay_ms(1000);
    return;
}

void resetStepper(void) {
    /* 1. Type in the tube id you want to reset. 
     *    Tube 1 will go backwards for 7 tires; tube 2: 8 tires.
     * 2. Type in the distance in mm of the extension motor you want to reset. 
     *    It'll go backwards for this distance*/
    unsigned int x = 40;
    unsigned int extDist = 0;
    unsigned char counter = 0;
    unsigned char temp = 0;
    int index = 0;
    int i = 0;
    int stepper_para;
//7 * 135
    lcd_clear();
    lcd_home();
    printf("Reset Tube Step");
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    printf("Tube ID:");
    readKey = true;
    while (readKey) {
        while (PORTBbits.RB1 == 0) {
            continue;
        }
        keypress = (PORTB & 0xF0) >> 4;
        while (PORTBbits.RB1 == 1) {
            continue;
        }
        Nop(); // Apply breakpoint here to prevent compiler optimizations
        data = keys[keypress];
        if (keypress == 14) {// Enter: end of input
            readKey = false;
            break;
        } else {
            putch(data); // Push the character to be displayed on the LCD
            tubeID = keypadConver(keypress);
        }
    }
    lcd_clear();
    lcd_home();
    printf("Reset Tube %d",tubeID);
    
    if (tubeID == 1) {
        TRISAbits.RA0 = 0; //step
        LATAbits.LATA0 = 0;
        TRISAbits.RA1 = 0; //direction
        LATAbits.LATA1 = 1; //high going backward
        stepper_para=7 * 135;
        for (index = 0; index < stepper_para; index++) {
            for (i = 0; i < 5; i++) {
                Trigger_Pulse(x, 1);
            }
            if (x > 15) {
                x--;
            }
            if(!moving){
                break;
            }
        }   
    } else if (tubeID == 2){
        TRISEbits.RE0 = 0; //step
        LATEbits.LATE0 = 0;
        TRISCbits.RC5 = 0; //direction
        LATCbits.LATC5 = 1; //low
        stepper_para=8 * 135;
        for (index = 0; index < stepper_para; index++) {
            for (i = 0; i < 5; i++) {
                Trigger_Pulse(x, 2);
            }
            if (x > 15) {
                x--;
            }
            if(!moving){
                break;
            }
        }
    }
    LATAbits.LATA0 = 0;
    LATAbits.LATA1 = 0;
    LATAbits.LATA2 = 0;
    LATAbits.LATA3 = 0;
    __delay_ms(10);
    
    lcd_clear();
    lcd_home();
    printf("Reset Ext Step");
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    printf("Ext Dist:");
    readKey = true;
    while (readKey) {
        while (PORTBbits.RB1 == 0) {
            continue;
        }
        keypress = (PORTB & 0xF0) >> 4;
        while (PORTBbits.RB1 == 1) {
            continue;
        }
        Nop(); // Apply breakpoint here to prevent compiler optimizations
        data = keys[keypress];
        if (keypress == 14) {// Enter: end of input
            readKey = false;
            break;
        }
        else {
                putch(data); // Push the character to be displayed on the LCD
                temp = keypadConver(keypress);
                if (temp >= 0 && temp < 10) {
                    extDist = temp + (extDist * 10);
                    counter++;
                    if (counter > 3) {
                        counter = 0;
                        extDist = 0;
                    }
                } else {
                    extDist = 0;
                    counter = 0;
                }
            }
        }
        if (extDist < 300) {
            lcd_clear();
            lcd_home();
            printf("Ext Dist=%d", extDist);
            ext_stepper_control(extDist, 1);
            
        } else {
            lcd_clear();
            lcd_home();
            printf("Invaild Dist:%d!", extDist);
            __delay_ms(1000);
            return;
        }
    return;
}

//void wheelTest(void){
//    lcd_clear();
//    lcd_home();
//    printf("Wheel Test");
//    TRISBbits.RB3 = 0; //mux choice
//    LATBbits.LATB3 = 1;
//    set_pwm_duty_cycle(startPWM, 1);
//    set_pwm_duty_cycle(startPWM, 2);
//    __delay_ms(3000);
//    slowStop();
//    LATBbits.LATB3 = 0;
//    set_pwm_duty_cycle(startPWM, 1);
//    set_pwm_duty_cycle(startPWM, 2);
//    __delay_ms(3000);
//    set_pwm_duty_cycle(0, 1);
//    set_pwm_duty_cycle(0, 2);
//    slowStop();
//    return;
//}

//void wheelBack(void){
//    lcd_clear();
//    lcd_home();
//    printf("Wheel Test");
//    TRISBbits.RB3 = 0; //mux choice
//    LATBbits.LATB3 = 0;
//    set_pwm_duty_cycle(startPWM, 1);
//    set_pwm_duty_cycle(startPWM, 2);
//    __delay_ms(3000);
//    slowStop();
//}

void slowStart(void){
    while(pwmVal1<=500 && pwmVal2<=500){
        pwmVal1+=32;
        pwmVal2+=32;
        set_pwm_duty_cycle(pwmVal1, 1);
        set_pwm_duty_cycle(pwmVal2, 2);
        __delay_ms(20);
    }
    set_pwm_duty_cycle(startPWM, 1);
    set_pwm_duty_cycle(startPWM, 2);
    return;
}

void shaftEncoder(void) {
    /*We were trying to change speed to make it go straight.
    Then I realize, this is not possible for our structure.*/
    lcd_clear();
    lcd_home();
    printf("Encoder Test");
    int pwmVal1=startPWM;//Left
    int pwmVal2=startPWM;//Right
    int cnt=0;
    set_pwm_duty_cycle(pwmVal1, 1);
    set_pwm_duty_cycle(pwmVal2, 2);
    pulse1=0;
    pulse2=0;
    leftWheelDist=0;
    rightWheelDist=0;
    while(moving){
        __delay_ms(200);
//        timePulse=(TMR1L|(TMR1H<<8));
//        rpm=(6000) / timePulse * pulse1;
//        leftWheelDist=(pulse1/20)*(M_PI)*6.7;
//        rightWheelDist=(pulse2/20)*(M_PI)*6.7;
//        if ((pulse1-pulse2)>3){
////            pwmVal1=pwmVal1+30;
//            pwmVal2=pwmVal2+30;
//        }
//        else if((pulse2-pulse1)>3){
//            pwmVal1=pwmVal1+30;
////            pwmVal2=pwmVal2+30;
//        }
        cnt++;
        set_pwm_duty_cycle(pwmVal1, 1);
        set_pwm_duty_cycle(pwmVal2, 2);
        if (cnt >= 10) {
            lcd_clear();
            lcd_home();
            leftWheelDist=leftWheelDist+(pulse1/20)*(M_PI)*6.7;
            rightWheelDist=rightWheelDist+(pulse2/20)*(M_PI)*6.7;
            printf("L:%d,R:%d", pulse1, pulse2);
            lcd_set_ddram_addr(LCD_LINE2_ADDR);
            printf("L:%0.3f,R:%0.3f", leftWheelDist,rightWheelDist);
            lcd_set_ddram_addr(LCD_LINE3_ADDR);
            printf("L:%d,R:%d", pwmVal1,pwmVal2);
            pulse1 = 0;
            pulse2 = 0;
            cnt=0;
        }
    }
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    LATBbits.LATB3 = 1;
    return;
}

void distCheck(void){
    pulse1=0;
    pulse2=0;
    set_pwm_duty_cycle(startPWM, 1);
    set_pwm_duty_cycle(startPWM, 2);
    while(moving){
//        if((pulse2)>endDist){
//            break;
//        }
        continue;
    }
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    moving=true;
    lcd_clear();
    lcd_home();
    printf("L:%d,R:%d", pulse1, pulse2);
    leftWheelDist = leftWheelDist + (pulse1 / 20)*(M_PI)*6.7;
    rightWheelDist = rightWheelDist + (pulse2 / 20)*(M_PI)*6.7;
    printf("L:%0.3f,R:%0.3f", leftWheelDist,rightWheelDist);
    while(moving){
        continue;
    }
    moving=true;
    LATBbits.LATB3 = 0;
    PWM_freq=5000;
    initPWM();
    set_pwm_duty_cycle(returnPWM, 1);
    set_pwm_duty_cycle(returnPWM, 2);
    pulse2=0;
    while(moving && (pulse2<(endDist+200))){
        continue;
    }
    LATBbits.LATB3 = 1;
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    return;
}

void timeReceive(void){
    bool timeGet = false;
    data = 'T';
    I2C_Master_Start(); // Start condition
    I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
    I2C_Master_Write(data); // Write key press data
    I2C_Master_Stop();
    while(!timeGet && moving){
        I2C_Master_Start();
        I2C_Master_Write(0b00001111); // 7-bit Arduino slave address + Read
        data = I2C_Master_Read(NACK); // Read one char only
        I2C_Master_Stop();
        if ((data)&&(data < 255)&&(data != 10)){
            runTime = data;
            timeGet=true;
        }
    }
}

void runTimeTest(void) {
    unsigned int time;
    T1CKPS1 = 1; // bits 5-4  Prescaler Rate Select bits
    T1CKPS0 = 0; // bit 4
    T1OSCEN = 0; // bit 3 Timer1 Oscillator Enable Control bit 0 = off
    T1SYNC = 1; // bit 2 Timer1 External Clock Input Synchronization Control bit...1 = Do not synchronize external clock input
    TMR1CS = 0; // bit 1 Timer1 Clock Source Select bit...0 = Internal clock (FOSC/4)
    TMR1ON = 1; // bit 0 enables timer
    TMR1H = 11; // preset for timer1 MSB register
    TMR1L = 220; // preset for timer1 LSB register
    
    INTCON = 0; // clear the interrpt control register
    TMR0IE = 0; // bit5 TMR0 Overflow Interrupt Enable bit...0 = Disables the TMR0 interrupt
    TMR1IF = 0; // clear timer1 interupt flag TMR1IF
    TMR1IE = 1; // enable Timer1 interrupts
    TMR0IF = 0; // bit2 clear timer 0 interrupt flag
    GIE = 1; // bit7 global interrupt enable
    PEIE = 1; 
    lcd_clear();
    lcd_home();
    printf("Run Timer Test");
    runTime=0;
    returning=false;
    moving=true;
    TMR1ON = 1;
    while(!returning){
        continue;
    }
    TMR1ON = 0;
    lcd_clear();
    lcd_home();
    printf("Sec:%d",runTime);
//    time= (TMR1L|(TMR1H<<8));
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    time=TMR1L;
    printf("TimerL:%u",time);
    lcd_set_ddram_addr(LCD_LINE3_ADDR);
    time=TMR1H;
    printf("TimerH:%u",time);
    moving=true;
    while(moving){
        continue;
    }
    return;
}

//void run(void) {
//    /*Initialize I2C with address 7*/
//    I2C_Master_Init(100000);
//    I2C_Master_Start();
//    I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
//    I2C_Master_Stop();
//    int t = 0;
//    for (t = 0; t < 7; t++) {
//        runClk[t] = time[t];
//    }
//    /*Initialize timer interrupts*/
//    initTimer();
//    /*Initialize PWM*/
//    initPWM();
//    /*Initialize all output pins for motors preventing floating in the begining*/
//    //<editor-fold defaultstate="collapsed" desc="initMotor">
//    //tube 1 
//    TRISAbits.RA4 = 0; //step 
//    LATAbits.LATA4 = 0;
//    TRISAbits.RA5 = 0; //direction
//    LATAbits.LATA5 = 0;
//    //tube 2
//    TRISEbits.RE0 = 0; //step
//    LATEbits.LATE0 = 0;
//    TRISCbits.RC5 = 0; //direction
//    LATCbits.LATC5 = 0; //low
//    //ext
//    TRISAbits.RA0 = 0; //step
//    LATAbits.LATA0 = 0;
//    TRISAbits.RA1 = 0; //direction
//    LATAbits.LATA1 = 0; //low
//    //mux
//    TRISBbits.RB3 = 0; //mux choice
//    LATBbits.LATB3 = 1; // 1 going forward
//    //PWM duty cycle = 0 in the beginning
//    set_pwm_duty_cycle(0, 1);
//    set_pwm_duty_cycle(0, 2);
//    //</editor-fold>
//    /*Private variables*/
//    unsigned char in_cnt = 0;
//    unsigned char dist[3];
//    /*Public variables*/
//    send = true;
//    detect = false;
//    deploy = false;
//    poleCnt = 0; //No pole has counted in the beginning
//    pulse1 = 0; //Clear pulses before the left wheel starts
//    pulse2 = 0; //Clear pulses before the right wheel starts
//    newPulse1 = 0; //Clear the short-term storage of pulse 1
//    newPulse2 = 0; //Clear the short-term storage of pulse 2
//    pulseTemp = 0; //Clear the temporary storage of pulses
//    tube1_tire = tube1Tire; //Initial number of tires stored in tube 1
//    tube2_tire = tube2Tire; //Initial number of tires stored in tube 2
//    runTime = 0;
//    stateCode = 0;
//    lcd_clear();
//    lcd_home();
//    printf("Sensor Adjust");
//    /*Process interruptions*/
//    returning = false; //Checking if the machine need to return back
//    moving = true; //Ult check of infinite loop check
//    looping = false; //Checking if the machine is trapped in a infinite loop
//    /*The main loop is a while loop. Making sure there is no delay in the receiving process*/
//    while (moving) {
//        if (send) {
//            if (!detect) {
//                set_pwm_duty_cycle(startPWM, 1); //Start DC motors right 
//                set_pwm_duty_cycle(startPWM, 2);//left
//                lcd_clear();
//                lcd_home();
//                printf("Wheel Started");
//                /*The DC motors need some time to start 
//                 *and accelerate to a desired speed*/
//                stateCode = 1;
//                pulseTemp = pulse2;
//                startMotor = true;
//                loopTime = 0; //timer interrupt counter
//                while (startMotor && moving) {
//                    continue;
//                }
//                //Start Data sending
//                data = 'A';
//                I2C_Master_Start(); // Start condition
//                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
//                I2C_Master_Write(data); // Write key press data
//                I2C_Master_Stop();
//                send = false;
//                continue;
//            } else {
//                data = 'S';
//                I2C_Master_Start(); // Start condition
//                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
//                I2C_Master_Write(data); // Write key press data
//                I2C_Master_Stop();
//                send = false;
//                stateCode = 3;
//                lcd_clear();
//                lcd_home();
//                printf("Signal Sent");
//                continue;
//            }
//        } else {//receiving
//            I2C_Master_Start();
//            I2C_Master_Write(0b00001111); // 7-bit Arduino slave address + Read
//            data = I2C_Master_Read(NACK); // Read one char only
//            I2C_Master_Stop();
//            /*Removed end conditions*/
//            if ((data)&&(data < 255)&&(data != 10)) {
//                if (data == 'B' && !detect && !deploy) {
//                    lcd_clear();
//                    lcd_home();
//                    printf("Pole Detected");
//                    /*Pole Detected*/
//                    stateCode = 1;
//                    detect = true; //a pole is detected
//                    send = true;
//                    deploy = false;
//                    /*Stop and align with sensors*/
//                    pulseTemp = pulse2;
//                    firstStop = true;
//                    loopTime = 0; //timer interrupt counter
//                    while (firstStop && moving) {
//                        //stop the machine ~3cm after the first detection
//                        continue;
//                    }
//                    /*Sharp stop by reversing the wheels*/
//                    LATBbits.LATB3 = 0;
//                    __delay_ms(50);
////                    LATCbits.LATC1 = 0;
////                    LATCbits.LATC2 = 0;
////                    initPWM();
//                    set_pwm_duty_cycle(0, 1);
//                    set_pwm_duty_cycle(0, 2);
//                    LATBbits.LATB3 = 1;
//                    totStartDist[poleCnt] = pulse2;
//                    totCntrDist[poleCnt] = pulse1;
//                    totStartDistCm[poleCnt] = (int) (slop * pulse2 + intercept);
//                    if(poleCnt==0){
//                        totStartDistCm[poleCnt] = (int) (slop * pulse2 + intercept-6);
//                    }
//                    lcd_clear();
//                    lcd_home();
//                    printf("Machine Stop%d", poleCnt);
//                    continue;
//                }
//                if (data == 'D' && detect && !deploy) {
//                    /*Receiving Data*/
//                    lcd_clear();
//                    lcd_home();
//                    printf("Receiving Data");
//                    send = false;
//                    detect = false;
//                    deploy = true;
//                    in_cnt = 0;
//                    loopTime = 0; //timer interrupt counter
//                    stateCode = 4;
//                    continue;
//                }
//                if (deploy) {
//                    if (in_cnt == 0) {
//                        dist[in_cnt] = data;
//                    } else if (in_cnt < 3) {
//                        dist[in_cnt] = data - 1;
//                    } else if (data == 'E' && in_cnt >= 3) {
//                        stateCode = 5;
//                        cntrDist = (dist[0] | (dist[1] << 8));
//                        totCntrDist[poleCnt] = cntrDist;
//                        oldTire = dist[2];
//                        totOldTire[poleCnt] = oldTire;
//                        lcd_clear();
//                        lcd_home();
//                        printf("Pole Dist:%d", cntrDist);
//                        lcd_set_ddram_addr(LCD_LINE2_ADDR);
//                        printf("Tire Num:%d", oldTire);
//                        supplyTire = tireCheck(oldTire); //fuc calculate # of tires to supply
//                        totSupplyTire[poleCnt] = supplyTire;
//                        totTire += supplyTire;
//                        stateCode = 6;
//                        deployTireFin(cntrDist - extOffset, supplyTire); //fuc deploy # of tires from a selected pole
//                        totStateCode[poleCnt] = stateCode;
//                        poleCnt++;
//                        in_cnt = 0;
//                        send = true;
//                        detect = false;
//                        deploy = false;
//                        continue;
//                    }
//                    in_cnt++;
//                }
//            }
//            if (pulse2 > endDist) {//distance of 4m
//                returning = true;
//                moving=false;
//                break;
//            }
//            if (returning) {
//                break;
//            }
//        }
//    }
//    lcd_clear();
//    lcd_home();
//    printf("Run Terminated:%d",pulse2);
//    returnRun();
//    TMR1ON = 0; //stop timer
//    moving = true;
//    machine_state = doneRunning_state;
//    return;
//}

void sensorAdjust(void) {
    /*Initialize I2C with address 7*/
    I2C_Master_Init(100000);
    I2C_Master_Start();
    I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
    I2C_Master_Stop();
    int t = 0;
    for (t = 0; t < 7; t++) {
        runClk[t] = time[t];
    }
    /*Initialize timer interrupts*/
    initTimer();
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
    /*Private variables*/
    unsigned char in_cnt = 0;
    unsigned char dist[3];
    /*Public variables*/
    send = true;
    detect = false;
    deploy = false;
    poleCnt = 0; //No pole has counted in the beginning
    pulse1 = 0; //Clear pulses before the left wheel starts
    pulse2 = 0; //Clear pulses before the right wheel starts
    newPulse1 = 0; //Clear the short-term storage of pulse 1
    newPulse2 = 0; //Clear the short-term storage of pulse 2
    pulseTemp = 0; //Clear the temporary storage of pulses
    tube1_tire = 0;//Initial number of tires stored in tube 1
    tube2_tire = tube2Tire;//Initial number of tires stored in tube 2
    runTime = 0;
    stateCode = 0;
    lcd_clear();
    lcd_home();
    printf("Sensor Adjust");
    /*Process interruptions*/
    returning = false; //Checking if the machine need to return back
    moving = true; //Ult check of infinite loop check
    looping = false; //Checking if the machine is trapped in a infinite loop
    /*The main loop is a while loop. Making sure there is no delay in the receiving process*/
    while (moving) {
        if (send) {
            if (!detect) {
                set_pwm_duty_cycle(startPWM, 1); //Start DC motors
                set_pwm_duty_cycle(startPWM, 2);
                lcd_clear();
                lcd_home();
                printf("Wheel Started");
                /*The DC motors need some time to start 
                 *and accelerate to a desired speed*/
                stateCode = 1;
                pulseTemp = pulse2;
                startMotor = true;
                loopTime = 0; //timer interrupt counter
                while (startMotor && moving) {
                    continue;
                }
                //Start Data sending
                data = 'A';
                I2C_Master_Start(); // Start condition
                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
                I2C_Master_Write(data); // Write key press data
                I2C_Master_Stop();
                send = false;
                continue;
            } else {
                data = 'S';
                I2C_Master_Start(); // Start condition
                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
                I2C_Master_Write(data); // Write key press data
                I2C_Master_Stop();
                send = false;
                stateCode = 3;
                lcd_clear();
                lcd_home();
                printf("Signal Sent");
                continue;
            }
        } else {//receiving
            I2C_Master_Start();
            I2C_Master_Write(0b00001111); // 7-bit Arduino slave address + Read
            data = I2C_Master_Read(NACK); // Read one char only
            I2C_Master_Stop();
            /*Removed end conditions*/
            if ((data)&&(data < 255)&&(data != 10)) {
                if (data == 'B' && !detect && !deploy) {
                    lcd_clear();
                    lcd_home();
                    printf("Pole Detected");
                    /*Pole Detected*/
                    stateCode = 1;
                    detect = true; //a pole is detected
                    send = true;
                    deploy = false;
                    /*Stop and align with sensors*/
                    pulseTemp = pulse2;
                    firstStop = true;
                    loopTime = 0; //timer interrupt counter
                    while (firstStop && moving) {
                        //stop the machine ~3cm after the first detection
                        continue;
                    }
                    /*Sharp stop by reversing the wheels*/
                    LATBbits.LATB3 = 0;
                    __delay_ms(50);
//                    LATCbits.LATC1 = 0;
//                    LATCbits.LATC2 = 0;
//                    initPWM();
                    set_pwm_duty_cycle(0, 1);
                    set_pwm_duty_cycle(0, 2);
                    LATBbits.LATB3 = 1;
                    totStartDist[poleCnt] = pulse2;
                    totCntrDist[poleCnt] = pulse1;
                    totStartDistCm[poleCnt] = (int) ((slop * pulse2) + intercept - 11.5);
                    lcd_clear();
                    lcd_home();
                    printf("Machine Stop%d", poleCnt);
                    continue;
                }
                if (data == 'D' && detect && !deploy) {
                    /*Receiving Data*/
                    lcd_clear();
                    lcd_home();
                    printf("Receiving Data");
                    send = false;
                    detect = false;
                    deploy = true;
                    in_cnt = 0;
                    loopTime = 0; //timer interrupt counter
                    stateCode = 4;
                    continue;
                }
                if (deploy) {
                    if (in_cnt == 0) {
                        dist[in_cnt] = data;
                    } else if (in_cnt < 3) {
                        dist[in_cnt] = data - 1;
                    } else if (data == 'E' && in_cnt >= 3) {
                        stateCode = 5;
                        cntrDist = (dist[0] | (dist[1] << 8));
                        totCntrDist[poleCnt] = cntrDist;
                        oldTire = dist[2];
                        totOldTire[poleCnt] = oldTire;
                        lcd_clear();
                        lcd_home();
                        printf("Pole Dist:%d", cntrDist);
                        lcd_set_ddram_addr(LCD_LINE2_ADDR);
                        printf("Tire Num:%d", oldTire);
                        supplyTire = tireCheck(oldTire); //fuc calculate # of tires to supply
                        totSupplyTire[poleCnt] = supplyTire;
                        totTire += supplyTire;
                        stateCode = 6;
                        deployTireFin(cntrDist - extOffset, supplyTire); //fuc deploy # of tires from a selected pole
                        totStateCode[poleCnt] = stateCode;
                        poleCnt++;
                        in_cnt = 0;
                        send = true;
                        detect = false;
                        deploy = false;
                        continue;
                    }
                    in_cnt++;
                }
            }
            if (returning) {
                break;
            }
        }
    }
    lcd_clear();
    lcd_home();
    printf("Run Terminated:%d",pulse2);
    returnRun();
    TMR1ON = 0; //stop timer
    moving = true;
    machine_state = doneRunning_state;
    return;
}
//void run(void) {
//    /*Initialize I2C with address 7*/
//    I2C_Master_Init(100000);
//    I2C_Master_Start();
//    I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
//    I2C_Master_Stop();
//    int t=0;
//    for(t=0;t<7;t++){
//        runClk[t]=time[t];
//    }
//    /*Initialize timer interrupts*/
//    initTimer();
//    /*Initialize PWM*/
//    initPWM();
//    /*Initialize all output pins for motors preventing floating in the begining*/
//    //<editor-fold defaultstate="collapsed" desc="initMotor">
//    //tube 1 
//    TRISAbits.RA4 = 0; //step 
//    LATAbits.LATA4 = 0;
//    TRISAbits.RA5 = 0; //direction
//    LATAbits.LATA5 = 0;
//    //tube 2
//    TRISEbits.RE0 = 0; //step
//    LATEbits.LATE0 = 0;
//    TRISCbits.RC5 = 0; //direction
//    LATCbits.LATC5 = 0; //low
//    //ext
//    TRISAbits.RA0 = 0; //step
//    LATAbits.LATA0 = 0;
//    TRISAbits.RA1 = 0; //direction
//    LATAbits.LATA1 = 0; //low
//    //mux
//    TRISBbits.RB3 = 0; //mux choice
//    LATBbits.LATB3 = 1; // 1 going forward
//    //PWM duty cycle = 0 in the beginning
//    set_pwm_duty_cycle(0, 1);
//    set_pwm_duty_cycle(0, 2);
//    //</editor-fold>
//    /*Private variables*/
//    unsigned char in_cnt = 0;
//    unsigned char dist[3];
//    /*Public variables*/
//    send = true;
//    detect = false;
//    deploy = false;
//    poleCnt=0;//No pole has counted in the beginning
//    pulse1 = 0;//Clear pulses before the left wheel starts
//    pulse2 = 0;//Clear pulses before the right wheel starts
//    newPulse1 = 0;//Clear the short-term storage of pulse 1
//    newPulse2 = 0;//Clear the short-term storage of pulse 2
//    pulseTemp = 0;//Clear the temporary storage of pulses
//    tube1_tire = tube1Tire;//Initial number of tires stored in tube 1
//    tube2_tire = tube2Tire;//Initial number of tires stored in tube 2
//    runTime=0;
//    stateCode = 0;
//    lcd_clear();
//    lcd_home();
//    printf("Run Start");
//    /*Process interruptions*/
//    returning = false;//Checking if the machine need to return back
//    moving=true;//Ult check of infinite loop check
//    looping=false;//Checking if the machine is trapped in a infinite loop
//    /*The main loop is a while loop. Making sure there is no delay in the receiving process*/
//    while(moving){
//        if(send){
//            if(!detect) {
//                set_pwm_duty_cycle(startPWM, 1);//Start DC motors
//                set_pwm_duty_cycle(startPWM, 2);
//                lcd_clear();
//                lcd_home();
//                printf("Wheel Started");
//                /*The DC motors need some time to start 
//                 *and accelerate to a desired speed*/
//                stateCode = 1;
//                pulseTemp=pulse2;
//                startMotor=true;
//                loopTime = 0;//timer interrupt counter
//                while(startMotor && moving){
//                    continue;
//                }
//                //Start Data sending
//                data='A';
//                I2C_Master_Start(); // Start condition
//                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
//                I2C_Master_Write(data); // Write key press data
//                I2C_Master_Stop();
//                send = false;
//                continue;
//            }
//            else{
//                data = 'S';
//                I2C_Master_Start(); // Start condition
//                I2C_Master_Write(0b00001110); // 7-bit Arduino slave address + write
//                I2C_Master_Write(data); // Write key press data
//                I2C_Master_Stop();
//                send = false;
//                stateCode=3;
//                lcd_clear();
//                lcd_home();
//                printf("Signal Sent");
//                continue;
//                continue;
//            }
//        }else{
//            I2C_Master_Start();
//            I2C_Master_Write(0b00001111); // 7-bit Arduino slave address + Read
//            data = I2C_Master_Read(NACK); // Read one char only
//            I2C_Master_Stop();
//            /*Removed end conditions*/
//            if ((data)&&(data < 255)&&(data != 10)) {
//                if (data == 'B' && !detect) {
//                    lcd_clear();
//                    lcd_home();
//                    printf("Pole Detected");
//                    /*Pole Detected*/
//                    stateCode = 1;
//                    detect = true; //a pole is detected
//                    send = true;
//                    deploy = false;
//                    /*Stop and align with sensors*/
//                    pulseTemp = pulse2;
//                    firstStop=true;
//                    loopTime=0;//timer interrupt counter
//                    while(firstStop && moving){
//                        //stop the machine ~3cm after the first detection
//                        continue;
//                    }
//                    /*Sharp stop by reversing the wheels*/
//                    LATBbits.LATB3 = 0;
//                    __delay_ms(50);
//                    set_pwm_duty_cycle(0, 1);
//                    set_pwm_duty_cycle(0, 2);
//                    LATBbits.LATB3 = 1;
//                    totStartDist[poleCnt] = pulse2;
//                    totCntrDist[poleCnt] = pulse1;
//                    totStartDistCm[poleCnt] = (int) (slop * pulse2 + intercept);
//                    lcd_clear();
//                    lcd_home();
//                    printf("Machine Stop%d", poleCnt);
//                    continue;
//                }
//                if(data == 'D' && detect) {
//                    /*Receiving Data*/
//                    lcd_clear();
//                    lcd_home();
//                    printf("Receiving Data");
//                    send = false;
//                    detect = false;
//                    deploy = true;
//                    in_cnt = 0;
//                    loopTime = 0;//timer interrupt counter
//                    stateCode=4;
//                    continue;
//                }
//                if (deploy) {
//                    if (in_cnt == 0) {
//                        dist[in_cnt] = data;
//                    }
//                    else if(in_cnt < 3){
//                        dist[in_cnt] = data-1;
//                    }
//                    else if (data=='E' && in_cnt>=3) {
//                        stateCode=5;
//                        cntrDist = (dist[0] | (dist[1] << 8));
//                        totCntrDist[poleCnt]=cntrDist;
//                        oldTire = dist[2];
//                        totOldTire[poleCnt]=oldTire;
//                        lcd_clear();
//                        lcd_home();
//                        printf("Pole Dist:%d", cntrDist);
//                        lcd_set_ddram_addr(LCD_LINE2_ADDR);
//                        printf("Tire Num:%d", oldTire);
//                        supplyTire=tireCheck(oldTire);//fuc calculate # of tires to supply
//                        totSupplyTire[poleCnt]=supplyTire;
//                        totTire+=supplyTire;
//                        stateCode=6;
//                        lcd_set_ddram_addr(LCD_LINE3_ADDR);
//                        printf("Dist:%d;Supply:%d", pulse1, supplyTire);
//                        deployTireFin(cntrDist-extOffset,supplyTire);//fuc deploy # of tires from a selected pole
//                        totStateCode[poleCnt]=stateCode;
//                        poleCnt++;
//                        in_cnt = 0;
//                        send=true;
//                        detect=false;
//                        deploy=false;
//                        continue;
//                    }
//                    in_cnt++;
//                }
//            }
//            if(returning){
//                break;
//            }
//        }
//    }
//    lcd_clear();
//    lcd_home();
//    printf("Run Terminated");
//    returnRun();
//    TMR1ON = 0;//stop timer
//    moving=true;
//    machine_state=doneRunning_state;
//    return;
//}
