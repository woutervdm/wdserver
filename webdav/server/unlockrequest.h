#ifndef _UNLOCK_REQUEST_H
#define _UNLOCK_REQUEST_H

#include "request.h"

class UnlockRequest : public Request
{
public:
	UnlockRequest(WebdavServer& owner);

	int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize);
};
#endif
