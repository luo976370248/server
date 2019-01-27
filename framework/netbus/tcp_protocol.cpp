#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/cache_manager.h"
#include "tcp_protocol.h"


namespace bb {
	extern cache_allocer* wbuf_allocer;


	/** 读取一个TCP包的头 */
	bool tcp_protocol::read_header(unsigned char* data, int data_len, int* pkg_size, int* out_header_size) {
		if (data_len < 2) {
			// 不够解析出包
			return false;
		}

		*pkg_size = (data[0] | data[1] << 8);
		*out_header_size = 2;
		return true;
	}

	unsigned char* tcp_protocol::package(const unsigned* raw_data, int len, int* pkg_len) {
		int head_size = 2;

		// cache malloc
		*pkg_len = (head_size + len);
		unsigned char* data_buf = (unsigned char*)cache_manager::cache_alloc(wbuf_allocer, *pkg_len);
		data_buf[0] = (unsigned char)((*pkg_len) & 0x000000ff);
		data_buf[1] = (unsigned char)((*pkg_len) & 0x0000ff00 >> 8);
		memcpy(data_buf + head_size, raw_data, len);
		return data_buf;
	}

	void tcp_protocol::release_package(unsigned char* tcp_pkg) {
		// cache free
		cache_manager::cache_free(wbuf_allocer, tcp_pkg);
	}
}
