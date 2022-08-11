
#include <SHT31.h>
#include <ezTime.h>
#include <ESP8266WebServer.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Logo.h"
#include <Metro.h>

String appVers ="Version 1.7.8";
// *******************************  All these will need to be changed for the esp8266 pins ********************************
#define light1Pin D0                    //Relay 1 
#define light2Pin D3                    //Relay 4    
//#define nightLightPin D5                 //Relay 5  Design issue on V1.0 of pcb prevents night lights :-(
//#define topFanPin D8                     //Fan to remove condensation in the front glass
#define topFanPin D5                     //Fan to remove condensation in the front glass
#define pumpPin D7                       //Relay 5 (RX pin on NodeMCU)
#define roPin 3                         //Reverse Osmosis relay
#define fullROpin D6                     //Switch 1 - P6
#define tempPin D4                      //D5   DS18B20 Temperature sensor

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define timeCheckInterval 45    // Number of seconds for each time checks
#define displayRefreshInterval 5
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ESP8266WebServer server(80);

//#define ledPin 13
// *************************************************************************************************************************
/*
  #define pumpTime 10000           //Number of seconds to spray
  #define fogInterval 3600000        //Number of hours between each fogging
  #define fogTime 1800000            //Number of seconds to let the fogger run
  #define timeCheck 30              // interval to check the time
  #define nightInterval 43200000‬
  #define topFanInterval 45   // Interval for top ventilation fan
*/
//**********************  Temperature Sensor variables  ***************
SHT31 sht;
//**********************  ezTime stuff ***********************
Timezone cda;
//*******************  Wi-Fi info ************************
char ssid[] = "Whipet 2.4";
char password[] = "pastelbird150";

//***********************  Timed events schedule  ************************
//Zoomed Light = Light   Sunblaster Light = light2
//NightLight is the Zoomed blue Leds
String light1On = "08:00";
String light1Off = "20:55";
String light2On = "08:10";
String light2Off = "21:00";
String nightLightOn = "21:00";
String nightLightOff = "23:45";
uint8_t pumpFreq = 180;         //Pump frequency in minutes
uint8_t pumpTime = 10;          //Number of seconds to run the pump
uint8_t fanTime = 10;           //Number of minutes to run the fan
uint8_t fanFreq = 45;           //Fan frequency in minutes
//char token[] = "vRYJMQpRDKhEWmpShmRxQ0jHGX1kzGGz";
// ****************  recurring events ****************
Metro sprayMetro ;         //
Metro displayMetro ;
//Metro fanMetro ;
Metro checkCurTime ;      // Event to check the current time and act on any timed event
Metro topFanMetro ;       // Top fan to remove water from front glass
Metro manualFillMetro;    //RO water manual fill
//****************************************************
struct relayState_t {
  int pump;
  int topFan;
  int light1;
  int light2;
  int ro;
} relayState;
/*
int pumpState = LOW;
//int fogState = LOW;               //No need for a fan state as it will run at the same time as the fan
int topFanState = LOW;            //Top ventilation fan
int light1RelayState = LOW;
int light2RelayState = LOW;
int nightLightRelayState = LOW;
int roRelayState = LOW;
*/
uint8_t manfillTime = 20;   //Nuber of minutes to manually fill RO bottles
uint8_t fullRO;                //Level of RO supply
bool manFilling = false;

// 'images', 128x64px



