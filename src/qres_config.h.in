/** @file
 ** @brief Configuration file for QRES module
 **/

#ifndef __QRES_CONFIG_H__
#  define __QRES_CONFIG_H__

#define QRES_LIB_DEBUG_LEVEL QOS_LEVEL_INFO
#define QRES_MOD_DEBUG_LEVEL QOS_LEVEL_INFO
#define QSUP_MOD_DEBUG_LEVEL QOS_LEVEL_INFO

/** Trace time needed for various kernel operations.
 * Trace only 1 feature at once, as tracing in called
 * functions alters execution time of calling ones
 * (enable hex trace fmt to mitigate this effect)
 */
#undef PROF_QRES_MOD_DEV
#undef PROF_QRES_MOD
#undef PROF_QSUP_MOD

/** Warning: this will slow down memory operations	*/
#undef QOS_MEMORY_CHECK

/** Set the path to the folder containing device files	*/
#define QOS_DEV_PATH "/dev"

/** Enable profiling through qos_prof.h interface	*/
#undef QOS_PROFILE

/** Enable filtering of bandwidth requests by the QSUP **/
#define QRES_ENABLE_QSUP

#define MODPARM_DEFAULT_SRV_Q 10000L	/**< Max budget of default server */
#define MODPARM_DEFAULT_SRV_Q_min 10000L  /**< Guaranteed budget for default server */
#define MODPARM_DEFAULT_SRV_P 30000L	/**< Period of default server */

/** support for proc filesystem */
#define CONFIG_OC_QRES_PROC

/** Enable automatic expansion of bandwidths up to allowed
 ** maximum per-user (i.e., emulate dynamic reclamation).
 **/
#undef QSUP_EXPAND

/** Enable dynamically reclaim a server budget whenever
 ** all of the attached tasks are blocked. The reclaimed
 ** budget is distributed by the supervisor to other
 ** competing active servers.
 ** @note
 ** Enabling this option, the wake-up latencies of reserved
 ** tasks may be negatively affected.
 **/
#undef QSUP_DYNAMIC_RECLAIM

#endif /* __QRES_CONFIG_H__ */
