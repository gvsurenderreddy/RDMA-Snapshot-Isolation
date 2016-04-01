/*
 * MultiValueHashIndex_Test.hpp
 *
 *  Created on: Mar 30, 2016
 *      Author: erfanz
 */

#ifndef SRC_UNIT_TESTS_INDEX_MULTIVALUEHASHINDEX_TEST_HPP_
#define SRC_UNIT_TESTS_INDEX_MULTIVALUEHASHINDEX_TEST_HPP_

#include "../TestBase.hpp"
#include "../../index/hash/MultiValueHashIndex.hpp"
#include <functional>	// std::function
#include <vector>

class MultiValueHashIndex_Test : public TestBase {
private:
	static std::vector<std::function<void()>> functionList_;
public:
	static std::vector<std::function<void()>>& getFunctionList();
	static void test_get_singlevalue_key();
	static void test_get_multivalued_key();
	static void test_get_nonexisting_key();
	static void test_get_deleted_key();
	static void test_append_nonexisting_key();
	static void test_append_deleted_key();
	static void test_append_million_values();
};

#endif /* SRC_UNIT_TESTS_INDEX_MULTIVALUEHASHINDEX_TEST_HPP_ */
