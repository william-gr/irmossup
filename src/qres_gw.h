#ifndef __QRES_GW_H__
#define __QRES_GW_H__

//#include <linux/types.h>
#include "qos_types.h"

/** @file
 *
 * @brief Common defines for the US to KS gateway
 *
 * This file defines the QoS Manager operation codes used
 * to request an operation to the KS running module.
 * See qres_gw_lib.h or qres_gw_lib.c for details.
 *
 * @ingroup QRES_LIB QRES_MOD
 */

/** QRES Parameters' specification structure */
typedef struct qres_params_t {
  qres_time_t Q_min;	/**< The server guaranteed budget	*/
  qres_time_t Q;	/**< The server actual budget (request)	*/
  qres_time_t P;	/**< The server period			*/
  unsigned int flags;	/**< Combination of QOS_F_* flags	*/
  qres_time_t timeout;	/**< Maximum auto-destroy time out	*/
} qres_params_t;

/** QRES time info returned by qres_get_exec_time()	*/
typedef struct qres_time_iparams_t {
  qres_sid_t server_id;		/**< Server identifier or QRES_SID_NULL	*/
  qres_time_t exec_time;	/**< Consumed computation time		*/
  qres_atime_t abs_time;	/**< Time since some point in the past	*/
} qres_time_iparams_t;

typedef struct qres_attach_iparams_t {
  qres_sid_t server_id;		/**< Server identifier or QRES_SID_NULL	*/
  pid_t pid;			/**< PID of the process to affect	*/
  tid_t tid;			/**< TID of the thread to affect	*/
} qres_attach_iparams_t;

/** QRES Parameters' internal specification structure
 *
 * Also includes the PID of the affected process, and
 * return parameters from KS to US	*/
typedef struct qres_iparams_t {
  qres_sid_t server_id;		/**< Server identifier or QRES_SID_NULL	*/
  qres_params_t params;		/**< Server static parameters		*/
} qres_iparams_t;

typedef struct qres_timespec_iparams_t {
  qres_sid_t server_id;		/**< Server identifier or QRES_SID_NULL	*/
  struct timespec timespec;	/**< Timespec information		*/
} qres_timespec_iparams_t;

/** Carries weight information for set/get weight */
typedef struct qres_weight_iparams_t {
  qres_sid_t server_id;         /**< Server identifier or QRES_SID_NULL */
  unsigned long weight;         /**< Weight information                 */
} qres_weight_iparams_t;

/** Types of operation that can be requested to the QRES module */
typedef enum {
  QRES_OP_CREATE_SERVER,
  QRES_OP_DESTROY_SERVER,
  QRES_OP_ATTACH_TO_SERVER,
  QRES_OP_DETACH_FROM_SERVER,
  QRES_OP_SET_PARAMS,
  QRES_OP_GET_PARAMS,
  QRES_OP_GET_EXEC_TIME,
  QRES_OP_GET_CURR_BUDGET,
  QRES_OP_GET_SERVER_ID,
  QRES_OP_GET_NEXT_BUDGET,
  QRES_OP_GET_APPR_BUDGET,
  QRES_OP_GET_DEADLINE,
  QRES_OP_SET_WEIGHT,
  QRES_OP_GET_WEIGHT
} qres_op_t;

/** Name of the QoS Manager device used to	*
 * communicate with the kernel module		*/
#define QRES_DEV_NAME "qosres"

/** Major number for the QoS Manager device, set	*
 * as the first number in the 'local/experimental'	*
 * range from Documentation/devices.txt			*/
#define QRES_MAJOR_NUM 240


#endif
