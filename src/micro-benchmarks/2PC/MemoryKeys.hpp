/*
 *	MemoryKeys.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: erfanz
 */

#ifndef MEMORY_KEYS_H_
#define MEMORY_KEYS_H_

#include <infiniband/verbs.h>

struct MemoryKeys {
	struct ibv_mr peer_mr;
};


#endif /* MEMORY_KEYS_H_ */