/** @file
 ** @brief Main QRES kernel module implementation file.
 **
 ** Handle all requests made on server through the QRES library.
 **
 ** @todo
 ** Add creation flag that, if set, allows to set required bandwidth to zero when
 ** the rres server has no activations (should be optional as it may introduce a
 ** latency in reclaiming back the given-away bandwidth when the server activates back).
 **/

#include "rres_config.h"
#include "qres_config.h"
#define QOS_DEBUG_LEVEL QRES_MOD_DEBUG_LEVEL
#include "qos_debug.h"

#ifdef QRES_MOD_PROFILE
#  define QOS_PROFILE
#endif
#include "qos_prof.h"

#include "qres_gw_ks.h"
#include "qsup_gw_ks.h"
#include "qres_kpi_protected.h"

#include "qos_types.h"
#include "qos_memory.h"
#include "qos_func.h"
#include "kal_sched.h"

#include "rres_ready_queue.h"
#include "rres_server.h"

/** Find the thread identified by the caller through <pid, tid>.
 **
 ** The caller thread may refer to itself as <0, 0>.
 **/
static qos_rv find_task(pid_t pid, tid_t tid, struct task_struct **p_ptsk) {
  qos_chk_do(kal_atomic(), return QOS_E_INTERNAL_ERROR);
  if (pid != 0 && tid == 0)
    return QOS_E_INVALID_PARAM;
  if ((tid == 0) || (tid == current->pid)) {
    *p_ptsk = current;
  } else {
    *p_ptsk = kal_find_task_by_pid(tid);
    if (*p_ptsk == 0)
      return QOS_E_INVALID_PARAM;
  }
  return QOS_OK;
}

///** Find the task identified by <pid, tid>, and check if affecting its reservation
// ** status would be legal or not.
// ** 
// ** @see #authorize_for_task(task_struct *)
// **/
//static qos_rv find_task_and_auth(pid_t pid, tid_t tid, struct task_struct **p_ptsk) {
//  qos_chk_ok_ret(find_task(pid, tid, p_ptsk));
//  qos_chk_rv(authorize_for_task(*p_ptsk), QOS_E_UNAUTHORIZED);
//  return QOS_OK;
//}

/** IOCTL helpers */

#define COPY_FROM_USER_TO(void_ptr, size, typed_ptr) do { \
  if ((size) != sizeof(*(typed_ptr))) {		\
    qos_log_crit("Got %lu bytes, expecting %zu", \
      (size), sizeof(*(typed_ptr)));		\
    return(QOS_E_INTERNAL_ERROR);		\
  }						\
  if (copy_from_user(typed_ptr, void_ptr, (size))) \
    return(QOS_E_INVALID_PARAM);		\
} while (0)

#define call_sync(func) ({					\
  kal_irq_state flags;						\
  qos_rv __rv;							\
  kal_spin_lock_irqsave(rres_get_spinlock(), &flags);		\
  __rv = (func);						\
  kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);	\
  __rv;								\
})

qos_func_define(qos_rv, qres_gw_destroy_server, qres_sid_t sid) {
  qres_server_t *qres = qres_find_by_id(sid);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;
  return qres_destroy_server(qres);
}

qos_func_define(qos_rv, qres_gw_attach_task, qres_attach_iparams_t *iparams) {
  qres_server_t *qres;
  struct task_struct *tsk;

  qos_chk_ok_ret(find_task(iparams->pid, iparams->tid, &tsk));
  qres = qres_find_by_id(iparams->server_id);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;
  return qres_attach_task(qres, tsk);
}

qos_func_define(qos_rv, qres_gw_detach_task, qres_attach_iparams_t *iparams) {
  qres_server_t *qres;
  struct task_struct *tsk;

  qos_chk_ok_ret(find_task(iparams->pid, iparams->tid, &tsk));
  qres = qres_find_by_id(iparams->server_id);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;
  return qres_detach_task(qres, tsk);
}

qos_func_define(qos_rv, qres_gw_set_params, qres_iparams_t *iparams) {
  qres_server_t *qres;

  qres = qres_find_by_id(iparams->server_id);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;
  return qres_set_params(qres, &iparams->params);
}

qos_func_define(qos_rv, qres_gw_get_params, qres_iparams_t *iparams) {
  qres_server_t *qres;

  qres = qres_find_by_id(iparams->server_id);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;
  return qres_get_params(qres, &iparams->params);
}

/** Get the execution time of the server since its creation.
 *
 * This is the total amount of time for which the tasks attached
 * to the server have been scheduled by the system
 */
qos_func_define(qos_rv, qres_gw_get_exec_time, qres_time_iparams_t *iparams) {
  qres_server_t *qres;

  qres = qres_find_by_id(iparams->server_id);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;

  return qres_get_exec_abs_time(qres, &iparams->exec_time,
				&iparams->abs_time);
}