void setup() {
    
  Serial.begin(115200);
  while (!Serial);

   //************************  Set Pins  **************************
  pinMode(light1Pin, OUTPUT);
  pinMode(light2Pin, OUTPUT);
 // pinMode(nightLightPin, OUTPUT);
  pinMode(topFanPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(roPin, OUTPUT);
  pinMode(fullROpin, INPUT_PULLUP);

  digitalWrite(light1Pin, LOW);
  digitalWrite(light2Pin, LOW);
  //digitalWrite(nightLightPin, LOW);
  digitalWrite(topFanPin, LOW);
  digitalWrite(pumpPin, LOW);
  digitalWrite(roPin, LOW);
  //************************ Set Display ***************************   
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.clearDisplay();
  display.display(); 
  splashScreen();

  delay(3000); // wait for console opening

 
  //************************ Set recurring events  ********************
  sprayMetro.interval(hoursToMillis(20));        //Set to a ridiculously long delay . Startup and Timed events will take care of the rest
  topFanMetro.interval(hoursToMillis(20));      //Set to a ridiculously long delay . Startup and Timed events will take care of the rest
  checkCurTime.interval(secondsToMillis(timeCheckInterval));   //Check current time for any sceduled events
  manualFillMetro.interval(hoursToMillis(20));        //Set to a ridiculously long delay .
  displayMetro.interval(secondsToMillis(displayRefreshInterval));
  display.clearDisplay();
  display.setCursor(0,0);

  display.display();
  start_Wifi();
  Serial.println("Connecting to ");
  display.print("Connecting to \n");
  Serial.println(ssid);
  display.println(ssid);
  display.display();
  
  
  waitForSync();
  display.clearDisplay();
  display.setCursor(0,0);
  Serial.println("");
  Serial.println("WiFi connected..!\n");
  
  display.println("WiFi connected..!");
  Serial.print("IP: ");  
  Serial.println(WiFi.localIP());
  display.print("IP: ");  
  display.println(WiFi.localIP());
  display.display();
  delay(3000);

  Wire.begin();
  sht.begin(0x44);


  cda.setPosix("EST5EDT,M3.2.0,M11.1.0");     //Sets timezone to GMT+5, aka here
  Serial.println(cda.dateTime());             //Prints datetime
  Serial.println(cda.dateTime("H:i"));        //Prints Time in HH:MM format . This is how we will compare timed events
  startup();
  // ******************************  Initialize Web Server and web events ******************************
  server.on("/", handle_OnConnect);
  server.on("/light1on", handle_light1on);
  server.on("/light1off", handle_light1off);
  server.on("/light2on", handle_light2on);
  server.on("/light2off", handle_light2off);
 // server.on("/nightlighton",handle_nightlighton);
 // server.on("/nightlightoff",handle_nightlightoff);
  server.on("/FillRO", handle_FillRO);
  server.on("/SetTimes",handle_SetTimes);
  server.on("/settings",handle_settings);
  server.on("/pumpOn",handle_pumpOn);
  server.on("/pumpOff", handle_pumpOff);
  server.on("/topfan", handle_topFan);
  server.begin();
  Serial.println("HTTP server started");

  
  manFilling = false;
}

void startup() {
  // This sub is called once during setup, to initialize anything based on the schedule after power up
  Serial.println("Initializing after power On");
  /*
    Serial.print("CDA Hour:");
    Serial.print(cda.hour());
    Serial.print("CDA Minute:");
    Serial.print(cda.minute());
    Serial.print("     ");
    Serial.print("Light1Hour:");
    Serial.print(light1On.substring(0, 2));
    Serial.print("   ");
    Serial.print("Light1 Minute:");
    Serial.println(light1On.substring(3, 5));
  */
  if (cda.hour() >= light1On.substring(0, 2).toInt() && cda.hour() < light1Off.substring(0, 2).toInt()) {

    Serial.println("Turning Light1 On");
    digitalWrite(light1Pin, HIGH);    // Turning Light 1 ON
    relayState.light1 = HIGH;

  }
  if ((cda.hour() >= light2On.substring(0, 2).toInt()) && (cda.hour() < light2Off.substring(0, 2).toInt())) {

    Serial.println("Turning Light2 On");
    digitalWrite(light2Pin, HIGH);    // Turning Light 2 ON
    relayState.light2 = HIGH;
    spray();     //Lights are on, start the misting sequence
    topFan();  //Starts top fan


  }
 /*
  if ((cda.hour() >= nightLightOn.substring(0, 2).toInt()) && (cda.hour() <= nightLightOff.substring(0, 2).toInt())) {

    Serial.println("Turning Night Light On");
  //  digitalWrite(nightLightPin, HIGH);    // Turning night Light  ON
    nightLightRelayState=HIGH;

  }
  */
  else {
    //Nothing for now

  }


}

void splashScreen() {
  

 display.drawBitmap(0, 0,  koala, 128, 64, WHITE);
 display.display();
 delay(3000);
 display.clearDisplay();
 display.setCursor(0,0);
 display.println("Terrarium Control");
 display.println(appVers);
 display.println();
 display.println("Koala In The Rain ");
 display.println("Corporation.");
 display.println("No Fucks Given....");

 display.display();
 
 delay(3000);
}

void loop() {
  static String lastTime;         //Prevents calling the same Timed event twice
  
  if (displayMetro.check() ==1 ) {
     updateDisplay("");       //Update LCD display with alternating info
  }
  
  if ((checkCurTime.check() == 1) && (cda.dateTime("H:i")!=lastTime)) {
    lastTime=cda.dateTime("H:i");
    Serial.println("timed event");
    timedEvents(cda.dateTime("H:i"));
    
  }

  if (sprayMetro.check() == 1) {
    Serial.println("Pump Event");
    spray();
  }

  if (topFanMetro.check() == 1) {
    Serial.println("Top Fan Event");
    topFan();
  }

  if ((manFilling == true) && (manualFillMetro.check() == 1) && (relayState.ro == HIGH)) {
    Serial.println("Manual Filling Timed event");
    stopRO();
    manFilling=false;
  }
  else if (manFilling == false) {
    //Serial.println("Checking RO");
    checkRO();
  }
  sht.read();
  checkhttpclient();
  
}
float GetHumidity() {
    return sht.getHumidity();
}

float GetTemp() {
    return sht.getTemperature();
 }

void spray() {
  if (relayState.pump == LOW) {
    relayState.pump = HIGH;
    Serial.println("Pumping");
    sprayMetro.reset();
    sprayMetro.interval(secondsToMillis(pumpTime));
  }
  else {
    relayState.pump = LOW;
    Serial.println("Stopping pump");
    if ((relayState.light1 == LOW) && (relayState.light2 == LOW)) {
      sprayMetro.reset();
      sprayMetro.interval(hoursToMillis(23));        // Stop the pump untill the next time Light and 2 are turned on
    }
    else {
      sprayMetro.reset();
      sprayMetro.interval(minutesToMillis(pumpFreq));
    }
    
  }
  digitalWrite(pumpPin, relayState.pump);
  server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp()));    // If a Web client is connected, refresh the page to update the button state
 
}

