/** @file
 ** @brief Main QRES server "class" implementation file.
 **
 ** Implements all operations defined for a QRES server.
 **
 ** @note
 ** All operations are supposed to be called with a global lock held. This lock
 ** may be obtained through the qres_get_lock() function.
 **
 ** @todo
 ** Add creation flag that, if set, allows to set required bandwidth to zero when
 ** the rres server has no activations (should be optional as it may introduce a
 ** latency in reclaiming back the given-away bandwidth when the server activates back).
 **/

#include "rres_config.h"
#include "qres_config.h"
#include "qos_debug.h"
#include <linux/posix-timers.h>
#include <linux/time.h>
#include <linux/cgroup.h>
#include <linux/err.h>
#include <linux/sched.h>

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
#include "rres.h"
#include "kal_sched.h"

qres_sid_t server_id = 1;
struct list_head server_list;

/** Static QRES constructor  */
qos_rv qres_init(void) {
  // compile-time check, run-time error at module insertion
  int rv_sched;

  INIT_LIST_HEAD(&server_list);
  if (offsetof(qres_server_t, rres) != 0) {
    qos_log_crit("Please, ensure qres_server_t.rres field has a null offset");
    return QOS_E_INTERNAL_ERROR;
  }

  rv_sched = sched_group_set_rt_runtime(&init_task_group, 1, 100000);
  if (rv_sched<0) {
     qos_log_debug("Error setting rt runtime for root: %d, retval: %d", 100000, rv_sched);
  }

  qos_log_debug("Task period for root is: %ld", sched_group_rt_period(&init_task_group, 1));
  qos_log_debug("Task tuntime for root is: %ld", sched_group_rt_runtime(&init_task_group, 1));

  return QOS_OK;
}

/** Static QRES destructor   */
qos_rv qres_cleanup(void) {
  server_t *srv;
  struct list_head *tmp, *tmpdel;

  // ** IMPORTANT DO FOLLOWING LINE
  //qos_chk_do(kal_atomic(), return QOS_E_INTERNAL_ERROR);

  /* Destroy all servers, including the default one, if configured. */
  for_each_server_safe(srv, tmp, tmpdel) {
    qos_log_debug("Destroying sever %d ", srv->id);
    /* Virtual call to _qres_cleanup_server() */
    qos_chk_ok_ret(qres_destroy_server(qres_find_by_rres(srv)));
    srv = NULL;
  }
  return QOS_OK;
}

/** Check authorizations for operations affecting a task.
 *
 * Operations on a process different from the current one
 * are only allowed if the two processes have the same euid, or
 * if the calling process is effectively-owned by the super-user.
 *
 * @param tsk
 *   Pointer to the task_struct corresponding to process that is
 *   going to be affected by the operation.
 *
 * @return    non-zero on success, 0 otherwise.
 */
static qos_bool_t authorize_for_task(struct task_struct *tsk) {
  return (kal_task_get_uid(kal_task_current()) == 0) ||
         (kal_task_get_uid(kal_task_current()) == kal_task_get_uid(tsk));
}

/** Check authorizations for operations affecting an existing server.
 *
 * @todo It is completely unimplemented, thus it represents a SECURITY FLAW
 */
static qos_bool_t authorize_for_server(qres_server_t *qres) {
  return (kal_task_get_uid(kal_task_current()) == 0) ||
         (kal_task_get_uid(kal_task_current()) == qres_get_owner_uid(qres));
}

/** QRES Factory Method */
qos_func_define(qos_rv, qres_create_server, qres_params_t *param, qres_sid_t *p_sid) {
  qres_server_t *qres;
  qos_rv rv;

  //qos_chk_do(kal_atomic(), return QOS_E_INTERNAL_ERROR);
  qos_log_debug("q=" QRES_TIME_FMT ", q_min=" QRES_TIME_FMT ", p=" QRES_TIME_FMT ", flags=%d",
      param->Q, param->Q_min, param->P, param->flags);
  qres = qos_create(qres_server_t);
  qos_chk_rv(qres != NULL, QOS_E_NO_MEMORY);
  rv = qres_init_server(qres, param);
  if (rv != QOS_OK) {
    qos_free(qres);
    qos_log_info("qres_init_server failed: %s", qos_strerror(rv));
    return rv;
  }
  *p_sid = qres->rres.id;
  return QOS_OK;
}

/* TODO: Avoid admission control by RRES without going through all existing servers */
void qres_update_bandwidths(void) {
  struct list_head *tmp;
  server_t *srv;
  for_each_server(srv, tmp) {
    //qres_time_t q = bw2Q(rres_get_bandwidth(srv), rres_get_period(srv));
    //rres_set_budget(srv, q);
  }
}

