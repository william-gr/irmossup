#ifndef KAL_TIMER_H_
#define KAL_TIMER_H_

#include "kal_config.h"

#ifdef KAL_USE_HRTIMER
#  include "kal_timer_hrtimer.h"
#else
#  include "kal_timer_wheel.h"
#endif

#endif /*KAL_TIMER_H_*/
