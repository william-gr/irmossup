#ifndef __QOS_MEMORY_H__
#define __QOS_MEMORY_H__

/** @file
 * @brief Space-independent memory allocation functions
 *
 * These functions allocate and free a memory segment,
 * independently of the space in which they are being compiled.
 * For user-space, they map to malloc() and free().
 * For kernel-space, they map to kmalloc() and kfree().
 *
 * @todo Check what are the proper flags to use with kmalloc().
 * @todo These functions may be made static inline.
 */

/** Allocates a memory segment either in user-space or in kernel-space.
 *
 * In case of no memory available returns 0.
 *
 * @note
 * Another possible implementation of allocation function can be:
 * 
 * @code
 *   #define qos_malloc(x) do { x = xalloc(sizeof (*(x))); } while (0)
 * @endcode
 */
void *qos_malloc(long size);

/** Like qos_malloc(), but adds a name for the chunk useful when debugging */
void *qos_malloc_named(long size, const char *name);

/** Deallocates a memory segment	*/
void qos_free(void *ptr);

/** Check if the supplied pointer is within allocated segments.
 **
 ** If memory chunks tracking is disabled, then this function returns always 1.
 **/
int qos_mem_valid(void *ptr);

/** If memory check enabled, return 1 if all allocated
 ** memory chunks have been freed, and no chunk-related
 ** errors occurred ever during qos_malloc() / qos_free().
 **/
int qos_mem_clean(void);

/** Create an instance of the supplied type.    **/
#define qos_create(type) ({					\
      type *__ptr = qos_malloc_named(sizeof(type), #type);	\
      if (__ptr == NULL)					\
	qos_log_err("Could not allocate memory for " #type);	\
      __ptr;							\
})

#endif
