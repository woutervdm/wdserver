#include <iostream>
#include <vector>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <stdio.h>

#include "propfindparser.h"
#include "propfindrequest.h"
#include "urlencode.h"

using namespace std;

PropfindRequest::prop_t::prop_t(const std::string& _name, const std::string& _value, const std::string& _ns) :
	name(_name),
	value(_value),
	xmlns(_ns)
{
}

PropfindRequest::PropfindRequest(WebdavServer& owner) :
	Request(owner),
	m_parser(0)
{
}

PropfindRequest::~PropfindRequest()
{
	if (m_parser)
	{
		delete m_parser;
	}
}

int PropfindRequest::handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)
{
	int ret;

	if (m_parser == 0)
	{
		const char* _depth = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "depth");
		m_depth = -1;
		if (_depth && strcasecmp(_depth, "infinity") != 0)
		{
			m_depth = atoi(_depth);
		}

		if (m_parser)
		{
			delete m_parser;
		}

		m_parser = new PropfindParser();
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
		const char* contentLength = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "content-length");
		if (contentLength && strcmp(contentLength, "0") != 0 && !m_parsedSuccessfully)
		{
			MHD_Response* response = MHD_create_response_from_data(0, 0, MHD_NO, MHD_NO);
			m_owner.initRequest(response);
			ret = MHD_queue_response(connection, 400, response);
			MHD_destroy_response(response);
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

		string xml("<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
				"<D:multistatus xmlns:D=\"DAV:\">\n"
				);

		m_nsDefs = "xmlns:ns0=\"urn:uuid:c2f41010-65b3-11d1-a29f-00aa00c14882/\"";
		m_nsHash.clear();

		addResponse(xml, url, m_depth);

		xml.append("</D:multistatus>\n");
		MHD_Response* response = MHD_create_response_from_data(xml.length(), (void*) xml.c_str(), MHD_NO, MHD_YES);
		m_owner.initRequest(response);
		MHD_add_response_header(response, "content-type", "application/xml; charset=\"utf-8\"");
		
		ret = MHD_queue_response(connection, 207, response);

		MHD_destroy_response(response);
		return ret;
	}

	return MHD_NO;
}

void PropfindRequest::hashNamespace(const string& ns)
{
	map<string, string>::iterator k = m_nsHash.find(ns);
	if (k == m_nsHash.end())
	{
		char nsname[512];
		sprintf(nsname, "ns%lu", m_nsHash.size()+1);
		m_nsHash[ns] = nsname;
		m_nsDefs.append(" xmlns:");
		m_nsDefs.append(nsname);
		m_nsDefs.append("=\"");
		m_nsDefs.append(ns);
		m_nsDefs.append("\"");
	}
}

