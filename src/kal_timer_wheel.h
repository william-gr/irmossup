#ifndef KAL_TIMER_WHEEL_H_
#define KAL_TIMER_WHEEL_H_

#include "kal_time.h"

#include "qos_debug.h"
#include "qos_types.h"
#include "qos_memory.h"

#include <linux/time.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

/********************** TIMER RELATED **********************/

#include "kal_arg.h"
typedef void (*linux_timer_cb)(unsigned long cb_data);
typedef void (*kal_timer_cb)(kal_arg_t cb_data);

typedef struct {
  struct timer_list timer;      //< The kernel timer instance
  kal_timer_cb timer_cb;        //< The KAL timer callback
  kal_arg_t timer_cb_data;      //< The opaque data to be supplied to the KAL timer callback
  char handler_running;         //< Whether we are within this timer handler
} kal_timer_t;

static void timer_callback(unsigned long cb_data) {
  kal_timer_t *p_timer = (kal_timer_t *) cb_data;
  qos_chk_do_msg(qos_mem_valid(p_timer), return, "Ignoring timer with deallocated kal_timer_t data");
  p_timer->handler_running = 1;
  p_timer->timer_cb(p_timer->timer_cb_data);
  p_timer->handler_running = 0;
}

static inline void kal_timer_init(kal_timer_t * p_timer, kal_timer_cb cb, kal_arg_t cb_data) {
  p_timer->timer_cb = cb;
  p_timer->timer_cb_data = cb_data;
  setup_timer(&p_timer->timer, timer_callback, (unsigned long) p_timer);
}

static inline void kal_timer_init_now(kal_timer_t * p_timer, kal_timer_cb cb, kal_arg_t cb_data) {
  unsigned long expires = get_jiffies();
  kal_timer_init(p_timer, cb, cb_data);
  //printk("Setting expires to %lu (timer unarmed)\n", expires);
  p_timer->timer.expires = expires;
}

static inline void kal_timer_set(kal_timer_t * p_timer, kal_time_t t) {
  unsigned long expires = kal_time2jiffies(t);
  //printk("Setting expires to %lu\n", expires);
  del_timer(&p_timer->timer);
  p_timer->timer.expires = expires;
  add_timer(&p_timer->timer);
}

static inline void kal_timer_del(kal_timer_t * p_timer) {
  if (timer_pending(&p_timer->timer) && (! p_timer->handler_running))
    del_timer_sync(&p_timer->timer);
}

static inline void kal_timer_forward(kal_timer_t * p_timer, kal_time_t t) {
  unsigned long expires = p_timer->timer.expires + kal_time2jiffies(t);
  //printk("Forwarding expires from %lu to %lu (delta=%lu)\n",
  //        p_timer->expires, expires, kal_time2jiffies(t));
  kal_timer_del(p_timer);
  p_timer->timer.expires = expires;
  add_timer(&p_timer->timer);
}

static inline int kal_timer_pending(kal_timer_t *p_timer) {
  return timer_pending(&p_timer->timer);
}

#endif /*KAL_TIMER_WHEEL_H_*/
