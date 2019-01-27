#ifndef _TIMER_LIST_H_
#define _TIMER_LIST_H_

namespace bb {
#ifdef __cplusplus
	extern "C" {
#endif
		struct timer;
		class timer_list {
		public:
			static struct timer* schedule(void(*on_timer)(void* udata), void* udata, int after_msec, int repeat_count,int repeat_msec);

			static void unschedule(struct timer* t);

			static struct timer* scheduleOnce(void(*on_timer)(void* udata), void* udata, int after_msec);

			static void* get_timer_udata(struct timer* t);
		};
#ifdef __cplusplus
	}
#endif
}

#endif // !_TIMER_LIST_H_
