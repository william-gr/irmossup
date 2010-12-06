#ifndef __QOS_DEBUG_H__
#define __QOS_DEBUG_H__

/** @file
 *
 * @brief Space-Independent debugging functions.
 *
 * Most macros in this file may be called for logging or debugging
 * both from kernel-space and from user-space. They automatically
 * log messages with printk or fprintf(stderr, ...).
 *
 */

/************************************************************
 *                      ERROR CODES                         *
 ************************************************************/

/** @name QoS Library success and error return values
 * 
 * @{
 */

/** The QoS Library return value, either QOS_OK or one of the QOS_E_error codes.
 * 
 *  It may be converted to/from an integer through the qos_rv_int() and qos_int_rv() macros.
 *  
 *  @see qos_rv_int qos_int_rv
 * 
 *  @note Defined as a pointer in order to generate compile-time warnings in case
 *  of assignment with unallowed values (e.g. integers).
 */
typedef struct __qos_rv { } *qos_rv;

#define qos_rv_int(rv) ((int) (long) (rv))
#define qos_int_rv(err) ((qos_rv) (long) err)

  /** Operation successfully completed		*/
#define QOS_OK			qos_int_rv(0)
  /** Unspecified generic error			*/
#define QOS_E_GENERIC		qos_int_rv(-16)
  /** Memory insufficient for the operation	*/
#define QOS_E_NO_MEMORY	qos_int_rv(-17)
  /** Invalid input parameter(s)		*/
#define QOS_E_INVALID_PARAM	qos_int_rv(-18)
  /** Operation has not been authorized according
   * to the configured security policy		*/
#define QOS_E_UNAUTHORIZED	qos_int_rv(-19)
  /** Operation is not (yet) implemented	*/
#define QOS_E_UNIMPLEMENTED	qos_int_rv(-20)
  /** Dynamic loadable component not found, e.g. kernel module	*/
#define QOS_E_MISSING_COMPONENT qos_int_rv(-21)
  /** Internal state inconsistent with operation		*/
#define QOS_E_INCONSISTENT_STATE qos_int_rv(-22)
  /** Request rejected due to a temporary system overload	*/
#define QOS_E_SYSTEM_OVERLOAD	qos_int_rv(-23)
  /** Internal error: report to authors, please			*/
#define QOS_E_INTERNAL_ERROR	qos_int_rv(-24)
  /** Operation could not be completed because the reservation server could not be found */
#define QOS_E_NOT_FOUND		qos_int_rv(-25)
  /** Operation could not be completed because the object was "full" */
#define QOS_E_FULL		qos_int_rv(-26)
  /** Operation could not be completed because the object was "empty" */
#define QOS_E_EMPTY		qos_int_rv(-27)

/** Here for backward compatibility, will be removed in future releases.
 ** @deprecated
 **/
#define QOS_E_NOSERVER QOS_E_NOT_FOUND

/** @} */
#ifdef QOS_KS
//#  include <linux/autoconf.h>
#  include <linux/kernel.h>
#  include <linux/version.h>
#  include <linux/module.h>
#  include <asm/atomic.h>
#  include "kal_time.h"
#  define QOS_FFLUSH
#  ifdef __LOGGER__
#    include "logger.h"
#    define debug_print log_printf
#    define DBG_DEV 0,
#    define DBG_LEV 
#  else
#    define debug_print printk
#    define DBG_LEV KERN_DEBUG
#    define DBG_DEV
#  endif
#  define QOS_DEBUG_WITH_TIME
#  ifdef QOS_DEBUG_WITH_TIME
     extern kal_time_t sched_time;
#    define DBG_TIME_FORMAT KAL_TIME_FMT
#    define DBG_TIME KAL_TIME_FMT_ARG(kal_time_now()),
#  else
#    define DBG_TIME_FORMAT
#    define DBG_TIME
#  endif
#else
#  include <stdio.h>
#  include <unistd.h>
#  include <stdlib.h>
#  define debug_print fprintf
#  define DBG_LEV 
#  define DBG_DEV stderr,
#  define QOS_FFLUSH fflush(stderr)
#  define DBG_TIME_FORMAT
#  define DBG_TIME

/* TODO: replace with gcc built-in atomic memory operations (__sync_xxx) */
#  define atomic_t int
#  define ATOMIC_INIT(x) (x)
#  define atomic_inc_return(p) (++(*(p)))
#endif

#ifndef QOS_GLOBAL_LEVEL
#  define QOS_GLOBAL_LEVEL QOS_LEVEL_DEBUG
#endif

/** This is used to detect loss of messages */
extern atomic_t qos_log_msg_id;

/** This is used to highlight run-time nesting level of functions
 ** logging messages, through appropriate indentation
 **/
extern int indent_lev;

#define MAX_INDENT_LEVEL 32
extern char * func_names[MAX_INDENT_LEVEL];

