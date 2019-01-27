#ifndef _WSPROTOCOL_H_
#define _WSPROTOCOL_H_

namespace bb {
	class session;

	class ws_protocol {
	public:
		static bool ws_shake_hand(session* s, char* body, int len);
		static int read_ws_header(unsigned char* pkg_data, int pkg_len, int* pkg_size, int* out_header_size);
		static void parser_ws_recv_data(unsigned char* raw_data, unsigned char* mask, int raw_len);

		static unsigned char* package_ws_send_data(unsigned char* raw_data, int len, int* ws_data_len);
		
		static void free_ws_send_data(unsigned char* ws_pkg);
	};
}
#endif
