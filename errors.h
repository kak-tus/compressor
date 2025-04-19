#ifndef _Errors_h
#define _Errors_h

class Errors {
public:
  // 0 - short beep
  // 1 - long beep
  static const uint8_t ERR_THR_SENSORS = 0b0;

  Errors(uint8_t pin) : _pin(pin) { pinMode(pin, OUTPUT); }

  void error(uint8_t code) {
    for (int8_t i = 0; i >= 0; i--) {
      if (bitRead(code, i) == 0) {
        tone(_pin, 1000);
        delay(300);
        noTone(_pin);
        delay(300);
      } else {
        tone(_pin, 1000);
        delay(1000);
        noTone(_pin);
        delay(1000);
      }
    }
  }

private:
  const uint8_t _pin;

  bool _on = true;
};

#endif
