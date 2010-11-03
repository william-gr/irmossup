#include "qres_config.h"
//#define QOS_DEBUG_LEVEL QSUP_MOD_DEBUG_LEVEL
#include "qos_debug.h"

#ifdef PROF_QSUP_MOD
#  define QOS_PROFILE
#endif
#include "qos_prof.h"

#include "qsup.h"

#include "qos_memory.h"
#include "qos_ul.h"

/** @addtogroup QSUP
 * @{
 *
 */

/** @file
 *
 * @brief	Implementation of the QoS Supervisor module.
 *
 * @todo	Add synchronization primitives.
 * @todo	After a while the QSUP goes on, the truncations in computations
 *		might lead to very bad things. A fix could be a periodic/sporadic
 *		recomputation of all values, including partials and totals.
 * @todo	All globals should be placed inside a structure, to be instantiated
 *		one time for each RR-enabled processor, for SMP systems.
 */

qsup_group_rule_t *group_rules = 0;
int num_group_rules = 0;

qsup_user_rule_t *user_rules = 0;
int num_user_rules = 0;

qos_bw_t spare_bw = 0;

/** Bandwidth coefficients are stored as fixed-point integers.	*/
typedef long int qsup_coeff_t;

/** Number of binary decimal digits in qsup_coeff_t	*/
#define QSUP_COEFF_BITS 16

/** This corresponds to a qsup_coeff_t of 1.0		*/
#define QSUP_COEFF_ONE (1ul << QSUP_COEFF_BITS)

/** Scale (multiply) a bandwidth value by a coefficient	*/
//#define coeff_apply(a, b) ( ((a) * (b)) >> QSUP_COEFF_BITS )
#define coeff_apply(a, b) ((unsigned long) ul_mul_shr((__u32) (a), (__u32) (b), QSUP_COEFF_BITS) )

/** Compute a coefficient value as the equivalent operation on reals: a/b	*/
//#define coeff_compute(a, b) ( (((qsup_coeff_t) (a)) << QSUP_COEFF_BITS) / (b) )
#define coeff_compute(a, b) ((unsigned long) ul_shl_div((__u32) (a), QSUP_COEFF_BITS, (__u32) (b)) )

#define MAX_NUM_LEVELS 2

static qsup_constraints_t default_constraint = {
  .level = 0,
  .weight = 1,
  .max_bw = U_LUB,
  .max_min_bw = U_LUB,
  .flags_mask = 0x00000000
};

/** Head of global list of created qsup_servers.	*/
qsup_server_t *qsup_servers;

/** Id assigned to the next created qsup_server_t	*/
int next_server_id;

/** QoS Sup related data for each user	*/
typedef struct qsup_user_t {
  int uid;			/**< UID of user			*/
  qos_bw_t user_req;		/**< Sum of all (saturated) requests	*/
  qsup_coeff_t user_coeff;	/**< Used when user_req > max_user_bw	*/
  qos_bw_t user_gua;		/**< Sum of all guaranteed minimums	*/
  qos_bw_t user_used_gua;	/**< Sum of actually used guaranteed min*/
  struct qsup_user_t *next;	/**< Pointer to next item in list	*/
} qsup_user_t;

/** QoS Sup related data for each level	*/
typedef struct qsup_level_t {
  qos_bw_t level_max;		/**< Maximum bw allowed per-level	*/
  qos_bw_t level_req;		/**< Total requested per-level		*/
  qos_bw_t level_sum;		/**< Total approved per-level		*/
  qsup_coeff_t level_coeff;	/**< Level coefficient			*/
  qos_bw_t level_gua;		/**< Total guaranteed bw per-level	*/
} qsup_level_t;

/** User related data	*/
static qsup_user_t *qsup_users;
/** Level related data	*/
static qsup_level_t qsup_levels[MAX_NUM_LEVELS];

