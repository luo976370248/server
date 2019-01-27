#ifndef __SESSION_EXPORT_TO_LUA_H__
#define __SESSION_EXPORT_TO_LUA_H__

#include "lua.hpp"

namespace bb {
	int register_session_export(lua_State* tolua_S);
}

#endif