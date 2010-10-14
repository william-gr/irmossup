#ifndef __QSUP_GW_KS_H__
#define __QSUP_GW_KS_H__

/** @addtogroup QSUP_MOD
 * @{
 */

/** @file
 * @brief	QSUP UserSpace to KernelSpace gateway common interface.
 *
 * @todo	Use custom lock in addition to generic_scheduler_lock ?.
 */

#include "qsup_gw.h"
#include "qsup.h"

#include <linux/aquosa/kal_sched.h>

/** UID of the user authorized to create the default server	*/
#define QSUP_DEFAULT_SRV_UID 0
/** GID of the group authorized to create the default server	*/
#define QSUP_DEFAULT_SRV_GID 0

/** Check authorizations for requested operation.
 *
 * Operations on QSUP rules may only be done by root.
 *
 * @param iparams	Ptr to the iparams struct, in kernel-space.
 *
 * @return		1 on success, 0 otherwise.
 */
static inline int qsup_authorize_op(qsup_iparams_t *iparams) {
  return kal_get_current_uid() == 0;
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
qos_rv qsup_gw_ks(qsup_op_t op, void __user *up_iparams, unsigned long size);

/** Initialize QSUP	*/
qos_rv qsup_init_ks(void);

/** Cleanup QSUP module	*/
qos_rv qsup_cleanup_ks(void);

/** Initialize the QSUP default server */
qos_rv qsup_init_default_server(qsup_server_t *default_qsup, qres_time_t Q, qres_time_t Q_min, qres_time_t P);

/** @} */

#endif
