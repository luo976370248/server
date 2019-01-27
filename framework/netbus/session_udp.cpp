#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "uv.h"
#include "session.h"
#include "proto_man.h"

#include "session_udp.h"

namespace bb {
	
	static void on_udp_send_end_cb(uv_udp_send_t* req, int status) {
		if (status == 0) {

		}
		free(req);
	}
	
	void session_udp::close() {

	}

	void session_udp::send(unsigned char* body, int len) {
		/*uv_buf_t w_buf;
		w_buf = uv_buf_init((char*)body, len);
		uv_udp_send_t* req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
		uv_udp_send(req, this->upd_handle, &w_buf, this->c_address, on_udp_send_end_cb);*/
	}
	const char* session_udp::getAddress(int* client_port) {
		*client_port = this->c_port;
		return (const char*)this->c_address;
	}

	void session_udp::send_msg(struct cmd_msg* msg) {
		unsigned char* encode_pkg = NULL;
		int encode_len = 0;
		encode_pkg = proto_man::encode_msg_to_raw(msg, &encode_len);
		if (encode_pkg) {
			this->send(encode_pkg, encode_len);
			proto_man::msg_raw_free(encode_pkg);
		}
	}
}		 