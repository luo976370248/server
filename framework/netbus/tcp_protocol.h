#ifndef _TCP_PROTOCOL_H_
#define _TCP_PROTOCOL_H_

namespace bb {
	class tcp_protocol {
	public:
		/* ��ȡһ��TCP����ͷ */
		static bool read_header(unsigned char* data, int data_len, int* pkg_size, int* out_header_size);

		/* tcp ��� */
		static  unsigned char* package(const unsigned* raw_data, int len, int* pkg_len);

		/* �ͷ� tcp ��������� */
		static void release_package(unsigned char* tcp_pkg);
	};
}


#endif 
