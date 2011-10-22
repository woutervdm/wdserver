#ifndef _COPY_REQUEST_H
#define _COPY_REQUEST_H

#include "request.h"

class CopyRequest : public Request
{
public:
	CopyRequest(WebdavServer& owner, bool deleteSource);

	int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize);

private:
	bool m_deleteSource;
};
#endif
