class Sensor {
public:
  Sensor(uint8_t tempPin, uint8_t mapPin, int16_t mapCorrection)
      : _tempPin(tempPin), _mapPin(mapPin), _mapCorrection(mapCorrection) {
    pinMode(_tempPin, INPUT);
    pinMode(_mapPin, INPUT);
  }

  int16_t temperature() {
    _voltageTemp = analogRead(_tempPin);

    int16_t voltageFrom, voltageDiff, tempFrom, tempDiff;

    // Themperature sensor is not linear, so use this calculation
    if (_voltageTemp >= 707) {
      voltageFrom = 1024;
      voltageDiff = voltageFrom - 707;
      tempFrom = -40;
      tempDiff = 20 - tempFrom;
    } else if (_voltageTemp >= 670) {
      voltageFrom = 707;
      voltageDiff = voltageFrom - 670;
      tempFrom = 20;
      tempDiff = 22 - tempFrom;
    } else if (_voltageTemp >= 584) {
      voltageFrom = 670;
      voltageDiff = voltageFrom - 584;
      tempFrom = 30;
      tempDiff = 40 - tempFrom;
    } else if (_voltageTemp >= 512) {
      voltageFrom = 584;
      voltageDiff = voltageFrom - 512;
      tempFrom = 40;
      tempDiff = 50 - tempFrom;
    } else if (_voltageTemp >= 427) {
      voltageFrom = 512;
      voltageDiff = voltageFrom - 427;
      tempFrom = 50;
      tempDiff = 60 - tempFrom;
    } else if (_voltageTemp >= 319) {
      voltageFrom = 427;
      voltageDiff = voltageFrom - 319;
      tempFrom = 60;
      tempDiff = 70 - tempFrom;
    } else if (_voltageTemp >= 290) {
      voltageFrom = 319;
      voltageDiff = voltageFrom - 290;
      tempFrom = 70;
      tempDiff = 80 - tempFrom;
    } else if (_voltageTemp >= 247) {
      voltageFrom = 290;
      voltageDiff = voltageFrom - 247;
      tempFrom = 80;
      tempDiff = 90 - tempFrom;
    } else {
      voltageFrom = 247;
      voltageDiff = voltageFrom - 0;
      tempFrom = 90;
      tempDiff = 130 - tempFrom;
    }

    int16_t rawTemp = tempFrom + (int32_t)(voltageFrom - _voltageTemp) *
                                     tempDiff / voltageDiff;

    _temp += (rawTemp - _temp) * 0.1;

    return int16_t(_temp);
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

    _pressure += (rawPressure - _pressure) * 0.5;
    return uint32_t(_pressure);
  }

  uint16_t pressureInMM() { return _pressure / kpaToMM; }
  uint16_t voltageMap() { return _voltageMap; }
  uint16_t voltageTemp() { return _voltageTemp; }

private:
  // delta = (voltage2 * pressure1 - voltage1 * pressure2) / (pressure2 -
  // pressure1)
  //
  // angle = (pressure1 - pressure2) / (voltage1 - voltage2)
  //
  // Voltages must be native arduino integer values (0-1024)
  //
  // nativeVoltage = voltage * 1024 / 5
  const float deltaMAP = 33.29;
  const float angleMAP = 0.394;

  const float kpaToMM = 133.3224;

  const uint8_t _tempPin;
  const uint8_t _mapPin;

  float _temp;
  float _pressure;

  uint16_t _voltageMap, _voltageTemp;

  int16_t _mapCorrection;
};
