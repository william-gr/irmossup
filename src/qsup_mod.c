#include "qres_config.h"
#define QOS_DEBUG_LEVEL QSUP_MOD_DEBUG_LEVEL
#include "qos_debug.h"

#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/version.h>	/* For KERNEL_VERSION macro */
#include <linux/module.h>	/* Specifically, a module */
#include <linux/fs.h>
#include <asm/uaccess.h>	/* for get_user and put_user */
#include <linux/proc_fs.h>

#include "qsup_mod.h"
#include "qres_gw_ks.h"
#include "qsup_gw_ks.h"
#include "qos_kernel_dep.h"
#include "rres_interface.h"

#include "rres_proc_fs.h"

#define SUCCESS 0
#define BUF_LEN 80
#define DEBUG

/**
 * Is the device open right now? Used to prevent
 * concurent access into the same device 
 */
static int qsup_Device_Open = 0;

/* 
 * This is called whenever a process attempts to open the device file 
 */
static int qsup_device_open(struct inode *inode, struct file *file)
{
  qos_log_debug("qsup_device_open(%p)", file);
  qsup_Device_Open++;
  KERN_INCREMENT;
  return SUCCESS;
}

/** Called when device is closed by a process.
 *
 * @todo 	Either force destroy_server() to be called here,
 *		or intercept kill hook for handling premature process end.
 */
static int qsup_device_release(struct inode *inode, struct file *file)
{
  qos_log_debug("qsup_device_release(%p,%p)", inode, file);
  qsup_Device_Open--;
  KERN_DECREMENT;
  return SUCCESS;
}

/* 
 * This function is called whenever a process which has already opened the
 * device file attempts to read from it.
 */
static ssize_t qsup_device_read(struct file *file,	/* see include/linux/fs.h   */
			   char __user * buffer,	/* buffer to be
							 * filled with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
  qos_log_debug("qsup_device_read(%p,%p,%zu)", file, buffer, length);
  /* 
   * Read functions are supposed to return the number
   * of bytes actually inserted into the buffer 
   */
  return 0;
}

/* 
 * This function is called when somebody tries to
 * write into our device file. 
 */
static ssize_t
qsup_device_write(struct file *file,
	     const char __user * buffer, size_t length, loff_t * offset)
{
  qos_log_debug("qsup_device_write(%p,%s,%zu)", file, buffer, length);
  /* 
   * Again, return the number of input characters used 
   */
  return 0;
}

/* 
 * This function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * If the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 *
 */
int qsup_device_ioctl(struct inode *inode,	/* see include/linux/fs.h */
		 struct file *file,	/* ditto */
		 unsigned int ioctl_num,/* number and param for ioctl */
		 unsigned long ioctl_param)
{
  qos_rv rv;
  qos_log_debug("Starting function");
  /* SUCCESS is zero, error is negative, same as QOS_x error codes	*/
  rv = qsup_gw_ks(_IOC_NR(ioctl_num), (void *) ioctl_param,
		   _IOC_SIZE(ioctl_num));
  qos_log_debug("Returning %d (%s)", qos_rv_int(rv), qos_strerror(rv));
  return qos_rv_int(rv);
}

struct file_operations qsup_Fops = {
	.read = qsup_device_read,
	.write = qsup_device_write,
	.ioctl = qsup_device_ioctl,
	.open = qsup_device_open,
	.release = qsup_device_release,	/* a.k.a. close */
};

static qos_dev_info_t qsup_dev_info;

/** Device registration should be done when kernel module is
 ** already able to accept ioctl requests
 **/
qos_rv qsup_dev_register(void) {
  qos_rv rv;
  //if ((rv = qos_dev_register(&qsup_dev_info, QSUP_DEV_NAME, QSUP_MAJOR_NUM, &qsup_Fops)) != QOS_OK) {
  //  qos_log_err("Registration of device %s failed", QSUP_DEV_NAME);
  //  return rv;
  //}

  qos_log_info("Registered QSUP device with major device number %d.", MAJOR(qsup_dev_info.dev_num));
  qos_log_info("If you want to talk to the device driver,");
  qos_log_info("you'll have to create a device file. ");
  qos_log_info("We suggest you use:");
  qos_log_info("mknod %s c %d 0", QSUP_DEV_NAME, MAJOR(qsup_dev_info.dev_num));

  return QOS_OK;
}

/** 
 ** Initialize the module - Register the character device 
 **
 ** @note Unused, so far.
 **/
int qsup_init_module(void) {
  qos_rv rv;

  // spin_lock

  qos_log_debug("Initing QSUP");
  rv = qsup_init_ks();
  if (rv != QOS_OK) {
    qos_log_crit("qsup_init() failed: %s", qos_strerror(rv));
    return -1;
  }

  // spin_unlock

  qsup_dev_register();

  return 0;
}

/** 
 ** Unregister the device 
 **/
qos_rv qsup_dev_unregister(void) {
  //return qos_dev_unregister(&qsup_dev_info);
}

/** 
 ** Cleanup - unregister the appropriate file from /proc 
 **
 ** @note Unused, so far.
 **/
void qsup_cleanup_module(void) {
  qos_rv rv;

  // spin_lock

  rv = qsup_cleanup();
  if (rv != QOS_OK)
    qos_log_err("Error in qsup_cleanup_ks(): %s", qos_strerror(rv));

  // spin_unlock

  qsup_dev_unregister();
}

/** Show the list of all tasks served by any server */
static int rres_read_qsup(char *page, char **start, off_t off, int count,
                           int *eof, void *data)
{
  kal_irq_state flags;
  unsigned long long int curr_time;
  PROC_PRINT_VARS;

  kal_spin_lock_irqsave(rres_get_spinlock(), &flags);

  curr_time = sched_read_clock_us();
  PROC_PRINT("time (us): %lli\n", curr_time);
  qsup_dump();
  PROC_PRINT_DONE;

  kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);

  PROC_PRINT_END;
}

//extern struct proc_dir_entry *qres_proc_root;
EXPORT_SYMBOL_GPL(qres_proc_root);

int qsup_register_proc(void) {
  struct proc_dir_entry *proc_sched_ent;
  proc_sched_ent = create_proc_entry("qsup", S_IFREG|S_IRUGO|S_IWUSR, qres_proc_root);
  if (!proc_sched_ent) {
    printk("Unable to initialize /proc/" OC_PROC_ROOT "/qres/qsup\n");
    return -1;
  }
  proc_sched_ent->read_proc = rres_read_qsup;
  return 0;
}

void qsup_proc_unregister(void) {
  remove_proc_entry("qsup", qres_proc_root);
}
