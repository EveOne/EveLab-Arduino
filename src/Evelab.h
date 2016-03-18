#ifndef Evelab_h
#define Evelab_h

#include "Arduino.h"
#include "lib/CmdProcessor.h"
#include "lib/Adafruit_MPR121.h"
#include <EEPROM.h>

#define STATUS_LED 13

#define Evelab_VERSION "2.0.2"

#define MAGIC_BYTE_1 0xF0
#define MAGIC_BYTE_2 0x0D

//12 Musical Notes Chromatic Scale C-B
#define NOTE_C 262
#define NOTE_CSHARP 277
#define NOTE_D 294
#define NOTE_DSHARP 311
#define NOTE_E 330
#define NOTE_F 349
#define NOTE_FSHARP 370
#define NOTE_G 392
#define NOTE_GSHARP 415
#define NOTE_A 440
#define NOTE_ASHARP 466
#define NOTE_B 493

struct HwVersion {
  byte major;
  byte minor;
};

class Evelab {
  public:
    Evelab();
    void setup();
    void setup(Stream &s);
    void pause();
    void resume();
    void stop();
    void reset();
    short getTempC();
    short getTempF();
    short getCDSLumens();
    short getAnalogPinValue(byte);
    void setTempPin(byte);
    void setCDSPin(byte);
    void setSpeakerPin(byte);
    void gpioOn(byte);
    void gpioOff(byte);
    void gpioPWM3(byte);
    void gpioPWM5(byte);
    void gpioPWM6(byte);
    byte mpr121read();
    void note(short,short);
    void setHwVersion(char&);
    boolean ready();
    void process();
    HwVersion hwVersion;
    boolean blocking;
    void version(char);
  private:
    void wait();
    void mpr121Process();
    void SensorUpdateHandler();
    void ledHandler();
    void checkState();
    void initHwVersion();
    Evelab& self() { return *this; }
    boolean paused;
};

#endif
