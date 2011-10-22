#include <iostream>
#include <sys/stat.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "copyrequest.h"
#include "urlencode.h"

using namespace std;

CopyRequest::CopyRequest(WebdavServer& owner, bool deleteSource) :
	Request(owner),
	m_deleteSource(deleteSource)
{

}

int CopyRequest::handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)
{
	const char* contentLength = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "content-length");
	struct MHD_Response* response = MHD_create_response_from_data(0, 0, MHD_NO, MHD_NO);

	m_owner.initRequest(response);

	int ret;
	if (contentLength && strcmp(contentLength, "0") != 0)
	{
		ret = MHD_queue_response(connection, 415, response);
		MHD_destroy_response(response);
		return ret;
	}


	if (m_deleteSource && !checkLock(connection, url, ret))
	{
		return ret;
	}

	const char* depth = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "depth");
	if (!depth)
		depth = "infinity";

	int iDepth = strcasecmp(depth, "infinity") == 0 ? -1 : atoi(depth);

	const char* _destination = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "destination");
	const char* host = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "host");

	// check for other destination
	string self("http://");
	self.append(host);
	if (self.compare(0, self.length(), _destination, self.length()) != 0)
	{
		ret = MHD_queue_response(connection, 502, response);
		MHD_destroy_response(response);
		return ret;
	}
	string destination(_destination);
	cout << "Moving file to: " << destination << endl;
	destination = destination.substr(self.length());

	const char* _overwrite = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "overwrite");
	bool overwrite = _overwrite && strcmp(_overwrite, "T") == 0;

	string source = m_owner.getRoot() + url;
	if (!m_owner.checkPath(source))
	{
		ret = MHD_queue_response(connection, 404, response);
		MHD_destroy_response(response);
		return ret;
	}
	string destUrl = destination;

	if (!checkLock(connection, destUrl.c_str(), ret))
	{
		return ret;
	}
	destination = m_owner.getRoot() + Util::UriDecode(destination);
	bool newResource = !m_owner.checkPath(destination);
	struct stat st;
	bool destIsDir = stat(destination.c_str(), &st)==0 && S_ISDIR(st.st_mode);

	if (!newResource && m_deleteSource && destIsDir && !overwrite)
	{
		ret = MHD_queue_response(connection, 412, response);
		MHD_destroy_response(response);
		return ret;
	}

	bool existingCol = false;
	if (!newResource && m_deleteSource && destIsDir)
	{
		char* dup = strdup(url);
		destination.append(basename(dup));
		free(dup);
		if (stat(destination.c_str(), &st) != 0)
		{
			newResource = true;
			existingCol = true;
		}
	}

	if (!newResource && !overwrite)
	{
		ret = MHD_queue_response(connection, 412, response);
		MHD_destroy_response(response);
		return ret;
	}
	else if (!newResource && overwrite)
	{
		WebdavServer::recursiveDelete(destination);
	}

	char* dup = strdup(destination.c_str());
	string destDirname = dirname(dup);
	free(dup);
	if (!m_owner.checkPath(destDirname))
	{
		ret = MHD_queue_response(connection, 409, response);
		MHD_destroy_response(response);
		return ret;
	}
	else if (m_deleteSource && !rename(source.c_str(), destination.c_str()))
	{
		m_owner.moveProperties(url, destUrl);
	}
	else if (!m_deleteSource)
	{
		// TODO: copy properties
		WebdavServer::recursiveCopy(source, destination, iDepth);
	}
	ret = MHD_queue_response(connection, newResource && !existingCol ? 201 : 204, response);
	MHD_destroy_response(response);
	return ret;
}