void stopRO() {
  Serial.println("Stopping RO");
  updateDisplay("Stopping RO");
  relayState.ro = LOW;
  digitalWrite(roPin, relayState.ro);
  
}

void checkRO() {
  if (manFilling == false) {           //Only check Float switch if not manually filling
  static unsigned long t;
  fullRO = digitalRead(fullROpin);
  //Serial.print("Full RO = ");
  //Serial.println(fullRO);
  if ((relayState.ro == LOW) && (fullRO == HIGH) && (t==0)) {
    Serial.println("Not filling , Not Full");
       if (t==0){
          t=millis();
       }
    }
   else if ((relayState.ro == LOW) && (fullRO == HIGH) &&((millis()-t)>3000)) {              //Debounce switch 
    Serial.println((millis()-t));
    Serial.print("Ro Relay State :");
    Serial.println(relayState.ro);
    
    Serial.println("Starting to fill....");
    updateDisplay("Starting to fill RO");
    relayState.ro = HIGH;
    digitalWrite(roPin, relayState.ro);
    
    }
   else if ((relayState.ro == HIGH) && (fullRO == LOW)) {
    Serial.println("Filling , Full");
    Serial.println("Stop filling....");
    updateDisplay("Stop filling RO");
    relayState.ro = LOW;
    digitalWrite(roPin, relayState.ro);
    t=0;              // reset t for next occurence
    }
    else {
      //Serial.println("RO Is full");
    }
  
  
  }
  else {
    Serial.println("Manually filling.... ");
  }
 
 
}

void manualFill(uint8_t T) {
  Serial.println("Manual Fill called");
  if (relayState.ro == LOW) {
      manualFillMetro.reset();
      manualFillMetro.interval(minutesToMillis(T));
      digitalWrite(roPin, HIGH);
      relayState.ro = HIGH;
      manFilling = true;
      
    
  }
 else {
      stopRO();
      manFilling=false;
      manualFillMetro.reset();
      manualFillMetro.interval(hoursToMillis(24));
      
 }
}

void switchLight1(uint8_t state) {
   digitalWrite(light1Pin, state);
   relayState.light1=state;
}

void switchLight2(uint8_t state) {
   digitalWrite(light2Pin, state);
   relayState.light2=state;
}

