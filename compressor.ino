#include <BTS7960.h>
#include <TimerMs.h>

#include "calibrate.h"
#include "controller.h"
#include "emulator.h"
#include "errors.h"
#include "poweroff.h"
#include "poweroff_notify.h"
#include "sensor.h"
#include "switch.h"
#include "temperature_control.h"
#include "throttle.h"

const uint8_t POWEROFF_PIN = 2;

const uint8_t TEMP1_PIN = A4;
const uint8_t MAP1_PIN = A0;

const uint8_t MAP2_PIN = A3;

const uint8_t THROTTLE_POSITION1_PIN = A1;
const uint8_t THROTTLE_POSITION2_PIN = A2;

const bool LOG_TEMPERATURE = false;
const bool LOG_SENSOR = false;
const bool LOG_SENSOR_RAW = false;
const bool LOG_THROTTLE = true;
const bool LOG_THROTTLE_RAW = true;
const bool LOG_THROTTLE_INTERNAL = false;
const bool LOG_EMULATOR = false;
const bool LOG_EMULATOR_INTERNAL = false;
const bool LOG_CONTROLLER = false;

const uint8_t PUMP_PIN = 7;
const uint8_t COOLER_PIN = 8;

const uint8_t PUMP_ON_TEMPERATURE = 20;
const uint8_t PUMP_OFF_TEMPERATURE = 19;

const uint8_t COOLER_ON_TEMPERATURE = 21;
const uint8_t COOLER_OFF_TEMPERATURE = 20;

const uint8_t EN_PIN = 3;
const uint8_t R_PWM_PIN = 6;
const uint8_t L_PWM_PIN = 5;

const uint8_t BEEP_PIN = 10;

TimerMs poweroffCheck(100, true, false);
TimerMs logCheck(1000, true, false);
TimerMs heatCheck(1000, true, false);

PowerOff powerOff(POWEROFF_PIN);
PowerOffNotify powerOffNotify;

const int16_t sensor1MapCorrection = 0;
const int16_t sensor2MapCorrection = 1400;

// Sensor 1 - in sensor, before throttle
Sensor sens1(TEMP1_PIN, MAP1_PIN, sensor1MapCorrection);
// Set same temperature pin as in sens1 because temp from sensor 2 is not used
// by controller, it used by EBU block
// Sensor 2 - out sensor, after throttle
Sensor sens2(TEMP1_PIN, MAP2_PIN, sensor2MapCorrection);

const bool USE_EMULATOR = true;

const uint8_t EMULATOR_PIN = A5;

Emulator emul1(EMULATOR_PIN, Emulator::BEFORE_THROTTLE);
Emulator emul2(EMULATOR_PIN, Emulator::AFTER_THROTTLE);

Throttle thr(THROTTLE_POSITION1_PIN, THROTTLE_POSITION2_PIN, EN_PIN, L_PWM_PIN,
             R_PWM_PIN);

TemperatureControl tControlPump(PUMP_PIN, PUMP_ON_TEMPERATURE,
                                PUMP_OFF_TEMPERATURE);
TemperatureControl tControlCooler(COOLER_PIN, COOLER_ON_TEMPERATURE,
                                  COOLER_OFF_TEMPERATURE);

Errors err(BEEP_PIN);

bool failed = false;
bool poweredoff = false;

Controller cntrl;

const uint8_t COMPRESSOR_PIN = 4;

Switch compressor(COMPRESSOR_PIN);

const bool USE_CALIBRATE = false;
Calibrate clbr;

void setup() {
  Serial.begin(115200);

  if (!USE_EMULATOR) {
    cntrl.setNormalPressure(sens1.pressure());
  }

  thr.check();
}

void loop() {
  if (!failed && !poweredoff) {
    if (USE_EMULATOR) {
      emul1.setRealThrottle(thr.position());
      emul2.setRealThrottle(thr.position());

      if (USE_CALIBRATE) {
        thr.hold(clbr.position());
      } else {
        thr.hold(cntrl.position(emul1.pressure(), emul2.pressure()));
      }
    } else {
      if (USE_CALIBRATE) {
        thr.hold(clbr.position());
      } else {
        thr.hold(cntrl.position(sens1.pressure(), sens2.pressure()));
      }
    }
  }

  if (!thr.control()) {
    if (!failed) {
      failed = true;

      tControlPump.poweroff();
      tControlCooler.poweroff();

      compressor.poweroff();

      Serial.print("Fail state: ");
      Serial.println(thr.failStateCode());

      err.error(thr.failStateCode());
    }
  }

  if (poweroffCheck.tick()) {
    if (powerOff.need() && !poweredoff) {
      poweredoff = true;

      powerOffNotify.poweroff();

      compressor.poweroff();

      thr.poweroff();

      tControlPump.poweroff();
      tControlCooler.poweroff();
    } else if (!powerOff.need() && poweredoff) {
      poweredoff = false;

      powerOffNotify.poweron();
    }
  }

  if (logCheck.tick()) {
    log();
  }

  if (heatCheck.tick() && !failed && !poweredoff) {
    int16_t temp = sens1.temperature();

    tControlPump.control(temp);
    tControlCooler.control(temp);

    cntrl.setTemperature(temp);
  }
}

