#include <BTS7960.h>
#include <TimerMs.h>

#include "calibrate.h"
#include "consumptionControl.h"
#include "controller.h"
#include "emulator.h"
#include "errors.h"
#include "multiplexor.h"
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
const bool LOG_PRESSURE = true;
const bool LOG_SENSOR_RAW = false;
const bool LOG_POSITION = true;
const bool LOG_THROTTLE_RAW = false;
const bool LOG_THROTTLE_INTERNAL = false;
const bool LOG_EMULATOR = false;
const bool LOG_EMULATOR_INTERNAL = false;
const bool LOG_CONTROLLER_INTERNAL = false;
const bool LOG_CONSUMPTION = false;
const bool LOG_IDLE = true;
const bool LOG_COMPRESSOR_STATUS = false;

const uint8_t PUMP_PIN = 7;
const uint8_t COOLER_PIN = 9;

const uint8_t PUMP_ON_TEMPERATURE = 35;
const uint8_t PUMP_OFF_TEMPERATURE = 30;

const uint8_t COOLER_ON_TEMPERATURE = 45;
const uint8_t COOLER_OFF_TEMPERATURE = 23;

const uint8_t EN_PIN = 3;
const uint8_t R_PWM_PIN = 6;
const uint8_t L_PWM_PIN = 5;

const uint8_t BEEP_PIN = 10;

TimerMs poweroffCheck(100, true, false);
TimerMs logMain(100, true, false);
TimerMs logTemp(1000, true, false);
TimerMs logPressure(100, true, false);
TimerMs logPosition(100, true, false);
TimerMs logIdle(100, true, false);
TimerMs logOther(1000, true, false);
TimerMs heatCheck(1000, true, false);
TimerMs consumptionCheck(500, true, false);
TimerMs sensorsCheck(500, true, false);

unsigned long consumptionCheckLogged;
unsigned long sensorsCheckLogged;

PowerOff powerOff(POWEROFF_PIN);
PowerOffNotify powerOffNotify;

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
// 100.5408578 225
// 42.70882656 65
const float sensor1MapDelta = 53.15964446;
const float sensor1MapAngle = 0.3614501953;

// Map sensor 2 is differ from map1
// From customer:
// Delta -0.098
// Angle 65.8
// Map sensor 2 use vcc/gnd from ecu, so we have a little difference in pressure
// 100.5408578 333
// 42.70882656 153
const float sensor2MapDelta = -20.07039998;
const float sensor2MapAngle = 0.3212890624;

// Sensor 1 - in sensor, before throttle
Sensor sens1(TEMP1_PIN, MAP1_PIN, sensor1MapDelta, sensor1MapAngle);
// Set same temperature pin as in sens1 because temp from sensor 2 is not used
// by controller, it used by EBU block
// Sensor 2 - out sensor, after throttle
Sensor sens2(TEMP1_PIN, MAP2_PIN, sensor2MapDelta, sensor2MapAngle);

const bool USE_EMULATOR = false;

const uint8_t EMULATOR_VIRTUAL_PIN = 3;

Emulator emul1(EMULATOR_VIRTUAL_PIN, Emulator::BEFORE_THROTTLE, &muxRead);
Emulator emul2(EMULATOR_VIRTUAL_PIN, Emulator::AFTER_THROTTLE, &muxRead);

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
Calibrate clbr;

const uint8_t MUX_Z_PIN = A5;
const uint8_t MUX_E_PIN = 11;
const uint8_t MUX_S0_PIN = 12;
const uint8_t MUX_S1_PIN = 13;
const uint8_t MUX_GND_VIRTUAL_PIN = 0;

Multiplexor mux(MUX_Z_PIN, MUX_E_PIN, MUX_S0_PIN, MUX_S1_PIN,
                MUX_GND_VIRTUAL_PIN);

const uint8_t R_IS_L_IS_VIRTUAL_PIN = 1;
const uint8_t COMPRESSOR_CONSUMPTION_VIRTUAL_PIN = 2;

ConsumptionControl consumption(COMPRESSOR_CONSUMPTION_VIRTUAL_PIN, 100,
                               &muxRead);

void setup() { Serial.begin(115200); }

void loop() {
  if (USE_CALIBRATE) {
    loopCalibrate();
  } else {
    loopNormal();
  }
}

