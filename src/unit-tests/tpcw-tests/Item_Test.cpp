/*
 * TPCCTests.cpp
 *
 *  Created on: Feb 21, 2016
 *      Author: erfanz
 */

#include "Item_Test.hpp"
#include "../../benchmarks/TPC-C/tables/ItemTable.hpp"
#include <assert.h>


#define CLASS_NAME	"TPCC_Tests"


//std::vector<std::function<void(TestBase&)>> Item_Test::functionList_ {
//	&Item_Test::test_constructor
//};

std::vector<std::function<void(TestBase&)>>& Item_Test::getFunctionList() {
	//return PointerTest::functionList;
	return functionList_;
}

void Item_Test::test_constructor() {
	TestBase::printMessage(CLASS_NAME, __func__);
	assert(itemTable->getSize() == 10);
	assert(itemTable->getMaxVersionsCnt() == 2);
}

Item_Test::Item_Test() {
	itemTable = new TPCC::ItemTable(10, 2);
}

Item_Test::~Item_Test() {
	// TODO Auto-generated destructor stub
}

