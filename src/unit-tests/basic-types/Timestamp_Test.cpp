/*
 * Timestamp_Test.cpp
 *
 *  Created on: Apr 15, 2016
 *      Author: erfanz
 */

#include "Timestamp_Test.hpp"
#include "../../util/utils.hpp"
#include <assert.h>
#include <stdexcept>      // std::out_of_range

#define CLASS_NAME	"Timestamp_Test"

std::vector<std::function<void()>> Timestamp_Test::functionList_ {
	test_construct,
	test_set_all,
	test_locking,
	test_deleting,
	test_equality
};

std::vector<std::function<void()>>& Timestamp_Test::getFunctionList() {
	return functionList_;
}

void Timestamp_Test::test_construct() {
	TestBase::printMessage(CLASS_NAME, __func__);

	bool isDeleted = false;
	bool isLocked = false;
	primitive::version_offset_t versionOffset = 2;
	primitive::client_id_t clientID = 12;
	primitive::timestamp_t timestamp = 24;

	Timestamp ts(isDeleted, isLocked, versionOffset, clientID, timestamp);
	assert(ts.isDeleted() == isDeleted);
	assert(ts.isLocked() == isDeleted);
	assert(ts.getClientID() == clientID);
	assert(ts.getTimestamp() == timestamp);
	assert(ts.getVersionOffset() == versionOffset);
}

void Timestamp_Test::test_set_all() {
	TestBase::printMessage(CLASS_NAME, __func__);

	bool isDeleted = false;
	bool isLocked = false;
	primitive::version_offset_t versionOffset = 2;
	primitive::client_id_t clientID = 12;
	primitive::timestamp_t timestamp = 24;

	Timestamp ts;
	ts.setAll(isDeleted, isLocked, versionOffset, clientID, timestamp);
	assert(ts.isDeleted() == isDeleted);
	assert(ts.isLocked() == isDeleted);
	assert(ts.getClientID() == clientID);
	assert(ts.getTimestamp() == timestamp);
	assert(ts.getVersionOffset() == versionOffset);
}

void Timestamp_Test::test_locking() {
	TestBase::printMessage(CLASS_NAME, __func__);

	bool isDeleted = false;
	bool isLocked = false;
	primitive::version_offset_t versionOffset = 2;
	primitive::client_id_t clientID = 12;
	primitive::timestamp_t timestamp = 24;

	Timestamp ts(isDeleted, isLocked, versionOffset, clientID, timestamp);
	Timestamp ts_copy(ts);
	assert(ts.isLocked() == false);
	ts.markLocked();
	assert(ts.isLocked() == true);
	ts.markUnlocked();
	assert(ts.isLocked() == false);
	assert(ts.isEqual(ts_copy) == true);
}

void Timestamp_Test::test_deleting() {
	TestBase::printMessage(CLASS_NAME, __func__);

	bool isDeleted = false;
	bool isLocked = false;
	primitive::version_offset_t versionOffset = 2;
	primitive::client_id_t clientID = 12;
	primitive::timestamp_t timestamp = 24;

	Timestamp ts(isDeleted, isLocked, versionOffset, clientID, timestamp);
	Timestamp ts_copy(ts);
	assert(ts.isDeleted() == false);
	ts.markDeleted();
	assert(ts.isDeleted() == true);
}

void Timestamp_Test::test_equality(){
	TestBase::printMessage(CLASS_NAME, __func__);

	bool isDeleted = false;
	bool isLocked = false;
	primitive::version_offset_t versionOffset = 2;
	primitive::client_id_t clientID = 12;
	primitive::timestamp_t timestamp = 24;

	Timestamp ts(isDeleted, isLocked, versionOffset, clientID, timestamp);
	Timestamp ts_eq(isDeleted, isLocked, versionOffset, clientID, timestamp);
	Timestamp ts2(!isDeleted, isLocked, versionOffset, clientID, timestamp);
	Timestamp ts3(isDeleted, !isLocked, versionOffset, clientID, timestamp);
	Timestamp ts4(isDeleted, isLocked, versionOffset + 1, clientID, timestamp);
	Timestamp ts5(isDeleted, isLocked, versionOffset, clientID + 1, timestamp);
	Timestamp ts6(isDeleted, isLocked, versionOffset, clientID + 1, timestamp + 1);

	assert(ts.isEqual(ts_eq) == true);
	assert(ts.isEqual(ts2) == false);
	assert(ts.isEqual(ts3) == false);
	assert(ts.isEqual(ts4) == false);
	assert(ts.isEqual(ts5) == false);
	assert(ts.isEqual(ts6) == false);
}
