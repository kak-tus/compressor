class Switch {
public:
  Switch(uint8_t controlPin) : _pin(controlPin) {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, OFF);
  }

  void poweron() {
    if (_on) {
      return;
    }

    digitalWrite(_pin, ON);
    _on = true;
  }

  void poweroff() {
    if (!_on) {
      return;
    }

    digitalWrite(_pin, OFF);
    _on = false;
  }

private:
  const uint8_t _pin;

  const uint8_t ON = LOW;
  const uint8_t OFF = HIGH;

  bool _on = false;
};
