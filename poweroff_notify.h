class PowerOffNotify {
public:
  PowerOffNotify() { pinMode(SYSTEMLED, OUTPUT); }

  void poweroff() {
    if (!_on) {
      return;
    }

    digitalWrite(SYSTEMLED, HIGH);
    _on = false;
  }

  void poweron() {
    if (_on) {
      return;
    }

    digitalWrite(SYSTEMLED, LOW);
    _on = true;
  }

private:
  const uint8_t SYSTEMLED = 13;

  bool _on = true;
};