/*
void switchNightLight(uint8_t state) {
//   digitalWrite(nightLightPin, state);
   nightLightRelayState=state;
}
*/
void updateDisplay(String msg) {
  static int disp;      
  
   display.clearDisplay();
   if (msg == "" ) {
    
   
      switch(disp) {
      case 0:
        display.setCursor(0,14);
        display.setTextSize(4);
        display.println(cda.dateTime("H:i"));
        disp +=1;
        break;
      case 1:
        display.setCursor(0,14);
        display.setTextSize(2);
        display.println("IP Address");
        display.setTextSize(1);
        display.println(WiFi.localIP());
        /*
        display.setTextSize(2);
        display.println("Status");
        display.println(WiFi.status());
        */
        disp +=1;
        break;
      case 2:
        display.setCursor(0,14);
        display.setTextSize(3);
        display.print(GetTemp());
        display.write(248);     //ASCII Degree symbol
        display.println("C");
        disp +=1;
        break;
      case 3:
        display.setCursor(0, 14);
        display.setTextSize(3);
        display.print(GetHumidity());
        display.println("%");     //ASCII Degree symbol
        //display.println("");
        disp += 1;
        break;
      case 4:
        display.setCursor(0,0);
        display.setTextSize(2);
        display.println("Terrarium");
        display.println("Control");
        display.setTextSize(1);
        display.println(appVers);
        disp=0;
        break;        
      default:
        disp=0;
        break;
      }
   }
   else {
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println(msg);
    
   }
   display.display();
   /*
   time_t tnow = time(nullptr);
  //Serial.println();
   display.clearDisplay();
  
   
   display.print("Temperature : ");
   
   display.println(msg);
   
   //display.startscrollright(0x02, 0x02);
   Serial.print("Last NTP Update : ");
   Serial.println(lastNtpUpdateTime());
   */
}

void topFan() {
  String msg;
  if (relayState.topFan  == LOW) {
    topFanMetro.reset();
    topFanMetro.interval(minutesToMillis(fanTime));
    relayState.topFan = HIGH;
    msg="Starting Top Fan for ";
    msg += fanTime;
    msg += " minutes";
    Serial.println(msg);
    updateDisplay(msg);
  }
  else {
    topFanMetro.reset();
    topFanMetro.interval(minutesToMillis(fanFreq));
    msg = "Pausing Top Fan for";
    msg += fanFreq;
    msg += " minutes";
    relayState.topFan = LOW;
    Serial.println(msg);
    updateDisplay(msg);
    
  }
  digitalWrite(topFanPin, relayState.topFan);

}

void timedEvents(String curTime) {
  Serial.print("current Time : ");
  Serial.println(cda.dateTime("H:i"));

  if (curTime == light1On) {
    updateDisplay("Turn Light1 ON");
    Serial.println("Turn Light1 ON");
    switchLight1(HIGH);
    Serial.println("Start Top fan");
    
    topFan();
  }
  else if (curTime == light2On) {
    updateDisplay("Turn Light2 ON");
    Serial.println("Turn Light2 ON");
    switchLight2(HIGH);
    //pump();
    sprayMetro.reset();
    sprayMetro.interval(secondsToMillis(10));    //pump will run 10 seconds after light are turned on

  }
  else if (curTime == light1Off) {
    updateDisplay("Turn Light1 Off");
    Serial.println("Turn Light1 Off & Night Light On");
    switchLight1(LOW);
    
    if (relayState.topFan == HIGH) {            //If top fan is running, then stop it
        topFan();  
    }
    
    topFanMetro.reset();
    topFanMetro.interval(hoursToMillis(23));       //stops top fan for the night
    sprayMetro.reset();
    sprayMetro.interval(hoursToMillis(23));         // stops pumpEvents for the night
  }
  else if (curTime == light2Off) {
    updateDisplay("Turn Light2 Off");
    Serial.println("Turn Light2 Off");
    switchLight2(LOW);
  }
  /*      Night Lights stuff disabled due to V1.0 of PCB issues
  else if (curTime == nightLightOn) {
    Serial.println("Turn Night Light On");
    switchNightLight(HIGH);
  }
  else if (curTime == nightLightOff) {
    Serial.println("Turn Night Light Off");
    switchNightLight(LOW);
  }
  */
}

