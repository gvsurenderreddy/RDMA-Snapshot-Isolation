/*
 *	IPTradClientContext.cpp
 *
 *	Created on: 20.Feb.2015
 *	Author: erfanz
 */

#include "IPTradClientContext.hpp"
#include "../../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>

int IPTradClientContext::create_context() {
	return 0;
}

int IPTradClientContext::register_memory() {
	return 0;
}

int IPTradClientContext::destroy_context () {
	if (sockfd >= 0)	TEST_NZ (close (sockfd));
	return 0;
}