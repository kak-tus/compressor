class Sensor {
public:
  Sensor(uint8_t tempPin, uint8_t mapPin, int16_t mapCorrection)
      : _tempPin(tempPin), _mapPin(mapPin), _mapCorrection(mapCorrection) {
    pinMode(_tempPin, INPUT);
    pinMode(_mapPin, INPUT);
  }

  int16_t temperature() {
    _voltageTemp = analogRead(_tempPin);

    float rawTemp = (_voltageTemp + _deltaTemp) * _angleTemp;

    _temp += (rawTemp - _temp) * 0.1;

    return _temp;
  }

  uint32_t pressure() {
    _voltageMap = analogRead(_mapPin);

    float rawPressure =
        (_voltageMap + deltaMAP) * angleMAP * 1000 + _mapCorrection;

    // Initial
    if (_pressure < 0.1) {
      _pressure = rawPressure;
      return _pressure;
    }

    _pressure += (rawPressure - _pressure) * 0.1;
    return _pressure;
  }

  uint16_t pressureInMM() { return _pressure / kpaToMM; }
  uint16_t voltageMap() { return _voltageMap; }
  uint16_t voltageTemp() { return _voltageTemp; }

private:
  // delta = (voltage2 * pressure1 - voltage1 * pressure2) / (pressure2 -
  // pressure1) angle = (pressure1 - pressure2) / (voltage1 - voltage2) Voltages
  // must be native arduino integer values (0-1024) nativeVoltage = voltage *
  // 1024 / 5
  const float deltaMAP = 86.707;
  const float angleMAP = 0.110;

  // Calc same as pressure
  float _deltaTemp = -838.943;
  float _angleTemp = -0.135;

  const float kpaToMM = 133.3224;

  const uint8_t _tempPin;
  const uint8_t _mapPin;

  int16_t _temp;
  uint32_t _pressure;

  uint16_t _voltageMap, _voltageTemp;

  int16_t _mapCorrection;
};
