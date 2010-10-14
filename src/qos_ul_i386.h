/** @file
 *
 * @brief Composite operations on unsigned long integers for i386.
 *
 * These are implemented in assembler with intermediate results on 64 bits,
 * using native 32-bit multiply and divide operations of the processor.
 * Resulting implementation is hopefully more appropriate than general
 * long-long arithmetics functions coming with gcc, which would not be
 * available anycase inside the kernel (their use would require linking
 * the gcc long-long module within a kernel module).
 *
 * @author Tommaso Cucinotta
 */

#ifndef __QOS_UL_I386_H__
#define __QOS_UL_I386_H__

#ifndef __i386__
#  error "This module only works for i386 architectures"
#endif

#include <linux/types.h>

/** Computes '(num << SHF) / den' as unsigned long (32 bits).
 *
 * Computation is carried on with intermediate result (num << SHF)
 * on 64 bits. This macro is useful for representing the result
 * of the division num/dev as a real in fixed precision with
 * SHF decimal bits (i.e. 1.0 is represented as 1ul<<SHF).
 *
 * @param num	an unsigned long integer (32 bits)
 * @param SHF	a macro which evaluates to an integer within 1 and 31
 * @param den	an unsigned long integer (32 bits)
 * @result	the result as an unsigned long integer (32 bits)
 *
 * @note	No overflow check is made after the division.
 * @todo	Investigate on volatiles, that do not allow optimizations.
 *		They should not be required, but without them bw2Q(), which
 *		uses ul_mul_shr, outputs zero instead of the correct value.
 */
#define ul_shl_div(num, SHF, den) ({			\
  __volatile__ __u32 _ul_num = (num);		\
  __volatile__ __u32 _ul_den = (den);		\
  __volatile__ __u32 _ul_res;			\
  __asm__ __volatile__("\n"				\
	   "	xorl  %%edx, %%edx	\n"		\
	   "	shldl %2, %%eax, %%edx	\n"		\
	   "	shll  %2, %%eax		\n"		\
	   "	divl  %3		\n"		\
	   : "=a" (_ul_res)				\
	   : "0" (_ul_num), "I" ((SHF)), "r" (_ul_den)	\
	   : "%edx");					\
  _ul_res;						\
})

/** Computes 'ceil((num << SHF) / den' as unsigned long (32 bits).
 *
 * This macro makes the same computation as ul_shl_div, but
 * the result is rounded up to the next value, if it cannot
 * be precisely represented.
 */
#define ul_shl_ceil(num, SHF, den) ({			\
  __volatile__ __u32 _ul_num = (num);		\
  __volatile__ __u32 _ul_den = (den);		\
  __volatile__ __u32 _ul_res;			\
  __asm__ __volatile__("\n"				\
	   "	xorl  %%edx, %%edx	\n"		\
	   "	shldl %2, %%eax, %%edx	\n"		\
	   "	shll  %2, %%eax		\n"		\
	   "	divl  %3		\n"		\
	   "	cmpl  $0,%%edx		\n"		\
	   "	jz    0f		\n"		\
	   "    incl  %%eax		\n"		\
	   "0:\n"		\
	   : "=a" (_ul_res)				\
	   : "0" (_ul_num), "I" ((SHF)), "r" (_ul_den)	\
	   : "%edx");					\
  _ul_res;						\
})


/** Computes '(x * num) / den' as unsigned long (32 bits).
 *
 * Computation is carried on with intermediate result (x * num)
 * on 64 bits.
 *
 * @param x	an unsigned long integer (32 bits)
 * @param num	an unsigned long integer (32 bits)
 * @param den	an unsigned long integer (32 bits)
 * @result	the result as an unsigned long integer (32 bits)
 *
 * @note	No overflow check is made after the division.
 * @todo	Investigate on volatiles, that do not allow optimizations.
 *		They should not be required, but without them bw2Q(), which
 *		uses ul_mul_shr, outputs zero instead of the correct value.
 */
#define ul_mul_div(x, num, den) ({			\
  __volatile__ __u32 _ul_x = (x);		\
  __volatile__ __u32 _ul_num = (num);		\
  __volatile__ __u32 _ul_den = (den);		\
  __volatile__ __u32 _ul_res;			\
  __asm__ __volatile__("\n"				\
	   "	mull  %%edx		\n"		\
	   "	divl  %3		\n"		\
	   : "=a" (_ul_res)				\
	   : "0" (_ul_x), "d" (_ul_num), "r" (_ul_den)); \
  _ul_res;						\
})


/** Computes '(x * y) >> SHF' as unsigned long (32 bits).
 *
 * The computation is carried on with intermediate result
 * (x*y) on 64 bits.
 *
 * This macro is useful for multiplying two integers where
 * one of them represents a real in fixed precision with SHF
 * decimal bits (i.e. 1.0 is represented as 1ul<<SHF).
 *
 * @param x	an unsigned long integer (32 bits)
 * @param y	an unsigned long integer (32 bits)
 * @param SHF	a macro which evaluates to an integer within 1 and 31
 * @result	the result as an unsigned long integer (32 bits)
 *
 * @note	No overflow check is made after the shift.
 * @todo	Investigate on volatiles, that do not allow optimizations.
 *		They should not be required, but without them bw2Q(), which
 *		uses ul_mul_shr, outputs zero instead of the correct value.
 */
#define ul_mul_shr(x, y, SHF) ({			\
  __volatile__ __u32 _ul_x = (x);		\
  __volatile__ __u32 _ul_y = (y);		\
  __volatile__ __u32 _ul_res;			\
  __asm__ __volatile__ ("\n"				\
	   "	mull  %%edx		\n"		\
	   "	shrdl %2, %%edx, %%eax	\n"		\
	   : "=a" (_ul_res)				\
	   : "0" (_ul_x), "I" ((SHF)), "d" (_ul_y));	\
  _ul_res;						\
})

#endif
