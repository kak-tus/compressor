class Controller {
public:
  enum directionType { OPEN, CLOSE };

  Controller() {
    // Don't use real initial percent
    // this controller control how MUST be percent set, but not real percent
    // Real and wanted percents controlled by throttle
    _position = maximumOpen;
  }

  void poweroff() { _position = maximumOpen; }

  void poweron() { _position = maximumOpen; }

  void control(uint16_t pressure2, uint8_t positionMainThrottle) {
    _pressure2 = pressure2;
    _positionMainThrottle = positionMainThrottle;
  }

  uint8_t position() {
    if (_pressure2 > limitPressure2) {
      if (timeout(_logLimit, 1000)) {
        _logLimit = millis();

        Serial.print("Pressure limit: pressure2=");
        Serial.println(_pressure2);
      }

      incPosition();
    } else if (_positionMainThrottle < 10) {
      setPosition(maximumOpen);
    } else if (_positionMainThrottle < 15) {
      incPosition();
    } else if (_positionMainThrottle > 20) {
      decPositionSlow();
    } else if (_positionMainThrottle > 30) {
      decPosition();
    }

    return _position;
  }

  void setTemperature(int16_t temperature) {
    if (temperature > boostOffTemperature) {
      _minPercent = 100;
      _minPercentChanged = millis();
      return;
    } else if (temperature > boostLowerTemperature) {
      if (_minPercent < 100 && timeout(_minPercentChanged, 5000)) {
        _minPercent++;
        _minPercentChanged = millis();
      }
    } else {
      if (_minPercent > 0 && timeout(_minPercentChanged, 10000)) {
        _minPercent--;
        _minPercentChanged = millis();
      }
    }
  }

  uint8_t positionVal() { return _position; }

private:
  bool timeout(unsigned long start, uint16_t operationLimit) {
    if (millis() - start < operationLimit) {
      return false;
    }

    return true;
  }

  void incPosition() {
    if (!timeout(_positionChanged, openDelay)) {
      return;
    }

    _positionChanged = millis();

    if (_position >= maximumOpen) {
      return;
    }

    _position++;
  }

  void decPosition() {
    if (!timeout(_positionChanged, closeDelay)) {
      return;
    }

    _positionChanged = millis();

    if (_position == 0) {
      return;
    }

    _position--;
  }

  void decPositionSlow() {
    if (!timeout(_positionChanged, closeDelaySlow)) {
      return;
    }

    _positionChanged = millis();

    if (_position == 0) {
      return;
    }

    _position--;
  }

  void setPosition(uint8_t position) {
    _positionChanged = millis();
    _position = position;
  }

  uint16_t limitPressure2 = 170;

  const uint8_t closeDelay = 2;
  const uint8_t closeDelaySlow = 50;
  const uint8_t openDelay = 2;

  uint8_t maximumOpen = 100;

  // 60-70 - is ok temperature
  // 80 - is bad, stop boost
  const int16_t boostOffTemperature = 80;
  const int16_t boostLowerTemperature = 75;

  uint8_t _position, _minPercent;
  unsigned long _positionChanged, _minPercentChanged, _logLimit;

  uint8_t _positionMainThrottle;

  uint16_t _pressure2;
};
