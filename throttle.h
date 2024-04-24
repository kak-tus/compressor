#include <BTS7960.h>
#include <TimerMs.h>

#define PID_INTEGER
#include <GyverPID.h>

class Throttle {
public:
  Throttle(uint8_t position1Pin, uint8_t position2Pin, uint8_t en, uint8_t lPwm,
           uint8_t rPwm)
      : _pos1Pin(position1Pin), _pos2Pin(position2Pin), _motor(en, lPwm, rPwm),
        _openedCheck(1000, true, false) {
    pinMode(_pos1Pin, INPUT);
    pinMode(_pos2Pin, INPUT);
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

    if (_voltagePos2 < minVoltageSens2 + delta) {
      _pos2 = 100;
    } else if (_voltagePos2 > maxVoltageSens2 - delta) {
      _pos2 = 0;
    } else {
      _pos2 = 100 - (uint32_t)(_voltagePos2 - minVoltageSens2) * 100 /
                        (maxVoltageSens2 - minVoltageSens2);
    }

    return _pos1;
  }

  uint16_t voltagePosition1() { return _voltagePos1; }
  uint16_t voltagePosition2() { return _voltagePos2; }

  uint8_t status() { return uint8_t(_status); }
  uint8_t speed() { return _currentSpeed; }
  uint8_t holdStatus() { return uint8_t(_holdStatus); }
  uint8_t holdPositionWant() { return _holdPositionWant; }
  uint8_t holdPositionFinal() { return _holdPositionFinal; }
  uint8_t holdPositionStart() { return _holdPositionStart; }
  uint8_t holdSpeed() { return _holdSpeed; }
  bool holdReached() { return _holdReached; }
  uint8_t holdDirection() { return uint8_t(_holdDirection); }
  unsigned long holdStartAt() { return _holdStartAt; }

  void check() {
    _motor.Enable();

    regulator.setMode(ON_RATE);
    regulator.input = position();

    regulator.setLimits(0, speedMaxOpen);
    regulator.setDirection(NORMAL);
    regulator.setpoint = 100;
    regulator.output = 0;

    unsigned long start = millis();

    while (position() < 100) {
      int val = regulator.getResultTimer();
      open(val);

      if (!sensorsOk()) {
        _fail(1);
        return;
      }

      if (timeout(start, _operationLimit)) {
        _fail(2);
        return;
      }
    }

    _motor.Stop();
    delay(1000);

    regulator.setLimits(0, speedMaxClose);
    regulator.setDirection(REVERSE);
    regulator.setpoint = 70;
    regulator.output = 0;

    start = millis();

    while (position() >= 70) {
      int val = regulator.getResultTimer();
      close(val);

      if (!sensorsOk()) {
        _fail(3);
        return;
      }

      if (timeout(start, _operationLimit)) {
        _fail(4);
        return;
      }
    }

    _motor.Stop();
    delay(1000);

    regulator.setLimits(0, speedMaxOpen);
    regulator.setDirection(NORMAL);
    regulator.setpoint = 100;
    regulator.output = 0;

    start = millis();

    while (position() < 100) {
      int val = regulator.getResultTimer();
      open(val);

      if (!sensorsOk()) {
        _fail(5);
        return;
      }

      if (timeout(start, _operationLimit)) {
        _fail(6);
        return;
      }
    }

    _motor.Disable();
  }

