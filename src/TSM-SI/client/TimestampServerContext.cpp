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

	TEST_Z(mr_recv			= ibv_reg_mr(pd, &recv_msg, sizeof(struct message::TimestampServerMemoryKeys), mr_flags));
	TEST_Z(mr_trx_status	= ibv_reg_mr(pd, &trx_status, sizeof(uint8_t), mr_flags));
	TEST_Z(mr_read_epoch	= ibv_reg_mr(pd, &read_epoch, sizeof(Timestamp), mr_flags));
	return 0;
}

int TimestampServerContext::destroy_context () {
	if (qp)				TEST_NZ (ibv_destroy_qp (qp));
	if (mr_recv)		TEST_NZ (ibv_dereg_mr (mr_recv));
	if (mr_trx_status)	TEST_NZ (ibv_dereg_mr (mr_trx_status));
	if (mr_read_epoch)	TEST_NZ (ibv_dereg_mr (mr_read_epoch));

	this->BaseContext::destroy_context();
	return 0;
}
