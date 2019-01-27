#ifndef _EXPORT_MYSQL_TO_LUA_H_
#define _EXPORT_MYSQL_TO_LUA_H_

#include "lua.hpp"

namespace bb {
	int register_mysql_export(lua_State* L);
}

#endif // !_EXPORT_MYSQL_TO_LUA_H_
