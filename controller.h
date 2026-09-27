class Controller {
public:
  Controller() {}

  void control(uint8_t pos) {
    if (pos <= offPos) {
      if (_state != OFF) {
        _inState = millis();
        _state = OFF;
        _prevState = OFF;
      }

      if (_allowCompressor && timeout(_inState, offTimeout)) {
        _allowCompressor = false;

        Serial.println("Compressor: disallow");
      }

      if (_lowGas && timeout(_inState, lowGasOffTimeout)) {
        _lowGas = false;
      }
    } else if (pos >= onPos) {
      if (_state != ON) {
        _inState = millis();
        _state = ON;
        _prevState = ON;
        _lowGas = false;
      }

      if (!_allowCompressor && timeout(_inState, onTimeout)) {
        _allowCompressor = true;

        Serial.println("Compressor: allow");
      }
    } else if (pos >= cruiseFromPos && pos <= cruiseToPos) {
      if (_state != CRUISE) {
        _inState = millis();
        _state = CRUISE;
      }

      if (_prevState == OFF) {
        if (timeout(_inState, offInCruiseTimeout)) {
          if (!_lowGas) {
            _lowGas = true;
          }

          if (_allowCompressor) {
            _allowCompressor = false;

            Serial.println("Compressor: disallow");
          }
        } else {
          if (!_allowCompressor && timeout(_inState, onTimeout) && !_lowGas) {
            _allowCompressor = true;

            Serial.println("Compressor: allow");
          }
        }
      } else {
        if (_allowCompressor && timeout(_inState, onToCruiseTimeout)) {
          _allowCompressor = false;

          Serial.println("Compressor: disallow");
        }
      }
    }
  }

  void setTemperature(int16_t temperature) {
    if (temperature > boostOffTemperature) {
      _compressorBlocked = true;
    } else if (temperature < boostOffTemperature - 5) {
      _compressorBlocked = false;
    }
  }

  uint8_t allowCompressor() {
    if (_compressorBlocked) {
      return false;
    }

    return _allowCompressor;
  }

private:
  bool timeout(unsigned long start, uint16_t operationLimit) {
    if (millis() - start < operationLimit) {
      return false;
    }

    return true;
  }

  // 70 - is ok temperature
  // 80 - is bad, stop boost
  const int16_t boostOffTemperature = 80;

  unsigned long _inState;

  bool _allowCompressor = true, _compressorBlocked = false;

  const uint8_t onTimeout = 10;
  const uint8_t offTimeout = 1000;
  const uint16_t offInCruiseTimeout = 250;
  const uint16_t onToCruiseTimeout = 1000;
  const uint16_t lowGasOffTimeout = 5000;

  /*
  off - compressor is always off
  off -> cruise (and then on) - compressor is on
  on -> cruise - compressor is off after onCruiseTimeout
  off -> cruise (and stay in cruise) - compressor is off after offCruiseTimeout
  on - compressor is always on

  Off 0-1
  Cruise 3-12
  On 16-100
  */
  const uint8_t offPos = 1;
  const uint8_t cruiseFromPos = 3;
  const uint8_t cruiseToPos = 12;
  const uint8_t onPos = 16;

  enum state { OFF, CRUISE, ON };
  state _state = OFF, _prevState = OFF;

  bool _lowGas = false;
};
