#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uv.h>
#include "session.h"
#include "session_uv.h"
#include "session_udp.h"
#include "ws_protocol.h"
#include "tcp_protocol.h"
#include "../constant.h"
#include "proto_man.h"
#include "service_man.h"
#include "netbus.h"


namespace bb {
	extern "C" {
		
		static void on_recv_client_cmd(session* s,unsigned char* body, int len) {

			struct cmd_msg* msg = NULL;
			if (proto_man::decode_cmd_msg(body, len, &msg)) {
				if(!service_man::on_recv_cmd_msg((session*)s, msg)) {
					s->close();
				}
				proto_man::cmd_msg_free(msg);
			}
		}

		static void on_recv_ws_data(session_uv* s) {
			unsigned char* pkg_data = s->long_pkg != nullptr ? (unsigned char*)s->long_pkg : (unsigned char*)s->recv_buf;

			while (s->recv_size > 0) {
				int pkg_size = 0;
				int head_size = 0;

				if (!ws_protocol::read_ws_header(pkg_data, s->recv_size, &pkg_size, &head_size)) {
					// 不能解析出包的头
					break;
				}

				if (s->recv_size < pkg_size) {
					// 数据包没有收完，直接退出。
					break;
				}
				
				unsigned char* raw_data = pkg_data + head_size;

				unsigned char* mask = raw_data - 4;
				ws_protocol::parser_ws_recv_data(raw_data, mask, pkg_size - head_size);

				// 收到一个完成的数据包
				on_recv_client_cmd(s, raw_data, pkg_size - head_size);
				
				if (s->recv_size > pkg_size) {
					memmove(pkg_data, pkg_data + pkg_size, s->recv_size - pkg_size);
				}
				s->recv_size -= pkg_size;

				if (s->recv_size == 0 && s->long_pkg_size != NULL) {
					free(s->long_pkg);
					s->long_pkg = NULL;
					s->long_pkg_size = 0;
				}	
			}
		}


		static void on_recv_tcp_data(session_uv* s) {
			unsigned char* pkg_data = s->long_pkg != nullptr ? (unsigned char*)s->long_pkg : (unsigned char*)s->recv_buf;

			while (s->recv_size > 0) {
				int pkg_size = 0;
				int head_size = 0;

				if (!tcp_protocol::read_header(pkg_data, s->recv_size, &pkg_size, &head_size)) {
					// 不能解析出包的头
					break;
				}

				if (s->recv_size < pkg_size) {
					// 数据包没有收完，直接退出。
					break;
				}

				unsigned char* raw_data = pkg_data + head_size;

				// 收到一个完成的数据包
				on_recv_client_cmd(s, raw_data, pkg_size - head_size);

				if (s->recv_size > pkg_size) {
					memmove(pkg_data, pkg_data + pkg_size, s->recv_size - pkg_size);
				}
				s->recv_size -= pkg_size;

				if (s->recv_size == 0 && s->long_pkg_size != NULL) {
					free(s->long_pkg);
					s->long_pkg = NULL;
					s->long_pkg_size = 0;
				}
			}
		}


		struct udp_recv_buf {
			char* recv_buf;
			int max_recv_len;
		};
		static void on_udp_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
			suggested_size = (suggested_size < 8096) ? 8096 : suggested_size;
			struct udp_recv_buf* udp_buf = (struct udp_recv_buf*)handle->data;
			if (udp_buf->max_recv_len < suggested_size) {
				if (udp_buf->recv_buf) {
					free(udp_buf->recv_buf);
					udp_buf->recv_buf = NULL;
				}

				udp_buf->recv_buf = (char*)malloc(suggested_size);
				udp_buf->max_recv_len = suggested_size;
			}
			buf->base = udp_buf->recv_buf;
			buf->len = udp_buf->max_recv_len;
		}

		static void after_udp_recv_cb(uv_udp_t* handle,ssize_t nread,const uv_buf_t* buf,const struct sockaddr* addr,unsigned flags) {

			/*session_udp udp_s;
			udp_s.upd_handle = handle;
			uv_ip4_name((struct sockaddr_in*)addr, udp_s.c_address, 128);
			udp_s.c_port = ntohs(((struct sockaddr_in*)addr)->sin_port);;

			on_recv_client_cmd((session*)&udp_s, (unsigned char*)buf->base, nread);*/
			// uv_ii
		}

