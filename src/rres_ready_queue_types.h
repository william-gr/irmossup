/** @addtogroup RRES_MOD
 * @{
 */

/** @file
 * @brief Types related to the ready queue needed by rres.h within server_t definition
 *
 */ 
#ifndef __RRES_EDF_TYPES_H__
#define __RRES_EDF_TYPES_H__

#include "rres_config.h"

typedef struct server_t *server_p;

#ifdef RRES_USE_HEAP

#define RRES_MAX_NUM_SERVERS_BITS 12
#define RRES_MAX_NUM_SERVERS (1 << RRES_MAX_NUM_SERVERS_BITS)

#include "kal_timer.h"
#define kal_eheap_key_lt kal_time_lt
#include "kal_eheap.h"
#include "qos_types.h"

kal_define_eheap(kal_time_t, server_p);

/**
 * Ready queue placeholder, useful for being embedded within an enqueued
 * item for subsequenct extraction
 */
typedef kal_eheap_iterator_t(kal_time_t, server_p) rq_placeholder_t;

/** Inititalize a ready queue placeholder as invalid (item not enqueued) */
#define rq_placeholder_init(p_it) kal_eheap_iterator_init(p_it)

#else

/**
 * Ready queue placeholder, useful for being embedded within an enqueued
 * item for subsequenct extraction
 */
typedef struct list_head rq_placeholder_t;

/** Inititalize a ready queue placeholder as invalid (item not enqueued) */
#define rq_placeholder_init(p_list_head) INIT_LIST_HEAD_NULL(p_list_head)

#endif

#endif /* __RRES_EDF_TYPES_H__ */

/** @} */
