#include <Wire.h>
#include <SharpIR.h>
/*Macros*/
/*IR sensors*/
#define ir1 A0 //Input from IR sensor detecting base
#define ir2 A2 //Input from IR sensor detecting lower tire
#define ir3 A1 //Input from IR sensor detecting higher tire
#define ir4 A3
#define led 13 //Output to LED light

bool send_to_pic = false; //True when pic is receiving data; False when pic is sending
bool read_from_ir = false; //True when machine is at running state (A)
bool robot_stop= false;
bool pole_detected = false; //True when a pole is detected (B)

volatile uint8_t signal_byte;//Data read/write via I2C
volatile uint8_t in_cnt;

int tireNum=0;
int poleDist=0;
int poleCnt=0;
float poleDistAvg;
float tireHighAvg;
float tireLowAvg;

void setup() {
  // Join I2C bus with address 7
  Wire.begin(7); 
  // Register callback functions
  Wire.onReceive(receiveEvent); // Called when this slave device receives a data transmission from master
  Wire.onRequest(requestEvent); // Called when master requests data from this slave device
  // Configure pins in order to receive signal from two ultrosonic sensors (us)
  pinMode(led, OUTPUT);
  digitalWrite(led,LOW);
  // Open serial port to PC (hardware UART)
  Serial.begin(9600);  
}

void tireCount(void){
  int tireNumLow;
  int tireNumHigh;
  poleDistAvg=0;
  tireHighAvg=0;
  tireLowAvg=0;
  int cnt=0;
  int loopCheck=0;
  while(pole_detected && !read_from_ir){
    float volt1=0.0;
    float volt2=0.0;
    float volt3=0.0;
    float volt4=0.0;
    for(int i=0;i<10;i++){
      volt1+=analogRead(ir1);
      volt2+=analogRead(ir2);
      volt3+=analogRead(ir3);
      volt4+=analogRead(ir4);
    }
    float dis1=1163.8*pow(volt1/10,-0.825);
    float dis2=1163.8*pow(volt2/10,-0.825);
    float dis3=1163.8*pow(volt3/10,-0.825);
    float dis4=1163.8*pow(volt4/10,-0.825);
    if(dis1<27){
      tireNumLow=0;
      tireNumHigh=0;
      if(abs(dis1-dis4)<2.5){
      /*Sensors are in front of pole. 
       *At this point, sensor 4 cannot be the control group. 
       *We then compare the difference between the bottom sensor and low/high sensor 
       *In this case, the result is less accurate.
       */
        if(abs(dis1-dis2)<1.7){
          tireNumLow++;
        }
        if(abs(dis1-dis3)<1.7){
          tireNumHigh++;
        }
      }
      else{
      /*sensors are not in front of pole
       *Sensor 4 can be the control group
       *Compare the difference between control sensor and low/high sensor to get tire number.
       */
        if(abs(dis2-dis4)>=2.2 && abs(dis1-dis2)<2.5){
          tireNumLow++;
        }  
        if(abs(dis3-dis4)>=2.2 && abs(dis1-dis3)<2.5){
          tireNumHigh++;
        }
      }
      poleDistAvg+=dis1;
      tireHighAvg+=tireNumHigh;
      tireLowAvg+=tireNumLow;
      cnt++;
    }
    loopCheck++;
    if(cnt>50){
      tireNum=0;
      poleDist=(poleDistAvg/cnt)*10;
      tireLowAvg=tireLowAvg/cnt;
      tireHighAvg=tireHighAvg/cnt;
      if(tireLowAvg>0.6){
        tireNum++;
      }
      if(tireHighAvg>0.6){
        tireNum++;
      }
      return;
    }
    if(loopCheck>200){
      tireNum=0;
      poleDist=(poleDistAvg/cnt)*10;
      tireLowAvg=tireLowAvg/cnt;
      tireHighAvg=tireHighAvg/cnt;
      if(tireLowAvg>0.6){
        tireNum++;
      }
      if(tireHighAvg>0.6){
        tireNum++;
      }
      return;
    }
    delay(5);
  }
}

