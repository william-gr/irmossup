/** @addtogroup RRES_MOD
 * @{
 */

/** @file
 * @brief List-based implementation of an edf queue
 */
#ifndef __RRES_READY_QUEUE_LIST_H__
#define __RRES_READY_QUEUE_LIST_H__

#include "rres_config.h"
#include "kal_timer.h"
#include "qos_types.h"
#include "qos_func.h"

#include "qos_list.h"

/**
 * Ready queue placeholder, useful for being embedded within an enqueued
 * item for subsequenct extraction
 */
typedef struct list_head rq_placeholder_t;

#include "rres_server.h"

/** List of ready servers, min-ordered by abs deadline */
extern struct list_head ready_queue;

/** Inititalize a ready queue placeholder as invalid (item not enqueued) */
#define rq_placeholder_init(p_list_head) INIT_LIST_HEAD_NULL(p_list_head)

/** return a pointer to the highest priority server (the first of EDF queue) */
static inline server_t *get_highest_priority_server(void) {
  if (list_empty(&ready_queue))
    return NULL;
  else
    return list_entry(ready_queue.next, server_t, rq_ph);
}

/** iterate over ready server list 
 *  @param 
 *  srv:         (server_t *)             variable for "current" server (used inside the loop)\n
 *  pos:         (rq_placeholder_t *)     temporal variable used to iterate (not used in following loop block) */
#define for_each_ready_server(srv, pos) \
  for_each_srv((srv), &ready_queue,  rq_ph,    (pos))

/** return true if the ready queue is empty (no ready server in the system) */
static inline int ready_queue_empty(void) {
  return list_empty(&ready_queue);
}

/** return true if the server is the only ready server in the system */
static inline int srv_alone(server_t *srv) {
  return (((srv)->rq_ph.prev == &ready_queue) && ((srv)->rq_ph.next == &ready_queue));
}

/** return true if the server is in ready queue */
static inline int in_ready_queue(server_t *srv) {
  return ((srv)->rq_ph.next != NULL);
}

/** Remove the server from ready queue */
//static qos_func_define(void, ready_queue_remove, server_t *srv)
//{
//  qos_log_debug("(s:%d): Removing from ready queue", srv->id);
//
//  qos_chk_do_msg(in_ready_queue(srv), return, "Attempt to remove (s:%d) from ready queue while it is not in rq. IGNORED", srv->id);
//
//  list_del_null(&srv->rq_ph);
//}

static inline qos_rv rres_edf_init(void) {
   INIT_LIST_HEAD(&ready_queue);
   return QOS_OK;
}

static inline void rres_edf_cleanup(void) {
  // No action required
  qos_chk(list_empty(&ready_queue));
}

/** Add the server to ready queue, ordered by deadline (EDF)  **/
//static qos_func_define(qos_rv, ready_queue_add, server_t *srv) {
//  int ret;
//
//  qos_log_debug("%d", srv->id);
//
//  if (in_ready_queue(srv)) {
//    /* @todo (mid) leave for debug? (it happened when an old iris_hr_reactivate timer fired very late (after a lot of other events)) ;
//    *  check if this happened due to a bug in programming timer (not stopped when needed?) or to a temporary overload situation.
//    *  I could not verify this because in /var/log/messages  the previous messages get lost */
//    qos_log_crit("%d is already in ready queue", srv->id);
//    DUMP_STACK;
//    ready_queue_remove(srv);
//  }
//  list_add_ordered(ready_queue, server_t, srv, rq_ph, deadline, ret);
//  return QOS_OK;
//}

#endif /* __RRES_READY_QUEUE_LIST_H__ */

/** @} */
