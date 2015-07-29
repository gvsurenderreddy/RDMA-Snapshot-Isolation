/*
 *	IPTradClientContext.hpp
 *
 *	Created on: 20.Feb.2015
 *	Author: erfanz
 */

#ifndef IP_TRAD_CLIENT_CONTEXT_H_
#define IP_TRAD_CLIENT_CONTEXT_H_

#include "../../util/BaseContext.hpp"
#include "../TradMessage.hpp"
#include "../../../config.hpp"

class IPTradClientContext : public BaseContext {
public:
	int trx_num; 
	struct Cart				shopping_cart;
	
	std::string client_ip;
	int client_port;

	// memory bufferes
	struct CommitRequest	commit_request;
	struct CommitResponse	commit_response;
	
	int create_context ();
	int register_memory ();
	int destroy_context ();
};
#endif // IP_TRAD_CLIENT_CONTEXT_H_