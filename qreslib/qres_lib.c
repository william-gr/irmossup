#include "qres_config.h"
/* #define QOS_DEBUG_LEVEL QRES_LIB_DEBUG_LEVEL */

#include "qres_lib.h"
//#include <linux/aquosa/qos_debug.h>
#include "qos_types.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <unistd.h>
#include <string.h>
#include <errno.h>

/** All of these must have qres_iparams_t as argument, otherwise
 * qres_gw_ks() complaints about wrong size */

#define IOCTL_OP_CREATE_SERVER	_IOR (QRES_MAJOR_NUM, QRES_OP_CREATE_SERVER, qres_iparams_t)
#define IOCTL_OP_DESTROY_SERVER	_IOR (QRES_MAJOR_NUM, QRES_OP_DESTROY_SERVER, qres_sid_t)
#define IOCTL_OP_ATTACH_TO_SERVER _IOR (QRES_MAJOR_NUM, QRES_OP_ATTACH_TO_SERVER, qres_attach_iparams_t)
#define IOCTL_OP_DETACH_FROM_SERVER _IOR (QRES_MAJOR_NUM, QRES_OP_DETACH_FROM_SERVER, qres_attach_iparams_t)
#define IOCTL_OP_SET_PARAMS	_IOR (QRES_MAJOR_NUM, QRES_OP_SET_PARAMS, qres_iparams_t)
#define IOCTL_OP_GET_PARAMS	_IOWR(QRES_MAJOR_NUM, QRES_OP_GET_PARAMS, qres_iparams_t)
#define IOCTL_OP_GET_SERVER_ID _IOWR(QRES_MAJOR_NUM, QRES_OP_GET_SERVER_ID, qres_attach_iparams_t)
#define IOCTL_OP_GET_EXEC_TIME	_IOWR(QRES_MAJOR_NUM, QRES_OP_GET_EXEC_TIME, qres_time_iparams_t)
#define IOCTL_OP_GET_CURR_BUDGET       _IOWR(QRES_MAJOR_NUM, QRES_OP_GET_CURR_BUDGET, qres_time_iparams_t)
#define IOCTL_OP_GET_NEXT_BUDGET       _IOWR(QRES_MAJOR_NUM, QRES_OP_GET_NEXT_BUDGET, qres_time_iparams_t)
#define IOCTL_OP_GET_APPR_BUDGET       _IOWR(QRES_MAJOR_NUM, QRES_OP_GET_APPR_BUDGET, qres_time_iparams_t)
#define IOCTL_OP_GET_DEADLINE          _IOWR(QRES_MAJOR_NUM, QRES_OP_GET_DEADLINE, qres_timespec_iparams_t)
#define IOCTL_OP_SET_WEIGHT            _IOR (QRES_MAJOR_NUM, QRES_OP_SET_WEIGHT, qres_weight_iparams_t)
#define IOCTL_OP_GET_WEIGHT            _IOWR(QRES_MAJOR_NUM, QRES_OP_GET_WEIGHT, qres_weight_iparams_t)

/** File descriptor of the QoS Res Device		*/
int qres_fd = -1;

/** Server period set with create_server or set_params	*/
qres_params_t server_params = {
  .Q = 0,
  .Q_min = 0,
  .P = 0,
  .flags = 0,
  .timeout = 0
};


/** @todo If not open, then open it !			*/
static inline qos_rv check_open() {
  if (qres_fd == -1) {
    return qres_init();
  }
  return QOS_OK;
}


qos_rv qres_get_sid(pid_t pid, tid_t tid, qres_sid_t *p_sid) {
  qres_attach_iparams_t iparams;

  qos_rv rv = check_open();
  qos_chk_rv(rv == QOS_OK, rv);

  iparams.pid = pid;
  iparams.tid = tid;

  if (ioctl(qres_fd, IOCTL_OP_GET_SERVER_ID, &iparams) < 0)
    return qos_int_rv(-errno);

  *p_sid = iparams.server_id;

  return QOS_OK;
}


qos_rv qres_create_server(qres_params_t * p_params, qres_sid_t *p_sid) {
  qres_iparams_t iparams;

  qos_rv rv = check_open();
  qos_chk_rv(rv == QOS_OK, rv);

  iparams.params = *p_params;

  if (ioctl(qres_fd, IOCTL_OP_CREATE_SERVER, &iparams) < 0)
    return qos_int_rv(-errno);

  if (p_sid != NULL)
    *p_sid = iparams.server_id;

  server_params = *p_params;
  return QOS_OK;
}


qos_rv qres_destroy_server(qres_sid_t sid) {
  qos_rv rv = check_open();
  qos_chk_rv(rv == QOS_OK, rv);

  if (ioctl(qres_fd, IOCTL_OP_DESTROY_SERVER, &sid) < 0)
    return qos_int_rv(-errno);

  return QOS_OK;
}


