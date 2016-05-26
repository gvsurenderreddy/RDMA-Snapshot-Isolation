/*
 * Runnable.cpp
 *
 *  Created on: May 22, 2016
 *      Author: erfanz
 */

#include "Runnable.hpp"

Runnable::Runnable()
: isAlive_(true),
  isRunning_(false){
}

Runnable::~Runnable() {
	// TODO Auto-generated destructor stub
}

bool Runnable::isRunning() const{
	return isRunning_;
}

bool Runnable::isAlive() const{
	return isAlive_;
}

void Runnable::run(){
	isRunning_ = true;
	handlerThread_ = std::thread(&Runnable::execute, this);
}

void Runnable::shutDown(){
	isAlive_ = false;
	waitUntilFinish();
}

void Runnable::waitUntilFinish(){
	if (isRunning_)
		handlerThread_.join();
	isRunning_ = false;
}
