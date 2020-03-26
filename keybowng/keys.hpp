#include <array>
#include <functional>
#include <memory>

#include "usb.hpp"
#pragma once

#define HID_REPORT_SIZE 16

class Keys {
public:
  using TKeyHandler = std::function<void(int key, bool state)>;

  Keys(std::shared_ptr<USB> usb);

  bool init(TKeyHandler keyHandler);
  void deinit();

  bool sendHIDReport(const unsigned char *buf, size_t len);
  bool sendMIDIData(const unsigned char *buf, size_t len);

  void update();

  struct Key {
    int gpio_bcm;
    int hid_code;
    int led_index;
  };

  static const std::array<Key, 12> mapping_table;
private:
  std::shared_ptr<USB> _usb;
  TKeyHandler _keyHandler;
  std::array<bool, mapping_table.size()> _states;
};