/** QRES Server constructor.    */
qos_func_define(qos_rv, qres_init_server, qres_server_t *qres, qres_params_t *param) {
  qos_rv rv;
  qres_time_t approved_Q;
  qos_bw_t bw_min;
  qos_bw_t bw_req;
  kal_uid_t uid;
  kal_gid_t gid;
  int rv_sched;

  //qos_chk_do(kal_atomic(), return QOS_E_INTERNAL_ERROR);
  qos_log_debug("(Q, P): (" QRES_TIME_FMT ", " QRES_TIME_FMT ")", param->Q, param->P);
  if (param->P < MIN_SRV_PERIOD || param->Q > param->P)
    return QOS_E_INVALID_PARAM;

  if (param->Q < MIN_SRV_MAX_BUDGET) {
//    param->Q = MIN_SRV_MAX_BUDGET;
    return QOS_E_INVALID_PARAM;
  }

  /* Round the minimum required bandwidth to the lowest >= value
   * available due to qos_bw_t granularity
   */
  bw_min = r2bw_ceil(param->Q_min, param->P);

  /* Round the requested values according to the qos_bw_t granularity */
  param->Q_min = bw2Q(bw_min, param->P);
  param->Q = bw2Q(r2bw_ceil(param->Q, param->P), param->P);
  qos_log_debug("Rounded (Q, Q_min, P): (" QRES_TIME_FMT ", " QRES_TIME_FMT ", " QRES_TIME_FMT ")", param->Q, param->Q_min, param->P);

  uid = kal_task_get_uid(kal_task_current());
  gid = kal_task_get_gid(kal_task_current());

#ifdef QRES_ENABLE_QSUP
  /* First, check if request of guaranteed bw min respects constraints
   * for the requesting user (according to its effective uid/guid), and
   * create RRES server (i.e. perform admission control) based on this
   * value.
   */
  if (param->flags & QOS_F_DEFAULT) {
    if (uid != 0 && uid != QSUP_DEFAULT_SRV_UID && gid != QSUP_DEFAULT_SRV_GID)
      return QOS_E_UNAUTHORIZED;
  }
  qos_chk_ok_ret(qsup_init_server(&qres->qsup, kal_task_get_uid(kal_task_current()),
                                  kal_task_get_gid(kal_task_current()), param));

  /* Then, set the actual request (may be < = > than min guaranteed)    */
  bw_req = r2bw(param->Q, param->P);
  qos_log_debug("Setting required bw to " QOS_BW_FMT, bw_req);
  rv = qsup_set_required_bw(&qres->qsup, bw_req);
  if (rv != QOS_OK) {
    qos_log_info("qsup_set_required_bw() failed: %s", qos_strerror(rv));
    qsup_cleanup_server(&qres->qsup);
    return rv;
  }

  /** Compute Q value as approved by supervisor */
  approved_Q = bw2Q(qsup_get_approved_bw(&qres->qsup), param->P);

#else
  if ((param->flags & QOS_F_DEFAULT) && (uid != 0))
    return QOS_E_UNAUTHORIZED;
  approved_Q = param->Q;
#endif

  qos_log_debug("Required=" QRES_TIME_FMT ", Approved=" QRES_TIME_FMT, param->Q, approved_Q);

  // New incomplete server not yet enqueued, so it is safe to loop all servers.
  // All servers get possibly reduced bandwidths due to the presence of the new server.
  qres_update_bandwidths();

  /* Then, we create the cgroup for this reservation */
  qos_log_debug("Creating a new cgroup reservation");

  

  struct task_group *tg = sched_create_group(&init_task_group);
  if (IS_ERR(tg)) {

    qos_log_info("sched_create_group() failed: %s", qos_strerror(rv));
#ifdef QRES_ENABLE_QSUP
    qsup_cleanup_server(&qres->qsup);
#endif
    // New incomplete server was not enqueued, so it is safe to loop all servers.
    // All servers have their bandwidths back like before creation of this server.
    qres_update_bandwidths();
    return rv;

  }
  qres->qsup.tg = tg;


  rv_sched = sched_group_set_rt_period(qres->qsup.tg, 0, param->P);
  if (rv_sched<0) {
     qos_log_debug("Error setting rt period: %ld", param->P);
     qos_log_debug("Period setted: %ld", sched_group_rt_period(qres->qsup.tg, 0));
     qsup_cleanup_server(&qres->qsup);
     qres_update_bandwidths();
     return QOS_E_UNAUTHORIZED;
  }
  qos_log_debug("Period added is: %ld", sched_group_rt_period(qres->qsup.tg, 0));

  rv_sched = sched_group_set_rt_runtime(qres->qsup.tg, 0, approved_Q);
  if (rv_sched<0) {
     qos_log_debug("Error setting rt runtime: %ld", approved_Q);
     qos_log_debug("Runtime setted: %ld", sched_group_rt_runtime(qres->qsup.tg, 0));
     qsup_cleanup_server(&qres->qsup);
     qres_update_bandwidths();
     return QOS_E_UNAUTHORIZED;
  }
  qos_log_debug("Runtime for task added is: %ld", sched_group_rt_runtime(qres->qsup.tg, 0));

  rv_sched = sched_group_set_rt_period(qres->qsup.tg, 1, param->P);
  if (rv_sched<0) {
     qos_log_debug("Error setting rt task period: %ld", param->P);
     qos_log_debug("Task period setted: %ld", sched_group_rt_period(qres->qsup.tg, 1));
     qsup_cleanup_server(&qres->qsup);
     qres_update_bandwidths();
     return QOS_E_UNAUTHORIZED;
  }
  qos_log_debug("Period for task added is: %ld", sched_group_rt_period(qres->qsup.tg, 1));

  rv_sched = sched_group_set_rt_runtime(qres->qsup.tg, 1, approved_Q);
  if (rv_sched<0) {
     qos_log_debug("Error setting rt task runtime: %ld", approved_Q);
     qos_log_debug("Task runtime setted: %ld", sched_group_rt_runtime(qres->qsup.tg, 1));
     qsup_cleanup_server(&qres->qsup);
     qres_update_bandwidths();
     return QOS_E_UNAUTHORIZED;
  }
  qos_log_debug("Runtime for task added is: %ld", sched_group_rt_runtime(qres->qsup.tg, 1));

  /** Then, create associated RRES resources            */
  //rv = rres_init_server(&qres->rres, approved_Q,
  //                      param->P, param->flags);
  //if (rv != QOS_OK) {
  //  qos_log_info("rres_init_server() failed: %s", qos_strerror(rv));
#ifdef QRES_ENABLE_QSUP
  //  qsup_cleanup_server(&qres->qsup);
#endif
  //  // New incomplete server was not enqueued, so it is safe to loop all servers.
  //  // All servers have their bandwidths back like before creation of this server.
  //  qres_update_bandwidths();
  //  return rv;
  //}
  qos_log_debug("Done creating a new cgroup reservation");

  qres->owner_uid = uid;
  qres->owner_gid = gid;

  qres->params = *param;

  ///* Override parent class vtable */
  qres->rres.cleanup = &_qres_cleanup_server;
  qres->rres.get_bandwidth = &_qres_get_bandwidth;
  qres->rres.id = new_server_id();
  rres_add_to_srv_set(&qres->rres);

  return QOS_OK;
}

