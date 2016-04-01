/*
 * MultiValueHashIndex_Test.cpp
 *
 *  Created on: Mar 30, 2016
 *      Author: erfanz
 */

#include "MultiValueHashIndex_Test.hpp"
#include <assert.h>
#include <stdexcept>      // std::out_of_range
#include "../../util/utils.hpp"

#define CLASS_NAME	"MultiValueHashIndex_Test"


std::vector<std::function<void()>> MultiValueHashIndex_Test::functionList_ {
	test_get_singlevalue_key,
	test_get_multivalued_key,
	test_get_nonexisting_key,
	test_get_deleted_key,
	test_append_nonexisting_key,
	test_append_deleted_key,
	test_append_million_values
};

std::vector<std::function<void()>>& MultiValueHashIndex_Test::getFunctionList() {
	return functionList_;
}

void MultiValueHashIndex_Test::test_get_singlevalue_key() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		MultiValueHashIndex<std::string, int> index;
		int val = 123;
		index.append("key1", val);
		std::vector<int> vals = index.get("key1");
		assert(1 == vals.size());
		assert(val == vals.at(0));
	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
}

void MultiValueHashIndex_Test::test_get_multivalued_key() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		MultiValueHashIndex<std::string, int> index;
		int val1 = 123;
		int val2 = 456;
		index.append("key1", val1);
		index.append("key1", val2);

		std::vector<int> vals = index.get("key1");
		assert(2 == vals.size());
		assert(val1 == vals.at(0));
		assert(val2 == vals.at(1));
	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
}

void MultiValueHashIndex_Test::test_get_nonexisting_key() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		MultiValueHashIndex<std::string, int> index;
		int val = 123;
		index.append("key1", val);

		index.get("key2");
		assert(1 == 2);	// we shouldn't reach this point
	}
	catch (const std::exception& e){
		assert(1 == 1);
	}
}

void MultiValueHashIndex_Test::test_get_deleted_key() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		MultiValueHashIndex<std::string, int> index;
		int val = 123;
		index.append("key1", val);
		index.erase("key1");

		index.get("key1");
		assert(1 == 2);	// we shouldn't reach this point
	}
	catch (const std::exception& e){
		assert(1 == 1);
	}
}

void MultiValueHashIndex_Test::test_append_nonexisting_key() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		MultiValueHashIndex<std::string, int> index;
		int val = 123;
		index.append("key1", val);
		std::vector<int> vals = index.get("key1");
		assert(vals.size() == 1);
		assert(vals.at(0) == val);
	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
}

void MultiValueHashIndex_Test::test_append_deleted_key() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		MultiValueHashIndex<std::string, int> index;
		index.append("key1", 123);
		index.erase("key1");
		index.append("key1", 456);

		std::vector<int> vals = index.get("key1");
		assert(vals.size() == 1);
		assert(vals.at(0) == 456);
	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
}

void MultiValueHashIndex_Test::test_append_million_values() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		MultiValueHashIndex<std::string, int> index;
		for (int i = 1; i <= 1000*1000; i++)
			index.append("key1", i);

		std::vector<int> vals = index.get("key1");
		assert(vals.size() == 1000*1000);
	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
}


