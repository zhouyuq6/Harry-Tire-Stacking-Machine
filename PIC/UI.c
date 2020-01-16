#include <xc.h>
#include <stdio.h>
#include <stdbool.h>
#include "configBits.h"
#include "I2C.h"
#include "lcd.h"
#include "UI.h"
#include "run.h"
#include "mainheader.h"

void initUI(void){
    // Print received data on LCD
    getRTC();
    lcd_home();
    printf("%02x/%02x/%02x", time[6],time[5],time[4]); // Print date in YY/MM/DD
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    printf("%02x:%02x:%02x", time[2],time[1],time[0]); // HH:MM:SS
    lcd_set_ddram_addr(LCD_LINE4_ADDR);
    printf("PRESS # TO START");
    __delay_ms(200);
}

void getRTC(void){
    I2C_Master_Start(); // Start condition
    I2C_Master_Write(0b11010000); // 7 bit RTC address + Write
    I2C_Master_Write(0x00); // Set memory pointer to seconds
    I2C_Master_Stop(); // Stop condition
    // Read current time
    I2C_Master_Start(); // Start condition
    I2C_Master_Write(0b11010001); // 7 bit RTC address + Read
    for(unsigned char i = 0; i < 6; i++){
        time[i] = I2C_Master_Read(ACK); // Read with ACK to continue reading
    }
    time[6] = I2C_Master_Read(NACK); // Final Read with NACK
    I2C_Master_Stop(); // Stop condition
}

void initTimeRTC(void){
    I2C_Master_Start(); // Start condition
    I2C_Master_Write(0b11010000); //7 bit RTC address + Write
    I2C_Master_Write(0x00); // Set memory pointer to seconds
    // Write array
    for(char i=0; i < 7; i++){
        I2C_Master_Write(resetTime[i]);
    }
    I2C_Master_Stop(); //Stop condition
}

void instructMenu(void){
    lcd_clear();
    lcd_home();
    if(machine_state==doneRunning_state){
        printf("--RUN FINISHED--");
        lcd_set_ddram_addr(LCD_LINE2_ADDR);
        printf("Report:#");
        lcd_set_ddram_addr(LCD_LINE3_ADDR);
        printf("Restart:*");
        while(!return_key_pressed){
            if(enter_key_pressed){
                enter_key_pressed=false;
                report_page=pg0;
                return;
            }
        }
        restartRoutine();
        return;
    }
    if(machine_state==logging_state){
        instruct_page_1();
    }
}

void instruct_page_1(void){
    lcd_clear();
    lcd_home();
    printf("--INSTRUCTION--");
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    printf("Page Up:A Down:B");
    lcd_set_ddram_addr(LCD_LINE3_ADDR);
    printf("To View Report:#");
    lcd_set_ddram_addr(LCD_LINE4_ADDR);
    printf("Go Back:*");
    while(1){
        if(return_key_pressed){
            return_key_pressed=false;
            return;
        }
        else if(enter_key_pressed){
            report_page=pg1;
            enter_key_pressed=false;
            return;
        }
    }
}

void report(void) {
    pageCnt = 0;
    itemCnt = 0;
    if ((poleCnt % 3) != 0) {
        pageCnt = ((poleCnt) / 3) + 1;
        itemCnt = poleCnt % 3;
    } else if (poleCnt != 0) {
        pageCnt = ((poleCnt) / 3);
        itemCnt = 3;
    }
    switch(report_page){
        case pg1:
            report_page_1();
            break;
        case pg2:
            report_page_2();
            break;
        case pg11:
            report_page_11();//Operation Time
            break;
        case pg12:
            report_page_12();//
            break;
        case pg13:
            report_page_13();
            break;
        case pg24:
            report_page_24();
            break;
        case pg25:
            report_page_25();
            break;
        default:break;
    }
    lcd_clear();
    lcd_home();
}

void report_page_1(void){
    lcd_clear();
    lcd_home();
//    printf("RUN %02x/%02x %02x:%02x", runClk[5],runClk[4],runClk[2],runClk[1]);
    printf("-OPERATION INFO-");
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    printf("1.Operation Time");
    lcd_set_ddram_addr(LCD_LINE3_ADDR);
    printf("2.Old Tires/Pole");
    lcd_set_ddram_addr(LCD_LINE4_ADDR);
    printf("Back:*|%c:A|%c:B",upArrow,downArrow);
    while(1){
        if(down_key_pressed){
            down_key_pressed=false;
            return;}
        if(return_key_pressed){
            return_key_pressed=false;
            return;
        }
        if(sub_key_pressed){
            sub_key_pressed=false;
            return;
        }
    }
}

void report_page_2(void){
    lcd_clear();
    lcd_home();
//    printf("RUN %02x/%02x %02x:%02x", runClk[5],runClk[4],runClk[2],runClk[1]);
    printf("3.New Tires/Pole");
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    printf("4.Supplied Tires");
    lcd_set_ddram_addr(LCD_LINE3_ADDR);
    printf("5.Startline Dist");
    lcd_set_ddram_addr(LCD_LINE4_ADDR);
    printf("Back:*|%c:A|%c:B", upArrow, downArrow);
    while(1){
        if(up_key_pressed){
            up_key_pressed=false;
            return;
        }
        if(return_key_pressed){
            return_key_pressed=false;
            return;
        }
        if(sub_key_pressed){
            sub_key_pressed=false;
            return;
        }
    }
}

