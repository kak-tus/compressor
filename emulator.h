#include <TimerMs.h>

class Emulator {
public:
  enum EmulatorType { BEFORE_THROTTLE = 1, AFTER_THROTTLE };

  Emulator(uint8_t emulatorPin, EmulatorType type)
      : _emulatorPin(emulatorPin), _rpmChange(100, true, false), _type(type) {
    pinMode(_emulatorPin, INPUT);
    _rpm = minRPM;
    _rpmChangedAt = millis();
  }

  uint32_t pressure() {
    uint8_t _prevPos = _throttlePos;

    uint16_t value = analogRead(_emulatorPin);

    // Close faster, open slower
    float voltageCoeff = 0.001;
    if (value < _voltage) {
      voltageCoeff = 0.01;
    }

    _voltage += (value - _voltage) * voltageCoeff;

    if (_voltage < minVoltage + delta) {
      _throttlePos = 0;
    } else if (_voltage > maxVoltage + delta) {
      _throttlePos = 100;
    } else {
      _throttlePos =
          (uint32_t)(_voltage - minVoltage) * 100 / (maxVoltage - minVoltage);
    }

    if (_rpmChange.tick()) {
      if (_throttlePos <= throttleRPMDown && _rpm > minRPM) {
        uint16_t delta = (throttleRPMDown - _throttlePos) * 10;

        delta *= ((millis() - _rpmChangedAt) / 100);

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

        if (_rpm + delta > maxRPM) {
          _rpm = maxRPM;
        } else {
          _rpm += delta;
        }
      }

      _rpmChangedAt = millis();
    }

    if (_rpm <= middleRPM) {
      _efficiency =
          minEfficiency + ((uint32_t)(_rpm - minRPM) *
                           (uint32_t)(middleEfficiency - minEfficiency)) /
                              (middleRPM - minRPM);
    } else {
      _efficiency =
          middleEfficiency + ((uint32_t)(_rpm - middleRPM) *
                              (uint32_t)(maxEfficiency - middleEfficiency)) /
                                 (maxRPM - middleRPM);
    }

    _pressureClear =
        normalPressure +
        ((uint32_t)_efficiency * (uint32_t)(maxPressure - normalPressure)) /
            100;

    if (_type == BEFORE_THROTTLE) {
      return normalPressure + ((uint32_t)(100 - _throttleRealPos) *
                               (uint32_t)(_pressureClear - normalPressure)) /
                                  100;

    } else {
      return closedPressure + ((uint32_t)_throttlePos *
                               (uint32_t)(_pressureClear - closedPressure)) /
                                  100;
    }
  }

  uint8_t throttle() { return _throttlePos; }
  uint16_t rpm() { return _rpm; }
  uint16_t efficiency() { return _efficiency; }
  uint32_t pressureClear() { return _pressureClear; }

  void setRealThrottle(uint8_t pos) { _throttleRealPos = pos; }

private:
  const uint8_t _emulatorPin;

  const uint16_t minRPM = 850;
  const uint16_t middleRPM = 2000;
  const uint16_t maxRPM = 5000;

  uint8_t _throttlePos;
  uint8_t _throttleRealPos = 100;

  uint16_t _voltage;

  uint16_t _rpm;
  TimerMs _rpmChange;
  unsigned long _rpmChangedAt;

  const uint8_t throttleRPMDown = 10;
  const uint8_t throttleRPMUp = 20;

  const EmulatorType _type;

  const uint32_t closedPressure = 60000;
  const uint32_t normalPressure = 100000;
  const uint32_t maxPressure = 180000;

  const uint8_t minEfficiency = 5;
  const uint8_t middleEfficiency = 50;
  const uint8_t maxEfficiency = 80;

  uint8_t _efficiency;
  uint32_t _pressureClear;

  // Use delta to guarantee get 100% open and 0% close
  const uint8_t delta = 2;

  // Max voltage as native integer data
  const uint16_t maxVoltage = 1024;
  const uint16_t minVoltage = 0;
};
