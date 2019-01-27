#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../framework/netbus/netbus.h"
#include "../../framework/netbus/proto_man.h"
#include "../../framework/lua_wrapper/lua_wrapper.h"

int main(int argc, char** argv) {

	bb::netbus::instance()->init();

	bb::lua_wrapper::init();

	if (argc != 3) {
		// ²âÊÔÊý¾Ý
		std::string search_path = "../../apps/lua/scripts/";
		bb::lua_wrapper::add_search_path(search_path);
		std::string lua_file = search_path + "auth_server/main.lua";
		bb::lua_wrapper::do_file(lua_file);
	}
	else {
		std::string search_path = argv[1];
		if (*(search_path.end() - 1) != '/') {
			search_path += '/';
		}

		bb::lua_wrapper::add_search_path(search_path);
		std::string lua_file = search_path + argv[2];
		bb::lua_wrapper::do_file(lua_file);
	}

	bb::netbus::instance()->run();
	bb::lua_wrapper::exit();
	system("pause");
	return 0;
}