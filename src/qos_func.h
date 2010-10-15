#ifndef QOS_FUNC_H_
#define QOS_FUNC_H_

#include "qos_debug.h"
#include "qos_prof.h"

/* gcc 3.x might have bugs in compiling __builtin functions */
#ifdef QOS_FUNC_WRAPPERS

#ifndef QOS_FUNC_MAX_ARGS_SIZE
/** The default maximum size (in bytes) of functions defined through OML wrappers.
 **
 ** Each wrapped function will require, at runtime, at least QOS_FUNC_MAX_ARGS_SIZE
 ** bytes in the stack for the call (if debugging is enabled). Therefore, override
 ** this value to greater values only when actually needed.
 **
 ** @note
 ** This value may be overridden by defining this macro to something
 ** different than 64 before inclusion of qos_func.h.
 **/
#  define QOS_FUNC_MAX_ARGS_SIZE 64
#endif

typedef void (* __void_func)(void);

/** @note
 ** This works only with functions that require less than
 ** QOS_FUNC_MAX_ARGS_SIZE bytes in the stack.
 ** for passing arguments.
 **/
#define qos_func_define(rv, name, params...)    \
  rv name(params);                              \
  static rv name##__(params);                   \
  rv name(params) {                             \
    void *__func_args, *__func_rv;              \
    qos_log_debug("Entering function: " #name); \
    if (indent_lev < MAX_INDENT_LEVEL)		\
      func_names[indent_lev] = #name;		\
    indent_lev++;                               \
    __func_args = __builtin_apply_args();       \
    __func_rv = __builtin_apply((__void_func) name##__, __func_args, QOS_FUNC_MAX_ARGS_SIZE); \
    indent_lev--;                               \
    qos_log_debug("Leaving function: " #name);  \
    __builtin_return(__func_rv);                \
  }                                             \
  static rv name##__(params) 

#define qos_func_define_prof(rv, name, params...) \
  static rv name##__(params);                   \
  rv name(params) {                             \
    void *__func_args, *__func_rv;              \
    prof_vars;                                  \
    qos_log_debug("Entering function: " #name); \
    if (indent_lev < MAX_INDENT_LEVEL)		\
      func_names[indent_lev] = #name;		\
    indent_lev++;                               \
    __func_args = __builtin_apply_args();       \
    prof_func();                                \
    __func_rv = __builtin_apply((__void_func) name##__, __func_args, QOS_FUNC_MAX_ARGS_SIZE); \
    prof_end();                                 \
    indent_lev--;                               \
    qos_log_debug("Leaving function: " #name);  \
    __builtin_return(__func_rv);                \
  }                                             \
  static rv name##__(params) 

#define qos_trblock(rv, name)                   \
  rv name(void);                                \
  static rv name##__(void);                     \
  rv name(void) {                               \
    void *__func_args, *__func_rv;              \
    qos_log_debug("Entering function: " #name); \
    if (indent_lev < MAX_INDENT_LEVEL)		\
      func_names[indent_lev] = #name;		\
    indent_lev++;                               \
    __func_args = __builtin_apply_args();       \
    __func_rv = __builtin_apply((__void_func) name##__, __func_args, 0); \
    indent_lev--;                               \
    qos_log_debug("Leaving function: " #name);  \
    __builtin_return(__func_rv);                \
  }                                             \
  static rv name##__(void) 

#else

#define qos_func_define(rv, name, params...) \
  rv name(params)

#define qos_func_define_prof(rv, name, params...) \
  rv name(params)

#endif

#endif /*QOS_FUNC_H_*/