/** Sum of guaranteed bandwidths of accepted servers	*/
static qos_bw_t tot_gua_bw = 0;
/** Sum of actually used guaranteed bw by all servers	*/
static qos_bw_t tot_used_gua_bw = 0;

/** Global QSUP coefficients lock
 * @todo  use one for each CPU ? */
/* spinlock_t qsup_lock __cacheline_aligned = SPIN_LOCK_UNLOCKED; */

/** Lock QSup coefficients for reading */
static inline void qsup_lock_coeffs_read(unsigned long *p_flags) {
  /* spin_lock_irqsave(&qsup_lock, *p_flags); */
  /** @todo  evaluate if using read_lock_irqsave() is worth for anything */
}

/** Unlock QSup coefficients for reading */
static inline void qsup_unlock_coeffs_read(unsigned long flags) {
  /* spin_unlock_irqrestore(&qsup_lock, flags); */
}

/** Lock QSup coefficients for writing */
static inline void qsup_lock_coeffs_write(unsigned long *p_flags) {
  /* spin_lock_irqsave(&qsup_lock, *p_flags); */
}

/** Unlock QSup coefficients for writing */
static inline void qsup_unlock_coeffs_write(unsigned long flags) {
  /* spin_unlock_irqrestore(&qsup_lock, flags); */
}

static inline qos_bw_t bw_min(qos_bw_t a, qos_bw_t b) { return ((a < b) ? (a) : (b)); }

qos_rv qsup_add_level_rule(int level, qos_bw_t max_bw) {
  if (level < 0 || level >= MAX_NUM_LEVELS)
    return QOS_E_INVALID_PARAM;

  /* Make new rule active */
  qsup_levels[level].level_max = bw_min(max_bw, U_LUB);

  return QOS_OK;
}

qos_rv qsup_add_group_constraints(int gid, qsup_constraints_t *constr) {
  qsup_group_rule_t *rule = qos_create(qsup_group_rule_t);
  if (rule == 0)
    return QOS_E_NO_MEMORY;
  rule->gid = gid;
  rule->constr = *constr;
  /* Insert new rule at head of group_rules list */
  rule->next = group_rules;
  group_rules = rule;
  /* Increment group_rule counter       */
  num_group_rules++;
  return QOS_OK;
}

qos_rv qsup_add_user_constraints(int uid, qsup_constraints_t *constr) {
  qsup_user_rule_t *rule = qos_malloc(sizeof(qsup_user_rule_t));
  if (rule == 0)
    return QOS_E_NO_MEMORY;
  rule->uid = uid;
  rule->constr = *constr;
  /* Insert new rule at head of user_rules list */
  rule->next = user_rules;
  user_rules = rule;
  /* Increment user_rule counter        */
  num_user_rules++;
  return QOS_OK;
}

qos_rv qsup_init() {
  int l;
  group_rules = 0;
  num_group_rules = 0;
  user_rules = 0;
  num_user_rules = 0;
  qsup_servers = 0;
  next_server_id = 0;

  for (l=0; l<MAX_NUM_LEVELS; l++) {
    qsup_levels[l].level_coeff = QSUP_COEFF_ONE;
    qsup_levels[l].level_max = U_LUB;
  }
  tot_gua_bw = 0;
  tot_used_gua_bw = 0;

  return QOS_OK;
}

qos_rv qsup_cleanup() {
  qsup_server_t *srv = qsup_servers;
  qsup_user_t *usr = qsup_users;

  /* Cleanup qsup_server_t */
  while (srv != 0) {
    qsup_server_t *tmp = srv;
    srv = srv->next;
    qos_free(tmp);
  }
  /* Cleanup qsup_user_t */
  while (usr != 0) {
    qsup_user_t *tmp = usr;
    usr = usr->next;
    qos_free(tmp);
  }
  /* Cleanup group rules */
  while (group_rules != 0) {
    qsup_group_rule_t *tmp = group_rules;
    group_rules = group_rules->next;
    qos_free(tmp);
  }
  /* Cleanup user rules */
  while (user_rules != 0) {
    qsup_user_rule_t *tmp = user_rules;
    user_rules = user_rules->next;
    qos_free(tmp);
  }
  return QOS_OK;
}

