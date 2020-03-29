#include <string>

#pragma once

#define VENDOR          0x1d6b
#define PRODUCT         0x0104
#define HID_REPORT_SIZE 16

class USBImpl;

class USB {
public:
  USB();
  ~USB();
  bool init();
  bool initNull();

  bool sendHIDReport(std::string report);
  bool sendMIDIReport(std::string report);

  void deinit();
private:
  USBImpl *_impl;
};

