/*
 * hashIndexTest.cpp
 *
 *  Created on: Mar 30, 2016
 *      Author: erfanz
 */

#include "HashIndex_Test.hpp"
#include <assert.h>
#include <stdexcept>      // std::out_of_range
#include "../../util/utils.hpp"

#define CLASS_NAME	"HashIndex_Test"


std::vector<std::function<void()>> HashIndex_Test::functionList_ {
	test_get_existing_key,
	test_get_nonexisting_key,
	test_get_deleted_key,
	test_put_nonexisting_key,
	test_put_existing_key,
	test_put_deleted_key
};

std::vector<std::function<void()>>& HashIndex_Test::getFunctionList() {
	return functionList_;
}

void HashIndex_Test::test_get_existing_key() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		HashIndex<std::string, int> index;
		int val = 123;
		index.put("key1", val);
		assert(val == index.get("key1"));
	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
}

void HashIndex_Test::test_get_nonexisting_key() {
	TestBase::printMessage(CLASS_NAME, __func__);
	try{
		HashIndex<std::string, int> index;
		index.get("must_return_error");
		assert(1 == 2);		// we shouldn't reach this point
	}
	catch (const std::out_of_range& oor) {
		assert(1 == 1);
	}
}

void HashIndex_Test::test_get_deleted_key() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		HashIndex<std::string, int> index;
		int val = 123;
		index.put("key1", val);
		index.erase("key1");

		index.get("must_return_error");
		assert(1 == 2); // we shouldn't reach this point
	}
	catch (const std::out_of_range& oor) {
		assert(1 == 1);
	}
}

void HashIndex_Test::test_put_nonexisting_key() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		HashIndex<std::string, int> index;
		index.put("key1", 1);
		index.put("key2", 2);

		assert(index.get("key1") == 1);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
}

void HashIndex_Test::test_put_existing_key() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		HashIndex<std::string, int> index;
		index.put("key1", 1);
		assert(index.get("key1") == 1);

		index.put("key1", 11);
		assert(index.get("key1") == 11);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
}

void HashIndex_Test::test_put_deleted_key() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		HashIndex<std::string, int> index;
		index.put("key1", 1);
		index.erase("key1");

		index.put("key1", 11);
		assert(index.get("key1") == 11);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
}
