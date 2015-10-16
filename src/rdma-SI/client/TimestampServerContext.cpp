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

	TEST_Z(recv_mr					= ibv_reg_mr(pd, &recv_msg, sizeof(struct message::TimestampServerMemoryKeys), mr_flags));
	TEST_Z(transaction_result_mr	= ibv_reg_mr(pd, &transaction_result, sizeof(struct message::TransactionResult), mr_flags));
	TEST_Z(transaction_result_ack_mr= ibv_reg_mr(pd, &transaction_result_ack, sizeof(struct message::TransactionResult), mr_flags));
	TEST_Z(read_timestamp_mr		= ibv_reg_mr(pd, &read_timestamp, sizeof(Timestamp), mr_flags));
	TEST_Z(commit_timestamp_mr		= ibv_reg_mr(pd, &commit_timestamp, sizeof(Timestamp), mr_flags));
	return 0;
}

int TimestampServerContext::destroy_context () {
	if (qp)						TEST_NZ (ibv_destroy_qp (qp));
	if (recv_mr)				TEST_NZ (ibv_dereg_mr (recv_mr));
	if (transaction_result_mr)	TEST_NZ (ibv_dereg_mr (transaction_result_mr));
	if (transaction_result_ack_mr)	TEST_NZ (ibv_dereg_mr (transaction_result_ack_mr));
	if (read_timestamp_mr)		TEST_NZ (ibv_dereg_mr (read_timestamp_mr));
	if (commit_timestamp_mr)	TEST_NZ (ibv_dereg_mr (commit_timestamp_mr));

	this->BaseContext::destroy_context();
	return 0;
}