void report_page_11(void){
    lcd_clear();
    lcd_home();
    printf("-Operation Time-");
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    printf("%d Second",runTime/10);
    while(!return_key_pressed){
        __delay_ms(200);
    }
    return_key_pressed=false;
    return;
}

void report_page_12(void){//
    int i = 0;
    int j = 0;
    readKey=true;
    while(!return_key_pressed){
        if((i+1)>=pageCnt && readKey) {
            lcd_clear();
            lcd_home();
            printf("-Old Tires/Pole-");
            i = pageCnt - 1;
            for(j=0;j<itemCnt;j++){//starting from the second line
                newLine(j);
                printf("Pole#%d:%d",(i*3+j+1),totOldTire[(i*3+j)]);
            }
            readKey=false;
        }
        else if(i<=0 && readKey){
            lcd_clear();
            lcd_home();
            printf("-Old Tires/Pole-");
            i=0;
            for(j=0;j<3;j++){//starting from the second line
                newLine(j);
                printf("Pole#%d:%d",(i*3+j+1),totOldTire[(i*3+j)]);
            }
            readKey=false;
        }
        else if((i+1)<pageCnt && (readKey)){
            lcd_clear();
            lcd_home();
            printf("-Old Tires/Pole-");
            for(j=0;j<3;j++){//starting from the second line to the last line
                newLine(j);
                printf("Pole#%d:%d",(i*3+j+1),totOldTire[(i*3+j)]);
            }
            readKey=false;
        }
        if(up_key_pressed){
            i--;
            up_key_pressed=false;
            readKey=true;
        }
        else if(down_key_pressed){
            i++;
            down_key_pressed=false;
            readKey=true;
        }
        __delay_ms(200);
    }
    return;
}

void report_page_13(void) {
    int i = 0;
    int j = 0;
    readKey=true;
    while(!return_key_pressed){
        if((i+1)>=pageCnt && readKey) {
            lcd_clear();
            lcd_home();
            printf("-New Tires/Pole-");
            i = pageCnt - 1;
            for(j=0;j<itemCnt;j++){//starting from the second line
                newLine(j);
                printf("Pole#%d:%d",(i*3+j+1),totNewTire[(i*3+j)]);
            }
            readKey=false;
        }
        else if(i<=0 && readKey){
            lcd_clear();
            lcd_home();
            printf("-New Tires/Pole-");
            i=0;
            for(j=0;j<3;j++){//starting from the second line
                newLine(j);
                printf("Pole#%d:%d",(i*3+j+1),totNewTire[(i*3+j)]);
            }
            readKey=false;
        }
        else if((i+1)<pageCnt && (readKey)){
            lcd_clear();
            lcd_home();
            printf("-New Tires/Pole-");
            for(j=0;j<3;j++){//starting from the second line to the last line
                newLine(j);
                printf("Pole#%d:%d",(i*3+j+1),totNewTire[(i*3+j)]);
            }
            readKey=false;
        }
        if(up_key_pressed){
            i--;
            up_key_pressed=false;
            readKey=true;
        }
        else if(down_key_pressed){
            i++;
            down_key_pressed=false;
            readKey=true;
        }
        __delay_ms(200);
    }
    return;
}

void report_page_24(void){
    lcd_clear();
    lcd_home();
    printf("-Supplied Tires-");
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    if(totTire>15){
        totTire=15;
    }
    printf("%d",totTire);
    while(!return_key_pressed){
        __delay_ms(200);
    }
    return_key_pressed=false;
    return;
}

void report_page_25(void) {
    int i = 0;
    int j = 0;
    readKey = true;
    while (!return_key_pressed) {
        if ((i + 1) >= pageCnt && readKey) {
            lcd_clear();
            lcd_home();
            printf("-Start Dist(cm)-");
            i = pageCnt - 1;
            for (j = 0; j < itemCnt; j++) {//starting from the second line
                newLine(j);
                printf("Pole#%d:%d;%d", (i * 3 + j + 1), totStartDist[(i * 3 + j)],totStartDistCm[(i * 3 + j)]);
            }
            readKey = false;
        } else if (i <= 0 && readKey) {
            lcd_clear();
            lcd_home();
            printf("-Start Dist(cm)-");
            i = 0;
            for (j = 0; j < 3; j++) {//starting from the second line
                newLine(j);
                printf("Pole#%d:%d;%d", (i * 3 + j + 1), totStartDist[(i * 3 + j)],totStartDistCm[(i * 3 + j)]);
            }
            readKey = false;
        } else if ((i + 1) < pageCnt && (readKey)) {
            lcd_clear();
            lcd_home();
            printf("-Start Dist(cm)-");
            for (j = 0; j < 3; j++) {//starting from the second line to the last line
                newLine(j);
                printf("Pole#%d:%d;%d", (i * 3 + j + 1), totStartDist[(i * 3 + j)],totStartDistCm[(i * 3 + j)]);
            }
            readKey = false;
        }
        if (up_key_pressed) {
            i--;
            up_key_pressed = false;
            readKey = true;
        } else if (down_key_pressed) {
            i++;
            down_key_pressed = false;
            readKey = true;
        }
        __delay_ms(200);
    }
    return;
}