/* Find appropriate qsup_constraints_t for specified uid/gid.
 *
 * First, search for a user-rule matching the specified uid.
 * Then, search for a group-rule matching the specified gid
 * (i.e. user-rules override group-rules). If no rules match,
 * then return a pointer to the default_constraint.
 *
 * @return	Pointer to the matching qsup_constraints_t.
 */
qsup_constraints_t *qsup_find_constr(int uid, int gid) {
  qsup_user_rule_t *urule = user_rules;
  qsup_group_rule_t *grule;
  /* First, search user-rules	*/
  while (urule != 0)
    if (urule->uid == uid)
      return &urule->constr;
    else
      urule = urule->next;
  /* No matching user-rule found. Search group-rules	*/
  grule = group_rules;
  while (grule != 0)
    if (grule->gid == gid)
      return &grule->constr;
    else
      grule = grule->next;
  /* No matching group-rule found. Return default const	*/
  return &default_constraint;
}

/** Return qsup_server_t structure for specified server_id,
 * or zero if not found */
qsup_server_t *qsup_find_server_by_id(int server_id) {
  qsup_server_t *srv = qsup_servers;
  while ((srv != 0) && (srv->server_id != server_id))
    srv = srv->next;
  return srv;
}

/** Retrieve a qsup_user_t structure for the specified uid.
 * If structure does not exist, create it and fill static
 * parameters from the user and group rules.
 */
qos_rv get_user_info(qsup_user_t **pp, int uid) {
  qsup_user_t *usr = qsup_users;
  while ((usr != 0) && (usr->uid != uid))
    usr = usr->next;
  if (usr == 0) {
    /** Not found: create a new qsup_user_t */
    usr = qos_malloc(sizeof(qsup_user_t));
    if (usr == 0)
      return QOS_E_NO_MEMORY;
    /** Add to head of qsup_users list */
    usr->next = qsup_users;
    qsup_users = usr;
    /** Fill in static data ?!? */
    usr->uid = uid;
    usr->user_req = 0;
    usr->user_gua = 0;
    usr->user_used_gua = 0;
    usr->user_coeff = QSUP_COEFF_ONE;
  }
  *pp = usr;
  return QOS_OK;
}

qos_bw_t qsup_get_max_gua_bw(int uid, int gid) {
  qsup_constraints_t *constr;
  constr = qsup_find_constr(uid, gid);
  return constr->max_min_bw;
}

qos_rv qsup_get_avail_gua_bw(int uid, int gid, qos_bw_t *p_avail_bw) {
  qos_rv rv = QOS_OK;
  qsup_constraints_t *constr;
  qsup_user_t *usr;

  constr = qsup_find_constr(uid, gid);
  rv = get_user_info(&usr, uid);
  qos_chk_go_msg(rv == QOS_OK, end, "get_user_info() failed");
  *p_avail_bw = constr->max_min_bw - usr->user_gua;

end:

  return rv;
}

qos_rv qsup_get_avail_bw(int uid, int gid, qos_bw_t *p_avail_bw) {
  qos_rv rv = QOS_OK;
  qsup_constraints_t *constr;
  qsup_user_t *usr;

  constr = qsup_find_constr(uid, gid);
  rv = get_user_info(&usr, uid);
  qos_chk_go_msg(rv == QOS_OK, end, "get_user_info() failed");
  *p_avail_bw = constr->max_bw - usr->user_req;

end:

  return rv;
}

