#include <linux/aquosa/qsup.h>

#include <linux/aquosa/qos_debug.h>
#include <linux/aquosa/qos_types.h>

#include <math.h>
#include <sys/time.h>
#include <time.h>

/*
 * Only 1 level is used, with two servers inside, of users 0 and 1 in same group 0.
 * Level rule:
 *   level 0: max = 0.75
 * Group rule:
 *   max = 0.5
 *   min_max = 0.0
 * Server 0 and 1:
 *   min_required = 0.0
 */

typedef double pair_t[2];

pair_t bw_requests[] = {
  /* No correction enforced */
  { 0.2, 0.2 },
  /* Violating total per-level */
  { 0.5, 0.5 },
  /* Violating total per-level */
  { 0.4, 0.5 },
  /* Violating per-user with saturation */
  { 0.2, 0.6 },
  /* Violating per-user with saturation and per-level */
  { 0.4, 0.6 },
  /* At last, set required bw to zero before destroying	*/
  { 0.0, 0.0 },
};

pair_t bw_approved[] = {
  { 0.2, 0.2 },
  { 0.5/(0.5+0.5)*0.75, 0.5/(0.5+0.5)*0.75},
  { 0.4/(0.4+0.5)*0.75, 0.5/(0.4+0.5)*0.75},
  { 0.2, 0.5 },
  { 0.4/(0.4+0.5)*0.75, 0.5/(0.4+0.5)*0.75},
  { 0.0, 0.0 },
};

double tolerance = 0.0001;

int main(int argc, char *argv[]) {
  int err = 0;
  unsigned int n;
  qsup_server_t *srv0, *srv1;

  qos_log_debug("Initing...");
  qsup_init();

  qsup_add_level_rule(0, d2bw(0.75));

  qsup_add_group_constraints(0, & ((qsup_constraints_t) { 0, 1, d2bw(0.5), d2bw(0.0), 0 }) );

  qos_chk_ok_exit(qsup_create_server(&srv0, 0, 0, & ((qres_params_t) { 0, 0, 10000, 0 }) ));
  qos_chk_ok_exit(qsup_create_server(&srv1, 1, 0, & ((qres_params_t) { 0, 0, 10000, 0 }) ));

  qsup_dump();

  for (n = 0; n < sizeof(bw_requests) / sizeof(pair_t); n++) {
    int i;
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    for (i = 0; i < 1000000; ++i)
      qsup_set_required_bw(srv0, d2bw(bw_requests[n][0]));
    gettimeofday(&tv2, NULL);
    qos_log_debug("# microseconds: %g\n", (tv2.tv_usec - tv1.tv_usec + (tv2.tv_sec - tv1.tv_sec) * 1000000) / 1000000.0);
    qsup_set_required_bw(srv1, d2bw(bw_requests[n][1]));
    if (
	(fabs(bw2d(qsup_get_approved_bw(srv0)) - bw_approved[n][0]) > tolerance)
	|| (fabs(bw2d(qsup_get_approved_bw(srv1)) - bw_approved[n][1]) > tolerance)
	) {
      qos_log_err("Requests were %g, %g. Expecting %g, %g while got %g, %g.",
		  bw_requests[n][0], bw_requests[n][1],
		  bw_approved[n][0], bw_approved[n][1],
		  bw2d(qsup_get_approved_bw(srv0)), bw2d(qsup_get_approved_bw(srv1)));
      err = -1;
    }
  }

  qsup_dump();

  qos_log_debug("Cleaning up supervisor (all at once)");
  qsup_cleanup();

  return err;
}
