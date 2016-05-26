/*
 * Runnable.hpp
 *
 *  Created on: May 22, 2016
 *      Author: erfanz
 */

#ifndef SRC_UTIL_RUNNABLE_HPP_
#define SRC_UTIL_RUNNABLE_HPP_

#include <thread>

class Runnable {
protected:
	bool isAlive_;
private:
	bool isRunning_;
	std::thread handlerThread_;

public:
	Runnable();
	virtual ~Runnable();
	bool isRunning() const;
	bool isAlive() const;
	void run();
	void shutDown();
	void waitUntilFinish();
	virtual void execute() = 0;
};

#endif /* SRC_UTIL_RUNNABLE_HPP_ */
