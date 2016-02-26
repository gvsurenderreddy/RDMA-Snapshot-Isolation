/*
 * TPCCClient.cpp
 *
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#include "TPCCClient.hpp"
#include "../../../util/RDMACommon.hpp"
#include "../../../util/utils.hpp"
#include "../tables/TPCCUtil.hpp"
#include "../tables/WarehouseTable.hpp"
#include <infiniband/verbs.h>
#include <string>
#include <vector>



#define CLASS_NAME "TPCCClient"

TPCCClient::TPCCClient(unsigned instanceNum, uint8_t ibPort)
: instanceNum_(instanceNum),
  ibPort_(ibPort){
	srand ((unsigned int)utils::generate_random_seed());		// initialize random seed
	//zipf_initialize(config::SKEWNESS_IN_ITEM_ACCESS, config::ITEM_PER_SERVER);
	// nextEpoch_ = (primitive::timestamp_t) 1ULL;

	TPCC::NURandC cLoad = TPCC::NURandC::makeRandom(random_);
	random_.setC(cLoad);

	context_ = new RDMAContext(ibPort_);

	sessionState.nextCommitOrderID = clientID_;
	sessionState.nextEpoch_ = (primitive::timestamp_t) 1ULL;


	for (int i = 0; i < config::SERVER_CNT; i++){
		// Connect to servers
		int sockfd;
		TEST_NZ (utils::establish_tcp_connection(config::SERVER_ADDR[i], config::TCP_PORT[i], &sockfd));

		// build server context
		dsCtx_.push_back(new ServerContext(sockfd, config::SERVER_ADDR[i], config::TCP_PORT[i], config::IB_PORT[i], instanceNum, *context_));

		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to server " << i);


		// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
		RDMACommon::post_RECEIVE(
				dsCtx_[i]->getQP(),
				dsCtx_[i]->getRemoteMemoryKeys()->getRDMAHandler(),
				(uintptr_t)dsCtx_[i]->getRemoteMemoryKeys()->getRegion(),
				(uint32_t)dsCtx_[i]->getRemoteMemoryKeys()->getRegionSizeInByte());

		dsCtx_[i]->activateQueuePair(*context_);
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] QPed to server " << i);

		TEST_NZ(RDMACommon::poll_completion(context_->getRecvCq()));
		DEBUG_COUT(CLASS_NAME, __func__, "[Recv] buffers info from server " << i);
	}

	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Starting transactions ");

	for (int i=0; i < config::tpcc_settings::NEWORDER_TRANSACTION_CNT; i++){
		DEBUG_COUT(CLASS_NAME, __func__, "[Info] Transaction " << i << ":");
		doNewOrder();
	}


	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Client is done, and is ready to destroy its resources!");
	for (int i = 0; i < config::SERVER_CNT; i++){
		TEST_NZ (utils::sock_sync (dsCtx_[i]->getSockFd()));	// just send a dummy char back and forth
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Notified server " << i << " it's done");
		delete dsCtx_[i];
	}
}

Cart TPCCClient::buildCart(){
	int w_id = 0;	// TODO
	Cart cart;

	// 2.4.1.1 the home warehouse number (W_ID) is constant over the whole measurement interval
	cart.wID = (uint16_t) random_.number(0, config::tpcc_settings::WAREHOUSE_CNT);

	// 2.4.1.2 The district number (D_ID) is randomly selected within [1 .. 10] from the home warehouse (D_W_ID = W_ID).
	cart.dID = (uint8_t) random_.number(0, config::tpcc_settings::DISTRICT_PER_WAREHOUSE - 1);

	// 2.4.1.2 The non-uniform random customer number (C_ID) is selected using the NURand (1023,1,3000) function
	cart.cID = (uint32_t) random_.NURand(1023, 0, config::tpcc_settings::CUSTOMER_PER_DISTRICT - 1);

	// 2.4.1.3 The number of items in the order (ol_cnt) is randomly selected within [5 .. 15] (an average of 10)
	uint8_t ol_cnt = (uint8_t ) random_.number(tpcc_settings::ORDER_MIN_OL_CNT, tpcc_settings::ORDER_MAX_OL_CNT);


	// 2.4.1.4 A fixed 1% of the New-Order transactions are chosen at random to simulate user data entry errors and exercise the performance of rolling back update transactions.
	// bool rollback = random_.number(1, 100) == 1;

	for (int i = 0; i < ol_cnt; ++i) {
		NewOrderItem noi;
		// A non-uniform random item number (OL_I_ID) is selected using the NURand (8191,1,100000) function
		noi.I_ID = (uint32_t) random_.NURand(8191, 0, config::tpcc_settings::ITEMS_CNT - 1); ;

		// TPC-C suggests generating a number in range (1, 100) and selecting remote on 1 (clause 2.4.1.5.2)
		// This provides more variation, and lets us tune the fraction of "remote" transactions.
		bool remote = random_.number(0, 1) <= config::tpcc_settings::REMOTE_WAREHOUSE_PROB;
		if (config::tpcc_settings::WAREHOUSE_CNT > 1 && remote) {
			noi.OL_SUPPLY_W_ID = random_.numberExcluding(0, config::tpcc_settings::WAREHOUSE_CNT - 1, w_id);
		} else {
			noi.OL_SUPPLY_W_ID = w_id;
		}

		// 2.4.1.5.3 A quantity (OL_QUANTITY) is randomly selected within [1 .. 10]
		noi.OL_QUANTITY = (uint8_t) random_.number(1, 10);
		cart.items.push_back(noi);
	}
	return cart;
}

void TPCCClient::doNewOrder(){
	Cart cart = buildCart();

	time_t timer;
	std::time(&timer);

	// From Warehouse table, retrieve W_TAX
	float wTax = retrieveWarehouseTax(cart.wID);
	std::cout << "wTax: " << wTax << std::endl;



	// From District table, retrieve D_TAX
	float dTax = retrieveDistrictTax(cart.wID, cart.dID);
	std::cout << "dTax: " << dTax << std::endl;

	// In District table, increment the next available order number for the district D_NEXT_O_ID
	uint32_t oID = retrieveAndIncrementDistrictNextOID(cart.wID, cart.dID);
	std::cout << "oID: " << oID << std::endl;

	// From Customer table, retrieve C_DISCOUNT (the customer's discount rate), C_LAST (the customer's last name), and C_CREDIT (the customer's credit status)
	char		cLast[16];
	float		cDiscount;
	char		cCredit[2];
	getCustomerInformation(cart.wID, cart.dID, cart.cID, cLast, cDiscount, cCredit);
	std::cout << "cLast: " << cLast << std::endl;
	std::cout << "cDiscount: " << cDiscount << std::endl;
	std::cout << "cCredit: " << cCredit << std::endl;


	// insert a new row into New-Order and Order table.
	// O_CARRIER_ID is set to a null value. If the order includes only home order-lines, then O_ALL_LOCAL is set to 1, otherwise O_ALL_LOCAL is set to 0.
	// The number of items, O_OL_CNT, is computed to match ol_cnt
	primitive::lock_status_t 		lockStatus = 1;
	primitive::version_offset_t		versionOffset = 0;
	primitive::client_id_t			clientID = clientID_;
	const primitive::timestamp_t	timestamp = sessionState.nextEpoch_;
	Timestamp ts (lockStatus, versionOffset, clientID, timestamp);
	TPCC::OrderVersion* orderV = insertIntoOrder(oID, cart, timer, ts);
	TPCC::NewOrderVersion* newOrderV = insertIntoNewOrder(oID, cart.wID, cart.dID, ts);
	(void) orderV;
	(void) newOrderV;



	// For each O_OL_CNT item on the order:
	// |-- In Item table, I_PRICE, I_NAME and I_DATA are retrieved
	// |
	// |-- In Stock table, S_QUANTITY, the quantity in stock, S_DIST_xx, where xx represents the district number, and S_DATA are retrieved
	// |   If the retrieved value for S_QUANTITY exceeds OL_QUANTITY by 10 or more, then S_QUANTITY is decreased by OL_QUANTITY; otherwise
	// |   S_QUANTITY is updated to (S_QUANTITY - OL_QUANTITY)+91. S_YTD is increased by OL_QUANTITY and S_ORDER_CNT is incremented by 1.
	// |   If the order-line is remote, then S_REMOTE_CNT is incremented by 1.
	// |
	// |-- The amount for the item in the order (OL_AMOUNT) is computed as: OL_QUANTITY * I_PRICE
	// |
	// |-- The strings in I_DATA and S_DATA are examined. If they both include the string "ORIGINAL",
	// |   the brand-generic field for that item is set to "B", otherwise, the brand-generic field is set to "G".
	// |
	// |-- A new row is inserted into the ORDER-LINE table to reflect the item on the order. OL_DELIVERY_D is set to a null value, OL_NUMBER is set to a unique value within
	// |   all the ORDER-LINE rows that have the same OL_O_ID value, and OL_DIST_INFO is set to the content of S_DIST_xx, where xx represents the district number (OL_D_ID)
	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		TPCC::Item *item = retrieveItem(cart.items.at(olNumber).I_ID, cart.wID);
		TPCC::StockVersion *stockV = retrieveStock(cart.items.at(olNumber).I_ID, cart.wID);

		bool remote = cart.items.at(olNumber).OL_SUPPLY_W_ID == 0;	// TODO

		stockV->writeTimestamp.copy(ts);
		if (stockV->stock.S_QUANTITY - cart.items.at(olNumber).OL_QUANTITY >= 10)
			stockV->stock.S_QUANTITY = (uint16_t)(stockV->stock.S_QUANTITY - cart.items.at(olNumber).OL_QUANTITY);
		else
			stockV->stock.S_QUANTITY = (uint16_t)(stockV->stock.S_QUANTITY - cart.items.at(olNumber).OL_QUANTITY + 91);
		stockV->stock.S_YTD = (uint32_t)(stockV->stock.S_YTD + cart.items.at(olNumber).OL_QUANTITY);
		stockV->stock.S_ORDER_CNT = (uint16_t)(stockV->stock.S_ORDER_CNT + 1);
		if (remote)
			stockV->stock.S_REMOTE_CNT++;

		updateStock(stockV);
		insertIntoOrderLine(olNumber, oID, cart, cart.items.at(olNumber), item, stockV, ts);
	}
}

ServerContext* TPCCClient::getServerContext(uint16_t wID){
	(void) wID;
	return dsCtx_[0];
}


float TPCCClient::retrieveWarehouseTax(uint16_t wID){
	ServerContext* serverContext = getServerContext(wID);

	size_t tableOffset = (size_t)(wID * sizeof(TPCC::WarehouseVersion));				// offset of WarehouseVersion in WarehouseTable
	size_t warehouseOffset = (size_t)TPCC::WarehouseVersion::getOffsetOfWarehouse();	// offset of Warehouse in WarehouseVersion
	size_t fieldOffset = TPCC::Warehouse::getOffsetOfTax();								// offset of W_TAX in Warehouse

	// The remote address to read the warehouse tax
	float *lookupAddress =  (float*)(
			tableOffset
			+ warehouseOffset
			+ fieldOffset
			+ ((uint64_t)serverContext->getRemoteMemoryKeys()->getRegion()->warehouseTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(float);	// Warehouse::W_TAX is of type float

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			serverContext->getQP(),
			serverContext->getLocalWarehouseHead()->getRDMAHandler(),
			(uintptr_t)&(serverContext->getLocalWarehouseHead()->getRegion()->warehouse.W_TAX),
			&serverContext->getRemoteMemoryKeys()->getRegion()->warehouseTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	return serverContext->getLocalWarehouseHead()->getRegion()->warehouse.W_TAX;
}

float TPCCClient::retrieveDistrictTax(uint16_t wID, uint8_t dID){
	ServerContext* serverContext = getServerContext(wID);

	size_t tableOffset = (size_t)((wID * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	size_t districtOffset = (size_t)TPCC::DistrictVersion::getOffsetOfDistrict();		// offset of District in DistrictVersion
	size_t fieldOffset = TPCC::District::getOffsetOfTax();		// offset of D_TAX in District

	// The remote address to read the district tax
	float *lookupAddress =  (float *)(
			tableOffset
			+ districtOffset
			+ fieldOffset
			+ ((uint64_t)serverContext->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(float);	// District::D_TAX is of type float

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			serverContext->getQP(),
			serverContext->getLocalDistrictHead()->getRDMAHandler(),
			(uintptr_t)&(serverContext->getLocalDistrictHead()->getRegion()->district.D_TAX),
			&serverContext->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	return serverContext->getLocalDistrictHead()->getRegion()->district.D_TAX;
}

uint32_t TPCCClient::retrieveAndIncrementDistrictNextOID(uint16_t wID, uint8_t dID){
	ServerContext* serverContext = getServerContext(wID);

	size_t tableOffset = (size_t)((wID * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	size_t districtOffset = (size_t)TPCC::DistrictVersion::getOffsetOfDistrict();		// offset of District in DistrictVersion
	size_t fieldOffset = TPCC::District::getOffsetOfNextOID();		// offset of D_NEXT_O_ID in District

	// The remote address to read the district tax
	uint64_t *lookupAddress =  (uint64_t *)(
			tableOffset
			+ districtOffset
			+ fieldOffset
			+ ((uint64_t)serverContext->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	size_t size = sizeof(serverContext->getLocalDistrictHead()->getRegion()->district.D_NEXT_O_ID);

	TEST_NZ (RDMACommon::post_RDMA_FETCH_ADD(
			serverContext->getQP(),
			serverContext->getLocalDistrictHead()->getRDMAHandler(),
			(uintptr_t)&(serverContext->getLocalDistrictHead()->getRegion()->district.D_NEXT_O_ID),
			&serverContext->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			(uint64_t)1ULL,
			(uint32_t)size));
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	// Byte swapping, since values read by atomic operations are in Big Endian order
	return (uint32_t) utils::bigEndianToHost(serverContext->getLocalDistrictHead()->getRegion()->district.D_NEXT_O_ID);
}


void TPCCClient::getCustomerInformation(uint16_t wID, uint8_t dID, uint32_t cID, char* out_cLast, float &out_cDiscount, char *out_cCredit){
	ServerContext* serverContext = getServerContext(wID);

	size_t tableOffset = (size_t)(((wID * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID)
			* sizeof(TPCC::CustomerVersion));										// offset of CustomerVersion in CustomerTable
	size_t customerOffset = (size_t)TPCC::CustomerVersion::getOffsetOfCustomer();	// offset of Customer in CustomerVersion

	// The remote address to read the customer info
	TPCC::Customer *lookupAddress =  (TPCC::Customer *)(
			tableOffset
			+ customerOffset
			+ ((uint64_t)serverContext->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::Customer);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			serverContext->getQP(),
			serverContext->getLocalCustomerHead()->getRDMAHandler(),
			(uintptr_t)&(serverContext->getLocalCustomerHead()->getRegion()->customer),
			&serverContext->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	// results are ready. now filling in the output arguments
	TPCC::Customer &customer = serverContext->getLocalCustomerHead()->getRegion()->customer;
	std::memcpy(out_cLast, customer.C_LAST, sizeof(customer.C_LAST));
	out_cDiscount = customer.C_DISCOUNT;
	std::memcpy(out_cCredit, customer.C_CREDIT, sizeof(customer.C_CREDIT));
}

TPCC::OrderVersion* TPCCClient::insertIntoOrder(uint32_t oID, Cart &cart, time_t timer, Timestamp writeTimestamp){
	ServerContext* serverContext = getServerContext(cart.wID);

	TPCC::OrderVersion *ov = serverContext->getLocalOrderHead()->getRegion();

	ov->writeTimestamp.copy(writeTimestamp);

	ov->order.O_ID = oID;
	ov->order.O_W_ID = cart.wID;
	ov->order.O_D_ID = cart.dID;
	ov->order.O_C_ID = cart.cID;
	ov->order.O_ENTRY_D = timer;
	ov->order.O_CARRIER_ID = 0;
	ov->order.O_OL_CNT = (uint8_t)cart.items.size();
	ov->order.O_ALL_LOCAL = 1;	// TODO: must be fixed

	size_t tableOffset = (size_t)(((cart.wID * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + cart.dID)
			* config::tpcc_settings::ORDER_PER_DISTRICT + oID) * sizeof(TPCC::OrderVersion));	// offset of OrderVersion in OrderTable

	// The remote address to which the order will be written
	TPCC::OrderVersion *writeAddress =  (TPCC::OrderVersion *)(tableOffset + ((uint64_t)serverContext->getRemoteMemoryKeys()->getRegion()->orderTableHeadVersions.rdmaHandler_.addr));

	// Size to be written tothe remote side
	uint32_t size = (uint32_t) sizeof(TPCC::OrderVersion);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			serverContext->getLocalOrderHead()->getRDMAHandler(),
			(uintptr_t)ov,
			&serverContext->getRemoteMemoryKeys()->getRegion()->orderTableHeadVersions.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	return ov;
}

TPCC::NewOrderVersion* TPCCClient::insertIntoNewOrder(uint32_t oID, uint16_t wID, uint8_t dID, Timestamp writeTimestamp){
	ServerContext* serverContext = getServerContext(wID);
	TPCC::NewOrderVersion *nov = serverContext->getLocalNewOrderHead()->getRegion();

	nov->writeTimestamp.copy(writeTimestamp);

	nov->newOrder.NO_O_ID = oID;
	nov->newOrder.NO_W_ID = wID;
	nov->newOrder.NO_D_ID = dID;

	size_t tableOffset = (size_t)(((wID * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::NEWORDER_PER_DISTRICT + oID) * sizeof(TPCC::NewOrderVersion));	// offset of NewOrderVersion in NewOrderTable

	// The remote address to which the order will be written
	TPCC::NewOrderVersion *writeAddress =  (TPCC::NewOrderVersion *)(tableOffset + ((uint64_t)serverContext->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions.rdmaHandler_.addr));

	// Size to be written tothe remote side
	uint32_t size = (uint32_t) sizeof(TPCC::NewOrderVersion);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			serverContext->getLocalNewOrderHead()->getRDMAHandler(),
			(uintptr_t)nov,
			&serverContext->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	return nov;
}

TPCC::Item* TPCCClient::retrieveItem(uint32_t iID, uint16_t wID){
	ServerContext* serverContext = getServerContext(wID);

	size_t tableOffset = (size_t)(iID * sizeof(TPCC::ItemVersion));		// offset of ItemVersion in ItemTable
	size_t itemOffset = (size_t)TPCC::ItemVersion::getOffsetOfItem();	// offset of Item in ItemVersion

	// The remote address to read the item info
	TPCC::Item *lookupAddress =  (TPCC::Item *)(tableOffset + itemOffset + ((uint64_t)serverContext->getRemoteMemoryKeys()->getRegion()->itemTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::Item);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			serverContext->getQP(),
			serverContext->getLocalItemHead()->getRDMAHandler(),
			(uintptr_t)&(serverContext->getLocalItemHead()->getRegion()->item),
			&serverContext->getRemoteMemoryKeys()->getRegion()->itemTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	// results are ready. now filling in the output arguments
	return &serverContext->getLocalItemHead()->getRegion()->item;
}

TPCC::StockVersion* TPCCClient::retrieveStock(uint32_t iID, uint16_t wID){
	ServerContext* serverContext = getServerContext(wID);

	size_t tableOffset = (size_t)((wID * config::tpcc_settings::ITEMS_CNT + iID) * sizeof(TPCC::StockVersion));		// offset of StockVersion in StockTable

	// The remote address to read the item info
	TPCC::StockVersion *lookupAddress =  (TPCC::StockVersion *)(tableOffset + ((uint64_t)serverContext->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::StockVersion);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			serverContext->getQP(),
			serverContext->getLocalStockHead()->getRDMAHandler(),
			(uintptr_t)serverContext->getLocalStockHead()->getRegion(),
			&serverContext->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	// results are ready. now filling in the output arguments
	return serverContext->getLocalStockHead()->getRegion();
}

/***
 * If the retrieved value for S_QUANTITY exceeds OL_QUANTITY by 10 or more, then S_QUANTITY is decreased by OL_QUANTITY; otherwise
 * S_QUANTITY is updated to (S_QUANTITY - OL_QUANTITY)+91. S_YTD is increased by OL_QUANTITY and S_ORDER_CNT is incremented by 1.
 * If the order-line is remote, then S_REMOTE_CNT is incremented by 1.
 */
