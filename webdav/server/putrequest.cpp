#include <iostream>
#include <libgen.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "urlencode.h"
#include "putrequest.h"

using namespace std;

PutRequest::PutRequest(WebdavServer& owner) :
	Request(owner),
	m_handle(0)
{

}

PutRequest::~PutRequest()
{
	if (m_handle)
	{
		fclose(m_handle);
	}
}

int PutRequest::handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize)
{
	int ret;
	struct MHD_Response* response;

	if (!m_handle)
	{
		response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
		m_owner.initRequest(response);
		const char* contentType = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "content-type");
		if (contentType && strncmp(contentType, "multipart/", 10) == 0)
		{
			cout << "\tResponding with 501" << endl;
			ret = MHD_queue_response(connection, 501, response);
			MHD_destroy_response(response);
			return ret;
		}

		if (!checkLock(connection, url, ret))
		{
			cout << "\tResponding with locked" << endl;
			return ret;
		}

		// check content- headers
		m_status = 200;
		MHD_get_connection_values(connection, MHD_HEADER_KIND, &_headerIterator, (void*) this);

		if (m_status != 200)
		{
			cout << "\tResponding with " << m_status << endl;
			ret = MHD_queue_response(connection, m_status, response);
			MHD_destroy_response(response);
			return ret;
		}

		char* dup = strdup(url);
		string path = m_owner.getRoot() + dirname(dup);
		string fullPath = path + "/" + basename(dup);
		free(dup);

		struct stat st;
		if (!m_owner.checkPath(path) || stat(path.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
		{
			cout << "\tResponding with 409" << endl;
			ret = MHD_queue_response(connection, 409, response);
			MHD_destroy_response(response);
			return ret;
		}

		m_newFile = stat(fullPath.c_str(), &st) != 0;
		m_handle = fopen(fullPath.c_str(), "w");
		MHD_destroy_response(response);
		return MHD_YES;
	}

	if (m_handle && *uploadDataSize != 0)
	{
		*uploadDataSize -= fwrite(uploadData, 1, *uploadDataSize, m_handle);
		return MHD_YES;
	}
	else if (m_handle && *uploadDataSize == 0)
	{
		fclose(m_handle);
		m_handle = 0;
		response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
		m_owner.initRequest(response);
		ret = MHD_queue_response(connection, m_newFile ? 201 : 204, response);
		cout << "\tResponding with " << (m_newFile ? 201 : 204) << endl;
		MHD_destroy_response(response);
		return ret;
	}

	return MHD_NO;
}

int PutRequest::_headerIterator(void* cls, enum MHD_ValueKind kind, const char* key, const char* value)
{
	PutRequest* self = (PutRequest*) cls;
	return self->headerIterator(kind, key, value);
}

int PutRequest::headerIterator(enum MHD_ValueKind kind, const char* key, const char* value)
{
	if (strcasecmp(key, "content-encoding") == 0)
	{
		m_status = 501;
		return MHD_NO;
	}

	// todo check for content-range
	if (strcasecmp(key, "content-range") == 0)
	{
		return MHD_NO;
	}

	if (strncasecmp(key, "content-", 11) == 0 && strcasecmp(key, "content-language") != 0 && strcasecmp(key, "content-location") != 0)
	{
		// content-language and  content-location can safely be ignored
		// all other content- headers should end up in a 501
		m_status = 501;
		return MHD_NO;
	}
	return MHD_YES;
}
