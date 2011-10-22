#ifndef _MKCOL_REQUEST_H
#define _MKCOL_REQUEST_H

#include "request.h"

class MkcolRequest : public Request
{
public:
	MkcolRequest(WebdavServer& owner);

	int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize);
};
#endif
