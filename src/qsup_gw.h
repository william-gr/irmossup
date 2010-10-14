#ifndef __QSUP_KS_H__
#define __QSUP_KS_H__

#include "qos_debug.h"
#include "qos_types.h"

/** @file
 *
 * @brief Common defines and structures for the US to KS gateway
 *
 * This file defines the QoS Supervisor operation codes used
 * to request an operation to the KS running module.
 * See qsup_gw_lib.h or qsup_gw_lib.c for details.
 *
 * @ingroup QSUP_MOD QSUP_LIB
 */

/** Set of per-user or per-group constraints	*/
typedef struct qsup_constraints_t {
  int level;		/**< Level of the user/group processes	*/
  int weight;		/**< Weight of the user/group processes	*/
  qos_bw_t max_bw;	/**< Maximum per-user bandwidth		*/
  qos_bw_t max_min_bw;	/**< Max per-process guaranteed bw	*/
  unsigned int flags_mask; /**< Mask of unallowed flags         */
} qsup_constraints_t;

/** Types of operation that can be requested to the QSUP module */
typedef enum {
  QSUP_OP_ADD_LEVEL_RULE,
  QSUP_OP_ADD_GROUP_RULE,
  QSUP_OP_ADD_USER_RULE,
  QSUP_OP_FIND_CONSTR,
  QSUP_OP_GET_AVAIL_GUA_BW,
  QSUP_OP_RESERVE_SPARE,
} qsup_op_t;

typedef struct qsup_iparams_t {
  union {
    struct {
      int level_id;
      qos_bw_t max_level_bw;
    } level_rule;
    struct {
      int gid;
      qsup_constraints_t constr;
    } group_rule;
    struct {
      int uid;
      qsup_constraints_t constr;
    } user_rule;
    struct {
      int uid, gid;
      qsup_constraints_t constr;
    } found_rule;
    struct {
      int uid, gid;
      qos_bw_t avail_gua_bw;
    } avail;
    qos_bw_t spare_bw;
  } u;
} qsup_iparams_t;

/** Name of the QoS Supervisor device used to	*
 * communicate with the kernel module		*/
#define QSUP_DEV_NAME "qossup"

/** Major number for the QoS Supervisor device	*/
#define QSUP_MAJOR_NUM 242

/** @} */

#endif
