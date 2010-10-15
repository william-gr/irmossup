/** @addtogroup RRES_MOD
 * @{
 */

/** @file
 * @brief Internal resource reservation header file.
 *
 * Exports resource reservation function to other files in rres component */

#ifndef __RRES_H__
#define __RRES_H__

#include "rres_config.h"
#include "rres_interface.h"

#include "qos_types.h"
#include "qos_memory.h"
#include "qos_list.h"
//#include "qos_kernel_dep.h"
#include "kal_generic.h"
#include "rres_time.h"

struct kal_timer_t;

extern spinlock_t generic_scheduler_lock __cacheline_aligned; /**< used for spinlock on hook handlers and timer handler */
extern struct list_head server_list;    /**< list of the servers */
extern kal_time_t last_update_time;   /**< time of last budget updating */
#ifdef CONFIG_RRES_DEFAULT_SRV
extern server_t *default_srv;           /**< default_srv server */
#endif

/** Initialise server lists, timers, and algorithm related data, then call rres_schedule() */
qos_rv rres_init(void);

/** Destroy all servers in the system  */
qos_rv rres_cleanup(void);

/** Detach all the tasks served by the default server and cleanup the server */
qos_rv rres_cleanup_default_server_nosched(server_t *srv);

/* Remove a task from its server and let it be scheduled by the system default scheduling policy (e.g. Linux in a Linux box) */
qos_rv rres_detach_task_nosched(server_t *srv, struct task_struct *task);

/** Move into the specified server the specified task. */
qos_rv rres_move_nosched(server_t * new_srv, struct task_struct *task);

/** Activate the server due to a new job arrival or add
 *  a pending request of activation if already active */
//qos_rv rres_activate_nosched(server_t *srv);

/** Called when a job ends (tasks blocks)
 *  if it was the last active task in the server,
 *  remove the server from the ready queue */
//qos_rv rres_deactivate_nosched(server_t *srv);

/** Main RRES server scheduling function.
 **
 ** Update budget of currently running server, if any.
 ** Select the earliest deadline server and dispatch it.
 **/
void rres_schedule(void);

/** Recharge the budget and update the deadline to the old value plus the server period. */
/* @todo (low) add a condition like (if (approved_changed()) {recharge();}).  */
void recharge(server_t *srv);

/** recharge the budget and set the deadline to the actual time plus the server period */
void recharge_reset_from_now(server_t *srv);

qos_rv rres_on_task_block(struct task_struct *task);

qos_rv rres_on_task_unblock(struct task_struct *task);

/** Macro used by for_each_*_server to iterate over a server list.
 * 
 *  @param srv  (server_t *)            variable for "current" server (used inside the loop)
 *  @param head (struct list_head *)    head of the list of servers (ready, recharging etc.)
 *  @param list_field ("name of field") name of field used to insert the object in the list(e.g. slist)
 *  @param pos  (struct list_head *)    temporal variable used to iterate (not used in following loop block)
 */
#define for_each_srv(srv, head, list_field, pos) \
        for ((pos) = (head)->next, prefetch((pos)->next); \
        (srv) = (list_entry((pos), server_t, list_field)), (pos) != (head); \
                (pos) = (pos)->next, prefetch((pos)->next))

/** Macro used against removal risks to iterate over the server list
 * 
 *  @param srv    (server_t *)           variable for "current" server (used inside the loop)
 *  @param head   (struct list_head *)   head of the list of servers (ready, recharging etc.)
 *  @parma list_field ("name of field")  name of field used to insert the object in the list(e.g. slist)
 *  @param pos    (struct list_head *)   temporal variable used to iterate (not used in following loop block)
 *  @param tmppos (struct list_head *)   temporal variable used to store temporarily the pos variable used during removal
 */
#define for_each_srv_safe(srv, head, list_field, pos, postmp) \
   for (pos = (head)->next, postmp = pos->next; \
         (srv) = (list_entry((pos), server_t, list_field)), pos != (head); \
         pos = postmp, postmp = pos->next)

