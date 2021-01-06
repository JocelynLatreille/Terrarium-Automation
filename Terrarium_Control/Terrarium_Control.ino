#include <DallasTemperature.h>
#include <OneWire.h>

#include <ezTime.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Metro.h>

String appVers ="Version 1.0";
// *******************************  All these will need to be changed for the esp8266 pins ********************************
#define light1Pin D0                    //Relay 1 
#define light2Pin D3                    //Relay 4
#define nightLightPin D5                 //Relay 5
#define topFanPin D8                     //Fan to remove condensation in the front glass
#define pumpPin 3                       //Relay 5 (RX pin on NodeMCU)
#define roPin D7                        //Reverse Osmosis relay
#define fullROpin D6                     //Switch 1 - P6
#define tempPin D4                      //D5   DS18B20 Temperature sensor

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

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
OneWire ow(tempPin);
DallasTemperature tempSensor(&ow);
//**********************  ezTime stuff ***********************
Timezone cda;
const long utcOffsetInSeconds = -18000;           // Offset for the Timezone
//*******************  Wi-Fi info ************************
const char* ssid = "whipet";  // Enter SSID here
const char* password = "b1gb00b5";  //Enter Password here
ESP8266WebServer server(80);
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
// ****************  recurring events ****************
Metro pumpMetro ;         //
//Metro fogMetro ;
//Metro fanMetro ;
Metro checkCurTime ;      // Event to check the current time and act on any timed event
Metro topFanMetro ;       // Top fan to remove water from front glass
Metro manualFillMetro;    //RO water manual fill
//****************************************************
int pumpState = LOW;
//int fogState = LOW;               //No need for a fan state as it will run at the same time as the fan
int topFanState = LOW;            //Top ventilation fan
// Rename those to relayX
int light1RelayState = LOW;
int light2RelayState = LOW;
int nightLightRelayState = LOW;
int roRelayState = LOW;
uint8_t manfillTime = 11;   //Nuber of minutes to manually fill RO bottles
uint8_t fullRO;                //Level of RO supply


typedef struct {
  String IP;
  String Temp;
}messages;

 messages mssg={"",""} ;


void setup() {
  
  Wire.begin();
  Adafruit_SSD1306 display(-1);
  Serial.begin(9600);
  while (!Serial);
 
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
 
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.clearDisplay();
  display.display(); 

  delay(3000); // wait for console opening

  //************************  Set Pins  **************************
  pinMode(light1Pin, OUTPUT);
  pinMode(light2Pin, OUTPUT);
  pinMode(nightLightPin, OUTPUT);
  pinMode(topFanPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(roPin, OUTPUT);
  pinMode(fullROpin, INPUT_PULLUP);

  digitalWrite(light1Pin, LOW);
  digitalWrite(light2Pin, LOW);
  digitalWrite(nightLightPin, LOW);
  digitalWrite(topFanPin, LOW);
  digitalWrite(pumpPin, LOW);
  digitalWrite(roPin, LOW);
  //************************ Set recurring events  ********************
  pumpMetro.interval(hoursToMillis(20));        //Set to a ridiculously long delay . Startup and Timed events will take care of the rest
  topFanMetro.interval(hoursToMillis(20));      //Set to a ridiculously long delay . Startup and Timed events will take care of the rest
  checkCurTime.interval(secondsToMillis(45));   //Check current time for any sceduled events
  

  Serial.println("Connecting to ");
  display.print("Connecting to \n");
  Serial.println(ssid);
  display.println(ssid);
  display.display();
  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    display.print(".");
    display.display();
  }
  waitForSync();
  display.clearDisplay();
  display.setCursor(0,0);
  Serial.println("");
  Serial.println("WiFi connected..!\n");
  
  display.println("WiFi connected..!");
  mssg.IP ="test";
  Serial.print("Got IP: ");  
  Serial.println(WiFi.localIP());
  display.print("Got IP: ");  
  display.println(WiFi.localIP());
  display.display();
  delay(3000);

  cda.setPosix("EST5EDT,M3.5.0,M10.5.0");     //Sets timezone to GMT+5, aka here
  Serial.println(cda.dateTime());             //Prints datetime
  Serial.println(cda.dateTime("H:i"));        //Prints Time in HH:MM format . This is how we will compare timed events
  startup();
  // ******************************  Initialize Web Server and web events ******************************
  server.on("/", handle_OnConnect);
  server.on("/light1on", handle_light1on);
  server.on("/light1off", handle_light1off);
  server.on("/light2on", handle_light2on);
  server.on("/light2off", handle_light2off);
  server.on("/nightlighton",handle_nightlighton);
  server.on("/nightlightoff",handle_nightlightoff);
  server.on("/FillRO", handle_FillRO);
  server.on("/SetTimes",handle_SetTimes);
  server.on("/settings",handle_settings);
  server.on("/pumpOn",handle_pumpOn);
  server.on("/pumpOff",handle_pumpOff);
  server.begin();
  Serial.println("HTTP server started");

  tempSensor.begin();
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
    light1RelayState=HIGH;

  }
  if ((cda.hour() >= light2On.substring(0, 2).toInt()) && (cda.hour() < light2Off.substring(0, 2).toInt())) {

    Serial.println("Turning Light2 On");
    digitalWrite(light2Pin, HIGH);    // Turning Light 2 ON
    light2RelayState=HIGH;
    pump();     //Lights are on, start the misting sequence
    topFan();  //Starts top fan


  }
 
  if ((cda.hour() >= nightLightOn.substring(0, 2).toInt()) && (cda.hour() <= nightLightOff.substring(0, 2).toInt())) {

    Serial.println("Turning Night Light On");
    digitalWrite(nightLightPin, HIGH);    // Turning night Light  ON
    nightLightRelayState=HIGH;

  }
  else {
    //Nothing for now

  }


}

