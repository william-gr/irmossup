#ifndef __QRES_GW_KS_H__
#define __QRES_GW_KS_H__

/** @addtogroup QRES_MOD
 * @{
 */

/** @file
 * @brief	QRES UserSpace to KernelSpace gateway common interface.
 *
 * @todo	Use custom lock in addition to generic_scheduler_lock ?.
 */

#include "qres_interface.h"
#include "qos_kernel_dep.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/sched.h>

qos_rv qres_get_exec_abs_time(qres_server_t *qres, qres_time_t *exec_time, qres_atime_t *abs_time);

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
qos_rv qres_gw_ks(qres_op_t op, void __user *up_iparams, unsigned long size);

/** @} */

#endif
