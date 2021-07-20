/*
 * CMGD Backend Client Connection Adapter
 * Copyright (C) 2021  Vmware, Inc.
 *		       Pushpasis Sarkar <spushpasis@vmware.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; see the file COPYING; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _FRR_CMGD_BCKND_ADAPTER_H_
#define _FRR_CMGD_BCKND_ADAPTER_H_

#include "lib/typesafe.h"
#include "cmgd/cmgd_defines.h"
#include "lib/cmgd_bcknd_client.h"

typedef enum cmgd_bcknd_req_type_ {
        CMGD_BCKND_REQ_NONE = 0,
        CMGD_BCKND_REQ_CFG_VALIDATE,
        CMGD_BCKND_REQ_CFG_APPLY,
        CMGD_BCKND_REQ_DATA_GET_ELEM,
        CMGD_BCKND_REQ_DATA_GET_NEXT
} cmgd_bcknd_req_type_t;

typedef struct cmgd_bcknd_cfgreq_ {
        cmgd_bcknd_req_type_t req_type;
        struct nb_yang_xpath_elem elems[GMGD_BCKND_MAX_NUM_REQ_ITEMS];
        int num_elems;
} cmgd_bcknd_cfgreq_t;

typedef struct cmgd_bcknd_datareq_ {
        cmgd_bcknd_req_type_t req_type;
        struct nb_yang_xpath xpaths[GMGD_BCKND_MAX_NUM_REQ_ITEMS];
        int num_xpaths;
} cmgd_bcknd_datareq_t;

typedef struct cmgd_bcknd_dataresults_ {
        cmgd_bcknd_req_type_t req_type;
        struct nb_yang_xpath_elem elems[GMGD_BCKND_MAX_NUM_REQ_ITEMS];
        int num_elems;
        int next_data_indx;
} cmgd_bcknd_dataresults_t;

typedef void (*cmgd_bcknd_trxn_result_notify_t)(
        cmgd_trxn_id_t trxn_id, cmgd_result_t result);

typedef void (*cmgd_bcknd_cfg_result_notify_t)(
        cmgd_trxn_id_t trxn_id, cmgd_trxn_batch_id_t batch_id,
        cmgd_result_t result, cmgd_bcknd_req_type_t orig_req);

typedef void (*cmgd_bcknd_get_data_result_notify_t)(
        cmgd_trxn_id_t trxn_id, cmgd_trxn_batch_id_t batch_id,
        cmgd_result_t result, cmgd_bcknd_dataresults_t *data_resp);

typedef void (*cmgd_bcknd_data_notify_t)(
        struct nb_yang_xpath *xpaths[], int num_xpaths);

PREDECL_LIST(cmgd_bcknd_adptr_list);
PREDECL_LIST(cmgd_trxn_badptr_list);

typedef struct cmgd_bcknd_client_adapter_ {
        int conn_fd;
        union sockunion conn_su;
        struct thread *conn_read_ev;
        struct thread *conn_write_ev;
        struct thread *proc_msg_ev;
        char name[CMGD_CLIENT_NAME_MAX_LEN];
        uint8_t num_xpath_reg;
        char xpath_reg[CMGD_MAX_NUM_XPATH_REG][CMGD_MAX_XPATH_LEN];

        cmgd_bcknd_trxn_result_notify_t trxn_result_cb;
        cmgd_bcknd_cfg_result_notify_t cfgresult_cb;
        cmgd_bcknd_get_data_result_notify_t dataresult_cb;
        cmgd_bcknd_data_notify_t notifydata_cb;

        /* IO streams for read and write */
	// pthread_mutex_t ibuf_mtx;
	struct stream_fifo *ibuf_fifo;
	// pthread_mutex_t obuf_mtx;
	// struct stream_fifo *obuf_fifo;

	// /* Private I/O buffers */
	struct stream *ibuf_work;
	// struct stream *obuf_work;

	/* Buffer of data waiting to be written to client. */
	// struct buffer *wb;

        int refcount;

        struct cmgd_bcknd_adptr_list_item list_linkage;
        struct cmgd_trxn_badptr_list_item trxn_list_linkage;
} cmgd_bcknd_client_adapter_t;

DECLARE_LIST(cmgd_bcknd_adptr_list, cmgd_bcknd_client_adapter_t, list_linkage);
DECLARE_LIST(cmgd_trxn_badptr_list, cmgd_bcknd_client_adapter_t, trxn_list_linkage);

extern int cmgd_bcknd_adapter_init(struct thread_master *tm);

extern void cmgd_bcknd_adapter_lock(cmgd_bcknd_client_adapter_t *adptr);

extern void cmgd_bcknd_adapter_unlock(cmgd_bcknd_client_adapter_t **adptr);

extern cmgd_bcknd_client_adapter_t *cmgd_bcknd_create_adapter(
        int conn_fd, union sockunion *su);

extern cmgd_bcknd_client_adapter_t *cmgd_bcknd_get_adapter(const char *name);

extern int cmgd_bcknd_create_trxn(
        cmgd_bcknd_client_adapter_t *adptr, cmgd_trxn_id_t trxn_id);

extern int cmgd_bcknd_destroy_trxn(
        cmgd_bcknd_client_adapter_t *adptr, cmgd_trxn_id_t trxn_id);

extern int cmgd_bcknd_send_cfg_req(
        cmgd_bcknd_client_adapter_t *adptr, cmgd_trxn_id_t trxn_id,
        cmgd_trxn_batch_id_t batch_id, cmgd_bcknd_cfgreq_t *cfg_req);

extern int cmgd_bcknd_send_get_data_req(
        cmgd_bcknd_client_adapter_t *adptr, cmgd_trxn_id_t trxn_id,
        cmgd_trxn_batch_id_t batch_id, cmgd_bcknd_datareq_t *data_req);

extern int cmgd_bcknd_send_get_next_data_req(
        cmgd_bcknd_client_adapter_t *adptr, cmgd_trxn_id_t trxn_id,
        cmgd_trxn_batch_id_t batch_id, cmgd_bcknd_datareq_t *data_req);

extern void cmgd_bcknd_adapter_status_write(struct vty *vty);

#endif /* _FRR_CMGD_BCKND_ADAPTER_H_ */