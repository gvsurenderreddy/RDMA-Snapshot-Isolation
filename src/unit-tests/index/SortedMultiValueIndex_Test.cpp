/*
 * SortedMultiValueIndex_Test.cpp
 *
 *  Created on: Apr 18, 2016
 *      Author: erfanz
 */

#include "SortedMultiValueIndex_Test.hpp"
#include "../../index/hash/SortedMultiValueHashIndex.hpp"
#include <assert.h>
#include <stdexcept>      // std::out_of_range
#include "../../util/utils.hpp"

#define CLASS_NAME	"SortedMVHashIndex_Test"


std::vector<std::function<void()>> SortedMultiValueIndex_Test::functionList_ {
	test_general,
	test_manual_class
};

std::vector<std::function<void()>>& SortedMultiValueIndex_Test::getFunctionList() {
	return functionList_;
}

void SortedMultiValueIndex_Test::test_general() {
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		SortedMultiValueHashIndex<std::string, int> index;
		int val1 = 3;
		int val2 = 1;
		int val3 = 2;

		index.push("key", val1);
		index.push("key", val2);
		index.push("key", val3);

		assert(index.top("key") == val2);
		index.pop("key");

		assert(index.top("key") == val3);
		index.pop("key");

		assert(index.top("key") == val1);
		index.pop("key");
	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
}

void SortedMultiValueIndex_Test::test_manual_class() {
	TestBase::printMessage(CLASS_NAME, __func__);

	struct S {
	    int a, b;
	    S(int a, int b) { this->a = a; this->b = b; }
	    bool operator>(const S &other) const{ return b > other.b; }
	    bool operator==(const S &other) const{ return (a == other.a && b == other.b); }
	};

	try{
		SortedMultiValueHashIndex<std::string, S> index;
		S s1(1,3);
		S s2(2,1);
		S s3(3,2);

		index.push("key", s1);
		index.push("key", s2);
		index.push("key", s3);

		assert(index.top("key") == s2);
		index.pop("key");

		assert(index.top("key") == s3);
		index.pop("key");

		assert(index.top("key") == s1);
		index.pop("key");
	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
}
