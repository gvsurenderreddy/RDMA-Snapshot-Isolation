/*
 *	BaseContext.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: erfanz
 */

#include "BaseContext.hpp"
#include "utils.hpp"
#include "RDMACommon.hpp"

#include <iostream>

int BaseContext::create_context() {
	TEST_NZ (RDMACommon::build_connection(ib_port, &ib_ctx, &port_attr, &pd, &cq, &comp_channel, 10));
	TEST_NZ (register_memory());
	TEST_NZ (RDMACommon::create_queuepair(ib_ctx, pd, cq, &qp));
	return 0;
}