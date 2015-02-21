/*
 *	TradResManagerContext.hpp
 *
 *	Created on: 20.Feb.2015
 *	Author: erfanz
 */

#ifndef TRAD_RES_MANAGER_CONTEXT_H_
#define TRAD_RES_MANAGER_CONTEXT_H_

#include "../util/BaseContext.hpp"
#include "TradMessage.hpp"
#include "../../config.hpp"

class TradResManagerContext : public BaseContext {
public:
	int trx_num;
	
	std::string client_ip;
	int client_port;
	
	struct ibv_mr *item_info_req_mr;
	struct ibv_mr *item_info_res_mr;
	struct ibv_mr *lock_req_mr;
	struct ibv_mr *lock_res_mr;
	struct ibv_mr *write_data_req_mr;
	
	struct ItemInfoRequest	item_info_request;
	struct ItemInfoResponse	item_info_response;
	struct LockRequest		lock_request;
	struct LockResponse		lock_response;
	struct WriteDataRequest	write_data_request;
	
	int register_memory();
	int destroy_context ();
	
};
#endif // TRAD_RES_MANAGER_CONTEXT_H_