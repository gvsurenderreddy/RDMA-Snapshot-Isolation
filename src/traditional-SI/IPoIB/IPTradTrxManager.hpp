/*
 *	IPTradTrxManager.hpp
 *
 *	Created on: 21.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef IP_TRAD_TRX_MANAGER_H_
#define IP_TRAD_TRX_MANAGER_H_

#include "IPTradTrxManagerContext.hpp"
#include "../BaseTradTrxManager.hpp"


class IPTradTrxManager : public BaseTradTrxManager {
private:	
	static void*	handle_client(void *param);
	
	static int		start_transactions(IPTradTrxManagerContext &ctx);	

public:
	int start_server ();

};
#endif // IP_TRADITIONAL_TRX_MANAGER_H_