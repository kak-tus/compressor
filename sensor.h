class Sensor {
public:
  Sensor(uint8_t tempPin, uint8_t mapPin) : _tempPin(tempPin), _mapPin(mapPin) {
    pinMode(_tempPin, INPUT);
    pinMode(_mapPin, INPUT);

    _deltaTemp =
        (temp2Voltage * temp1 - temp1Voltage * temp2) / (temp2 - temp1);

    _angleTemp = (temp1 - temp2) / (temp1Voltage - temp2Voltage);
  }

  float temperature() {
    int value = analogRead(_tempPin);
    _voltageTemp = (float)(value * baseVoltage) / 1024;

    float rawTemp = (_voltageTemp + _deltaTemp) * _angleTemp;

    _temp += (rawTemp - _temp) * 0.1;

    return _temp;
  }

  float pressure() {
    int value = analogRead(_mapPin);
    _voltageMap = (float)(value * baseVoltage) / 1024;

    float rawPressure = (_voltageMap + deltaMAP) * angleMAP * 1000;

    // Initial
    if (_pressure < 0.1) {
      _pressure = rawPressure;
      return _pressure;
    }

    _pressure += (rawPressure - _pressure) * 0.1;
    return _pressure;
  }

  float pressureInMM() { return _pressure / kpaToMM; }
  float voltageMap() { return _voltageMap; }
  float voltageTemp() { return _voltageTemp; }

private:
  const float baseVoltage = 5.0;
  const float deltaMAP = 0.37;
  const float angleMAP = 22.643;
  const float kpaToMM = 133.3224;
  const float temp1 = 18.5;
  const float temp2 = 62;
  const float temp1Voltage = 3.43;
  const float temp2Voltage = 1.86;

  const uint8_t _tempPin;
  const uint8_t _mapPin;
  float _pressure, _temp;
  float _deltaTemp, _angleTemp;
  float _voltageMap, _voltageTemp;
};
