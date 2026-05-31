class Sensor {
public:
  Sensor(uint8_t tempPin) {
    _tempPin = tempPin;

    pinMode(_tempPin, INPUT);
  }

  Sensor(uint8_t positionPin, float min, float max) {
    _positionPin = positionPin;
    _positionMin = min;
    _positionMax = max;

    pinMode(_positionPin, INPUT);
  }

  int16_t temperature() {
    uint16_t rawVoltage = analogRead(_tempPin);

    _voltageTemp += ((float)rawVoltage - _voltageTemp) * 0.5;

    float inVolt = _voltageTemp * 5.0 / (float)1024;

    float voltageFrom, voltageDiff, tempFrom, tempDiff;

    // Themperature sensor is not linear, so use this calculation
    if (inVolt >= 4.844) {
      voltageFrom = 5.0;
      voltageDiff = voltageFrom - 4.844;
      tempFrom = -45;
      tempDiff = -27 - tempFrom;
    } else if (inVolt >= 4.688) {
      voltageFrom = 4.844;
      voltageDiff = voltageFrom - 4.688;
      tempFrom = -27;
      tempDiff = -15 - tempFrom;
    } else if (inVolt >= 4.531) {
      voltageFrom = 4.688;
      voltageDiff = voltageFrom - 4.531;
      tempFrom = -15;
      tempDiff = -7 - tempFrom;
    } else if (inVolt >= 4.375) {
      voltageFrom = 4.531;
      voltageDiff = voltageFrom - 4.375;
      tempFrom = -7;
      tempDiff = 0 - tempFrom;
    } else if (inVolt >= 4.219) {
      voltageFrom = 4.375;
      voltageDiff = voltageFrom - 4.219;
      tempFrom = 0;
      tempDiff = 5 - tempFrom;
    } else if (inVolt >= 4.063) {
      voltageFrom = 4.219;
      voltageDiff = voltageFrom - 4.063;
      tempFrom = 5;
      tempDiff = 10 - tempFrom;
    } else if (inVolt >= 3.906) {
      voltageFrom = 4.063;
      voltageDiff = voltageFrom - 3.906;
      tempFrom = 10;
      tempDiff = 14 - tempFrom;
    } else if (inVolt >= 3.750) {
      voltageFrom = 3.906;
      voltageDiff = voltageFrom - 3.750;
      tempFrom = 14;
      tempDiff = 18 - tempFrom;
    } else if (inVolt >= 3.594) {
      voltageFrom = 3.750;
      voltageDiff = voltageFrom - 3.594;
      tempFrom = 18;
      tempDiff = 22 - tempFrom;
    } else if (inVolt >= 3.438) {
      voltageFrom = 3.594;
      voltageDiff = voltageFrom - 3.438;
      tempFrom = 22;
      tempDiff = 25 - tempFrom;
    } else if (inVolt >= 3.281) {
      voltageFrom = 3.438;
      voltageDiff = voltageFrom - 3.281;
      tempFrom = 25;
      tempDiff = 29 - tempFrom;
    } else if (inVolt >= 3.125) {
      voltageFrom = 3.281;
      voltageDiff = voltageFrom - 3.125;
      tempFrom = 29;
      tempDiff = 32 - tempFrom;
    } else if (inVolt >= 2.969) {
      voltageFrom = 3.125;
      voltageDiff = voltageFrom - 2.969;
      tempFrom = 32;
      tempDiff = 35 - tempFrom;
    } else if (inVolt >= 2.813) {
      voltageFrom = 2.969;
      voltageDiff = voltageFrom - 2.813;
      tempFrom = 35;
      tempDiff = 38 - tempFrom;
    } else if (inVolt >= 2.656) {
      voltageFrom = 2.813;
      voltageDiff = voltageFrom - 2.656;
      tempFrom = 38;
      tempDiff = 41 - tempFrom;
    } else if (inVolt >= 2.500) {
      voltageFrom = 2.656;
      voltageDiff = voltageFrom - 2.500;
      tempFrom = 41;
      tempDiff = 44 - tempFrom;
    } else if (inVolt >= 2.344) {
      voltageFrom = 2.500;
      voltageDiff = voltageFrom - 2.344;
      tempFrom = 44;
      tempDiff = 48 - tempFrom;
    } else if (inVolt >= 2.188) {
      voltageFrom = 2.344;
      voltageDiff = voltageFrom - 2.188;
      tempFrom = 48;
      tempDiff = 51 - tempFrom;
    } else if (inVolt >= 2.031) {
      voltageFrom = 2.188;
      voltageDiff = voltageFrom - 2.031;
      tempFrom = 51;
      tempDiff = 55 - tempFrom;
    } else if (inVolt >= 1.875) {
      voltageFrom = 2.031;
      voltageDiff = voltageFrom - 1.875;
      tempFrom = 55;
      tempDiff = 58 - tempFrom;
    } else if (inVolt >= 1.719) {
      voltageFrom = 1.875;
      voltageDiff = voltageFrom - 1.719;
      tempFrom = 58;
      tempDiff = 62 - tempFrom;
    } else if (inVolt >= 1.563) {
      voltageFrom = 1.719;
      voltageDiff = voltageFrom - 1.563;
      tempFrom = 62;
      tempDiff = 66 - tempFrom;
    } else if (inVolt >= 1.408) {
      voltageFrom = 1.563;
      voltageDiff = voltageFrom - 1.408;
      tempFrom = 66;
      tempDiff = 71 - tempFrom;
    } else if (inVolt >= 1.250) {
      voltageFrom = 1.408;
      voltageDiff = voltageFrom - 1.250;
      tempFrom = 71;
      tempDiff = 75 - tempFrom;
    } else if (inVolt >= 1.094) {
      voltageFrom = 1.250;
      voltageDiff = voltageFrom - 1.094;
      tempFrom = 75;
      tempDiff = 81 - tempFrom;
    } else if (inVolt >= 0.938) {
      voltageFrom = 1.094;
      voltageDiff = voltageFrom - 0.938;
      tempFrom = 81;
      tempDiff = 86 - tempFrom;
    } else if (inVolt >= 0.781) {
      voltageFrom = 0.938;
      voltageDiff = voltageFrom - 0.781;
      tempFrom = 86;
      tempDiff = 94 - tempFrom;
    } else if (inVolt >= 0.625) {
      voltageFrom = 0.781;
      voltageDiff = voltageFrom - 0.625;
      tempFrom = 94;
      tempDiff = 102 - tempFrom;
    } else if (inVolt >= 0.469) {
      voltageFrom = 0.625;
      voltageDiff = voltageFrom - 0.469;
      tempFrom = 102;
      tempDiff = 117 - tempFrom;
    } else if (inVolt >= 0.313) {
      voltageFrom = 0.469;
      voltageDiff = voltageFrom - 0.313;
      tempFrom = 117;
      tempDiff = 131 - tempFrom;
    } else if (inVolt >= 0.156) {
      voltageFrom = 0.313;
      voltageDiff = voltageFrom - 0.156;
      tempFrom = 131;
      tempDiff = 150 - tempFrom;
    } else {
      voltageFrom = 0.156;
      voltageDiff = voltageFrom - 0;
      tempFrom = 150;
      tempDiff = 184 - tempFrom;
    }

    int16_t _temp = tempFrom + (int32_t)(voltageFrom - inVolt) *
                                   tempDiff / voltageDiff;

    return _temp;
  }

  uint8_t position() {
    uint16_t rawVoltage = analogRead(_positionPin);

    _voltagePosition = (float)rawVoltage * 5.0 / (float)1024;

    uint8_t position;

    if (_voltagePosition <= _positionMin) {
      position = 0;
    } else if (_voltagePosition >= _positionMax) {
      position = 100;
    } else {
      position = (_voltagePosition - _positionMin) * 100 /
                 (_positionMax - _positionMin);
    }

    return position;
  }

  uint16_t voltageTemp() { return _voltageTemp; }

  float voltagePosition() { return _voltagePosition; }

private:
  uint8_t _tempPin;
  uint8_t _positionPin;

  float _positionMin, _positionMax;

  float _voltageTemp, _voltagePosition;

  uint16_t _pressure;
};
