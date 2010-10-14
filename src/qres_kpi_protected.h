/** @file
 ** @brief Protected Kernel Programming Interface for QRES Module.
 **
 ** This include file may be used by classes that wish to extend the QRES
 ** server "class". For example, it is needed in order to make non-virtual
 ** calls to QRES operations that would otherwise generate a virtual call.
 **/

#ifndef QRES_KPI_PROTECTED_H_
#define QRES_KPI_PROTECTED_H_

#include "qres_interface.h"

qos_bw_t _qres_get_bandwidth(server_t *srv);
qos_rv _qres_cleanup_server(server_t *rres);

#endif /*QRES_KPI_PROTECTED_H_*/
