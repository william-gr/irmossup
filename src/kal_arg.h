#ifndef __KAL_ARG_H__
#define __KAL_ARG_H__

typedef unsigned long kal_arg_t;

static inline int kal_arg_int(kal_arg_t arg) {
  return (int) arg;
}

static inline long kal_arg_long(kal_arg_t arg) {
  return (long) arg;
}

static inline unsigned long kal_arg_ulong(kal_arg_t arg) {
  return (unsigned long) arg;
}

static inline void * kal_arg_voidptr(kal_arg_t arg) {
  return (void *) arg;
}

static inline kal_arg_t kal_int_arg(int i) {
  return (kal_arg_t) i;
}

static inline kal_arg_t kal_long_arg(long i) {
  return (kal_arg_t) i;
}

static inline kal_arg_t kal_ulong_arg(unsigned long i) {
  return (kal_arg_t) i;
}

static inline kal_arg_t kal_voidptr_arg(void *i) {
  return (kal_arg_t) i;
}

#endif
