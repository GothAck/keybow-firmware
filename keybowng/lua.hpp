#include <memory>
#include <string>
#include <unordered_set>
#include "keys.hpp"
#include "lights.hpp"

#pragma once

class LuaKeybowImpl;
class LuaImpl;

class Lua : public std::enable_shared_from_this<Lua> {
public:
  Lua(std::shared_ptr<Keys> keys, std::shared_ptr<Lights> lights);
  ~Lua();

  bool init();
  void deinit();

  bool load(std::string file);

  void handleKey(int key, bool state);

  void setup();
  void tick();
private:
  std::shared_ptr<Keys> _keys;
  std::shared_ptr<Lights> _lights;
  LuaImpl *_impl;
  friend class LuaKeybowImpl;
};

