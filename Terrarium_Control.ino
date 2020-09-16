#include <ezTime.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DS3231.h>

// Metro - Version: Latest
#include <Metro.h>
// *******************************  All these will need to be changed for the esp8266 pins ********************************
#define light1Pin 11
#define light2Pin 12
#define nightLightPin 6
#define topFanPin 10                //Fan to remove condensation in the front glass
#define pumpPin 7
#define fogPin 8                    //fogger relay
#define fanPin 9                    //internal fan that goes along with fogger
#define ledPin 13
// *************************************************************************************************************************
/*
#define pumpTime 10000           //Number of seconds to spray
#define fogInterval 3600000        //Number of hours between each fogging
#define fogTime 1800000            //Number of seconds to let the fogger run 
#define timeCheck 30              // interval to check the time
#define nightInterval 43200000‬
#define topFanInterval 45   // Interval for top ventilation fan
*/
 //Zoomed Light = Light   Sunblaster Light = light2
 //NightLight is the Zoomed blue Leds
int light1_2OnHr =8;   
int light1OnMin =0;
int light1OffHr= 21;
int light1OffMin=0;
int light2OnMin =5;
int light2OffHr= 20;
int light2OffMin=55;

int nightLightOffHr = 23;
int nightLightOffMin = 0;


Metro pumpMetro ;
Metro fogMetro ;
//Metro fanMetro ;
Metro checkCurTime ;
Metro topFanMetro ;

int pumpState = LOW;
int fogState = LOW;               //No need for a fan state as it will run at the same time as the fan
int topFanState=LOW;              //Top ventilation fan       


// Init the DS3231 using the hardware interface
DS3231  rtc(4, 5);

void setup() {
  
    
  
  pinMode(light2Pin,OUTPUT);
  pinMode(nightLightPin,OUTPUT);
  pinMode(topFanPin,OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(fogPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  Serial.begin(115200);
  delay(3000); // wait for console opening
  
  Serial.println("Starting RTC");
  rtc.begin();
  
  delay(1000);
  init(rtc.getTime());
  
  
  
    rtc.setTime(7, 40, 0);
  
     //Set the time to 07:38:10 (24hr format)
}

void init(Time t){
  Serial.println("Initializing");
  Serial.println(hoursToMillis(3));
  if (t.hour >= light1_2OnHr && t.hour < light1OffHr){
    Serial.println("Turning Light1 On");
    digitalWrite(light1Pin,LOW);     // Turning Light 1 ON
  }
  if(t.hour >= light1_2OnHr && t.hour < light2OffHr){
     Serial.println("Turning Light2 On");
    digitalWrite(light2Pin,LOW);     // Turning Light 2 ON
     pumpMetro.interval(hoursToMillis(3));
     topFanMetro.interval(minutesToMillis(45));
  }
  else {
    pumpMetro.interval(hoursToMillis(11));
    
  }
  
  fogMetro.interval(hoursToMillis(1));
  checkCurTime.interval(secondsToMillis(30));
  
}
  

void loop() {
    
  if (checkCurTime.check() ==1) {
    Serial.println("timed event");
    timedEvents(rtc.getTime());
  }
   
  if (pumpMetro.check() == 1) {
    Serial.println("Pump Event");
    pump();
    
  }
  /*
  The relay I used somehow seem to be triggered by a lOW pin, and not a hHIG|h one
  somehow....
  */
  if (fogMetro.check() == 1) {
    Serial.println("Fogger event");
    fogger();
  }
  
 
  
}

void fogger() {
    if (fogState == HIGH) {
      fogState = LOW;
      Serial.println("Fogging");
      fogMetro.interval(minutesToMillis(15));
      PulseFan();
    }
    else {
      fogState = HIGH;
      Serial.println("Stopping fogger");
      fogMetro.interval(hoursToMillis(1));
      analogWrite(fanPin,0);
     
    }
    digitalWrite(fogPin,fogState);
    //digitalWrite(fanPin,fogState);  
}

void pump() {
  if (pumpState == LOW) {
      pumpState = HIGH;
      Serial.println("Pumping");
      pumpMetro.interval(secondsToMillis(10));
    }
    else {
      pumpState=LOW;
      Serial.println("Stopping pump");
      pumpMetro.interval(hoursToMillis(3));
    }
   digitalWrite(pumpPin,pumpState);  
   digitalWrite(ledPin,pumpState);
}

void TopFan() {
  if (topFanState==LOW) {
    topFanState=HIGH;
    Serial.println("Starting Top Fan");
    topFanMetro.interval(minutesToMillis(10));
  }
  else {
  topFanState=LOW;
   Serial.println("Stoping Top Fan");
  topFanMetro.interval(minutesToMillis(45));
  }
  digitalWrite(topFanPin,topFanState);
  
}

void timedEvents(Time t) {
  Serial.print("current Time : ");
  Serial.print(t.hour);
  Serial.print(":");
  Serial.print(t.min);
  Serial.print(":");
  Serial.print(t.sec);
  Serial.println("");
  
 if (t.hour==light1_2OnHr && t.min==light1OnMin) {
     Serial.println("Turn Light1 ON");
     digitalWrite(light1Pin,HIGH);
     TopFan();
  }
 else if(t.hour==light1_2OnHr && t.min==light2OnMin) {
   Serial.println("Turn Light2 ON");
   digitalWrite(light2Pin,HIGH);
   pumpMetro.interval(hoursToMillis(1));
   
 }
  else if (t.hour==light1OffHr && t.min==light1OffMin) {
     Serial.println("Turn Light1 Off & Night Light On");
     digitalWrite(light1Pin,LOW);
     digitalWrite(nightLightPin,HIGH);  // when the last day light turns off, turn on night light 
     topFanMetro.interval(hoursToMillis(11));
     pumpMetro.interval(hoursToMillis(11));  // stops pumpEvents for the night
  }
 else if(t.hour==light2OffHr && t.min==light2OffMin) {
   Serial.println("Turn Light2 Off");
   digitalWrite(light2Pin,LOW);
 }
 else if(t.hour==nightLightOffHr && t.min==nightLightOffMin) {
   digitalWrite(nightLightPin,LOW);
 }
}

void PulseFan() {
  const int speedSteps= 5;
  static bool goingUp =true;
  static int x;
  int fanSpeed;
 
  if (goingUp == true  && x == 100) {
    goingUp = !goingUp;
    x=x-speedSteps;
  }
  else if (goingUp== true && x <100) {
    x=x+speedSteps;
  }
  else if (goingUp==false && x==0) {
    goingUp = !goingUp;
    x=x+speedSteps;
  }
  else {
    x=x-speedSteps;
  }
  Serial.println("Pulsing Fan");
  Serial.println(x);
  fanSpeed=map(x,0,100,101,200);
  
  
  analogWrite(fanPin,fanSpeed);
 
  
}

unsigned long secondsToMillis(unsigned long sec) {
 return sec*1000;
}

unsigned long minutesToMillis(unsigned long min) {
  return min*60*1000;
}

unsigned long hoursToMillis(unsigned long hrs) {
  unsigned long val=hrs*60*60*1000;
  Serial.print(hrs);
  Serial.print(" Hours to Millis = ");
  Serial.println(val);
  return val;
}
