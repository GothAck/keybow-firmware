#include <string>
#include <memory>
#include <set>

#include <sol/sol.hpp>

#pragma once

class Lua;
class LuaKeybowImpl;

struct LuaKeybow {
  static void initTable(std::shared_ptr<sol::state> lua, std::shared_ptr<Lua> ref);
};

