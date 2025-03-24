class Controller {
public:
  enum directionType { OPEN, CLOSE };

  Controller() {
    // Don't use real initial percent
    // this controller control how MUST be percent set, but not real percent
    // Real and wanted percents controlled by throttle
    _position = 100;
  }

  void control(uint16_t pressure1, uint16_t pressure2, bool poweredoff) {
    setPreviousPressure(pressure1, pressure2);
    checkEngineIdle(pressure1, pressure2, poweredoff);
  }

  uint8_t position(uint16_t pressure1, uint16_t pressure2, bool poweredoff) {
    if (isBlowoff(pressure1)) {
      if (timeout(_logBlowoff, 1000)) {
        _logBlowoff = millis();

        Serial.print("Blowoff: from ");
        Serial.print(_previousPressure1);
        Serial.print(" to ");
        Serial.println(pressure1);
      }

      setPosition(100);
    } else if (pressure2 > limitPressure2 + pressureDelta) {
      if (timeout(_logLimit, 1000)) {
        _logLimit = millis();

        Serial.print("Pressure limit: ");
        Serial.println(pressure2);
      }

      incPosition();
    } else if (isEngineIdle()) {
      setPosition(100);
    } else {
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

  bool isEngineIdle() {
    if (_idle) {
      if (timeout(_idleDetectedAt, idleSwitchOnTimeout)) {
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  bool isBlowoff(uint16_t pressure1) {
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
    _position = position;
  }

  void setPreviousPressure(uint16_t pressure1, uint16_t pressure2) {
    if (!timeout(_previousPressureChanged, 50)) {
      return;
    }

    _previousPressureChanged = millis();

    _previousPressure1 = _previousPressure1_0;
    _previousPressure2 = _previousPressure2_0;

    _previousPressure1_0 = pressure1;
    _previousPressure2_0 = pressure2;
  }

  void checkEngineIdle(uint16_t pressure1, uint16_t pressure2,
                       bool poweredoff) {
    if (!poweredoff && _debugMinMax) {
      bool log;

      if (pressure2 < _min) {
        _min = pressure2;
        log = true;
      }
      if (pressure2 > _max) {
        _max = pressure2;
        log = true;
      }
      if (_previousPressure2 < _min) {
        _min = _previousPressure2;
        log = true;
      }
      if (_previousPressure2 > _max) {
        _max = _previousPressure2;
        log = true;
      }

      if (log) {
        Serial.print("min=");
        Serial.print(_min);
        Serial.print(", _max=");
        Serial.println(_max);
      }
    }

    uint16_t min, max;
    bool idleNow;

    if (poweredoff) {
      min = closedPressure2MinPoweredOff;
      max = closedPressure2MaxPoweredOff;
    } else {
      if (pressure1 < idlePressure1) {
        min = closedPressure2MinThrottle100;
        max = closedPressure2MaxThrottle100;
      } else {
        min = closedPressure2MinThrottle0;
        max = closedPressure2MaxThrottle0;
      }
    }

    if (pressure2 < min) {
      idleNow = false;
    } else if (pressure2 > max) {
      idleNow = false;
    } else if (_previousPressure2 < min) {
      idleNow = false;
    } else if (_previousPressure2 > max) {
      idleNow = false;
    } else {
      idleNow = true;
    }

    if (idleNow == _idle) {
      return;
    }

    if (idleNow) {
      _idleDetectedAt = millis();
    }

    _idle = idleNow;
  }

  const uint16_t closedPressure2MinPoweredOff = 26;
  const uint16_t closedPressure2MaxPoweredOff = 50;

  const uint16_t closedPressure2MinThrottle100 = 20;
  const uint16_t closedPressure2MaxThrottle100 = 70;

  const uint16_t closedPressure2MinThrottle0 = 16;
  const uint16_t closedPressure2MaxThrottle0 = 72;

  const uint16_t idlePressure1 = 110;

  const uint16_t limitPressure2 = 170;
  const uint16_t pressureDelta = 5;
  const uint16_t blowoffDelta = 20;

  const uint8_t closeDelay = 5;
  const uint8_t openDelay = 2;

  // 60-70 - is ok temperature
  // 80 - is bad, stop boost
  const int16_t boostOffTemperature = 80;
  const int16_t boostLowerTemperature = 75;

  uint8_t _position, _minPercent;
  unsigned long _positionChanged, _minPercentChanged, _logLimit, _logBlowoff,
      _logIdle;

  uint16_t _previousPressure1, _previousPressure2, _previousPressure1_0,
      _previousPressure2_0;
  unsigned long _previousPressureChanged;

  unsigned long _idleDetectedAt;
  const uint16_t idleSwitchOnTimeout = 1000;
  bool _idle;

  uint16_t _min = 100, _max = 10;
  const bool _debugMinMax = false;
};
