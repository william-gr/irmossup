#include "qos_kernel_dep.h"

#include "qos_debug.h"
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>

#if LINUX_VERSION_CODE<KERNEL_VERSION(2,6,0)

/* Stuff that is 2.4.x compliant */
int qos_dev_register(
  qos_dev_info_t *dev_info,
  char *dev_name, int def_major,
  struct file_operations *Fops)

/* int qos_dev_register( */
/*   char *dev_name, int def_major, */
/*   struct file_operations *Fops, qos_dev_info_t *dev_info) */
{
  int ret_val;

  dev_info->dev_name = dev_name;

  ret_val = register_chrdev(def_major, dev_name, Fops);

  /* 
   * Negative values signify an error 
   */
  if (ret_val < 0) {
    printk("Sorry, registering the character device failed with %d\n", ret_val);
    return QOS_E_GENERIC;
  }

  dev_info->dev_num = MKDEV(def_major, 0);
  return QOS_OK;
}

int qos_dev_unregister(qos_dev_info_t *dev_info) {
  int ret;

  //ret = unregister_chrdev(MAJOR(dev_info->dev_num), dev_info->dev_name);
  ret = unregister_chrdev(dev_info->dev_num, dev_info->dev_name);
  if (ret < 0) {
    qos_log_crit("Error in module_unregister_chrdev: %d\n", ret);
    return QOS_E_GENERIC;
  }
  return QOS_OK;
}

#else

/* Stuff that is 2.6.x compliant */

qos_rv qos_dev_register(
  qos_dev_info_t *dev_info,
  char *dev_name, int def_major,
  struct file_operations *Fops)
{
  int ret_val;

  dev_info->dev_name = dev_name;

  ret_val = alloc_chrdev_region(&dev_info->dev_num, 0, 1, dev_name);
  if (ret_val < 0) {
    printk("Allocation of device major failed with %d\n", ret_val);
    return QOS_E_GENERIC;
  }

  cdev_init(&dev_info->cdev, Fops);
  kobject_set_name(&dev_info->cdev.kobj, "%s", dev_name);
  dev_info->cdev.owner = THIS_MODULE;

  ret_val = cdev_add(&dev_info->cdev, dev_info->dev_num, 1);
  if (ret_val < 0) {
    printk("Registration of cdev struct failed with %d\n", ret_val);
    return QOS_E_GENERIC;
  }

  return QOS_OK;
}

qos_rv qos_dev_unregister(qos_dev_info_t *dev_info) {
  cdev_del(&dev_info->cdev);
  unregister_chrdev_region(dev_info->dev_num, 1);
  return QOS_OK;
}

#endif

EXPORT_SYMBOL_GPL(qos_dev_register);
EXPORT_SYMBOL_GPL(qos_dev_unregister);
