#include <BTS7960.h>
#include <TimerMs.h>

#include "controller.h"
#include "errors.h"
#include "poweroff.h"
#include "poweroff_notify.h"
#include "sensor.h"
#include "switch.h"
#include "temperature_control.h"
#include "throttle.h"

const uint8_t POWEROFF_PIN = 2;

const uint8_t TEMP1_PIN = A4;

const uint8_t THROTTLE_PIN = A0;
const uint16_t THROTTLE_MIN = 90;
const uint16_t THROTTLE_MAX = 845;

const uint8_t THROTTLE_POSITION1_PIN = A1;
const uint8_t THROTTLE_POSITION2_PIN = A2;

const bool LOG_TEMPERATURE = true;
const bool LOG_SENSOR_RAW = false;
const bool LOG_POSITION = true;
const bool LOG_THROTTLE_RAW = false;
const bool LOG_THROTTLE_INTERNAL = false;
const bool LOG_CONTROLLER_INTERNAL = false;
const bool LOG_COMPRESSOR_STATUS = false;
const bool LOG_COOLER_INTERNAL = false;
const bool LOG_PRESSURE = true;

const uint8_t PUMP_PIN = 7;
const uint8_t COOLER_PIN = 9;

const uint8_t PUMP_ON_TEMPERATURE = 35;
const uint8_t PUMP_OFF_TEMPERATURE = 10;

const uint8_t COOLER_ON_TEMPERATURE = 40;
const uint8_t COOLER_OFF_TEMPERATURE = 35;

const uint8_t EN_PIN = 3;
const uint8_t R_PWM_PIN = 6;
const uint8_t L_PWM_PIN = 5;

const uint8_t BEEP_PIN = 10;

const uint8_t MAP2_PIN = A3;

TimerMs logMain(100, true, false);
TimerMs logTemp(10000, true, false);
TimerMs logPosition(100, true, false);
TimerMs logIdle(100, true, false);
TimerMs logOther(1000, true, false);
TimerMs heatCheck(1000, true, false);
TimerMs sensorsCheck(10000, true, false);
TimerMs logPressure(10, true, false);

unsigned long sensorsCheckLogged;

PowerOff powerOff(POWEROFF_PIN);
PowerOffNotify powerOffNotify;

// Sensor 1 - in sensor, before throttle
// Only temperature
Sensor sens1(TEMP1_PIN);

// Main throttle (uncontrolled)
Sensor sensThrottle(THROTTLE_PIN, THROTTLE_MIN, THROTTLE_MAX);

// delta = (voltage2 * pressure1 - voltage1 * pressure2) / (pressure2 -
// pressure1)
//
// angle = (pressure1 - pressure2) / (voltage1 - voltage2)
//
// Voltages must be native arduino integer values (0-1024)
// Pressure - is value in kpa
//
// nativeVoltage = voltage * 1024 / 5
// mm to kpa mm*0.1333224
//
// From customer:
// Delta -0.098
// Angle 65.8
//
// Map sensor 2 use vcc/gnd from ecu, so we have a little difference in pressure
//
// 100.5408578 333
// 42.70882656 153
const float sensor2MapDelta = -20.07039998;
const float sensor2MapAngle = 0.3212890624;

// Sensor 2 - out sensor, after throttle
// Only pressure
Sensor sens2(MAP2_PIN, sensor2MapDelta, sensor2MapAngle);

Throttle thr(THROTTLE_POSITION1_PIN, THROTTLE_POSITION2_PIN, EN_PIN, L_PWM_PIN,
             R_PWM_PIN);

TemperatureControl tControlPump(PUMP_PIN, PUMP_ON_TEMPERATURE,
                                PUMP_OFF_TEMPERATURE, false);
TemperatureControl tControlCooler(COOLER_PIN, COOLER_ON_TEMPERATURE,
                                  COOLER_OFF_TEMPERATURE, true);

Errors err(BEEP_PIN);

bool poweredoff = true;

Controller cntrl;

const uint8_t COMPRESSOR_PIN = 4;

Switch compressor(COMPRESSOR_PIN);

const bool USE_CALIBRATE = false;

void setup() { Serial.begin(115200); }

void loop() {
  if (USE_CALIBRATE) {
    loopCalibrate();
  } else {
    loopNormal();
  }
}

