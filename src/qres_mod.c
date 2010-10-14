/** @file
 ** @brief QRES kernel module and device initialisation and cleanup.
 **/

#include "qres_config.h"
#define QOS_DEBUG_LEVEL QRES_MOD_DEBUG_LEVEL
#include "qos_debug.h"
#include "qsup_gw_ks.h"
#include "qres_proc_fs.h"

#ifdef PROF_QRES_MOD_DEV
#  define QOS_PROFILE
#endif
#include "qos_prof.h"

#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>	/* for get_user and put_user */
#include <asm/processor.h>

#include "qres_gw_ks.h"
#include "qsup_mod.h"

#include "qos_kernel_dep.h"

#define SUCCESS 0
#define BUF_LEN 80
#define DEBUG

/**
 * Is the device open right now? Used to prevent
 * concurent access into the same device
 */
static int Device_Open = 0;

/*
 * The message the device will give when asked
 */
static char Message[BUF_LEN];

/*
 * How far did the process reading the message get?
 * Useful if the message is larger than the size of the
 * buffer we get to fill in device_read.
 */
static char *Message_Ptr;

/*
 * This is called whenever a process attempts to open the device file
 */
static int device_open(struct inode *inode, struct file *file) {
  qos_log_debug("device_open(%p)", file);

  Device_Open++;
  /*
   * Initialize the message
   */
  Message_Ptr = Message;

  KERN_INCREMENT;

  return SUCCESS;
}

/** Called when device is closed by a process.
 *
 * @todo 	Either force destroy_server() to be called here,
 *		or intercept kill hook for handling premature process end.
 */
static int device_release(struct inode *inode, struct file *file)
{
  qos_log_debug("device_release(%p,%p)", inode, file);
  /*
   * We're now ready for our next caller
   */
  Device_Open--;

  KERN_DECREMENT;

  return SUCCESS;
}

/*
 * This function is called whenever a process which has already opened the
 * device file attempts to read from it.
 */
static ssize_t device_read(struct file *file,	/* see include/linux/fs.h   */
			   char __user * buffer,	/* buffer to be
							 * filled with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
  /*
   * Number of bytes actually written to the buffer
   */
  int bytes_read = 0;

  qos_log_debug("device_read(%p,%p,%zu)", file, buffer, length);

  /*
   * If we're at the end of the message, return 0
   * (which signifies end of file)
   */
  if (*Message_Ptr == 0)
    return 0;

  /*
   * Actually put the data into the buffer
   */
  while (length && *Message_Ptr) {

    /*
     * Because the buffer is in the user data segment,
     * not the kernel data segment, assignment wouldn't
     * work. Instead, we have to use put_user which
     * copies data from the kernel data segment to the
     * user data segment.
     */
    put_user(*(Message_Ptr++), buffer++);
    length--;
    bytes_read++;
  }

  qos_log_debug("Read %d bytes, %zu left", bytes_read, length);

  /*
   * Read functions are supposed to return the number
   * of bytes actually inserted into the buffer
   */
  return bytes_read;
}

/*
 * This function is called when somebody tries to
 * write into our device file.
 */
static ssize_t
device_write(struct file *file,
	     const char __user * buffer, size_t length, loff_t * offset)
{
  int i;
  qos_log_debug("device_write(%p,%s,%zu)", file, buffer, length);

  for (i = 0; i < length && i < BUF_LEN; i++)
    get_user(Message[i], buffer + i);

  Message_Ptr = Message;

  /*
   * Again, return the number of input characters used
   */
  return i;
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
int device_ioctl(struct inode *inode,	/* see include/linux/fs.h */
		 struct file *file,	/* ditto */
		 unsigned int ioctl_num,/* number and param for ioctl */
		 unsigned long ioctl_param)
{
  qos_rv err;
  /* SUCCESS is zero, error is negative, same as QOS_x error codes	*/
  err = qres_gw_ks(_IOC_NR(ioctl_num), (void *) ioctl_param,
		   _IOC_SIZE(ioctl_num));
  qos_log_debug("Returning %d (%s)", qos_rv_int(err), qos_strerror(err));
  return qos_rv_int(err);
}

/* Module Declarations */

/** Handlers for the various operation requests on the device
 *
 * This structure will hold the functions to be called
 * when a process does something to the device we
 * created. Since a pointer to this structure is kept in
 * the devices table, it can't be local to
 * init_module. NULL is for unimplemented functions.
 */
struct file_operations Fops = {
	.read = device_read,
	.write = device_write,
	.ioctl = device_ioctl,
	.open = device_open,
	.release = device_release,	/* a.k.a. close */
};

static qos_dev_info_t qres_dev_info;

#ifdef QSUP_DYNAMIC_RECLAIM
static void (*old_block_hook)(struct task_struct *t) = 0;
static void (*old_unblock_hook)(struct task_struct *t, long old_state) = 0;
static void (*old_stop_hook)(struct task_struct *t) = 0;
static void (*old_continue_hook)(struct task_struct *t, long old_state) = 0;

void qres_block_hook(struct task_struct *t) {
  kal_irq_state flags;
  server_t *rres;

  old_block_hook(t);
  kal_spin_lock_irqsave(rres_get_spinlock(), &flags);
  rres = rres_find_by_task(t);
  if (rres && ! rres_has_ready_tasks(rres)) {
    qres_server_t *qres = qres_find_by_rres(rres);
    /* Reduce current bandwidth request to minimum between Q and Q_min */
    if (qres->params.Q > qres->params.Q_min)
      qsup_set_required_bw(&qres->qsup, qres->params.Q_min);
    //qsup_set_required_bw(&qres->qsup, 0);
  }
  kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);
}

