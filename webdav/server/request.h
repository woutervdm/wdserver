#ifndef _REQUEST_H
#define _REQUEST_H

#include "webdavserver.h"
struct MHD_Connection;

class Request
{
public:
	Request(WebdavServer& owner);
	virtual ~Request();

	virtual int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)=0;

	bool checkLock(struct MHD_Connection* connection, const char* url, int& ret);
protected:
	WebdavServer& m_owner;

	static std::string getLockToken(const char* ifHeader);
};

#endif
