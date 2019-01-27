#ifndef _TCP_PROTOCOL_H_
#define _TCP_PROTOCOL_H_

namespace bb {
	class tcp_protocol {
	public:
		/* 读取一个TCP包的头 */
		static bool read_header(unsigned char* data, int data_len, int* pkg_size, int* out_header_size);

		/* tcp 封包 */
		static  unsigned char* package(const unsigned* raw_data, int len, int* pkg_len);

		/* 释放 tcp 封包的数据 */
		static void release_package(unsigned char* tcp_pkg);
	};
}


#endif 
