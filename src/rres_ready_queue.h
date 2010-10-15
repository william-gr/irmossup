/** @addtogroup RRES_MOD
 * @{
 */

/** @file
 * @brief Implementation of an edf queue
 *
 */ 
#ifndef __RRES_READY_QUEUE_H__
#define __RRES_READY_QUEUE_H__

#include "rres_config.h"
#include "qos_debug.h"
#include "qos_func.h"

/** Avoids circular dependencies **/
typedef struct server_t server_t, *server_p;

#ifdef RRES_USE_HEAP
#  include "rres_ready_queue_eheap.h"
#else
#  include "rres_ready_queue_list.h"
#endif

#endif /* __RRES_READY_QUEUE_H__ */

/** @} */