qos_func_define(qos_rv, qres_gw_get_curr_budget, qres_time_iparams_t *iparams) {
  qres_server_t *qres;

  qres = qres_find_by_id(iparams->server_id);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;

  iparams->exec_time = qres_get_curr_budget(qres);
  iparams->abs_time = 0;

  return QOS_OK;
}

qos_func_define(qos_rv, qres_gw_get_next_budget, qres_time_iparams_t *iparams) {
  qres_server_t *qres;

  qres = qres_find_by_id(iparams->server_id);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;

  iparams->exec_time = qres_get_next_budget(qres);
  iparams->abs_time = 0;

  return QOS_OK;
}

qos_func_define(qos_rv, qres_gw_get_appr_budget, qres_time_iparams_t *iparams) {
  qres_server_t *qres;

  qres = qres_find_by_id(iparams->server_id);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;

  iparams->exec_time = qres_get_appr_budget(qres);
  iparams->abs_time = 0;

  return QOS_OK;
}

qos_func_define(qos_rv, qres_gw_get_server_id, qres_attach_iparams_t *iparams) {
  server_t *rres;
  struct task_struct *tsk;

  qos_chk_ok_ret(find_task(iparams->pid, iparams->tid, &tsk));
  qos_log_debug("Got tsk with: pid=%d, tgid=%d", tsk->pid, tsk->tgid);
  rres = rres_find_by_task(tsk);
  if (rres == NULL)
    return QOS_E_NOT_FOUND;

  //iparams->server_id = rres_get_sid(rres);

  return QOS_OK;
}

qos_func_define(qos_rv, qres_gw_get_deadline, qres_timespec_iparams_t *iparams) {
  qres_server_t *qres;

  qres = qres_find_by_id(iparams->server_id);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;

  return qres_get_deadline(qres, &iparams->timespec);
}

qos_func_define(qos_rv, qres_gw_set_weight, qres_weight_iparams_t *iparams) {
  qres_server_t *qres;

  qres = qres_find_by_id(iparams->server_id);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;
  if (iparams->weight == 0)
    return QOS_E_INVALID_PARAM;
  rres_set_weight(&qres->rres, iparams->weight);
  return QOS_OK;
}

qos_func_define(qos_rv, qres_gw_get_weight, qres_weight_iparams_t *iparams) {
  qres_server_t *qres;

  qres = qres_find_by_id(iparams->server_id);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;
  iparams->weight = rres_get_weight(&qres->rres);
  return QOS_OK;
}

/** Main US-to-KS gateway function.
 *
 * Copies parameters from US to KS, checks if requested operation is
 * allowed by the calling process, distinguishes among various kinds
 * of requests, calling appropriate per-request functions. If needed,
 * copies return parameters back from KS to US.
 *
 * Assuming the limits configured in the supervisor are not violated
 * by the request, for what concerns security of the OS w.r.t. uid
 * and euid of the requesting process, the access control model
 * should satisfy the following requirements:
 * <ol>
 *   <li>let "user process" denote a process with a non-zero effective
 *       uid, "root process" one with null euid
 *   <li>a user process should not be able to change parameters of,
 *       or destroy, a server created by a privileged process; the only
 *       exception may occurr if the process dies and there are no more
 *       processes left in a non-persistent server
 *   <li>a user process should not be able to attach any processes owned
 *       by anyone to a server created by a privileged process
 *   <li>a privileged process should be able to create and destroy
 *       servers, attach and detach threads at will
 *   <li>a privileged process may emulate creation of a server by a
 *       normal user if it sets its own euid to the one of the user
 *   <li>if a privileged process is placed into a server owned by an
 *       unprivileged user, then the user may keep changing the server
 *       parameters and/or destroy the server as usual (destruction of
 *       a server only means detach of contained processes, not kill)
 * </ol>
 * @todo  avoid copy_from_user and copy_to_user with entire iparams struct
 *        when unneeded.
 */
