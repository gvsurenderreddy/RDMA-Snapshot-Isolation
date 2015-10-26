/*
 *	benchmark-config.hpp
 *
 *  Created on: 27.Mar.2015
 *	Author: Erfan Zamanian
 */

#ifndef BENCHMARK_CONFIG_HPP_
#define BENCHMARK_CONFIG_HPP_

#include <stdint.h>
#include <string>

namespace benchmark_config {
	static const size_t BUFFER_WORDS 		= 1;	// # words (8 bytes)
	static const size_t SERVER_REGION_WORDS = 1000;	// # words (8 bytes)
	static const int OPERATIONS_CNT			= 5000000;

	enum VERB_TYPE_ENUM {READ, WRITE, CAS, FA};
	enum MEMORY_ACCESS_ENUM {SHARED, DEDICATED};
}
#endif /* BENCHMARK_CONFIG_H_ */
