class Throttle {
public:
  Throttle(uint8_t position1Pin, uint8_t position2Pin)
      : _pos1Pin(position1Pin), _pos2Pin(position2Pin) {
    pinMode(_pos1Pin, INPUT);
    pinMode(_pos2Pin, INPUT);
  }

  float position1() {
    int value = analogRead(_pos1Pin);
    _voltagePos1 = (float)(value * baseVoltage) / 1024;

    float rawPos = _voltagePos1 * 100 / baseVoltage;

    // Initial
    if (_pos1 < 0.1) {
      _pos1 = rawPos;
      return _pos1;
    }

    _pos1 += (rawPos - _pos1) * 0.1;

    return _pos1;
  }

  float position2() {
    int value = analogRead(_pos1Pin);
    _voltagePos2 = (float)(value * baseVoltage) / 1024;

    float rawPos = _voltagePos2 * 100 / baseVoltage;

    // Initial
    if (_pos2 < 0.1) {
      _pos2 = rawPos;
      return _pos2;
    }

    _pos2 += (rawPos - _pos2) * 0.1;

    return _pos2;
  }

  float voltagePosition1() { return _voltagePos1; }
  float voltagePosition2() { return _voltagePos2; }

private:
  const uint8_t _pos1Pin, _pos2Pin;
  float _voltagePos1, _voltagePos2;
  float _pos1, _pos2;

  const float baseVoltage = 5.0;
};
