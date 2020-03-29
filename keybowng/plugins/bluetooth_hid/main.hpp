#include <string>

#pragma once

// forward declare as a C struct
// so a pointer to lua_State can be part of a signature
extern "C" {
	struct lua_State;
}
// you can replace the above if you're fine with including
// <sol.hpp> earlier than absolutely necessary

namespace bthid {

} // namespace bthid

// this function needs to be exported from your
// dll. "extern 'C'" should do the trick, but
// we're including platform-specific stuff here to help
// see bthid_api.hpp for details
extern "C" __attribute__ ((visibility ("default"))) int luaopen_bthid(lua_State* L);