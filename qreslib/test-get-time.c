unsigned long long int sched_time;
unsigned long cpu_khz;
unsigned long fast_gettimeoffset_quotient;

#include <sys/time.h>
#include <stdio.h>

static inline long long llimd(long long ll, int mult, int div);

/* fast_gettimeoffset_quotient = 2^32 * (1 / (TSC clocks per usec)) */
static inline unsigned long long int us2clock(unsigned long u)
{
  return llimd(u, cpu_khz, 1000);
}

static inline unsigned long int clock2ms(unsigned long long c)
{
  return (unsigned long int)(llimd(c, fast_gettimeoffset_quotient, 1000) >> 32);
}

static inline unsigned long int clock2us(unsigned long long c)
{
    return  (unsigned long int) ((c * fast_gettimeoffset_quotient) >> 32);
}

static inline unsigned long long sched_read_clock(void)
{
  unsigned long long res;

  __asm__ __volatile__( "rdtsc" : "=A" (res));
  return res;
}

static inline unsigned long sched_read_clock_us(void)
{
  return clock2us(sched_read_clock());
}

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

int main() {
  unsigned long long clocks = 0;
  unsigned long long clocks1, clocks2;
  unsigned long long clocks_per_sec;
  unsigned long usecs_old = 0;
  unsigned long usecs;
  struct timeval tv1, tv2;
  printf("Computing HZ\n");
  gettimeofday(&tv1, NULL);
  clocks1 = sched_read_clock();
  do {
    gettimeofday(&tv2, NULL);
    clocks2 = sched_read_clock();
    usecs = (tv2.tv_sec-tv1.tv_sec)*1000000L + (tv2.tv_usec-tv1.tv_usec);
  } while(usecs < 1000000L);
  clocks_per_sec = clocks2-clocks1;
  printf("clk1=%llu, clk2=%llu, passed clocks=%llu\n", clocks1, clocks2, clocks_per_sec);
  fast_gettimeoffset_quotient = llimd(1ULL << 32, usecs, clocks_per_sec);
  printf("quotient=%lu\n", fast_gettimeoffset_quotient);

  usecs = 0;
  do {
    unsigned long long ull1, ull2, ulld;
    clocks += clocks_per_sec * 100;
    usecs_old = usecs;
    usecs = clock2us(clocks);
    ull1 = (unsigned long long) usecs;
    ull2 = (unsigned long long) usecs_old;
    ulld = (unsigned long long) (usecs-usecs_old);
    if (usecs < usecs_old)
      printf("XX ");
    else
      printf("   ");
    printf("clk=%llu, us=%llu, us_old=%llu, us_d=%llu\n", clocks, ull1, ull2, ulld);
  } while(1);
  return 0;
}
