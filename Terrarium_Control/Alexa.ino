
//******************************Alexa callback Functions and Object declaration ******************
void lightChange(uint8_t OnOff);
void pumpChange(uint8_t OnOff);


void lightChange(uint8_t OnOff)
{
  if((OnOff ==255) && (relayState.light1==LOW ))
  {
    Serial.println("Alexa allume les lumieres");
    switchLight1(HIGH);
    switchLight2(HIGH);
  }
  else if ((OnOff ==0) && (relayState.light1==HIGH ))
  {
    Serial.println("Alexa eteint les lumieres");
    switchLight1(LOW);
    switchLight2(LOW);
  }
}

void fanChange(uint8_t OnOff)
{
  if((OnOff ==255) && (relayState.topFan==LOW ))
  {
    Serial.println("Alexa allume les fans des terrariums");
    topFan();
  }
  else if ((OnOff ==0) && (relayState.topFan==HIGH))
  {
    Serial.println("Alexa eteint les fans des terrariums");
    topFan();
  }
}

void pumpChange(uint8_t OnOff)
{
  if (relayState.pump != HIGH){
    Serial.println("Alexa arose le terrarium");
    spray();
  }
  
}

void addAlexaDevices() {
  espalexa.addDevice("Lumieres Terrarium",lightChange);
  espalexa.addDevice("Pompe Terrarium",pumpChange);
  espalexa.addDevice("Fan Terrarium", fanChange);
  espalexa.begin(&server);
}
//*************************************************************************