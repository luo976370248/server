#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "session.h"
#include "proto_man.h"
#include "service.h"

namespace bb {
	bool service::on_session_recv_cmd(session* s, struct cmd_msg* msg) {
		return false;
	}
	void service::on_session_disconnect(session* s) {
	}
}