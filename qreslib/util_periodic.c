#include "util_periodic.h"

void spin_periodic_call(const struct timeval *p_t0, const struct timeval *p_t1, const struct timeval *p_tick, void (*cb_func)(void *), void * cb_param) {
  struct timeval t_target = *p_t0;
  while (timeval_lt(&t_target, p_t1)) {
    struct timeval t;
    gettimeofday(&t, NULL);
    do {
      timeval_add(&t_target, &t_target, p_tick);
    } while (timeval_lt(&t_target, &t));
    do {
      gettimeofday(&t, NULL);
    } while (timeval_lt(&t, &t_target));
    cb_func(cb_param);
  }
}
