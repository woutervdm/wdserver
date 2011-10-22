#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/stat.h>

#include "lockinfoparser.h"
#include "lockrequest.h"

using namespace std;

LockRequest::LockRequest(WebdavServer& owner) :
	Request(owner),
	m_parser(0),
	m_initialized(false)
{

}

LockRequest::~LockRequest()
{
	if (m_parser)
	{
		delete m_parser;
	}
}

int LockRequest::handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)
{
	int ret = MHD_NO;

	if (!m_initialized)
	{
		m_initialized = true;
		const char* contentLength = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "content-length");
		if (m_parser)
		{
			delete m_parser;
		}

		m_parser = contentLength == 0 || strcmp(contentLength, "0") == 0 ? 0 : new LockinfoParser();
		m_parsedSuccessfully = true;
		if (*uploadDataSize == 0)
			return MHD_YES;
	}

	if (m_parser && *uploadDataSize != 0)
	{
		m_parsedSuccessfully = m_parsedSuccessfully && m_parser->parse(uploadData, *uploadDataSize);
		*uploadDataSize = 0;
		return MHD_YES;
	}
	else if (*uploadDataSize == 0)
	{
		if (m_parser)
		{
			// finish parser and handle request
			m_parsedSuccessfully = m_parsedSuccessfully && m_parser->finish();
		}

		struct MHD_Response* response;

		const char* depth = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "depth");

		bool infinite = false;
		if (depth && strcasecmp(depth, "infinity") == 0)
		{
			infinite = true;
		}
		else if (depth && strcmp(depth, "0") != 0)
		{
			// send bad request
			response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
			m_owner.initRequest(response);
			ret = MHD_queue_response(connection, 400, response);
			MHD_destroy_response(response);
			return ret;
		}

		// check for IF header
		const char* ifHeader = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "if");
		if ((ifHeader && m_parser) || (!m_parser && !ifHeader))
		{
			// send bad request
			response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
			m_owner.initRequest(response);
			ret = MHD_queue_response(connection, 400, response);
			MHD_destroy_response(response);
			return ret;
		}

		stringstream xml;
		//const char* timeout = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "timeout");
		string lockToken;

		if (ifHeader)
		{
			lockToken = getLockToken(ifHeader);
		}

		// check if file exists, if not then create it
		int status = 200;

		struct stat st;
		string path = m_owner.getRoot() + url;
		if (stat(path.c_str(), &st) != 0)
		{
			FILE *handle = fopen(path.c_str(), "w");
			fclose(handle);
			status = 201;
		}

		try
		{

			const Lock& lock = m_parser ? m_owner.setLock(url,
					strcasecmp(m_parser->lockScope.c_str(), "exclusive") != 0,
					infinite,
					m_parser->owner)
					:
					m_owner.getLock(url, lockToken);

			xml << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
					"<D:prop xmlns:D='DAV:'>\n"
						"<D:lockdiscovery>\n"
							"<D:activelock>\n"
								"<D:lockscope><D:" << (lock.shared ? "shared" : "exclusive") << "/></D:lockscope>\n" <<
								"<D:locktype><D:write/></D:locktype>\n" <<
								"<D:depth>" << (lock.infinite ? "infinity" : "0") << "</D:depth>\n"
								"<D:owner>" << lock.owner << "</D:owner>\n"
								"<D:timeout>Infinite</D:timeout>\n"
								"<D:locktoken><D:href>" << lock.token << "</D:href></D:locktoken>\n"
							"</D:activelock>\n"
						"</D:lockdiscovery>\n"
					"</D:prop>\n";

			lockToken = lock.token;
		}
		catch(WebdavServer::AlreadyLocked&)
		{
			status = 423;
		}
		catch(WebdavServer::NotLocked&)
		{
			status = 412;
		}
		string xmlString = xml.str();
		response = MHD_create_response_from_data(xmlString.length(), (void*) xmlString.c_str(), MHD_NO, MHD_YES);
		m_owner.initRequest(response);
		if (status >= 200 && status < 300)
		{
			MHD_add_response_header(response, "Content-Type", "application/xml; charset=\"utf-8\"");
			MHD_add_response_header(response, "Lock-Token", lockToken.c_str());
		}
		ret = MHD_queue_response(connection, status, response);
		MHD_destroy_response(response);
	}

	return ret;
}
