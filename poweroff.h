class PowerOff {
public:
  PowerOff(uint8_t pin) : _pin(pin) {
    pinMode(_pin, INPUT);
    _tmr = millis();
  }

  bool need() {
    bool powerState = digitalRead(_pin);

    if (_previousState != powerState) {
      _previousState = powerState;
      _tmr = millis();
    }

    if (!powerState && millis() - _tmr >= DEBOUNCE) {
      _previousState = powerState;
      return true;
    }

    return false;
  }

private:
  const int DEBOUNCE = 100;

  const uint8_t _pin;
  uint32_t _tmr;
  bool _previousState;
};
