#include <BTS7960.h>
#include <TimerMs.h>

#include "errors.h"
#include "poweroff.h"
#include "poweroff_notify.h"
#include "sensor.h"
#include "temperature_control.h"
#include "throttle.h"

const uint8_t POWEROFF_PIN = 2;

const uint8_t TEMP1_PIN = A4;
const uint8_t MAP1_PIN = A0;

const uint8_t MAP2_PIN = A3;

const uint8_t THROTTLE_POSITION1_PIN = A1;
const uint8_t THROTTLE_POSITION2_PIN = A2;

const bool LOG_SENSOR = false;
const bool LOG_SENSOR_RAW = false;
const bool LOG_THROTTLE = true;
const bool LOG_THROTTLE_RAW = false;

const uint8_t PUMP_PIN = 7;
const uint8_t COOLER_PIN = 8;

const float PUMP_ON_TEMPERATURE = 30.0;
const float PUMP_OFF_TEMPERATURE = 20.0;

const float COOLER_ON_TEMPERATURE = 35.0;
const float COOLER_OFF_TEMPERATURE = 25.0;

const uint8_t EN_PIN = 3;
const uint8_t R_PWM_PIN = 6;
const uint8_t L_PWM_PIN = 5;

const uint8_t BEEP_PIN = 10;

TimerMs poweroffCheck(100, 1, 0);
TimerMs logCheck(1000, 1, 0);
TimerMs heatCheck(1000, 1, 0);

PowerOff powerOff(POWEROFF_PIN);
PowerOffNotify powerOffNotify;

Sensor sens1(TEMP1_PIN, MAP1_PIN);
// Set same temperature pin as in sens1 because temp from sensor 2 is not used
// by controller, it used by EBU block
Sensor sens2(TEMP1_PIN, MAP2_PIN);

Throttle thr(THROTTLE_POSITION1_PIN, THROTTLE_POSITION2_PIN, EN_PIN, L_PWM_PIN,
             R_PWM_PIN);

TemperatureControl tControlPump(PUMP_PIN, PUMP_ON_TEMPERATURE,
                                PUMP_OFF_TEMPERATURE);
TemperatureControl tControlCooler(COOLER_PIN, COOLER_ON_TEMPERATURE,
                                  COOLER_OFF_TEMPERATURE);

Errors err(BEEP_PIN);

bool failed = false;

void setup() {
  Serial.begin(9600);
  thr.check();
  thr.hold(50);
}

void loop() {
  if (!thr.control()) {
    if (!failed) {
      failed = true;

      tControlPump.poweroff();
      tControlCooler.poweroff();

      Serial.print("Fail state: ");
      Serial.println(thr.failStateCode());

      err.error(thr.failStateCode());
    }
  }

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

  if (heatCheck.tick() && !failed) {
    float temp = sens1.temperature();
    tControlPump.control(temp);
    tControlCooler.control(temp);
  }
}

void log() {
  if (LOG_SENSOR) {
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

  if (LOG_SENSOR_RAW) {
    Serial.print(">sens1 voltage temp:");
    Serial.println(sens1.voltageTemp());

    Serial.print(">sens1 voltage map:");
    Serial.println(sens1.voltageMap());

    Serial.print(">sens2 voltage map:");
    Serial.println(sens2.voltageMap());
  }

  if (LOG_THROTTLE) {
    Serial.print(">pos1:");
    Serial.println(thr.position1());

    Serial.print(">pos2:");
    Serial.println(thr.position2());
  }

  if (LOG_THROTTLE_RAW) {
    Serial.print(">throttle 1 voltage:");
    Serial.println(thr.voltagePosition1());

    Serial.print(">throttle 2 voltage:");
    Serial.println(thr.voltagePosition2());
  }
}
