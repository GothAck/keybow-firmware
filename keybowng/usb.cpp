#include <linux/usb/ch9.h>

#include <usbg/usbg.h>
#include <usbg/function/hid.h>
#include <usbg/function/midi.h>

#include <plog/Log.h>

#include "usb.hpp"

using namespace std;

struct USBImpl {
  USBImpl(USB *p): p(p) {}

  USB *p;

  usbg_state *s = nullptr;
  usbg_gadget *g = nullptr;
  usbg_config *c = nullptr;
  usbg_function *f_hid = nullptr;
  usbg_function *f_midi = nullptr;
  usbg_function *f_acm0 = nullptr;

  int fd_hid = -1;
  int fd_midi = -1;
};

char report_desc[] = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xA1, 0x01, // COLLECTION (Application)

    0x85, 0x01, // REPORT_ID (1)

    //             Bitmapped Modifiers

    0x05, 0x07, // USAGE_PAGE (Key Codes)
    0x19, 0xE0, // USAGE_MINIMUM (224)
    0x29, 0xE7, // USAGE_MAXIMUM (231)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x75, 0x01, // REPORT_SIZE (1)
    0x95, 0x08, // REPORT_COUNT (8)
    0x81, 0x02, // INPUT (Data, Variable, Absolute)

    //             Reserved

    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x08, // REPORT_SIZE (8)
    0x81, 0x01, // INPUT (Constant)

    //             LEDs

    0x05, 0x08, // USAGE_PAGE (LED)
    0x19, 0x01, // USAGE_MINIMUM (1)
    0x29, 0x05, // USAGE_MAXIMUM (5)
    0x95, 0x05, // REPORT_COUNT (5)
    0x75, 0x01, // REPORT_SIZE (1)
    0x91, 0x02, // OUTPUT (Data, Variable, Absolute)

    //             Padding

    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x03, // REPORT_SIZE (3)
    0x91, 0x03, // OUTPUT (Constant)

    //             Keyboard Keys

    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0xFF, // LOGICAL_MAXIMUM (255)
    0x05, 0x07, // USAGE_PAGE (Key Codes)
    0x19, 0x00, // USAGE_MINIMUM (0)
    0x29, 0xFF, // USAGE_MAXIMUM (255)
    0x95, 13,   // REPORT_COUNT (12)
    0x75, 0x08, // REPORT_SIZE (8)
    0x81, 0x00, // INPUT (Data, Array, Absolute)

    0xC0,       // END_COLLECTION

    //             Media Keys

    0x05, 0x0C, // USAGE_PAGE (Conumer)
    0x09, 0x01, // USAGE (Consumer Control)
    0xA1, 0x01, // Collection (Application)

    0x85, 0x02, //   Report ID (2)

    0x05, 0x0C, //   Usage Page (Consumer)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)

    0x95, 0x06, //   Report Count (6)
    0x09, 0xB5, //   Usage (Scan Next Track)
    0x09, 0xB6, //   Usage (Scan Previous Track)
    0x09, 0xB7, //   Usage (Stop)
    0x09, 0xB8, //   Usage (Eject)
    0x09, 0xCD, //   Usage (Play/Pause)
    0x09, 0xE2, //   Usage (Mute)
    0x81, 0x06, //   Input (Data,Var,Rel)

    0x95, 0x02, //   Report Count (2)
    0x09, 0xE9, //   Usage (Volume Increment)
    0x09, 0xEA, //   Usage (Volume Decrement)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0xC0,       // END_COLLECTION
};

struct usbg_gadget_attrs g_attrs = {
  .bcdUSB = 0x0200,
  .bDeviceClass = USB_CLASS_PER_INTERFACE,
  .bDeviceSubClass = 0x00,
  .bDeviceProtocol = 0x00,
  .bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
  .idVendor = VENDOR,
  .idProduct = PRODUCT,
  .bcdDevice = 0x0001, /* Verson of device */
};

struct usbg_gadget_strs g_strs = {
  .manufacturer = "Pimoroni", /* Manufacturer */
  .product = "Keybow", /* Product string */
  .serial = "0123456789", /* Serial number */
};

struct usbg_config_strs c_strs = {
  .configuration = "1xHID",
};

struct usbg_f_midi_attrs midi_attrs = {
  .index = 1,
  .id = "usb1",
  .in_ports = 1,
  .out_ports = 1,
  .buflen = 128,
  .qlen = 16,
};

struct usbg_f_hid_attrs f_attrs = {
  .protocol = 1,
  .report_desc = {
    .desc = (char *)&report_desc,
    .len = sizeof(report_desc),
  },
  .report_length = HID_REPORT_SIZE,
  .subclass = 0,
};

USB::USB(): _impl(new USBImpl(this)) {}

USB::~USB() {
  delete _impl;
}

