#include <stdint.h>

class Lock {
public:
	static uint32_t get_lock_status	(uint64_t lock);
	static uint32_t get_version		(uint64_t lock);
	static uint64_t set_lock		(uint32_t lock_status,	uint32_t version);
	static bool		are_equals		(uint64_t lock_1,		uint64_t lock_2);
	
};