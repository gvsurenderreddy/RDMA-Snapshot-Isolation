/*
 * Error.hpp
 *
 *  Created on: Sep 25, 2015
 *      Author: erfanz
 */

#ifndef ERRORS_ERROR_HPP_
#define ERRORS_ERROR_HPP_

namespace error{
	enum ErrorType {
		SUCCESS = 0,
		NO_SUITABLE_VERSION
	};
//
//	static std::map<int,std::string> createErrorCodeToStringMap ();
//	static std::map<int, std::string> getErrorName = createErrorCodeToStringMap();

	class Throwable;
}
#endif /* ERRORS_ERROR_HPP_ */
