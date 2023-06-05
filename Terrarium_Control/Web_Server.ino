


void Init_WebServer() {
   // ******************************  Initialize Web Server and web events ******************************
  server.on("/", handle_OnConnect);
  server.on("/light1on", handle_light1on);
  server.on("/light1off", handle_light1off);
  server.on("/light2on", handle_light2on);
  server.on("/light2off", handle_light2off);
  server.on("/SetTimes",handle_SetTimes);
  server.on("/settings",handle_settings);
  server.on("/pumpOn",handle_pumpOn);
  server.on("/pumpOff", handle_pumpOff);
  server.on("/topfan", handle_topFan);
  server.onNotFound([](){
	if (!espalexa.handleAlexaApiCall(server.uri(),server.arg(0)))
	{
		server.send(404, "text/plain", "Not found");
	}
});
  server.begin();
  Serial.println("HTTP server started");
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

    server.send(200, "text/html", sendMainHTML(relayState, GetTemp(),GetHumidity()));
}
void handle_light1on() {
    //LED1status = HIGH;
    Serial.println("Light1 Status: ON");
    updateDisplay("Manually turning Light 1 ON");
    switchLight1(HIGH);
    server.send(200, "text/html", sendMainHTML(relayState, GetTemp(),GetHumidity()));
}

void handle_light1off() {
    //LED1status = LOW;
    Serial.println("Light1 Status: OFF");
    updateDisplay("Manually turning Light 1 OFF");
    switchLight1(LOW);
    server.send(200, "text/html", sendMainHTML(relayState, GetTemp(),GetHumidity()));
}

void handle_light2on() {
    //LED2status = HIGH;
    Serial.println("Light2 Status: ON");
    updateDisplay("Manually turning Light 2 ON");
    switchLight2(HIGH);
    server.send(200, "text/html", sendMainHTML(relayState, GetTemp(),GetHumidity()));
}

void handle_light2off() {

    Serial.println("Light2 Status: OFF");
    updateDisplay("Manually turning Light 2 OFF");
    switchLight2(LOW);
    server.send(200, "text/html", sendMainHTML(relayState, GetTemp(),GetHumidity()));
}

void handle_NotFound() {
    server.send(404, "text/plain", "Not found");
}

void handle_SetTimes() {
    appSettings.light1_On = server.arg("light1start");
    appSettings.light1_Off = server.arg("light1stop");
    appSettings.light2_On = server.arg("light2start");
    appSettings.light2_Off = server.arg("light2stop");
    appSettings.pumpFrequency = server.arg("pumpfreq").toInt();
    appSettings.pumpDuration = server.arg("pumplen").toInt();
    server.send(200, "text/html", sendMainHTML(relayState, GetTemp(),GetHumidity()));
}

void handle_pumpOn() {
    //pumpState = HIGH;
    Serial.println("Manually Pumping");
    //digitalWrite(pumpPin, pumpState);
    spray();
    server.send(200, "text/html", sendMainHTML(relayState, GetTemp(),GetHumidity()));
}

void handle_pumpOff() {
    //pumpState = LOW;
    Serial.println("Manually Stoping Pump");
    //digitalWrite(pumpPin, pumpState);
    spray();
    server.send(200, "text/html", sendMainHTML(relayState, GetTemp(),GetHumidity()));
}

void handle_settings() {
    server.send(200, "text/html", sendSettingsHTML(appSettings));
}

void handle_topFan() {
    topFan();
    server.send(200, "text/html", sendMainHTML(relayState, GetTemp(),GetHumidity()));
}