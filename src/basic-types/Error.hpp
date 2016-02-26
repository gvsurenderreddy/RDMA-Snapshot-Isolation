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
		CAS_FAILED,
		KEY_NOT_FOUND,
		NO_UPDATE_KEY_IN_CHANGE,
		DIFFERENT_BUCKET_HEADS,
		NOT_MARKED_SERIALIZED,
		CHANGE_FAILURE,
		UNSERIALIZABLE,
		RESOLVE_FAILED,
		SET_WITH_NO_GET,
		GET_POINTER_CHANGED,
		UNKNOWN_REQUEST_TYPE,
		ENTRY_DOES_NOT_EXIST
	};
//
//	static std::map<int,std::string> createErrorCodeToStringMap ();
//	static std::map<int, std::string> getErrorName = createErrorCodeToStringMap();

	class Throwable;
}
#endif /* ERRORS_ERROR_HPP_ */
