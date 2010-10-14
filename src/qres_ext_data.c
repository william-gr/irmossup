/** Allocate ext_data struct, associate it to tsk and attach qsup to ext_data */
qos_rv qres_setup_qsup_for_rres(server_t *rres, qsup_server_t *qsup);

/**
 * External data attached to each RR server by the QRES.
 *
 * This is used for attaching to each RR server multiple pointers,
 * by using the rres_set_ext_data and rres_get_ext_data RR functions
 * to attach a single qres_ext_data_t instance.
 */
typedef struct qres_ext_data {
  void *data[QRES_SLOT_NUMBER]; /**< QoS Sup, QoS Mgr, RFU  */
} qres_ext_data_t;

/** Attach to ext_data custom data at slot s, with parameter checking */
qos_rv qres_attach_to_ext_data(qres_ext_data_t * ext_data, qres_slot_t s, void *data);

/*
 * RR slots management
 */

/** Available slots for attaching custom data to each RR server.
 *
 * @see qres_slot_t
 */
typedef enum {
  QRES_SLOT_QSUP, /**< Reserved to the QRES module, points qsup_server_t  */
  QRES_SLOT_QMGR, /**< Reserved to the QMGR module, points to qmgr_t. */
  QRES_SLOT_RFU,  /**< Reserved for Future Use.       */
  QRES_SLOT_NUMBER  /**< Num of slots: must be last value     */
} qres_slot_t;

/** Attach to RRES server custom data at slot s.
 *
 * This function may only be invoked after the qres_create_server
 * attached a qres_ext_data_t struct to the created RRES server */
qos_rv qres_attach_data_to_rres(server_t *rres, qres_slot_t s, void *data);

/** Attach to RRES server (by task) custom data at slot s.
 *
 * This function may only be invoked after the qres_create_server
 * attached a qres_ext_data_t struct to the created RRES server */
qos_rv qres_attach_data_to_task(struct task_struct *tsk, qres_slot_t s, void *data);

/** Retrieve custom data at slot s from an RRES server.
 *
 * This function may only be invoked after the qres_create_server
 * attached an ext_data_t struct to the created RRES server
 * @todo  we want compile out all checks when DEBUG not defined */
qos_rv qres_retrieve_from_rres(server_t *rres, int s, void **data);

static inline qos_rv qres_retrieve_from_task(struct task_struct *tsk, int s, void **data) {
  server_t *rres = rres_find_by_task(tsk);
  if (rres == 0)
    return QOS_E_INCONSISTENT_STATE;
  return qres_retrieve_from_rres(rres, s, data);
}

/** Allocate ext_data struct, associate it to tsk and attach qsup to ext_data */
qos_rv qres_setup_qsup_for_rres(server_t *rres, qsup_server_t *qsup) {
  int i;
  /** Create the qres_ext_data array and attach to rres */
  qres_ext_data_t *ext_data = qos_malloc(sizeof(qres_ext_data_t));
  if (ext_data == 0) {
    qos_log_err("Couldn't allocate memory for ext_data");
    return QOS_E_NO_MEMORY;
  }
  /* Init ext_data entries to 0 */
  for (i=0; i<QRES_SLOT_NUMBER; i++)
    ext_data->data[i] = 0;

  /** Store the ExtData pointer into the RRES struct  */
  rres_set_ext_data(rres, ext_data);

  /** Store the qsup pointer into the RRES struct */
  qres_attach_to_ext_data(ext_data, QRES_SLOT_QSUP, qsup);

  return QOS_OK;
}

qos_rv qres_attach_to_ext_data(qres_ext_data_t * ext_data, qres_slot_t s, void *data) {
  if (ext_data == 0) {
    qos_log_err("No ext_data set in rres server, yet");
    return QOS_E_INTERNAL_ERROR;
  }
  if ((s < 0) || (s >= QRES_SLOT_NUMBER)) {
    qos_log_err("Index out of range");
    return QOS_E_INVALID_PARAM;
  }
  ext_data->data[s] = data;
  return QOS_OK;
}

qos_rv qres_attach_data_to_rres(server_t *rres, qres_slot_t s, void *data) {
  return qres_attach_to_ext_data(rres_get_ext_data(rres), s, data);
}

qos_rv qres_attach_data_to_task(struct task_struct *tsk, qres_slot_t s, void *data) {
  server_t *rres = rres_find_by_task(tsk);
  if (rres == 0)
    return QOS_E_INCONSISTENT_STATE;
  if (s == QRES_SLOT_QSUP)
    return QOS_E_INVALID_PARAM;
  return qres_attach_data_to_rres(rres, s, data);
}

qos_rv qres_retrieve_from_rres(server_t *rres, int s, void **data) {
  qres_ext_data_t * ext_data;
  if (rres == 0) {
    qos_log_err("NULL rres pointer");
    return QOS_E_INTERNAL_ERROR;
  }
  ext_data = rres_get_ext_data(rres);
  if (ext_data == 0) {
    qos_log_err("No ext_data set in rres module");
    return QOS_E_INTERNAL_ERROR;
  }
  if ((s < 0) || (s >= QRES_SLOT_NUMBER)) {
    qos_log_err("Index out of range");
    return QOS_E_INVALID_PARAM;
  }
  *data = ext_data->data[s];
  return QOS_OK;
}

EXPORT_SYMBOL_GPL(qres_attach_data_to_rres);
EXPORT_SYMBOL_GPL(qres_attach_data_to_task);
EXPORT_SYMBOL_GPL(qres_retrieve_from_rres);
