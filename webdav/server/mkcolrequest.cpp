#include <iostream>
#include <sys/stat.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>

#include "mkcolrequest.h"

using namespace std;

MkcolRequest::MkcolRequest(WebdavServer& owner) :
	Request(owner)
{

}

int MkcolRequest::handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)
{
	struct MHD_Response* response;
	response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
	m_owner.initRequest(response);
	int ret = 0;
	char* dup = strdup(url);
	string path = m_owner.getRoot() + dirname(dup);
	string name = basename(dup);
	free(dup);
	if (!m_owner.checkPath(path))
	{
		ret = MHD_queue_response(connection, 409, response);
		MHD_destroy_response(response);
		return ret;
	}
	string fullPath(path);
	if (fullPath[fullPath.length()-1] != '/')
		fullPath += "/";
	fullPath += name;

	if (!checkLock(connection, url, ret))
	{
		return ret;
	}
	ret = 0;

	struct stat st;
	if (ret == 0 && stat(path.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
	{
		ret = MHD_queue_response(connection, 403, response);
		MHD_destroy_response(response);
		return ret;
	}

	if (ret == 0 && stat(fullPath.c_str(), &st) == 0)
	{
		ret = MHD_queue_response(connection, 405, response);
		MHD_destroy_response(response);
		return ret;
	}

	if (ret == 0 && mkdir(fullPath.c_str(), 0777) != 0)
	{
		ret = MHD_queue_response(connection, 403, response);
		MHD_destroy_response(response);
		return ret;
	}

	// check content-length
	const char* _contentLength = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "content-length");
	long long contentLength = _contentLength ? atoll(_contentLength) : 0;
	if (contentLength != 0)
	{
		ret = MHD_queue_response(connection, 415, response);
		MHD_destroy_response(response);
		return ret;
	}

	ret = MHD_queue_response(connection, 201, response);
	MHD_destroy_response(response);
	return ret;
}
