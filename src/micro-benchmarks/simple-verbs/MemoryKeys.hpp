/*
 *	MemoryKeys.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef MEMORY_KEYS_H_
#define MEMORY_KEYS_H_

#include <infiniband/verbs.h>

struct MemoryKeys {
	struct ibv_mr peer_mr;
	uint16_t client_num;
};


#endif /* MEMORY_KEYS_H_ */
