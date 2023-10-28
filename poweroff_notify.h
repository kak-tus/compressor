const uint8_t SYSTEMLED = 13;

class PowerOffNotify {
public:
  PowerOffNotify() { pinMode(SYSTEMLED, OUTPUT); }

  void poweroff() {
    if (_on) {
      return;
    }

    digitalWrite(SYSTEMLED, HIGH);
  }

  void poweron() {
    if (!_on) {
      return;
    }

    digitalWrite(SYSTEMLED, LOW);
  }

private:
  bool _on;
};
