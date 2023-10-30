class Sensor {
public:
  Sensor(byte pin) : _pin(pin) {
    pinMode(_pin, INPUT);
  }

  int temperature() {
    int temperature = analogRead(_pin);
    return temperature;
  }

private:
  const byte _pin;
};
