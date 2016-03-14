/*
 *	IPTradResManager.hpp
 *
 *	Created on: 21.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef IPTRAD_RES_MANAGER_H_
#define IPTRAD_RES_MANAGER_H_

#include "../../two-sided/BaseTradResManager.hpp"
#include "../../two-sided/IPoIB/IPTradResManagerContext.hpp"


class IPTradResManager : public BaseTradResManager {
private:
	static void* handle_trx_mng_thread(void *param);
	
public:
	int start_server (int server_num);
};
#endif // IP_TRAD_RES_MANAGER_H_ 