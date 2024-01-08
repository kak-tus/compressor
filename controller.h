class Controller {
public:
  Controller() {
    // Don't use real initial percent
    // this controller control how MUST be percent set, but not real percent
    // Real and wanted percents controlled by throttle
    _percent = 100;
  }

  uint8_t percent(uint32_t pressure1, uint32_t pressure2) {
    return 100;
    // if (pressure1 < pressure2) {
    //   // TODO add error
    //   // It is not normal situation
    //   // Possible sensor error
    //   return 100;
    // }

    // With closed throttle
    if (pressure2 <= closedPressure) {
      // No need in high pressure1 if it bigger, then normal pressure
      if (pressure1 > _normalPressure + pressureDelta && _percent < 100 &&
          timeout(_percentChanged, 100)) {
        _percent++;
        _percentChanged = millis();
      }

      return _percent;
    }

    if (pressure2 >= limitPressure) {
      if (pressure1 > limitPressure + pressureDelta && _percent < 100 &&
          timeout(_percentChanged, 100)) {
        _percent++;
        _percentChanged = millis();
      }

      return _percent;
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

  bool timeout(unsigned long start, uint16_t operationLimit) {
    if (millis() - start < operationLimit) {
      return false;
    }

    return true;
  }

  void setNormalPressure(uint32_t pressure) { _normalPressure = pressure; }

private:
  const uint32_t closedPressure = 60000;
  const uint32_t limitPressure = 115000;
  const uint32_t pressureDelta = 5000;

  const uint8_t boostOffTemperature = 80;
  const uint8_t boostLowerTemperature = 75;

  uint8_t _percent, _minPercent;
  unsigned long _percentChanged, _minPercentChanged;
  uint32_t _normalPressure = 115000;
};