/** Initialize a new qsup_server_t structure. **/
qos_rv qsup_init_server(qsup_server_t *srv, int uid, int gid, qres_params_t *param) {
  qsup_user_t *usr;
  qsup_constraints_t *constr;
  qos_bw_t new_tot_gua;
  qos_bw_t min_bw;

  min_bw = r2bw_ceil(param->Q_min, param->P);

  /** @todo  lock qsup_servers list ? */
  qos_log_debug("Adding server: uid=%d gid=%d min_bw=%ld", uid, gid, min_bw);
  constr = qsup_find_constr(uid, gid);

  if (param->flags & constr->flags_mask) {
    qos_log_err("Required flags violates configured mask for user/group");
    return QOS_E_UNAUTHORIZED;
  }

  if (min_bw > constr->max_min_bw) {
    qos_log_err("Minimum guaranteed requested violates max_min");
    /* @todo  should we allow saturation policy instead of reject ? */
    return QOS_E_UNAUTHORIZED;
  }

  /* Schedulability test: \sum min_bw_i <= U_LUB - spare_bw */
  new_tot_gua = tot_gua_bw + min_bw;
  if (new_tot_gua > U_LUB - spare_bw) {
    qos_log_err("New guaranteed task rejected");
    return QOS_E_SYSTEM_OVERLOAD;
  }

  qos_chk_ok_ret(get_user_info(&usr, uid));

  if (usr->user_gua + min_bw > U_LUB - spare_bw) {
    qos_log_err("Minimum guaranteed requested by all user apps violates U_LUB - spare_bw");
    return QOS_E_SYSTEM_OVERLOAD;
  }

  if (usr->user_gua + min_bw > constr->max_min_bw) {
    qos_log_err("Minimum guaranteed requested by all user apps violates max_min");
    return QOS_E_UNAUTHORIZED;
  }

  srv->server_id = next_server_id++;
  srv->tg = NULL;
  srv->level = constr->level;
  srv->weight = constr->weight;
  srv->max_user_bw = constr->max_bw;
  srv->max_level_bw = qsup_levels[srv->level].level_max;
  srv->uid = uid;
  srv->gid = gid;
  srv->req_bw = 0;
  srv->gua_bw = min_bw;
  srv->used_gua_bw = 0;		/**< Guaranteed minimum not used yet */
  srv->p_level_sum   = &qsup_levels[srv->level].level_sum;
  srv->p_level_req   = &qsup_levels[srv->level].level_req;
  srv->p_level_coeff = &qsup_levels[srv->level].level_coeff;
  srv->p_level_gua   = &qsup_levels[srv->level].level_gua;
  srv->p_user_req = &usr->user_req;
  srv->p_user_coeff = &usr->user_coeff;
  srv->p_user_gua = &usr->user_used_gua;

  /* Add to head of qsup_servers list */
  srv->next = qsup_servers;
  qsup_servers = srv;

  /** Update sum of guaranteed bw to all servers */
  tot_gua_bw = new_tot_gua;

  return QOS_OK;
}

qos_rv qsup_create_server(qsup_server_t **pp, int uid, int gid, qres_params_t *param) {
  qsup_server_t *qsup;
  qos_rv rv;
  qsup = qos_create(qsup_server_t);
  if (qsup == 0)
    return QOS_E_NO_MEMORY;
  rv = qsup_init_server(qsup, uid, gid, param);
  if (rv != QOS_OK) {
    qos_log_err("qsup_init_server() failed: %s", qos_strerror(rv));
    qos_free(qsup);
    return rv;
  }
  *pp = qsup;
  return QOS_OK;
}

