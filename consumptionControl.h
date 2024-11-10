class ConsumptionControl {
public:
  ConsumptionControl(uint8_t acsPin, uint8_t isPin, uint16_t (*mux)(uint8_t))
      : _acsPin(acsPin), _isPin(isPin), _mux(mux) {}

  uint16_t consumptionACS() { return _mux(_acsPin); }
  uint16_t consumptionIS() { return _mux(_isPin); }

  bool failed() {
    if (_mux(_acsPin) > maxACSConsumption) {
      return true;
    }

    // Here we check two variants:
    // current from throttle driver
    // and error flag from throttle driver
    // error flag == 1 in digital mode, so we get maximum value from analog read
    if (_mux(_isPin) > maxISConsumption) {
      return true;
    }

    return false;
  }

private:
  const uint8_t _acsPin, _isPin;
  uint16_t (*_mux)(uint8_t);

  const uint16_t maxACSConsumption = 1000;
  // From BTS 7960 docs
  // a resistor value of RIS = 1kΩ leads
  // to VIS = (IL / 8.5 A)V
  const uint16_t maxISConsumption = 1000;
};
