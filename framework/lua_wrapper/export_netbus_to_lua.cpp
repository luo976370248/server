#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../../framework/netbus/netbus.h"
#include "lua_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"
#include "export_netbus_to_lua.h" 


namespace bb{
	static int lua_udp_listen(lua_State* tolua_S) {
		int argc = lua_gettop(tolua_S);
		if (argc != 1) {
			goto lua_failed;
		}
		// 在c++ Primer中倒是提到，goto 不能向前跳过变量定义语句，给出的理由是可能引起未定义的变量使用。当确实要这样使用时，需要把定义语句使用{}括起来
		{
			int port = (int)lua_tointeger(tolua_S, 1);
			netbus::instance()->start_udp_server(port);
		}
	lua_failed:
		return 0;
	}

	static int lua_tcp_listen(lua_State* tolua_S) {
		int argc = lua_gettop(tolua_S);
		if (argc != 1) {
			goto lua_failed;
		}
		{
			int port = (int)lua_tointeger(tolua_S, 1);
			netbus::instance()->start_tcp_server(port);
		}
	lua_failed:
		return 0;
	}

	static int lua_ws_listen(lua_State* tolua_S) {
		int argc = lua_gettop(tolua_S);
		if (argc != 1) {
			goto lua_failed;
		}
		{
			int port = (int)lua_tointeger(tolua_S, 1);
			netbus::instance()->start_ws_server(port);
		}
	lua_failed:
		return 0;
	}
	
	static void on_tcp_connected_cb(int err, session* s, void* udata) {
		if (err) {
			lua_pushinteger(lua_wrapper::lua_state(), err);
			lua_pushnil(lua_wrapper::lua_state());
		}
		else {
			lua_pushinteger(lua_wrapper::lua_state(), err);
			tolua_pushuserdata(lua_wrapper::lua_state(), s);
		}

		lua_wrapper::execute_script_handler((int)udata, 2);
		lua_wrapper::remove_script_handler((int)udata);
	}

	static int lua_tcp_connect(lua_State* tolua_S) {
		const char* ip = luaL_checkstring(tolua_S, 1);
		if (ip == NULL) {
			return 0;
		}

		int port = luaL_checkinteger(tolua_S, 2);
		int handler = toluafix_ref_function(tolua_S, 3, 0);
		if (handler == 0) {
			return 0;
		}
		//netbus::instance()->tcp_connect(ip, port, on_tcp_connected_cb, (void*)handler);
	}

	int register_netbus_export(lua_State* tolua_S) {
		lua_getglobal(tolua_S, "_G");
		if (lua_istable(tolua_S, -1)) {
			tolua_open(tolua_S);
			tolua_module(tolua_S, "netbus", 0);
			tolua_beginmodule(tolua_S, "netbus");

			tolua_function(tolua_S, "udp_listen",lua_udp_listen);
			tolua_function(tolua_S, "tcp_listen", lua_tcp_listen);
			tolua_function(tolua_S, "ws_listen", lua_ws_listen);
			tolua_function(tolua_S, "tcp_connect", lua_tcp_connect);
			tolua_endmodule(tolua_S);
		}

		lua_pop(tolua_S, 1);
		return 0;
	}
}