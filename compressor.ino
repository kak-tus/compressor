#include <TimerMs.h>

#include "poweroff.h"
#include "poweroff_notify.h"

const uint8_t POWEROFFPIN = 2;
const int LOOPDELAY = 10;

TimerMs poweroffCheck(100, 1, 0);

PowerOff powerOff(POWEROFFPIN);
PowerOffNotify powerOffNotify;

void setup() {
  pinMode(POWEROFFPIN, INPUT);
}

void loop() {
  if (poweroffCheck.tick()) {
    if (powerOff.need()) {
      powerOffNotify.poweroff();
    } else {
      powerOffNotify.poweron();
    }
  }

  delay(LOOPDELAY);
}
