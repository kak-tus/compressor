class Calibrate {
public:
  uint8_t percent() {
    if (timeout(_changed, 30000)) {
      _changed = millis();

      if (_percent == 100) {
        _percent = 0;
      } else {
        _percent = 100;
      }
    }

    return _percent;
  }

private:
  bool timeout(unsigned long start, uint16_t operationLimit) {
    if (millis() - start < operationLimit) {
      return false;
    }

    return true;
  }

  uint8_t _percent = 100;
  unsigned long _changed;
};
