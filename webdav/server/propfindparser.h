#ifndef _PROPFIND_PARSER_H
#define _PROPFIND_PARSER_H

#include "../expat/expat.h"
#include <string>
#include <vector>

class PropfindParser
{
public:
	PropfindParser();
	~PropfindParser();

	class prop_t
	{
	public:
		prop_t(const std::string& _name, const std::string& _xmlns);

		std::string name;
		std::string xmlns;

		bool operator<(const prop_t& second) const;
	};

	bool parse(const char* data, int length);
	bool finish();

	const std::vector<prop_t>& getProps() const;
	const std::string& getPropString() const;

private:
	XML_Parser m_parser;

	static void _startElement(void* userData, const XML_Char* name, const XML_Char** atts);
	void startElement(const XML_Char* name, const XML_Char** atts);
	static void _endElement(void* userData, const XML_Char* name);
	void endElement(const XML_Char* name);

	int m_depth;
	std::string m_propString;
	std::vector<prop_t> m_props;
};

#endif