void PropfindRequest::getProperties(const std::string& url, props_t& found, props_t& notFound)
{
	const vector<PropfindParser::prop_t>& requested = m_parser->getProps();
	map<PropfindParser::prop_t, string> allProperties;
	allProperties[PropfindParser::prop_t("displayname", "DAV:")] = url;

	struct stat st;
	string path = m_owner.getRoot() + url;
	stat(path.c_str(), &st);
	//gmdate("Y-m-d\\TH:i:s\\Z", $prop['val']);

	char buf[512];
	struct tm* tm_time;
	tm_time = gmtime(&st.st_mtimespec.tv_sec);
	strftime(buf, 512, "%Y-%m-%dT%H:%M:%SZ", tm_time);
	allProperties[PropfindParser::prop_t("creationdate", "DAV:")] = buf;

	tm_time = gmtime(&st.st_ctimespec.tv_sec);
	//"D, d M Y H:i:s"
	strftime(buf, 512, "%a, %d %b %Y %H:%M:%S", tm_time);
	allProperties[PropfindParser::prop_t("getlastmodified", "DAV:")] = buf;

	if (S_ISDIR(st.st_mode))
	{
		allProperties[PropfindParser::prop_t("resourcetype", "DAV:")] = "collection";
		allProperties[PropfindParser::prop_t("getcontenttype", "DAV:")] = "httpd/unix-directory";

		// get quota
		unsigned long long used = 0;
		unsigned long long avail = 0;
		if (m_owner.getQuota(url.c_str(), used, avail))
		{
			sprintf(buf, "%llu", avail);
			allProperties[PropfindParser::prop_t("quota-available-bytes", "DAV:")] = buf;
			sprintf(buf, "%llu", used);
			allProperties[PropfindParser::prop_t("quota-used-bytes", "DAV:")] = buf;
		}
	}
	else
	{
		allProperties[PropfindParser::prop_t("resourcetype", "DAV:")] = "";
		allProperties[PropfindParser::prop_t("getcontenttype", "DAV:")] = "appliciation/x-non-readable";
		char size[512];
		sprintf(size, "%llu", st.st_size);
		allProperties[PropfindParser::prop_t("getcontentlength", "DAV:")] = size;
	}

	if (m_parser->getPropString().length() == 0)
	{
		// use props
		for(unsigned int i=0; i<requested.size(); i++)
		{
			bool handled = false;
			map<PropfindParser::prop_t, string>::iterator j;
			j = allProperties.find(requested[i]);
			if (j != allProperties.end())
			{
				found.push_back(prop_t(requested[i].name, (*j).second, requested[i].xmlns));
				handled = true;
			}

			// try in file properties
			try
			{
				const WebdavServer::FileProperties_t& props = m_owner.getProperties(url);
				WebdavServer::FileProperties_t::const_iterator j = props.find(WebdavServer::Selector_t(requested[i].xmlns, requested[i].name));
				if (j != props.end())
				{
					found.push_back(prop_t(requested[i].name, (*j).second, requested[i].xmlns));
					handled = true;
				}
			}
			catch(WebdavServer::NotFound& e)
			{
				// no properties for this file
			}

			if (!handled)
			{
				notFound.push_back(prop_t(requested[i].name, "", requested[i].xmlns));
			}

			// namespace handling
			if (requested[i].xmlns.length() > 0 && requested[i].xmlns.compare("DAV:") != 0)
			{
				hashNamespace(requested[i].xmlns);
			}
		}
	}
	else
	{
		bool namesOnly = m_parser->getPropString().compare("names") == 0;
		map<PropfindParser::prop_t, string>::iterator j;
		for(j=allProperties.begin(); j!=allProperties.end(); j++)
		{
			found.push_back(prop_t((*j).first.name, namesOnly ? "" : (*j).second, (*j).first.xmlns));

			if ((*j).first.xmlns.length() > 0 && (*j).first.xmlns.compare("DAV:") != 0)
			{
				hashNamespace((*j).first.xmlns);
			}
		}

		// add custom properties
		try
		{
			const WebdavServer::FileProperties_t& props = m_owner.getProperties(url);

			WebdavServer::FileProperties_t::const_iterator i;
			for(i=props.begin(); i!=props.end(); i++)
			{
				found.push_back(prop_t((*i).first.name, namesOnly ? "" : (*i).second, (*i).first.ns));

				if ((*i).first.ns.length() > 0)
				{
					hashNamespace((*i).first.ns);
				}
			}
		}
		catch(WebdavServer::NotFound& e)
		{
			// no custom properties
		}
	}
}

