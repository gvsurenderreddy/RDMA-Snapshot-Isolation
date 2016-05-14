/*
 * NewOrderTransaction.hpp
 *
 *  Created on: Mar 14, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_NEWORDERTRANSACTION_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_NEWORDERTRANSACTION_HPP_

#include "../BaseTransaction.hpp"
#include "NewOrderLocalMemory.hpp"
#include "../../TPCCClient.hpp"
#include "../../../../random/randomgenerator.hpp"
#include "../../../../../../rdma-region/RDMAContext.hpp"
#include "../../../../../../recovery/RecoveryClient.hpp"
#include "../../../../../../util/ZipfDistribution.hpp"
#include <sstream>


namespace TPCC {
struct NewOrderItem {
	uint32_t I_ID;
	uint16_t OL_SUPPLY_W_ID;
	uint8_t OL_QUANTITY;
};

struct NewOrderCart{
	uint16_t wID;
	uint8_t dID;
	uint32_t cID;
	std::vector<NewOrderItem> items;

	void reset(){
		wID = 0;
		dID = 0;
		cID = 0;
		items.clear();
	}

	friend std::ostream& operator<<(std::ostream& os, const NewOrderCart& c) {
		os << "wID:" << (int)c.wID << " | dID:" << (int)c.dID << " | cID:" << (int)c.cID << " -- Items (iID,suppWID):";
		for(auto const& value: c.items)
			os << " {i:" << value.I_ID << " | W:" << value.OL_SUPPLY_W_ID << "},";
		return os;
	}

	size_t logMessage(char *destintionBuffer){
		std::stringstream ss;
		ss << "NewOrderTrx:" << "w" << (int)wID << "|d" << (int)dID << "|c" << (int)cID << "|n" << items.size() << "|";
		for(auto const& value: items)
			ss << value.I_ID << "," << value.OL_SUPPLY_W_ID << "|";

		std::string str = ss.str();
		str.copy(destintionBuffer, str.size());
		return str.size();
	}
};

// ************************************************
//	Transaction Info:
//		- Read Set: {1 warehouse, 1 district, 1 customer, 5-15 items, 5-15 stocks}
//		- Write Set:
//			- Modify set: {1 district, 5-15 stocks}
//			- New: {1 order, 1 neworder, 5-15 orderlines}
//
//		Network Messages Size: (V = # of versions)
//			- read: (5322 + 104*V + 4 * #clients)B = (4 * #clients + 108 + 8*V + 120 + 8*V + 690 + 8*V + 10*(100 + 330 + 8*V) + 8 + 10*8 + 8 + 8)
//			- write: (7712 + 80*V)B = (16 + 10*(16) + 10*(330 + 8*V) + 16 + 40 + 12 + 10*(330 + 80) + 64 + 4)
//			- total (for V=3 and 100 clients): 14000 B
// ************************************************
class NewOrderTransaction : public BaseTransaction {
private:
	NewOrderLocalMemory* localMemory_;
	NewOrderCart cart_;
	ZipfDistribution zipf;
	void buildCart();



public:
	NewOrderTransaction(TPCCClient &client, DBExecutor &executor);
	virtual ~NewOrderTransaction();

	void initilizeTransaction();
	TPCC::TransactionResult doOne();

	NewOrderTransaction& operator=(const NewOrderTransaction&) = delete;	// Disallow copying
	NewOrderTransaction(const NewOrderTransaction&) = delete;				// Disallow copying
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_NEWORDERTRANSACTION_HPP_ */
