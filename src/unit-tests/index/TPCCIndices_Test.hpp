/*
 * TPCCIndices_Test.hpp
 *
 *  Created on: Mar 31, 2016
 *      Author: erfanz
 */

#ifndef SRC_UNIT_TESTS_INDEX_TPCCINDICES_TEST_HPP_
#define SRC_UNIT_TESTS_INDEX_TPCCINDICES_TEST_HPP_

#include "../TestBase.hpp"
#include "../../benchmarks/TPC-C/tables/TPCCDB.hpp"
#include "../../rdma-region/RDMAContext.hpp"
#include "../../benchmarks/TPC-C/random/randomgenerator.hpp"
#include "../../../config.hpp"
#include <functional>	// std::function
#include <vector>

using namespace config::tpcc_settings;

class TPCCIndices_Test: public TestBase {
private:
	static std::vector<std::function<void()>> functionList_;
	static TPCC::TPCCDB *db;
	static RDMAContext *context;

public:
	static std::vector<std::function<void()>>& getFunctionList();
	static void setup();
	static void test_getExistingCustomerByLastName();
	static void test_getNonExistingCustomerByLastName();

	// Order indices
	static void test_clearOrderIndex();
	static void test_getExistingOrderAddressByOrderID();
	static void test_getNonExistingOrderAddressByOrderID();
	static void test_getLastOrderOfCustomer();
	static void test_getNonExistingLastOrderOfCustomer();
	static void cleanup();

};

#endif /* SRC_UNIT_TESTS_INDEX_TPCCINDICES_TEST_HPP_ */
