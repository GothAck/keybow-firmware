#include <poll.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <string>
#include <iostream>
#include <thread>
#include <sstream>

#include <sol/sol.hpp>

#include "main.hpp"

using namespace std;

namespace repl {

  sol::table module;
  std::unique_ptr<sol::state_view> lua;
  thread repl_thread;
  bool running = false;



  void eval(string code) {
    sol::safe_function eval = (*lua)["eval"];
    if (eval.valid()) {
      auto ret = eval(code);
      if (!ret.valid()) {
        sol::error err = ret;
        cerr << err.what() << endl;
      }
    }
  }

  void lhandler(char *line) {
    if (line) {
      add_history(line);
      eval(line);
      free(line);
    }
  }

  void repl() {
    rl_callback_handler_install("lua> ", lhandler);
    while (running) {
      struct pollfd fds = {
        .fd = 0,
        .events = POLLIN,
      };
      if (poll(&fds, 1, 500) == POLLIN) {
        rl_callback_read_char();
      }
    }
    rl_callback_handler_remove();
  }

  void init() {
    running = true;
    repl_thread = thread(repl);
  }

  void deinit() {
    running = false;
    if (repl_thread.joinable()) {
      repl_thread.join();
    }
  }

	sol::table open_repl(sol::this_state L) {
		lua = make_unique<sol::state_view>(L);
		module = lua->create_table();

		module["type"] = "none";
		module["init"] = init;
		module["deinit"] = deinit;

		return module;
	}

} // namespace repl

extern "C" int luaopen_repl(lua_State* L) {
	return sol::stack::call_lua(L, 1, repl::open_repl );
}