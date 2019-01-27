#ifndef _TIME_STAMP_H_
#define _TIME_STAMP_H_

namespace bb {
		// ��ȡ��ǰ��ʱ���
	    unsigned long timestamp();

		// ��ȡ�������ڵ�ʱ���"%Y(��)%m(��)%d(��)%H(Сʱ)%M(��)%S(��)"
		unsigned long date2timestamp(const char* fmt_date, const char* date);

		// fmt_date "%Y(��)%m(��)%d(��)%H(Сʱ)%M(��)%S(��)"
		void timestamp2date(unsigned long t, char*fmt_date, char* out_buf, int buf_len);

		unsigned long timestamp_today();

		unsigned long timestamp_yesterday();
}



#endif // !_TIME_STAMP_H_
