#include <Evelab.h>
#include <EEPROM.h>

Evelab evelab;

void setup(){
  Serial.begin(57600);
  evelab.setup(Serial);
}

void loop(){
  evelab.process();
}
