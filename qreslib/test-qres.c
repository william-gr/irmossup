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

/** Bandwidth ratios (with respect to the original assignment)
 * tried by the test program */
static double target_ratios[] = {
  0.5, 0.3, 0.1
};

/** Tolerance in ratio evaluation set to 15% */
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

int main(int argc, char **argv) {
  qres_params_t params = {
    .Q_min = 0,
    .Q = ORIG_Q,
    .P = ORIG_P,
    .flags = 0,
  };
  unsigned long long cnt1, cnt2;
  int i;
  int rv = 0;	/* Program exit code */
  qres_sid_t sid;

  qos_log_debug("Initializing qres library");
  qos_chk_ok_exit(qres_init());

  qos_log_debug("Creating QRES Server with Q=" QRES_TIME_FMT ", P=" QRES_TIME_FMT, params.Q, params.P);
  qos_chk_ok_exit(qres_create_server(&params, &sid));
  qos_chk_ok_exit(qres_attach_thread(sid, 0, 0));

  cnt1 = count_and_wait();

  for (i = 0; i < (int) (sizeof(target_ratios) / sizeof(*target_ratios)); i++) {
    double target_ratio = target_ratios[i];
    double cnt2_exp, reldiff;
    params.Q = (qres_time_t) ((double) ORIG_Q * target_ratio);
    qos_log_debug("Changing QRES Parameters to Q=" QRES_TIME_FMT ", P=" QRES_TIME_FMT,
		  params.Q, params.P);
    qos_chk_ok_exit(qres_set_params(sid, &params));

    cnt2_exp = target_ratio * cnt1;
    cnt2 = count_and_wait();

    reldiff = (((double) cnt2) - cnt2_exp) / cnt2_exp;
    qos_log_debug("reldiff=%2.2g percentage", reldiff*100);
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

  qos_log_debug("Finalizing QRES library");
  qos_chk_ok_exit(qres_cleanup());

  return rv;
}