bool poleCheck(void){
  poleDistAvg=0;
  int cnt=0;
  while(cnt<15){
//    float dis1=SharpIR1.distance();
    float volt1=0.0;
    for(int i=0;i<10;i++){
      volt1+=analogRead(ir1);
    }
    float dis1=1163.8*pow(volt1/10,-0.825);
    poleDistAvg+=dis1;
    cnt++;
    delay(5);
  }
  poleDistAvg=(poleDistAvg/cnt);
  if((poleDistAvg)<27){
    return true;
  }
  else{
    return false;
  }
}

void ledBlink(int tireNum){
  if(tireNum==0){
    return;
  }
  else if(tireNum==1){
    delay(500);
    digitalWrite(led,HIGH);
    delay(500);
    digitalWrite(led,LOW);
  }else if(tireNum==2){
    delay(500);
    digitalWrite(led,HIGH);
    delay(500);
    digitalWrite(led,LOW);
    delay(500);
    digitalWrite(led,HIGH);
    delay(500);
    digitalWrite(led,LOW);
  }
  else{
    for(int i=0;i<10;i++){
      delay(200);
      digitalWrite(led,HIGH);
      delay(200);
      digitalWrite(led,LOW);
    }
  }
}

void loop() {
  if (send_to_pic && read_from_ir){
    float volt1=0.0;
    for(int i=0;i<10;i++){
      volt1+=analogRead(ir1);
    }
    float dis1=1163.8*pow(volt1/10,-0.825);
    if((dis1<27) && !pole_detected){//a pole is detected, feedback to pic; go to B
      bool pole_checked=poleCheck();
      if(pole_checked){
        Serial.print("Pole detected:\n");
        digitalWrite(led,HIGH);
        signal_byte='B';
        in_cnt=0;
        read_from_ir=false;//stop reading from ir
      }
    }
  }
  else if(send_to_pic && robot_stop){//after machine stopped, ready to 
    Serial.print("Machine Stopped:\n");
    signal_byte='D';
    robot_stop=false;
  }
  else if(send_to_pic && pole_detected){
      /* After the machine stops, start counting the tires*/
      //Nano send distance to pic. 3 byte data.
      Serial.print("Start Tire Counting\n");
      tireCount();
      Serial.print("Finish Tire Counting\n");
      Serial.println(tireHighAvg);
      Serial.println(tireLowAvg);
      if(in_cnt==0){//first byte: dist low from center line
        signal_byte=poleDist;
        Serial.print("pole_dist_low:");
        Serial.println(poleDist);
      }
      else if(in_cnt==1){//second byte: dist high from center line
        signal_byte=(poleDist>>8)+1;
        Serial.print("pole_dist_high:");
        Serial.println(signal_byte-1);
      }
      else if(in_cnt==2){
        signal_byte=(tireNum)+1;
        Serial.print("tire_num:");
        Serial.println(signal_byte-1);        
      }
      else if(in_cnt==3){
        signal_byte='E';
        Serial.print("End of Sending");
        Serial.println(poleCnt);
        poleCnt++; 
      }
      else if(in_cnt>3){
        pole_detected=false;//clear flag
      }
      in_cnt++;
      delay(10);
    }
}

void receiveEvent(void){
  uint8_t x = Wire.read(); // Receive byte
  Serial.println((char)x); // Print to serial output as char (ASCII representation)
  if(x=='A'){
    send_to_pic=true;
    read_from_ir=true;
    robot_stop= false;
    pole_detected = false;
    Serial.print("Start running\n");//switch to state A
  }
  else if(x=='S'){
    send_to_pic=true;
    robot_stop=true;
    Serial.print("Robot stopped\n");//switch to state A
  }
}

void requestEvent(void){// Respond with message of 1 byte
  Wire.write(signal_byte);
//  Serial.println((char)signal_byte); // Print to serial output as char (ASCII representation)
  if(signal_byte=='B'){
    send_to_pic=false;
  }
  else if(signal_byte=='D'){
    //Nano send distance to pic. 1+3+1 byte data.
    pole_detected=true;//a pole is detected
    in_cnt=0;
  }
  else if(signal_byte=='E'){//end of sending
    send_to_pic=false;
    pole_detected=false;
    in_cnt=0;
    digitalWrite(led,LOW);
//    ledBlink(tireNum);
//    digitalWrite(led,LOW);
  }
  signal_byte = 0; // Clear output buffer
}
