#ifndef  _REDIS_WRAPPER_H_
#define _REDIS_WRAPPER_H_

#include <hiredis.h>


namespace bb {
	typedef void(*redis_open_cb)(const char* err, void* context, void* udata);
	typedef void(*redis_query_cb)(const char* err, redisReply* result, void* udata);

	class redis_wrapper {
	public:
		static void connect(char* ip, int port,redis_open_cb open_cb, void* udata = NULL);
		
		static void close_redis(void* context);
		
		static void query(void* context, char* cmd, redis_query_cb query_cb, void* udata = NULL);
	};
}

#endif // ! _REDIS_WRAPPER_H_
