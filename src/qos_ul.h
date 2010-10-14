/** @file
 *
 * @brief Composite mul/div, shl/div and mul/shr operations on unsigned long integers.
 *
 * On i386 architectures, these are implemented in assembler with
 * intermediate results on 64 bits, using native 32-bit multiply and
 * divide operations of the processor.  Resulting implementation is
 * hopefully more appropriate than general long-long arithmetics
 * functions coming with gcc, which would not be available anycase
 * inside the kernel (their use would require linking the gcc
 * long-long module within a kernel module).
 */

#ifndef __QOS_UL_H__
#define __QOS_UL_H__

#ifdef __i386__
#  include "qos_ul_i386.h"
#else
#  include "qos_ul_other.h"
#endif

#endif
