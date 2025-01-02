#ifndef _Errors_h
#define _Errors_h

class Errors {
public:
  // 0 - short beep
  // 1 - long beep
  static const uint8_t ERR_THR_CHECK_1 = 0b0000;
  static const uint8_t ERR_THR_CHECK_2 = 0b0001;
  static const uint8_t ERR_THR_CHECK_3 = 0b0010;
  static const uint8_t ERR_THR_CHECK_4 = 0b0011;
  static const uint8_t ERR_THR_CHECK_5 = 0b0100;
  static const uint8_t ERR_THR_CHECK_6 = 0b0101;
  static const uint8_t ERR_THR_SENSORS_1 = 0b0110;
  static const uint8_t ERR_THR_SENSORS_2 = 0b0111;
  static const uint8_t ERR_THR_SENSORS_3 = 0b1000;
  static const uint8_t ERR_THR_SENSORS_4 = 0b1001;
  static const uint8_t ERR_THR_SENSORS_5 = 0b1010;
  static const uint8_t ERR_COMPRESSOR_CONSUMPTION = 0b1011;

  Errors(uint8_t pin) : _pin(pin) { pinMode(pin, OUTPUT); }

  void error(uint8_t code) {
    for (int8_t i = 3; i >= 0; i--) {
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
