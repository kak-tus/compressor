#include <BTS7960.h>
#include <TimerMs.h>

#include "controller.h"
#include "poweroff.h"
#include "poweroff_notify.h"
#include "sensor.h"
#include "switch.h"
#include "temperature_control.h"

const uint8_t POWEROFF_PIN = 2;

const uint8_t TEMP1_PIN = A4;

const uint8_t THROTTLE_PIN = A0;
const uint16_t THROTTLE_MIN = 90;
const uint16_t THROTTLE_MAX = 845;

const bool LOG_TEMPERATURE = true;
const bool LOG_SENSOR_RAW = false;
const bool LOG_POSITION = true;
const bool LOG_COMPRESSOR_STATUS = false;
const bool LOG_COOLER_INTERNAL = false;
const bool LOG_PRESSURE = true;

const uint8_t PUMP_PIN = 7;
const uint8_t COOLER_PIN = 9;

const uint8_t PUMP_ON_TEMPERATURE = 35;
const uint8_t PUMP_OFF_TEMPERATURE = 20;

const uint8_t COOLER_ON_TEMPERATURE = 40;
const uint8_t COOLER_OFF_TEMPERATURE = 35;

const uint8_t MAP2_PIN = A3;

TimerMs logMain(100, true, false);
TimerMs logTemp(10000, true, false);
TimerMs logPosition(10, true, false);
TimerMs logIdle(100, true, false);
TimerMs logOther(1000, true, false);
TimerMs heatCheck(1000, true, false);
TimerMs logPressure(10, true, false);

PowerOff powerOff(POWEROFF_PIN);
PowerOffNotify powerOffNotify;

// Sensor 1 after compressor
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

// Sensor 2 - main sensor, after compressor
// Only pressure
Sensor sens2(MAP2_PIN, sensor2MapDelta, sensor2MapAngle);

TemperatureControl tControlPump(PUMP_PIN, PUMP_ON_TEMPERATURE,
                                PUMP_OFF_TEMPERATURE, false);
TemperatureControl tControlCooler(COOLER_PIN, COOLER_ON_TEMPERATURE,
                                  COOLER_OFF_TEMPERATURE, true);

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
  Serial.println("Now, change main throttle position by pressing gaz pedal to "
                 "minimum and maximum position");

  // need to read position to fill voltages
  sensThrottle.position();

  unsigned long started = millis();

  uint16_t maxMainThrottle = sensThrottle.voltagePosition(),
           minMainThrottle = sensThrottle.voltagePosition();

  while (!timeout(started, 5000)) {
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

    if (cntrl.allowCompressor()) {
      compressor.poweron();
    } else {
      compressor.poweroff();
    }
  }

  if (logMain.tick()) {
    log();
  }

  if (heatCheck.tick() && !poweredoff) {
    int16_t temp = sens1.temperature();

    tControlPump.control(temp);
    tControlCooler.control(temp);

    cntrl.setTemperature(temp);
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
