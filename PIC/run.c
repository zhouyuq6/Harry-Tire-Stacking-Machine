/*
 * File:   subsys.c
 * Author: Yuqian
 * Version History:
 */

/* Terms:
 * 1.temp.:temporary; will be discarded after a function ends
 * 2.short-term: is initialized since reset
 * 3.long-term: is written into eeprom
 * 4.Tubes: Tube 1 is at the front and tube 2 represents the rare motor. 
 *  Each has two pins step and direction. 
 *  Direction should be kept 0/1 => forward/backward
 *  Step should alter b/w 0 and 1 to drive the motor
 *  See Trigger_Pulse() for details
 * 5.Wheel ID: 1 is left and 2 is right
 *  Select high/1 represents forward; low/0 represents backwards
 * 
 * Procedule: 
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
 * 9.Repeat 1-8 until reach an end condition
 * 10.Terminate the run
 */

/*Includes*/
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
/*Main Function*/
void run(void) {
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
    tube1_tire = tube1Tire; //Initial number of tires stored in tube 1
    tube2_tire = tube2Tire; //Initial number of tires stored in tube 2
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
                set_pwm_duty_cycle(startPWM, 1); //Start DC motors right 
                set_pwm_duty_cycle(startPWM, 2);//left
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
                    totStartDistCm[poleCnt] = (int) (slop * pulse2 + intercept);
                    if(poleCnt==0){
                        totStartDistCm[poleCnt] = (int) (slop * pulse2 + intercept-6);
                    }
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
            if (pulse2 > endDist) {//distance of 4m
                returning = true;
                moving=false;
                break;
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

void returnRun(void) {
    int returnDist;
    returnDist=pulse2;
    slowStop(startPWM);
    LATBbits.LATB3 = 0;
    __delay_ms(50);
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    LATBbits.LATB3 = 0; //turning back
    pulse1 = 0;
    pulse2 = 0;
    set_pwm_duty_cycle(returnPWM+75, 1);
    set_pwm_duty_cycle(returnPWM, 2);//turn towards left
    moving = true;
    while ((pulse2 <= 50) && moving) {
        continue;
    }
    set_pwm_duty_cycle(returnPWM, 1);//turn back towards straight
    set_pwm_duty_cycle(returnPWM+60, 2);
    moving = true;
    while ((pulse2 <= 100) && moving) {
        continue;
    }
    set_pwm_duty_cycle(returnPWM, 1);
    set_pwm_duty_cycle(returnPWM, 2);
    moving = true;
    while ((pulse2 <= (returnDist+50)) && moving) {
        continue;
    }
    slowStop(returnPWM);
    LATBbits.LATB3 = 1;//fast stop
    __delay_ms(50);
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    return;
}

void initPWM(void){
    /* This is the function to initialize CCPs to generate PWM output
     * For details of CCP registers, 
     * check Chp.16 in the PIC18F4620 datasheet about the ECCP modules
     */
    
    //Don't need A2D here so all digital GPIO
    ADCON1 = 0b00001111;
    /*Interrupt Configuration for keypad interrupts.
     *This is repeated since these functions are copied from 
     *an individual project for DC motors.
     *I think its fine to keep it, 
     *because we will need to use keypad interrupts afterwards anyway
     */
    INT1IE = 1;
    ei(); 
    // Disable output from PWM pin while we are setting up PWM
    TRISCbits.TRISC1 = 1;
    TRISCbits.TRISC2 = 1;
    /* Configure PWM frequency
     * I learned this part from a tutorial online:https://bit.ly/2OEh8dq
     * Few changes were made because CCP module is different in PIC18F series
     */
    PR2 = (_XTAL_FREQ/(PWM_freq*4*TMR2PRESCALE)) - 1;
    
    // Configure CCP1CON, single output mode, all active high
    CCP1M3 = 1;
    CCP1M2 = 1;
    CCP1M1 = 0;
    CCP1M0 = 0;
    //Similar for CCP2CON
    CCP2M3 = 1;
    CCP2M2 = 1;
    CCP2M1 = 0;
    CCP2M0 = 0;

    // Set timer 2 prescaler to 4
    T2CKPS0 = 0;
    T2CKPS1 = 1;

    // Enable timer 2
    TMR2ON = 1;

    // Enable PWM output pin since setup is done
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
    //Enable LOW and SELECT
    TRISDbits.TRISD0 = 0;//low
    LATDbits.LATD0 = 0;
    TRISBbits.TRISB3 = 0;//select
    LATBbits.LATB3 = 1;//start with 1 going forward
}

void initTimer(void){
    T1CKPS1 = 1; // bits 5-4  Prescaler Rate Select bits
    T1CKPS0 = 0; // bit 4
    T1OSCEN = 0; // bit 3 Timer1 Oscillator Enable Control bit 1 = on
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
}

void set_pwm_duty_cycle(unsigned int duty,int wheel_id){
    /* This is the function writing duty cycle
     * Write a duty cycle value out of 1023 in duty.
     * If duty < 200, the cycle is too small for DC brush motor. It will stop.
     */
    if (duty<1023){
        duty = ((float)duty/1023)*(_PWM_FREQ/(PWM_freq*TMR2PRESCALE));       
            // Save the duty cycle into the registers
        if(wheel_id==1){//wheel_id = 1:  left wheel
            CCP2X = duty & 2; // Set the 2 least significant bit in CCP1CON register
            CCP2Y = duty & 1;
            CCPR2L = duty >> 2; // Set rest of the duty cycle bits in CCPR1L
        }
        else if(wheel_id==2){
            CCP1X = duty & 2; // Set the 2 least significant bit in CCP1CON register
            CCP1Y = duty & 1;
            CCPR1L = duty >> 2; // Set rest of the duty cycle bits in CCPR1L
        }
    }
}

void tube_stepper_control(int tubeID, int tire_num){
    /* This is the function control stepper motors to push tires
     * tubeID = 1 front; tubeID = 2 rare.
     */
    unsigned int x=40;
    int index=0;
    int i=0;
    int stepper_para;
    stepper_para=tire_num*135;
    /* The stepper motor need to accelerate to a desired speed
     * index and i are for the acceleration
     * acceleration parameters refer to https://github.com/hs105/nema17-A4988-high-rpm-speed
     * 8 mm = 360 deg= 1.8 deg *5*40.
     * Horizontal distance (mm)= (1.8 * 5) * stepper_para.
     * for dist = 27 mm = tire width, stepper_para=135.
     * for dist count at 1 mm, stepper_para=5.
     */
    if(tubeID==1){
        //Initialize again. Without these, the motors inputs could be floating
        TRISAbits.RA0 = 0;//step
        LATAbits.LATA0 = 0;
        TRISAbits.RA1 = 0;//direction
        LATAbits.LATA1 = 0;//low
      
        for(index=0;index<stepper_para;index++){
            for(i=0;i<5;i++){
                Trigger_Pulse(x,1);
            }
            if (x>15){
                x--;
            }
        }
         tube1_tire=tube1_tire-tire_num;
    }
    else if (tubeID==2){
        TRISEbits.RE0 = 0;//step
        LATEbits.LATE0 = 0;
        TRISCbits.RC5 = 0;//direction
        LATCbits.LATC5 = 0;//low
        for(index=0;index<stepper_para;index++){
            for(i=0;i<5;i++){
                Trigger_Pulse(x,2);
            }
            if (x>15){
                x--;
            }
        }
        tube2_tire=tube2_tire-tire_num;
    }
    LATAbits.LATA0 = 0;
    LATAbits.LATA1 = 0;
    LATEbits.LATE0 = 0;
    LATCbits.LATC5 = 0;//low
    __delay_ms(10);
    return;
}

void ext_stepper_control(int ext_dist,bool extend){//ext_dist in mm
    /* This is the function control stepper motors to extent. Same story.*/
    unsigned int x=40;
    int index=0;
    int i=0;
    int stepper_para;
    stepper_para=ext_dist*5;

    TRISAbits.RA4 = 0;//step
    LATAbits.LATA4 = 0;
    TRISAbits.RA5 = 0;//direction
    LATAbits.LATA5 = extend;//extend: low; retract:high
    
    for(index=0;index<stepper_para;index++){
        for(i=0;i<5;i++){
            Trigger_Pulse(x,3);
        }
        if (x>15){
            x--;
        }
    }
    __delay_ms(10);
    LATAbits.LATA5 = 0; //low
    LATAbits.LATA4 = 0; //low
    return;
    
}

void Trigger_Pulse(unsigned int x, int stepper_id){
    /*This is the function that continuously writing high and low to step pin
     *with a pulse of every 100 microsecond. 
     *check the NEMA17 datasheet*/
    int cnt=0;
    if(stepper_id==1){//tube 1
        LATAbits.LATA0 = 1;
        for(cnt=0;cnt<x;cnt++){
            __delay_us(100);
        }
        LATAbits.LATA0 = 0;
        return;
    }
    else if(stepper_id==2){//tube 2
        LATEbits.LATE0 = 1;
        for(cnt=0;cnt<x;cnt++){
            __delay_us(100);
        }
        LATEbits.LATE0 = 0;
        return;
    }
    else if(stepper_id==3) {//extension
        LATAbits.LATA4 = 1;
        for(cnt=0;cnt<x;cnt++){
            __delay_us(100);
        }
        LATAbits.LATA4 = 0;
        return;
    }
    else{return;}
}

void slowStop(int currPWM){
    pwmVal1=currPWM;
    pwmVal2=currPWM;
    while(pwmVal1>=100 && pwmVal2>=100){
        pwmVal1-=20;
        pwmVal2-=20;
        set_pwm_duty_cycle(pwmVal1, 1);
        set_pwm_duty_cycle(pwmVal2, 2);
        __delay_ms(20);
    }
    set_pwm_duty_cycle(0, 1);
    set_pwm_duty_cycle(0, 2);
    return;
}

int tireCheck(int oldTire){
    int supplyTire=0;
    int newTire=0;
    int pulseCnt=0;
    pulseCnt=pulse2-newPulse2;
    if(poleCnt==0){//first pole
        newTire=2;
        newPulse2 = pulse2;
        shortDist = false;
    }
    else if(pulseCnt>upperBound){//300 mm
        newTire=2;
        newPulse2=pulse2;
        shortDist=false;
    }
    else if(pulseCnt>lowerBound){//200mm
        newTire=1;
        newPulse2=pulse2;
        shortDist=true;
    }
    else{
        newTire=0;
        poleCnt--;
    }
    if(newTire<=oldTire){//enough tire stacked,keep going
        supplyTire=0;
        totNewTire[poleCnt]=oldTire;
    }
    else{
        supplyTire=newTire-oldTire;
        totNewTire[poleCnt]=newTire;
    }
    lcd_set_ddram_addr(LCD_LINE3_ADDR);
    printf("Dist:%d;Supply:%d", pulseCnt, supplyTire);
    return supplyTire;
}

void deployTireFin(int extDist, int supplyTire) {
    alignDist=0;
    pulseTemp=0;
    if(supplyTire<=0){//no need to deploy
        noDeploy=true;
        return;
    }
    
    if(supplyTire<=tube1_tire){
        tubeID=1;
        alignDist=alignDistLow;  
    }
    else if(supplyTire<=tube2_tire){
        tubeID=2;
//        extDist = extDist - extOffset2;
        alignDist=alignDistHigh;
    }
    else{
        returning=true;
        return;
    }
//    pulseTemp=pulse2;
//    secondStop=true;
//    if(tubeID==1){
//        __delay_ms(900);
//        LATBbits.LATB3 = 0;
//        __delay_ms(50);
//        set_pwm_duty_cycle(0, 1);
//        set_pwm_duty_cycle(0, 2);
//        LATBbits.LATB3 = 1;
//    }
    if(tubeID==2){
        pulseTemp=pulse2;
        while((pulse2-pulseTemp)<alighDistDiff && moving){
            set_pwm_duty_cycle(stopPWM, 1);
            set_pwm_duty_cycle(stopPWM, 2);
            __delay_ms(200);
            set_pwm_duty_cycle(0, 1);
            set_pwm_duty_cycle(0, 2);
            __delay_ms(100);
        }
        LATBbits.LATB3 = 0;
        __delay_ms(50);
        set_pwm_duty_cycle(0, 1);
        set_pwm_duty_cycle(0, 2);
        LATBbits.LATB3 = 1;
    }
    stateCode=7;
//    while (secondStop && moving) {
//        continue;
//    }
//    __delay_ms(800);
//    LATBbits.LATB3 = 0;
//    __delay_ms(100);
//    set_pwm_duty_cycle(0, 1);
//    set_pwm_duty_cycle(0, 2);
//    LATBbits.LATB3 = 1;
    if(extDist>extMax || extDist<extMin){
        lcd_set_ddram_addr(LCD_LINE4_ADDR);
        printf("Out of Scope");
        __delay_ms(1000);
        return;
    }
    ext_stepper_control(extDist, 0);
    tube_stepper_control(tubeID,supplyTire);
    __delay_ms(500);
    ext_stepper_control(extDist, 1);
    stateCode=8;
    lcd_set_ddram_addr(LCD_LINE4_ADDR);
    printf("Deploy:%d",alignDist);
    __delay_ms(1000);
    return;
}