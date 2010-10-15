/** @addtogroup RRES_MOD
 * @{
 */

/** @file
 * @brief Support of proc file system
 *
 */ 
/*
This file comes from RTAI
   
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it), 2003 Luca Marzario

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/


#ifndef _RRES_PROC_FS_H_
#define _RRES_PROC_FS_H_

#include "rres_config.h"

#ifdef CONFIG_OC_RRES_PROC
#include <linux/proc_fs.h>

#define OC_PROC_ROOT "aquosa" 

// proc print macros - Contributed by: Erwin Rol (erwin@muffin.org)

/** macro that holds the local variables that
 ** we use in the PROC_PRINT_* macros. We have
 ** this macro so we can add variables with out
 ** changing the users of this macro, of course
 ** only when the names don't colide!
 **/
#define PROC_PRINT_VARS                                 \
    off_t pos = 0;                                      \
    off_t begin = 0;                                    \
    int len = 0 /* no ";" */

/** Macro for printf-like writes into the procfs kernel-supplied buffer.
 **
 ** This macro expects the function arguments to be
 ** named as follows (is should be a read procfs callback):
 **
 ** static int FOO(char *page, char **start,
 **                off_t off, int count, int *eof, void *data)
 **
 ** @todo
 ** I've got the "THIS SHOULD..." message, thus I suspect a bug somewhere.
 **/
#define PROC_PRINT(fmt,args...)                         \
  do {							\
    len = scnprintf(page + pos - begin, count - (pos - begin), fmt, ##args); \
    pos += len;                                         \
    if (pos <= off)                                     \
        begin = pos;                                    \
    if (pos - begin == count)			        \
        goto done;					\
    else if (pos - begin > count) {			\
        qos_log_crit("THIS SHOULD NEVER HAPPEN: pos=%lu, begin=%lu, count=%d", pos, begin, count); \
        goto done;                                      \
    }                                                   \
  } while (0)

/** Macro to jump directly to the PROC_PRINT_DONE block **/
#define PROC_PRINT_BREAK                                \
  do {						 	\
    *eof = 1;                                           \
    goto done; 						\
  } while (0)

/** Macro that should only be used once near the end of the
 ** read function; should be followed by a PROC_PRINT_END
 **
 ** @note
 ** To return from another place in the read function, use
 ** the PROC_PRINT_BREAK macro.
 **/
#define PROC_PRINT_DONE                                 \
  do {							\
        *eof = 1;                                       \
    done:                                               \
        *start = page + (off - begin);                  \
  } while (0)

#define PROC_PRINT_END do { return pos - off; } while (0)

// End of proc print macros

#endif /* CONFIG_OC_RRES_PROC */

int rres_proc_register(void);
void rres_proc_unregister(void);

/** pointer to qres proc_fs root directory (/proc/qres) */
extern struct proc_dir_entry *qres_proc_root;
//struct proc_dir_entry *qres_proc_root;
//EXPORT_SYMBOL_GPL(qres_proc_root);

#endif  //  _RRES_PROC_FS_H_

/** @} */