qos_rv qres_attach_thread(qres_sid_t sid, pid_t pid, tid_t tid) {
  qres_attach_iparams_t iparams;

  qos_rv rv = check_open();
  qos_chk_rv(rv == QOS_OK, rv);

  iparams.server_id = sid;
  iparams.pid = pid;
  iparams.tid = tid;

  if (ioctl(qres_fd, IOCTL_OP_ATTACH_TO_SERVER, &iparams) < 0)
    return qos_int_rv(-errno);

  return QOS_OK;
}


qos_rv qres_detach_thread(qres_sid_t sid, pid_t pid, tid_t tid) {
  qres_attach_iparams_t iparams;

  qos_rv rv = check_open();
  qos_chk_rv(rv == QOS_OK, rv);

  iparams.server_id = sid;
  iparams.pid = pid;
  iparams.tid = tid;

  if (ioctl(qres_fd, IOCTL_OP_DETACH_FROM_SERVER, &iparams) < 0)
    return qos_int_rv(-errno);

  return QOS_OK;
}


#define QRES_DEV_PATHNAME QOS_DEV_PATH "/" QRES_DEV_NAME

qos_rv qres_init() {
  qres_fd = open(QRES_DEV_PATHNAME, O_RDONLY);
  if (qres_fd < 0) {
    qos_log_debug("Failed to open device %s", QRES_DEV_PATHNAME);
    return QOS_E_MISSING_COMPONENT;
  }
  return QOS_OK;
}


qos_rv qres_cleanup() {
  int fd = qres_fd;

  if (qres_fd == -1)
    return QOS_E_INCONSISTENT_STATE;

  qres_fd = -1;
  if (close(fd) < 0)
    return QOS_E_GENERIC;

  return QOS_OK;
}


qos_rv qres_module_exists () 
{
	FILE* module_file;
        char buffer[1024];

        module_file = fopen ("/proc/modules", "r");

        while (fgets (buffer, 1024, module_file) != NULL)
        {
                if (strstr (buffer, "qresmod") != NULL)
                        return QOS_OK;
        }
	return QOS_E_GENERIC;	
}


qos_rv qres_get_exec_time(qres_sid_t sid, qres_time_t *p_exec_time, qres_atime_t *p_abs_time) {
  qres_time_iparams_t iparams;

  qos_rv rv = check_open();
  qos_chk_rv(rv == QOS_OK, rv);

  iparams.server_id = sid;
  if (ioctl(qres_fd, IOCTL_OP_GET_EXEC_TIME, &iparams) < 0) {
    qos_log_err("Got error: %s", qos_strerror(qos_int_rv(-errno)));
    return qos_int_rv(-errno);
  }
  if (p_exec_time != NULL)
    *p_exec_time = iparams.exec_time;
  if (p_abs_time != NULL)
    *p_abs_time = iparams.abs_time;
  return QOS_OK;
}


qos_rv qres_get_curr_budget(qres_sid_t sid, qres_time_t *p_curr_budget) {
  qres_time_iparams_t iparams;
  qos_rv rv;

  qos_chk_ok_do(rv = check_open(), return rv);
  if (p_curr_budget == NULL)
    return QOS_E_INVALID_PARAM;

  iparams.server_id = sid;
  if (ioctl(qres_fd, IOCTL_OP_GET_CURR_BUDGET, &iparams) < 0) {
    rv = qos_int_rv(-errno);
    qos_log_err("Got error: %s", qos_strerror(rv));
    return rv;
  }
  *p_curr_budget = iparams.exec_time; /* used as current budget */
  return QOS_OK;
}


qos_rv qres_get_next_budget(qres_sid_t sid, qres_time_t *p_next_budget) {
  qres_time_iparams_t iparams;
  qos_rv rv;

  qos_chk_ok_do(rv = check_open(), return rv);
  if (p_next_budget == NULL)
    return QOS_E_INVALID_PARAM;

  iparams.server_id = sid;
  if (ioctl(qres_fd, IOCTL_OP_GET_NEXT_BUDGET, &iparams) < 0) {
    rv = qos_int_rv(-errno);
    qos_log_err("Got error: %s", qos_strerror(rv));
    return rv;
  }
  *p_next_budget = iparams.exec_time; /* used as next budget */
  return QOS_OK;
}


qos_rv qres_get_appr_budget(qres_sid_t sid, qres_time_t *p_appr_budget) {
  qres_time_iparams_t iparams;
  qos_rv rv;

  qos_chk_ok_do(rv = check_open(), return rv);
  if (p_appr_budget == NULL)
    return QOS_E_INVALID_PARAM;

  iparams.server_id = sid;
  if (ioctl(qres_fd, IOCTL_OP_GET_APPR_BUDGET, &iparams) < 0) {
    rv = qos_int_rv(-errno);
    qos_log_err("Got error: %s", qos_strerror(rv));
    return rv;
  }
  *p_appr_budget = iparams.exec_time; /* used as approved budget */
  return QOS_OK;
}


