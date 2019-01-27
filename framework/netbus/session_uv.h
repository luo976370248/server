#ifndef _UVSESSION_H_
#define _UVSESSION_H_

#include <uv.h>
#include "Session.h"
#define  REVC_LEN 4096

namespace bb {

	class session_uv : public session
	{
	public:
		uv_tcp_t handler;
		char ip[32];
		int port;
		int sock_type;

		uv_shutdown_t shutdown;
		bool is_showdown;

		/* �����ݵİ� */
		char recv_buf[REVC_LEN];
		/* �յ����ݵĴ�С */
		int recv_size;

		/* ���� */
		char* long_pkg;
		/* �����Ĵ�С */
		int long_pkg_size;

		/** �ͷ��� ws ���� */
		int is_ws_shake;

	public:
		static session_uv* create();
		static void init_session_allocer();
		static void destroy(session_uv* s);

	public:
		virtual void close();
		virtual void send(unsigned char* body, int len);
		virtual const char* getAddress(int* port);
		virtual void send_msg(struct cmd_msg* msg);

	private:
		void init();
		void exit();
	};
}



#endif