inline void plog_usbg_err(int usbg_ret) {
  auto err = static_cast<usbg_error>(usbg_ret);
  PLOG_ERROR << "Error:" << usbg_error_name(err) << " : " << usbg_strerror(err);
}

bool USB::init() {
  int usbg_ret;

  usbg_ret = usbg_init("/sys/kernel/config", &_impl->s);
  if (usbg_ret != USBG_SUCCESS) {
    PLOG_ERROR << "Error on usbg init";
    plog_usbg_err(usbg_ret);
    return false;
  }

  usbg_ret = usbg_create_gadget(_impl->s, "g1", &g_attrs, &g_strs, &_impl->g);
  if (usbg_ret != USBG_SUCCESS) {
    PLOG_ERROR << "Error on create gadget";
    plog_usbg_err(usbg_ret);
    return false;
  }

  usbg_ret = usbg_create_function(_impl->g, USBG_F_ACM, "usb0", NULL, &_impl->f_acm0);
  if (usbg_ret != USBG_SUCCESS) {
    PLOG_ERROR << "Error creating function acm";
    plog_usbg_err(usbg_ret);
    return false;
  }

  usbg_ret = usbg_create_function(_impl->g, USBG_F_HID, "usb0", &f_attrs, &_impl->f_hid);
  if (usbg_ret != USBG_SUCCESS) {
    PLOG_ERROR << "Error creating function hid";
    plog_usbg_err(usbg_ret);
    return false;
  }

  usbg_ret = usbg_create_function(_impl->g, USBG_F_MIDI, "usb0", &midi_attrs, &_impl->f_midi);
  if (usbg_ret != USBG_SUCCESS) {
    PLOG_ERROR << "Error creating function midi";
    plog_usbg_err(usbg_ret);
    return false;
  }

  usbg_ret = usbg_create_config(_impl->g, 1, "config", NULL, &c_strs, &_impl->c);
  if (usbg_ret != USBG_SUCCESS) {
    PLOG_ERROR << "Error creating config";
    plog_usbg_err(usbg_ret);
    return false;
  }

  usbg_ret = usbg_add_config_function(_impl->c, "acm.GS0", _impl->f_acm0);
  if (usbg_ret != USBG_SUCCESS) {
    PLOG_ERROR << "Error adding function ecm.GS0";
    plog_usbg_err(usbg_ret);
    return false;
  }

  usbg_ret = usbg_add_config_function(_impl->c, "keyboard", _impl->f_hid);
  if (usbg_ret != USBG_SUCCESS) {
    PLOG_ERROR << "Error adding function: keyboard";
    plog_usbg_err(usbg_ret);
    return false;
  }

  usbg_ret = usbg_add_config_function(_impl->c, "midi", _impl->f_midi);
  if (usbg_ret != USBG_SUCCESS) {
    PLOG_ERROR << "Error adding function: midi";
    plog_usbg_err(usbg_ret);
    return false;
  }

  usbg_ret = usbg_enable_gadget(_impl->g, DEFAULT_UDC);
  if (usbg_ret != USBG_SUCCESS) {
    PLOG_ERROR << "Error enabling gadget";
    plog_usbg_err(usbg_ret);
    return false;
  }

  _impl->fd_hid = open("/dev/hidg0", O_WRONLY | O_NDELAY);
  if (_impl->fd_hid < 0) {
    PLOG_ERROR << "Error opening /dev/hidg0";
    return false;
  }
  _impl->fd_midi = open("/dev/snd/midiC1D0", O_WRONLY | O_NDELAY);
  if (_impl->fd_midi < 0) {
    PLOG_ERROR << "Error opening /dev/snd/midiC1D0";
    return false;
  }

  return true;
}

bool USB::initNull() {
  _impl->fd_hid = open("/dev/null", O_WRONLY | O_NDELAY);
  if (_impl->fd_hid < 0) {
    PLOG_ERROR << "Error opening hid /dev/null";
    return false;
  }
  _impl->fd_midi = open("/dev/null", O_WRONLY | O_NDELAY);
  if (_impl->fd_midi < 0) {
    PLOG_ERROR << "Error opening midi /dev/null";
    return false;
  }

  return true;
}

void USB::deinit() {
  if (_impl->g) {
    usbg_disable_gadget(_impl->g);
    usbg_rm_gadget(_impl->g, USBG_RM_RECURSE);
    _impl->g = nullptr;
  }
  if (_impl->s) {
    usbg_cleanup(_impl->s);
    _impl->s = nullptr;
  }
}

bool USB::sendHIDReport(string report) {
  int ret = write(_impl->fd_hid, report.c_str(), report.length());
  if (ret != report.length()) {
    PLOG_ERROR << "Error writing HID report";
    return false;
  }
  return true;
}

bool USB::sendMIDIReport(string report) {
  int ret = write(_impl->fd_midi, report.c_str(), report.length());
  if (ret != report.length()) {
    PLOG_ERROR << "Error writing MIDI data";
    return false;
  }
  return true;
}


