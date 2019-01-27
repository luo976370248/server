#ifndef _lua_wrapper_h_
#define _lua_wrapper_h_

#include <iostream>
#include <lua.hpp>


namespace bb {
	class lua_wrapper {
	public:
		static void init();
		static void exit();

		static bool do_file(std::string& lua_file);

		static lua_State* lua_state();

		static void reg_func2lua(const char* name, int(*c_func)(lua_State* L));

		static void add_search_path(std::string& path);

		static int execute_script_handler(int handler, int numArgs);
		static void remove_script_handler(int handler);
	};
}

#endif // !_lua_wrapper_h_
