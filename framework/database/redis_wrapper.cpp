#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hiredis.h>
#ifdef  WIN32
#define NO_QFORKIMPL //这一行必须加才能正常使用
#include <Win32_Interop/win32fixes.h>
#pragma comment(lib,"hiredis.lib")
#pragma comment(lib,"Win32_Interop.lib")
#endif 

#include "uv.h"

#include "redis_wrapper.h"

namespace bb {
	static char* my_strdup(const char* src) {
		int len = strlen(src) + 1;
		char*dst = (char*)malloc(len);
		strcpy(dst, src);
		return dst;
	}

	static void free_strdup(char* str) {
		free(str);
	}

	struct connect_req {
		char* ip;
		int port;

		void(*open_cb)(const char* err, void* context, void* udata);

		char* err;
		void* context;
		void* udata;
	};

	struct redis_context {
		void* pconn; // redis
		uv_mutex_t lock;

		int is_closed;
	};

	struct query_req {
		void* context;
		char* cmd;
		void(*query_cb)(const char* err, redisReply* result, void* udata);

		char* err;
		redisReply* result;
		void* udata;
	};

	static void on_connect_work_cb(uv_work_t* req) {
		struct connect_req* r = (struct connect_req*)req->data;
		struct timeval timeout = { 5, 0 }; // 5 secondes
		redisContext* rc = redisConnectWithTimeout((char*)r->ip,r->port,timeout);
		if (rc->err) {
			printf("Connection error: %s\n", rc->errstr);
			r->err = my_strdup(rc->errstr);
			r->context = NULL;
			redisFree(rc);
		}
		else {
			struct redis_context* c = (struct redis_context*)malloc(sizeof(struct redis_context));
			memset(c, 0, sizeof(struct redis_context));
			c->pconn = rc;
			uv_mutex_init(&c->lock);
			r->err = NULL;
			r->context = c;
		}
	}
		
	static void on_connect_complete_cb(uv_work_t* req, int status) {
		struct connect_req* r = (struct connect_req*)req->data;
		r->open_cb(r->err, r->context, r->udata);

		if (r->ip) {
			free_strdup(r->ip);
		}

		if (r->err) {
			free_strdup(r->err);
		}

		free(r);
		free(req);
	}


	static void on_close_work_cb(uv_work_t* req) {
		struct redis_context* r = (struct redis_context*)(req->data);
		uv_mutex_lock(&r->lock);
		redisContext* c = (redisContext*)r->pconn;
		redisFree(c);
		r->pconn = NULL;
		uv_mutex_unlock(&r->lock);
	}

	static void on_close_complete_cb(uv_work_t* req, int status) {
		struct redis_context* r = (struct redis_context*)(req->data);
		free(r);
		free(req);
	}


	static void on_query_work_cb(uv_work_t* req) {
		query_req* r = (query_req*)req->data;

		struct redis_context* my_conn = (struct redis_context*)(r->context);
		redisContext* rc = (redisContext*)my_conn->pconn;

		uv_mutex_lock(&my_conn->lock);
		redisReply* replay = (redisReply*)redisCommand(rc, r->cmd);
		if (replay->type == REDIS_REPLY_ERROR) {
			r->err = my_strdup(replay->str);
			r->result = NULL;
			freeReplyObject(replay);
		}
		else {
			r->result = replay;
			r->err = NULL;
		}
		uv_mutex_unlock(&my_conn->lock);
	}

	static void on_query_complete_cb(uv_work_t* req, int status) {
		query_req* r = (query_req*)req->data;
		r->query_cb(r->err, r->result, r->udata);

		if (r->cmd) {
			free_strdup(r->cmd);
		}

		if (r->result) {
			freeReplyObject(r->result);
		}

		if (r->err) {
			free_strdup(r->err);
		}

		free(r);
		free(req);
	}


	void redis_wrapper::connect(char* ip, int port, redis_open_cb open_cb, void* udata) {
		uv_work_t* w = (uv_work_t*)malloc(sizeof(uv_work_t*));
		memset(w, 0, sizeof(uv_work_t));

		struct connect_req* r = (struct connect_req*)malloc(sizeof(struct connect_req));
		memset(r, 0, sizeof(struct connect_req));
		r->ip = my_strdup(ip);
		r->port = port;
		r->open_cb = open_cb;
		r->udata = udata;
		w->data = (void*)r;
		uv_queue_work(uv_default_loop(),w, on_connect_work_cb, on_connect_complete_cb);
	}

	void redis_wrapper::close_redis(void* context) {
		struct redis_context* c = (struct redis_context*) context;
		if (c->is_closed) {
			return;
		}

		uv_work_t* w = (uv_work_t*)malloc(sizeof(uv_work_t));
		memset(w, 0, sizeof(uv_work_t));
		w->data = (context);

		c->is_closed = 1;
		uv_queue_work(uv_default_loop(), w, on_close_work_cb, on_close_complete_cb);
	}

	void redis_wrapper::query(void* context, char* cmd, redis_query_cb query_cb, void* udata) {
		struct redis_context* c = (struct redis_context*) context;
		if (c->is_closed) {
			return;
		}

		uv_work_t* w = (uv_work_t*)malloc(sizeof(uv_work_t));
		memset(w, 0, sizeof(uv_work_t));

		query_req* r = (query_req*)malloc(sizeof(query_req));
		memset(r, 0, sizeof(query_req));
		r->context = context;
		r->cmd = my_strdup(cmd);
		r->query_cb = query_cb;
		r->udata = udata;

		w->data = r;
		uv_queue_work(uv_default_loop(), w, on_query_work_cb, on_query_complete_cb);
	}

}
