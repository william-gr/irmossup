#include "qres_config.h"
#define QOS_DEBUG_LEVEL QSUP_MOD_DEBUG_LEVEL
#include "qos_debug.h"

#ifdef PROF_QSUP_MOD
#  define QOS_PROFILE
#endif
#include "qos_prof.h"

#include "qsup_gw_ks.h"
#include "qsup.h"
#include "kal_sched.h"
#include "qos_memory.h"
#include "rres_interface.h"

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/sched.h>

#define QSUP_DEFAULT_SRV_MAX_BW r2bw(45000, 50000)
#define QSUP_DEFAULT_SRV_MAX_MIN_BW r2bw(40000, 50000)

qos_rv qsup_init_ks(void) {
  qos_log_debug("Starting function");

  qos_chk_ok_ret(qsup_init());

  qos_log_debug("Ending function");
  return QOS_OK;
}

qos_rv qsup_cleanup_ks(void) {
  return qsup_cleanup();
}

/** Main US-to-KS gateway function.
 *
 * Copies parameters from US to KS, checks if requested operation is
 * allowed by the calling process, distinguishes among various kinds
 * of requests, calling appropriate per-request functions. If needed,
 * copies return parameters back from KS to US.
 *
 * @todo  avoid copy_from_user and copy_to_user with entire iparams struct
 *        when unneeded.
 */
qos_rv qsup_gw_ks(qsup_op_t op, void __user *up_iparams, unsigned long size) {
  qsup_iparams_t iparams;
  qos_rv err = QOS_OK;
  kal_irq_state flags;

  qos_log_debug("Starting qsup_gw_ks()");

  if (size != sizeof(qsup_iparams_t)) {
    qos_log_err("Wrong size");
    return QOS_E_INTERNAL_ERROR;
  }

  if (copy_from_user(&iparams, up_iparams, sizeof(qsup_iparams_t)))
    return QOS_E_INVALID_PARAM;

  kal_spin_lock_irqsave(rres_get_spinlock(), &flags);

  if (! qsup_authorize_op(&iparams)) {
    kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);
    return QOS_E_UNAUTHORIZED;
  }

  switch (op) {
  case QSUP_OP_ADD_LEVEL_RULE:
    err = qsup_add_level_rule(iparams.u.level_rule.level_id, iparams.u.level_rule.max_level_bw);
    break;
  case QSUP_OP_ADD_GROUP_RULE:
    err = qsup_add_group_constraints(iparams.u.group_rule.gid,
				     &iparams.u.group_rule.constr);
    break;
  case QSUP_OP_ADD_USER_RULE:
    err = qsup_add_group_constraints(iparams.u.user_rule.uid,
				     &iparams.u.user_rule.constr);
  case QSUP_OP_FIND_CONSTR:
    iparams.u.found_rule.constr = 
      *(qsup_find_constr(iparams.u.found_rule.uid, iparams.u.found_rule.gid));
    kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);
    if (copy_to_user(up_iparams, &iparams, sizeof(qsup_iparams_t)))
      err = QOS_E_INTERNAL_ERROR;
    return err;
    break;
  case QSUP_OP_GET_AVAIL_GUA_BW:
    err = qsup_get_avail_gua_bw(iparams.u.avail.uid, iparams.u.avail.gid,
                                  &iparams.u.avail.avail_gua_bw);
    if (err == QOS_OK) {
      kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);
      if (copy_to_user(up_iparams, &iparams, sizeof(qsup_iparams_t)))
        err = QOS_E_INTERNAL_ERROR;
      return err;
    }
    break;
  case QSUP_OP_RESERVE_SPARE:
    err = qsup_reserve_spare(iparams.u.spare_bw);
    break;
  default:
    qos_log_err("Unhandled operation code");
    err = QOS_E_INTERNAL_ERROR;	/* For debugging purposes */
  }

  kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);
  return err;
}