/** Allocate a new server identifier    **/
qres_sid_t new_server_id(void) {
  while (server_id == QRES_SID_NULL || rres_find_by_id(server_id) != NULL) {
    qos_log_debug("Skipping sid (s:%d) already in use", server_id);
    ++server_id;
  }
  return server_id;
}

/** Return the pointer to the server with the specified server id, or
 ** NULL if not found.
 **
 ** @todo
 ** Speed-up in case of many servers, by using a hashmap
 **/
server_t* rres_find_by_id(qres_sid_t sid) {
  struct list_head *tmp_list;
  server_t *srv;
  if (sid == QRES_SID_NULL)
    return rres_find_by_task(kal_task_current());
  for_each_server(srv, tmp_list) {
    qos_log_debug("For each server: %d\n", srv->id);
    if (srv->id == sid)
      return srv;
  }
  return NULL;
}

qos_func_define(qos_rv, qres_destroy_server, qres_server_t *qres) {
  //struct task_struct *task;

  //qos_chk_do(kal_atomic(), return QOS_E_INTERNAL_ERROR);
  //while ((task = rres_any_ready_task(&qres->rres)) != NULL) {
    //qos_chk_ok_ret(rres_detach_task(&qres->rres, task));
  //}
  //while ((task = rres_any_blocked_task(&qres->rres)) != NULL) {
    //qos_chk_ok_ret(rres_detach_task(&qres->rres, task));
  //}

  /* Would we need to hold some lock? */
  qos_log_debug("Destroy group pointer %d", (int) qres->qsup.tg);
  sched_destroy_group(qres->qsup.tg);

  //qos_chk_ok_ret(rres_destroy_server(&qres->rres));
  qres = NULL; // Don't use qres pointer from here on

  return QOS_OK;

}