		static void on_alloc_buf_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
			session_uv* s = (session_uv*)handle->data;
			
			
			if (s->recv_size < REVC_LEN) {
				//  预定内存还没有收满
				*buf = uv_buf_init(s->recv_buf + s->recv_size, REVC_LEN - s->recv_size);
			}
			else {
				// 如果 预定内存收满了后，还有数据没有收完，需要准备一个长包
				if (s->long_pkg == NULL) {
					if (s->sock_type == WS_SOCKET && s->is_ws_shake) {
						// ws > RECV_LEN package
						int pkg_size;
						int head_size;

						ws_protocol::read_ws_header((unsigned char*)s->recv_buf, s->recv_size, &pkg_size, &head_size);
						s->long_pkg_size = pkg_size;
						s->long_pkg = (char*)malloc(pkg_size);
						memcpy(s->long_pkg, s->recv_buf, s->recv_size);
					}
					else {
						// tcp > RECV_LEN package
					}
					*buf = uv_buf_init(s->long_pkg + s->recv_size, s->long_pkg_size - s->recv_size);
				}
			}

			
		}

		static void on_close_cb(uv_handle_t* handle) {
			session_uv* session = (session_uv*)handle->data;
			session_uv::destroy(session);
		}

		static void on_showdown_cb(uv_shutdown_t* req, int status) {
			uv_close((uv_handle_t*)req->handle, on_close_cb);
		}

		static void on_after_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
			session_uv* s = (session_uv*)stream->data;
			if (nread < 0) {
				s->close();
				return;
			}

			s->recv_size += nread;
			if (s->sock_type == WS_SOCKET) {
				// web socket
				if (s->is_ws_shake == 0) {
					// shake hand
					if (ws_protocol::ws_shake_hand((session*)s, s->recv_buf, s->recv_size)) {
						s->is_ws_shake = 1;
					}
				}
				else {
					// websocket recv/send data
					on_recv_ws_data(s);
				}
			}
			else {
				// tcp socket
				on_recv_tcp_data(s);
			}
		}


		static void on_connect_cb(uv_stream_t* server, int status) {

			session_uv* s = session_uv::create();
			uv_tcp_t* client = &s->handler;
			memset(client, 0, sizeof(uv_tcp_t));
			uv_tcp_init(uv_default_loop(), client);
			client->data = (void*)s;
			uv_accept(server, (uv_stream_t*)client);

			struct sockaddr_in addr;
			int len = sizeof(addr);
			uv_tcp_getpeername(client, (sockaddr*)&addr, &len);
			uv_ip4_name(&addr, (char*)s->ip, 64);
			s->port = ntohs(addr.sin_port);
			printf("new client %s:%d \n ", s->ip, s->port);
			s->sock_type = (int)server->data;
			uv_read_start((uv_stream_t*)client, on_alloc_buf_cb, on_after_read_cb);
		}


	}


	static netbus _instance;
	netbus* netbus::instance() {
		return &_instance;
	}

	

	void netbus::start_tcp_server(int port) {
		uv_tcp_t* listen = new uv_tcp_t();
		memset(listen, 0, sizeof(uv_tcp_t));

		uv_tcp_init(uv_default_loop(), listen);

		struct sockaddr_in addr;
		uv_ip4_addr("0,0,0,0", port, &addr);

		int ret = uv_tcp_bind(listen, (const sockaddr*)&addr, 0);
		if (ret != 0) {
			printf("bind error \n");
			free(listen);
			return;
		}
		listen->data = (void*)TCP_SOCKET;
		uv_listen((uv_stream_t*)listen, SOMAXCONN, on_connect_cb);
		
	}

	void netbus::start_ws_server(int port) {
		uv_tcp_t* listen = new uv_tcp_t();
		memset(listen, 0, sizeof(uv_tcp_t));

		uv_tcp_init(uv_default_loop(), listen);

		struct sockaddr_in addr;
		uv_ip4_addr("0,0,0,0", port, &addr);

		int ret = uv_tcp_bind(listen, (const sockaddr*)&addr, 0);
		if (ret != 0) {
			printf("bind error \n");
			free(listen);
			return;
		}
		listen->data = (void*)WS_SOCKET;
		uv_listen((uv_stream_t*)listen, SOMAXCONN, on_connect_cb);
		
	}

	struct connect_cb {
		void(*on_connected)(int err, session* s, void* udata);
		void* udata;
	};

	void netbus::start_udp_server(int port) {
		uv_udp_t* server = (uv_udp_t*)malloc(sizeof(uv_udp_t));
		memset(server, 0, sizeof(uv_udp_t));

		uv_udp_init(uv_default_loop(), server);

		struct udp_recv_buf* udp_buf = (struct udp_recv_buf*)malloc(sizeof(struct udp_recv_buf));
		memset(udp_buf, 0, sizeof(struct udp_recv_buf));
		server->data = udp_buf;
		struct sockaddr_in addr;
		uv_ip4_addr("0,0,0,0", port, &addr);
		uv_udp_bind(server, (const struct sockaddr*)&addr, 0);
		uv_udp_recv_start(server, on_udp_alloc_cb, after_udp_recv_cb);
	}

	static void uv_alloc_buf(uv_handle_t* handle,
			size_t suggested_size,
			uv_buf_t* buf) {

		session_uv* s = (session_uv*)handle->data;

		if (s->recv_size < RECV_LEN) {
			*buf = uv_buf_init(s->recv_buf + s->recv_size, RECV_LEN - s->recv_size);
		}
		else {
			if (s->long_pkg == NULL) { // alloc mem
				if (s->sock_type == WS_SOCKET && s->is_ws_shake) { // ws > RECV_LEN's package
					int pkg_size;
					int head_size;
					ws_protocol::read_ws_header((unsigned char*)s->recv_buf, s->recv_size, &pkg_size, &head_size);
					s->long_pkg_size = pkg_size;
					s->long_pkg = (char*)malloc(pkg_size);
					memcpy(s->long_pkg, s->recv_buf, s->recv_size);
				}
				else { // tcp > RECV_LEN's package
					int pkg_size;
					int head_size;
					tcp_protocol::read_header((unsigned char*)s->recv_buf, s->recv_size, &pkg_size, &head_size);
					s->long_pkg_size = pkg_size;
					s->long_pkg = (char*)malloc(pkg_size);
					memcpy(s->long_pkg, s->recv_buf, s->recv_size);
				}
			}
			*buf = uv_buf_init(s->long_pkg + s->recv_size, s->long_pkg_size - s->recv_size);
		}
	}

	static void after_read(uv_stream_t* stream,
			ssize_t nread,
			const uv_buf_t* buf) {
		session_uv* s = (session_uv*)stream->data;
		if (nread < 0) {
			// uv_shutdown_t* reg = &s->shutdown;
			// memset(reg, 0, sizeof(uv_shutdown_t));
			// uv_shutdown(reg, stream, on_shutdown);
			s->close();
			return;
		}
		// end

		s->recv_size += nread;

		if (s->sock_type == WS_SOCKET) { // websocket
			if (s->is_ws_shake == 0) { // shake handle
				if (ws_protocol::ws_shake_hand((session*)s, s->recv_buf, s->recv_size)) {
					s->is_ws_shake = 1;
					s->recv_size = 0;
				}
			}
			else { // websocket recv/send data
				on_recv_ws_data(s);
			}
		}
		else { // TCP sokcet
			on_recv_tcp_data(s);
		}
	}

	static void after_connect(uv_connect_t* handle, int status) {
		session_uv* s = (session_uv*)handle->handle->data;
		struct connect_cb* cb = (struct connect_cb*)handle->data;

		if (status) {
			if (cb->on_connected) {
				cb->on_connected(1, NULL, cb->udata);
			}
			s->close();
			free(cb);
			free(handle);
			return;
		}

		if (cb->on_connected) {
			cb->on_connected(0, (session*)s, cb->udata);
		}
		uv_read_start((uv_stream_t*)handle->handle, uv_alloc_buf, after_read);

		free(cb);
		free(handle);
	}

	void netbus::tcp_connect(const char* server_ip, int port, on_connected_cb connected_cb, void* udata) {
		struct sockaddr_in bind_addr;
		int iret = uv_ip4_addr(server_ip, port, &bind_addr);
		if (iret) {
			return;
		}

		session_uv* s = session_uv::create();
		uv_tcp_t* client = &s->handler;
		memset(client, 0, sizeof(uv_tcp_t));
		uv_tcp_init(uv_default_loop(), client);
		client->data = (void*)s;
		s->as_client = 1;
		s->sock_type = TCP_SOCKET;
		strcpy(s->ip, server_ip);
		s->port = port;

		uv_connect_t* connect_req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
		struct connect_cb* cb = (struct connect_cb*)malloc(sizeof(struct connect_cb));
		cb->on_connected = connected_cb;
		cb->udata = udata;
		connect_req->data = (void*)cb;

		iret = uv_tcp_connect(connect_req, client, (struct sockaddr*)&bind_addr, after_connect);
		if (iret) {
			// log_error("uv_tcp_connect error!!!");
			return;
		}
	}

	void netbus::run() {
		uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	}

	void netbus::init() {
		service_man::init();
		session_uv::init_session_allocer();
	}
}


