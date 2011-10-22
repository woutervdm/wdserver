#ifndef _PROPPATCH_PARSER_H
#define _PROPPATCH_PARSER_H

#include "../expat/expat.h"
#include <string>
#include <vector>

class ProppatchParser
{
public:
	ProppatchParser();
	~ProppatchParser();

	class prop_t
	{
	public:
		prop_t(const std::string& _name, const std::string& _xmlns, bool _remove);

		std::string name;
		std::string xmlns;
		std::string value;
		bool remove;

		bool operator<(const prop_t& second) const;
	};

	bool parse(const char* data, int length);
	bool finish();

	const std::vector<prop_t>& getProps() const;

private:
	XML_Parser m_parser;

	static void _startElement(void* userData, const XML_Char* name, const XML_Char** atts);
	void startElement(const XML_Char* name, const XML_Char** atts);
	static void _endElement(void* userData, const XML_Char* name);
	void endElement(const XML_Char* name);
	static void _data(void* userData, const XML_Char* data, int length);
	void data(const XML_Char* data, int length);

	int m_depth;
	std::vector<prop_t> m_props;
	std::string m_mode;
};

#endif
