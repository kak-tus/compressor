class Calibrate {
public:
  uint8_t position() {
    if (timeout(_changed, 300)) {
      _changed = millis();

      if (_to == 0) {
        _percent--;
      } else {
        _percent++;
      }

      if (_percent == 100) {
        _to = 0;
      } else if (_percent == 0) {
        _to = 100;
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
  uint8_t _to = 0;
};
