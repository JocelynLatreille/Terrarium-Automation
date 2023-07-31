#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

//*******************  Wi-Fi info ************************
char ssid[] = "Whipet 2.4";
char password[] = "pastelbird150";
char hostName[] = "Dendrobates";
//***********************************************************************

// Set your Static IP address
IPAddress local_IP(10, 0, 0, 82);
// Set your Gateway IP address
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);


void start_Wifi() {
    //connect to your local wi-fi network
    WiFi.mode(WIFI_AP_STA);
    WiFi.hostname(hostName);
    WiFi.begin(ssid, password);
    Serial.println("Connecting to ");
    display.print("Connecting to \n");
    Serial.println(ssid);
    display.println(ssid);
    display.display();
    //check wi-fi is connected to wi-fi network
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        display.print(".");
        display.display();
    }

    //Start OTA Monitor
    ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n OTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("OTA Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("OTA Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("OTA Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("OTA Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("OTA End Failed");
  });
    ArduinoOTA.setHostname(hostName);
    ArduinoOTA.begin();
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
}

 
void checkArduino_OTA() {
  ArduinoOTA.handle();
}
