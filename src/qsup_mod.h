#ifndef __QSUP_MOD_H__
#define __QSUP_MOD_H__

int qsup_init_module(void);
qos_rv qsup_dev_register(void);
void qsup_cleanup_module(void);
qos_rv qsup_dev_unregister(void);

#endif