void loopCalibrate() {
  Serial.println("Test pump");

  unsigned long started = millis();

  tControlPump.control(PUMP_ON_TEMPERATURE);

  while (!timeout(started, 5000)) {
    Serial.print(">sens1 temperature:");
    Serial.println(sens1.temperature());

    delay(10);
  }

  Serial.println("Test cooler");

  started = millis();

  tControlCooler.control(COOLER_ON_TEMPERATURE);

  while (!timeout(started, 5000)) {
    Serial.print(">sens1 temperature:");
    Serial.println(sens1.temperature());

    delay(10);
  }

  tControlPump.poweroff();
  tControlCooler.poweroff();

  Serial.println("Test compressor");

  Serial.print("consumption compressor:");
  Serial.println(consumption.consumption());

  compressor.poweron();
  delay(1000);

  Serial.print("consumption compressor:");
  Serial.println(consumption.consumption());

  compressor.poweroff();
  delay(1000);

  Serial.print("consumption compressor:");
  Serial.println(consumption.consumption());

  Serial.println("Test poweroff button");

  for (uint8_t i = 0; i < 5; i++) {
    Serial.print("poweroff need:");
    Serial.println(powerOff.need());
    delay(1000);
  }

  Serial.println("Test sensors");

  Serial.print("sens1 temperature:");
  Serial.println(sens1.temperature());
  Serial.print("sens1 voltage temp:");
  Serial.println(sens1.voltageTemp());

  Serial.print("sens1 pressure (kpa):");
  Serial.println(sens1.pressure());
  Serial.print("sens1 pressure (mm):");
  Serial.println(sens1.pressureInMM());
  Serial.print("sens1 voltage map:");
  Serial.println(sens1.voltageMap());

  Serial.print("sens2 pressure (kpa):");
  Serial.println(sens2.pressure());
  Serial.print("sens2 pressure (mm):");
  Serial.println(sens2.pressureInMM());
  Serial.print("sens2 voltage map:");
  Serial.println(sens2.voltageMap());

  Serial.println("Throttle calibrate close");

  // need to read position to fill voltages
  thr.position();

  uint16_t maxPos1 = thr.voltagePosition1(), maxPos2 = thr.voltagePosition2(),
           minPos1 = thr.voltagePosition1(), minPos2 = thr.voltagePosition2();

  started = millis();

  while (!timeout(started, 10000)) {
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

    Serial.print(">throttle 1 voltage:");
    Serial.println(thr.voltagePosition1());

    Serial.print(">throttle 2 voltage:");
    Serial.println(thr.voltagePosition2());

    Serial.print(">throttle voltage sum:");
    Serial.println(thr.voltagePosition1() + thr.voltagePosition2());
  }

  Serial.println("Throttle calibrate open");

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

    Serial.print(">throttle 1 voltage:");
    Serial.println(thr.voltagePosition1());

    Serial.print(">throttle 2 voltage:");
    Serial.println(thr.voltagePosition2());

    Serial.print(">throttle voltage sum:");
    Serial.println(thr.voltagePosition1() + thr.voltagePosition2());
  }

  Serial.print("Min voltage 1 ");
  Serial.println(minPos1);
  Serial.print("Max voltage 1 ");
  Serial.println(maxPos1);
  Serial.print("Min voltage 2 ");
  Serial.println(minPos2);
  Serial.print("Max voltage 2 ");
  Serial.println(maxPos2);

  while (true) {
  }
}

void loopNormal() {
  if (!poweredoff) {
    if (USE_EMULATOR) {
      emul1.setRealThrottle(thr.position());
      emul2.setRealThrottle(thr.position());

      thr.hold(cntrl.position(emul1.pressure(), emul2.pressure()));
    } else {
      thr.hold(cntrl.position(sens1.pressure(), sens2.pressure()));
    }
  }

  thr.control();
  cntrl.control(sens1.pressure(), sens2.pressure());

  if (poweroffCheck.tick()) {
    if (powerOff.need() && !poweredoff) {
      Serial.println("Power off");

      poweredoff = true;

      powerOffNotify.poweroff();
      compressor.poweroff();
      thr.poweroff();

      tControlPump.poweroff();
      tControlCooler.poweroff();
    } else if (!powerOff.need() && poweredoff) {
      Serial.println("Power on");

      poweredoff = false;

      powerOffNotify.poweron();
      compressor.poweron();
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

  if (consumptionCheck.tick() && !poweredoff) {
    if (consumption.failed() && timeout(consumptionCheckLogged, 5000)) {
      consumptionCheckLogged = millis();

      Serial.print("Failed compressor: no consumption, current=");
      Serial.println(consumption.consumption());

      err.error(Errors::ERR_COMPRESSOR_CONSUMPTION);
    }
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
    if (USE_EMULATOR) {
      Serial.print(">sens1 emulated pressure (kpa):");
      Serial.println(emul1.pressure());

      Serial.print(">sens2 emulated pressure (kpa):");
      Serial.println(emul2.pressure());
    } else {
      Serial.print(">sens1 pressure (kpa):");
      Serial.println(sens1.pressure());

      Serial.print(">sens2 pressure (kpa):");
      Serial.println(sens2.pressure());
    }
  }

  if (LOG_POSITION && logPosition.tick()) {
    Serial.print(">throttle position:");
    Serial.println(thr.position());
  }

  if (LOG_IDLE && logIdle.tick()) {
    if (USE_EMULATOR) {
      Serial.print(">controller engine idle:");
      Serial.println(cntrl.isEngineIdle(emul2.pressure()));
    } else {
      Serial.print(">controller engine idle:");
      Serial.println(cntrl.isEngineIdle(sens2.pressure()));
    }
  }

  if (logOther.tick()) {
    if (LOG_COMPRESSOR_STATUS) {
      Serial.print(">compressor status: ");
      Serial.println(compressor.status());
    }

    if (LOG_SENSOR_RAW) {
      Serial.print(">sens1 pressure (mm):");
      Serial.println(sens1.pressureInMM());

      Serial.print(">sens2 pressure (mm):");
      Serial.println(sens2.pressureInMM());

      Serial.print(">sens1 voltage temp:");
      Serial.println(sens1.voltageTemp());

      Serial.print(">sens1 voltage map:");
      Serial.println(sens1.voltageMap());

      Serial.print(">sens2 voltage map:");
      Serial.println(sens2.voltageMap());
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

    if (LOG_CONTROLLER_INTERNAL) {
      Serial.print(">controller position:");
      Serial.println(cntrl.positionVal());
    }

    if (LOG_CONSUMPTION) {
      Serial.print(">consumption compressor:");
      Serial.println(consumption.consumption());
    }
  }
}

uint16_t muxRead(uint8_t pin) { return mux.read(pin); }

bool timeout(unsigned long start, uint16_t operationLimit) {
  if (millis() - start < operationLimit) {
    return false;
  }

  return true;
}
