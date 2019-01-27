#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uv.h"

#ifdef WIN32
#include <winsock.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libmysql.lib")
#endif

#include "mysql.h"

#include "mysql_wrapper.h"

namespace bb {
	struct connect_req {
		char* ip;
		int port;
		char* db_name;
		char* uname;
		char* upwd;
		void(*open_cb)(const char* err, void* context, void* udata);
		char* err;
		void* context;
		void* udata;
	};

	struct mysql_context {
		void* conn; // mysql
		uv_mutex_t lock;
		int is_close;
	};

	struct query_req {
		void* context;
		char* sql;
		void(*query_cb)(const char* err, MYSQL_RES* result, void* udata);
		char* err;
		MYSQL_RES* result;
		void* udata;
	};

	static char* my_strdup(const char* src) {
		int len = strlen(src) + 1;
		char* dst = (char*)malloc(len);
		strcpy(dst, src);
		return dst;
	}

	static void free_strdup(char* str) {
		free(str);
	}
	static void on_connect_work_cb(uv_work_t* req) {
		struct connect_req* r = (struct connect_req*)req->data;
		MYSQL* pconn = mysql_init(NULL);
		if (mysql_real_connect(pconn, r->ip, r->uname, r->upwd, r->db_name, r->port, NULL, 0)) {
			struct mysql_context* c = (struct mysql_context*)malloc(sizeof(struct mysql_context));
			memset(c, 0, sizeof(struct mysql_context));
			c->conn = pconn;
			uv_mutex_init(&c->lock);
			r->context = c;
			r->err = NULL;
		}
		else {
			r->context = nullptr;
			r->err = my_strdup(mysql_error(pconn));
		}
	}

	static void on_connect_complete_cb(uv_work_t* req, int status) {
		struct connect_req* r = (struct connect_req*)req->data;
		r->open_cb(r->err, r->context, r->udata);
		if (r->ip) {
			free_strdup(r->ip);
		}

		if (r->db_name) {
			free_strdup(r->db_name);
		}

		if (r->uname) {
			free_strdup(r->uname);
		}

		if (r->upwd) {
			free_strdup(r->upwd);
		}

		if (r->err) {
			free_strdup(r->err);
		}

		free(r);
		free(req);
	}

	static void on_clos_work_cb(uv_work_t* req) {
		struct mysql_context* r = (struct mysql_context*)(req->data);
		uv_mutex_lock(&r->lock);
		MYSQL* pconn = (MYSQL*)r->conn;
		mysql_close(pconn);
		uv_mutex_unlock(&r->lock);
	}

	static void on_close_complete_cb(uv_work_t* req, int status) {
		struct mysql_context* r = (struct mysql_context*)(req->data);
		free(r);
		free(req);
	}
	
	static void on_query_work_cb(uv_work_t* req) {
		query_req* r = (query_req*)req->data;
		struct mysql_context* my_conn = (struct mysql_context*)(r->context);
		uv_mutex_lock(&my_conn->lock);

		MYSQL* pconn = (MYSQL*)my_conn->conn;
		int ret = mysql_query(pconn, r->sql);
		if (ret != 0) {
			r->err = my_strdup(mysql_error(pconn));
			r->result = NULL;
			uv_mutex_unlock(&my_conn->lock);
			return;
		}
		r->err = NULL;
		MYSQL_RES* result = mysql_store_result(pconn);
		r->result = result;
		uv_mutex_unlock(&my_conn->lock);
	}

	static void  on_query_complete_cb(uv_work_t* req, int status) { 
		query_req* r = (query_req*)req->data;
		r->query_cb(r->err, r->result, r->udata);
	
		if (r->sql) {
			free_strdup(r->sql);
		}

		if (r->result) {
			mysql_free_result(r->result);
			r->result = NULL;
		}

		if (r->err) {
			free_strdup(r->err);
		}

		free(r);
		free(req);
	}

	void mysql_wrapper::connect(char* ip, int port, char* db_name, char* uname, char* pwd, mysql_open_cb open_cb, void* udata) {
		uv_work_t* w = (uv_work_t*)malloc(sizeof(uv_work_t));
		memset(w, 0, sizeof(uv_work_t));

		struct connect_req* r = (struct connect_req*)malloc(sizeof(struct connect_req));
		memset(r, 0, sizeof(struct connect_req));
		r->ip = my_strdup(ip);
		r->port = port;
		r->db_name = my_strdup(db_name);
		r->uname = my_strdup(uname);
		r->upwd = my_strdup(pwd);
		r->open_cb = open_cb;
		r->udata = udata;
		w->data = (void*)r;
		uv_queue_work(uv_default_loop(), w, on_connect_work_cb, on_connect_complete_cb);
	}

	void mysql_wrapper::close(void* context) {
		struct mysql_context* c = (struct mysql_context*)context;
		if (c->is_close) {
			return;
		}

		uv_work_t* w = (uv_work_t*)malloc(sizeof(uv_work_t));
		memset(w, 0, sizeof(uv_work_t));
		w->data = context;
		c->is_close = 1;

		uv_queue_work(uv_default_loop(), w, on_clos_work_cb, on_close_complete_cb);
	}


	void mysql_wrapper::query(void* context, char* sql, mysql_query_cb query_cb, void* udata) {
		struct mysql_context* c = (struct mysql_context*)context;
		if (c->is_close) {
			return;
		}

		uv_work_t* w = (uv_work_t*)malloc(sizeof(uv_work_t));
		memset(w, 0, sizeof(uv_work_t));

		struct query_req* r = (query_req*)malloc(sizeof(query_req));
		memset(r, 0, sizeof(query_req));
		r->context = context;
		r->sql = my_strdup(sql);
		r->query_cb = query_cb;
		r->udata = udata;

		w->data = r;
		uv_queue_work(uv_default_loop(), w, on_query_work_cb, on_query_complete_cb);
	}
}

