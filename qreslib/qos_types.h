#ifndef __QOS_TYPES_H__
#define __QOS_TYPES_H__

#include "qos_ul.h"
#include "rres_config.h"

#ifdef QOS_KS
#  include <linux/types.h>
#else
#  include <sys/types.h>
#endif

/****************************************************************************
 *                             BASIC TYPES                                  *
 ****************************************************************************/
/** Represents times in usecs	*/
typedef long qres_time_t;
/** Represents absolute time    */
typedef long long qres_atime_t;

#define QRES_TIME_FMT "%ld"	/**< Format string for printing qres_time_t	*/
#define QRES_ATIME_FMT "%lld"	/**< Format string for printing qres_time_t	*/

static inline struct timespec qres_time_to_timespec(qres_time_t time) {
  struct timespec dummy_ts;
  dummy_ts.tv_sec = time / 1000000;
  dummy_ts.tv_nsec = (time % 1000000) * 1000;
  return dummy_ts;
}

static inline qres_time_t qres_timespec_to_time(struct timespec ts) {
  return (qres_time_t)
    ts.tv_sec * 1000000 + (qres_time_t) ts.tv_nsec / 1000;
}

typedef int qres_sid_t;		/**< Identifies uniquely a server		*/
#define QRES_SID_FMT "%d"	/**< Format string for printing qres_sid_t	*/

#define QRES_SID_NULL ((qres_sid_t)0) /**< Null server identifier		*/

/** Boolean type.
 * 
 * This is mainly used for the purpose of clarifying that the value
 * may be used within an if statement condition, and that boolean
 * operators should be used in place of bitwise ones, when logically
 * combining multiple values of this kind.
 */
typedef int qos_bool_t;

/** Identifies a thread together with the pid of the owning process
 * 
 * @note
 *   A value of 0, only allowed if coupled with a pid = 0, identifies
 *   the requesting thread in the operation.
 * 
 * @note
 *   This identifier may be retrieved by a thread through a gettid()
 *   syscall, and is completely different from the pthread_t value
 *   retrieved through pthreads API calls.
 */
typedef pid_t tid_t;

/** The type used for representing a CPU bandwidth.
 *
 * A CPU bandwidth is a fraction of usage of the processor, in the
 * Resource Reservation meaning, whose maximum value is 1.0. It is
 * represented as a fixed precision number.
 */
typedef unsigned long int qos_bw_t;

/** Format string to be used for qos_bw_t types in printf-like functions */
#define QOS_BW_FMT "%lu"

/** Precision of representation of a bandwidth value.
 *
 * This is the number of bits used to represent a bandwidth value in
 * the range [0.0, 1.0].
 */
#define QOS_BW_BITS 24

/** Corresponds to maximum CPU usage (1.0).
 *
 * This is a theoretical value, never assigned to any task in practice.	*/
#define MAX_BW (1ul << QOS_BW_BITS)

/** Maximum utilizable bandwidth. This may be less than
 ** one in order to account for scheduling overhead
 **/
#define U_LUB r2bw_c(RRES_U_LUB, 100)

#ifndef QOS_KS
/** Convert a double in the range [0..1] to a qos_bw_t.
 *
 * This function should not be used inside the kernel.
 */
static inline qos_bw_t d2bw(double b) {
  return (qos_bw_t) (b * (double) MAX_BW);
}

/** Warning: inside kernel, use exclusively with constant parameters */
#define md2bw(b) ((qos_bw_t) (b * (double) MAX_BW))

/** Convert a qos_bw_t to a double in the range [0..1].
 *
 * This function should not be used inside the kernel.
 */
static inline double bw2d(qos_bw_t b) {
  return ((double) (b)) / ((double) MAX_BW);
}
#endif

/** Converts a reservation (Q,P) into the ratio Q/P represented as
 * a qos_bw_t value.
 */
static inline qos_bw_t r2bw(qres_time_t Q, qres_time_t P) {
  return ul_shl_div((unsigned long) Q, QOS_BW_BITS, (unsigned long) P);
}

/** Convert a reservation (Q,P) into the ratio Q/P represented as
 ** a qos_bw_t value, upper-rounding the result.
 **/
static inline qos_bw_t r2bw_ceil(qres_time_t Q, qres_time_t P) {
  return ul_shl_ceil((unsigned long) Q, QOS_BW_BITS, (unsigned long) P);
}

static inline unsigned long div_by_bw(unsigned long num, qos_bw_t bw) {
  return ul_shl_div(num, QOS_BW_BITS, bw);
}

static inline unsigned long mul_by_bw(unsigned long value, qos_bw_t bw) {
  return ul_mul_shr(value, bw, QOS_BW_BITS);
}

/** Converts a reservation (Q,P) into the ratio Q/P represented as
 * a qos_bw_t value. This macro is more convenient than r2bw() for initializing
 * constants, but has fewer parameter checks and is more inefficient.
 */
#define r2bw_c(Q, P) ((qos_bw_t) ((((unsigned long long) (Q)) << QOS_BW_BITS) / (P)))

#include "qos_debug.h"

/** Computes the budget Q of a reservation (Q, P) from the server
 * period P and the ratio Q/P represented as a qos_bw_t value.
 */
static inline qres_time_t bw2Q(qos_bw_t bw, qres_time_t P) {
  /* bw=Q*MAX_BW/P ==> Q=bw*P/MAX_BW */
  //return ((unsigned long) bw) * ((unsigned long) P) >> QOS_BW_BITS;
/*    unsigned long Q = ul_mul_shr(bw, P, QOS_BW_BITS); */

/*   return Q; */
  return ul_mul_shr(bw, P, QOS_BW_BITS);
}

/** The default P value used when not specified */
#define QRES_DEF_P 100000

/** The server to create is the default one.
 **
 ** @note
 ** Only one default server may exist into the system, and must be created by root.
 **/
#define QOS_F_DEFAULT   0x00000001

/** Server tasks are scheduled within the reservation, and also by Linux outside,
 ** when budget is exhausted.
 **
 ** @note
 ** Supervisor configuration may allow or deny the requesting user to use this flag.
 **/
#define QOS_F_SOFT	 0x00000002
#define QOS_F_NOMULTI	 0x00000004  /**< Do not allow more tasks in the new server     */
#define QOS_F_PERSISTENT 0x00000008  /**< Empty server survives until a maximum timeout */

#endif /* __QOS_TYPES_H__ */
