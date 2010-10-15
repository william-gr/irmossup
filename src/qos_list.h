#ifndef __QOS_LIST_H__
#define __QOS_LIST_H__

#ifdef QOS_KS
  #include <linux/list.h>
#else /* QOS_KS */
  /* this is to force to include <linux/list.h> in user space*/
  #undef offsetof
  #define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
  #define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
  #define __KERNEL__
  #define __ASM_I386_PROCESSOR_H
  #define _LINUX_KERNEL_H
  #define __ASM_SYSTEM_H
  #include <linux/list.h> /* included in user space! */
  #undef __KERNEL__
  #undef __ASM_I386_PROCESSOR_H
  #undef _LINUX_KERNEL_H
  #undef __ASM_SYSTEM_H
#endif /* QOS_KS */

/* @todo (low) check all list operations, in particular the use of NULL value and *_NULL macro */
/* @todo (low) rename the macro parameters name adding __ */
/* @todo (mid) move here other functions? */
#define INIT_LIST_HEAD_NULL(ptr) do { (ptr)->prev = NULL; (ptr)->next = NULL; } while (0)

/** add an object in an ordered list 
 *  insert in the (head) list the (obj) of type (obj_type), 
 *  ordered in increasing order by field (ord_field) of (obj)
 *  example list_add_ordered (ready_list, server_t, srv, rq_ph, deadline, ret)
 *  NOTE: in the parameters description "" are used for argument that are name and not variables.
 *  @param
 *  head:        (struct list_head * )    head of the list\n
 *  obj_type:    ("object type")          type of each element of the list\n
 *  obj:         (obj_type object)        object that must be added to the list
 *  list_field:  ("name of field")        name of field used to insert the object in the list\n
 *  ord_field:   ("name of filed")        name of filed the list is ordered by\n
 *  ret:         (int)                    return value is put here\n
 *                                        return 1 if the object is inserted at the beginning of the list\n
 *                                        return 0  otherwise\n
 */
#define list_add_ordered(head, obj_type, obj, list_field, ord_field, ret) \
do { \
  struct list_head *h; \
  if (list_empty(&head)) { \
    list_add(&((obj)->list_field ), &(head)); \
    goto __ret1; \
  } else { \
    for (h = (head).next; h != &(head); h = h->next) { \
      if ( kal_time_lt( (obj)->ord_field, (list_entry(h, struct obj_type, list_field)->ord_field) ) ) { \
        break; \
      } \
    } \
    list_add_tail(&((obj)->list_field), h); \
  } \
  if (&((obj)->list_field) == (head).next) { \
    goto __ret1; \
  } \
  (ret) = 0; \
  break; \
__ret1: \
  (ret) = 1; \
} while (0)

/** Remove from list and set to NULL pointers that point to next and previous objects */
static inline void list_del_null(struct list_head *entry)
{
  list_del(entry);
  entry->next = entry->prev = 0;
}

#endif /* __QOS_LIST_H__ */
