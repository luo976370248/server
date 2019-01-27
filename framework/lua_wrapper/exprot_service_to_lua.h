#ifndef __SERVICE_EXPORT_TO_LUA_H__
#define __SERVICE_EXPORT_TO_LUA_H__

#include "lua.hpp"
namespace bb {
	int register_service_export(lua_State* L);
}
#endif