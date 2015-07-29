/*
 *	IPTradResManagerContext.hpp
 *
 *	Created on: 20.Feb.2015
 *	Author: erfanz
 */

#ifndef IP_TRAD_RES_MANAGER_CONTEXT_H_
#define IP_TRAD_RES_MANAGER_CONTEXT_H_

#include "../../util/BaseContext.hpp"
#include "../TradMessage.hpp"
#include "../../../config.hpp"

class IPTradResManagerContext : public BaseContext {
public:
	int trx_num;
	
	std::string client_ip;
	int client_port;
	
	struct ItemInfoRequest	item_info_request;
	struct ItemInfoResponse	item_info_response;
	struct LockRequest		lock_request;
	struct LockResponse		lock_response;
	struct WriteDataRequest	write_data_request;
	struct WriteDataResponse	write_data_response;
	
	
	int register_memory();
	int destroy_context ();
};
#endif // IP_TRAD_RES_MANAGER_CONTEXT_H_