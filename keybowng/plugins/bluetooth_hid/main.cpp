#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#include <string>
#include <iostream>
#include <thread>

#include <sol/sol.hpp>
#include <sdbus-c++/sdbus-c++.h>

#include "sdp_record.xml.h"

#include "main.hpp"

using namespace std;

const string sdp_record((char *)&sdp_record_xml, sdp_record_xml_len);

namespace bthid {

  unique_ptr<sdbus::IConnection> connection;
  unique_ptr<sdbus::IProxy> manager;
  unique_ptr<sdbus::IObject> profile;
  int socket_control = -1;
  int socket_interrupt = -1;
  int socket_control_client = -1;
  int socket_interrupt_client = -1;
  sdbus::ObjectPath hci_adapter;
  thread loop;
  thread bt_thread;

  map<sdbus::ObjectPath, map<string, map<string, sdbus::Variant>>> GetManagedObjects() {
    return {};
  }

  void Release() {
    cerr << "Profile released" << endl;
  }

  void NewConnection(sdbus::ObjectPath device, int fd, map<string, sdbus::Variant> fd_props) {
    cerr << "New Connection" << endl;
  }

  void RequestDisconnection(sdbus::ObjectPath device) {
    cerr << "Request Disconnection" << endl;
  }

  bool findAdapter() {
    auto bluez = sdbus::createProxy("org.bluez", "/");
    std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> objectsInterfacesAndProperties;
    bluez->callMethod("GetManagedObjects").onInterface("org.freedesktop.DBus.ObjectManager").storeResultsTo(objectsInterfacesAndProperties);
    for (auto &p : objectsInterfacesAndProperties) {
      if (p.second.find("org.bluez.Adapter1") != p.second.end()) {
        hci_adapter = p.first;
        return true;
      }
    }
    return false;
  }

    void init() {
    cerr << "bthid init called" << endl;
    connection = sdbus::createSystemBusConnection("org.pimoroni.bthidgatt");
    cerr << "Have connection" << endl;

    if (!findAdapter()) {
      cerr << "Failed to find hci adapter";
      return;
    }

    manager = sdbus::createProxy(*connection, "org.bluez", "/org/bluez");

    sdbus::ObjectPath path = "/com/pimoroni/btkbd";
    profile = sdbus::createObject(*connection, path);
    profile->registerMethod("Release").onInterface("org.bluez.Profile1").implementedAs(Release);
    profile->registerMethod("NewConnection").onInterface("org.bluez.Profile1").implementedAs(NewConnection);
    profile->registerMethod("RequestDisconnection").onInterface("org.bluez.Profile1").implementedAs(RequestDisconnection);
    profile->finishRegistration();

    map<string, sdbus::Variant> options = {
      {"ServiceRecord", sdp_record},
      {"Role", "Server"},
      {"RequireAuthentication", true},
      {"RequireAuthorization", true},
    };

    manager->callMethod("RegisterProfile").onInterface("org.bluez.ProfileManager1").withArguments(path, "00001124-0000-1000-8000-00805f9b34fb", options).dontExpectReply();

    struct sockaddr_l2 addr;
    socket_control = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    socket_interrupt = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

    memset(&addr, 0, sizeof(addr));
    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = 17;
    bind(socket_control, (struct sockaddr *)&addr, sizeof(addr));
    listen(socket_control, 1);

    memset(&addr, 0, sizeof(addr));
    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = 19;
    bind(socket_interrupt, (struct sockaddr *)&addr, sizeof(addr));
    listen(socket_interrupt, 1);

    loop = thread([] {
      cerr << "thread start" << endl;
      connection->enterEventLoop();
      cerr << "thread end" << endl;
    });

    bt_thread = thread([] {
      cerr << "bluetooth thread" << endl;
      struct sockaddr_l2 addr;
      socklen_t len = sizeof(addr);
      socket_control_client = accept(socket_control, (struct sockaddr *)&addr, &len);
      cerr << "bluetooth accepted control client" << endl;
      len = sizeof(addr);
      socket_interrupt_client = accept(socket_interrupt, (struct sockaddr *)&addr, &len);
      cerr << "bluetooth accepted interrupt client" << endl;
    });
  }

  void deinit() {
    cerr << "bthid deinit called" << endl;
    if (connection)
      connection->leaveEventLoop();
    if (loop.joinable())
      loop.join();
    close(socket_interrupt_client);
    close(socket_control_client);
    close(socket_interrupt);
    close(socket_control);
    connection.reset();
  }

  void sendHIDReport(string hidReport) {
    cerr << "bthid sendHIDReport" << endl;

    if (socket_interrupt_client != -1) {
      string packet;
      packet += 0xa1;
      packet += hidReport;
      write(socket_interrupt_client, packet.data(), packet.length());
    }
  }

	sol::table open_bthid(sol::this_state L) {
		sol::state_view lua(L);
		sol::table module = lua.create_table();

		module["type"] = "hid";
		module["init"] = init;
		module["deinit"] = deinit;
		module["sendHIDReport"] = sendHIDReport;

		return module;
	}

} // namespace bthid

extern "C" int luaopen_bthid(lua_State* L) {
	// pass the lua_State,
	// the index to start grabbing arguments from,
	// and the function itself
	// optionally, you can pass extra arguments to the function if that's necessary,
	// but that's advanced usage and is generally reserved for internals only
	return sol::stack::call_lua(L, 1, bthid::open_bthid );
}