#include "qres_lib.h"

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>

int main() {
  qos_rv err;
  qres_params_t params = {
    .Q_min = 100000,
    .Q = 200000,
    .P = 1000000,
    .flags = 0,
  };
  qres_sid_t sid;

  printf("Initializing qres library\n");

  err = qres_init();
  qos_chk_ok_exit(err);

  printf("Creating QRES Server\n");
  qos_chk_ok_exit(qres_create_server(&params, &sid));
  qos_chk_ok_exit(qres_attach_thread(sid, 0, 0));
  qos_chk_ok_exit(err);

  struct sched_param sp;

  /** Actively block forever */
  while (1)
    ;

  return 0;
}
