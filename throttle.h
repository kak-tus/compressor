#include <BTS7960.h>
#include <TimerMs.h>

class Throttle {
public:
  Throttle(uint8_t position1Pin, uint8_t position2Pin, uint8_t en, uint8_t lPwm,
           uint8_t rPwm)
      : _pos1Pin(position1Pin), _pos2Pin(position2Pin), _motor(en, lPwm, rPwm),
        _openedCheck(1000, true, false) {
    pinMode(_pos1Pin, INPUT);
    pinMode(_pos2Pin, INPUT);
  }

  uint8_t position1() {
    int value = analogRead(_pos1Pin);
    _voltagePos1 = (float)(value * baseVoltage) / 1024;

    if (_voltagePos1 < minVoltageSens1) {
      _pos1 = 0;
    } else if (_voltagePos1 > maxVoltageSens1) {
      _pos1 = 100;
    } else {
      _pos1 = (_voltagePos1 - minVoltageSens1) * 100 /
              (maxVoltageSens1 - minVoltageSens1);
    }

    return _pos1;
  }

  uint8_t position2() {
    int value = analogRead(_pos2Pin);
    _voltagePos2 = (float)(value * baseVoltage) / 1024;

    if (_voltagePos2 < minVoltageSens2) {
      _pos2 = 0;
    } else if (_voltagePos2 > maxVoltageSens2) {
      _pos2 = 100;
    } else {
      _pos2 = (_voltagePos2 - minVoltageSens2) * 100 /
              (maxVoltageSens2 - minVoltageSens2);
    }

    return _pos2;
  }

  uint8_t position() { return position1(); }

  float voltagePosition1() { return _voltagePos1; }
  float voltagePosition2() { return _voltagePos2; }

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

  // Call sensorsOk only after position() call
  bool sensorsOk() {
    // Use stored _pos1 value - assume that sensorsOk called only after
    // position()
    if (abs(_pos1 - (100 - position2())) <= sensorsOkDelta) {
      return true;
    }

    return false;
  }

  void check() {
    _motor.Enable();

    unsigned long start = millis();

    while (position() < 100) {
      open(speedLow);
      delay(1);

      if (!sensorsOk()) {
        _failed = true;
        _failStateCode = 1;
        syncOpen();
        _motor.Disable();
        return;
      }

      if (timeout(start, _operationLimit)) {
        _failed = true;
        _failStateCode = 2;
        syncOpen();
        _motor.Disable();
        return;
      }
    }

    _motor.Stop();
    delay(1000);

    start = millis();

    while (position() > 70) {
      close(speedLow);
      delay(1);

      if (!sensorsOk()) {
        _failed = true;
        _failStateCode = 3;
        syncOpen();
        _motor.Disable();
        return;
      }

      if (timeout(start, _operationLimit)) {
        _failed = true;
        _failStateCode = 4;
        syncOpen();
        _motor.Disable();
        return;
      }
    }

    _motor.Stop();
    delay(1000);

    start = millis();

    while (position() < 100) {
      open(speedLow);
      delay(1);

      if (!sensorsOk()) {
        _failed = true;
        _failStateCode = 5;
        syncOpen();
        _motor.Disable();
        return;
      }

      if (timeout(start, _operationLimit)) {
        _failed = true;
        _failStateCode = 6;
        syncOpen();
        _motor.Disable();
        return;
      }
    }

    _motor.Disable();
  }

  void syncOpen() {
    unsigned long start = millis();

    while (position() < 100 && !timeout(start, _operationLimit)) {
      open(speedLow);
      delay(1);
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
        // Periodically check that throttle was'n uncontrolled open
        if (position() < 100) {
          poweroff();
        }
      }

      return true;
    }
  }

  bool controlHold() {
    uint8_t pos = position();

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
      _holdPositionWant = _holdPositionStart + (millis() - _holdStartAt) / 10;
      if (_holdPositionWant > _holdPositionFinal) {
        _holdPositionWant = _holdPositionFinal;
      }

      _holdReached = false;
      _holdSpeed = speedLow;
      _holdSpeedChanged = millis();
    } else if (_holdPositionWant > _holdPositionFinal) {
      _holdPositionWant = _holdPositionStart - (millis() - _holdStartAt) / 100;
      if (_holdPositionWant < _holdPositionFinal) {
        _holdPositionWant = _holdPositionFinal;
      }

      _holdReached = false;
      _holdSpeed = speedLow;
      _holdSpeedChanged = millis();
    }

    return true;
  }

  bool controlPoweroff() {
    if (position() < 100) {
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

  uint8_t failStateCode() { return _failStateCode; }

  void hold(uint8_t pos) {
    if (_failed) {
      return;
    }

    if (_holdStatus != IN_HOLD) {
      _holdStatus = IN_HOLD;
      _motor.Enable();

      uint8_t _currPos = position();

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
      }

      return;
    }

    if (pos == _holdPositionFinal) {
      return;
    }

    uint8_t _currPos = position();

    if (pos == _currPos) {
      _holdPositionStart = _currPos;
      _holdPositionFinal = pos;
      _holdStartAt = millis();
      _holdReached = true;
      _holdSpeed = speedLow;
      _holdSpeedChanged = millis();
      return;
    }

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
  const uint8_t _pos1Pin, _pos2Pin;
  float _voltagePos1, _voltagePos2;
  uint8_t _pos1, _pos2;

  const float baseVoltage = 5.0;

  // Use delta to guarantee get 100% open and 0% close
  const float delta = 0.04;
  const float maxVoltageSens1 = 4.54 - delta;
  const float minVoltageSens1 = 0.68 + delta;
  const float maxVoltageSens2 = 4.35 - delta;
  const float minVoltageSens2 = 0.49 + delta;

  const uint8_t sensorsOkDelta = 4;

  BTS7960 _motor;

  enum statusType { IN_OPEN = 1, IN_CLOSE, IN_STOP };

  statusType _status;

  uint8_t _currentSpeed;

  const uint8_t speedLow = 20;

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
};
