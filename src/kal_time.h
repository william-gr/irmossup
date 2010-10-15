#ifndef KAL_TIME_H_
#define KAL_TIME_H_

#include "kal_config.h"

#ifdef KAL_USE_HRTIMER
#  include "kal_time_timespec.h"
#else
#  include "kal_time_jiffies.h"
#endif

#ifdef QOS_KS

/** Convert a kal_time_t into a ktime_t **/
static inline ktime_t kal_time2ktime(kal_time_t t) {
  return ktime_set(kal_time_get_sec(t), kal_time_get_nsec(t));
}

/** Convert a ktime_t into a kal_time_t **/
static inline kal_time_t kal_ktime2time(ktime_t kt) {
  struct timespec ts = ktime_to_timespec(kt);
  return kal_time_ns(ts.tv_sec, ts.tv_nsec);
}

#endif

#endif /*KAL_TIME_H_*/
