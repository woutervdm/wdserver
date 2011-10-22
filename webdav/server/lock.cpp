#include <stdio.h>

#include "md5.h"
#include "lock.h"

int Lock::s_counter = 0;

Lock::Lock() :
	shared(false),
	infinite(false)
{
}

Lock::Lock(bool _shared, bool _infinite, const std::string& _owner) :
	shared(_shared),
	infinite(_infinite),
	owner(_owner)
{
	genToken();
}

void Lock::genToken()
{
	char buf[512];
	sprintf(buf, "%d", s_counter++);
	Util::MD5 uniqid;
	uniqid.update("Saltyi2g09jePickle");
	uniqid.update((unsigned char*) &s_counter, sizeof(s_counter));
	uniqid.finalize();

	token = "urn:uuid:unique_";
	token.append(buf);
}
