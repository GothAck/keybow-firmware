#include <plog/Log.h>

#include <sol/sol.hpp>

#include "lua.hpp"
#include "luakeybow.hpp"

using namespace std;

struct LuaImpl {
  LuaImpl(Lua *p): p(p) {}

  sol::state lua;

  Lua *p;
};

Lua::Lua(std::shared_ptr<Keys> keys, std::shared_ptr<Lights> lights):
  _keys(keys),
  _lights(lights),
  _impl(new LuaImpl(this))
  {}

Lua::~Lua() {
  delete _impl;
}

bool Lua::init() {
  PLOG_DEBUG << "open libraries";
  _impl->lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::package);
  PLOG_DEBUG << "init keybow table";
  LuaKeybow::initTable(_impl->lua, shared_from_this());

  return true;
}

void Lua::deinit() {
}

bool Lua::load(std::string file) {
  return _impl->lua.script_file(file).valid();
}

void Lua::setup() {
  sol::protected_function setup = _impl->lua["setup"];
  auto ret = setup();

  if (!ret.valid()) {
    sol::error err = ret;
    PLOG_WARNING << "Failed to call setup function";
    PLOG_ERROR << err.what();
  }
}

void Lua::tick() {
  sol::protected_function tick = _impl->lua["tick"];
  if (tick.valid()) {
    tick();
  }
}

void Lua::handleKey(int key, bool state) {
  PLOG_DEBUG << "handleKey " << key << " " << state;
  char fn[14];
  snprintf(fn, 14, "handle_key_%02d", key);
  const char *fnc = (char *)&fn;
  sol::protected_function handle_key = _impl->lua[fnc];

  if (!handle_key.valid()) {
    PLOG_ERROR << "Failed to find function " << fnc;
    return;
  }

  auto ret = handle_key((bool) state);

  if (!ret.valid()) {
    sol::error err = ret;
    PLOG_ERROR << "Failed to call lua function " << fnc;
    PLOG_ERROR << err.what();
  }
}