void log() {
  if (LOG_TEMPERATURE) {
    Serial.print(">temperature:");
    Serial.println(sens1.temperature());
  }

  if (LOG_SENSOR) {
    if (USE_EMULATOR) {
      Serial.print(">sens1 emulated pressure (pa):");
      Serial.println(emul1.pressure());

      Serial.print(">sens2 emulated pressure (pa):");
      Serial.println(emul2.pressure());
    } else {
      Serial.print(">sens1 pressure (pa):");
      Serial.println(sens1.pressure());

      Serial.print(">sens2 pressure (pa):");
      Serial.println(sens2.pressure());
    }
  }

  if (LOG_SENSOR_RAW) {
    Serial.print(">sens1 pressure (mm):");
    Serial.println(sens1.pressureInMM());

    Serial.print(">sens2 pressure (mm):");
    Serial.println(sens2.pressureInMM());

    Serial.print(">voltage temp:");
    Serial.println(sens1.voltageTemp());

    Serial.print(">sens1 voltage map:");
    Serial.println(sens1.voltageMap());

    Serial.print(">sens2 voltage map:");
    Serial.println(sens2.voltageMap());
  }

  if (LOG_THROTTLE) {
    Serial.print(">throttle position:");
    Serial.println(thr.position());
  }

  if (LOG_THROTTLE_RAW) {
    Serial.print(">throttle 1 voltage:");
    Serial.println(thr.voltagePosition1());

    Serial.print(">throttle 2 voltage:");
    Serial.println(thr.voltagePosition2());
  }

  if (LOG_THROTTLE_INTERNAL) {
    Serial.print(">throttle status:");
    Serial.println(thr.status());

    Serial.print(">throttle speed:");
    Serial.println(thr.speed());

    Serial.print(">throttle hold status:");
    Serial.println(thr.holdStatus());

    Serial.print(">throttle hold speed:");
    Serial.println(thr.holdSpeed());

    Serial.print(">throttle hold position start:");
    Serial.println(thr.holdPositionStart());

    Serial.print(">throttle hold position want:");
    Serial.println(thr.holdPositionWant());

    Serial.print(">throttle hold position final:");
    Serial.println(thr.holdPositionFinal());

    Serial.print(">throttle hold reached:");
    Serial.println(thr.holdReached());

    Serial.print(">throttle hold direction:");
    Serial.println(thr.holdDirection());

    Serial.print(">throttle hold start at:");
    Serial.println(thr.holdStartAt());
  }

  if (USE_EMULATOR && LOG_EMULATOR) {
    Serial.print(">emulator throttle position:");
    Serial.println(emul1.throttle());

    Serial.print(">emulator rpm:");
    Serial.println(emul1.rpm());
  }

  if (USE_EMULATOR && LOG_EMULATOR_INTERNAL) {
    Serial.print(">emulator efficiency:");
    Serial.println(emul1.efficiency());

    Serial.print(">emulator pressure clear:");
    Serial.println(emul1.pressureClear());

    Serial.print(">emulator voltage:");
    Serial.println(emul1.voltage());
  }

  if (LOG_CONTROLLER) {
    Serial.print(">controller pressure 1 want:");
    Serial.println(cntrl.pressure1Want());

    Serial.print(">controller position:");
    Serial.println(cntrl.positionVal());

    Serial.print(">controller direction:");
    Serial.println(cntrl.direction());

    Serial.print(">controller reached:");
    Serial.println(cntrl.reached());

    if (USE_EMULATOR) {
      uint32_t pressure2 = emul2.pressure();

      Serial.print(">controller is pressure2 up:");
      Serial.println(cntrl.isPressure2Up(pressure2));

      Serial.print(">controller is pressure2 down:");
      Serial.println(cntrl.isPressure2Down(pressure2));
    } else {
      uint32_t pressure2 = sens2.pressure();

      Serial.print(">controller is pressure2 up:");
      Serial.println(cntrl.isPressure2Up(pressure2));

      Serial.print(">controller is pressure2 down:");
      Serial.println(cntrl.isPressure2Down(pressure2));
    }
  }
}
