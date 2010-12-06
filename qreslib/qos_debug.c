#include "qos_debug.h"

static char *qos_errors[] = {
  "Unspecified error",
  "Insufficient memory",
  "Invalid parameter(s)",
  "Unauthorized",
  "Unimplemented",
  "Missing component",
  "Inconsistent state",
  "System overload",
  "Internal error: report to authors, please",
  "Not found",
  "Full container",
  "Empty container"
};

qos_rv qos_err = QOS_OK;

/** Logging message identifier number, used to detect loss of messages */
atomic_t qos_log_msg_id = ATOMIC_INIT(0);

/** Indentation level when logging (currently uncorrect
    w.r.t. concurrent kernel sections) **/
int indent_lev = 0;

char * func_names[MAX_INDENT_LEVEL];

char *qos_strerror(qos_rv err) {
  int index;
  int err_num = qos_rv_int(err);
  if (err == QOS_OK)
    return "Success";

  index = -err_num - 16;
  if ( (index < 0) || (index >= (int) (sizeof(qos_errors) / sizeof(*qos_errors))) ) {
    qos_log_err("Invalid index: %d", index);
    return "Bug: unclassified error in qos_types.c";
  }
  return qos_errors[index];
}

void qos_dump_stack(void) {
  int i;
  qos_log_crit("Stack dump:");
  for (i = 0; i < indent_lev; ++i)
    qos_log_crit("  %s", func_names[i]);
}

#if defined(QOS_KS)
EXPORT_SYMBOL_GPL(qos_strerror);
EXPORT_SYMBOL_GPL(qos_log_msg_id);
EXPORT_SYMBOL_GPL(indent_lev);
EXPORT_SYMBOL_GPL(func_names);
EXPORT_SYMBOL_GPL(qos_dump_stack);
#endif
