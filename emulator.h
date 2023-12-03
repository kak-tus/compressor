#include "sensor.h"
#include <TimerMs.h>

class Emulator {
public:
  enum EmulatorType { BEFORE_THROTTLE = 1, AFTER_THROTTLE };

  Emulator(uint8_t tempPin, uint8_t mapPin, int16_t mapCorrection,
           uint8_t emulatorPin, EmulatorType type)
      : _sens(tempPin, mapPin, mapCorrection), _emulatorPin(emulatorPin),
        _rpmChange(100, true, false) {
    pinMode(_emulatorPin, INPUT);
    _rpm = minRPM;
    _rpmChangedAt = millis();
  }

  float temperature() { return _sens.temperature(); }

  float pressure() {
    uint8_t _prevPos = _throttlePos;

    int value = analogRead(_emulatorPin);
    float raw = (float)(value * baseVoltage) / 1024;

    _voltage += (raw - _voltage) * 0.001;

    _throttlePos = _voltage * 100 / baseVoltage;

    if (_throttlePos == 1) {
      _throttlePos = 0;
    } else if (_throttlePos == 99) {
      _throttlePos = 100;
    }

    if (_rpmChange.tick()) {
      if (_throttlePos <= throttleRPMDown && _rpm > minRPM) {
        uint16_t delta = (throttleRPMDown - _throttlePos) * 10;

        delta *= ((millis() - _rpmChangedAt) / 100);

        _rpmChangedAt = millis();

        if (delta > _rpm) {
          _rpm = minRPM;
        } else if (_rpm - delta < minRPM) {
          _rpm = minRPM;
        } else {
          _rpm -= delta;
        }
      } else if (_throttlePos >= throttleRPMUp && _rpm < maxRPM) {
        uint16_t delta = _throttlePos / 3;

        delta *= ((millis() - _rpmChangedAt) / 100);

        _rpmChangedAt = millis();

        if (_rpm + delta > maxRPM) {
          _rpm = maxRPM;
        } else {
          _rpm += delta;
        }
      }
    }

    return _sens.pressure();
  }

  float pressureInMM() { return _sens.pressureInMM(); }
  float voltageMap() { return _sens.voltageMap(); }
  float voltageTemp() { return _sens.voltageTemp(); }

  uint8_t throttle() { return _throttlePos; }
  uint16_t rpm() { return _rpm; }

private:
  Sensor _sens;
  const uint8_t _emulatorPin;

  const uint16_t minRPM = 850;
  const uint16_t maxRPM = 5000;

  uint8_t _throttlePos;

  const float baseVoltage = 5.0;

  float _voltage;

  uint16_t _rpm;
  TimerMs _rpmChange;
  unsigned long _rpmChangedAt;

  const uint8_t throttleRPMDown = 10;
  const uint8_t throttleRPMUp = 20;
};
