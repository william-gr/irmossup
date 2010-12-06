#define QOS_DEBUG_LEVEL QOS_LEVEL_DEBUG
#include <linux/aquosa/qos_debug.h>

#include <aquosa/qres_lib.h>

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>

/** The original bandwidth assignment */
#define ORIG_Q  50000L
#define ORIG_P 100000L

/** Actively increments a counter and gets time until 1 second is
 * elapsed. Returns the counter value. */
long count_and_wait() {
  unsigned long counted_sheeps = 0;
  unsigned long elapsed_usecs;
  struct timeval tv1, tv2;

  qos_log_debug("Counting sheeps for 2 seconds ...");
  gettimeofday(&tv1, NULL);
  do {
    gettimeofday(&tv2, NULL);
    elapsed_usecs = tv2.tv_usec - tv1.tv_usec + (tv2.tv_sec - tv1.tv_sec) * 1000000;
    counted_sheeps++;
  } while (elapsed_usecs < 2000000);
  qos_log_debug("Counted %lu sheeps in 2 seconds", counted_sheeps);
  return counted_sheeps;
}

int main() {
  qres_params_t params = {
    .Q_min = 0,
    .Q = ORIG_Q,
    .P = ORIG_P,
    .flags = 0,
  };
  qres_sid_t sid;

  /* Testing init/create/destroy/cleanup sequence	*/
  qos_log_debug("Initializing qres library");
  qos_chk_ok_exit(qres_init());

  qos_log_debug("Creating QRES Server with Q=" QRES_TIME_FMT ", P=" QRES_TIME_FMT, params.Q, params.P);
  qos_chk_ok_exit(qres_create_server(&params, &sid));
  qos_chk_ok_exit(qres_attach_thread(sid, 0, 0));

  count_and_wait();

  qos_log_debug("Destroying QRES Server");
  qos_chk_ok_exit(qres_destroy_server(sid));

  qos_log_debug("Finalizing QRES library");
  qos_chk_ok_exit(qres_cleanup());

  /* Testing init/create/detach/cleanup sequence	*/

  qos_log_debug("Initializing qres library");
  qos_chk_ok_exit(qres_init());

  qos_log_debug("Creating QRES Server with Q=" QRES_TIME_FMT ", P=" QRES_TIME_FMT, params.Q, params.P);
  qos_chk_ok_exit(qres_create_server(&params, &sid));
  qos_chk_ok_exit(qres_attach_thread(sid, 0, 0));

  count_and_wait();

  qos_log_debug("Detaching QRES Server");
  qos_chk_ok_exit(qres_detach_thread(QRES_SID_NULL, 0, 0));

  qos_log_debug("Finalizing QRES library");
  qos_chk_ok_exit(qres_cleanup());

  /* Testing init/create/setparams/destroy/cleanup sequence	*/

  qos_log_debug("Initializing qres library");
  qos_chk_ok_exit(qres_init());

  qos_log_debug("Creating QRES Server with Q=" QRES_TIME_FMT ", P=" QRES_TIME_FMT, params.Q, params.P);
  qos_chk_ok_exit(qres_create_server(&params, &sid));
  qos_chk_ok_exit(qres_attach_thread(sid, 0, 0));

  count_and_wait();

  params.Q = params.Q / 2;
  qos_log_debug("Setting QRES params");
  qos_chk_ok_exit(qres_set_params(sid, &params));

  count_and_wait();

  params.Q = params.Q * 2;
  qos_log_debug("Setting QRES params");
  qos_chk_ok_exit(qres_set_params(sid, &params));

  count_and_wait();

  qos_log_debug("Detaching QRES Server");
  qos_chk_ok_exit(qres_destroy_server(sid));

  qos_log_debug("Finalizing QRES library");
  qos_chk_ok_exit(qres_cleanup());

  return 0;
}
