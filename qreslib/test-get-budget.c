#include "qos_debug.h"
#include "qres_lib.h"

#include "util_periodic.h"
#include <stdio.h>

#define N 1000
#define TICK_US 100ull

struct timeval t0;

struct data {
  unsigned long timestamps[N];
  qres_time_t gua_budgets[N];
  qres_time_t req_budgets[N];
  qres_time_t appr_budgets[N];
  qres_time_t next_budgets[N];
  qres_time_t curr_budgets[N];
  int num;
  qres_sid_t sid;
} data;

void f(void * p) {
  struct data * p_data = (struct data *) p;
  struct timeval t;
  unsigned long timestamp;
  qres_time_t gua_budget, req_budget, appr_budget, next_budget, curr_budget;
  qres_params_t params;

  gettimeofday(&t, NULL);

  timestamp = timeval_sub_us(&t, &t0);
  qos_chk_ok_exit(qres_get_params(p_data->sid, &params));
  gua_budget = params.Q_min;
  req_budget = params.Q;
  qos_chk_ok_exit(qres_get_appr_budget(p_data->sid, &appr_budget));
  qos_chk_ok_exit(qres_get_next_budget(p_data->sid, &next_budget));
  qos_chk_ok_exit(qres_get_curr_budget(p_data->sid, &curr_budget));

  if (p_data->num < N) {
    p_data->timestamps[p_data->num] = timestamp;
    p_data->gua_budgets[p_data->num] = gua_budget;
    p_data->req_budgets[p_data->num] = req_budget;
    p_data->appr_budgets[p_data->num] = appr_budget;
    p_data->next_budgets[p_data->num] = next_budget;
    p_data->curr_budgets[p_data->num] = curr_budget;
    p_data->num++;
  }
}

void data_write_timestamps(struct data *p_data, const char *fname) {
  FILE *f;
  int i;
  qos_chk_exit((f = fopen(fname, "w")) != NULL);

  for (i = 0; i < p_data->num; ++i) {
    fprintf(f, "%lu\t" QRES_TIME_FMT "\t" QRES_TIME_FMT "\t" QRES_TIME_FMT
	    "\t" QRES_TIME_FMT "\t" QRES_TIME_FMT "\n", p_data->timestamps[i], p_data->gua_budgets[i],
	    p_data->req_budgets[i], p_data->appr_budgets[i],
	    p_data->next_budgets[i], p_data->curr_budgets[i]);
  }

  fclose(f);
}

int main(int argc, char *argv[])
{
  qres_params_t params;
  struct timeval t1;
  struct timeval duration = { .tv_sec = (TICK_US * N) / 1000000, .tv_usec = (TICK_US * N) % 1000000 };
  struct timeval tick = { .tv_sec = TICK_US / 1000000, .tv_usec = TICK_US % 1000000 };

  qos_chk_ok_exit(qres_init());

  params.Q = 9900;
  params.Q_min = 0;
  params.P = 100000;
  params.flags = 0;
  qos_chk_ok_exit(qres_create_server(&params, &data.sid));
  qos_chk_ok_exit(qres_attach_thread(data.sid, 0, 0));

  gettimeofday(&t0, NULL);
  timeval_add(&t1, &t0, &duration);

  data.num = 0;
  spin_periodic_call(&t0, &t1, &tick, f, &data);

  qos_chk_ok_exit(qres_destroy_server(data.sid));

  data_write_timestamps(&data, "/tmp/ts.txt");

  qos_chk_ok_exit(qres_cleanup());

  return 0;
}
