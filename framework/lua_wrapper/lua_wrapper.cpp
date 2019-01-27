#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <string>

#include "tolua_fix.h"

#include "../utils/logger.h"


#include "lua_wrapper.h"
#include "lua_wrapper.h"
#include "./export_mysql_to_lua.h"
#include "./export_redis_to_lua.h"
#include "./exprot_service_to_lua.h"
#include "./exprot_session_to_lua.h"
#include "./exprot_scheduler_to_lua.h"
#include "./export_netbus_to_lua.h"
#include "./export_proto_man_to_lua.h"

namespace bb {
	lua_State* g_lua_State = NULL;

	static void do_log_message(void(*log)(const char* file_name, int line_num, const char* msg), const char* msg) {
		lua_Debug info;
		int depth = 0;
		while (lua_getstack(g_lua_State, depth, &info)) {

			lua_getinfo(g_lua_State, "S", &info);
			lua_getinfo(g_lua_State, "n", &info);
			lua_getinfo(g_lua_State, "l", &info);

			if (info.source[0] == '@') {
				log(&info.source[1], info.currentline, msg);
				return;
			}

			++depth;
		}
		if (depth == 0) {
			log("trunk", 0, msg);
		}
	}

	static void print_error(const char* file_name, int line_num, const char* msg) {
		logger::log(file_name, line_num, ERROR, msg);
	}

	static void print_warning(const char* file_name, int line_num, const char* msg) {
		logger::log(file_name, line_num, WARNING, msg);
	}

	static void print_debug(const char* file_name, int line_num, const char* msg) {
		logger::log(file_name, line_num, DEBUG, msg);
	}

	int lua_log_debug(lua_State* L) {
		const char* msg = luaL_checkstring(L, -1);
		if (msg) {
			do_log_message(print_debug, msg);
		}
		return 0;
	}

	int lua_log_warning(lua_State* L) {
		const char* msg = luaL_checkstring(L, -1);
		if (msg) {
			do_log_message(print_warning, msg);
		}
		return 0;
	}

	int lua_log_error(lua_State* L) {
		const char* msg = luaL_checkstring(L, -1);
		if (msg) {
			do_log_message(print_error, msg);
		}
		return 0;
	}

	static int lua_panic(lua_State *L) {
		const char* msg = luaL_checkstring(L, -1);
		if (msg) { // file_name, line_num
			do_log_message(print_error, msg);
		}
		return 0;
	}

	static int lua_add_search_path(lua_State* L) {
		const char* path = luaL_checkstring(L, 1);
		if (path) {
			std::string str_path = path;
			lua_wrapper::add_search_path(str_path);
		}
		return 0;
	}



	void lua_wrapper::init() {
		g_lua_State = luaL_newstate();
		lua_atpanic(g_lua_State, lua_panic);
		luaL_openlibs(g_lua_State);
		toluafix_open(g_lua_State);

		lua_wrapper::reg_func2lua("log_error", lua_log_error);
		lua_wrapper::reg_func2lua("log_debug", lua_log_debug);
		lua_wrapper::reg_func2lua("log_warning", lua_log_warning);

		lua_wrapper::reg_func2lua("add_search_path", lua_add_search_path);

		//register_logger_export(g_lua_State);

		register_mysql_export(g_lua_State);
		register_redis_export(g_lua_State);
		register_service_export(g_lua_State);
		register_session_export(g_lua_State);
		register_scheduler_export(g_lua_State);
		register_netbus_export(g_lua_State);
		register_proto_man_export(g_lua_State);
		register_raw_cmd_export(g_lua_State);
	}


	void lua_wrapper::exit() {
		if (g_lua_State != NULL) {
			lua_close(g_lua_State);
			g_lua_State = NULL;
		}
	}

	bool lua_wrapper::do_file(std::string& lua_file) {
		if (luaL_dofile(g_lua_State, lua_file.c_str())) {
			lua_log_error(g_lua_State);
			return false;
		}
		return true;
	}

	void lua_wrapper::reg_func2lua(const char* name, int(*c_func)(lua_State* L)) {
		lua_pushcfunction(g_lua_State, c_func);
		lua_setglobal(g_lua_State, name);
	}

	void lua_wrapper::lua_wrapper::add_search_path(std::string& path) {
		char strPath[1024] = { 0 };
		sprintf(strPath, "local path = string.match([[%s]],[[(.*)/[^/]*$]])\n package.path = package.path .. [[;]] .. path .. [[/?.lua;]] .. path .. [[/?/init.lua]]\n", path.c_str());
		luaL_dostring(g_lua_State, strPath);
	}

	lua_State* lua_wrapper::lua_state() {
		return g_lua_State;
	}


	static bool pushFunctionByHandler(int nHandler)
	{
		toluafix_get_function_by_refid(g_lua_State, nHandler);                  /* L: ... func */
		if (!lua_isfunction(g_lua_State, -1))
		{
			log_error("[LUA ERROR] function refid '%d' does not reference a Lua function", nHandler);
			lua_pop(g_lua_State, 1);
			return false;
		}
		return true;
	}

	static int executeFunction(int numArgs)
	{
		int functionIndex = -(numArgs + 1);
		if (!lua_isfunction(g_lua_State, functionIndex))
		{
			log_error("value at stack [%d] is not function", functionIndex);
			lua_pop(g_lua_State, numArgs + 1); // remove function and arguments
			return 0;
		}

		int traceback = 0;
		lua_getglobal(g_lua_State, "__G__TRACKBACK__");                         /* L: ... func arg1 arg2 ... G */
		if (!lua_isfunction(g_lua_State, -1))
		{
			lua_pop(g_lua_State, 1);                                            /* L: ... func arg1 arg2 ... */
		}
		else
		{
			lua_insert(g_lua_State, functionIndex - 1);                         /* L: ... G func arg1 arg2 ... */
			traceback = functionIndex - 1;
		}

		int error = 0;
		error = lua_pcall(g_lua_State, numArgs, 1, traceback);                  /* L: ... [G] ret */
		if (error)
		{
			if (traceback == 0)
			{
				log_error("[LUA ERROR] %s", lua_tostring(g_lua_State, -1));        /* L: ... error */
				lua_pop(g_lua_State, 1); // remove error message from stack
			}
			else                                                            /* L: ... G error */
			{
				lua_pop(g_lua_State, 2); // remove __G__TRACKBACK__ and error message from stack
			}
			return 0;
		}

		// get return value
		int ret = 0;
		if (lua_isnumber(g_lua_State, -1))
		{
			ret = (int)lua_tointeger(g_lua_State, -1);
		}
		else if (lua_isboolean(g_lua_State, -1))
		{
			ret = (int)lua_toboolean(g_lua_State, -1);
		}
		// remove return value from stack
		lua_pop(g_lua_State, 1);                                                /* L: ... [G] */

		if (traceback)
		{
			lua_pop(g_lua_State, 1); // remove __G__TRACKBACK__ from stack      /* L: ... */
		}

		return ret;
	}


	int lua_wrapper::execute_script_handler(int nHandler, int numArgs) {
		int ret = 0;
		if (pushFunctionByHandler(nHandler))                                /* L: ... arg1 arg2 ... func */
		{
			if (numArgs > 0)
			{
				lua_insert(g_lua_State, -(numArgs + 1));                        /* L: ... func arg1 arg2 ... */
			}
			ret = executeFunction(numArgs);
		}
		lua_settop(g_lua_State, 0);
		return ret;
	}


	void lua_wrapper::remove_script_handler(int nHandler) {
		toluafix_remove_function_by_refid(g_lua_State, nHandler);
	}
}