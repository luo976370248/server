#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"
#include "lua_wrapper.h"
#include "../database/redis_wrapper.h"
#include "export_redis_to_lua.h"

namespace bb {

	static void on_open_cb(const char* err, void* context, void* udata) {
		if (err) {
			lua_pushstring(lua_wrapper::lua_state(), err);
			lua_pushnil(lua_wrapper::lua_state());
		}
		else {
			lua_pushnil(lua_wrapper::lua_state());
			tolua_pushuserdata(lua_wrapper::lua_state(), context);
		}

		lua_wrapper::execute_script_handler((int)udata, 2);
		lua_wrapper::remove_script_handler((int)udata);
	}

	static int lua_redis_connect(lua_State* tolua_S) {
		char* ip = (char*)tolua_tostring(tolua_S, 1, 0);
		int port = (int)tolua_tonumber(tolua_S, 2, 0);
		int handler = toluafix_ref_function(tolua_S, 3, 0);

		if (ip == NULL) {
			goto lua_failed;
		}
		redis_wrapper::connect(ip, port, on_open_cb, (void*)handler);

	lua_failed:
		return 0;
	}

	static int lua_redis_close(lua_State* tolua_S) {
		void* context = tolua_touserdata(tolua_S, 1, 0);
		if (context) {
			redis_wrapper::close_redis(context);
		}
		return 0;
	}

	static void push_result_to_lua(redisReply* result) {
		switch (result->type) {
		case REDIS_REPLY_STRING:
		case REDIS_REPLY_STATUS:
			lua_pushstring(lua_wrapper::lua_state(), result->str);
			break;
		case REDIS_REPLY_INTEGER:
			lua_pushinteger(lua_wrapper::lua_state(), result->integer);
			break;
		case REDIS_REPLY_NIL:
			lua_pushnil(lua_wrapper::lua_state());
			break;
		case REDIS_REPLY_ARRAY:
			lua_newtable(lua_wrapper::lua_state());
			int index = 1;
			for (int i = 0; i < result->elements; i++) {
				push_result_to_lua(result->element[i]);
				lua_rawseti(lua_wrapper::lua_state(), -2, index); 
				++index;
			}
			break;
		}
	}

	static void on_lua_query_cb(const char* err, redisReply* result, void* udata) {
		if (err) {
			lua_pushstring(lua_wrapper::lua_state(), err);
			lua_pushnil(lua_wrapper::lua_state());
		}
		else {
			lua_pushnil(lua_wrapper::lua_state());
			if (result) { 
				push_result_to_lua(result);
			}
			else {
				lua_pushnil(lua_wrapper::lua_state());
			}
		}

		lua_wrapper::execute_script_handler((int)udata, 2);
		lua_wrapper::remove_script_handler((int)udata);
	}

	static int lua_redis_query(lua_State* tolua_S) {
		void* context = tolua_touserdata(tolua_S, 1, 0);
		char* cmd = (char*)tolua_tostring(tolua_S, 2, 0);
		int handler = toluafix_ref_function(tolua_S, 3, 0);
		if (!context || cmd == NULL || handler == 0) {
			goto lua_failed;
		}
		redis_wrapper::query(context, cmd, on_lua_query_cb, (void*)handler);
	lua_failed:
		return 0;
	}



	int register_redis_export(lua_State* L) {
		lua_getglobal(L, "_G");
		if (lua_istable(L, -1)) {
			tolua_open(L);
			tolua_module(L, "Redis", 0);
			tolua_beginmodule(L, "Redis");

			tolua_function(L, "connect", lua_redis_connect);
			tolua_function(L, "close", lua_redis_close);
			tolua_function(L, "query", lua_redis_query);
			tolua_endmodule(L);
		}
		lua_pop(L, 1);

		return 0;
	}


}