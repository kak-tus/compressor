#include <BTS7960.h>

class Throttle {
public:
  Throttle(uint8_t position1Pin, uint8_t position2Pin, uint8_t en, uint8_t lPwm,
           uint8_t rPwm)
      : _pos1Pin(position1Pin), _pos2Pin(position2Pin), _motor(en, lPwm, rPwm) {
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

  // Call sensorsOk only after position() call
  bool sensorsOk() {
    // Use stored _pos1 value - assume that sensorsOk called only after
    // position()
    if (abs(_pos1 - (100 - position2())) <= 2) {
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

  void close(uint8_t pwm) {
    if (_inClose) {
      return;
    }

    _inClose = true;
    _inOpen = false;
    _motor.TurnRight(pwm);
  }

  void open(uint8_t pwm) {
    if (_inOpen) {
      return;
    }

    _inOpen = true;
    _inClose = false;
    _motor.TurnLeft(pwm);
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

    return true;
  }

  uint8_t failStateCode() { return _failStateCode; }

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

  BTS7960 _motor;

  bool _inOpen = false;
  bool _inClose = false;

  const uint8_t speedLow = 20;

  bool _failed = false;

  const uint16_t _operationLimit = 1500;

  uint8_t _failStateCode = 0;
};
