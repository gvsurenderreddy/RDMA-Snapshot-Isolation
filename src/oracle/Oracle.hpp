/*
 *	Oracle.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef SRC_ORACLE_ORACLE_HPP_
#define SRC_ORACLE_ORACLE_HPP_

#include "../../config.hpp"
#include "../rdma-region/MemoryHandler.hpp"
#include "../rdma-region/RDMAContext.hpp"
#include "../rdma-region/RDMARegion.hpp"
#include "../basic-types/timestamp.hpp"
#include "../basic-types/PrimitiveTypes.hpp"
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>

class Oracle{
private:
	const size_t	clients_cnt_;	// coming from the command line.
	int				server_sockfd_;	// Server's socket file descriptor
	uint16_t		tcp_port_;
	uint8_t			ib_port_;

	RDMAContext *context_;
	RDMARegion<primitive::timestamp_t> *lastCommittedVector_;
	
public:
	Oracle(size_t clients_cnt);
	~Oracle();
};
#endif /* SRC_ORACLE_ORACLE_HPP_ */
