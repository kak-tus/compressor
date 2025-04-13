class Controller {
public:
  enum directionType { OPEN, CLOSE };

  Controller() {
    // Don't use real initial percent
    // this controller control how MUST be percent set, but not real percent
    // Real and wanted percents controlled by throttle
    _position = 100;
    _idle = true;
    _idleDetectedAt = 0;
  }

  void poweroff() {
    _position = 100;
    _idle = true;
    _idleDetectedAt = 0;
  }

  void poweron() {
    _position = 100;
    _idle = true;
    _idleDetectedAt = 0;
  }

  void control(uint16_t pressure1, uint16_t pressure2, bool poweredoff) {
    checkEngineIdle(pressure1, pressure2, poweredoff);
    checkEngineAccelerate(pressure2);

    setPreviousPressure(pressure1, pressure2);
  }

  uint8_t position(uint16_t pressure1, uint16_t pressure2, bool poweredoff) {
    if (pressure2 > limitPressure2) {
      if (timeout(_logLimit, 1000)) {
        _logLimit = millis();

        Serial.print("Pressure limit: pressure1=");
        Serial.print(pressure1);
        Serial.print(", pressure2=");
        Serial.println(pressure2);
      }

      incPosition();
    } else if (isEngineIdle()) {
      incPosition();
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

    // Serial.print("pressure2=");
    // Serial.print("pressure2=");
    // Serial.print(pressure2);
    // Serial.print(" _previousPressure2=");
    // Serial.println(_previousPressure2);

    if (idleNow) {
      _idleDetectedAt = millis();
    }

    _idle = idleNow;
  }

  void checkEngineAccelerate(uint16_t pressure2) {
    if (pressure2 > closedPressure2MaxThrottle100 &&
        pressure2 > _previousPressure2_0 &&
        _previousPressure2_0 > _previousPressure2) {
      _idle = false;
    }
  }

  const uint16_t closedPressure2MinPoweredOff = 26;
  const uint16_t closedPressure2MaxPoweredOff = 50;

  const uint16_t closedPressure2MinThrottle100 = 18;
  const uint16_t closedPressure2MaxThrottle100 = 70;

  const uint16_t closedPressure2MinThrottle0 = 16;
  const uint16_t closedPressure2MaxThrottle0 = 72;

  const uint16_t idlePressure1 = 110;

  const uint16_t limitPressure2 = 130;

  const uint8_t closeDelay = 2;
  const uint8_t openDelay = 15;

  // 60-70 - is ok temperature
  // 80 - is bad, stop boost
  const int16_t boostOffTemperature = 80;
  const int16_t boostLowerTemperature = 75;

  uint8_t _position, _minPercent;
  unsigned long _positionChanged, _minPercentChanged, _logLimit,
      _logIdle;

  uint16_t _previousPressure1, _previousPressure2, _previousPressure1_0,
      _previousPressure2_0;
  unsigned long _previousPressureChanged;

  unsigned long _idleDetectedAt;
  const uint16_t idleSwitchOnTimeout = 3000;
  bool _idle;
};
