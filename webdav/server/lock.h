#ifndef _LOCK_H
#define _LOCK_H

#include <string>

class Lock
{
public:
	Lock();
	Lock(bool _shared, bool _infinite, const std::string& _owner);

	bool shared;
	bool infinite;
	std::string owner;
	std::string token;

private:
	void genToken();

	static int s_counter;
};

#endif
