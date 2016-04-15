/*
 * Timestamp_Test.hpp
 *
 *  Created on: Apr 15, 2016
 *      Author: erfanz
 */

#ifndef SRC_UNIT_TESTS_BASIC_TYPES_TIMESTAMP_TEST_HPP_
#define SRC_UNIT_TESTS_BASIC_TYPES_TIMESTAMP_TEST_HPP_

#include "../TestBase.hpp"
#include "../../basic-types/timestamp.hpp"
#include "../../basic-types/PrimitiveTypes.hpp"
#include "../../util/utils.hpp"
#include <functional>	// std::function
#include <vector>
#include <assert.h>
#include <stdexcept>      // std::out_of_range



class Timestamp_Test : public TestBase {
private:
	static std::vector<std::function<void()>> functionList_;
public:
	static std::vector<std::function<void()>>& getFunctionList();
	static void test_construct();
	static void test_set_all();
	static void test_locking();
	static void test_deleting();
	static void test_equality();
};



#endif /* SRC_UNIT_TESTS_BASIC_TYPES_TIMESTAMP_TEST_HPP_ */
