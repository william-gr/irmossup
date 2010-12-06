#include <sys/time.h>
#include <time.h>

/** Add two timeval values and normalize result */
static inline void timeval_add(struct timeval *dst_ts, const struct timeval *src1_ts, const struct timeval *src2_ts) {
  dst_ts->tv_sec = src1_ts->tv_sec + src2_ts->tv_sec;
  dst_ts->tv_usec = src1_ts->tv_usec + src2_ts->tv_usec;
  long int delta_us = dst_ts->tv_usec - 1000000lu;
  if (delta_us >= 0) {
    dst_ts->tv_usec = delta_us;
    dst_ts->tv_sec++;
  }
}

/** Sub two timeval values and normalize result.
 *
 * @note This also works if dst_ts == src1 || dst_ts == src2 || src1 == src2
 */
static inline void timeval_sub(struct timeval *dst_ts, const struct timeval *src1_ts, const struct timeval *src2_ts) {
  dst_ts->tv_sec = src1_ts->tv_sec - src2_ts->tv_sec;
  dst_ts->tv_usec = src1_ts->tv_usec - src2_ts->tv_usec;
  if (dst_ts->tv_usec < 0) {
    dst_ts->tv_usec = dst_ts->tv_usec + 1000000lu;
    dst_ts->tv_sec--;
  }
}

static inline long timeval_sub_us(const struct timeval *src1_tv, const struct timeval *src2_tv) {
  return (src1_tv->tv_sec - src2_tv->tv_sec) * 1000000L
      + (src1_tv->tv_usec - src2_tv->tv_usec);
}

static inline int timeval_lt(const struct timeval *t1, const struct timeval *t2) {
  return (t1->tv_sec < t2->tv_sec)
    || ((t1->tv_sec == t2->tv_sec) && (t1->tv_usec < t2->tv_usec));
}

static inline int timeval_le(const struct timeval *t1, const struct timeval *t2) {
  return (t1->tv_sec < t2->tv_sec)
    || ((t1->tv_sec == t2->tv_sec) && (t1->tv_usec <= t2->tv_usec));
}

static inline int timeval_eq(const struct timeval *t1, const struct timeval *t2) {
  return (t1->tv_sec == t2->tv_sec) && (t1->tv_usec == t2->tv_usec);
}