/** Non-virtual QRES server destructor  */
qos_func_define(qos_rv, _qres_cleanup_server, server_t *rres) {
  qres_server_t *qres = qres_find_by_rres(rres);

  //qos_chk_do(kal_atomic(), return QOS_E_INTERNAL_ERROR);
  if (! authorize_for_server(qres))
    return QOS_E_UNAUTHORIZED;

  /* First, destroy child extensions keeping overridden vtable */

#ifdef QRES_ENABLE_QSUP
  qos_chk_ok_ret(qsup_cleanup_server(&qres->qsup));
#endif

  /* Then, reset original parent class vtable	*/

  //qres->rres.cleanup = &_rres_cleanup_server;
  //qres->rres.get_bandwidth = &_rres_get_bandwidth;

  /* Finally, destroy parent class		*/
  //qos_chk_ok_ret(_rres_cleanup_server(&qres->rres));
  return QOS_OK;
}

/** Non-virtual bandwidth getter        */
qos_bw_t _qres_get_bandwidth(server_t *srv) {
  qres_server_t *qres = qres_find_by_rres(srv);
  /* r2bw works with clocks too, as long as all in/out params fit into an unsigned long */
#ifdef QRES_ENABLE_QSUP
  return qsup_get_approved_bw(&qres->qsup);
#else
  return _rres_get_bandwidth(&qres->rres);
#endif
}

/** Attach to the server identified by srv_id the task identified by tsk */
qos_func_define(qos_rv, qres_attach_task, qres_server_t *qres, struct task_struct *tsk) {
  int rv;

  //qos_chk_do(kal_atomic(), return QOS_E_INTERNAL_ERROR);
  if ((! authorize_for_task(tsk)) || (! authorize_for_server(qres)))
    return QOS_E_UNAUTHORIZED;
  //qos_chk_ok_ret(rres_attach_task(&qres->rres, tsk));

  //qos_log_debug("task cgroups pointer %d", (int) tsk->cgroups);
  tsk->tg = qres->qsup.tg;
  qos_log_debug("going to move task");
  sched_move_task(tsk);
  qos_log_debug("sched move task rv: %d", rv);

  //if(rv<0) {
  //   qos_log_debug("Error attaching task to group");
  //}

  /* Dynamic reclamation is automatic here, no need to explicitly       *
   * require the QSUP_DYNAMIC_RECLAIM switch !                          */
  //if (rres_has_ready_tasks(&qres->rres)) {
  //  qsup_set_required_bw(&qres->qsup, r2bw(qres->params.Q, qres->params.P));
  //}

  return QOS_OK;
}

/**
 * Asks the RRES to detach the current task from its server. If the
 * detached task was the last one attached to the server, then this
 * function asks supervisor to destroy information associated to this
 * server, and also asks the RRES to destroy the server related info.
 *
 * @note
 * When this function returns, the memory pointed to by the qres parameter
 * could have been deallocated.
 */
qos_func_define(qos_rv, qres_detach_task, qres_server_t *qres, struct task_struct *tsk) {
  //qos_chk_do(kal_atomic(), return QOS_E_INTERNAL_ERROR);
  if (qres == NULL)
    return QOS_E_NOT_FOUND;
  if ((! authorize_for_task(tsk)) || (! authorize_for_server(qres)))
    return QOS_E_UNAUTHORIZED;
  //qos_chk_ok_ret(rres_detach_task(&qres->rres, tsk));

  tsk->tg = &init_task_group;
  sched_move_task(tsk);

  /* Dynamic reclamation is automatic here, no need to explicitly       *
   * require the QSUP_DYNAMIC_RECLAIM switch !                          */
  //if (! rres_has_ready_tasks(&qres->rres)) {
  //  qsup_set_required_bw(&qres->qsup, 0);
  //}

  //qos_chk_ok_ret(rres_check_destroy(&qres->rres));
  qres = NULL; // DO NOT USE qres POINTER, FROM HERE ON
  return QOS_OK;
}

