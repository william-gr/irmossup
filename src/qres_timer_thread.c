#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/mm.h>
//#include <linux/devfs_fs_kernel.h>
#include <linux/mmtimer.h>
#include <linux/miscdevice.h>
#include <linux/posix-timers.h>
#include <linux/interrupt.h>
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>

#include "qos_debug.h"

pid_t timer_thread_pid;
struct task_struct *p_timer_thread_ts = 0;

//extern int force_ksoftirqd;
int force_ksoftirqd;

struct k_itimer timer;

struct itimerspec its = {
  .it_interval = {
    .tv_sec = 0,
    .tv_nsec = 100000000
  },
  .it_value = {
    .tv_sec = 0,
    .tv_nsec = 100000000
  }
};

struct itimerval itv = {
  .it_interval = {
    .tv_sec = 0,
    .tv_usec = 100000
  },
  .it_value = {
    .tv_sec = 0,
    .tv_usec = 100000
  }
};

int timer_thread(void *_c)
{
  int rv;
  unsigned long flags;

  p_timer_thread_ts = current;

  qos_log_crit("timer_thread(): Daemonizing timer thread (pid=%d)", (int)current->pid);
  daemonize("timer");
  qos_log_crit("timer_thread(): Allowing signals");
  allow_signal(SIGKILL);
  allow_signal(SIGSTOP);
  allow_signal(SIGCONT);
  allow_signal(SIGALRM);

  qos_log_crit("timer_thread(): forcing execution of softirqs outside interrupt context");
  force_ksoftirqd = 1;

  //rv = do_setitimer(ITIMER_REAL, &itv, NULL);
  //qos_log_crit("timer_thread(): do_setitimer() returned: %d", rv);
  //if (rv != 0)
    goto die;
  spin_lock_init(&timer.it_lock);
  spin_lock_irqsave(&timer.it_lock, flags);

//  memset(&timer, 0, sizeof(timer));
//  spin_lock_init(&timer.it_lock);
//  timer.it_clock = MAKE_THREAD_CPUCLOCK(0, CPUCLOCK_SCHED);
//  timer.it_overrun = -1;
//  timer.it_sigev_notify = SIGEV_SIGNAL;
//  timer.it_sigev_signo = SIGALRM;
//  qos_log_crit("timer_thread(): Creating timer");
//  rv = posix_cpu_timer_create(&timer);
//  qos_log_crit("timer_thread(): timer_create() returned: %d", rv);
//  if (rv != 0)
//    goto die;
//  timer.it_sigev_notify = SIGEV_SIGNAL;
//  timer.it_sigev_signo = SIGALRM;
//  timer.it_process = current;
//  qos_log_crit("timer_thread(): Setting timer");
//  spin_lock_irqsave(&timer.it_lock, flags);
//  rv = posix_cpu_timer_set(&timer, 0, &its, NULL);
//  qos_log_crit("timer_thread(): timer_set() returned: %d", rv);
//  if (rv != 0) {
//    spin_unlock_irqrestore(&timer.it_lock, flags);
//    goto die;
//  }

  //genschedify(current, MAX_RT_PRIO-1);

  do {
    siginfo_t info;
    unsigned long signr;

    while (! signal_pending(current)) {
      qos_log_crit("timer_thread(): Waiting signal...");
      set_current_state(TASK_INTERRUPTIBLE);
      spin_unlock_irqrestore(&timer.it_lock, flags);
      schedule();
      spin_lock_irqsave(&timer.it_lock, flags);
    }

    spin_unlock_irqrestore(&timer.it_lock, flags);

    qos_log_crit("timer_thread(): Dequeuing signal");
    signr = dequeue_signal_lock(current, &current->blocked, &info);
    qos_log_crit("timer_thread(): Dequeued signal: %lu", signr);

    switch(signr) {
    case SIGSTOP:
      qos_log_crit("timer_thread(): SIGSTOP received.");
      set_current_state(TASK_STOPPED);
      schedule();
      break;

    case SIGKILL:
      qos_log_crit("timer_thread(): SIGKILL received.");
      goto die;

    case SIGHUP:
      qos_log_crit("timer_thread(): SIGHUP received.");
      break;
      
    case SIGALRM:
      qos_log_crit("timer_thread(): SIGALARM received: %lu", jiffies);
      break;

    default:
      qos_log_crit("timer_thread(): signal %ld received", signr);
    }
    spin_lock_irqsave(&timer.it_lock, flags);
  } while (1);

  qos_log_crit("timer_thread(): Ended while loop");

 die:

  qos_log_crit("timer_thread(): unforcing execution of softirqs outside interrupt context");
  force_ksoftirqd = 0;

  qos_log_crit("timer_thread(): Exiting");
}

void start_timer_thread(void)
{
  int ret = 0;

  qos_log_crit("Starting timer thread");
  timer_thread_pid = kernel_thread(timer_thread, NULL, CLONE_KERNEL);
  if (timer_thread_pid < 0) {
    qos_log_crit("fork failed for timer thread: %d", -timer_thread_pid);
  }
  qos_log_crit("Timer thread started");
}

void stop_timer_thread(void)
{
  qos_log_crit("Stopping timer thread");
  if (p_timer_thread_ts)
    send_sig(SIGKILL, p_timer_thread_ts, 1);
  qos_log_crit("Timer thread stopped");
}
