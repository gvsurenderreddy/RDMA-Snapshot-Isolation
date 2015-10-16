/*
 *	TradMessage.hpp
 *
 *	Created on: 20.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef TRAD_MESSAGE_H_
#define TRAD_MESSAGE_H_

#include <infiniband/verbs.h>
#include "../tpcw-tables/item_version.hpp"
#include "../tpcw-tables/orders.hpp"
#include "../tpcw-tables/cc_xacts.hpp"
#include "../tpcw-tables/shopping_cart_line.hpp"
#include "../auxilary/timestamp.hpp"
#include "../../config.hpp"


struct Cart {
	ShoppingCartLine	cart_lines[ORDERLINE_PER_ORDER];
};

struct CommitRequest {
	Cart		cart;
	Orders		order;
	CCXacts		ccxact;
};

struct CommitResponse {
	enum CommitOutcome {
		COMMITTED,
		ABORTED
	} commit_outcome;
};

struct ItemInfoRequest {
	int I_ID;
};

struct ItemInfoResponse {
	ItemVersion		item_version;
};

struct LockRequest {
	int I_ID;
};

struct LockResponse {
	bool was_successful;
};

struct WriteDataRequest {
	enum Type {
		DO_NOTHING,
		UNLOCK,
		INSTALL_VERSION
	} type;
	
	union {
		struct WriteVersionRequest {
			ShoppingCartLine	cart_line;
			Timestamp			commit_ts;
		} write_ver_req;
		int I_ID;
	} content;
};

struct WriteDataResponse {
	int I_ID;
};

#endif /* TRAD_MESSAGE_H_ */