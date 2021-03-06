/*
 * DistrictTable.hpp
 *
 *  Created on: Feb 16, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_DISTRICTTABLE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_DISTRICTTABLE_HPP_

#include "../../../basic-types/timestamp.hpp"
#include "TPCCUtil.hpp"
#include "../../../rdma-region/RDMARegion.hpp"
#include "../../../../config.hpp"

using namespace tpcc_settings;

namespace TPCC{
class District{
public:
	//	Primary Key: (D_W_ID, D_ID)
	//	D_W_ID Foreign Key, references W_ID
	// 	Size:	112 Bytes

	uint8_t		D_ID;			// 20 unique IDs 10 are populated per warehouse
	uint16_t 	D_W_ID; 		// 2*W unique IDs
	uint64_t	D_NEXT_O_ID;	// 10,000,000 unique IDs Next available Order number (64bit for atomic operations)
	char 		D_NAME[11];		// variable text, size 10
	char 		D_STREET_1[21];	// variable text, size 20
	char 		D_STREET_2[21]; // variable text, size 20
	char		D_CITY[21]; 	// variable text, size 20
	char		D_STATE[3]; 	// fixed text, size 2
	char		D_ZIP[10];		// fixed text, size 9
	float 		D_YTD;			// signed numeric(12,2) Year to date balance
	float 		D_TAX;			// signed numeric(4,4) Sales tax

	void initialize(uint8_t dID, uint16_t wID, TPCC::RandomGenerator& random){
		D_ID = dID;
		D_W_ID = wID;
		D_TAX = makeTax(random);
		D_YTD = DISTRICT_INITIAL_YTD;
		//D_NEXT_O_ID = config::tpcc_settings::CUSTOMER_PER_DISTRICT + 1;
		D_NEXT_O_ID = 0;
		random.astring(D_NAME, DISTRICT_MIN_NAME, DISTRICT_MAX_NAME);
		random.astring(D_STREET_1, ADDRESS_MIN_STREET, ADDRESS_MAX_STREET);
		random.astring(D_STREET_2, ADDRESS_MIN_STREET, ADDRESS_MAX_STREET);
		random.astring(D_CITY, ADDRESS_MIN_CITY, ADDRESS_MAX_CITY);
		random.astring(D_STATE, ADDRESS_STATE, ADDRESS_STATE);
		makeZip(random, D_ZIP);
	}

	static size_t getOffsetOfTax(){
		return offsetof(District, D_TAX);
	}

	static size_t getOffsetOfNextOID(){
		return offsetof(District, D_NEXT_O_ID);
	}

	friend std::ostream& operator<<(std::ostream& os, const District& d) {
		os << "D_ID:" << (int)d.D_ID << "|D_W_ID:" << (int)d.D_W_ID;
		return os;
	}
};

class DistrictVersion{
public:
	Timestamp writeTimestamp;
	District district;

	static size_t getOffsetOfDistrict(){
		return offsetof(DistrictVersion, district);
	}

	static size_t getOffsetOfTimestamp(){
		return offsetof(DistrictVersion, writeTimestamp);
	}

	friend std::ostream& operator<<(std::ostream& os, const DistrictVersion& v) {
		os << v.district << "(" << v.writeTimestamp << ")";
		return os;
	}
};

class DistrictTable{
public:
	RDMARegion<DistrictVersion> *headVersions;
	RDMARegion<Timestamp> 		*tsList;
	RDMARegion<DistrictVersion>	*olderVersions;


	DistrictTable(	std::ostream &os, size_t size, size_t maxVersionsCnt, RDMAContext &baseContext, int mrFlags)
	: os_(os),
	  size_(size),
	  maxVersionsCnt_(maxVersionsCnt){
		headVersions 	= new RDMARegion<DistrictVersion>(size, baseContext, mrFlags);
		tsList 			= new RDMARegion<Timestamp>(size * maxVersionsCnt, baseContext, mrFlags);
		olderVersions	= new RDMARegion<DistrictVersion>(size * maxVersionsCnt, baseContext, mrFlags);

		DEBUG_WRITE(os_, "DistrictTable", __func__, "[Info] District table initialized");
	}

	static size_t getRecordPositionInTable(size_t warehouseOffset, uint16_t dID){
		return (size_t) (warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID);
	}


	void insert(size_t warehouseOffset, uint8_t dID, uint16_t wID, TPCC::RandomGenerator& random, Timestamp &ts){
		size_t tablePosition = getRecordPositionInTable(warehouseOffset, dID);
		headVersions->getRegion()[tablePosition].district.initialize(dID, wID, random);
		headVersions->getRegion()[tablePosition].writeTimestamp.copy(ts);
	}

	void getMemoryHandler(MemoryHandler<DistrictVersion> &headVersionsMH, MemoryHandler<Timestamp> &tsListMH, MemoryHandler<DistrictVersion> &olderVersionsMH){
		headVersions->getMemoryHandler(headVersionsMH);
		tsList->getMemoryHandler(tsListMH);
		olderVersions->getMemoryHandler(olderVersionsMH);
	}


	~DistrictTable(){
		DEBUG_WRITE(os_, "DistrictTable", __func__, "[Info] Deconstructor called");
		delete headVersions;
		delete tsList;
		delete olderVersions;
	}

private:
	std::ostream &os_;
	size_t size_;
	size_t maxVersionsCnt_;
};
}	// namespace TPCC



#endif /* SRC_BENCHMARKS_TPC_C_TABLES_DISTRICTTABLE_HPP_ */
