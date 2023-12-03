#include "sensor.h"

class Emulator {
public:
  enum EmulatorType { BEFORE_THROTTLE = 1, AFTER_THROTTLE };

  Emulator(uint8_t tempPin, uint8_t mapPin, int16_t mapCorrection,
           uint8_t emulatorPin, EmulatorType type)
      : _sens(tempPin, mapPin, mapCorrection), _emulatorPin(emulatorPin) {
    pinMode(_emulatorPin, INPUT);
  }

  float temperature() { return _sens.temperature(); }

  float pressure() {
    int value = analogRead(_emulatorPin);
    float raw = (float)(value * baseVoltage) / 1024;

    _voltage += (raw - _voltage) * 0.001;

    _throttlePos = _voltage * 100 / baseVoltage;

    if (_throttlePos == 1) {
      _throttlePos = 0;
    } else if (_throttlePos == 99) {
      _throttlePos = 100;
    }

    return _sens.pressure();
  }

  float pressureInMM() { return _sens.pressureInMM(); }
  float voltageMap() { return _sens.voltageMap(); }
  float voltageTemp() { return _sens.voltageTemp(); }

  uint8_t throttle() { return _throttlePos; }

private:
  Sensor _sens;
  const uint8_t _emulatorPin;

  const uint16_t minRPM = 850;
  const uint16_t maxRPM = 5000;

  uint8_t _throttlePos;

  const float baseVoltage = 5.0;

  float _voltage;
};
