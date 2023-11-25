class Controller {
public:
  Controller() {}

  uint8_t percent(float pressureIn, float pressureOut) {
    return 50;
    if (pressureOut < closedPressure) {
      // return 100;
    }

    if (pressureOut > maxPressure) {
      if (_percent < 100 && timeout(_percentChanged, 10)) {
        // _percent++;
        _percentChanged = millis();
      }
    } else if (pressureIn - pressureDelta < pressureOut) {
      if (_percent > 0 && timeout(_percentChanged, 100)) {
        _percent--;
        _percentChanged = millis();
      }
    } else if (pressureIn - pressureDelta > pressureOut) {
      if (_percent < 100 && timeout(_percentChanged, 10)) {
        // _percent++;
        _percentChanged = millis();
      }
    }

    if (_percent < _minPercent) {
      // _percent = _minPercent;
    }

    return _percent;
  }

  void setTemperature(float temperature) {
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

private:
  const float closedPressure = 60000;
  const float maxPressure = 115000;
  const float pressureDelta = 5000;

  const float boostOffTemperature = 80;
  const float boostLowerTemperature = 75;

  uint8_t _percent, _minPercent;
  unsigned long _percentChanged, _minPercentChanged;
};
