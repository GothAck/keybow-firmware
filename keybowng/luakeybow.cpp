#include <thread>
#include <chrono>

#include <plog/Log.h>

#include "luakeybow.hpp"

#include "lua.hpp"

using namespace std;

struct LuaKeybowImpl {
  static constexpr int LEFT_SHIFT = 1;

  static bool set_modifier(int key, int state);
  static bool set_media_key(int key, int state);

  static uint64_t millis();
  static void sleep(int time);
  static void usleep(int time);

  static void send_text(std::string text);

  static bool set_pixel(int x, int r, int g, int b);
  static bool auto_lights(bool state);
  static bool clear_lights();
  static bool load_pattern(std::string file);

  static int ascii_to_shift(int key); //
  static int ascii_to_hid(int key); //

  static bool set_key(int key, bool pressed);
  static bool set_key_str(std::string key, int pressed);

  static bool send_midi_note(int channel, int note, int velocity, int speed);

  static shared_ptr<Lua> lua;
  static unsigned short _modifiers;
  static unsigned short _media_keys;
  static std::set<unsigned short> _keys;

  static void sendHIDReport();
};

void LuaKeybow::initTable(sol::state &lua, std::shared_ptr<Lua> ref) {
  PLOG_DEBUG << "init table";
  LuaKeybowImpl::lua = ref;
  lua["keybow_set_modifier"] = &LuaKeybowImpl::set_modifier;
  lua["keybow_set_media_key"] = &LuaKeybowImpl::set_media_key;
  lua["millis"] = &LuaKeybowImpl::millis;
  lua["keybow_sleep"] = &LuaKeybowImpl::sleep;
  lua["keybow_usleep"] = &LuaKeybowImpl::usleep;
  lua["keybow_send_text"] = &LuaKeybowImpl::send_text;
  lua["keybow_set_pixel"] = &LuaKeybowImpl::set_pixel;
  lua["keybow_auto_lights"] = &LuaKeybowImpl::auto_lights;
  lua["keybow_clear_lights"] = &LuaKeybowImpl::clear_lights;
  lua["keybow_load_pattern"] = &LuaKeybowImpl::load_pattern;
  lua["keybow_set_key"] = &LuaKeybowImpl::set_key;
  lua["keybow_send_midi_note"] = &LuaKeybowImpl::send_midi_note;
}

bool LuaKeybowImpl::set_modifier(int key, int state) {
  unsigned short current = !!(_modifiers & (1 << key));

  if (state != current) {
    _modifiers &= ~(1 << key);
    _modifiers |= state << key;
  }
  sendHIDReport();
}

bool LuaKeybowImpl::set_media_key(int key, int state) {
  unsigned short current = !!(_media_keys & (1 << key));

  if (state != current) {
    _media_keys &= ~(1 << key);
    _media_keys |= state << key;
  }
  sendHIDReport();
}

uint64_t LuaKeybowImpl::millis() {
  auto now = chrono::steady_clock::now();
  return chrono::time_point_cast<chrono::milliseconds>(now).time_since_epoch().count();
}

void LuaKeybowImpl::sleep(int time) {
  std::this_thread::sleep_for(std::chrono::milliseconds(time));
}

void LuaKeybowImpl::usleep(int time) {
  std::this_thread::sleep_for(std::chrono::microseconds(time));
}

void LuaKeybowImpl::send_text(std::string text) {
  for (auto c : text) {
    bool shift = ascii_to_shift(c);
    int hid_code = ascii_to_hid(c);

    if (hid_code) {
      if (shift) set_modifier(LEFT_SHIFT, true);
      set_key(hid_code, true);
      set_key(hid_code, false);
      if (shift) set_modifier(LEFT_SHIFT, false);
    }
  }
}

bool LuaKeybowImpl::set_pixel(int x, int r, int g, int b) {
  try {
    lua->_lights->setPixel(x, r, g, b);
  } catch (exception &e) {
    PLOG_ERROR << "Failed to set pixel " << x << " " << e.what();
    return false;
  }
  return true;
}

bool LuaKeybowImpl::auto_lights(bool state) {
  lua->_lights->setAuto(state);
  return true;
}

bool LuaKeybowImpl::clear_lights() {
  lua->_lights->clear();
  return true;
}

bool LuaKeybowImpl::load_pattern(std::string file) {
  lua->_lights->read(file);
}

int LuaKeybowImpl::ascii_to_shift(int key) {
  if (key >= 65 && key <= 90) {
    return true;
  }
}

int LuaKeybowImpl::ascii_to_hid(int key) {
  if (key == 48) {
    return 39;
  } else
  if (key == 32) {
    return 44;
  } else
  if (key >= 49 && key <= 57) {
    return key - 19;
  } else
  if (key >= 65 && key <= 90) {
    return key - 61;
  } else
  if (key >= 97 && key <= 122) {
    return key - 93;
  }

  return 0;
}

bool LuaKeybowImpl::set_key(int key, bool pressed) {
  PLOG_DEBUG << "set_key " << key << " " << pressed;
  if (pressed) {
    _keys.insert(key);
  } else {
    _keys.erase(key);
  }

  sendHIDReport();
}

void LuaKeybowImpl::sendHIDReport() {
  unsigned char buf[HID_REPORT_SIZE];
  memset(&buf, 0, HID_REPORT_SIZE);

  buf[0] = 1; // Report ID
  buf[1] = _modifiers;
  buf[2] = 0; // Padding
  int i = 3;
  for (auto &key : _keys) {
    buf[i++] = key;
    if (i >= 16) break; // Break at max buffer size
  }

  lua->_keys->sendHIDReport((const unsigned char *)&buf, HID_REPORT_SIZE);

  // TODO: Add media keys
}

bool LuaKeybowImpl::send_midi_note(int channel, int note, int velocity, int speed) {
  unsigned char buf[3];

  if (speed == 1) {
    buf[0] = 0x90;
  } else {
    buf[0] = 0x80;
  }

  buf[0] |= channel & 0xf;
  buf[1] = note & 0x7f;
  buf[2] = velocity & 0x7f;

  lua->_keys->sendMIDIData((const unsigned char *)&buf, 3);
}

shared_ptr<Lua> LuaKeybowImpl::lua = nullptr;
unsigned short LuaKeybowImpl::_modifiers = 0;
unsigned short LuaKeybowImpl::_media_keys = 0;
std::set<unsigned short> LuaKeybowImpl::_keys = {};
