#include <ezTime.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>


// Metro - Version: Latest
#include <Metro.h>
// *******************************  All these will need to be changed for the esp8266 pins ********************************
#define light1Pin 2
#define light2Pin 15
#define nightLightPin 14
#define topFanPin 0                     //Fan to remove condensation in the front glass
#define pumpPin 16
#define roPin 14                        //Reverse Osmosis relay
#define fullROpin 16                     //internal fan that goes along with fogger
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
String light1Off = "23:00";
String light2On = "08:10";
String light2Off = "23:00";
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
bool fillingRO = false;
bool manualFillRO = false;
uint8_t fullRO;                //Level of RO supply

void setup() {
  Serial.begin(115200);
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
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  waitForSync();
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

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
  server.on("/FillRO", handle_FillRO);
  server.begin();
  Serial.println("HTTP server started");
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


  }
  if ((cda.hour() >= light2On.substring(0, 2).toInt()) && (cda.hour() < light2Off.substring(0, 2).toInt())) {

    Serial.println("Turning Light2 On");
    digitalWrite(light2Pin, HIGH);    // Turning Light 2 ON
    pump();     //Lights are on, start the misting sequence
    TopFan();  //Starts top fan


  }
 
  if ((cda.hour() >= nightLightOn.substring(0, 2).toInt()) && (cda.hour() <= nightLightOff.substring(0, 2).toInt())) {

    Serial.println("Turning Night Light On");
    digitalWrite(nightLightPin, HIGH);    // Turning night Light  ON


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

void pump() {
  if (pumpState == LOW) {
    pumpState = HIGH;
    Serial.println("Pumping");
    pumpMetro.interval(secondsToMillis(pumpTime));
  }
  else {
    pumpState = LOW;
    Serial.println("Stopping pump");
    pumpMetro.interval(minutesToMillis(pumpFreq));
  }
  digitalWrite(pumpPin, pumpState);
  //digitalWrite(ledPin,pumpState);
}

void checkRO() {
  fullRO = digitalRead(fullROpin);

  if ((fillingRO == false) && (fullRO == HIGH)) {
    //Fill RO
    Serial.println("Not filling , Not Full");
    Serial.println("Starting to fill....");
    digitalWrite(roPin, LOW);
    fillingRO = true;
  }
  else if ((fillingRO == true) && (fullRO == LOW)) {
    Serial.println("Filling , Full");
    Serial.println("Stop filling....");
    digitalWrite(roPin, HIGH);
    fillingRO = false;
  }

}


void manual_Fill(uint8_t T) {

  if (fillingRO == false)
  {
    Serial.println("Not filling Automatically");

    
      manualFillMetro.interval(minutesToMillis(T));
                               digitalWrite(roPin, LOW);
                               manualFillRO = true;
    
  }
 
}



void TopFan() {
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
    digitalWrite(light1Pin, HIGH);
    TopFan();
  }
  else if (curTime == light2On) {
    Serial.println("Turn Light2 ON");
    digitalWrite(light2Pin, HIGH);
    pumpMetro.interval(hoursToMillis(1));    //pump will run 1 hour after light are turned on

  }
  else if (curTime == light1Off) {
    Serial.println("Turn Light1 Off & Night Light On");
    digitalWrite(light1Pin, LOW);
    digitalWrite(nightLightPin, HIGH);             // when the last day light turns off, turn on night light
    topFanMetro.interval(hoursToMillis(23));       //stops top fan for the night
    pumpMetro.interval(hoursToMillis(23));         // stops pumpEvents for the night
  }
  else if (curTime == light2Off) {
    Serial.println("Turn Light2 Off");
    digitalWrite(light2Pin, LOW);
  }
  else if (curTime == nightLightOff) {
    Serial.println("Turn Night Light Off");
    digitalWrite(nightLightPin, LOW);
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

  server.send(200, "text/html", SendHTML(light1RelayState, light2RelayState, nightLightRelayState, roRelayState, light1On, light1Off, light2On, light2Off, nightLightOn, nightLightOff, manfillTime,pumpFreq));
}

void handle_light1on() {
  //LED1status = HIGH;
  Serial.println("Light1 Status: ON");
  server.send(200, "text/html", SendHTML(true, light2RelayState, nightLightRelayState, roRelayState, light1On, light1Off, light2On, light2Off, nightLightOn, nightLightOff, manfillTime,pumpFreq));
}

void handle_light1off() {
  //LED1status = LOW;
  Serial.println("Light1 Status: OFF");
  server.send(200, "text/html", SendHTML(false, light2RelayState, nightLightRelayState, roRelayState, light1On, light1Off, light2On, light2Off, nightLightOn, nightLightOff, manfillTime,pumpFreq));
}

void handle_light2on() {
  //LED2status = HIGH;
  Serial.println("Light2 Status: ON");
  server.send(200, "text/html", SendHTML(light1RelayState, true, nightLightRelayState, roRelayState, light1On, light1Off, light2On, light2Off, nightLightOn, nightLightOff, manfillTime,pumpFreq));
}

void handle_light2off() {
  //LED2status = LOW;
  Serial.println("Light2 Status: OFF");
  server.send(200, "text/html", SendHTML(light1RelayState, false, nightLightRelayState, roRelayState, light1On, light1Off, light2On, light2Off, nightLightOn, nightLightOff, manfillTime,pumpFreq));
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

void handle_FillRO() {
  manfillTime = server.arg("filltime").toInt();
  Serial.println(manfillTime);
  Serial.println("Fill RO");

  server.send(200, "text/html", SendHTML(light1RelayState, light2RelayState, nightLightRelayState, true, light1On, light1Off, light2On, light2Off, nightLightOn, nightLightOff, manfillTime,pumpFreq));
}

String SendHTML(uint8_t light1stat, uint8_t light2stat, uint8_t nitelitestat, uint8_t rostat, String l1start, String l1stop, String l2start, String l2stop, String nlstart, String nlstop, uint8_t mf , uint8_t pumpfreq) {
  String ptr = "<!DOCTYPE html> <html>\n"; 
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>LED Control</title>\n";
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
  ptr += "<h1>Terrariium Control</h1>\n";

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

  ptr += "<form action=/FillRO>";
  ptr += "<label for=fname>Number of minutes to fill</label><br>";
  ptr += "<input type=number id=filltime name=filltime value=";
  ptr += mf;
  ptr += "><br>";

  ptr += "<input type=submit value=Submit>";
  ptr += "</form> ";

  ptr += "<form action=/SetTimes>";
  ptr += "<label for=fname>Light 1 Start / Stop Time</label><br>";
  ptr += "<input type=Time id=light1start name=light1start value=";
  ptr += l1start;
  ptr += ">";
  ptr += "<input type=Time id=light1stop name=light1stop value=";
  ptr += l1stop;
  ptr += ">";
  ptr += "<br>";
  ptr += "<label for=fname>Light 2 Start / Stop Time</label><br>";
  ptr += "<input type=Time id=light2start name=light2start value=";
  ptr += l2start;
  ptr += ">";
  ptr += "<input type=Time id=light2stop name=light2stop value=";
  ptr += l2stop;
  ptr += ">";
  ptr += "<br>";
  ptr += "<label for=fname>Night Light Start / Stop Time</label><br>";
  ptr += "<input type=Time id=nlightstart name=nlightstart value=";
  ptr += nlstart;
  ptr += ">";
  ptr += "<input type=Time id=nlightstop name=nlightstop value=";
  ptr += nlstop;
  ptr += ">";
  ptr += "<br>";
  ptr += "<label for=fname>Pump frequency (minutes)</label><br>";
  ptr += "<input type=number=pumpfreq name=pumpfreq value=";
  ptr += pumpfreq;
  ptr += ">";
  ptr += "<br>";

  ptr += "<input type=submit value=Save>";
  ptr += "</form> ";

  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}




unsigned long secondsToMillis(unsigned long sec) {
  return sec * 1000;
}

unsigned long minutesToMillis(unsigned long min) {
  return min * 60 * 1000;
}

unsigned long hoursToMillis(unsigned long hrs) {
  unsigned long val = hrs * 60 * 60 * 1000;
  return val;
}
