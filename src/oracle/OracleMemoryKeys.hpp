/*
 * OracleMemoryKeys.hpp
 *
 *  Created on: Feb 29, 2016
 *      Author: erfanz
 */

#ifndef SRC_ORACLE_ORACLEMEMORYKEYS_HPP_
#define SRC_ORACLE_ORACLEMEMORYKEYS_HPP_

#include "../rdma-region/MemoryHandler.hpp"
#include "../basic-types/PrimitiveTypes.hpp"

struct OracleMemoryKeys{
public:
	MemoryHandler<primitive::timestamp_t> lastCommittedVector;
	primitive::client_id_t client_id;
	size_t client_cnt;
};

#endif /* SRC_ORACLE_ORACLEMEMORYKEYS_HPP_ */
