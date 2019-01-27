#include <iostream>

#include <uv.h>
#include "Session.h"
#include "../utils/cache_manager.h"
#include "ws_protocol.h"
#include "tcp_protocol.h"
#include "../constant.h"
#include "proto_man.h"
#include "service_man.h"
#include "session_uv.h"



namespace bb {

	struct cache_allocer* session_allocer = nullptr;
	struct cache_allocer* wq_allocer = nullptr;
	cache_allocer* wbuf_allocer = nullptr;
	extern "C" {
		

		static void on_after_write_cb(uv_write_t* req, int status) {
			if (status == 0) {
				printf("write success \n");
			}
			cache_manager::cache_free(wq_allocer, req);
		}

		static void on_close_cb(uv_handle_t* handle) {
			session_uv* session = (session_uv*)handle->data;
			session_uv::destroy(session);
		}

		static void on_shutdown_cb(uv_shutdown_t* req, int status) {
			uv_close((uv_handle_t*)req->handle, on_close_cb);
		}
	}
	
	void session_uv::init_session_allocer() {
		if (session_allocer == nullptr) {
			session_allocer = cache_manager::create(SESSION_CACHE_CAPACITY, sizeof(session_uv));
		}

		if (wq_allocer == nullptr) {
			wq_allocer = cache_manager::create(WQ_CACHE_CAPACITY, sizeof(uv_write_t));
		}

		if (wbuf_allocer == nullptr) {
			wbuf_allocer = cache_manager::create(WBUF_CACHE_CAPACITY, CMD_SZIE_CACHE);
		}
	}

	session_uv* session_uv::create() {
		session_uv* s = (session_uv*)cache_manager::cache_alloc(session_allocer, sizeof(session_uv));
		// TODO 
		s->session_uv::session_uv();
		
		s->init();
		return s;
	}

	void session_uv::destroy(session_uv* s) {
		s->exit();
		// TODO 
		s->session_uv::~session_uv();
		cache_manager::cache_free(session_allocer, s);
	}

	void session_uv::init() {
		memset(this->ip, 0, sizeof(this->ip));
		this->port = 0;
		this->recv_size = 0;
		this->is_showdown = false;
		this->is_ws_shake = 0;
		this->long_pkg = nullptr;
		this->long_pkg_size = 0;
	}

	void session_uv::exit() {

	}

	void session_uv::close() {
		if (this->is_showdown) {
			return;
		}
		service_man::on_session_disconnect(this);
		this->is_showdown = true;
		uv_shutdown_t* req = &this->shutdown;
		memset(req, 0, sizeof(uv_shutdown_t));
		uv_shutdown(req, (uv_stream_t*)&this->handler, on_shutdown_cb);
	}
	void session_uv::send(unsigned char* body, int len) {
		uv_write_t* w_req = (uv_write_t*)cache_manager::cache_alloc(wq_allocer, sizeof(uv_write_t));

		uv_buf_t w_buf;
		if (this->sock_type == WS_SOCKET) {
			if (!this->is_ws_shake) {
				// ws ÎÕÊÖ
				w_buf = uv_buf_init((char*)body, len);
				uv_write(w_req, (uv_stream_t*)&this->handler, &w_buf, 1, on_after_write_cb);
				return;
			}
			int ws_pkg_len;
			unsigned char* ws_pkg = ws_protocol::package_ws_send_data(body, len, &ws_pkg_len);
			w_buf = uv_buf_init((char*)ws_pkg, ws_pkg_len);
			uv_write(w_req, (uv_stream_t*)&this->handler, &w_buf, 1, on_after_write_cb);
			ws_protocol::free_ws_send_data(ws_pkg);
		}
		else {
			// tcp£¬
			int tcp_pkg_len;
			unsigned char* tcp_pkg = tcp_protocol::package((const unsigned int*)body, len, &tcp_pkg_len);
			w_buf = uv_buf_init((char*)tcp_pkg, tcp_pkg_len);
			uv_write(w_req, (uv_stream_t*)&this->handler, &w_buf, 1, on_after_write_cb);
			tcp_protocol::release_package(tcp_pkg);
		}


		
	}

	const char* session_uv::getAddress(int* port) {
		*port = this->port;
		return this->ip;
	}

	void session_uv::send_msg(struct cmd_msg* msg) {
		unsigned char* encode_pkg = NULL;
		int encode_len = 0;
		encode_pkg = proto_man::encode_msg_to_raw(msg, &encode_len);
		if (encode_pkg) {
			this->send(encode_pkg, encode_len);
			proto_man::msg_raw_free(encode_pkg);
		}
	}
}
