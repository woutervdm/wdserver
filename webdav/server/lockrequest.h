#ifndef _LOCK_REQUEST_H
#define _LOCK_REQUEST_H

#include "request.h"

class LockinfoParser;

class LockRequest : public Request
{
public:
	LockRequest(WebdavServer& owner);
	~LockRequest();

	int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize);

private:
	LockinfoParser* m_parser;
	bool m_parsedSuccessfully;
	bool m_initialized;
};
#endif
