#include <iostream>

#include "postrequest.h"

using namespace std;

PostRequest::PostRequest(WebdavServer& owner) :
	Request(owner)
{

}

int PostRequest::handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)
{
	return MHD_NO;
}
