#include <unordered_map>
#include <vector>
#include <string>
#include <sol/sol.hpp>

#pragma once

struct LuaImpl {
  LuaImpl(Lua *p);

  bool sendHIDReport(std::string report);
  bool sendMIDIReport(std::string report);

  std::shared_ptr<sol::state> lua;
  
  Lua *p;
};
