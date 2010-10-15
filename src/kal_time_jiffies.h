#ifndef KAL_TIME_JIFFY_H_
#define KAL_TIME_JIFFY_H_

#include <linux/time.h>
#ifdef QOS_KS
#  include <linux/timer.h>
#  include <linux/jiffies.h>
#endif

/********************** TIME RELATED **********************/

/** Kernel-dependent long time information. **/
typedef unsigned long kal_time_t;

#define KAL_TIME_NS(sec, nsec) timespec_to_jiffies(&(struct timespec) { .tv_sec = (sec), .tv_nsec = ((long unsigned) nsec) })
#define KAL_TIME_US(sec, usec) timespec_to_jiffies(&(struct timespec) { .tv_sec = (sec), .tv_nsec = ((long unsigned) usec) * 1000 })

#define KAL_TIME_FMT "<%6lu.%5lu>"

#define KAL_TIME_FMT_ARG(t) kal_time_get_sec(t), kal_time_get_usec(t)

static inline kal_time_t kal_time_ns(unsigned long sec, unsigned long nsec) {
  struct timespec ts = { .tv_sec = (sec), .tv_nsec = (nsec) };
  return timespec_to_jiffies(&ts);
}

static inline unsigned long kal_time_get_sec(kal_time_t t) {
  return jiffies_to_msecs(t) / 1000;
}

static inline unsigned long kal_time_get_usec(kal_time_t t) {
  return jiffies_to_usecs(t) % 1000000;
}

static inline unsigned long kal_time_get_nsec(kal_time_t t) {
  struct timespec ts;
  jiffies_to_timespec(t, &ts);
  return ts.tv_nsec;
}

static inline unsigned long long kal_time2usec(kal_time_t t) {
  return jiffies_to_usecs(t);
}

static inline kal_time_t kal_usec2time(unsigned long long usec) {
  return usecs_to_jiffies(usec);
}

static inline kal_time_t kal_time_add(kal_time_t ta, kal_time_t tb) {
  return ta + tb;
}

static inline kal_time_t kal_time_sub(kal_time_t ta, kal_time_t tb) {
  return ta - tb;
}

/** Forcing a (unneeded) signed comparison among seconds */
static inline int kal_time_lt(kal_time_t t1, kal_time_t t2) {
  return time_before(t1, t2);
}

static inline int kal_time_le(kal_time_t t1, kal_time_t t2) {
  return time_before_eq(t1, t2);
}

static inline struct timespec kal_time2timespec(kal_time_t t) {
  struct timespec ts;
  jiffies_to_timespec(t, &ts);
  return ts;
}

#ifdef QOS_KS
static inline unsigned long get_jiffies(void) { return jiffies; }

#else /* !QOS_KS */

unsigned long get_simulation_time(void);
static inline unsigned long get_jiffies(void) { return get_simulation_time();}

static inline unsigned long timespec_to_jiffies(const struct timespec *t) {
  return t->tv_sec * 1000000ull + (unsigned long long) (t->tv_nsec / 1000);
}

static inline void jiffies_to_timespec(const unsigned long jiffies, struct timespec *t) {
  unsigned long us = jiffies;
  *t = KAL_TIME_US((unsigned long) (us / 1000000ul), (unsigned long) (us % 1000000ul));
}
#endif /* QOS_KS */

static inline kal_time_t kal_time_now(void) {
  return jiffies;
}

static inline unsigned long kal_time2jiffies(kal_time_t t) {
  return t;
}

static inline kal_time_t kal_jiffies2time(unsigned long jiffies) {
  return jiffies;
}

#endif /*KAL_TIME_JIFFY_H_*/
