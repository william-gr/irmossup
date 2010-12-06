#ifndef __UTIL_PERIODIC_H__
#define __UTIL_PERIODIC_H__

#include "util_timeval.h"

void spin_periodic_call(const struct timeval *p_t0, const struct timeval *p_t1,
			const struct timeval *p_tick, void (*cb_func)(void *), void * cb_param);

#endif // __UTIL_PERIODIC_H__