void qos_dump_stack(void);

/** Convert a QOS_E_* error code into a string.
 *
 * Returns a pointer to a statically allocated string with a human
 * readable description of the error coded into the err parameter.
 */
char *qos_strerror(qos_rv err);

#define _min(a, b) ((a) < (b) ? (a) : (b))
#define _max(a, b) ((a) > (b) ? (a) : (b))

#define pad "+ + + + + + + + + + + + + + + + "

/** For internal use only	*/
#define qos_log_str(level,str,msg,args...)			\
do {								\
  int __level = (level);					\
  if (__level <= QOS_DEBUG_LEVEL && __level <= QOS_GLOBAL_LEVEL) {  \
    debug_print(DBG_LEV DBG_DEV DBG_TIME_FORMAT				\
		"[%6d]" "[%.24s%*s]" str " %.*s" msg "\n",	\
		DBG_TIME					\
		atomic_inc_return(&qos_log_msg_id), __func__,	\
		24-_min(24, (int)sizeof(__func__)-1), "",	\
		2*indent_lev, pad, ##args);			\
    QOS_FFLUSH;							\
  }								\
} while (0)

//    debug_print(DBG_LEV DBG_DEV DBG_TIME_FORMAT "[%6d][%*s%.*s%*s] " str msg "\n", DBG_TIME qos_log_msg_id, _min(2*indent_lev, 24), "", 24-_min(2*indent_lev, 24), __func__, 24-_min(2*indent_lev+sizeof(__func__)-1, 24), "", ##args);

/** Log a message without function name and extra information */
#define qos_log_simple(level,msg,args...) \
do {            \
  if ((level) <= QOS_DEBUG_LEVEL) {   \
    debug_print(DBG_LEV DBG_DEV msg "\n", ##args);  \
    QOS_FFLUSH;         \
  }           \
} while (0)

/** No debug: no log messages are generated, except the ones
 ** generated through qos_log_crit().
 **/
#define QOS_LEVEL_NODEBUG 0

/** Error messages: always reported if QOS_DEBUG_LEVEL >= 1
 **
 ** Used to report an unexpected condition that is at risk of
 ** compromising component functionality.
 **/
#define QOS_LEVEL_ERROR 1

/** Warning messages: reported only if QOS_DEBUG_LEVEL >= 2
 **
 ** Used to report a condition that does not compromise a component
 ** functionality, but may possibly be a symptom of strange behaviours.
 **/
#define QOS_LEVEL_WARN 2

/** Information messages: reported only if QOS_DEBUG_LEVEL >= 3
 **
 ** Used to report useful information for the users, for example
 ** to report where and why specifically a failure occurred, in addition
 ** to the user-level error code that comes as a consequence.
 **/
#define QOS_LEVEL_INFO 3

/** Debug messages: reported only if QOS_DEBUG_LEVEL >= 4
 **
 ** Report debugging information useful for components developers themselves.
 **/
#define QOS_LEVEL_DEBUG 4

/** Verbose messages: reported only if QOS_DEBUG_LEVEL >= 8
 **
 ** Report pedantic debugging information.
 **/
#define QOS_LEVEL_VERB 8

#ifndef QOS_DEBUG_LEVEL
/** QOS_DEBUG_LEVEL sets the verbosity level of debug messages
 * (see QOS_DEBUG_xx constants for details).
 *
 * QOS_DEBUG_LEVEL is set by default to 1 (warnings and errors)
 */
#  define QOS_DEBUG_LEVEL QOS_LEVEL_WARN
#endif /* QOS_DEBUG_LEVEL */

/** Log a message with a specified level	*/
#define qos_log(level,msg,args...) qos_log_str(level,"",msg,##args)

/** Log an error message	*/
#define qos_log_err(msg,args...) qos_log_str(QOS_LEVEL_ERROR,   "<ERR> ",msg,##args)

/** Log a warning message	*/
#define qos_log_warn(msg,args...) qos_log_str(QOS_LEVEL_WARN,  "<WRN> ",msg,##args)

/** Log an informative message	*/
#define qos_log_info(msg,args...) qos_log_str(QOS_LEVEL_INFO,  "<INF> ",msg,##args)

/** Log a debugging message	*/
#define qos_log_debug(msg,args...) qos_log_str(QOS_LEVEL_DEBUG, "<DBG> ",msg,##args)

/** Log a verbose debugging message     */
#define qos_log_verb(msg,args...) qos_log_str(QOS_LEVEL_VERB, "<VRB> ",msg,##args)

/** Log a variable. 
 *  Example: to print the long int variable "my_long_int" as a debug message  -> qos_log_var(QOS_LEVEL_DEBUG, my_long_int, "%ld") */
#define qos_log_var(level, __var_name, __var_type) qos_log(level, #__var_name ": " __var_type, __var_name)

/** Log a critical error: always reported.	*/
#define qos_log_crit(msg,args...)			\
do {							\
  debug_print(DBG_LEV DBG_DEV "[%15s] <CRIT> " msg "\n", __func__ , ##args); \
  QOS_FFLUSH;						\
} while (0)

#ifdef QOS_KS
#  define DUMP_STACK do { dump_stack(); qos_dump_stack(); } while (0)
#else
#  define DUMP_STACK do { qos_dump_stack(); } while (0)
#endif

#define qos_chk_do_msg(cond, stmt, msg, args...) do {		\
    if (!(cond)) {						\
      qos_log_crit("ASSERTION '" #cond "' FAILED "			\
		   "at line %d of file %s: " msg, __LINE__, __FILE__, ##args); \
      stmt;								\
    }								\
} while (0)

#define qos_chk_do(cond, stmt) qos_chk_do_msg(cond, stmt, "")

/** Evaluate cond and log a custom message with args if it is not satisfied.
 *
 * Allows to specify a custom printf-like format message with arguments.
 * Condition is always evaluated, independently of debug settings.
 */
#define qos_chk_msg(cond, msg, args...) qos_chk_do_msg(cond, , msg, ##args)

/** Evaluate condition and log a warning message if it is not satisfied.
 *
 * Condition is always evaluated, independently of debug settings.
 */
#define qos_chk(cond) qos_chk_do(cond, )

/**
 * Evaluate condition, and, if unsatisfied, then log a warning message and return
 * from current function.
 *
 * Condition is always evaluated, independently of debug settings.
 */
#define qos_chk_rv(cond,rv) qos_chk_do(cond, return rv)

/** Evaluate cond. If it is satisfied, log a custom message with args and return.
 *
 * Allows to specify a custom printf-like format message with arguments.
 * Condition is always evaluated, independently of debug settings.
 */
#define qos_return_if_cond(cond,rv,msg,args...) do {      \
  if (cond) {            \
    qos_log_debug("Check failed: '" #cond      \
      "' at line %d of file %s. " msg "\n", __LINE__, __FILE__,##args); \
    return rv;    \
  }               \
} while (0)

#define qos_chk_ok_do(expr,stmt) do {                                   \
  qos_rv __rv = (expr);                                                 \
  if (__rv != QOS_OK) {                                                 \
    qos_log_crit("ASSERTION FAILED: '" #expr " = %s' at line %d of file %s.",\
             qos_strerror(__rv), __LINE__, __FILE__);                   \
    DUMP_STACK;								\
    stmt;                                                               \
  }                                                                     \
} while (0)

/** Evaluate expr and log a warning message with stringified
 *  error if it does not evaluate to QOS_OK.
 *
 * Expression is always evaluated, independently of debug settings.
 */
#define qos_chk_ok(expr) qos_chk_ok_do(expr, )

/** Evaluate expr once and, if it does not evaluate to QOS_OK, then log a
 * warning message with the stringified error and return it to the caller.
 *
 * Expression is always evaluated, independently of debug settings.
 */
#define qos_chk_ok_ret(expr) do {		\
  qos_rv __rv = (expr);				\
  if (__rv != QOS_OK) {				\
    qos_log_crit("Check failed: '" #expr " = %s' at line %d of file %s.",\
       qos_strerror(__rv), __LINE__, __FILE__);	\
    DUMP_STACK;					\
    return __rv;				\
  }						\
} while (0)

/** Evaluate cond and, if it is not satisfied, log a warning and go to label lab.
 *
 * Allows to specify a custom printf-like format message with arguments.
 * Condition is always evaluated, independently of debug settings.
 */
#define qos_chk_go(cond,lab) qos_chk_do(cond,goto lab)

#define qos_chk_go_msg(cond,lab,msg,args...) qos_chk_do_msg(cond,goto lab,msg,##args)

/** Check if debug is enabled for (at least) the specified level	*/
#define qos_debug_enabled_for(lev) (QOS_DEBUG_LEVEL >= (lev))

#ifdef QOS_KS

#else	/* QOS_KS */

/** Evaluate cond and cause exit(-1) if it is not satisfied.
 *
 * Only available in user-space.
 */
#define qos_chk_exit(cond) do {						\
  int ok = (cond);							\
  if (! ok) {								\
    qos_log_err("ASSERTION FAILED: '" #cond "' at line %d of file %s.",\
	     __LINE__, __FILE__);					\
    exit(-1);								\
  }									\
} while (0)

/** Evaluate expr and, if it does not evaluate to QOS_OK, prints the
 *  stringified error and causes exit(-1).
 *
 * Only available in user-space */
#define qos_chk_ok_exit(expr) qos_chk_ok_do(expr, exit(-1))

#endif	/* QOS_KS */

#endif /* __QOS_DEBUG_H__ */
