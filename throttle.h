#include <BTS7960.h>

#define PID_INTEGER
#include <GyverPID.h>

#include "errors.h"

class Throttle {
public:
  Throttle(uint8_t position1Pin, uint8_t position2Pin, uint8_t en, uint8_t lPwm,
           uint8_t rPwm)
      : _pos1Pin(position1Pin), _pos2Pin(position2Pin), _motor(en, lPwm, rPwm) {
    pinMode(_pos1Pin, INPUT);
    pinMode(_pos2Pin, INPUT);

    regulator.setMode(ON_ERROR);
  }

  uint8_t position() {
    _voltagePos1 = analogRead(_pos1Pin);

    if (_voltagePos1 < minVoltageSens1 + delta) {
      _pos1 = 0;
    } else if (_voltagePos1 > maxVoltageSens1 - delta) {
      _pos1 = 100;
    } else {
      _pos1 = (uint32_t)(_voltagePos1 - minVoltageSens1) * 100 /
              (maxVoltageSens1 - minVoltageSens1);
    }

    _voltagePos2 = analogRead(_pos2Pin);

    return _pos1;
  }

  uint16_t voltagePosition1() { return _voltagePos1; }
  uint16_t voltagePosition2() { return _voltagePos2; }

  uint8_t status() { return uint8_t(_status); }
  uint8_t speed() { return _currentSpeed; }
  uint8_t holdStatus() { return uint8_t(_holdStatus); }
  uint8_t holdPosition() { return regulator.setpoint; }
  bool holdReached() { return _holdReached; }
  uint8_t holdDirection() { return uint8_t(_holdDirection); }

  void check() {
    _motor.Enable();

    regulator.input = position();

    setRegulator(100, OPEN);
    regulator.setDt(controlTimeout);

    unsigned long start = millis();

    while (position() < 100) {
      int val = regulator.getResultTimer();
      open(val);

      if (!sensorsStartupOk()) {
        _fail(Errors::ERR_THR_CHECK_1);
        return;
      }

      if (timeout(start, _operationLimit)) {
        _fail(Errors::ERR_THR_CHECK_2);
        return;
      }
    }

    _motor.Stop();
    delay(1000);

    setRegulator(70, CLOSE);
    regulator.setDt(controlTimeout);

    start = millis();

    while (position() >= 70) {
      int val = regulator.getResultTimer();
      close(val);

      if (!sensorsStartupOk()) {
        _fail(Errors::ERR_THR_CHECK_3);
        return;
      }

      if (timeout(start, _operationLimit)) {
        _fail(Errors::ERR_THR_CHECK_4);
        return;
      }
    }

    _motor.Stop();
    delay(1000);

    setRegulator(100, OPEN);
    regulator.setDt(controlTimeout);

    start = millis();

    while (position() < 100) {
      int val = regulator.getResultTimer();
      open(val);

      if (!sensorsStartupOk()) {
        _fail(Errors::ERR_THR_CHECK_5);
        return;
      }

      if (timeout(start, _operationLimit)) {
        _fail(Errors::ERR_THR_CHECK_6);
        return;
      }
    }

    _motor.Disable();
  }

  bool control() {
    if (_failed) {
      return false;
    }

    if (!timeout(_controlTime, controlTimeout)) {
      return true;
    }

    _controlTime = millis();

    switch (_holdStatus) {
    case IN_HOLD:
      return controlHold();
    case IN_POWEROFF:
      return controlPoweroff();
    default:
      if (timeout(_openedChecked, openedCheckedTimeout)) {
        // Periodically check that throttle wasn't uncontrolled open
        _openedChecked = millis();

        uint8_t _currPos = position();

        if (!sensorsOk()) {
          _fail(Errors::ERR_THR_SENSORS_1);
          return false;
        }

        if (_currPos < 100) {
          poweroff();
        }
      }

      return true;
    }
  }

  uint8_t failStateCode() { return _failStateCode; }

