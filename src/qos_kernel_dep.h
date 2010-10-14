#ifndef __QOS_KERNEL_DEP_H__
#define __QOS_KERNEL_DEP_H__

#if ! (defined(QOS_KS) && defined(__KERNEL__))
#  error "Cannot include qos_kernel_dep.h in user-space"
#endif

/** @file
 * @brief Portability across different kernel versions.
 *
 * Purpose of this file is to provide a set of macros and static
 * inline functions which allow module code to be compiled with
 * different kernel versions.
 */

//#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

#include <linux/aquosa/qos_debug.h>

#if LINUX_VERSION_CODE<KERNEL_VERSION(2,6,0)

/* Stuff that is 2.4.x compliant */

/** Increment module usage counter within the kernel */
#  define KERN_INCREMENT MOD_INC_USE_COUNT
/** Decrement module usage counter within the kernel */
#  define KERN_DECREMENT MOD_DEC_USE_COUNT
/* Support 2.6.x style user-space pointer declarations */
#  define __user

#else /* LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,0) */

/* Stuff that is 2.6.x compliant */

#include <linux/cdev.h>
#include <linux/kobject.h>

/** Increment module usage counter within the kernel */
#  define KERN_INCREMENT try_module_get(THIS_MODULE)
/** Decrement module usage counter within the kernel */
#  define KERN_DECREMENT module_put(THIS_MODULE)

#endif

/** Kernel-dependent information about a registered virtual device */
typedef struct {
  dev_t dev_num;
  char *dev_name;
#if LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,0)
  struct cdev cdev;
#endif
} qos_dev_info_t;

/** kernel-dependent registration of a virtual device	*/
qos_rv qos_dev_register(
  qos_dev_info_t *dev_info,
  char *dev_name, int def_major,
  struct file_operations *Fops
);

/** kernel-dependent registration of a virtual device	*/
qos_rv qos_dev_unregister(qos_dev_info_t *dev_info);

#endif
