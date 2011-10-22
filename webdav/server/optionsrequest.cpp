#include <iostream>

#include "optionsrequest.h"

using namespace std;

OptionsRequest::OptionsRequest(WebdavServer& owner) :
	Request(owner)
{

}

int OptionsRequest::handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)
{
	struct MHD_Response* response;
	response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
	m_owner.initRequest(response);
	MHD_add_response_header(response, "MS-Author-Via", "DAV");
	MHD_add_response_header(response, "DAV", "1, 2");
	MHD_add_response_header(response, "Allow", "OPTIONS, HEAD, GET, DELETE, MKCOL, COPY, MOVE, POST, PUT, PROPFIND, PROPPATCH, LOCK, UNLOCK");

	int ret = MHD_queue_response(connection, 200, response);
	MHD_destroy_response(response);

	return ret;
}
