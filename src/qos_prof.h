#ifndef __QOS_PROF_H__
#define __QOS_PROF_H__

/** @file
 *
 * @brief Function-level profiling utilities.
 *
 * This file contains utilities for function-level profiling in
 * either user-space or kernel-space. For each profiled function,
 * the number of invocations and the total time spent inside function
 * code (comprising called functions) is collected.
 *
 * The prof_dump() function may be used to dump a report on the
 * profiled functions. When in US, this info goes to stderr, when
 * in KS, it goes to printk or klogger, depending on configuration.
 *
 * In order to enable profiling, the QOS_PROFILE macro must be defined.
 *
 * Use of this module prescribes to add, for each function to be profiled,
 * the var declaration statement prof_vars, a call to prof_func() at the
 * begin of the function, and exiting from the function through prof_return().
 *
 * Furthermore, profiled code needs to be linked with qos_profile.c.
 *
 * The typical sample use is as follows:
 *
 * @code
 *
 *   void f(int x) {
 *     ...
 *     prof_vars;
 *     ...
 *     prof_func();
 *     ....
 *     if (! condition)
 *       prof_return();
 *     ...
 *     prof_return();
 *   }
 *
 *   int g(int x) {
 *     ...
 *     prof_vars;
 *     ...
 *     prof_func();
 *     ....
 *     if (! condition)
 *       prof_return(-1);
 *     ...
 *     prof_return(0);
 *   }
 * @endcode
 */

//#include <linux/aquosa/qos_debug.h>
#include "qos_debug.h"

#ifdef QOS_KS
//#  include <linux/autoconf.h>
#  include <linux/kernel.h>
#  include <linux/version.h>
#  include <linux/module.h>
#  include <linux/string.h>
#  include "rres_time.h"
#else
#  include <string.h>
#endif

/** Maximum number of functions that can be profiled.
 *
 * Memory usage for profile data is statically fixed to
 * QOS_PROF_MAX_COUNTS * sizeof(qos_profile_t)
 */
#define QOS_PROF_MAX_COUNTS 128

/** Maximum length for the name of a profiled function, including string terminator */
#define QOS_PROF_NAME_SIZE 32

/** Profile data that is allocated for each profiled function	*/
typedef struct {
  unsigned long int counter;
  unsigned long long time;
  char func_name[QOS_PROF_NAME_SIZE];
} qos_profile_t;

#ifdef QOS_PROFILE

/** Array of profile_data_t */
extern qos_profile_t qos_prof_data[QOS_PROF_MAX_COUNTS];

/** Number of used entries in the qos_prof_data[] array */
extern int qos_prof_num;

/** Dump collected profile data */
void prof_dump(void);

#else

static inline void prof_dump(void) { }

#endif

static inline qos_profile_t * prof_register(const char *name) {
#ifdef QOS_PROFILE
  qos_profile_t * p;
  if (qos_prof_num == QOS_PROF_MAX_COUNTS) {
    qos_log_crit("Profile slots exhausted, so I'm rewriting last one. Please, increment QOS_PROF_MAX_COUNTS.");
    qos_prof_num--;
  }
  p = qos_prof_data + qos_prof_num;
  qos_prof_num++;
  strncpy(p->func_name, name, sizeof(p->func_name));
  p->counter = 0;
  p->time = 0;
  return p;
#else
  return 0;
#endif
}

/** This must be added in the var declaration section of a profiled function */
#ifdef QOS_PROFILE
#  define prof_vars				\
     unsigned long long _prof_clk1, _prof_clk2;	\
     static qos_profile_t *_p_prof = 0
#else
#  define prof_vars
#endif

/* @todo read kstat.context_swtch in prof_func() and verify if is the same at prof_end/return()
 * in this case, discard misured value and decrement _prof_counter
 * reading time and kstat.context_switch must be done atomically 
 * kstat variable must be exported by scheduler */

/** This must be the first statement of a profiled function		*/
#ifdef QOS_PROFILE
#define prof_func() do {		\
  if (_p_prof == 0)			\
    _p_prof = prof_register(__func__);	\
  _prof_clk1 = sched_read_clock();	\
} while (0)
#else
#define prof_func()
#endif

#ifdef QOS_PROFILE
/** Update profiling variables: used only for internal pourpose */
#define _update_prof_var() do {		\
  _prof_clk2 = sched_read_clock();	\
  _p_prof->counter++;			\
  _p_prof->time += _prof_clk2 - _prof_clk1; \
} while (0)

/** This must be used in order to return from a profiled function	*/
#define prof_return(args...) do {	\
  _update_prof_var();			\
  return args;				\
} while (0)

/** This must be used when you cannot use prof_return (in case you want to trace a critical section)	*/
#define prof_end() do {	\
  _update_prof_var();			\
} while (0)
#define prof_call(args...) do { prof_func(); args; prof_end(); } while(0)
#else
#define prof_return(args...) do { return args; } while (0)
#define prof_end() do { } while (0)
#define prof_call(args...) do { } while (0)
#endif

#endif
