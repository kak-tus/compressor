class Controller {
public:
  enum directionType { OPEN, CLOSE };

  Controller() {
    // Don't use real initial percent
    // this controller control how MUST be percent set, but not real percent
    // Real and wanted percents controlled by throttle
    _position = MAXIMUM_OPEN;
  }

  void poweroff() {
    _position = MAXIMUM_OPEN;
    _poweredoffAt = millis();
    _allowCompressor = false;
  }

  void poweron() {
    _position = MAXIMUM_OPEN;
    _allowCompressor = true;

    if (_poweredoffAt != 0 && !timeout(_poweredoffAt, 2000)) {
      if (_mode == PERFOMANCE) {
        _mode = NORMAL;

        Serial.println("Mode: normal");
      } else {
        _mode = PERFOMANCE;

        Serial.println("Mode: perfomance");
      }
    } else {
      _mode = NORMAL;

      Serial.println("Mode: normal");
    }
  }

  void control(uint8_t posMainThrottle) {
    if (_mode == PERFOMANCE) {
      // Don't use abs here, because we check only positive delta in case of
      // lower main throttle position
      int16_t delta =
          (int16_t)_prevPositionMainThrottle - (int16_t)posMainThrottle;

      if (delta > 2 && _blowOffCheckStartedAt == 0) {
        _blowOffCheckStartedAt = millis();
        _blowOffPositionCheckStarted = _prevPositionMainThrottle;

        Serial.print("Blowoff: start check from ");
        Serial.print(_prevPositionMainThrottle);
        Serial.print(" to ");
        Serial.println(posMainThrottle);
      }

      delta = (int16_t)_blowOffPositionCheckStarted - (int16_t)posMainThrottle;

      if (_blowOffCheckStartedAt != 0 && timeout(_blowOffCheckStartedAt, 100)) {
        if (delta >= BLOWOFF_DELTA) {
          setPosition(BLOWOFF_OPEN);
          _blowOffStartedAt = millis();

          _blowOffCheckStartedAt = 0;
          _blowOffPositionCheckStarted = 0;

          _prevPositionMainThrottle = posMainThrottle;
          _positionMainThrottleTimeout = millis();

          Serial.println("Blowoff: do blowoff");
        } else {
          _blowOffCheckStartedAt = 0;
          _blowOffPositionCheckStarted = 0;
        }
      } else if (delta >= BLOWOFF_DELTA) {
        setPosition(BLOWOFF_OPEN);
        _blowOffStartedAt = millis();

        _blowOffCheckStartedAt = 0;
        _blowOffPositionCheckStarted = 0;

        _prevPositionMainThrottle = posMainThrottle;
        _positionMainThrottleTimeout = millis();

        Serial.println("Blowoff: do blowoff");
      }

      if (_blowOffStartedAt == 0 || timeout(_blowOffStartedAt, 100)) {
        setPosition(MAXIMUM_CLOSE);
      }

      if (timeout(_positionMainThrottleTimeout, 50)) {
        _prevPositionMainThrottle = posMainThrottle;
        _positionMainThrottleTimeout = millis();
      }
    } else {
      if (posMainThrottle < 5) {
        _allowAt = 0;

        if (_allowCompressor && _disallowAt == 0) {
          _disallowAt = millis();
        }
      } else if (posMainThrottle > 10) {
        _disallowAt = 0;

        if (!_allowCompressor && _allowAt == 0) {
          _allowAt = millis();
        }
      }

      if (_disallowAt != 0 && timeout(_disallowAt, disallowTimeout)) {
        _allowCompressor = false;
        _disallowAt = 0;
        _allowAt = 0;

        Serial.println("Disallow compressor: disallow");
      } else if (_allowAt != 0 && timeout(_allowAt, allowTimeout)) {
        _allowCompressor = true;
        _disallowAt = 0;
        _allowAt = 0;

        Serial.println("Disallow compressor: allow");
      }

      if (posMainThrottle < 10) {
        setPosition(MAXIMUM_OPEN);
      } else if (posMainThrottle > 25) {
        setPosition(MAXIMUM_CLOSE);
      }
    }
  }

  uint8_t position() { return _position; }

  void setTemperature(int16_t temperature) {
    if (temperature > boostOffTemperature) {
      _minPosition = 100;
    } else {
      _minPosition = MAXIMUM_CLOSE;
    }
  }

  uint8_t positionVal() { return _position; }
  uint8_t allowCompressor() { return _allowCompressor; }

private:
  bool timeout(unsigned long start, uint16_t operationLimit) {
    if (millis() - start < operationLimit) {
      return false;
    }

    return true;
  }

  void setPosition(uint8_t position) {
    if (position <= _minPosition) {
      _position = _minPosition;
    } else {
      _position = position;
    }
  }

  const uint8_t MAXIMUM_OPEN = 30;
  const uint8_t MAXIMUM_CLOSE = 0;
  const uint8_t BLOWOFF_OPEN = 10;

  // 70 - is ok temperature
  // 80 - is bad, stop boost
  const int16_t boostOffTemperature = 80;

  uint8_t _position, _minPosition;

  enum modeType { NORMAL = 0, PERFOMANCE = 1, COMFORT = 2 };
  modeType _mode = NORMAL;

  unsigned long _poweredoffAt;

  uint8_t _prevPositionMainThrottle, _blowOffPositionCheckStarted;

  unsigned long _blowOffCheckStartedAt, _blowOffStartedAt,
      _positionMainThrottleTimeout, _disallowAt, _allowAt;

  const uint8_t BLOWOFF_DELTA = 10;

  bool _allowCompressor = true;

  const uint8_t allowTimeout = 100;
  const uint8_t disallowTimeout = 1000;
};
