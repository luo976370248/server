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
#include "../database/mysql_wrapper.h"
#include "export_mysql_to_lua.h"
#include "mysql.h"

namespace bb {
	static void on_open_mysql_cb(const char* err, void* context, void* udata) {
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

	static int lua_mysql_connect(lua_State* L) {
		char* ip = (char*)tolua_tostring(L, 1, 0);
		int port = (int)tolua_tonumber(L, 2, 0);
		char* db_name = (char*)tolua_tostring(L, 3, 0);
		char* uname = (char*)tolua_tostring(L, 4, 0);
		char* upwd = (char*)tolua_tostring(L, 5, 0);
		int handler = toluafix_ref_function(L, 6, 0);
		if (ip == NULL || db_name == NULL || uname == NULL || upwd == NULL) {
			goto lua_failed;
		}

		mysql_wrapper::connect(ip, port, db_name, uname, upwd, on_open_mysql_cb, (void*)handler);

	lua_failed:
		return 0;
	}


	static int lua_mysql_close(lua_State* tolua_S) {
		void* context = tolua_touserdata(tolua_S, 1, 0);
		if (context) {
			mysql_wrapper::close(context);
		}
		return 0;
	}

	static void push_mysql_row(MYSQL_ROW row, int num) {
		lua_newtable(lua_wrapper::lua_state());
		int index = 1;
		for (int i = 0; i < num; i++) {
			if (row[i] == NULL) {
				lua_pushnil(lua_wrapper::lua_state());
			}
			else {
				lua_pushstring(lua_wrapper::lua_state(), row[i]);
			}

			lua_rawseti(lua_wrapper::lua_state(), -2, index);
			++index;
		}
	}

	static void on_lua_mysql_query_cb(const char* err, MYSQL_RES* result, void* udata) {
		if (err) {
			lua_pushstring(lua_wrapper::lua_state(), err);
			lua_pushnil(lua_wrapper::lua_state());
		}
		else {
			lua_pushnil(lua_wrapper::lua_state());
			if (result) {
				lua_newtable(lua_wrapper::lua_state());
				int index = 1;
				int num = mysql_num_fields(result);
				MYSQL_ROW row;
				while (row = mysql_fetch_row(result)) {
					push_mysql_row(row, num);
					lua_rawseti(lua_wrapper::lua_state(), -2, index);
					++index;
				}
			}
			else {
				lua_pushnil(lua_wrapper::lua_state());
			}
		}

		lua_wrapper::execute_script_handler((int)udata, 2);
		lua_wrapper::remove_script_handler((int)udata);
	}

	static int lua_mysql_query(lua_State* tolua_S) {
		void* context = tolua_touserdata(tolua_S, 1, 0);
		char* sql = (char*)tolua_tostring(tolua_S, 2, 0);
		int handler = toluafix_ref_function(tolua_S, 3, 0);
		if (!context || sql == NULL || handler == 0) {
			goto lua_failed;
		}
		mysql_wrapper::query(context, sql, on_lua_mysql_query_cb, (void*)handler);
	lua_failed:
		return 0;
	}

	int register_mysql_export(lua_State* L) {
		lua_getglobal(L, "_G");
		if (lua_istable(L, -1)) {
			tolua_open(L);
			tolua_module(L, "mysql_wrapper", 0);
			tolua_beginmodule(L, "mysql_wrapper");

			tolua_function(L, "connect", lua_mysql_connect);
			tolua_function(L, "close", lua_mysql_close);
			tolua_function(L, "query", lua_mysql_query);
		}
		lua_pop(L, 1);
		return 0;
	}
}
