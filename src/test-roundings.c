#define QOS_DEBUG_LEVEL QOS_LEVEL_DEBUG
#include <linux/aquosa/qos_debug.h>
#include <linux/aquosa/qos_types.h>
#include <aquosa/qres_lib.h>

void test(qres_time_t Q, qres_time_t Q_min, qres_time_t P) {
  qos_log_debug("(Q, Q_min, P): (" QRES_TIME_FMT ", " QRES_TIME_FMT ", " QRES_TIME_FMT ")", Q, Q_min, P);
  qos_bw_t bw_min;

  /* Round the minimum required bandwidth to the lowest >= value
   * available due to qos_bw_t granularity
   */
  bw_min = r2bw_ceil(Q_min, P);
  qos_log_debug("bw_min=%g", bw_min / (double) (1ul<<QOS_BW_BITS));

  /* Round the requested values according to the qos_bw_t granularity */
  Q_min = bw2Q(bw_min, P);
  Q = bw2Q(r2bw_ceil(Q, P), P);
  qos_log_debug("Rounded (Q, Q_min, P): (" QRES_TIME_FMT ", " QRES_TIME_FMT ", " QRES_TIME_FMT ")", Q, Q_min, P);
}

int main() {
  test(10, 0, 20);
  test(10000, 0, 20000);
  test(19999, 0, 20000);
  return 0;
}