void loop() {

  if (checkCurTime.check() == 1) {
    Serial.println("timed event");
    timedEvents(cda.dateTime("H:i"));
  }

  if (pumpMetro.check() == 1) {
    Serial.println("Pump Event");
    pump();
  }

  if (topFanMetro.check() ==1) {
    Serial.println("Top Fan Event");
    topFan();
  }

  if ((manualFillMetro.check() == 1) && (roRelayState == HIGH)) {
    stopRO();
  }
  /*
    The relay I used somehow seem to be triggered by a lOW pin, and not a hHIG|h one
    somehow....

    if (fogMetro.check() == 1) {
    Serial.println("Fogger event");
    fogger();
    }
  */
  checkhttpclient();
  checkRO();
}

float GetTemp() {
  
  tempSensor.requestTemperatures();
  return tempSensor.getTempCByIndex(0);
  /*
  return 37.99;
  */
}

void pump() {
  if (pumpState == LOW) {
    pumpState = HIGH;
    Serial.println("Pumping");
    pumpMetro.interval(secondsToMillis(pumpTime));
  }
  else {
    pumpState = LOW;
    Serial.println("Stopping pump");
    if ((light1RelayState==LOW) && (light2RelayState==LOW)) {
      pumpMetro.interval(minutesToMillis(23));
    }
    else {
      pumpMetro.interval(minutesToMillis(pumpFreq));
    }
    
  }
  digitalWrite(pumpPin, pumpState);
  //digitalWrite(ledPin,pumpState);
}

void stopRO() {
  digitalWrite(roPin, LOW);
  roRelayState = LOW;
}

void checkRO() {
  static unsigned long t;
  fullRO = digitalRead(fullROpin);
  if ((roRelayState == LOW) && (fullRO == HIGH)) {
    if (t==0) {
    t=millis();
    }
   else if (millis()-t>3000) {              //Debounce switch 
    Serial.println("Not filling , Not Full");
    Serial.println("Starting to fill....");
    digitalWrite(roPin, HIGH);
    roRelayState=HIGH;
    }
  }
  else if ((roRelayState == HIGH) && (fullRO == LOW)) {
    Serial.println("Filling , Full");
    Serial.println("Stop filling....");
    digitalWrite(roPin, LOW);
    roRelayState=LOW;
    t=0;
  }
 
}

void manualFill(uint8_t T) {

  if (roRelayState == LOW)
  
  {
       
      manualFillMetro.interval(minutesToMillis(T));
      digitalWrite(roPin, HIGH);
      roRelayState = HIGH;
  }
 else {
      stopRO();
      manualFillMetro.interval(hoursToMillis(24));
 }
}

void switchLight1(uint8_t state) {
   digitalWrite(light1Pin, state);
   light1RelayState=state;
}

void switchLight2(uint8_t state) {
   digitalWrite(light2Pin, state);
   light2RelayState=state;
}

void switchNightLight(uint8_t state) {
   digitalWrite(nightLightPin, state);
   nightLightRelayState=state;
}

void topFan() {
  if (topFanState == LOW) {
    topFanState = HIGH;
    Serial.println("Starting Top Fan");
    topFanMetro.interval(minutesToMillis(10));
  }
  else {
    topFanState = LOW;
    Serial.println("Stoping Top Fan");
    topFanMetro.interval(minutesToMillis(45));
  }
  digitalWrite(topFanPin, topFanState);

}

