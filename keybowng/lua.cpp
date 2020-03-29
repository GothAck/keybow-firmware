#include <unordered_map>

#include <plog/Log.h>


#include "lua.hpp"
#include "lua_impl.hpp"
#include "luakeybow.hpp"

using namespace std;

LuaImpl::LuaImpl(Lua *p): lua(std::make_shared<sol::state>()), p(p) {}

bool LuaImpl::sendHIDReport(std::string report) {
  if (plugins.find("hid") != plugins.end()) {
    for (auto &p : plugins["hid"]) {
      sol::safe_function func = p.second["sendHIDReport"];
      auto res = func(report);
      if (!res.valid()) {
        sol::error err = res;
        PLOG_ERROR << "HID Plugin " << p.first << " failed call to sendHIDReport " << err.what();
      }
    }
  }
  return p->_keys->sendHIDReport(report);
}

bool LuaImpl::sendMIDIReport(std::string report) {
  if (plugins.find("midi") != plugins.end()) {
    for (auto &p : plugins["midi"]) {
      sol::safe_function func = p.second["sendMIDIReport"];
      auto res = func(report);
      if (!res.valid()) {
        sol::error err = res;
        PLOG_ERROR << "MIDI Plugin " << p.first << " failed call to sendMIDIReport " << err.what();
      }
    }
  }
  return p->_keys->sendMIDIReport(report);
}

sol::table LuaImpl::load_plugin(std::string plugin) {
    PLOG_INFO << "Loading plugin " << plugin;
  sol::safe_function require = (*lua)["require"];

  auto res = require(plugin);

  bool success = false;
  if (res.valid()) {
    sol::table table = res;
    string plugin_type = table["type"];
    PLOG_WARNING << plugin_type;

    if (plugins.find(plugin_type) == plugins.end()) {
      plugins[plugin_type] = {{plugin, table}};
      success = true;
    } else {
      auto &plugins_type = plugins[plugin_type];
      if (plugins_type.find(plugin) == plugins_type.end()) {
        plugins_type[plugin] = table;
        success = true;
      }
    }

    if (success) {
      sol::safe_function init = table["init"];
      PLOG_DEBUG << "Plugin has init function? " << init.valid();
      if (init.valid()) {
        auto ret = init();
        if (!ret.valid()) {
          sol::error err = ret;
          PLOG_ERROR << "Error initializing plugin " << err.what();
        }
      }
      return table;
    }
  }

  PLOG_ERROR << "Failed to load plugin " << plugin;

  return lua->create_table();
}

Lua::Lua(std::shared_ptr<Keys> keys, std::shared_ptr<Lights> lights):
  _keys(keys),
  _lights(lights),
  _impl(new LuaImpl(this))
  {}

Lua::~Lua() {
  delete _impl;
}

bool Lua::init() {
  unique_lock<recursive_mutex> lock(_mutex);
  PLOG_DEBUG << "open libraries";
  _impl->lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::package);
  PLOG_DEBUG << "init keybow table";
  LuaKeybow::initTable(_impl->lua, shared_from_this());

  return true;
}

void Lua::deinit() {
  unique_lock<recursive_mutex> lock(_mutex);

  for (auto &pm : _impl->plugins) {
    for (auto &p : pm.second) {
      sol::safe_function deinit = p.second["deinit"];
      if (deinit.valid()) {
        PLOG_DEBUG << "Calling plugin " << p.first << " deinit";
        auto ret = deinit();
        if (!ret.valid()) {
          sol::error err = ret;
          PLOG_ERROR << "Error calling deinit " << err.what();
        }
      }
    }
  }
}

bool Lua::load(std::string file) {
  unique_lock<recursive_mutex> lock(_mutex);
  sol::protected_function_result res = _impl->lua->safe_script_file(file, &sol::script_pass_on_error);
  if (!res.valid()) {
    sol::error err = res;
    PLOG_ERROR << "Error running " << file;
    PLOG_ERROR << err.what();
    return false;
  }
  return true;
}

bool Lua::interpret(std::string code, bool print_exception) {
  unique_lock<recursive_mutex> lock(_mutex);
  auto res = _impl->lua->safe_script(code);
  if (!res.valid()) {
    if (print_exception) {
      sol::error err = res;
      PLOG_ERROR << err.what();
    }
    return false;
  }
  return true;
}


bool Lua::load_plugin(std::string plugin) {
  return !_impl->load_plugin(plugin).empty();
}

void Lua::setup() {
  unique_lock<recursive_mutex> lock(_mutex);
  sol::protected_function setup = (*_impl->lua)["setup"];
  if (setup.valid()) {
    auto ret = setup();

    if (!ret.valid()) {
      sol::error err = ret;
      PLOG_WARNING << "Failed to call setup function";
      PLOG_ERROR << err.what();
    }
  }
}

void Lua::tick() {
  unique_lock<recursive_mutex> lock(_mutex);
  sol::protected_function tick = (*_impl->lua)["tick"];
  if (tick.valid()) {
    tick();
  }
}

void Lua::handleKey(int key, bool state) {
  unique_lock<recursive_mutex> lock(_mutex);
  PLOG_DEBUG << "handleKey " << key << " " << state;
  char fn[14];
  snprintf(fn, 14, "handle_key_%02d", key);
  const char *fnc = (char *)&fn;
  sol::protected_function handle_key = (*_impl->lua)[fnc];

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
