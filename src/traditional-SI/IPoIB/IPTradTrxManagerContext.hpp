/*
 *	IPTradTrxManagerContext.hpp
 *
 *	Created on: 21.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef IP_TRAD_TRX_MANAGER_CONTEXT_H_
#define IP_TRAD_TRX_MANAGER_CONTEXT_H_

#include "../../util/BaseContext.hpp"
#include "../TradMessage.hpp"
#include "IPTradClientContext.hpp"
#include "IPTradResManagerContext.hpp"
#include "../../../config.hpp"

class IPTradTrxManagerContext : public BaseContext {
public:
	int trx_num; 
	
	//std::string client_ip;
	//int	client_port;
	
	IPTradClientContext		client_ctx;
	IPTradResManagerContext	res_mng_ctxs[SERVER_CNT];
	

	int create_context ();

	int register_memory ();
	
	int destroy_context ();
};
#endif // IP_TRAD_TRX_MANAGER_CONTEXT_H_