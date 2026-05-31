#include <BTS7960.h>
#include <TimerMs.h>

#include "controller.h"
#include "sensor.h"
#include "switch.h"
#include "temperature_control.h"

const uint8_t TEMP_PIN = A3;

const uint8_t THROTTLE_PIN = A0;
const float THROTTLE_MIN = 0.58;
const float THROTTLE_MAX = 4.12;

const bool LOG_TEMPERATURE = false;
const bool LOG_SENSOR_RAW = false;
const bool LOG_POSITION = false;
const bool LOG_COMPRESSOR_STATUS = false;
const bool LOG_COOLER_INTERNAL = false;

const uint8_t PUMP_PIN = 7;
const uint8_t COOLER_PIN = 9;

const uint8_t PUMP_ON_TEMPERATURE = 35;
const uint8_t PUMP_OFF_TEMPERATURE = 20;

const uint8_t COOLER_ON_TEMPERATURE = 40;
const uint8_t COOLER_OFF_TEMPERATURE = 35;

TimerMs logMain(100, true, false);
TimerMs logTemp(10000, true, false);
TimerMs logPosition(10, true, false);
TimerMs logIdle(100, true, false);
TimerMs logOther(1000, true, false);
TimerMs heatCheck(1000, true, false);
TimerMs logPressure(10, true, false);

Sensor sensTemp(TEMP_PIN);

// Main throttle (uncontrolled)
Sensor sensThrottle(THROTTLE_PIN, THROTTLE_MIN, THROTTLE_MAX);

TemperatureControl tControlPump(PUMP_PIN, PUMP_ON_TEMPERATURE,
                                PUMP_OFF_TEMPERATURE, false);
TemperatureControl tControlCooler(COOLER_PIN, COOLER_ON_TEMPERATURE,
                                  COOLER_OFF_TEMPERATURE, true);

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
  cntrl.control(sensThrottle.position());

  if (cntrl.allowCompressor()) {
    compressor.poweron();
  } else {
    compressor.poweroff();
  }

  if (logMain.tick()) {
    log();
  }

  if (heatCheck.tick()) {
    int16_t temp = sensTemp.temperature();

    tControlPump.control(temp);
    tControlCooler.control(temp);

    cntrl.setTemperature(temp);
  }
}

void log() {
  if (LOG_TEMPERATURE && logTemp.tick()) {
    Serial.print(">sens temperature:");
    Serial.println(sensTemp.temperature());
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
      Serial.print(">sens temp voltage:");
      Serial.println(sensTemp.voltageTemp());

      Serial.print(">sens main throttle voltage:");
      Serial.println(sensThrottle.voltagePosition());
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
