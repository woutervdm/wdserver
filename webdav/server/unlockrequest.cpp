#include <iostream>

#include "unlockrequest.h"

using namespace std;

UnlockRequest::UnlockRequest(WebdavServer& owner) :
	Request(owner)
{

}

int UnlockRequest::handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)
{
	struct MHD_Response* response;
	response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
	m_owner.initRequest(response);

	string lockToken = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "lock-token");
	lockToken = lockToken.substr(1, lockToken.length()-2);
	int ret;
	try
	{
		m_owner.removeLock(url, lockToken);
		ret = MHD_queue_response(connection, 204, response);
		MHD_destroy_response(response);
	}
	catch(WebdavServer::NotLocked&)
	{
		ret = MHD_queue_response(connection, 409, response);
		MHD_destroy_response(response);
	}
	return ret;
}
