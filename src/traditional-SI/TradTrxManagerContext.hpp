/*
 *	TradTrxManagerContext.hpp
 *
 *	Created on: 20.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef TRAD_TRX_MANAGER_CONTEXT_H_
#define TRAD_TRX_MANAGER_CONTEXT_H_

#include "../util/BaseContext.hpp"
#include "TradMessage.hpp"
#include "TradClientContext.hpp"
#include "TradResManagerContext.hpp"
#include "../../config.hpp"

class TradTrxManagerContext : public BaseContext {
public:
	int trx_num; 
	
	//std::string client_ip;
	//int	client_port;
	
	TradClientContext		client_ctx;
	TradResManagerContext	res_mng_ctxs[SERVER_CNT];
	

	int create_context ();

	int register_memory ();
	
	int destroy_context ();
};
#endif // TRAD_TRX_MANAGER_CONTEXT_H_