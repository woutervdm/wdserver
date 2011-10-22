#include <iostream>
#include <sys/stat.h>
#include <strings.h>

#include "deleterequest.h"

using namespace std;

DeleteRequest::DeleteRequest(WebdavServer& owner) :
	Request(owner)
{

}

int DeleteRequest::handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)
{
	struct MHD_Response* response;
	response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
	m_owner.initRequest(response);

	int ret = 0;
	const char* httpDepth = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "http-depth");
	if (httpDepth && strcasecmp(httpDepth, "infinity") != 0)
	{
		ret = MHD_queue_response(connection, 400, response);
		MHD_destroy_response(response);
		return ret;
	}

	if (!checkLock(connection, url, ret))
	{
		return ret;
	}

	string path = m_owner.getRoot() + url;
	if (!m_owner.checkPath(path))
	{
		ret = MHD_queue_response(connection, 404, response);
		MHD_destroy_response(response);
		return ret;
	}
	struct stat st;
	stat(path.c_str(), &st);
	// TODO: remove properties
	if (S_ISDIR(st.st_mode))
	{
		WebdavServer::recursiveDelete(path);
	}
	else
	{
		unlink(path.c_str());
	}
	ret = MHD_queue_response(connection, 204, response);
	MHD_destroy_response(response);
	return ret;
}
