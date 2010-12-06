#include <aquosa/qres_lib.h>
#include <linux/aquosa/qos_debug.h>

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

/** The original bandwidth assignment */
#define ORIG_Q  15000L
#define ORIG_P  60000L

qres_params_t params = {
  .Q_min = 0,
  .Q = 15000,
  .P = 60000,
  .flags = 0,
};

qres_params_t child_params = {
  .Q_min = 0,
  .Q = 30000,
  .P = 60000,
  .flags = 0,
};

void *main_loop(void *ptr) {
  qres_params_t *params = (qres_params_t *) ptr;
  qres_sid_t sid;

  qos_log_debug("Creating QRES Server with Q=" QRES_TIME_FMT
		", P=" QRES_TIME_FMT, params->Q, params->P);
  qos_chk_ok_exit(qres_create_server(params, &sid));
  qos_chk_ok_exit(qres_attach_thread(sid, 0, 0));

  qres_time_t prev_exec_time, exec_time;
  qres_atime_t prev_abs_time, abs_time;
  qos_chk_ok_exit(qres_get_exec_time(sid, &prev_exec_time, &prev_abs_time));
  do {
    qos_chk_ok_exit(qres_get_exec_time(sid, &exec_time, &abs_time));
    //printf("Elapsed: " QRES_TIME_FMT "\n", abs_time - prev_abs_time);
  } while (abs_time - prev_abs_time < 4 * 1000000L);
  return NULL;
}

int main() {
  int rv = 0;	/* Program exit code */

  qos_log_debug("Initializing qres library");
  qos_chk_ok_exit(qres_init());

  pthread_t child_tid;
  void *child_rv;

  rv = pthread_create(&child_tid, NULL, main_loop, (void *) &child_params);
  qos_chk_exit(rv == 0);

  /* Parent */
  main_loop(&params);

  pthread_join(child_tid, &child_rv);

  qos_log_debug("Finalizing QRES library");
  qos_chk_ok_exit(qres_cleanup());

  /* Failure of either father or child implies failure of program */
  return rv | (int) (long) child_rv; // Double cast needed for 64-bit architectures
}
