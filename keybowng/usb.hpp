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

  bool sendHIDReport(const unsigned char *buf, size_t len);
  bool sendMIDIData(const unsigned char *buf, size_t len);

  void deinit();
private:
  USBImpl *_impl;
};