  void hold(uint8_t pos) {
    if (_failed) {
      return;
    }

    if (_holdStatus != IN_HOLD) {
      _holdStatus = IN_HOLD;
      _motor.Enable();

      uint8_t _currPos = position();

      if (!sensorsOk()) {
        _fail(Errors::ERR_THR_SENSORS_2);
        return;
      }

      if (_currPos < pos) {
        _holdDirection = OPEN;
        _holdReached = false;
        setRegulator(pos, OPEN);
      } else if (_currPos > pos) {
        _holdDirection = CLOSE;
        _holdReached = false;
        setRegulator(pos, CLOSE);
      } else {
        _holdReached = true;
        stop();
      }

      return;
    }

    if (pos == regulator.setpoint) {
      return;
    }

    uint8_t _currPos = position();

    if (!sensorsOk()) {
      _fail(Errors::ERR_THR_SENSORS_3);
      return;
    }

    if (pos == _currPos) {
      _holdReached = true;
      stop();
      regulator.setpoint = pos;
      return;
    }

    if (_holdReached) {
      _holdReached = false;

      if (_currPos < pos) {
        _holdDirection = OPEN;
        setRegulator(pos, OPEN);
      } else {
        _holdDirection = CLOSE;
        setRegulator(pos, CLOSE);
      }

      return;
    }

    if (_holdDirection == OPEN) {
      if (_currPos < pos) {
        // Continue open
        _holdReached = false;
        regulator.setpoint = pos;
      } else {
        // Direction changed
        _holdDirection = CLOSE;
        _holdReached = false;
        setRegulator(pos, CLOSE);
      }
    } else if (_holdDirection == CLOSE) {
      if (_currPos > pos) {
        // Continue close
        _holdReached = false;
        regulator.setpoint = pos;
      } else {
        // Direction changed
        _holdDirection = OPEN;
        _holdReached = false;
        setRegulator(pos, OPEN);
      }
    }
  }

  void poweroff() {
    if (_failed) {
      return;
    }

    if (_holdStatus != IN_HOLD) {
      return;
    }

    _holdStatus = IN_POWEROFF;
    _motor.Enable();
    setRegulator(100, OPEN);
  }

  void calibrateClose() {
    _motor.Enable();

    _motor.TurnLeft(speedMaxClose);
  }

  void calibrateOpen() {
    _motor.Enable();

    _motor.TurnRight(speedMaxOpen);
  }

  void calibrateStop() { _motor.Stop(); }

private:
  // Call only after position() call
  bool sensorsOk() {
    // Use stored voltage values - assume that sensorsOk called only after
    // position()
    if (abs((int16_t)1024 - (int16_t)(_voltagePos1 + _voltagePos2)) > sensorsOkVoltageDelta) {
      return false;
    }

    return true;
  }

  // Call only after position() call
  bool sensorsStartupOk() {
    // Use stored voltage values - assume that sensorsStartupOk called only after
    // position()
    if (abs((int16_t)1024 - (int16_t)(_voltagePos1 + _voltagePos2)) > sensorsStartupOkVoltageDelta) {
      return false;
    }

    return true;
  }

  void _fail(uint8_t code) {
    Serial.print("Fail data:");
    Serial.print(" pos1=");
    Serial.print(_pos1);
    Serial.print(" voltagePos1=");
    Serial.print(_voltagePos1);
    Serial.print(" voltagePos2=");
    Serial.print(_voltagePos2);
    Serial.println("");

    _failed = true;
    _failStateCode = code;

    syncOpen();

    _motor.Disable();
  }

  void syncOpen() {
    unsigned long start = millis();

    setRegulator(100, OPEN);
    regulator.setDt(controlTimeout);

    while (position() < 100 && !timeout(start, _operationLimit)) {
      int val = regulator.getResultTimer();
      open(val);
    }
  }

  void close(uint8_t speed) {
    if (_status == IN_CLOSE) {
      if (_currentSpeed != speed) {
        _currentSpeed = speed;
        _motor.TurnLeft(speed);
      }

      return;
    }

    _status = IN_CLOSE;
    _currentSpeed = speed;

    _motor.TurnLeft(speed);
  }

