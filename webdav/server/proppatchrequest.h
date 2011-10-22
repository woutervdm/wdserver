#ifndef _PROPPATCH_REQUEST_H
#define _PROPPATCH_REQUEST_H

#include "request.h"
#include "proppatchparser.h"

class ProppatchRequest : public Request
{
public:
	ProppatchRequest(WebdavServer& owner);
	~ProppatchRequest();

	int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize);

private:
	ProppatchParser* m_parser;
	bool m_parsedSuccessfully;
};
#endif
