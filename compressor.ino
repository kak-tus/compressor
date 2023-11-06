#include <TimerMs.h>

#include "poweroff.h"
#include "poweroff_notify.h"
#include "sensor.h"
#include "throttle.h"

const uint8_t POWEROFFPIN = 2;

const uint8_t TEMP1PIN = A4;
const uint8_t MAP1PIN = A0;

const uint8_t MAP2PIN = A3;

const uint8_t THROTTLEPOSITION1PIN = A1;
const uint8_t THROTTLEPOSITION2PIN = A2;

const int LOOPDELAY = 10;

const bool LOGSENSOR = false;
const bool LOGSENSORRAW = false;
const bool LOGTHROTTLE = true;
const bool LOGTHROTTLERAW = true;

TimerMs poweroffCheck(100, 1, 0);
TimerMs logCheck(1000, 1, 0);

PowerOff powerOff(POWEROFFPIN);
PowerOffNotify powerOffNotify;

Sensor sens1(TEMP1PIN, MAP1PIN);
// Set same temperature pin as in sens1 because temp from sensor 2 is not used
// by controller, it used by EBU block
Sensor sens2(TEMP1PIN, MAP2PIN);

Throttle thr(THROTTLEPOSITION1PIN, THROTTLEPOSITION2PIN);

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
    log();
  }

  delay(LOOPDELAY);
}

void log() {
  if (LOGSENSOR) {
    Serial.print(">temperature:");
    Serial.println(sens1.temperature());

    Serial.print(">sens1 pressure (pa):");
    Serial.println(sens1.pressure());

    Serial.print(">sens1 pressure (mm):");
    Serial.println(sens1.pressureInMM());

    Serial.print(">sens2 pressure (pa):");
    Serial.println(sens2.pressure());

    Serial.print(">sens2 pressure (mm):");
    Serial.println(sens2.pressureInMM());
  }

  if (LOGSENSORRAW) {
    Serial.print(">sens1 voltage temp:");
    Serial.println(sens1.voltageTemp());

    Serial.print(">sens1 voltage map:");
    Serial.println(sens1.voltageMap());

    Serial.print(">sens2 voltage map:");
    Serial.println(sens2.voltageMap());
  }

  if (LOGTHROTTLE) {
    Serial.print(">pos1:");
    Serial.println(thr.position1());

    Serial.print(">pos2:");
    Serial.println(thr.position2());
  }

  if (LOGTHROTTLERAW) {
    Serial.print(">throttle 1 voltage:");
    Serial.println(thr.voltagePosition1());

    Serial.print(">throttle 2 voltage:");
    Serial.println(thr.voltagePosition2());
  }
}
