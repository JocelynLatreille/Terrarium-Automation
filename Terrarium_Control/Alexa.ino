#include <Espalexa.h>
//******************************Alexa callback Functions and Object declaration ******************
void lightChange(uint8_t OnOff);
void pumpChange(uint8_t OnOff);
Espalexa espalexa;

void lightChange(uint8_t OnOff)
{
  if(OnOff ==255)
  {
    switchLight1(HIGH);
    switchLight2(HIGH);
  }
  else
  {
    switchLight1(LOW);
    switchLight2(LOW);
  }
}

void pumpChange(uint8_t OnOff)
{
  spray();
}

void addAlexaDevices() {
  espalexa.addDevice("Lumieres Terrarium",lightChange);
  espalexa.addDevice("Pompe Terrarium",pumpChange);
}
//*************************************************************************