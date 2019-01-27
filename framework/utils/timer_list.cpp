#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uv.h"
#include "timer_list.h"

namespace bb {
#define  my_malloc malloc
#define  my_free free

	struct timer {
		uv_timer_t uv_timer; // libuv timer handle
		void(*on_timer)(void* udata);
		void* udata;
		int repeat_count; // -1:表示一直循环
	};

	static struct timer* alloc_timer(void(*on_timer)(void* udata), void* udata, int repeat_count) {
		struct timer* t = (struct timer*)my_malloc(sizeof(struct timer));
		memset(t, 0, sizeof(struct timer));
		t->on_timer = on_timer;
		t->repeat_count = repeat_count;
		t->udata = udata;
		uv_timer_init(uv_default_loop(), &t->uv_timer);
		return t;
	}

	static void free_timer(struct timer* t) {
		my_free(t);
	}

	static void on_uv_timer_cb(uv_timer_t* handle) {
		struct timer* t = (struct timer*)handle->data;
		if (t->repeat_count < 0) {
			// 不断触发
			t->on_timer(t->udata);
		}
		else {
			t->repeat_count--;
			t->on_timer(t->udata);
			if (t->repeat_count == 0) {
				uv_timer_stop(&t->uv_timer);
				free_timer(t);
			}
		}
	}


	struct timer* timer_list::schedule(void(*on_timer)(void* udata), void* udata, int after_msec, int repeat_count, int repeat_msec) {
		struct timer* t = alloc_timer(on_timer, udata, repeat_count);
		t->uv_timer.data = t;
		uv_timer_start(&t->uv_timer, on_uv_timer_cb, after_msec, repeat_msec);
		return t;
	}

	void timer_list::unschedule(struct timer* t) {
		if (t->repeat_count == 0) { // 全部触发完成，;
			return;
		}
		uv_timer_stop(&t->uv_timer);
		free_timer(t);
	}

	struct timer* timer_list::scheduleOnce(void(*on_timer)(void* udata), void* udata, int after_msec) {
		return schedule(on_timer, udata, after_msec, 1, after_msec);
	}

	void* timer_list::get_timer_udata(struct timer* t) {
		return t->udata;
	}

}
