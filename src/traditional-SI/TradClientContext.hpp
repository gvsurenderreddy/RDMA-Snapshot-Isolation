/*
 *	TradClientContext.hpp
 *
 *	Created on: 20.Feb.2015
 *	Author: erfanz
 */

#ifndef TRAD_CLIENT_CONTEXT_H_
#define TRAD_CLIENT_CONTEXT_H_

#include "../util/BaseContext.hpp"
#include "TradMessage.hpp"
#include "../../config.hpp"

class TradClientContext : public BaseContext {
public:
	int trx_num; 
	struct Cart				shopping_cart;
	
	std::string client_ip;
	int client_port;
	
	// memory handler
	struct ibv_mr			*commit_req_mr;
	struct ibv_mr			*commit_res_mr;
	
	// memory bufferes
	struct CommitRequest	commit_request;
	struct CommitResponse	commit_response;
	
	int register_memory ();
	int destroy_context ();
};
#endif // TRAD_CLIENT_CONTEXT_H_