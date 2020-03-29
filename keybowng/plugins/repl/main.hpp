#include <string>

#pragma once

extern "C" {
	struct lua_State;
}

namespace repl {

} // namespace repl


extern "C" __attribute__ ((visibility ("default"))) int luaopen_repl(lua_State* L);