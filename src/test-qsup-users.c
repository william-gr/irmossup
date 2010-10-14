#include <linux/aquosa/qsup.h>

#include <linux/aquosa/qos_debug.h>
#include <math.h>

/*
 * Only level 0 is used, with two servers inside, of same user 0 in group 0.
 * Level rule:
 *   level 0: max = 0.75
 * User rule:
 *   max = 0.5
 *   min_max = 0.0
 * Server 0 and 1:
 *   min_required = 0.0
 */

typedef double pair_t[2];

pair_t bw_requests[] = {
  /* No correction enforced	*/
  { 0.1, 0.3 },
  /* Violating total per-user	*/
  { 0.3, 0.3 },
  /* Violating total per-user	*/
  { 0.2, 0.4 },
  /* Violating per-user and per-level, but only per-user compression acts	*/
  { 0.4, 0.4 },
  /* Violating per-user and per-level, but only per-user compression acts	*/
  { 0.3, 0.5 },
  /* At last, set required bw to zero before destroying	*/
  { 0.0, 0.0 },
};

pair_t bw_approved[] = {
  { 0.1, 0.3 },
  { 0.3*0.5/(0.3+0.3), 0.3*0.5/(0.3+0.3) },
  { 0.2*0.5/(0.2+0.4), 0.4*0.5/(0.2+0.4) },
  { 0.4*0.5/(0.4+0.4), 0.4*0.5/(0.4+0.4) },
  { 0.3*0.5/(0.3+0.5), 0.5*0.5/(0.3+0.5) },
  { 0.0, 0.0 },
};

#define P 10000
#define build_params(req_bw, min_bw, flags) (& ((qres_params_t) { bw2Q(min_bw, P), bw2Q(req_bw, P), P, flags }) )
#define build_constr(l, w, max_bw, max_min_bw, flags) (& ((qsup_constraints_t) { l, w, max_bw, max_min_bw, flags }))

double tolerance = 0.0001;

int main(int argc, char *argv[]) {
  int err = 0;
  unsigned int n;
  qsup_server_t *srv0, *srv1;

  qos_log_debug("Initing...");
  qsup_init();

  qsup_add_level_rule(0, d2bw(0.75));

  qsup_add_user_constraints(0, build_constr(0, 1, d2bw(0.5), d2bw(0.0), 0));

  qos_chk_ok_exit(qsup_create_server(&srv0, 0, 0, build_params(0, d2bw(0.0), 0)));
  qos_chk_ok_exit(qsup_create_server(&srv1, 0, 0, build_params(0, d2bw(0.0), 0)));

  qsup_dump();

  for (n = 0; n < sizeof(bw_requests) / sizeof(pair_t); n++) {
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

  qos_log_debug("Destroying servers (latest to earliest)");
  qos_chk_ok_exit(qsup_destroy_server(srv1));
  qos_chk_ok_exit(qsup_destroy_server(srv0));

  qos_log_debug("Cleaning up supervisor");
  qsup_cleanup();

  return err;
}
