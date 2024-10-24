#define PID_INTEGER
#include <GyverPID.h>

class TemperatureControl {
public:
  TemperatureControl(uint8_t controlPin, int8_t onTemp, int8_t offTemp)
      : _pin(controlPin), _onTemp(onTemp), _offTemp(offTemp) {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, OFF);

    regulator.setpoint = onTemp;
    regulator.setDirection(REVERSE);
    regulator.setLimits(minPWM, maxPWM);
  }

  void control(int16_t temp) {
    if (temp > _onTemp) {
      if (_on) {
        return;
      }

      digitalWrite(_pin, ON);
      _on = true;
    }

    if (temp < _offTemp) {
      if (!_on) {
        return;
      }

      digitalWrite(_pin, OFF);
      _on = false;
    }
  }

  void poweroff() {
    if (!_on) {
      return;
    }

    digitalWrite(_pin, OFF);
    _on = false;
  }

  void controlPWM(int16_t temp) {
    if (temp > _onTemp) {
      _on = true;
      regulator.input = temp;
      analogWrite(_pin, regulator.getResult());
    }

    if (temp < _offTemp) {
      if (!_on) {
        return;
      }

      digitalWrite(_pin, OFF);
      _on = false;
    }
  }

private:
  const uint8_t _pin;
  const int8_t _onTemp, _offTemp;

  const uint8_t ON = LOW;
  const uint8_t OFF = HIGH;

  bool _on = false;

  const uint16_t controlTimeout = 1000;

  GyverPID regulator = GyverPID(1, 0.1, 0.1, controlTimeout);

  // Manually tested minimal start cooler speed
  const uint8_t minPWM = 40;
  const uint8_t maxPWM = 255;
};