void qres_unblock_hook(struct task_struct *t, long old_state) {
  kal_irq_state flags;
  server_t *rres;

  old_unblock_hook(t, old_state);
  kal_spin_lock_irqsave(rres_get_spinlock(), &flags);
  rres = rres_find_by_task(t);
  if (rres && rres_has_ready_tasks(rres)) {
    qres_server_t *qres = qres_find_by_rres(rres);
    qsup_set_required_bw(&qres->qsup, r2bw(qres->params.Q, qres->params.P));
  }
  kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);
}

void qres_stop_hook(struct task_struct *t) {
  kal_irq_state flags;
  server_t *rres;

  old_stop_hook(t);
  kal_spin_lock_irqsave(rres_get_spinlock(), &flags);
  rres = rres_find_by_task(t);
  if (rres && ! rres_has_ready_tasks(rres)) {
    qres_server_t *qres = qres_find_by_rres(rres);
    qsup_set_required_bw(&qres->qsup, 0);
  }
  kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);
}

void qres_continue_hook(struct task_struct *t, long old_state) {
  kal_irq_state flags;
  server_t *rres;

  old_continue_hook(t, old_state);
  kal_spin_lock_irqsave(rres_get_spinlock(), &flags);
  rres = rres_find_by_task(t);
  if (rres && rres_has_ready_tasks(rres)) {
    qres_server_t *qres = qres_find_by_rres(rres);
    qsup_set_required_bw(&qres->qsup, r2bw(qres->params.Q, qres->params.P));
  }
  kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);
}
#endif

/** Initialize the module - Register the character device	*/
static int qres_init_module(void) {
  qos_rv rv;
  kal_irq_state flags;

  kal_spin_lock_irqsave(rres_get_spinlock(), &flags);

  qres_init();
  qos_log_debug("Initing QSUP");
  if ((rv = qsup_init_ks()) != QOS_OK) {
    qos_log_crit("qsup_init_ks() failed: %s", qos_strerror(rv));
    qres_cleanup();
    goto err;
  }

//  start_timer_thread();

#ifdef QSUP_DYNAMIC_RECLAIM
  write_lock(&hook_lock);
  old_block_hook = block_hook;
  old_unblock_hook = unblock_hook;
  old_stop_hook = stop_hook;
  old_continue_hook = continue_hook;
  block_hook = qres_block_hook;
  unblock_hook = qres_unblock_hook;
  stop_hook = qres_stop_hook;
  continue_hook = qres_continue_hook;
  write_unlock(&hook_lock);
#endif

  qos_log_debug("qres module initialization finished");

  kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);

  /* Device registration should be done when kernel module is
   * already able to accept ioctl requests. Furthermore, spinlocks
   * cannot be held because it might sleep.
   */
  if (qos_dev_register(&qres_dev_info, QRES_DEV_NAME, QRES_MAJOR_NUM, &Fops) != QOS_OK) {
    qos_log_crit("Registration of device %s failed", QRES_DEV_NAME);
  } else {
    qos_log_info("Registered QRES device with major device number %d.", MAJOR(qres_dev_info.dev_num));
    qos_log_info("If you want to talk to the device driver,");
    qos_log_info("you'll have to create a device file. ");
    qos_log_info("We suggest you use:");
    qos_log_info("mknod %s c %d 0", QRES_DEV_NAME, MAJOR(qres_dev_info.dev_num));
  }

  qsup_dev_register();

  /** ProcFS-related functions may sleep, so no spinlock should be held **/
  if (qres_proc_register())
    qos_log_err("Failed to register qres module in /proc filesystem");

  return 0;

 err:

  kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);
  return -1;
}

/** Cleanup - unregister the appropriate file from /proc	*/
static void qres_cleanup_module(void) {
  kal_irq_state flags;
  qos_rv ret;

  qres_proc_unregister();

  /*
   * Unregister the device
   */
  qos_dev_unregister(&qres_dev_info);
  qsup_dev_unregister();

  kal_spin_lock_irqsave(rres_get_spinlock(), &flags);

#ifdef QSUP_DYNAMIC_RECLAIM
  write_lock(&hook_lock);
  block_hook = old_block_hook;
  unblock_hook = old_unblock_hook;
  stop_hook = old_stop_hook;
  continue_hook = old_continue_hook;
  write_unlock(&hook_lock);
#endif

  ret = qres_cleanup();
  if (ret != QOS_OK) {
    qos_log_crit("Error in qres_cleanup_ks(): %s", qos_strerror(ret));
    goto err;
  }

  qsup_cleanup();

//  stop_timer_thread();

 err:

  kal_spin_unlock_irqrestore(rres_get_spinlock(), &flags);
}

module_init(qres_init_module);
module_exit(qres_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ReTiS Lab");
MODULE_DESCRIPTION("AQuoSA Resource Reservation - Support for user-space library and supervisor");
MODULE_SUPPORTED_DEVICE("qosres");
