#ifndef _session_udp_h_
#define _session_udp_h_

#include <uv.h>
#include "session.h"
namespace bb {
	struct cmd_msg;
	class session_udp : public session {
	public:
		uv_udp_t* upd_handle;
		char c_address[32];
		int c_port;
		const struct sockaddr* addr;

	public:
		virtual void close();
		virtual void send(unsigned char* body, int len);
		virtual const char* getAddress(int* client_port);
		virtual void send_msg(struct cmd_msg* msg);
	};
}

#endif // !_session_udp_h_
