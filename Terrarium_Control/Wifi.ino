#include <ESP8266WiFi.h>



// Set your Static IP address
IPAddress local_IP(10, 0, 0, 82);
// Set your Gateway IP address
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);


void start_Wifi() {
    //connect to your local wi-fi network
    WiFi.hostname("Dendrobates");
    WiFi.begin(ssid, password);

    //check wi-fi is connected to wi-fi network
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        display.print(".");
        display.display();
    }
}
