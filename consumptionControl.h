class ConsumptionControl {
public:
  ConsumptionControl(uint8_t pin, uint8_t minimalConsumption,
                     uint16_t (*mux)(uint8_t))
      : _pin(pin), _minimalConsumption(minimalConsumption), _mux(mux) {}

  uint16_t consumption() { return _mux(_pin); }

  bool failed() {
    if (_mux(_pin) < _minimalConsumption) {
      return true;
    }

    return false;
  }

private:
  const uint8_t _pin, _minimalConsumption;
  uint16_t (*_mux)(uint8_t);
};
