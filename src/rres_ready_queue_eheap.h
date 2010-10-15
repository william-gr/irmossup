/** @addtogroup RRES_MOD
 * @{
 */

/** @file
 * @brief Heap-based implementation of an edf queue
 */
#ifndef __RRES_READY_QUEUE_EHEAP_H__
#define __RRES_READY_QUEUE_EHEAP_H__

#include "rres_config.h"
#include "kal_timer.h"
#include "qos_types.h"
#include "qos_func.h"

#define kal_eheap_key_lt kal_time_lt
#include "kal_eheap.h"

kal_define_eheap(kal_time_t, server_p);

/**
 * Ready queue placeholder, useful for being embedded within an enqueued
 * item for subsequenct extraction
 */
typedef kal_eheap_iterator_t(kal_time_t, server_p) rq_placeholder_t;

#include "rres_server.h"

#define RRES_MAX_NUM_SERVERS_BITS 12
#define RRES_MAX_NUM_SERVERS (1 << RRES_MAX_NUM_SERVERS_BITS)

/** Inititalize a ready queue placeholder as invalid (item not enqueued) */
#define rq_placeholder_init(p_it) kal_eheap_iterator_init(p_it)

/** Heap of ready servers, min-ordered by abs deadline */
extern kal_eheap_t(kal_time_t, server_p) eheap;

/** return a pointer to the highest priority server (the first of EDF queue) */
static inline server_t *get_highest_priority_server(void) {
  server_t * srv = NULL;
  kal_time_t dl;
  if (kal_eheap_get_min(&eheap, &dl, &srv) == QOS_E_EMPTY)
    return NULL;
  return srv;
}

/** iterate over ready server list 
 *  @param 
 *  srv:         (server_t *)             variable for "current" server (used inside the loop)
 *  pos:         (rq_placeholder_t *)     temporal variable used to iterate (not used in following loop block) */
#define for_each_ready_server(srv, it) \
  for ( \
    kal_eheap_begin(&eheap, &(it)); \
    && (kal_eheap_next(&eheap, &(it), NULL, &(srv)) == QOS_OK); \
    /* Increment not necessary, already made by kal_eheap_next */ \
  )

/** return true if the ready queue is empty (no ready server in the system) */
static inline int ready_queue_empty(void) {
  return kal_eheap_size(&eheap) == 0;
}

/** return true if the server is the only ready server in the system */
static inline int srv_alone(server_t *srv) {
  kal_time_t dl;
  server_t *top_srv;
  qos_chk_ok(kal_eheap_get_min(&eheap, &dl, &top_srv));
  return (srv == top_srv) && (kal_eheap_size(&eheap) == 1);
}

/** return true if the server is in ready queue */
static inline int in_ready_queue(server_t *srv) {
  return kal_eheap_iterator_valid(&srv->rq_ph);
}

/** Remove the server from ready queue */
static qos_func_define(void, ready_queue_remove, server_t *srv) {
  qos_log_debug("(s:%d): Removing from ready queue", srv->id);

  qos_chk_do_msg(in_ready_queue(srv), return, "Attempt to remove (s:%d) from ready queue while it is not in rq. IGNORED", srv->id);

  qos_chk_ok(kal_eheap_del(&eheap, &srv->rq_ph));
  qos_chk(! kal_eheap_iterator_valid(&srv->rq_ph));
  qos_chk(! in_ready_queue(srv));
}

static inline qos_rv rres_edf_init(void) {
   qos_chk_ok_ret(kal_eheap_init(&eheap, RRES_MAX_NUM_SERVERS_BITS));
   return QOS_OK;
}

static inline void rres_edf_cleanup(void) {
   kal_eheap_cleanup(&eheap);
}

/** Add the server to ready queue, ordered by deadline (EDF)  **/
static qos_func_define(qos_rv, ready_queue_add, server_t *srv) {
  qos_log_debug("(s:%d): Adding to ready queue", srv->id);

  qos_chk_do_msg(! in_ready_queue(srv), return QOS_E_INTERNAL_ERROR, "(s:%d) already in ready queue", srv->id);
  qos_chk_ok_ret(kal_eheap_add(&eheap, srv->deadline, srv, &srv->rq_ph));
  return QOS_OK;
}

#endif /* __RRES_READY_QUEUE_EHEAP_H__ */

/** @} */
