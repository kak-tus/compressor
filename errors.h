class Errors {
public:
  Errors(uint8_t pin) : _pin(pin) { pinMode(pin, OUTPUT); }

  void error(uint8_t code) {
    for (uint8_t i = 0; i < code; i++) {
      tone(_pin, 1000);
      delay(1000);
      noTone(_pin);
      delay(1000);
    }
  }

private:
  const uint8_t _pin;

  bool _on = true;
};
