#include "qres_lib.h"

#include <assert.h>
#include <unistd.h>
#include <stdio.h>

int main() {
  qres_params_t params = {
    .Q_min = 0,
    .Q = 10000,
    .P = 1000000,
    .flags = 0,
  };
  qres_sid_t sid;

  printf("Initializing qres library\n");

  qos_chk_ok_exit(qres_init());

  printf("Creating QRES Server\n");
  qos_chk_ok_exit(qres_create_server(&params, &sid));
  printf("QRES Server create with ID %d\n", (int) sid);
  qos_chk_ok_exit(qres_attach_thread(sid, 0, 0));

  printf("Destroying QRES Server\n");
  qos_chk_ok_exit(qres_destroy_server(sid));

  printf("Finalizing library\n");
  qos_chk_ok_exit(qres_cleanup());

  return 0;
}
