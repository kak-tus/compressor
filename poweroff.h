const int DEBOUNCE = 100;

class PowerOff {
public:
  PowerOff(byte pin) : _pin(pin) {
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
  const byte _pin;
  uint32_t _tmr;
  bool _previousState;
};
