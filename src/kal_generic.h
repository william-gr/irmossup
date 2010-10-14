#ifndef __KAL_GENERIC_H__
#define __KAL_GENERIC_H__

#ifdef QOS_KS
//#include <linux/autoconf.h>
  #include <linux/kernel.h>
  #include <linux/spinlock.h>
  #include <linux/smp_lock.h> /* @todo smp_lock.h or spinlock.h or both? */
  static inline void kal_init(void) {}
#else

//#  warn "COMPILING KAL_GENERIC IN USER-SPACE"

  #define EXPORT_SYMBOL_GPL(x) 
  #include <linux/spinlock.h>
  #define __cacheline_aligned
  #define SPIN_LOCK_UNLOCKED ((spinlock_t){0})
  static inline void spin_lock_irqsave(spinlock_t *x, unsigned long y) {}
  static inline void spin_unlock_irqrestore(spinlock_t *x, unsigned long y) {} 
  void kal_init(void);
#endif /* QOS_KS */

#endif /* __KAL_GENERIC_H__ */
