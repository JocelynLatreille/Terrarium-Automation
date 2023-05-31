
//******************************Alexa callback Functions and Object declaration ******************
void lightChange(uint8_t OnOff);
void pumpChange(uint8_t OnOff);


void lightChange(uint8_t OnOff)
{
  if(OnOff ==255)
  {
    Serial.println("Alexa allume les lumieres");
    switchLight1(HIGH);
    switchLight2(HIGH);
  }
  else
  {
    Serial.println("Alexa eteint les lumieres");
    switchLight1(LOW);
    switchLight2(LOW);
  }
}

void pumpChange(uint8_t OnOff)
{
  Serial.println("Alexa arose le terrarium");
  spray();
}

void addAlexaDevices() {
  espalexa.addDevice("Lumieres Terrarium",lightChange);
  espalexa.addDevice("Pompe Terrarium",pumpChange);
  espalexa.begin();
}
//*************************************************************************