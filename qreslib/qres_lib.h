#ifndef __QRES_LIB_H__
#define __QRES_LIB_H__

#include "qos_types.h"
#include "qos_debug.h"

#include "qres_gw.h"

/** @addtogroup QRES_LIB QoS Resource Reservation Library
 *
 * @ingroup QRES
 *
 * Allows applications to take advantage of Resource Reservation scheduling
 * services available within the kernel when the @ref QRES_MOD module is
 * loaded.
 * @{
 */

/** @file
 * @brief QRES Library Application Programming Interface
 *
 * @todo  Check status of library/QRESMod inside each function
 * @todo  qres library level calls take pid_t;
 *        qres supervisor-filtered calls take struct task_struct
 *        (they use task info in order to enforce qossup policy);
 *        qres lowest level calls (cbs) take struct cbs_struct;
 *
 * @todo  Supervisor (Tom)
 *        Invariant-based Ctrl and Pred (Tom)
 *        Translation of qosres to IRIS (Luca)
 *        Port to 2.4.27
 *        Test inside 2.4.27
 *        Overhead measurements
 */

/** Initializes the QoS RES library.
 *
 * Checks that the kernel supports QoS Management, then
 * initializes the QRES library.
 *
 * @return	QOS_OK if initialization was succesful
 *		QOS_E_MISSING_COMPONENT if kernel does not support QoS management
 */
qos_rv qres_init(void);

/** Cleanup resources associated to the QoS RES Library
 *
 * After this call, it is not possible to use any qres_xx() call */
qos_rv qres_cleanup(void);

/** @Note: in the following functions, if pid == 0 -> apply to itself */

/** Create a new server with specified parameters.
 *
 * Change scheduling policy of the specified process to benefit of QoS
 * support into the kernel with provided parameters.
 *
 * @param p_params
 *   If the QRES_F_PERSISTENT flag is set in p_params->flags, then
 *   the server is not automatically destroyed after detach of the
 *   last thread. Instead, it keeps existing, where further threads
 *   may be attached to it by using the a qres_sid_t value for
 *   identification.
 *
 * @param p_sid
 *   Pointer to a qres_sid_t variable that is filled with the server ID
 *   of the created server. This ID is needed to perform any further
 *   operation on the created server (attaching/detaching threads,
 *   getting or setting parameters, destroying the server).
 *
 * @note
 *   If the QRES_WATCHDOG has been enabled in the module configuration,
 *   then, if a persistent server remains empty (with no attached threads)
 *   for too much time, the system destroys it automatically. This is
 *   a precaution for avoiding persistence of "unreachable" servers
 *   in the system, due to programming bugs or application crashes.
 *
 * @todo
 *   Implement the QRES_WATCHDOG feature.
 */
qos_rv qres_create_server(qres_params_t * p_params, qres_sid_t *p_sid);

/** Attach a task to an already existing server.
 *
 * @param pid
 *   The process ID of the task to affect, or zero
 *   for the current process
 *
 * @param tid
 *   The thread ID of the thread to affect, or zero
 *   for the current thread (only valid if pid is zero as well)
 *
 * @param server_id
 *   The identifier of the server to which the task has to be attached
 */
qos_rv qres_attach_thread(qres_sid_t server_id, pid_t pid, tid_t tid);

/** Detach from the QoS Server and possibly destroy it.
 *
 * Detach the specified process from the QoS server and return the
 * scheduling policy to the standard default system scheduler behaviour. If no more
 * processes reside in the server, destroy resources associated to it.
 *
 * For parameters specification, please see qres_attach_to_server()
 *
 * @see qres_attach_to_server()
 */
qos_rv qres_detach_thread(qres_sid_t sid, pid_t pid, tid_t tid);

/** Detach all processes from a server and destroy it.
 *
 * Detach all attached threads from a QRES server, then destroy it. This
 * causes the QRES system to free any resources previously associated to
 * the destroyed server, and to the attached tasks, if any.
 *
 * @note
 *   After a successfull completition of this call, the specified
 *   server ID is not valid anymore. Though, it may be reused later by
 *   the system to identify a new server created subsequently through
 *   a qres_create_server() call.
 */
