#include <iostream>
#include <stdlib.h>

#include "request.h"

using namespace std;

Request::Request(WebdavServer& owner) :
	m_owner(owner)
{

}

Request::~Request()
{

}

std::string Request::getLockToken(const char* _ifHeader)
{
	if (!_ifHeader)
		return "";

	string ifHeader(_ifHeader);
	string::size_type pos1 = ifHeader.find("(<");
	string::size_type pos2 = ifHeader.find(">)");

	string ret = pos1 != string::npos && pos2 != string::npos ? ifHeader.substr(pos1+2, pos2-pos1-2) : "";
	return ret;
}

bool Request::checkLock(struct MHD_Connection* connection, const char* url, int& ret)
{
	string lockToken = getLockToken(MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "if"));

	if (!m_owner.checkLock(url, lockToken))
	{
		MHD_Response* response = MHD_create_response_from_data(0, 0, MHD_NO, MHD_NO);
		m_owner.initRequest(response);
		ret = MHD_queue_response(connection, lockToken.substr(0, 4).compare("DAV:") == 0 ? 412 : 423, response);
		MHD_destroy_response(response);

		return false;
	}

	return true;
}
