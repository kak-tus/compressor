class Controller {
public:
  enum directionType { OPEN, CLOSE };

  Controller() {
    // Don't use real initial percent
    // this controller control how MUST be percent set, but not real percent
    // Real and wanted percents controlled by throttle
    _position = 100;
  }

  uint8_t position(uint32_t pressure1, uint32_t pressure2) {
    setPreviousPressure(pressure1, pressure2);

    if (isBlowoff(pressure1)) {
      if (timeout(_logBlowoff, 1000)) {
        Serial.print("Blowoff: from ");
        Serial.print(_previousPressure1);
        Serial.print(" to ");
        Serial.println(pressure1);
      }

      setPosition(100);
    } else if (pressure2 > limitPressure2 + pressureDelta) {
      if (timeout(_logLimit, 1000)) {
        Serial.print("Pressure limit: ");
        Serial.println(pressure2);
      }

      incPosition();
    } else if (isEngineIdle(pressure2)) {
      setPosition(100);
    } else {
      decPosition();
    }

    return _position;
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

  uint8_t positionVal() { return _position; }

  bool isEngineIdle(uint32_t pressure2) {
    if (pressure2 < closedPressure2 - pressureIdleDelta) {
      return false;
    } else if (pressure2 > closedPressure2 + pressureIdleDelta) {
      return false;
    }

    if (_previousPressure2 < closedPressure2 - pressureIdleDelta) {
      return false;
    } else if (_previousPressure2 > closedPressure2 + pressureIdleDelta) {
      return false;
    }

    return true;
  }

  bool isBlowoff(uint32_t pressure1) {
    if (pressure1 > _previousPressure1 + blowoffDelta) {
      return true;
    } else {
      return false;
    }
  }

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

    if (_position >= 100) {
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

  void setPosition(uint8_t position) {
    _positionChanged = millis();
    _position = 100;
  }

  void setPreviousPressure(uint32_t pressure1, uint32_t pressure2) {
    if (!timeout(_previousPressureChanged, 100)) {
      return;
    }

    _previousPressureChanged = millis();

    _previousPressure1 = pressure1;
    _previousPressure2 = pressure2;
  }

  const uint32_t closedPressure2 = 42000;
  const uint32_t limitPressure2 = 115000;
  const uint32_t pressureDelta = 2000;
  const uint32_t pressureIdleDelta = 4000;
  const uint32_t maxPressure = 180000;
  const uint32_t blowoffDelta = 10000;

  const uint8_t closeDelay = 10;
  const uint8_t openDelay = 10;

  const uint8_t boostOffTemperature = 80;
  const uint8_t boostLowerTemperature = 75;

  uint8_t _position, _minPercent;
  unsigned long _positionChanged, _minPercentChanged, _logLimit, _logBlowoff;

  uint32_t _previousPressure1, _previousPressure2;
  unsigned long _previousPressureChanged;
};