/** Iterate over all servers in the system
 * 
 *  @param srv (server_t *)             variable for "current" server (used inside the loop)\n
 *  @param pos (struct list_head *)     temporal variable used to iterate (not used in following loop block) */
#define for_each_server(srv, pos)        for_each_srv((srv), &server_list, slist,    (pos))

/** Iterate over all servers in the system - safe against removal
 * 
 *  @param srv    (server_t *)             variable for "current" server (used inside the loop)\n
 *  @param pos    (struct list_head *)     temporal variable used to iterate (not used in following loop block) 
 *  @param tmppos (struct list_head *)     temporal variable used to store temporarily the pos variable used during removal */
#define for_each_server_safe(srv, pos, tmppos)        for_each_srv_safe((srv), &server_list, slist,    (pos), (tmppos))

/** Return the ponter to task_list referred by (h) */
#define get_task_entry(h) list_entry((h), struct task_list, others)

/** Loop on all tasks in a server (do not modify the list)
 * 
 *  @param __srv (server_t *)            server that contains the list of tasks
 *  @param t     (struct task_struct *)  pointer to "current" task in iteration
 *  @param pos   (list_head *)           temporary variable used for iteration
 * 
 * @note: t is modified by this macro; t is not valid outside (after) loop; */
#define for_each_ready_task_in_server(__srv, t, pos) \
        for ((pos) = (__srv)->ready_tasks.next, prefetch((pos)->next); \
        (t) = (list_entry((pos), struct task_list, node))->task, (pos) != &((__srv)->ready_tasks); \
                (pos) = (pos)->next, prefetch((pos)->next))

#define for_each_blocked_task_in_server(__srv, t, pos) \
        for ((pos) = (__srv)->blocked_tasks.next, prefetch((pos)->next); \
        (t) = (list_entry((pos), struct task_list, node))->task, (pos) != &((__srv)->blocked_tasks); \
                (pos) = (pos)->next, prefetch((pos)->next))

/** Loop on all tasks in a server.
 * 
 *  use this instead of for_each_task_in_server() if you need to modify the list (adding or removing an entry)
 *  @param __srv (server_t *)            server that contains the list of tasks
 *  @param t     (struct task_struct *)  pointer to "current" task in iteration
 *  @param pos   (list_head *)           temporary variable used for iteration
 * 
 * @note: t is modified by this macro; t is not valid outside (after) loop; */
#define for_each_ready_task_in_server_safe(__srv, t, __pos, __tmp) \
        for ((__pos) = (__srv)->ready_tasks.next, __tmp = (__pos)->next; \
        (t) = (list_entry((__pos), struct task_list, node))->task, (__pos) != &((__srv)->ready_tasks); \
                (__pos) = (__tmp), __tmp = (__pos)->next)

#define for_each_blocked_task_in_server_safe(__srv, t, __pos, __tmp) \
        for ((__pos) = (__srv)->blocked_tasks.next, __tmp = (__pos)->next; \
        (t) = (list_entry((__pos), struct task_list, node))->task, (__pos) != &((__srv)->blocked_tasks); \
                (__pos) = (__tmp), __tmp = (__pos)->next)

/** Check if the task is served by the default server */
static inline qos_bool_t is_in_default_server(struct task_struct *t) {
#if defined (CONFIG_RRES_DEFAULT_SRV)
  return rres_find_by_task(t) == default_srv;
#else
  return 0;
#endif  
}

/** It returns true if the server is the default server */
static inline qos_bool_t is_default_server(server_t * srv) { 
#if defined (CONFIG_RRES_DEFAULT_SRV)
  return (srv == default_srv); 
#else
  return 0;
#endif  
}

/*********** DISPATCH RELATED ****************/

/** Stop all tasks handled by the server */
void rres_stop(server_t * srv);

/** Dispatch (force scheduling) of all tasks handled by the server */ 
void rres_dispatch(server_t *srv);

void stop_task_safe(server_t *srv, struct task_struct *task);

void dispatch_task_safe(server_t *srv, struct task_struct *task);

/** @} */

#endif
