#include <ESP8266WebServer.h>
#include <Espalexa.h>
#include <SHT31.h>
#include <ezTime.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Logo.h"
#include <Metro.h>


String appVers ="Version 1.8.0";
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
Espalexa espalexa;

//**********************  Temperature Sensor variables  ***************
SHT31 sht;
//**********************  ezTime stuff ***********************
Timezone cda;

// ****************  recurring events ****************
Metro sprayMetro ;         //
Metro displayMetro ;
//Metro fanMetro ;
Metro checkCurTime ;      // Event to check the current time and act on any timed event
Metro topFanMetro ;       // Top fan to remove water from front glass
//****************************************************
struct relayState_t {
  int pump;
  int topFan;
  int light1;
  int light2;
  int ro;
} relayState;

struct settings_t {
  uint16_t pumpFrequency;          //Pump frequency in minutes
  uint8_t pumpDuration;           //Number of seconds to run the pump
  uint8_t fanDuration;            //Number of minutes to run the fan
  uint8_t fanFrequency;           //Fan frequency in minutes
  String light1_On;
  String light1_Off;
  String light2_On;
  String light2_Off;
} appSettings;

uint8_t fullRO;                //Level of RO supply

void Init_AppValues() {

    appSettings.light1_On = "08:00";
    appSettings.light1_Off = "20:55";
    appSettings.light2_On = "08:10";
    appSettings.light2_Off = "21:00";
    appSettings.pumpFrequency = 180;
    appSettings.pumpDuration = 10;
    appSettings.fanFrequency = 45;
    appSettings.fanDuration = 10;
}

void setup() {
    
  Serial.begin(115200);
  while (!Serial);
  Init_AppValues();
  

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
  displayMetro.interval(secondsToMillis(displayRefreshInterval));
  display.clearDisplay();
  display.setCursor(0,0);

  display.display();

  start_Wifi();
  Init_WebServer();  
  addAlexaDevices();
  
  waitForSync();
  
  Wire.begin();

  sht.begin(0x44);


  cda.setPosix("EST5EDT,M3.2.0,M11.1.0");     //Sets timezone to GMT+5, aka here
  Serial.println(cda.dateTime());             //Prints datetime
  Serial.println(cda.dateTime("H:i"));        //Prints Time in HH:MM format . This is how we will compare timed events
  startup();
  
}

void startup() {
  // This sub is called once during setup, to initialize anything based on the schedule after power up
  Serial.println("Initializing after power On");
  
  if (cda.hour() >= appSettings.light1_On.substring(0, 2).toInt() && cda.hour() < appSettings.light1_Off.substring(0, 2).toInt()) {

    Serial.println("Turning Light1 On");
    digitalWrite(light1Pin, HIGH);    // Turning Light 1 ON
    relayState.light1 = HIGH;

  }
  if ((cda.hour() >= appSettings.light2_On.substring(0, 2).toInt()) && (cda.hour() < appSettings.light2_Off.substring(0, 2).toInt())) {

    Serial.println("Turning Light2 On");
    digitalWrite(light2Pin, HIGH);    // Turning Light 2 ON
    relayState.light2 = HIGH;
    spray();     //Lights are on, start the misting sequence
    topFan();  //Starts top fan


  }
 
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
  espalexa.loop();
  checkArduino_OTA();
  checkRO();

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
    sprayMetro.interval(secondsToMillis(appSettings.pumpDuration));
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
      sprayMetro.interval(minutesToMillis(appSettings.pumpFrequency));
    }
    
  }
  digitalWrite(pumpPin, relayState.pump);
  server.send(200, "text/html", sendMainHTML(relayState, GetTemp(),GetHumidity()));    // If a Web client is connected, refresh the page to update the button state
 
}

void stopRO() {
  Serial.println("Stopping RO");
  updateDisplay("Stopping RO");
  relayState.ro = LOW;
  digitalWrite(roPin, relayState.ro);
  
}

void checkRO() {
  
  static unsigned long t;
  fullRO = digitalRead(fullROpin);
  //Serial.print("Full RO = ");
  //Serial.println(fullRO);
  if ((relayState.ro == LOW) && (fullRO == LOW) && (t==0)) {
    Serial.println("Not filling , Not Full");

      t=millis();  //Start timer to debounce switch
       
    }
   else if ((relayState.ro == LOW) && (fullRO == LOW) &&((millis()-t)>3000)) {              //Debounce switch 
    Serial.println((millis()-t));
    Serial.print("Ro Relay State :");
    Serial.println(relayState.ro);
    
    Serial.println("Starting to fill....");
    updateDisplay("Starting to fill RO");
    relayState.ro = HIGH;
    digitalWrite(roPin, relayState.ro);
    
    }
   else if ((relayState.ro == HIGH) && (fullRO == HIGH)) {
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
   
}

void topFan() {
  String msg;
  if (relayState.topFan  == LOW) {
    topFanMetro.reset();
    topFanMetro.interval(minutesToMillis(appSettings.fanDuration));
    relayState.topFan = HIGH;
    msg="Starting Top Fan for ";
    msg += appSettings.fanDuration;
    msg += " minutes";
    Serial.println(msg);
    updateDisplay(msg);
  }
  else {
    topFanMetro.reset();
    topFanMetro.interval(minutesToMillis(appSettings.fanFrequency));
    msg = "Pausing Top Fan for";
    msg += appSettings.fanFrequency;
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

  if (curTime == appSettings.light1_On) {
    updateDisplay("Turn Light1 ON");
    Serial.println("Turn Light1 ON");
    switchLight1(HIGH);
    Serial.println("Start Top fan");
    
    topFan();
  }
  else if (curTime == appSettings.light2_On) {
    updateDisplay("Turn Light2 ON");
    Serial.println("Turn Light2 ON");
    switchLight2(HIGH);
    //pump();
    sprayMetro.reset();
    sprayMetro.interval(secondsToMillis(10));    //pump will run 10 seconds after light are turned on

  }
  else if (curTime == appSettings.light1_Off) {
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
  else if (curTime == appSettings.light2_Off) {
    updateDisplay("Turn Light2 Off");
    Serial.println("Turn Light2 Off");
    switchLight2(LOW);
  }
  
}

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







