#include <TimerMs.h>

#include "poweroff.h"
#include "poweroff_notify.h"
#include "sensor.h"

const uint8_t POWEROFFPIN = 2;
const uint8_t TEMP1PIN = A4;

const int LOOPDELAY = 10;

TimerMs poweroffCheck(100, 1, 0);
TimerMs temperatureCheck(500, 1, 0);

PowerOff powerOff(POWEROFFPIN);
PowerOffNotify powerOffNotify;

Sensor sens2(TEMP1PIN);

void setup() { Serial.begin(9600); }

void loop() {
  if (poweroffCheck.tick()) {
    if (powerOff.need()) {
      powerOffNotify.poweroff();
    } else {
      powerOffNotify.poweron();
    }
  }

  if (temperatureCheck.tick()) {
    Serial.print(">temperature:");
    Serial.println(sens2.temperature());
  }

  delay(LOOPDELAY);
}
