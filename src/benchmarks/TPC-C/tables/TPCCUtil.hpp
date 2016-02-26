/*
 * TPCCSettings.hpp
 *
 *  Created on: Feb 19, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_TPCCUTIL_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_TPCCUTIL_HPP_

#include "../../../../config.hpp"
#include "../random/randomgenerator.hpp"
#include <cstring>




namespace tpcc_settings{
static const int ADDRESS_MIN_STREET = 10;
static const int ADDRESS_MAX_STREET = 20;
static const int ADDRESS_MIN_CITY = 10;
static const int ADDRESS_MAX_CITY = 20;
static const int ADDRESS_STATE = 2;
static const int ADDRESS_ZIP = 9;

static const int ITEM_MIN_IM = 1;
static const int ITEM_MAX_IM = 10000;
static const float ITEM_MIN_PRICE = 1.00;
static const float ITEM_MAX_PRICE = 100.00;
static const int ITEM_MIN_NAME = 14;
static const int ITEM_MAX_NAME = 24;
static const int ITEM_MIN_DATA = 26;
static const int ITEM_MAX_DATA = 50;

static const float WAREHOUSE_MIN_TAX = 0;
static const float WAREHOUSE_MAX_TAX = 0.2000f;
static const float WAREHOUSE_INITIAL_YTD = 300000.00f;
static const int WAREHOUSE_MIN_NAME = 6;
static const int WAREHOUSE_MAX_NAME = 10;

static const float DISTRICT_MIN_TAX = 0;
static const float DISTRICT_MAX_TAX = 0.2000f;
static const float DISTRICT_INITIAL_YTD = 30000.00;  // different from Warehouse
static const int DISTRICT_INITIAL_NEXT_O_ID = 3001;
static const int DISTRICT_MIN_NAME = 6;
static const int DISTRICT_MAX_NAME = 10;

static const int STOCK_MIN_QUANTITY = 10;
static const int STOCK_MAX_QUANTITY = 100;
static const int STOCK_DIST = 24;
static const int STOCK_MIN_DATA = 26;
static const int STOCK_MAX_DATA = 50;

static const float CUSTOMER_INITIAL_CREDIT_LIM = 50000.00;
static const float CUSTOMER_MIN_DISCOUNT = 0.0000;
static const float CUSTOMER_MAX_DISCOUNT = 0.5000;
static const float CUSTOMER_INITIAL_BALANCE = -10.00;
static const float CUSTOMER_INITIAL_YTD_PAYMENT = 10.00;
static const int CUSTOMER_INITIAL_PAYMENT_CNT = 1;
static const int CUSTOMER_INITIAL_DELIVERY_CNT = 0;
static const int CUSTOMER_MIN_FIRST = 6;
static const int CUSTOMER_MAX_FIRST = 10;
static const int CUSTOMER_MIDDLE = 2;
static const int CUSTOMER_MAX_LAST = 16;
static const int CUSTOMER_PHONE = 16;
static const int CUSTOMER_CREDIT = 2;
static const int CUSTOMER_MIN_DATA = 300;
static const int CUSTOMER_MAX_DATA = 500;
static const int CUSTOMER_NUM_PER_DISTRICT = 3000;

static const int ORDER_MIN_CARRIER_ID = 1;
static const int ORDER_MAX_CARRIER_ID = 10;
// HACK: This is not strictly correct, but it works
static const int ORDER_NULL_CARRIER_ID = 0;
// Less than this value, carrier != null, >= -> carrier == null
static const int ORDER_NULL_CARRIER_LOWER_BOUND = 2101;
static const int ORDER_MIN_OL_CNT = 5;
static const int ORDER_MAX_OL_CNT = 15;
static const int ORDER_INITIAL_ALL_LOCAL = 1;
static const int ORDER_INITIAL_ORDERS_PER_DISTRICT = 3000;
// See TPC-C 1.3.1 (page 15)
static const int ORDER_MAX_ORDER_ID = 10000000;


static const int ORDERLINE_MIN_I_ID = 0;
static const int ORDERLINE_MAX_I_ID = config::tpcc_settings::ITEMS_CNT - 1;  // Item::NUM_ITEMS
static const int ORDERLINE_INITIAL_QUANTITY = 5;
static const float ORDERLINE_MIN_AMOUNT = 0.01f;
static const float ORDERLINE_MAX_AMOUNT = 9999.99f;

static const int HISTORY_MIN_DATA = 12;
static const int HISTORY_MAX_DATA = 24;
static const float HISTORY_INITIAL_AMOUNT = 10.00f;

static const int NEWORDER_INITIAL_NUM_PER_DISTRICT = 900;


static inline float makeTax(TPCC::RandomGenerator& random) {
	return random.fixedPoint(4, WAREHOUSE_MIN_TAX, WAREHOUSE_MAX_TAX);
}

static inline void makeZip(TPCC::RandomGenerator& random, char* zip) {
	random.nstring(zip, 4, 4);
	std::memcpy(zip + 4, "11111", 6);
}

}	// namespace tpcc_settings






#endif /* SRC_BENCHMARKS_TPC_C_TABLES_TPCCUTIL_HPP_ */
