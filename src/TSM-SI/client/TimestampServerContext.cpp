/*
 * TrxServerContext.cpp
 *
 *  Created on: Oct 4, 2015
 *      Author: erfanz
 */

#include "TimestampServerContext.hpp"

#include "../../util/utils.hpp"
#include <iostream>

#define CLASS_NAME	"TSContext"

int TimestampServerContext::register_memory() {
	int mr_flags = IBV_ACCESS_LOCAL_WRITE;

	last_trx_timestamp_per_client = new primitive::timestamp_t[1000];

	TEST_Z(mr_recv			= ibv_reg_mr(pd, &recv_msg, sizeof(struct message::TimestampServerMemoryKeys), mr_flags));
	//TEST_Z(mr_trx_status	= ibv_reg_mr(pd, &trx_status, sizeof(uint8_t), mr_flags));
	//TEST_Z(mr_read_trx		= ibv_reg_mr(pd, &read_trx, sizeof(Timestamp), mr_flags));
	TEST_Z(mr_last_trx_timestamp_per_client	= ibv_reg_mr(pd, last_trx_timestamp_per_client, sizeof(primitive::timestamp_t) * 1000, mr_flags));
	TEST_Z(mr_commit_timestamp		= ibv_reg_mr(pd, &commit_timestamp, sizeof(primitive::timestamp_t), mr_flags));

	return 0;
}

int TimestampServerContext::destroy_context () {
	if (qp)				TEST_NZ (ibv_destroy_qp (qp));
	if (mr_recv)		TEST_NZ (ibv_dereg_mr (mr_recv));
	//if (mr_trx_status)	TEST_NZ (ibv_dereg_mr (mr_trx_status));
	//if (mr_read_trx)	TEST_NZ (ibv_dereg_mr (mr_read_trx));
	if (mr_last_trx_timestamp_per_client)	TEST_NZ (ibv_dereg_mr (mr_last_trx_timestamp_per_client));
	if (mr_commit_timestamp)	TEST_NZ (ibv_dereg_mr (mr_commit_timestamp));
	this->BaseContext::destroy_context();
	return 0;
}
