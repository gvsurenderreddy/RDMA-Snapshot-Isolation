/*
 *	IPTradTrxManagerContext.cpp
 *
 *	Created on: 21.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "IPTradTrxManagerContext.hpp"
#include "../../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>

// Override
int IPTradTrxManagerContext::create_context() {
	return 0;
}

int IPTradTrxManagerContext::register_memory() {
	return 0;
}

int IPTradTrxManagerContext::destroy_context () {
	//for (int s = 0; s < SERVER_CNT; s++)
	//	TEST_NZ (res_mng_ctxs[s].destroy_context());	
	
	TEST_NZ (client_ctx.destroy_context());	
	return 0;
}