qos_rv qres_set_params(qres_sid_t sid, qres_params_t * p_params) {
  qres_iparams_t iparams;
  int err;

  qos_rv rv = check_open();
  qos_chk_rv(rv == QOS_OK, rv);

  iparams.server_id = sid;
  iparams.params = *p_params;
  err = ioctl(qres_fd, IOCTL_OP_SET_PARAMS, &iparams);
  if (err == -1)
    rv = qos_int_rv(-errno);
  else
    server_params = *p_params;
  return rv;
}


qos_rv qres_get_params(qres_sid_t sid, qres_params_t * qres_p) {
  qres_iparams_t iparams;
  int err;

  qos_rv rv = check_open();
  qos_chk_rv(rv == QOS_OK, rv);

  iparams.server_id = sid;
  err = ioctl(qres_fd, IOCTL_OP_GET_PARAMS, &iparams);
  if (err == 0) {
    memcpy(qres_p, &iparams.params, sizeof(qres_params_t));
    return QOS_OK;
  } else
    return qos_int_rv(-errno);
}


qos_rv qres_set_bandwidth(qres_sid_t sid, qos_bw_t bw) {
  qres_iparams_t iparams;
  int err;

  qos_rv rv = check_open();
  qos_chk_rv(rv == QOS_OK, rv);

  iparams.server_id = sid;
  iparams.params = server_params;
  iparams.params.Q = bw2Q(bw, server_params.P);
  err = ioctl(qres_fd, IOCTL_OP_SET_PARAMS, &iparams);
  if (err == -1) {
    qos_log_debug("ioctl() FAILED: %s", qos_strerror(qos_int_rv(-errno)));
    return qos_int_rv(-errno);
  }
  return QOS_OK;
}


qos_rv qres_get_bandwidth(qres_sid_t sid, float *bw) {
  qres_iparams_t iparams;
  int err;

  qos_chk_ok_ret(check_open());

  iparams.server_id = sid;
  err = ioctl(qres_fd, IOCTL_OP_GET_PARAMS, &iparams);
  if (err == 0)
    *bw = ((float) iparams.params.Q) / ((float) iparams.params.P);
  else
    return qos_int_rv(-errno);
  return QOS_OK;
}

qos_rv qres_get_deadline(qres_sid_t sid, struct timespec *p_deadline) {
  qres_timespec_iparams_t iparams;
  qos_rv rv;

  qos_chk_ok_do(rv = check_open(), return rv);
  if (p_deadline == NULL)
    return QOS_E_INVALID_PARAM;

  iparams.server_id = sid;
  if (ioctl(qres_fd, IOCTL_OP_GET_DEADLINE, &iparams) < 0) {
    rv = qos_int_rv(-errno);
    qos_log_err("Got error: %s", qos_strerror(rv));
    return rv;
  }
  *p_deadline = iparams.timespec; /* used as approved budget */
  return QOS_OK;
}

qos_rv qres_set_weight(qres_sid_t sid, unsigned int weight) {
  qres_weight_iparams_t iparams;
  int err;

  qos_rv rv = check_open();
  qos_chk_rv(rv == QOS_OK, rv);

  iparams.server_id = sid;
  iparams.weight = weight;
  err = ioctl(qres_fd, IOCTL_OP_SET_WEIGHT, &iparams);
  if (err == -1) {
    qos_log_debug("ioctl() FAILED: %s", qos_strerror(qos_int_rv(-errno)));
    return qos_int_rv(-errno);
  }
  return QOS_OK;
}

qos_rv qres_get_weight(qres_sid_t sid, unsigned int *p_weight) {
  qres_weight_iparams_t iparams;
  qos_rv rv;

  qos_chk_ok_do(rv = check_open(), return rv);
  if (p_weight == NULL)
    return QOS_E_INVALID_PARAM;

  iparams.server_id = sid;
  if (ioctl(qres_fd, IOCTL_OP_GET_WEIGHT, &iparams) < 0) {
    rv = qos_int_rv(-errno);
    qos_log_err("Got error: %s", qos_strerror(rv));
    return rv;
  }
  *p_weight = iparams.weight;
  return QOS_OK;
}

qos_rv qres_get_servers(qres_sid_t *sids, size_t *n) {
  FILE* proc_scheduler_file;
  char buffer[1024];
  int dim = 0;

  proc_scheduler_file = fopen ("/proc/aquosa/qres/scheduler", "r");
  if (proc_scheduler_file == NULL)
    return QOS_E_MISSING_COMPONENT;

  if (fgets (buffer, sizeof(buffer), proc_scheduler_file) == NULL
      || fgets (buffer, sizeof(buffer), proc_scheduler_file) == NULL
      || fgets (buffer, sizeof(buffer), proc_scheduler_file) == NULL) {
    *n = 0;
    return QOS_OK;
  }

  while (dim < *n && fgets (buffer, sizeof(buffer), proc_scheduler_file) != NULL)
    if (sscanf(buffer, "%d", &sids[dim]) == 1)
      dim++;
    else
      qos_log_err("Could not parse line: %s", buffer);

  fclose(proc_scheduler_file);

  *n = dim;

  return QOS_OK;
}