  void open(uint8_t speed) {
    if (_status == IN_OPEN) {
      if (_currentSpeed != speed) {
        _currentSpeed = speed;
        _motor.TurnRight(speed);
      }

      return;
    }

    _status = IN_OPEN;
    _currentSpeed = speed;

    _motor.TurnRight(speed);
  }

  void stop() {
    if (_status == IN_STOP) {
      return;
    }

    _status = IN_STOP;
    _motor.Stop();
  }

  bool timeout(unsigned long start, uint16_t operationLimit) {
    if (millis() - start < operationLimit) {
      return false;
    }

    return true;
  }

  bool controlHold() {
    if (_failed) {
      return false;
    }

    uint8_t pos = position();

    if (!sensorsOk()) {
      _fail(Errors::ERR_THR_SENSORS_4);
      return false;
    }

    if (_holdReached) {
      // TODO Add possible corrections
    } else {
      if (_holdDirection == OPEN) {
        if (pos < regulator.setpoint) {
          int val = regulator.getResultNow();
          open(val);
        } else {
          _holdReached = true;
          stop();
        }
      } else if (_holdDirection == CLOSE) {
        if (pos > regulator.setpoint) {
          int val = regulator.getResultNow();
          close(val);
        } else {
          _holdReached = true;
          stop();
        }
      }
    }

    return true;
  }

  bool controlPoweroff() {
    if (_failed) {
      return false;
    }

    uint8_t pos = position();

    if (!sensorsOk()) {
      _fail(Errors::ERR_THR_SENSORS_5);
      return false;
    }

    if (pos < 100) {
      int val = regulator.getResultNow();
      open(val);
    } else {
      _motor.Disable();
      _holdStatus = holdStatusType(0);
      _holdReached = true;
    }

    return true;
  }

  const uint8_t _pos1Pin, _pos2Pin;

  uint16_t _voltagePos1, _voltagePos2;
  uint8_t _pos1;

  // Use delta to guarantee get 100% open and 0% close
  const uint8_t delta = 5;

  // Max voltage as native integer data
  uint16_t minVoltageSens1 = 160;
  uint16_t maxVoltageSens1 = 890;
  uint16_t minVoltageSens2 = 120;
  uint16_t maxVoltageSens2 = 840;

const uint8_t sensorsStartupOkVoltageDelta = 50;
  const uint8_t sensorsOkVoltageDelta = 30;

  BTS7960 _motor;

  enum statusType { IN_OPEN = 1, IN_CLOSE, IN_STOP };
  statusType _status;

  uint8_t _currentSpeed;

  // Open faster, then close
  // to do blowoff
  const uint8_t speedMinOpen = 20;
  const uint8_t speedMaxOpen = 35;
  const uint8_t speedMinClose = 20;
  const uint8_t speedMaxClose = 35;

  bool _failed = false;

  const uint16_t _operationLimit = 2000;

  uint8_t _failStateCode = 0;

  enum holdStatusType { IN_HOLD = 1, IN_POWEROFF };
  holdStatusType _holdStatus;

  bool _holdReached;

  enum holdDirectionType { OPEN, CLOSE };
  holdDirectionType _holdDirection;

  unsigned long _openedChecked;
  const uint16_t openedCheckedTimeout = 5000;

  unsigned long _controlTime;
  const uint16_t controlTimeout = 10;

  GyverPID regulator = GyverPID(1, 0.1, 0.1, controlTimeout);

  void setRegulator(uint8_t pos, holdDirectionType direction) {
    regulator.setpoint = pos;
    regulator.output = 0;
    regulator.getResultNow();

    if (direction == OPEN) {
      regulator.setLimits(speedMinOpen, speedMaxOpen);
      regulator.setDirection(NORMAL);
    } else {
      regulator.setLimits(speedMinClose, speedMaxClose);
      regulator.setDirection(REVERSE);
    }
  }
};
