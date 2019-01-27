#ifndef _SESSION_H_
#define _SESSION_H_

namespace bb {
	class session {
	public:
		unsigned int as_client;
		unsigned int utag;
		unsigned int uid;
		session() {
			this->as_client = 0;
			this->utag = 0;
			this->uid = 0;
		}
	public:
		virtual void close() = 0;
		virtual void send(unsigned char* body, int len) = 0;
		virtual const char* getAddress(int* port) = 0;
		virtual void send_msg(struct cmd_msg* msg) = 0;
		virtual void send_raw_cmd(struct raw_cmd* msg) = 0;
	};
}
#endif