/*
  void PulseFan() {
  const int speedSteps = 5;
  static bool goingUp = true;
  static int x;
  int fanSpeed;

  if (goingUp == true  && x == 100) {
    goingUp = !goingUp;
    x = x - speedSteps;
  }
  else if (goingUp == true && x < 100) {
    x = x + speedSteps;
  }
  else if (goingUp == false && x == 0) {
    goingUp = !goingUp;
    x = x + speedSteps;
  }
  else {
    x = x - speedSteps;
  }
  Serial.println("Pulsing Fan");
  Serial.println(x);
  fanSpeed = map(x, 0, 100, 101, 200);


  analogWrite(fanPin, fanSpeed);


  }
*/

unsigned long secondsToMillis(unsigned long sec) {
  return sec * 1000;
}

unsigned long minutesToMillis(unsigned long minutes) {
  return minutes * 60 * 1000;
}

unsigned long hoursToMillis(unsigned long hrs) {
  unsigned long val = hrs * 60 * 60 * 1000;
  return val;
}

void checkhttpclient() {

    server.handleClient();

    digitalWrite(light1Pin, relayState.light1);
    digitalWrite(light2Pin, relayState.light2);
    digitalWrite(topFanPin, relayState.topFan);
    digitalWrite(roPin, relayState.ro);
}

void handle_OnConnect() {
    //LED1status = LOW;
    //LED2status = LOW;

    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp()));
}

void handle_light1on() {
    //LED1status = HIGH;
    Serial.println("Light1 Status: ON");
    updateDisplay("Manually turning Light 1 ON");
    switchLight1(HIGH);
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp()));
}

void handle_light1off() {
    //LED1status = LOW;
    Serial.println("Light1 Status: OFF");
    updateDisplay("Manually turning Light 1 OFF");
    switchLight1(LOW);
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp()));
}

void handle_light2on() {
    //LED2status = HIGH;
    Serial.println("Light2 Status: ON");
    updateDisplay("Manually turning Light 2 ON");
    switchLight2(HIGH);
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp()));
}

void handle_light2off() {

    Serial.println("Light2 Status: OFF");
    updateDisplay("Manually turning Light 2 OFF");
    switchLight2(LOW);
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp()));
}
/*
void handle_nightlightoff() {
  Serial.println("Night Light Status: OFF");
  updateDisplay("Manually turning OFF Night Light");
  switchNightLight(LOW);
  server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState,  manfillTime,GetTemp(),pumpState));
}

void handle_nightlighton() {
  Serial.println("Night Light Status: ON");
  updateDisplay("Manually turning ON Night Light");
  switchNightLight(HIGH);
  server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState, manfillTime,GetTemp(),pumpState));
}
*/

void handle_NotFound() {
    server.send(404, "text/plain", "Not found");
}

void handle_FillRO() {


    manfillTime = server.arg("filltime").toInt();
    Serial.println(manfillTime);
    String msg = "Manually Filling RO for ";
    msg += manfillTime;
    msg += " minutes";
    updateDisplay(msg);
    Serial.println(msg);
    manualFill(manfillTime);
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp()));


}

void handle_SetTimes() {
    light1On = server.arg("light1start");
    light1Off = server.arg("light1stop");
    light2On = server.arg("light2start");
    light2Off = server.arg("light2stop");
    nightLightOn = server.arg("nlightstart");
    nightLightOff = server.arg("nlightstop");
    pumpFreq = server.arg("pumpfreq").toInt();
    pumpTime = server.arg("pumplen").toInt();
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp()));
}

void handle_pumpOn() {
    //pumpState = HIGH;
    Serial.println("Manually Pumping");
    //digitalWrite(pumpPin, pumpState);
    spray();
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp()));
}

void handle_pumpOff() {
    //pumpState = LOW;
    Serial.println("Manually Stoping Pump");
    //digitalWrite(pumpPin, pumpState);
    spray();
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp()));
}

void handle_settings() {
    server.send(200, "text/html", sendSettingsHTML(pumpFreq, pumpTime, fanTime, fanFreq, light1On, light1Off, light2On, light2Off, nightLightOn, nightLightOff));
}

void handle_topFan() {
    topFan();
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp()));
}
