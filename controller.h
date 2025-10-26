class Controller {
public:
  Controller() {}

  void poweroff() {
    _poweredoffAt = millis();
    _allowCompressor = false;
    _compressorBlocked = false;
  }

  void poweron() {
    _poweredonAt = millis();
    _compressorBlocked = false;

    if (_poweredoffAt != 0 && !timeout(_poweredoffAt, 2000)) {
      if (_mode == NORMAL) {
        _mode = COMFORT;
        _allowCompressor = false;

        Serial.println("Mode: comfort");
      } else {
        _mode = NORMAL;
        _allowCompressor = true;

        Serial.println("Mode: normal");
      }
    } else {
      _mode = NORMAL;
      _allowCompressor = false;

      Serial.println("Mode: normal");
    }
  }

  void control(uint8_t posMainThrottle) {
    if (posMainThrottle <= 3) {
      _allowAt = 0;

      if (_allowCompressor && _disallowAt == 0) {
        _disallowAt = millis();
      }
    } else if (posMainThrottle > 3) {
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
  }

  void setTemperature(int16_t temperature) {
    if (temperature > boostOffTemperature) {
      _compressorBlocked = true;
    } else {
      _compressorBlocked = false;
    }
  }

  uint8_t allowCompressor() {
    // Don't switch compressor on while power on engine
    // To fix hight RPM
    if (!timeout(_poweredonAt, engineOnTimeout)) {
      return false;
    }

    if (_compressorBlocked) {
      return false;
    }

    return _allowCompressor;
  }

private:
  bool timeout(unsigned long start, uint16_t operationLimit) {
    if (millis() - start < operationLimit) {
      return false;
    }

    return true;
  }

  // 70 - is ok temperature
  // 80 - is bad, stop boost
  const int16_t boostOffTemperature = 80;

  enum modeType { COMFORT = 0, NORMAL = 1 };
  modeType _mode = NORMAL;

  unsigned long _poweredoffAt, _poweredonAt;

  unsigned long _disallowAt, _allowAt;

  bool _allowCompressor = true, _compressorBlocked = false;

  const uint8_t allowTimeout = 100;
  const uint16_t disallowTimeout = 1000;
  const uint16_t engineOnTimeout = 3000;
};