  bool control() {
    if (_failed) {
      return false;
    }

    switch (_holdStatus) {
    case IN_HOLD:
      return controlHold();
    case IN_POWEROFF:
      return controlPoweroff();
    default:
      if (_openedCheck.tick()) {
        // Periodically check that throttle wasn't uncontrolled open
        uint8_t _currPos = position();

        if (!sensorsOk()) {
          _fail(7);
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
        _fail(8);
        return;
      }

      _holdPositionWant = _currPos;
      _holdPositionStart = _currPos;
      _holdPositionFinal = pos;
      _holdStartAt = millis();

      if (_currPos < pos) {
        _holdDirection = OPEN;
        _holdReached = false;
        _holdSpeed = speedLow;
        _holdSpeedChanged = millis();
      } else if (_currPos > pos) {
        _holdDirection = CLOSE;
        _holdReached = false;
        _holdSpeed = speedLow;
        _holdSpeedChanged = millis();
      } else {
        _holdReached = true;
        stop();
      }

      return;
    }

    if (pos == _holdPositionFinal) {
      return;
    }

    uint8_t _currPos = position();

    if (!sensorsOk()) {
      _fail(9);
      return;
    }

    if (pos == _currPos) {
      _holdPositionStart = _currPos;
      _holdPositionFinal = pos;
      _holdStartAt = millis();
      _holdReached = true;
      stop();
      _holdSpeed = speedLow;
      _holdSpeedChanged = millis();
      return;
    }

    if (_holdReached) {
      _holdPositionStart = _currPos;
      _holdPositionFinal = pos;
      _holdStartAt = millis();
      _holdReached = false;
      _holdSpeed = speedLow;
      _holdSpeedChanged = millis();

      if (_currPos < pos) {
        _holdDirection = OPEN;
      } else {
        _holdDirection = CLOSE;
      }
    } else {
      if (_holdDirection == OPEN) {
        if (_currPos < pos) {
          // Continue open
          _holdPositionFinal = pos;
          _holdReached = false;
        } else {
          // Direction changed
          _holdPositionStart = _currPos;
          _holdPositionFinal = pos;
          _holdStartAt = millis();
          _holdDirection = CLOSE;
          _holdReached = false;
          _holdSpeed = speedLow;
          _holdSpeedChanged = millis();
        }
      } else if (_holdDirection == CLOSE) {
        if (_currPos > pos) {
          // Continue close
          _holdPositionFinal = pos;
          _holdReached = false;
        } else {
          // Direction changed
          _holdPositionStart = _currPos;
          _holdPositionFinal = pos;
          _holdStartAt = millis();
          _holdDirection = OPEN;
          _holdReached = false;
          _holdSpeed = speedLow;
          _holdSpeedChanged = millis();
        }
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
    _holdSpeed = speedLow;
    _holdSpeedChanged = millis();

    // Not using in poweroff operation processing, only for logging purposes
    _holdPositionFinal = 100;
    _holdPositionStart = position();
    _holdDirection = OPEN;
    _holdStartAt = millis();
  }

private:
  // Call sensorsOk only after position() call
  bool sensorsOk() {
    // Use stored _pos1 value - assume that sensorsOk called only after
    // position()
    if (abs(_pos1 - _pos2) >= sensorsOkDelta) {
      return false;
    }

    return true;
  }

  void _fail(uint8_t code) {
    Serial.print("Fail data:");
    Serial.print(" pos1=");
    Serial.print(_pos1);
    Serial.print(" pos2=");
    Serial.print(_pos2);
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

    regulator.setLimits(0, speedMaxOpen);
    regulator.setDirection(NORMAL);
    regulator.setpoint = 100;
    regulator.output = 0;

    while (position() < 100 && !timeout(start, _operationLimit)) {
      int val = regulator.getResultTimer();
      open(val);
    }
  }

  void close(uint8_t speed) {
    if (_status == IN_CLOSE) {
      if (_currentSpeed != speed) {
        _currentSpeed = speed;
        _motor.TurnRight(speed);
      }

      return;
    }

    _status = IN_CLOSE;
    _currentSpeed = speed;

    _motor.TurnRight(speed);
  }

  void open(uint8_t speed) {
    if (_status == IN_OPEN) {
      if (_currentSpeed != speed) {
        _currentSpeed = speed;
        _motor.TurnLeft(speed);
      }

      return;
    }

    _status = IN_OPEN;
    _currentSpeed = speed;

    _motor.TurnLeft(speed);
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
      _fail(10);
      return false;
    }

    if (_holdReached) {
      // TODO Add possible corrections
    } else {
      if (_holdDirection == OPEN) {
        if (pos < _holdPositionWant) {
          open(_holdSpeed);

          // TODO control speed
          //   // Open faster, then close
          //   // to do blowoff
          //   if (timeout(_holdSpeedChanged, 10)) {
          //     _holdSpeed += 1;
          //     _holdSpeedChanged = millis();
          //   }
        } else {
          _holdSpeed = speedLow;
          _holdSpeedChanged = millis();
          _holdReached = true;
          stop();
        }
      } else if (_holdDirection == CLOSE) {
        if (pos > _holdPositionWant) {
          close(_holdSpeed);

          //   // Close slower, then open
          //   // to do smooth boost
          //   if (timeout(_holdSpeedChanged, 100)) {
          //     _holdSpeed += 1;
          //     _holdSpeedChanged = millis();
          //   }
        } else {
          _holdSpeed = speedLow;
          _holdSpeedChanged = millis();
          _holdReached = true;
          stop();
        }
      }
    }

    // Open faster, then close
    // to do blowoff
    // Close slower, then open
    // to do smooth boost
    if (_holdPositionWant < _holdPositionFinal) {
      unsigned long _newWant =
          (unsigned long)_holdPositionStart + (millis() - _holdStartAt) / 10;

      if (_newWant > _holdPositionFinal) {
        _holdPositionWant = _holdPositionFinal;
      } else {
        _holdPositionWant = _newWant;
      }

      _holdReached = false;
      _holdSpeed = speedLow;
      _holdSpeedChanged = millis();
    } else if (_holdPositionWant > _holdPositionFinal) {
      unsigned long _newWant = (millis() - _holdStartAt) / 100;

      if (_newWant > _holdPositionStart) {
        _holdPositionWant = _holdPositionFinal;
      } else {
        _holdPositionWant = _holdPositionStart - _newWant;
      }

      _holdReached = false;
      _holdSpeed = speedLow;
      _holdSpeedChanged = millis();
    }

    return true;
  }

  bool controlPoweroff() {
    if (_failed) {
      return false;
    }

    uint8_t pos = position();

    if (!sensorsOk()) {
      _fail(11);
      return false;
    }

    if (pos < 100) {
      open(_holdSpeed);

      if (timeout(_holdSpeedChanged, 100)) {
        _holdSpeed += 1;
        _holdSpeedChanged = millis();
      }
    } else {
      _holdSpeed = speedLow;
      _holdSpeedChanged = millis();
      _motor.Disable();
      _holdStatus = holdStatusType(0);
      _holdReached = true;
    }

    return true;
  }

  const uint8_t _pos1Pin, _pos2Pin;
  uint16_t _voltagePos1, _voltagePos2;
  uint8_t _pos1, _pos2;

  // Use delta to guarantee get 100% open and 0% close
  const uint8_t delta = 5;

  // Max voltage as native integer data
  uint16_t minVoltageSens1 = 160;
  uint16_t maxVoltageSens1 = 890;
  uint16_t minVoltageSens2 = 120;
  uint16_t maxVoltageSens2 = 840;

  const uint8_t sensorsOkDelta = 4;

  BTS7960 _motor;

  enum statusType { IN_OPEN = 1, IN_CLOSE, IN_STOP };

  statusType _status;

  uint8_t _currentSpeed;

  const uint8_t speedLow = 20;

  // Open faster, then close
  // to do blowoff
  const uint8_t speedMaxOpen = 30;
  const uint8_t speedMaxClose = 20;

  bool _failed = false;

  const uint16_t _operationLimit = 2000;

  uint8_t _failStateCode = 0;

  enum holdStatusType { IN_HOLD = 1, IN_POWEROFF };

  holdStatusType _holdStatus;
  uint8_t _holdPositionWant, _holdPositionFinal, _holdPositionStart;
  uint8_t _holdSpeed;
  bool _holdReached;

  enum holdDirectionType { OPEN, CLOSE };
  holdDirectionType _holdDirection;

  unsigned long _holdSpeedChanged, _holdStartAt;

  TimerMs _openedCheck;

  GyverPID regulator = GyverPID(1, 0.1, 0.1, 10);
};
