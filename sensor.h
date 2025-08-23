class Sensor {
public:
  Sensor(uint8_t tempPin) {
    _tempPin = tempPin;

    pinMode(_tempPin, INPUT);
  }

  Sensor(uint8_t positionPin, uint16_t min, uint16_t max) {
    _positionPin = positionPin;
    _positionMin = min;
    _positionMax = max;

    pinMode(_positionPin, INPUT);
  }

  int16_t temperature() {
    uint16_t rawVoltage = analogRead(_tempPin);

    _voltageTemp += ((float)rawVoltage - _voltageTemp) * 0.5;

    int16_t voltageFrom, voltageDiff, tempFrom, tempDiff;

    // Themperature sensor is not linear, so use this calculation
    if (_voltageTemp >= 829) {
      voltageFrom = 1024;
      voltageDiff = voltageFrom - 829;
      tempFrom = -40;
      tempDiff = 1 - tempFrom;
    } else if (_voltageTemp >= 707) {
      voltageFrom = 829;
      voltageDiff = voltageFrom - 707;
      tempFrom = 1;
      tempDiff = 20 - tempFrom;
    } else if (_voltageTemp >= 670) {
      voltageFrom = 707;
      voltageDiff = voltageFrom - 670;
      tempFrom = 20;
      tempDiff = 22 - tempFrom;
    } else if (_voltageTemp >= 584) {
      voltageFrom = 670;
      voltageDiff = voltageFrom - 584;
      tempFrom = 22;
      tempDiff = 30 - tempFrom;
    } else if (_voltageTemp >= 512) {
      voltageFrom = 584;
      voltageDiff = voltageFrom - 512;
      tempFrom = 30;
      tempDiff = 40 - tempFrom;
    } else if (_voltageTemp >= 427) {
      voltageFrom = 512;
      voltageDiff = voltageFrom - 427;
      tempFrom = 40;
      tempDiff = 50 - tempFrom;
    } else if (_voltageTemp >= 319) {
      voltageFrom = 427;
      voltageDiff = voltageFrom - 319;
      tempFrom = 50;
      tempDiff = 60 - tempFrom;
    } else if (_voltageTemp >= 290) {
      voltageFrom = 319;
      voltageDiff = voltageFrom - 290;
      tempFrom = 60;
      tempDiff = 70 - tempFrom;
    } else if (_voltageTemp >= 247) {
      voltageFrom = 290;
      voltageDiff = voltageFrom - 247;
      tempFrom = 70;
      tempDiff = 80 - tempFrom;
    } else {
      voltageFrom = 247;
      voltageDiff = voltageFrom - 0;
      tempFrom = 80;
      tempDiff = 130 - tempFrom;
    }

    int16_t _temp = tempFrom + (int32_t)(voltageFrom - _voltageTemp) *
                                   tempDiff / voltageDiff;

    return _temp;
  }

  uint8_t position() {
    uint16_t rawVoltage = analogRead(_positionPin);

    _voltagePosition += ((float)rawVoltage - _voltagePosition) * 0.8;

    uint8_t position;

    if (_voltagePosition <= _positionMin) {
      position = 0;
    } else if (_voltagePosition >= _positionMax) {
      position = 100;
    } else {
      position = (uint32_t)(_voltagePosition - _positionMin) * 100 /
              (_positionMax - _positionMin);
    }

    return position;
  }

  uint16_t voltageTemp() { return _voltageTemp; }
  uint16_t voltagePosition() { return _voltagePosition; }

private:
  uint8_t _tempPin;
  uint8_t _positionPin;

  uint16_t _positionMin, _positionMax;

  float _voltageTemp, _voltagePosition;
};
