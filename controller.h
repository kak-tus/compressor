class Controller {
public:
  enum directionType { OPEN, CLOSE };

  Controller() {
    // Don't use real initial percent
    // this controller control how MUST be percent set, but not real percent
    // Real and wanted percents controlled by throttle
    _percent = 100;
  }

  uint8_t position(uint32_t pressure1, uint32_t pressure2) {
    setPreviousPressure2(pressure2);

    if (timeout(_pressure1WantChanged, 100)) {
      if (isPressure2Up(pressure2)) {
        _pressure1Want = maxPressure;
        _pressure1WantChanged = millis();
      } else if (isPressure2Down(pressure2)) {
        _pressure1Want = pressure2 + pressureOver;
        _pressure1WantChanged = millis();
      }

      // With closed throttle
      // No need in high boost
      // Hold little overboost on normal pressure
      if (pressure2 <= closedPressure) {
        _pressure1Want = _normalPressure + pressureOver;
        _pressure1WantChanged = millis();
      }

      // Limit, need lower boost
      if (pressure2 >= limitPressure + pressureOver) {
        _pressure1Want = limitPressure + pressureOver;
        _pressure1WantChanged = millis();
      }
    }

    if (pressure1 < _pressure1Want - pressureDelta) {
      _direction = CLOSE;
      _reached = false;
    } else if (pressure1 > _pressure1Want + pressureDelta) {
      _direction = OPEN;
      _reached = false;
    } else {
      _reached = true;
    }

    if (!_reached) {
      if (_direction == OPEN) {
        incPercent();
      } else if (_direction == CLOSE) {
        decPercent();
      }
    }

    return _percent;
  }

  void setTemperature(uint16_t temperature) {
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

  void setNormalPressure(uint32_t pressure) { _normalPressure = pressure; }

  uint32_t pressure1Want() { return _pressure1Want; }
  uint8_t positionVal() { return _percent; }
  directionType direction() { return _direction; }
  bool reached() { return _reached; }

  bool isPressure2Up(uint32_t pressure2) {
    if (_previousPressure2_1 == 0) {
      return false;
    }

    if (_previousPressure2_1 < _previousPressure2_2 + pressureUpDownDelta &&
        _previousPressure2_2 < pressure2 + pressureUpDownDelta) {
      return true;
    }

    return false;
  }

  bool isPressure2Down(uint32_t pressure2) {
    if (_previousPressure2_1 == 0) {
      return false;
    }

    if (pressure2 < _previousPressure2_1 + pressureUpDownDelta &&
        _previousPressure2_1 < _previousPressure2_2 + pressureUpDownDelta) {
      return true;
    }

    return false;
  }

private:
  bool timeout(unsigned long start, uint16_t operationLimit) {
    if (millis() - start < operationLimit) {
      return false;
    }

    return true;
  }

  void incPercent() {
    if (!timeout(_percentChanged, 50)) {
      return;
    }

    _percentChanged = millis();

    if (_percent >= 100) {
      return;
    }

    _percent++;
  }

  void decPercent() {
    if (!timeout(_percentChanged, 100)) {
      return;
    }

    _percentChanged = millis();

    if (_percent == 0) {
      return;
    }

    _percent--;
  }

  void setPreviousPressure2(uint32_t pressure2) {
    if (!timeout(_previousPressure2Changed, 100)) {
      return;
    }

    _previousPressure2_1 = _previousPressure2_2;
    _previousPressure2_2 = pressure2;
  }

  const uint32_t closedPressure = 62000;
  const uint32_t limitPressure = 115000;
  const uint32_t pressureDelta = 1000;
  const uint32_t pressureUpDownDelta = 5000;
  const uint32_t pressureOver = 2000;
  const uint32_t maxPressure = 180000;

  const uint8_t boostOffTemperature = 80;
  const uint8_t boostLowerTemperature = 75;

  uint32_t _normalPressure = 100000;

  uint8_t _percent, _minPercent;
  unsigned long _percentChanged, _minPercentChanged, _previousPressure2Changed;

  uint32_t _pressure1Want;
  unsigned long _pressure1WantChanged;

  directionType _direction;

  bool _reached;

  uint32_t _previousPressure2_1, _previousPressure2_2;
};