/** Set QRES server parameters Q, Q_min and P. **/
qos_func_define(qos_rv, qres_set_params, qres_server_t *qres, qres_params_t *param) {
  qres_time_t approved_Q;

  //qos_chk_do(kal_atomic(), return QOS_E_INTERNAL_ERROR);
  qos_log_debug("Q=" QRES_TIME_FMT "  Q_min=" QRES_TIME_FMT "  P=" QRES_TIME_FMT,
		param->Q, param->Q_min, param->P);

  if (! authorize_for_server(qres))
    return(QOS_E_UNAUTHORIZED);

  if (param->P < MIN_SRV_PERIOD || param->Q > param->P)
    return QOS_E_INVALID_PARAM;

  if (param->Q < MIN_SRV_MAX_BUDGET)
    return QOS_E_INVALID_PARAM;

  if (param->flags != qres->params.flags)
    return QOS_E_UNIMPLEMENTED;

  /* Round the requested values according to the qos_bw_t granularity */
  param->Q_min = bw2Q(r2bw_ceil(param->Q_min, param->P), param->P);
  param->Q = bw2Q(r2bw_ceil(param->Q, param->P), param->P);
  qos_log_debug("Rounded (Q, Q_min, P): (" QRES_TIME_FMT ", " QRES_TIME_FMT ", " QRES_TIME_FMT ")", param->Q, param->Q_min, param->P);

#ifdef QRES_ENABLE_QSUP
  if (param->Q_min != qres->params.Q_min
      || param->P != qres->params.P) {
    qos_chk_ok_ret(qsup_cleanup_server(&qres->qsup));
    if (qsup_init_server(&qres->qsup, qres->owner_uid, qres->owner_gid, param) != QOS_OK)
      qos_chk_ok_ret(qsup_init_server(&qres->qsup, qres->owner_uid, qres->owner_gid, &qres->params));
  }
  qos_chk_ok_ret(qsup_set_required_bw(&qres->qsup, r2bw(param->Q, param->P)));
  approved_Q = bw2Q(qsup_get_approved_bw(&qres->qsup), param->P);
#else
  approved_Q = param->Q;
#endif
  qos_log_debug("Required=" QRES_TIME_FMT ", Approved=" QRES_TIME_FMT, param->Q, approved_Q);
  //qos_chk_ok_ret(rres_set_params(&qres->rres, approved_Q, param->P));
  qres->params = *param;

  return QOS_OK;
}

qos_func_define(qos_rv, qres_get_params, qres_server_t *qres, qres_params_t *params) {
  //qos_chk_do(kal_atomic(), return QOS_E_INTERNAL_ERROR);
  *params = qres->params;
  return QOS_OK;
}

qos_func_define(qres_time_t, qres_get_curr_budget, qres_server_t *qres) {
  return rres_get_curr_budget(&qres->rres);
}

qos_func_define(qres_time_t, qres_get_next_budget, qres_server_t *qres) {
  return bw2Q(rres_get_current_bandwidth(&qres->rres), qres->params.P);
}

qos_func_define(qres_time_t, qres_get_appr_budget, qres_server_t *qres) {
  return bw2Q(rres_get_bandwidth(&qres->rres), qres->params.P);
}

qos_func_define(qos_rv, qres_get_deadline, qres_server_t *qres, struct timespec *p_deadline) {
  return rres_get_deadline(&qres->rres, p_deadline);
}

qos_func_define(qos_rv, qres_get_exec_abs_time, qres_server_t *qres,
              qres_time_t *exec_time, qres_atime_t *abs_time) {
  //*exec_time = rres_get_exec_time(&qres->rres);
  *abs_time = kal_time2usec(kal_time_now());
  return QOS_OK;
}

/** Return the user id of the owner of the server. */
kal_uid_t qres_get_owner_uid(qres_server_t* qres) {
  return qres->owner_uid;
}
EXPORT_SYMBOL_GPL(qres_get_owner_uid);

/** Return the group id of the owner of the server. */
kal_gid_t qres_get_owner_gid(qres_server_t* qres) {
  return qres->owner_gid;
}
EXPORT_SYMBOL_GPL(qres_get_owner_gid);

EXPORT_SYMBOL_GPL(qres_create_server);
EXPORT_SYMBOL_GPL(qres_destroy_server);
EXPORT_SYMBOL_GPL(qres_attach_task);
EXPORT_SYMBOL_GPL(qres_detach_task);
EXPORT_SYMBOL_GPL(qres_set_params);
EXPORT_SYMBOL_GPL(qres_get_params);
//EXPORT_SYMBOL_GPL(qres_get_exec_time);
EXPORT_SYMBOL_GPL(qres_get_exec_abs_time);
EXPORT_SYMBOL_GPL(qres_get_deadline);

/* Export protected symbols */
EXPORT_SYMBOL_GPL(_qres_get_bandwidth);
EXPORT_SYMBOL_GPL(_qres_cleanup_server);
