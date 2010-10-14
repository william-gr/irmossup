/** @file
 ** @brief Configuration file for RRES module
 **/

#ifndef __RRES_CONFIG_H__
#  define __RRES_CONFIG_H__

/* NOTE: select only one of the following options */

/** Enable heap-ordered deadlines */
#undef RRES_USE_HEAP

/* NOTE: select only one dispatching mechanism or none for the default */

/** Enable activate_task_local()/deactivate_task_local() based dispatching mechanism. **/
#define RRES_DISPATCH_NEW

/** Enable signal-based SIGSTOP/SIGCONT dispatching mechanism. **/
/* #define RRES_DISPATCH_SIG */

/** Real-Time scheduling priority when dispatching AQuoSA tasks **/
#define RRES_DISPATCH_PRIORITY 50

/** Disable activation counting in servers, along with block/stop hook pairs **/
#undef RRES_DISABLE_ACTIVATIONS

/** support for the creation of a default server */
#define CONFIG_RRES_DEFAULT_SRV

/** support for proc filesystem */
#define CONFIG_OC_RRES_PROC

/** Debugging level for RRES */
#define RRES_MOD_DEBUG_LEVEL QOS_LEVEL_INFO

/** Embedded logger (NOT YET SUPPORTED) */
#undef CONFIG_OC_RRES_LOGGER
/** LOGGER include files directory */
#define CONFIG_OC_RRES_LOGGER_DIR "/usr/local/logger-1.0"

/** Enable profiling globally	*/
#undef QOS_PROFILE
/** Enable profiling of hooks	*/
#undef PROF_RRES_HOOKS

/** Enable old kernel gensched patch */
#undef __AQUOSA_OLD_PATCH

/** Enable wrappers for functions defined with qos_func.h macros */
#undef QOS_FUNC_WRAPPERS

/** Enable use of hrtimers **/
#undef KAL_USE_HRTIMER

/** Enable memory checks in qos_memory.c **/
#undef QOS_MEMORY_CHECK

/** Set to 1 in order to enable paranoid checks.
 **
 ** These are further checks of parameters and assert conditions that
 ** should not be needed, if all of the components behaved as expected.
 **/
#define RRES_PARANOID 0

/** Enable SHRUB reclaiming */
#undef SHRUB

/** Utilization limit for all AQuoSA tasks, as integer < 100 **/
#define RRES_U_LUB 95

/** Instantaneous budget increase in qres_set_params(), if possible **/
#undef RRES_INSTANT_SETPARAMS

#endif /* __RRES_CONFIG_H__ */
