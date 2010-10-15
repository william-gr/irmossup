/** @addtogroup RRES_MOD Resource Reservation Low Level Implementation
 * 
 * @ingroup QRES
 * 
 * @{
 */

/** @file
 * @brief RRES Module protected interface header.
 * 
 * The functions exported through this header file are equivalent to C++
 * "protected" methods, which are expected to be used directly by methods
 * of a subclass which override them, in place of the public calls, which,
 * by realizing a virtual call, would result in a recursive activation of
 * the overridden version of the function. 
 */

#ifndef RRES_PROTECTED_KPI_H_
#define RRES_PROTECTED_KPI_H_

#include "qos_debug.h"
#include "rres_server.h"

/** Non-virtual server destructor             */
qos_rv _rres_cleanup_server(server_t *srv);

/** Non-virtual bandwidth retrieval function  */
qos_bw_t _rres_get_bandwidth(server_t *srv);

int rres_has_ready_tasks(server_t *srv);

/** Return the approved bandwidth for the server.
 **
 ** @note
 ** This may be higher than the bandwidth that is going to be
 ** used for the very next server instance, that may be retrieved
 ** through rres_get_current_bandwidth().
 **/
static inline qos_bw_t rres_get_bandwidth(server_t *srv) {
  return srv->get_bandwidth(srv);
}

/** This function sets the current bandwidth U_current as specified in the
 ** supplied parameter, updating the global variables U_tot and U_active_tot.
 **/
static inline qos_bw_t rres_set_current_bandwidth(server_t *srv, qos_bw_t U_approved) {
  qos_bw_t U_max_avail, U_new;
  /* Compute max available for server           */
  U_max_avail = U_LUB2 - (U_tot - srv->U_current);
  /* Assign minimum between authorized value and available value */
  U_new = (U_approved < U_max_avail ? U_approved : U_max_avail);
  qos_log_debug("U_tot=" QOS_BW_FMT ", U_curr_old=" QOS_BW_FMT ", U_appr=" QOS_BW_FMT
                ", U_avail=" QOS_BW_FMT ", U_curr_new=" QOS_BW_FMT,
                U_tot, srv->U_current, U_approved, U_max_avail, U_new);
  U_tot = U_tot - srv->U_current + U_new;
#ifdef SHRUB
  if (rres_has_ready_tasks(srv))
    U_active_tot = U_active_tot - srv->U_current + U_new;
#endif
  srv->U_current = U_new;
  return U_new;
}

static inline qos_bw_t rres_update_current_bandwidth(server_t *srv) {
  return rres_set_current_bandwidth(srv, rres_get_bandwidth(srv));
}

/** Return the current bandwidth allocated to the server.
 **
 ** This is the value that is going to be used for the very next
 ** server instance, and its value is updated by the
 ** rres_update_current_bandwidth() function.
 **/
static inline qos_bw_t rres_get_current_bandwidth(server_t *srv) {
  return srv->U_current;
}

#endif /*RRES_PROTECTED_KPI_H_*/

/** @} */
