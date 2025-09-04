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

  void control(uint8_t positionMainThrottle) {
    _positionMainThrottle = positionMainThrottle;
  }

  uint8_t position() {
    if (_positionMainThrottle < 5) {
      setPosition(maximumOpen);
    } else if (_positionMainThrottle < 10) {
      incPosition();
    } else if (_positionMainThrottle > 20) {
      decPosition();
    } else if (_positionMainThrottle > 15) {
      decPositionSlow();
    }

    return _position;
  }

  void setTemperature(int16_t temperature) {
    if (temperature > boostOffTemperature) {
      _minPosition = 100;
      _minPercentChanged = millis();
      return;
    } else if (temperature > boostLowerTemperature) {
      if (_minPosition < 100 && timeout(_minPercentChanged, 2000)) {
        _minPosition++;
        _minPercentChanged = millis();

        if (_position < _minPosition) {
          _position = _minPosition;
        }
      }
    } else {
      if (_minPosition > 0 && timeout(_minPercentChanged, 10000)) {
        _minPosition--;
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

    if (_position <= _minPosition) {
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

    if (_position <= _minPosition) {
      return;
    }

    _position--;
  }

  void setPosition(uint8_t position) {
    _positionChanged = millis();
    _position = position;
  }

  const uint8_t closeDelay = 2;
  const uint8_t closeDelaySlow = 50;
  const uint8_t openDelay = 2;

  uint8_t maximumOpen = 100;

  // 60-70 - is ok temperature
  // 80 - is bad, stop boost
  const int16_t boostOffTemperature = 80;
  const int16_t boostLowerTemperature = 75;

  uint8_t _position, _minPosition;
  unsigned long _positionChanged, _minPercentChanged;

  uint8_t _positionMainThrottle;
};