qos_rv qsup_cleanup_server(qsup_server_t *srv) {
  qsup_server_t *tmp = qsup_servers;
  prof_vars;

  prof_func();

  if (tmp == 0)
    prof_return(QOS_E_INCONSISTENT_STATE);
  /* In order to correctly update partials, set request to zero
   * before destroying server	*/
  if (srv->req_bw != 0) {
    qos_rv err = qsup_set_required_bw(srv, 0);
    if (err != QOS_OK)
      prof_return(err);
  }

  /* The only total that has not been updated is gua_bw */
  tot_gua_bw -= srv->gua_bw;

  /* Remove srv from list	*/
  if (srv == qsup_servers)
    qsup_servers = srv->next;
  else {
    while ((tmp->next != 0) && (tmp->next != srv))
      tmp = tmp->next;
    if (tmp->next == 0)
      prof_return(QOS_E_INVALID_PARAM);
    tmp->next = srv->next;
  }

  prof_end();

  return QOS_OK;
}

qos_rv qsup_destroy_server(qsup_server_t *srv) {
  qos_rv rv = qsup_cleanup_server(srv);
  /* Free descriptor pointed to by srv  */
  qos_free(srv);
  return rv;
}

qos_rv qsup_set_required_bw(qsup_server_t *srv, qos_bw_t server_req) {
  qos_bw_t user_req;	/* New requested total per-user		*/
  qos_bw_t level_req;	/* New requested total per-level	*/
  int l;
  qos_bw_t avail_bw;
  qos_bw_t used_gua_bw;
  prof_vars;

  qos_log_debug("Changing required bw of server %d from " QOS_BW_FMT " to " QOS_BW_FMT,
		srv->server_id, srv->req_bw, server_req);

  prof_func();

  /* Check violation of single-process max_bw. If server_req > max,
   * then saturate request. @todo  should we reject request ? */
  if (server_req > srv->max_user_bw) {
    qos_log_debug("Saturating request from " QOS_BW_FMT " to " QOS_BW_FMT,
		  server_req, srv->max_user_bw);
    server_req = srv->max_user_bw;
  }

  /* First, compute new minimum guaranteed if requested */
  used_gua_bw = bw_min(server_req, srv->gua_bw);
  /* Then, update affected guaranteed partials		*/
  *(srv->p_user_gua)	+= used_gua_bw - srv->used_gua_bw;
  *(srv->p_level_gua)	+= used_gua_bw - srv->used_gua_bw;
  tot_used_gua_bw	+= used_gua_bw - srv->used_gua_bw;
  /* Finally, update new minimum guaranteed		*/
  srv->used_gua_bw = used_gua_bw;

  /* Compute updated sum of per-user requests	*/
  user_req = *(srv->p_user_req) - srv->req_bw + server_req;

  qos_log_debug("Updated sum of user-reqs: " QOS_BW_FMT, user_req);

  /* Check violation of per-user max_bw while	*
   * updating user-compression coefficient	*/
  if (user_req > srv->max_user_bw) {
    qos_log_debug("Rescaling per-user request of " QOS_BW_FMT " to max=" QOS_BW_FMT,
		  user_req, srv->max_user_bw);
    *(srv->p_user_coeff) = coeff_compute(srv->max_user_bw - *(srv->p_user_gua), user_req - *(srv->p_user_gua));
  } else {
#ifndef QSUP_EXPAND
    *(srv->p_user_coeff) = QSUP_COEFF_ONE;
#else
    /* A request of zero cannot be expanded */
    if (user_req - *(srv->p_user_gua) == 0)
      *(srv->p_user_coeff) = QSUP_COEFF_ONE;
    else
      *(srv->p_user_coeff) = coeff_compute(srv->max_user_bw - *(srv->p_user_gua), user_req - *(srv->p_user_gua));
#endif
  }

  /* Compute new request for the level */
  level_req = (*(srv->p_level_req)) - bw_min(*(srv->p_user_req), srv->max_user_bw) + bw_min(user_req, srv->max_user_bw);

  /* Update required bw for server, user and level */
  srv->req_bw = server_req;
  *(srv->p_user_req) = user_req;
  *(srv->p_level_req) = level_req;

  //qos_log_debug("Server %d requirements: srv=%ld, usr=%ld, lev=%ld",
  //	srv->server_id, srv->req_bw, user_req, level_req);

  /* level_req is the new required bw for level, which must be
   * compared with available residual bw from higher priority levels;
   * change in minimum of one level could potentially affect all
   * higher and lower levels, so we need to go through all of them.
   */
  avail_bw = U_LUB;
  for (l=0; l<MAX_NUM_LEVELS; l++) {
    qsup_level_t *lev = &qsup_levels[l];
    //qos_log_debug("Level %d: avail_bw=%ld", l, avail_bw);
    /* Will get the actually assigned bw to the level */
    qos_bw_t assigned;
    /* Actual level bw is saturated with maximum configured per-level
     * and maximum available for the level and all lower-priority ones	*/
    assigned = bw_min(bw_min(lev->level_req, lev->level_max), avail_bw);
    //qos_log_debug("Level %d: Assigned=%ld", l, assigned);

    /* Update actually assigned bw to the level	*/
    lev->level_sum = assigned;
    //qos_log_debug("Level %d: req=%ld, gua=%ld, sum=%ld",
    //	  l, lev->level_req, lev->level_gua, lev->level_sum);
    /* Prevent division-by-zero on empty levels */
    if (lev->level_req > lev->level_gua) {
      //qos_log_debug("Level %d: Dividing by %ld", l, lev->level_req - lev->level_gua);
      lev->level_coeff = coeff_compute(assigned - lev->level_gua, lev->level_req - lev->level_gua);
    } else
      lev->level_coeff = QSUP_COEFF_ONE;
    /* Update available bandwidth for next level */
    avail_bw -= assigned;
  }

  prof_end();

  return QOS_OK;
}

