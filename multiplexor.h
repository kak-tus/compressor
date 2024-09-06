#include <Mux.h>

class Multiplexor {
public:
  Multiplexor(uint8_t ZPin, uint8_t EPin, uint8_t S0Pin, uint8_t S1Pin,
               uint8_t GndChannel)
      : mux(admux::Pin(ZPin, INPUT, admux::PinType::Analog),
            admux::Pinset(S0Pin, S1Pin)),
        _gndChannel(GndChannel) {}

  uint16_t read(uint8_t channel) {
    mux.enabled(false);
    mux.channel(channel);
    mux.enabled(true);
    uint16_t val = mux.read();

    mux.enabled(false);
    mux.channel(_gndChannel);
    mux.enabled(true);
    mux.read(_gndChannel);

    return val;
  }

private:
  admux::Mux mux;
  uint8_t _gndChannel;
};
