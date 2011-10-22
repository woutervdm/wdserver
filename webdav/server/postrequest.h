#ifndef _POST_REQUEST_H
#define _POST_REQUEST_H

#include "request.h"

class PostRequest : public Request
{
public:
	PostRequest(WebdavServer& owner);

	int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize);
};
#endif