qos_rv qres_gw_ks(qres_op_t op, void __user *up_iparams, unsigned long size) {
  union {
    qres_sid_t server_id;
    qres_iparams_t iparams;
    qres_attach_iparams_t attach_iparams;
    qres_time_iparams_t time_iparams;
    qres_timespec_iparams_t timespec_iparams;
    qres_weight_iparams_t weight_iparams;
  } u;
  qos_rv err = QOS_OK;

  switch (op) {
  case QRES_OP_CREATE_SERVER:
    COPY_FROM_USER_TO(up_iparams, size, &u.iparams);
    err = call_sync(qres_create_server(&u.iparams.params, &u.iparams.server_id));
    if (err == QOS_OK && copy_to_user(up_iparams, &u.iparams, sizeof(qres_iparams_t)))
      err = QOS_E_INTERNAL_ERROR;
    break;
  case QRES_OP_GET_SERVER_ID:
    COPY_FROM_USER_TO(up_iparams, size, &u.attach_iparams);
    err = call_sync(qres_gw_get_server_id(&u.attach_iparams));
    if (err == QOS_OK && copy_to_user(up_iparams, &u.iparams, sizeof(qres_attach_iparams_t)))
      err = QOS_E_INTERNAL_ERROR;
    break;
  case QRES_OP_DESTROY_SERVER:
    COPY_FROM_USER_TO(up_iparams, size, &u.server_id);
    err = call_sync(qres_gw_destroy_server(u.server_id));
    break;
  case QRES_OP_ATTACH_TO_SERVER:
    COPY_FROM_USER_TO(up_iparams, size, &u.attach_iparams);
    err = call_sync(qres_gw_attach_task(&u.attach_iparams));
    break;
  case QRES_OP_DETACH_FROM_SERVER:
    COPY_FROM_USER_TO(up_iparams, size, &u.attach_iparams);
    err = call_sync(qres_gw_detach_task(&u.attach_iparams));
    break;
  case QRES_OP_SET_PARAMS:
    COPY_FROM_USER_TO(up_iparams, size, &u.iparams);
    err = call_sync(qres_gw_set_params(&u.iparams));
    break;
  case QRES_OP_GET_PARAMS:
    COPY_FROM_USER_TO(up_iparams, size, &u.iparams);
    err = call_sync(qres_gw_get_params(&u.iparams));
    if (err == QOS_OK && copy_to_user(up_iparams, &u.iparams, sizeof(qres_iparams_t)))
      err = QOS_E_INTERNAL_ERROR;
    break;
  case QRES_OP_GET_EXEC_TIME:
    COPY_FROM_USER_TO(up_iparams, size, &u.time_iparams);
    err = call_sync(qres_gw_get_exec_time(&u.time_iparams));
    if (err == QOS_OK && copy_to_user(up_iparams, &u.time_iparams, sizeof(qres_time_iparams_t)))
      err = QOS_E_INTERNAL_ERROR;
    break;
  case QRES_OP_GET_CURR_BUDGET:
    COPY_FROM_USER_TO(up_iparams, size, &u.time_iparams);
    err = call_sync(qres_gw_get_curr_budget(&u.time_iparams));
    if (err == QOS_OK && copy_to_user(up_iparams, &u.time_iparams, sizeof(qres_time_iparams_t)))
      err = QOS_E_INTERNAL_ERROR;
    break;
  case QRES_OP_GET_NEXT_BUDGET:
    COPY_FROM_USER_TO(up_iparams, size, &u.time_iparams);
    err = call_sync(qres_gw_get_next_budget(&u.time_iparams));
    if (err == QOS_OK && copy_to_user(up_iparams, &u.time_iparams, sizeof(qres_time_iparams_t)))
      err = QOS_E_INTERNAL_ERROR;
    break;
  case QRES_OP_GET_APPR_BUDGET:
    COPY_FROM_USER_TO(up_iparams, size, &u.time_iparams);
    err = call_sync(qres_gw_get_appr_budget(&u.time_iparams));
    if (err == QOS_OK && copy_to_user(up_iparams, &u.time_iparams, sizeof(qres_time_iparams_t)))
      err = QOS_E_INTERNAL_ERROR;
    break;
  case QRES_OP_GET_DEADLINE:
    COPY_FROM_USER_TO(up_iparams, size, &u.timespec_iparams);
    err = call_sync(qres_gw_get_deadline(&u.timespec_iparams));
    if (err == QOS_OK && copy_to_user(up_iparams, &u.time_iparams, sizeof(qres_timespec_iparams_t)))
      err = QOS_E_INTERNAL_ERROR;
    break;
  case QRES_OP_SET_WEIGHT:
    COPY_FROM_USER_TO(up_iparams, size, &u.weight_iparams);
    err = call_sync(qres_gw_set_weight(&u.weight_iparams));
    break;
  case QRES_OP_GET_WEIGHT:
    COPY_FROM_USER_TO(up_iparams, size, &u.weight_iparams);
    err = call_sync(qres_gw_set_weight(&u.weight_iparams));
    if (err == QOS_OK && copy_to_user(up_iparams, &u.weight_iparams, sizeof(qres_weight_iparams_t)))
      err = QOS_E_INTERNAL_ERROR;
    break;
  default:
    qos_log_err("Unhandled operation code");
    err = QOS_E_INTERNAL_ERROR;	/* For debugging purposes */
  }
  qos_log_debug("Returning: %s", qos_strerror(err));
  return err;
}
