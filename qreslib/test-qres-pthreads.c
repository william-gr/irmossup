#define QOS_DEBUG_LEVEL QOS_LEVEL_DEBUG
#include <linux/aquosa/qos_debug.h>
#include <aquosa/qres_lib.h>

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>

/** The original bandwidth assignment */
#define ORIG_Q  20000L
#define ORIG_P 100000L

/** Bandwidth ratios (with respect to the original assignment)
 * tried by the test program */
static double target_ratios[] = {
  0.5, 0.3, 0.1
};

/** Tolerance in ratio evaluation set to 10% */
#define RATIO_TOLERANCE 0.15

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

void *main_loop(void *ptr) {
  int i;
  int rv = 0;	/* Thread exit code */
  unsigned long long cnt1, cnt2;
  qres_sid_t sid;

  qres_params_t *params = (qres_params_t *) ptr;

  qos_log_debug("Creating QRES Server with Q=" QRES_TIME_FMT
		", P=" QRES_TIME_FMT, params->Q, params->P);
  qos_chk_ok_exit(qres_create_server(params, &sid));
  qos_chk_ok_exit(qres_attach_thread(sid, 0, 0));

  cnt1 = count_and_wait();

  for (i = 0; i < (int) (sizeof(target_ratios) / sizeof(*target_ratios)); i++) {
    double target_ratio = target_ratios[i];
    double cnt2_exp, reldiff;
    params->Q = (qres_time_t) ((double) ORIG_Q * target_ratio);
    qos_log_debug("Changing QRES Parameters to Q=" QRES_TIME_FMT
		  ", P=" QRES_TIME_FMT, params->Q, params->P);
    qos_chk_ok_exit(qres_set_params(sid, params));

    cnt2_exp = target_ratio * cnt1;
    cnt2 = count_and_wait();

    reldiff = (((double) cnt2) - cnt2_exp) / cnt2_exp;
    qos_log_debug("%d: reldiff=%02.2g%% / 100.00%%", i, reldiff*100);
    if (fabs(reldiff) > RATIO_TOLERANCE) {
      qos_log_err("Expecting to count %g while counted %llu (reldiff=%g)",
		  cnt2_exp, cnt2, reldiff);
      qos_log_err("This is outside defined tolerance of %g",
		  RATIO_TOLERANCE);
      rv = -1;
    }
  }

  qos_log_debug("Destroying QRES Server");
  qos_chk_ok_exit(qres_destroy_server(sid));

  pthread_exit((void *) (intptr_t) rv);
}

qres_params_t params = {
  .Q = ORIG_Q,
  .Q_min = 0,
  .P = ORIG_P,
  .flags = 0,
};

int main() {
  int rv = 0;	/* Program exit code */

  qos_log_debug("Initializing qres library");
  qos_chk_ok_exit(qres_init());

  pthread_t child_tid;
  void *child_rv;
  qos_chk_exit(pthread_create(&child_tid, NULL, main_loop, (void *) &params) == 0);

  main_loop(&params);

  pthread_join(child_tid, &child_rv);

  qos_log_debug("Finalizing QRES library");
  qos_chk_ok_exit(qres_cleanup());

  /* Failure of either father or child implies failure of program */
  return rv | (int) (long) child_rv;	// Double cast needed for 64-bit compatibility
}
