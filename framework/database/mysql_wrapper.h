#ifndef _MYSQL_WRAPPER_H_
#define _MYSQL_WRAPPER_H_

#include "mysql.h"

namespace bb {
	typedef void(*mysql_open_cb)(const char* err, void* context, void* udata);
	typedef void(*mysql_query_cb)(const char* err, MYSQL_RES* result, void* udata);

	class mysql_wrapper {
	public:
		static void connect(char* ip, int port, char* db_name, char* uname, char* pwd, mysql_open_cb open_cb, void* udata = NULL);
		static void close(void* context);
		static void query(void* context, char* sql, mysql_query_cb query_cb, void* udata = NULL);
	};
}


#endif // !_MYSQL_WRAPPER_H_
