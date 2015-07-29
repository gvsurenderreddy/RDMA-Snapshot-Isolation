/*
 *	IPCoordinatorContext.cpp
 *
 *	Created on: 26.Mar.2015
 *	Author: erfanz
 */

#include "IPCoordinatorContext.hpp"
#include "../../../../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>


int IPCoordinatorContext::create_context() {
	return 0;
}

int IPCoordinatorContext::register_memory() {
	return 0;
}

int IPCoordinatorContext::destroy_context () {
	if (sockfd >= 0)
		TEST_NZ (close (sockfd));
	
	return 0;
}