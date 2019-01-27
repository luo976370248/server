#ifndef __SCHEDULER_EXPORT_TO_LUA_H__
#define __SCHEDULER_EXPORT_TO_LUA_H__

#include "lua.hpp"

namespace bb{
	int register_scheduler_export(lua_State* L);
}

#endif

