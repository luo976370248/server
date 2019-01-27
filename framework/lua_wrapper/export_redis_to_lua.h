#ifndef __REDIS_EXPORT_LUA_H__
#define __REDIS_EXPORT_LUA_H__

#include "lua.hpp"

namespace bb {
	int register_redis_export(lua_State* L);
}

#endif

