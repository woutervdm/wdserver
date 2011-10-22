#ifndef _PROPFIND_REQUEST_H
#define _PROPFIND_REQUEST_H

#include "request.h"
#include <map>
#include <vector>

class PropfindParser;

class PropfindRequest : public Request
{
public:
	PropfindRequest(WebdavServer& owner);
	~PropfindRequest();

	int handleRequest(struct MHD_Connection* connection, const char* url, const char* version, const char* uploadData, size_t* uploadDataSize);

private:
	void addResponse(std::string& xml, std::string path, int depth);

	PropfindParser* m_parser;
	int m_depth;
	bool m_parsedSuccessfully;

	class prop_t
	{
	public:
		prop_t(const std::string& _name, const std::string& _value, const std::string& _ns);

		std::string name;
		std::string value;
		std::string xmlns;
	};
	typedef std::vector<prop_t> props_t;
	void getProperties(const std::string& path, props_t& found, props_t& notFound);

	void hashNamespace(const std::string& ns);

	std::map<std::string, std::string> m_nsHash;
	std::string m_nsDefs;

};
#endif