void timedEvents(String curTime) {
  Serial.print("current Time : ");
  Serial.println(curTime);

  if (curTime == light1On) {
    Serial.println("Turn Light1 ON");
    switchLight1(HIGH);
    Serial.println("Start Top fan");
    topFan();
  }
  else if (curTime == light2On) {
    Serial.println("Turn Light2 ON");
    switchLight2(HIGH);
    pumpMetro.interval(hoursToMillis(1));    //pump will run 1 hour after light are turned on

  }
  else if (curTime == light1Off) {
    Serial.println("Turn Light1 Off & Night Light On");
    switchLight1(LOW);
    topFanMetro.interval(hoursToMillis(23));       //stops top fan for the night
    pumpMetro.interval(hoursToMillis(23));         // stops pumpEvents for the night
  }
  else if (curTime == light2Off) {
    Serial.println("Turn Light2 Off");
    switchLight2(LOW);
  }
  else if (curTime == nightLightOn) {
    Serial.println("Turn Night Light On");
    switchNightLight(HIGH);
  }
  else if (curTime == nightLightOff) {
    Serial.println("Turn Night Light Off");
    switchNightLight(LOW);
  }
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
void checkhttpclient() {

  server.handleClient();
  /*
    if(light1RelayState)
    {digitalWrite(light1pin, HIGH);}
    else
    {digitalWrite(light1pin, LOW);}

    if(light2RelayState)
    {digitalWrite(light2pin, HIGH);}
    else
    {digitalWrite(light2pin, LOW);}
  */

  digitalWrite(light1Pin, light1RelayState);
  digitalWrite(light2Pin, light2RelayState);
  digitalWrite(nightLightPin, nightLightRelayState);
  digitalWrite(roPin, roRelayState);
}

void handle_OnConnect() {
  //LED1status = LOW;
  //LED2status = LOW;

  server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState,  manfillTime,GetTemp(),pumpState));
}

void handle_light1on() {
  //LED1status = HIGH;
  Serial.println("Light1 Status: ON");
  switchLight1(HIGH);
  server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState,  manfillTime,GetTemp(),pumpState));
}

void handle_light1off() {
  //LED1status = LOW;
  Serial.println("Light1 Status: OFF");
  switchLight1(LOW);
  server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState,  manfillTime,GetTemp(),pumpState));
}

void handle_light2on() {
  //LED2status = HIGH;
  Serial.println("Light2 Status: ON");
  switchLight2(HIGH);
  server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState,  manfillTime,GetTemp(),pumpState));
}

void handle_light2off() {

  Serial.println("Light2 Status: OFF");
  switchLight2(LOW);
  server.send(200, "text/html", sendMainHTML(light1RelayState,light2RelayState, nightLightRelayState, roRelayState, manfillTime,GetTemp(),pumpState));
}

void handle_nightlightoff() {
  Serial.println("Night Light Status: OFF");
  switchNightLight(LOW);
  server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState,  manfillTime,GetTemp(),pumpState)); 
}

void handle_nightlighton() {
  Serial.println("Night Light Status: ON");
  switchNightLight(HIGH);
  server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState, manfillTime,GetTemp(),pumpState)); 
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

void handle_FillRO() {
  
  
    manfillTime = server.arg("filltime").toInt();
    Serial.println(manfillTime);
    Serial.println("Fill RO");
    manualFill(manfillTime);
    server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState, manfillTime,GetTemp(),pumpState));
  
  
}

void handle_SetTimes() {
  light1On=server.arg("light1start");
  light1Off=server.arg("light1stop");
  light2On=server.arg("light2start");
  light2Off=server.arg("light2stop");
  nightLightOn=server.arg("nlightstart");
  nightLightOff=server.arg("nlightstop");
  pumpFreq=server.arg("pumpfreq").toInt();
  pumpTime=server.arg("pumplen").toInt();
  server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState, manfillTime,GetTemp(),pumpState));
}

void handle_pumpOn() {
  pumpState = HIGH;
  Serial.println("Manually Pumping");
  digitalWrite(pumpPin, pumpState);
  server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState, manfillTime,GetTemp(),pumpState));
}

void handle_pumpOff() {
  pumpState = LOW;
  Serial.println("Stoping Pump");
  digitalWrite(pumpPin, pumpState);
  server.send(200, "text/html", sendMainHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState, manfillTime,GetTemp(),pumpState));
}

void handle_settings() {
  server.send(200,"text/html",sendSettingsHTML(pumpFreq,pumpTime, light1On, light1Off, light2On, light2Off, nightLightOn, nightLightOff));
}

