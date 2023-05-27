#include <ESP8266WiFi.h>

//*******************  Wi-Fi info ************************
char ssid[] = "Whipet 2.4";
char password[] = "pastelbird150";
//***********************************************************************

// Set your Static IP address
IPAddress local_IP(10, 0, 0, 82);
// Set your Gateway IP address
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);


void start_Wifi() {
    //connect to your local wi-fi network
    WiFi.hostname("Dendrobates");
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
