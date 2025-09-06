#define PID_INTEGER
#include <GyverPID.h>

class TemperatureControl {
public:
  TemperatureControl(uint8_t controlPin, int8_t onTemp, int8_t offTemp,
                     bool isPWM)
      : _pin(controlPin), _onTemp(onTemp), _offTemp(offTemp), _isPWM(isPWM) {
    pinMode(_pin, OUTPUT);

    if (isPWM) {
      analogWrite(_pin, 0);
    } else {
      digitalWrite(_pin, OFF);
    }

    regulator.setpoint = offTemp;
    regulator.setDirection(REVERSE);
    regulator.setLimits(minPWM, maxPWM);
  }

  void control(int16_t temp) {
    if (temp >= _onTemp) {
      _on = true;

      if (_isPWM) {
        regulator.input = temp;
        analogWrite(_pin, regulator.getResult());
      } else {
        digitalWrite(_pin, ON);
      }
    }

    if (temp <= _offTemp) {
      if (!_on) {
        return;
      }

      if (_isPWM) {
        analogWrite(_pin, 0);
      } else {
        digitalWrite(_pin, OFF);
      }

      _on = false;
    }
  }

  void poweroff() {
    if (!_on) {
      return;
    }

    if (_isPWM) {
      analogWrite(_pin, 0);
    } else {
      digitalWrite(_pin, OFF);
    }

    _on = false;
  }

  int regulatorValue() { return regulator.getResult(); }

private:
  const uint8_t _pin;
  const int8_t _onTemp, _offTemp;
  const bool _isPWM;

  const uint8_t ON = HIGH;
  const uint8_t OFF = LOW;

  bool _on = false;

  const uint16_t controlTimeout = 1000;

  GyverPID regulator = GyverPID(1, 0.1, 0.1, controlTimeout);

  // Manually tested minimal start cooler speed
  const uint8_t minPWM = 40;
  const uint8_t maxPWM = 255;
};
