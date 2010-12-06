#define QOS_DEBUG_LEVEL QOS_LEVEL_DEBUG
#include <linux/aquosa/qos_debug.h>

#include <aquosa/qres_lib.h>
#include "util_timeval.h"

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

/** The original bandwidth assignment */
#define ORIG_Q  20000L
#define ORIG_P 100000L

/** Replicas of myself */
#define N 1

char cmd_line[180];	///< Used for replicating

// When program is run without args it replicates, otherwise it does not.
FILE *replicas[N];

int main_body() {
  qres_params_t params = {
    .Q_min = ORIG_Q,
    .Q = ORIG_Q,
    .P = ORIG_P,
    .flags = 0,
  };
  int j;
  int rv = 0;	/* Program exit code */
  qres_atime_t prev_abs_time, abs_time;
  qres_time_t prev_exec_time, exec_time;
  qres_sid_t sid;

  qos_log_debug("Initializing qres library");
  qos_chk_ok_exit(qres_init());

  qos_log_debug("Creating QRES Server with Q=" QRES_TIME_FMT ", P=" QRES_TIME_FMT, params.Q, params.P);
  qos_chk_ok_exit(qres_create_server(&params, &sid));
  qos_chk_ok_exit(qres_attach_thread(sid, 0, 0));

  qres_get_exec_time(sid, &prev_exec_time, &prev_abs_time);
  for (j = 0; j < 100; j++) {
    do {
      qres_get_exec_time(sid, &exec_time, &abs_time);
    } while (abs_time - prev_abs_time < ORIG_P);
    qos_log_debug("exec_time: " QRES_TIME_FMT ", abs_time: " QRES_ATIME_FMT,
		  exec_time, abs_time);
    qres_time_t delta = exec_time - prev_exec_time;
    if (delta < ORIG_Q * 9 / 10 || delta > ORIG_Q * 11 / 10) {
      qos_log_err("Didn't get enough budget: " QRES_TIME_FMT
        ", expecting " QRES_TIME_FMT, delta, (qres_time_t) ORIG_Q);
      rv = -1;
    }
    prev_exec_time += ORIG_Q;
    prev_abs_time += ORIG_P;
  }

  qos_log_debug("Destroying QRES Server");
  qos_chk_ok_exit(qres_destroy_server(sid));

  qos_log_debug("Finalizing QRES library");
  qos_chk_ok_exit(qres_cleanup());

  return rv;
}

int main(int argc, char **argv) {
  int rv = 0;
  int j;

  if (argc == 1) {
    qos_log_debug("Replicating myself...");

    for (j = 0; j < N; j++) {
      strncpy(cmd_line, argv[0], sizeof(cmd_line));
      strncat(cmd_line, " fake_arg", sizeof(cmd_line));
      qos_log_debug("... through command: %s", cmd_line);
      replicas[j] = popen(cmd_line, "r");
      qos_chk_exit(replicas[j] != NULL);
    }

    rv = main_body();

    for (j = 0; j < N; j++) {
      qos_chk_exit(pclose(replicas[j]) == 0);
    }
  } else {
    rv = main_body();
  }

  return rv;
}
