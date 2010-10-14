#include <linux/aquosa/qsup.h>

#include <linux/aquosa/qos_debug.h>
#include <math.h>

/*
 * Only 1 level is used, with two servers inside, of users 0 and 1 in same group 0.
 * Level rule:
 *   level 0: max = 0.75
 * Group rule:
 *   max = 0.75
 *   min_max = 0.3
 * Server 0 and 1:
 *   min_required = 0.1
 */

typedef double pair_t[2];

pair_t bw_requests[] = {
  /* Use below guaranteed minimums (no correction)	*/
  { 0.05, 0.05 },
  /* Use beyond minimums (no correction)		*/
  { 0.3, 0.3 },
  /* Violating total per-level. Compression retains minimum guaranteed */
  { 0.6, 0.4 },
  /* At last, set required bw to zero before destroying	*/
  { 0.0, 0.0 },
};

pair_t bw_approved[] = {
  { 0.05, 0.05 },
  { 0.3, 0.3 },
  { 0.1+(0.6-0.1)/(0.6-0.1+0.4-0.1)*(0.75-0.1-0.1), 0.1+(0.4-0.1)/(0.6-0.1+0.4-0.1)*(0.75-0.1-0.1) },
  { 0.0, 0.0 },
};

double tolerance = 0.0001;

#define P 10000
#define build_params(req_bw, min_bw, flags) (& ((qres_params_t) { bw2Q(min_bw, P), bw2Q(req_bw, P), P, flags }) )
#define build_constr(l, w, max_bw, max_min_bw, flags) (& ((qsup_constraints_t) { l, w, max_bw, max_min_bw, flags }))

int main(int argc, char *argv[]) {
  int err = 0;
  unsigned int n;
  qsup_server_t *srv0, *srv1;

  qos_log_debug("Initing...");
  qsup_init();

  qsup_add_level_rule(0, d2bw(0.75));

  qsup_add_group_constraints(0, build_constr(0, 1, d2bw(0.75), d2bw(0.3), 0));

  /* Test check on max_min configured through rules */
  qos_chk_exit(qsup_create_server(&srv0, 0, 0, build_params(0, d2bw(0.5), 0)) == QOS_E_UNAUTHORIZED);

  qos_chk_ok_exit(qsup_create_server(&srv0, 0, 0, build_params(0, d2bw(0.1), 0)));
  qos_chk_ok_exit(qsup_create_server(&srv1, 1, 0, build_params(0, d2bw(0.1), 0)));

  qsup_dump();

  for (n = 0; n < sizeof(bw_requests) / sizeof(pair_t); n++) {
    qos_log_debug("n=%d", n);
    qsup_set_required_bw(srv0, d2bw(bw_requests[n][0]));
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

  qos_log_debug("Destroying servers (earliest to latest)");
  qos_chk_ok_exit(qsup_destroy_server(srv0));
  qos_chk_ok_exit(qsup_destroy_server(srv1));

  qos_log_debug("Cleaning up supervisor");
  qsup_cleanup();

  return err;
}
