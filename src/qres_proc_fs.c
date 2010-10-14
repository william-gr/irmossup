/** @file
 * @brief Proc FileSystem support for AQuoSA QRES Module.
 *
 */ 

#include "qres_config.h"
#define QOS_DEBUG_LEVEL QRES_MOD_DEBUG_LEVEL
#include <linux/aquosa/qos_debug.h>

#ifdef CONFIG_OC_QRES_PROC
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/ctype.h> //for numeric conversion
#include <stdarg.h>

#include <linux/aquosa/rres_proc_fs.h>

extern struct proc_dir_entry proc_root;           /**< pointer to linux proc file-system root */

/** Show the list of configuration options */
/* @todo (low) complete this function */
static int qres_read_modinfo(char *page, char **start, off_t off, int count,
			     int *eof, void *data) {
  PROC_PRINT_VARS;
  PROC_PRINT("Enabled options for AQuoSA QRES:\n\n");

  PROC_PRINT("QSUP Expand\t\t");
#ifdef QSUP_EXPAND
    PROC_PRINT("on\n");
#else
    PROC_PRINT("off\n");
#endif

  PROC_PRINT("QSUP Dynamic Reclaim\t");
#ifdef QSUP_DYNAMIC_RECLAIM
    PROC_PRINT("on\n");
#else
    PROC_PRINT("off\n");
#endif

  PROC_PRINT("\n");
  PROC_PRINT_DONE;

  PROC_PRINT_END;
}  /* End function - rres_read_param */

/** regiter entries in the proc file-system */
int qres_proc_register(void) {
  struct proc_dir_entry *proc_modinfo_ent;

  proc_modinfo_ent = create_proc_entry("qres-modinfo", S_IFREG|S_IRUGO|S_IWUSR, qres_proc_root);
  if (!proc_modinfo_ent) {
    printk("Unable to initialize /proc/" OC_PROC_ROOT "/qres/qres-modinfo\n");
    return(-1);
  }
  proc_modinfo_ent->read_proc = qres_read_modinfo;

  return 0;
}

/** unregiter entries in the proc file-system */
void qres_proc_unregister(void) {
  remove_proc_entry("qres-modinfo", qres_proc_root);
}

#else /* ! CONFIG_OC_QRES_PROC */
int qres_proc_register(void) { return 0; }
void qres_proc_unregister(void) {}
#endif /* CONFIG_OC_QRES_PROC */