void PropfindRequest::addResponse(std::string& xml, std::string url, int depth)
{
	vector<prop_t> found;
	vector<prop_t> notFound;

	string path = m_owner.getRoot() + Util::UriDecode(url);
	struct stat st;
	if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode) && url[url.length()-1] != '/')
	{
		url.append(1, '/');
	}

	getProperties(url, found, notFound);

	xml.append("<D:response ");
	xml.append(m_nsDefs);
	xml.append(">\n");
	xml.append("<D:href>");
	xml.append(url);
	xml.append("</D:href>\n");

	if (found.size() > 0)
	{
		xml.append("<D:propstat>\n"
				"<D:prop>\n");

		// add found properties
		for(unsigned int i = 0; i<found.size(); i++)
		{
			if (found[i].value.length() == 0)
			{
				// empty nodes

				if (found[i].xmlns.compare("DAV:") == 0)
				{
					xml.append("<D:");
					xml.append(found[i].name);
					xml.append("/>\n");
				}
				else if (found[i].xmlns.length() > 0)
				{
					xml.append("<");
					xml.append(m_nsHash[found[i].xmlns]);
					xml.append(":");
					xml.append(found[i].name);
					xml.append("/>\n");
				}
				else
				{
					xml.append("<");
					xml.append(found[i].name);
					xml.append("/>\n");
				}
			}
			else if (found[i].xmlns.compare("DAV:") == 0)
			{
				if (found[i].name.compare("creationdate") == 0)
				{
					xml.append("<D:creationdate ns0:dt=\"dateTime.tz\">");
					xml.append(found[i].value);
					xml.append("</D:creationdate>\n");
				}
				else if (found[i].name.compare("getlastmodified") == 0)
				{
					xml.append("<D:getlastmodified ns0:dt=\"dateTime.rfc1123\">");
					xml.append(found[i].value);
					xml.append("</D:getlastmodified>\n");
				}
				else if (found[i].name.compare("resourcetype") == 0)
				{
					xml.append("<D:resourcetype><D:");
					xml.append(found[i].value);
					xml.append("/></D:resourcetype>\n");
				}
				else
				{
					xml.append("<D:");
					xml.append(found[i].name);
					xml.append(">");
					xml.append(WebdavServer::encode(found[i].value));
					xml.append("</D:");
					xml.append(found[i].name);
					xml.append(">\n");
				}
			}
			else if (found[i].xmlns.length() > 0)
			{
				// properties from namespace != "DAV:"
				xml.append("<");
				xml.append(m_nsHash[found[i].xmlns]);
				xml.append(":");
				xml.append(found[i].name);
				xml.append(">");

				xml.append(WebdavServer::encode(found[i].value));

				xml.append("</");
				xml.append(m_nsHash[found[i].xmlns]);
				xml.append(":");
				xml.append(found[i].name);
				xml.append(">\n");
			}
			else
			{
				// properties without a namespace
				xml.append("<");
				xml.append(found[i].name);
				xml.append(" xmlns=\"\">");

				xml.append(WebdavServer::encode(found[i].value));

				xml.append("</");
				xml.append(found[i].name);
				xml.append(">\n");

			}
		}

		xml.append("</D:prop>\n"
				"<D:status>HTTP/1.1 200 OK</D:status>\n"
				"</D:propstat>\n");
	}

	// add non found properties
	if (notFound.size() > 0)
	{
		xml.append("<D:propstat>\n"
				"<D:prop>\n");

		for(unsigned int i=0; i<notFound.size(); i++)
		{
			if (notFound[i].xmlns.compare("DAV:") == 0)
			{
				xml.append("<D:");
				xml.append(notFound[i].name);
				xml.append("/>\n");
			}
			else if (notFound[i].xmlns.length() > 0)
			{
				xml.append("<");
				xml.append(m_nsHash[notFound[i].xmlns]);
				xml.append(":");
				xml.append(notFound[i].name);
				xml.append("/>\n");
			}
			else
			{
				xml.append("<");
				xml.append(notFound[i].name);
				xml.append(" xmlns=\"\"/>\n");
			}
		}

		xml.append("</D:prop>\n"
				"<D:status>HTTP/1.1 404 Not Found</D:status>\n"
				"</D:propstat>\n");
	}

	xml.append("</D:response>\n");

	if (depth == -1 || depth != 0)
	{
		string path = m_owner.getRoot() + url;
		DIR* dir = opendir(path.c_str());
		struct dirent* entry;
		while(dir && (entry = readdir(dir)))
		{
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			{
				continue;
			}

			addResponse(xml, url + entry->d_name + (entry->d_type & DT_DIR ? "/" : ""), depth-1);
		}

		if (dir)
		{
			closedir(dir);
		}
	}
}