qos_bw_t qsup_get_required_bw(qsup_server_t *srv) {
  return srv->req_bw;
}

qos_bw_t qsup_get_guaranteed_bw(qsup_server_t *srv) {
  return srv->gua_bw;
}

void qsup_dump(void) {
  int l;
  qsup_user_t *usr;
  qsup_server_t *srv;

  qos_log_debug("Current user coefficients:");
  for (usr = qsup_users; usr != 0; usr = usr->next)
    qos_log_debug("User %d: coeff=%lu/1000", usr->uid, coeff_apply(usr->user_coeff, 1000));

  qos_log_debug("Current level coefficients:");
  for (l = 0; l < MAX_NUM_LEVELS; l++)
    qos_log_debug("Level %d: coeff=%lu/1000", l, coeff_apply(qsup_levels[l].level_coeff, 1000));

  qos_log_debug("Current list of servers:");
  for (srv = qsup_servers; srv != 0; srv = srv->next) {
    qos_log_debug("Server %d: lev=%d req=" QRES_TIME_FMT "/1000 eff=" QRES_TIME_FMT "/1000",
		  srv->server_id, srv->level,
		  bw2Q(srv->req_bw, 1000),
		  /* Compute actual bandwidth using the user and level coefficients	*/
		  bw2Q(qsup_get_approved_bw(srv), 1000));
  }
}

qos_bw_t qsup_get_approved_bw(qsup_server_t *srv) {
  qos_bw_t bw;
  qsup_coeff_t c1, c2;
  prof_vars;

  prof_func();
  bw = srv->used_gua_bw;
  c1 = *(srv->p_level_coeff);
  c2 = *(srv->p_user_coeff);

  bw += coeff_apply(coeff_apply(srv->req_bw - srv->used_gua_bw, c2), c1);
  prof_return(bw);
}

/** Cannot reserve more than U_LUB as spare, and cannot change the
 ** reserved spare when servers are already active.
 **/
qos_rv qsup_reserve_spare(qos_bw_t bw) {
  if (bw > U_LUB)
    return QOS_E_INVALID_PARAM;
  if (qsup_servers != NULL)
    return QOS_E_INCONSISTENT_STATE;
  spare_bw = bw;

  return QOS_OK;
}

/** @} */