String sendMainHTML(uint8_t light1stat, uint8_t light2stat, uint8_t nitelitestat, uint8_t rostat,  uint8_t mf ,float temp,uint8_t pumpstate) {
  String ptr = "<!DOCTYPE html> <html>\n"; 
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>Terrarium Control</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #1abc9c;}\n";
  ptr += ".button-on:active {background-color: #16a085;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>Terrarium Control</h1>";
  ptr += "<h2>";
  ptr += appVers;
  ptr += "</h2>\n";
  ptr += "<p>Temperature: ";
  ptr += temp;
  ptr += "C</p>";
  if (light1stat)
  {
    ptr += "<p>Light1 Status: ON</p><a class=\"button button-off\" href=\"/light1off\">OFF</a>\n";
  }
  else
  {
    ptr += "<p>Light1 Status: OFF</p><a class=\"button button-on\" href=\"/light1on\">ON</a>\n";
  }

  if (light2stat)
  {
    ptr += "<p>Light2 Status: ON</p><a class=\"button button-off\" href=\"/light2off\">OFF</a>\n";
  }
  else
  {
    ptr += "<p>Light2 Status: OFF</p><a class=\"button button-on\" href=\"/light2on\">ON</a>\n";
  }

  if (nitelitestat)
  {
    ptr += "<p>Night Light Status: ON</p><a class=\"button button-off\" href=\"/nightlightoff\">OFF</a>\n";
  }
  else
  {
    ptr += "<p>Night Light Status: OFF</p><a class=\"button button-on\" href=\"/nightlighton\">ON</a>\n";
  }
  if (pumpstate==LOW) {
    ptr += "<p>Water Misting</p><a class=\"button button-on\" href=\"/pumpOn\">Start</a>\n";
  }
  else {
    ptr += "<p>Water Misting</p><a class=\"button button-off\" href=\"/pumpOff\">Stop</a>\n";
  }

  ptr += "<form action=/FillRO>";
  ptr += "<label for=fname>Number of minutes to fill</label><br>";
  ptr += "<input type=number id=filltime name=filltime value=";
  ptr += mf;
  ptr += "><br>";
  if (rostat==LOW)
  {
  ptr += "<input type=submit value=Fill><br><br>";
  }
  else
  {
  ptr += "<input type=submit value=Stop><br><br>";  
  }
  ptr += "</form> ";

  //ptr += "<p>Application Settings</p><a class=\"button button-on\" href=\"/settings\">Settings</a>\n";
  ptr += "<a href=/settings>Settings</a>";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

String sendSettingsHTML(uint8_t pumpfreq,uint8_t pumpLen,String l1start, String l1stop, String l2start, String l2stop, String nlstart, String nlstop) {
  String ptr = "<!DOCTYPE html> <html>\n"; 
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>App Settings</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #1abc9c;}\n";
  ptr += ".button-on:active {background-color: #16a085;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>Terrarium Control Settings</h1>";
  ptr += "<form action=/SetTimes>";
  ptr += "<label for=fname>Pump frequency (minutes)</label><br>";
  ptr += "<input type=number id=pumpfreq name=pumpfreq value=";
  ptr += pumpfreq;
  ptr += "><br><br>";
  ptr += "<label for=fname>Pump Duration (seconds)</label><br>";
  ptr += "<input type=number id=pumplen name=pumplen value=";
  ptr += pumpLen;
  ptr += "><br><br>";
  ptr += "<label for=fname>Light 1 Start / Stop Time</label><br>";
  ptr += "<input type=Time id=light1start name=light1start value=";
  ptr += l1start;
  ptr += ">";
  ptr += "<input type=Time id=light1stop name=light1stop value=";
  ptr += l1stop;
  ptr += "><br><br>";
  ptr += "<label for=fname>Light 2 Start / Stop Time</label><br>";
  ptr += "<input type=Time id=light2start name=light2start value=";
  ptr += l2start;
  ptr += ">";
  ptr += "<input type=Time id=light2stop name=light2stop value=";
  ptr += l2stop;
  ptr += "><br><br>";
  ptr += "<label for=fname>Night Light Start / Stop Time</label><br>";
  ptr += "<input type=Time id=nlightstart name=nlightstart value=";
  ptr += nlstart;
  ptr += ">";
  ptr += "<input type=Time id=nlightstop name=nlightstop value=";
  ptr += nlstop;
  ptr += "><br><br>";
 
  ptr += "<input type=submit value=Save>";
  ptr += "</form> ";

  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
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
