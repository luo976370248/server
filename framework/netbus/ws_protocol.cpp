#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../3rd/http_parser/http_parser.h"
#include "../../3rd/crypto/base64_encoder.h"
#include "../../3rd/crypto/sha1.h"

#include "session.h"
#include "../utils/cache_manager.h"
#include "ws_protocol.h"



namespace bb {

	extern cache_allocer* wbuf_allocer;

	// 
	static const char* wb_migic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	// base64(sha1(key + wb_migic))
	static const char *wb_accept = "HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade:websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: %s\r\n"
		"WebSocket-Protocol:chat\r\n\r\n";
	static char filed_sec_key[512];
	static char value_sec_key[512];
	static int is_sec_key = 0;
	static int has_sec_key = 0;
	static int is_shaker_ended = 0;

	extern "C" {
		int on_message_complete_cb(http_parser* p) {
			is_shaker_ended = 1;
			return 0;
		}
	}

	static int on_header_field_cb(http_parser* p, const char *at, size_t length) {
		if (strncmp(at, "Sec-WebSocket-Key", length) == 0) {
			is_sec_key = 1;
		}
		else {
			is_sec_key = 0;
		}
		return 0;
	}

	static int on_header_value_cb(http_parser* p, const char *at, size_t length) {
		if (!is_sec_key) {
			return 0;
		}
		strncpy(value_sec_key, at, length);
		value_sec_key[length] = 0;
		has_sec_key = 1;
		return 0;
	}

	bool ws_protocol::ws_shake_hand(session* s, char* body, int len) {
		http_parser_settings setting;
		http_parser_settings_init(&setting);

		setting.on_header_field = on_header_field_cb;
		setting.on_header_value = on_header_value_cb;
		setting.on_message_complete = on_message_complete_cb;

		http_parser p;
		http_parser_init(&p, HTTP_REQUEST);
		is_sec_key = 0;
		has_sec_key = 0;
		is_shaker_ended = 0;

		http_parser_execute(&p, &setting, body, len);

		if (has_sec_key && is_shaker_ended) {
			printf("Sec-WebSocket-Key: %s\n", value_sec_key);

			static char key_migic[512];
			static char sha1_key_migic[SHA1_DIGEST_SIZE];
			static char send_client[512];

			int sha1_size;

			sprintf(key_migic, "%s%s", value_sec_key, wb_migic);
			crypt_sha1((unsigned char*)key_migic, strlen(key_migic), (unsigned char*)&sha1_key_migic, &sha1_size);
			int base64_len;
			char* base_buf = base64_encode((uint8_t*)sha1_key_migic, sha1_size, &base64_len);
			sprintf(send_client, wb_accept, base_buf);
			base64_encode_free(base_buf);

			s->send((unsigned char*)send_client, strlen(send_client));
			return true;
		}
		return false;
	}

	int ws_protocol::read_ws_header(unsigned char* pkg_data, int pkg_len, int* pkg_size, int* out_header_size) {

		// ��ȡͷ
		if (pkg_data[0] != 0x81 && pkg_data[0] != 0x82) {
			return false;
		}

		// ������������Ϣ
		if (pkg_len < 2) {
			return false;
		}

		unsigned int data_len = pkg_data[1] & 0x0000007f;
		int head_size = 2;
		if (data_len == 126) {
			head_size += 2;
			if (pkg_len < head_size) {
				return false;
			}
			data_len = pkg_data[3] | (pkg_data[2] << 8);

		}
		else if (data_len == 127) {
			head_size += 8;
			if (pkg_len < head_size) {
				return false;
			}

			unsigned int low = pkg_data[5] | (pkg_data[4] << 8) | (pkg_data[3] << 16) | (pkg_data[2] << 24);
			unsigned int hight = pkg_data[9] | (pkg_data[8] << 8) | (pkg_data[7] << 16) | (pkg_data[6] << 24);
			data_len = low;
		}

		head_size += 4; // 4 ��mask
		*pkg_size = data_len + head_size;
		*out_header_size = head_size;

		return true;
	 }

	void ws_protocol::parser_ws_recv_data(unsigned char* raw_data, unsigned char* mask, int raw_len) {
		for (unsigned char i = 0; i < raw_len; i++) {
			raw_data[i] = raw_data[i] ^ mask[i % 4];
		}

	}
	unsigned char* ws_protocol::package_ws_send_data(unsigned char* raw_data, int len, int* ws_data_len) {
			int head_size = 2;
			if (len > 125 && len < 65536) {
				head_size += 2;
			}
			else if (len >= 65536) {
				head_size += 8;
				return NULL;
			}
			// cache malloc
			unsigned char* data_buf = (unsigned char*)cache_manager::cache_alloc(wbuf_allocer, head_size + len);
			data_buf[0] = 0x81;
			if (len <= 125) {
				data_buf[1] = len;
			}
			else if (len > 125 && len < 65536) {
				data_buf[1] = 126;
				data_buf[2] = (len & 0x0000ff00) >> 8;
				data_buf[3] = (len & 0x000000ff);
			}

			memcpy(data_buf + head_size, raw_data, len);
			*ws_data_len = (head_size + len);

			return data_buf;
	}

	void ws_protocol::free_ws_send_data(unsigned char* ws_pkg) {
		cache_manager::cache_free(wbuf_allocer, ws_pkg);
	}
}