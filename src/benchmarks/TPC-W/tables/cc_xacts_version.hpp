/*
 *	cc_xacts_version.hpp
 *
 *	Created on: 22.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef CC_XACTS_VERSION_HPP_
#define CC_XACTS_VERSION_HPP_

#include "cc_xacts.hpp"

namespace TPCW {
class CCXactsVersion {
public:
	int write_timestamp;
	CCXacts cc_xacts;
};
}	// namespace TPCW

#endif // CC_XACTS_VERSION_HPP_
