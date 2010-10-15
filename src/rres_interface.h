/** @addtogroup RRES_MOD Resource Reservation Low Level Implementation
 *
 * @ingroup QRES
 *
 * Provides CPU resource reservations into the kernel through a kernel API.
 *
 * @{
 */

/** @file
 * @brief RRES Module main interface header.
 *
 * @todo (high) TOMMASO distinguish synchronized and not synchronized functions
 */

#ifndef __RRES_INTERFACE_H__
#define __RRES_INTERFACE_H__

#include "rres_config.h"
#include "qos_types.h"
#include "kal_sched.h"
#include "rres_ready_queue.h"

/** @todo (high) use types defined in qos_types.h instead of using primitive types likes long long */

#if defined(CONFIG_OC_RRES_HRT) || LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,0)
#  define MIN_SRV_PERIOD      1000LU	/**< usec. Minimum allowed server period (HRT) */
#  define MIN_SRV_MAX_BUDGET  0LU	/**< usec. Minimum allowed server budget (HRT) */
#else
#  define MIN_SRV_PERIOD     20000LU	/**< usec. Minimum allowed server period */
#  define MIN_SRV_MAX_BUDGET 0LU	/**< usec. Minimum allowed server budget */
#endif

#define MIN_SRV_PERIOD_DEBUG 200000LU	/**< usec. Minimum allowed server period while debugging */

static inline struct task_list *rres_find_task_list(kal_task_t *task) {
  return (struct task_list *) kal_task_get_data(task);
}

/** Return the pointer to the server the task is attached to, or NULL */
static inline server_t * rres_find_by_task(kal_task_t *task) {
  struct task_list *tl = rres_find_task_list(task);
  if (tl != NULL)
    return tl->srv;
  else
    return NULL;
}

/** Initialise rres module */
int rres_init_module(void);

/** Cleanup rres module */
void rres_cleanup_module(void);

/** Create a new server for task t.
 *
 * It performs extra operation with respect to rres_create_server if
 * the task is already served by an existing server.
 *
 * max_budget and period are expressed in usecs. */
qos_rv rres_create_server(server_t ** new_srv, qres_time_t max_budget, qres_time_t period, unsigned long flags);

/** Initialize an already allocated server_t (or derived class) */
qos_rv rres_init_server(server_t * new_srv, qres_time_t max_budget, qres_time_t period, unsigned long flags);

/** Attach a task to an existing (possibly empty) server */
qos_rv rres_attach_task(server_t *srv, struct task_struct *task);

/** Detach specified task from its own RRES server.
 *
 * Do not automatically destroy the server if it becomes empty. */
qos_rv rres_detach_task(server_t *srv, struct task_struct *task);

/** Destroy the specified server.
 **
 ** All attached tasks are detached and return to the default scheduling
 ** policy. Any resource associated to the server pointed to by rres
 ** parameter, including the server_t struct itself, is deallocated.
 **/
qos_rv rres_destroy_server(server_t *rres);

/** Cleanup the specified server, which must already be empty.
 **
 ** Any resource associated to the server pointed to by rres
 ** parameter, except the server_t struct itself, is deallocated.
 **
 ** @note
 ** Before calling this function, ensure the server is empty by calling
 ** rres_detach_task(), if needed.
 **/
qos_rv rres_cleanup_server(server_t *rres);

/** Check if server empty and, if appropriate for the server flags, destroy it.
 **
 ** This must be explicitly called after any call to functions that may detach
 ** tasks from a server leaving it empty, e.g. rres_detach_server(), rres_move(),
 ** rres_detach_all_tasks(), in order to enforce non-persistent servers semantics.
 **/
qos_rv rres_check_destroy(server_t *srv);

/** Return the pointer to the server with identification number "id".
 *
 * Return NULL if no server with the specified id is found.
 */
server_t* rres_find_by_id(qres_sid_t sid);

/** Returns a server unique identifier	*/
qres_sid_t rres_get_sid(server_t *rres);

/** Set budget and period of RRES server (us) */
qos_rv rres_set_params(server_t *rres, qres_time_t budget, qres_time_t period);

/** Get RRES scheduling params (us).		*/
qos_rv rres_get_params(server_t *rres, qres_time_t *budget, qres_time_t *period);

/** Set budget of RRES server (us).		*/
qos_rv rres_set_budget(server_t *rres, qres_time_t budget);

/** Get remaining budget for the current server instance (us).	*/
static inline qres_time_t rres_get_curr_budget(server_t *srv) {
  return kal_time2usec(srv->c);
}

/** Get RRES server period (us).		*/
qres_time_t rres_get_period(server_t *rres);

/** Get RRES current deadline (timespec).	*/
static inline qos_rv rres_get_deadline(server_t *rres, struct timespec *p_deadline) {
  *p_deadline = kal_time2timespec(rres->deadline);
  return QOS_OK;
}

/** Get RRES server flags.                      */
static inline unsigned long rres_get_flags(server_t *rres) {
  return rres->flags;
}

/** Get execution time since server creation (clocks).	*/
qres_time_t rres_get_exec_time(server_t *rres);

/* /\** Get abs time wrt some point in the past (usec).	*\/ */
/* qres_time_t rres_get_time_us(void); */

/** Get pointer to default server.
 *  Return NULL if default server is not configured */
server_t * rres_get_default_server(void);

/** Initialize the default server.
 *
 * The default server hosts all tasks that do not explicitly use AQuoSA.
 * After this call, all system tasks have been attached to the default server.
 *
 * @param srv   Pointer to an already allocated structure of type server_t (or subtype).
 */
qos_rv rres_init_default_server(server_t * srv, qres_time_t Q, qres_time_t P, unsigned long flags);

/** returns true if the task is served by any server*/
int rres_has_server(struct task_struct *t);

/** return true if the server has no task to serve */
int rres_empty(server_t *srv);

/** Return true is the specified server has no attached tasks */
int rres_empty(server_t *srv);

/** Check if the server is currently running. */
int rres_running(server_t *srv);

/** Just return one of the attached ready tasks, or NULL if none exists **/
static inline struct task_struct *rres_any_ready_task(server_t * srv) {
  //struct task_list *tl;
  if (RRES_PARANOID)
    qos_chk_do(srv != NULL && qos_mem_valid(srv), return NULL);
  //if (list_empty(&srv->ready_tasks))
    return NULL;
  //tl = list_entry(srv->ready_tasks.next, struct task_list, node);
  //return tl->task;
}

/** Just return one of the attached blocked tasks, or NULL if none exists **/
static inline struct task_struct *rres_any_blocked_task(server_t * srv) {
  //struct task_list *tl;
  if (RRES_PARANOID)
    qos_chk_do(srv != NULL && qos_mem_valid(srv), return NULL);
  //if (list_empty(&srv->blocked_tasks))
    return NULL;
  //tl = list_entry(srv->blocked_tasks.next, struct task_list, node);
  //return tl->task;
}

extern spinlock_t rres_lock;

static inline kal_lock_t *rres_get_spinlock(void) {
  return &rres_lock;
}

#endif /* __RRES_INTERFACE_H__ */

/** @} */
