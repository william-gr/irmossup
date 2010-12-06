#include "qres_lib.h"

#include <assert.h>
#include <unistd.h>
#include <stdio.h>

int main() {
  qos_rv err;
  int i;
  qres_params_t params = {
    .Q_min = 0,
    .Q = 10000,
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

/*   printf("Changing QRES Parameters\n"); */
/*   err = qres_set_params(sid, &params); */
/*   qos_chk_ok_exit(err); */

  for (i=0; i<10; i++) {
    qres_time_t exec_time;
    qres_atime_t abs_time;
    qres_get_exec_time(sid, &exec_time, &abs_time);
    printf("exec_time=" QRES_TIME_FMT ", abs_time=" QRES_ATIME_FMT "\n", exec_time, abs_time);
  }

  printf("Destroying QRES Server\n");
  err = qres_destroy_server(sid);
  qos_chk_ok_exit(err);

  printf("Finalizing library\n");
  err = qres_cleanup();
  qos_chk_ok_exit(err);

  return 0;
}
