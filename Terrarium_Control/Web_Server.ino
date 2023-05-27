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

    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp(),GetHumidity()));
}
void handle_light1on() {
    //LED1status = HIGH;
    Serial.println("Light1 Status: ON");
    updateDisplay("Manually turning Light 1 ON");
    switchLight1(HIGH);
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp(),GetHumidity()));
}

void handle_light1off() {
    //LED1status = LOW;
    Serial.println("Light1 Status: OFF");
    updateDisplay("Manually turning Light 1 OFF");
    switchLight1(LOW);
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp(),GetHumidity()));
}

void handle_light2on() {
    //LED2status = HIGH;
    Serial.println("Light2 Status: ON");
    updateDisplay("Manually turning Light 2 ON");
    switchLight2(HIGH);
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp(),GetHumidity()));
}

void handle_light2off() {

    Serial.println("Light2 Status: OFF");
    updateDisplay("Manually turning Light 2 OFF");
    switchLight2(LOW);
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp(),GetHumidity()));
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
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp(),GetHumidity()));


}

void handle_SetTimes() {
    light1On = server.arg("light1start");
    light1Off = server.arg("light1stop");
    light2On = server.arg("light2start");
    light2Off = server.arg("light2stop");
    //nightLightOn = server.arg("nlightstart");
    //nightLightOff = server.arg("nlightstop");
    pumpFreq = server.arg("pumpfreq").toInt();
    pumpTime = server.arg("pumplen").toInt();
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp(),GetHumidity()));
}

void handle_pumpOn() {
    //pumpState = HIGH;
    Serial.println("Manually Pumping");
    //digitalWrite(pumpPin, pumpState);
    spray();
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp(),GetHumidity()));
}

void handle_pumpOff() {
    //pumpState = LOW;
    Serial.println("Manually Stoping Pump");
    //digitalWrite(pumpPin, pumpState);
    spray();
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp(),GetHumidity()));
}

void handle_settings() {
    server.send(200, "text/html", sendSettingsHTML( pumpFreq, pumpTime, fanTime, fanFreq, light1On, light1Off, light2On, light2Off));
}

void handle_topFan() {
    topFan();
    server.send(200, "text/html", sendMainHTML(relayState, manfillTime, GetTemp(),GetHumidity()));
}