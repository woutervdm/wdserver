#include <iostream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

#include "getrequest.h"

using namespace std;

const std::string GetRequest::s_error = "<html><body><p>Error!</p></body></html>";

GetRequest::GetRequest(WebdavServer& owner, bool headersOnly) :
	Request(owner),
	m_handle(0),
	m_headersOnly(headersOnly)
{

}

GetRequest::~GetRequest()
{
	if (m_handle)
		fclose(m_handle);
}

int GetRequest::handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)
{
	struct MHD_Response* response;
	int ret = 0;

	string path = m_owner.getRoot() + url;

	struct stat st;
	if (m_owner.checkPath(path))
	{
		if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
		{
			string html;
			getIndexHTML(url, html);
			response = MHD_create_response_from_data(html.length(), (void*) html.c_str(), MHD_NO, MHD_YES);
			m_owner.initRequest(response);
			ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
			MHD_destroy_response(response);
			return ret;
		}

		m_handle = fopen(path.c_str(), "rb");
	}

	if (!m_handle)
	{
		response = MHD_create_response_from_data(s_error.length(), (void*) s_error.c_str(), MHD_NO, MHD_YES);
		m_owner.initRequest(response);
		ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
		MHD_destroy_response(response);
	}
	else
	{
		// check for if modified header
		bool notModified = false;

		const char* ifModifiedSince = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "if-modified-since");
		if (ifModifiedSince)
		{
			cout << "If modified since: " << ifModifiedSince << endl;
			// todo
		}

		const char* range = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "range");
		if (range)
		{
			parseRanges(range);
			if (m_ranges.size() > 1)
			{
				cout << "Multiple ranges not supported yet." << endl;
				m_ranges.clear();
			}
		}

		fseek(m_handle, 0l, SEEK_END);
		unsigned long long size = ftell(m_handle);
		fseek(m_handle, 0l, SEEK_SET);
		if (m_headersOnly || size == 0 || notModified)
		{
			response = MHD_create_response_from_data(0, 0, MHD_NO, MHD_NO);
			m_owner.initRequest(response);
			stringstream contentLength;
			contentLength << size;
			MHD_add_response_header(response, "content-length", contentLength.str().c_str());
		}
		else
		{
			if (m_ranges.size() > 0)
			{
				Range_t& range = m_ranges[0];

				if (!range.startValid)
				{
					range.start = size - range.end;
					range.end = size-1;
				}
				if (!range.endValid)
				{
					range.end = size-1;
				}

				if (range.start >= size)
				{
					response = MHD_create_response_from_data(0, 0, MHD_NO, MHD_NO);
					m_owner.initRequest(response);
					ret = MHD_queue_response(connection, MHD_HTTP_REQUESTED_RANGE_NOT_SATISFIABLE, response);
					MHD_destroy_response(response);
					return ret;
				}

				stringstream header;
				fseek(m_handle, range.start, SEEK_SET);
				response = MHD_create_response_from_callback(range.end-range.start+1, 8192, &_contentReader, (void*) this, 0);
				m_owner.initRequest(response);

				header << (range.end-range.start+1);
				MHD_add_response_header(response, "content-length", header.str().c_str());

				header.clear();
				if (range.startValid)
					header << range.startValid;
				header << "-";
				if (range.endValid)
					header << range.endValid;
				header << "/" << size;
				MHD_add_response_header(response, "content-range", header.str().c_str());
			}
			else
			{
				response = MHD_create_response_from_callback(size, 8192, &_contentReader, (void*) this, 0);
				m_owner.initRequest(response);
				stringstream contentLength;
				contentLength << size;
				MHD_add_response_header(response, "content-length", contentLength.str().c_str());
			}
		}

		char buf[512];
		struct tm* tm_time;
		tm_time = gmtime(&st.st_ctimespec.tv_sec);
		//"D, d M Y H:i:s"
		strftime(buf, 512, "%a, %d %b %Y %H:%M:%S", tm_time);

		MHD_add_response_header(response, "last-modified", buf);
		int status = m_ranges.size() > 0 ? MHD_HTTP_PARTIAL_CONTENT : MHD_HTTP_OK;
		if (notModified)
			status = MHD_HTTP_NOT_MODIFIED;
		ret = MHD_queue_response(connection, status, response);
		MHD_destroy_response(response);
	}

	return ret;
}

int GetRequest::_contentReader(void* cls, uint64_t pos, char* buf, int max)
{
	GetRequest* self = (GetRequest*) cls;
	return self->contentReader(pos, buf, max);
}

int GetRequest::contentReader(uint64_t pos, char* buf, int max)
{
	// ignore pos
	return max != 0 ? fread(buf, 1, max, m_handle) : 0;
}

void GetRequest::parseRanges(const std::string& ranges)
{
	string::size_type pos = ranges.find("bytes=");
	if (pos == string::npos)
	{
		return;
	}

	pos += 6;
	bool done = false;

	m_ranges.clear();
	while(!done)
	{
		string::size_type dash = ranges.find('-', pos);
		string::size_type comma = ranges.find(',', pos);
		string firstPart = dash != string::npos ? ranges.substr(pos, dash-pos) : "";
		string secondPart = dash != string::npos ? ranges.substr(dash+1, comma != string::npos ? comma-dash-1 : string::npos) : "";

		Range_t range(firstPart, secondPart);
		m_ranges.push_back(range);
		if (comma != string::npos)
		{
			pos = comma + 1;
		}
		done = comma == string::npos;
	}
}

GetRequest::Range_t::Range_t(const std::string& first, const std::string& second)
{
	string::size_type pos = 0;
	startValid = false;
	while(pos < first.length() && !startValid)
	{
		startValid = !isspace(first[pos]);
		pos++;
	}

	pos = 0;
	endValid = false;
	while(pos < second.length() && !endValid)
	{
		endValid = !isspace(second[pos]);
		pos++;
	}

	start = startValid ? strtoull(first.c_str(), NULL, 10) : 0;
	end = endValid ? strtoull(second.c_str(), NULL, 10) : 0;
}

void GetRequest::getIndexHTML(const string& url, string& html)
{
	stringstream stream;
	stream <<
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">"
		"<html>"
			"<head><title>" << m_owner.getRealm() << " " << url << "</title></head>"
			"<body>"
			"<h1>Index of " << url << "</h1>"
			"<pre>";

	const char* format = "%15s  %-25s  ";
	char buf[512];
	char time[512];
	struct tm* tm_time;
	char size[512];
	sprintf(buf, format, "Size", "Last modified");
	stream << buf << "Filename\n<hr>";

	if (url.compare("/") != 0)
	{
		string::size_type pos = url.rfind('/', url.length()-2);
		string dirup = url.substr(0, pos+1);
		sprintf(buf, format, "", "");
		stream << buf << "<a href='" << dirup << "'>..</a>\n";
	}
	string path = m_owner.getRoot() + url;
	DIR* dir = opendir(path.c_str());
	struct dirent* entry;
	struct stat st;
	while(dir && (entry = readdir(dir)))
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}
		string name = entry->d_name;
		if (entry->d_type & DT_DIR)
			name.append(1, '/');

		stat((path + entry->d_name).c_str(), &st);
		tm_time = gmtime(&st.st_ctimespec.tv_sec);
		strftime(time, 512, "%a, %d %b %Y %H:%M:%S", tm_time);

		sprintf(size, "%llu", st.st_size);

		sprintf(buf, format, size, time);
		stream << buf << "<a href='" << name << "'>" << name << "</a>\n";
	}
	stream <<
			"</pre>"
			"</body>"
		"</html>";

	html = stream.str();
}
