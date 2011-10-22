#ifndef _GETREQUEST_H
#define _GETREQUEST_H

#include <string>
#include <vector>
#include "request.h"

class GetRequest : public Request
{
public:
	GetRequest(WebdavServer& owner, bool headersOnly=false);
	~GetRequest();

	int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize);

	void parseRanges(const std::string& ranges);
private:
	static const std::string s_error;
	FILE* m_handle;
	bool m_headersOnly;

	static int _contentReader(void *cls, uint64_t pos, char *buf, int max);
	int contentReader(uint64_t pos, char* buf, int max);

	class Range_t
	{
	public:
		Range_t(const std::string& first, const std::string& second);

		unsigned long long start;
		unsigned long long end;

		bool startValid;
		bool endValid;
	};

	std::vector<Range_t> m_ranges;

	void getIndexHTML(const std::string& url, std::string& html);
};

#endif
