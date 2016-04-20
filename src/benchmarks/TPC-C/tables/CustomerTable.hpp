/*
 * CustomerTable.hpp
 *
 *  Created on: Feb 15, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_CUSTOMERTABLE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_CUSTOMERTABLE_HPP_

#include "../../../basic-types/timestamp.hpp"
#include "TPCCUtil.hpp"
#include "../random/randomgenerator.hpp"
#include "../../../../config.hpp"
#include "../../../rdma-region/RDMARegion.hpp"
#include "../../../index/hash/MultiValueHashIndex.hpp"
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

using namespace tpcc_settings;

namespace TPCC{
class Customer{
public:
	// 	Primary Key: (C_W_ID, C_D_ID, C_ID)
	//	(C_W_ID, C_D_ID) Foreign Key, references (D_W_ID, D_ID)
	// 	Size: 656 bytes

	uint32_t 	C_ID;			// 96,000 unique IDs 3,000 are populated per district
	uint8_t		C_D_ID;			// 20 unique IDs
	uint16_t	C_W_ID;			// 2*W unique IDs
	char		C_STREET_1[21];	// variable text, size 20
	char		C_STREET_2[21];	// variable text, size 20
	char		C_CITY[21];		// variable text, size 20
	char		C_STATE[3];		// fixed text, size 2
	char		C_ZIP[10];		// fixed text, size 9
	char		C_PHONE[17];	// fixed text, size 16
	time_t		C_SINCE;		// date and time
	char		C_FIRST[17];	// variable text, size 16
	char		C_MIDDLE[3];	// fixed text, size 2
	char		C_LAST[17];		// variable text, size 16
	float		C_DISCOUNT;		// signed numeric(4, 4)
	char		C_CREDIT[3];	// fixed text, size 2 "GC"=good, "BC"=bad
	float		C_CREDIT_LIM;	// signed numeric(12, 2)
	float		C_BALANCE;		// signed numeric(12, 2)
	float		C_YTD_PAYMENT;	// signed numeric(12, 2)
	uint16_t	C_PAYMENT_CNT;	// numeric(4)
	uint16_t	C_DELIVERY_CNT; // numeric(4)
	char		C_DATA[501];	// variable text, size 500 Miscellaneous information

	void initialize(uint32_t cID, uint8_t dID, uint16_t wID, bool bad_credit, TPCC::RandomGenerator& random, time_t now) {
		C_ID = cID;
		C_D_ID = dID;
		C_W_ID = wID;
		C_CREDIT_LIM = CUSTOMER_INITIAL_CREDIT_LIM;
		C_DISCOUNT = random.fixedPoint(4, CUSTOMER_MIN_DISCOUNT, CUSTOMER_MAX_DISCOUNT);
		C_BALANCE = CUSTOMER_INITIAL_BALANCE;
		C_YTD_PAYMENT = CUSTOMER_INITIAL_YTD_PAYMENT;
		C_PAYMENT_CNT = CUSTOMER_INITIAL_PAYMENT_CNT;
		C_DELIVERY_CNT = CUSTOMER_INITIAL_DELIVERY_CNT;
		random.astring(C_FIRST, CUSTOMER_MIN_FIRST, CUSTOMER_MAX_FIRST);
		strcpy(C_MIDDLE, "OE");

		if (cID < 1000) {
			TPCC::makeLastName(cID, C_LAST);
		} else {
			random.lastName(C_LAST, config::tpcc_settings::CUSTOMER_PER_DISTRICT);
		}

		random.astring(C_STREET_1, ADDRESS_MIN_STREET, ADDRESS_MAX_STREET);
		random.astring(C_STREET_2, ADDRESS_MIN_STREET, ADDRESS_MAX_STREET);
		random.astring(C_CITY, ADDRESS_MIN_CITY, ADDRESS_MAX_CITY);
		random.astring(C_STATE, ADDRESS_STATE, ADDRESS_STATE);
		makeZip(random, C_ZIP);
		random.nstring(C_PHONE, CUSTOMER_PHONE, CUSTOMER_PHONE);
		C_SINCE = now;
		if (bad_credit) {
			strcpy(C_CREDIT, "BC");
		} else {
			strcpy(C_CREDIT, "GC");
		}
		random.astring(C_DATA, CUSTOMER_MIN_DATA, CUSTOMER_MAX_DATA);
	}

	friend std::ostream& operator<<(std::ostream& os, const Customer& c) {
		os << "C_ID:" << (int)c.C_ID << "|C_D_ID:" << (int)c.C_D_ID << "|C_W_ID:" << (int)c.C_W_ID;
		return os;
	}
};


class CustomerVersion{
public:
	Timestamp writeTimestamp;
	Customer customer;

	static size_t getOffsetOfCustomer(){
		return offsetof(CustomerVersion, customer);
	}

	static size_t getOffsetOfTimestamp(){
		return offsetof(CustomerVersion, writeTimestamp);
	}

	friend std::ostream& operator<<(std::ostream& os, const CustomerVersion& v) {
		os << v.customer << "(" << v.writeTimestamp << ")";
		return os;
	}
};


class CustomerTable{
private:
	std::ostream &os_;
	MultiValueHashIndex<std::string, uint32_t> customerLastNameToID_Index_;

public:
	RDMARegion<CustomerVersion> *headVersions;
	RDMARegion<Timestamp> 	*tsList;
	RDMARegion<CustomerVersion>	*olderVersions;


	CustomerTable(std::ostream &os, size_t size, size_t maxVersionsCnt, RDMAContext &baseContext, int mrFlags)
	: os_(os),
	  size_(size),
	  maxVersionsCnt_(maxVersionsCnt){
		headVersions 	= new RDMARegion<CustomerVersion>(size, baseContext, mrFlags);
		tsList 			= new RDMARegion<Timestamp>(size * maxVersionsCnt, baseContext, mrFlags);
		olderVersions	= new RDMARegion<CustomerVersion>(size * maxVersionsCnt, baseContext, mrFlags);

		bool isLocked = false;
		bool isDeleted = true;
		primitive::client_id_t clientID = 0;
		primitive::timestamp_t timestamp = 0;
		primitive::version_offset_t versionOffset = 0;
		for (unsigned int  i = 0; i < size_; ++i) {
			for (size_t j = 0; j < maxVersionsCnt_; j++){
				tsList->getRegion()[i * maxVersionsCnt_ + j].setAll(isDeleted, isLocked, versionOffset, clientID, timestamp);
			}
		}
	}

	~CustomerTable(){
		DEBUG_WRITE(os_, "CustomerTable", __func__, "[Info] Deconstructor called");
		delete headVersions;
		delete tsList;
		delete olderVersions;
	}

	void insert(size_t warehouseOffset, uint32_t cID, uint8_t dID, uint16_t wID, bool bad_credit, TPCC::RandomGenerator& random, time_t now, Timestamp &ts){
		size_t index = (warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID ) * config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID;
		headVersions->getRegion()[index].customer.initialize(cID, dID, wID, bad_credit, random, now);
		headVersions->getRegion()[index].writeTimestamp.copy(ts);
	}

	void getMemoryHandler(MemoryHandler<CustomerVersion> &headVersionsMH, MemoryHandler<Timestamp> &tsListMH, MemoryHandler<CustomerVersion> &olderVersionsMH){
		headVersions->getMemoryHandler(headVersionsMH);
		tsList->getMemoryHandler(tsListMH);
		olderVersions->getMemoryHandler(olderVersionsMH);
	}

	void buildIndexOnLastName() {
		for (size_t i = 0; i < headVersions->getRegionSize(); i++){
			uint16_t wID = headVersions->getRegion()[i].customer.C_W_ID;
			uint8_t dID = headVersions->getRegion()[i].customer.C_D_ID;
			uint32_t cID = headVersions->getRegion()[i].customer.C_ID;
			std::string lastName = std::string(headVersions->getRegion()[i].customer.C_LAST);
			std::string key = "w_" + std::to_string(wID) + "_d_" + std::to_string(dID) + "_c_" + lastName;
			customerLastNameToID_Index_.append(key, cID);
		}
	}

	Customer& getCustomer(size_t warehouseOffset, uint8_t dID, uint32_t cID){
		size_t index = (warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID ) * config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID;
		return headVersions->getRegion()[index].customer;
	}

	uint32_t getMiddleIDByLastName(uint16_t wID, uint8_t dID, std::string lastName) {
		std::string key = "w_" + std::to_string(wID) + "_d_" + std::to_string(dID) + "_c_" + lastName;
		std::vector<uint32_t> ids = customerLastNameToID_Index_.get(key);
		return ids.at(ids.size() / 2);
	}

private:
	size_t size_;
	size_t maxVersionsCnt_;
};

}	// namespace TPCC

#endif /* SRC_BENCHMARKS_TPC_C_TABLES_CUSTOMERTABLE_HPP_ */
