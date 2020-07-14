#include "Adafruit_NeoPixel.h"
#include "EasingLib.h"

#define LEDPIN 2
#define MINBRIGHTNESS 50  // (max = 255)
#define MAXBRIGHTNESS 255 // (max = 255)
#define IDLEHUE 32768
#define CRANKHUE 0
#define CORES 24
#define REFRESHRATE 1000
#define SERIALTIMEOUT 2000
#define STANDBYTIMEOUT 1000
#define SLEEPTIMEOUT 30000

enum operatingModes { sleep, standby, startup, run };
operatingModes operation = startup;

Adafruit_NeoPixel strip(CORES, LEDPIN, NEO_GRBW + NEO_KHZ800);
Easing easedCore[CORES];
// Easing easedBrightness;
// float usage[CORES];
long lastCoreUpdate[CORES];
long recentSerialResponseTime = 0;
// long lastBrightnessUpdate = 0;
long enteredStandby = 0;
uint8_t activeCore = 0;

void enterStandby(){
  enteredStandby = millis();
  operation = standby;
}

bool testSerial(){
  long serialRequestedTime = millis();
  Serial.write(char(0));
  while(Serial.available() == 0){
    if(millis() - serialRequestedTime > SERIALTIMEOUT){
      recentSerialResponseTime = millis() - serialRequestedTime;
      return false;
    }
  }
  if (Serial.available() > 0){
    recentSerialResponseTime = millis() - serialRequestedTime;
    while (Serial.available() > 0) Serial.read();
    return true;
  } else {
    return false;
  }
}

uint16_t calculateHue(float usage){
  usage = max(0, usage);
  usage = min(100, usage);
  return (IDLEHUE - ((usage/100) * abs(CRANKHUE - IDLEHUE)));
}

uint8_t calculateBrightness(){
  long brightness = 0;
  for (uint8_t core=0; core < CORES; core++) brightness += easedCore[core].GetValue();
  brightness = constrain(brightness, 0, 100 * CORES);
  return map(brightness, 0, 100 * CORES, MINBRIGHTNESS, MAXBRIGHTNESS);
}

void updateDisplay(uint8_t core, uint16_t hue, bool updateStrip) {
  strip.setPixelColor(CORES - core, strip.ColorHSV(hue, 255, calculateBrightness()));
  if(updateStrip) strip.show();
}

float getUsage(uint8_t core){
  if(millis() - lastCoreUpdate[core] > REFRESHRATE){
    long serialRequestedTime = millis();
    Serial.write(core);
    while(Serial.available() == 0){
      if(millis() - serialRequestedTime > SERIALTIMEOUT){
        recentSerialResponseTime = millis() - serialRequestedTime;
        return 0;
      }
    }
    float usage = Serial.parseFloat();
    usage = constrain(usage, 0.00, 100.00);
    easedCore[core].SetSetpoint(usage);
    lastCoreUpdate[core] = millis();
    recentSerialResponseTime = millis() - serialRequestedTime;
  }
  while (Serial.available() > 0) Serial.read();
  return easedCore[core].GetValue();
}

void startupShow(){
  for (uint8_t core=0; core < CORES; core++) {
    strip.setPixelColor(core, 0);
  }
  long randomCore = random(CORES);
  int8_t step = 1;
  for (uint8_t brightness = 1; brightness > 0; brightness += step){
    if (brightness >= MINBRIGHTNESS) step = -1;
    strip.setPixelColor(randomCore, strip.ColorHSV(IDLEHUE, 255, brightness));
    strip.show();
    delay(10);
  }
}

void setup(){
  Serial.begin(115200);
  Serial.setTimeout(SERIALTIMEOUT);
  strip.begin();
  strip.setBrightness(MINBRIGHTNESS);
  strip.show();
  for (uint8_t core=0; core < CORES; core++){
    easedCore[core].SetMillisInterval(REFRESHRATE);
    easedCore[core].SetMode(LINEAR);
    easedCore[core].Init(0);
    lastCoreUpdate[core] = millis();
    delay(REFRESHRATE / CORES);
  }
  enterStandby();
}

void loop(){
  switch (operation){
  case sleep:
    strip.setPixelColor(activeCore, 0);
    strip.show();
    if (testSerial()) operation = run;
    break;

  case standby:
    startupShow();
    if (millis() - enteredStandby > SLEEPTIMEOUT) operation = sleep;
    if (testSerial()) operation = run;
    break;

  default:
    if ( recentSerialResponseTime > STANDBYTIMEOUT * 0.75 ) {
      enterStandby();
    } else {
      updateDisplay( activeCore, calculateHue( getUsage( activeCore ) ), true );
    }
    break;
  }
  activeCore++;
  if ( activeCore == CORES ) activeCore = 0;
}