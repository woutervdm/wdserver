#include <iostream>

#include "proppatchrequest.h"

using namespace std;

ProppatchRequest::ProppatchRequest(WebdavServer& owner) :
	Request(owner),
	m_parser(0)
{

}

ProppatchRequest::~ProppatchRequest()
{
	if (m_parser)
	{
		delete m_parser;
	}
}

int ProppatchRequest::handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)
{
	int ret;

	if (m_parser == 0)
	{
		if (m_parser)
		{
			delete m_parser;
		}

		m_parser = new ProppatchParser();
		m_parsedSuccessfully = true;
		return MHD_YES;
	}

	if (m_parser && *uploadDataSize != 0)
	{
		m_parsedSuccessfully = m_parsedSuccessfully && m_parser->parse(uploadData, *uploadDataSize);
		*uploadDataSize = 0;
		return MHD_YES;
	}
	else if (m_parser && *uploadDataSize == 0)
	{
		// finish parser and handle request
		m_parsedSuccessfully = m_parsedSuccessfully && m_parser->finish();
		if (!m_parsedSuccessfully)
		{
			MHD_Response* response = MHD_create_response_from_data(0, 0, MHD_NO, MHD_NO);
			m_owner.initRequest(response);
			ret = MHD_queue_response(connection, 400, response);
			MHD_destroy_response(response);
			return ret;
		}

		if (!checkLock(connection, url, ret))
		{
			return ret;
		}

		// check path
		string path = m_owner.getRoot() + url;
		if (!m_owner.checkPath(path))
		{
			MHD_Response* response = MHD_create_response_from_data(0, 0, MHD_NO, MHD_NO);
			m_owner.initRequest(response);
			ret = MHD_queue_response(connection, 404, response);
			MHD_destroy_response(response);
			return ret;
		}

		string xml("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
				"<D:multistatus xmlns:D=\"DAV:\">\n"
				"<D:response>\n"
				"<D:href>");
		xml.append(url);
		xml.append("</D:href>\n");

		const vector<ProppatchParser::prop_t>& props = m_parser->getProps();

		for(unsigned i=0; i<props.size(); i++)
		{
			xml.append("<D:propstat>\n"
					"<D:prop><");
			xml.append(props[i].name);
			xml.append(" xmlns=\"");
			xml.append(props[i].xmlns);
			xml.append("\"/></D:prop>\n"
					"<D:status>HTTP/1.1 ");

			if (props[i].xmlns.compare("DAV:") == 0)
			{
				xml.append("403 Forbidden");
			}
			else
			{
				if (props[i].remove)
				{
					m_owner.unsetProperty(url, props[i].name, props[i].xmlns);
				}
				else
				{
					m_owner.setProperty(url, props[i].name, props[i].xmlns, props[i].value);
				}
				xml.append("200 OK");
			}

			xml.append("</D:status>\n"
					"</D:propstat>\n");
		}

		xml.append("</D:response>\n"
				"</D:multistatus>\n");
		MHD_Response* response = MHD_create_response_from_data(xml.length(), (void*) xml.c_str(), MHD_NO, MHD_YES);
		m_owner.initRequest(response);
		MHD_add_response_header(response, "content-type", "application/xml; charset=\"utf-8\"");
		ret = MHD_queue_response(connection, 207, response);
		MHD_destroy_response(response);
		return ret;
	}

	return MHD_NO;
}
