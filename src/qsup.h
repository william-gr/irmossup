#ifndef __QSUP_H__
#define __QSUP_H__

/** @defgroup QSUP QoS Supervisor Component
 */

/** @addtogroup QSUP_MOD QoS Supervisor Kernel Module
 *
 * @ingroup QSUP
 *
 * Mediates bandwidth requests made to the @ref QRES_MOD, enforcing
 * system-level security policy and QoS behaviour configured through
 * the use of level, group and user rules. Such rules may be configured
 * through the use of the @ref QSUP_LIB interface.
 * @{
 */

/** @file
 * @brief QoS Supervisor kernel-interface.
 */

#include "qsup_gw.h"
#include "qres_gw.h"
#include "qos_debug.h"
#include "qos_types.h"
#include <linux/cgroup.h>

/** Level rule: applies to all servers within level	*/
typedef struct qsup_level_rule_t {
  int level;				/**< Level to which rule applies	*/
  qos_bw_t max_bw;			/**< Maximum per-level bw		*/
  struct qsup_level_rule_t *next;	/**< Pointer to next qsup_level_rule_t	*/
} qsup_level_rule_t;

/** Group rule: applies to all users in the group	*/
typedef struct qsup_group_rule_t {
  int gid;				/**< Group Id of users to which rule applies	*/
  qsup_constraints_t constr;		/**< Constraints enforced for all group users	*/
  struct qsup_group_rule_t *next;	/**< Pointer to next qsup_group_rule_t		*/
} qsup_group_rule_t;

/** User rule: applies to one user only			*/
typedef struct qsup_user_rule_t {
  int uid;				/**< User Id of user to which rule applies	*/
  qsup_constraints_t constr;		/**< Constraints enforced for all group users	*/
  struct qsup_user_rule_t *next;	/**< Pointer to next qsup_user_rule_t		*/
} qsup_user_rule_t;

/** QoS Sup related data for each server */
typedef struct qsup_server_t {
  /* Statically configured data */
  int server_id;	/**< Unique ID of this server		*/
  int level;		/**< Level where this server resides	*/
  int weight;		/**< w.r.t. other servers in same level	*/
  qos_bw_t gua_bw;	/**< Minimum guaranteed requested	*/
  qos_bw_t max_user_bw;	/**< Maximum per-user total request	*/
  qos_bw_t max_level_bw;/**< Maximum per-level total request	*/
  int uid, gid;		/**< UID and GID of this server		*/

  /* Dynamically changing data */
  struct task_group *tg;
  qos_bw_t req_bw;	/**< Non-guaranteed required bandwidth		*/
  qos_bw_t used_gua_bw;	/**< Current guaranteed bandwidth to the server	*/
  qos_bw_t *p_level_sum;	/**< Total approved for level	*/
  qos_bw_t *p_user_req;		/**< Total request for user	*/
  qos_bw_t *p_level_req;	/**< Total request for level	*/
  long int *p_user_coeff;	/**< Coefficient for user	*/
  long int *p_level_coeff;	/**< Coefficient for level	*/
  qos_bw_t *p_user_gua;		/**< Total guaranteed for user	*/
  qos_bw_t *p_level_gua;	/**< Total guaranteed for level	*/
  struct qsup_server_t *next;	/**< Next qsup_server_t struct in global qsup_servers list */
} qsup_server_t;

/** Initialize the QSUP subsystem just after module load into the kernel. */
qos_rv qsup_init(void);

/** Cleanup the QSUP subsystem before unloading the module from kernel */
qos_rv qsup_cleanup(void);

//int qsup_add_level(int level_id, qos_bw_t max_level_bw);
//int qsup_set_level(int level_id, qos_bw_t max_level_bw);
//qos_bw_t qsup_get_level_bw(int level_id);

//int qsup_add_group(int group_id, qsup_constraints_t * rule);
//int qsup_set_group(int group_id, qsup_constraints_t * rule);
//qsup_constraints_t * qsup_get_group(int group_id);

//int qsup_add_user(int user_id, qsup_constraints_t * rule);
//int qsup_set_user(int user_id, qsup_constraints_t * rule);
//qsup_constraints_t * qsup_get_user(int user_id);

/** Add a level rule to the QSUP */
qos_rv qsup_add_level_rule(int level, qos_bw_t max_bw);

/** Add a group rule to the QSUP */
qos_rv qsup_add_group_constraints(int gid, qsup_constraints_t *constr);

/** Add a user rule to the QSUP */
qos_rv qsup_add_user_constraints(int uid, qsup_constraints_t *constr);

/** Create a new qsup_server_t structure
 *
 * @return	The server_id of the new server, if greater than or equal to zero,
 *		or a QOS_E_ error code, if negative
 */
qos_rv qsup_create_server(qsup_server_t **p_srv, int uid, int gid, qres_params_t *param);

qos_rv qsup_init_server(qsup_server_t *srv, int uid, int gid, qres_params_t *param);

/** Destroy a qsup_server_t and free associated resources.	*/
qos_rv qsup_destroy_server(qsup_server_t *srv);

qos_rv qsup_cleanup_server(qsup_server_t *srv);

/** Sets the required bandwidth for the specified server	*/
qos_rv qsup_set_required_bw(qsup_server_t *srv, qos_bw_t server_req);

/** Gets the required bandwidth for the specified server	*/
qos_bw_t qsup_get_required_bw(qsup_server_t *srv);

/** Gets the minimum granted bandwidth for the specified server	*/
qos_bw_t qsup_get_guaranteed_bw(qsup_server_t *srv);

/** Returns the bandwidth approved for the specified server	*/
qos_bw_t qsup_get_approved_bw(qsup_server_t *srv);

/** Returns the maximum guaranteed bandwidth for the specified user */
qos_bw_t qsup_get_max_gua_bw(int uid, int gid);

/** Returns in *p_avail_bw the available guaranteed bw for the specified user  */
qos_rv qsup_get_avail_gua_bw(int uid, int gid, qos_bw_t *p_avail_bw);

/** Returns in *p_avail_bw the available bw for the specified user  */
qos_rv qsup_get_avail_bw(int uid, int gid, qos_bw_t *p_avail_bw);

/** Dumps into the log system the QSUP server complete state	*/
void qsup_dump(void);

/** Find the constraints in force for the supplied uid/gid pair */
qsup_constraints_t *qsup_find_constr(int uid, int gid);

/** Set the spare bandwidth for admission control purposes	*/
qos_rv qsup_reserve_spare(qos_bw_t spare_bw);

/** @} */

#endif