void loopCalibrate() {
  // need to read position to fill voltages
  thr.position();

  uint16_t maxPos1 = thr.voltagePosition1(), maxPos2 = thr.voltagePosition2(),
           minPos1 = thr.voltagePosition1(), minPos2 = thr.voltagePosition2();

  unsigned long started = millis();

  while (!timeout(started, 14000)) {
    // need to read position to fill voltages
    thr.position();

    if (thr.voltagePosition1() > maxPos1) {
      maxPos1 = thr.voltagePosition1();
    }
    if (thr.voltagePosition2() > maxPos2) {
      maxPos2 = thr.voltagePosition2();
    }
    if (thr.voltagePosition1() < minPos1) {
      minPos1 = thr.voltagePosition1();
    }
    if (thr.voltagePosition2() < minPos2) {
      minPos2 = thr.voltagePosition2();
    }

    thr.calibrateClose();
  }

  started = millis();

  while (!timeout(started, 14000)) {
    // need to read position to fill voltages
    thr.position();

    if (thr.voltagePosition1() > maxPos1) {
      maxPos1 = thr.voltagePosition1();
    }
    if (thr.voltagePosition2() > maxPos2) {
      maxPos2 = thr.voltagePosition2();
    }
    if (thr.voltagePosition1() < minPos1) {
      minPos1 = thr.voltagePosition1();
    }
    if (thr.voltagePosition2() < minPos2) {
      minPos2 = thr.voltagePosition2();
    }

    thr.calibrateOpen();
  }

  Serial.print("Min voltage 1: ");
  Serial.println(minPos1);
  Serial.print("Max voltage 1: ");
  Serial.println(maxPos1);
  Serial.print("Min voltage 2: ");
  Serial.println(minPos2);
  Serial.print("Max voltage 2: ");
  Serial.println(maxPos2);

  Serial.println("Now, change main throttle position by pressing gaz pedal to "
                 "minimum and maximum position");

  // need to read position to fill voltages
  sensThrottle.position();

  started = millis();

  uint16_t maxMainThrottle = sensThrottle.voltagePosition(),
           minMainThrottle = sensThrottle.voltagePosition();

  while (!timeout(started, 10000)) {
    // need to read position to fill voltages
    sensThrottle.position();

    if (sensThrottle.voltagePosition() < minMainThrottle) {
      minMainThrottle = sensThrottle.voltagePosition();
    }
    if (sensThrottle.voltagePosition() > maxMainThrottle) {
      maxMainThrottle = sensThrottle.voltagePosition();
    }
  }

  Serial.print("Main throttle min voltage: ");
  Serial.println(minMainThrottle);
  Serial.print("Main throttle max voltage: ");
  Serial.println(maxMainThrottle);

  while (true) {
  }
}

void loopNormal() {
  bool powerOffNeed = powerOff.need();

  if (powerOffNeed && !poweredoff) {
    Serial.println("Power off");

    poweredoff = true;

    powerOffNotify.poweroff();
    compressor.poweroff();
    thr.poweroff();

    tControlPump.poweroff();
    tControlCooler.poweroff();
    cntrl.poweroff();
    sensThrottle.poweroff();
    sens2.poweroff();
  } else if (!powerOffNeed && poweredoff) {
    Serial.println("Power on");

    poweredoff = false;

    powerOffNotify.poweron();
    compressor.poweron();
    cntrl.poweron();
    sensThrottle.poweron();
    sens2.poweron();
  }

  if (!poweredoff) {
    cntrl.control(sensThrottle.position());
    thr.hold(cntrl.position());

    if (cntrl.allowCompressor()) {
      compressor.poweron();
    } else {
      compressor.poweroff();
    }
  }

  thr.control();

  if (logMain.tick()) {
    log();
  }

  if (heatCheck.tick() && !poweredoff) {
    int16_t temp = sens1.temperature();

    tControlPump.control(temp);
    tControlCooler.control(temp);

    cntrl.setTemperature(temp);
  }

  if (sensorsCheck.tick() && !poweredoff) {
    if (!thr.sensorsOk() && timeout(sensorsCheckLogged, 5000)) {
      sensorsCheckLogged = millis();

      Serial.print("Failed throttle: no sensors ok, voltage1=");
      Serial.print(thr.voltagePosition1());
      Serial.print(", voltage2=");
      Serial.println(thr.voltagePosition2());

      err.error(Errors::ERR_THR_SENSORS);
    }
  }
}

void log() {
  if (LOG_TEMPERATURE && logTemp.tick()) {
    Serial.print(">sens1 temperature:");
    Serial.println(sens1.temperature());
  }

  if (LOG_PRESSURE && logPressure.tick()) {
    Serial.print(">sens2 pressure (kpa):");
    Serial.println(sens2.pressure());
  }

  if (LOG_POSITION && logPosition.tick()) {
    Serial.print(">throttle position:");
    Serial.println(thr.position());

    Serial.print(">main throttle position:");
    Serial.println(sensThrottle.position());
  }

  if (logOther.tick()) {
    if (LOG_COMPRESSOR_STATUS) {
      Serial.print(">compressor status: ");
      Serial.println(compressor.status());
    }

    if (LOG_SENSOR_RAW) {
      Serial.print(">sens1 voltage temp:");
      Serial.println(sens1.voltageTemp());

      Serial.print(">sens main throttle voltage position:");
      Serial.println(sensThrottle.voltagePosition());

      Serial.print(">sens2 voltage map:");
      Serial.println(sens2.voltagePressure());
    }

    if (LOG_THROTTLE_RAW) {
      Serial.print(">throttle 1 voltage:");
      Serial.println(thr.voltagePosition1());

      Serial.print(">throttle 2 voltage:");
      Serial.println(thr.voltagePosition2());

      Serial.print(">throttle voltage sum:");
      Serial.println(thr.voltagePosition1() + thr.voltagePosition2());
    }

    if (LOG_THROTTLE_INTERNAL) {
      Serial.print(">throttle status:");
      Serial.println(thr.status());

      Serial.print(">throttle speed:");
      Serial.println(thr.speed());

      Serial.print(">throttle hold status:");
      Serial.println(thr.holdStatus());

      Serial.print(">throttle hold position:");
      Serial.println(thr.holdPosition());

      Serial.print(">throttle hold reached:");
      Serial.println(thr.holdReached());

      Serial.print(">throttle hold direction:");
      Serial.println(thr.holdDirection());
    }

    if (LOG_CONTROLLER_INTERNAL) {
      Serial.print(">controller position:");
      Serial.println(cntrl.positionVal());
    }

    if (LOG_COOLER_INTERNAL) {
      Serial.print(">cooler regulator position:");
      Serial.println(tControlCooler.regulatorValue());
    }
  }
}

bool timeout(unsigned long start, uint16_t operationLimit) {
  if (millis() - start < operationLimit) {
    return false;
  }

  return true;
}
