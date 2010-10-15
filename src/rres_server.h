/** @addtogroup RRES_MOD
 ** @{
 **/

/** @file
 ** @brief Internal resource reservation header file.
 **
 ** Exports resource reservation function to other files in rres component
 **/

#ifndef __RRES_SERVER_H__
#define __RRES_SERVER_H__

#ifndef __RRES_READY_QUEUE_H__
#  error "Include rres_ready_queue.h first"
#endif

#include "rres_config.h"
#include "qos_debug.h"
#include "qos_types.h"
#include "qos_list.h"
#include "kal_timer.h"
#include "kal_sched.h"

/** Set to 1 in order to enable paranoid checks.
 **
 ** These are further checks of parameters and assert conditions that
 ** should not be needed, if all of the components behaved as expected.
 **/
#define RRES_PARANOID 1

/** RRES server main structure */
struct server_t {
  qres_sid_t id;              /**< Server identifier                    */
  kal_time_t period;          /**< Server period                        */
  qres_time_t period_us;      /**< Server period (microseconds)         */
  kal_time_t max_budget;      /**< Maximum budget                       */
  qres_time_t max_budget_us;  /**< Maximum budget (microseconds)        */
  kal_time_t c;               /**< Current budget. This may be negative */
  kal_time_t deadline;        /**< Absolute deadline                    */
  qos_bw_t U_current;         /**< Bandwidth currently allocated to the server (U_current = MAX_BW -> 100% of bandwidth) */
  struct rres_stat {          /**< Server Statistics                    */
    unsigned long int n_rcg;  /**< Number of budget recharges           */
    kal_time_t exec_time;     /**< Server execution time (total of served task) since its creation */
  } stat;                     /**< Statistics                           */
  struct list_head slist;     /**< Used to queue into list of servers   */

  unsigned int flags;         /**< Bitmask of server flags              */

  /** Reference to the server readyqueue insertion point for subsequent removal */
  rq_placeholder_t rq_ph;

  /** Virtual function to be called on budget recharge    */
  qos_bw_t (*get_bandwidth)(struct server_t *srv);

  /** Virtual destructor (does not deallocate memory for server_t structure)    */
  qos_rv (*cleanup)(struct server_t *srv);

  struct list_head tasks;	/**< Head of list of tasks served by the server  */
  int activations;		/**< Number of pending activations               */
  kal_timer_t reactive;		/**< When this timer fires, the server is reactivated (put in ready queue) */
#ifdef CONST_TIME_DISPATCH
  struct list_head disp_tasks;	/**< Head of task_struct list used by CONST_TIME dispatching mechanism */
#endif

  int forbid_reorder;		/**< Forbids update_task_order() while iterating on task list **/
};

//typedef struct server_t server_t;

extern qos_bw_t U_tot;          /**< Total allocated bandwidth */

/** This adds a tolerance to U_LUB as defined for QRES/QSUP, so to
 ** avoid that RRESMOD notifies a system overload when QRES/QSUP do not.
 **/
#define U_LUB2 (U_LUB+r2bw_c(1,100))

static inline qos_rv rres_cleanup_server(server_t *srv) {
  if (RRES_PARANOID)
    qos_chk_do(srv != NULL, return QOS_E_INTERNAL_ERROR);
  return srv->cleanup(srv);
}

/** Used to queue a task in a MULTITASKING server */
struct task_list {
  struct task_struct *task;       /**< pointer to the task served                       */
  struct list_head others;        /**< used to queue the task in the server's task list */
  qos_bool_t is_stopped;          /**< used to avoid stopping a task twice              */
  server_t *srv;                  /**< RRES server                                      */
};

#endif

/** @} */