qos_rv qres_destroy_server(qres_sid_t sid);

/** Get a thread server id, or QOS_E_NOT_FOUND if it is not
 * attached to any server.
 */
qos_rv qres_get_sid(pid_t pid, tid_t tid, qres_sid_t *p_sid);

/** Change bandwidth			*/
qos_rv qres_set_bandwidth(qres_sid_t sid, qos_bw_t bw);

/** Retrieve QoS scheduling parameters.
 **
 ** The returned values may be slightly different
 ** than those set through qres_set_params().
 **
 ** @note
 **   Returned budget values may be slightly higher than the ones
 **   set through qres_init_server() or qres_set_params(), because
 **   any granted (budget, period) pair corresponds to a qos_bw_t
 **   value (fixed-point representation of the ratio budget/period).
 **/
qos_rv qres_get_params(qres_sid_t sid, qres_params_t *p_params);

/** Change QoS scheduling parameters  */
qos_rv qres_set_params(qres_sid_t sid, qres_params_t * p_params);

/** Get the bandwidth of the server with the supplied ID.	*/
qos_rv qres_get_bandwidth(qres_sid_t sid, float *bw);

/** Detect module in /proc/modules */
qos_rv qres_module_exists ();

/** Retrieve time info relative to the current server.
 *
 * @param exec_time
 *
 *	If non-null, returns the sum of the execution times of all the processes
 *	belonging to the same server as the one identified by the sid
 *	parameter, measured since the call to * qres_create_server()
 *
 * @param abs_time
 *
 *	If non-null, returns the absolute time, measured since some fixed point
 *	in the past (e.g. machine boot)
 *
 * @todo
 *
 *	specify better the time-reference of abs_time, so that it may
 *	be used together with standard timer-related POSIX calls from
 *	user-space
 *
 *
 * @return	The server execution time, of one of the
 *		QOS_E_ error codes if negative.
 */
qos_rv qres_get_exec_time(qres_sid_t sid, qres_time_t *exec_time, qres_atime_t *abs_time);

/** Retrieve remaining budget for the current server instance */
qos_rv qres_get_curr_budget(qres_sid_t sid, qres_time_t *curr_budget);

/** Retrieve the budget that is likely to be used for the very next server instance.
 **
 ** The value returned by this function may change asynchronously from, and in a way
 ** that depends on, the bandwidth changes requested on all the servers in the system.
 **/
qos_rv qres_get_next_budget(qres_sid_t sid, qres_time_t *next_budget);

/** Retrieve the budget that has been approved for the subsequent server instances
 **
 ** Whenever a change of budget is requested, either as a result of
 ** a qres_set_params() or as a result of the creation of a new server,
 ** each server that has been approved a budget increase from the
 ** supervisor may undergo (if such increase in not immediately and
 ** entirely available) a transitory period during which the current
 ** budget, as returned by qres_get_curr_budget(), may change multiple
 ** times, until it reaches the approved value.
 **/
qos_rv qres_get_appr_budget(qres_sid_t sid, qres_time_t *appr_budget);

/** Retrieve current scheduling deadline	*/
qos_rv qres_get_deadline(qres_sid_t sid, struct timespec *p_deadline);

/** Set scheduling weight (i.e., used by shrub) */
qos_rv qres_set_weight(qres_sid_t sid, unsigned int weight);

/** Retrieve scheduling weight (i.e., used by shrub) */
qos_rv qres_get_weight(qres_sid_t sid, unsigned int *p_weight);

/** Retrieve the existing servers
 ** @param sids
 **   A pre-allocated array supplied by the caller for storing the server ids
 ** @param p_num_sids
 **   An I/O parameter: the caller supplies the maximum number of server that
 **   can be stored in the sids[] array, the function returns the number of
 **   actually filled vector entries.
 **
 ** @note
 **   If the caller finds *p_num_sids untouched, and the function returned
 **   QOS_OK, then the number of existing servers may actually be higher.
 **/
qos_rv qres_get_servers(qres_sid_t *sids, size_t *p_num_sids);

/** @} */

#endif
