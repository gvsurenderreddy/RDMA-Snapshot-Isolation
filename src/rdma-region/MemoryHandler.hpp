/*
 * MemoryHandler.hpp
 *
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#ifndef SRC_RDMA_REGION_MEMORYHANDLER_HPP_
#define SRC_RDMA_REGION_MEMORYHANDLER_HPP_

#include <infiniband/verbs.h>


template <class T>  class MemoryHandler{
public:
	struct ibv_mr	rdmaHandler_;	// for remote access
	T				*region_;		// for local access
	size_t			regionSize_;
};


#endif /* SRC_RDMA_REGION_MEMORYHANDLER_HPP_ */
