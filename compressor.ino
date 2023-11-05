#include <TimerMs.h>

#include "poweroff.h"
#include "poweroff_notify.h"
#include "sensor.h"

const uint8_t POWEROFFPIN = 2;
const uint8_t TEMP1PIN = A4;
const uint8_t MAP1PIN = A0;

const int LOOPDELAY = 10;

TimerMs poweroffCheck(100, 1, 0);
TimerMs logCheck(500, 1, 0);

PowerOff powerOff(POWEROFFPIN);
PowerOffNotify powerOffNotify;

Sensor sens1(TEMP1PIN, MAP1PIN);

void setup() { Serial.begin(9600); }

void loop() {
  if (poweroffCheck.tick()) {
    if (powerOff.need()) {
      powerOffNotify.poweroff();
    } else {
      powerOffNotify.poweron();
    }
  }

  if (logCheck.tick()) {
    Serial.print(">temperature:");
    Serial.println(sens1.temperature());

    float pressure = sens1.pressure();
    Serial.print(">pressure (pa):");
    Serial.println(pressure);

    Serial.print(">pressure (mm):");
    Serial.println(sens1.pressureInMM());

    Serial.print(">voltage temp:");
    Serial.println(sens1.voltageTemp());

    Serial.print(">voltage map:");
    Serial.println(sens1.voltageMap());
  }

  delay(LOOPDELAY);
}
