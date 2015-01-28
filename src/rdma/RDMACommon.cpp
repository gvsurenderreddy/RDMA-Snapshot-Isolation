/*
 *	RDMACommon.cpp
 *
 *	Created on: 26.01.2015
 *	Author: erfanz
 */


#include "RDMACommon.hpp"
#include <iostream>
#include <cstring>
using namespace std;


int RDMACommon::post_SEND (struct ibv_qp *qp, struct ibv_mr *local_mr, uintptr_t local_buffer, uint32_t length)
{
	struct ibv_send_wr sr;
	struct ibv_sge sge;
	struct ibv_send_wr *bad_wr = NULL;
	int rc;

	/* prepare the scatter/gather entry */
	memset (&sge, 0, sizeof (sge));
	sge.addr		= local_buffer;
	sge.length		= length;
	sge.lkey		= local_mr->lkey;
	/* prepare the send work request */
	memset (&sr, 0, sizeof (sr));
	sr.next			= NULL;
	sr.wr_id		= 0;
	sr.sg_list		= &sge;
	sr.num_sge		= 1;
	sr.opcode		= IBV_WR_SEND;
	sr.send_flags	= IBV_SEND_SIGNALED;
	
	/* there is a Receive Request in the responder side, so we won't get any into RNR flow */
	rc = ibv_post_send (qp, &sr, &bad_wr);
	if (rc != 0)
		cerr << "Failed to post Send Request" << endl;
	else
		cout << "Send Request was posted" << endl;
	
	return rc;
}

int RDMACommon::post_RECEIVE (struct ibv_qp *qp, struct ibv_mr *local_mr, uintptr_t local_buffer, uint32_t length)
{
	struct ibv_recv_wr wr;
	struct ibv_sge sge;
	struct ibv_recv_wr *bad_wr;
	int rc;
	/* prepare the scatter/gather entry */
	memset (&sge, 0, sizeof (sge));
	sge.addr		= local_buffer;
	sge.length		= length;
	sge.lkey		= local_mr->lkey;
	/* prepare the receive work request */
	memset (&wr, 0, sizeof (wr));
	wr.next			= NULL;
	wr.wr_id		= 0;
	wr.sg_list		= &sge;
	wr.num_sge		= 1;
	/* post the Receive Request to the RQ */
	rc = ibv_post_recv (qp, &wr, &bad_wr);
	if (rc != 0)
		cerr << "Failed to post Receive Request" << endl;
	else
		cerr << "Receive Request was posted" << endl;
	
	return rc;
}

int RDMACommon::create_queuepair(struct ibv_context *ib_ctx, struct ibv_pd *pd, struct ibv_cq *cq, struct ibv_qp **qp)
{
	struct ibv_exp_device_attr dev_attr;
	struct ibv_exp_qp_init_attr	attr;
	
	memset(&dev_attr, 0, sizeof(dev_attr));
	dev_attr.comp_mask |= IBV_EXP_DEVICE_ATTR_EXT_ATOMIC_ARGS | IBV_EXP_DEVICE_ATTR_EXP_CAP_FLAGS;
	if (ibv_exp_query_device(ib_ctx, &dev_attr)) {
		fprintf(stderr, "ibv_exp_query_device failed\n");
		return -1;
	}

	memset(&attr, 0, sizeof(struct ibv_exp_qp_init_attr));
	attr.pd = pd;
	attr.send_cq = cq;
	attr.recv_cq = cq;
	attr.cap.max_send_wr  = 1;
	attr.cap.max_send_sge = 1;
	attr.cap.max_inline_data = 0;
	attr.cap.max_recv_wr  = 1;
	attr.cap.max_recv_sge = 1;
	//attr.max_atomic_arg = pow(2,5);
	attr.max_atomic_arg = 32;
	attr.exp_create_flags = IBV_EXP_QP_CREATE_ATOMIC_BE_REPLY;
	attr.comp_mask = IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS | IBV_EXP_QP_INIT_ATTR_PD;
	attr.comp_mask |= IBV_EXP_QP_INIT_ATTR_ATOMICS_ARG;
	attr.srq = NULL;
	attr.qp_type = IBV_QPT_RC;
	
	(*qp) = ibv_exp_create_qp(ib_ctx, &attr);
   	if (!(*qp))
	{
		cerr << "failed to create QP" << endl;
		return -1;
	}
	fprintf (stdout, "QP was created, QP number=0x%x\n", (*qp)->qp_num);
	
	if ((*qp) == NULL || (*qp) == 0)
		cout << "queue pair is REALLY empty" << endl;
	
	
	return 0;
}


int RDMACommon::modify_qp_to_init (int ib_port, struct ibv_qp *qp)
{
	struct ibv_qp_attr attr;
	int flags;
	int rc;
	memset (&attr, 0, sizeof (attr));
	attr.qp_state = IBV_QPS_INIT;
	attr.port_num = ib_port;
	attr.pkey_index = 0;
	attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
	flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
	rc = ibv_modify_qp (qp, &attr, flags);
	if (rc)
		cerr << "failed to modify QP state to INIT" << endl;
	return rc;
}


int RDMACommon::modify_qp_to_rtr (int ib_port, struct ibv_qp *qp, uint32_t remote_qpn, uint16_t dlid, uint8_t * dgid)
{
	struct ibv_qp_attr attr;
	int flags;
	int rc;
	memset (&attr, 0, sizeof (attr));
	attr.qp_state = IBV_QPS_RTR;
	attr.path_mtu = IBV_MTU_256;
	attr.dest_qp_num = remote_qpn;
	attr.rq_psn = 0;
	attr.max_dest_rd_atomic = 1;
	attr.min_rnr_timer = 0x12;
	attr.ah_attr.is_global = 0;
	attr.ah_attr.dlid = dlid;
	attr.ah_attr.sl = 0;
	attr.ah_attr.src_path_bits = 0;
	attr.ah_attr.port_num = ib_port;

	flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
		IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
	rc = ibv_modify_qp (qp, &attr, flags);
	if (rc)
		cerr << "failed to modify QP state to RTR" << endl;
	return rc;
}

int RDMACommon::modify_qp_to_rts (struct ibv_qp *qp)
{
	struct ibv_qp_attr attr;
	int flags;
	int rc;
	memset (&attr, 0, sizeof (attr));
	attr.qp_state = IBV_QPS_RTS;
	attr.timeout = 0x12;
	attr.retry_cnt = 6;
	attr.rnr_retry = 0;
	attr.sq_psn = 0;
	attr.max_rd_atomic = 1;
	flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
		IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
	rc = ibv_modify_qp (qp, &attr, flags);
	if (rc)
		cerr << "failed to modify QP state to RTS" << endl;
	return rc;
}

int RDMACommon::poll_completion(struct ibv_cq* cq)
{
	struct ibv_wc wc;
	int ne;
	do {
		wc.status = IBV_WC_SUCCESS;
		ne = ibv_poll_cq(cq, 1, &wc);
		if(wc.status != IBV_WC_SUCCESS) {
			cerr << "RDMA completion event in CQ with error!" << endl;
			return -1;
		}
	}while(ne==0);

	if(ne<0) {
		cerr << "RDMA polling from CQ failed!" << endl;
		return -1;
	}
	cout << ("WQ polled from CQ successfully") << endl;
	return 0;
}
