#include "rres_config.h"
#define QOS_DEBUG_LEVEL QOS_LEVEL_INFO
#include "qos_debug.h"

#if defined(QOS_KS)
//#  include <linux/autoconf.h>
#  include <linux/kernel.h>
#  include <linux/version.h>
#  include <linux/module.h>
#  include <linux/sched.h>
#  include <linux/list.h>
#  include <linux/slab.h>
#  include "kal_sched.h"
#  include <linux/string.h>
#else
#  include <stdlib.h>
#  include <string.h>
#endif

#include "qos_memory.h"

#ifdef QOS_MEMORY_CHECK

#define MAX_CHUNK_NUMBER 1024
#define CHUNK_NAME_MAX_LEN 16

/** @note
 ** We strncpy the chunk name into name[] because the module with leakages might have already
 ** been unloaded when some other module calls qos_mem_clean()
 **/
static struct chunk {
  void * ptr;		//< Chunk memory pointer
  unsigned long size;	//< Chunk size
  char name[CHUNK_NAME_MAX_LEN]; //< Chunk name used for debugging
} chunks[MAX_CHUNK_NUMBER];
static int num_chunks = 0;
static int mem_failure = 0;
static int chunks_inited = 0;
#ifdef QOS_KS
kal_lock_define(chunks_spin_lock);
#endif

/** Call only under chunk_lock **/
static void chunks_init(void) {
  int i;
  if (! chunks_inited) {
    for (i=0; i<MAX_CHUNK_NUMBER; i++)
      chunks[i].ptr = NULL;
    chunks_inited = 1;
  }
}

/** Return index of supplied chunk, or MAX_CHUNK_NUMBER if not found
 **
 ** May also be used to find the first free chunk position, if NULL is passed.
 **/
int find_chunk(void *ptr) {
  int i;
  for (i=0; i<MAX_CHUNK_NUMBER; ++i)
    if (chunks[i].ptr == ptr)
      break;
  return i;
}

/** Adds ptr to chunks[].
 * Performance is not important, here */
void add_chunk(void *ptr, unsigned long size, const char *name) {
  int i = find_chunk(NULL);
  qos_chk_do(i < MAX_CHUNK_NUMBER, return);
  chunks[i].ptr = ptr;
  chunks[i].size = size;
  strncpy(chunks[i].name, name, sizeof(chunks[i].name) - 1);
  chunks[i].name[sizeof(chunks[i].name) - 1] = '\0';
  qos_log_debug("Adding chunk: ptr=%p, size=%lu, name='%s'",
      chunks[i].ptr, chunks[i].size, chunks[i].name);
  num_chunks++;
}

/** If ptr is found in chunks[], then remove it and return 1.
 * Otherwise, return 0 */
int rem_chunk(void *ptr) {
  int i = find_chunk(ptr);
  qos_chk_do(i < MAX_CHUNK_NUMBER, return 0);
  qos_log_debug("Freeing chunk: ptr=%p, size=%lu, name='%s'",
      chunks[i].ptr, chunks[i].size, chunks[i].name);
  chunks[i].ptr = NULL;
  num_chunks--;
  return 1;
}

#ifdef QOS_KS
#  define chunks_vars unsigned long _chunks_flags
#  define chunks_lock kal_spin_lock_irqsave(&chunks_spin_lock, &_chunks_flags)
#  define chunks_unlock kal_spin_unlock_irqrestore(&chunks_spin_lock, &_chunks_flags)
#else
#  define chunks_vars
#  define chunks_lock
#  define chunks_unlock
#endif

/** Check whether the supplied pointer is found within any of the chunks
 ** allocated through qos_malloc().
 **/
int qos_mem_valid(void *ptr) {
  int i;
  int valid = 0;
  chunks_vars;

  chunks_lock;
  chunks_init();
  for (i = 0; i < MAX_CHUNK_NUMBER; ++i)
    if (ptr >= chunks[i].ptr && ptr < chunks[i].ptr + chunks[i].size) {
      valid = 1;
      break;
    }
  chunks_unlock;
  return valid;
}

#else /* MEMORY_CHECK */

/** Check whether the supplied pointer is found within any of the chunks
 ** allocated through qos_malloc().
 **/
int qos_mem_valid(void *ptr) {
  return 1;
}

#endif /* MEMORY_CHECK */

static inline void *qos_ll_malloc(long size) {
#if defined(QOS_KS)
  return kmalloc(size, GFP_ATOMIC);
#else
  return malloc(size);
#endif
}

static inline void qos_ll_free(void *ptr) {
#if defined(QOS_KS)
  kfree(ptr);
#else
  free(ptr);
#endif
}

void *qos_malloc(long size) {
  return qos_malloc_named(size, "Unknown");
}

void *qos_malloc_named(long size, const char *name) {
  void *ptr;
#ifdef QOS_MEMORY_CHECK
  chunks_vars;
#endif
  ptr = qos_ll_malloc(size);
  if (ptr == NULL)
    return NULL;
#ifdef QOS_MEMORY_CHECK
  chunks_lock;
  chunks_init();
  if (num_chunks == MAX_CHUNK_NUMBER) {
    qos_log_crit("Chunks exhausted: cannot debug memory allocation");
    qos_ll_free(ptr);
    ptr = NULL;
    mem_failure = 1;
  } else {
    add_chunk(ptr, size, name);
  }
  qos_log_debug("Current number of chunks: %d", num_chunks);
  chunks_unlock;
#endif
  return ptr;
}

void qos_free(void *ptr) {
#ifdef QOS_MEMORY_CHECK
  chunks_vars;
#endif
  if (ptr == NULL) {
    qos_log_crit("Trying to free a NULL pointer");
    return;
  }
#ifdef QOS_MEMORY_CHECK
  chunks_lock;
  chunks_init();
  if (! rem_chunk(ptr)) {
    qos_log_crit("Freeing a non-allocated or already freed memory chunk");
    chunks_unlock;
    return;
  }
  qos_log_debug("Current number of chunks: %d", num_chunks);
  chunks_unlock;
#endif
  qos_ll_free(ptr);
}

int qos_mem_clean() {
#ifdef QOS_MEMORY_CHECK
  int ret;
  chunks_vars;
  chunks_lock;
  chunks_init();
  if (num_chunks > 0) {
    int i = 0;
    qos_log_debug("Residual chunks:");
    for (i = 0; i < MAX_CHUNK_NUMBER; ++i)
      if (chunks[i].ptr != NULL)
	qos_log_debug("  ptr=%p, size=%lu, name='%s'",
		      chunks[i].ptr, chunks[i].size, chunks[i].name);
  }
  ret = ((num_chunks == 0) && (mem_failure == 0));
  chunks_unlock;
  return ret;
#else
  return 1;
#endif
}

#ifdef QOS_KS
EXPORT_SYMBOL(qos_malloc);
EXPORT_SYMBOL_GPL(qos_malloc_named);
EXPORT_SYMBOL_GPL(qos_free);
EXPORT_SYMBOL_GPL(qos_mem_clean);
EXPORT_SYMBOL_GPL(qos_mem_valid);
#endif
