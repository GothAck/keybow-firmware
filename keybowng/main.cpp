#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <thread>
#include <functional>

#include <docopt/docopt.h>

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <csignal>

#include "lua.hpp"
#include "usb.hpp"
#include "lights.hpp"
#include "keys.hpp"

static const char USAGE[] =
R"(Keybow.

    Usage:
      keybow [options] [--plugin=PLUGIN]...

    Options:
      -h --help                     This help.
      -d DIR --dir DIR              Directory containing scripts, config, and plugins. [default: /boot]
      -n --no-output                Redirect all HID and MIDI output to /dev/null.
      -l LEVEL --loglevel LEVEL     Set log level (verbose, debug, warning, error, none). [default: info]
      -p PLUGIN --plugin=PLUGIN     Load a plugin
)";

using namespace std;

bool running = true;

shared_ptr<USB> usb = make_shared<USB>();
shared_ptr<Keys> keys = make_shared<Keys>(usb);
shared_ptr<Lights> lights = make_shared<Lights>(keys);
shared_ptr<Lua> lua = make_shared<Lua>(keys, lights);

void signal_handler(int signal) {
  running = false;
}

int main(int argc, char *argv[]) {
  int ret = 0;
  auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, true);

  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::severityFromString(args["--loglevel"].asString().c_str()), &consoleAppender);

  const auto scriptDir = args["--dir"].asString();
  filesystem::current_path(scriptDir);
  PLOG_DEBUG << "Current path: " << filesystem::current_path().string();
  const auto noOutput = args["--no-output"].asBool();

  PLOG_INFO << "Initializing";

  PLOG_DEBUG << "keys init";
  if (!keys->init(bind(mem_fn(&Lua::handleKey), lua, placeholders::_1, placeholders::_2))) {
    PLOG_ERROR << "Failed to init keys";
    ret = 1;
    goto keys_deinit;
  }

  PLOG_DEBUG << "lights init";
  if (!lights->init()) {
    PLOG_ERROR << "Failed to init lights";
    ret = 1;
    goto lights_deinit;
  }

  if (!noOutput) {
    PLOG_DEBUG << "usb init";
    if (!usb->init()) {
      PLOG_ERROR << "Failed to init usb";
      ret = 1;
      goto usb_deinit;
    }
  } else {
    PLOG_DEBUG << "null usb init";
    if (!usb->initNull()) {
      PLOG_ERROR << "Failed to init null usb";
      ret = 1;
      goto usb_deinit;
    }
  }

  PLOG_DEBUG << "lua init";
  if (!lua->init()) {
    PLOG_ERROR << "Failed to init lua";
    ret = 1;
    goto lua_deinit;
  }

  PLOG_DEBUG << "read default.png";
  lights->read("default.png");

  signal(SIGINT, signal_handler);

  PLOG_DEBUG << "load keys.lua";
  if (!lua->load("keys.lua")) {
    PLOG_ERROR << "Failed to load keys.lua";
  }

  lua->setup();

  if (args.find("--plugin") != args.end()) {
    for (auto &plugin : args["--plugin"].asStringList()) {
      lua->load_plugin(plugin);
    }
  }

  PLOG_INFO << "Running main loop";
  while (running) {
    lua->tick();
    keys->update();
    this_thread::sleep_for(chrono::milliseconds(1));
  }

  PLOG_INFO << "Stopping";

lua_deinit:
  PLOG_DEBUG << "lua deinit";
  lua->deinit();
usb_deinit:
  PLOG_DEBUG << "usb deinit";
  usb->deinit();
lights_deinit:
  PLOG_DEBUG << "lights deinit";
  lights->deinit();
keys_deinit:
  PLOG_DEBUG << "keys deinit";
  keys->deinit();
  return ret;
}