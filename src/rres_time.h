/** @addtogroup RRES_MOD
 * @{
 */

/** @file
 * @brief time and clock related functions
 *
 */ 
/*
 * Copyright (C) 2002 Luca Abeni, 2003 Luca Marzario
 * This is Free Software; see GPL.txt for details
 */

#ifndef __RRES_TIME_H__
#define __RRES_TIME_H__

#include <asm/param.h>
#include <linux/version.h>
#include "kal_timer.h"

#if (LINUX_VERSION_CODE<KERNEL_VERSION(2,6,0))
#define CPU_KHZ_TYPE unsigned long
#else
#define CPU_KHZ_TYPE unsigned int
#endif
#ifdef QOS_KS
  extern CPU_KHZ_TYPE cpu_khz;
#endif

#if !defined(QOS_KS)
#  include <sys/time.h>
#  include <time.h>
#  define do_gettimeofday(__tv) gettimeofday((__tv), NULL);
#endif

/* This function comes from RTAI (http://www.rtai.org) */
static inline long long llimd(long long ll, int mult, int div)
{
	__asm__ __volatile (\
		 "movl %%edx,%%ecx; mull %%esi;       movl %%eax,%%ebx;  \n\t"
	         "movl %%ecx,%%eax; movl %%edx,%%ecx; mull %%esi;        \n\t"
		 "addl %%ecx,%%eax; adcl $0,%%edx;    divl %%edi;        \n\t"
	         "movl %%eax,%%ecx; movl %%ebx,%%eax; divl %%edi;        \n\t"
		 "sal $1,%%edx;     cmpl %%edx,%%edi; movl %%ecx,%%edx;  \n\t"
		 "jge 1f;           addl $1,%%eax;    adcl $0,%%edx;     1:"
		 : "=A" (ll)
		 : "A" (ll), "S" (mult), "D" (div)
		 : "%ebx", "%ecx");
	return ll;
}


/* fast_gettimeoffset_quotient = 2^32 * (1 / (TSC clocks per usec)) */
static inline unsigned long long int us2clock(unsigned long u)
{
#if LINUX_VERSION_CODE<KERNEL_VERSION(2,6,0)
  return llimd(u, cpu_khz, 1000);
#else
  return u;
#endif
}

static inline unsigned long int clock2ms(unsigned long long c)
{
#if LINUX_VERSION_CODE<KERNEL_VERSION(2,6,0)
  return (unsigned long int)(llimd(c, fast_gettimeoffset_quotient, 1000) >> 32);
#else
  return llimd(c, 1, 1000);
#endif
}

static inline unsigned long int clock2us(unsigned long long c)
{
#if LINUX_VERSION_CODE<KERNEL_VERSION(2,6,0)
    return  (unsigned long int) ((c * fast_gettimeoffset_quotient) >> 32);
#else
    return c;
#endif
}

static inline unsigned long int clock2jiffies(unsigned long long c)
{
#if LINUX_VERSION_CODE<KERNEL_VERSION(2,6,0)
  c = (c * fast_gettimeoffset_quotient) >> 32;
#endif  
  return llimd(c, HZ, 1000000);
}

static inline unsigned long long sched_read_clock(void)
{
  unsigned long long int res;
#if LINUX_VERSION_CODE<KERNEL_VERSION(2,6,0)
  __asm__ __volatile__( "rdtsc" : "=A" (res));
#else
  struct timeval tv;

  do_gettimeofday(&tv);
  res = (unsigned long long int)tv.tv_usec + (unsigned long long int)tv.tv_sec * 1000000LLU;
#endif           
  return res;
}

static inline unsigned long sched_read_clock_us(void)
{
  return clock2us(sched_read_clock());
}

#if LINUX_VERSION_CODE<KERNEL_VERSION(2,6,0)
static inline unsigned long int clock2subjiffies(unsigned long long c)
{
  unsigned long long int tmp, tmp1, tmp2;
  
  tmp = (c * fast_gettimeoffset_quotient) >> 32;
  tmp1 = llimd(tmp, HZ, 1000000);
  tmp2 = llimd(tmp1, 1000000, HZ);
  return c - llimd(tmp2, cpu_khz, 1000);
}
#endif

//extern unsigned long long sched_time; /**< used to avoid reading too frequently the time. Time must be read at each "entry point" */
extern kal_time_t sched_time; /**< used to avoid reading too frequently the time. Time must be read at each "entry point" */
//extern kal_time_t init_time;

static inline void rres_init_time(void) {
  //init_time = kal_time_now();
  //qos_log_debug("Init time: " KAL_TIME_FMT, KAL_TIME_FMT_ARG(init_time));
}

/**
 * Sample the current time, with the best available precision, into sched_time.
 * 
 * Used at every synchronous or asynchronous enter of code flow within rres,
 * in order to avoid too frequent precise reads of the current time.
 */
static inline void rres_sample_time(void) {
  sched_time = kal_time_now();
    //kal_time2usec(kal_time_sub(kal_time_now(), init_time)); //sched_read_clock_us();
  qos_log_debug("Sampled time: " KAL_TIME_FMT, KAL_TIME_FMT_ARG(sched_time));
}

#endif /* __TIME_H__ */

/** @} */
