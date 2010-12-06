/** @file
 *
 * @brief Composite operations on unsigned long integers for non-i386
 * architectures.
 */

#ifndef __QOS_UL_OTHER_H__
#define __QOS_UL_OTHER_H__

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
#ifndef __KERNEL__
#define ul_shl_div(num, SHF, den) ((__u32) ((((__u64) ((__u32)(num))) << (SHF)) / (__u32)(den)))
#else
#define ul_shl_div(num, SHF, den) ({		      \
      __u64 result = ((__u64)(num)) << (SHF);	      \
      do_div(result, (den));			      \
      result;					      \
    })
#endif

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

#ifndef __KERNEL__
#define ul_mul_div(x, num, den) ((__u32) (((__u64) (x)) * (num) / (__u32)(den)))
#else
#define ul_mul_div(x, num, den) ({		      \
      __u64 result = ((__u64)(x)) * (num);		      \
      do_div(result, (den));			      \
      result;					      \
    })
#endif

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
#define ul_mul_shr(x, y, SHF) ((__u32) ((((__u64) (x)) * (y)) >> (SHF)))

/** Computes 'ceil((num << SHF) / den)' as unsigned long (32 bits).
 *
 * This macro makes the same computation as ul_shl_div, but
 * the result is rounded up to the next value, if it cannot
 * be precisely represented.
 */
#ifndef __KERNEL__
#define ul_shl_ceil(num, SHF, den) ({			\
      __u64 _n = (__u64) (num);				\
      __u32 _d = (__u32) (den);				\
      __u32 _r = (__u32) ( ( (_n << (SHF)) + _d - 1) / _d ); \
      _r;						\
    })
#else
#define ul_shl_ceil(num, SHF, den) ({		      \
      __u64 __den = (__u64) (den);			      \
      __u64 __result = ((__u64)(num)) << (SHF);	      \
      __result += __den - 1;			      \
      do_div(__result, __den);			      \
      __result;					      \
    })
#endif

#endif
