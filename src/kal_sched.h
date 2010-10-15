#ifndef __KAL_TASK_H__
#define __KAL_TASK_H__

/** @file
 * @brief Abstraction of a task within the kernel abstraction layer (KAL).
 *
 * This module abstracts the concept of task/thread from the scheduler perspective.
 * It allows to associate generic "opaque" data to a thread, to retrieve the
 * numeric unique identifier of a thread, to retrieve the effective user and
 * group identifier, to check if a task is ready to run, and to change a task
 * scheduling policy so to force the kernel to schedule it.
 */

#include "qos_debug.h"

#ifdef QOS_KS

//#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/hardirq.h>

typedef struct task_struct kal_task_t;

/** the type of uid is uid_t in linux */
typedef uid_t kal_uid_t;

/** the type of gid is gid_t in linux */
typedef gid_t kal_gid_t;

#if LINUX_VERSION_CODE<KERNEL_VERSION(2,6,0)

#  define set_tsk_need_resched(tsk) do { (tsk)->need_resched = 1; } while (0)

#else /* LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,0) */

#  define for_each_task(t)         for_each_process(t)

#endif

#if LINUX_VERSION_CODE<KERNEL_VERSION(2,6,27)

# define current_uid()		(current->uid)
# define current_gid()		(current->gid)
# define current_euid()		(current->euid)
# define current_egid()		(current->egid)

# define task_uid(task)		((task)->uid)
# define task_gid(task)		((task)->gid)
# define task_euid(task)	((task)->euid)
# define task_egid(task)	((task)->egid)

#elif LINUX_VERSION_CODE>KERNEL_VERSION(2,6,28)

#  define task_gid(task)	(task_cred_xxx((task), gid))
#  define task_egid(task)	(task_cred_xxx((task), egid))

#endif

/** return the user id of the process currently executing */
static inline kal_uid_t kal_get_current_uid(void)
{
  return current_uid();
}

/** return the group id of the process currently executing */
static inline kal_uid_t kal_get_current_gid(void)
{
  return current_gid();
}

/** return the user id of the specified task */
static inline kal_uid_t kal_task_get_uid(kal_task_t *p_task)
{
  return task_euid(p_task);
}

/** return the group id of the specified task */
static inline kal_gid_t kal_task_get_gid(kal_task_t *p_task)
{
  return task_egid(p_task);
}

/** return the task that is currently executing */
static inline kal_task_t *kal_task_current(void)
{
  return (kal_task_t *) current;
}

static inline void kal_task_link_data(kal_task_t *task, void *data) {
  //task->private_data = data;
}

static inline void kal_task_unlink_data(kal_task_t *task) {
  //task->private_data = NULL;
}

static inline void *kal_task_get_data(kal_task_t *task) {
  qos_chk_do(task != NULL, return NULL);
  //return task->private_data;
}

static inline int kal_task_get_id(kal_task_t *task) {
  qos_chk(task != NULL);
  return task->pid;
}

/** Perform extra sanity checks on supplied task before
 ** allowing for customized scheduling policy on it.
 **
 ** Currently, this is supposed to avoid concurrent attach
 ** and kill of tasks, where the attach operation is invoked
 ** after the exit hook has passed in exit.c, but the task
 ** may still be found through find_task_by_id().
 **/
static inline int kal_task_alive(struct task_struct *task) {
  return (task->flags & PF_EXITING) == 0;
}

#if LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,27)
#  define kal_find_task_by_pid find_task_by_vpid
#else
#  define kal_find_task_by_pid find_task_by_pid
#endif

typedef unsigned long kal_irq_state;
typedef spinlock_t kal_lock_t;

/** Define a kal_lock_t and initialize it as unlocked **/
#define kal_lock_define(name) \
  spinlock_t name __cacheline_aligned = SPIN_LOCK_UNLOCKED

#define kal_spin_lock_irqsave(p_lock, p_state) do { \
  spin_lock_irqsave(p_lock, *p_state); \
  qos_log_debug("Acquired spinlock"); \
} while (0)

#define kal_spin_unlock_irqrestore(p_lock, p_state) do { \
  qos_log_debug("Releasing spinlock"); \
  spin_unlock_irqrestore(p_lock, *p_state); \
} while (0)

#define kal_atomic() (in_atomic() || irqs_disabled())

/* @todo maybe task_ready goes here ? */

#else /* !QOS_KS */

#include <sys/types.h>
#include <linux/types.h>
#include "kal_generic.h"
#include "qos_list.h"

#define TASK_RUNNING 0

struct task_struct;

typedef struct task_struct kal_task_t;

extern void *current;  
extern void *idle;  
extern int need_resched;
extern struct list_head kernel_task_list;


#define set_tsk_need_resched(x) do { need_resched = 1; } while (0)

/** the type of uid is uid_t in linux */
typedef uid_t kal_uid_t;

/** the type of gid is gid_t in linux */
typedef gid_t kal_gid_t;

/** return the user id of the process currently executing */
static inline uid_t kal_get_current_uid(void)
{
  return current_uid();
}

/** return the group id of the process currently executing */
static inline gid_t kal_get_current_gid(void)
{
  return current_gid();
}

#define for_each_task(t, pos) \
        for ((pos) = kernel_task_list.next, prefetch((pos)->next); \
        (t) = (list_entry((pos), struct task_struct, task_list)), (pos) != &kernel_task_list; \
                (pos) = (pos)->next, prefetch((pos)->next))

extern void (*block_hook)(struct task_struct *tsk);
extern void (*unblock_hook)(struct task_struct *tsk, long old_state);
extern void (*stop_hook)(struct task_struct *tsk);
extern void (*continue_hook)(struct task_struct *tsk, long old_state);
extern void (*fork_hook)(struct task_struct *tsk);
extern void (*cleanup_hook)(struct task_struct *tsk);
extern rwlock_t hook_lock;

void set_task_rr_prio(kal_task_t *p, int priority);

void kal_task_link_data(kal_task_t *task, void *data);
void kal_task_unlink_data(kal_task_t *task);
void *kal_task_get_data(kal_task_t *task);
int kal_task_get_id(kal_task_t *task);
int kal_task_ready(kal_task_t * task);

#endif /* QOS_KS */


#endif /* __KAL_TASK_H__ */
