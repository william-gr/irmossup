/** @defgroup QRES QoS Resource Reservation
 **/

/** @addtogroup QRES_MOD QoS Resource Reservation Kernel Module
 **
 ** @ingroup QRES
 **
 ** Provides high-level Resource Reservation (RR) scheduling services.
 **
 ** Applications make RR requests through the library API defined in the
 ** @ref QRES_LIB interface. All bandwidth requests are mediated
 ** by the @ref QSUP_MOD module.
 ** @{
 **/

/** @file
 ** @brief	QRES public interface for use by other kernel modules.
 **/

#ifndef __QRES_INTERFACE_H__
#define __QRES_INTERFACE_H__

#include "qres_gw.h"
#include "qsup.h"
#include "qos_debug.h"
#include "rres_interface.h"
#include "rres_kpi_protected.h"

/** Main QRES Server struct, conceptually extends the RRES Server struct (in a OO fashion).
 **
 ** The QRES Server adds to resource servers of the RRES Module capabilities related to:
 ** <ul>
 **   <li>interfacing with the applications through the QRES and QSUP libraries;
 **   <li>admission control, through the QSUP Module functionality;
 **   <li>access control, remembering the uid/gid of the server creator (its owner),
 **       and comparing them with the uid/gid requesting any further operation on the
 **       server (root can do everything on any server, owner can do everything on its
 **       servers, all other users can only read server information/parameters.
 ** </ul>
 **/
typedef struct qres_server {
  server_t rres;        /**< Parent class istance       **/
#ifdef QRES_ENABLE_QSUP
  qsup_server_t qsup;   /**< Supervisor related info    **/
#endif
  qres_params_t params; /**< Parameters                 **/
  kal_uid_t owner_uid;  /**< UID of this server owner   **/
  kal_gid_t owner_gid;  /**< GID of this server owner   **/
} qres_server_t;

static inline qres_server_t *qres_find_by_rres(server_t *srv) {
  qres_server_t *qres = (qres_server_t *) NULL;
  unsigned long rres_offset = ((unsigned long) &qres->rres) - ((unsigned long) qres);
  qos_chk((qres_server_t *) (((unsigned long) srv) - rres_offset) == (qres_server_t *) srv);
  return (qres_server_t *) (((unsigned long) srv) - rres_offset);
}

/*
 * Synchronization management
 */

/** Return a spinlock_t suitable for synchronizing against concurrent
 ** requests made through the QRES device interface.
 **
 ** Before calling any of the functions defined in qres_interface.h,
 ** the caller should have gained a lock on the spinlock_t variable
 ** returned by this function. This is usually accomplished by using
 ** the qres_lock_irqsave and qres_unlock_irqrestore macros. Though,
 ** for more complex cases, this function may be useful as well.
 **/
static inline kal_lock_t *qres_get_lock(void) {
  return rres_get_spinlock();
}

/*
 * The QRES kernel API functions.
 */

/** Perform QoS Res/Sup initialization	*/
qos_rv qres_init(void);

/** Perform QoS Res and Sup cleanup	*/
qos_rv qres_cleanup(void);

/** Create a new server for the specified task with provided parameters.
 * 
 * Return the new server identifier in *p_sid
 */
qos_rv qres_create_server(qres_params_t *param, qres_sid_t *p_sid);

qos_rv qres_init_server(qres_server_t *srv, qres_params_t *param);

/** Detach all tasks from from the specified server, and destroy
 ** the server
 **/
qos_rv qres_destroy_server(qres_server_t *srv);

/** Virtual destructor override **/
qos_rv _qres_cleanup_server(server_t *srv);

/** Attach to the server identified by srv_id the task identified by tsk */
qos_rv qres_attach_task(qres_server_t *qres, struct task_struct *tsk);

/** Detach the specified task from its server and, if
 ** no other tasks reside therein, destroy the server.
 **/
qos_rv qres_detach_task(qres_server_t *qres, struct task_struct *tsk);

/** Change scheduling parameters of the server to which
 ** the specified task is attached				*/
qos_rv qres_set_params(qres_server_t *qres, qres_params_t *param);

/** Get the scheduling parameters of the server attached to
 ** the specified task.
 **
 ** @note
 **   Returned budget values may be slightly higher than the ones
 **   set through qres_init_server() or qres_set_params(), because
 **   any granted (budget, period) pair corresponds to a qos_bw_t
 **   value (fixed-point representation of the ratio budget/period).
 **/
qos_rv qres_get_params(qres_server_t *srv, qres_params_t *params);

/** This is used by QMGR kernel mod, too */
qos_rv qres_get_exec_abs_time(
  qres_server_t *qres, qres_time_t *exec_time, qres_atime_t *abs_time
);

/** Retrieve the remaining budget for the current server instance */
qres_time_t qres_get_curr_budget(qres_server_t *qres);

/** Retrieve the budget to be used for the very next server instance */
qres_time_t qres_get_next_budget(qres_server_t *qres);

/** Retrieve the budget approved for the subsequent server instances.
 **
 ** Whenever a change of budget is requested, either as a result of
 ** a qres_set_params() or as a result of the creation of a new server,
 ** each server that has been approved a budget increase from the
 ** supervisor may undergo (if such increase in not immediately and
 ** entirely available) a transitory period during which the current
 ** budget, as returned by qres_get_curr_budget(), may change multiple
 ** times, until it reaches the approved value.
 **/
qres_time_t qres_get_appr_budget(qres_server_t *qres);

/** Retrieve the current server deadline */
qos_rv qres_get_deadline(qres_server_t *qres, struct timespec *p_deadline);

#ifdef QRES_ENABLE_QSUP
/** Retrieve the qsup_server_t instance attached to a RR server	*/
static inline qsup_server_t * qres_get_qsup(qres_server_t *qres) {
  return &qres->qsup;
}
#endif

/** Retrieve the owner UID for the current server.
 * 
 * This field is set automatically by the QRES module, whenever a process,
 * creates a server, to the process' euid.
 */
kal_uid_t qres_get_owner_uid(qres_server_t *qres);

/** Retrieve the owner GID for the current server.
 * 
 * This field is set automatically by the QRES module, whenever a process,
 * creates a server, to the process' egid.
 */
kal_gid_t qres_get_owner_gid(qres_server_t *qres);

static inline qres_server_t * qres_find_by_id(qres_sid_t sid) {
  //return qres_find_by_rres(rres_find_by_id(sid));
  return NULL;
}

/** @} */

#endif
