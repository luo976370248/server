#ifndef _SERVICE_MAN_H_
#define _SERVICE_MAN_H_

namespace bb {
	class service;
	class session;
	struct cmd_msg;

	class service_man {
	public:	 
		static void init();
		static bool register_service(int stype, service* s);
		static bool on_recv_cmd_msg(session* s, struct cmd_msg* msg);
		static void on_session_disconnect(session* s);
	};
}


#endif // !_SERVICE_MAN_H_
