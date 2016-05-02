/*
 * argumentParser.hpp
 *
 *  Created on: May 1, 2016
 *      Author: erfanz
 */

#ifndef SRC_UTIL_ARGUMENTPARSER_HPP_
#define SRC_UTIL_ARGUMENTPARSER_HPP_


#include <unordered_map>
#include <cstring> //std::strlen

class SimpleArgumentParser{
private:
	std::unordered_map<std::string, std::string> options_;

public:
	SimpleArgumentParser(int argc, char* const argv[]){
		int i = 0;
		while(i < argc){
			if (argv[i][0] == '-'){
				if (i < argc - 1 && argv[i+1][0] != '-') {
					options_[std::string(argv[i])] = std::string(argv[i+1]);
					i++;
				}
				else options_[std::string(argv[i])] = std::string("");
			}
			else{
				options_[std::string(argv[i])] = std::string("");
			}
			i++;
		}
	}

	bool hasOption(std::string option) const {
		return options_.find(option) != options_.end();
	}

	std::string getArgumentForOption(std::string option) const {
		return options_.at(option);
	}
};
#endif /* SRC_UTIL_ARGUMENTPARSER_HPP_ */
