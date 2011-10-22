#ifndef _OPTIONS_REQUEST_H
#define _OPTIONS_REQUEST_H

#include "request.h"

class OptionsRequest : public Request
{
public:
	OptionsRequest(WebdavServer& owner);

	int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize);
};
#endif
