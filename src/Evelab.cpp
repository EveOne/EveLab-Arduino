#include "Evelab.h"


// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

CmdProcessor p;
short analogPin[8] = {0,0,0,0,0,0,0,0};
byte speakerPin = 2;
byte tempPin = 0;
byte cdsPin = 1;
byte mpr = 0;

// Pointer to the bootloader memory location
void* bl = (void *) 0x3c00;

Evelab::Evelab(){
  blocking = true;
  version(2);
}

void Evelab::setup(){
  //set pinModes
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);

  while (!Serial);        // needed to keep leonardo/micro from starting too fast!
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    while (1);
  }
}

void Evelab::setup(Stream &s){
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);

  while (!Serial);        // needed to keep leonardo/micro from starting too fast!
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    while (1);
  }

  // We will be non-blocking so we can continue to process serial input
  blocking = false;
  // Set up the command processor
  p.setup(s, self());
  initHwVersion();
}

void Evelab::initHwVersion(){
  if(EEPROM.read(0) == MAGIC_BYTE_1 && EEPROM.read(1) == MAGIC_BYTE_2){
    // We've previously written something valid to the EEPROM
    hwVersion.major = EEPROM.read(2);
    hwVersion.minor = EEPROM.read(3);
  }else{
    hwVersion.major = 0;
    hwVersion.minor = 0;
  }
}

void Evelab::setHwVersion(char &version){
  char v[4];
  int i;
  int v_ptr = 0;
  char *ptr = &version;
  for(i = 0; i < 9; i++){
    if(ptr[i] >= 0x30 && ptr[i] <= 0x39){
      v[v_ptr++] = ptr[i];
    }
    if(ptr[i] == '.'){
      v[v_ptr++] = '\0';
      break;
    }
  }
  hwVersion.major = atoi(v);
  v_ptr = 0;
  for(i = i; i < 9; i++){
    if(ptr[i] >= 0x30 && ptr[i] <= 0x39){
      v[v_ptr++] = ptr[i];
    }
    if(ptr[i] == '\0'){
      v[v_ptr++] = '\0';
      break;
    }
  }
  v[v_ptr] = '\0';
  hwVersion.minor = atoi(v);
  EEPROM.write(0, MAGIC_BYTE_1);
  EEPROM.write(1, MAGIC_BYTE_2);
  EEPROM.write(2, hwVersion.major);
  EEPROM.write(3, hwVersion.minor);
}


/*
MPR 121
*/
byte Evelab::mpr121read(){
  return mpr;
}

void Evelab::mpr121Process() {
  // Get the currently touched pads
  currtouched = cap.touched();

  for (uint8_t i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      mpr = i+1;
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
      mpr = 0;
    }
  }

  // reset our state
  lasttouched = currtouched;
  return;
}


void Evelab::pause(){
  paused = true;
}

void Evelab::resume(){
  paused = false;
}

void Evelab::stop(){
}

void Evelab::reset(){
  // Give the response message time to get out
  delay(100);
  goto *bl;
}

/*
SPEAKER NOTES
*/
void Evelab::note(short duration, short note){
  tone(speakerPin, note, duration);
}

void Evelab::setSpeakerPin(byte pin){
  speakerPin = pin;
}

/*
TEMP
*/
short Evelab::getTempC(){
  //TODO MAKE THE TEMP IN C
  return analogPin[tempPin];
}

short Evelab::getTempF(){
  //TODO MAKE THE TEMP IN F
  return analogPin[tempPin];
}

void Evelab::setTempPin(byte pin){
  tempPin = pin;
}

/*
CDS
*/
short Evelab::getCDSLumens(){
  return analogPin[cdsPin];
}

void Evelab::setCDSPin(byte pin){
  cdsPin = pin;
}

/*
GPIO
*/
void Evelab::gpioOn(byte pin){
  digitalWrite(pin, 1);
}

void Evelab::gpioOff(byte pin){
  digitalWrite(pin, 0);
}

/*
PWM
*/
void Evelab::gpioPWM3(byte pin_value){
  analogWrite(3,pin_value);
}

void Evelab::gpioPWM5(byte pin_value){
  analogWrite(5,pin_value);
}

void Evelab::gpioPWM6(byte pin_value){
  analogWrite(6,pin_value);
}

/*
ANALOG IN
*/
void Evelab::SensorUpdateHandler(){
  analogPin[0] = analogRead(0);
  analogPin[1] = analogRead(1);
  analogPin[2] = analogRead(2);
  analogPin[3] = analogRead(3);
  analogPin[4] = analogRead(4);
  analogPin[5] = analogRead(5);
  analogPin[6] = analogRead(6);
  analogPin[7] = analogRead(7);
}

short Evelab::getAnalogPinValue(byte pin){
  return analogPin[pin];
}


boolean Evelab::ready(){
  return 0;
}

void Evelab::wait(){
  if(blocking){
    while(!ready()){
      if(1){
        //wait
      }
    }
  }
}


// This allows for runtime configuration of which hardware is used
void Evelab::version(char v){
  if(v == 1){
  }else if(v == 2){
  }
}

void Evelab::process(){
  SensorUpdateHandler();
  mpr121Process();
  p.process();
}
