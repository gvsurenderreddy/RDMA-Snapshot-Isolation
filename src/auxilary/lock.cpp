/*
 *	lock.cpp
 *
 *	Created on: 5.Feb.2015
 *	Author: erfanz
 */

#include "lock.hpp"
#include <stdint.h>
#include <stdio.h>
#include "../util/utils.hpp"
#include <iostream>

uint32_t Lock::get_lock_status(uint64_t lock)
{
	return (uint32_t)(lock >> 32);
}

uint32_t Lock::get_version(uint64_t lock)
{
	return (uint32_t)((lock << 32) >> 32);
}

uint64_t Lock::set_lock(uint32_t lock_status, uint32_t version)
{
	uint64_t lock;
	lock = lock_status;
	lock = lock << 32;
	lock += version;
	return lock;
}

bool Lock::are_equals(uint64_t lock_1, uint64_t lock_2)
{
	if (get_lock_status(lock_1) == get_lock_status(lock_2)
		&& get_version(lock_1) == get_version(lock_2))
			return true;
	else return false;
}