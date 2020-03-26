#include <unistd.h>
#include <bcm2835.h>

#include <plog/Log.h>

#include "keys.hpp"

using namespace std;

Keys::Keys(shared_ptr<USB> usb):
  _usb(usb)
  {}

bool Keys::init(TKeyHandler keyHandler) {
  if (!bcm2835_init()) {
    return false;

  }

  for (int i = 0; i < mapping_table.size(); ++i) {
    auto &key = mapping_table[i];
    bcm2835_gpio_fsel(key.gpio_bcm, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(key.gpio_bcm, BCM2835_GPIO_PUD_UP);
    _states[i] = 0;
  }

  _keyHandler = keyHandler;
  return true;
}

void Keys::deinit() {
  bcm2835_close();
}

bool Keys::sendHIDReport(const unsigned char *buf, size_t len) {
  return _usb->sendHIDReport(buf, len);
}

bool Keys::sendMIDIData(const unsigned char *buf, size_t len) {
  return _usb->sendMIDIData(buf, len);
}

void Keys::update() {
  for (int i = 0; i < mapping_table.size(); ++i) {
    auto &key = mapping_table[i];

    bool state = !bcm2835_gpio_lev(key.gpio_bcm);
    if (state != _states[i]) {
      PLOG_DEBUG << "update " << i << " " << state;
      _keyHandler(i, state);
    }
    _states[i] = state;
  }
}

const std::array<Keys::Key, 12> Keys::mapping_table = {
  Key { RPI_V2_GPIO_P1_11, 0x27, 3 },
  Key { RPI_V2_GPIO_P1_13, 0x37, 7 },
  Key { RPI_V2_GPIO_P1_16, 0x28, 11 },
  Key { RPI_V2_GPIO_P1_15, 0x1e, 2 },
  Key { RPI_V2_GPIO_P1_18, 0x1f, 6 },
  Key { RPI_V2_GPIO_P1_29, 0x20, 10 },
  Key { RPI_V2_GPIO_P1_31, 0x21, 1 },
  Key { RPI_V2_GPIO_P1_32, 0x22, 5 },
  Key { RPI_V2_GPIO_P1_33, 0x23, 9 },
  Key { RPI_V2_GPIO_P1_38, 0x24, 0 },
  Key { RPI_V2_GPIO_P1_36, 0x25, 4 },
  Key { RPI_V2_GPIO_P1_37, 0x26, 8 },
};
