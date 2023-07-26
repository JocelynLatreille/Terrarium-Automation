


String sendMainHTML(struct relayState_t relSt, float temp, float Humidity) {
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
  ptr += "input[type=submit] {background-color: #1abc9c; border: none; color: white; padding: 13px 30px; text-decoration: none; font-size: 25px; margin: 4px 2px; cursor: pointer;}\n";
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
  if (relSt.light1) {
    ptr += "<p>Light1 Status: ON</p><a class=\"button button-off\" href=\"/light1off\">OFF</a>\n";
  } else {
    ptr += "<p>Light1 Status: OFF</p><a class=\"button button-on\" href=\"/light1on\">ON</a>\n";
  }

  if (relSt.light2) {
    ptr += "<p>Light2 Status: ON</p><a class=\"button button-off\" href=\"/light2off\">OFF</a>\n";
  } else {
    ptr += "<p>Light2 Status: OFF</p><a class=\"button button-on\" href=\"/light2on\">ON</a>\n";
  }
  if (relSt.topFan) {
    ptr += "<p>Top Fan Status: ON</p><a class=\"button button-off\" href=\"/topfan\">OFF</a>\n";
  } else {
    ptr += "<p>Top Fan Status: OFF</p><a class=\"button button-on\" href=\"/topfan\">ON</a>\n";
  }
  
  if (relSt.pump == LOW) {
    ptr += "<p>Water Misting</p><a class=\"button button-on\" href=\"/pumpOn\">Start</a>\n";
  } else {
    ptr += "<p>Water Misting</p><a class=\"button button-off\" href=\"/pumpOff\">Stop</a>\n";
  }

  //ptr += "<p>Application Settings</p><a class=\"button button-on\" href=\"/settings\">Settings</a>\n";
  ptr += "<a href=/settings>Settings</a>";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

String sendSettingsHTML(struct settings_t mySettings) {
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
  ptr += "<label for=fname>Pump Frequency (minutes)</label><br>";
  ptr += "<input type=number id=pumpfreq name=pumpfreq value=";
  ptr += mySettings.pumpFrequency;
  ptr += "><br><br>";
  ptr += "<label for=fname>Pump Duration (seconds)</label><br>";
  ptr += "<input type=number id=pumplen name=pumplen value=";
  ptr += mySettings.pumpDuration;
  ptr += "><br><br>";
  ptr += "<label for=fname>Fan frequency (minutes)</label><br>";
  ptr += "<input type=number id=fanfreq name=fanfreq value=";
  ptr += mySettings.fanFrequency;
  ptr += "><br><br>";
  ptr += "<label for=fname>Fan Duration (minutes)</label><br>";
  ptr += "<input type=number id=fanLen name=fanLen value=";
  ptr += mySettings.fanDuration;
  ptr += "><br><br>";
  ptr += "<label for=fname>Light 1 Start / Stop Time</label><br>";
  ptr += "<input type=Time id=light1start name=light1start value=";
  ptr += mySettings.light1_On;
  ptr += ">";
  ptr += "<input type=Time id=light1stop name=light1stop value=";
  ptr += mySettings.light1_Off;
  ptr += "><br><br>";
  ptr += "<label for=fname>Light 2 Start / Stop Time</label><br>";
  ptr += "<input type=Time id=light2start name=light2start value=";
  ptr += mySettings.light2_On;
  ptr += ">";
  ptr += "<input type=Time id=light2stop name=light2stop value=";
  ptr += mySettings.light2_Off;
  ptr += "><br><br>";
  
  ptr += "<input type=submit value=Save>";
  ptr += "</form> ";

  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}