error::ErrorType TPCCClient::updateStock(TPCC::StockVersion *stockV){
	ServerContext* serverContext = getServerContext(stockV->stock.S_W_ID);

	size_t tableOffset = (size_t)((stockV->stock.S_W_ID * config::tpcc_settings::ITEMS_CNT + stockV->stock.S_I_ID) * sizeof(TPCC::StockVersion));		// offset of StockVersion in StockTable

	// The remote address to read the item info
	TPCC::StockVersion *writeAddress =  (TPCC::StockVersion *)(tableOffset + ((uint64_t)serverContext->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::StockVersion);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			serverContext->getLocalStockHead()->getRDMAHandler(),
			(uintptr_t)serverContext->getLocalStockHead()->getRegion(),
			&serverContext->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	return error::ErrorType::SUCCESS;
}

// |-- The amount for the item in the order (OL_AMOUNT) is computed as: OL_QUANTITY * I_PRICE
// |
// |-- The strings in I_DATA and S_DATA are examined. If they both include the string "ORIGINAL",
// |   the brand-generic field for that item is set to "B", otherwise, the brand-generic field is set to "G".
// |
// |-- A new row is inserted into the ORDER-LINE table to reflect the item on the order. OL_DELIVERY_D is set to a null value, OL_NUMBER is set to a unique value within
// |   all the ORDER-LINE rows that have the same OL_O_ID value, and OL_DIST_INFO is set to the content of S_DIST_xx, where xx represents the district number (OL_D_ID)

TPCC::OrderLineVersion* TPCCClient::insertIntoOrderLine(uint8_t olNumber, uint32_t oID, Cart &cart, NewOrderItem &newOrderItem, TPCC::Item *item, TPCC::StockVersion *stockV, Timestamp &ts){
	ServerContext* serverContext = getServerContext(stockV->stock.S_W_ID);
	TPCC::OrderLineVersion *olv = serverContext->getLocalOrderLineHead()->getRegion();

	olv->writeTimestamp.copy(ts);

	olv->orderLine.OL_O_ID 			= oID;
	olv->orderLine.OL_D_ID 			= cart.dID;
	olv->orderLine.OL_W_ID 			= cart.wID;
	olv->orderLine.OL_NUMBER 		= olNumber;
	olv->orderLine.OL_I_ID 			= item->I_ID;
	olv->orderLine.OL_SUPPLY_W_ID 	= newOrderItem.OL_SUPPLY_W_ID;
	olv->orderLine.OL_DELIVERY_D 	= (time_t)0;
	olv->orderLine.OL_QUANTITY 		=  newOrderItem.OL_QUANTITY;
	olv->orderLine.OL_AMOUNT 		= (float)newOrderItem.OL_QUANTITY * item->I_PRICE;
	std::memcpy(olv->orderLine.OL_DIST_INFO, stockV->stock.S_DIST[cart.dID], 25);
	size_t tableOffset = (size_t)(
			(
					(cart.wID * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + cart.dID)
					* config::tpcc_settings::ORDER_PER_DISTRICT + oID
			)
			* tpcc_settings::ORDER_MAX_OL_CNT + olNumber
	)
	* sizeof(TPCC::OrderLineVersion);

	// The remote address to read the item info
	TPCC::OrderLineVersion *writeAddress =  (TPCC::OrderLineVersion *)(tableOffset + ((uint64_t)serverContext->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::OrderLineVersion);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			serverContext->getLocalOrderLineHead()->getRDMAHandler(),
			(uintptr_t)serverContext->getLocalOrderLineHead()->getRegion(),
			&serverContext->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	return serverContext->getLocalOrderLineHead()->getRegion();
}

TPCCClient::~TPCCClient(){
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Destructor called");
	delete context_;
}
