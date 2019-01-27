#ifndef _PF_CMD_MAP_H_
#define _PF_CMD_MAP_H_

namespace bb {
	extern char* pf_cmd_map[];

	class cmd_map {
	public:
		static void init_pf_cmd_map();
	};
}

#endif // !_PF_CMD_MAP_H_
