#ifndef _EXPORT_NETBUS_TO_LUA_
#define _EXPORT_NETBUS_TO_LUA_

#include "lua.hpp"

namespace bb {
	int register_netbus_export(lua_State* tolua_S);
}

#endif
