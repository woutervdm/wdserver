#ifndef _DELETE_REQUEST_H
#define _DELETE_REQUEST_H

#include "request.h"

class DeleteRequest : public Request
{
public:
	DeleteRequest(WebdavServer& owner);

	int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize);
};
#endif
