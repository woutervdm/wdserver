#ifndef _PUT_REQUEST_H
#define _PUT_REQUEST_H

#include "request.h"

class PutRequest : public Request
{
public:
	PutRequest(WebdavServer& owner);
	~PutRequest();

	int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize);

private:
	static int _headerIterator(void* cls, MHD_ValueKind kind, const char* key, const char* value);
	int headerIterator(MHD_ValueKind kind, const char* key, const char* value);

	int m_status;
	FILE* m_handle;
	bool m_newFile;
};
#endif
