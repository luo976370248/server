#ifndef _NETBUS_H_
#define _NETBUS_H_

#include <uv.h>

class session;

namespace bb {
	typedef  void(*on_connected_cb)(int err, session* s, void* udata);

	class netbus
	{
	public:
		static netbus* instance();

	public:
		void init();
		void start_tcp_server(int port);
		void start_ws_server(int port);
		void start_udp_server(int port);
		void tcp_connect(const char* server_ip, int port, on_connected_cb connected_cb, void* udata);
		void run();
	};
}
#endif // !_NETBUS_H_
